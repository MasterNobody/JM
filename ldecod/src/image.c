/*!
 *	\file
 *			Image.c
 *	\brief
 *			Decode a Slice
 *
 *	\author
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *
 *			Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *			Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *			Jani Lainema                    <jani.lainema@nokia.com>
 *			Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *			Byeong-Moon Jeon                <jeonbm@lge.com>
 *			Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 */

#include "contributors.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "elements.h"
#include "image.h"


/*!
 *	\fn		decode_slice()
 *	\brief	decode one slice
 */
int decode_slice(
	struct img_par *img,	/*!< pointer to image parameters */
	struct inp_par *inp,	/*!< pointer to configuration parameters read from the config file */
	int mb_nr,				/*!< macroblock number from where to start decoding */
	FILE *p_out)
{
	int len,info;
	int scale_factor;
	int i,j;
	int mb_width, stop_mb;
	static int first_P = TRUE;

	if (mb_nr < 0)
	{
		stop_mb = mb_nr;
		mb_nr = 0;
	}

	if(mb_nr == 0)
	{
#if TRACE
		//printf ("DecodeSlice, found mb_nr == 0, start decoding picture header\n");
#endif

		get_symbol("Image Type",&len, &info, SE_PTYPE);
		img->type = (int) pow(2,(len/2))+info-1;        /* find image type INTRA/INTER*/

		if(img->type==INTRA_IMG) // I picture
			prevP_tr=img->tr;
		else if(img->type == INTER_IMG_1 || img->type == INTER_IMG_MULT)  // P pictures
			nextP_tr=img->tr;
				
		if(img->type == INTER_IMG_1 || img->type == INTER_IMG_MULT)
		{
			if(first_P) // first P picture
			{
				first_P = FALSE;
				P_interval=nextP_tr-prevP_tr;
			}
			else // all other P pictures
			{
				write_prev_Pframe(img, p_out);	// imgY_prev, imgUV_prev -> file
				// oneforthpix_2(img);      // mref_P, mcef_p -> mref[][][], mcef[][][][]
				copy2mref_2(img);
			}
		}

		if (img->type > B_IMG_MULT)
		{
			set_ec_flag(SE_PTYPE);
			img->type = INTER_IMG_1;	/* concealed element */
		}

		if (img->format == QCIF)
			scale_factor=1;                             /* QCIF */
		else
			scale_factor=2;                             /* CIF  */

		img->width      = IMG_WIDTH     * scale_factor; /* luma   */
		img->height     = IMG_HEIGHT    * scale_factor;

		img->width_cr   = IMG_WIDTH_CR  * scale_factor; /* chroma */
		img->height_cr  = IMG_HEIGHT_CR * scale_factor;

		for(i=0;i<img->width/BLOCK_SIZE+1;i++)          /* set edge to -1, indicate nothing to predict from */
			img->ipredmode[i+1][0]=-1;
		for(j=0;j<img->height/BLOCK_SIZE+1;j++)
			img->ipredmode[0][j+1]=-1;

		// I or P or B pictures
		{
			for (i=0;i<90;i++)
			{
				for (j=0;j<74;j++)
				{
					loopb[i+1][j+1]=0;
				}
			}
			for (i=0;i<46;i++)
			{
				for (j=0;j<38;j++)
				{
					loopc[i+1][j+1]=0;
				}
			}
		}
	}

	mb_width = img->width/16;
	img->mb_x = mb_nr%mb_width;
	img->mb_y = mb_nr/mb_width;

	// I or P pictures
	if(img->type <= INTRA_IMG)
	{
		for (; img->mb_y < img->height/MB_BLOCK_SIZE;img->mb_y++)
		{
			img->block_y = img->mb_y * BLOCK_SIZE;        /* vertical luma block position         */
			img->pix_c_y=img->mb_y *MB_BLOCK_SIZE/2;      /* vertical chroma macroblock position  */
			img->pix_y=img->mb_y *MB_BLOCK_SIZE;          /* vertical luma macroblock position    */

			for (; img->mb_x < img->width/MB_BLOCK_SIZE;img->mb_x++)
			{
				img->block_x=img->mb_x*BLOCK_SIZE;        /* luma block  */
				img->pix_c_x=img->mb_x*MB_BLOCK_SIZE/2;
				img->pix_x=img->mb_x *MB_BLOCK_SIZE;      /* horizontal luma macroblock position */

				if(nal_startcode_follows() || (stop_mb++ == 0))
					return DECODING_OK;

				img->current_mb_nr = img->mb_y*img->width/MB_BLOCK_SIZE+img->mb_x;
				slice_numbers[img->current_mb_nr] = img->current_slice_nr;

#if TRACE
				fprintf(p_trace, "\n*********** Pic: %i (I/P) MB: %i Slice: %i **********\n\n", img->tr, img->current_mb_nr, slice_numbers[img->current_mb_nr]);
#endif
				if(macroblock(img)==SEARCH_SYNC)          /* decode one frame */
					return SEARCH_SYNC;                   /* error in bitstream, look for synk word next frame */
			}
			img->mb_x = 0;
		}
		loopfilter(img);
		if(img->number == 0)
		{
			// oneforthpix(img);
			copy2mref(img);
		}
		else
		{
			// oneforthpix_1(img);
			copy2mref_1(img);
		}
	}
	else	// B pictures
	{
		Bframe_ctr++;
		initialize_Bframe();
		decode_Bframe(img, mb_nr, stop_mb);
        //g.b. post filter like loop filter
        loopfilter(img);
	}

	return PICTURE_DECODED;
}

