/****************************************************************
 *
 *  File B_frame.c :	B picture Decoding 
 *
 *	Main contributor and Contact Information
 *
 *      Byeong-Moon Jeon			<jeonbm@lge.com>
 *      Yoon-Seong Soh				<yunsung@lge.com>
 *      LG Electronics Inc., Digital Media Research Lab.
 *      16 Woomyeon-Dong, Seocho-Gu, Seoul, 137-724, Korea
 *
 *  Changes:
 *      Thomas Wedi					<wedi@tnt.uni-hannover.de>
 *
 *****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "global.h"
#include "B_frame.h"
#include "elements.h"

/*!
 *	\fn		write_prev_Pframe()
 *	\brief	Write previous decoded P frame to output file
 */

void write_prev_Pframe(struct img_par *img,	FILE *p_out)
{
	int i,j;

	for(i=0;i<img->height;i++)
		for(j=0;j<img->width;j++)
			fputc(imgY_prev[i][j],p_out);

	for(i=0;i<img->height_cr;i++)
		for(j=0;j<img->width_cr;j++)
			fputc(imgUV_prev[0][i][j],p_out);

	for(i=0;i<img->height_cr;i++)
		for(j=0;j<img->width_cr;j++)
			fputc(imgUV_prev[1][i][j],p_out);
}

/*!
 *	\fn		copy_Pframe()
 *	\brief	Copy decoded P frame to temporary image array
 */
void copy_Pframe(struct img_par *img,	int postfilter)
{
	int i,j;

	if(postfilter)
	{
		for(i=0;i<img->height;i++)
			for(j=0;j<img->width;j++)
				imgY_prev[i][j] = (byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgY_pf[i][j]));

		for(i=0;i<img->height_cr;i++)
			for(j=0;j<img->width_cr;j++)
				imgUV_prev[0][i][j] = (byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV_pf[0][i][j]));

		for(i=0;i<img->height_cr;i++)
			for(j=0;j<img->width_cr;j++)
				imgUV_prev[1][i][j] = (byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV_pf[1][i][j]));
	}
	else
	{
		for(i=0;i<img->height;i++)
			for(j=0;j<img->width;j++)
				imgY_prev[i][j] = (byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgY[i][j]));

		for(i=0;i<img->height_cr;i++)
			for(j=0;j<img->width_cr;j++)
				imgUV_prev[0][i][j] = (byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV[0][i][j]));

		for(i=0;i<img->height_cr;i++)
			for(j=0;j<img->width_cr;j++)
				imgUV_prev[1][i][j] = (byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV[1][i][j]));
	}
}

/************************************************************************
*  Name :    oneforthpix_1(), one_forthpix_2()
*
*  Description: 
*      Upsample 4 times, store them in mref_P, mcef_P for next P picture
*      and move them to mref, mcef
*
*  Remark:
*      Thomas Wedi: Not used any more. The pels are directly interpolated
*					in the compensation module.
*
************************************************************************/
void oneforthpix_1(struct img_par *img)
{
	int imgY_tmp[288][704];
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

			mref_P[j2  ][i*2]=imgY_tmp[j][i];     /* 1/2 pix */
			mref_P[j2+2][i*2]=max(0,min(255,is)); /* 1/2 pix */
		}
	}

	/* 1/4 pix */
	/* luma */
	ie2=(img->width-1)*4;
	je2=(img->height-1)*4;
	for (j=0;j<je2+1;j+=2)
		for (i=0;i<ie2;i+=2)
			mref_P[j][i+1]=(mref_P[j][i]+mref_P[j][min(ie2,i+2)])/2;

	for (i=0;i<ie2+1;i++)
	{
		for (j=0;j<je2;j+=2)
		{
			mref_P[j+1][i]=(mref_P[j][i]+mref_P[min(je2,j+2)][i])/2;

			/* "funny posision" */
			if( ((i&3) == 3)&&(((j+1)&3) == 3))
			{
				mref_P[j+1][i] = (mref_P[j-2][i-3]+mref_P[j+2][i-3]+mref_P[j-2][i+1]+mref_P[j+2][i+1]+2)/4;
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
				mcef_P[uv][j][i]=imgUV[uv][j][i]; /* just copy 1/1 pix, interpolate "online" */
			}
		}
	}
}

void oneforthpix_2(struct img_par *img)
{
	int i, j, uv;

	/* Luma */
	for (j=0; j < img->height*4; j++)
		for (i=0; i < img->width*4; i++)
			mref[img->frame_cycle][j][i] = mref_P[j][i];

	/*  Chroma: */
	for (uv=0; uv < 2; uv++)
		for (i=0; i < img->width_cr; i++)
			for (j=0; j < img->height_cr; j++)
				mcef[img->frame_cycle][uv][j][i]=mcef_P[uv][j][i];

}

/************************************************************************
*  Name :    copy2mref_1(), copy2mref_2()
*
*  Description: 
*		Substitutes functions oneforthpix_1 and oneforthpix_2. The whole
*		frame is not sampled up any more. The frames are stored at their
*       original size in the multiple reference buffer. The subpels
*		that are used are directily interpolated in the compensation module.
*
************************************************************************/

void copy2mref_1(struct img_par *img)
{
  int i,j, uv;
  
  img->frame_cycle=img->number % MAX_MULT_PRED;

  for (j=0; j < img->height; j++)
    for (i=0; i < img->width; i++)
	  mref_P_small[j][i]=imgY[j][i];

  for (uv=0; uv < 2; uv++)
	for (i=0; i < img->width_cr; i++)
	   for (j=0; j < img->height_cr; j++)
		   mcef_P[uv][j][i]=imgUV[uv][j][i];
}  

void copy2mref_2(struct img_par *img)
{
  int i,j, uv;
  
  for (j=0; j < img->height; j++)
    for (i=0; i < img->width; i++)
	  mref[img->frame_cycle][j][i]=imgY_prev[j][i];
 
  for (uv=0; uv < 2; uv++)
	for (i=0; i < img->width_cr; i++)
	  for (j=0; j < img->height_cr; j++)
		mcef[img->frame_cycle][uv][j][i]=mcef_P[uv][j][i];

}  

///////////////////////////////////////////////////////
// Initialize 
///////////////////////////////////////////////////////
void initialize_Bframe( )
{
	int i, j, k;
	
	// fw_refFrArr[72][88], bw_refFrArr[72][88] malloc
	fw_refFrArr=(int**)malloc(sizeof(int)*72);
	bw_refFrArr=(int**)malloc(sizeof(int)*72);
	for(i=0; i<72; i++) 
	{
		fw_refFrArr[i]=(int*)malloc(sizeof(int)*88); 
		bw_refFrArr[i]=(int*)malloc(sizeof(int)*88); 
	}
	// initialize
	for(i=0; i<72; i++)
		for(j=0; j<88; j++) 
		{
			fw_refFrArr[i][j]=bw_refFrArr[i][j]=-1;
		}

	// dfMV[92][72][2], dbMV[92][72][2] malloc
	dfMV=(int***)malloc(sizeof(int)*92);
	dbMV=(int***)malloc(sizeof(int)*92);
	for(i=0; i<92; i++) 
	{
		dfMV[i]=(int**)malloc(sizeof(int)*72); 
		dbMV[i]=(int**)malloc(sizeof(int)*72); 
	}
	for(i=0; i<92; i++)
		for(j=0; j<72; j++) 
		{
			dfMV[i][j]=(int*)malloc(sizeof(int)*2);			//[92][72][2];
			dbMV[i][j]=(int*)malloc(sizeof(int)*2);			//[92][72][2];
		}
	// initialize
	for(i=0; i<92; i++)
		for(j=0; j<72; j++) 
			for(k=0; k<2; k++)
			{
				dfMV[i][j][k]=0;
				dbMV[i][j][k]=0;
			}	
}

/***********************************************
 *    Decoding B pictures
 **********************************************/
