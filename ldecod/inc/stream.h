/************************************************************************
*
*  stream.h for H.26L decoder. 
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
#ifndef STREAM_H
#define STREAM_H

#include <global.h>

#define  MAX_INFO_WORD  300000       /* for one frame */
#define  MAX_FRAME_SIZE 200000      /* for bitstream */

void fillbuf(byte *,FILE *p_in);
int findinfo(byte *, int, int* ,FILE *p_in);

int  info_ctr=0;

int info_arr[MAX_INFO_WORD];
int len_arr[MAX_INFO_WORD];

#endif








