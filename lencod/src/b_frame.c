// *************************************************************************************
// *************************************************************************************
// B_frame.c :	B picture coding 
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Byeong-Moon Jeon                <jeonbm@lge.com>
// Yoon-Seong Soh                  <yunsung@lge.com>
// Thomas Stockhammer              <stockhammer@ei.tum.de>
// Detlev Marpe                    <marpe@hhi.de>
// Guido Heising                   <heising@hhi.de>
// *************************************************************************************
// *************************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#include "global.h"
#include "elements.h"
#include "b_frame.h"


/************************************************************************
*  Name :    copy2mref()
*
*  Description: Substitutes function oneforthpix_2. It should be worked 
*								out how this copy procedure can be avoided.
*
************************************************************************/
void copy2mref(struct img_par *img)
{
	int j, uv;

	img->frame_cycle=img->number % img->no_multpred;  /*GH img->no_multpred used insteadof MAX_MULT_PRED
		                                                frame buffer size = img->no_multpred+1*/
	/* Luma */
	for (j=0; j < img->height*4; j++)
		memcpy(mref[img->frame_cycle][j],mref_P[j], img->width*4);


	/*  Chroma: */
	for (uv=0; uv < 2; uv++)
		for (j=0; j < img->height_cr; j++)
				memcpy(mcef[img->frame_cycle][uv][j],mcef_P[uv][j],img->width_cr);
}



/************************************************************************
*
*  Name :       SetLoopfilterStrength_B()
*
*  Description: Set the filter strength for a macroblock of a B-frame
*
************************************************************************/
void SetLoopfilterStrength_B(struct img_par *img)
{
	int i,j;
	int ii,jj;
	int i3,j3,mvDiffX,mvDiffY;
	
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
	if (img->imod==B_Forward || img->imod==B_Bidirect) 
	{
		for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;
        
				mvDiffX = tmp_fwMV[0][jj][ii+4] - tmp_fwMV[0][jj][ii-1+4];
				mvDiffY = tmp_fwMV[1][jj][ii+4] - tmp_fwMV[1][jj][ii-1+4];
				
				if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
				{ 
					loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}
				
				if (jj >0)
				{
					mvDiffX = tmp_fwMV[0][jj][ii+4] - tmp_fwMV[0][jj-1][ii+4];
					mvDiffY = tmp_fwMV[1][jj][ii+4] - tmp_fwMV[1][jj-1][ii+4];
					
					if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16))
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
	if(img->imod==B_Backward || img->imod==B_Bidirect) 
	{
		for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;
        
				mvDiffX = tmp_bwMV[0][jj][ii+4] - tmp_bwMV[0][jj][ii-1+4];
				mvDiffY = tmp_bwMV[1][jj][ii+4] - tmp_bwMV[1][jj][ii-1+4];
				
				if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
				{ 
					loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}
				
				if (jj > 0)
				{
					mvDiffX = tmp_bwMV[0][jj][ii+4] - tmp_bwMV[0][jj-1][ii+4];
					mvDiffY = tmp_bwMV[1][jj][ii+4] - tmp_bwMV[1][jj-1][ii+4];
					
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
	
	// LG : loopb, loopc at Direct mode 
	if(img->imod==B_Direct) 
	{
		for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;
        
				mvDiffX = dfMV[0][jj][ii+4] - dfMV[0][jj][ii-1+4];
				mvDiffY = dfMV[1][jj][ii+4] - dfMV[1][jj][ii-1+4];
				
				if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
				{ 
					loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}
				
				if (jj > 0)
				{
					mvDiffX = dfMV[0][jj][ii+4] - dfMV[0][jj-1][ii+4];
					mvDiffY = dfMV[1][jj][ii+4] - dfMV[1][jj-1][ii+4];
					
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
		
		for (i=0;i<4;i++)
		{
			ii=img->block_x+i;
			i3=ii/2;
			for (j=0;j<4;j++)
			{
				jj=img->block_y+j;
				j3=jj/2;
        
				mvDiffX = dbMV[0][jj][ii+4] - dbMV[0][jj][ii-1+4];
				mvDiffY = dbMV[1][jj][ii+4] - dbMV[1][jj][ii-1+4];
				
				if((mvDiffX*mvDiffX >= 16 || mvDiffY*mvDiffY >= 16) && ii > 0)
				{ 
					loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
					loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
					loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
					loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
				}
				
				if (jj > 0)
				{
					mvDiffX = dbMV[0][jj][ii+4] - dbMV[0][jj-1][ii+4];
					mvDiffY = dbMV[1][jj][ii+4] - dbMV[1][jj-1][ii+4];
					
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
}

/************************************************************************
*
*  Name :       SetRefFrameInfo_B()
*
*  Description: Set reference frame information in global arrays 
*								depending on mode decision. Used for motion vector prediction.
*
************************************************************************/
void SetRefFrameInfo_B(struct img_par *img)
{
	int i,j;
	const int fw_predframe_no = img->mb_data[img->current_mb_nr].ref_frame;

	if(img->imod==B_Direct)
	{
		for (j = 0; j < 4;j++)	
		{
			for (i = 0; i < 4;i++)
			{
				fw_refFrArr[img->block_y+j][img->block_x+i] = 
						bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
			}
		}
	}
	else 
		if (img->imod == B_Forward) 
		{
			for (j = 0;j < 4;j++)	
			{
				for (i = 0;i < 4;i++)
				{
					fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no;
					bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
				}
			}
		}
		else 
			if(img->imod == B_Backward) 
			{
				for (j = 0;j < 4;j++)	
				{
					for (i = 0;i < 4;i++)
					{
						fw_refFrArr[img->block_y+j][img->block_x+i] = -1;
						bw_refFrArr[img->block_y+j][img->block_x+i] = 0;
					}
				}
			}
			else 
				if(img->imod == B_Bidirect) 
				{
					for (j = 0;j < 4;j++)	
					{
						for (i = 0;i < 4;i++)
						{
							fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no;
							bw_refFrArr[img->block_y+j][img->block_x+i] = 0;
						}
					}
				}
				else /* 4x4-, 16x16-intra */
				{ 
					for (j = 0;j < 4;j++)	
					{
						for (i = 0;i < 4;i++)
						{
							fw_refFrArr[img->block_y+j][img->block_x+i] = 
								bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
						}
					}
				}

}


/************************************************************************
*
*  Name :       LumaResidualCoding_B()
*
*  Description: Performs DCT, quantization, run/level pre-coding and IDCT 
*               for the MC-compensated MB residue of a B-frame; 
*								current cbp (for LUMA only) is affected 
*
************************************************************************/
void LumaResidualCoding_B(struct img_par *img)
{
	int cbp_mask, sum_cnt_nonz, coeff_cost, nonzero;
	int mb_y, mb_x, block_y, block_x, i, j, pic_pix_y, pic_pix_x, pic_block_x, pic_block_y;
	int ii4, jj4, iii4, jjj4, i2, j2, fw_pred, bw_pred, ref_inx, df_pred, db_pred; 
	Macroblock *currMB = &img->mb_data[img->current_mb_nr];

	switch(img->imod) 
	{
		case B_Forward :
			currMB->cbp=0;
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
							
							ii4=(img->pix_x+block_x)*4+tmp_fwMV[0][pic_block_y][pic_block_x+4];
							jj4=(img->pix_y+block_y)*4+tmp_fwMV[1][pic_block_y][pic_block_x+4];
							
							for (j=0;j<4;j++) {
								j2=j*4;
								for (i=0;i<4;i++) {
									i2=i*4;
									img->mpr[i+block_x][j+block_y]=mref[img->fw_multframe_no][jj4+j2][ii4+i2];
								}
							}
							
							for (j=0; j < BLOCK_SIZE; j++) 
							{
								for (i=0; i < BLOCK_SIZE; i++) 
								{
									img->m7[i][j]=
										imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
								}
							}
							nonzero=dct_luma(block_x,block_y,&coeff_cost,img);
							if (nonzero) 
							{
								currMB->cbp |= cbp_mask;
							}
						} // block_x
					} // block_y
					
					if (coeff_cost > 3) {
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
				} // mb_x
			}	// mb_y					
			
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
			break;
			
		case B_Backward :
			currMB->cbp=0;
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
							
							iii4=(img->pix_x+block_x)*4+tmp_bwMV[0][pic_block_y][pic_block_x+4];
							jjj4=(img->pix_y+block_y)*4+tmp_bwMV[1][pic_block_y][pic_block_x+4];
							
							for (j=0;j<4;j++) {
								j2=j*4;
								for (i=0;i<4;i++) {
									i2=i*4;
									img->mpr[i+block_x][j+block_y]=mref_P[jjj4+j2][iii4+i2];
								}
							}
							
							for (j=0; j < BLOCK_SIZE; j++) 
							{
								for (i=0; i < BLOCK_SIZE; i++) 
								{
									img->m7[i][j]=
										imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
								}
							}
							nonzero=dct_luma(block_x,block_y,&coeff_cost,img);
							if (nonzero) 
							{
								currMB->cbp |= cbp_mask;
							}
						} // block_x
					} // block_y
					
					if (coeff_cost > 3) {
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
				} // mb_x
			}	// mb_y					
			
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
			break;
			
		case B_Bidirect :
			currMB->cbp=0;
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
							
							ii4=(img->pix_x+block_x)*4+tmp_fwMV[0][pic_block_y][pic_block_x+4];
							jj4=(img->pix_y+block_y)*4+tmp_fwMV[1][pic_block_y][pic_block_x+4];
							iii4=(img->pix_x+block_x)*4+tmp_bwMV[0][pic_block_y][pic_block_x+4];
							jjj4=(img->pix_y+block_y)*4+tmp_bwMV[1][pic_block_y][pic_block_x+4];
							
							for (j=0;j<4;j++) {
								j2=j*4;
								for (i=0;i<4;i++) {
									i2=i*4;
									fw_pred=mref[img->fw_multframe_no][jj4+j2][ii4+i2];
									bw_pred=mref_P[jjj4+j2][iii4+i2];
									img->mpr[i+block_x][j+block_y]=(int)((fw_pred+bw_pred)/2.0+0.5);
								}
							}
							
							for (j=0; j < BLOCK_SIZE; j++) 
							{
								for (i=0; i < BLOCK_SIZE; i++) 
								{
									img->m7[i][j]=
										imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
								}
							}
							nonzero=dct_luma(block_x,block_y,&coeff_cost,img);
							if (nonzero) 
							{
								currMB->cbp |= cbp_mask;
							}
						} // block_x
					} // block_y
					
					if (coeff_cost > 3) {
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
				} // mb_x
			}	// mb_y					
			
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
			break;
			
		case B_Direct :
			currMB->cbp=0;
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
							
							ii4=(img->pix_x+block_x)*4+dfMV[0][pic_block_y][pic_block_x+4];
							jj4=(img->pix_y+block_y)*4+dfMV[1][pic_block_y][pic_block_x+4];
							iii4=(img->pix_x+block_x)*4+dbMV[0][pic_block_y][pic_block_x+4];
							jjj4=(img->pix_y+block_y)*4+dbMV[1][pic_block_y][pic_block_x+4];
							
							// next P is intra mode
							if(refFrArr[pic_block_y][pic_block_x]==-1)
								ref_inx=(img->number-1)%img->no_multpred;
							// next P is skip or inter mode
							else
								ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%img->no_multpred;
							
							for (j=0;j<4;j++) {
								j2=j*4;
								for (i=0;i<4;i++) {
									i2=i*4;								
									df_pred=mref[ref_inx][jj4+j2][ii4+i2];
									db_pred=mref_P[jjj4+j2][iii4+i2];
									img->mpr[i+block_x][j+block_y]=(int)((df_pred+db_pred)/2.0+0.5);                
								}
							}
							
							// LG : direct residual coding
							for (j=0; j < BLOCK_SIZE; j++) 
							{
								for (i=0; i < BLOCK_SIZE; i++) 
								{
									img->m7[i][j]=										
										imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-img->mpr[i+block_x][j+block_y];
								}
							}
							nonzero=dct_luma(block_x,block_y,&coeff_cost,img);
							if (nonzero) 
							{
								currMB->cbp |= cbp_mask;
							}
						} // block_x
					} // block_y
					
					// LG : direct residual coding
					if (coeff_cost > 3) {
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
				} // mb_x
			} // mb_y					
			
			// LG : direct residual coding
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
			break;
			
		default:
			break;
	} // switch()	  
}