/************************************************************************
*
*  Name :       oneforthpix()
*
*  Description: Upsample 4 times and store in buffer for multiple
*               reference frames
*
*  Remark:
*      Thomas Wedi: Not used any more. The pels are directly interpolated
*					in the compensation module.
*
************************************************************************/
void oneforthpix(struct img_par *img)
{
	int imgY_tmp[576][704];
	int i,j,i2,j2;
	int is,ie2,je2;
	int uv;

	img->frame_cycle=img->number % MAX_MULT_PRED;

	for (j=0; j < img->height; j++)
	{
		for (i=0; i < img->width; i++)
		{
			i2=i*2;
			is=(ONE_FOURTH_TAP[0][0]*(imgY[j][i         ]+imgY[j][min(img->width-1,i+1)])+
			    ONE_FOURTH_TAP[1][0]*(imgY[j][max(0,i-1)]+imgY[j][min(img->width-1,i+2)])+
			    ONE_FOURTH_TAP[2][0]*(imgY[j][max(0,i-2)]+imgY[j][min(img->width-1,i+3)])+16)/32;
			imgY_tmp[j][i2  ]=imgY[j][i];             /* 1/1 pix pos */
			imgY_tmp[j][i2+1]=max(0,min(255,is));     /* 1/2 pix pos */

		}
	}
	for (i=0; i < (img->width-1)*2+1; i++)
	{
		for (j=0; j < img->height; j++)
		{
			j2=j*4;
			/* change for TML4, use 6 TAP vertical filter */
			is=(ONE_FOURTH_TAP[0][0]*(imgY_tmp[j         ][i]+imgY_tmp[min(img->height-1,j+1)][i])+
			    ONE_FOURTH_TAP[1][0]*(imgY_tmp[max(0,j-1)][i]+imgY_tmp[min(img->height-1,j+2)][i])+
			    ONE_FOURTH_TAP[2][0]*(imgY_tmp[max(0,j-2)][i]+imgY_tmp[min(img->height-1,j+3)][i])+16)/32;

			mref[img->frame_cycle][j2  ][i*2]=imgY_tmp[j][i];     /* 1/2 pix */
			mref[img->frame_cycle][j2+2][i*2]=max(0,min(255,is)); /* 1/2 pix */
		}
	}

	/* 1/4 pix */
	/* luma */
	ie2=(img->width-1)*4;
	je2=(img->height-1)*4;
	for (j=0;j<je2+1;j+=2)
		for (i=0;i<ie2;i+=2)
			mref[img->frame_cycle][j][i+1]=(mref[img->frame_cycle][j][i]+mref[img->frame_cycle][j][min(ie2,i+2)])/2;


	for (i=0;i<ie2+1;i++)
	{
		for (j=0;j<je2;j+=2)
		{
			mref[img->frame_cycle][j+1][i]=(mref[img->frame_cycle][j][i]+mref[img->frame_cycle][min(je2,j+2)][i])/2;

			/* "funny posision" */
			if( ((i&3) == 3)&&(((j+1)&3) == 3))
			{
				mref[img->frame_cycle][j+1][i] = (mref[img->frame_cycle][j-2][i-3]+
				                                  mref[img->frame_cycle][j+2][i-3]+
				                                  mref[img->frame_cycle][j-2][i+1]+
				                                  mref[img->frame_cycle][j+2][i+1]+2)/4;
			}
		}
	}

	/*  Chroma: */
	for (uv=0; uv < 2; uv++)
	{
		for (i=0; i < img->width_cr; i++)
		{
			for (j=0; j < img->height_cr; j++)
			{
				mcef[img->frame_cycle][uv][j][i]=imgUV[uv][j][i];/* just copy 1/1 pix, interpolate "online" */
			}
		}
	}
}