void decode_Bframe(struct img_par *img, int mb_nr, int stop_mb)
{
	int mb_width;
	int i, j; 

	mb_width = img->width/16;
	img->mb_x = mb_nr%mb_width;
	img->mb_y = mb_nr/mb_width;

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
				return ;

			img->current_mb_nr = img->mb_y*img->width/MB_BLOCK_SIZE+img->mb_x;
			slice_numbers[img->current_mb_nr] = img->current_slice_nr;

#if TRACE
			fprintf(p_trace, "\n*********** Pic: %i(B) MB: %i Slice: %i **********\n\n", img->tr, img->current_mb_nr, slice_numbers[img->current_mb_nr]);
#endif
			if(mb_Bframe(img)==SEARCH_SYNC)          /* decode one frame */
				return ;                   /* error in bitstream, look for synk word next frame */
		}
		img->mb_x = 0;
	}
	

   // memory free
	for(i=0; i<92; i++)
	  for(j=0; j<72; j++)
		{
			free(dfMV[i][j]);
			free(dbMV[i][j]);
		}
		
	for(i=0; i<92; i++)
	{
		free(dfMV[i]);
		free(dbMV[i]);
	}
	free(dfMV);
	free(dbMV);

	for(i=0; i<72; i++)
	{
		free(fw_refFrArr[i]);
		free(bw_refFrArr[i]);
	}
	free(fw_refFrArr);
	free(bw_refFrArr);

}

/*!
 *	\fn		mb_Bframe()
 *
 *	\brief	Decode one macroblock of B frame
 *  \return SEARCH_SYNC : error in bitstream
 *          DECODING_OK : no error was detected
 */