/************************************************************************
*
*  Name :       ChromaCoding_B()
*
*  Description: Performs DCT, quantization, run/level pre-coding and IDCT 
*               for the chrominance of a B-frame macroblock; 
*								current cbp and cr_cbp are affected 
*
************************************************************************/
void ChromaCoding_B(int *cr_cbp, struct img_par *img)
{
	int i, j;
	int uv, ii,jj,ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;	
	int pic_block_y, pic_block_x, ref_inx, fw_pred, bw_pred;
	Macroblock *currMB = &img->mb_data[img->current_mb_nr];

	*cr_cbp=0;
	for (uv=0; uv < 2; uv++) 
	{      
		if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW) 
		{
			intrapred_chroma(img,img->pix_c_x,img->pix_c_y,uv); 
		}        
		else if(img->imod == B_Forward)  
		{
			for (j=0; j < MB_BLOCK_SIZE/2; j++) 
			{
				pic_block_y=(img->pix_c_y+j)/2;
				for (i=0; i < MB_BLOCK_SIZE/2; i++) 
				{
					pic_block_x=(img->pix_c_x+i)/2;
					ii=(img->pix_c_x+i)*8+tmp_fwMV[0][pic_block_y][pic_block_x+4];
					jj=(img->pix_c_y+j)*8+tmp_fwMV[1][pic_block_y][pic_block_x+4];
					ii0=ii/8;
					jj0=jj/8;
					ii1=(ii+7)/8;
					jj1=(jj+7)/8;
					if1=(ii & 7);
					jf1=(jj & 7);
					if0=8-if1;
					jf0=8-jf1;
					img->mpr[i][j]=(if0*jf0*mcef[img->fw_multframe_no][uv][jj0][ii0]+
						if1*jf0*mcef[img->fw_multframe_no][uv][jj0][ii1]+
						if0*jf1*mcef[img->fw_multframe_no][uv][jj1][ii0]+
						if1*jf1*mcef[img->fw_multframe_no][uv][jj1][ii1]+32)/64;
					img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j]; 
				}
			}
		}
		else if(img->imod == B_Backward)  
		{
			for (j=0; j < MB_BLOCK_SIZE/2; j++) 
			{
				pic_block_y=(img->pix_c_y+j)/2;
				for (i=0; i < MB_BLOCK_SIZE/2; i++) 
				{
					pic_block_x=(img->pix_c_x+i)/2;
					ii=(img->pix_c_x+i)*8+tmp_bwMV[0][pic_block_y][pic_block_x+4];
					jj=(img->pix_c_y+j)*8+tmp_bwMV[1][pic_block_y][pic_block_x+4];
					ii0=ii/8;
					jj0=jj/8;
					ii1=(ii+7)/8;
					jj1=(jj+7)/8;
					if1=(ii & 7);
					jf1=(jj & 7);
					if0=8-if1;
					jf0=8-jf1;
					img->mpr[i][j]=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
						if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+32)/64;
					img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j]; 
				}
			}
		}
		else if(img->imod == B_Bidirect)  
		{
			for (j=0; j < MB_BLOCK_SIZE/2; j++) 
			{
				pic_block_y=(img->pix_c_y+j)/2;
				for (i=0; i < MB_BLOCK_SIZE/2; i++) 
				{
					pic_block_x=(img->pix_c_x+i)/2;
					ii=(img->pix_c_x+i)*8+tmp_fwMV[0][pic_block_y][pic_block_x+4];
					jj=(img->pix_c_y+j)*8+tmp_fwMV[1][pic_block_y][pic_block_x+4];
					ii0=ii/8;
					jj0=jj/8;
					ii1=(ii+7)/8;
					jj1=(jj+7)/8;
					if1=(ii & 7);
					jf1=(jj & 7);
					if0=8-if1;
					jf0=8-jf1;
					fw_pred=(if0*jf0*mcef[img->fw_multframe_no][uv][jj0][ii0]+
						if1*jf0*mcef[img->fw_multframe_no][uv][jj0][ii1]+
						if0*jf1*mcef[img->fw_multframe_no][uv][jj1][ii0]+
						if1*jf1*mcef[img->fw_multframe_no][uv][jj1][ii1]+32)/64;
					
					ii=(img->pix_c_x+i)*8+tmp_bwMV[0][pic_block_y][pic_block_x+4];
					jj=(img->pix_c_y+j)*8+tmp_bwMV[1][pic_block_y][pic_block_x+4];
					ii0=ii/8;
					jj0=jj/8;
					ii1=(ii+7)/8;
					jj1=(jj+7)/8;
					if1=(ii & 7);
					jf1=(jj & 7);
					if0=8-if1;
					jf0=8-jf1;
					bw_pred=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
						if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+32)/64;
					
					img->mpr[i][j]=(int)((fw_pred+bw_pred)/2.0+0.5);
					img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j]; 
				}
			}
		}
		else // img->imod == B_Direct  
		{
			for (j=0; j < MB_BLOCK_SIZE/2; j++) 
			{
				pic_block_y=(img->pix_c_y+j)/2;
				for (i=0; i < MB_BLOCK_SIZE/2; i++) 
				{
					pic_block_x=(img->pix_c_x+i)/2;
					
					ii=(img->pix_c_x+i)*8+dfMV[0][pic_block_y][pic_block_x+4];
					jj=(img->pix_c_y+j)*8+dfMV[1][pic_block_y][pic_block_x+4];
					ii0=ii/8;
					jj0=jj/8;
					ii1=(ii+7)/8;
					jj1=(jj+7)/8;
					if1=(ii & 7);
					jf1=(jj & 7);
					if0=8-if1;
					jf0=8-jf1;
					
					// next P is intra mode
					if(refFrArr[pic_block_y][pic_block_x]==-1)
						ref_inx=(img->number-1)%MAX_MULT_PRED;
					// next P is skip or inter mode
					else
						ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%MAX_MULT_PRED;
					
					fw_pred=(if0*jf0*mcef[ref_inx][uv][jj0][ii0]+
						if1*jf0*mcef[ref_inx][uv][jj0][ii1]+
						if0*jf1*mcef[ref_inx][uv][jj1][ii0]+
						if1*jf1*mcef[ref_inx][uv][jj1][ii1]+32)/64;
					
					ii=(img->pix_c_x+i)*8+dbMV[0][pic_block_y][pic_block_x+4];
					jj=(img->pix_c_y+j)*8+dbMV[1][pic_block_y][pic_block_x+4];
					ii0=ii/8;
					jj0=jj/8;
					ii1=(ii+7)/8;
					jj1=(jj+7)/8;
					if1=(ii & 7);
					jf1=(jj & 7);
					if0=8-if1;
					jf0=8-jf1;
					
					bw_pred=(if0*jf0*mcef_P[uv][jj0][ii0]+if1*jf0*mcef_P[uv][jj0][ii1]+
						if0*jf1*mcef_P[uv][jj1][ii0]+if1*jf1*mcef_P[uv][jj1][ii1]+32)/64;
					
					img->mpr[i][j]=(int)((fw_pred+bw_pred)/2.0+0.5);
					
					// LG : direct residual coding
					img->m7[i][j]=imgUV_org[uv][img->pix_c_y+j][img->pix_c_x+i]-img->mpr[i][j]; 
				}
			}
		}
		*cr_cbp=dct_chroma(uv, *cr_cbp, img);
	}
	// LG : direct residual coding
	currMB->cbp += *cr_cbp*16;
}


