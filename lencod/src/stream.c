/************************************************************************
*
*  stream.c for H.26L encoder. 
*  Copyright (C) 1999  Telenor Satellite Services, Norway
*  
*  Contacts: 
*  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
* 
*
*  Telenor Satellite Services 
*  P.O.Box 6914 St.Olavs plass                       
*  N-0130 Oslo, Norway                 
*  
************************************************************************/
#include <stdio.h>
#include "global.h"
#include "stream.h"

/************************************************************************
*
*  Routine      put
*
*  Description: Makes code word and write it to output bitstream 
*               A code word has the following format: 0 Xn...0 X2 0 X1 0 X0 1             
*               
*  Input:       Info   : Xn..X2 X1 X0
*               Length : Total number of bits in the codeword    
*               
*
************************************************************************/ 
void put(char *tracestring,int len,int info)
{
  int i;
  static byte active_bitpos=0x80;         /* start writing to MSB                 */
  static byte outbyte=0x00;               /* data of byte currently being filled  */
  static int bytecount=0;                 /* active byte in buffer                */
  static int bitcount=0;                  /* active bit in buffer                 */
  
  static byte outbuf[MAX_FRAME_SIZE];     /* store bitstream for one frame        */

#if TRACE  
  tracebits(tracestring, len, info);        /* trace info */
#endif
  
  for (i=0;i<len/2;i++)
  {
    outbyte=outbyte & ~active_bitpos;          /* set odd bit positions to 0 (ref VLC) */
    if ((active_bitpos=active_bitpos>>1) <= 0) /* check if byte is full */
    {
      active_bitpos=0x80;                    /* bit pos 8, first bit positions */
      outbuf[bytecount++]=outbyte;           /* over byte boundary,write byte to buffer */     
      outbyte=0;
    }
    if (0x01 & ( info >> (len/2-i-1)))
      outbyte=(outbyte | active_bitpos) ;    /* add info part to codeword */
    
    if ((active_bitpos=active_bitpos>>1) <= 0) /* check again if byte is full */
    {
      active_bitpos=0x80;                    /* bit pos 8 */
      outbuf[bytecount++]=outbyte;           /* over byte boundary,write byte to buffer */
      outbyte=0;
    }
  }    
  outbyte=outbyte | active_bitpos;               /* set last bit in info word to 1  (ref VLC)  */ 
  if ((active_bitpos=active_bitpos>>1) <= 0)
  {
    active_bitpos=0x80;                        /* bit pos 8 */ 
    outbuf[bytecount++]=outbyte;               /* over byte boundary,write byte to buffer */ 
    outbyte=0;
  }
  bitcount += len; 
  
  if (len==LEN_STARTCODE)                        /* header or new image, write last one */ 
  {
    if (info == EOS)                              /* end of sequence, stuff last byte for byte alignment */
    {
      int stuffing=0xff;
      while (stuffing>active_bitpos)
        stuffing=stuffing>>1;  
      outbuf[bytecount] = outbyte | stuffing;   /*  stuff unused bit pos in the last byte with '1' */
      bytecount++;
    }
    
    for (i=0; i < bytecount; i++) 
    {
      fputc(outbuf[i],p_out);
    }
    bytecount=0;                               /* reset buffer index for the next frame */  
    return;
  }
}

void tracebits(char *trace_str, int len, int info)
{
  static int bitcounter = 0;
  int i;
    
  fprintf(p_trace, "@%5i %30s         ", bitcounter, trace_str);
  
  /* Align bitpattern */
  if(len<15)
    for(i=0 ; i<15-len ; i++)
      fputc(' ', p_trace); 

  /* Print bitpattern */
  for(i=0 ; i<len-1 ; i++) {
    if(i%2 == 0) {
      fputc('0', p_trace);
    } else {
      if (0x01 & ( info >> ((len-i)/2-1)))
        fputc('1', p_trace);
      else
        fputc('0', p_trace);
    }
  }

  /* put out the last 1 */
  fprintf(p_trace, "1\n"); 

  bitcounter += len;
}