/************************************************************************
*
*  Name :       copy2mref()
*
*  Description: store reconstructed frames in multiple
*               reference buffer
*
************************************************************************/
void copy2mref(struct img_par *img)
{
  int i,j,uv;
  
  img->frame_cycle=img->number % MAX_MULT_PRED;
  for (j=0; j < img->height; j++)
	for (i=0; i < img->width; i++)
	   mref[img->frame_cycle][j][i]=imgY[j][i];

  for (uv=0; uv < 2; uv++)
	for (i=0; i < img->width_cr; i++)
	  for (j=0; j < img->height_cr; j++)
		mcef[img->frame_cycle][uv][j][i]=imgUV[uv][j][i];/* just copy 1/1 pix, interpolate "online" */

}  



/*!
 *	\fn		find_snr()
 *	\brief	Find PSNR for all three components.Compare decoded frame with
 *          the original sequence. Read inp->jumpd frames to reflect frame skipping.
 */
void find_snr(
	struct snr_par *snr,	/*!< pointer to snr parameters */
	struct img_par *img,	/*!< pointer to image parameters */
	FILE *p_ref,			/*!< filestream to reference YUV file */
	int postfilter)			/*!< postfilterin on (=1) or off (=1) */
{
	int i,j;
	int diff_y,diff_u,diff_v;
	int uv;
	byte imgY_ref[288][352];
	byte imgUV_ref[2][144][176];
	int  status;
	byte diff;

	if(img->type<=INTRA_IMG) // I, P pictures
		frame_no=img->number*P_interval;
	else // B pictures
	{
		diff=nextP_tr-img->tr;
		frame_no=(img->number-1)*P_interval-diff;
	}

	rewind(p_ref);
	status = fseek (p_ref, frame_no*img->height*img->width*3/2, 0);
	if (status != 0) {
		printf(" Error in seeking img->tr: %d\n", img->tr);
		exit (-1);
	}    
	for (j=0; j < img->height; j++) 
		for (i=0; i < img->width; i++) 
			imgY_ref[j][i]=fgetc(p_ref);         
	for (uv=0; uv < 2; uv++) 
		for (j=0; j < img->height_cr ; j++) 
			for (i=0; i < img->width_cr; i++) 
				imgUV_ref[uv][j][i]=fgetc(p_ref);  

	img->quad[0]=0;
	diff_y=0;
	for (i=0; i < img->width; ++i)
	{
		for (j=0; j < img->height; ++j)
		{
			diff_y += img->quad[abs(imgY[j][i]-imgY_ref[j][i])];
		}
	}

	/*  Chroma */
	diff_u=0;
	diff_v=0;

	for (i=0; i < img->width_cr; ++i)
	{
		for (j=0; j < img->height_cr; ++j)
		{
			diff_u += img->quad[abs(imgUV_ref[0][j][i]-imgUV[0][j][i])];
			diff_v += img->quad[abs(imgUV_ref[1][j][i]-imgUV[1][j][i])];
		}
	}

	/*  Collecting SNR statistics */
	if (diff_y != 0)
	{
		snr->snr_y=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)diff_y));        /* luma snr for current frame */
	}

	if (diff_u != 0)
	{
		snr->snr_u=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_u)));    /*  chroma snr for current frame  */
		snr->snr_v=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_v)));    /*  chroma snr for current frame  */
	}

	if (img->number == 0) /* first */
	{
		snr->snr_y1=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)diff_y));       /* keep luma snr for first frame */
		snr->snr_u1=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_u)));   /* keep chroma snr for first frame */
		snr->snr_v1=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_v)));   /* keep chroma snr for first frame */
		snr->snr_ya=snr->snr_y1;
		snr->snr_ua=snr->snr_u1;
		snr->snr_va=snr->snr_v1;
	}
	else
	{
		if(img->type <= INTRA_IMG) // P pictures
		{
			snr->snr_ya=(float)(snr->snr_ya*(img->number+Bframe_ctr)+snr->snr_y)/(img->number+Bframe_ctr+1);      /* average snr chroma for all frames except first */
			snr->snr_ua=(float)(snr->snr_ua*(img->number+Bframe_ctr)+snr->snr_u)/(img->number+Bframe_ctr+1);      /* average snr luma for all frames except first  */
			snr->snr_va=(float)(snr->snr_va*(img->number+Bframe_ctr)+snr->snr_v)/(img->number+Bframe_ctr+1);      /* average snr luma for all frames except first  */
		}
		else // B pictures
		{
			snr->snr_ya=(float)(snr->snr_ya*(img->number+Bframe_ctr-1)+snr->snr_y)/(img->number+Bframe_ctr);      /* average snr chroma for all frames except first */
			snr->snr_ua=(float)(snr->snr_ua*(img->number+Bframe_ctr-1)+snr->snr_u)/(img->number+Bframe_ctr);      /* average snr luma for all frames except first  */
			snr->snr_va=(float)(snr->snr_va*(img->number+Bframe_ctr-1)+snr->snr_v)/(img->number+Bframe_ctr);      /* average snr luma for all frames except first  */
		}
	}
}


