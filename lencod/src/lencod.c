// *************************************************************************************
// *************************************************************************************
// Lencod.c	 H.26L encoder project main
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
// Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
// Stephan Wenger                  <stewe@cs.tu-berlin.de>
// Jani Lainema                    <jani.lainema@nokia.com>
// Byeong-Moon Jeon                <jeonbm@lge.com>
// Yoon-Seong Soh                  <yunsung@lge.com>
// Thomas Stockhammer              <stockhammer@ei.tum.de>
// Detlev Marpe                    <marpe@hhi.de>
// Guido Heising                   <heising@hhi.de> 
// *************************************************************************************
// *************************************************************************************

#include "contributors.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdlib.h>

#include "global.h"
#include "configfile.h"

#define TML 	"6"
#define VERSION "6.50"

InputParameters inputs, *input = &inputs;
ImageParameters images, *img   = &images;
StatParameters  stats,  *stat  = &stats;
SNRParameters   snrs,   *snr   = &snrs;


#ifdef _ADAPT_LAST_GROUP_
int initial_Bframes = 0;
#endif


int main(int argc,char **argv)
{
	p_dec = p_stat = p_log = p_datpart = p_trace = NULL;

	Configure (argc, argv);

	/* Initialize Image Parameters */
	init_img();

	/* Allocate Slice data struct */
	malloc_slice();

#ifdef _RD_OPT_
	/* create and init structure for rd-opt. mode decision */
	init_rdopt ();
#endif
#ifdef _FAST_FULL_ME_
	/* create arrays for fast full motion estimation */
	InitializeFastFullIntegerSearch	(input->search_range);
#endif

	/* Initialize Statistic Parameters */
	init_stat();

	/* allocate memory for frame buffers*/
	get_mem4global_buffers(); 

	/* Just some information which goes to the standard output */
	information_init(argv[1]);

	/* Write sequence header; open bitstream files */
	start_sequence();

	/* B pictures */
	Bframe_ctr=0;
	tot_time=0;                 // time for total encoding session 

#ifdef _ADAPT_LAST_GROUP_
	if (input->last_frame > 0)
	  input->no_frames = 1 + (input->last_frame + input->jumpd) / (input->jumpd + 1);
	initial_Bframes = input->successive_Bframe;
#endif

	for (img->number=0; img->number < input->no_frames; img->number++)
	{
		if (img->number == 0)           
			img->type = INTRA_IMG;				/* set image type for first image to I-frame */
		else                          
			img->type = INTER_IMG;				/* P-frame */

#ifdef _ADAPT_LAST_GROUP_
		if (input->successive_Bframe && input->last_frame && img->number+1 == input->no_frames)
		  {
		    int bi = (int)((float)(input->jumpd+1)/(input->successive_Bframe+1.0)+0.499999);
		    input->successive_Bframe = (input->last_frame-(img->number-1)*(input->jumpd+1))/bi-1;
		  }
#endif

		encode_one_frame(); /* encode one I- or P-frame */ 
		
		if ((input->successive_Bframe != 0) && (img->number > 0)) /* B-frame(s) to encode */
		{
			img->type = B_IMG;						/* set image type to B-frame */
			for(img->b_frame_to_code=1; img->b_frame_to_code<=input->successive_Bframe; img->b_frame_to_code++) 
				encode_one_frame();	 /* encode one B-frame */ 					
		}
	}

	/* terminate sequence */
	terminate_sequence();

#ifdef _FAST_FULL_ME_
	/* free arrays for fast full motion estimation */
	ClearFastFullIntegerSearch ();
#endif
#ifdef _RD_OPT_
	/* free structure for rd-opt. mode decision */
	clear_rdopt ();
#endif

	/* report everything */
	report();
	
	free_slice();

	/* free allocated memory for frame buffers*/
	free_mem4global_buffers(); 

	/* free image mem */
	free_img ();

	return 0; 
}


/************************************************************************
*
*  Name :       void init_img()
*
*  Description: Initializes the Image structure with appropriate parameters
*
*  Input      : Input Parameters struct inp_par *inp
*
*  Output     : Image Parameters struct img_par *img
*
************************************************************************/
void init_img()
{
    int i,j;

    img->no_multpred=input->no_multpred;
#ifdef _ADDITIONAL_REFERENCE_FRAME_
    img->buf_cycle = max (input->no_multpred, input->add_ref_frame+1);
#else
    img->buf_cycle = input->no_multpred;
#endif
    img->framerate=INIT_FRAME_RATE;   /* The basic frame rate (of the original sequence) */

    get_mem_mv (&(img->mv));
    get_mem_mv (&(img->p_fwMV));
    get_mem_mv (&(img->p_bwMV));
    get_mem_mv (&(img->all_mv));
    get_mem_mv (&(img->all_bmv));

	img->width    = input->img_width;
	img->height   = input->img_height;
	img->width_cr = input->img_width/2;
	img->height_cr= input->img_height/2;
 
		if(((img->mb_data) = (Macroblock *) calloc((img->width/MB_BLOCK_SIZE) * (img->height/MB_BLOCK_SIZE),sizeof(Macroblock))) == NULL) 
			no_mem_exit(1);

 		if(((img->slice_numbers) = (int *) calloc((img->width/MB_BLOCK_SIZE) * (img->height/MB_BLOCK_SIZE),sizeof(int))) == NULL) 
			no_mem_exit(1);
 
 
		init(img);
    
    /* allocate memory for intra pred mode buffer for each block: img->ipredmode*/ 
    /* int  img->ipredmode[90][74]; */
    get_mem2Dint(&(img->ipredmode), img->width/BLOCK_SIZE+2, img->height/BLOCK_SIZE+2);

    /* Prediction mode is set to -1 outside the frame, indicating that no prediction can be made from this part*/
    for (i=0; i < img->width/BLOCK_SIZE+1; i++)
    {
    	img->ipredmode[i+1][0]=-1;
    }
    for (j=0; j < img->height/BLOCK_SIZE+1; j++)
    {
    	img->ipredmode[0][j+1]=-1;
    }

    img->mb_y_upd=0;

}


void free_img ()
{
  free_mem_mv (img->mv);
  free_mem_mv (img->p_fwMV);
  free_mem_mv (img->p_bwMV);
  free_mem_mv (img->all_mv);
  free_mem_mv (img->all_bmv);
}


