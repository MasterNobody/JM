/*!
 *	\file
 *			Ldecod.c	
 *	\brief
 *			TML decoder project main()
 *	\author
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *
 *			Inge Lille-Langøy       <inge.lille-langoy@telenor.com>
 *			Rickard Sjoberg         <rickard.sjoberg@era.ericsson.se>
 *			Stephan Wenger		    <stewe@cs.tu-berlin.de>
 *			Jani Lainema		    <jani.lainema@nokia.com>
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 *			Byeong-Moon Jeon		<jeonbm@lge.com>
 *      Gabi Blaettermann               <blaetter@hhi.de>
 *
 *	\note	tags are used for document system "doxygen"
 *			available at http://www.stack.nl/~dimitri/doxygen/index.html
 *
 *		
 * 
 * 	\note
 *		-Limitations:
 *			Using different NAL's the assignment of partition-id to containing
 *			syntax elements may got lost, if this information is not transmitted.
 *			The same has to be stated for the partitionlength if partitions are
 *			merged by the NAL. \par
 *		
 *			The presented solution in Q15-K-16 solves both of this problems as the
 *			departitioner parses the bitstream before decoding. Due to syntax element 
 *			dependencies both, partition bounds and partitionlength information can
 *			be parsed by the departitioner. \par
 *		
 *		-Handling partition information in external file:
 *			As the TML is still a work in progress, it makes sense to handle this 
 *			information for simplification in an external file, here called partition 
 *			information file, which can be found by the extension .dp extending the 
 *			original encoded H.26L bitstream. In this file partition-ids followed by its
 *			partitionlength is written. Instead of parsing the bitstream we get the 
 *			partition information now out of this file. 
 *			This information is assumed to be never sent over transmission channels 
 *			(simulation scenarios) as it's information we allways get using a 
 *			"real" departitioner before decoding \par
 *
 *		-Extension of Interim File Format:
 *			Therefore a convention has to be made within the interim file format.
 *			The underlying NAL has to take care of fulfilling these conventions.
 *			All partitions have to be bytealigned to be readable by the decoder, 
 *			So if the NAL-encoder merges partitions, >>this is only possible to use the 
 *			VLC	structure of the H.26L bitstream<<, this bitaligned structure has to be 
 *			broken up by the NAL-decoder. In this case the NAL-decoder is responsable to 
 *			read the partitionlength information from the partition information file.
 *			Partitionlosses are signaled with a partition of zero length containing no
 *			syntax elements.
 *
 *	
 */

#include "contributors.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include "global.h"
#include "elements.h"
#include "bitsbuf.h"

#define TML			"6"
#define VERSION		"6.50"
#define LOGFILE		"log.dec"
#define DATADECFILE	"data.dec"
#define TRACEFILE	"trace_dec.txt"

/*! 
 *	\fn		main()
 *	\brief	main function for TML decoder
 */

int main(int argc, char **argv)
{
    struct inp_par    *inp;         /* input parameters from input configuration file   */
	struct snr_par    *snr;         /* statistics                                       */
	struct img_par    *img;         /* image parameters                                 */

    /* allocate memory for the structures */
	inp =  (struct inp_par *)calloc(1, sizeof(struct inp_par));
	snr =  (struct snr_par *)calloc(1, sizeof(struct snr_par));
	img =  (struct img_par *)calloc(1, sizeof(struct img_par));

	/* Read Configuration File */
    if (argc != 2)
	{
		fprintf(stdout,"Usage: %s <config.dat> \n",argv[0]);
		fprintf(stdout,"\t<config.dat> defines decoder parameters\n");
		exit(-1);
	}
    
    /* Initializes Configuration Parameters with configuration file */
	init_conf(inp, argv[1]);
	
    /* Allocate Slice data struct */
	malloc_slice(inp,img);

	init(img);
	img->number=0;

	/* B pictures */
	Bframe_ctr=0;

    /* time for total decoding session */
    tot_time = 0;
   
    while (decode_one_frame(img, inp, snr) != EOS);
    
    // B PICTURE : save the last P picture
	write_prev_Pframe(img, p_out);

    report(inp, img, snr);

    free_slice(inp,img);
    free_mem4global_buffers(inp, img);

	CloseBitstreamFile();

    fclose(p_out);
    fclose(p_ref);
#if TRACE
    fclose(p_trace);
#endif
	return 0;
}


