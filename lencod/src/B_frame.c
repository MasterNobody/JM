/****************************************************************
 *
 *  File B_frame.c :	B picture coding 
 *
 *	Main contributor and Contact Information
 *
 *		Byeong-Moon Jeon			<jeonbm@lge.com>
 *      Yoon-Seong Soh				<yunsung@lge.com>
 *      LG Electronics Inc., Digital Media Research Lab.
 *      16 Woomyeon-Dong, Seocho-Gu, Seoul, 137-724, Korea
 *
 *****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "global.h"
#include "b_frame.h"

/************************************************************************
*  Name :    oneforthpix_1(), one_forthpix_2()
*
*  Description: 
*      Upsample 4 times, store them in mref_P, mcef_P for next P picture
*      and move them to mref, mcef
*
*  Changes: 
*	    Thomas Wedi, 12.01.2001		<wedi@tnt.uni-hannover.de>:
*	    Slightly changed to avoid mismatches with the direct 
*		interpolation filter in the decoder
*
************************************************************************/
void oneforthpix_1(struct img_par *img)
{
  // temporary solution:
  // int imgY_tmp[576][704];
  long int is;
  int i,j,i2,j2,j4;
  int ie2,je2;
  int uv;
  
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
	  mref_P[j4  ][i*2]=max(0,min(255,(int)((imgY_tmp[j2][i]+512)/1024)));     /* 1/2 pix */
	  mref_P[j4+2][i*2]=max(0,min(255,(int)((is+512)/1024))); /* 1/2 pix */
	}
    }
  
  /* 1/4 pix */
  /* luma */
  ie2=(img->width-1)*4;
  je2=(img->height-1)*4;
  
  for (j=0;j<je2+4;j+=2)
    for (i=0;i<ie2+3;i+=2)
      mref_P[j][i+1]=max(0,min(255,(int)(imgY_tmp[j/2][i/2]+imgY_tmp[j/2][min(ie2/2+1,i/2+1)]+1024)/2048));
  
  for (i=0;i<ie2+4;i++)
    {
      for (j=0;j<je2+3;j+=2)
	{
	  if( i%2 == 0 )
	    mref_P[j+1][i]=max(0,min(255,(int)(imgY_tmp[j/2][i/2]+imgY_tmp[min(je2/2+1,j/2+1)][i/2]+1024)/2048));
	  else
	    mref_P[j+1][i]=max(0,min(255,(int)(
							       imgY_tmp[j/2               ][i/2               ] +
							       imgY_tmp[min(je2/2+1,j/2+1)][i/2               ] +
							       imgY_tmp[j/2               ][min(ie2/2+1,i/2+1)] +
							       imgY_tmp[min(je2/2+1,j/2+1)][min(ie2/2+1,i/2+1)] + 2048)/4096));
	  
	  /* "funny posision" */
	  if( ((i&3) == 3)&&(((j+1)&3) == 3))
	    {
	      mref_P[j+1][i] = (mref_P[j-2         ][i-3         ] +
						mref_P[min(je2,j+2)][i-3         ] +
						mref_P[j-2         ][min(ie2,i+1)] +
						mref_P[min(je2,j+2)][min(ie2,i+1)] + 2 )/4;
	    }
	}
    }
  
  /*  Chroma: */
  
  for (uv=0; uv < 2; uv++)
    for (i=0; i < img->width_cr; i++)
	  for (j=0; j < img->height_cr; j++)
	    {
	      mcef_P[uv][j][i]=imgUV[uv][j][i];/* just copy 1/1 pix, interpolate "online" */
	    }
}


