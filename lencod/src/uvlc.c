// *************************************************************************************
// *************************************************************************************
// UVLC.c	 UVLC table helper functions
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
// Detlev Marpe                    <marpe@hhi.de>
// Stephan Wenger				   <stewe@cs.tu-berlin.de>
// *************************************************************************************
// *************************************************************************************
#include "contributors.h"


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <assert.h>

#include "global.h"
#include "elements.h"

/************************************************************************
*
*  Routine      n_linfo 
*
*  Description:   
*               Input: Number in the code table
*               Output: lenght and info
*                    
************************************************************************/
void n_linfo(int n, int *len,int *info)
{
	int i,nn;

	nn=(n+1)/2;

	for (i=0; i < 16 && nn != 0; i++)
	{
		nn /= 2;
	}
	*len= 2*i + 1;
	*info=n+1-(int)pow(2,i);
}

/************************************************************************
*
*  Routine      n_linfo2 (modif. to fit the generic interface)
*
*  Description:   
*               Input: Number in the code table
*               Output: lenght and info
*                    
************************************************************************/
void n_linfo2(int n, int dummy, int *len,int *info)
{
	int i,nn;

	nn=(n+1)/2;

	for (i=0; i < 16 && nn != 0; i++)
	{
		nn /= 2;
	}
	*len= 2*i + 1;
	*info=n+1-(int)pow(2,i);
}

/************************************************************************
*
*  Routine      intrapred_linfo (pairing of intra prediction modes is done
*																	here)
*
*  Description:   
*               Input: Number in the code table
*               Output: lenght and info
*                    
************************************************************************/
void intrapred_linfo(int ipred1, int ipred2, int *len,int *info)
{
	extern const int IPRED_ORDER[6][6];
	n_linfo(IPRED_ORDER[ipred1][ipred2],len,info);

}

/************************************************************************
*
*  Routine      cbp_linfo_intra
*
*  Description:   
*               Input: Number in the code table
*               Output: lenght and info
*                    
************************************************************************/
void cbp_linfo_intra(int cbp, int dummy, int *len,int *info)
{
	extern const int NCBP[48][2];
	n_linfo(NCBP[cbp][0],len,info);
}


/************************************************************************
*
*  Routine      cbp_linfo_inter
*
*  Description:   
*               Input: Number in the code table
*               Output: lenght and info
*                    
************************************************************************/
void cbp_linfo_inter(int cbp, int dummy, int *len,int *info)
{
	extern const int NCBP[48][2];
	n_linfo(NCBP[cbp][1],len,info);
}


/************************************************************************
*
*  Routine      mvd_linfo2 (modif. to fit the generic interface)
*
*  Description: 
*               Input: motion vector differense
*               Output: lenght and info
*                    
************************************************************************/
void mvd_linfo2(int mvd, int dummy, int *len,int *info)
{
	
	int i,n,sign,nn;

	sign=0;

	if (mvd <= 0)
	{
		sign=1;
	}
	n=abs(mvd) << 1;

	/*
	n+1 is the number in the code table.  Based on this we find length and info 
	*/

	nn=n/2;
	for (i=0; i < 16 && nn != 0; i++)
	{
		nn /= 2;
	}
	*len=i*2 + 1;
	*info=n - (int)pow(2,i) + sign;
}


/************************************************************************
*
*  Routine      levrun_linfo_c2x2
*
*  Description: 2x2 transform of chroma DC
*               Input: level and run for coefficiets
*               Output: lenght and info
*               see ITU document for bit assignment
*
************************************************************************/
void levrun_linfo_c2x2(int level,int run,int *len,int *info)
{
	const int NTAB[2][2]=
	    {
	        {
	            1,5
	        }
	,{3,0}
	    };
	const int LEVRUN[4]=
	    {
	        2,1,0,0
	    };

	int levabs,i,n,sign,nn;

	if (level == 0) /*  check if the coefficient sign EOB (level=0) */
	{
		*len=1;
		return;
	}
	sign=0;
	if (level <= 0)
	{
		sign=1;
	}
	levabs=abs(level);
	if (levabs <= LEVRUN[run])
	{
		n=NTAB[levabs-1][run]+1;
	}
	else
	{
		n=(levabs-LEVRUN[run])*8 + run*2;
	}

	nn=n/2;

	for (i=0; i < 16 && nn != 0; i++)
	{
		nn /= 2;
	}
	*len= 2*i + 1;
	*info=n-(int)pow(2,i)+sign;
}

