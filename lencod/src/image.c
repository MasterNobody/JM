// *************************************************************************************
// *************************************************************************************
// Image.c   Code one image/slice
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
// Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
// Jani Lainema                    <jani.lainema@nokia.com>
// Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
// Byeong-Moon Jeon                <jeonbm@lge.com>
// *************************************************************************************
// *************************************************************************************
#include "contributors.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <assert.h>

#include "global.h"
#include "elements.h"
#include "image.h"


static int ReadSourcePicture (FILE *in, struct img_par *img, int frame);

/************************************************************************
*
*  Name :       image()
*
*  Description: Encode one image
*
************************************************************************/
void image(struct img_par *img,struct inp_par *inp,struct stat_par *stat, struct snr_par *snr)
{
	int multpred;
	int len,info;
	int i,j;	

	/* B pictures */
	int B_interval, Bframe_to_code;
	int k;

	time_t ltime1;   // for time measurement 
	time_t ltime2;

#ifdef WIN32
	struct _timeb tstruct1;
	struct _timeb tstruct2;
#else
	struct tm *l_time;
	char string[20];
	struct timeb tstruct1;
	struct timeb tstruct2;
	time_t now;
#endif

	int tmp_time;

#ifdef WIN32
	_ftime( &tstruct1 );    // start time ms 
#else 
	ftime(&tstruct1);
#endif
	time( &ltime1 );        // start time s  
  
	img->current_mb_nr=0;
	img->current_slice_nr=0;
	stat->bit_slice = 0;

	if (img->number == 0)           /* first frame */
	{
		img->type = INTRA_IMG;        /* set image type for first image to intra */
		img->qp = inp->qp0;           /* set quant. parameter for first frame */
	}
	else                            /* inter frame */
	{
		img->type = INTER_IMG;

		img->qp = inp->qpN;

		if (inp->no_multpred <= 1)
			multpred=FALSE;
		else
			multpred=TRUE;               /* multiple reference frames in motion search */
	}
	
	img->tr=img->number*(inp->jumpd+1);
	if(img->number!=0 && inp->successive_Bframe != 0) 
		nextP_tr=img->tr;

	img->mb_y_intra=img->mb_y_upd;   /*  img->mb_y_intra indicates which GOB to intra code for this frame */

	if (inp->intra_upd > 0)          /* if error robustness, find next GOB to update */
	{
		img->mb_y_upd=(img->number/inp->intra_upd) % (img->width/MB_BLOCK_SIZE);
	}

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

	/* Bits for picture header and picture type */
 	len = nal_put_startcode(img->number*(inp->jumpd+1) % 256,
	                        img->current_mb_nr, img->qp, inp->img_format,
	                        img->current_slice_nr, SE_HEADER);

	stat->bit_ctr += len;                             /* keep bit usage */
	stat->bit_slice += len;
	stat->bit_use_head_mode[img->type] += len;

	if (img->type == INTRA_IMG)
	{
		len=3;
		info=1;
	}
	else if((img->type == INTER_IMG) && (multpred == FALSE)) /* inter single reference frame */
	{
		len=1;
		info=0;
	}
	else if((img->type == INTER_IMG) && (multpred == TRUE)) /* inter multipple reference frames */
	{
		len=3;
		info=0;
	}
	sprintf(tracestring, "Image type = %3d ", img->type);
	put_symbol(tracestring,len,info, img->current_slice_nr, SE_PTYPE);  /* write picture type to stream */

	stat->bit_ctr += len;
	stat->bit_slice += len;
	stat->bit_use_head_mode[img->type] += len;

	/* Read one new frame */

/* Old Telenor code, buggy, prevents source sequence looping */
/* also there was a missunderstanding of fseek() semantics: */
/* fseek() can seek beyond the EOF, and fgetc() returns then -1 */
/* hence the pink pictures after the end of the sequence */
/* fixed and optimized: Stephan Wenger, 11.03.2001 */

	frame_no = img->number*(inp->jumpd+1);

/*	
	rewind (p_in);
	status = fseek (p_in, frame_no*img->height*img->width*3/2, 0);
	if (status != 0) {
		printf(" Error in seeking frame no: %d\n", frame_no);
		exit (-1);
	}    
	for (j=0; j < img->height; j++) 
		for (i=0; i < img->width; i++) 
			imgY_org[j][i]=fgetc(p_in);         
	for (uv=0; uv < 2; uv++) 
		for (j=0; j < img->height_cr ; j++) 
			for (i=0; i < img->width_cr; i++) 
				imgUV_org[uv][j][i]=fgetc(p_in);  
*/
	
	if (ReadSourcePicture (p_in, img, frame_no)) {
		printf ("Error while reading picture %d\n", frame_no);
		exit (-1);
	}

   
	/* Intra and P pictures : Loops for coding of all macro blocks in a frame */
	for (img->mb_y=0; img->mb_y < img->height/MB_BLOCK_SIZE; ++img->mb_y)
	{
		img->block_y = img->mb_y * BLOCK_SIZE;  /* vertical luma block posision */
		img->pix_y   = img->mb_y * MB_BLOCK_SIZE; /* vertical luma macroblock posision */
		img->pix_c_y = img->mb_y * MB_BLOCK_SIZE/2;  /* vertical chroma macroblock posision */

		if (inp->intra_upd > 0 && img->mb_y <= img->mb_y_intra)
			img->height_err=(img->mb_y_intra*16)+15;     /* for extra intra MB */
		else
			img->height_err=img->height-1;

		if (img->type == INTER_IMG)
		{
			++stat->quant0;
			stat->quant1 += img->qp;      /* to find average quant for inter frames */
		}

		for (img->mb_x=0; img->mb_x < img->width/MB_BLOCK_SIZE; ++img->mb_x)
		{
			img->block_x = img->mb_x * BLOCK_SIZE;        /* luma block           */
			img->pix_x   = img->mb_x * MB_BLOCK_SIZE;     /* luma pixel           */
			img->block_c_x = img->mb_x * BLOCK_SIZE/2;    /* chroma block         */
			img->pix_c_x   = img->mb_x * MB_BLOCK_SIZE/2; /* chroma pixel         */

#if TRACE
			fprintf(p_trace, "\n*********** Pic: %i (I/P) MB: %i Slice: %i **********\n\n", frame_no, img->current_mb_nr, img->current_slice_nr);
#endif
			macroblock(img,inp,stat);                     /* code one macroblock  */
			img->current_mb_nr++;
		}
	}
	
 	loopfilter(img);          // reconstructed imgY, imgUV -> loop-filtered imgY, imgUV
	if(img->number == 0) 
		oneforthpix(img);				// loop-filtered imgY, imgUV -> mref[][][], mcef[][][][]
	else oneforthpix_1(img);  // loop-filtered imgY, imgUV -> mref_P, mcef_P
	find_snr(snr,img);     
    
	time(&ltime2);       // end time sec 
#ifdef WIN32
	_ftime(&tstruct2);   // end time ms  
#else 
	ftime(&tstruct2);    // end time ms  
#endif
	tmp_time=(ltime2*1000+tstruct2.millitm) - (ltime1*1000+tstruct1.millitm);
	tot_time=tot_time + tmp_time;

	// write reconstructed image (IPPP)
	if(inp->write_dec && inp->successive_Bframe==0)
	{
		for (i=0; i < img->height; i++) 
			for (j=0; j < img->width; j++) 
				fputc(min(imgY[i][j],255),p_dec);
      
		for (k=0; k < 2; ++k) 
			for (i=0; i < img->height/2; i++) 
				for (j=0; j < img->width/2; j++) 
					fputc(min(imgUV[k][i][j],255),p_dec);
	}

	// write reconstructed image (IBPBP) : only intra written 
	else if (inp->write_dec && img->number==0 && inp->successive_Bframe!=0)
	{
		for (i=0; i < img->height; i++) 
			for (j=0; j < img->width; j++) 
				fputc(min(imgY[i][j],255),p_dec);
      
		for (k=0; k < 2; ++k) 
			for (i=0; i < img->height/2; i++) 
				for (j=0; j < img->width/2; j++) 
					fputc(min(imgUV[k][i][j],255),p_dec);
	}

	// next P picture. This is saved with recon B picture after B picture coding
	if (inp->write_dec && img->number!=0 && inp->successive_Bframe!=0)           
	{
		for (i=0; i < img->height; i++) 
			for (j=0; j < img->width; j++) 
				nextP_imgY[i][j]=imgY[i][j];      
		for (k=0; k < 2; ++k) 
			for (i=0; i < img->height/2; i++) 
				for (j=0; j < img->width/2; j++) 
					nextP_imgUV[k][i][j]=imgUV[k][i][j];         
	}
	  
	if(img->number !=0) 
		stat->bit_ctr_P += stat->bit_ctr-stat->bit_ctr_n;  

	if(img->number == 0)
		printf("%2d(I) %8d %4d %7.4f %7.4f %7.4f  %5d \n",
				frame_no, stat->bit_ctr-stat->bit_ctr_n, 
				img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time); 
	else 
		printf("%2d(P) %8d %4d %7.4f %7.4f %7.4f  %5d \n",
			frame_no, stat->bit_ctr-stat->bit_ctr_n, 
			img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time); 

	if (img->number == 0) 
	{
		stat->bitr0=stat->bitr;           
		stat->bit_ctr_0=stat->bit_ctr;   
		stat->bit_ctr=0;                  
	}
	stat->bit_ctr_n=stat->bit_ctr; 	
	if(img->number == 0) return; 	

	/****************************************************
	 *                                                  *
	 *                   B pictures                     *
	 *                                                  *
	 ****************************************************/
	if(inp->successive_Bframe != 0) { 		
		if(inp->successive_Bframe > inp->jumpd) {
			printf("Warning !! Number of B-frames %d can not exceed the number of frames skipped\n",
					inp->successive_Bframe);
			exit(-1);
		}
		for(Bframe_to_code=1; Bframe_to_code<=inp->successive_Bframe; Bframe_to_code++) {				

#ifdef WIN32
			_ftime(&tstruct1);    // start time ms 
#else 
			ftime(&tstruct1);
#endif
			time(&ltime1);        // start time s  		

            //g.b. init post filter like loop filter
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

			Bframe_ctr++;
			initialize_Bframe(&B_interval, inp->successive_Bframe, Bframe_to_code, img, inp, stat);
			frame_no = (img->number-1)*(inp->jumpd+1)+B_interval*Bframe_to_code;
			ReadImage(frame_no, img);
			code_Bframe(img, inp, stat);
            //g.b. post filter like loop filter
            loopfilter(img);          // reconstructed imgY, imgUV -> loop-filtered imgY, imgUV
			writeimage(Bframe_to_code, inp->successive_Bframe, img, inp); 
			find_snr(snr,img);
						
			stat->bit_ctr_B += stat->bit_ctr-stat->bit_ctr_n;  			
			time(&ltime2);          // end time sec 
#ifdef WIN32
			_ftime(&tstruct2);      // end time ms  
#else 
			ftime(&tstruct2);       // end time ms  
#endif
			tmp_time=(ltime2*1000+tstruct2.millitm) - (ltime1*1000+tstruct1.millitm);
			tot_time=tot_time + tmp_time;
			
			printf("%2d(B) %8d %4d %7.4f %7.4f %7.4f  %5d \n",
					frame_no, stat->bit_ctr-stat->bit_ctr_n, img->qp, 
					snr->snr_y, snr->snr_u, snr->snr_v, tmp_time); 
			
			stat->bit_ctr_n=stat->bit_ctr;  			
		}	  
	} 

	oneforthpix_2(img);      // mref_P, mcef_p -> mref[][][], mcef[][][][]
}