/************************************************************************
*
*  Name :       writeMotionInfo2NAL_Bframe()
*
*  Description: Passes for a given MB of a B picture the reference frame 
*								parameter and motion parameters to the NAL 
*
************************************************************************/
void writeMotionInfo2NAL_Bframe(struct img_par *img,struct inp_par *inp)
{
	int i,j,k,l,m;
	int step_h,step_v;
	int curr_mvd, fw_blk_shape, bw_blk_shape;
	int mb_nr = img->current_mb_nr;
	Macroblock *currMB = &img->mb_data[mb_nr];
	const int fw_predframe_no=currMB->ref_frame;
	SyntaxElement *currSE = &img->MB_SyntaxElements[currMB->currSEnr];
	int *bitCount = currMB->bitcounter;
	Slice *currSlice = img->currentSlice;
	DataPartition *dataPart;
	int *partMap = assignSE2partition[inp->partition_mode];


	/* Write fw_predframe_no(Forward, Bidirect) */ 
	if(img->imod==B_Forward || img->imod==B_Bidirect) 
	{
		if (img->no_multpred > 1) 
		{		
			currSE->value1 = currMB->ref_frame;
			currSE->type = SE_REFFRAME;
			if (inp->symbol_mode == UVLC)
				currSE->mapping = n_linfo2;
			else
				currSE->writing = writeRefFrame2Buffer_CABAC;

			dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
			dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
			bitCount[BITS_INTER_MB]+=currSE->len;
#if TRACE
			sprintf(currSE->tracestring, "B_fw_Reference frame no %3d ", currMB->ref_frame);
#endif				
		
			/* proceed to next SE */
			currSE++;	
			currMB->currSEnr++;			
		}
	}
			
	/* Write Blk_size(Bidirect) */					  
	if(img->imod==B_Bidirect) 
	{
		/* Write blockshape for forward pred */
		fw_blk_shape = BlkSize2CodeNumber(img->fw_blc_size_h, img->fw_blc_size_v);
		
		currSE->value1 = fw_blk_shape;
		currSE->type = SE_BFRAME;
		if (inp->symbol_mode == UVLC)
			currSE->mapping = n_linfo2;
		else
			currSE->writing = writeBiDirBlkSize2Buffer_CABAC;
				
		dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
		dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
		bitCount[BITS_INTER_MB]+=currSE->len;
#if TRACE
		sprintf(currSE->tracestring, "B_bidiret_fw_blk %3d x %3d ", img->fw_blc_size_h, img->fw_blc_size_v);
#endif
				
		/* proceed to next SE */
		currSE++;			
		currMB->currSEnr++;			

		/* Write blockshape for backward pred */
		bw_blk_shape = BlkSize2CodeNumber(img->bw_blc_size_h, img->bw_blc_size_v);
		
		currSE->value1 = bw_blk_shape;
		currSE->type = SE_BFRAME;
		if (inp->symbol_mode == UVLC)
			currSE->mapping = n_linfo2;
		else
			currSE->writing = writeBiDirBlkSize2Buffer_CABAC;
				
		dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
		dataPart->writeSyntaxElement(	currSE, img, inp, dataPart);
		bitCount[BITS_INTER_MB]+=currSE->len;
#if TRACE
		sprintf(currSE->tracestring, "B_bidiret_bw_blk %3d x %3d ", img->bw_blc_size_h, img->bw_blc_size_v);
#endif
				
		/* proceed to next SE */
		currSE++;			
		currMB->currSEnr++;			
		
	}

	/* Write MVDFW(Forward, Bidirect) */
	if(img->imod==B_Forward || img->imod==B_Bidirect) 
	{
		step_h=img->fw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize 
		step_v=img->fw_blc_size_v/BLOCK_SIZE;  // vertical stepsize 

		for (j=0; j < BLOCK_SIZE; j += step_v) 
		{            
			for (i=0;i < BLOCK_SIZE; i += step_h) 
			{
				for (k=0; k < 2; k++) 
				{
					if(img->mb_mode==1) // fw 16x16
						curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][k]);
					else 
						if(img->mb_mode==3) // bidirectinal
						{
							switch(fw_blk_shape)
							{
								case 0:
									curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][k]);
									break;
								case 1:
									curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][2][k]);
									break;
								case 2:
									curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][3][k]);
									break;
								case 3:
									curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][4][k]);
									break;
								case 4:
									curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][5][k]);
									break;
								case 5:
									curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][6][k]);
									break;
								case 6:
									curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][7][k]);
									break;
								default:
									break;
							}
						}
						else
							curr_mvd=(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][(img->mb_mode)/2][k]);
				
					/* store (oversampled) mvd */
					for (l=0; l < step_v; l++) 
						for (m=0; m < step_h; m++)	
							currMB->mvd[0][j+l][i+m][k] = curr_mvd;
				
					currSE->value1 = curr_mvd;
					currSE->type = SE_MVD;
					if (inp->symbol_mode == UVLC)
						currSE->mapping = mvd_linfo2;
					else
					{
						img->subblock_x = i; // position used for context determination
						img->subblock_y = j; // position used for context determination
						currSE->value2 = 2*k; // identifies the component and the direction; only used for context determination
						currSE->writing = writeBiMVD2Buffer_CABAC;
					}	
					dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
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
			
	/* Write MVDBW(Backward, Bidirect) */
	if(img->imod==B_Backward || img->imod==B_Bidirect) 
	{
		step_h=img->bw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize 
		step_v=img->bw_blc_size_v/BLOCK_SIZE;  // vertical stepsize 

		for (j=0; j < BLOCK_SIZE; j += step_v) 
		{
			for (i=0;i < BLOCK_SIZE; i += step_h) 
			{
				for (k=0; k < 2; k++) 
				{
					if(img->mb_mode==2) // bw 16x16
						curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][k]);
					else 
						if(img->mb_mode==3) // bidirectional
						{
							switch(bw_blk_shape)
							{
								case 0:
									curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][k]);
									break;
								case 1:
									curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][2][k]);
									break;
								case 2:
									curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][3][k]);
									break;
								case 3:
									curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][4][k]);
									break;
								case 4:
									curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][5][k]);
									break;
								case 5:
									curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][6][k]);
									break;
								case 6:
									curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][7][k]);
									break;
								default:
									break;
							}
						}
						else // other bw
							curr_mvd=(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][(img->mb_mode-1)/2][k]);						  				


					/* store (oversampled) mvd */
					for (l=0; l < step_v; l++) 
						for (m=0; m < step_h; m++)	
							currMB->mvd[1][j+l][i+m][k] = curr_mvd;
				
					currSE->value1 = curr_mvd;
					currSE->type = SE_MVD;

					if (inp->symbol_mode == UVLC)
						currSE->mapping = mvd_linfo2;
					else
					{
						img->subblock_x = i; // position used for context determination
						img->subblock_y = j; // position used for context determination
						currSE->value2 = 2*k+1; // identifies the component and the direction; only used for context determination
						currSE->writing = writeBiMVD2Buffer_CABAC;
					}	

					dataPart = &(currSlice->partArr[partMap[SE_BFRAME]]);
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
}