void oneforthpix_2(struct img_par *img)
{
	int i, j, uv;

	img->frame_cycle=img->number % MAX_MULT_PRED;

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

/*************************************************************************************
*
* Routine : initialize_Bframe( )
* Description : Get B_interval, img->type, qp, img->tr, 
*                  B_interval=(floor)(P_interval/(inp->successive_Bframe+1)
*               prevP_no = (img->number-1)*P_interval
*               nextP_no = img->number*P_interval
*               check if prevP_no+B_intervalxBframe_to_code is larger than nextP_no
*
*************************************************************************************/

void initialize_Bframe(int *B_interval, int successive_Bframe, int Bframe_to_code,
						struct img_par *img, struct inp_par *inp, struct stat_par *stat)
{
	int len, info, k, i,j;
	int prevP_no, nextP_no;

	img->current_mb_nr=0;
	img->current_slice_nr=0;
	stat->bit_slice = 0;

	P_interval = inp->jumpd+1;
	prevP_no = (img->number-1)*P_interval;
	nextP_no = img->number*P_interval;

	*B_interval = (int)((float)P_interval/(successive_Bframe+1.0)+0.5);
	img->type = B_IMG;
	img->qp = inp->qpB;
     
	img->tr= prevP_no+(*B_interval)*Bframe_to_code; // from prev_P
	if(img->tr >= nextP_no) 	
		img->tr=nextP_no-1;

	len=nal_put_startcode(img->tr%256, img->current_mb_nr, img->qp, inp->img_format, img->current_slice_nr, SEB);
	stat->bit_ctr += len;   /* keep bit usage */
	stat->bit_slice += len;
	stat->bit_use_head_mode[img->type] += len;

	if(inp->no_multpred == 1) {
		len=5; 
		info=0;
	}
	else {
		len=5;
		info=1;
	}
	sprintf(tracestring, "Image type ---- %d (B) ", img->type);
	put_symbol(tracestring,len,info, img->current_slice_nr, SEB);    /* write picture type to stream */
	stat->bit_ctr += len;
	stat->bit_slice += len;
	stat->bit_use_head_mode[img->type] += len;

	// create tmp_fwMV[2][72][92], tmp_bwMV[2][72][92], dfMV[2][72][92], dbMV[2][72][92] array by malloc()
	tmp_fwMV=(int***)malloc(sizeof(int)*2);
	tmp_bwMV=(int***)malloc(sizeof(int)*2);
	dfMV=(int***)malloc(sizeof(int)*2);
	dbMV=(int***)malloc(sizeof(int)*2);
	for(k=0; k<2; k++) 
	{
		tmp_fwMV[k]=(int**)malloc(sizeof(int)*72); 
		tmp_bwMV[k]=(int**)malloc(sizeof(int)*72); 
		dfMV[k]=(int**)malloc(sizeof(int)*72); 
		dbMV[k]=(int**)malloc(sizeof(int)*72); 
	}
	for(k=0; k<2; k++)
		for(i=0; i<72; i++) 
		{
			tmp_fwMV[k][i]=(int*)malloc(sizeof(int)*92);  //[2][72][92];
			tmp_bwMV[k][i]=(int*)malloc(sizeof(int)*92);	//[2][72][92];
			dfMV[k][i]=(int*)malloc(sizeof(int)*92);			//[2][72][92];
			dbMV[k][i]=(int*)malloc(sizeof(int)*92);			//[2][72][92];
		}

	// initialize
	for(k=0; k<2; k++)
		for(i=0; i<72; i++)
			for(j=0; j<92; j++) 
			{
				tmp_fwMV[k][i][j]=0;
				tmp_bwMV[k][i][j]=0;
				dfMV[k][i][j]=0;
				dbMV[k][i][j]=0;
			}		

	// create fw_refFrArr[72][88], bw_refFrArr[72][88] 
	fw_refFrArr=(int**)malloc(sizeof(int)*72);
	bw_refFrArr=(int**)malloc(sizeof(int)*72);
	for(k=0; k<72; k++) 
	{
		fw_refFrArr[k]=(int*)malloc(sizeof(int)*88); 
		bw_refFrArr[k]=(int*)malloc(sizeof(int)*88); 
	}

	for(i=0; i<72; i++)
		for(j=0; j<88; j++) 
		{
			fw_refFrArr[i][j]=bw_refFrArr[i][j]=-1;
		}
}

void ReadImage(int frame_no, struct img_par *img)
{
	int frame_size, i, j, uv, status;
	
	frame_size = img->height*img->width*3/2;

	rewind (p_in);
	/* Find the correct image */
	status = fseek (p_in, frame_no * frame_size, 0);
	if (status != 0) {
		printf(" Error in seeking frame no: %d\n", frame_no);
		exit (-1);
	}

	/* Read image */
	for (j=0; j < img->height; j++) 
		for (i=0; i < img->width; i++) 
			imgY_org[j][i]=fgetc(p_in);         
	for (uv=0; uv < 2; uv++) 
		for (j=0; j < img->height_cr ; j++) 
			for (i=0; i < img->width_cr; i++) 
				imgUV_org[uv][j][i]=fgetc(p_in);  
}

void code_Bframe(struct img_par *img, struct inp_par *inp, struct stat_par *stat)
{
	int i,j,k, 
        i3,j3;

	int cbp, cr_cbp;         // codec block pattern variables 
	int tot_intra_sad;
	int intra_pred_modes[8]; // store intra prediction modes, two and two 
	int intra_pred_mode_2;   // best 16x16 intra mode 
	int fw_predframe_no;

	int uv,step_h,step_v;
	int len,info;
	int kk,kbeg,kend, mb_y, mb_x, ii, jj;
	int level,run;
	int i1,j1,m2,jg2;
	int mb_nr;

    int mvDiffX,mvDiffY;

	int x, y, recode_macroblock;
	int loopb_store[6][6];
	int loopc_store[4][4];
	int use_bitstream_backing;
	void (*put)(char *tracestring, int len, int info, int sliceno, int type); 
		
	/* Loops for coding of all macro blocks in a frame */
	for (img->mb_y=0; img->mb_y < img->height/MB_BLOCK_SIZE; ++img->mb_y) {
		img->block_y = img->mb_y * BLOCK_SIZE;  // vertical luma block posision 
		img->pix_y   = img->mb_y * MB_BLOCK_SIZE; // vertical luma macroblock posision 
		img->pix_c_y = img->mb_y * MB_BLOCK_SIZE/2;  // vertical chroma macroblock posision 
    
		if (inp->intra_upd > 0 && img->mb_y <= img->mb_y_intra) 
		  img->height_err=(img->mb_y_intra*16)+15;     // for extra intra MB 
		else
		  img->height_err=img->height-1;
    
		for (img->mb_x=0; img->mb_x < img->width/MB_BLOCK_SIZE; ++img->mb_x) {
		  img->block_x = img->mb_x * BLOCK_SIZE;        // luma block           
		  img->pix_x   = img->mb_x * MB_BLOCK_SIZE;     // luma pixel           
		  img->block_c_x = img->mb_x * BLOCK_SIZE/2;    // chroma block         
		  img->pix_c_x   = img->mb_x * MB_BLOCK_SIZE/2; // chroma pixel         

		  /* Use function pointer so that the bits are stored instead of put out */
		  /* when backing is in use */
		  use_bitstream_backing = (inp->slice_mode == 2 || inp->slice_mode == 3); 						
		  if(use_bitstream_backing)
			put = store_symbol;
		  else		
			put = put_symbol;
		
		  /* Save loopb and loopc in the case of recoding this macroblock */			
		  if(use_bitstream_backing)
		  {
			for(x = 0 ; x<6 ; x++) 
				for(y = 0 ; y<6 ; y++)
					loopb_store[x][y] = loopb[img->block_x+x][img->block_y+y];
			for(x=0 ; x<4 ; x++) 
				for(y=0 ; y<4 ; y++)
					loopc_store[x][y] = loopc[img->block_x/2+x][img->block_y/2+y];
		  }

		  /* recode_macroblock is used in slice mode two and three where */
		  /* backing of one macroblock in the bitstream is possible      */
		  recode_macroblock = 1;
		  while(recode_macroblock)	
		  {
			recode_macroblock=0;

			mb_nr=img->current_mb_nr;
			img->slice_numbers[mb_nr]=img->current_slice_nr;

			/* If MB is next to a slice boundary, mark neighbor blocks unavailable for prediction */
			ResetSlicePredictions(img);

#if TRACE
			fprintf(p_trace, "\n*********** Pic: %i (B) MB: %i Slice: %i **********\n\n", frame_no, img->current_mb_nr, img->current_slice_nr);
#endif

			// 1. 4x4 intra prediction coding
			intra_4x4(&cbp, &tot_intra_sad, intra_pred_modes, img, inp, stat);

			// 2. 16x16 intra prediction coding : till now, we get img->imod, imgY, cbpY from two intra
			intra_16x16(&tot_intra_sad, &intra_pred_mode_2, img);

			// 3. motion search 
			//			get_fwMV() : best img->fw_mb_mode, img->fw_multframe_no, fw_sad, tmp_fwMV
			//		    get_bwMV() : best img->bw_mb_mode, bw_sad, tmp_bwMV
			//			get_bid()  : img->fw_pred, img->bw_pred, img->bid_pred, bid_sad
			//			get_dir()  : dfMV, dbMV, img->dir_pred, dir_sad
			//			compare_sad(): determine img->imod(prediction type)
			fw_predframe_no=motion_search_Bframe(tot_intra_sad, img, inp);
	
			// 4. create imgY by dct_luma() according to img->imod
			create_imgY_cbp(&cbp, img);			

			// 5. 6. 4x4 chroma blocks and calculate cpb 
			chroma_block(&cbp, &cr_cbp, img);
			
			// 7. if current MB uses 16x16 intra prediction
			if (img->imod==INTRA_MB_NEW)
				img->mb_mode += intra_pred_mode_2 + 4*cr_cbp + 12*img->kac;

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

          
			/* Notify the NAL layer if a new slice is starting here*/
			if(mb_nr > 0)
			{
				if(img->slice_numbers[mb_nr] != img->slice_numbers[mb_nr-1])
				{
					len = nal_put_startcode(img->number*(inp->jumpd+1) % 256, mb_nr, img->qp, inp->img_format,img->current_slice_nr, SEB);
					stat->bit_ctr += len;
					stat->bit_slice += len;
					stat->bit_use_head_mode[img->type] += len;
				}
			}

			// 8. img->mb_mode written
			n_linfo(img->mb_mode,&len,&info); 
			sprintf(tracestring, "B_MB mode(%2d,%2d) = %3d",img->mb_x, img->mb_y, img->mb_mode);    
			put(tracestring,len,info, img->current_slice_nr, SEB);   
			stat->bit_ctr += len;
			stat->bit_slice += len;
			stat->bit_use_head_mode[img->type] += len;

			// when direct mode is used, then return
			if(img->imod==B_Direct)
			{
				for (j = 0; j < 4;j++)	{
					for (i = 0; i < 4;i++)
					{
						fw_refFrArr[img->block_y+j][img->block_x+i] = 
						bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
					}
				}
			}

			else if (img->imod == B_Forward) 
			{
				for (j = 0;j < 4;j++)	{
					for (i = 0;i < 4;i++)
					{
						fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no;
						bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
					}
				}
			}
			else if(img->imod == B_Backward) 
			{
				for (j = 0;j < 4;j++)	{
					for (i = 0;i < 4;i++)
					{
						fw_refFrArr[img->block_y+j][img->block_x+i] = -1;
						bw_refFrArr[img->block_y+j][img->block_x+i] = 0;
					}
				}
			}
			else if(img->imod == B_Bidirect) 
			{
				for (j = 0;j < 4;j++)	{
					for (i = 0;i < 4;i++)
					{
						fw_refFrArr[img->block_y+j][img->block_x+i] = fw_predframe_no;
						bw_refFrArr[img->block_y+j][img->block_x+i] = 0;
					}
				}
			}
			else { // 4x4, 16x16 intra
				for (j = 0;j < 4;j++)	{
					for (i = 0;i < 4;i++)
					{
						fw_refFrArr[img->block_y+j][img->block_x+i] = 
						bw_refFrArr[img->block_y+j][img->block_x+i] = -1;
					}
				}
			}

			// 9. intra_pred_mode(4x4 intra) written
			if (img->imod == INTRA_MB_OLD) 
			{
				for (i=0; i < MB_BLOCK_SIZE/2; i++) 
				{
					n_linfo(intra_pred_modes[i],&len,&info);
					sprintf(tracestring, "B_Intra mode = %3d\t",intra_pred_modes[i]);
					put(tracestring,len,info, img->current_slice_nr, SEB);
					stat->bit_ctr += len;
					stat->bit_slice += len;
					stat->bit_use_coeffY[img->type] += len;
				}
			}

			// 10.1. fw_predframe_no(Forward, Bidirect) written 
			if(img->imod==B_Forward || img->imod==B_Bidirect) {
				if (inp->no_multpred > 1) 
				{
					n_linfo(fw_predframe_no, &len, &info);
					sprintf(tracestring, "B_fw_Reference frame no %3d ", fw_predframe_no);
					put(tracestring,len,info, img->current_slice_nr, SEB);
					stat->bit_ctr += len;
					stat->bit_slice += len;
					stat->bit_use_mode_Bframe[img->mb_mode] += len;
				}
			}
			
			// 10.2. Blk_size(Bidirect) written					  
			if(img->imod==B_Bidirect) {
				if(img->fw_blc_size_h==16 && img->fw_blc_size_v==16) { // 16x16 : code_number 0
					len=1; info=0;
				}
				else if(img->fw_blc_size_h==16 && img->fw_blc_size_v==8) { // 16x8 : code_number 1
					len=3; info=0;
				}
				else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==16) { // 8x16 : code_number 2
					len=3; info=1;
				}				
				else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==8) { // 8x8 : code_number 3
					len=5; info=0;
				}
				else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==4) { // 8x4 : code_number 4
					len=5; info=1;
				}
				else if(img->fw_blc_size_h==4 && img->fw_blc_size_v==8) { // 4x8 : code_number 5
					len=5; info=2;
				}
				else { // 4x4 : code_number 6
					len=5; info=3;
				}
				sprintf(tracestring, "B_bidiret_fw_blk %3d x %3d ", img->fw_blc_size_h, img->fw_blc_size_v);
				put(tracestring,len,info, img->current_slice_nr, SEB);
				stat->bit_ctr += len;
				stat->bit_slice += len;
				stat->bit_use_mode_Bframe[img->mb_mode] += len;

				if(img->bw_blc_size_h==16 && img->bw_blc_size_v==16) { // 16x16 : code_number 0
					len=1; info=0;
				}
				else if(img->bw_blc_size_h==16 && img->bw_blc_size_v==8) { // 16x8 : 1
					len=3; info=0;
				}
				else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==16) { // 8x16 : 2
					len=3; info=1;
				}
				else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==8) { // 8x8 : 3
					len=5; info=0;
				}
				else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==4) { // 8x4 : 4
					len=5; info=1;
				}
				else if(img->bw_blc_size_h==4 && img->bw_blc_size_v==8) { // 4x8 : 5
					len=5; info=2;
				}
				else { // 4x4 : 6
					len=5; info=3;
				}
				sprintf(tracestring, "B_bidiret_bw_blk %3d x %3d ", img->bw_blc_size_h, img->bw_blc_size_v);
				put(tracestring,len,info, img->current_slice_nr, SEB);
				stat->bit_ctr += len;
				stat->bit_slice += len;
				stat->bit_use_mode_Bframe[img->mb_mode] += len;
			}

			// 10.3. MVDFW(Forward, Bidirect) written
			if(img->imod==B_Forward || img->imod==B_Bidirect) {
				step_h=img->fw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize 
				step_v=img->fw_blc_size_v/BLOCK_SIZE;  // vertical stepsize 

				for (j=0; j < BLOCK_SIZE; j += step_v) 
				{            
					for (i=0;i < BLOCK_SIZE; i += step_h) 
					{
						for (k=0; k < 2; k++) 
						{
							if(img->mb_mode==1) // fw 16x16
								mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][k],&len,&info);
							else if(img->mb_mode==3) { // bidirectinal 
								if(img->fw_blc_size_h==16 && img->fw_blc_size_v==16)			// 16x16 : blocktype 1
									mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][1][k],&len,&info);
								else if(img->fw_blc_size_h==16 && img->fw_blc_size_v==8)   // 16x8 : blocktype 2
									mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][2][k],&len,&info);
								else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==16)  // 8x16 : blocktype 3
									mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][3][k],&len,&info);
								else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==8)  // 8x8 : blocktype 4
									mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][4][k],&len,&info);
								else if(img->fw_blc_size_h==8 && img->fw_blc_size_v==4)   // 8x4 : blocktype 5
									mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][5][k],&len,&info);
								else if(img->fw_blc_size_h==4 && img->fw_blc_size_v==8)   // 4x8 : blocktype 6
									mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][6][k],&len,&info);
								else																											// 4x4 : blocktype 7
									mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][7][k],&len,&info);								
							}
							else
								mvd_linfo(tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][(img->mb_mode)/2][k],&len,&info);
		          
							sprintf(tracestring, " MVD(%d) = %3d",
								k, tmp_fwMV[k][img->block_y+j][img->block_x+i+4]-img->p_fwMV[i][j][fw_predframe_no][img->mb_mode][k]);
							put(tracestring,len,info, img->current_slice_nr, SEB);
							stat->bit_ctr += len;
							stat->bit_slice += len;
							stat->bit_use_mode_Bframe[img->mb_mode] += len;
						}
					}
				}
			}
			
			// 10.4. MVDBW(Backward, Bidirect) written
			if(img->imod==B_Backward || img->imod==B_Bidirect) {
				step_h=img->bw_blc_size_h/BLOCK_SIZE;  // horizontal stepsize 
				step_v=img->bw_blc_size_v/BLOCK_SIZE;  // vertical stepsize 

				for (j=0; j < BLOCK_SIZE; j += step_v) 
				{
					for (i=0;i < BLOCK_SIZE; i += step_h) 
					{
						for (k=0; k < 2; k++) 
						{
							if(img->mb_mode==2) // bw 16x16
								mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][k],&len,&info);
							else if(img->mb_mode==3) { // bidirectional
								if(img->bw_blc_size_h==16 && img->bw_blc_size_v==16)		 // 16x16 : blocktype 1
									mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][1][k],&len,&info);
								else if(img->bw_blc_size_h==16 && img->bw_blc_size_v==8)   // 16x8 : blocktype 2
									mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][2][k],&len,&info);
								else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==16)  // 8x16 : blocktype 3
									mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][3][k],&len,&info);
								else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==8)  // 8x8 : blocktype 4
									mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][4][k],&len,&info);
								else if(img->bw_blc_size_h==8 && img->bw_blc_size_v==4)   // 8x4 : blocktype 5
									mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][5][k],&len,&info);
								else if(img->bw_blc_size_h==4 && img->bw_blc_size_v==8)   // 4x8 : blocktype 6
									mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][6][k],&len,&info);
								else 																										 // 4x4 : blocktype 7
									mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][7][k],&len,&info);								
							}
							else // other bw
							 	mvd_linfo(tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][(img->mb_mode-1)/2][k],&len,&info);						  				
														
							sprintf(tracestring, " MVD(%d) = %3d",
								k, tmp_bwMV[k][img->block_y+j][img->block_x+i+4]-img->p_bwMV[i][j][0][img->mb_mode][k]);
							put(tracestring,len,info, img->current_slice_nr, SEB);
							stat->bit_ctr += len;
							stat->bit_slice += len;
							stat->bit_use_mode_Bframe[img->mb_mode] += len;
						}
					}
				}
			}

			// 11. cbp written
			if (img->imod!=INTRA_MB_NEW)
			{
				sprintf(tracestring, " CBP (%2d,%2d) = %3d\t",img->mb_x, img->mb_y, cbp);
				if (img->imod == INTRA_MB_OLD) n_linfo(NCBP[cbp][0],&len,&info);
				else n_linfo(NCBP[cbp][1],&len,&info);
				put(tracestring,len,info, img->current_slice_nr, SEB);	
				stat->bit_ctr += len;
				stat->bit_slice += len;
				stat->tmp_bit_use_cbp[img->type] += len;

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
												sprintf(tracestring, "Luma coeff dbl(%2d,%2d)  level=%3d Run=%2d",kk,k,level,run);
												put(tracestring,len,info, img->current_slice_nr, SEB);
												stat->bit_ctr += len;
												stat->bit_slice += len;
												stat->bit_use_coeffY[img->type] += len;
                                                //g.b. post filter like loop filter
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
									else      /* single scan */
									{
										level=1; /* get inside loop */
										for(k=0;k<=16 && level !=0; k++)
										{
											level=img->cof[i][j][k][0][SINGLE_SCAN];
											run=  img->cof[i][j][k][1][SINGLE_SCAN];
											levrun_linfo_inter(level,run,&len,&info);
											sprintf(tracestring, "Luma coeff sng(%2d) level =%3d run =%2d", k, level,run);
											put(tracestring,len,info, img->current_slice_nr, SEB);
											stat->bit_ctr += len;
											stat->bit_slice += len;
											stat->bit_use_coeffY[img->type] += len;
                                            //g.b. post filter like loop filter
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
					sprintf(tracestring, "DC luma coeff new intra sng(%2d) level =%3d run =%2d", k, level, run);
					put(tracestring,len,info, img->current_slice_nr, SEB);
					stat->bit_ctr += len;
					stat->bit_slice += len;
					stat->bit_use_coeffY[img->type] += len;
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
										sprintf(tracestring, "AC luma coeff new intra sng(%2d) level =%3d run =%2d", k, level, run);
										put(tracestring,len,info, img->current_slice_nr, SEB);
										stat->bit_ctr += len;
										stat->bit_slice += len;
										stat->bit_use_coeffY[img->type] += len;
									}
								}
							}
						}
					}
				}
			}

			// 12. bits for chroma
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
						sprintf(tracestring, "DC Chroma coeff %2d: level =%3d run =%2d",k, level, img->cofu[k][1][uv]);
						put(tracestring,len,info, img->current_slice_nr, SEB);
						stat->bit_ctr += len;
						stat->bit_slice += len;
						stat->bit_use_coeffC[img->type] += len;	
                        //g.b. post filter like loop filter
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
									sprintf(tracestring, "AC Chroma coeff %2d: level =%3d run =%2d",k, level, img->cof[i][j][k][1][0]);
									put(tracestring,len,info, img->current_slice_nr, SEB);
									stat->bit_ctr += len;
									stat->bit_slice += len;
									stat->bit_use_coeffC[img->type] += len;	
                                    //g.b. post filter like loop filter
                                    if (level != 0)
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
				for(x = 0 ; x<6 ; x++) 
					for(y = 0 ; y<6 ; y++)
						loopb[img->block_x+x][img->block_y+y] = loopb_store[x][y];
				for(x=0 ; x<4 ; x++) 
					for(y=0 ; y<4 ; y++)
						loopc[img->block_x/2+x][img->block_y/2+y] = loopc_store[x][y];
			}
		  } // while(recode_macroblock)

		  /* The final coding of the macroblock has been done */

		  /* Send the stored symbols generated for this macroblock to the NAL layer */
		  if(use_bitstream_backing)
			put_stored_symbols();

		  // 13. 
		  ++stat->mode_use_Bframe[img->mb_mode];
		  img->current_mb_nr++;		
		} // img->mb_x
	} // img->mb_y

	free(tmp_fwMV);
	free(tmp_bwMV);
	free(dfMV);
	free(dbMV);
	free(fw_refFrArr);
	free(bw_refFrArr);
}

