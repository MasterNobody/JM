/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

/*!
 ***************************************************************************
 * \file
 *    cabac.h
 *
 * \brief
 *    Headerfile for entropy coding routines
 *
 * \author
 *    Detlev Marpe                                                         \n
 *    Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *
 * \date
 *    21. Oct 2000 (Changes by Tobias Oelbaum 28.08.2001)
 ***************************************************************************
 */

#ifndef _CABAC_H_
#define _CABAC_H_

#include "global.h"


/*******************************************************************************************
 * l o c a l    c o n s t a n t s   f o r   i n i t i a l i z a t i o n   o f   m o d e l s
 *******************************************************************************************
 */
static const int MB_TYPE_Ini[3][11][5]=
{
  {{8,1,50,0,0},  {2,1,50,0,0}, {2,1,50,0,0}, {1,5,50,0,0},   {1,1,50,0,0},  {1,1,50,0,0}, {2,1,50,0,0},  {2,1,50,0,0}, {1,1,50,0,0}, {1,1,50,0,0}, {1,1,50,0,0}},
  {{7,2,50,2,0},  {1,2,50,0,0}, {1,2,50,0,0}, {1,10,50,0,-2}, {30,1,50,0,0}, {2,1,50,0,0}, {5,9,50,4,-4}, {4,3,50,0,0}, {7,2,50,1,0}, {2,1,50,0,0}, {3,2,50,0,0}},
  {{7,2,50,2,0},  {1,2,50,0,0}, {1,2,50,0,0}, {1,10,50,0,-2}, {6,2,50,0,0},  {1,1,50,0,0}, {1,1,50,0,0},  {1,1,50,0,0}, {7,2,50,0,0}, {2,1,50,0,0}, {3,2,50,0,0}}
};

static const int B8_TYPE_Ini[2][9][3]=
{
  {{1,3,50}, {1,2,50}, {2,1,50}, {1,1,50}, {1,2,50}, {1,1,50}, {1,1,50}, {1,1,50}, {1,1,50}},
  {{1,3,50}, {3,1,50}, {3,2,50}, {1,1,50}, {1,1,50}, {1,1,50}, {1,1,50}, {1,1,50}, {1,1,50}}
};

static const int MV_RES_Ini[2][10][3]=
{
  {{9,5,50}, {1,1,50}, {1,1,50}, {4,5,50},  {1,1,50}, {13,5,50}, {1,1,50}, {6,5,50}, {1,1,50},  {1,1,50}},
  {{1,2,50}, {1,4,50}, {1,2,50}, {1,10,50}, {6,5,50}, {4,5,50},  {1,4,50}, {2,5,50}, {1,10,50}, {1,1,50}}
};

static const int REF_NO_Ini[6][3]=
{
  {10,1,50},  {2,1,50}, {1,1,50}, {1,3,50}, {2,1,50}, {1,1,50}
};

static const int DELTA_QP_Ini[4][3]=
{
  {1,1,50}, {1,1,50}, {1,1,50}, {1,1,50}
};

static const int CBP_Ini[2][3][4][5]=
{
  { {{1,4,50,0,0},   {1,2,50,0,0},   {1,2,50,0,0},    {4,3,50,0,0}},
    {{1,2,50,0,0},   {1,3,50,0,0},   {1,3,50,0,0},    {1,3,50,0,0}},
    {{1,1,50,4,2},   {1,1,50,0,0},   {1,2,50,0,0},    {1,2,50,0,0}} }, //!< intra cbp
  { {{1,4,50,1,-1},  {1,1,50,2,0},   {1,1,50,2,0},    {3,1,50,4,0}},
    {{3,1,50,2,0},   {6,5,50,0,-1},  {6,5,50,-2,-2},  {1,2,50,1,0}},
    {{5,2,50,1,0},   {1,1,50,2,1},   {1,1,50,0,0},    {1,2,50,0,0}} }  //!<inter cbp
};