/************************************************************************
*
*  Name :       BlkSize2CodeNumber()
*
*  Description: Passes back the code number given the blocksize width and
*								height (should be replaced by an appropriate table lookup)
*
************************************************************************/
int BlkSize2CodeNumber(int blc_size_h, int blc_size_v)
{

	if(blc_size_h==16 && blc_size_v==16)  // 16x16 : code_number 0			
		return 0;
	else 
		if(blc_size_h==16 && blc_size_v==8)  // 16x8 : code_number 1
			return 1;
		else 
			if(blc_size_h==8 && blc_size_v==16) // 8x16 : code_number 2
				return 2;
			else 
				if(blc_size_h==8 && blc_size_v==8) // 8x8 : code_number 3
					return 3;
				else 
					if(blc_size_h==8 && blc_size_v==4)  // 8x4 : code_number 4
						return 4;
					else 
						if(blc_size_h==4 && blc_size_v==8) // 4x8 : code_number 5
							return 5;
						else  // 4x4 : code_number 6
							return 6;

}



////////////////////////////////////////////////////////////////////
// select intra, forward, backward, bidirectional, direct mode
////////////////////////////////////////////////////////////////////
int motion_search_Bframe(int tot_intra_sad, struct img_par *img, struct inp_par *inp)
{
	int fw_sad, bw_sad, bid_sad, dir_sad;
	int fw_predframe_no;

	fw_predframe_no=get_fwMV(&fw_sad, img, inp, tot_intra_sad); 
	get_bwMV(&bw_sad, img, inp); 
	get_bid(&bid_sad, fw_predframe_no, img, inp); 
	get_dir(&dir_sad, img, inp); 
	compare_sad(tot_intra_sad, fw_sad, bw_sad, bid_sad, dir_sad, img); 

	return fw_predframe_no;	
}

