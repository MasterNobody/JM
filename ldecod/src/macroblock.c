/*!
 *	\file
 *			Macroblock.c
 *	\brief
 *			Decode a Macroblock
 *	\author
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *			Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *			Jani Lainema                    <jani.lainema@nokia.com>
 *			Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *			Thomas Wedi						<wedi@tnt.uni-hannover.de>
 */
#include "contributors.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "global.h"
#include "elements.h"
#include "macroblock.h"


/*!
 *	\fn		ResetSlicePredictions()
 *	\brief
 */
void ResetSlicePredictions(struct img_par *img)	
{
	int x_pos = img->mb_x * 16;
	int y_pos = img->mb_y * 16;
	int mb_nr = img->current_mb_nr;
	int mb_width = img->width/16;
	/* change ipredmode for this block depending on slice */

	if(x_pos >= 16)
	{
		if(slice_numbers[mb_nr] != slice_numbers[mb_nr-1])
		{
			img->ipredmode[x_pos/4][y_pos/4+1] = -1;
			img->ipredmode[x_pos/4][y_pos/4+2] = -1;
			img->ipredmode[x_pos/4][y_pos/4+3] = -1;
			img->ipredmode[x_pos/4][y_pos/4+4] = -1;
		}
	}

	if(y_pos >= 16)
	{
		if(slice_numbers[mb_nr] != slice_numbers[mb_nr-mb_width])
		{
			img->ipredmode[x_pos/4+1][y_pos/4] = -1;
			img->ipredmode[x_pos/4+2][y_pos/4] = -1;
			img->ipredmode[x_pos/4+3][y_pos/4] = -1;
			img->ipredmode[x_pos/4+4][y_pos/4] = -1;
		}
	}
}


/*!
 *	\fn		macroblock()
 *
 *	\brief	Decode one macroblock
 *  \return SEARCH_SYNC : error in bitstream
 *          DECODING_OK : no error was detected
 */
int macroblock(
	struct img_par *img) 	/*!< image parameters (modified) */
{
	int js[2][2];
	int i,j,ii,jj,iii,jjj,i1,j1,ie,j4,i4,k,i0,pred_vec,j0;
	int js0,js1,js2,js3,jf,mvDiffX,mvDiffY;
	int coef_ctr,vec,cbp,uv,ll;
	int scan_loop_ctr;
	int level,run;
	int ref_frame;
	int len,info;
	int predframe_no;         /* frame number which current MB is predicted from */
	int block_x,block_y;
	int cbp_idx,dbl_ipred_word;
	int vec1_x,vec1_y,vec2_x,vec2_y;
	int scan_mode;

	const int ICBPTAB[6] = {0,16,32,15,31,47};
	int mod0;
	int kmod;
	int start_scan;
	int ioff,joff;
	int i3,j3;

	int ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;

	int m2;
	int jg2;

	int interintra; /* distinction between Inter and Intra MBs */
	int se;			/* syntax element type */


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

