/*!
 *	\file	
 *			Nal_Part.c
 *	\brief	
 *			Network Adaptation layer for partition file
 *	\author  
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Stephan Wenger			<stewe@cs.tu-berlin.de>
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 */

#include "contributors.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#include "global.h"
#include "elements.h"
#include "nal_part.h"

#define	NO_HEADER_DETECT	-2
#define	EOF_DETECT			-1
#define	SLICE_DETECT		0
#define	HEADER_DETECT		1

/*!
 *	\fn		int part_tml_symbols_available (int type) {
 *	\brief	checks if element type remains in partition
 *	\return	TRUE	elements to read remain in partition
 *			FALSE	no more elements in partition
 */
int part_tml_symbols_available (
	int type)	/*!< syntaxelementtype */
{
	int partitionid;
	partitionid = assignSE2partition[PartitionMode][type];

	if (p[partitionid] == NULL)
		return FALSE;
	else
		return ((p[partitionid]->symptr < p[partitionid]->symbols));
};


/*!
 *	\fn		int part_tml_get_symbol (int *len, int *info, int type)
 *	\brief	gets symbol out of partition/slice/frame
 *
 *	\return check if partition contains requested element
 *			TRUE	element exist
 *			FALSE	partition does not contain any more elements
 *	\note
 *	- Side effects:
 *			Removes the read symbol from buffer
 *			Terminates decoder through assert() if there is no more symbol of the
 *			requested type available.  This can never happen in case of uncorrupted
 *			partition files.
 *	- Shortcomings:
 *			A more prudent reaction to missing data may be desirable if bit errors
 *			in partitions can occur.  Completely lost partitions should be handled
 *			elsewhere if possible, as this function is called really frequently.
 */
void part_tml_get_symbol(
	int *len,	/*!< pointer to length of element (modified) */
	int *info,  /*!< pointer to infoword (modified) */
	int type)	/*!< Data Type (Partition ID) of symbol requested */
{
	int partitionid;

//#if TRACE
	static int count =0;
	if (type == 0)
		count =0;
//#endif

	partitionid = assignSE2partition[PartitionMode][type];

	assert (p[partitionid] != NULL);
	
	*len = p[partitionid]->len[p[partitionid]->symptr];
	*info = p[partitionid]->info[p[partitionid]->symptr++];

#if TRACE
	if (type == SE_MBTYPE)
		printf("partition %3d %20s %3d %3d %3d\n",partitionid, SEtypes[type], (int)*len, (int)*info, ++count);
	else
		printf("partition %3d %20s %3d %3d \n",partitionid, SEtypes[type], (int)*len, (int)*info);
#endif

}


/*!
 *	\fn		void tml_init()
 *	\brief	Initialization code for this module
 */
void part_tml_init()
{
	int i;

	for (i=0; i<MAXPARTITION; i++)
		p[i]=NULL;
}


/*!
 *	\fn		void tml_finit()
 *	\brief	Finalization code for this module
 */
void part_tml_finit()
{}


/*!
 *	\fn		part_tml_find_startcode()
 *
 *	\return	tr, qp, mb_nr, format	Picture/Slice Header content
 *			0 == End of sequence, 
 *			1 == Startcode found
 *			2 == Headerpartition lost
 *	\note
 *	-Side effects:
 *			Reads a complete partition.  Throws out any pictures that contain
 *			not at least a TYPE_HEADER and a TYPE_MBTYPE Partition
 *  -Remarks:
 *			In a bitstream environment, this routine is necessary.  In a packet environment
 *			that assumes no packet corruption, however, there is no need to search for
 *			a start code.  By definition, every Slice/Picture has a start code partition,
 *			and, by convention of the file format, this partition is the first partition
 *			of the slice/picture in the file.  Therefore, part_nal_find_startcode() reads
 *			a slice/picture (using ReadSlice() ) and simply returns the values found in
 *			the individual fields of the file structure.
 *
 *			New in Version 5.1: When employing packetization as discussed in section 8 of
 *			the test model document, it is possible that pictures without necessary
 *			partitions are fed into the decoder (e.g. when the 'First' packet is lost).
 *			Therefore, part_tml_find_startcode ignores all unfinshed pictures that do
 *			not contain at least the TYPE_HEADER and TYPE_MBHEADER partitions.
 */