static const int IPR_Ini[9][2][3]=
{
  {{2,1,50},  {1,1,50}},
  {{3,2,50},  {1,1,50}},
  {{1,1,50},  {2,3,50}},
  {{1,1,50},  {2,3,50}},
  {{1,1,50},  {1,1,50}},
  {{2,3,50},  {1,1,50}},
  {{1,1,50},  {1,1,50}},
  {{1,1,50},  {1,1,50}},
  {{1,1,50},  {1,1,50}}
};

static const int Run_Ini[18][2][5]=
{
  {{7,5,64,5,2},  {11,7,64,1,0}},
  {{1,1,64,2,7},    {2,3,64,1,7}},    //!< single scan, inter
  {{1,1,64,3,6},    {2,3,64,0,4}},    //!< single scan, intra
  {{1,1,64,0,0},    {2,3,64,3,4}},    //!< 16x16 DC
  {{7,12,64,0,0},   {5,6,64,-1,1}},   //!< 16x16 AC
  {{7,9,64,0,-1},   {9,7,64,1,4}},    //!< chroma inter DC
  {{5,3,64,6,4},    {11,4,64,-9,-3}}, //!< chroma intra DC
  {{11,9,64,-10,-7},{5,4,64,0,7}},    //!< chroma inter AC
  {{5,6,64,-4,-5},  {3,2,64,-1,1}},   //!< chroma intra AC

  {{6,1,64,1,0},  {9,1,64,2,1}},
  {{1,1,64,11,10},  {9,10,64,-3,-3}}, //!< single scan, inter
  {{8,5,64,3,2},    {11,12,64,-10,-11}},//!< single scan, intra
  {{11,5,64,-4,-3}, {11,5,64,-9,-4}}, //!< 16x16 DC
  {{9,5,64,2,1},    {9,4,64,2,1}},    //!< 16x16 AC
  {{12,5,64,0,0},   {2,1,64,0,0}},    //!< chroma inter DC
  {{9,1,64,-4,0},   {10,3,64,-2,0}},  //!< chroma intra DC
  {{2,1,64,0,0},    {3,2,64,8,5}},    //!< chroma inter AC
  {{10,3,64,-3,-1}, {6,1,64,2,2}},    //!< chroma intra AC
};