int mb_Bframe(struct img_par *img)
{
	int js[2][2];
	int i,j,ii,jj,iii,jjj,i1,j1,ie,j4,i4,k,i0,pred_vec,j0;
	int js0,js1,js2,js3,jf,ifx;
	int coef_ctr,vec,cbp,uv,ll;
	int scan_loop_ctr;
	int level,run;
	int ref_frame;
	int len,info;
	int fw_predframe_no, bw_predframe_no;     /* frame number which current MB is predicted from */
	int block_x,block_y;
	int cbp_idx,dbl_ipred_word;
	int vec1_x,vec1_y,vec2_x,vec2_y, vec1_xx, vec1_yy;
	int scan_mode;
	int fw_blocktype, bw_blocktype, blocktype, fw_pred, bw_pred;

	const int ICBPTAB[6] = {0,16,32,15,31,47};
	int mod0;
	int kmod;
	int start_scan;
	int ioff,joff;
	int ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;
	int m2;
	int jg2;

    int i3,j3,mvDiffX,mvDiffY;

	//int interintra; /* distinction between Inter and Intra MBs */
	int se;			/* syntax element type */
	int hv;
	byte refP_tr, TRb, TRp;

	/* keep track of neighbouring macroblocks available for prediction */
	int mb_nr = img->current_mb_nr;
	int mb_width = img->width/16;
	int mb_available_up = (img->mb_y == 0) ? 0 : (slice_numbers[mb_nr] == slice_numbers[mb_nr-mb_width]);
	int mb_available_left = (img->mb_x == 0) ? 0 : (slice_numbers[mb_nr] == slice_numbers[mb_nr-1]);
	int mb_available_upleft  = (img->mb_x == 0 || img->mb_y == 0) ? 0 : (slice_numbers[mb_nr] == slice_numbers[mb_nr-mb_width-1]);
	int mb_available_upright = (img->mb_x >= mb_width-1 || img->mb_y == 0) ? 0 : (slice_numbers[mb_nr] == slice_numbers[mb_nr-mb_width+1]);

	/* keep track of neighbouring blocks available for motion vector prediction */
	int block_available_up, block_available_left, block_available_upright, block_available_upleft;
	int mv_a, mv_b, mv_c, mv_d;
	int mvPredType, rFrameL, rFrameU, rFrameUR;

	ResetSlicePredictions(img);

	get_symbol("MB mode",&len,&info,SE_BFRAME);
	img->mb_mode = (int)pow(2,(len/2))+info-1;   // Note -1 !! (len, info)=(3,1) => 2 (code_number=2)
		
	if (img->mb_mode>40) 
	{
		set_ec_flag(SE_MBTYPE);
		img->imod = B_Direct;
		img->mb_mode = 0;
	}
	else /* no error concealment necessary */
	{
		if (img->mb_mode == INTRA_MB_B) /* 4x4 intra */
			img->imod=INTRA_MB_OLD;
		if (img->mb_mode > INTRA_MB_B)  /* 16x16 intra */
		{
			img->imod=INTRA_MB_NEW;
			mod0=img->mb_mode - INTRA_MB_B -1;
			kmod=mod0 & 3;
			cbp = ICBPTAB[mod0/4];
		}
		if (img->mb_mode < INTRA_MB_B)  /* intra in inter frame */
			if(img->mb_mode == 0) 
			{
				img->imod=B_Direct;	
				for (i=0;i<BLOCK_SIZE;i++)
				{                           
					for(j=0;j<BLOCK_SIZE;j++)
					{
						img->fw_mv[img->block_x+i+4][img->block_y+j][0]=img->fw_mv[img->block_x+i+4][img->block_y+j][1]=0;
						img->bw_mv[img->block_x+i+4][img->block_y+j][0]=img->bw_mv[img->block_x+i+4][img->block_y+j][1]=0;
						img->ipredmode[img->block_x+i+1][img->block_y+j+1] = 0;
					}
				}
			}
			else if(img->mb_mode == 3) img->imod=B_Bidirect;
			else if(img->mb_mode==1 || (img->mb_mode>3 && img->mb_mode%2==0)) img->imod=B_Forward;
			else if(img->mb_mode==2 || (img->mb_mode>4 && img->mb_mode%2==1)) img->imod=B_Backward;
	}

	//img->fw_mv[img->block_x+4][img->block_y][2]=img->number;
	//img->bw_mv[img->block_x+4][img->block_y][2]=img->number;

	// reset vectors and pred. modes
	for (i=0;i<BLOCK_SIZE;i++)
	{                           
		for(j=0;j<BLOCK_SIZE;j++)
		{
			img->fw_mv[img->block_x+i+4][img->block_y+j][0]=img->fw_mv[img->block_x+i+4][img->block_y+j][1]=0;
			img->bw_mv[img->block_x+i+4][img->block_y+j][0]=img->bw_mv[img->block_x+i+4][img->block_y+j][1]=0;
			img->ipredmode[img->block_x+i+1][img->block_y+j+1] = 0;
		}
	}

	ref_frame = (img->frame_cycle-1+MAX_MULT_PRED)% MAX_MULT_PRED;
	fw_predframe_no=1;
	bw_predframe_no=0;

	// Set the reference frame information for motion vector prediction 
	if (img->imod==INTRA_MB_OLD || img->imod==INTRA_MB_NEW)
	{
		for (j = 0;j < BLOCK_SIZE;j++)
		{
			for (i = 0;i < BLOCK_SIZE;i++)
			{
				fw_refFrArr[img->block_y+j][img->block_x+i] = 
				bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
			}
		}
	}
	else if(img->imod == B_Forward)
	{
		for (j = 0;j < BLOCK_SIZE;j++)
		{
			for (i = 0;i < BLOCK_SIZE;i++)
			{
				fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no; // previous P
				bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
			}
		}
	}
	else if(img->imod == B_Backward)
	{
		for (j = 0;j < BLOCK_SIZE;j++)
		{
			for (i = 0;i < BLOCK_SIZE;i++)
			{
				fw_refFrArr[img->block_y+j][img->block_x+i] = -1;
				bw_refFrArr[img->block_y+j][img->block_x+i] = 0; // next P
			}
		}
	}
	else if(img->imod == B_Bidirect)
	{
		for (j = 0;j < BLOCK_SIZE;j++)
		{
			for (i = 0;i < BLOCK_SIZE;i++)
			{
				fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no; // previous P
				bw_refFrArr[img->block_y+j][img->block_x+i] = 0; // next P
			}
		}
	}

	///////////////////////////////////////////////////////////////
	// find two and two intra prediction mode for a macroblock
	///////////////////////////////////////////////////////////////
	if(img->imod == INTRA_MB_OLD)
	{
		for(i=0;i<MB_BLOCK_SIZE/2;i++)
		{
			get_symbol("Intra mode", &len,&info,SE_BFRAME);
			dbl_ipred_word = (int) pow(2,(len/2))+info-1;  // Note -1, (len, info)=(3,1) => 2
			if (dbl_ipred_word>35)
			{
				/*!
				 * \note
				 *	Error concealment for intra prediction mode, 
				 *	sets IntraPredictionMode to 0.
				 */
				set_ec_flag(SE_INTRAPREDMODE);
				get_concealed_element(&len, &info, SE_INTRAPREDMODE);
			}

			i1=img->block_x + 2*(i&0x01);
			j1=img->block_y + i/2;

			/* find intra prediction mode for two blocks */
			img->ipredmode[i1+1][j1+1] = PRED_IPRED[img->ipredmode[i1+1][j1]+1][img->ipredmode[i1][j1+1]+1][IPRED_ORDER[dbl_ipred_word][0]];
			img->ipredmode[i1+2][j1+1] = PRED_IPRED[img->ipredmode[i1+2][j1]+1][img->ipredmode[i1+1][j1+1]+1][IPRED_ORDER[dbl_ipred_word][1]];
		}
	}

	///////////////////////////////////////////////////////////////
	// find fw_predfame_no 
	///////////////////////////////////////////////////////////////
	if(img->imod==B_Forward || img->imod==B_Bidirect)
	{
		if(img->type==B_IMG_MULT)
		{
			get_symbol("Reference frame",&len,&info,SE_BFRAME);
			fw_predframe_no=(int)pow(2,len/2)+info;   // Note !! (len, info)=(3,1) => 3 (code_number=2)   
			if (fw_predframe_no > MAX_MULT_PRED)
			{
				set_ec_flag(SE_REFFRAME);
				fw_predframe_no = 1;
			}
			ref_frame=(img->frame_cycle+5-fw_predframe_no) % MAX_MULT_PRED;

			if (ref_frame > img->number)
			{
				ref_frame = 0;
			}

			/* Update the reference frame information for motion vector prediction */
			for (j = 0;j < BLOCK_SIZE;j++)
			{
				for (i = 0;i < BLOCK_SIZE;i++)
				{
					fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no;
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////
	// find blk_size
	///////////////////////////////////////////////////////////////
	if(img->imod==B_Bidirect)
	{
		get_symbol("FW blk_Size ",&len,&info,SE_BFRAME);
		fw_blocktype=(int)pow(2,len/2)+info;     // Note !! (len, info)=(3,1) => 3 (code_number=2)    
		if (len > 5)
		{
			set_ec_flag(SE_REFFRAME);
			fw_blocktype = 1;
		}

		get_symbol("BW blk_Size ",&len,&info,SE_BFRAME);
		bw_blocktype=(int)pow(2,len/2)+info;     // Note !! (len, info)=(3,1) => 3 (code_number=2)    
		if (len > 5)
		{
			set_ec_flag(SE_REFFRAME);
			bw_blocktype = 1;
		}
	}

	///////////////////////////////////////////////////////////////
	// find MVDFW
	///////////////////////////////////////////////////////////////
	if(img->imod==B_Forward || img->imod==B_Bidirect)
	{
		// forward : note realtion between blocktype and img->mb_mode
		// bidirect : after reading fw_blk_size, fw_pmv is obtained

		if(img->mb_mode==1) blocktype=1;
		else if(img->mb_mode>3) blocktype=img->mb_mode/2;
		else if(img->mb_mode==3) blocktype=fw_blocktype;
		
		ie=4-BLOCK_STEP[blocktype][0];
		for(j=0;j<4;j=j+BLOCK_STEP[blocktype][1])   /* Y position in 4-pel resolution */
		{
			block_available_up = mb_available_up || (j > 0);
			j4=img->block_y+j;
			for(i=0;i<4;i=i+BLOCK_STEP[blocktype][0]) /* X position in 4-pel resolution */
			{
				i4=img->block_x+i;
				block_available_left = mb_available_left || (i > 0);

				if (j > 0)
					block_available_upright = i != ie ? 1 : 0;
				else if (i != ie)
					block_available_upright = block_available_up;
				else
					block_available_upright = mb_available_upright;

				if (i > 0)
					block_available_upleft = j > 0 ? 1 : block_available_up;
				else if (j > 0)
					block_available_upleft = block_available_left;
				else
					block_available_upleft = mb_available_upleft;

				mvPredType = MVPRED_MEDIAN;

				rFrameL    = block_available_left    ? fw_refFrArr[j4][i4-1]   : -1;
				rFrameU    = block_available_up      ? fw_refFrArr[j4-1][i4]   : -1;
				rFrameUR   = block_available_upright ? fw_refFrArr[j4-1][i4+BLOCK_STEP[blocktype][0]] :
				             block_available_upleft  ? fw_refFrArr[j4-1][i4-1] : -1;

				/* Prediction if only one of the neighbors uses the selected reference frame */
				if(rFrameL == fw_predframe_no && rFrameU != fw_predframe_no && rFrameUR != fw_predframe_no)
					mvPredType = MVPRED_L;
				else if(rFrameL != fw_predframe_no && rFrameU == fw_predframe_no && rFrameUR != fw_predframe_no)
					mvPredType = MVPRED_U;
				else if(rFrameL != fw_predframe_no && rFrameU != fw_predframe_no && rFrameUR == fw_predframe_no)
					mvPredType = MVPRED_UR;

				/* Directional predictions */
				else if(blocktype == 3)
				{
					if(i == 0)
					{
						if(rFrameL == fw_predframe_no)
							mvPredType = MVPRED_L;
					}
					else
					{
						if(rFrameUR == fw_predframe_no)
							mvPredType = MVPRED_UR;
					}
				}
				else if(blocktype == 2)
				{
					if(j == 0)
					{
						if(rFrameU == fw_predframe_no)
							mvPredType = MVPRED_U;
					}
					else
					{
						if(rFrameL == fw_predframe_no)
							mvPredType = MVPRED_L;
					}
				}
				else if(blocktype == 5 && i == 2)
					mvPredType = MVPRED_L;
				else if(blocktype == 6 && j == 2)
					mvPredType = MVPRED_U;

				for (k=0;k<2;k++)                       /* vector prediction. find first koordinate set for pred */
				{
					mv_a = block_available_left ? img->fw_mv[i4-1+BLOCK_SIZE][j4][k] : 0;
					mv_b = block_available_up   ? img->fw_mv[i4+BLOCK_SIZE][j4-1][k]   : 0;
					mv_d = block_available_upleft  ? img->fw_mv[i4-1+BLOCK_SIZE][j4-1][k] : 0;
					mv_c = block_available_upright ? img->fw_mv[i4+BLOCK_STEP[blocktype][0]+BLOCK_SIZE][j4-1][k] : mv_d;

					switch (mvPredType)
					{
					case MVPRED_MEDIAN:
						if(!(block_available_upleft || block_available_up || block_available_upright))
							pred_vec = mv_a;
						else
							pred_vec =mv_a+mv_b+mv_c-min(mv_a,min(mv_b,mv_c))-max(mv_a,max(mv_b,mv_c));
						break;
					case MVPRED_L:
						pred_vec = mv_a;
						break;
					case MVPRED_U:
						pred_vec = mv_b;
						break;
					case MVPRED_UR:
						pred_vec = mv_c;
						break;
					default:
						break;
					}

					get_symbol("MVDFW",&len,&info, SE_BFRAME); //SE_MVD);
					if (len>17)
					{
						set_ec_flag(SE_MVD);
						get_concealed_element(&len, &info, SE_MVD);
					}
					vec=linfo_mvd(len,info)+pred_vec;           /* find motion vector */
					for(ii=0;ii<BLOCK_STEP[blocktype][0];ii++)
					{
						for(jj=0;jj<BLOCK_STEP[blocktype][1];jj++)
						{
							img->fw_mv[i4+ii+BLOCK_SIZE][j4+jj][k]=vec;
						}
					}
				}
			}
		}
	}

	///////////////////////////////////////////////////////////////
	// find MVDBW
	///////////////////////////////////////////////////////////////
	if(img->imod==B_Backward || img->imod==B_Bidirect)
	{
		// backward : note realtion between blocktype and img->mb_mode
		// bidirect : after reading bw_blk_size, bw_pmv is obtained
		
		if(img->mb_mode==2) blocktype=1;
		else if(img->mb_mode>4) blocktype=(img->mb_mode-1)/2;
		else if(img->mb_mode==3) blocktype=bw_blocktype;

		ie=4-BLOCK_STEP[blocktype][0];
		for(j=0;j<4;j=j+BLOCK_STEP[blocktype][1])   /* Y position in 4-pel resolution */
		{
			block_available_up = mb_available_up || (j > 0);
			j4=img->block_y+j;
			for(i=0;i<4;i=i+BLOCK_STEP[blocktype][0]) /* X position in 4-pel resolution */
			{
				i4=img->block_x+i;
				block_available_left = mb_available_left || (i > 0);

				if (j > 0)
					block_available_upright = i != ie ? 1 : 0;
				else if (i != ie)
					block_available_upright = block_available_up;
				else
					block_available_upright = mb_available_upright;

				if (i > 0)
					block_available_upleft = j > 0 ? 1 : block_available_up;
				else if (j > 0)
					block_available_upleft = block_available_left;
				else
					block_available_upleft = mb_available_upleft;

				mvPredType = MVPRED_MEDIAN;

				rFrameL    = block_available_left    ? bw_refFrArr[j4][i4-1]   : -1;
				rFrameU    = block_available_up      ? bw_refFrArr[j4-1][i4]   : -1;
				rFrameUR   = block_available_upright ? bw_refFrArr[j4-1][i4+BLOCK_STEP[blocktype][0]] :
				             block_available_upleft  ? bw_refFrArr[j4-1][i4-1] : -1;

				/* Prediction if only one of the neighbors uses the selected reference frame */
				if(rFrameL == bw_predframe_no && rFrameU != bw_predframe_no && rFrameUR != bw_predframe_no)
					mvPredType = MVPRED_L;
				else if(rFrameL != bw_predframe_no && rFrameU == bw_predframe_no && rFrameUR != bw_predframe_no)
					mvPredType = MVPRED_U;
				else if(rFrameL != bw_predframe_no && rFrameU != bw_predframe_no && rFrameUR == bw_predframe_no)
					mvPredType = MVPRED_UR;

				/* Directional predictions */
				else if(blocktype == 3)
				{
					if(i == 0)
					{
						if(rFrameL == bw_predframe_no)
							mvPredType = MVPRED_L;
					}
					else
					{
						if(rFrameUR == bw_predframe_no)
							mvPredType = MVPRED_UR;
					}
				}
				else if(blocktype == 2)
				{
					if(j == 0)
					{
						if(rFrameU == bw_predframe_no)
							mvPredType = MVPRED_U;
					}
					else
					{
						if(rFrameL == bw_predframe_no)
							mvPredType = MVPRED_L;
					}
				}
				else if(blocktype == 5 && i == 2)
					mvPredType = MVPRED_L;
				else if(blocktype == 6 && j == 2)
					mvPredType = MVPRED_U;

				for (k=0;k<2;k++)                       /* vector prediction. find first koordinate set for pred */
				{
					mv_a = block_available_left ? img->bw_mv[i4-1+BLOCK_SIZE][j4][k] : 0;
					mv_b = block_available_up   ? img->bw_mv[i4+BLOCK_SIZE][j4-1][k]   : 0;
					mv_d = block_available_upleft  ? img->bw_mv[i4-1+BLOCK_SIZE][j4-1][k] : 0;
					mv_c = block_available_upright ? img->bw_mv[i4+BLOCK_STEP[blocktype][0]+BLOCK_SIZE][j4-1][k] : mv_d;

					switch (mvPredType)
					{
					case MVPRED_MEDIAN:
						if(!(block_available_upleft || block_available_up || block_available_upright))
							pred_vec = mv_a;
						else
							pred_vec =mv_a+mv_b+mv_c-min(mv_a,min(mv_b,mv_c))-max(mv_a,max(mv_b,mv_c));
						break;
					case MVPRED_L:
						pred_vec = mv_a;
						break;
					case MVPRED_U:
						pred_vec = mv_b;
						break;
					case MVPRED_UR:
						pred_vec = mv_c;
						break;
					default:
						break;
					}

					get_symbol("MVDBW",&len,&info, SE_BFRAME); //SE_MVD);
					if (len>17)
					{
						set_ec_flag(SE_MVD);
						get_concealed_element(&len, &info, SE_MVD);
					}
					vec=linfo_mvd(len,info)+pred_vec;           /* find motion vector */
					for(ii=0;ii<BLOCK_STEP[blocktype][0];ii++)
					{
						for(jj=0;jj<BLOCK_STEP[blocktype][1];jj++)
						{
							img->bw_mv[i4+ii+BLOCK_SIZE][j4+jj][k]=vec;
						}
					}
				}
			}
		}
	}
          
	//interintra = inter_intra(img);	/* <pu> get information if MB is INTRA or INTER coded */

	////////////////////////////////////////////////
	// read CBP if not new intra mode 
	////////////////////////////////////////////////
	if (img->imod != INTRA_MB_NEW)
	{
		/* <pu> check for Inter or Intra CBP syntax element */
		//if ( interintra == INTRA_CODED_MB )
		//	se = SE_CBP_INTRA;
		//else
		//	se = SE_CBP_INTER;
		se = SE_BFRAME;
		get_symbol("CBP",&len,&info, SE_BFRAME); //SE_CBP_INTER);
		cbp_idx = (int) pow(2,(len/2))+info-1;      // Note -1!! (len, info)=(3,1) => 2 (code_number=2)   
		if (cbp_idx>47)                             /* illegal value, exit to find next sync word */
		{
			printf("Error in index n to array cbp\n");
			set_ec_flag(se);
			get_concealed_element(&len, &info, se);
		}
		if (img->imod == INTRA_MB_OLD) cbp=NCBP[cbp_idx][0];
		else cbp=NCBP[cbp_idx][1];
	}

	/* reset luma coeffs */
	for (i=0;i<BLOCK_SIZE;i++)
		for (j=0;j<BLOCK_SIZE;j++)
			for(iii=0;iii<BLOCK_SIZE;iii++)
				for(jjj=0;jjj<BLOCK_SIZE;jjj++)
					img->cof[i][j][iii][jjj]=0;

	////////////////////////////////////////////////////////
	// read DC coeffs for new intra modes 
	////////////////////////////////////////////////////////
	if(img->imod==INTRA_MB_NEW) 
	{
		for (i=0;i<BLOCK_SIZE;i++)
			for (j=0;j<BLOCK_SIZE;j++)
				img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;

		coef_ctr=-1;
		len = 0;                            /* just to get inside the loop */
		for(k=0;(k<17) && (len!=1);k++)
		{
			get_symbol("DC luma 16x16",&len,&info, SE_BFRAME); //SE_LUM_DC_INTRA);

			if(len>29)  /* illegal length, start_scan sync search */
			{
				set_ec_flag(SE_LUM_DC_INTRA);
				get_concealed_element(&len, &info, SE_LUM_DC_INTRA);
			}
			if (len != 1)                     /* leave if len=1 */
			{
				linfo_levrun_inter(len,info,&level,&run);
				coef_ctr=coef_ctr+run+1;

				i0=SNGL_SCAN[coef_ctr][0];
				j0=SNGL_SCAN[coef_ctr][1];

				if(coef_ctr > 15)
				{
					set_ec_flag(SE_LUM_DC_INTRA);
					get_concealed_element(&len, &info, SE_LUM_DC_INTRA);
				}
				img->cof[i0][j0][0][0]=level;/* add new intra DC coeff */
			}
		}
		itrans_2(img);/* transform new intra DC  */
	}

    //g.b. post filter like loop filter
    if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
    {
        for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;
				loopb[ii+1][jj+1]=3;
				loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],2);
				loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],2);
				loopb[ii+2][jj+1]=max(loopb[ii+2][jj+1],2);
				loopb[ii+1][jj+2]=max(loopb[ii+1][jj+2],2);

				loopc[i3+1][j3+1]=2;
				loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
				loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
				loopc[i3+2][j3+1]=max(loopc[i3+2][j3+1],1);
				loopc[i3+1][j3+2]=max(loopc[i3+1][j3+2],1);
			}
		}
    }
    if (img->imod==B_Forward || img->imod==B_Bidirect) /* inter one direction */
    {
        for (i=0;i<4;i++)
        {
            ii=img->block_x+i;
            i3=ii/2;
            for (j=0;j<4;j++)
            {
                jj=img->block_y+j;
                j3=jj/2;
        
                mvDiffX = img->fw_mv[ii+4][jj][0] - img->fw_mv[ii-1+4][jj][0];
                mvDiffY = img->fw_mv[ii+4][jj][1] - img->fw_mv[ii-1+4][jj][1];

                if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
                { 
                    loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
                    loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
                    loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
                    loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
                }

                if (jj >0)
                {
                    mvDiffX = img->fw_mv[ii+4][jj][0] - img->fw_mv[ii+4][jj-1][0];
                    mvDiffY = img->fw_mv[ii+4][jj][1] - img->fw_mv[ii+4][jj-1][1];

                    if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && jj > 0)
                    {
                        loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],1);
                        loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
                        loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
                        loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
                    }
                }
            }
        }
    }
    if(img->imod==B_Backward || img->imod==B_Bidirect) /* inter other direction */
    {
        for (i=0;i<4;i++)
        {
            ii=img->block_x+i;
            i3=ii/2;
            for (j=0;j<4;j++)
            {
                jj=img->block_y+j;
                j3=jj/2;
        
                mvDiffX = img->bw_mv[ii+4][jj][0] - img->bw_mv[ii-1+4][jj][0];
                mvDiffY = img->bw_mv[ii+4][jj][1] - img->bw_mv[ii-1+4][jj][1];

                if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
                { 
                    loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
                    loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
                    loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
                    loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);                    
                }
                    
                if (jj > 0)
                {
                    mvDiffX = img->bw_mv[ii+4][jj][0] - img->bw_mv[ii+4][jj-1][0];
                    mvDiffY = img->bw_mv[ii+4][jj][1] - img->bw_mv[ii+4][jj-1][1];

                    if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && jj > 0)
                    {
                        loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],1);
                        loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
                        loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
                        loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
                    }
                }
            }
        }
    }

	if (img->imod == INTRA_MB_OLD && img->qp < 24)
		scan_mode=DOUBLE_SCAN;
	else
		scan_mode=SINGLE_SCAN;

	/* luma coefficients */
	for (block_y=0; block_y < 4; block_y += 2) /* all modes */
	{
		for (block_x=0; block_x < 4; block_x += 2)
		{
			for (j=block_y; j < block_y+2; j++)
			{
				jj=j/2;
				for (i=block_x; i < block_x+2; i++)
				{
					ii=i/2;
					if (img->imod == INTRA_MB_NEW)
						start_scan = 1; /* skip DC coeff */
					else
						start_scan = 0; /* take all coeffs */

					if((cbp & (int)pow(2,(ii+2*jj))) != 0)  /* are there any coeff in current block at all */
					{
						if (scan_mode==SINGLE_SCAN)
						{
							coef_ctr=start_scan-1;
							len = 0;                            /* just to get inside the loop */
							for(k=start_scan;(k<17) && (len!=1);k++)
							{
								/* 
								 * make distinction between INTRA and INTER coded
								 * luminance coefficients
								 */
								/*
								if (interintra == INTER_CODED_MB )
								{
									if (k == 0)
										se = SE_LUM_DC_INTER;
									else
										se = SE_LUM_AC_INTER;
								}
								else
								{
									if (k == 0)
										se = SE_LUM_DC_INTRA;
									else
										se = SE_LUM_AC_INTRA;
								}
								*/
								get_symbol("Luma sng",&len,&info, SE_BFRAME); //se);

								if(len>29)		/* illegal length, do error concealment */
								{
									/*!
									 * \note
									 *	Error concealment for illegal luminance coeffs:
									 *	return EOB element to finalize block
									 */
									set_ec_flag(se);
									get_concealed_element(&len, &info, se);
								}
								if (len != 1)		/* leave if len=1 */
								{
									linfo_levrun_inter(len,info,&level,&run);
									coef_ctr += run+1;

									i0=SNGL_SCAN[coef_ctr][0];
									j0=SNGL_SCAN[coef_ctr][1];

									if(coef_ctr > 15)
									{
										/*!
										 * \note
										 *	Error concealment for luminance coeffs blocks 
										 *	exceeding the maximum possible coeff number within the block:
										 *	return EOB element to finalize block
										 */
										set_ec_flag(se);
										get_concealed_element(&len, &info, se);
									}

									img->cof[i][j][i0][j0]=level*JQ1[img->qp];
                                    //g.b. post filter like loop filter
                                    if (level=!0)
									{
										loopb[img->block_x+i+1][img->block_y+j+1]=max(loopb[img->block_x+i+1][img->block_y+j+1],2);
										loopb[img->block_x+i  ][img->block_y+j+1]=max(loopb[img->block_x+i  ][img->block_y+j+1],1);
										loopb[img->block_x+i+1][img->block_y+j  ]=max(loopb[img->block_x+i+1][img->block_y+j  ],1);
										loopb[img->block_x+i+2][img->block_y+j+1]=max(loopb[img->block_x+i+2][img->block_y+j+1],1);
										loopb[img->block_x+i+1][img->block_y+j+2]=max(loopb[img->block_x+i+1][img->block_y+j+2],1);
									}
								}
							}
						}
						else    /* double scan (old intra with QP<24*/
						{
							for(scan_loop_ctr=0;scan_loop_ctr<2;scan_loop_ctr++)
							{
								coef_ctr=start_scan-1;
								len=0;                          /* just to get inside the loop */
								for(k=0; k<9 && len!=1;k++)
								{
									/* <pu> distinction between DC and AC luminance coeffs */
									if ( k == 0 )
										se = SE_LUM_DC_INTRA;
									else
										se = SE_LUM_AC_INTRA;
									get_symbol("Luma dbl",&len,&info, SE_BFRAME); //se);

									if(len>29)		/* illegal length, do error concealment */
									{
										set_ec_flag(se);
										get_concealed_element(&len, &info, se);
									}
									if (len != 1)		/* leave if len=1 */
									{
										linfo_levrun_intra(len,info,&level,&run);
										coef_ctr=coef_ctr+run+1;

										i0=DBL_SCAN[coef_ctr][0][scan_loop_ctr];
										j0=DBL_SCAN[coef_ctr][1][scan_loop_ctr];

										if(coef_ctr > 7)		/* illegal number of coeffs, do error concealment */
										{
											/*!
											 * \note
											 *	Error concealment for luminance coeffs blocks 
											 *	exceeding the maximum possible coeff number within the block:
											 *	return EOB element to finalize block
											 */
											set_ec_flag(se);
											get_concealed_element(&len, &info, se);
										}
										img->cof[i][j][i0][j0]=level*JQ1[img->qp];
                                        //g.b. post filter like loop filter
                                        if (level=!0)
										{
											loopb[img->block_x+i+1][img->block_y+j+1]=max(loopb[img->block_x+i+1][img->block_y+j+1],2);
											loopb[img->block_x+i  ][img->block_y+j+1]=max(loopb[img->block_x+i  ][img->block_y+j+1],1);
											loopb[img->block_x+i+1][img->block_y+j  ]=max(loopb[img->block_x+i+1][img->block_y+j  ],1);
											loopb[img->block_x+i+2][img->block_y+j+1]=max(loopb[img->block_x+i+2][img->block_y+j+1],1);
											loopb[img->block_x+i+1][img->block_y+j+2]=max(loopb[img->block_x+i+1][img->block_y+j+2],1);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	/* reset all chroma coeffs before read */
	for (j=4;j<6;j++) 
		for (i=0;i<4;i++)
			for (iii=0;iii<4;iii++)
				for (jjj=0;jjj<4;jjj++)
					img->cof[i][j][iii][jjj]=0;

	m2=img->mb_x*2;
	jg2=img->mb_y*2;

	/* chroma 2x2 DC coeff */
	if(cbp>15)
	{
		for (ll=0;ll<3;ll+=2)
		{
			for (i=0;i<4;i++)
				img->cofu[i]=0;

			coef_ctr=-1;
			len=0;
			for(k=0;(k<5)&&(len!=1);k++)
			{
				/* <pu> distinct between INTRA and INTER coded elements */
				/*
				if ( interintra == INTRA_CODED_MB)
					se = SE_CHR_DC_INTRA;
				else
					se = SE_CHR_DC_INTER;
				*/
				get_symbol("2x2 DC Chroma",&len,&info, SE_BFRAME); //SE_CHR_DC_INTER);

				if (len>29)		/* illegal length do error concealment */
				{
					/*!
					 * \note
					 *	Error concealment for illegal chrominance coeffs:
					 *	return EOB element to finalize block
					 */
					set_ec_flag(se);
					get_concealed_element(&len, &info, se);
				}
				if (len != 1)
				{
					linfo_levrun_c2x2(len,info,&level,&run);
					coef_ctr=coef_ctr+run+1;
					if(coef_ctr>3)		/* illegal number of coeffs, do error concealment */
					{
						/*!
						 * \note
						 *	Error concealment for chrominance coeffs blocks 
						 *	exceeding the maximum possible coeff number within the block:
						 *	return EOB element to finalize block
						 */
						set_ec_flag(se);
						get_concealed_element(&len, &info, se);
					}
					img->cofu[coef_ctr]=level*JQ1[QP_SCALE_CR[img->qp]];
                    //g.b. post filter like loop filter
                    if (level != 0 );
					{
						for (j=0;j<2;j++)
						{
							for (i=0;i<2;i++)
							{
								loopc[m2+i+1][jg2+j+1]=max(loopc[m2+i+1][jg2+j+1],2);
							}
						}

						for (i=0;i<2;i++)
						{
							loopc[m2+i+1][jg2    ]=max(loopc[m2+i+1][jg2    ],1);
							loopc[m2+i+1][jg2+3  ]=max(loopc[m2+i+1][jg2+3  ],1);
							loopc[m2    ][jg2+i+1]=max(loopc[m2    ][jg2+i+1],1);
							loopc[m2+3  ][jg2+i+1]=max(loopc[m2+3  ][jg2+i+1],1);
						}
					}
				}
			}
			img->cof[0+ll][4][0][0]=(img->cofu[0]+img->cofu[1]+img->cofu[2]+img->cofu[3])/2;
			img->cof[1+ll][4][0][0]=(img->cofu[0]-img->cofu[1]+img->cofu[2]-img->cofu[3])/2;
			img->cof[0+ll][5][0][0]=(img->cofu[0]+img->cofu[1]-img->cofu[2]-img->cofu[3])/2;
			img->cof[1+ll][5][0][0]=(img->cofu[0]-img->cofu[1]-img->cofu[2]+img->cofu[3])/2;
		}
	}

	/* chroma AC coeff, all zero fram start_scan */
	if (cbp>31)
	{
		block_y=4;
		for (block_x=0; block_x < 4; block_x += 2)
		{
			for (j=block_y; j < block_y+2; j++)
			{
				jj=j/2;
				j1=j-4;
				for (i=block_x; i < block_x+2; i++)
				{
					ii=i/2;
					i1=i%2;
					{
						coef_ctr=0;
						len=0;
						for(k=0;(k<16)&&(len!=1);k++)
						{
							/* <pu> distinct between Intra and Inter coded elements */
							/*
							if ( interintra == INTER_CODED_MB )
								se = SE_CHR_AC_INTER;
							else
								se = SE_CHR_AC_INTRA;
							*/
							get_symbol("AC Chroma",&len,&info, SE_BFRAME); //SE_CHR_AC_INTER);

							if (len>29)	/* illegal length, do error concealment */
							{
								/*!
								 * \note
								 *	Error concealment for illegal chrominance coeffs:
								 *	return EOB element to finalize block
								 */
								set_ec_flag(se);
								get_concealed_element(&len, &info, se);
							}
							if (len != 1)
							{
								linfo_levrun_inter(len,info,&level,&run);
								coef_ctr=coef_ctr+run+1;
								i0=SNGL_SCAN[coef_ctr][0];
								j0=SNGL_SCAN[coef_ctr][1];
								if(coef_ctr>15)	/* illegal number of coeffs, do error concealment */
								{
									/*!
									 * \note
									 *	Error concealment for chrominance coeffs blocks 
									 *	exceeding the maximum possible coeff number within the block:
									 *	return EOB element to finalize block
									 */
									set_ec_flag(se);
									get_concealed_element(&len, &info, se);
								}
								img->cof[i][j][i0][j0]=level*JQ1[QP_SCALE_CR[img->qp]];	
                                //g.b. post filter like loop filter
                                if (level!=0)
								{
									loopc[m2+i1+1][jg2+j1+1]=max(loopc[m2+i1+1][jg2+j1+1],2);

									loopc[m2+i1  ][jg2+j1+1]=max(loopc[m2+i1  ][jg2+j1+1],1);
									loopc[m2+i1+1][jg2+j1  ]=max(loopc[m2+i1+1][jg2+j1  ],1);
									loopc[m2+i1+2][jg2+j1+1]=max(loopc[m2+i1+2][jg2+j1+1],1);
									loopc[m2+i1+1][jg2+j1+2]=max(loopc[m2+i1+1][jg2+j1+2],1);
								}
							}
						}
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////
	// start_scan decoding 
	//////////////////////////////////////////////////////////////
	/**********************************
	 *    luma                        *
	 *********************************/	
	if (img->imod==INTRA_MB_NEW)
		intrapred_luma_2(img,kmod);

	for(j=0;j<MB_BLOCK_SIZE/BLOCK_SIZE;j++)
	{
		joff=j*4;
		j4=img->block_y+j;
		for(i=0;i<MB_BLOCK_SIZE/BLOCK_SIZE;i++)
		{
			ioff=i*4;
			i4=img->block_x+i;
			if(img->imod==INTRA_MB_OLD)
			{
				if (intrapred(img,ioff,joff,i4,j4)==SEARCH_SYNC)  /* make 4x4 prediction block mpr from given prediction img->mb_mode */
				return SEARCH_SYNC;                   /* bit error */
			}

			////////////////////////////////////
			// Forward MC using img->fw_MV
			////////////////////////////////////
			else if(img->imod==B_Forward)
			{
				for(ii=0;ii<BLOCK_SIZE;ii++)
				{
					vec2_x=(i4*4+ii)*4;
					vec1_x=vec2_x+img->fw_mv[i4+BLOCK_SIZE][j4][0];
					for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
					{
						vec2_y=(j4*4+jj)*4;
						vec1_y=vec2_y+img->fw_mv[i4+BLOCK_SIZE][j4][1];
						// direct interpolation:
						// img->mpr[ii+ioff][jj+joff]=mref[ref_frame][vec1_y][vec1_x];
						img->mpr[ii+ioff][jj+joff]=get_pixel(ref_frame,vec1_x,vec1_y,img);
					}
				}
			}

			////////////////////////////////////
			// Backward MC using img->bw_MV
			////////////////////////////////////
			else if(img->imod==B_Backward)
			{
				for(ii=0;ii<BLOCK_SIZE;ii++)
				{
					vec2_x=(i4*4+ii)*4;
					vec1_x=vec2_x+img->bw_mv[i4+BLOCK_SIZE][j4][0];
					for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
					{
						vec2_y=(j4*4+jj)*4;
						vec1_y=vec2_y+img->bw_mv[i4+BLOCK_SIZE][j4][1];
						// direct interpolation:
						// img->mpr[ii+ioff][jj+joff]=mref_P[vec1_y][vec1_x];
						img->mpr[ii+ioff][jj+joff]=get_pixel_nextP(vec1_x,vec1_y,img);
					}
				}
			}

			////////////////////////////////////
			// Bidirect MC using img->fw_MV, bw_MV
			////////////////////////////////////
			else if(img->imod==B_Bidirect)
			{
				for(ii=0;ii<BLOCK_SIZE;ii++)
				{
					vec2_x=(i4*4+ii)*4;
					vec1_x=vec2_x+img->fw_mv[i4+BLOCK_SIZE][j4][0];
					vec1_xx=vec2_x+img->bw_mv[i4+BLOCK_SIZE][j4][0];
					for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
					{
						vec2_y=(j4*4+jj)*4;
						vec1_y=vec2_y+img->fw_mv[i4+BLOCK_SIZE][j4][1];
						vec1_yy=vec2_y+img->bw_mv[i4+BLOCK_SIZE][j4][1];
						// direct interpolation:
						// img->mpr[ii+ioff][jj+joff]=(int)((mref[ref_frame][vec1_y][vec1_x]+mref_P[vec1_yy][vec1_xx])/2.+.5);
						img->mpr[ii+ioff][jj+joff]=(int)((get_pixel(ref_frame,vec1_x,vec1_y,img)+
														  get_pixel_nextP(vec1_xx,vec1_yy,img))/2.+.5);
					}
				}
			}

			////////////////////////////////////
			// Direct MC using img->mv
			////////////////////////////////////
			else if(img->imod==B_Direct)
			{
				// next P is intra mode
				if(refFrArr[j4][i4]==-1)
				{
					for(hv=0; hv<2; hv++) 
					{
						dfMV[i4+BLOCK_SIZE][j4][hv]=dbMV[i4+BLOCK_SIZE][j4][hv]=0;
					}
					ref_frame=(img->number-2)%MAX_MULT_PRED;
				}
				// next P is skip or inter mode
				else 
				{
					refP_tr=nextP_tr-(refFrArr[j4][i4]*P_interval);
					TRb=img->tr-refP_tr;
					TRp=nextP_tr-refP_tr;
					dfMV[i4+BLOCK_SIZE][j4][0]=TRb*img->mv[i4+BLOCK_SIZE][j4][0]/TRp;
					dfMV[i4+BLOCK_SIZE][j4][1]=TRb*img->mv[i4+BLOCK_SIZE][j4][1]/TRp;
					dbMV[i4+BLOCK_SIZE][j4][0]=(TRb-TRp)*img->mv[i4+BLOCK_SIZE][j4][0]/TRp;
					dbMV[i4+BLOCK_SIZE][j4][1]=(TRb-TRp)*img->mv[i4+BLOCK_SIZE][j4][1]/TRp;
					ref_frame=(img->number-refFrArr[j4][i4]-1)%MAX_MULT_PRED;
				}

				for(ii=0;ii<BLOCK_SIZE;ii++)
				{
					vec2_x=(i4*4+ii)*4;
					vec1_x=vec2_x+dfMV[i4+BLOCK_SIZE][j4][0];
					vec1_xx=vec2_x+dbMV[i4+BLOCK_SIZE][j4][0];				
					for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
					{
						vec2_y=(j4*4+jj)*4;
						vec1_y=vec2_y+dfMV[i4+BLOCK_SIZE][j4][1];
						vec1_yy=vec2_y+dbMV[i4+BLOCK_SIZE][j4][1];

						// LG :  direct residual coding
						// Wedi: direct interpolation
						// img->mpr[ii+ioff][jj+joff]=(int)((mref[ref_frame][vec1_y][vec1_x]+mref_P[vec1_yy][vec1_xx])/2.+.5);
						img->mpr[ii+ioff][jj+joff]=(int)((get_pixel(ref_frame,vec1_x,vec1_y,img)+get_pixel_nextP(vec1_xx,vec1_yy,img))/2.+.5);
					}
				}

				// LG : loopb, loopc at Direct mode 
				i3=i4/2;
				j3=j4/2;

				mvDiffX = dfMV[i4+4][j4][0] - dfMV[i4-1+4][j4][0];
				mvDiffY = dfMV[i4+4][j4][1] - dfMV[i4-1+4][j4][1];
				if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && i4 > 0)
				{ 
					loopb[i4  ][j4+1]=max(loopb[i4  ][j4+1],1);
					loopb[i4+1][j4+1]=max(loopb[i4+1][j4+1],1); 
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);                    
				}
			    if (j4 > 0)
				{
					mvDiffX = dfMV[i4+4][j4][0] - dfMV[i4+4][j4-1][0];
					mvDiffY = dfMV[i4+4][j4][1] - dfMV[i4+4][j4-1][1];

					if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && j4 > 0)
					{
						loopb[i4+1][j4  ]=max(loopb[i4+1][j4  ],1);
						loopb[i4+1][j4+1]=max(loopb[i4+1][j4+1],1);
						loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
						loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
					}
				}		

				mvDiffX = dbMV[i4+4][j4][0] - dbMV[i4-1+4][j4][0];
				mvDiffY = dbMV[i4+4][j4][1] - dbMV[i4-1+4][j4][1];
				if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && i4 > 0)
				{ 
					loopb[i4  ][j4+1]=max(loopb[i4  ][j4+1],1);
					loopb[i4+1][j4+1]=max(loopb[i4+1][j4+1],1); 
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);                    
		        }
				if (j4 > 0)
				{
					mvDiffX = dbMV[i4+4][j4][0] - dbMV[i4+4][j4-1][0];
					mvDiffY = dbMV[i4+4][j4][1] - dbMV[i4+4][j4-1][1];

					if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && j4 > 0)
					{
						loopb[i4+1][j4  ]=max(loopb[i4+1][j4  ],1);
						loopb[i4+1][j4+1]=max(loopb[i4+1][j4+1],1);
						loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
						loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
					}
				}
			}

			itrans(img,ioff,joff,i,j);      /* use DCT transform and make 4x4 block m7 from prediction block mpr */

			for(ii=0;ii<BLOCK_SIZE;ii++)
			{
				for(jj=0;jj<BLOCK_SIZE;jj++)
				{
					imgY[j4*BLOCK_SIZE+jj][i4*BLOCK_SIZE+ii]=img->m7[ii][jj]; /* contruct picture from 4x4 blocks*/
				}
			}
		}
	}

	/**********************************
	 *    chroma                      *
	 *********************************/
	for(uv=0;uv<2;uv++)
	{
		if (img->imod==INTRA_MB_OLD || img->imod==INTRA_MB_NEW)/* intra mode */
		{
			js0=0;
			js1=0;
			js2=0;
			js3=0;
			for(i=0;i<4;i++)
			{
				if(mb_available_up)
				{
					js0=js0+imgUV[uv][img->pix_c_y-1][img->pix_c_x+i];
					js1=js1+imgUV[uv][img->pix_c_y-1][img->pix_c_x+i+4];
				}
				if(mb_available_left)
				{
					js2=js2+imgUV[uv][img->pix_c_y+i][img->pix_c_x-1];
					js3=js3+imgUV[uv][img->pix_c_y+i+4][img->pix_c_x-1];
				}
			}
			if(mb_available_up && mb_available_left)
			{
				js[0][0]=(js0+js2+4)/8;
				js[1][0]=(js1+2)/4;
				js[0][1]=(js3+2)/4;
				js[1][1]=(js1+js3+4)/8;
			}
			if(mb_available_up && !mb_available_left)
			{
				js[0][0]=(js0+2)/4;
				js[1][0]=(js1+2)/4;
				js[0][1]=(js0+2)/4;
				js[1][1]=(js1+2)/4;
			}
			if(mb_available_left && !mb_available_up)
			{
				js[0][0]=(js2+2)/4;
				js[1][0]=(js2+2)/4;
				js[0][1]=(js3+2)/4;
				js[1][1]=(js3+2)/4;
			}
			if(!mb_available_up && !mb_available_left)
			{
				js[0][0]=128;
				js[1][0]=128;
				js[0][1]=128;
				js[1][1]=128;
			}
		}

		for (j=4;j<6;j++)
		{
			joff=(j-4)*4;
			j4=img->pix_c_y+joff;
			for(i=0;i<2;i++)
			{
				ioff=i*4;
				i4=img->pix_c_x+ioff;
				/* make pred */
				if(img->imod==INTRA_MB_OLD|| img->imod==INTRA_MB_NEW)/* intra */
				{
					for(ii=0;ii<4;ii++)
						for(jj=0;jj<4;jj++)
						{
							img->mpr[ii+ioff][jj+joff]=js[i][j-4];
						}
				}
				////////////////////////////////////
				// Forward Reconstruction
				////////////////////////////////////
				else if(img->imod==B_Forward)
				{
					for(jj=0;jj<4;jj++)
					{
						jf=(j4+jj)/2;
						for(ii=0;ii<4;ii++)
						{
							if1=(i4+ii)/2;
							i1=(img->pix_c_x+ii+ioff)*8+img->fw_mv[if1+4][jf][0];
							j1=(img->pix_c_y+jj+joff)*8+img->fw_mv[if1+4][jf][1];

							ii0=i1/8;
							jj0=j1/8;
							ii1=(i1+7)/8;
							jj1=(j1+7)/8;

							if1=(i1 & 7);
							jf1=(j1 & 7);
							if0=8-if1;
							jf0=8-jf1;
							img->mpr[ii+ioff][jj+joff]=(if0*jf0*mcef[ref_frame][uv][jj0][ii0]+
							                            if1*jf0*mcef[ref_frame][uv][jj0][ii1]+
							                            if0*jf1*mcef[ref_frame][uv][jj1][ii0]+
							                            if1*jf1*mcef[ref_frame][uv][jj1][ii1]+32)/64;
						}
					}
				}
				////////////////////////////////////
				// Backward Reconstruction
				////////////////////////////////////
				else if(img->imod==B_Backward)
				{
					for(jj=0;jj<4;jj++)
					{
						jf=(j4+jj)/2;
						for(ii=0;ii<4;ii++)
						{
							if1=(i4+ii)/2;
							i1=(img->pix_c_x+ii+ioff)*8+img->bw_mv[if1+4][jf][0];
							j1=(img->pix_c_y+jj+joff)*8+img->bw_mv[if1+4][jf][1];

							ii0=i1/8;
							jj0=j1/8;
							ii1=(i1+7)/8;
							jj1=(j1+7)/8;

							if1=(i1 & 7);
							jf1=(j1 & 7);
							if0=8-if1;
							jf0=8-jf1;
							img->mpr[ii+ioff][jj+joff]=(if0*jf0*mcef_P[uv][jj0][ii0]+
							                            if1*jf0*mcef_P[uv][jj0][ii1]+
							                            if0*jf1*mcef_P[uv][jj1][ii0]+
							                            if1*jf1*mcef_P[uv][jj1][ii1]+32)/64;
						}
					}
				}
				////////////////////////////////////
				// Bidirect Reconstruction
				////////////////////////////////////
				else if(img->imod==B_Bidirect)
				{
					for(jj=0;jj<4;jj++)
					{
						jf=(j4+jj)/2;
						for(ii=0;ii<4;ii++)
						{
							ifx=(i4+ii)/2;
							i1=(img->pix_c_x+ii+ioff)*8+img->fw_mv[ifx+4][jf][0];
							j1=(img->pix_c_y+jj+joff)*8+img->fw_mv[ifx+4][jf][1];
							ii0=i1/8;
							jj0=j1/8;
							ii1=(i1+7)/8;
							jj1=(j1+7)/8;
							if1=(i1 & 7);
							jf1=(j1 & 7);
							if0=8-if1;
							jf0=8-jf1;

							fw_pred=(if0*jf0*mcef[ref_frame][uv][jj0][ii0]+
							         if1*jf0*mcef[ref_frame][uv][jj0][ii1]+
							         if0*jf1*mcef[ref_frame][uv][jj1][ii0]+
							         if1*jf1*mcef[ref_frame][uv][jj1][ii1]+32)/64;

							i1=(img->pix_c_x+ii+ioff)*8+img->bw_mv[ifx+4][jf][0];
							j1=(img->pix_c_y+jj+joff)*8+img->bw_mv[ifx+4][jf][1];
							ii0=i1/8;
							jj0=j1/8;
							ii1=(i1+7)/8;
							jj1=(j1+7)/8;
							if1=(i1 & 7);
							jf1=(j1 & 7);
							if0=8-if1;
							jf0=8-jf1;

							bw_pred=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
							         if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+32)/64;

							img->mpr[ii+ioff][jj+joff]=(int)((fw_pred+bw_pred)/2.+.5);
						}
					}
				}
				////////////////////////////////////
				// Direct Reconstruction
				////////////////////////////////////
				else if(img->imod==B_Direct)
				{
					for(jj=0;jj<4;jj++)
					{
						jf=(j4+jj)/2;
						for(ii=0;ii<4;ii++)
						{
							ifx=(i4+ii)/2;
							i1=(img->pix_c_x+ii+ioff)*8+dfMV[ifx+4][jf][0];
							j1=(img->pix_c_y+jj+joff)*8+dfMV[ifx+4][jf][1];
							ii0=i1/8;
							jj0=j1/8;
							ii1=(i1+7)/8;
							jj1=(j1+7)/8;
							if1=(i1 & 7);
							jf1=(j1 & 7);
							if0=8-if1;
							jf0=8-jf1;

							if(refFrArr[jf][ifx]==-1)
								ref_frame=(img->number-2)%MAX_MULT_PRED;
							else 
								ref_frame=(img->number-refFrArr[jf][ifx]-1)%MAX_MULT_PRED;

							fw_pred=(if0*jf0*mcef[ref_frame][uv][jj0][ii0]+
							         if1*jf0*mcef[ref_frame][uv][jj0][ii1]+
							         if0*jf1*mcef[ref_frame][uv][jj1][ii0]+
							         if1*jf1*mcef[ref_frame][uv][jj1][ii1]+32)/64;

							i1=(img->pix_c_x+ii+ioff)*8+dbMV[ifx+4][jf][0];
							j1=(img->pix_c_y+jj+joff)*8+dbMV[ifx+4][jf][1];
							ii0=i1/8;
							jj0=j1/8;
							ii1=(i1+7)/8;
							jj1=(j1+7)/8;
							if1=(i1 & 7);
							jf1=(j1 & 7);
							if0=8-if1;
							jf0=8-jf1;

							bw_pred=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
							         if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+32)/64;
						
							// LG : direct residual coding
							img->mpr[ii+ioff][jj+joff]=(int)((fw_pred+bw_pred)/2.+.5);
						}
					}
				}

				itrans(img,ioff,joff,2*uv+i,j);
				for(ii=0;ii<4;ii++)
					for(jj=0;jj<4;jj++)
					{
						imgUV[uv][j4+jj][i4+ii]=img->m7[ii][jj];
					}
			} // for(i=0;i<2;i++)
		} // for (j=4;j<6;j++)
	} // for(uv=0;uv<2;uv++)

	return DECODING_OK;
}

/************************************************************************
*
*  Name :       get_pixel_nextP()
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

byte get_pixel_nextP(int x_pos, int y_pos, struct img_par *img)
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
     result=imgY_prev[pres_y][pres_x];
     return result;
   }
 else if(dx==3&&dy==3) /* funny position */
   {
     pres_y=y_pos/4;
     pres_x=x_pos/4;
     result=(mref_P_small[pres_y                ][pres_x                ]+
			 mref_P_small[pres_y                ][min(maxold_x,pres_x+1)]+
			 mref_P_small[min(maxold_y,pres_y+1)][min(maxold_x,pres_x+1)]+
			 mref_P_small[min(maxold_y,pres_y+1)][pres_x                ]+2)/4;
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
	    result+=mref_P_small[pres_y][pres_x]*two[x+2];
	  }
      }
    else if(dx==0&&dy==1)
      {
	pres_x=x_pos/4;
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    result+=mref_P_small[pres_y][pres_x]*two[y+2];
	  }
      }
    else if(dx==2&&dy==0)
      {
	pres_y=y_pos/4;
	for(x=-2;x<4;x++)
	  {
	    pres_x=max(0,min(maxold_x,x_pos/4+x));
	    result+=mref_P_small[pres_y][pres_x]*three[x+2];
	  }
      }
    else if(dx==0&&dy==2)
      {
	pres_x=x_pos/4;
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    result+=mref_P_small[pres_y][pres_x]*three[y+2];
	  }
      }
    else if(dx==3&&dy==0)
      {
	pres_y=y_pos/4;
	for(x=-2;x<4;x++)
	  {
	    pres_x=max(0,min(maxold_x,x_pos/4+x));
	    result+=mref_P_small[pres_y][pres_x]*two[3-x];   /* four */
	  }
      }
    else if(dx==0&&dy==3)
      {
	pres_x=x_pos/4;
	for(y=-2;y<4;y++)
	  {
	    pres_y=max(0,min(maxold_y,y_pos/4+y));
	    result+=mref_P_small[pres_y][pres_x]*two[3-y];   /* four */
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
		result+=mref_P_small[pres_y][pres_x]*six[y+2][x+2];  
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
		result+=mref_P_small[pres_y][pres_x]*seven[y+2][x+2];
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
		result+=mref_P_small[pres_y][pres_x]*seven[x+2][y+2]; /* eight */
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
		result+=mref_P_small[pres_y][pres_x]*six[y+2][3-x];   /* nine */
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
		result+=mref_P_small[pres_y][pres_x]*six[3-y][x+2];   /* ten */
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
		result+=mref_P_small[pres_y][pres_x]*five[y+2][x+2];
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
		result+=mref_P_small[pres_y][pres_x]*seven[3-x][3-y]; /* eleven */
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
		result+=mref_P_small[pres_y][pres_x]*seven[3-y][x+2]; /* twelve */
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
		result+=mref_P_small[pres_y][pres_x]*six[3-y][3-x]; /* thirteen */
	      }
	  }
      }
    
  }
 
 return max(0,min(255,(result+2048)/4096));
 
}
