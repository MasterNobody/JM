// *************************************************************************************
// *************************************************************************************
// Macroblock.c  Process one macroblock
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
// Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
// Jani Lainema                    <jani.lainema@nokia.com>
// Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
// *************************************************************************************
// *************************************************************************************
#include "contributors.h"

#include <math.h>
#include <stdlib.h>

#include "global.h"
#include "elements.h"
#include "macroblock.h"


/************************************************************************
*
*  Name :       ResetSlicePredictions(struct img_par *img)
*
*  Description: Set ipredmode for the neighboring blocks of the current
*               macroblock to -1 if the blocks are unavailable for prediction
*
************************************************************************/
void ResetSlicePredictions(struct img_par *img)
{
	int x_pos = img->mb_x * 16;
	int y_pos = img->mb_y * 16;
	int mb_nr = img->current_mb_nr;
	int mb_width = img->width/16;

	/* Check MB to the left */
	if(x_pos >=16)
	{
		if(img->slice_numbers[mb_nr] != img->slice_numbers[mb_nr-1])
		{
			img->ipredmode[x_pos/4][y_pos/4+1] = -1;
			img->ipredmode[x_pos/4][y_pos/4+2] = -1;
			img->ipredmode[x_pos/4][y_pos/4+3] = -1;
			img->ipredmode[x_pos/4][y_pos/4+4] = -1;
		}
	}

	/* Check MB above */
	if(y_pos >= 16)
	{
		if(img->slice_numbers[mb_nr] != img->slice_numbers[mb_nr-mb_width])
		{
			img->ipredmode[x_pos/4+1][y_pos/4] = -1;
			img->ipredmode[x_pos/4+2][y_pos/4] = -1;
			img->ipredmode[x_pos/4+3][y_pos/4] = -1;
			img->ipredmode[x_pos/4+4][y_pos/4] = -1;
		}
	}
}

