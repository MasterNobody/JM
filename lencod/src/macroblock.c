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
// Detlev Marpe                    <marpe@hhi.de>
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
*  Name :       proceed2nextMacroblock()
*
*  Description: updates the coordinates and statistics parameter for the 
*				next macroblock
*
************************************************************************/
void proceed2nextMacroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat)
{
	
	int use_bitstream_backing = (inp->slice_mode == FIXED_RATE || inp->slice_mode == CALLBACK);
	const int number_mb_per_row = img->width / MB_BLOCK_SIZE ;
	Macroblock *currMB = &img->mb_data[img->current_mb_nr];

#if TRACE
	int i;
	if(use_bitstream_backing)
		fprintf(p_trace, "\n*********** Pic: %i (I/P) MB: %i Slice: %i **********\n\n", frame_no, img->current_mb_nr, img->current_slice_nr);
	/* Write out the tracestring for each symbol */
	for (i=0; i<currMB->currSEnr; i++)
		trace2out(&(img->MB_SyntaxElements[i]));
#endif
	
	/* Update the statistics */
	stat->bit_use_head_mode[img->type]		+= currMB->bitcounter[BITS_HEADER_MB];
	stat->bit_use_coeffY[img->type]				+= currMB->bitcounter[BITS_COEFF_Y_MB] ;
	stat->bit_use_mode_inter[img->mb_mode]+= currMB->bitcounter[BITS_INTER_MB];
	stat->tmp_bit_use_cbp[img->type]			+= currMB->bitcounter[BITS_CBP_MB];
	stat->bit_use_coeffC[img->type]				+= currMB->bitcounter[BITS_COEFF_UV_MB];

	if (inp->symbol_mode == UVLC)
		stat->bit_ctr += currMB->bitcounter[BITS_TOTAL_MB];
	if (img->type==INTRA_IMG)
		++stat->mode_use_intra[img->mb_mode];   
	else
		if (img->type != B_IMG)
			++stat->mode_use_inter[img->mb_mode];
		else
			++stat->mode_use_Bframe[img->mb_mode];
	
	/* Update coordinates of macroblock */
	img->mb_x++;
	if (img->mb_x == number_mb_per_row) /* next row of MBs */
	{
		img->mb_x = 0; /* start processing of next row */
		img->mb_y++;
	}
	img->current_mb_nr++;
	

	/* Define vertical positions */
	img->block_y = img->mb_y * BLOCK_SIZE;      /* vertical luma block position */
	img->pix_y   = img->mb_y * MB_BLOCK_SIZE;   /* vertical luma macroblock position */
	img->pix_c_y = img->mb_y * MB_BLOCK_SIZE/2; /* vertical chroma macroblock position */

	if (img->type != B_IMG)
		if (inp->intra_upd > 0 && img->mb_y <= img->mb_y_intra)
			img->height_err=(img->mb_y_intra*16)+15;     /* for extra intra MB */
		else
			img->height_err=img->height-1;
	
	/* Define horizontal positions */
	img->block_x = img->mb_x * BLOCK_SIZE;        /* luma block           */
	img->pix_x   = img->mb_x * MB_BLOCK_SIZE;     /* luma pixel           */
	img->block_c_x = img->mb_x * BLOCK_SIZE/2;    /* chroma block         */
	img->pix_c_x   = img->mb_x * MB_BLOCK_SIZE/2; /* chroma pixel         */
	
	/* Statistics */
	if (img->type == INTER_IMG)
	{
		++stat->quant0;
		stat->quant1 += img->qp;      /* to find average quant for inter frames */
	}
}

/************************************************************************
*
*  Name :       start_macroblock()
*
*  Description: initializes the current macroblock
*
************************************************************************/
void start_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat)
{
	int i,j,k,l;
	int x,y;
	int use_bitstream_backing = (inp->slice_mode == FIXED_RATE || inp->slice_mode == CALLBACK);
	Macroblock *currMB = &img->mb_data[img->current_mb_nr];
	Slice *curr_slice = img->currentSlice;
	DataPartition *dataPart;
	Bitstream *currStream;

	if(use_bitstream_backing)
	{
		/* Save loopb and loopc in the case of recoding this macroblock */
		for(x = 0 ; x<6 ; x++) 
			for(y = 0 ; y<6 ; y++)
				img->tmp_loop_Y[x][y] = loopb[img->block_x+x][img->block_y+y];
		for(x=0 ; x<4 ; x++) 
			for(y=0 ; y<4 ; y++)
				img->tmp_loop_UV[x][y] = loopc[img->block_x/2+x][img->block_y/2+y];

		/* Keep the current state of the bitstreams */
		for (i=0; i<curr_slice->max_part_nr; i++)
		{
			dataPart = &(curr_slice->partArr[i]);
			currStream = dataPart->bitstream;
			currStream->stored_bits_to_go = currStream->bits_to_go;
			currStream->stored_byte_pos		= currStream->byte_pos;
			currStream->stored_byte_buf		= currStream->byte_buf;
		}
	}
	
	/* Save the slice number of this macroblock. When the macroblock below     */
	/* is coded it will use this to decide if prediction for above is possible */
	img->slice_numbers[img->current_mb_nr] = img->current_slice_nr;

	/* Save the slice and macroblock number of the current MB */
	currMB->slice_nr = img->current_slice_nr;

	/* Initialize counter for MB symbols */
	currMB->currSEnr=0;

	/* If MB is next to a slice boundary, mark neighboring blocks unavailable for prediction */
	CheckAvailabilityOfNeighbors(img);

	/* Reset vectors before doing motion search in motion_search(). */
	if (img->type != B_IMG)
	{
		for (k=0; k < 2; k++)
		{
			for (j=0; j < BLOCK_MULTIPLE; j++)
				for (i=0; i < BLOCK_MULTIPLE; i++)
					tmp_mv[k][img->block_y+j][img->block_x+i+4]=0;
		}
	}

	/* Reset syntax element entries in MB struct */
	currMB->mb_type = 0;
	currMB->ref_frame = 0;
	currMB->cbp = 0;

	for (l=0; l < 2; l++)
		for (j=0; j < BLOCK_MULTIPLE; j++)
			for (i=0; i < BLOCK_MULTIPLE; i++)
				for (k=0; k < 2; k++)
					currMB->mvd[l][j][i][k] = 0;

	for (i=0; i < (BLOCK_MULTIPLE*BLOCK_MULTIPLE); i++)
		currMB->intra_pred_modes[i] = 0;


	/* Initialize bitcounters for this macroblock */
	if(img->current_mb_nr == 0) /* No slice header to account for */
	{
		currMB->bitcounter[BITS_HEADER_MB] = 0;
	}
	else
		if (img->slice_numbers[img->current_mb_nr] == img->slice_numbers[img->current_mb_nr-1]) /* current MB belongs to the */
																																														/* same slice as the last MB */
		{
			currMB->bitcounter[BITS_HEADER_MB] = 0;
		}

	currMB->bitcounter[BITS_COEFF_Y_MB] = 0;
	currMB->bitcounter[BITS_INTER_MB] = 0;
	currMB->bitcounter[BITS_CBP_MB] = 0;
	currMB->bitcounter[BITS_COEFF_UV_MB] = 0;

}

