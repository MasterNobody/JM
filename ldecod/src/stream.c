/************************************************************************
*
*  stream.c for H.26L decoder, reads MSB in stream first. 
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

#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "stream.h"


/************************************************************************
*
*  Routine:     write_frame()
*
*  Description: Write decoded frame to output file
*                              
*               
*  Input:       Image parameters and postfilter
*
*  Output:             
*                    
************************************************************************/
void write_frame(struct img_par *img,int postfilter,FILE *p_out) 
{  
  int i,j;

  if(postfilter)
  {
    for(i=0;i<img->height;i++)
      for(j=0;j<img->width;j++)
        fputc((byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgY_pf[i][j])),p_out);
    
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)    
        fputc((byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV_pf[0][i][j])),p_out);
      
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)    
        fputc((byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV_pf[1][i][j])),p_out);
  }
  else
  { 
    for(i=0;i<img->height;i++)
      for(j=0;j<img->width;j++)
        fputc((byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgY[i][j])),p_out);
    
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)    
        fputc((byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV[0][i][j])),p_out);
      
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)    
        fputc((byte)mmin(MAX_PIX_VAL,mmax(MIN_PIX_VAL,imgUV[1][i][j])),p_out);
  }     
}


/************************************************************************
*
*  Routine:     findinfo()
*
*  Description: find lenght and info of one codeword
*                                             
*  Input:       Stream buffer, bit offset from start of frame and input stream file
*
*  Output:      bit offset from start of frame and info (through pointer)     
*                    
************************************************************************/
int findinfo(byte buffer[],int totbitoffset,int *info,FILE *p_in)   
{   
  int byteoffset;                                                 /* byte from start of frame */
  int bitoffset;                                                  /* bit from start of byte */                                                 
  int ctr_bit;                                                    /* control bit for current bit posision */  
  int bitcounter=1;
  
  byteoffset= totbitoffset/8;                                    
  bitoffset= 7-(totbitoffset%8);                                  /* Find bit offset in current byte, start with MSB  */
  
    if (bitoffset==7)                                             /* first bit in new byte */ 
    {
        buffer[byteoffset]=fgetc( p_in );                         /* read new byte from stream */
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


/************************************************************************
*
*  Routine:     read_frame()
*
*  Description: read one frame
*                                             
*  Input:       Input stream file
*
*  Output:      Number of codewords   
*                    
************************************************************************/
int read_frame(FILE *p_in)                      
{
  static int frame_bitoffset=0;                                   /* number of bit from start frame */
  static first=TRUE;
  
  static byte buf[MAX_FRAME_SIZE];                                /* bitstream for one frame        */
  
  int info;                                                       /* one infoword                   */ 
  int len;                                                        /* length of codeword             */
  int no_code_word;
  int code_word_ctr=0;
  static int info_sc;                                             /* info part of start code        */
  static finish = FALSE;                                          /* TRUE if end of sequence (eos)  */
  
  if (finish)
    return 0;                                                     /* indicates eos                  */
  /* 
     We don't know when a frame is finish until the next startcode, which belong to the next frame, 
     therefore we have to add the stored one for all frames except the first  
  */
  if (first==TRUE)                                                  
    first=FALSE;
  else                                                             
  {
    len_arr[0]=31;
    info_arr[0]=info_sc;
    code_word_ctr++;
  }

  for (;;)                                                  /* repeate until new start/end code is found */
  {  
    len =  findinfo(buf,frame_bitoffset,&info,p_in);        /* read a new word */ 
  
    if (len >= LEN_STARTCODE && code_word_ctr > 0)          /* Added the ability to handle byte aligned start code. KOL 03/15/99 */
    {
      if (info & 0x01)                                      /* eos found, stored to next time read_frame() is called */
        finish=TRUE;
      
      info_ctr=0;
      buf[0]=buf[(frame_bitoffset+LEN_STARTCODE)/8];        /* highest byte used for new frame also   */
      frame_bitoffset=(frame_bitoffset+LEN_STARTCODE)%8;    /* bitoffest for first byte in new frame  */
      no_code_word=code_word_ctr;
      info_sc=info;                                         /* store start code for next frame        */
      code_word_ctr=0;
        
      return no_code_word;
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
}


/************************************************************************
*
*  Routine:     get_info()
*
*  Description: read one info word and the lenght of the codeword
*                                             
*  Output:      info, and codeword length through pointer   
*                    
************************************************************************/
int get_info(int *len)                                            /* read next infoword */
{  
  *len = len_arr[info_ctr];                                       /* lenght of a codeword */
  return info_arr[info_ctr++];                                    /* return info part of codeword */
}
                                                