/*!
 *	\fn		loopfilter()
 *	\brief	Filter to reduce blocking artifacts. The filter strengh
 *          is QP dependent.
 */

static unsigned char
  overallActivity[256], qp_overallActivity = 255;
static int
  beta;

void loopfilter(struct img_par *img)
{
  static int MAP[32];
  byte imgY_tmp[288][352];                      /* temp luma image */
  byte imgUV_tmp[2][144][176];                  /* temp chroma image */

  int x,y,y4,x4,k,uv,str1,str2;

  if (qp_overallActivity != img->qp)
  {
    int
      i, alpha,
      ALPHA_TABLE[32] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        3,  3,  3,  7,  7,  7, 12, 17,
       17, 22, 28, 34, 40, 47, 60, 67,
       82, 89,112,137,153,187,213,249
      },
      BETA_TABLE[32] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        3,  3,  3,  4,  4,  4,  6,  6,
        6,  7,  8,  8,  9,  9, 10, 10,
       11, 11, 12, 12, 13, 13, 14, 14
      };

    for (i=0; i<32; i++)
      MAP[i] = QP2QUANT[i]-1;

    /*
     * ALPHA(img->qp) = ceil(2.0 * MAP(img->qp) * ln(MAP(img->qp)))
     */
    alpha = ALPHA_TABLE[img->qp];

    /*
     * BETA(img->qp) = floor(4.0 * log(MAP[img->qp]) + 0.5)
     */
    beta = BETA_TABLE[img->qp];

    for (k=0; k<256; k++)
      if (2*k >= alpha+(alpha/2))
        overallActivity[k] = 1;
      else if (2*k >= alpha)
        overallActivity[k] = 2;
      else
        overallActivity[k] = 3;

    qp_overallActivity = img->qp;

  }

  /* Luma hor */ 
  for(y=0;y<img->height;y++)
    for(x=0;x<img->width;x++)
      imgY_tmp[y][x]=imgY[y][x];

  for(y=0;y<img->height;y++)
  {
    y4=y/4;
    for(x=4;x<img->width;x=x+4)
    {
      x4=x/4;

      str1 = loopb[x4][y4+1];
      str2 = loopb[x4+1][y4+1];

      if((str1!=0)&&(str2!=0))
      {
        /*
         * Copy one line of 4+4 reconstruction pels into the filtering buffer.
         */
        for(k=0;k<8;k++)
          img->li[k]=imgY_tmp[y][x-4+k];

        /*
         * Check for longer filtering on macroblock border if either block is intracoded.
         */
        if(!(x4%4) && (str1 == 3 || str2 == 3))
        {
          if(loop(img,str1,str2, 1, 0))
          {
            imgY_tmp[y][x-4+6] = img->lu[6];
            imgY[y][x-3]       = img->lu[1];
            imgY[y][x+2]       = img->lu[6];
          }
        }
        else
          loop(img,str1,str2, 0, 0);

        /*
         * Place back the filtered values into the filtered reconstruction buffer.
         */
        for (k=2;k<6;k++)
          imgY[y][x-4+k]=img->lu[k];
      }
    }
  }

  /* Luma vert */ 
  for(y=0;y<img->height;y++)       
    for(x=0;x<img->width;x++)
      imgY_tmp[y][x]=imgY[y][x];

  for(x=0;x<img->width;x++)
  {
    x4=x/4;
    for(y=4;y<img->height;y=y+4)
    {
      y4=y/4;

      str1 = loopb[x4+1][y4];
      str2 = loopb[x4+1][y4+1];

      if((str1 != 0) && (str2 != 0))
      {
        /*
         * Copy one line of 4+4 reconstruction pels into the filtering buffer.
         */
        for(k=0;k<8;k++)
          img->li[k]=imgY_tmp[y-4+k][x];

        /*
         * Check for longer filtering on macroblock border if either block is intracoded.
         */
        if(!(y4%4) && (str1 == 3 || str2 == 3))
        {
          if(loop(img,str1,str2, 1, 0)){
            imgY_tmp[y-4+6][x] = img->lu[6];
            imgY[y-3][x]       = img->lu[1];
            imgY[y+2][x]       = img->lu[6];
          }
        }
        else
          loop(img,str1,str2, 0, 0);

        /*
         * Place back the filtered values into the filtered reconstruction buffer.
         */
        for (k=2;k<6;k++)
          imgY[y-4+k][x]=img->lu[k];
      }
    }
  }

  /* chroma */

  for (uv=0;uv<2;uv++)
  {
    /* horizontal chroma */
    for(y=0;y<img->height_cr;y++)
      for(x=0;x<img->width_cr;x++)
        imgUV_tmp[uv][y][x]=imgUV[uv][y][x];
    
    for(y=0;y<img->height_cr;y++)
    {
      y4=y/4;
      for(x=4;x<img->width_cr;x=x+4)
      {
        x4=x/4;

        str1 = loopc[x4][y4+1];
        str2 = loopc[x4+1][y4+1];

        if((str1 != 0) && (str2 != 0))
        {
          /*
           * Copy one line of 4+4 reconstruction pels into the filtering buffer.
           */
          for(k=0;k<8;k++)
            img->li[k]=imgUV_tmp[uv][y][x-4+k];

          /*
           * Check for longer filtering on macroblock border if either block is intracoded.
           */
          if(!(x4%2) && (loopb[2*x4][2*y4+1] == 3 || loopb[2*x4+1][2*y4+1] == 3))
            loop(img,str1,str2, 1, 1);
          else
            loop(img,str1,str2, 0, 1);

          /*
           * Place back the filtered values into the filtered reconstruction buffer.
           */
          for (k=2;k<6;k++)
            imgUV[uv][y][x-4+k]=img->lu[k];
        }
      }
    }
    
    /* vertical chroma */
    for(y=0;y<img->height_cr;y++)       
      for(x=0;x<img->width_cr;x++)
        imgUV_tmp[uv][y][x]=imgUV[uv][y][x];      
    
    for(x=0;x<img->width_cr;x++)
    {
      x4=x/4;
      for(y=4;y<img->height_cr;y=y+4)
      {
        y4=y/4;

        str1 = loopc[x4+1][y4];
        str2 = loopc[x4+1][y4+1];

        if((str1 != 0) && (str2 != 0))
        {
          /*
           * Copy one line of 4+4 reconstruction pels into the filtering buffer.
           */
          for(k=0;k<8;k++)
            img->li[k]=imgUV_tmp[uv][y-4+k][x];

          /*
           * Check for longer filtering on macroblock border if either block is intracoded.
           */
          if(!(y4%2) && (loopb[2*x4+1][2*y4] == 3 || loopb[2*x4+1][2*y4+1] == 3))
            loop(img,str1,str2, 1, 1);
          else
            loop(img,str1,str2, 0, 1);

          /*
           * Place back the filtered values into the filtered reconstruction buffer.
           */
          for (k=2;k<6;k++)
            imgUV[uv][y-4+k][x]=img->lu[k];
        }
      }
    }
  }
}