void intra_4x4(int *cbp, int *tot_intra_sad, int *intra_pred_modes, 
							 struct img_par *img, struct inp_par *inp, struct stat_par *stat)
{
	int mb_y, mb_x, block_y, block_x;
	int coeff_cost, cbp_mask, pic_pix_y, pic_pix_x, pic_block_y, pic_block_x;
	int best_intra_sad, current_intra_sad;
	int ipmode, best_ipmode;
	int last_ipred, nonzero;
	int i, j;

	*cbp=0;
	*tot_intra_sad=QP2QUANT[img->qp]*24;/* sum of intra sad values, start with a 'handicap'*/
  
	for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += BLOCK_SIZE*2) {
		for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += BLOCK_SIZE*2) { // 16x16 -> four 8x8
			cbp_mask=(int)pow(2,(mb_x/8+mb_y/4));

			for (block_y=mb_y; block_y < mb_y+BLOCK_SIZE*2; block_y += BLOCK_SIZE) {
				pic_pix_y=img->pix_y+block_y;
				pic_block_y=pic_pix_y/BLOCK_SIZE;
				for (block_x=mb_x; block_x < mb_x+BLOCK_SIZE*2; block_x += BLOCK_SIZE) { // 8x8 -> four 4x4
					pic_pix_x=img->pix_x+block_x;
					pic_block_x=pic_pix_x/BLOCK_SIZE;
 
					// intrapred_luma() makes and returns 4x4 blocks with all 5 intra prediction modes. 
					//  Notice that some modes are not possible at frame edges.          
					intrapred_luma(img,pic_pix_x,pic_pix_y);

					best_intra_sad=MAX_VALUE; // initial value, will be modified below                 
					img->imod = INTRA_MB_OLD; // for now mode set to intra, may be changed in motion_search() 
          
					// find best intra mode from 6 intra prediction modes by comparing sad
					for (ipmode=0; ipmode < NO_INTRA_PMODE; ipmode++)  {
						// Horizontal pred from Y neighbour pix , vertical use X pix, diagonal needs both 
						// DC or vert pred or hor edge
						if (ipmode==DC_PRED||ipmode==HOR_PRED||img->ipredmode[pic_pix_x/BLOCK_SIZE+1][pic_pix_y/BLOCK_SIZE] >= 0)
						{
							// DC or hor pred or vert edge
							if (ipmode==DC_PRED||ipmode==VERT_PRED||img->ipredmode[pic_pix_x/BLOCK_SIZE][pic_pix_y/BLOCK_SIZE+1] >= 0)
							{
								// dummy residue(diff) to get sad
								for (j=0; j < BLOCK_SIZE; j++) 
									for (i=0; i < BLOCK_SIZE; i++) 
										img->m7[i][j]=imgY_org[pic_pix_y+j][pic_pix_x+i]-img->mprr[ipmode][j][i]; 
                  
								current_intra_sad=QP2QUANT[img->qp]*PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][ipmode]*2;
								current_intra_sad += find_sad(img,inp->hadamard); 

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
          
					*tot_intra_sad += best_intra_sad;
					img->ipredmode[pic_block_x+1][pic_block_y+1]=best_ipmode;

					if (pic_block_x & 1 == 1) /* just even blocks, two and two predmodes are sent together */
					{
						intra_pred_modes[block_x/8+block_y/2]=IPRED_ORDER[last_ipred][PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][best_ipmode]];
					}
					last_ipred=PRED_IPRED[img->ipredmode[pic_block_x+1][pic_block_y]+1][img->ipredmode[pic_block_x][pic_block_y+1]+1][best_ipmode];

					// residue = original - predicted value
					for (j=0; j < BLOCK_SIZE; j++) 
						for (i=0; i < BLOCK_SIZE; i++)
						{
							img->m7[i][j] =imgY_org[img->pix_y+block_y+j][img->pix_x+block_x+i] - img->mpr[i+block_x][j+block_y];
						}

					nonzero = dct_luma(block_x,block_y, &coeff_cost,img);

					// set coded block pattern if there are nonzero coeffs 
					if (nonzero) {
						*cbp=*cbp | cbp_mask;
					}
				}
			}
		}
	}
}