int get_fwMV(int *min_fw_sad, struct img_par *img, struct inp_par *inp_par, int tot_intra_sad)
{
	int fw_predframe_no;
	int numc,j,ref_frame,i;
	int vec1_x,vec1_y,vec2_x,vec2_y;
	int pic_block_x,pic_block_y,block_x,ref_inx,block_y,pic_pix_y,pic4_pix_y,pic_pix_x,pic4_pix_x,i22,lv,hv;
	int all_mv[4][4][MAX_MULT_PRED][9][2]; 
	int center_x,s_pos_x,ip0,ip1, center_y,blocktype,s_pos_y,iy0,jy0,center_h2,curr_search_range,center_v2;
	int s_pos_x1,s_pos_y1,s_pos_x2,s_pos_y2;
	int abort_search;
	int mb_y,mb_x;
	int tot_inter_sad, best_inter_sad, current_inter_sad;
	int mo[16][16];
	int blockshape_x,blockshape_y;
	int tmp0,tmp1;
	int mb_nr = img->current_mb_nr;
	int mb_width = img->width/16;
	int mb_available_up      = (img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width]);
	int mb_available_left    = (img->mb_x == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-1]);
	int mb_available_upleft  = (img->mb_x == 0 || img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width-1]);
	int mb_available_upright = (img->mb_x >= mb_width-1 || img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width+1]);
	int block_available_up, block_available_left, block_available_upright, block_available_upleft;
	int mv_a, mv_b, mv_c, mv_d, pred_vec;
	int mvPredType, rFrameL, rFrameU, rFrameUR;
   
	*min_fw_sad=MAX_VALUE; /* start value for inter SAD search */

	/*  Loop through all reference frames under consideration */  
	for (ref_frame=0; ref_frame < min(img->number,img->no_multpred); ref_frame++) 
	{
		for (j = 0;j < 4;j++)
		{
			for (i = 0;i < 4;i++)
			{
				fw_refFrArr[img->block_y+j][img->block_x+i] = ref_frame;
			}
		}

		/* find  reference image */
		ref_inx=(img->number-ref_frame-1)%img->no_multpred; 

		/*  Looping through all the chosen block sizes: */        
		blocktype=1;      
		while (inp_par->blc_size[blocktype][0]==0 && blocktype<=7) /* skip blocksizes not chosen */
			blocktype++;

		for (;blocktype <= 7;) 
		{   
			blockshape_x=inp_par->blc_size[blocktype][0];/* inp_par->blc_size has information of the 7 blocksizes */
			blockshape_y=inp_par->blc_size[blocktype][1];/* array iz stores offset inside one MB */           
      
			/*  curr_search_range is the actual search range used depending on block size and reference frame.  
			  It may be reduced by 1/2 or 1/4 for smaller blocks and prediction from older frames due to compexity */
			curr_search_range=inp_par->search_range/((min(ref_frame,1)+1) * min(2,blocktype));

			tot_inter_sad=QP2QUANT[img->qp] * min(ref_frame,1) * 2; /* start 'handicap ' */

			/*  Loop through the hole MB with all block sizes  */             
			for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += blockshape_y) 
			{
				block_available_up = mb_available_up || (mb_y > 0);
				
				block_y=mb_y/BLOCK_SIZE;
				pic_block_y=img->block_y+block_y;
				pic_pix_y=img->pix_y+mb_y;
				pic4_pix_y=pic_pix_y*4;

				for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += blockshape_x) 
				{
					block_x=mb_x/BLOCK_SIZE;
					pic_block_x=img->block_x+block_x;
					pic_pix_x=img->pix_x+mb_x;
					pic4_pix_x=pic_pix_x*4;          

					block_available_left = mb_available_left || (mb_x > 0);

					if (mb_y > 0)
						block_available_upright = mb_x+blockshape_x != MB_BLOCK_SIZE ? 1 : 0;
					else if (mb_x+blockshape_x != MB_BLOCK_SIZE)
						block_available_upright = block_available_up;
					else
						block_available_upright = mb_available_upright;

					if (mb_x > 0)
						block_available_upleft = mb_y > 0 ? 1 : block_available_up;
					else if (mb_y > 0)
						block_available_upleft = block_available_left;
					else
						block_available_upleft = mb_available_upleft;

					mvPredType = MVPRED_MEDIAN;

					rFrameL    = block_available_left    ? fw_refFrArr[pic_block_y][pic_block_x-1]   : -1;
					rFrameU    = block_available_up      ? fw_refFrArr[pic_block_y-1][pic_block_x]   : -1;
					rFrameUR   = block_available_upright ? fw_refFrArr[pic_block_y-1][pic_block_x+blockshape_x/4] :
					             block_available_upleft  ? fw_refFrArr[pic_block_y-1][pic_block_x-1] : -1;

					if(rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
						mvPredType = MVPRED_L;
					else if(rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
						mvPredType = MVPRED_U;
					else if(rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
						mvPredType = MVPRED_UR;

					else if(blockshape_x == 8 && blockshape_y == 16)
					{
						if(mb_x == 0)
						{
							if(rFrameL == ref_frame)
								mvPredType = MVPRED_L;
						}
						else
						{
							if(rFrameUR == ref_frame)
								mvPredType = MVPRED_UR;
						}
					}
					else if(blockshape_x == 16 && blockshape_y == 8)
					{
						if(mb_y == 0)
						{
							if(rFrameU == ref_frame)
								mvPredType = MVPRED_U;
						}
						else
						{
							if(rFrameL == ref_frame)
								mvPredType = MVPRED_L;
						}
					}
					else if(blockshape_x == 8 && blockshape_y == 4 && mb_x == 8)
						mvPredType = MVPRED_L;
					else if(blockshape_x == 4 && blockshape_y == 8 && mb_y == 8)
						mvPredType = MVPRED_U;

					for (hv=0; hv < 2; hv++)
					{

						mv_a = block_available_left    ? tmp_fwMV[hv][pic_block_y][pic_block_x-1+4]   : 0;
						mv_b = block_available_up      ? tmp_fwMV[hv][pic_block_y-1][pic_block_x+4]   : 0;
						mv_d = block_available_upleft  ? tmp_fwMV[hv][pic_block_y-1][pic_block_x-1+4] : 0;
						mv_c = block_available_upright ? tmp_fwMV[hv][pic_block_y-1][pic_block_x+blockshape_x/4+4] : mv_d;

						switch (mvPredType)
						{
						case MVPRED_MEDIAN:
							if(!(block_available_upleft || block_available_up || block_available_upright))
								pred_vec = mv_a;
							else
								pred_vec = mv_a+mv_b+mv_c-min(mv_a,min(mv_b,mv_c))-max(mv_a,max(mv_b,mv_c));

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

						img->p_fwMV[block_x][block_y][ref_frame][blocktype][hv]=pred_vec;
					}

					ip0=img->p_fwMV[block_x][block_y][ref_frame][blocktype][0];
					ip1=img->p_fwMV[block_x][block_y][ref_frame][blocktype][1];

					/* Integer pel search.  center_x,center_y is the 'search center'*/          
					center_x=ip0/4;
					center_y=ip1/4;          

					/* Limitation of center_x,center_y so that the search wlevel ow contains the (0,0) vector*/
					center_x=max(-(inp_par->search_range),min(inp_par->search_range,center_x));
					center_y=max(-(inp_par->search_range),min(inp_par->search_range,center_y));

					/* Search center corrected to prevent vectors outside the frame, this is not permitted in this model. */          
					center_x=max(min(center_x,img->width-1-pic_pix_x-(blockshape_x-1)-curr_search_range),curr_search_range-pic_pix_x);
					center_y=max(min(center_y,img->height_err-pic_pix_y-(blockshape_y-1)-curr_search_range),curr_search_range-pic_pix_y);

					/*  mo() is the the original block to be used in the search process */
					for (i=0; i < blockshape_x; i++) 
						for (j=0; j < blockshape_y; j++) 
							mo[i][j]=imgY_org[pic_pix_y+j][pic_pix_x+i];

					best_inter_sad=MAX_VALUE;

					numc=(curr_search_range*2+1)*(curr_search_range*2+1);

					for (lv=0; lv < numc; lv++) 
					{
						s_pos_x=(center_x+img->spiral_search[0][lv])*4;
						s_pos_y=(center_y+img->spiral_search[1][lv])*4;

						iy0=pic4_pix_x+s_pos_x;
						jy0=pic4_pix_y+s_pos_y;

						/*  
						The initial setting of current_inter_sad reflects the number of bits used to signal the vector.  
						It also depends on the QP. (See documented approach of RD constrained motion search) 
						*/
						tmp0=s_pos_x-ip0;
						tmp1=s_pos_y-ip1;
						current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));

						abort_search=FALSE;

						for (j=0; j < blockshape_y && !abort_search; j++) 
						{
							vec2_y=jy0+j*4;
                
							for (i=0; i < blockshape_x ; i++)
							{
								tmp0=mo[i][j]-mref[ref_inx][vec2_y][iy0+i*4];

								current_inter_sad += absm(tmp0);
							}
							if (current_inter_sad >= best_inter_sad) 
							{
								abort_search=TRUE;
							}
						}
						if(!abort_search)
						{
							center_h2=s_pos_x;
							center_v2=s_pos_y;
							best_inter_sad=current_inter_sad;
						}
					}
					center_h2=max(2-pic4_pix_x,min(center_h2,(img->width-1-pic_pix_x-(blockshape_x-1))*4-2));
					center_v2=max(2-pic4_pix_y,min(center_v2,(img->height_err-pic_pix_y-(blockshape_y-1))*4-2));
         
					/*  1/2 pixel search.  In this version the search is over 9 vector positions. */
					best_inter_sad=MAX_VALUE;
					for (lv=0; lv < 9; lv++) 
					{
						s_pos_x=center_h2+img->spiral_search[0][lv]*2;
						s_pos_y=center_v2+img->spiral_search[1][lv]*2;
						iy0=pic4_pix_x+s_pos_x;
						jy0=pic4_pix_y+s_pos_y;
						tmp0=s_pos_x-ip0;
						tmp1=s_pos_y-ip1;
						current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));

						for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4) 
						{
							for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4) 
							{
								for (i=0; i < BLOCK_SIZE; i++) 
								{
									vec2_x=i+vec1_x;
									i22=iy0+vec2_x*4;
									for (j=0; j < BLOCK_SIZE; j++) 
									{
										vec2_y=j+vec1_y;
										img->m7[i][j]=mo[vec2_x][vec2_y]-mref[ref_inx][jy0+vec2_y*4][i22];
									}
								}
								current_inter_sad += find_sad(img,inp_par->hadamard);
							}
						}
						if (current_inter_sad < best_inter_sad) 
						{
							s_pos_x1=s_pos_x;
							s_pos_y1=s_pos_y;
							s_pos_x2=s_pos_x;
							s_pos_y2=s_pos_y;

							for (i=0; i < blockshape_x/BLOCK_SIZE; i++) 
							{
								for (j=0; j < blockshape_y/BLOCK_SIZE; j++) 
								{
									all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=s_pos_x1;
									all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=s_pos_y1;

									tmp_fwMV[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_x1;
									tmp_fwMV[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_y1;
								}
							}
							best_inter_sad=current_inter_sad;
						}
					}
				
					/*  1/4 pixel search.   */
					s_pos_x2=max(1-pic4_pix_x,min(s_pos_x2,(img->width-1-pic_pix_x-(blockshape_x-1))*4-1));
					s_pos_y2=max(1-pic4_pix_y,min(s_pos_y2,(img->height_err-pic_pix_y-(blockshape_y-1))*4-1));
					for (lv=1; lv < 9; lv++) 
					{
						s_pos_x=s_pos_x2+img->spiral_search[0][lv];
						s_pos_y=s_pos_y2+img->spiral_search[1][lv];

						iy0=pic4_pix_x+s_pos_x;
						jy0=pic4_pix_y+s_pos_y;
						tmp0=s_pos_x-ip0;
						tmp1=s_pos_y-ip1;
						current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));
              
						for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4) 
						{
							for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4) 
							{
								for (i=0; i < BLOCK_SIZE; i++) 
								{
									vec2_x=i+vec1_x;
									i22=iy0+vec2_x*4;
									for (j=0; j < BLOCK_SIZE; j++) 
									{
										vec2_y=j+vec1_y;
										img->m7[i][j]=mo[vec2_x][vec2_y]-mref[ref_inx][jy0+vec2_y*4][i22];
									}
								}
								current_inter_sad += find_sad(img,inp_par->hadamard);
							}
						}
						if (current_inter_sad < best_inter_sad) 
						{
							s_pos_x1=s_pos_x;
							s_pos_y1=s_pos_y;
                
							for (i=0; i < blockshape_x/BLOCK_SIZE; i++) 
							{
								for (j=0; j < blockshape_y/BLOCK_SIZE; j++) 
								{
									all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=s_pos_x1;
									all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=s_pos_y1;
									tmp_fwMV[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_x1;
									tmp_fwMV[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_y1;
								}
							}
							best_inter_sad=current_inter_sad;
						}
					}            
					tot_inter_sad += best_inter_sad;
				}
			} // inside MB
      
			if (tot_inter_sad <= *min_fw_sad) 
			{
				if(blocktype==1) img->fw_mb_mode=blocktype;
				else			 img->fw_mb_mode=blocktype*2;
				img->fw_blc_size_h=inp_par->blc_size[blocktype][0];
				img->fw_blc_size_v=inp_par->blc_size[blocktype][1];
				fw_predframe_no=ref_frame;
				img->fw_multframe_no=ref_inx;
				*min_fw_sad=tot_inter_sad;
			}       
			while (inp_par->blc_size[++blocktype][0]==0 && blocktype<=7); /* only go through chosen blocksizes */ 
		}   // blocktype
	} // reference frame
	// after the loop, we can get best blocktype, refernce frame no, min_fw_sad

	// save forward MV to tmp_fwMV
	for (hv=0; hv < 2; hv++) 
		for (i=0; i < 4; i++) 
			for (j=0; j < 4; j++) {
				if(img->fw_mb_mode==1)  
					tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=all_mv[i][j][fw_predframe_no][1][hv];
				else
					tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=all_mv[i][j][fw_predframe_no][(img->fw_mb_mode)/2][hv];
			}

	return fw_predframe_no;  
}

