// *************************************************************************************
// *************************************************************************************
// Nal_bits.c  Network adaptation layer old TML-type bitstream
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
// Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
// *************************************************************************************
// *************************************************************************************
#include "contributors.h"

#include <stdio.h>
#include <string.h>
#include "global.h"
#include "elements.h"
#include "stream.h"


void
bits_tml_init()
{}


void
bits_tml_putbits(int len, int bitpattern, int sliceno, int type)
{
	int i;

	/* byte is used to store bits between calls */
	static unsigned char byte;

	/* bitcounter tells how many more bits can fit in the the 'byte' variable */
	static int bitcounter = 8;

	/* Add the new bits to the bitstream. */
	/* Write out a byte if it is full     */
	unsigned int mask = 1 << (len-1);
	for (i=0; i<len; i++)
	{
		byte <<= 1;
		if (bitpattern & mask)
			byte |= 1;
		bitcounter--;
		mask >>= 1;
		if (bitcounter==0)
		{
			bitcounter = 8;
			putc(byte,p_out);
		}
	}

	/* Stuff with ones and put out the last byte if this is the EOS symbol */
	if (type == SE_EOS)
	{
		int stuffing=0xff;
		int active_bitpos = 1 << (bitcounter-1);
		byte <<= bitcounter;

		while (stuffing>active_bitpos)
			stuffing=stuffing>>1;
		byte |= stuffing;
		putc(byte,p_out);
	}
}

int
bits_tml_put_startcode(int tr, int mb_nr, int qp, int image_format, int sliceno, int type)
{
	int info;
	if(mb_nr == 0)
	{

		/* Picture startcode */
		int len=LEN_STARTCODE;
		info=(tr<<7)+(qp << 2)+(image_format << 1);
		put_symbol("\nHeaderinfo",len,info, sliceno, type);
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

		bits_tml_putbits(1,0,sliceno, type);
		bits_tml_putbits(32, bitpattern,sliceno, type);
		return 33;
	}
}

int
bits_tml_slice_too_big(int bits_slice)
{
	return 0; /* Never any slices */
}

void
bits_tml_finit()
{}
