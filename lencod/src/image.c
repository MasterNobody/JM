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
// Yoon-Seong Soh                  <yunsung@lge.com>
// Thomas Stockhammer              <stockhammer@ei.tum.de>
// Detlev Marpe                    <marpe@hhi.de>
// Guido Heising                   <heising@hhi.de> 
// *************************************************************************************
// *************************************************************************************
#include "contributors.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <memory.h>

#include "global.h"
#include "elements.h"
#include "image.h"


/************************************************************************
*
*  Name :       encode_one_frame()
*
*  Description: Encodes one frame
*
************************************************************************/
int encode_one_frame(struct img_par *img,struct inp_par *inp,struct stat_par *stat, struct snr_par *snr)
{
	Boolean end_of_frame = FALSE;
	const int total_number_mb = (img->width * img->height)/(MB_BLOCK_SIZE*MB_BLOCK_SIZE);
	SyntaxElement imgTypeSymbol;

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
  
	
	/* Initialize frame with all stat and img variables */
	init_frame(img, inp, stat);

	/* Initialize loopfilter */
	init_loop_filter(img);

	/* Select picture type: write coding to symbol */
	select_picture_type(img, inp, &imgTypeSymbol);
	
	/* Read one new frame */
	read_one_new_frame(img, inp);

	if (img->type == B_IMG)		
		Bframe_ctr++;

	while (end_of_frame == FALSE)	/* loop over slices */
	{
		/* Encode the current slice */
		encode_one_slice(img,inp,stat,&imgTypeSymbol);

		/* Proceed to next slice */
		img->current_slice_nr++;
		stat->bit_slice = 0;
		if (img->current_mb_nr == total_number_mb) /* end of frame reached? */
			end_of_frame = TRUE;
	}

 	loopfilter(img); // reconstructed imgY, imgUV -> loop-filtered imgY, imgUV

	// in future only one call of oneforthpix() for all frame tyoes will be necessary, because
	// mref buffer will be increased by one frame to store also the next P-frame. Then mref_P
	// will not be used any more
	if (img->type != B_IMG) /*all I- and P-frames*/
	{
		if (inp->successive_Bframe == 0 || img->number == 0)
			oneforthpix(img,inp);  // I- and P-frames:loop-filtered imgY, imgUV -> mref[][][], mcef[][][][]
		else
			oneforthpix_2(img);    // I- and P-frames prior a B-frame:loop-filtered imgY, imgUV -> mref_P[][][], mcef_P[][][][]
    	                           // I- and P-frames prior a B-frame:loop-filtered imgY, imgUV -> mref_P[][][], mcef_P[][][][]
	}
	else 
		if (img->b_frame_to_code == inp->successive_Bframe) 
			copy2mref(img);		       // last successive B-frame: mref_P[][][], mcef_P[][][][] (loop-filtered imgY, imgUV)-> mref[][][], mcef[][][][]	

	find_snr(snr,img);     
    
	time(&ltime2);       // end time sec 
#ifdef WIN32
	_ftime(&tstruct2);   // end time ms  
#else 
	ftime(&tstruct2);    // end time ms  
#endif
	tmp_time=(ltime2*1000+tstruct2.millitm) - (ltime1*1000+tstruct1.millitm);
	tot_time=tot_time + tmp_time;

	/* Write reconstructed images */
	write_reconstructed_image(img, inp);
	  		
	if(img->number == 0)
	{
		printf("%3d(I) %8d %4d %7.4f %7.4f %7.4f  %5d \n",
				frame_no, stat->bit_ctr-stat->bit_ctr_n, 
				img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time); 
		stat->bitr0=stat->bitr;           
		stat->bit_ctr_0=stat->bit_ctr;   
		stat->bit_ctr=0;                  
	}
	else
	{
		if (img->type != B_IMG)
		{
			stat->bit_ctr_P += stat->bit_ctr-stat->bit_ctr_n;  
			printf("%3d(P) %8d %4d %7.4f %7.4f %7.4f  %5d \n",
				frame_no, stat->bit_ctr-stat->bit_ctr_n, 
				img->qp, snr->snr_y, snr->snr_u, snr->snr_v, tmp_time); 
		}
		else
		{
			stat->bit_ctr_B += stat->bit_ctr-stat->bit_ctr_n;  		
			printf("%3d(B) %8d %4d %7.4f %7.4f %7.4f  %5d \n",
				frame_no, stat->bit_ctr-stat->bit_ctr_n, img->qp, 
				snr->snr_y, snr->snr_u, snr->snr_v, tmp_time); 	
		}
	}
	stat->bit_ctr_n=stat->bit_ctr; 	
	

	if(img->number == 0) 
		return 0; 	
	else
		return 1;
}


