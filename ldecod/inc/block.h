/************************************************************************
*
*  block.h for H.26L decoder. 
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
#ifndef BLOCK_H
#define BLOCK_H

#include "global.h"

const int JQQ = 1048576;
const int JQQ2 = 524288;

int const MAP[4][4]=
{
  {0,2,4,5},
  {1,3,5,5},
  {2,4,5,5},
  {3,5,5,5},
};

extern const int JQ1[];
extern const byte FILTER_STR[32][4];/*defined in image.h */


#endif