/************************************************************************
*
*  Name :       terminate_macroblock()
*
*  Description: terminates processing of the current macroblock depending
*								on the chosen slice mode
*
************************************************************************/
void terminate_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat,
													Boolean *end_of_slice, Boolean *recode_macroblock)
{
	int i,x,y;
	int mb_nr = img->current_mb_nr;
	const int total_number_mb = (img->width * img->height)/(MB_BLOCK_SIZE*MB_BLOCK_SIZE);
	Slice *curr_slice = img->currentSlice;
	DataPartition *dataPart;
	Bitstream *currStream;

	switch(inp->slice_mode)
	{
		case NO_SLICES:
			*recode_macroblock = FALSE;
			if ((mb_nr+1) == total_number_mb) /* maximum number of MBs */
				*end_of_slice = TRUE; 
			break;
		case FIXED_MB:
			/* For slice mode one, check if a new slice boundary follows */
			*recode_macroblock = FALSE;
			if ( ((mb_nr+1) % inp->slice_argument == 0) || ((mb_nr+1) == total_number_mb) )
			{
				*end_of_slice = TRUE; 
			}
			break;

		/* For slice modes two and three, check if coding of this macroblock */
		/* resulted in too many bits for this slice. If so, indicate slice   */
		/* boundary before this macroblock and code the macroblock again     */
		case FIXED_RATE:
			if (mb_nr > 0 && img->mb_data[mb_nr-1].slice_nr == img->current_slice_nr)
			{
				if (stat->bit_slice > inp->slice_argument*8)
				{
					//printf("MB: %d\tBits: %d\n",mb_nr,stat->bit_slice);
					*recode_macroblock = TRUE;
					*end_of_slice = TRUE;
				} 
			} 
			if ( (*recode_macroblock == FALSE) && ((mb_nr+1) == total_number_mb) )	/* maximum number of MBs */
				*end_of_slice = TRUE; 
			break;
		case	CALLBACK:
			if (mb_nr > 0 && img->mb_data[mb_nr-1].slice_nr == img->current_slice_nr)
			{
				if (curr_slice->slice_too_big(stat->bit_slice))
				{
					*recode_macroblock = TRUE;
					*end_of_slice = TRUE;
				}
			}
			if ( (*recode_macroblock == FALSE) && ((mb_nr+1) == total_number_mb) )	/* maximum number of MBs */
				*end_of_slice = TRUE; 
			break;
		default:
			fprintf(stderr, "Slice Mode %d not supported", inp->slice_mode);
			exit(1);
	}
		
	if(*recode_macroblock == TRUE)
	{
		/* Restore the state of the bitstreams */
		for (i=0; i<curr_slice->max_part_nr; i++)
		{
			dataPart = &(curr_slice->partArr[i]);
			currStream = dataPart->bitstream;
			currStream->bits_to_go	= currStream->stored_bits_to_go;
			currStream->byte_pos		=	currStream->stored_byte_pos;
			currStream->byte_buf		= currStream->stored_byte_buf;
		}

		/* Restore loopb and loopc before coding the MB again*/
		for(x = 0 ; x<6 ; x++) for(y = 0 ; y<6 ; y++)
			loopb[img->block_x+x][img->block_y+y] = img->tmp_loop_Y[x][y];
		for(x=0 ; x<4 ; x++) for(y=0 ; y<4 ; y++)
			loopc[img->block_x/2+x][img->block_y/2+y] = img->tmp_loop_UV[x][y];
	}

}