/************************************************************************
*
*  Name :       void malloc_slice()
*
*  Description: Allocates the slice structure along with its dependent 
*				data structures
*
*  Input      : Input Parameters struct inp_par *inp,  struct img_par *img
*
************************************************************************/
void malloc_slice()
{
	int i;
	DataPartition *dataPart;
	Slice *currSlice;
	const int buffer_size = (img->width * img->height * 3)/2; /* DM 070301: The only assumption here is that we */
															  /* do not consider the case of data expansion. */
															  /* So this is a strictly conservative estimation	*/
															  /* of the size of the bitstream buffer for one frame */																												/* ~DM */

	switch(input->of_mode) /* init depending on NAL mode */
	{
		case PAR_OF_26L:
			/* Current File Format */
			img->currentSlice = (Slice *) calloc(1, sizeof(Slice));
			if ( (currSlice = img->currentSlice) == NULL)
			{
				fprintf(stderr, "Memory allocation for Slice datastruct in NAL-mode %d failed", input->of_mode);
				exit(1);
			}
			if (input->symbol_mode == CABAC)
			{
				/* create all context models */
				currSlice->mot_ctx = create_contexts_MotionInfo();
				currSlice->tex_ctx = create_contexts_TextureInfo();
			}

			switch(input->partition_mode)
			{
			case PAR_DP_1:
				currSlice->max_part_nr = 1;
				break;
			default:
				error("Data Partitioning Mode not supported!");
				break;
			}



			currSlice->partArr = (DataPartition *) calloc(currSlice->max_part_nr, sizeof(DataPartition));
			if (currSlice->partArr == NULL)
			{
				fprintf(stderr, "Memory allocation for Data Partition datastruct in NAL-mode %d failed", input->of_mode);
				exit(1);
			}			
			for (i=0; i<currSlice->max_part_nr; i++) /* loop over all data partitions */
			{				
				dataPart = &(currSlice->partArr[i]);
				dataPart->bitstream = (Bitstream *) calloc(1, sizeof(Bitstream));
				if (dataPart->bitstream == NULL)
				{
					fprintf(stderr, "Memory allocation for Bitstream datastruct in NAL-mode %d failed", input->of_mode);
					exit(1);
				}
				dataPart->bitstream->streamBuffer = (byte *) calloc(buffer_size, sizeof(byte));
				if (dataPart->bitstream->streamBuffer == NULL)
				{
					fprintf(stderr, "Memory allocation for bitstream buffer in NAL-mode %d failed", input->of_mode);
					exit(1);
				}
				/* Initialize storage of bitstream parameters */
				dataPart->bitstream->stored_bits_to_go = 8;
				dataPart->bitstream->stored_byte_pos = 0;
				dataPart->bitstream->stored_byte_buf = 0;
			}
			return;
		default: 
			fprintf(stderr, "Output File Mode %d not supported", input->of_mode);
			exit(1);
	}				
	
}



/************************************************************************
*
*  Name :       void free_slice()
*
*  Description: Memory frees of the Slice structure and of its dependent 
*				data structures
*
*  Input      : Input Parameters struct inp_par *inp,  struct img_par *img
*
************************************************************************/
void free_slice()
{
	int i;
	DataPartition *dataPart;
	Slice *currSlice = img->currentSlice;

	switch(input->of_mode) /* init depending on NAL mode */
	{
		case PAR_OF_26L:
			/* Current File Format */
			/* Remenber that we have two partitions for CABAC */
			for (i=0; i<currSlice->max_part_nr; i++) /* loop over all data partitions */
			{	
				dataPart = &(currSlice->partArr[i]);	
				if (dataPart->bitstream->streamBuffer != NULL)
					free(dataPart->bitstream->streamBuffer);
				if (dataPart->bitstream != NULL)
					free(dataPart->bitstream);
			}
			if (currSlice->partArr != NULL)
				free(currSlice->partArr);
			if (input->symbol_mode == CABAC)
			{
				/* delete all context models */
				delete_contexts_MotionInfo(currSlice->mot_ctx);
				delete_contexts_TextureInfo(currSlice->tex_ctx);
			}
			if (currSlice != NULL)
				free(img->currentSlice);
			break;
		default: 
			fprintf(stderr, "Output File Mode %d not supported", input->of_mode);
			exit(1);
	}				
	
}


/************************************************************************
*
*  Name :       void init_stat()
*
*  Description: Initializes the Image structure with appropriate parameters
*
*  Input      : Input Parameters struct inp_par *inp
*
*  Output     : Statistic Parameters struct stat_par *stat
*
************************************************************************/
void init_stat()
{
	int i;
	stat->mode_use_Bframe = (int *)malloc(sizeof(int)*41);
	stat->bit_use_mode_Bframe =  (int *)malloc(sizeof(int)*41);
	for(i=0; i<41; i++) 
		stat->mode_use_Bframe[i]=stat->bit_use_mode_Bframe[i]=0;
}

/************************************************************************
*
*  Name :       void report()
*
*  Description: Reports the gathered information to appropriate outputs
*
*  Input      : struct inp_par *inp, 
*				struct img_par *img, 
*				struct stat_par *stat,
*				struct stat_par *stat
*
*  Output     : None
*
************************************************************************/
void report()
{
	int bit_use[2][2] ;
	int i,j;
	char name[20];
	int bit_use_Bframe=0;
	int total_bits;
	float frame_rate;

#ifndef WIN32
	time_t now;
	struct tm *l_time;
	char string[1000];
#else
	char timebuf[128];
#endif
	bit_use[0][0]=1;
	bit_use[1][0]=max(1,input->no_frames-1);

	/*  Accumulate bit usage for inter and intra frames */
	bit_use[0][1]=bit_use[1][1]=0;

	for (i=0; i < 9; i++)
		bit_use[1][1] += stat->bit_use_mode_inter[i];

	for (j=0;j<2;j++)
	{
		bit_use[j][1]+=stat->bit_use_head_mode[j];
		bit_use[j][1]+=stat->tmp_bit_use_cbp[j];
		bit_use[j][1]+=stat->bit_use_coeffY[j];
		bit_use[j][1]+=stat->bit_use_coeffC[j];
	}

	/* B pictures */
	if(Bframe_ctr!=0) {
		bit_use_Bframe=0;
		for(i=0; i<41; i++)
			bit_use_Bframe += stat->bit_use_mode_Bframe[i]; // fw_predframe_no, blk_size
		bit_use_Bframe += stat->bit_use_head_mode[2];
		bit_use_Bframe += stat->tmp_bit_use_cbp[2];
		bit_use_Bframe += stat->bit_use_coeffY[2];
		bit_use_Bframe += stat->bit_use_coeffC[2];

		stat->bitrate_P=(stat->bit_ctr_0+stat->bit_ctr_P)*(float)(img->framerate/(input->jumpd+1))/input->no_frames;	
#ifdef _ADAPT_LAST_GROUP_
		stat->bitrate_B=(stat->bit_ctr_B)*(float)(img->framerate/(input->jumpd+1))*initial_Bframes/Bframe_ctr;
#else
		stat->bitrate_B=(stat->bit_ctr_B)*(float)(img->framerate/(input->jumpd+1))*input->successive_Bframe/Bframe_ctr;
#endif
	}
	else {
		if (input->no_frames > 1) 
		{
			stat->bitrate=(bit_use[0][1]+bit_use[1][1])*(float)img->framerate/(input->no_frames*(input->jumpd+1));
		}
	}

	fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout,   " Freq. for encoded bitstream       : %1.0f\n",(float)img->framerate/(float)(input->jumpd+1));
	if(input->hadamard)
		fprintf(stdout," Hadamard transform                : Used\n");
	else
		fprintf(stdout," Hadamard transform                : Not used\n");
 
	fprintf(stdout," Image format                      : %dx%d\n",input->img_width,input->img_height);

	if(input->intra_upd)
		fprintf(stdout," Error robustness                  : On\n");
	else 
		fprintf(stdout," Error robustness                  : Off\n");
	fprintf(stdout,    " Search range                      : %d\n",input->search_range);

	if(input->mv_res)
		fprintf(stdout," MV resolution                     : 1/8-pel\n");
	else
		fprintf(stdout," MV resolution                     : 1/4-pel\n");

