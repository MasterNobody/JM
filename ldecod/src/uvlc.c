/*!
 * \file
 *			uvlc.c	
 *	\brief	
 *			UVLC support functions
 *	\author
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Inge Lille-Lang�y               <inge.lille-langoy@telenor.com>
 *      Detlev Marpe                    <marpe@hhi.de>
 *      Gabi Blaettermann               <blaetter@hhi.de>
 */
#include "contributors.h"

#include <math.h>
#include <memory.h>
#include "global.h"
#include "uvlc.h"
#include "elements.h"
#include "bitsbuf.h"
#include "header.h"


// A little trick to avoid those horrible #if TRACE all over the source code
#if TRACE
#define SYMTRACESTRING(s) strcpy(sym.tracestring,s)
#else
#define SYMTRACESTRING(s)	/* to nothing */
#endif

extern void tracebits(const char *trace_str,	int len,	int info,int value1,
    int value2)	;

/************************************************************************
*
*  Routine      linfo
*
*  Description:   
*               Input: lenght and info
*               Output: number in the code table
*                    
************************************************************************/
void linfo(int len,	int info, int *value1, int *dummy) 
{
	*value1 = (int)pow(2,(len/2))+info-1; // *value1 = (int)(2<<(len>>1))+info-1;
}

/************************************************************************
*
*  Routine      linfo_mvd
*
*  Description:   
*               Input: lenght and info
*               Output: signed mvd
*                    
************************************************************************/
void linfo_mvd(int len,	int info, int *signed_mvd, int *dummy)  
{
	int n;
	n = (int)pow(2,(len/2))+info-1;
	*signed_mvd = (n+1)/2;
	if((n & 0x01)==0)                           /* lsb is signed bit */
		*signed_mvd = -*signed_mvd;
}

/************************************************************************
*
*  Routine      linfo_cbp_intra
*
*  Description:   
*               Input: lenght and info
*               Output: cbp (intra)
*                    
************************************************************************/
void linfo_cbp_intra(int len,int info,int *cbp, int *dummy)
{
	extern const byte NCBP[48][2];
    int cbp_idx;
	linfo(len,info,&cbp_idx,dummy);
    *cbp=NCBP[cbp_idx][0];
}

/************************************************************************
*
*  Routine      linfo_cbp_inter
*
*  Description:   
*               Input: lenght and info
*               Output: cbp (inter)
*                    
************************************************************************/
void linfo_cbp_inter(int len,int info,int *cbp, int *dummy)
{
	extern const byte NCBP[48][2];
	int cbp_idx;
	linfo(len,info,&cbp_idx,dummy);
    *cbp=NCBP[cbp_idx][1];
}
/************************************************************************
*
*  Routine      linfo_levrun_inter
*
*  Description:   
*               Input: lenght and info
*               Output: level, run
*                    
************************************************************************/
void linfo_levrun_inter(int len, int info, int *level, int *irun)	
{
	int l2;
	int inf;
	if (len<=9)
	{
		l2=mmax(0,len/2-1);
		inf=info/2;
		*level=NTAB1[l2][inf][0];
		*irun=NTAB1[l2][inf][1];
		if ((info&0x01)==1)
			*level=-*level;                   /* make sign */
	}
	else                                  /* if len > 9, skip using the array */
	{
		*irun=(info&0x1e)>>1;
		*level = LEVRUN1[*irun] + info/32 + (int)pow(2,len/2 - 5);
		if ((info&0x01)==1)
			*level=-*level;
	}
    /* GB */
    if (len == 1) /* EOB */
        *level = 0;
}

/************************************************************************
*
*  Routine      linfo_levrun_intra
*
*  Description:   
*               Input: lenght and info
*               Output: level, run
*                    
************************************************************************/
void linfo_levrun_intra(int len, int info, int *level,	int *irun)	
{
	int l2;
	int inf;

	if (len<=9)
	{
		l2=mmax(0,len/2-1);
		inf=info/2;
		*level=NTAB2[l2][inf][0];
		*irun=NTAB2[l2][inf][1];
		if ((info&0x01)==1)
			*level=-*level;                 /* make sign */
	}
	else                                  /* if len > 9, skip using the array */
	{
		*irun=(info&0x0e)>>1;
		*level = LEVRUN2[*irun] + info/16 + (int)pow(2,len/2-4) -1;
		if ((info&0x01)==1)
			*level=-*level;
	}
     /* GB */
    if (len == 1) /* EOB */
        *level = 0;
}