void intra_16x16(int *tot_intra_sad, int *intra_pred_mode_2, struct img_par *img)
{
	int tot_intra_sad2;
	int i,j;

	/* new 16x16 based routine */
	intrapred_luma_2(img);                                /* make intra pred for the new 4 modes */
	tot_intra_sad2 = find_sad2(img, intra_pred_mode_2);   /* find best SAD for new modes */

	if (tot_intra_sad2 < *tot_intra_sad)
	{ 
		*tot_intra_sad = tot_intra_sad2; /* update best intra sad if necessary */
		img->imod = INTRA_MB_NEW;       /* one of the new modes is used */
		dct_luma2(img, *intra_pred_mode_2);
		for (i=0;i<4;i++)
			for (j=0;j<4;j++)
				img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;    
	}
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
	int all_mv[4][4][5][9][2]; 
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
	for (ref_frame=0; ref_frame < min(img->number,inp_par->no_multpred); ref_frame++) 
	{
		for (j = 0;j < 4;j++)
		{
			for (i = 0;i < 4;i++)
			{
				fw_refFrArr[img->block_y+j][img->block_x+i] = ref_frame;
			}
		}

		/* find  reference image */
		ref_inx=(img->number-ref_frame-1)%MAX_MULT_PRED; 

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
	int all_mv[4][4][5][17][2]; 
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

	ref_inx=(img->number-ref_frame)%MAX_MULT_PRED;  
  
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
						refP_tr=nextP_tr-((refFrArr[pic_block_y][pic_block_x]+1)*P_interval);
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
					if((jj4>=img->height*4-12)||(jj4<0)||(ii4>=img->width*4-12)||(ii4<0)
						||(jjj4>=img->height*4-12)||(jjj4<0)||(iii4>=img->width*4-12)||(iii4<0))
					{
						*dir_sad=MAX_DIR_SAD;
						return;
					}

					else 
					{
						// next P is intra mode
						if(refFrArr[pic_block_y][pic_block_x]==-1)
							ref_inx=(img->number-1)%MAX_MULT_PRED;
						// next P is skip or inter mode
						else
							ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%MAX_MULT_PRED;

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


void create_imgY_cbp(int *cbp, struct img_par *img)
{
	int cbp_mask, sum_cnt_nonz, coeff_cost, nonzero;
	int mb_y, mb_x, block_y, block_x, i, j, pic_pix_y, pic_pix_x, pic_block_x, pic_block_y;
	int ii4, jj4, iii4, jjj4, i2, j2, fw_pred, bw_pred, ref_inx, df_pred, db_pred; 

	switch(img->imod) {
	  case B_Forward :
		*cbp=0;
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
							*cbp= *cbp | cbp_mask;
						}
					} // block_x
				} // block_y

				if (coeff_cost > 3) {
					sum_cnt_nonz += coeff_cost;
				}
				else /*discard */
				{
					*cbp = *cbp & (63-cbp_mask);
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
			*cbp = *cbp & 48; /* mask bit 4 and 5 */ 
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
		*cbp=0;
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
							*cbp = *cbp | cbp_mask;
						}
					} // block_x
				} // block_y

				if (coeff_cost > 3) {
					sum_cnt_nonz += coeff_cost;
				}
				else /*discard */
				{
					*cbp = *cbp & (63-cbp_mask);
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
			*cbp = *cbp & 48; /* mask bit 4 and 5 */ 
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
		*cbp=0;
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
							*cbp = *cbp | cbp_mask;
						}
					} // block_x
				} // block_y

				if (coeff_cost > 3) {
					sum_cnt_nonz += coeff_cost;
				}
				else /*discard */
				{
					*cbp = *cbp & (63-cbp_mask);
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
			*cbp = *cbp & 48; /* mask bit 4 and 5 */ 
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
		*cbp=0;
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
							ref_inx=(img->number-1)%MAX_MULT_PRED;
						// next P is skip or inter mode
						else
							ref_inx=(img->number-refFrArr[pic_block_y][pic_block_x]-1)%MAX_MULT_PRED;

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
							*cbp = *cbp | cbp_mask;
						}
					} // block_x
				} // block_y

				// LG : direct residual coding
				if (coeff_cost > 3) {
					sum_cnt_nonz += coeff_cost;
				}
				else /*discard */
				{
					*cbp = *cbp & (63-cbp_mask);

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
			*cbp = *cbp & 48; /* mask bit 4 and 5 */ 
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

void chroma_block(int *cbp, int *cr_cbp, struct img_par *img)
{
	int i, j;
	int uv, ii,jj,ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;	
	int pic_block_y, pic_block_x, ref_inx, fw_pred, bw_pred;

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
	*cbp += *cr_cbp*16;
}

void writeimage(int Bframe_to_code, int no_Bframe, struct img_par *img, struct inp_par *inp) 
{
	int i, j, k;

	if (Bframe_to_code != no_Bframe)           
	{
		// save B picture
		for (i=0; i < img->height; i++) 
			for (j=0; j < img->width; j++) 
				fputc(min(imgY[i][j],255),p_dec);      
		for (k=0; k < 2; ++k) 
			for (i=0; i < img->height/2; i++) 
				for (j=0; j < img->width/2; j++) 
					fputc(min(imgUV[k][i][j],255),p_dec);
	}
	else 
	{
		// save B picture
		for (i=0; i < img->height; i++) 
			for (j=0; j < img->width; j++) 
				fputc(min(imgY[i][j],255),p_dec);      
		for (k=0; k < 2; ++k) 
			for (i=0; i < img->height/2; i++) 
				for (j=0; j < img->width/2; j++) 
					fputc(min(imgUV[k][i][j],255),p_dec);

		// save P picture
		for (i=0; i < img->height; i++) 
			for (j=0; j < img->width; j++) 
				fputc(min(nextP_imgY[i][j],255),p_dec);      
		for (k=0; k < 2; ++k) 
			for (i=0; i < img->height/2; i++) 
				for (j=0; j < img->width/2; j++) 
					fputc(min(nextP_imgUV[k][i][j],255),p_dec);	
	}
}


