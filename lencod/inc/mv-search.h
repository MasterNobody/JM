/************************************************************************
*
*  block.h for H.26L encoder. 
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
#ifndef _BLOCK_H_
#define _BLOCK_H_

/* convert from H.263 QP to H.26L quant given by: quant=pow(2,QP/6) */
const int QP2QUANT[32]= 
{
   1, 1, 1, 1, 2, 2, 2, 2,
   3, 3, 3, 4, 4, 4, 5, 6,
   6, 7, 8, 9,10,11,13,14,
  16,17,20,23,25,29,32,36
};


#endif
