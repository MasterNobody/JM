/*!
 *	\file	Nal_part.c  
 *	\brief	Network adaptation layer data partitioning file
 *
 *	\author	Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Stephan Wenger			<stewe@cs.tu-berlin.de>
 */

#include "contributors.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <malloc.h>

#include "global.h"
#include "elements.h"
#include "nal_part.h"

static PInfo * p[MAXPARTITION];   /*!< Main data structure to collect part. data */
static int CurrPicID = -1;        /*!< Current Picture ID */
static int CurrSliceID = -1;      /*!< Current Slice */
static int CurrStartMB = -1;
static int CurrQP = -1;
static int CurrFormat = -1;


/*!
 * \fn		void MallocPartition (int dt)
 * \brief	Allocates memory for specific partition
 * \param	dt, partitionid to allocate memory
 */

void MallocPartition (int dt)
{
	if (p[dt] == NULL)
	{
		assert ((p[dt]=calloc(1, sizeof (PInfo))) != NULL);
	}
	p[dt]->byteptr = 0;
	p[dt]->bitcount = 0;
	p[dt]->Symbols = 0;

	p[dt]->byte = 0;
	p[dt]->bitcounter = 8;
}


/*!
 * \fn		FreePartition
 * \brief	Resets PInfo struct of specific partition  
 * \param	Ptype, partitionid to reset structure
 * \sa		PInfo
 */

static void FreePartition (int PType)
{
	p[PType]->byteptr = 0;
	p[PType]->bitcount = 0;
	p[PType]->Symbols = 0;
	p[PType]->byte = 0;
	p[PType]->bitcounter = 8;
}


/*! 
 *	\fn		Highwrite
 *	\brief	Writes a partition
 *	\note	This now writes a slice and not necessarily a full picture
 *			HighWrite is called as soon as a start code is coded.
 *			In high.c numerical values are assigned for the various data
 *			types.  Those values are critical as they determine the order
 *			of the partitions in the file
 *			this would be the right place to implement channel coders,
 *			different file formats, packetization schemes, ...
 */

void HighWrite()
{
	int i, bytes, dt=0;

	for (i=0; i<MAXPARTITION; i++)
	{
		if (p[i] != NULL)
		{	
			/*!
			 *	\note
			 * 		open file to write partition id and lengths as
			 *		we do not have a "real" data departitioner at 
			 *		the decoding side, which requires parsing the stream
			 */
			fprintf(p_datpart,"%d\n%d\n",i , p[i]->bitcount);

			if (p[i]->bitcount > 0)
			{
				// found a partition containing data
				p[i]->d[p[i]->byteptr] = p[i]->byte << p[i]->bitcounter;    // put final byte
				// if (i == TYPE_HEADER) printf ("HighWrite: Putting final byte 0x%x\n", p[i]->d[p[i]->byteptr]);
				bytes = p[i]->byteptr;
				if (p[i]->bitcount % 8 != 0)
					bytes++ ;

				assert (1 == fwrite (&p[i]->bitcount, 4, 1, p_out));
				assert (1 == fwrite (&i, 4, 1, p_out));   // DataType
				assert (1 == fwrite (&CurrPicID, 4, 1, p_out));
				assert (1 == fwrite (&CurrSliceID, 4, 1, p_out));
				assert (1 == fwrite (&CurrStartMB, 4, 1, p_out));
				assert (1 == fwrite (&CurrQP, 4, 1, p_out));
				assert (1 == fwrite (&CurrFormat, 4, 1, p_out));
				assert (bytes == (int) fwrite (p[i]->d, 1, bytes, p_out));
				// printf ("Wrote %4d bytes  %5d bits  %5d Symbols of part %d Slice %d Pic %3d\n", bytes, p[i]->bitcount, p[i]->Symbols, i, CurrSliceID, CurrPicID);
				// getchar();
				FreePartition (i);
			}
		}
	}
}


/*!
 *	\fn		part_tml_putbits
 *	\brief	This function puts all bits into p[DataType->d except the maximum 7 bits
 *			leftover at the end of the slice.  These bits are stored in p[DataType]->byte
 *			and have to be put in HighWrite()
 *	\param	len			length of bitpattern in bits
 *	\param	bitpattern	bitpattern as vlc coded word
 *	\param	SLiceNo		number of current slice
 *	\param	DataType	type of syntaxelement
 */