/*!
 *	\fn		init()
 *	\brief	Initilize some arrays
 */
void init(struct img_par *img)	/*!< image parameters */
{
	int i;
	/* initilize quad matrix used in snr routine */

	for (i=0; i <  256; i++)
	{
		img->quad[i]=i*i; /* fix from TML 1, truncation removed */
	}
}

/************************************************************************
*
*  Name :       init_conf(struct inp_par *inp, char *config_filename)
*
*  Description: Read input from configuration file
*
*  Input      : Name of configuration filename
*
*  Output     : none
*
************************************************************************/

void init_conf(struct inp_par *inp,
               char *config_filename)
{
	FILE *fd;
    int NAL_mode;
    //char string[255];

    /* read the decoder configuration file */
	if((fd=fopen(config_filename,"r")) == NULL)           
	{
		fprintf(stdout,"Error: Control file %s not found\n",config_filename);
		exit(0);
	}

	fscanf(fd,"%s",inp->infile);                /* H.26L compressed input bitsream */ 
	fscanf(fd,"%*[^\n]");

	fscanf(fd,"%s",inp->outfile);               /* YUV 4:2:2 input format */
	fscanf(fd,"%*[^\n]");

	fscanf(fd,"%s",inp->reffile);               /* reference file */
	fscanf(fd,"%*[^\n]");

    /* Symbol mode */
	fscanf(fd,"%d,",&inp->symbol_mode);        /* 0: UVLC 1: CABAC */
	fscanf(fd,"%*[^\n]");
	if (inp->symbol_mode != UVLC && inp->symbol_mode != CABAC)
	{
		sprintf(errortext, "Unsupported symbol mode=%d, use UVLC=0 or CABAC=1\n",inp->symbol_mode);
		error(errortext);
	}

	/* Frame buffer size */
	fscanf(fd,"%d,",&inp->buf_cycle);
	fscanf(fd,"%*[^\n]");
	if (inp->buf_cycle < 1)
	{
		sprintf(errortext, "Frame Buffer Size is %d. It has to be at least 1\n",inp->buf_cycle);
		error(errortext);
	}

	fscanf(fd,"%d",&(NAL_mode));                /* NAL mode */
    fscanf(fd,"%*[^\n]");

    switch(NAL_mode)
	{
	case 0:
		inp->of_mode = PAR_OF_26L;
		/* Note: Data Partitioning in 26L File Format not yet supported */
		inp->partition_mode = PAR_DP_1;
		break;
	default:
		printf("NAL mode %i is not supported\n", NAL_mode);
		exit(1);
	}


#if TRACE
	sprintf(string,"%s",TRACEFILE);
	if ((p_trace=fopen(string,"w"))==0)             /* append new statistic at the end */
	{
		printf("Error open file %s!\n",string);
		exit(0);
	}
#endif
    	
    
//    if ((p_in=fopen(inp->infile,"rb"))==0)
//	{
//		fprintf(stdout,"Input file %s does not exist \n",inp->infile);
//		exit(0);
//	}
	
	if (OpenBitstreamFile (inp->infile) < 0) {
			printf ("Cannot open bitstream file '%s'\n", inp->infile);
			exit (-1);
	}
	if ((p_out=fopen(inp->outfile,"wb"))==0)
	{
		fprintf(stdout,"Error open file %s  \n",inp->outfile);
		exit(0);
	}

    fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout," Decoder config file                    : %s \n",config_filename);
	fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout," Input H.26L bitstream                  : %s \n",inp->infile);
	fprintf(stdout," Output decoded YUV 4:2:0               : %s \n",inp->outfile);
	fprintf(stdout," Output status file                     : %s \n",LOGFILE);
	if ((p_ref=fopen(inp->reffile,"rb"))==0)
	{
		fprintf(stdout," Input reference file                   : %s does not exist \n",inp->reffile);
		fprintf(stdout,"                                          SNR values are not available\n");
	}
	else
		fprintf(stdout," Input reference file                   : %s \n",inp->reffile);
	fprintf(stdout,"--------------------------------------------------------------------------\n");


	fprintf(stdout,"Frame   TR    QP  SnrY    SnrU    SnrV   Time(ms)\n");
}
/************************************************************************
*
*  Name :       void report()
*
*  Description: Reports the gathered information to appropriate outputs
*
*  Input      : struct inp_par *inp, 
*				struct img_par *img, 
*				struct snr_par *stat
*
*  Output     : None
*
************************************************************************/
void report(struct inp_par *inp, struct img_par *img, struct snr_par *snr)
{
    char string[255];
    FILE *p_log;

#ifndef WIN32
    time_t	now;
    struct tm	*l_time;
#else
    char timebuf[128];
#endif

	fprintf(stdout,"-------------------- Average SNR all frames ------------------------------\n");
	fprintf(stdout," SNR Y(dB)           : %5.2f\n",snr->snr_ya);
	fprintf(stdout," SNR U(dB)           : %5.2f\n",snr->snr_ua);
	fprintf(stdout," SNR V(dB)           : %5.2f\n",snr->snr_va);
	fprintf(stdout," Total decoding time : %.3f sec \n",tot_time*0.001); 
	fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout," Exit TML %s decoder, ver %s \n",TML,VERSION);

	/* write to log file */

	sprintf(string, "%s", LOGFILE);
	if (fopen(string,"r")==0)                    /* check if file exist */
	{
		if ((p_log=fopen(string,"a"))==0)
		{
			fprintf(stdout,"Error open file %s for appending\n",string);
			exit(0);
		}
		else                                              /* Create header to new file */
		{
			fprintf(p_log," ------------------------------------------------------------------------------------------\n");
			fprintf(p_log,"|  Decoder statistics. This file is made first time, later runs are appended               |\n");
			fprintf(p_log," ------------------------------------------------------------------------------------------ \n");
			fprintf(p_log,"| Date  | Time  |    Sequence        |#Img|Format|SNRY 1|SNRU 1|SNRV 1|SNRY N|SNRU N|SNRV N|\n");
			fprintf(p_log," ------------------------------------------------------------------------------------------\n");
		}
	}
	else
		p_log=fopen("log.dec","a");                    /* File exist,just open for appending */

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

	fprintf(p_log,"%20.20s|",inp->infile);

	fprintf(p_log,"%3d |",img->number);

	fprintf(p_log,"%6.3f|",snr->snr_y1);
	fprintf(p_log,"%6.3f|",snr->snr_u1);
	fprintf(p_log,"%6.3f|",snr->snr_v1);
	fprintf(p_log,"%6.3f|",snr->snr_ya);
	fprintf(p_log,"%6.3f|",snr->snr_ua);
	fprintf(p_log,"%6.3f|\n",snr->snr_va);

	fclose(p_log);

	sprintf(string,"%s", DATADECFILE);
	p_log=fopen(string,"a");

	if(Bframe_ctr != 0) // B picture used
	{
		fprintf(p_log, "%3d %2d %2d %2.2f %2.2f %2.2f %5d "
			"%2.2f %2.2f %2.2f %5d "
			"%2.2f %2.2f %2.2f %5d %.3f\n",
			img->number, 0, img->qp,
			snr->snr_y1,
			snr->snr_u1,
			snr->snr_v1,
			0,
			0.0,
			0.0,
			0.0,
			0,
			snr->snr_ya,
			snr->snr_ua,
			snr->snr_va,
			0,
			(double)0.001*tot_time/(img->number+Bframe_ctr-1));
	}
	else 
	{
		fprintf(p_log, "%3d %2d %2d %2.2f %2.2f %2.2f %5d "
			"%2.2f %2.2f %2.2f %5d "
			"%2.2f %2.2f %2.2f %5d %.3f\n",
			img->number, 0, img->qp,
			snr->snr_y1,
			snr->snr_u1,
			snr->snr_v1,
			0,
			0.0,
			0.0,
			0.0,
			0,
			snr->snr_ya,
			snr->snr_ua,
			snr->snr_va,
			0,
			(double)0.001*tot_time/img->number);
    }	
    fclose(p_log);
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
void malloc_slice(struct inp_par *inp, struct img_par *img)
{
	DataPartition *dataPart;
	Slice *currSlice;
    const int buffer_size = MAX_CODED_FRAME_SIZE; /* picture size unknown at this time, this value is to check */

	switch(inp->of_mode) /* init depending on NAL mode */
	{
		case PAR_OF_26L:
			/* Current File Format */
			img->currentSlice = (Slice *) calloc(1, sizeof(Slice));
			if ( (currSlice = img->currentSlice) == NULL)
			{
				fprintf(stderr, "Memory allocation for Slice datastruct in NAL-mode %d failed", inp->of_mode);
				exit(1);
			}
			if (inp->symbol_mode == CABAC)
			{
				/* create all context models */
				currSlice->mot_ctx = create_contexts_MotionInfo();
				currSlice->tex_ctx = create_contexts_TextureInfo();
			}

			switch(inp->partition_mode)
			{
			case PAR_DP_1:
				currSlice->max_part_nr = 1;
				break;
			default:
				error("Data Partitioning Mode not supported!");
				break;
			}

			currSlice->partArr = (DataPartition *) calloc(1, sizeof(DataPartition));
			if (currSlice->partArr == NULL)
			{
				fprintf(stderr, "Memory allocation for Data Partition datastruct in NAL-mode %d failed", inp->of_mode);
				exit(1);
			}			
			dataPart = currSlice->partArr;	
			dataPart->bitstream = (Bitstream *) calloc(1, sizeof(Bitstream));
			if (dataPart->bitstream == NULL)
			{
				fprintf(stderr, "Memory allocation for Bitstream datastruct in NAL-mode %d failed", inp->of_mode);
				exit(1);
			}
			dataPart->bitstream->streamBuffer = (byte *) calloc(buffer_size, sizeof(byte));
			if (dataPart->bitstream->streamBuffer == NULL)
			{
				fprintf(stderr, "Memory allocation for bitstream buffer in NAL-mode %d failed", inp->of_mode);
				exit(1);
			}
			return;
		default: 
			fprintf(stderr, "Output File Mode %d not supported", inp->of_mode);
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
void free_slice(struct inp_par *inp, struct img_par *img)
{
	DataPartition *dataPart;
	Slice *currSlice = img->currentSlice;

	switch(inp->of_mode) /* init depending on NAL mode */
	{
		case PAR_OF_26L:
			/* Current File Format */
			dataPart = currSlice->partArr;	/* only one active data partition */
			if (dataPart->bitstream->streamBuffer != NULL)
				free(dataPart->bitstream->streamBuffer);
			if (dataPart->bitstream != NULL)
				free(dataPart->bitstream);
			if (currSlice->partArr != NULL)
				free(currSlice->partArr);
			if (inp->symbol_mode == CABAC)
			{
				/* delete all context models */
				delete_contexts_MotionInfo(currSlice->mot_ctx);
				delete_contexts_TextureInfo(currSlice->tex_ctx);
			}
			if (currSlice != NULL)
				free(img->currentSlice);
			break;
		default: 
			fprintf(stderr, "Output File Mode %d not supported", inp->of_mode);
			exit(1);
	}				
	
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
int get_mem4global_buffers(struct inp_par *inp, struct img_par *img)
{
	int j,memory_size=0;
#ifdef _ADAPT_LAST_GROUP_
	extern int *last_P_no;
#endif

	img->buf_cycle = inp->buf_cycle+1;

#ifdef _ADAPT_LAST_GROUP_
	if ((last_P_no = (int*)malloc(img->buf_cycle*sizeof(int))) == NULL) no_mem_exit(1);
#endif
	
	/* allocate memory for encoding frame buffers: imgY, imgUV*/ 
	/* byte imgY[288][352]; */
	/* byte imgUV[2][144][176]; */
	memory_size += get_mem2D(&imgY, img->height, img->width);
	memory_size += get_mem3D(&imgUV, 2, img->height_cr, img->width_cr);
	
	/* allocate memory for multiple ref. frame buffers: mref, mcref*/ 
	/* rows and cols for croma component mcef[ref][croma][4x][4y] are switched */
	/* compared to luma mref[ref][4y][4x] for whatever reason */
	/* number of reference frames increased by one for next P-frame*/
	memory_size += get_mem3D(&mref, img->buf_cycle+1, img->height, img->width);
	
	if((mcef = (byte****)calloc(img->buf_cycle+1,sizeof(byte***))) == NULL) no_mem_exit(1);
	for(j=0;j<img->buf_cycle+1;j++)
        memory_size += get_mem3D(&(mcef[j]), 2, img->height_cr*2, img->width_cr*2);
		
	/* allocate memory for B-frame coding (last upsampled P-frame): mref_P, mcref_P*/ 
	/*byte mref[1152][1408];  */   /* 1/4 pix luma  */
	/*byte mcef[2][352][288]; */   /* pix chroma    */
/*	memory_size += get_mem2D(&mref_P, img->height*4, img->width*4);
	memory_size += get_mem3D(&mcef_P, 2, img->height_cr*2, img->width_cr*2);
*/	
	/* allocate memory for mref_P_small B pictures */
	/* byte mref_P_small[288][352];       */
/*	memory_size += get_mem2D(&mref_P_small, img->height, img->width);
*/
    /* allocate memory for imgY_prev */
	/* byte imgY_prev[288][352];       */
	/* byte imgUV_prev[2][144][176];       */
	memory_size += get_mem2D(&imgY_prev, img->height, img->width);
    memory_size += get_mem3D(&imgUV_prev, 2, img->height_cr, img->width_cr);
	
	/* allocate memory for reference frames of each block: refFrArr*/ 
	/* int  refFrArr[72][88];  */
	memory_size += get_mem2Dint(&refFrArr, img->height/BLOCK_SIZE, img->width/BLOCK_SIZE);
	
	/* allocate memory for filter strength for 4x4 lums and croma blocks:loopb, loopc*/ 
	/*byte loopb[92][76]; i.e.[x][y]*/
	/*byte loopc[48][40]; */
	memory_size += get_mem2D(&loopb, img->width/BLOCK_SIZE+4, img->height/BLOCK_SIZE+4);
	memory_size += get_mem2D(&loopc, img->width_cr/BLOCK_SIZE+4, img->height_cr/BLOCK_SIZE+4);

	/* allocate memory for reference frame in find_snr */ 
    //byte imgY_ref[288][352];
	//byte imgUV_ref[2][144][176];
    memory_size += get_mem2D(&imgY_ref, img->height, img->width);
    memory_size += get_mem3D(&imgUV_ref, 2, img->height_cr, img->width_cr);

    /* allocate memory for loop_filter */ 
    // byte imgY_tmp[288][352];                      /* temp luma image */
    // byte imgUV_tmp[2][144][176];                  /* temp chroma image */
    memory_size += get_mem2D(&imgY_tmp, img->height, img->width);
    memory_size += get_mem3D(&imgUV_tmp, 2, img->height_cr, img->width_cr);

    /* allocate memory in structure img */ 
    if(((img->mb_data) = (Macroblock *) calloc((img->width/MB_BLOCK_SIZE) * (img->height/MB_BLOCK_SIZE),sizeof(Macroblock))) == NULL) 
	    no_mem_exit(1);
    if(((img->slice_numbers) = (int *) calloc((img->width/MB_BLOCK_SIZE) * (img->height/MB_BLOCK_SIZE),sizeof(int))) == NULL) 
	    no_mem_exit(1);
    // img => int mv[92][72][3]
    memory_size += get_mem3Dint(&(img->mv),img->width/BLOCK_SIZE +4, img->height/BLOCK_SIZE,3);
    // img => int ipredmode[90][74]
    memory_size += get_mem2Dint(&(img->ipredmode),img->width/BLOCK_SIZE +2 , img->height/BLOCK_SIZE +2);
    // int dfMV[92][72][3]; 
    memory_size += get_mem3Dint(&(img->dfMV),img->width/BLOCK_SIZE +4, img->height/BLOCK_SIZE,3);
    // int dbMV[92][72][3];
    memory_size += get_mem3Dint(&(img->dbMV),img->width/BLOCK_SIZE +4, img->height/BLOCK_SIZE,3);
    // int fw_refFrArr[72][88];
    memory_size += get_mem2Dint(&(img->fw_refFrArr),img->height/BLOCK_SIZE,img->width/BLOCK_SIZE);
    // int bw_refFrArr[72][88];
    memory_size += get_mem2Dint(&(img->bw_refFrArr),img->height/BLOCK_SIZE,img->width/BLOCK_SIZE);
    // int fw_mv[92][72][3];
    memory_size += get_mem3Dint(&(img->fw_mv),img->width/BLOCK_SIZE +4, img->height/BLOCK_SIZE,3);
    // int bw_mv[92][72][3];
    memory_size += get_mem3Dint(&(img->bw_mv),img->width/BLOCK_SIZE +4, img->height/BLOCK_SIZE,3);
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
void free_mem4global_buffers(struct inp_par *inp, struct img_par *img) 
{
	int i,j;
	
    /* imgY, imgUV */
	for(i=0;i<img->height;i++)
	{
		if(imgY[i] != NULL) free(imgY[i]);
	}
	if(imgY != NULL) free(imgY);
	
	for(i=0;i<img->height_cr;i++)
	{
		if(imgUV[0][i] != NULL) free(imgUV[0][i]);
		if(imgUV[1][i] != NULL) free(imgUV[1][i]);
	}
	if(imgUV[0] != NULL) free(imgUV[0]);
	if(imgUV[1] != NULL) free(imgUV[1]);
	if(imgUV != NULL) free(imgUV);

    /* imgY_prev, imgUV_prev */
	for(i=0;i<img->height;i++)
	{
		if(imgY_prev[i] != NULL) free(imgY_prev[i]);
	}
	if(imgY_prev != NULL) free(imgY_prev);
	
	for(i=0;i<img->height_cr;i++)
	{
		if(imgUV_prev[0][i] != NULL) free(imgUV_prev[0][i]);
		if(imgUV_prev[1][i] != NULL) free(imgUV_prev[1][i]);
	}
	if(imgUV_prev[0] != NULL) free(imgUV_prev[0]);
	if(imgUV_prev[1] != NULL) free(imgUV_prev[1]);
	if(imgUV_prev != NULL) free(imgUV_prev);
	
    /* mref_P_small */
/*	for(i=0;i<img->height;i++)  StW
	{
        if(mref_P_small[i] != NULL) free(mref_P_small[i]);
	}
	if(mref_P_small[i] != NULL) free(mref_P_small);
*/ 	
	/* free multiple ref frame buffers*/
	/* number of reference frames increased by one for next P-frame*/
	for(j=0;j<=img->buf_cycle;j++)
	{
		for(i=0;i<img->height;i++)
		{
			if(mref[j][i] != NULL) free(mref[j][i]);
		}
		if(mref[j] != NULL) free(mref[j]);
		
		for(i=0;i<img->height_cr*2;i++)
		{
			if(mcef[j][0][i] != NULL) free(mcef[j][0][i]);
			if(mcef[j][1][i] != NULL) free(mcef[j][1][i]);
		}
		if(mcef[j][0] != NULL) free(mcef[j][0]);
		if(mcef[j][1] != NULL) free(mcef[j][1]);
		if(mcef[j] != NULL) free(mcef[j]);
	} 
	if(mref != NULL) free(mref);
	if(mcef != NULL) free(mcef);
	
    /* mref_P, mcef_P */
/*	for(i=0;i<img->height*4;i++)
	{
		if(mref_P[i] != NULL) free(mref_P[i]);
	}
	if(mref_P != NULL)free(mref_P);
	
	for(i=0;i<img->height_cr*2;i++)
	{
		if(mcef_P[0][i] != NULL) free(mcef_P[0][i]);
		if(mcef_P[1][i] != NULL)free(mcef_P[1][i]);
	}
	if(mcef_P[0] != NULL) free(mcef_P[0]);
	if(mcef_P[1] != NULL) free(mcef_P[1]);
	if(mcef_P != NULL)free(mcef_P);
*/
	
	/* free ref frame buffer for blocks*/
	for(i=0;i<img->height/BLOCK_SIZE;i++)
	{
		if(refFrArr[i] != NULL) free(refFrArr[i]);
	}
	if(refFrArr != NULL) free(refFrArr);
	
	/* free loop filter strength buffer for 4x4 blocks*/
	for(i=0;i<img->width/BLOCK_SIZE+3;i++)
	{
		if(loopb[i] != NULL) free(loopb[i]);
	}
	if(loopb != NULL) free(loopb);
	
	for(i=0;i<img->width_cr/BLOCK_SIZE+3;i++)
	{
		if(loopc[i] != NULL) free(loopc[i]);
	}
	if(loopc != NULL) free(loopc);
	
    /* find_snr */
    for(i=0;i<img->height;i++)
	{
		if(imgY_ref[i] != NULL) free(imgY_ref[i]);
	}
	if(imgY_ref != NULL) free(imgY_ref);
	
	for(i=0;i<img->height_cr;i++)
	{
		if(imgUV_ref[0][i] != NULL) free(imgUV_ref[0][i]);
		if(imgUV_ref[1][i] != NULL) free(imgUV_ref[1][i]);
	}
	if(imgUV_ref[0] != NULL) free(imgUV_ref[0]);
	if(imgUV_ref[1] != NULL) free(imgUV_ref[1]);
	if(imgUV_ref != NULL) free(imgUV_ref);

    /* loop_filter */
     for(i=0;i<img->height;i++)
	{
		if(imgY_tmp[i] != NULL) free(imgY_tmp[i]);
	}
	if(imgY_tmp != NULL) free(imgY_tmp);
	
	for(i=0;i<img->height_cr;i++)
	{
		if(imgUV_tmp[0][i] != NULL) free(imgUV_tmp[0][i]);
		if(imgUV_tmp[1][i] != NULL) free(imgUV_tmp[1][i]);
	}
	if(imgUV_tmp[0] != NULL) free(imgUV_tmp[0]);
	if(imgUV_tmp[1] != NULL) free(imgUV_tmp[1]);
	if(imgUV_tmp != NULL) free(imgUV_tmp);

	/* free mem, allocated for structure img */
	if (img->mb_data != NULL) free(img->mb_data); 
    if (img->slice_numbers != NULL) free(img->slice_numbers);
	
    // img => int mv[92][72][3]
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
	    for(j=0;j<(img->height/BLOCK_SIZE);j++)
		    if(img->mv[i][j] != NULL) free(img->mv[i][j]);
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
		if(img->mv[i] != NULL) free(img->mv[i]);
    if(img->mv != NULL) free(img->mv);

    // img => int ipredmode[90][74]
    for(i=0;i<(img->width/BLOCK_SIZE + 2);i++)
		if(img->ipredmode[i] != NULL) free(img->ipredmode[i]);
    if(img->ipredmode != NULL) free(img->ipredmode);
  
    // int dfMV[92][72][3]; 
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
	    for(j=0;j<(img->height/BLOCK_SIZE);j++)
		    if(img->dfMV[i][j] != NULL) free(img->dfMV[i][j]);
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
		if(img->dfMV[i] != NULL) free(img->dfMV[i]);
    if(img->dfMV != NULL) free(img->dfMV);
   
    // int dbMV[92][72][3];
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
	    for(j=0;j<(img->height/BLOCK_SIZE);j++)
		    if(img->dbMV[i][j] != NULL) free(img->dbMV[i][j]);
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
		if(img->dbMV[i] != NULL) free(img->dbMV[i]);
    if(img->dbMV != NULL) free(img->dbMV);
    
    // int fw_refFrArr[72][88];
    for(i=0;i<img->height/BLOCK_SIZE;i++)
		if(img->fw_refFrArr[i] != NULL) free(img->fw_refFrArr[i]);
    if(img->fw_refFrArr != NULL) free(img->fw_refFrArr);

    // int bw_refFrArr[72][88];
    for(i=0;i<img->height/BLOCK_SIZE;i++)
		if(img->bw_refFrArr[i] != NULL) free(img->bw_refFrArr[i]);
    if(img->bw_refFrArr != NULL) free(img->bw_refFrArr);

    // int fw_mv[92][72][3];
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
	    for(j=0;j<(img->height/BLOCK_SIZE);j++)
		    if(img->fw_mv[i][j] != NULL) free(img->fw_mv[i][j]);
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
		if(img->fw_mv[i] != NULL) free(img->fw_mv[i]);
    if(img->fw_mv != NULL) free(img->fw_mv);
  
    // int bw_mv[92][72][3];
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
	    for(j=0;j<(img->height/BLOCK_SIZE);j++)
		    if(img->bw_mv[i][j] != NULL) free(img->bw_mv[i][j]);
    for(i=0;i<(img->width/BLOCK_SIZE + 4);i++)
		if(img->bw_mv[i] != NULL) free(img->bw_mv[i]);
    if(img->bw_mv != NULL) free(img->bw_mv);

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


