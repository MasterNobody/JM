/*!
 *	\file	Stream.c
 *	\brief	VLC to symbol mapping
 *
 *	\author	Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Inge Lille-Langøy		<inge.lille-langoy@telenor.com>
 *			Rickard Sjoberg			<rickard.sjoberg@era.ericsson.se>
 *			Stephan Wenger			<stewe@cs.tu-berlin.de>
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 */

#include "contributors.h"

#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "elements.h"
#include "stream.h"


/*!
 *	\brief	Get syntax element out of the corresponding partition
 *		In case that an element is requested that has a set error
 *		concealment flag (see errorconcealment.c) a concealed element
 *		will be returned. For requests of elements out of non existing
 *		partitions the error concealment flags will be set first and
 *		and a concealed element will be retured either.
 *
 *	\param	tracestring, string for trace output
 *	\param	type, syntax element type
 *	\param	len, pointer to length of element "type" (modified)
 *	\param	info, pointer to infoword of element "type" (modified)
 */

void get_symbol(char *tracestring, int *len, int *info, int type)
{

	if (nal_symbols_available(type))		/* check on existing elements in partition */
	{
		nal_get_symbol(len, info, type);	/* if element exist in partition */
	}
	else
	{
		set_ec_flag(type);					/* otherwise set error concealment flag */
	}

	get_concealed_element(len, info, type);	/* check if element type needs any concealment */
											/* if not, len and info are untouched */
#if TRACE
	tracebits(tracestring, *len, *info);
#endif
}


/*!
 *	\fn	GetVLCSymbol is the old findinfo();
 *	\brief	Moves the read pointer of the partition forward by one symbol
 *
 *	\param	byte buffer[]		containing VLC-coded data bits)
 *	\param	int totbitoffset	bit offset from start of partition
 *	\param	int type			expected data type (Partiotion ID)
 *	\return	int info, len		Length and Value of the next symbol
 *
 *	\note	As in both nal_bits.c and nal_part.c all data of one partition, slice,
 *			picture was already read into a buffer, there is no need to read any data
 *			here again.
 *		\par
 *			GetVLCInfo was extracted because there should be only one place in the
 *			source code that has knowledge about symbol extraction, regardless of
 *			the number of different NALs.
 *		\par
 *			This function could (and should) be optimized considerably
 *		\par
 *			If it is ever decided to have different VLC tables for different symbol
 *			types, then this would be the place for the implementation
 */

int GetVLCSymbol (byte buffer[],int totbitoffset,int *info)
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
	{									// repeate until next 0, ref. VLC
		bitoffset-=2;										// from MSB to LSB
		if (bitoffset<0)
		{									// finish with current byte ?
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
	return bitcounter;                                              /* return absolute offset in bit from start of frame */
}

