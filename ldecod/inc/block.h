/*!
 ************************************************************************
 * \file block.h
 *
 * \author
 *  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>    \n
 *  Telenor Satellite Services                                         \n
 *  P.O.Box 6914 St.Olavs plass                                        \n
 *  N-0130 Oslo, Norway
 *
 ************************************************************************
 */

#ifndef _BLOCK_H_
#define _BLOCK_H_

#include "global.h"

const int JQQ = 1048576;
const int JQQ2 = 524288;
const int JQQ3= 349525;
const int JQQ4= 174762;

int const MAP[4][4]=
{
  {0,2,4,5},
  {1,3,5,5},
  {2,4,5,5},
  {3,5,5,5},
};

extern const int JQ1[];
extern const int JQ[32];
extern const byte FILTER_STR[32][4];//!< defined in image.h
extern const byte QP_SCALE_CR[32] ;

#endif
