/*!
 *	\file	
 *			Block.c	
 *	\brief
 *			Block functions
 *	\author
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Inge Lille-Langøy			<inge.lille-langoy@telenor.com>
 *			Rickard Sjoberg             <rickard.sjoberg@era.ericsson.se>
 */
#include "contributors.h"

#include <stdlib.h>
#include <math.h>
#include "block.h"

/*! 
 *	\fn		intrapred()
 *	\brief	makes and returns 4x4 blocks with all 5 intra prediction modes 
 *	\return	DECODING_OK		decoding of intraprediction mode was sucessfull
 *			SEARCH_SYNC		search next sync element as errors while decoding occured
 */

int intrapred(
	struct img_par *img,	/*!< image parameters */
	int ioff,				/*!< ?? */
	int joff,				/*!< ?? */
	int img_block_x,		/*!< ?? */
	int img_block_y)		/*!< ?? */
{
	int js0,js1,js2,i,j;
	int img_y,img_x;
	int ia[7];

	byte predmode = img->ipredmode[img_block_x+1][img_block_y+1];

	int block_available_up = (img->ipredmode[img_block_x+1][img_block_y] >=0);
	int block_available_left = (img->ipredmode[img_block_x][img_block_y+1] >=0);

	img_x=img_block_x*4;
	img_y=img_block_y*4;

	switch (predmode)
	{
	case DC_PRED:                         /* DC prediction */

		js1=0;
		js2=0;
		for (i=0;i<BLOCK_SIZE;i++)
		{
			if (block_available_up)
				js1=js1+imgY[img_y-1][img_x+i];
			if (block_available_left)
				js2=js2+imgY[img_y+i][img_x-1];
		}
		if(block_available_left && block_available_up)
			js0=(js1+js2+4)/8;                /* make DC prediction from both block above and to the left */
		if(block_available_left && !block_available_up)
			js0=(js2+2)/4;                    /* make DC prediction from block to the left */
		if(!block_available_left && block_available_up)
			js0=(js1+2)/4;                    /* make DC prediction from block above */
		if(!block_available_left && !block_available_up)
			js0=128;                          /* current block is the upper left */
		/* no block to make DC pred.from */
		for(i=0;i<BLOCK_SIZE;i++)
			for(j=0;j<BLOCK_SIZE;j++)
				img->mpr[i+ioff][j+joff]=js0;             /* store predicted 4x4 block */
		break;

	case VERT_PRED:                       /* vertical prediction from block above */
		for(j=0;j<BLOCK_SIZE;j++)
			for(i=0;i<BLOCK_SIZE;i++)
				img->mpr[i+ioff][j+joff]=imgY[img_y-1][img_x+i];/* store predicted 4x4 block */
		break;

	case HOR_PRED:                        /* horisontal prediction from left block */
		for(j=0;j<BLOCK_SIZE;j++)
			for(i=0;i<BLOCK_SIZE;i++)
				img->mpr[i+ioff][j+joff]=imgY[img_y+j][img_x-1]; /* store predicted 4x4 block */
		break;

	case DIAG_PRED_LR_45:       /* diagonal prediction from left to right 45 degree*/
		ia[0]=(imgY[img_y+3][img_x-1]+2*imgY[img_y+2][img_x-1]+imgY[img_y+1][img_x-1]+2)/4;
		ia[1]=(imgY[img_y+2][img_x-1]+2*imgY[img_y+1][img_x-1]+imgY[img_y+0][img_x-1]+2)/4;
		ia[2]=(imgY[img_y+1][img_x-1]+2*imgY[img_y+0][img_x-1]+imgY[img_y-1][img_x-1]+2)/4;
		ia[3]=(imgY[img_y+0][img_x-1]+2*imgY[img_y-1][img_x-1]+imgY[img_y-1][img_x+0]+2)/4;
		ia[4]=(imgY[img_y-1][img_x-1]+2*imgY[img_y-1][img_x+0]+imgY[img_y-1][img_x+1]+2)/4;
		ia[5]=(imgY[img_y-1][img_x+0]+2*imgY[img_y-1][img_x+1]+imgY[img_y-1][img_x+2]+2)/4;
		ia[6]=(imgY[img_y-1][img_x+1]+2*imgY[img_y-1][img_x+2]+imgY[img_y-1][img_x+3]+2)/4;
		for(i=0;i<BLOCK_SIZE;i++)
			for(j=0;j<BLOCK_SIZE;j++)
				img->mpr[i+ioff][j+joff]=ia[i-j+3];
		break;

	case DIAG_PRED_RL: /* diagonal prediction 22.5 deg to vertical plane*/
		ia[0]=(imgY[img_y-1][img_x+0]+imgY[img_y-1][img_x+1])/2;
		ia[1]= imgY[img_y-1][img_x+1];
		ia[2]=(imgY[img_y-1][img_x+1]+imgY[img_y-1][img_x+2])/2;
		ia[3]= imgY[img_y-1][img_x+2];
		ia[4]=(imgY[img_y-1][img_x+2]+imgY[img_y-1][img_x+3])/2;
		ia[5]= imgY[img_y-1][img_x+3];
		for(i=0;i<BLOCK_SIZE;i++)
			for(j=0;j<BLOCK_SIZE;j++)
				img->mpr[i+ioff][j+joff]=ia[MAP[j][i]];
		break;

	case  DIAG_PRED_LR:/* diagonal prediction -22.5 deg to horizontal plane */
		ia[0]=(imgY[img_y+0][img_x-1]+imgY[img_y+1][img_x-1])/2;
		ia[1]= imgY[img_y+1][img_x-1];
		ia[2]=(imgY[img_y+1][img_x-1]+imgY[img_y+2][img_x-1])/2;
		ia[3]= imgY[img_y+2][img_x-1];
		ia[4]=(imgY[img_y+2][img_x-1]+imgY[img_y+3][img_x-1])/2;
		ia[5]= imgY[img_y+3][img_x-1];
		for(i=0;i<BLOCK_SIZE;i++)
			for(j=0;j<BLOCK_SIZE;j++)
				img->mpr[i+ioff][j+joff]=ia[MAP[i][j]];
		break;

	default:
		printf("Error: illegal prediction mode input: %d\n",predmode);
		return SEARCH_SYNC;
		break;
	}
	return DECODING_OK;
}