void get_bwMV(int *min_bw_sad, struct img_par *img, struct inp_par *inp_par)
{
	int bw_predframe_no;
	int numc,j,i;
	int vec1_x,vec1_y,vec2_x,vec2_y;
	int pic_block_x,pic_block_y,block_x, ref_frame, ref_inx,block_y,
		pic_pix_y,pic4_pix_y,pic_pix_x,pic4_pix_x,i22,lv,hv;
	int all_mv[4][4][MAX_MULT_PRED][17][2]; 
	int center_x,s_pos_x,ip0,ip1,center_y,blocktype,s_pos_y,iy0,jy0,center_h2,curr_search_range,center_v2;
	int s_pos_x1,s_pos_y1,s_pos_x2,s_pos_y2;
	int abort_search;
	int mb_y,mb_x;
	int tot_inter_sad, best_inter_sad, current_inter_sad;
	int mo[16][16];
	int blockshape_x,blockshape_y;
	int tmp0,tmp1;
	int mb_nr = img->current_mb_nr;
	int mb_width = img->width/16;
	int mb_available_up      = (img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width]);
	int mb_available_left    = (img->mb_x == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-1]);
	int mb_available_upleft  = (img->mb_x == 0 || img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width-1]);
	int mb_available_upright = (img->mb_x >= mb_width-1 || img->mb_y == 0) ? 0 : (img->slice_numbers[mb_nr] == img->slice_numbers[mb_nr-mb_width+1]);
	int block_available_up, block_available_left, block_available_upright, block_available_upleft;
	int mv_a, mv_b, mv_c, mv_d, pred_vec;
	int mvPredType, rFrameL, rFrameU, rFrameUR;

	*min_bw_sad=MAX_VALUE; /* start value for inter SAD search */

	/* find  reference image */
	ref_frame=0;

	for (j = 0;j < 4;j++)
	{
		for (i = 0;i < 4;i++)
		{
			bw_refFrArr[img->block_y+j][img->block_x+i] = ref_frame;
		}
	}

	ref_inx=(img->number-ref_frame)%img->no_multpred;  
  
	/*  Looping through all the chosen block sizes: */        
	blocktype=1;      
	while (inp_par->blc_size[blocktype][0]==0 && blocktype<=7) /* skip blocksizes not chosen */
		blocktype++;
    
    for (;blocktype <= 7;) 
    {   
		blockshape_x=inp_par->blc_size[blocktype][0];/* inp_par->blc_size has information of the 7 blocksizes */
		blockshape_y=inp_par->blc_size[blocktype][1];/* array iz stores offset inside one MB */           

		/*  curr_search_range is the actual search range used depending on block size and reference frame.  
          It may be reduced by 1/2 or 1/4 for smaller blocks and prediction from older frames due to compexity */
		curr_search_range=inp_par->search_range/((min(ref_frame,1)+1) * min(2,blocktype));

		tot_inter_sad=QP2QUANT[img->qp] * min(ref_frame,1) * 2; /* start 'handicap ' */

		/*  Loop through the hole MB with all block sizes  */             
		for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += blockshape_y) 
		{
			block_available_up = mb_available_up || (mb_y > 0);

			block_y=mb_y/BLOCK_SIZE;
			pic_block_y=img->block_y+block_y;
			pic_pix_y=img->pix_y+mb_y;
			pic4_pix_y=pic_pix_y*4;

			for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += blockshape_x) 
			{
				block_x=mb_x/BLOCK_SIZE;
				pic_block_x=img->block_x+block_x;
				pic_pix_x=img->pix_x+mb_x;
				pic4_pix_x=pic_pix_x*4;

				block_available_left = mb_available_left || (mb_x > 0);

				if (mb_y > 0)
					block_available_upright = mb_x+blockshape_x != MB_BLOCK_SIZE ? 1 : 0;
				else if (mb_x+blockshape_x != MB_BLOCK_SIZE)
					block_available_upright = block_available_up;
				else
					block_available_upright = mb_available_upright;

				if (mb_x > 0)
					block_available_upleft = mb_y > 0 ? 1 : block_available_up;
				else if (mb_y > 0)
					block_available_upleft = block_available_left;
				else
					block_available_upleft = mb_available_upleft;

				mvPredType = MVPRED_MEDIAN;

				rFrameL    = block_available_left    ? bw_refFrArr[pic_block_y][pic_block_x-1]   : -1;
				rFrameU    = block_available_up      ? bw_refFrArr[pic_block_y-1][pic_block_x]   : -1;
				rFrameUR   = block_available_upright ? bw_refFrArr[pic_block_y-1][pic_block_x+blockshape_x/4] :
					         block_available_upleft  ? bw_refFrArr[pic_block_y-1][pic_block_x-1] : -1;

				if(rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
					mvPredType = MVPRED_L;
				else if(rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
					mvPredType = MVPRED_U;
				else if(rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
					mvPredType = MVPRED_UR;

				else if(blockshape_x == 8 && blockshape_y == 16)
				{
					if(mb_x == 0)
					{
						if(rFrameL == ref_frame)
							mvPredType = MVPRED_L;
					}
					else
					{
						if(rFrameUR == ref_frame)
							mvPredType = MVPRED_UR;
					}
				}
				else if(blockshape_x == 16 && blockshape_y == 8)
				{
					if(mb_y == 0)
					{
						if(rFrameU == ref_frame)
							mvPredType = MVPRED_U;
					}
					else
					{
						if(rFrameL == ref_frame)
							mvPredType = MVPRED_L;
					}
				}
				else if(blockshape_x == 8 && blockshape_y == 4 && mb_x == 8)
					mvPredType = MVPRED_L;
				else if(blockshape_x == 4 && blockshape_y == 8 && mb_y == 8)
					mvPredType = MVPRED_U;

				for (hv=0; hv < 2; hv++)
				{
					mv_a = block_available_left    ? tmp_bwMV[hv][pic_block_y][pic_block_x-1+4]   : 0;
					mv_b = block_available_up      ? tmp_bwMV[hv][pic_block_y-1][pic_block_x+4]   : 0;
					mv_d = block_available_upleft  ? tmp_bwMV[hv][pic_block_y-1][pic_block_x-1+4] : 0;
					mv_c = block_available_upright ? tmp_bwMV[hv][pic_block_y-1][pic_block_x+blockshape_x/4+4] : mv_d;

					switch (mvPredType)
					{
					case MVPRED_MEDIAN:
						if(!(block_available_upleft || block_available_up || block_available_upright))
							pred_vec = mv_a;
						else
							pred_vec = mv_a+mv_b+mv_c-min(mv_a,min(mv_b,mv_c))-max(mv_a,max(mv_b,mv_c));

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

					img->p_bwMV[block_x][block_y][ref_frame][blocktype][hv]=pred_vec;
				}
          
				ip0=img->p_bwMV[block_x][block_y][ref_frame][blocktype][0];
				ip1=img->p_bwMV[block_x][block_y][ref_frame][blocktype][1];

				/* Integer pel search.  center_x,center_y is the 'search center'*/          
				center_x=ip0/4;
				center_y=ip1/4;          

				/* Limitation of center_x,center_y so that the search wlevel ow contains the (0,0) vector*/
				center_x=max(-(inp_par->search_range),min(inp_par->search_range,center_x));
				center_y=max(-(inp_par->search_range),min(inp_par->search_range,center_y));

				/* Search center corrected to prevent vectors outside the frame, this is not permitted in this model. */          
				center_x=max(min(center_x,img->width-1-pic_pix_x-(blockshape_x-1)-curr_search_range),curr_search_range-pic_pix_x);
				center_y=max(min(center_y,img->height_err-pic_pix_y-(blockshape_y-1)-curr_search_range),curr_search_range-pic_pix_y);

				/*  mo() is the the original block to be used in the search process */
				for (i=0; i < blockshape_x; i++) 
					for (j=0; j < blockshape_y; j++) 
						mo[i][j]=imgY_org[pic_pix_y+j][pic_pix_x+i];

				best_inter_sad=MAX_VALUE;
            
				numc=(curr_search_range*2+1)*(curr_search_range*2+1);

				for (lv=0; lv < numc; lv++) 
				{
					s_pos_x=(center_x+img->spiral_search[0][lv])*4;
					s_pos_y=(center_y+img->spiral_search[1][lv])*4;

					iy0=pic4_pix_x+s_pos_x;
					jy0=pic4_pix_y+s_pos_y;

					tmp0=s_pos_x-ip0;
					tmp1=s_pos_y-ip1;
					current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));

					abort_search=FALSE;
					for (j=0; j < blockshape_y && !abort_search; j++) 
					{
						vec2_y=jy0+j*4;

						for (i=0; i < blockshape_x ; i++)
						{
							tmp0=mo[i][j]-mref_P[vec2_y][iy0+i*4];

							current_inter_sad += absm(tmp0);
						}
						if (current_inter_sad >= best_inter_sad) 
						{
							abort_search=TRUE;
						}
					}
					if(!abort_search)
					{
						center_h2=s_pos_x;
						center_v2=s_pos_y;
						best_inter_sad=current_inter_sad;
					}
				}
            
				center_h2=max(2-pic4_pix_x,min(center_h2,(img->width-1-pic_pix_x-(blockshape_x-1))*4-2));
				center_v2=max(2-pic4_pix_y,min(center_v2,(img->height_err-pic_pix_y-(blockshape_y-1))*4-2));

				/*  1/2 pixel search.  In this version the search is over 9 vector positions. */
				best_inter_sad=MAX_VALUE;
				for (lv=0; lv < 9; lv++) 
				{
					s_pos_x=center_h2+img->spiral_search[0][lv]*2;
					s_pos_y=center_v2+img->spiral_search[1][lv]*2;
					iy0=pic4_pix_x+s_pos_x;
					jy0=pic4_pix_y+s_pos_y;
					tmp0=s_pos_x-ip0;
					tmp1=s_pos_y-ip1;
					current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));
              
					for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4) 
					{
						for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4) 
						{
							for (i=0; i < BLOCK_SIZE; i++) 
							{
								vec2_x=i+vec1_x;
								i22=iy0+vec2_x*4;
								for (j=0; j < BLOCK_SIZE; j++) 
								{
									vec2_y=j+vec1_y;
									img->m7[i][j]=mo[vec2_x][vec2_y]-mref_P[jy0+vec2_y*4][i22];
								}
							}
							current_inter_sad += find_sad(img,inp_par->hadamard);
						}
					}
					if (current_inter_sad < best_inter_sad) 
					{
						/*  Vectors are saved in all_mv[] to be able to assign correct vectors to each block after mode selection.
							tmp_mv[] is a 'temporary' assignment of vectors to be used to estimate 'bit cost' in vector prediction.
						*/
						s_pos_x1=s_pos_x;
						s_pos_y1=s_pos_y;
						s_pos_x2=s_pos_x;
						s_pos_y2=s_pos_y;

						for (i=0; i < blockshape_x/BLOCK_SIZE; i++) 
						{
							for (j=0; j < blockshape_y/BLOCK_SIZE; j++) 
							{
								all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=s_pos_x1;
								all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=s_pos_y1;

								tmp_bwMV[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_x1;
								tmp_bwMV[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_y1;
							}
						}
						best_inter_sad=current_inter_sad;
					}
				}
            
				/*  1/4 pixel search.   */
 
				s_pos_x2=max(1-pic4_pix_x,min(s_pos_x2,(img->width-1-pic_pix_x-(blockshape_x-1))*4-1));
				s_pos_y2=max(1-pic4_pix_y,min(s_pos_y2,(img->height_err-pic_pix_y-(blockshape_y-1))*4-1));
				for (lv=1; lv < 9; lv++) 
				{
					s_pos_x=s_pos_x2+img->spiral_search[0][lv];
					s_pos_y=s_pos_y2+img->spiral_search[1][lv];
             
					iy0=pic4_pix_x+s_pos_x;
					jy0=pic4_pix_y+s_pos_y;
					tmp0=s_pos_x-ip0;
					tmp1=s_pos_y-ip1;
					current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));

					for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4) 
					{
						for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4) 
						{
							for (i=0; i < BLOCK_SIZE; i++) 
							{
								vec2_x=i+vec1_x;
								i22=iy0+vec2_x*4;
								for (j=0; j < BLOCK_SIZE; j++) 
								{
									vec2_y=j+vec1_y;
									img->m7[i][j]=mo[vec2_x][vec2_y]-mref_P[jy0+vec2_y*4][i22];
								}
							}
							current_inter_sad += find_sad(img,inp_par->hadamard);
						}
					}
					if (current_inter_sad < best_inter_sad) 
					{
						/*  Vectors are saved in all_mv[] to be able to assign correct vectors to each block after mode selection.
							tmp_mv[] is a 'temporary' assignment of vectors to be used to estimate 'bit cost' in vector prediction.
						*/

						s_pos_x1=s_pos_x;
						s_pos_y1=s_pos_y;
                
						for (i=0; i < blockshape_x/BLOCK_SIZE; i++) 
						{
							for (j=0; j < blockshape_y/BLOCK_SIZE; j++) 
							{
								all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=s_pos_x1;
								all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=s_pos_y1;
								tmp_bwMV[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_x1;
								tmp_bwMV[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_y1;
							}
						}
						best_inter_sad=current_inter_sad;
					}
				}
            
				tot_inter_sad += best_inter_sad;
			}
		}
      
		if (tot_inter_sad <= *min_bw_sad) 
		{
			if(blocktype==1) img->bw_mb_mode=blocktype+1;
			else			 img->bw_mb_mode=blocktype*2+1;
			img->bw_blc_size_h=inp_par->blc_size[blocktype][0];
			img->bw_blc_size_v=inp_par->blc_size[blocktype][1];
			bw_predframe_no=ref_frame; 
			img->bw_multframe_no=ref_inx;
			*min_bw_sad=tot_inter_sad;
		}       
		while (inp_par->blc_size[++blocktype][0]==0 && blocktype<=7); /* only go through chosen blocksizes */ 
    }  // blocktype
	// after the loop, we can get best blocktype, and min_bw_sad

	// save backward MV to tmp_bwMV
	for (hv=0; hv < 2; hv++) 
		for (i=0; i < 4; i++) 
			for (j=0; j < 4; j++) {
				if(img->bw_mb_mode==2) 
					tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=all_mv[i][j][bw_predframe_no][1][hv];
				else 
					tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=all_mv[i][j][bw_predframe_no][(img->bw_mb_mode-1)/2][hv];
			}
}