/************************************************************************
*
*  Name :       encode_one_slice()
*
*  Description: Encodes one slice
*
************************************************************************/
void encode_one_slice(struct img_par *img,struct inp_par *inp,struct stat_par *stat, SyntaxElement *imgTypeSym)
{
	Boolean end_of_slice = FALSE;
	Boolean recode_macroblock;
	int len;

	/* Initializes the parameters of the current slice */
	init_slice(img,inp);

	/* Write slice or picture header */
	len = start_slice(img, inp, imgTypeSym);

	/* Update statistics */
	if (img->current_mb_nr > 0)	
	{
 		img->mb_data[img->current_mb_nr].bitcounter[BITS_HEADER_MB] = len;
	}
	else															/* first slice includes picture header */
	{
		if (inp->symbol_mode == UVLC)
		{
			stat->bit_ctr += len;
			stat->bit_slice += len;
		}
		stat->bit_use_head_mode[img->type] += len;
	}

	while (end_of_slice == FALSE)	/* loop over macroblocks */
	{

		/* recode_macroblock is used in slice mode two and three where */
		/* backing of one macroblock in the bitstream is possible      */
		recode_macroblock = FALSE;

		/* Initializes the current macroblock */
		start_macroblock(img,inp,stat);

		/* Encode one macroblock */
		encode_one_macroblock(img,inp,stat);

		/* Pass the generated syntax elements to the NAL */
		write_one_macroblock(img,inp,stat);

		/* Terminate processing of the current macroblock */
		terminate_macroblock(img, inp, stat, &end_of_slice, &recode_macroblock);

		if (recode_macroblock == FALSE)					/* The final processing of the macroblock has been done */
			proceed2nextMacroblock(img,inp,stat); /* Go to next macroblock */
			
	}

	terminate_slice(img,inp,stat);
}


/************************************************************************
*
*  Name :       init_frame()
*
*  Description: Initializes the parameters for a new frame
*               
************************************************************************/
void init_frame(struct img_par *img, struct inp_par *inp, struct stat_par *stat)
{
	int i,j,k;
	int prevP_no, nextP_no;

	img->current_mb_nr=0;
	img->current_slice_nr=0;
	stat->bit_slice = 0;

	img->mb_y = img->mb_x = 0;
	img->block_y = img->pix_y = img->pix_c_y = 0; 	/* define vertical positions */
	img->block_x = img->pix_x = img->block_c_x = img->pix_c_x = 0; /* define horizontal positions */

	if (inp->intra_upd > 0 && img->mb_y <= img->mb_y_intra)
		img->height_err=(img->mb_y_intra*16)+15;     /* for extra intra MB */
	else
		img->height_err=img->height-1;

	if(img->type != B_IMG)
	{
		if (img->type == INTRA_IMG)            
			img->qp = inp->qp0;					/* set quant. parameter for I-frame */
		else															
			img->qp = inp->qpN;					/* set quant. parameter for P-frame */

		img->tr=img->number*(inp->jumpd+1);
	
		if(img->number!=0 && inp->successive_Bframe != 0) 	/* B pictures to encode */
			nextP_tr=img->tr;

		img->mb_y_intra=img->mb_y_upd;   /*  img->mb_y_intra indicates which GOB to intra code for this frame */

		if (inp->intra_upd > 0)          /* if error robustness, find next GOB to update */
		{
			img->mb_y_upd=(img->number/inp->intra_upd) % (img->width/MB_BLOCK_SIZE);
		}
	}
	else
	{
		img->p_interval = inp->jumpd+1;
		prevP_no = (img->number-1)*img->p_interval;
		nextP_no = img->number*img->p_interval;


		img->b_interval = (int)((float)(inp->jumpd+1)/(inp->successive_Bframe+1.0)+0.5);

		img->qp = inp->qpB;
     
		img->tr= prevP_no+img->b_interval*img->b_frame_to_code; // from prev_P
		if(img->tr >= nextP_no) 	
			img->tr=nextP_no-1;

		// initialize arrays
		for(k=0; k<2; k++)
			for(i=0; i<img->height/BLOCK_SIZE; i++)
				for(j=0; j<img->width/BLOCK_SIZE+4; j++) 
				{
					tmp_fwMV[k][i][j]=0;
					tmp_bwMV[k][i][j]=0;
					dfMV[k][i][j]=0;
					dbMV[k][i][j]=0;
				}		
		for(i=0; i<img->height/BLOCK_SIZE; i++)
			for(j=0; j<img->width/BLOCK_SIZE; j++) 
			{
				fw_refFrArr[i][j]=bw_refFrArr[i][j]=-1;
			}		
			
	}
}