/*!
 *	\fn		loop()
 *  \brief	Filter 4 or 6 from 8 pix input
 */

int loop(struct img_par *img, int ibl, int ibr, int longFilt, int chroma)
{
  int
    delta, halfLim, dr, dl, i, truncLimLeft, truncLimRight,
    diff, clip, clip_left, clip_right;

  /* Limit the difference between filtered and nonfiltered based
   * on QP and strength
   */
  clip_left  = FILTER_STR[img->qp][ibl];
  clip_right = FILTER_STR[img->qp][ibr];

  /* The step across the block boundaries */
  delta   = abs(img->li[3]-img->li[4]);

  /* Find overall activity parameter (n/2) */
  halfLim = overallActivity[delta];

  /* Truncate left limit to 2 for small stengths */
  if (ibl <= 1)
    truncLimLeft = (halfLim > 2) ? 2 : halfLim;
  else
    truncLimLeft = halfLim;

  /* Truncate right limit to 2 for small stengths */
  if (ibr <= 1)
    truncLimRight = (halfLim > 2) ? 2 : halfLim;
  else
    truncLimRight = halfLim;

  /* Find right activity parameter dr */
  for (dr=1; dr<truncLimRight; dr++)
  {
    if (dr*abs(img->li[4]-img->li[4+dr]) > beta)
      break;
  }

  /* Find left activity parameter dl */
  for (dl=1; dl<truncLimLeft; dl++)
  {
    if (dl*abs(img->li[3]-img->li[3-dl]) > beta)
      break;
  }

  if(dr < 2 || dl < 2)
  {
    /* no filtering when either one of activity params is below two */
    img->lu[2] = img->li[2];
    img->lu[3] = img->li[3];
    img->lu[4] = img->li[4];
    img->lu[5] = img->li[5];

    return 0;
  }

  if(longFilt)
  {
    if(dr    == 3 && dl    == 3 &&
       delta >= 2 && delta <  img->qp/4)
    {
      if(chroma)
      {
        img->lu[3] = (25*(img->li[1] + img->li[5]) + 26*(img->li[2] + img->li[3] + img->li[4]) + 64) >> 7;
        img->lu[2] = (25*(img->li[0] + img->li[4]) + 26*(img->li[1] + img->li[2] + img->lu[3]) + 64) >> 7;
        img->lu[4] = (25*(img->li[2] + img->li[6]) + 26*(img->li[3] + img->li[4] + img->li[5]) + 64) >> 7;
        img->lu[5] = (25*(img->li[3] + img->li[7]) + 26*(img->lu[4] + img->li[5] + img->li[6]) + 64) >> 7;

        return 0;
      }
      else
      {
        img->lu[3] = (25*(img->li[1] + img->li[5]) + 26*(img->li[2] + img->li[3] + img->li[4]) + 64) >> 7;
        img->lu[2] = (25*(img->li[0] + img->li[4]) + 26*(img->li[1] + img->li[2] + img->lu[3]) + 64) >> 7;
        img->lu[1] = (25*(img->li[1] + img->lu[3]) + 26*(img->li[0] + img->li[1] + img->lu[2]) + 64) >> 7;
        img->lu[4] = (25*(img->li[2] + img->li[6]) + 26*(img->li[3] + img->li[4] + img->li[5]) + 64) >> 7;
        img->lu[5] = (25*(img->li[3] + img->li[7]) + 26*(img->lu[4] + img->li[5] + img->li[6]) + 64) >> 7;
        img->lu[6] = (25*(img->lu[4] + img->li[6]) + 26*(img->lu[5] + img->li[6] + img->li[7]) + 64) >> 7;
        
        return 1;
      }
    }
  }

  /* Filter pixels at the edge */
  img->lu[4] = (21*(img->li[3] + img->li[5]) + 22*img->li[4] + 32) >> 6;
  img->lu[3] = (21*(img->li[2] + img->li[4]) + 22*img->li[3] + 32) >> 6;

  if(dr == 3)
    img->lu[5] = (21*(img->lu[4] + img->li[6]) + 22*img->li[5] + 32) >> 6;
  else
    img->lu[5] = img->li[5];

  if(dl == 3)
    img->lu[2] = (21*(img->li[1] + img->lu[3]) + 22*img->li[2] + 32) >> 6;
  else
    img->lu[2] = img->li[2];

  /* Clipping parameter depends on one table and left and right act params */
  clip = (clip_left + clip_right + dl + dr) / 2;

  /* Pixels at the edge are clipped differently */
  for (i=3; i<=4; i++)
  {
    diff = (int)img->lu[i] - (int)img->li[i];

    if (diff)
    {
      if (diff > clip)
        diff = clip;
      else if (diff < -clip)
        diff = -clip;

      img->lu[i] = img->li[i] + diff;
    }
  }

  /* pixel from left is clipped */
  diff = (int)img->lu[2] - (int)img->li[2];

  if (diff)
  {
    if (diff > clip_left)
      diff = clip_left;
    else if (diff < -clip_left)
      diff = -clip_left;

    img->lu[2] = img->li[2] + diff;
  }

  /* pixel from right is clipped */
  diff = (int)img->lu[5] - (int)img->li[5];

  if (diff)
  {
    if (diff > clip_right)
      diff = clip_right;
    else if (diff < -clip_right)
      diff = -clip_right;

    img->lu[5] = img->li[5] + diff;
  }

  return 0;
}