/************************************************************************
*
*  Name :       macroblock()
*
*  Description: Encode one macroblock
*
************************************************************************/
void macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat)
{
	int i,j,k;
	int predframe_no;

	int cbp,cr_cbp,cbp_mask;              /* codec block pattern variables */
	int sum_cnt_nonz,coeff_cost;
	int mb_y,mb_x,block_x,block_y;
	int best_intra_sad,current_intra_sad,tot_intra_sad;
	int best_ipmode;
	int uv,jj4,ii4,i2,j2,step_h,step_v;
	int len,info;
	int kk,kbeg,kend;
	int level,run;
	int intra_pred_modes[8];              /* store intra prediction modes, two and two */
	int pic_pix_y,pic_pix_x,pic_block_y,pic_block_x;
	int last_ipred;
	int ipmode;                           /* intra pred mode */
	int nonzero;                          /* keep track of nonzero coefficients */
	int mb_nr = img->current_mb_nr;
	int x, y;

	int tot_intra_sad2;                   /* SAD for 16x16 intra pred modes */
	int intra_pred_mode_2;                /* best 16x16 intra mode */

	int ii,jj,ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;
	int i1,j1,i3,j3,m2,jg2;

	int interintra;                       /* distinction between inter or intra elements */
	int se;                               /* type of syntaxelement */

	/* Bit counting */
	int bits_mb;
	int bit_use_head_mode;
	int bit_use_coeffY;
	int bit_use_mode_inter;
	int tmp_bit_use_cbp;
	int bit_use_coeffC;

	int recode_macroblock;

	int loopb_store[6][6];
	int loopc_store[4][4];

	int use_bitstream_backing = (inp->slice_mode == 2 || inp->slice_mode == 3);

	/* Use function pointer so that the bits are stored instead of put out */
	/* when backing is in use */
	void (*put)(char *tracestring,int len,int info, int sliceno, int type);
	if(use_bitstream_backing)
		put = store_symbol;
	else
		put = put_symbol;

	/* Save loopb and loopc in the case of recoding this macroblock */
	if(use_bitstream_backing)
	{
		for(x = 0 ; x<6 ; x++) for(y = 0 ; y<6 ; y++)
				loopb_store[x][y] = loopb[img->block_x+x][img->block_y+y];
		for(x=0 ; x<4 ; x++) for(y=0 ; y<4 ; y++)
				loopc_store[x][y] = loopc[img->block_x/2+x][img->block_y/2+y];
	}


	/* recode_macroblock is used in slice mode two and three where */
	/* backing of one macroblock in the bitstream is possible      */
	recode_macroblock = 1;
	while(recode_macroblock)
	{
		recode_macroblock = 0;

		/* Counts the bits used for this macroblock */
		bit_use_head_mode = 0;
		bit_use_coeffY = 0;
		bit_use_mode_inter = 0;
		tmp_bit_use_cbp = 0;
		bit_use_coeffC = 0;

		/* Save the slice number of this macroblock. When the macroblock below     */
		/* is coded it will use this to decide if prediction for above is possible */
		img->slice_numbers[mb_nr] = img->current_slice_nr;

		/* If MB is next to a slice boundary, mark neighbor blocks unavailable for prediction */
		ResetSlicePredictions(img);

		/* Reset vectors before doing motion search in motion_search(). */
		for (k=0; k < 2; k++)
		{
			for (j=0; j < 4; j++)
				for (i=0; i < 4; i++)
					tmp_mv[k][img->block_y+j][img->block_x+i+4]=0;
		}

		/*
		start making intra prediction
		*/
		cbp=0;
		tot_intra_sad=QP2QUANT[img->qp]*24;/* sum of intra sad values, start with a 'handicap'*/
		for(block_y = 0 ; block_y < 16 ; block_y += 4)
		{
			for(block_x = 0 ; block_x < 16 ; block_x += 4)
			{

				pic_pix_y=img->pix_y+block_y;
				pic_block_y=pic_pix_y/BLOCK_SIZE;

				cbp_mask=(int)pow(2,((2*(block_y/8)+block_x/8)));


				pic_pix_x=img->pix_x+block_x;
				pic_block_x=pic_pix_x/BLOCK_SIZE;

				/*
				intrapred_luma() makes and returns 4x4 blocks with all 5 intra prediction modes.
				Notice that some modes are not possible at frame edges.
				*/
				intrapred_luma(img,pic_pix_x,pic_pix_y);


				best_intra_sad=MAX_VALUE; /* initial value, will be modified below                */
				img->imod = INTRA_MB_OLD;  /* for now mode set to intra, may be changed in motion_search() */

				for (ipmode=0; ipmode < NO_INTRA_PMODE; ipmode++)   /* all intra prediction modes */
				{
					/* Horizontal pred from Y neighbour pix , vertical use X pix, diagonal needs both */
					if (ipmode==DC_PRED||ipmode==HOR_PRED||img->ipredmode[pic_pix_x/BLOCK_SIZE+1][pic_pix_y/BLOCK_SIZE] >= 0)/* DC or vert pred or hor edge*/
					{
						if (ipmode==DC_PRED||ipmode==VERT_PRED||img->ipredmode[pic_pix_x/BLOCK_SIZE][pic_pix_y/BLOCK_SIZE+1] >= 0)/* DC or hor pred or vert edge*/
						{
							for (j=0; j < BLOCK_SIZE; j++)
							{
								for (i=0; i < BLOCK_SIZE; i++)
									img->m7[i][j]=imgY_org[pic_pix_y+j][pic_pix_x+i]-img->mprr[ipmode][j][i]; /* find diff */
							}
							current_intra_sad=QP2QUANT[img->qp]*PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][ipmode]*2;

							current_intra_sad += find_sad(img,inp->hadamard); /* add the start 'handicap' and the computed SAD */

							if (current_intra_sad < best_intra_sad)
							{
								best_intra_sad=current_intra_sad;
								best_ipmode=ipmode;

								for (j=0; j < BLOCK_SIZE; j++)
									for (i=0; i < BLOCK_SIZE; i++)
										img->mpr[i+block_x][j+block_y]=img->mprr[ipmode][j][i];       /* store the currently best intra prediction block*/
							}
						}
					}
				}
				tot_intra_sad += best_intra_sad;

				img->ipredmode[pic_block_x+1][pic_block_y+1]=best_ipmode;

				if (pic_block_x & 1 == 1) /* just even blocks, two and two predmodes are sent together */
				{
					intra_pred_modes[block_x/8+block_y/2]=IPRED_ORDER[last_ipred][PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][best_ipmode]];

				}
				last_ipred=PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][best_ipmode];

				/*  Make difference from prediction to be transformed*/
				for (j=0; j < BLOCK_SIZE; j++)
					for (i=0; i < BLOCK_SIZE; i++)
					{
						img->m7[i][j] =imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i] - img->mpr[i+block_x][j+block_y];
					}

				nonzero = dct_luma(block_x,block_y,&coeff_cost,img);

				if (nonzero)
				{
					cbp=cbp | cbp_mask;/* set coded block pattern if there are nonzero coeffs */
				}
			}
		}

		/* new 16x16 based routine */
		intrapred_luma_2(img);                                /* make intra pred for the new 4 modes */
		tot_intra_sad2 = find_sad2(img,&intra_pred_mode_2);   /* find best SAD for new modes */
		if (tot_intra_sad2<tot_intra_sad)
		{
			tot_intra_sad = tot_intra_sad2; /* update best intra sad if necessary */
			img->imod = INTRA_MB_NEW;       /* one of the new modes is used */
			dct_luma2(img,intra_pred_mode_2);
			for (i=0;i<4;i++)
				for (j=0;j<4;j++)
					img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;
		}

		if ((img->mb_y == img->mb_y_upd && img->mb_y_upd != img->mb_y_intra) || img->type == INTRA_IMG)
		{
			img->mb_mode=8*img->type+img->imod;  /* init value, set if intra image or if intra GOB for error robustness */
		}
		else
		{
			predframe_no=motion_search(inp,img,tot_intra_sad);
		}
		if (img->imod != INTRA_MB_OLD && img->imod != INTRA_MB_NEW)     /* inter */
		{
			cbp=0;
			sum_cnt_nonz=0;
			for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
			{
				for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
				{
					cbp_mask=(int)pow(2,(mb_x/8+mb_y/4));
					coeff_cost=0;
					for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE)
					{
						pic_pix_y=img->pix_y+block_y;
						pic_block_y=pic_pix_y/BLOCK_SIZE;

						for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE)
						{
							pic_pix_x=img->pix_x+block_x;
							pic_block_x=pic_pix_x/BLOCK_SIZE;

							img->ipredmode[pic_block_x+1][pic_block_y+1]=0;
							ii4=(img->pix_x+block_x)*4+tmp_mv[0][pic_block_y][pic_block_x+4];
							jj4=(img->pix_y+block_y)*4+tmp_mv[1][pic_block_y][pic_block_x+4];
							for (j=0;j<4;j++)
							{
								j2=j*4;
								for (i=0;i<4;i++)
								{
									i2=i*4;
									img->mpr[i+block_x][j+block_y]=mref[img->multframe_no][jj4+j2][ii4+i2];
								}
							}

							for (j=0; j < BLOCK_SIZE; j++)
							{
								for (i=0; i < BLOCK_SIZE; i++)
								{
									img->m7[i][j] =imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i] - img->mpr[i+block_x][j+block_y];
								}
							}
							nonzero=dct_luma(block_x,block_y,&coeff_cost,img);
							if (nonzero)
							{
								cbp=cbp | cbp_mask;
							}
						}
					}

					/*
					The purpose of the action below is to prevent that single or 'expensive' coefficients are coded.
					With 4x4 transform there is larger chance that a single coefficient in a 8x8 or 16x16 block may be nonzero.
					A single small (level=1) coefficient in a 8x8 block will cost: 3 or more bits for the coefficient,
					4 bits for EOBs for the 4x4 blocks,possibly also more bits for CBP.  Hence the total 'cost' of that single
					coefficient will typically be 10-12 bits which in a RD consideration is too much to justify the distortion improvement.
					The action below is to watch such 'single' coefficients and set the reconstructed block equal to the prediction according
					to a given criterium.  The action is taken only for inter luma blocks.

					  Notice that this is a pure encoder issue and hence does not have any implication on the standard.
					  coeff_cost is a parameter set in dct_luma() and accumulated for each 8x8 block.  If level=1 for a coefficient,
					  coeff_cost is increased by a number depending on RUN for that coefficient.The numbers are (see also dct_luma()): 3,2,2,1,1,1,0,0,...
					  when RUN equals 0,1,2,3,4,5,6, etc.
					  If level >1 coeff_cost is increased by 9 (or any number above 3). The threshold is set to 3. This means for example:
					  1: If there is one coefficient with (RUN,level)=(0,1) in a 8x8 block this coefficient is discarded.
					  2: If there are two coefficients with (RUN,level)=(1,1) and (4,1) the coefficients are also discarded
					  sum_cnt_nonz is the accumulation of coeff_cost over a whole macro block.  If sum_cnt_nonz is 5 or less for the whole MB,
					  all nonzero coefficients are discarded for the MB and the reconstructed block is set equal to the prediction.
					*/
					if (coeff_cost > 3)
					{
						sum_cnt_nonz += coeff_cost;
					}
					else /*discard */
					{
						cbp=cbp & (63-cbp_mask);
						for (i=mb_x; i < mb_x+BLOCK_SIZE*2; i++)
						{
							for (j=mb_y; j < mb_y+BLOCK_SIZE*2; j++)
							{
								imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
							}
						}
					}
				}
			}
			if (sum_cnt_nonz <= 5 )
			{
				cbp=cbp & 48; /* mask bit 4 and 5 */
				for (i=0; i < MB_BLOCK_SIZE; i++)
				{
					for (j=0; j < MB_BLOCK_SIZE; j++)
					{
						imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
					}
				}
			}
		}

		/*     4x4 chroma blocks */

		cr_cbp=0;

		for (uv=0; uv < 2; uv++)
		{
			if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
			{
				intrapred_chroma(img,img->pix_c_x,img->pix_c_y,uv);
			}
			else /* inter MB*/
			{
				for (j=0; j < MB_BLOCK_SIZE/2; j++)
				{
					pic_block_y=(img->pix_c_y+j)/2;
					for (i=0; i < MB_BLOCK_SIZE/2; i++)
					{
						pic_block_x=(img->pix_c_x+i)/2;
						ii=(img->pix_c_x+i)*8+tmp_mv[0][pic_block_y][pic_block_x+4];
						jj=(img->pix_c_y+j)*8+tmp_mv[1][pic_block_y][pic_block_x+4];
						ii0=ii/8;
						jj0=jj/8;
						ii1=(ii+7)/8;
						jj1=(jj+7)/8;
						if1=(ii & 7);
						jf1=(jj & 7);
						if0=8-if1;
						jf0=8-jf1;
						img->mpr[i][j]=(if0*jf0*mcef[img->multframe_no][uv][jj0][ii0]+
						                if1*jf0*mcef[img->multframe_no][uv][jj0][ii1]+
						                if0*jf1*mcef[img->multframe_no][uv][jj1][ii0]+
						                if1*jf1*mcef[img->multframe_no][uv][jj1][ii1]+32)/64;
						img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j];
					}
				}
			}
			cr_cbp=dct_chroma(uv,cr_cbp,img);
		}
		cbp += cr_cbp*16;

		/* set new intra mode and make "intra CBP", new in TML3*/
		if (img->imod==INTRA_MB_NEW)
			img->mb_mode += intra_pred_mode_2 + 4*cr_cbp + 12*img->kac;


		/*  Check if a MB is not coded (no coeffs. only 0-vectors and prediction from the most recent frame) */

		if (img->mb_mode==M16x16_MB && img->imod==INTRA_MB_INTER && cbp==0 && tmp_mv[0][img->block_y][img->block_x+4]==0
		        && tmp_mv[1][img->block_y][img->block_x+4]==0 && predframe_no==0)
		{
			img->mb_mode=COPY_MB;
		}

		/* Set the reference frame information for motion vector prediction */

		if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
		{
			for (j = 0;j < 4;j++)
			{
				for (i = 0;i < 4;i++)
				{
					refFrArr[img->block_y+j][img->block_x+i] = -1;
				}
			}
		}
		else
		{
			for (j = 0;j < 4;j++)
			{
				for (i = 0;i < 4;i++)
				{
					refFrArr[img->block_y+j][img->block_x+i] = predframe_no;
				}
			}
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
		else /* inter  */
		{
			for (i=0;i<4;i++)
			{
				ii=img->block_x+i;
				i3=ii/2;
				for (j=0;j<4;j++)
				{
					jj=img->block_y+j;
					j3=jj/2;

					if (((tmp_mv[0][jj][ii+4]/4!=tmp_mv[0][jj][ii-1+4]/4)||(tmp_mv[1][jj][ii+4]/4!=tmp_mv[1][jj][ii-1+4]/4)))
					{
						loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
						loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
						/*          if (img->block_x>0 )*/ /* CIF mismatch */
						/* fix from ver 4.1 */
						loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
						loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
					}
					if (((tmp_mv[0][jj][ii+4]/4!=tmp_mv[0][jj-1][ii+4]/4)||(tmp_mv[1][jj][ii+4]/4!=tmp_mv[1][jj-1][ii+4]/4)))
					{
						loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],1);
						loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
						/*if (img->block_x>0 )*/ /* CIF mismatch */
						/* fix from ver 4.1 */
						loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
						loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
					}
				}
			}
		}


		/*  Bit file prouction (and collectiong statistics) for one macro block */

		/* Notify the NAL layer if a new slice is starting here*/
		if(mb_nr > 0)
		{
			if(img->slice_numbers[mb_nr] != img->slice_numbers[mb_nr-1])
			{
				len = nal_put_startcode(img->number*(inp->jumpd+1) % 256, mb_nr, img->qp, inp->img_format,img->current_slice_nr, SE_HEADER);
				bit_use_head_mode+= len;
			}
		}

		/*  Bits for mode.  This does not apply for intra frames*/
		n_linfo(img->mb_mode,&len,&info);
		sprintf(tracestring, "MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y,img->mb_mode);
		put(tracestring,len,info,img->current_slice_nr,SE_MBTYPE);
		bit_use_head_mode += len;

		/*  Do nothing more if copy and inter mode  */
		if (img->mb_mode != COPY_MB || img->imod != INTRA_MB_INTER)
		{

			/*  Bits for intra prediction modes*/

			if (img->imod == INTRA_MB_OLD)
			{
				for (i=0; i < MB_BLOCK_SIZE/2; i++)
				{
					n_linfo(intra_pred_modes[i],&len,&info);
					sprintf(tracestring, "Intra mode     = %3d",intra_pred_modes[i]);
					put(tracestring,len,info,img->current_slice_nr,SE_INTRAPREDMODE);
					bit_use_coeffY += len;
				}
			}

			/*  Bits for vector data*/

			if (img->imod == INTRA_MB_INTER) /* inter */
			{
				/*  If multipple prediction mode, write reference frame for the MB */
				if (inp->no_multpred > 1)
				{
					n_linfo(predframe_no,&len,&info);
					sprintf(tracestring, "Reference frame no %d", predframe_no);
					put(tracestring,len,info,img->current_slice_nr,SE_REFFRAME);
					bit_use_mode_inter += len;
				}

				step_h=img->blc_size_h/BLOCK_SIZE;      /* horizontal stepsize */
				step_v=img->blc_size_v/BLOCK_SIZE;      /* vertical stepsize */

				for (j=0; j < BLOCK_SIZE; j += step_v)
				{
					for (i=0;i < BLOCK_SIZE; i += step_h)
					{
						for (k=0; k < 2; k++)
						{
							mvd_linfo(tmp_mv[k][img->block_y+j][img->block_x+i+4]-img->mv[i][j][predframe_no][img->mb_mode][k],&len,&info);
							sprintf(tracestring, " MVD(%d) = %3d",k, tmp_mv[k][img->block_y+j][img->block_x+i+4]-img->mv[i][j][predframe_no][img->mb_mode][k]);
							put(tracestring,len,info,img->current_slice_nr,SE_MVD);
							bit_use_mode_inter += len;
						}
					}
				}
			}

			interintra = inter_intra(img);  /* get type of current slice/picture */

			/*  Bits for CBP */
			if (img->imod != INTRA_MB_NEW)
			{
				sprintf(tracestring, "CBP (%2d,%2d) = %3d",img->mb_x, img->mb_y, cbp);
				n_linfo(NCBP[cbp][img->imod/INTRA_MB_INTER],&len,&info);

				if ( interintra == INTRA_CODED_MB )   /* check for Inter or Intra CBP syntax element */
					put(tracestring,len,info,img->current_slice_nr,SE_CBP_INTRA);
				else
					put(tracestring,len,info,img->current_slice_nr,SE_CBP_INTER);

				tmp_bit_use_cbp += len;

				/*  Bits for luma coefficients */
				for (mb_y=0; mb_y < 4; mb_y += 2)
				{
					for (mb_x=0; mb_x < 4; mb_x += 2)
					{
						for (j=mb_y; j < mb_y+2; j++)
						{
							jj=j/2;
							for (i=mb_x; i < mb_x+2; i++)
							{
								ii=i/2;
								if ((cbp & (int)pow(2,ii+jj*2)) != 0)               /* check for any coeffisients */
								{
									if (img->imod == INTRA_MB_OLD && img->qp < 24)  /* double scan */
									{
										for(kk=0;kk<2;kk++)
										{
											kbeg=kk*9;
											kend=kbeg+8;
											level=1; /* get inside loop */

											for(k=kbeg;k <= kend && level !=0; k++)
											{
												level=img->cof[i][j][k][0][DOUBLE_SCAN];
												run=  img->cof[i][j][k][1][DOUBLE_SCAN];
												levrun_linfo_intra(level,run,&len,&info);
												sprintf(tracestring, "Luma dbl(%2d,%2d)  level=%3d Run=%2d",kk,k,level,run);

												if (k == kbeg)
												{  /* element is of type DC */
													if (interintra == INTER_CODED_MB)
														se = SE_LUM_DC_INTER;
													else
														se = SE_LUM_DC_INTRA;
												}
												else
												{  /* element is of type AC */
													if (interintra == INTER_CODED_MB)
														se = SE_LUM_AC_INTER;
													else
														se = SE_LUM_AC_INTRA;
												}
												put(tracestring,len,info,img->current_slice_nr,se);

												bit_use_coeffY += len;

												if (level!=0)
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
									else                                            /* single scan */
									{
										level=1; /* get inside loop */
										for(k=0;k<=16 && level !=0; k++)
										{
											level=img->cof[i][j][k][0][SINGLE_SCAN];
											run=  img->cof[i][j][k][1][SINGLE_SCAN];
											levrun_linfo_inter(level,run,&len,&info);
											sprintf(tracestring, "Luma sng(%2d) level =%3d run =%2d", k, level,run);

											if (k == 0)
											{                               /* element is of type DC */
												if (interintra == INTER_CODED_MB)
													se = SE_LUM_DC_INTER;
												else
													se = SE_LUM_DC_INTRA;
											}
											else
											{                                      /* element is of type AC */
												if (interintra == INTER_CODED_MB)
													se = SE_LUM_AC_INTER;
												else
													se = SE_LUM_AC_INTRA;
											}
											put(tracestring,len,info,img->current_slice_nr,se);

											bit_use_coeffY += len;
											if (level!=0)
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
			else /* 16x16 based intra modes */
			{
				/* DC coeffs */
				level=1; /* get inside loop */
				for (k=0;k<=16 && level !=0;k++)
				{
					level=img->cof[0][0][k][0][1];
					run = img->cof[0][0][k][1][1];
					levrun_linfo_inter(level,run,&len,&info);
					sprintf(tracestring, "DC luma 16x16 sng(%2d) level =%3d run =%2d", k, level, run);
					put(tracestring,len,info,img->current_slice_nr,SE_LUM_DC_INTRA);
					bit_use_coeffY += len;
				}
				/* AC coeffs */
				if (img->kac==1)
				{
					for (mb_y=0; mb_y < 4; mb_y += 2)
					{
						for (mb_x=0; mb_x < 4; mb_x += 2)
						{
							for (j=mb_y; j < mb_y+2; j++)
							{
								for (i=mb_x; i < mb_x+2; i++)
								{
									level=1; /* get inside loop */
									for (k=0;k<16 && level !=0;k++)
									{
										level=img->cof[i][j][k][0][SINGLE_SCAN];
										run=  img->cof[i][j][k][1][SINGLE_SCAN];
										levrun_linfo_inter(level,run,&len,&info);
										sprintf(tracestring, "AC luma 16x16 sng(%2d) level =%3d run =%2d", k, level, run);
										put(tracestring,len,info,img->current_slice_nr,SE_LUM_AC_INTRA);
										bit_use_coeffY += len;
									}
								}
							}
						}
					}
				}
			}

			/*  Bits for chroma 2x2 DC transform coefficients */

			m2=img->mb_x*2;
			jg2=img->mb_y*2;

			if (cbp > 15)  /* check if any chroma bits in coded block pattern is set */
			{
				for (uv=0; uv < 2; uv++)
				{

					level=1;
					for (k=0; k < 5 && level != 0; ++k)
					{

						level=img->cofu[k][0][uv];
						levrun_linfo_c2x2(level,img->cofu[k][1][uv],&len,&info);
						sprintf(tracestring, "2x2 DC Chroma %2d: level =%3d run =%2d",k, level, img->cofu[k][1][uv]);

						if ( interintra == INTER_CODED_MB )
							se = SE_CHR_DC_INTER;
						else
							se = SE_CHR_DC_INTRA;
						put(tracestring,len,info,img->current_slice_nr,se);
						bit_use_coeffC += len;

						if (level != 0)/* fix from ver 4.1 */
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
			}

			/*  Bits for chroma AC-coeffs. */

			if (cbp >> 4 == 2) /* chech if chroma bits in coded block pattern = 10b */
			{
				for (mb_y=4; mb_y < 6; mb_y += 2)
				{
					for (mb_x=0; mb_x < 4; mb_x += 2)
					{

						for (j=mb_y; j < mb_y+2; j++)
						{
							jj=j/2;
							j1=j-4;
							for (i=mb_x; i < mb_x+2; i++)
							{
								ii=i/2;
								i1=i%2;
								level=1;
								for (k=0; k < 16 && level != 0; k++)
								{
									level=img->cof[i][j][k][0][0];
									levrun_linfo_inter(level,img->cof[i][j][k][1][0],&len,&info);
									sprintf(tracestring, "AC Chroma %2d: level =%3d run =%2d",k, level, img->cof[i][j][k][1][0]);

									if ( interintra == INTER_CODED_MB )
										se = SE_CHR_AC_INTER;
									else
										se = SE_CHR_AC_INTRA;
									put(tracestring,len,info,img->current_slice_nr,se);
									bit_use_coeffC += len;

									if (level != 0)
									{
										loopc[m2+i1+1][jg2+j1+1]=max(loopc[m2+i1+1][jg2+j1+1],2);

										loopc[m2+i1  ][jg2+j1+1]=max(loopc[m2+i1  ][jg2+j1+1],1);
										loopc[m2+i1+1][jg2+j1  ]=max(loopc[m2+i1+1][jg2+j1  ],1);
										loopc[m2-i1+2][jg2+j1+1]=max(loopc[m2-i1+2][jg2+j1+1],1);
										loopc[m2+i1+1][jg2+j1+2]=max(loopc[m2+i1+1][jg2+j1+2],1);
									}
								}
							}
						}
					}
				}
			}
		}
		bits_mb = bit_use_head_mode + bit_use_coeffY + bit_use_mode_inter + tmp_bit_use_cbp + bit_use_coeffC;
		stat->bit_slice += bits_mb;

		/* For slice mode one, check if a new slice boundry follows */
		if(inp->slice_mode == 1)
		{
			if((mb_nr+1) % inp->slice_argument == 0)
			{
				img->current_slice_nr++;
			}
		}

		/* For slice modes two and three, check if coding of this macroblock */
		/* resulted in too many bits for this slice. If so, indicate slice   */
		/* boundary before this macroblock and code the macroblock again     */
		if(inp->slice_mode == 2)
		{
			if(mb_nr > 0 && img->slice_numbers[mb_nr-1] == img->current_slice_nr)
				if(stat->bit_slice > inp->slice_argument*8)
					recode_macroblock = 1;
		}
		if(inp->slice_mode == 3)
		{
			if(mb_nr > 0 && img->slice_numbers[mb_nr-1] == img->current_slice_nr)
				if(nal_slice_too_big(stat->bit_slice))
					recode_macroblock = 1;
		}
		if(recode_macroblock)
		{
			img->current_slice_nr++;
			flush_macroblock_symbols();
			stat->bit_slice = 0;

			/* Restore loopb and loopc before coding the MB again*/
			for(x = 0 ; x<6 ; x++) for(y = 0 ; y<6 ; y++)
					loopb[img->block_x+x][img->block_y+y] = loopb_store[x][y];
			for(x=0 ; x<4 ; x++) for(y=0 ; y<4 ; y++)
					loopc[img->block_x/2+x][img->block_y/2+y] = loopc_store[x][y];
		}
	}

	/* The final coding of the macroblock has been done */

	/* Send the stored symbols generated for this macroblock to the NAL layer */
	if(use_bitstream_backing)
		put_stored_symbols();

	/* Update the statistics */
	stat->bit_use_head_mode[img->type] += bit_use_head_mode;
	stat->bit_use_coeffY[img->type] += bit_use_coeffY;
	stat->bit_use_mode_inter[img->type] += bit_use_mode_inter;
	stat->tmp_bit_use_cbp[img->type] += tmp_bit_use_cbp;
	stat->bit_use_coeffC[img->type] += bit_use_coeffC;
	stat->bit_ctr += bits_mb;
	if (img->type==INTRA_IMG)
		++stat->mode_use_intra[img->mb_mode];    /* statistics */
	else
		++stat->mode_use_inter[img->mb_mode];

}


//<pu added 10/6/2000>
/************************************************************************
*
* Name:         inter_intra(struct img_par *img)
*
* Description:  returns mode of actual macroblock if inter or intra coded
*
************************************************************************/

int inter_intra(struct img_par *img)
{
	if ( img->imod == INTRA_MB_OLD )
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
//<pu>