/************************************************************************
*
*  Name :       oneforthpix()
*
*  Description: Upsample 4 times and store in buffer for multiple
*               reference frames
*
*  Changes: Thomas Wedi, 12.01.2001		<wedi@tnt.uni-hannover.de>
*
*	            Slightly changed to avoid mismatches with the direct 
*				interpolation filter in the decoder
*
************************************************************************/
void oneforthpix(struct img_par *img)
{
  // temporary solution:
  // int imgY_tmp[576][704];
  int is;
  int i,j,i2,j2,j4;
  int ie2,je2;
  int uv;
 
  img->frame_cycle=img->number % MAX_MULT_PRED;
  
  for (j=0; j < img->height; j++)
    {
      for (i=0; i < img->width; i++)
	{
	  i2=i*2;
	  is=(ONE_FOURTH_TAP[0][0]*(imgY[j][i         ]+imgY[j][min(img->width-1,i+1)])+
	      ONE_FOURTH_TAP[1][0]*(imgY[j][max(0,i-1)]+imgY[j][min(img->width-1,i+2)])+
	      ONE_FOURTH_TAP[2][0]*(imgY[j][max(0,i-2)]+imgY[j][min(img->width-1,i+3)]));
	  imgY_tmp[2*j][i2  ]=imgY[j][i]*1024;    /* 1/1 pix pos */
	  imgY_tmp[2*j][i2+1]=is*32;              /* 1/2 pix pos */
	}
    }
  
  for (i=0; i < img->width*2; i++)
    {
      for (j=0; j < img->height; j++)
	{
	  j2=j*2;
	  j4=j*4;
	  /* change for TML4, use 6 TAP vertical filter */
	  is=(ONE_FOURTH_TAP[0][0]*(imgY_tmp[j2         ][i]+imgY_tmp[min(2*img->height-2,j2+2)][i])+
	      ONE_FOURTH_TAP[1][0]*(imgY_tmp[max(0,j2-2)][i]+imgY_tmp[min(2*img->height-2,j2+4)][i])+
	      ONE_FOURTH_TAP[2][0]*(imgY_tmp[max(0,j2-4)][i]+imgY_tmp[min(2*img->height-2,j2+6)][i]))/32;
	  
	  imgY_tmp[j2+1][i]=is;                  /* 1/2 pix */
	  mref[img->frame_cycle][j4  ][i*2]=max(0,min(255,(int)((imgY_tmp[j2][i]+512)/1024)));     /* 1/2 pix */
	  mref[img->frame_cycle][j4+2][i*2]=max(0,min(255,(int)((is+512)/1024))); /* 1/2 pix */
	}
    }
  
  /* 1/4 pix */
  /* luma */
  ie2=(img->width-1)*4;
  je2=(img->height-1)*4;
  
  for (j=0;j<je2+4;j+=2)
    for (i=0;i<ie2+3;i+=2)
      mref[img->frame_cycle][j][i+1]=max(0,min(255,(int)(imgY_tmp[j/2][i/2]+imgY_tmp[j/2][min(ie2/2+1,i/2+1)]+1024)/2048));
  
  for (i=0;i<ie2+4;i++)
    {
      for (j=0;j<je2+3;j+=2)
	{
	  if( i%2 == 0 )
	    mref[img->frame_cycle][j+1][i]=max(0,min(255,(int)(imgY_tmp[j/2][i/2]+imgY_tmp[min(je2/2+1,j/2+1)][i/2]+1024)/2048));
	  else
	    mref[img->frame_cycle][j+1][i]=max(0,min(255,(int)(
							       imgY_tmp[j/2               ][i/2               ] +
							       imgY_tmp[min(je2/2+1,j/2+1)][i/2               ] +
							       imgY_tmp[j/2               ][min(ie2/2+1,i/2+1)] +
							       imgY_tmp[min(je2/2+1,j/2+1)][min(ie2/2+1,i/2+1)] + 2048)/4096));
	  
	  /* "funny posision" */
	  if( ((i&3) == 3)&&(((j+1)&3) == 3))
	    {
	      mref[img->frame_cycle][j+1][i] = (mref[img->frame_cycle][j-2         ][i-3         ] +
						mref[img->frame_cycle][min(je2,j+2)][i-3         ] +
						mref[img->frame_cycle][j-2         ][min(ie2,i+1)] +
						mref[img->frame_cycle][min(je2,j+2)][min(ie2,i+1)] + 2 )/4;
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
*  Name :       find_snr()
*
*  Description: Find SNR for all three components
*
************************************************************************/
void find_snr(struct snr_par *snr,struct img_par *img)
{
	int i,j;
	int diff_y,diff_u,diff_v;
	int impix;

	/*  Calculate  PSNR for Y, U and V. */

	/*     Luma. */
	impix = img->height*img->width;

	diff_y=0;
	for (i=0; i < img->width; ++i)
	{
		for (j=0; j < img->height; ++j)
		{
			diff_y += img->quad[abs(imgY_org[j][i]-imgY[j][i])];
		}
	}

	/*     Chroma. */

	diff_u=0;
	diff_v=0;

	for (i=0; i < img->width_cr; i++)
	{
		for (j=0; j < img->height_cr; j++)
		{
			diff_u += img->quad[abs(imgUV_org[0][j][i]-imgUV[0][j][i])];
			diff_v += img->quad[abs(imgUV_org[1][j][i]-imgUV[1][j][i])];
		}
	}

	/*  Collecting SNR statistics */
	if (diff_y != 0)
	{
		snr->snr_y=(float)(10*log10(65025*(float)impix/(float)diff_y));        /* luma snr for current frame */
		snr->snr_u=(float)(10*log10(65025*(float)impix/(float)(4*diff_u)));    /* u croma snr for current frame, 1/4 of luma samples*/
		snr->snr_v=(float)(10*log10(65025*(float)impix/(float)(4*diff_v)));    /* v croma snr for current frame, 1/4 of luma samples*/
	}

	if (img->number == 0)
	{
		snr->snr_y1=(float)(10*log10(65025*(float)impix/(float)diff_y));       /* keep luma snr for first frame */
		snr->snr_u1=(float)(10*log10(65025*(float)impix/(float)(4*diff_u)));   /* keep croma u snr for first frame */
		snr->snr_v1=(float)(10*log10(65025*(float)impix/(float)(4*diff_v)));   /* keep croma v snr for first frame */
		snr->snr_ya=snr->snr_y1;
		snr->snr_ua=snr->snr_u1;
		snr->snr_va=snr->snr_v1;
	}
	/* B pictures */
	else
	{
		snr->snr_ya=(float)(snr->snr_ya*(img->number+Bframe_ctr)+snr->snr_y)/(img->number+Bframe_ctr+1);   /* average snr lume for all frames inc. first */
		snr->snr_ua=(float)(snr->snr_ua*(img->number+Bframe_ctr)+snr->snr_u)/(img->number+Bframe_ctr+1);   /* average snr u croma for all frames inc. first  */
		snr->snr_va=(float)(snr->snr_va*(img->number+Bframe_ctr)+snr->snr_v)/(img->number+Bframe_ctr+1);   /* average snr v croma for all frames inc. first  */
	}
}

/************************************************************************
*
*  Name :       loopfilter()
*
*  Description: Filter to reduce blocking artifacts. The filter strengh 
*               is QP dependent.
*
************************************************************************/

static unsigned char
  overallActivity[256], qp_overallActivity = 255;
static int
  beta;

extern const int QP2QUANT[32];

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

/************************************************************************
*
*  Name :       loop()
*
*  Description: Filters up to 6 pixels from 8 pixel input
*
************************************************************************/
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

	
static int ReadSourcePicture (FILE *in, struct img_par *img, int frame) {

	static int FileSizeInPictures = -1;
	static unsigned char *ybuf, *uvbuf, *p;

	int ysize, uvsize, n, i, j, uv;
	long FileSizeInBytes; 

	// Determine the File size, but only once
	if (FileSizeInPictures < 0)	{	// first time
		
		if (0 != fseek (in, 0, SEEK_END)) {
			printf ("Problems in determining original sequence file size (fseek), exit\n");
			return -3;
		}
		FileSizeInBytes = ftell (in);
		n = FileSizeInBytes / img->height;
		n /= img->width;
		n *= 2;
		FileSizeInPictures = n / 3;
		if (FileSizeInPictures < 1)
			return -2;
	}

	ysize = img->height * img->width;
	uvsize = img->height * img->width / 2;

	if (NULL == (ybuf = malloc (ysize))) {
		printf ("Cannot malloc %d bytes for ybuf, exit\n", ysize);
		exit (-10);
	}
	if (NULL == (uvbuf = malloc (uvsize))) {
		printf ("Cannot malloc %d bytes for uvbuf, exit\n", uvsize);
		exit (-10);
	}

	frame %= FileSizeInPictures;	// allow sequence looping

	fseek (in, frame * (ysize + uvsize), SEEK_SET);
	if (1 != fread (ybuf, ysize, 1, in)) {
		printf ("Problems reading input sequence Y, frame %d, FileSizeInPictures %d\n", frame, FileSizeInPictures);
		getchar();
		return -1;
	}
	if (1 != fread (uvbuf, uvsize, 1, in)) {
		printf ("Problems reading input sequence Y, frame %d, FileSizeInPictures %d\n", frame, FileSizeInPictures);
		getchar();
		return -1;
	}
		
	// Now copy the data in whatever weird format is used in imgY_org and imgUV_org

	p = ybuf;
	for (j=0; j < img->height; j++) 
		for (i=0; i < img->width; i++) 
			imgY_org[j][i]=*p++;
	p = uvbuf;
	for (uv=0; uv < 2; uv++) 
		for (j=0; j < img->height_cr ; j++) 
			for (i=0; i < img->width_cr; i++) 
				imgUV_org[uv][j][i]=*p++;  


	free (ybuf);
	free (uvbuf);
	return 0;
}
