/************************************************************************
*
*  image.h for H.26L encoder. 
*  Copyright (C) 1999  Telenor Satellite Services, Norway
*  
*  Contacts: 
*  Inge Lille-Lang�y               <inge.lille-langoy@telenor.com>
* 
*
*  Telenor Satellite Services 
*  P.O.Box 6914 St.Olavs plass                       
*  N-0130 Oslo, Norway                 
*  
************************************************************************/
#ifndef _IMAGE_H_
#define _IMAGE_H_

/* QP dependent filter strenght clipping for loopfilter */
const byte FILTER_STR[32][4] = 
{
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
  {0,0,0,0},{0,0,0,1},{0,0,0,1},{0,0,1,1},{0,0,1,1},{0,0,1,1},{0,1,1,1},{0,1,1,1},
  {0,1,1,1},{0,1,1,2},{0,1,1,2},{0,1,1,2},{0,1,1,2},{0,1,2,3},{0,1,2,3},{0,1,2,3},
  {0,1,2,4},{0,2,3,4},{0,2,3,5},{0,2,3,5},{0,2,3,5},{0,2,4,7},{0,3,5,8},{0,3,5,9},
};


/* TAPs used in the oneforthpix()routine */
 const int ONE_FOURTH_TAP[3][2] =
  {
    {20,20},
    {-5,-4},
    { 1, 0},
  };

 const byte LIM[32] =
{  
   7,  8,  9, 10, 11, 12, 14, 16,
  18, 20, 22, 25, 28, 31, 35, 39,
  44, 49, 55, 62, 69, 78, 88, 98,
  110,124,139,156,175,197,221,248
};

#endif