/************************************************************************
*
*  Name :       CheckAvailabilityOfNeighbors(struct img_par *img)
*
*  Description: Checks the availability of neighboring macroblocks of 
*								the current macroblock for prediction and context determination;
*								marks the unavailable MBs for intra prediction in the 
*								ipredmode-array by -1. Only neighboring MBs in the causal 
*								past of the current MB are checked.
*
************************************************************************/
void CheckAvailabilityOfNeighbors(struct img_par *img)
{
	int i,j;
	const int mb_width = img->width/MB_BLOCK_SIZE;
	const int mb_nr = img->current_mb_nr;
	Macroblock *currMB = &img->mb_data[mb_nr];

	/* mark all neighbors as unavailable */
	for (i=0; i<3; i++)
		for (j=0; j<3; j++)
			img->mb_data[mb_nr].mb_available[i][j]=NULL;
	img->mb_data[mb_nr].mb_available[1][1]=currMB; /* current MB */

	/* Check MB to the left */
	if(img->pix_x >= MB_BLOCK_SIZE)
	{
		if(currMB->slice_nr != img->mb_data[mb_nr-1].slice_nr)
		{
			img->ipredmode[img->block_x][img->block_y+1] = -1;
			img->ipredmode[img->block_x][img->block_y+2] = -1;
			img->ipredmode[img->block_x][img->block_y+3] = -1;
			img->ipredmode[img->block_x][img->block_y+4] = -1;
		} else
				currMB->mb_available[1][0]=&(img->mb_data[mb_nr-1]);
	} 


	/* Check MB above */
	if(img->pix_y >= MB_BLOCK_SIZE)
	{
		if(currMB->slice_nr != img->mb_data[mb_nr-mb_width].slice_nr)
		{
			img->ipredmode[img->block_x+1][img->block_y] = -1;
			img->ipredmode[img->block_x+2][img->block_y] = -1;
			img->ipredmode[img->block_x+3][img->block_y] = -1;
			img->ipredmode[img->block_x+4][img->block_y] = -1;
		} else
				currMB->mb_available[0][1]=&(img->mb_data[mb_nr-mb_width]);
	}

	/* Check MB left above */
	if(img->pix_x >= MB_BLOCK_SIZE && img->pix_y  >= MB_BLOCK_SIZE )
	{
		if(currMB->slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr)
			img->mb_data[mb_nr].mb_available[0][0]=&(img->mb_data[mb_nr-mb_width-1]);
	}

	/* Check MB right above */
	if(img->pix_y >= MB_BLOCK_SIZE && img->pix_x < (img->width-MB_BLOCK_SIZE ))
	{
		if(currMB->slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr)
			currMB->mb_available[0][1]=&(img->mb_data[mb_nr-mb_width+1]);
	}
}

/************************************************************************
*
*  Name :       MakeIntraPrediction()
*
*  Description: Performs 4x4 and 16x16 intra prediction and transform coding
*								of the prediction residue. The routine returns the best cost; 
*								current cbp (for LUMA only) and intra pred modes are affected 
*
************************************************************************/
int MakeIntraPrediction(struct img_par *img,struct inp_par *inp, int *intra_pred_mode_2)
{

	int i,j;
	int block_x, block_y;
	int best_ipmode;
	int tot_intra_sad, tot_intra_sad2, best_intra_sad, current_intra_sad;
	int coeff_cost; // not used
	int pic_pix_y,pic_pix_x,pic_block_y,pic_block_x;
	int last_ipred;								        /* keeps last chosen intra prediction mode for 4x4 intra pred */
	int ipmode;                           /* intra pred mode */
	int nonzero;                          /* keep track of nonzero coefficients */
	int cbp_mask;
	Macroblock *currMB = &img->mb_data[img->current_mb_nr];

	/* start making 4x4 intra prediction */
	currMB->cbp=0;
	img->mb_data[img->current_mb_nr].intraOrInter = INTRA_MB_4x4;

	tot_intra_sad=QP2QUANT[img->qp]*24;/* sum of intra sad values, start with a 'handicap'*/
		
	for(block_y = 0 ; block_y < MB_BLOCK_SIZE ; block_y += BLOCK_MULTIPLE)
	{

		pic_pix_y=img->pix_y+block_y;
		pic_block_y=pic_pix_y/BLOCK_SIZE;

		for(block_x = 0 ; block_x < MB_BLOCK_SIZE  ; block_x += BLOCK_MULTIPLE)
		{

	
			cbp_mask=(1<<(2*(block_y/8)+block_x/8));


			pic_pix_x=img->pix_x+block_x;
			pic_block_x=pic_pix_x/BLOCK_SIZE;

			/*
			intrapred_luma() makes and returns 4x4 blocks with all 5 intra prediction modes.
			Notice that some modes are not possible at frame edges.
			*/
			intrapred_luma(img,pic_pix_x,pic_pix_y);


			best_intra_sad=MAX_VALUE; /* initial value, will be modified below                */
			img->imod = INTRA_MB_OLD;  /* for now mode set to intra, may be changed in motion_search() */
			/* DM: has to be removed */

			for (ipmode=0; ipmode < NO_INTRA_PMODE; ipmode++)   /* all intra prediction modes */
			{
				/* Horizontal pred from Y neighbour pix , vertical use X pix, diagonal needs both */
				if (ipmode==DC_PRED||ipmode==HOR_PRED||img->ipredmode[pic_block_x+1][pic_block_y] >= 0)/* DC or vert pred or hor edge*/
				{
					if (ipmode==DC_PRED||ipmode==VERT_PRED||img->ipredmode[pic_block_x][pic_block_y+1] >= 0)/* DC or hor pred or vert edge*/
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
				currMB->intra_pred_modes[block_x/4+block_y]=PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][best_ipmode];
				currMB->intra_pred_modes[block_x/4-1+block_y]=last_ipred;
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
					currMB->cbp |= cbp_mask;/* set coded block pattern if there are nonzero coeffs */
			}
		}
	}

	/* 16x16 intra prediction */
	intrapred_luma_2(img);                                /* make intra pred for the new 4 modes */
	tot_intra_sad2 = find_sad2(img,intra_pred_mode_2);   /* find best SAD for new modes */
		
	if (tot_intra_sad2<tot_intra_sad)
	{
		currMB->cbp = 0;												/* cbp for 16x16 LUMA is signaled by the MB-mode */
		tot_intra_sad = tot_intra_sad2; /* update best intra sad if necessary */
		img->imod = INTRA_MB_NEW;       /* one of the new modes is used */
		img->mb_data[img->current_mb_nr].intraOrInter = INTRA_MB_16x16;
		dct_luma2(img,*intra_pred_mode_2);
		for (i=0;i<4;i++)
			for (j=0;j<4;j++)
				img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;
	}
	return tot_intra_sad;
}

