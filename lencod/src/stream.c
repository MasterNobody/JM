// *************************************************************************************
// *************************************************************************************
// Stream.c	 Handle Symbol->VLC transform, Symbol-Buffer
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
// Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
// Stephan Wenger                  <stewe@cs.tu-berlin.de>
// *************************************************************************************
// *************************************************************************************
#include "contributors.h"


#include <stdio.h>
#include "global.h"
#include "stream.h"
#if TRACE
#include <string.h>    /* strncpy */
#endif

/* To make sure a slice does not exceed a certain number of bytes, backing     */
/* of one macroblock in the bitstream is supported. When symbols are generated */
/* for a macroblock they are stored in the symbol_array and not put out until   */
/* the final coding of the macroblock is done */
struct symboldata
{
#if TRACE
	char tracestring[50];
#endif
	int len;
	int info;
	int sliceno;
	int type;
}
symbol_array[MAX_SYMBOLS_PER_MACROBLOCK];
int next_symbol_position = 0;

/************************************************************************
*
*  Routine:     put_stored_symbols()
*
*  Description: Put out the stored symbols to the NAL by calling the
*               function pointer put_symbol()
*
************************************************************************/
void
put_stored_symbols()
{
	int i;
	for(i=0 ; i<next_symbol_position ; i++)
	{
#if TRACE
		put_symbol(symbol_array[i].tracestring,
		           symbol_array[i].len,
		           symbol_array[i].info,
		           symbol_array[i].sliceno,
		           symbol_array[i].type);
#else
put_symbol(NULL,
		symbol_array[i].len,
		symbol_array[i].info,
		symbol_array[i].sliceno,
		symbol_array[i].type);
#endif
	}
	next_symbol_position = 0;
}



/************************************************************************
*
*  Routine:     flush_macroblock_symbols()
*
*  Description: Resets the symbol array
*
************************************************************************/
void
flush_macroblock_symbols()
{
	next_symbol_position = 0;
}


/************************************************************************
*
*  Routine:     store_symbol()
*
*  Description: Store the symbol in the array
*
************************************************************************/
void store_symbol(char *tracestring,int len,int info, int sliceno, int type)
{
#if TRACE
	strncpy(symbol_array[next_symbol_position].tracestring, tracestring, 49);
	tracestring[50] = '\0';
#endif

	symbol_array[next_symbol_position].len = len;
	symbol_array[next_symbol_position].info = info;
	symbol_array[next_symbol_position].sliceno = sliceno;
	symbol_array[next_symbol_position].type = type;

	next_symbol_position++;
}

/************************************************************************
*
*  Routine      put_symbol()
*
*  Description: Makes code word and passes it down to the network adaptaion layer
*               A code word has the following format: 0 Xn...0 X2 0 X1 0 X0 1             
*               
*  Input:       Info   : Xn..X2 X1 X0
*               Length : Total number of bits in the codeword    
*               
*
************************************************************************/
void put_symbol(char *tracestring, int len, int info, int sliceno, int type)
{
	/* Convert info into a bitpattern int */
	unsigned int bitpattern = 0;

	/* vlc coding */
	int info_len = len/2;
	while(--info_len >= 0)
	{
		bitpattern <<= 2;
		bitpattern |= (0x01 & (info >> info_len));
	}
	bitpattern <<= 1;
	bitpattern |= 0x01;

#if TRACE
	tracebits(tracestring, len, bitpattern);
#endif

	/* Send bits to NAL, putbits is a function pointer */
	nal_putbits(len, bitpattern, sliceno, type);
}



/************************************************************************
*
*  Routine      tracebits()
*
*  Description: Write out a trace string on the trace file               
*
************************************************************************/
#if TRACE
void
tracebits(char *tracestring, int len, int bitpattern)
{
	static int bitcounter = 0;
	int i, chars;

	putc('@', p_trace);
	chars = fprintf(p_trace, "%i", bitcounter);
	while(chars++ < 6)
		putc(' ',p_trace);

	chars += fprintf(p_trace, "%s", tracestring);
	while(chars++ < 50)
		putc(' ',p_trace);

	/* Align bitpattern */
	if(len<15)
	{
		for(i=0 ; i<15-len ; i++)
			fputc(' ', p_trace);
	}
	/* Print bitpattern */
	bitcounter += len;
	for(i=1 ; i<=len ; i++)
	{
		if((bitpattern >> (len-i)) & 0x1)
			fputc('1', p_trace);
		else
			fputc('0', p_trace);
	}
	fprintf(p_trace, "\n");
}
#endif