#ifdef _ADDITIONAL_REFERENCE_FRAME_
	if (input->add_ref_frame >= input->no_multpred)
	{
	    fprintf(stdout,   " No of ref. frames used in P pred  : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
	    if(input->successive_Bframe != 0)
	      fprintf(stdout,   " No of ref. frames used in B pred  : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
	}
	else
#endif
	{
		fprintf(stdout,   " No of ref. frames used in P pred  : %d\n",input->no_multpred);
		if(input->successive_Bframe != 0)
			fprintf(stdout,   " No of ref. frames used in B pred  : %d\n",input->no_multpred);
	}
	fprintf(stdout,   " Total encoding time for the seq.  : %.3f sec \n",tot_time*0.001); 

	/* B pictures */
	fprintf(stdout, " Sequence type                     :" );
	if(input->successive_Bframe==1) 	fprintf(stdout, " IBPBP (QP: I %d, P %d, B %d) \n", 
		input->qp0, input->qpN, input->qpB);
	else if(input->successive_Bframe==2) fprintf(stdout, " IBBPBBP (QP: I %d, P %d, B %d) \n", 
		input->qp0, input->qpN, input->qpB);	
	else if(input->successive_Bframe==0)	fprintf(stdout, " IPPP (QP: I %d, P %d) \n", 	input->qp0, input->qpN);

	/* DM: report on entropy coding  method */
	if (input->symbol_mode == UVLC)
		fprintf(stdout," Entropy coding method             : UVLC\n");
	else
		fprintf(stdout," Entropy coding method             : CABAC\n");

#ifdef _FULL_SEARCH_RANGE_
	if (input->full_search == 2)
	  fprintf(stdout," Search range restrictions         : none\n");
	else if (input->full_search == 1)
	  fprintf(stdout," Search range restrictions         : older reference frames\n");
	else
	  fprintf(stdout," Search range restrictions         : smaller blocks and older reference frames\n");
#endif
#ifdef _RD_OPT_
	if (input->rdopt)
	  fprintf(stdout," RD-optimized mode decision        : used\n");
	else
	  fprintf(stdout," RD-optimized mode decision        : not used\n");
#endif

	switch(input->partition_mode)
		{
		case PAR_DP_1:
			fprintf(stdout," Data Partitioning Mode            : 1 partition \n");
			break;
		default:
			fprintf(stdout," Data Partitioning Mode            : not supported\n");
			break;
		}


		switch(input->of_mode)
		{
		case PAR_OF_26L:
			fprintf(stdout," Output File Format                : H.26L File Format \n");
			break;
		default:
			fprintf(stdout," Output File Format                : not supported\n");
			break;
		}


	

	fprintf(stdout,"------------------ Average data all frames  ------------------------------\n");
	fprintf(stdout," SNR Y(dB)                         : %5.2f\n",snr->snr_ya);
	fprintf(stdout," SNR U(dB)                         : %5.2f\n",snr->snr_ua);
	fprintf(stdout," SNR V(dB)                         : %5.2f\n",snr->snr_va);
  
	if(Bframe_ctr!=0) {

		fprintf(stdout, " Total bits                        : %d (I %5d, P %5d, B %d) \n",
						total_bits=stat->bit_ctr_P + stat->bit_ctr_0 + stat->bit_ctr_B, stat->bit_ctr_0, stat->bit_ctr_P, stat->bit_ctr_B); 

		frame_rate = (float)img->framerate / ( (float) (input->jumpd - input->successive_Bframe + 1) );
		stat->bitrate= ((float) total_bits * frame_rate)/((float) (input->no_frames + Bframe_ctr));

		fprintf(stdout, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, stat->bitrate/1000);

	}
	else {
		if (input->symbol_mode == UVLC)
			fprintf(stdout, " Total bits                        : %d (I %5d, P %5d) \n", 
				total_bits=bit_use[0][1]+bit_use[1][1], bit_use[0][1], bit_use[1][1]);
		else
			fprintf(stdout, " Total bits                        : %d (I %5d, P %5d) \n",
			total_bits=stat->bit_ctr_P + stat->bit_ctr_0 , stat->bit_ctr_0, stat->bit_ctr_P); 

		frame_rate = (float)img->framerate / ( (float) (input->jumpd + 1) );
		stat->bitrate= ((float) total_bits * frame_rate)/((float) input->no_frames );

		fprintf(stdout, " Bit rate (kbit/s)  @ %2.2f Hz     : %5.2f\n", frame_rate, stat->bitrate/1000);
	}	
	
	fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout,"Exit TML %s encoder ver %s\n", TML, VERSION);

	/* status file */
	if ((p_stat=fopen("stat.dat","wb"))==0)
	{
		printf("Error open file %s  \n", "stat.dat");
		exit(0);
	}
	fprintf(p_stat," -------------------------------------------------------------- \n");
	fprintf(p_stat,"  This file contains statistics for the last encoded sequence   \n");
	fprintf(p_stat," -------------------------------------------------------------- \n");
	fprintf(p_stat,   " Sequence                     : %s\n",input->infile);
	fprintf(p_stat,   " No.of coded pictures         : %d\n",input->no_frames+Bframe_ctr);
	fprintf(p_stat,   " Freq. for encoded bitstream  : %3.0f\n",frame_rate);
	
	/* B pictures*/
	if(input->successive_Bframe != 0) {
		fprintf(p_stat,   " BaseLayer Bitrate(kb/s)      : %6.2f\n", stat->bitrate_P/1000);
		fprintf(p_stat,   " EnhancedLyaer Bitrate(kb/s)  : %6.2f\n", stat->bitrate_B/1000);
	}
	else 
		fprintf(p_stat,   " Bitrate(kb/s)                : %6.2f\n", stat->bitrate/1000);
  
	if(input->hadamard)
		fprintf(p_stat," Hadamard transform           : Used\n");
	else
		fprintf(p_stat," Hadamard transform           : Not used\n");

	fprintf(p_stat," Image format                 : %dx%d\n",input->img_width,input->img_height);
	
	if(input->intra_upd)
		fprintf(p_stat," Error robustness             : On\n");
	else 
		fprintf(p_stat," Error robustness             : Off\n");

	fprintf(p_stat,    " Search range                 : %d\n",input->search_range);

	if(input->mv_res)
		fprintf(p_stat," MV resolution                : 1/8-pel\n");
	else
		fprintf(p_stat," MV resolution                : 1/4-pel\n");

#ifdef _ADDITIONAL_REFERENCE_FRAME_
	if (input->add_ref_frame >= input->no_multpred)
	  {
	    fprintf(p_stat,   " No of frame used in P pred   : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
	    if(input->successive_Bframe != 0)
	      fprintf(p_stat,   " No of frame used in B pred   : %d (+ no. %d)\n",input->no_multpred,input->add_ref_frame);
	  }
	else
#endif
	{
		fprintf(p_stat,   " No of frame used in P pred   : %d\n",input->no_multpred);
		if(input->successive_Bframe != 0)
			fprintf(p_stat,   " No of frame used in B pred   : %d\n",input->no_multpred);
	}
	if (input->symbol_mode == UVLC)
		fprintf(p_stat,   " Entropy coding method        : UVLC\n");
	else
		fprintf(p_stat,   " Entropy coding method        : CABAC\n");

#ifdef _FULL_SEARCH_RANGE_
	if (input->full_search == 2)
	  fprintf(p_stat," Search range restrictions    : none\n");
	else if (input->full_search == 1)
	  fprintf(p_stat," Search range restrictions    : older reference frames\n");
	else
	  fprintf(p_stat," Search range restrictions    : smaller blocks and older reference frames\n");
#endif
#ifdef _RD_OPT_
	if (input->rdopt)
	  fprintf(p_stat," RD-optimized mode decision   : used\n");
	else
	  fprintf(p_stat," RD-optimized mode decision   : not used\n");
#endif

	fprintf(p_stat," -------------------|---------------|---------------|\n");
	fprintf(p_stat,"     Item           |     Intra     |   All frames  |\n");
	fprintf(p_stat," -------------------|---------------|---------------|\n");
	fprintf(p_stat," SNR Y(dB)          |");
	fprintf(p_stat," %5.2f         |",snr->snr_y1);
	fprintf(p_stat," %5.2f         |\n",snr->snr_ya);
	fprintf(p_stat," SNR U/V (dB)       |");
	fprintf(p_stat," %5.2f/%5.2f   |",snr->snr_u1,snr->snr_v1);
	fprintf(p_stat," %5.2f/%5.2f   |\n",snr->snr_ua,snr->snr_va);

	/* QUANT. */
	fprintf(p_stat," Average quant      |");
	fprintf(p_stat," %5d         |",absm(input->qp0));
	fprintf(p_stat," %5.2f         |\n",(float)stat->quant1/max(1.0,(float)stat->quant0));
	
	/* MODE */
	fprintf(p_stat,"\n -------------------|---------------|\n");
	fprintf(p_stat,"   Intra            |   Mode used   |\n");
	fprintf(p_stat," -------------------|---------------|\n");

	fprintf(p_stat," Mode 0  intra old\t| %5d         |\n",stat->mode_use_intra[0]);
	for (i=1;i<24;i++)
		fprintf(p_stat," Mode %d intra new\t| %5d         |\n",i,stat->mode_use_intra[i]);

	fprintf(p_stat,"\n -------------------|---------------|---------------|\n");
	fprintf(p_stat,"   Inter            |   Mode used   | Vector bit use|\n");
	fprintf(p_stat," -------------------|---------------|---------------|");

	fprintf(p_stat,"\n Mode 0  (copy)  \t| %5d         | %5.0f         |",stat->mode_use_inter[0],(float)stat->bit_use_mode_inter[0]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 1  (16x16) \t| %5d         | %5.0f         |",stat->mode_use_inter[1],(float)stat->bit_use_mode_inter[1]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 2  (16x8)  \t| %5d         | %5.0f         |",stat->mode_use_inter[2],(float)stat->bit_use_mode_inter[2]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 3  (8x16)  \t| %5d         | %5.0f         |",stat->mode_use_inter[3],(float)stat->bit_use_mode_inter[3]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 4  (8x8)   \t| %5d         | %5.0f         |",stat->mode_use_inter[4],(float)stat->bit_use_mode_inter[4]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 5  (8x4)   \t| %5d         | %5.0f         |",stat->mode_use_inter[5],(float)stat->bit_use_mode_inter[5]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 6  (4x8)   \t| %5d         | %5.0f         |",stat->mode_use_inter[6],(float)stat->bit_use_mode_inter[6]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 7  (4x4)   \t| %5d         | %5.0f         |",stat->mode_use_inter[7],(float)stat->bit_use_mode_inter[7]/(float)bit_use[1][0]);
	fprintf(p_stat,"\n Mode 8 intra old\t| %5d         | --------------|",stat->mode_use_inter[8]);
	for (i=9;i<33;i++)
		fprintf(p_stat,"\n Mode %d intr.new \t| %5d         |",i,stat->mode_use_inter[i]);

	/* B pictures */
	if(input->successive_Bframe!=0 && Bframe_ctr!=0) {
		fprintf(p_stat,"\n\n -------------------|---------------|\n");
		fprintf(p_stat,"   B frame          |   Mode used   |\n");
		fprintf(p_stat," -------------------|---------------|");
  
		for(i=0; i<16; i++)
			fprintf(p_stat,"\n Mode %d   \t| %5d         |", i, stat->mode_use_Bframe[i]);        
		fprintf(p_stat,"\n Mode %d intra old\t| %5d     |", 16, stat->mode_use_Bframe[16]);             
		for (i=17; i<41; i++)
			fprintf(p_stat,"\n Mode %d intr.new \t| %5d     |",i, stat->mode_use_Bframe[i]);              
	}
  
	fprintf(p_stat,"\n\n -------------------|---------------|---------------|---------------|\n");
	fprintf(p_stat,"  Bit usage:        |     Intra     |    Inter      |   B frame     |\n");
	fprintf(p_stat," -------------------|---------------|---------------|---------------|\n");

	fprintf(p_stat," Header+modeinfo    |");
	fprintf(p_stat," %7d       |",stat->bit_use_head_mode[0]);
	fprintf(p_stat," %7d       |",stat->bit_use_head_mode[1]/bit_use[1][0]);
	if(input->successive_Bframe!=0 && Bframe_ctr!=0) 
		fprintf(p_stat," %7d       |",stat->bit_use_head_mode[2]/Bframe_ctr);
	else fprintf(p_stat," %7d       |", 0);
	fprintf(p_stat,"\n");

	fprintf(p_stat," CBP Y/C            |");
	for (j=0; j < 2; j++)
	{
		fprintf(p_stat," %7.2f       |", (float)stat->tmp_bit_use_cbp[j]/bit_use[j][0]);
	}
	if(input->successive_Bframe!=0 && Bframe_ctr!=0) 
		fprintf(p_stat," %7.2f       |", (float)stat->tmp_bit_use_cbp[2]/Bframe_ctr);
	else fprintf(p_stat," %7.2f       |", 0.);
	fprintf(p_stat,"\n");

	if(input->successive_Bframe!=0 && Bframe_ctr!=0) 
		fprintf(p_stat," Coeffs. Y          | %7.2f      | %7.2f       | %7.2f       |\n",
			(float)stat->bit_use_coeffY[0]/bit_use[0][0], (float)stat->bit_use_coeffY[1]/bit_use[1][0], (float)stat->bit_use_coeffY[2]/Bframe_ctr);
	else 
		fprintf(p_stat," Coeffs. Y          | %7.2f      | %7.2f       | %7.2f       |\n",
			(float)stat->bit_use_coeffY[0]/bit_use[0][0], (float)stat->bit_use_coeffY[1]/bit_use[1][0], 0.);
  
	if(input->successive_Bframe!=0 && Bframe_ctr!=0) 
		fprintf(p_stat," Coeffs. C          | %7.2f       | %7.2f       | %7.2f       |\n",
			(float)stat->bit_use_coeffC[0]/bit_use[0][0], (float)stat->bit_use_coeffC[1]/bit_use[1][0], (float)stat->bit_use_coeffC[2]/Bframe_ctr);   
	else 
		fprintf(p_stat," Coeffs. C          | %7.2f       | %7.2f       | %7.2f       |\n",
			(float)stat->bit_use_coeffC[0]/bit_use[0][0], (float)stat->bit_use_coeffC[1]/bit_use[1][0], 0.);   

	fprintf(p_stat," -------------------|---------------|---------------|---------------|\n");
  
	fprintf(p_stat," average bits/frame |");
	for (i=0; i < 2; i++) 
	{
		fprintf(p_stat," %7d       |",bit_use[i][1]/bit_use[i][0]);
	}
	if(input->successive_Bframe!=0 && Bframe_ctr!=0) 
		fprintf(p_stat," %7d       |", bit_use_Bframe/Bframe_ctr);
	else fprintf(p_stat," %7d       |", 0);

	fprintf(p_stat,"\n");
	fprintf(p_stat," -------------------|---------------|---------------|---------------|\n");

	/* write to log file	*/
	if (fopen("log.dat","r")==0)                      /* check if file exist */
	{
		if ((p_log=fopen("log.dat","a"))==0)            /* append new statistic at the end */
		{
			printf("Error open file %s  \n","log.dat");
			exit(0);
		}
		else                                            /* Create header for new log file */
		{
			fprintf(p_log," ---------------------------------------------------------------------------------------------------------------------------------------------------------------- \n");
			fprintf(p_log,"|            Encoder statistics. This file is generated during first encoding session, new sessions will be appended                                               |\n");
			fprintf(p_log," ---------------------------------------------------------------------------------------------------------------------------------------------------------------- \n");
			fprintf(p_log,"| Date  | Time  |    Sequence        |#Img|Quant1|QuantN|Format|Hadamard|Search r|#Ref |Freq |Intra upd|SNRY 1|SNRU 1|SNRV 1|SNRY N|SNRU N|SNRV N|#Bitr P|#Bitr B|\n");
			fprintf(p_log," ---------------------------------------------------------------------------------------------------------------------------------------------------------------- \n");
		}
	}
	else
		p_log=fopen("log.dat","a");                     /* File exist,just open for appending */

#ifdef WIN32
	_strdate( timebuf );
	fprintf(p_log,"| %1.5s |",timebuf );

	_strtime( timebuf);
	fprintf(p_log," % 1.5s |",timebuf);
#else
	now = time ((time_t *) NULL);	/* Get the system time and put it into 'now' as 'calender time' */
	time (&now);
	l_time = localtime (&now);
	strftime (string, sizeof string, "%d-%b-%Y", l_time);
	fprintf(p_log,"| %1.5s |",string );

	strftime (string, sizeof string, "%H:%M:%S", l_time);
	fprintf(p_log,"| %1.5s |",string );
#endif

	for (i=0;i<20;i++)
		name[i]=input->infile[i+max(0,strlen(input->infile)-20)]; /* write last part of path, max 20 chars */
	fprintf(p_log,"%20.20s|",name);

	fprintf(p_log,"%3d |",input->no_frames);
	fprintf(p_log,"  %2d  |",input->qp0);
	fprintf(p_log,"  %2d  |",input->qpN);

	
	fprintf(p_log,"%dx%d|",input->img_width,input->img_height);


	if (input->hadamard==1)
		fprintf(p_log,"   ON   |");
	else
		fprintf(p_log,"   OFF  |");

	fprintf(p_log,"   %2d   |",input->search_range );

	fprintf(p_log," %2d  |",input->no_multpred);

	fprintf(p_log," %2d  |",img->framerate/(input->jumpd+1));

	if (input->intra_upd==1)
		fprintf(p_log,"   ON    |");
	else
		fprintf(p_log,"   OFF   |");

	fprintf(p_log,"%5.3f|",snr->snr_y1);
	fprintf(p_log,"%5.3f|",snr->snr_u1);
	fprintf(p_log,"%5.3f|",snr->snr_v1);
	fprintf(p_log,"%5.3f|",snr->snr_ya);
	fprintf(p_log,"%5.3f|",snr->snr_ua);
	fprintf(p_log,"%5.3f|",snr->snr_va);
	if(input->successive_Bframe != 0)
	{
		fprintf(p_log,"%7.0f|",stat->bitrate_P);
		fprintf(p_log,"%7.0f|\n",stat->bitrate_B);
	}
	else
	{
		fprintf(p_log,"%7.0f|",stat->bitrate);
		fprintf(p_log,"%7.0f|\n",0.0);
	}

	fclose(p_log);

	p_log=fopen("data.txt","a");

	if(input->successive_Bframe != 0) // B picture used
	{
		fprintf(p_log, "%3d %2d %2d %2.2f %2.2f %2.2f %5d "
			    "%2.2f %2.2f %2.2f %5d "
				"%2.2f %2.2f %2.2f %5d %5d %.3f\n",
				input->no_frames, input->qp0, input->qpN,
				snr->snr_y1,
				snr->snr_u1,
				snr->snr_v1,
				stat->bit_ctr_0,
				0.0,
				0.0,
				0.0,
				0,
				snr->snr_ya,
				snr->snr_ua,
				snr->snr_va,
				(stat->bit_ctr_0+stat->bit_ctr)/input->no_frames,
				stat->bit_ctr_B/Bframe_ctr,
				(double)0.001*tot_time/(input->no_frames+Bframe_ctr));
	}
	else 
	{
		fprintf(p_log, "%3d %2d %2d %2.2f %2.2f %2.2f %5d "
			    "%2.2f %2.2f %2.2f %5d "
				"%2.2f %2.2f %2.2f %5d %5d %.3f\n",
				input->no_frames, input->qp0, input->qpN,
				snr->snr_y1,
				snr->snr_u1,
				snr->snr_v1,
				stat->bit_ctr_0,
				0.0,
				0.0,
				0.0,
				0,
				snr->snr_ya,
				snr->snr_ua,
				snr->snr_va,
				(stat->bit_ctr_0+stat->bit_ctr)/input->no_frames,
				0,
				(double)0.001*tot_time/input->no_frames);
	}

	fclose(p_log);

	/* B pictures */
	free(stat->mode_use_Bframe);
	free(stat->bit_use_mode_Bframe);
// getchar();
}

/************************************************************************
*
*  Name :       init()
*
*  Description: Some matrices are initilized here.
*
*  Input      : none
*
*  Output     : none
*
************************************************************************/
void init()
{
	int k, i, ii, ind, j, i2;

	InitMotionVectorSearchModule();
	
	/*  img->lpfilter is the table used for loop and post filter. */

	for (k=0; k < 6; k++)
	{
		for (i=1; i <= 100; i++)
		{
			ii=min(k,i);
			img->lpfilter[k][i+300]= ii;
			img->lpfilter[k][300-i]=-ii;
		}
	}
	/*  img->mv_bituse[] is the number of bits used for motion vectors.
	It is used to add a portion to the SAD depending on bit usage in the motion search
	*/

	img->mv_bituse[0]=1;
	ind=0;
	for (i=0; i < 9; i++)
	{
		ii= 2*i + 3;
		for (j=1; j <= (int)pow(2,i); j++)
		{
			ind++;
			img->mv_bituse[ind]=ii;
		}
	}

	/* quad(0:255) SNR quad array */
	for (i=0; i < 256; ++i) /* fix from TML1 / TML2 sw, truncation removed */
	{
		i2=i*i;
		img->quad[i]=i2;
	}

	/* B pictures : img->blk_bitsue[] is used when getting bidirection SAD */
	for (i=0; i < 7; i++) 
	{
		if(i==0) img->blk_bituse[i]=1;
		else if(i==1 || i==2) img->blk_bituse[i]=3;
		else img->blk_bituse[i]=5;
	}
}

/************************************************************************
*
*  Name :       information_init()
*
*  Description: Prints the header of the protocol
*
*  Input      : struct inp_par *inp, char *config_filename
*
*  Output     : none
*
************************************************************************/
void information_init(char *config_filename)
{
	printf("--------------------------------------------------------------------------\n");
	printf(" Input YUV file                    : %s \n",input->infile);
	printf(" Output H.26L bitstream            : %s \n",input->outfile);
	if (p_dec != NULL)
		printf(" Output YUV file(debug)            : test.dec \n");
	printf(" Output log file                   : log.dat \n");
	printf(" Output statistics file            : stat.dat \n");
	printf("--------------------------------------------------------------------------\n");


	printf(" Frame  Bit/pic   QP   SnrY    SnrU    SnrV    Time(ms)\n");
}


/************************************************************************
*
*  Name :       int get_mem4global_buffers(struct inp_par *inp, struct img_par *img)
*
*  Description: Dynamic memory allocation of frame size related global buffers
*               buffers are defined in global.h, allocated memory must be freed in
*               void free_mem4global_buffers()    
*
*  Input      : Input Parameters struct inp_par *inp, Image Parameters struct img_par *img
*
*  Output     : Number of allocated bytes
*
************************************************************************/
int get_mem4global_buffers()
{
	int j,memory_size=0;
#ifdef _ADAPT_LAST_GROUP_
	extern int *last_P_no;

	if ((last_P_no = (int*)malloc(img->buf_cycle*sizeof(int))) == NULL) no_mem_exit(1);
#endif
	
	/* allocate memory for encoding frame buffers: imgY, imgUV*/ 
	/* byte imgY[288][352];       */
	/* byte imgUV[2][144][176];   */
	memory_size += get_mem2D(&imgY, img->height, img->width);
	memory_size += get_mem3D(&imgUV, 2, img->height_cr, img->width_cr);
	
	/* allocate memory for reference frame buffers: imgY_org, imgUV_org*/ 
	/* byte imgY_org[288][352];       */
	/* byte imgUV_org[2][144][176];   */
	memory_size += get_mem2D(&imgY_org, img->height, img->width);
	memory_size += get_mem3D(&imgUV_org, 2, img->height_cr, img->width_cr);
	
	/* allocate memory for post filter frame buffers: imgY_pf, imgUV_pf*/ 
	/* byte imgY_pf[288][352];       */
	/* byte imgUV_pf[2][144][176];   */
	memory_size += get_mem2D(&imgY_pf, img->height, img->width);
	memory_size += get_mem3D(&imgUV_pf, 2, img->height_cr, img->width_cr);
	
	/* allocate memory for B frame coding: nextP_imgY, nextP_imgUV*/ 
	/* byte nextP_imgY[288][352];       */
	/* byte nextP_imgUV[2][144][176];   */
	memory_size += get_mem2D(&nextP_imgY, img->height, img->width);
	memory_size += get_mem3D(&nextP_imgUV, 2, img->height_cr, img->width_cr);
	
	/* allocate memory for multiple ref. frame buffers: mref, mcref*/ 
	/*byte mref[MAX_MULT_PRED][1152][1408];  */   /* 1/4 pix luma  */
	/*byte mcef[MAX_MULT_PRED][2][352][288]; */   /* pix chroma    */
	/* rows and cols for croma component mcef[ref][croma][4x][4y] are switched */
	/* compared to luma mref[ref][4y][4x] for whatever reason */
	/* number of reference frames increased by one for next P-frame*/
	memory_size += get_mem3D(&mref, img->buf_cycle+1, img->height*4, img->width*4);	
	if((mcef = (byte****)calloc(img->buf_cycle+1,sizeof(byte***))) == NULL) no_mem_exit(1);
	for(j=0;j<img->buf_cycle+1;j++)
	{
		memory_size += get_mem3D(&(mcef[j]), 2, img->width_cr*2, img->height_cr*2);
	}
	
	InitRefbuf (Refbuf11, Refbuf11_P);


	/* allocate memory for B-frame coding (last upsampled P-frame): mref_P, mcref_P*/ 
	/* is currently also used in oneforthpix_1() and _2() for coding without B-frames*/ 
	/*byte mref[1152][1408];  */   /* 1/4 pix luma  */
	/*byte mcef[2][352][288]; */   /* pix chroma    */
	memory_size += get_mem2D(&mref_P, img->height*4, img->width*4);
	memory_size += get_mem3D(&mcef_P, 2, img->width_cr*2, img->height_cr*2);
	
	if(input->successive_Bframe!=0)
	{
    /* allocate memory for temp B-frame motion vector buffer: tmp_fwMV, tmp_bwMV, dfMV, dbMV*/ 
    /* int ...MV[2][72][92];  ([2][72][88] should be enough) */
    memory_size += get_mem3Dint(&tmp_fwMV, 2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
    memory_size += get_mem3Dint(&tmp_bwMV, 2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
    memory_size += get_mem3Dint(&dfMV,     2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
    memory_size += get_mem3Dint(&dbMV,     2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
		
    /* allocate memory for temp B-frame motion vector buffer: fw_refFrArr, bw_refFrArr*/ 
    /* int ...refFrArr[72][88];  */
    memory_size += get_mem2Dint(&fw_refFrArr, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE);
    memory_size += get_mem2Dint(&bw_refFrArr, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE);
	}
	
	/* allocate memory for temp P and B-frame motion vector buffer: tmp_mv, temp_mv_block*/ 
	/* int tmp_mv[2][72][92];  ([2][72][88] should be enough) */
	memory_size += get_mem3Dint(&tmp_mv, 2, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE+4);
	
	/* allocate memory for temp quarter pel luma frame buffer: img4Y_tmp*/ 
	/* int img4Y_tmp[576][704];  (previously int imgY_tmp in global.h) */
	memory_size += get_mem2Dint(&img4Y_tmp, img->height*2, img->width*2);
	
	/* allocate memory for temp 1/8-pel luma frame buffer: img8Y_tmp*/ 
	memory_size += get_mem2D(&img8Y_tmp, 4*img->width, img->height);
	
	/* allocate memory for tmp loop filter frame buffers: imgY_tmp, imgUV_tmp*/ 
	/* byte imgY_tmp[288][352];   */
	memory_size += get_mem2D(&imgY_tmp, img->height, img->width);
	memory_size += get_mem3D(&imgUV_tmp, 2, img->height_cr, img->width_cr);
	
	/* allocate memory for reference frames of each block: refFrArr*/ 
	/* int  refFrArr[72][88];  */
	memory_size += get_mem2Dint(&refFrArr, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE);
	
	/* allocate memory for filter strength for 4x4 lums and croma blocks:loopb, loopc*/ 
	/*byte loopb[91][75]; i.e.[x][y]*/
	/*byte loopc[47][39]; */
	memory_size += get_mem2D(&loopb, img->width/BLOCK_SIZE+3, img->height/BLOCK_SIZE+3);
	memory_size += get_mem2D(&loopc, img->width_cr/BLOCK_SIZE+3, img->height_cr/BLOCK_SIZE+3);
	
	return (memory_size);
}

/************************************************************************
*
*  Name :       void free_mem4global_buffers(struct inp_par *inp, struct img_par *img)
*
*  Description: Free allocated memory of frame size related global buffers
*               buffers are defined in global.h, allocated memory is allocated in
*               int get_mem4global_buffers()    
*
*  Input      : Input Parameters struct inp_par *inp, Image Parameters struct img_par *img
*
*  Output     : none
*
************************************************************************/
void free_mem4global_buffers() 
{
	int i,j;
	
	for(i=0;i<img->height;i++)
	{
		free(imgY[i]);
	}
	free(imgY);
	
	for(i=0;i<img->height_cr;i++)
	{
		free(imgUV[0][i]);
		free(imgUV[1][i]);
	}
	free(imgUV[0]);
	free(imgUV[1]);
	free(imgUV);
	
	/* free ref frame buffers*/
	for(i=0;i<img->height;i++)
	{
		free(imgY_org[i]);
	}
	free(imgY_org);
	
	for(i=0;i<img->height_cr;i++)
	{
		free(imgUV_org[0][i]);
		free(imgUV_org[1][i]);
	}
	free(imgUV_org[0]);
	free(imgUV_org[1]);
	free(imgUV_org);
	
	/* free post filtering frame buffers*/
	for(i=0;i<img->height;i++)
	{
		free(imgY_pf[i]);
	}
	free(imgY_pf);
	
	for(i=0;i<img->height_cr;i++)
	{
		free(imgUV_pf[0][i]);
		free(imgUV_pf[1][i]);
	}
	free(imgUV_pf[0]);
	free(imgUV_pf[1]);
	free(imgUV_pf);
	
	/* free next frame buffers (for B frames)*/
	for(i=0;i<img->height;i++)
	{
		free(nextP_imgY[i]);
	}
	free(nextP_imgY);
	
	for(i=0;i<img->height_cr;i++)
	{
		free(nextP_imgUV[0][i]);
		free(nextP_imgUV[1][i]);
	}
	free(nextP_imgUV[0]);
	free(nextP_imgUV[1]);
	free(nextP_imgUV);
	
	
	/* free multiple ref frame buffers*/
	/* number of reference frames increased by one for next P-frame*/
	for(j=0;j<=img->buf_cycle;j++)
	{
		for(i=0;i<img->height*4;i++)
		{
			free(mref[j][i]);
		}
		free(mref[j]);
		
		for(i=0;i<img->width_cr*2;i++)
		{
			free(mcef[j][0][i]);
			free(mcef[j][1][i]);
		}
		free(mcef[j][0]);
		free(mcef[j][1]);
		free(mcef[j]);
	} 
	free(mref);
	free(mcef);
	for(i=0;i<img->height*4;i++)
	{
		free(mref_P[i]);
	}
	free(mref_P);
	
	for(i=0;i<img->width_cr*2;i++)
	{
		free(mcef_P[0][i]);
		free(mcef_P[1][i]);
	}
	free(mcef_P[0]);
	free(mcef_P[1]);
	free(mcef_P);
	
	if(input->successive_Bframe!=0)
	{
    /* free last P-frame buffers for B-frame coding*/
    for(j=0;i<2;j++)
    {
      for(i=0;i<img->height/BLOCK_SIZE;i++)
      {
        free(tmp_fwMV[j][i]);
				free(tmp_bwMV[j][i]);
				free(dfMV[j][i]);
				free(dbMV[j][i]);
      }
      free(tmp_fwMV[j]);
      free(tmp_bwMV[j]);
      free(dfMV[j]);
      free(dbMV[j]);
    }
    free(tmp_fwMV);
    free(tmp_bwMV);
    free(dfMV);
    free(dbMV);
    
		
    for(i=0;i<img->height/BLOCK_SIZE;i++)
    {
      free(fw_refFrArr[i]);
      free(bw_refFrArr[i]);
    }
    free(fw_refFrArr);
    free(bw_refFrArr);
	} /* end if B frame*/
	
	
	for(j=0;i<2;j++)
	{
		for(i=0;i<img->height/BLOCK_SIZE;i++)
		{
			free(tmp_mv[j][i]);
		}
		free(tmp_mv[j]);
	}
	free(tmp_mv);
	
	/* free temp quarter pel frame buffer*/
	for(i=0;i<img->height*2;i++)
	{
		free(img4Y_tmp[i]);
	}
	free(img4Y_tmp);
	
	
	/* free temp 1/8 pel frame buffer*/
	for(i=0;i<4*img->width;i++)
	{
		free(img8Y_tmp[i]);
	}
	free(img8Y_tmp);
	
	
	/* free temp loop filter frame buffer*/
	for(i=0;i<img->height;i++)
	{
		free(imgY_tmp[i]);
	}
	free(imgY_tmp);
	
	for(i=0;i<img->height_cr;i++)
	{
		free(imgUV_tmp[0][i]);
		free(imgUV_tmp[1][i]);
	}
	free(imgUV_tmp[0]);
	free(imgUV_tmp[1]);
	free(imgUV_tmp);
	
	/* free ref frame buffer for blocks*/
	for(i=0;i<img->height/BLOCK_SIZE;i++)
	{
		free(refFrArr[i]);
	}
	free(refFrArr);
	
	/* free loop filter strength buffer for 4x4 blocks*/
	for(i=0;i<img->width/BLOCK_SIZE+3;i++)
	{
		free(loopb[i]);
	}
	free(loopb);
	
	for(i=0;i<img->width_cr/BLOCK_SIZE+3;i++)
	{
		free(loopc[i]);
	}
	free(loopc);
	
	/* free mem, allocated in init_img()*/
	/* free intra pred mode buffer for blocks*/
	for(i=0;i<img->width/BLOCK_SIZE+2;i++)
	{
		free(img->ipredmode[i]);
	}
	free(img->ipredmode);
	
	free(img->mb_data); 
	
	free(img->slice_numbers);
}

/************************************************************************
*
*  Name :       int get_mem2D(byte ***array2D, int rows, int columns)
*
*  Description: Allocate 2D memory array -> unsigned char array2D[rows][columns]  
*
*  Input      : 
*
*  Output     : memory size in bytes
*
************************************************************************/
int get_mem2D(byte ***array2D, int rows, int columns)
{
  int i;
  
  if((*array2D = (byte**)calloc(rows,sizeof(byte*))) == NULL) no_mem_exit(1);
  for(i=0;i<rows;i++)
  {
    if(((*array2D)[i] = (byte*)calloc(columns,sizeof(byte))) == NULL) no_mem_exit(1);
  }
  return rows*columns;
}



int get_mem_mv (int****** mv)
{
  int i, j, k, l;

  if ((*mv = (int*****)calloc(4,sizeof(int****))) == NULL) no_mem_exit (1);
  for (i=0; i<4; i++)
    {
      if (((*mv)[i] = (int****)calloc(4,sizeof(int***))) == NULL) no_mem_exit (1);
      for (j=0; j<4; j++)
	{
	  if (((*mv)[i][j] = (int***)calloc(img->buf_cycle,sizeof(int**))) == NULL) no_mem_exit (1);
	  for (k=0; k<img->buf_cycle; k++)
	    {
	      if (((*mv)[i][j][k] = (int**)calloc(9,sizeof(int*))) == NULL) no_mem_exit (1);
	      for (l=0; l<9; l++)
		if (((*mv)[i][j][k][l] = (int*)calloc(2,sizeof(int))) == NULL) no_mem_exit (1);
	    }
	}
    }
  return 4*4*img->buf_cycle*9*2*sizeof(int);
}

void free_mem_mv (int***** mv)
{
  int i, j, k, l;

  for (i=0; i<4; i++)
    {
      for (j=0; j<4; j++)
	{
	  for (k=0; k<img->buf_cycle; k++)
	    {
	      for (l=0; l<9; l++)
		free (mv[i][j][k][l]);
	      free (mv[i][j][k]);
	    }
	  free (mv[i][j]);
	}
      free (mv[i]);
    }
  free (mv);
}

/************************************************************************
*
*  Name :       int get_mem2Dint(int ***array2D, int rows, int columns)
*
*  Description: Allocate 2D memory array -> int array2D[rows][columns]  
*
*  Input      : 
*
*  Output     : memory size in bytes
*
************************************************************************/
int get_mem2Dint(int ***array2D, int rows, int columns)
{
  int i;
  
  if((*array2D = (int**)calloc(rows,sizeof(int*))) == NULL) no_mem_exit(1);
  for(i=0;i<rows;i++)
  {
    if(((*array2D)[i] = (int*)calloc(columns,sizeof(int))) == NULL) no_mem_exit(1);
  }
  return rows*columns*sizeof(int);
}

/************************************************************************
*
*  Name :       int get_mem3D(byte ****array3D, int frames, int rows, int columns)
*
*  Description: Allocate 3D memory array -> unsigned char array3D[frames][rows][columns]  
*
*  Input      : 
*
*  Output     : memory size in bytes
*
************************************************************************/
int get_mem3D(byte ****array3D, int frames, int rows, int columns)
{
  int i, j;
	
  if(((*array3D) = (byte***)calloc(frames,sizeof(byte**))) == NULL) no_mem_exit(1);
	
  for(j=0;j<frames;j++)
  {
    if(((*array3D)[j] = (byte**)calloc(rows,sizeof(byte*))) == NULL) no_mem_exit(1);
    for(i=0;i<rows;i++)
    {
      if(((*array3D)[j][i] = (byte*)calloc(columns,sizeof(byte))) == NULL) no_mem_exit(1);
    }
  }
  
  return frames*rows*columns;
}
/************************************************************************
*
*  Name :       int get_mem3Dint(int ****array3D, int frames, int rows, int columns)
*
*  Description: Allocate 3D memory array -> int array3D[frames][rows][columns]  
*
*  Input      : 
*
*  Output     : memory size in bytes
*
************************************************************************/
int get_mem3Dint(int ****array3D, int frames, int rows, int columns)
{
  int i, j;
	
  if(((*array3D) = (int***)calloc(frames,sizeof(int**))) == NULL) no_mem_exit(1);
	
  for(j=0;j<frames;j++)
  {
    if(((*array3D)[j] = (int**)calloc(rows,sizeof(int*))) == NULL) no_mem_exit(1);
    for(i=0;i<rows;i++)
    {
      if(((*array3D)[j][i] = (int*)calloc(columns,sizeof(int))) == NULL) no_mem_exit(1);
    }
  }
  
  return frames*rows*columns*sizeof(int);
}
/************************************************************************
*
*  Name :       void no_mem_exit(int code)
*
*  Description: Exit program if there is not enough memory for frame buffers 
*
*  Input      : code for exit()
*
*  Output     : none
*
************************************************************************/
void no_mem_exit(int code)
{
   printf("Could not allocate frame buffer memory!\n");
   exit (code);
}

