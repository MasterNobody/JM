/*! 
 * 	\file 	
 *			NALdec4H223.c
 * 	\brief	
 *			NAL-Decoder for datapartitioned H.26L bitstreams to H.223 Annex B
 *	\date	
 *			22.10.2000
 *	\version
 *			1.0
 * 	\author	
 *			Stephan Wenger			<stewe@cs.tu-berlin.de>
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 *
 *	\note	
 *		-Limitations:
 *			The current function WritePicture() does not consider the different 
 *			logical channels to assign the correct partition-ID. \par
 *
 *		-Differences to Q15-K-16:
 *			As partitions with zero length are not transmitted over the H.223
 *			Mux-layer, the assignment of partition-ID to next partition-ID may
 *			get lost. \par
 *		
 *			Normally a parsing of the video-stream in the NAL would correct 
 *			this lost assignment. 
 *			As the standardization process for H.26L may still change the TML-decoding
 *			structure we implemented another mechanism here differing from 
 *			the proposed data-departitioner with parsing and error concealment of 
 *			document Q15-K-16. \par
 *
 *			The presented solution in Q15-K-16 solves both problems as the
 *			departitioner parses the bitstream before decoding. Due to syntax element 
 *			dependencies both, partition bounds and partitionlength information can
 *			be solved by the departitioner. \par
 *		
 *		-Handling partition information in external file:
 *			As the TML is still a work in progress, it makes sense to handle this 
 *			information for simplification in an external file, here called partition 
 *			information file, which can be found by the extension .dp extending the 
 *			original encoded H.26L bitstream. 
 *			With this solution it is not necessary to use the Q15-K-16 software.
 *			In this file partition-ids followed by its partitionlength is written. 
 *			Instead of parsing the bitstream we get the partition information now out 
 *			of this file. 
 *			This information is assumed to be never sent over transmission channels 
 *			(simulation scenarios) as we receive this information using a 
 *			"real" departitioner before decoding in the Video Coding Layer \par
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
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "NAL4H223.h"
#include "NALdec4H223.h"

#define ERR_IND	1	/* if defined the stream contains no error indication flag */

/*!
 * 	\fn 	main()
 *	\brief	main function
 */
int main (int ac, char *av[])
{
	int i=0;
	
	/*!
	 * mode to assign partitions to different logical channels as 
	 * required for H.223 Annex D 
	 * mode = 0 <=> H.223 Annex B only one logical channel available
	 */		
	int mode = 0; 	
	
	if (ac != 4)
	{
		printf ("Usage: %s <partfileinfo.dp> <infile.pac> <outfile.h26l>\n",av[0]);
		printf ("   partfileinfo.dp = Partition information file\n");
		printf ("   infile.pac      = Packetized H.26L bitstream from H.223 demux\n");    
		printf ("   outfile.h26l    = Data Partitioned H.26L bitstream\n");    
		exit (-1);
	}
	if ((partinf = fopen (av[1], "rb")) == NULL)
	{
		printf ("%s: can't open %s for reading... exit\n",av[0], av[1]);
		exit (-2);
	}
	if ((in = fopen (av[2], "rb")) == NULL)
	{
		printf ("%s: can't open %s for reading... exit\n",av[0], av[1]);
		exit (-3);
	}
	if ((out = fopen (av[3], "wb")) == NULL)
	{
		printf ("%s: can't open %s for writing... exit\n",av[0], av[2]);
		exit (-4);
	}

	while (!finished)
	{
		finished = ReadPicture();
		WritePicture();
	}
	return 0;
}


/*!
 *	\fn		ReadPicture
 *	\brief	Reads all partitions of one slice/frame
 *	\return	status if input file end is reached or not
 *			(0=no EOF; 1=EOF)
 *	\note	We assume that picture- or slice headers are allways assigned to the
 *			partitionID of 0.  
 */