void get_bid(int *bid_sad, int fw_predframe_no, struct img_par *img, struct inp_par *inp)
{
	int mb_y,mb_x, block_y, block_x, pic_pix_y, pic_pix_x, pic_block_y, pic_block_x;
	int i, j, ii4, jj4, iii4, jjj4, i2, j2; 
	int fw_pred, bw_pred, bid_pred[4][4];
	int code_num, step_h, step_v, mvd_x, mvd_y;

	// consider code number of fw_predframe_no
	*bid_sad = QP2QUANT[img->qp]*min(fw_predframe_no,1)*2;

	// consider bits of fw_blk_size 
	if(img->fw_blc_size_h==16 && img->fw_blc_size_v==16)			// 16x16 : blocktype 1
		code_num=0;
	else if(img->fw_blc_size_h==16 && img->fw_blc_size_v==8)   // 16x8 : blocktype 2
		code_num=1;						
	else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==16)  // 8x16 : blocktype 3
		code_num=2;						
	else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==8)  // 8x8 : blocktype 4
		code_num=3;
	else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==4)   // 8x4 : blocktype 5
		code_num=4;
	else if(img->fw_blc_size_h==4 && img->fw_blc_size_v==8)   // 4x8 : blocktype 6
		code_num=5;
	else				// 4x4 : blocktype 7
		code_num=6;
	*bid_sad += QP2QUANT[img->qp]*img->blk_bituse[code_num];	

	// consider bits of bw_blk_size
	if(img->bw_blc_size_h==16 && img->bw_blc_size_v==16)			// 16x16 : blocktype 1
		code_num=0;
	else if(img->bw_blc_size_h==16 && img->bw_blc_size_v==8)   // 16x8 : blocktype 2
		code_num=1;						
	else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==16)  // 8x16 : blocktype 3
		code_num=2;						
	else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==8)  // 8x8 : blocktype 4
		code_num=3;
	else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==4)   // 8x4 : blocktype 5
		code_num=4;
	else if(img->bw_blc_size_h==4 && img->bw_blc_size_v==8)   // 4x8 : blocktype 6
		code_num=5;
	else				// 4x4 : blocktype 7
		code_num=6;
	*bid_sad += QP2QUANT[img->qp]*img->blk_bituse[code_num];	

	// consider bits of mvdfw
	step_h=img->fw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize 
	step_v=img->fw_blc_size_v/BLOCK_SIZE;  // vertical stepsize 

	for (j=0; j < BLOCK_SIZE; j += step_v) 
	{            
		for (i=0;i < BLOCK_SIZE; i += step_h) 
		{
			if(img->fw_blc_size_h==16 && img->fw_blc_size_v==16) {			// 16x16 : blocktype 1
				mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][0];
				mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][1];
			}
			else if(img->fw_blc_size_h==16 && img->fw_blc_size_v==8) {  // 16x8 : blocktype 2
				mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][2][0];
				mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][2][1];					
			}
			else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==16) { // 8x16 : blocktype 3
				mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][3][0];
				mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][3][1];									
			}
			else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==8) { // 8x8 : blocktype 4
				mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][4][0];
				mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][4][1];									
			}
			else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==4) {  // 8x4 : blocktype 5
				mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][5][0];
				mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][5][1];									
			}
			else if(img->fw_blc_size_h==4 && img->fw_blc_size_v==8) {  // 4x8 : blocktype 6
				mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][6][0];
				mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][6][1];									
			}
			else {																											// 4x4 : blocktype 7 
				mvd_x=tmp_fwMV[0][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][7][0];
				mvd_y=tmp_fwMV[1][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][7][1];									
			}
			*bid_sad += QP2QUANT[img->qp]*(img->mv_bituse[absm(mvd_x)]+img->mv_bituse[absm(mvd_y)]);
		}
	}

	// consider bits of mvdbw
	step_h=img->bw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize 
	step_v=img->bw_blc_size_v/BLOCK_SIZE;  // vertical stepsize 
	
	for (j=0; j < BLOCK_SIZE; j += step_v) 
	{            
		for (i=0;i < BLOCK_SIZE; i += step_h) 
		{
			if(img->bw_blc_size_h==16 && img->bw_blc_size_v==16) {			// 16x16 : blocktype 1
				mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][0];
				mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][1];
			}
			else if(img->bw_blc_size_h==16 && img->bw_blc_size_v==8) {  // 16x8 : blocktype 2
				mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][2][0];
				mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][2][1];					
			}
			else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==16) { // 8x16 : blocktype 3
				mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][3][0];
				mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][3][1];									
			}
			else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==8) { // 8x8 : blocktype 4
				mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][4][0];
				mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][4][1];									
			}
			else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==4) {  // 8x4 : blocktype 5
				mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][5][0];
				mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][5][1];									
			}
			else if(img->bw_blc_size_h==4 && img->bw_blc_size_v==8) {  // 4x8 : blocktype 6
				mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][6][0];
				mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][6][1];									
			}
			else {																											// 4x4 : blocktype 7 
				mvd_x=tmp_bwMV[0][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][7][0];
				mvd_y=tmp_bwMV[1][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][7][1];									
			}
			*bid_sad += QP2QUANT[img->qp]*(img->mv_bituse[absm(mvd_x)]+img->mv_bituse[absm(mvd_y)]);
		}
	}

	for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2) 
	{
		for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2) 
		{  
			for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE) 
			{
				pic_pix_y=img->pix_y+block_y;
				pic_block_y=pic_pix_y/BLOCK_SIZE;
  
				for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE) 
				{
					pic_pix_x=img->pix_x+block_x;
					pic_block_x=pic_pix_x/BLOCK_SIZE;

					ii4=(img->pix_x+block_x)*4+tmp_fwMV[0][pic_block_y][pic_block_x+4];
					jj4=(img->pix_y+block_y)*4+tmp_fwMV[1][pic_block_y][pic_block_x+4];
					iii4=(img->pix_x+block_x)*4+tmp_bwMV[0][pic_block_y][pic_block_x+4];
					jjj4=(img->pix_y+block_y)*4+tmp_bwMV[1][pic_block_y][pic_block_x+4];

					// DM : To prevent mv outside the frame
					if((jj4>=img->height*4-12)||(jj4<0)||(ii4>=img->width*4-12)||(ii4<0)
						||(jjj4>=img->height*4-12)||(jjj4<0)||(iii4>=img->width*4-12)||(iii4<0))
					{
						*bid_sad= MAX_DIR_SAD; 
						return;
					}

					for (j=0;j<4;j++) {
						j2=j*4;
						for (i=0;i<4;i++) {
							i2=i*4;
							fw_pred=mref[img->fw_multframe_no][jj4+j2][ii4+i2];
							bw_pred=mref_P[jjj4+j2][iii4+i2];
							bid_pred[i][j]=(int)((fw_pred+bw_pred)/2.0+0.5);
						}
					}
						
					for (j=0; j < BLOCK_SIZE; j++) {
						for (i=0; i < BLOCK_SIZE; i++) {
							img->m7[i][j]=imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-bid_pred[i][j];
						}
					}	
					*bid_sad += find_sad(img, inp->hadamard);
				}
			}	
		}
	}
}