/************************************************************************
*
*  Routine      levrun_linfo_inter
*
*  Description: Single scan coefficients
*               Input: level and run for coefficiets
*               Output: lenght and info
*               see ITU document for bit assignment
*
************************************************************************/
void levrun_linfo_inter(int level,int run,int *len,int *info)
{
	const byte LEVRUN[16]=
	    {
	        4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0
	    };
	const byte NTAB[4][10]=
	    {
	        {
	            1, 3, 5, 9,11,13,21,23,25,27
	        },
	{ 7,17,19, 0, 0, 0, 0, 0, 0, 0},
	{15, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{29, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	    };

	int levabs,i,n,sign,nn;

	if (level == 0)           /*  check for EOB  */
	{
		*len=1;
		return;
	}

	if (level <= 0)
		sign=1;
	else
		sign=0;

	levabs=abs(level);
	if (levabs <= LEVRUN[run])
	{
		n=NTAB[levabs-1][run]+1;
	}
	else
	{
		n=(levabs-LEVRUN[run])*32 + run*2;
	}

	nn=n/2;

	for (i=0; i < 16 && nn != 0; i++)
	{
		nn /= 2;
	}
	*len= 2*i + 1;
	*info=n-(int)pow(2,i)+sign;

}

/************************************************************************
*
*  Routine      levrun_linfo_intra
*
*  Description: Double scan coefficients
*               Input: level and run for coefficiets
*               Output: lenght and info
*               see ITU document for bit assignment
*
************************************************************************/
void levrun_linfo_intra(int level,int run,int *len,int *info)
{
	const byte LEVRUN[8]=
	    {
	        9,3,1,1,1,0,0,0
	    };

	const byte NTAB[9][5] =
	    {
	        {
	            1, 3, 7,15,17
	        },
	{ 5,19, 0, 0, 0},
	{ 9,21, 0, 0, 0},
	{11, 0, 0, 0, 0},
	{13, 0, 0, 0, 0},
	{23, 0, 0, 0, 0},
	{25, 0, 0, 0, 0},
	{27, 0, 0, 0, 0},
	{29, 0, 0, 0, 0},
	    };

	int levabs,i,n,sign,nn;

	if (level == 0)     /*  check for EOB */
	{
		*len=1;
		return;
	}
	if (level <= 0)
		sign=1;
	else
		sign=0;

	levabs=abs(level);
	if (levabs <= LEVRUN[run])
	{
		n=NTAB[levabs-1][run]+1;
	}
	else
	{
		n=(levabs-LEVRUN[run])*16 + 16 + run*2;
	}

	nn=n/2;

	for (i=0; i < 16 && nn != 0; i++)
	{
		nn /= 2;
	}
	*len= 2*i + 1;
	*info=n-(int)pow(2,i)+sign;
}

/************************************************************************
*
*  Routine      symbol2uvlc()
*
*  Description: Makes code word and passes it back
*               A code word has the following format: 0 Xn...0 X2 0 X1 0 X0 1             
*               
*  Input:       Info   : Xn..X2 X1 X0
*               Length : Total number of bits in the codeword    
*               
************************************************************************/

int symbol2uvlc(SyntaxElement *sym)
{
	int info_len = sym->len/2;

	/* Convert info into a bitpattern int */
	sym->bitpattern = 0;

	/* vlc coding */
	while(--info_len >= 0)
	{
		sym->bitpattern <<= 2;
		sym->bitpattern |= (0x01 & (sym->inf >> info_len));
	}
	sym->bitpattern <<= 1;
	sym->bitpattern |= 0x01;

	return 0;
}


/************************************************************************
*
*  Routine      writeSyntaxElement_UVLC
*
*  Description: generates UVLC code and passes the codeword to the buffer
*
************************************************************************/
int	writeSyntaxElement_UVLC(SyntaxElement *se, DataPartition *this_dataPart)
{

	se->mapping(se->value1,se->value2,&(se->len),&(se->inf));

	symbol2uvlc(se);

	writeUVLC2buffer(se, this_dataPart->bitstream);

#if TRACE
		trace2out (se);
#endif
		
	return (se->len);
}


/************************************************************************
*
*  Routine      writeUVLC2buffer
*
*  Description: writes UVLC code to the appropriate buffer
*
************************************************************************/
void	writeUVLC2buffer(SyntaxElement *se, Bitstream *currStream)
{

	int i;
	unsigned int mask = 1 << (se->len-1);

	/* Add the new bits to the bitstream. */
	/* Write out a byte if it is full     */
	for (i=0; i<se->len; i++)
	{
		currStream->byte_buf <<= 1;
		if (se->bitpattern & mask)
			currStream->byte_buf |= 1;
		currStream->bits_to_go--;
		mask >>= 1;
		if (currStream->bits_to_go==0)
		{
			currStream->bits_to_go = 8;
			currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
		}
	}
}



/************************************************************************
*
*  Routine      writeEOS2buffer
*
*  Description: generates UVLC code for EOS and writes it to the appropriate buffer
*
************************************************************************/
void	writeEOS2buffer()
{
	int dP_nr = assignSE2partition[input->partition_mode][SE_EOS];
	Bitstream *currStream = ((img->currentSlice)->partArr[dP_nr]).bitstream;

	SyntaxElement sym;

	sym.len = LEN_STARTCODE;
	sym.inf = EOS;
	sym.type	= SE_EOS;
#if TRACE
	strcpy(sym.tracestring, "EOS");
#endif

	symbol2uvlc(&sym);

	writeUVLC2buffer(&sym, currStream);

#if TRACE
	trace2out(&sym);
#endif


}






/************************************************************************
*
*  Routine      trace2out()
*
*  Description: Write out a trace string on the trace file               
*
************************************************************************/
#if TRACE
void
trace2out(SyntaxElement *sym)
{
	static int bitcounter = 0;
	int i, chars;

	if (p_trace != NULL) {
		putc('@', p_trace);
		chars = fprintf(p_trace, "%i", bitcounter);
		while(chars++ < 6)
			putc(' ',p_trace);

		chars += fprintf(p_trace, "%s", sym->tracestring);
		while(chars++ < 50)
			putc(' ',p_trace);

	/* Align bitpattern */
		if(sym->len<15)
		{
			for(i=0 ; i<15-sym->len ; i++)
				fputc(' ', p_trace);
		}
		/* Print bitpattern */
		bitcounter += sym->len;
		for(i=1 ; i<=sym->len ; i++)
		{
			if((sym->bitpattern >> (sym->len-i)) & 0x1)
				fputc('1', p_trace);
			else
				fputc('0', p_trace);
		}
		fprintf(p_trace, "\n");
	}
}
#endif