int part_tml_find_startcode(
	FILE *p_in,		/*!< Input file containing Partition structures */
	int *tr,		/*!< Temporal Reference of current frame (modified) */
	int *qp,		/*!< Quantization Parameter (modified) */
	int *mb_nr,		/*!< Macroblock number of slice-start (modified) */
	int *format)	/*!< Picture format CIF/QCIF (modified) */
{
//	int GotNecessaryPartitions = FALSE;

	int headerpart = assignSE2partition[PartitionMode][SE_HEADER];	/* header partition is partition containing header element */

//	do
//	{
		if (ReadSlice(p_in) == FALSE)
			return FALSE;
		/*!
		 * \note
		 * Condition to decode frame/slice is that partitions containing Header and MB_TYPE 
		 * elements are available.
		 */
//		GotNecessaryPartitions = (p[headerpart]->symbols > 0)
//		                         && (p[assignSE2partition[PartitionMode][SE_MBTYPE]]->symbols > 0);
//	}
//	while (!GotNecessaryPartitions);

	// Now all partitions are read into the buffer and all VLC codes are transformed.
	// Just return the tr, qp, mb_nr, format values, as read for any partition.
	// Here, I currently use the header partition, but those values should be
	// identical for any partition.  Note that sooner or later this section should
	// change.  There is really no point in having this side information both in
	// the VLC-coded headers and in side information of the packet file.

	*tr = p[headerpart]->PicID;
	*qp = p[headerpart]->QP;
	*mb_nr = p[headerpart]->StartMB;
	if (p[headerpart]->StartMB == 0)		// Only if Picture Start code write Format,
		*format = p[headerpart]->Format;	// else leave it untouched
	return TRUE;
}


/*!
 *	\fn		int part_tml_startcode_follows()
 *	\brief	Input file containing Partition structures
 *	\return	TRUE if a startcode follows (meaning that all symbols are used.
 *			FALSE otherwise.
 */
int part_tml_startcode_follows()
{
	int i;
	int ok = TRUE;

	for (i=0;i<MAXPARTITION;i++)
	{
		if (p[i] != NULL && p[i]->PicID >=0)	/* Check only valid entries */
		{		
			ok = ok && (p[i]->symbols == p[i]->symptr);
		}
	}
	return ok;
}


/*!
 *	\fn		MallocPartition()
 *	\brief	allocate memory for new partition
 */
static void MallocPartition (
	int PType)		/*!< number of partition ID */
{
	int i;
	assert ((PType < MAXPARTITION) && (PType >= 0));
	if (p[PType] == NULL)
		assert ((p[PType]=malloc(sizeof (PInfo))) != NULL);
	p[PType]->symbols = 0;				// Total number of symbols, write ptr
	p[PType]->symptr = 0;				// Symbol read ptr
	p[PType]->bytecount = 0;
	p[PType]->bitcount = 0;
	p[PType]->PicID = -1;
	p[PType]->SliceID = -1;
	p[PType]->StartMB = -1;
	p[PType]->QP = -1;
	p[PType]->Format = -1;
	for (i=0; i<MAXSIZEPARTITION; i++)
		p[PType]->d[i] = 0;
}


/*!
 *	\fn		FreePartition()
 *	\brief	Reset variables in partition struct
 *	\warning
 *			does not free allocated memory by p[]!
 */
static void FreePartition(
int PType)		/*!< number of partition ID */
{
	int i;
	if (p[PType] == NULL)
		return;
	p[PType]->symbols = 0;
	p[PType]->symptr = 0;
	p[PType]->bytecount = 0;
	p[PType]->bitcount = 0;
	p[PType]->PicID = -1;
	p[PType]->SliceID = -1;
	p[PType]->StartMB = -1;
	p[PType]->QP = -1;
	p[PType]->Format = -1;
	for (i=0; i<MAXSIZEPARTITION; i++)
		p[PType]->d[i] = 0;
}


/*!
 *	\fn		int GetSlice (FILE *in)
 *	\brief	Allocates all necessary partition buffers
 *			reads VLC-coded data bits into the buffer, and sets size information
 *			Moves the read pointer of the partition forward by one symbol
 *			Sets PicId, SliceID, StartMB, QP, PicFormat as found in the file 
 *			(not using VLC coded information) \par
 *
 *			Terminates the decoder through assert() in case of read failures due to
 *			corrupted input file \par
 *
 *			In case of corrupted Psync or Slicemarker the function reads until it gets 
 *			a uncorrupted headerpartition. \par
 *
 *	\return	EOF indication (0 == success, 1 == EOF)
 */