static const int Level_Ini[36][4][5]=
{
  {{7,1,64,-4,4}, {9,2,64,-5,5},  {5,1,64,-4,8},  {1,1,64,0,0}},
  {{12,1,64,-5,1},  {11,1,64,-8,1}, {4,1,64,5,10},  {11,10,64,-10,-9}}, //!< single scan, inter
  {{5,1,64,2,10},   {3,1,64,-2,3},  {12,7,64,-11,5},{1,1,64,0,0}},      //!< single scan, intra
  {{9,4,64,-1,7},   {5,3,64,2,9},   {3,2,64,2,9},   {1,1,64,9,8}},      //!< 16x16 DC
  {{10,1,64,-5,0},  {11,2,64,-1,5}, {4,1,64,0,4},   {1,1,64,10,9}},     //!< 16x16 AC
  {{12,1,64,-6,0},  {9,4,64,-5,-1}, {2,1,64,5,11},  {11,12,64,-2,-2}},  //!< chroma inter DC
  {{3,1,64,0,7},    {2,1,64,0,8},   {11,6,64,-10,3},{1,1,64,0,0}},      //!< chroma intra DC
  {{7,2,64,2,0},    {11,4,64,-4,0}, {2,1,64,1,3},   {1,1,64,0,0}},      //!< chroma inter AC
  {{4,1,64,-3,0},   {4,1,64,4,10},  {5,2,64,-4,0},  {1,1,64,0,0}},      //!< chroma intra AC

  {{12,1,64,-9,1},  {8,1,64,-1,7},  {4,1,64,1,10},  {1,1,64,0,0}},
  {{12,1,64,-1,2},  {10,1,64,1,4},  {3,1,64,0,1},   {1,1,64,0,0}},    //!< single scan, inter
  {{10,1,64,-5,2},  {7,1,64,-4,7},  {7,1,64,-6,11}, {1,1,64,0,0}},    //!< single scan, intra
  {{6,1,64,4,2},    {10,3,64,-3,1}, {4,1,64,3,8},   {1,1,64,0,0}},    //!< 16x16 DC
  {{12,1,64,-8,0},  {7,1,64,4,5},   {4,1,64,7,11},  {1,1,64,0,0}},    //!< 16x16 AC
  {{9,1,64,-3,0},   {8,3,64,-5,-2}, {11,8,64,-8,-6},{1,1,64,0,0}},    //!< chroma inter DC
  {{8,1,64,-1,5},   {9,2,64,-2,7},  {11,3,64,-6,8}, {1,1,64,0,0}},    //!< chroma intra DC
  {{9,2,64,0,-1},   {9,5,64,-2,-3}, {3,2,64,0,0},   {1,1,64,0,0}},    //!< chroma inter AC
  {{12,1,64,-2,2},  {3,1,64,0,1},   {9,4,64,-1,7},  {1,1,64,0,0}},    //!< chroma intra AC

  {{7,1,64,0,7},  {6,1,64,-1,6}, {4,1,64,1,10},  {1,1,64,0,0}},
  {{9,1,64,-2,3},   {9,2,64,-1,3},  {5,2,64,0,2},   {1,1,64,0,0}},    //!< single scan, inter
  {{9,2,64,-5,3},   {7,1,64,-3,4},  {10,1,64,-9,7}, {1,1,64,0,0}},    //!< single scan, intra
  {{10,3,64,1,3},   {10,3,64,-3,2}, {7,3,64,0,7},   {1,1,64,0,0}},    //!< 16x16 DC
  {{11,2,64,-6,1},  {9,2,64,2,10},  {7,3,64,0,6},   {1,1,64,0,0}},    //!< 16x16 AC
  {{5,3,64,-4,-2},  {2,1,64,5,2},   {11,7,64,-2,-2},{1,1,64,0,0}},    //!< chroma inter DC
  {{4,1,64,1,6},    {11,3,64,-9,0}, {3,1,64,2,10},  {1,1,64,0,0}},    //!< chroma intra DC
  {{5,2,64,6,2},    {4,3,64,4,0},   {6,5,64,5,2},   {1,1,64,0,0}},    //!< chroma inter AC
  {{6,1,64,3,3},    {11,3,64,-1,4}, {2,1,64,3,6},   {1,1,64,0,0}},    //!< chroma intra AC

  {{11,3,64,-8,7},  {4,1,64,-2,6},  {10,3,64,-8,4},  {1,1,64,0,0}},
  {{3,1,64,8,11},   {7,4,64,2,6},   {7,3,64,0,6},   {1,1,64,0,0}},    //!< single scan, inter
  {{8,3,64,-7,9},   {7,3,64,0,5},   {2,1,64,-1,4},  {1,1,64,0,0}},    //!< single scan, intra
  {{9,4,64,0,7},    {11,4,64,-8,1}, {12,5,64,-11,-3},{1,1,64,0,0}},   //!< 16x16 DC
  {{9,4,64,-4,4},   {3,1,64,4,9},   {9,5,64,-7,2},  {1,1,64,0,0}},    //!< 16x16 AC
  {{1,1,64,2,10},   {9,7,64,-5,4},  {10,7,64,-8,-4},{1,1,64,0,0}},    //!< chroma inter DC
  {{11,7,64,-10,-3},{11,6,64,-10,-2},{2,1,64,-1,3}, {1,1,64,0,0}},    //!< chroma intra DC
  {{12,7,64,-3,3},  {7,5,64,0,4},   {11,9,64,-5,-2},{1,1,64,0,0}},    //!< chroma inter AC
  {{8,3,64,2,8},    {8,3,64,-3,3},  {2,1,64,5,9},   {1,1,64,0,0}}     //!< chroma intra AC
};