/*!
 *  \fn		intrapred_luma_2()
 *	\brief	
 *	\return best SAD
 */

int intrapred_luma_2(
	 struct img_par *img,	/*!< image parameters */
	 int predmode)			/*!< prediction mode */
{
	int s0,s1,s2;

	int i,j;

	int ih,iv;
	int ib,ic,iaa;

	int mb_nr = img->current_mb_nr;
	int mb_width = img->width/16;
	int mb_available_up = (img->mb_y == 0) ? 0 : (slice_numbers[mb_nr] == slice_numbers[mb_nr-mb_width]);
	int mb_available_left = (img->mb_x == 0) ? 0 : (slice_numbers[mb_nr] == slice_numbers[mb_nr-1]);

	s1=s2=0;

	switch (predmode)
	{
	case VERT_PRED_16:                       /* vertical prediction from block above */
		for(j=0;j<MB_BLOCK_SIZE;j++)
			for(i=0;i<MB_BLOCK_SIZE;i++)
				img->mpr[i][j]=imgY[img->pix_y-1][img->pix_x+i];/* store predicted 16x16 block */
		break;

	case HOR_PRED_16:                        /* horisontal prediction from left block */
		for(j=0;j<MB_BLOCK_SIZE;j++)
			for(i=0;i<MB_BLOCK_SIZE;i++)
				img->mpr[i][j]=imgY[img->pix_y+j][img->pix_x-1]; /* store predicted 16x16 block */
		break;

	case DC_PRED_16:                         /* DC prediction */
		s1=s2=0;
		for (i=0; i < MB_BLOCK_SIZE; i++)
		{
			if (mb_available_up)
				s1 += imgY[img->pix_y-1][img->pix_x+i];    /* sum hor pix     */
			if (mb_available_left)
				s2 += imgY[img->pix_y+i][img->pix_x-1];    /* sum vert pix    */
		}
		if (mb_available_up && mb_available_left)
			s0=(s1+s2+16)/(2*MB_BLOCK_SIZE);       /* no edge         */
		if (!mb_available_up && mb_available_left)
			s0=(s2+8)/MB_BLOCK_SIZE;              /* upper edge      */
		if (mb_available_up && !mb_available_left)
			s0=(s1+8)/MB_BLOCK_SIZE;              /* left edge       */
		if (!mb_available_up && !mb_available_left)
			s0=128;                            /* top left corner, nothing to predict from */
		for(i=0;i<MB_BLOCK_SIZE;i++)
			for(j=0;j<MB_BLOCK_SIZE;j++)
			{
				img->mpr[i][j]=s0;
			}
		break;
	case PLANE_16:/* 16 bit integer plan pred */
		ih=0;
		iv=0;
		for (i=1;i<9;i++)
		{
			ih += i*(imgY[img->pix_y-1][img->pix_x+7+i] - imgY[img->pix_y-1][img->pix_x+7-i]);
			iv += i*(imgY[img->pix_y+7+i][img->pix_x-1] - imgY[img->pix_y+7-i][img->pix_x-1]);
		}
		ib=5*(ih/4)/16;
		ic=5*(iv/4)/16;

		iaa=16*(imgY[img->pix_y-1][img->pix_x+15]+imgY[img->pix_y+15][img->pix_x-1]);
		for (j=0;j< MB_BLOCK_SIZE;j++)
		{
			for (i=0;i< MB_BLOCK_SIZE;i++)
			{
				img->mpr[i][j]=(iaa+(i-7)*ib +(j-7)*ic + 16)/32;
			}
		}/* store plane prediction */
		break;

	default:
		{                                    /* indication of fault in bitstream,exit */
			printf("Error: illegal prediction mode input: %d\n",predmode);
			return SEARCH_SYNC;
		}
	}

	return DECODING_OK;
}