/************************************************************************
*
*  Name :       init_slice()
*
*  Description: Initializes the parameters for a new slice
*               
************************************************************************/
void init_slice(struct img_par *img, struct inp_par *inp)
{

	int i;
	Slice *curr_slice = img->currentSlice;
	DataPartition *dataPart;
	Bitstream *currStream;

	curr_slice->picture_id = img->tr%256;
	curr_slice->slice_nr = img->current_slice_nr;
	curr_slice->qp = img->qp;
	curr_slice->format = inp->img_format;
	curr_slice->start_mb_nr = img->current_mb_nr;
	curr_slice->dp_mode = inp->partition_mode;
	curr_slice->slice_too_big = dummy_slice_too_big;

	for (i=0; i<curr_slice->max_part_nr; i++)
	{
		dataPart = &(curr_slice->partArr[i]);

		/* in priciple it is possible to assign to each partition */
		/* a different entropy coding method */ 
		if (inp->symbol_mode == UVLC)
			dataPart->writeSyntaxElement = writeSyntaxElement_UVLC; 
		else
			dataPart->writeSyntaxElement = writeSyntaxElement_CABAC;
		currStream = dataPart->bitstream;
		currStream->bits_to_go	= currStream->stored_bits_to_go;
		currStream->byte_pos		= currStream->stored_byte_pos;
		currStream->byte_buf		= currStream->stored_byte_buf;

	}
}
	
/************************************************************************
*
*  Name :       select_picture_type()
*
*  Description: Selects picture type and codes it to symbol
*               
************************************************************************/
void select_picture_type(struct img_par *img, struct inp_par *inp, SyntaxElement *symbol)
{
	int multpred;

	if (inp->no_multpred <= 1)
		multpred=FALSE;
	else
		multpred=TRUE;               /* multiple reference frames in motion search */

	if (img->type == INTRA_IMG)
	{
		symbol->len=3;
		symbol->inf=1;
		symbol->value1 = 2;
	}
	else if((img->type == INTER_IMG) && (multpred == FALSE)) /* inter single reference frame */
	{
		symbol->len=1;
		symbol->inf=0;
		symbol->value1 = 0;
	}
	else if((img->type == INTER_IMG) && (multpred == TRUE)) /* inter multiple reference frames */
	{
		symbol->len=3;
		symbol->inf=0;
		symbol->value1 = 1;
	}
	else if((img->type == B_IMG) && (multpred == FALSE))	
	{
		symbol->len=5; 
		symbol->inf=0;
		symbol->value1 = 3;
	}
	else if((img->type == B_IMG) && (multpred == TRUE))
	{
		symbol->len=5;
		symbol->inf=1;
		symbol->value1 = 4;
	}
	else
	{		
		error("Picture Type not supported!");
	}

#if TRACE
	sprintf(symbol->tracestring, "Image type = %3d ", img->type);
#endif
	symbol->type = SE_PTYPE;
}

/************************************************************************
*
*  Name :       init_loop_filter()
*
*  Description: initializes loop filter
*               
************************************************************************/
void init_loop_filter(struct img_par *img)
{
	int i, j;

	for (i=0;i<img->width/4+2;i++)
		for (j=0;j<img->height/4+2;j++)
			loopb[i+1][j+1]=0;

	for (i=0;i<img->width_cr/4+2;i++)
		for (j=0;j<img->height_cr/4+2;j++)
			loopc[i+1][j+1]=0;
}


/************************************************************************
*
*  Name :       read_one_new frame()
*
*  Description: Reads new frame from file and sets frame_no
*               
************************************************************************/
void read_one_new_frame(struct img_par *img, struct inp_par *inp)
{
	int i, j, uv;
	int status; //frame_no;
	int frame_size = img->height*img->width*3/2;

	if(img->type == B_IMG)
		frame_no = (img->number-1)*(inp->jumpd+1)+img->b_interval*img->b_frame_to_code;
	else
		frame_no = img->number*(inp->jumpd+1);

	rewind (p_in);
	status = fseek (p_in, frame_no * frame_size, 0);
	
	if (status != 0) 
	{
		sprintf(errortext, "Error in seeking frame no: %d\n", frame_no);
		error(errortext);
	}    

	for (j=0; j < img->height; j++) 
		for (i=0; i < img->width; i++) 
			imgY_org[j][i]=fgetc(p_in);         
	for (uv=0; uv < 2; uv++) 
		for (j=0; j < img->height_cr ; j++) 
			for (i=0; i < img->width_cr; i++) 
				imgUV_org[uv][j][i]=fgetc(p_in);  
}