static int GetSlice (
	FILE *in)	/*!< Input file containing Partition structures */
{
	static int currentPicID = 0, currentFormat = 0;
	int dt = 0, bitcount = 0;
	int firsttime = TRUE;
	int picid=0, sliceid=0, startmb=0, qp=0, format;
	int bytecount, res, i;
	int len, info;
	int headerpart;

	headerpart = assignSE2partition[PartitionMode][SE_HEADER];

	/* Cleanup from last time */
	res = NO_HEADER_DETECT;

	for (i=0; i<MAXPARTITION; i++)
		FreePartition (i);

	/* Read Partitions of one Slice */
	while (1)
	{
		if (fread(&bitcount, 4, 1, in) != 1)
		{
			res = EOF_DETECT;		/* EOF indication */
			break;
		}
		assert (1 == fread (&dt, 4, 1, in));	/* get partition-ID */
		assert (1 == fread (&picid, 4, 1, in));
		assert (1 == fread (&sliceid, 4, 1, in));
		assert (1 == fread (&startmb, 4, 1, in));
		assert (1 == fread (&qp, 4, 1, in));
		assert (1 == fread (&format, 4, 1, in));

#if TRACE
		printf ("GetSlice: read %d bytes  %d bits of partition %d for pic %d, Slice %d StartMB %d QP %d\n", bytecount, bitcount, dt, picid, sliceid, startmb, qp);
		//getchar();
#endif
	
		if ((dt == headerpart) && (firsttime == FALSE))		
		{
			assert (!fseek (in, -28, SEEK_CUR));	/* Go back, so that the header will be read next time */
			return (res);	
		}
		else
		{
			/* calculate necessary bytes to read from file */
			bytecount = bitcount / 8;
			if ((bitcount % 8) != 0)
				bytecount++;
	
			MallocPartition (dt);
			p[dt]->PicID		= currentPicID;
			p[dt]->SliceID		= sliceid;
			p[dt]->bytecount	= bytecount;
			p[dt]->bitcount		= bitcount;
			if (bitcount > 0) 
			{
				assert (bytecount == (int) fread (p[dt]->d, 1, bytecount, in));
				ConvertPartition(dt);
			}
		} 

		// I hate it, but Rickards idea of separating the decoding of picture/slice header
		// info makes sense.  So I do here the same.  Note that GetVLCSymbol is NOT removing
		// anything from the buffers.  This is not important at the moment but may be helpful
		// if header content would also be decoded later in the low level software

		if (dt == headerpart)  		// header stuff
		{
			if ( (p[dt]->bitcount > 0) )
			{
				/* destructive decoding of first codeword in HEADER partition */
				get_symbol("Headerelement",&len, &info, SE_HEADER);	

				switch (len)
				{
				case 33:
					p[dt]->StartMB	= info>>7;
					p[dt]->QP		= (info>>2)&0x1f;
					p[dt]->PicID	= currentPicID;			/* 8 bit temporal reference */
					res = SLICE_DETECT;
					break;

				case 31:
					p[dt]->StartMB	= 0;
					p[dt]->Format	= currentFormat = (info&ICIF_MASK)>>1;		/* image format , 0= QCIF, 1=CIF */
					p[dt]->QP		= (info&QP_MASK)>>2;						/* read quant parameter for current frame */
					p[dt]->PicID	= currentPicID = (info&TR_MASK)>>7;			/* 8 bit temporal reference */
					res = HEADER_DETECT;					
					break;

				default:
					/*! 
					 * \note
					 * setting error concealment for HEADER element: 
					 * set PTYPE to INTER_IMG_1 and set all MB_TYPES to COPY_MB
					 * increase temporal reference to predict from previous
					 * decoded image.
					 */
					printf("ERROR: Headerpartition damaged. Length < 31 !ERROR");
					set_ec_flag(SE_HEADER);
					res = NO_HEADER_DETECT;				
				}
			}
			else
			{
				printf("ERROR: Headerpartition damaged, Bitcount = 0 !\n");
				set_ec_flag(SE_HEADER);
				res = NO_HEADER_DETECT;				
			}
			firsttime = FALSE;	
		}
	}
	return (res);
}


/*!
 *	\fn		int ConvertPartition (int dt)
 *	\brief	Converts all VLC coded symbols into len/info tupels
 *	\note
 *		-Shortcomings:
 *			If Bit errors in partitions should be supported then it may make sense
 *			to include a few consistency checks (e.g. maximum symbol length) already here.
 */
static void ConvertPartition (
	int dt)		/*!< Data Type (Partition-ID) of the Partition to convert */
{

	register PInfo *pa = p[dt];
	int ofs, len;

	assert (pa!=NULL);

	ofs = 0;
	while (ofs < pa->bitcount)
	{
		len = GetVLCSymbol (pa->d, ofs, &pa->info[pa->symbols]);
		ofs += len;
		pa->len[pa->symbols] = len;
		pa->symbols++;
		if (ofs >= pa->bitcount)
			break;
	}
#if TRACE	
	printf ("Found %d Symbols in %d bits in Partition %d of Picture %d\n", pa->symbols, pa->bitcount, dt, pa->PicID);
#endif 

	p[dt]->symptr = 0;		// Setup Symbol read pointer
}


/*!
 *	\fn		int ReadSlice (FILE *in)
 *	\brief	Fills complete partition structures for a whole slice
 *	\retuen	0 for EOF, 1 for success.  This is for historic reasons:
 *			TELENOR's read_frame returned the number of symbols read
 *			(although this info was never used (?))
 */
static int ReadSlice (
	FILE *p_in)		/*!< Input file in partition format */
{
	static int FinishedLastPicture = FALSE;

	if (FinishedLastPicture)
		return (FALSE);						/* EOS found */
	if (GetSlice (p_in) == EOF_DETECT)	/* Read Partition data of a slice into p */
		FinishedLastPicture = TRUE;		/* EOF or error in ReadFrame */

	return TRUE;
}