void part_tml_putbits (int len,	int bitpattern, int SLiceNo, int DataType)
{
	int i, partitionnumber;
	PInfo *pa;
	unsigned int mask = 1 << (len-1);

	partitionnumber = assignSE2partition[PartitionMode][DataType];	/* assignment of syntaxelement to partition */

	if (p[partitionnumber] == NULL)
		MallocPartition (partitionnumber);

	pa = p[partitionnumber];
	pa->Symbols++;

#if TRACE
/*
	if (f == NULL)
	  f = fopen ("symenc.txt", "wb");
	if (len == 1)
	  info = 0;
	fprintf (f, "%d\t%d\t%d\n", symcount++, len, info);
*/
	if (partitionnumber == 0)
		printf ("part_tml_putbis: len %d bitpattern 0x%x\n", len, bitpattern);
#endif

	/* Add the new bits to the bitstream. */
	/* Write out a byte if it is full     */

	for (i=0; i<len; i++)
	{
		pa->byte <<= 1;
		if (bitpattern & mask)
			pa->byte |= 1;
		pa->bitcounter--;
		mask >>= 1;
		if (pa->bitcounter==0)
		{
			pa->bitcounter = 8;
			pa->d[pa->byteptr++]= pa->byte;
#if TRACE
			if (partitionnumber == 0)
				printf ("part_tml_putbits: put byte 0x%x\n, len %d bitpat 0x%x\n", pa->byte, len, bitpattern);
#endif
			pa->byte = 0;
		}
	}

	pa->bitcount += len;

}


/*!
 *	\fn		part_tml_init	
 *	\brief	initialize all partitions (does not allocate memory)
 */

void part_tml_init()
{
	int i;

	for (i=0; i<MAXPARTITION; i++)
		p[i]=NULL;
	//  printf ("Using data partinioned syntax by Stephan Wenger, TELES AG / TU Berlin\n");
}


/*!
 *	\fn		part_tml_put_startcode
 *	\brief	put start code into current partition
 *	\param	tr, Temporary Reference	
 *	\param	mb_nr, Macroblock number where slice starts
 *	\param	qp, Quantization parameter
 *	\param	image_format, Image format of picture
 *	\param	sliceno, current slicenumber
 *	\param	type, syntax elementtype (ought to be SE_HEADER)
 */

int part_tml_put_startcode(int tr, int mb_nr, int qp, int image_format, int sliceno, int type)
{
	int len, info;

	// printf ("part_tml_put_startcode (tr=%d, mb_nr=%d, qp=%d, format=%d, sliceno=%d, type=%d\n",
	//    tr, mb_nr, qp, image_format, sliceno, type);

	HighWrite();  // Flush accumulated bits of last Slice/Picture (if any)

	CurrPicID = tr;
	CurrSliceID = sliceno;
	CurrStartMB = mb_nr;
	CurrQP = qp;
	CurrFormat = image_format;

	if(mb_nr == 0)
	{
		/* Picture startcode */
		assert (CurrSliceID == 0);

		len = LEN_STARTCODE;
		info=(tr<<7)+(qp << 2)+(image_format << 1);
		//printf ("part_tml_put_startcode: putting len %d n info %d for first symbol\n", len, info);
		put_symbol("\nHeaderinfo",len,info, sliceno, SE_HEADER);
		return len;
	}
	else
	{
		/* Slice startcode */
		/* computing bitpattern since put_symbol() can not handle 33 bit symbols */
		unsigned int bitpattern = 0;
		int info_len = 16;
		info=(mb_nr << 7)+(qp << 2);
		while(--info_len >= 0)
		{
			bitpattern <<= 2;
			bitpattern |= (0x01 & (info >> info_len));
		}
		bitpattern <<= 1;
		bitpattern |= 0x01;
#if TRACE
		tracebits("Slice Header", 33, bitpattern);
#endif

		part_tml_putbits(1,0,sliceno, SE_HEADER);
		part_tml_putbits(32, bitpattern,sliceno, SE_HEADER);
		return 33;
	}
}


/*!
 *	\fn			
 *	\brief
 */

int part_tml_slice_too_big(int bits_slice)
{
	return 0; /* Never any slices */
}


/*!
 *	\fn			
 *	\brief
 */

void part_tml_finit()
{
	HighWrite();
}