	if (img->type==INTER_IMG_1 || (img->type==INTER_IMG_MULT))         /* inter frame */
	{
		get_symbol("MB mode",&len,&info,SE_MBTYPE);
		img->mb_mode = (int)pow(2,(len/2))+info-1;          /* inter prediction mode (block shape) */
		if (img->mb_mode>32) /* 8 inter + 24 new intra */
		{
			/*!
			 * \note
			 *	Error concealment for MBTYPE:
			 *  Copy macroblock of last decoded picture 
			 */
			set_ec_flag(SE_MBTYPE);
			img->imod = INTRA_MB_INTER;
			img->mb_mode = COPY_MB;
		}
		else /* no error concealment necessary */
		{
			if (img->mb_mode == INTRA_MB) /* 4x4 intra */
				img->imod=INTRA_MB_OLD;
			if (img->mb_mode > INTRA_MB)  /* 16x16 intra */
			{
				img->imod=INTRA_MB_NEW;
				mod0=img->mb_mode - INTRA_MB-1;
				kmod=mod0 & 3;
				cbp = ICBPTAB[mod0/4];
			}
			if (img->mb_mode < INTRA_MB)  /* intra in inter frame */
				img->imod=INTRA_MB_INTER;
		}
	}
	else if ((img->type==INTRA_IMG))
	{
		get_symbol("Mb mode",&len,&info,SE_MBTYPE);

		img->mb_mode = (int)pow(2,(len/2))+info-1;
		if (img->mb_mode>24)
		{ /* 24 new intra */
			/*!
			 * \note
			 *	Error concealment for Intra macroblocks:
			 *	as for Intra images we do not have a COPY_MB type for macroblocks, we
			 *	switch only for this macroblock to a INTER imod to copy the information
			 *	from the last decoded MB
			 */
			set_ec_flag(SE_MBTYPE);
			img->imod = INTRA_MB_INTER;
			img->mb_mode = COPY_MB;
		}
		else /* no error concealment necessary */
		{
			if (img->mb_mode == 0)
				img->imod=INTRA_MB_OLD; /* 4x4 intra */
			else
			{
				img->imod=INTRA_MB_NEW;/* new */
				mod0=img->mb_mode-1;
				kmod=mod0 & 3;
				cbp = ICBPTAB[mod0/4];
			}
		}
	}
	else
	{
		/* no error concealment if PTYPE is damaged */
		printf("Wrong image type, exiting\n");
		return SEARCH_SYNC;
	}

	img->mv[img->block_x+4][img->block_y][2]=img->number;

	for (i=0;i<BLOCK_SIZE;i++)
	{                           /* reset vectors and pred. modes  */
		for(j=0;j<BLOCK_SIZE;j++)
		{
			img->mv[img->block_x+i+4][img->block_y+j][0] = 0;
			img->mv[img->block_x+i+4][img->block_y+j][1] = 0;
			img->ipredmode[img->block_x+i+1][img->block_y+j+1] = 0;
		}
	}
	ref_frame = img->frame_cycle;
	predframe_no=1;

	/* Set the reference frame information for motion vector prediction */
	if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
	{
		for (j = 0;j < BLOCK_SIZE;j++)
		{
			for (i = 0;i < BLOCK_SIZE;i++)
			{
				refFrArr[img->block_y+j][img->block_x+i] = -1;
			}
		}
	}
	else
	{
		for (j = 0;j < BLOCK_SIZE;j++)
		{
			for (i = 0;i < BLOCK_SIZE;i++)
			{
				refFrArr[img->block_y+j][img->block_x+i] = predframe_no;
			}
		}
	}