int ReadPicture ()
{
	static int picFormat = 0, picID = 0, qp;
	static int partitionID = 0, idreadflag = TRUE;
	int firstflag = TRUE; 
	int i = 0, bytes, logicalchannel, errorindicator=0, bitcount;
	int len, info;
	
	/* reset parameters */
	for (i=0; i<MAXPARTITION; i++) 
	{
		p[i].partitionID = 0;
		p[i].picFormat = NO_PARTITION_CONTENT;		/* required to detect last partition of slice/frame */
		p[i].picID = 0;
		p[i].qp = 0;
		p[i].sliceID = 0;
	}	
	i = 0;

	/* Read all partitions in one slice */
	while (TRUE)
	{
		/* read partition */	
#if ERR_IND
		if (1 != fread (&errorindicator, 1, 1, in))
			return TRUE;		
#endif
		if (1 != fread (&logicalchannel, 1, 1, in))
			return TRUE;
		if (1 != fread (&bytes, 4, 1, in))
			return TRUE;
		if (bytes > 0 )
			if (1 != fread (&p[i].data[0], bytes, 1, in))
				return TRUE;		/* Finished, EOF */
		
		/* read from partition information file */
		if (idreadflag == TRUE)
		{
			do 
			{
				if (fscanf(partinf,"%ld,",&partitionID) == EOF)
				{
					printf("Error: The file containing partition info is empty!");
					exit(-1);
				}
				fscanf(partinf,"%*[^\n]");
				fscanf(partinf,"%ld,",&bitcount);
				fscanf(partinf,"%*[^\n]");
			} while (bitcount == 0);
		}
		idreadflag = TRUE;

		if (bytes > 0)		
		{
			if (errorindicator == 1)
				printf("Partition %d was partly lost!\n", partitionID);

			len = GetVLCSymbol(&p[i].data[0], 0, &info);

			if (len < 31)
			{
				if ((i+1) >= MAXPARTITION)
				{
					printf("Error: No picture or slice sync found but max. number of possible partitions exceeded! len %d\n",len);
					/* put red partition into max. partition until next header partition is found */
					i--;
				}
				if (partitionID == 0)	
				{
					printf("Error: No picture or slice sync found! len %d\n",len);
					exit(-1);
				}
			} 
			else if (len == 33)			/* slice header */
			{
				p[i].startMB	= info >> 7;
				qp				= (info >> 2)&0x1f;
			}
			else if (len == 31)			/* picture header */
			{
				if (firstflag == FALSE)
				{
					/* New picture header found -> new picture */
#if ERR_IND
					bytes++;	/* add 1 byte for error indication flag */
#endif
					assert (0 == fseek (in, -5-bytes, SEEK_CUR));
					idreadflag = FALSE;
					break;
				}
				p[i].startMB	= 0;
				picFormat		= ((info&ICIF_MASK) >> 1);	// image format , 0= QCIF, 1=CIF
				qp				= (info&QP_MASK) >> 2;		// read quant parameter for current frame
				picID			= (info&TR_MASK) >> 7;		// 8 bit temporal reference
				firstflag = FALSE;
			}
			/* calculate number of bits in partition */
			p[i].partitionsizebits = bytes*8;
		}
		else	/* partition is lost */
		{
			if (errorindicator == 1)
				printf("Partition %d was lost!\n", partitionID);

			if (partitionID==0)
			{
				if (firstflag == FALSE)
				{
					/* New picture header found -> new picture */
#if ERR_IND
					bytes++;	/* add 1 byte for error indication flag */
#endif
					assert (0 == fseek (in, -5-bytes, SEEK_CUR));
					idreadflag = FALSE;
					break;
				}		
				firstflag = FALSE; 
				picID++;		
			}
			p[i].partitionsizebits = 0;
		}

		p[i].partitionID = partitionID;
		p[i].picFormat	 = picFormat;	/* image format , 0= QCIF, 1=CIF */
		p[i].picID		 = picID;		/* picture ID */
		p[i].qp			 = qp;			
		i++;
	} /* end while(TRUE) */
	return FALSE;
}


/*!
 *	\fn 	WritePicture
 *	\brief 	Writes partitions of slice/frame in the interim file-format
 *	\param	mode, mode for assignment of partitions to logical channels
 */