/************************************************************************
*
*  Routine      linfo_levrun_c2x2
*
*  Description:   
*               Input: lenght and info
*               Output: level, run
*                    
************************************************************************/
void linfo_levrun_c2x2(int len, int info, int *level, int *irun)
{
	int l2;
	int inf;

	if (len<=5)
	{
		l2=mmax(0,len/2-1);
		inf=info/2;
		*level=NTAB3[l2][inf][0];
		*irun=NTAB3[l2][inf][1];
		if ((info&0x01)==1)
			*level=-*level;                 /* make sign */
	}
	else                                  /* if len > 5, skip using the array */
	{
		*irun=(info&0x06)>>1;
		*level = LEVRUN3[*irun] + info/8 + (int)pow(2,len/2 - 3);
		if ((info&0x01)==1)
			*level=-*level;
	}
     /* GB */
    if (len == 1) /* EOB */
        *level = 0;
}




//  readSliceUVLC for new Pictureheader according VCEG-M79 

// Slice Headers can start on every byte aligned position, provided zero-stuffing.
// This is implemented here in such a way that a slice header can be trailed by 
// any number of 0 bits.
//
// readSliceUVLC returns -1 in case of problems, or oen of SOP, SOS, EOS in case of success

int readSliceUVLC(struct img_par *img, struct inp_par *inp)
{
    Slice *currSlice = img->currentSlice;
	DataPartition *dP;
	Bitstream *currStream = currSlice->partArr[0].bitstream;
	int *partMap = assignSE2partition[inp->partition_mode];
	int frame_bitoffset = currStream->frame_bitoffset = 0;
    SyntaxElement sym;
	int dummy;
	byte *buf = currStream->streamBuffer;
	//	static int first=TRUE;

	int code_word_ctr=0;
	int len, info;
    
	memset (buf, 0xff, MAX_CODED_FRAME_SIZE);		// this prevents a buffer full with zeros
	currStream->bitstram_length = GetOneSliceIntoSourceBitBuffer(buf);
	if (currStream->bitstram_length > 4)  // More than just a start code
	{		
		sym.type = SE_HEADER;
#if TRACE
	strcpy(sym.tracestring, "\nHeaderinfo");
#endif
		dP = &(currSlice->partArr[partMap[sym.type]]);
		len =  GetVLCSymbol (buf, frame_bitoffset, &info, currStream->bitstram_length); 
		currStream->frame_bitoffset +=len;
		switch (info)
		{
		case 0: 
			/* read PICTURE HEADER */
			dummy = PictureHeader(img,inp);
			return SOP;
		case 1:
			/* read SLICE HEADER */
			dummy = SliceHeader(img,inp);
			return SOS;
		default:
			set_ec_flag(SE_HEADER);				/* conceal current frame/slice */
			break;
		}
	} 
	else 		// less than four bytes in file -> cannot be a slice
		return EOS;    
	return 0;
}


/************************************************************************
*
*  Routine      readSyntaxElement_UVLC
*
*  Description: read next UVLC codeword from UVLC-partition and
*               map it to the corresponding syntax element
*
************************************************************************/
int readSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP)
{
    Bitstream   *currStream = dP->bitstream;
	int frame_bitoffset = currStream->frame_bitoffset;
	byte *buf = currStream->streamBuffer; 
	int BitstreamLengthInBytes = currStream->bitstram_length; 
	
	sym->len =  GetVLCSymbol (buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes);
	if (sym->len == -1)
		return -1;
	currStream->frame_bitoffset += sym->len; 
	sym->mapping(sym->len,sym->inf,&(sym->value1),&(sym->value2));

#if TRACE
	tracebits(sym->tracestring, sym->len, sym->inf, sym->value1, sym->value2);
#endif
   
    return 1;
}

/************************************************************************
*
*  Routine      uvlc_startcode_follows
*
*  Description: Check if there are symbols for the next MB
*               
*
************************************************************************/

int uvlc_startcode_follows(struct img_par *img, struct inp_par *inp)
{
	Slice *currSlice = img->currentSlice;
	int dp_Nr = assignSE2partition[inp->partition_mode][SE_MBTYPE];
	DataPartition *dP = &(currSlice->partArr[dp_Nr]);
	Bitstream   *currStream = dP->bitstream;
	byte *buf = currStream->streamBuffer; 
	int frame_bitoffset = currStream->frame_bitoffset;
	int info;
	
	if (-1 == GetVLCSymbol (buf, frame_bitoffset, &info, currStream->bitstram_length)) 
		return TRUE;
	else
		return FALSE;
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

int GetVLCSymbol (byte buffer[],int totbitoffset,int *info, int bytecount) 
{

	register int inf;
	long byteoffset;			// byte from start of frame
	int bitoffset;			// bit from start of byte
	int ctr_bit=0;			// control bit for current bit posision
	int bitcounter=1;

	byteoffset= totbitoffset/8;
	bitoffset= 7-(totbitoffset%8);
	ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));		// set up control bit

	inf=0;													// shortest possible code is 1, then info is always 0

	while (ctr_bit==0)
	{									// repeate until next 0, ref. VLC
		bitoffset-=2;						// from MSB to LSB
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
		if (byteoffset > bytecount)
			return -1;
	}
	*info = inf;
	return bitcounter;           /* return absolute offset in bit from start of frame */
}