	if (img->imod==INTRA_MB_INTER && img->mb_mode==COPY_MB) /*keep last macroblock*/
	{
		// luma
		for(j=0;j<MB_BLOCK_SIZE;j++)
		{
			jj=img->pix_y+j;
			for(i=0;i<MB_BLOCK_SIZE;i++)
			{
				ii=img->pix_x+i;
				imgY[jj][ii]=get_pixel(ref_frame,ii*4,jj*4,img);
				// imgY[jj][ii]=mref[ref_frame][jj*4][ii*4];
			}
		}

		// chroma
		for(uv=0;uv<2;uv++)
		{
			for(j=0;j<MB_BLOCK_SIZE/2;j++)
			{
				jj=img->pix_c_y+j;
				for(i=0;i<MB_BLOCK_SIZE/2;i++)
				{
					ii=img->pix_c_x+i;
					imgUV[uv][jj][ii]=mcef[ref_frame][uv][jj][ii];
				}
			}
		}

		for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;
				
				if (((img->mv[ii-1+4][jj][0]/4)!=0||(img->mv[ii-1+4][jj][1]/4!=0)) && ii > 0)				
				{
					loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}
				if (((img->mv[ii+4][jj-1][0]/4!=0)||(img->mv[ii+4][jj-1][1]/4!=0)) && jj > 0)
				{
					loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
					loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}
			}
		}
		return DECODING_OK;
	}

	if (img->imod==INTRA_MB_OLD)                 /* find two and two intra prediction mode for a macroblock */
	{
		for(i=0;i<MB_BLOCK_SIZE/2;i++)
		{

			get_symbol("Intra mode", &len,&info,SE_INTRAPREDMODE);
			dbl_ipred_word = (int) pow(2,(len/2))+info-1;
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
	if(img->imod==INTRA_MB_INTER)               /* inter frame, read vector data */
	{
		if(img->type==INTER_IMG_MULT)
		{
			get_symbol("Reference frame",&len,&info,SE_REFFRAME);
			predframe_no=(int)pow(2,len/2)+info;      /* find frame number to copy blocks from (if mult.pred mode)*/
			if (predframe_no > MAX_MULT_PRED)
			{
				/*! 
				 * \note
				 *	Error concealment for REFFRAME:
				 *	predict from last decoded frame and set all MVD's
				 *	and the CBP to zero
				 * \sa errorconcealment.c
				 */
				set_ec_flag(SE_REFFRAME);
				predframe_no = 1;
			}
			ref_frame=(img->frame_cycle+6-predframe_no) % MAX_MULT_PRED;

			/*!
			 * \note
			 *	if frame lost occurs within MAX_MULT_PRED frames and buffer of previous 
			 *	decoded pictures is still empty, set ref_frame to last decoded 
			 *	picture to avoid prediction from unexistent frames.
			 */
			if (ref_frame > img->number)
			{
				ref_frame = 0;
			}

			/* Update the reference frame information for motion vector prediction */
			for (j = 0;j < BLOCK_SIZE;j++)
			{
				for (i = 0;i < BLOCK_SIZE;i++)
				{
					refFrArr[img->block_y+j][img->block_x+i] = predframe_no;
				}
			}

		}
		ie=4-BLOCK_STEP[img->mb_mode][0];
		for(j=0;j<4;j=j+BLOCK_STEP[img->mb_mode][1])   /* Y position in 4-pel resolution */
		{
			block_available_up = mb_available_up || (j > 0);
			j4=img->block_y+j;
			for(i=0;i<4;i=i+BLOCK_STEP[img->mb_mode][0]) /* X position in 4-pel resolution */
			{
				i4=img->block_x+i;

				/* D B C */
				/* A X   */

				/* 1 A, B, D are set to 0 if unavailable       */
				/* 2 If C is not available it is replaced by D */

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

				rFrameL    = block_available_left    ? refFrArr[j4][i4-1]   : -1;
				rFrameU    = block_available_up      ? refFrArr[j4-1][i4]   : -1;
				rFrameUR   = block_available_upright ? refFrArr[j4-1][i4+BLOCK_STEP[img->mb_mode][0]] :
				             block_available_upleft  ? refFrArr[j4-1][i4-1] : -1;

				/* Prediction if only one of the neighbors uses the selected reference frame */

				if(rFrameL == predframe_no && rFrameU != predframe_no && rFrameUR != predframe_no)
					mvPredType = MVPRED_L;
				else if(rFrameL != predframe_no && rFrameU == predframe_no && rFrameUR != predframe_no)
					mvPredType = MVPRED_U;
				else if(rFrameL != predframe_no && rFrameU != predframe_no && rFrameUR == predframe_no)
					mvPredType = MVPRED_UR;

				/* Directional predictions */

				else if(img->mb_mode == 3)
				{
					if(i == 0)
					{
						if(rFrameL == predframe_no)
							mvPredType = MVPRED_L;
					}
					else
					{
						if(rFrameUR == predframe_no)
							mvPredType = MVPRED_UR;
					}
				}
				else if(img->mb_mode == 2)
				{
					if(j == 0)
					{
						if(rFrameU == predframe_no)
							mvPredType = MVPRED_U;
					}
					else
					{
						if(rFrameL == predframe_no)
							mvPredType = MVPRED_L;
					}
				}
				else if(img->mb_mode == 5 && i == 2)
					mvPredType = MVPRED_L;
				else if(img->mb_mode == 6 && j == 2)
					mvPredType = MVPRED_U;

				for (k=0;k<2;k++)                       /* vector prediction. find first koordinate set for pred */
				{
					mv_a = block_available_left ? img->mv[i4-1+BLOCK_SIZE][j4][k] : 0;
					mv_b = block_available_up      ? img->mv[i4+BLOCK_SIZE][j4-1][k]   : 0;
					mv_d = block_available_upleft  ? img->mv[i4-1+BLOCK_SIZE][j4-1][k] : 0;
					mv_c = block_available_upright ? img->mv[i4+BLOCK_STEP[img->mb_mode][0]+BLOCK_SIZE][j4-1][k] : mv_d;

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

					get_symbol("MVD",&len,&info,SE_MVD);
					if (len>17)
					{
						/*!
						 * \note
						 *	Errorconcealment for motion vectors:
						 *	set all MVD to zero as well as possible CBP information
						 */
						set_ec_flag(SE_MVD);
						get_concealed_element(&len, &info, SE_MVD);
					}
					vec=linfo_mvd(len,info)+pred_vec;           /* find motion vector */
					for(ii=0;ii<BLOCK_STEP[img->mb_mode][0];ii++)
					{
						for(jj=0;jj<BLOCK_STEP[img->mb_mode][1];jj++)
						{
							img->mv[i4+ii+BLOCK_SIZE][j4+jj][k]=vec;
						}
					}
				}
			}
		}
	}


	interintra = inter_intra(img);	/* <pu> get information if MB is INTRA or INTER coded */

	/* read CBP if not new intra mode */
	if (img->imod != INTRA_MB_NEW)
	{
		/* <pu> check for Inter or Intra CBP syntax element */
		if ( interintra == INTRA_CODED_MB )
			se = SE_CBP_INTRA;
		else
			se = SE_CBP_INTER;
		get_symbol("CBP",&len,&info,se);

		cbp_idx = (int) pow(2,(len/2))+info-1;      /* find cbp index in VLC table */
		if (cbp_idx>47)                             /* illegal value, exit to find next sync word */
		{
			/*!
			 * \note
			 *	Error concealment for Coded Block Pattern:
			 *	Set CBP to no texture information for all following 
			 *	CBP elements of either type INTER or INTRA
			 */
			printf("Error in index n to array cbp\n");
			set_ec_flag(se);
			get_concealed_element(&len, &info, se);
		}
		cbp=NCBP[cbp_idx][img->imod/INTRA_MB_INTER];/* find coded block pattern for intra or inter */
	}

	for (i=0;i<BLOCK_SIZE;i++)
		for (j=0;j<BLOCK_SIZE;j++)
			for(iii=0;iii<BLOCK_SIZE;iii++)
				for(jjj=0;jjj<BLOCK_SIZE;jjj++)
					img->cof[i][j][iii][jjj]=0;/* reset luma coeffs */

	if(img->imod==INTRA_MB_NEW) /* read DC coeffs for new intra modes */
	{
		for (i=0;i<BLOCK_SIZE;i++)
			for (j=0;j<BLOCK_SIZE;j++)
				img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;


		coef_ctr=-1;
		len = 0;                            /* just to get inside the loop */
		for(k=0;(k<17) && (len!=1);k++)
		{
			get_symbol("DC luma 16x16",&len,&info,SE_LUM_DC_INTRA);

			if(len>29)  /* illegal length, start_scan sync search */
			{
				/*!
				 * \note
				 *	Error concealment for illegal luminance coeffs:
				 *	return EOB element to finalize block
				 */
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
					/*!
					 * \note
					 *	Error concealment for luminance coeffs blocks 
					 *	exceeding the maximum possible coeff number within the block:
					 *	return EOB element to finalize block
					 */
					set_ec_flag(SE_LUM_DC_INTRA);
					get_concealed_element(&len, &info, SE_LUM_DC_INTRA);
				}

				img->cof[i0][j0][0][0]=level;/* add new intra DC coeff */
			}
		}
		itrans_2(img);/* transform new intra DC  */
	}

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
	else /* inter */
	{
		for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;

				mvDiffX = img->mv[ii+4][jj][0] - img->mv[ii-1+4][jj][0];
				mvDiffY = img->mv[ii+4][jj][1] - img->mv[ii-1+4][jj][1];

				if ((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
				{
					loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}

				mvDiffX = img->mv[ii+4][jj][0] - img->mv[ii+4][jj-1][0];
				mvDiffY = img->mv[ii+4][jj][1] - img->mv[ii+4][jj-1][1];

				if ((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && jj > 0)
				{
					loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
					loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
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
								get_symbol("Luma sng",&len,&info,se);

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
									get_symbol("Luma dbl",&len,&info,se);

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

	for (j=4;j<6;j++) /* reset all chroma coeffs before read */
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
				if ( interintra == INTRA_CODED_MB)
					se = SE_CHR_DC_INTRA;
				else
					se = SE_CHR_DC_INTER;
				get_symbol("2x2 DC Chroma",&len,&info,se);

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
							if ( interintra == INTER_CODED_MB )
								se = SE_CHR_AC_INTER;
							else
								se = SE_CHR_AC_INTRA;
							get_symbol("AC Chroma",&len,&info,se);

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

	/* start_scan decoding */

	/* luma */
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
			else if(img->imod==INTRA_MB_INTER)
			{
				for(ii=0;ii<BLOCK_SIZE;ii++)
				{
					vec2_x=(i4*4+ii)*4;
					vec1_x=vec2_x+img->mv[i4+BLOCK_SIZE][j4][0];
					for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
					{
						vec2_y=(j4*4+jj)*4;
						vec1_y=vec2_y+img->mv[i4+4][j4][1];
						img->mpr[ii+ioff][jj+joff]=get_pixel(ref_frame,vec1_x,vec1_y,img);
						// img->mpr[ii+ioff][jj+joff]=mref[ref_frame][vec1_y][vec1_x];
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
	/* chroma */

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
				else
				{
					for(jj=0;jj<4;jj++)
					{
						jf=(j4+jj)/2;
						for(ii=0;ii<4;ii++)
						{
							if1=(i4+ii)/2;
							i1=(img->pix_c_x+ii+ioff)*8+img->mv[if1+4][jf][0];
							j1=(img->pix_c_y+jj+joff)*8+img->mv[if1+4][jf][1];

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
				itrans(img,ioff,joff,2*uv+i,j);
				for(ii=0;ii<4;ii++)
					for(jj=0;jj<4;jj++)
					{
						imgUV[uv][j4+jj][i4+ii]=img->m7[ii][jj];
					}
			}
		}
	}
	return DECODING_OK;
}


/*!
 *	\fn		inter_intra()
 *	\brief	returns coding mode of actual macroblock if it is either
 *			inter or intra coded
 *	\return	INTER_CODED_MB, macroblock is inter coded
 *			INTRA_CODED_MB, macroblock is intra coded,
 *			either with the old or new intra mode
 *	<pu> added 6.10.2000
 */

int inter_intra(
	struct img_par *img)  /*!< pointer to image struct */
{
	if ( img->imod == INTRA_MB_OLD ||  img->imod == INTRA_MB_NEW ) // bug fix, DM 14/03/01
	{
		return INTRA_CODED_MB;
	}
	else
	{
		if ( img->mb_mode < 8)
			return INTER_CODED_MB;
		else
			return INTRA_CODED_MB;
	}
}