/*!
 *  \fn		itrans()
 *	\brief	Inverse 4x4 transformation, transforms cof to m7 
 */

void itrans(
	struct img_par *img,	/*!< image parameters */
	int ioff,				/*!< index to 4x4 block */
	int joff,				/*!< */
	int i0,					/*!< */
	int j0)					/*!< */
{
	int i,j,i1,j1;
	int m5[4];
	int m6[4];

	/* horisontal */
	for (j=0;j<BLOCK_SIZE;j++)
	{
		for (i=0;i<BLOCK_SIZE;i++)
		{
			m5[i]=img->cof[i0][j0][i][j];
		}
		m6[0]=(m5[0]+m5[2])*13;
		m6[1]=(m5[0]-m5[2])*13;
		m6[2]=m5[1]*7-m5[3]*17;
		m6[3]=m5[1]*17+m5[3]*7;

		for (i=0;i<2;i++)
		{
			i1=3-i;
			img->m7[i][j]=m6[i]+m6[i1];
			img->m7[i1][j]=m6[i]-m6[i1];
		}
	}
	/* vertical */
	for (i=0;i<BLOCK_SIZE;i++)
	{
		for (j=0;j<BLOCK_SIZE;j++)
			m5[j]=img->m7[i][j];

		m6[0]=(m5[0]+m5[2])*13;
		m6[1]=(m5[0]-m5[2])*13;
		m6[2]=m5[1]*7-m5[3]*17;
		m6[3]=m5[1]*17+m5[3]*7;

		for (j=0;j<2;j++)
		{
			j1=3-j;
			img->m7[i][j]=mmax(0,mmin(255,(m6[j]+m6[j1]+img->mpr[i+ioff][j+joff]*JQQ+JQQ2)/JQQ));
			img->m7[i][j1]=mmax(0,mmin(255,(m6[j]-m6[j1]+img->mpr[i+ioff][j1+joff]*JQQ+JQQ2)/JQQ));
		}
	}

}


/*!
 *	\fn		itrans_2()
 *	\brief	invers  transform 
 */
void itrans_2(
	 struct img_par *img)	/*!< image parameters */
{
	int i,j,i1,j1;
	int M5[4];
	int M6[4];

	/* horizontal */
	for (j=0;j<4;j++)
	{
		for (i=0;i<4;i++)
			M5[i]=img->cof[i][j][0][0];

		M6[0]=(M5[0]+M5[2])*13;
		M6[1]=(M5[0]-M5[2])*13;
		M6[2]= M5[1]*7 -M5[3]*17;
		M6[3]= M5[1]*17+M5[3]*7;

		for (i=0;i<2;i++)
		{
			i1=3-i;
			img->cof[i ][j][0][0]= M6[i]+M6[i1];
			img->cof[i1][j][0][0]=M6[i]-M6[i1];
		}
	}

	/* vertical */
	for (i=0;i<4;i++)
	{
		for (j=0;j<4;j++)
			M5[j]=img->cof[i][j][0][0];

		M6[0]=(M5[0]+M5[2])*13;
		M6[1]=(M5[0]-M5[2])*13;
		M6[2]= M5[1]*7 -M5[3]*17;
		M6[3]= M5[1]*17+M5[3]*7;

		for (j=0;j<2;j++)
		{
			j1=3-j;
			img->cof[i][j][0][0] = ((M6[j]+M6[j1])/8) *JQ1[img->qp];
			img->cof[i][j1][0][0]= ((M6[j]-M6[j1])/8) *JQ1[img->qp];
		}
	}
	for (j=0;j<4;j++)
	{
		for (i=0;i<4;i++)
		{
			img->cof[i][j][0][0] = 3 * img->cof[i][j][0][0]/256;
		}
	}
}