/************************************************************************
*
*  Name :       LumaResidualCoding_P()
*
*  Description: Performs DCT, R-D constrained quantization, run/level 
*								pre-coding and IDCT for the MC-compensated MB residue  
*								of P-frame; current cbp (for LUMA only) is affected 
*
************************************************************************/
void LumaResidualCoding_P(struct img_par *img,struct inp_par *inp)
{

	int i,j;
	int block_x, block_y;
	int pic_pix_y,pic_pix_x,pic_block_y,pic_block_x;
	int ii4,i2,jj4,j2;
	int sum_cnt_nonz;
	int mb_x, mb_y;
	int cbp_mask;
	int coeff_cost;
	int nonzero;
	Macroblock *currMB = &img->mb_data[img->current_mb_nr];


	currMB->cbp=0;
	sum_cnt_nonz=0;
	for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2)
	{
		for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2)
		{
			cbp_mask=(1<<(mb_x/8+mb_y/4));
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
						currMB->cbp |= cbp_mask;
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
				currMB->cbp &= (63-cbp_mask);
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
		currMB->cbp &= 48; /* mask bit 4 and 5 */
		for (i=0; i < MB_BLOCK_SIZE; i++)
		{
			for (j=0; j < MB_BLOCK_SIZE; j++)
			{
				imgY[img->pix_y+j][img->pix_x+i]=img->mpr[i][j];
			}
		}
	}
}

/************************************************************************
*
*  Name :       ChromaCoding_P()
*
*  Description: Performs DCT, quantization, run/level pre-coding and IDCT 
*               for the chrominance of a I- of P-frame macroblock; 
*								current cbp and cr_cbp are affected 
*
************************************************************************/
void ChromaCoding_P(int *cr_cbp, struct img_par *img)
{
	int i, j;
	int uv, ii,jj,ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;	
	int pic_block_y, pic_block_x;
	Macroblock *currMB = &img->mb_data[img->current_mb_nr];

	*cr_cbp=0;
	for (uv=0; uv < 2; uv++) 
	{      
		if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW) 
		{
			intrapred_chroma(img,img->pix_c_x,img->pix_c_y,uv); 
		}        
		else
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
		*cr_cbp=dct_chroma(uv,*cr_cbp,img);
	}
	currMB->cbp += *cr_cbp*16;	
}

/************************************************************************
*
*  Name :       SetRefFrameInfo_P()
*
*  Description: Set reference frame information in global arrays 
*								depending on mode decision. Used for motion vector prediction.
*
************************************************************************/
void SetRefFrameInfo_P(struct img_par *img)
{
	int i,j;

	if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW) 
	{
		/* Set the reference frame information for motion vector prediction as unavailable */
		for (j = 0;j < BLOCK_MULTIPLE;j++)
		{
			for (i = 0;i < BLOCK_MULTIPLE;i++)
			{
				refFrArr[img->block_y+j][img->block_x+i] = -1;
			}
		}	
	}        
	else
	{
		/* Set the reference frame information for motion vector prediction  */
		for (j = 0;j < BLOCK_MULTIPLE;j++)
		{
			for (i = 0;i < BLOCK_MULTIPLE;i++)
			{
				refFrArr[img->block_y+j][img->block_x+i] = 	img->mb_data[img->current_mb_nr].ref_frame;
			}
		}		
	}
}