/************************************************************************
*
*  Name :       get_pixel()
*
*  Description: Direct interpolation of a specific subpel position
*
*  Author:      Thomas Wedi, 12.01.2001		<wedi@tnt.uni-hannover.de>
*
*  Remarks:     A further significant complexity reduction is possible 
*               if the direct interpolation is not performed on pixel basis
*               but on block basis,
*
************************************************************************/

byte get_pixel(int ref_frame,int x_pos, int y_pos, struct img_par *img)
{

  int dx=0, x=0;
  int dy=0, y=0;
  int maxold_x=0,maxold_y=0;

  int result=0;
  int pres_x=0;
  int pres_y=0; 
 
  /**********************/
  /*  applied filters   */
  /*                    */
  /*  X  2  3  2  X     */
  /*  2  6  7  6        */
  /*  3  7  5  7        */
  /*  2  6  7  6        */
  /*  X           X     */
  /*                    */
  /*  X-fullpel positon */
  /*                    */
  /**********************/

 dx = x_pos%4;
 dy = y_pos%4;
 maxold_x=img->width-1;
 maxold_y=img->height-1;

 if(dx==0&&dy==0) /* fullpel position */
   {
     pres_y=y_pos/4;
     pres_x=x_pos/4;
     result=mref[ref_frame][pres_y][pres_x];
     return result;
   }
 else if(dx==3&&dy==3) /* funny position */
   {
     pres_y=y_pos/4;
     pres_x=x_pos/4;
     result=(mref[ref_frame][pres_y                ][pres_x                ]+
			 mref[ref_frame][pres_y                ][min(maxold_x,pres_x+1)]+
			 mref[ref_frame][min(maxold_y,pres_y+1)][min(maxold_x,pres_x+1)]+
			 mref[ref_frame][min(maxold_y,pres_y+1)][pres_x                ]+2)/4;
     return result;
   }
 else /* other positions */
  {
    if(x_pos==img->width*4-1)
      dx=2;
    if(y_pos==img->height*4-1)
      dy=2;

    if(dx==1&&dy==0)
      {
	pres_y=y_pos/4;
	for(x=-2;x<4;x++)
	  {
	    pres_x=max(0,min(maxold_x,x_pos/4+x));
	    result+=mref[ref_frame][pres_y][pres_x]*two[x+2];
	  }
      }
    else if(dx==0&&dy==1)
      {
	pres_x=x_pos/4;
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    result+=mref[ref_frame][pres_y][pres_x]*two[y+2];
	  }
      }
    else if(dx==2&&dy==0)
      {
	pres_y=y_pos/4;
	for(x=-2;x<4;x++)
	  {
	    pres_x=max(0,min(maxold_x,x_pos/4+x));
	    result+=mref[ref_frame][pres_y][pres_x]*three[x+2];
	  }
      }
    else if(dx==0&&dy==2)
      {
	pres_x=x_pos/4;
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    result+=mref[ref_frame][pres_y][pres_x]*three[y+2];
	  }
      }
    else if(dx==3&&dy==0)
      {
	pres_y=y_pos/4;
	for(x=-2;x<4;x++)
	  {
	    pres_x=max(0,min(maxold_x,x_pos/4+x));
	    result+=mref[ref_frame][pres_y][pres_x]*two[3-x];   /* four */
	  }
      }
    else if(dx==0&&dy==3)
      {
	pres_x=x_pos/4;
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    result+=mref[ref_frame][pres_y][pres_x]*two[3-y];   /* four */
	  }
      }
    else if(dx==1&&dy==1)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*six[y+2][x+2];  
	      }
	  }
      }
    else if(dx==2&&dy==1)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*seven[y+2][x+2];
	      }
	  }
      }
    else if(dx==1&&dy==2)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*seven[x+2][y+2]; /* eight */
	      }
	  }
      }
    else if(dx==3&&dy==1)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*six[y+2][3-x];   /* nine */
	      }
	  }
      }
    else if(dx==1&&dy==3)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*six[3-y][x+2];   /* ten */
	      }
	  }
      }
    else if(dx==2&&dy==2)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*five[y+2][x+2];
	      }
	  }
      }
    else if(dx==3&&dy==2)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*seven[3-x][3-y]; /* eleven */
	      }
	  }
      }
    else if(dx==2&&dy==3)
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*seven[3-y][x+2]; /* twelve */
	      }
	  }
      }
    else if(dx==3&&dy==3) /* not used if funny position is on */
      {
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    for(x=-2;x<4;x++)
	      {
		pres_x=max(0,min(maxold_x,x_pos/4+x));
		result+=mref[ref_frame][pres_y][pres_x]*six[3-y][3-x]; /* thirteen */
	      }
	  }
      }
    
  }
 
 
 return max(0,min(255,(result+2048)/4096));
 
}