void WritePicture ()
{
	int i = 0;
	
	while (p[i].picFormat != NO_PARTITION_CONTENT) 
	{	
		printf ("WritePicture, id:%3d  picid:%3d  sliceid:%3d  startmb:%3d  pqp:%2d  pformat:%1d  size:%6d  bytes:%4d\n",
			p[i].partitionID,	p[i].picID, p[i].sliceID,	p[i].startMB,
			p[i].qp, p[i].picFormat, p[i].partitionsizebits, NumberOfBytes(p[i].partitionsizebits));
		
		assert (1 == fwrite (&p[i].partitionsizebits, 4, 1, out));
		assert (1 == fwrite (&p[i].partitionID, 4, 1, out));
		assert (1 == fwrite (&p[i].picID, 4, 1, out));
		assert (1 == fwrite (&p[i].sliceID, 4, 1, out));
		assert (1 == fwrite (&p[i].startMB, 4, 1, out));
		assert (1 == fwrite (&p[i].qp, 4, 1, out));
		assert (1 == fwrite (&p[i].picFormat, 4, 1, out));
		if (p[i].partitionsizebits > 0)
			assert (1 == fwrite (&p[i].data[0], NumberOfBytes(p[i].partitionsizebits), 1, out));

		i++;
	}
}


/*!
 *	\fn 	NumberOfBytes
 *	\brief	Calculates number of bytes from number of bits
 *	\param	bits, number of bits
 *	\return number of necessary bytes
 */
 
int NumberOfBytes (int bits)
{
	int bytes;

	bytes = bits / 8;
	if (bits % 8)
		bytes++;
	return bytes;
}


/*!
 *	\fn		GetVLCSymbol
 *	\brief	returns a VLC decoded syntax element
 *
 *	\param	byte buffer[]		containing VLC-coded data bits)
 *	\param	int totbitoffset	bit offset from start of partition
 *	\param	int type			expected data type (Partiotion ID)
 *	\return	int info, len		Length and Value of the next symbol
 *
 *	\note	As in both nal_bits.c and nal_part.c all data of one partition, slice,
 *			picture was already read into a buffer, there is no need to read any data
 *			here again. \par
 *
 *			GetVLCInfo was extracted because there should be only one place in the
 *			source code that has knowledge about symbol extraction, regardless of
 *			the number of different NALs. \par
 *
 *			This function could (and should) be optimized considerably \par
 *
 *			If it is ever decided to have different VLC tables for different symbol
 *			types, then this would be the place for the implementation
 */

int GetVLCSymbol (byte *buffer, int totbitoffset, int *info)
{

	register inf;
	int byteoffset;			// byte from start of frame
	int bitoffset;			// bit from start of byte
	int ctr_bit;			// control bit for current bit posision
	int bitcounter=1;

	byteoffset= totbitoffset/8;
	bitoffset= 7-(totbitoffset%8);
	ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));		// set up control bit

	inf=0;													// shortest possible code is 1, then info is always 0

	while (ctr_bit==0)
	{														// repeate until next 0, ref. VLC
		bitoffset-=2;										// from MSB to LSB
		if (bitoffset<0)
		{													// finish with current byte ?
			bitoffset=bitoffset+8;
			byteoffset++;
		}

		ctr_bit=buffer[byteoffset] & (0x01<<(bitoffset));

		// make infoword

		if(bitoffset>=7)									// first bit in new byte
			if (buffer[byteoffset-1] & (0x01))				// check last (info)bit of last byte
				inf = ((inf << 1) | 0x01);					// multiply with 2 and add 1, ref VLC
			else
				inf = (inf << 1);							// multiply with 2

		else
			if (buffer[byteoffset] & (0x01<<(bitoffset+1)))	// check infobit
				inf = ((inf << 1) | 0x01);
			else
				inf = (inf << 1);
		bitcounter+=2;
	}
	*info = inf;
	return bitcounter;                                      /* return absolute offset in bit from start of frame */
}