/************************************************************************
*
*  Name :       SetLoopfilterStrength_P()
*
*  Description: Set the filter strength for a macroblock of a I- or P-frame
*
************************************************************************/
void SetLoopfilterStrength_P(struct img_par *img)
{
	int i,j;
	int ii,jj;
	int i3,j3,mvDiffX,mvDiffY;

	if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW) 
	{
		for (i=0;i<BLOCK_MULTIPLE;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<BLOCK_MULTIPLE;j++)
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
	else
	{
		for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;
				
				mvDiffX = tmp_mv[0][jj][ii+4] - tmp_mv[0][jj][ii-1+4];
				mvDiffY = tmp_mv[1][jj][ii+4] - tmp_mv[1][jj][ii-1+4];
				
				if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
				{ 
					loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}
				
				if(jj > 0) /*GH: bug fix to avoid tmp_mv[][-1][ii+4]*/
				{
				  mvDiffX = tmp_mv[0][jj][ii+4] - tmp_mv[0][jj-1][ii+4];
				  mvDiffY = tmp_mv[1][jj][ii+4] - tmp_mv[1][jj-1][ii+4];
				
				  if(mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16)
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
}

/************************************************************************
*
*  Name :       encode_one_macroblock()
*
*  Description: Encode one macroblock depending on chosen picture type
*
************************************************************************/
void encode_one_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat)
{
	int cr_cbp;							/* chroma coded block pattern */
	int tot_intra_sad;
	int intra_pred_mode_2;	/* best 16x16 intra mode */

	Macroblock *currMB = &img->mb_data[img->current_mb_nr];

	/* Intra Prediction */
	tot_intra_sad = MakeIntraPrediction(img, inp, &intra_pred_mode_2);

	if (img->type != B_IMG)					/* I- or P-frame */
	{
		if ((img->mb_y == img->mb_y_upd && img->mb_y_upd != img->mb_y_intra) || img->type == INTRA_IMG)
		{
			img->mb_mode=8*img->type+img->imod;  /* Intra mode: set if intra image or if intra GOB for error robustness */
		}
		else						
		{			
			currMB->ref_frame = motion_search(inp,img,tot_intra_sad);	/* Motion vector search for P-frames */ 
		}
	}
	else													/* B-frame */
		currMB->ref_frame = motion_search_Bframe(tot_intra_sad, img, inp); /* Motion vector search for B-frames */ 

	if (img->type == B_IMG)					/* B-frame */
	{
		/* Residual coding of Luminance (only for B-modes) */
		LumaResidualCoding_B(img);	

		/* Coding of Chrominance */
		ChromaCoding_B(&cr_cbp, img);

		/* Set loop filter strength depending on mode decision */
		SetLoopfilterStrength_B(img); 

		/* Set reference frame information for motion vector prediction of future MBs */
		SetRefFrameInfo_B(img);					
	}
	else														/* I- or P-frame */
	{
		if (currMB->intraOrInter == INTER_MB)				
			LumaResidualCoding_P(img, inp);			/* Residual coding of Luminance (only for inter modes) */
																					/* Coding of Luma in intra mode is done implicitly in MakeIntraPredicition */
	
		/* Coding of Chrominance */
		ChromaCoding_P(&cr_cbp, img);
	
		/* Set loop filter strength depending on mode decision */
		SetLoopfilterStrength_P(img); 

		/* Set reference frame information for motion vector prediction of future MBs */
		SetRefFrameInfo_P(img);	
		
		/*  Check if a MB is skipped (no coeffs. only 0-vectors and prediction from the most recent frame) */
		if (img->mb_mode==M16x16_MB && currMB->intraOrInter == INTER_MB && (currMB->cbp == 0) 
				&& tmp_mv[0][img->block_y][img->block_x+4]==0 && tmp_mv[1][img->block_y][img->block_x+4]==0 
				&& (currMB->ref_frame == 0))
			img->mb_mode=COPY_MB;	
	}

	/* Set 16x16 intra mode and make "intra CBP" */
	if (img->imod==INTRA_MB_NEW)
		img->mb_mode += intra_pred_mode_2 + 4*cr_cbp + 12*img->kac;
					
}

/************************************************************************
*
*  Name :       write_one_macroblock()
*
*  Description: Passes the chosen syntax elements to the NAL
*
************************************************************************/
void write_one_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat)
{
	int i;
	int mb_nr = img->current_mb_nr;
	SyntaxElement *currSE = img->MB_SyntaxElements;
	Macroblock *currMB = &img->mb_data[mb_nr];
	int *bitCount = currMB->bitcounter;
	Slice *currSlice = img->currentSlice;
	DataPartition *dataPart;
	int *partMap = assignSE2partition[inp->partition_mode];


	/* Store imod for further use */
	currMB->mb_imode = img->imod;

	/* Store mb_mode for further use */
	currMB->mb_type = (currSE->value1 = img->mb_mode);

	/*  Bits for mode */
	if (inp->symbol_mode == UVLC)
		currSE->mapping = n_linfo2;
	else
		currSE->writing = writeMB_typeInfo2Buffer_CABAC;
	currSE->type = SE_MBTYPE;

											
	/* choose the appropriate data partition */
	if (img->type != B_IMG)
	{
#if TRACE
		sprintf(currSE->tracestring, "MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y,img->mb_mode);
#endif
		dataPart = &(currSlice->partArr[partMap[SE_MBTYPE]]);

	}
	else
	{
#if TRACE
		sprintf(currSE->tracestring, "B_MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y, img->mb_mode);
#endif
		dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
	}
	dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
	bitCount[BITS_HEADER_MB]+=currSE->len;

	/* proceed to next SE */
	currSE++;
	currMB->currSEnr++;
	
	/*  Do nothing more if copy and inter mode  */
	if (img->mb_mode != COPY_MB || currMB->intraOrInter != INTER_MB || img->type == B_IMG)
	{
		
		/*  Bits for intra prediction modes*/		
		if (img->imod == INTRA_MB_OLD)
		{
			for (i=0; i < MB_BLOCK_SIZE/2; i++)
			{
				currSE->value1 = currMB->intra_pred_modes[2*i];
				currSE->value2 = currMB->intra_pred_modes[2*i+1];
				if (inp->symbol_mode == UVLC)
					currSE->mapping = intrapred_linfo;
				else
					currSE->writing = writeIntraPredMode2Buffer_CABAC;
				currSE->type = SE_INTRAPREDMODE;
											
				/* choose the appropriate data partition */
				if (img->type != B_IMG)
				{
#if TRACE
					sprintf(currSE->tracestring, "Intra mode     = %3d",IPRED_ORDER[currSE->value1][currSE->value2]);
#endif
					dataPart = &(currSlice->partArr[partMap[SE_INTRAPREDMODE]]);

				}
				else
				{
#if TRACE
					sprintf(currSE->tracestring, "B_Intra mode = %3d\t",IPRED_ORDER[currSE->value1][currSE->value2]);
#endif
					dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

				}
				dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
				bitCount[BITS_COEFF_Y_MB]+=currSE->len;

				/* proceed to next SE */
				currSE++;
				currMB->currSEnr++;
			}
		}	
		/*  Bits for vector data*/
		if (img->type != B_IMG)
		{
			if (currMB->intraOrInter == INTER_MB) /* inter */
				writeMotionInfo2NAL_Pframe(img,inp);
		}
		else
		{
			if(img->imod != B_Direct)
				writeMotionInfo2NAL_Bframe(img, inp);
		}

		/* Bits for CBP and Coefficients */
		writeCBPandCoeffs2NAL(img,inp);
	}
	bitCount[BITS_TOTAL_MB] =	bitCount[BITS_HEADER_MB] + bitCount[BITS_COEFF_Y_MB]	+ bitCount[BITS_INTER_MB] 
													+ bitCount[BITS_CBP_MB] + bitCount[BITS_COEFF_UV_MB];
	stat->bit_slice += bitCount[BITS_TOTAL_MB];

}

/************************************************************************
*
*  Name :       writeMotionInfo2NAL_Pframe()
*
*  Description: Passes for a given MB of a P picture the reference frame 
*								parameter and the motion vectors to the NAL 
*
************************************************************************/
void writeMotionInfo2NAL_Pframe(struct img_par *img,struct inp_par *inp)
{
	int i,j,k,l,m;
	int step_h,step_v;
	int curr_mvd;
	int mb_nr = img->current_mb_nr;
	Macroblock *currMB = &img->mb_data[mb_nr];
	SyntaxElement *currSE = &img->MB_SyntaxElements[currMB->currSEnr];
	int *bitCount = currMB->bitcounter;
	Slice *currSlice = img->currentSlice;
	DataPartition *dataPart;
	int *partMap = assignSE2partition[inp->partition_mode];
	
	/*	If multiple ref. frames, write reference frame for the MB */
	if (inp->no_multpred > 1)
	{			

		currSE->value1 = currMB->ref_frame ;
		currSE->type = SE_REFFRAME;
		if (inp->symbol_mode == UVLC)
			currSE->mapping = n_linfo2;
		else
			currSE->writing = writeRefFrame2Buffer_CABAC;
		dataPart = &(currSlice->partArr[partMap[currSE->type]]);
		dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
		bitCount[BITS_INTER_MB]+=currSE->len;
#if TRACE
		sprintf(currSE->tracestring, "Reference frame no %d", currMB->ref_frame);
#endif
				
		/* proceed to next SE */
		currSE++;
		currMB->currSEnr++;				
	}

	/* Write motion vectors */
	step_h=img->blc_size_h/BLOCK_SIZE;			/* horizontal stepsize */
	step_v=img->blc_size_v/BLOCK_SIZE;			/* vertical stepsize */
	
	for (j=0; j < BLOCK_SIZE; j += step_v)
	{
		for (i=0;i < BLOCK_SIZE; i += step_h)
		{
			for (k=0; k < 2; k++)
			{
				curr_mvd = tmp_mv[k][img->block_y+j][img->block_x+i+4]-img->mv[i][j][currMB->ref_frame][img->mb_mode][k];						

				img->subblock_x = i; // position used for context determination
				img->subblock_y = j; // position used for context determination
				currSE->value1 = curr_mvd;
				/* store (oversampled) mvd */
				for (l=0; l < step_v; l++) 
					for (m=0; m < step_h; m++)	
						currMB->mvd[0][j+l][i+m][k] =  curr_mvd;
				currSE->value2 = k; // identifies the component; only used for context determination
				currSE->type = SE_MVD;
				if (inp->symbol_mode == UVLC)
					currSE->mapping = mvd_linfo2;
				else
					currSE->writing = writeMVD2Buffer_CABAC;
				dataPart = &(currSlice->partArr[partMap[currSE->type]]);
				dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
				bitCount[BITS_INTER_MB]+=currSE->len;
#if TRACE
				sprintf(currSE->tracestring, " MVD(%d) = %3d",k, curr_mvd);
#endif					
	
				/* proceed to next SE */
				currSE++;					
				currMB->currSEnr++;
						
			}
		}
	}		
}

/************************************************************************
*
*  Name :       writeCBPandCoeffs2NAL()
*
*  Description: Passes coded block pattern and coefficients (run/level)
*				to the NAL
*
************************************************************************/
void writeCBPandCoeffs2NAL(struct img_par *img,struct inp_par *inp)
{
	int i,j,k;
	int mb_y,mb_x;
	int level, run;
	int uv;
	int kk,kbeg,kend;
	int mb_nr = img->current_mb_nr;
	int ii,jj;
	int i1,j1, m2,jg2;
	Macroblock *currMB = &img->mb_data[mb_nr];
	const int cbp = currMB->cbp;
	SyntaxElement *currSE = &img->MB_SyntaxElements[currMB->currSEnr];
	int *bitCount = currMB->bitcounter;
	Slice *currSlice = img->currentSlice;
	DataPartition *dataPart;
	int *partMap = assignSE2partition[inp->partition_mode];
	
		
	/*	Bits for CBP */
	if (img->imod != INTRA_MB_NEW)
	{

		currSE->value1 = cbp;
#if TRACE
		sprintf(currSE->tracestring, "CBP (%2d,%2d) = %3d",img->mb_x, img->mb_y, cbp);
#endif

		if (img->imod == INTRA_MB_OLD) 
		{
			if (inp->symbol_mode == UVLC)
				currSE->mapping = cbp_linfo_intra;
			currSE->type = SE_CBP_INTRA;
		}
		else
		{
			if (inp->symbol_mode == UVLC)
				currSE->mapping = cbp_linfo_inter;
			currSE->type = SE_CBP_INTER;
		}

		if (inp->symbol_mode == CABAC)
			currSE->writing = writeCBP2Buffer_CABAC;
											
		/* choose the appropriate data partition */
		if (img->type != B_IMG)
			dataPart = &(currSlice->partArr[partMap[currSE->type]]);
		else
			dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

		dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
		bitCount[BITS_CBP_MB]+=currSE->len;
		
		/* proceed to next SE */
		currSE++;	
		currMB->currSEnr++;
		
		/*	Bits for luma coefficients */
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
						if ((cbp & (1<<(ii+jj*2))) != 0) 							/* check for any coefficients */
						{
							if (img->imod == INTRA_MB_OLD && img->qp < 24)	/* double scan */
							{

								for(kk=0;kk<2;kk++)
								{
									kbeg=kk*9;
									kend=kbeg+8;
									level=1; /* get inside loop */
									
									for(k=kbeg;k <= kend && level !=0; k++)
									{
										level = currSE->value1 = img->cof[i][j][k][0][DOUBLE_SCAN]; // level
										run = currSE->value2 = img->cof[i][j][k][1][DOUBLE_SCAN]; // run
	
										if (inp->symbol_mode == UVLC)
											currSE->mapping = levrun_linfo_intra;	
										else
										{
											currSE->context = 0; // for choosing context model
											currSE->writing = writeRunLevel2Buffer_CABAC;
										}

										if (k == kbeg)
										{ 
											currSE->type  = SE_LUM_DC_INTRA; /* element is of type DC */

											/* choose the appropriate data partition */
											if (img->type != B_IMG)
												dataPart = &(currSlice->partArr[partMap[SE_LUM_DC_INTRA]]);
											else
												dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
										}
										else
										{ 
											currSE->type  = SE_LUM_AC_INTRA;   /* element is of type AC */

											/* choose the appropriate data partition */
											if (img->type != B_IMG)
												dataPart = &(currSlice->partArr[partMap[SE_LUM_AC_INTRA]]);
											else
												dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
									
										}
										dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
										bitCount[BITS_COEFF_Y_MB]+=currSE->len;
#if TRACE
										sprintf(currSE->tracestring, "Luma dbl(%2d,%2d)  level=%3d Run=%2d",kk,k,level,run);
#endif							
										/* proceed to next SE */
										currSE++;	
										currMB->currSEnr++;
										
										if (level!=0)
										{
											loopb[img->block_x+i+1][img->block_y+j+1]=max(loopb[img->block_x+i+1][img->block_y+j+1],2);
											
											loopb[img->block_x+i	][img->block_y+j+1]=max(loopb[img->block_x+i	][img->block_y+j+1],1);
											loopb[img->block_x+i+1][img->block_y+j	]=max(loopb[img->block_x+i+1][img->block_y+j	],1);
											loopb[img->block_x+i+2][img->block_y+j+1]=max(loopb[img->block_x+i+2][img->block_y+j+1],1);
											loopb[img->block_x+i+1][img->block_y+j+2]=max(loopb[img->block_x+i+1][img->block_y+j+2],1);
										}
									}
								}
							}
							else																						/* single scan */
							{

								level=1; /* get inside loop */
								for(k=0;k<=16 && level !=0; k++)
								{
									level = currSE->value1 = img->cof[i][j][k][0][SINGLE_SCAN]; // level
									run = currSE->value2 = img->cof[i][j][k][1][SINGLE_SCAN]; // run

									if (inp->symbol_mode == UVLC)
										currSE->mapping = levrun_linfo_inter;		
									else							
										currSE->writing = writeRunLevel2Buffer_CABAC;

									if (k == 0)
									{ 
										if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
										{
											currSE->context = 2; // for choosing context model
											currSE->type  = SE_LUM_DC_INTRA;
										}
										else
										{
											currSE->context = 1; // for choosing context model
											currSE->type  = SE_LUM_DC_INTER;
										}
									}
									else
									{ 
										if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
										{
											currSE->context = 2; // for choosing context model
											currSE->type  = SE_LUM_AC_INTRA;
										}
										else
										{
											currSE->context = 1; // for choosing context model
											currSE->type  = SE_LUM_AC_INTER;
										}
									}
								
									/* choose the appropriate data partition */	
									if (img->type != B_IMG)
										dataPart = &(currSlice->partArr[partMap[currSE->type]]);
									else
										dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

									dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
									bitCount[BITS_COEFF_Y_MB]+=currSE->len;
#if TRACE
									sprintf(currSE->tracestring, "Luma sng(%2d) level =%3d run =%2d", k, level,run);
#endif		
									/* proceed to next SE */
									currSE++;	
									currMB->currSEnr++;
									
									
									if (level!=0)
									{
										loopb[img->block_x+i+1][img->block_y+j+1]=max(loopb[img->block_x+i+1][img->block_y+j+1],2);
										
										loopb[img->block_x+i	][img->block_y+j+1]=max(loopb[img->block_x+i	][img->block_y+j+1],1);
										loopb[img->block_x+i+1][img->block_y+j	]=max(loopb[img->block_x+i+1][img->block_y+j	],1);
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
			level = currSE->value1 = img->cof[0][0][k][0][1]; // level
			run = currSE->value2 = img->cof[0][0][k][1][1]; // run

			if (inp->symbol_mode == UVLC)
				currSE->mapping = levrun_linfo_inter;		
			else
			{
				currSE->context = 3; // for choosing context model
				currSE->writing = writeRunLevel2Buffer_CABAC;
			}
			currSE->type  = SE_LUM_DC_INTRA;   /* element is of type DC */

			/* choose the appropriate data partition */	
			if (img->type != B_IMG)
				dataPart = &(currSlice->partArr[partMap[currSE->type]]);
			else
				dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

			dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
			bitCount[BITS_COEFF_Y_MB]+=currSE->len;
#if TRACE
			sprintf(currSE->tracestring, "DC luma 16x16 sng(%2d) level =%3d run =%2d", k, level, run);
#endif
					
			/* proceed to next SE */
			currSE++;	
			currMB->currSEnr++;			
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
								level = currSE->value1 = img->cof[i][j][k][0][SINGLE_SCAN]; // level
								run = currSE->value2 = img->cof[i][j][k][1][SINGLE_SCAN]; // run

								if (inp->symbol_mode == UVLC)
									currSE->mapping = levrun_linfo_inter;		
								else
								{
									currSE->context = 4; // for choosing context model
									currSE->writing = writeRunLevel2Buffer_CABAC;
								}
								currSE->type  = SE_LUM_AC_INTRA;   /* element is of type AC */


								/* choose the appropriate data partition */	
								if (img->type != B_IMG)
									dataPart = &(currSlice->partArr[partMap[currSE->type]]);
								else
									dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

								dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
								bitCount[BITS_COEFF_Y_MB]+=currSE->len;
#if TRACE
								sprintf(currSE->tracestring, "AC luma 16x16 sng(%2d) level =%3d run =%2d", k, level, run);
#endif				
								/* proceed to next SE */
								currSE++;			
								currMB->currSEnr++;								
							}
						}
					}
				}
			}
		}
	}
		
	/*	Bits for chroma 2x2 DC transform coefficients */
	m2=img->mb_x*2;
	jg2=img->mb_y*2;
		
	if (cbp > 15)  /* check if any chroma bits in coded block pattern is set */
	{
		for (uv=0; uv < 2; uv++)
		{
			level=1;
			for (k=0; k < 5 && level != 0; ++k)
			{
				
				level = currSE->value1 = img->cofu[k][0][uv]; // level
				run = currSE->value2 = img->cofu[k][1][uv]; // run

				if (inp->symbol_mode == UVLC)
					currSE->mapping = levrun_linfo_c2x2;
				else							
					currSE->writing = writeRunLevel2Buffer_CABAC;

				if ( img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
				{
					currSE->context = 6; // for choosing context model
					currSE->type  = SE_CHR_DC_INTRA;
				}
				else
				{
					currSE->context = 5; // for choosing context model
					currSE->type  = SE_CHR_DC_INTER; 
				}
												
				/* choose the appropriate data partition */	
				if (img->type != B_IMG)
					dataPart = &(currSlice->partArr[partMap[currSE->type]]);
				else
					dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);

				dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
				bitCount[BITS_COEFF_UV_MB]+=currSE->len;
#if TRACE
				sprintf(currSE->tracestring, "2x2 DC Chroma %2d: level =%3d run =%2d",k, level, run);
#endif
						
				/* proceed to next SE */
				currSE++;	
				currMB->currSEnr++;
				
				
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
						loopc[m2+i+1][jg2 	 ]=max(loopc[m2+i+1][jg2		],1);
						loopc[m2+i+1][jg2+3  ]=max(loopc[m2+i+1][jg2+3	],1);
						loopc[m2		][jg2+i+1]=max(loopc[m2 	 ][jg2+i+1],1);
						loopc[m2+3	][jg2+i+1]=max(loopc[m2+3  ][jg2+i+1],1);
					}
				}
			}
		}
	}
		
	/*	Bits for chroma AC-coeffs. */		
	if (cbp >> 4 == 2) /* check if chroma bits in coded block pattern = 10b */
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
							level = currSE->value1 = img->cof[i][j][k][0][0]; // level
							run = currSE->value2 = img->cof[i][j][k][1][0]; // run

							if (inp->symbol_mode == UVLC)
								currSE->mapping = levrun_linfo_inter;
							else							
								currSE->writing = writeRunLevel2Buffer_CABAC;


							if ( img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
							{
								currSE->context = 8; // for choosing context model	
								currSE->type  = SE_CHR_AC_INTRA;
							}
							else
							{
								currSE->context = 7; // for choosing context model	
								currSE->type  = SE_CHR_AC_INTER; 
							}					
							/* choose the appropriate data partition */	
							if (img->type != B_IMG)
								dataPart = &(currSlice->partArr[partMap[currSE->type]]);
							else
								dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
							
							dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
							bitCount[BITS_COEFF_UV_MB]+=currSE->len;
#if TRACE
							sprintf(currSE->tracestring, "AC Chroma %2d: level =%3d run =%2d",k, level, run);
#endif
							/* proceed to next SE */
							currSE++;	
							currMB->currSEnr++;
			
							if (level != 0)
							{
								loopc[m2+i1+1][jg2+j1+1]=max(loopc[m2+i1+1][jg2+j1+1],2);
								
								loopc[m2+i1  ][jg2+j1+1]=max(loopc[m2+i1	][jg2+j1+1],1);
								loopc[m2+i1+1][jg2+j1  ]=max(loopc[m2+i1+1][jg2+j1	],1);
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
