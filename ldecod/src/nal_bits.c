/*!
 *	\file
 *			Nal_bits.c	
 *	\brief
 *			Network adaptation layer for old TML bitstream format
 *	\author
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Inge Lille-Langøy           <inge.lille-langoy@telenor.com>
 *			Rickard Sjoberg             <rickard.sjoberg@era.ericsson.se>
 *			Stephan Wenger				<stewe@cs.tu-berlin.de>
 */
#include "contributors.h"

#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "elements.h"
#include "stream.h"

/*!
 * NAL stuff
 */
int findinfo(byte *, int, int* ,FILE *p_in);

int info_ctr=0;
int no_symbols_in_slice;

int info_arr[MAX_INFO_WORD];
int len_arr[MAX_INFO_WORD];

void
bits_tml_init()
{}

void bits_tml_finit()
{}


/*!
 *	\fn		int  bits_tml_symbols_available (int type)
 *	\brief	check if symbols in bitstream are available
 *	\param	type is type of syntax element
 *	\return	returns allways TRUE
 */
int  bits_tml_symbols_available (int type) {
	return TRUE;
}


/*!
 *	\fn		findinfo()
 *
 *  \brief	find lenght and info of one codeword
 *
 *  \return bit offset from start of frame and info (through pointer)
 */
int findinfo(
	byte buffer[],		/*!< Stream buffer */
	int totbitoffset,	/*!< bit offset from start of frame */
	int *info,			/*!< infoword to return */
	FILE *p_in)			/*!< input stream file */
{
	int byteoffset;                                                 /* byte from start of frame */
	int bitoffset;                                                  /* bit from start of byte */
	int ctr_bit;                                                    /* control bit for current bit posision */
	int bitcounter=1;

	byteoffset= totbitoffset/8;
	bitoffset= 7-(totbitoffset%8);                                  /* Find bit offset in current byte, start with MSB  */

	if (bitoffset==7)                                             /* first bit in new byte */
	{
		buffer[byteoffset]=fgetc(p_in);                               /* read new byte from stream */
	}

	ctr_bit = (buffer[byteoffset] & (0x01<<bitoffset));             /* set up control bit */

	*info=0;                                                        /* shortest possible code is 1, then info is always 0 */

	while(ctr_bit==0)                                               /* repeate until next 0, ref. VLC */
	{
		bitoffset-=2;                                                 /* from MSB to LSB */

		if (bitoffset<0)                                              /* finish with current byte ? */
		{
			bitoffset=bitoffset+8;
			byteoffset++;
			buffer[byteoffset]=fgetc( p_in );                           /* read next */
		}

		ctr_bit=buffer[byteoffset] & (0x01<<(bitoffset));
		/* make infoword   */
		if(bitoffset>=7)                                              /* first bit in new byte */
		{
			if (buffer[byteoffset-1] & (0x01))                          /* check last (info)bit of last byte */
				*info = ((*info << 1) | 0x01);                            /* multiply with 2 and add 1, ref VLC */
			else
				*info = (*info << 1);                                     /* multiply with 2    */
		}
		else
		{
			if (buffer[byteoffset] & (0x01<<(bitoffset+1)))             /* check infobit */
				*info = ((*info << 1) | 0x01);
			else
				*info = (*info << 1);
		}
		bitcounter+=2;
	}                                                               /* lenght of codeword and info is found  */

	return bitcounter;                                              /* return absolute offset in bit from start of frame */
}


/*!
 *  \fn      nal_find_startcode()
 *  \brief		Find a start code
 *  \return		0  End of sequence
 *              1  Startcode found
 */
int bits_tml_find_startcode(
	FILE *p_in,		/*!< Input stream file */
	int *tr,		/*!< A place to write the new tr if changed */
	int *qp,		/*!< The new qp value */	
	int *mb_nr,		/*!< The macroblock number that follows */
	int *format)	/*!< A place to write the new format if changed */
{
	static int frame_bitoffset=0;                                   /* number of bit from start frame */
	static first=TRUE;

	static byte buf[MAX_FRAME_SIZE];                                /* bitstream for one frame        */

	int info;                                                       /* one infoword                   */
	int len;                                                        /* length of codeword             */

	int code_word_ctr=0;
	static int info_sc; /* info part of start code        */
	static int len_sc;
	static finish = FALSE;                                          /* TRUE if end of sequence (eos)  */

	if (finish)
	{
		info_ctr=0;
		get_symbol("EOS", &len_sc,&info_sc,SE_EOS);
		return 0;                                                     /* indicates eos                  */
	}
	/*
	   We don't know when a frame is finish until the next startcode, which belong to the next frame,
	   therefore we have to add the stored one for all frames except the first
	*/
	if (first==TRUE)
		first=FALSE;
	else
	{
		len_arr[0]=len_sc;
		info_arr[0]=info_sc;
		code_word_ctr++;
	}

	for (;;)                                                  /* repeate until new start/end code is found */
	{
		len =  findinfo(buf,frame_bitoffset,&info,p_in);      /* read a new word */

		if (len >= LEN_STARTCODE && code_word_ctr > 0)        /* Added the ability to handle byte aligned start code. KOL 03/15/99 */
		{
			info_sc=info;                                     /* store start code for next frame        */
			len_sc=len;
			if (info & 0x01)                                  /* eos found, stored to next time read_slice() is called */
				finish=TRUE;

			info_ctr=0;
			buf[0]=buf[(frame_bitoffset+len)/8];        /* highest byte used for new frame also   */
			frame_bitoffset=(frame_bitoffset+len)%8;    /* bitoffest for first byte in new frame  */
			no_symbols_in_slice=code_word_ctr;
			code_word_ctr=0;
			break;
		}

		frame_bitoffset+=len;
		len_arr[code_word_ctr]=len;
		info_arr[code_word_ctr]=info;
		code_word_ctr++;

		if (code_word_ctr > MAX_INFO_WORD)
		{
			printf("Did not find end of sequence, exiting\n");
			exit(1);
		}
	}

	/* Parse the first symbol which has the header information */
	get_symbol("Headerinfo", &len,&info, SE_HEADER);
	switch (len)
	{
	case 33:
		*mb_nr = info>>7;
		*qp   = (info>>2)&0x1f;
		break;
	
	case 31:
		*mb_nr = 0;
		*format=(info&ICIF_MASK)>>1;        /* image format , 0= QCIF, 1=CIF          */
		*qp=(info&QP_MASK)>>2;              /* read quant parameter for current frame */
		*tr=(info&TR_MASK)>>7;              /* 8 bit temporal reference               */
		break;
	
	default:
		set_ec_flag(SE_HEADER);				/* conceal current frame/slice */
		break;
	}
	return 1;
}


/*!
 *  \fn     tml_get_symbol(int *len)
 *  \brief	read one info word and the lenght of the codeword
 *  \return	info, and codeword length through pointer
 */
void bits_tml_get_symbol(
	int *len,	/*!< length of vlc codeword (return value) */
	int *info,	/*!< infoword of vlc codeword (return value) */
	int type)	/*!< type of syntax element */
{
	*len = len_arr[info_ctr];		/* lenght of a codeword */
	*info = info_arr[info_ctr++];	/* return info part of codeword */
}


int bits_tml_startcode_follows()
{
	return(info_ctr >= no_symbols_in_slice);
}