/************************************************************************
*
*  Name :       write_reconstructed_image()
*
*  Description: Writes reconstructed image(s) to file
*               This can be done more elegant!
*               
************************************************************************/
void write_reconstructed_image(struct img_par *img, struct inp_par *inp)
{
	int i, j, k;

	if(img->type != B_IMG)
	{
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
	}
	else
	{
		/* save B frame */
		for (i=0; i < img->height; i++) 
			for (j=0; j < img->width; j++) 
				fputc(min(imgY[i][j],255),p_dec);      
		for (k=0; k < 2; ++k) 
			for (i=0; i < img->height/2; i++) 
				for (j=0; j < img->width/2; j++) 
					fputc(min(imgUV[k][i][j],255),p_dec);
		
		/* If this is last B frame also store P frame */
		if(img->b_frame_to_code == inp->successive_Bframe)           
		{
			/* save P picture */
			for (i=0; i < img->height; i++) 
				for (j=0; j < img->width; j++) 
					fputc(min(nextP_imgY[i][j],255),p_dec);      
			for (k=0; k < 2; ++k) 
				for (i=0; i < img->height/2; i++) 
					for (j=0; j < img->width/2; j++) 
						fputc(min(nextP_imgUV[k][i][j],255),p_dec);	
		}
	}

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
*
************************************************************************/
void oneforthpix(struct img_par *img, struct inp_par *inp)
{
  // int img4Y_tmp[576][704];
  int is;
  int i,j,i2,j2,j4;
  int ie2,je2;
  int uv;
  	
  img->frame_cycle=img->number % inp->no_multpred;  /*GH inp->no_multpred used insteadof MAX_MULT_PRED
		                                              frame buffer size = inp->no_multpred+1*/
	 
  for (j=0; j < img->height; j++)
	{
		for (i=0; i < img->width; i++)
		{
			i2=i*2;
			is=(ONE_FOURTH_TAP[0][0]*(imgY[j][i         ]+imgY[j][min(img->width-1,i+1)])+
				ONE_FOURTH_TAP[1][0]*(imgY[j][max(0,i-1)]+imgY[j][min(img->width-1,i+2)])+
				ONE_FOURTH_TAP[2][0]*(imgY[j][max(0,i-2)]+imgY[j][min(img->width-1,i+3)]));
			img4Y_tmp[2*j][i2  ]=imgY[j][i]*1024;    /* 1/1 pix pos */
			img4Y_tmp[2*j][i2+1]=is*32;              /* 1/2 pix pos */
		}
  }
  
  for (i=0; i < img->width*2; i++)
	{
		for (j=0; j < img->height; j++)
		{
			j2=j*2;
			j4=j*4;
			/* change for TML4, use 6 TAP vertical filter */
			is=(ONE_FOURTH_TAP[0][0]*(img4Y_tmp[j2         ][i]+img4Y_tmp[min(2*img->height-2,j2+2)][i])+
				ONE_FOURTH_TAP[1][0]*(img4Y_tmp[max(0,j2-2)][i]+img4Y_tmp[min(2*img->height-2,j2+4)][i])+
				ONE_FOURTH_TAP[2][0]*(img4Y_tmp[max(0,j2-4)][i]+img4Y_tmp[min(2*img->height-2,j2+6)][i]))/32;
			
			img4Y_tmp[j2+1][i]=is;                  /* 1/2 pix */
			mref[img->frame_cycle][j4  ][i*2]=max(0,min(255,(int)((img4Y_tmp[j2][i]+512)/1024)));     /* 1/2 pix */
			mref[img->frame_cycle][j4+2][i*2]=max(0,min(255,(int)((is+512)/1024))); /* 1/2 pix */
		}
	}
  
  /* 1/4 pix */
  /* luma */
  ie2=(img->width-1)*4;
  je2=(img->height-1)*4;
  
  for (j=0;j<je2+4;j+=2)
    for (i=0;i<ie2+3;i+=2)
      mref[img->frame_cycle][j][i+1]=max(0,min(255,(int)(img4Y_tmp[j/2][i/2]+img4Y_tmp[j/2][min(ie2/2+1,i/2+1)]+1024)/2048));
		
		for (i=0;i<ie2+4;i++)
		{
			for (j=0;j<je2+3;j+=2)
			{
				if( i%2 == 0 )
					mref[img->frame_cycle][j+1][i]=max(0,min(255,(int)(img4Y_tmp[j/2][i/2]+img4Y_tmp[min(je2/2+1,j/2+1)][i/2]+1024)/2048));
				else
					mref[img->frame_cycle][j+1][i]=max(0,min(255,(int)(
					img4Y_tmp[j/2               ][i/2               ] +
					img4Y_tmp[min(je2/2+1,j/2+1)][i/2               ] +
					img4Y_tmp[j/2               ][min(ie2/2+1,i/2+1)] +
					img4Y_tmp[min(je2/2+1,j/2+1)][min(ie2/2+1,i/2+1)] + 2048)/4096));
				
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
			for (j=0; j < img->height_cr; j++)
				memcpy(mcef[img->frame_cycle][uv][j],imgUV[uv][j],img->width_cr); /* just copy 1/1 pix, interpolate "online" */
			
}

/************************************************************************
*  Name :   one_forthpix_2()
*
*  Description: 
*      Upsample 4 times, store them in mref_P, mcef_P for next P picture
*      and move them to mref, mcef
*
************************************************************************/

void oneforthpix_2(struct img_par *img)
{
  // int img4Y_tmp[576][704];
  int is;
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
			img4Y_tmp[2*j][i2  ]=imgY[j][i]*1024;    /* 1/1 pix pos */
			img4Y_tmp[2*j][i2+1]=is*32;              /* 1/2 pix pos */
		}
  }
  
  for (i=0; i < img->width*2; i++)
	{
		for (j=0; j < img->height; j++)
		{
			j2=j*2;
			j4=j*4;
			/* change for TML4, use 6 TAP vertical filter */
			is=(ONE_FOURTH_TAP[0][0]*(img4Y_tmp[j2         ][i]+img4Y_tmp[min(2*img->height-2,j2+2)][i])+
				ONE_FOURTH_TAP[1][0]*(img4Y_tmp[max(0,j2-2)][i]+img4Y_tmp[min(2*img->height-2,j2+4)][i])+
				ONE_FOURTH_TAP[2][0]*(img4Y_tmp[max(0,j2-4)][i]+img4Y_tmp[min(2*img->height-2,j2+6)][i]))/32;
			
			img4Y_tmp[j2+1][i]=is;                  /* 1/2 pix */
			mref_P[j4  ][i*2]=max(0,min(255,(int)((img4Y_tmp[j2][i]+512)/1024)));     /* 1/2 pix */
			mref_P[j4+2][i*2]=max(0,min(255,(int)((is+512)/1024))); /* 1/2 pix */
		}
	}
  
  /* 1/4 pix */
  /* luma */
  ie2=(img->width-1)*4;
  je2=(img->height-1)*4;
  
  for (j=0;j<je2+4;j+=2)
    for (i=0;i<ie2+3;i+=2)
      mref_P[j][i+1]=max(0,min(255,(int)(img4Y_tmp[j/2][i/2]+img4Y_tmp[j/2][min(ie2/2+1,i/2+1)]+1024)/2048));
		
		for (i=0;i<ie2+4;i++)
		{
			for (j=0;j<je2+3;j+=2)
			{
				if( i%2 == 0 )
					mref_P[j+1][i]=max(0,min(255,(int)(img4Y_tmp[j/2][i/2]+img4Y_tmp[min(je2/2+1,j/2+1)][i/2]+1024)/2048));
				else
					mref_P[j+1][i]=max(0,min(255,(int)(
					img4Y_tmp[j/2               ][i/2               ] +
					img4Y_tmp[min(je2/2+1,j/2+1)][i/2               ] +
					img4Y_tmp[j/2               ][min(ie2/2+1,i/2+1)] +
					img4Y_tmp[min(je2/2+1,j/2+1)][min(ie2/2+1,i/2+1)] + 2048)/4096));
				
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
			for (j=0; j < img->height_cr; j++)
				memcpy(mcef_P[uv][j],imgUV[uv][j],img->width_cr); /* just copy 1/1 pix, interpolate "online" */
			
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
  /*GH: see dynamic mem allocation in image.c                            */
  /*byte imgY_tmp[288][352]; */                     /* temp luma image   */
  /*byte imgUV_tmp[2][144][176]; */                 /* temp chroma image */

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

/************************************************************************
*
*  Name :       dummy_slice_too_big()
*
*  Description: Just a placebo
*
************************************************************************/
Boolean	dummy_slice_too_big(int bits_slice)
{
	return FALSE;
}