void get_dir(int *dir_sad, struct img_par *img, struct inp_par *inp)
{
	int mb_y,mb_x, block_y, block_x, pic_pix_y, pic_pix_x, pic_block_y, pic_block_x;
	int i, j, ii4, jj4, iii4, jjj4, i2, j2, hv; 
	int ref_inx, df_pred, db_pred, dir_pred[4][4];
	int refP_tr, TRb, TRp;

	// initialize with bias value
	*dir_sad=-QP2QUANT[img->qp] * 16; 

	// create dfMV, dbMV
	for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2) 
	{
		for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2) 
		{  
			for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE) 
			{
				pic_pix_y=img->pix_y+block_y;
				pic_block_y=pic_pix_y/BLOCK_SIZE;

				for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE) 
				{
					pic_pix_x=img->pix_x+block_x;
					pic_block_x=pic_pix_x/BLOCK_SIZE;
	
					// next P is intra mode
					if(refFrArr[pic_block_y][pic_block_x]==-1)
					{
						for(hv=0; hv<2; hv++) 
						{
							dfMV[hv][pic_block_y][pic_block_x+4]=dbMV[hv][pic_block_y][pic_block_x+4]=0;
						}
					}
					// next P is skip or inter mode
					else 
					{
						refP_tr=nextP_tr-((refFrArr[pic_block_y][pic_block_x]+1)*img->p_interval);
						TRb=img->tr-refP_tr;
						TRp=nextP_tr-refP_tr;
						for(hv=0; hv<2; hv++) 
						{
							dfMV[hv][pic_block_y][pic_block_x+4]=TRb*tmp_mv[hv][pic_block_y][pic_block_x+4]/TRp;
							dbMV[hv][pic_block_y][pic_block_x+4]=(TRb-TRp)*tmp_mv[hv][pic_block_y][pic_block_x+4]/TRp;
						}
					}
				}
			}
		}		
	}

	// prediction
	for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2) 
	{
		for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2) 
		{
			for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE) 
			{
				pic_pix_y=img->pix_y+block_y;
				pic_block_y=pic_pix_y/BLOCK_SIZE;

				for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE) 
				{
					pic_pix_x=img->pix_x+block_x;
					pic_block_x=pic_pix_x/BLOCK_SIZE;

					ii4=(img->pix_x+block_x)*4+dfMV[0][pic_block_y][pic_block_x+4];
					jj4=(img->pix_y+block_y)*4+dfMV[1][pic_block_y][pic_block_x+4];
					iii4=(img->pix_x+block_x)*4+dbMV[0][pic_block_y][pic_block_x+4];
					jjj4=(img->pix_y+block_y)*4+dbMV[1][pic_block_y][pic_block_x+4];
      
					// LG : To prevent mv outside the frame
					if(	(jj4 >= (img->height*4- 12) )||(jj4<0)||
							(ii4 >= (img->width*4 - 12) )||(ii4<0) ||
							(jjj4>= (img->height*4- 12) )||(jjj4<0)||
							(iii4>= (img->width*4 - 12) )||(iii4<0) )
					{
						*dir_sad= MAX_DIR_SAD; 
						return;
					}
					else 
					{
						// next P is intra mode
						if(refFrArr[pic_block_y][pic_block_x]==-1)
							ref_inx=(img->number-1)%img->no_multpred;
						// next P is skip or inter mode
						else
							ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%img->no_multpred;

						for (j=0;j<4;j++) {
							j2=j*4;
							for (i=0;i<4;i++) {
								i2=i*4;
								df_pred=mref[ref_inx][jj4+j2][ii4+i2];								
								db_pred=mref_P[jjj4+j2][iii4+i2];
								dir_pred[i][j]=(int)((df_pred+db_pred)/2.0+0.5);                
							}
						}
							
						for (j=0; j < BLOCK_SIZE; j++) {
							for (i=0; i < BLOCK_SIZE; i++) {
								img->m7[i][j]=imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i]-dir_pred[i][j];
							}
						}
						*dir_sad += find_sad(img, inp->hadamard);
					} // else
				} // block_x
			} // block_y
		} // mb_x
	} // mb_y
}

void compare_sad(int tot_intra_sad, int fw_sad, int bw_sad, int bid_sad, int dir_sad, struct img_par *img)
{
	int hv, i, j;

	// LG : dfMV, dbMV reset
	if( (dir_sad<=tot_intra_sad) && (dir_sad<=fw_sad) && (dir_sad<=bw_sad) && (dir_sad<=bid_sad) ) {
		img->imod = B_Direct;
		img->mb_mode = 0;
		for(hv=0; hv<2; hv++)
			for(i=0; i<4; i++)
				for(j=0; j<4; j++)				
					tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=
					tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=0;
	}
	else if( (bw_sad<=tot_intra_sad) && (bw_sad<=fw_sad) && (bw_sad<=bid_sad) && (bw_sad<=dir_sad) ) {
		img->imod = B_Backward;
		img->mb_mode = img->bw_mb_mode;
		for(hv=0; hv<2; hv++)
			for(i=0; i<4; i++)
				for(j=0; j<4; j++)
					tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=
					dfMV[hv][img->block_y+j][img->block_x+i+4]=
					dbMV[hv][img->block_y+j][img->block_x+i+4]=0;
	}
	else if( (fw_sad<=tot_intra_sad) && (fw_sad<=bw_sad) && (fw_sad<=bid_sad) && (fw_sad<=dir_sad) ) {
		img->imod = B_Forward;
		img->mb_mode = img->fw_mb_mode;
		for(hv=0; hv<2; hv++)
			for(i=0; i<4; i++)
				for(j=0; j<4; j++)
					tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=
					dfMV[hv][img->block_y+j][img->block_x+i+4]=
					dbMV[hv][img->block_y+j][img->block_x+i+4]=0;
	}
	else if( (bid_sad<=tot_intra_sad) && (bid_sad<=fw_sad) && (bid_sad<=bw_sad) && (bid_sad<=dir_sad) ) {
		img->imod = B_Bidirect;
		img->mb_mode = 3;
		for(hv=0; hv<2; hv++)
			for(i=0; i<4; i++)
				for(j=0; j<4; j++)
					dfMV[hv][img->block_y+j][img->block_x+i+4]=
					dbMV[hv][img->block_y+j][img->block_x+i+4]=0;				
	}	
	else if( (tot_intra_sad<=dir_sad) && (tot_intra_sad<=bw_sad) && (tot_intra_sad<=fw_sad) && (tot_intra_sad<=bid_sad) ) 
	{
		img->mb_mode=img->imod+8*img->type; // img->type=2

		for(hv=0; hv<2; hv++)
			for(i=0; i<4; i++)
				for(j=0; j<4; j++)
					tmp_fwMV[hv][img->block_y+j][img->block_x+i+4]=
					tmp_bwMV[hv][img->block_y+j][img->block_x+i+4]=
					dfMV[hv][img->block_y+j][img->block_x+i+4]=
					dbMV[hv][img->block_y+j][img->block_x+i+4]=0;
	}
}