static const int Coeff_Count_Ini[9][6][5]=
{
  { {5,6,64,-4,3},  {5,1,64,0,2},   {7,10,64,-6,1}, {1,1,64,0,0},  
    {7,8,64,-6,-1}, {5,4,64,-2,7}},                                   //!< double scan
  { {12,11,64,-8,-6},{1,1,64,0,2},  {9,10,64,-6,-2},{4,5,64,-3,7}, 
    {4,3,64,-2,4},  {12,5,64,-10,2}},                                 //!< single scan, inter
  { {5,4,64,-2,-2}, {2,3,64,-1,9},  {8,9,64,-7,3},  {2,5,64,-1,7}, 
    {5,6,64,-4,6},  {1,1,64,0,11}},                                   //!< single scan, intra
  { {1,3,64,0,9},   {1,1,64,0,0},   {1,1,64,0,0},   {1,1,64,0,0},  
    {3,8,64,-2,4},  {5,11,64,-3,-4}},                                 //!< 16x16 DC
  { {11,3,64,-10,-2},{6,1,64,5,5},  {10,7,64,-9,-5},{1,1,64,0,0}, 
    {10,7,64,-7,-3},{9,7,64,-8,-5}},                                  //!< 16x16 AC
  { {8,11,64,-2,0}, {7,8,64,-4,-4}, {1,1,64,0,0},   {1,1,64,0,0}, 
    {5,1,64,0,2},   {12,5,64,-5,1}},                                  //!< chroma inter DC
  { {2,9,64,-1,3},  {3,8,64,-2,4},  {1,1,64,0,0},   {1,1,64,0,0},  
    {1,1,64,0,8},   {2,1,64,1,7}},                                    //!< chroma intra DC
  { {2,1,64,-1,0},  {8,3,64,-1,3},  {1,1,64,0,0},   {1,1,64,0,0},  
    {9,7,64,-8,-4}, {11,4,64,-10,-2}},                                //!< chroma inter AC
  { {5,4,64,-4,0},  {11,4,64,-6,8}, {1,1,64,0,0},   {1,1,64,0,0},  
    {9,8,64,-8,-2}, {5,2,64,-3,3}}                                    //!< chroma intra AC
};

static const int ABT_Run_Ini[12][5][5]=
{
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x8, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x4,4x8, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode4x4, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x8, intra
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x4,4x8, intra
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode4x4, intra
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x8, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x4,4x8, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode4x4, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x8, intra
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }, //!< Mode8x4,4x8, intra
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} }  //!< Mode4x4, intra
};

static const int ABT_Coeff_Count_Ini[6][8][5]=
{
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},
    {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} },  //!< Mode8x8, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},
    {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} },  //!< Mode8x4,4x8, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},
    {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} },  //!< Mode4x4, inter
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},
    {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} },  //!< Mode8x8, intra
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},
    {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} },  //!< Mode8x4,4x8, intra
  { {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},
    {1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0},{1,1,64,0,0} },  //!< Mode4x4, intra
};

/***********************************************************************
 * L O C A L L Y   D E F I N E D   F U N C T I O N   P R O T O T Y P E S
 ***********************************************************************
 */



unsigned int unary_bin_decode(DecodingEnvironmentPtr dep_dp,
                              BiContextTypePtr ctx,
                              int ctx_offset);

unsigned int unary_level_decode(DecodingEnvironmentPtr dep_dp,
                                BiContextTypePtr ctx);

unsigned int unary_mv_decode(DecodingEnvironmentPtr dep_dp,
                             BiContextTypePtr ctx,
                             unsigned int max_bin);

unsigned int unary_bin_max_decode(DecodingEnvironmentPtr dep_dp,
                                  BiContextTypePtr ctx,
                                  int ctx_offset,
                                  unsigned int max_symbol);

unsigned int unary_bin_max_decodeABT(DecodingEnvironmentPtr dep_dp,
                                     BiContextTypePtr ctx,
                                     int ctx_offset,
                                     unsigned int max_symbol);

#endif  // _CABAC_H_

