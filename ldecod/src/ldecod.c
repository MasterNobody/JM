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
 *			Stephan Wenger			<stewe@cs.tu-berlin.de>
 *			Jani Lainema			<jani.lainema@nokia.com>
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
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

#define TML			"5"
#define VERSION		"5.2"
#define LOGFILE		"log.dec"
#define DATADECFILE	"data.dec"
#define TRACEFILE	"trace.dec"

/*! 
 *	\fn		main()
 *	\brief	main function for TML decoder
 */

int main(
	int argc,		
	char **argv)
{
	FILE *fd;                       /* pointer to input configuration file */
	FILE *p_in;                     /* pointer to input H.26L bitstream file */
	FILE *p_out;                    /* pointer to output YUV file */
	FILE *p_log;                    /* pointer to output log file */
	FILE *p_ref;                    /* pointer to input original reference YUV file file */

	char config_filename[100];      /* configuration filename */
	char string[255];

	time_t ltime1;                  /* for time measurement */
	time_t ltime2;

#ifdef WIN32
	struct _timeb tstruct1;
	struct _timeb tstruct2;
	char timebuf[128];
#else
	struct tm *l_time;
	struct timeb tstruct1;
	struct timeb tstruct2;
	time_t now;
#endif

	int tot_time=0;                 /* time for total encoding session */
	int tmp_time;                   /* time used by decoding the last frame */

	struct inp_par    *inp;         /* input parameters from input configuration file */
	struct snr_par    *snr;
	struct img_par    *img;

	int NAL_mode;
	int mb_nr;

	/* allocate memory for the structures */
	inp =  (struct inp_par *)calloc(1, sizeof(struct inp_par));
	snr =  (struct snr_par *)calloc(1, sizeof(struct snr_par));
	img =  (struct img_par *)calloc(1, sizeof(struct img_par));

	if (argc != 2)
	{
		fprintf(stdout,"Usage: %s <config.dat> \n",argv[0]);
		fprintf(stdout,"\t<config.dat> defines encoder parameters\n");
		exit(-1);
	}
	strcpy(config_filename,argv[1]);
	if((fd=fopen(config_filename,"r")) == NULL)           /* read the decoder configuration file */
	{
		fprintf(stdout,"Error: Control file %s not found\n",config_filename);
		exit(0);
	}

	fscanf(fd,"%s",inp->infile);                /* YUV 4:2:2 input format */
	fscanf(fd,"%*[^\n]");

	fscanf(fd,"%s",inp->outfile);               /* H.26L compressed output bitsream */
	fscanf(fd,"%*[^\n]");

	fscanf(fd,"%s",inp->reffile);               /* reference file */
	fscanf(fd,"%*[^\n]");

	fscanf(fd,"%d",&inp->postfilter);           /* not used */
	fscanf(fd,"%*[^\n]");

	fscanf(fd,"%d",&(NAL_mode));                /* NAL mode */
	fscanf(fd,"%*[^\n]");

	/*!
	 *	\note
	 *	NAL_mode=0 defines the conventional H.26L bitstream
	 *	all other NAL_modes are defined by assignSE2partition[][] in 
	 *	elements.h
	 */
	if (NAL_mode > 0)
	{
		PartitionMode = NAL_mode-1;
		if (PartitionMode >= MAXPARTITIONMODES) 
		{
			printf("NAL mode %i is not supported\n", NAL_mode);
			exit(1);
		}
		NAL_mode = 1;
	}

	switch(NAL_mode)
	{
	case 0:
		nal_init = bits_tml_init;
		nal_finit = bits_tml_init;
		nal_get_symbol = bits_tml_get_symbol;
		nal_startcode_follows = bits_tml_startcode_follows;
		nal_find_startcode = bits_tml_find_startcode;
		nal_symbols_available = bits_tml_symbols_available;
		break;
	case 1:
		nal_init = part_tml_init;
		nal_finit = part_tml_init;
		nal_get_symbol = part_tml_get_symbol;
		nal_startcode_follows = part_tml_startcode_follows;
		nal_find_startcode = part_tml_find_startcode;
		nal_symbols_available = part_tml_symbols_available;
		break;
	default:
		printf("NAL mode %i is not supported\n",NAL_mode);
		exit(1);
	}

	nal_init();

	if ((p_in=fopen(inp->infile,"rb"))==0)
	{
		fprintf(stdout,"Input file %s does not exist \n",inp->infile);
		exit(0);
	}
	if ((p_out=fopen(inp->outfile,"wb"))==0)
	{
		fprintf(stdout,"Error open file %s  \n",inp->outfile);
		exit(0);
	}

#if TRACE
	sprintf(string,"%s",TRACEFILE);
	if ((p_trace=fopen(string,"w"))==0)            /* append new statistic at the end */
	{
		printf("Error open file %s!\n",string);
		exit(0);
	}
#endif

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


	fprintf(stdout,"Frame  TR    QP   SnrY    SnrU    SnrV   Time(ms)\n");

	init(img);
	img->number=0;
	reset_ec_flags();  /*<pu> reset all error concealment flags */
	
	while (nal_find_startcode(p_in, &img->tr, &img->qp, &mb_nr, &img->format) != 0)                     /* while not end of seqence, read next image to buffer */
	{
		int return_value;

		if (ec_flag[SE_HEADER] == NO_EC) 
		{
			if(mb_nr == 0)
			{
				/* Picture start code */
				img->current_slice_nr = 0;
			}
			else
			{
				/* Slice start code */
				img->current_slice_nr++;
			}

#ifdef WIN32
			_ftime (&tstruct1);				/* start time ms */
#else
			ftime (&tstruct1);				/* start time ms */
#endif
			time( &ltime1 );                                  /* start time s  */

			return_value = decode_slice(img,inp,mb_nr);
			switch(return_value)
			{
			case SEARCH_SYNC:
				fprintf(stdout,"Biterror in frame %d\n",img->number);
				break;
			case DECODING_OK:
				break;
			case PICTURE_DECODED:
				if (p_ref)
					find_snr(snr,img,p_ref,inp->postfilter);      /* if ref sequence exist */

#ifdef WIN32
				_ftime (&tstruct2);		/* end time ms  */
#else
				ftime (&tstruct2);		/* end time ms  */
#endif
				time( &ltime2 );                                /* end time sec */			
				tmp_time=(ltime2*1000+tstruct2.millitm) - (ltime1*1000+tstruct1.millitm);
				tot_time=tot_time + tmp_time;

				fprintf(stdout,"%2d %5d %5d %7.4f %7.4f %7.4f %5d\n",
						img->number,img->tr,img->qp,snr->snr_y,snr->snr_u,snr->snr_v,
						tmp_time);

				write_frame(img,inp->postfilter,p_out);         /* write image to output YUV file */
				img->number++;                                  /* frame counter */
				break;
			}
		}
		reset_ec_flags();  /*<pu> reset all error concealment flags */
	}

	fprintf(stdout,"-------------------- Average SNR all frames ------------------------------\n");
	fprintf(stdout," SNR Y(dB)     : %5.2f\n",snr->snr_ya);
	fprintf(stdout," SNR U(dB)     : %5.2f\n",snr->snr_ua);
	fprintf(stdout," SNR V(dB)     : %5.2f\n",snr->snr_va);
	fprintf(stdout,"--------------------------------------------------------------------------\n");
	fprintf(stdout," Exit TML %s decoder, ver %s \n",TML,VERSION);

	/* write to log file */

	sprintf(string, "%s", LOGFILE);
	if (fopen(string,"r")==0)                    /* check if file exist */
	{
		if ((p_log=fopen(string,"a"))==0)
		{
			fprintf(stdout,"Error open file %s for appending\n",p_log);
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
		p_log=fopen("log_dec.dat","a");                    /* File exist,just open for appending */

#ifdef WIN32
	_strdate( timebuf );
	fprintf(p_log,"| %1.5s |",timebuf );

	_strtime( timebuf);
	fprintf(p_log," % 1.5s |",timebuf);
#else
	now = time ((time_t *) NULL);	/* Get the system time and put it into 'now' as 'calender time' */
	time (&now);
	l_time = localtime (&now);
	strftime (string, sizeof string, "%d-%b-%y", l_time);
	fprintf(p_log,"| %1.5s |",string );

	strftime (string, sizeof string, "%H:%M:%S", l_time);
	fprintf(p_log,"| %1.5s |",string );
#endif

	fprintf(p_log,"%20.20s|",inp->infile);

	fprintf(p_log,"%3d |",img->number);

	if(img->format==QCIF)
		fprintf(p_log," QCIF |");
	else
		fprintf(p_log," CIF  |");

	fprintf(p_log,"%6.3f|",snr->snr_y1);
	fprintf(p_log,"%6.3f|",snr->snr_u1);
	fprintf(p_log,"%6.3f|",snr->snr_v1);
	fprintf(p_log,"%6.3f|",snr->snr_ya);
	fprintf(p_log,"%6.3f|",snr->snr_ua);
	fprintf(p_log,"%6.3f|\n",snr->snr_va);

	fclose(p_log);

	sprintf(string,"%s", DATADECFILE);
	p_log=fopen(string,"a");

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
	        (double)0.001*tot_time/(img->number));

	fclose(p_log);

	//  getchar();
	exit(0);
}


/*!
 *	\fn		init()
 *	\brief	Initilize some arrays
 */
void init(
	struct img_par *img)	/*!< image parameters */
{
	int i;
	/* initilize quad matrix used in snr routine */

	for (i=0; i <  256; i++)
	{
		img->quad[i]=i*i; /* fix from TML 1, truncation removed */
	}
}


