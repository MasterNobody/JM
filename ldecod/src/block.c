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
 ***********************************************************************
 *  \file
 *      block.c
 *
 *  \brief
 *      Block functions
 *
 *  \author
 *      Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Inge Lille-Lang�y          <inge.lille-langoy@telenor.com>
 *      - Rickard Sjoberg            <rickard.sjoberg@era.ericsson.se>
 ***********************************************************************
 */

#include "contributors.h"

#include <stdlib.h>
#include <math.h>

#include "block.h"


#define Q_BITS          16

static const int quant_coef[6][4][4] = {
  {{16384,10486,16384,10486},{10486, 6453,10486, 6453},{16384,10486,16384,10486},{10486, 6453,10486, 6453}},
  {{14564, 9118,14564, 9118},{ 9118, 5785, 9118, 5785},{14564, 9118,14564, 9118},{ 9118, 5785, 9118, 5785}},
  {{13107, 8389,13107, 8389},{ 8389, 5243, 8389, 5243},{13107, 8389,13107, 8389},{ 8389, 5243, 8389, 5243}},
  {{11916, 7490,11916, 7490},{ 7490, 4660, 7490, 4660},{11916, 7490,11916, 7490},{ 7490, 4660, 7490, 4660}},
  {{10486, 6554,10486, 6554},{ 6554, 4194, 6554, 4194},{10486, 6554,10486, 6554},{ 6554, 4194, 6554, 4194}},
  {{9362,  5825, 9362, 5825},{ 5825, 3728, 5825, 3728},{ 9362, 5825, 9362, 5825},{ 5825, 3728, 5825, 3728}}
};


#ifndef USE_6_INTRA_MODES

// Notation for comments regarding prediction and predictors.
// The pels of the 4x4 block are labelled a..p. The predictor pels above
// are labelled A..H, from the left I..P, and from above left X, as follows:
//
//  X A B C D E F G H
//  I a b c d
//  J e f g h
//  K i j k l
//  L m n o p
//  M
//  N
//  O
//  P
//

// Predictor array index definitions
#define P_X (PredPel[0])
#define P_A (PredPel[1])
#define P_B (PredPel[2])
#define P_C (PredPel[3])
#define P_D (PredPel[4])
#define P_E (PredPel[5])
#define P_F (PredPel[6])
#define P_G (PredPel[7])
#define P_H (PredPel[8])
#define P_I (PredPel[9])
#define P_J (PredPel[10])
#define P_K (PredPel[11])
#define P_L (PredPel[12])
#define P_M (PredPel[13])
#define P_N (PredPel[14])
#define P_O (PredPel[15])
#define P_P (PredPel[16])

/*!
 ***********************************************************************
 * \brief
 *    makes and returns 4x4 blocks with all 5 intra prediction modes
 *
 * \return
 *    DECODING_OK   decoding of intraprediction mode was sucessfull            \n
 *    SEARCH_SYNC   search next sync element as errors while decoding occured
 ***********************************************************************
 */

int intrapred(
  struct img_par *img,  /*!< image parameters */
  int ioff,             /*!< ?? */
  int joff,             /*!< ?? */
  int img_block_x,      /*!< ?? */
  int img_block_y)      /*!< ?? */
{
  int i,j;
  int s0;
  int img_y,img_x;
  int PredPel[17];  // array of predictor pels

  int block_available_up;
  int block_available_up_right;
  int block_available_left;
  int block_available_left_down;

  byte predmode = img->ipredmode[img_block_x+1][img_block_y+1];

  img_x=img_block_x*4;
  img_y=img_block_y*4;

  block_available_up = (img->ipredmode[img_block_x+1][img_block_y] >=0);
  block_available_up_right  = (img->ipredmode[img_x/BLOCK_SIZE+2][img_y/BLOCK_SIZE] >=0); // ???
  block_available_left = (img->ipredmode[img_block_x][img_block_y+1] >=0);
  block_available_left_down = (img->ipredmode[img_x/BLOCK_SIZE][img_y/BLOCK_SIZE+2] >=0); // ???

  i = (img_x & 15);
  j = (img_y & 15);
  if (block_available_up_right)
  {
    if ((i == 4  && j == 4) ||
        (i == 12 && j == 4) ||
        (i == 12 && j == 8) ||
        (i == 4  && j == 12) ||
        (i == 12 && j == 12))
    {
      block_available_up_right = 0;
    }
  }

  if (block_available_left_down)
  {
    if (!(i == 0 && j == 0) &&
        !(i == 8 && j == 0) &&
        !(i == 0 && j == 4) &&
        !(i == 0 && j == 8) &&
        !(i == 8 && j == 8))
    {
      block_available_left_down = 0;
    }
  }

  // form predictor pels
  if (block_available_up)
  {
    P_A = imgY[img_y-1][img_x+0];
    P_B = imgY[img_y-1][img_x+1];
    P_C = imgY[img_y-1][img_x+2];
    P_D = imgY[img_y-1][img_x+3];

    if (block_available_up_right)
    {
      P_E = imgY[img_y-1][img_x+4];
      P_F = imgY[img_y-1][img_x+5];
      P_G = imgY[img_y-1][img_x+6];
      P_H = imgY[img_y-1][img_x+7];
    }
    else
    {
      P_E = P_F = P_G = P_H = P_D;
    }
  }
  else
  {
    P_A = P_B = P_C = P_D = P_E = P_F = P_G = P_H = 128;
  }

  if (block_available_left)
  {
    P_I = imgY[img_y+0][img_x-1];
    P_J = imgY[img_y+1][img_x-1];
    P_K = imgY[img_y+2][img_x-1];
    P_L = imgY[img_y+3][img_x-1];

    if (block_available_left_down)
    {
      P_M = imgY[img_y+4][img_x-1];
      P_N = imgY[img_y+5][img_x-1];
      P_O = imgY[img_y+6][img_x-1];
      P_P = imgY[img_y+7][img_x-1];
    }
    else
    {
      P_M = P_N = P_O = P_P = P_L;
    }
  }
  else
  {
    P_I = P_J = P_K = P_L = P_M = P_N = P_O = P_P = 128;
  }

  if (block_available_up && block_available_left)
  {
    P_X = imgY[img_y-1][img_x-1];
  }
  else
  {
    P_X = 128;
  }

  
  switch (predmode)
  {
  case DC_PRED:                         /* DC prediction */

    s0 = 0;
    if (block_available_up && block_available_left)
    {   
      // no edge
      s0 = (P_A + P_B + P_C + P_D + P_I + P_J + P_K + P_L + 4)/(2*BLOCK_SIZE);
    }
    else if (!block_available_up && block_available_left)
    {
      // upper edge
      s0 = (P_I + P_J + P_K + P_L + 2)/BLOCK_SIZE;             
    }
    else if (block_available_up && !block_available_left)
    {
      // left edge
      s0 = (P_A + P_B + P_C + P_D + 2)/BLOCK_SIZE;             
    }
    else //if (!block_available_up && !block_available_left)
    {
      // top left corner, nothing to predict from
      s0 = 128;                           
    }

    for (j=0; j < BLOCK_SIZE; j++)
    {
      for (i=0; i < BLOCK_SIZE; i++)
      {
        // store DC prediction
        img->mpr[i+ioff][j+joff] = s0;
      }
    }
    break;

  case VERT_PRED:                       /* vertical prediction from block above */
    for(j=0;j<BLOCK_SIZE;j++)
      for(i=0;i<BLOCK_SIZE;i++)
        img->mpr[i+ioff][j+joff]=imgY[img_y-1][img_x+i];/* store predicted 4x4 block */
    break;

  case HOR_PRED:                        /* horisontal prediction from left block */
    for(j=0;j<BLOCK_SIZE;j++)
      for(i=0;i<BLOCK_SIZE;i++)
        img->mpr[i+ioff][j+joff]=imgY[img_y+j][img_x-1]; /* store predicted 4x4 block */
    break;

  case DIAG_PRED_SE:
    img->mpr[0+ioff][3+joff] = (P_L + 2*P_K + P_J + 2) / 4; 
    img->mpr[0+ioff][2+joff] =
    img->mpr[1+ioff][3+joff] = (P_K + 2*P_J + P_I + 2) / 4; 
    img->mpr[0+ioff][1+joff] =
    img->mpr[1+ioff][2+joff] = 
    img->mpr[2+ioff][3+joff] = (P_J + 2*P_I + P_X + 2) / 4; 
    img->mpr[0+ioff][0+joff] =
    img->mpr[1+ioff][1+joff] =
    img->mpr[2+ioff][2+joff] =
    img->mpr[3+ioff][3+joff] = (P_I + 2*P_X + P_A + 2) / 4; 
    img->mpr[1+ioff][0+joff] =
    img->mpr[2+ioff][1+joff] =
    img->mpr[3+ioff][2+joff] = (P_X + 2*P_A + P_B + 2) / 4;
    img->mpr[2+ioff][0+joff] =
    img->mpr[3+ioff][1+joff] = (P_A + 2*P_B + P_C + 2) / 4;
    img->mpr[3+ioff][0+joff] = (P_B + 2*P_C + P_D + 2) / 4;
    break;

  case DIAG_PRED_NE:
    img->mpr[0+ioff][0+joff] = (P_A + P_C + P_I + P_K + 2*(P_B + P_J) + 4) / 8;
    img->mpr[1+ioff][0+joff] = 
    img->mpr[0+ioff][1+joff] = (P_B + P_D + P_J + P_L + 2*(P_C + P_K) + 4) / 8;
    img->mpr[2+ioff][0+joff] =
    img->mpr[1+ioff][1+joff] =
    img->mpr[0+ioff][2+joff] = (P_C + P_E + P_K + P_M + 2*(P_D + P_L) + 4) / 8;
    img->mpr[3+ioff][0+joff] = 
    img->mpr[2+ioff][1+joff] = 
    img->mpr[1+ioff][2+joff] = 
    img->mpr[0+ioff][3+joff] = (P_D + P_F + P_L + P_N + 2*(P_E + P_M) + 4) / 8;
    img->mpr[3+ioff][1+joff] = 
    img->mpr[2+ioff][2+joff] = 
    img->mpr[1+ioff][3+joff] = (P_E + P_G + P_M + P_O + 2*(P_F + P_N) + 4) / 8;
    img->mpr[3+ioff][2+joff] = 
    img->mpr[2+ioff][3+joff] = (P_F + P_H + P_N + P_P + 2*(P_G + P_O) + 4) / 8;
    img->mpr[3+ioff][3+joff] = (P_G + P_O + P_H + P_P + 2) / 4;
    break;

  case  DIAG_PRED_SSE:/* diagonal prediction -22.5 deg to horizontal plane */
    img->mpr[0+ioff][0+joff] = 
    img->mpr[1+ioff][2+joff] = (P_X + P_A + 1) / 2;
    img->mpr[1+ioff][0+joff] = 
    img->mpr[2+ioff][2+joff] = (P_A + P_B + 1) / 2;
    img->mpr[2+ioff][0+joff] = 
    img->mpr[3+ioff][2+joff] = (P_B + P_C + 1) / 2;
    img->mpr[3+ioff][0+joff] = (P_C + P_D + 1) / 2;
    img->mpr[0+ioff][1+joff] = 
    img->mpr[1+ioff][3+joff] = (P_I + 2*P_X + P_A + 2) / 4;
    img->mpr[1+ioff][1+joff] = 
    img->mpr[2+ioff][3+joff] = (P_X + 2*P_A + P_B + 2) / 4;
    img->mpr[2+ioff][1+joff] = 
    img->mpr[3+ioff][3+joff] = (P_A + 2*P_B + P_C + 2) / 4;
    img->mpr[3+ioff][1+joff] = (P_B + 2*P_C + P_D + 2) / 4;
    img->mpr[0+ioff][2+joff] = (P_X + 2*P_I + P_J + 2) / 4;
    img->mpr[0+ioff][3+joff] = (P_I + 2*P_J + P_K + 2) / 4;
    break;

  case  DIAG_PRED_NNE:/* diagonal prediction -22.5 deg to horizontal plane */
    img->mpr[0+ioff][0+joff] = (2*(P_A + P_B + P_K) + P_J + P_L + 4) / 8;
    img->mpr[1+ioff][0+joff] = 
    img->mpr[0+ioff][2+joff] = (P_B + P_C + 1) / 2;
    img->mpr[2+ioff][0+joff] = 
    img->mpr[1+ioff][2+joff] = (P_C + P_D + 1) / 2;
    img->mpr[3+ioff][0+joff] = 
    img->mpr[2+ioff][2+joff] = (P_D + P_E + 1) / 2;
    img->mpr[3+ioff][2+joff] = (P_E + P_F + 1) / 2;
    img->mpr[0+ioff][1+joff] = (2*(P_B + P_L) + P_A + P_C + P_K + P_M + 4) / 8;
    img->mpr[1+ioff][1+joff] = 
    img->mpr[0+ioff][3+joff] = (P_B + 2*P_C + P_D + 2) / 4;
    img->mpr[2+ioff][1+joff] = 
    img->mpr[1+ioff][3+joff] = (P_C + 2*P_D + P_E + 2) / 4;
    img->mpr[3+ioff][1+joff] = 
    img->mpr[2+ioff][3+joff] = (P_D + 2*P_E + P_F + 2) / 4;
    img->mpr[3+ioff][3+joff] = (P_E + 2*P_F + P_G + 2) / 4;
    break;

  case  DIAG_PRED_ENE:/* diagonal prediction -22.5 deg to horizontal plane */
    img->mpr[0+ioff][0+joff] = (2*(P_C + P_I + P_J) + P_B + P_D + 4) / 8;
    img->mpr[1+ioff][0+joff] = (2*(P_D + P_J) + P_C + P_E + P_I + P_K + 4) / 8;
    img->mpr[2+ioff][0+joff] = 
    img->mpr[0+ioff][1+joff] = (2*(P_E + P_J + P_K) + P_D + P_F + 4) / 8;
    img->mpr[3+ioff][0+joff] = 
    img->mpr[1+ioff][1+joff] = (2*(P_F + P_K) + P_E + P_G + P_J + P_L + 4) / 8;
    img->mpr[2+ioff][1+joff] = 
    img->mpr[0+ioff][2+joff] = (2*(P_G + P_K + P_L) + P_F + P_H + 4) / 8;
    img->mpr[3+ioff][1+joff] = 
    img->mpr[1+ioff][2+joff] = (2*(P_H + P_L) + P_G + P_H + P_K + P_L + 4) / 8;
    img->mpr[3+ioff][2+joff] = 
    img->mpr[1+ioff][3+joff] = (P_L + (P_M << 1) + P_N + 2) / 4;
    img->mpr[0+ioff][3+joff] = 
    img->mpr[2+ioff][2+joff] = (P_G + P_H + P_L + P_M + 2) / 4;
    img->mpr[2+ioff][3+joff] = (P_M + P_N + 1) / 2;
    img->mpr[3+ioff][3+joff] = (P_M + 2*P_N + P_O + 2) / 4;
    break;

  case  DIAG_PRED_ESE:/* diagonal prediction -22.5 deg to horizontal plane */
    img->mpr[0+ioff][0+joff] = 
    img->mpr[2+ioff][1+joff] = (P_X + P_I + 1) / 2;
    img->mpr[1+ioff][0+joff] = 
    img->mpr[3+ioff][1+joff] = (P_I + 2*P_X + P_A + 2) / 4;
    img->mpr[2+ioff][0+joff] = (P_X + 2*P_A + P_B + 2) / 4;
    img->mpr[3+ioff][0+joff] = (P_A + 2*P_B + P_C + 2) / 4;
    img->mpr[0+ioff][1+joff] = 
    img->mpr[2+ioff][2+joff] = (P_I + P_J + 1) / 2;
    img->mpr[1+ioff][1+joff] = 
    img->mpr[3+ioff][2+joff] = (P_X + 2*P_I + P_J + 2) / 4;
    img->mpr[0+ioff][2+joff] = 
    img->mpr[2+ioff][3+joff] = (P_J + P_K + 1) / 2;
    img->mpr[1+ioff][2+joff] = 
    img->mpr[3+ioff][3+joff] = (P_I + 2*P_J + P_K + 2) / 4;
    img->mpr[0+ioff][3+joff] = (P_K + P_L + 1) / 2;
    img->mpr[1+ioff][3+joff] = (P_J + 2*P_K + P_L + 2) / 4;
    break;

  default:
    printf("Error: illegal prediction mode input: %d\n",predmode);
    return SEARCH_SYNC;
    break;
  }

  return DECODING_OK;
}

#else // !USE_6_INTRA_MODES

/*!
 ***********************************************************************
 * \brief
 *    makes and returns 4x4 blocks with all 5 intra prediction modes
 *
 * \return
 *    DECODING_OK   decoding of intraprediction mode was sucessfull            \n
 *    SEARCH_SYNC   search next sync element as errors while decoding occured
 ***********************************************************************
 */
int intrapred(struct img_par *img,  //!< image parameters
              int ioff,             //!< ??
              int joff,             //!< ??
              int img_block_x,      //!< ??
              int img_block_y)      //!< ??
{
  int js0=0,js1,js2,i,j;
  int img_y=0,img_x=0;
  int ia[7];

  byte predmode = img->ipredmode[img_block_x+1][img_block_y+1];

  int block_available_up = (img->ipredmode[img_block_x+1][img_block_y] >=0);
  int block_available_left = (img->ipredmode[img_block_x][img_block_y+1] >=0);

  img_x=img_block_x*4;
  img_y=img_block_y*4;

  for (i=0; i<7; i++)
    ia[i]=0;

  switch (predmode)
  {
  case DC_PRED:                         // DC prediction

    js1=0;
    js2=0;
    for (i=0;i<BLOCK_SIZE;i++)
    {
      if (block_available_up)
        js1=js1+imgY[img_y-1][img_x+i];
      if (block_available_left)
        js2=js2+imgY[img_y+i][img_x-1];
    }
    if(block_available_left && block_available_up)
      js0=(js1+js2+4)/8;                // make DC prediction from both block above and to the left
    if(block_available_left && !block_available_up)
      js0=(js2+2)/4;                    // make DC prediction from block to the left
    if(!block_available_left && block_available_up)
      js0=(js1+2)/4;                    // make DC prediction from block above
    if(!block_available_left && !block_available_up)
      js0=128;                          // current block is the upper left
    // no block to make DC pred.from
    for(i=0;i<BLOCK_SIZE;i++)
      for(j=0;j<BLOCK_SIZE;j++)
        img->mpr[i+ioff][j+joff]=js0;             // store predicted 4x4 block
    break;

  case VERT_PRED:                       // vertical prediction from block above
    for(j=0;j<BLOCK_SIZE;j++)
      for(i=0;i<BLOCK_SIZE;i++)
        img->mpr[i+ioff][j+joff]=imgY[img_y-1][img_x+i];// store predicted 4x4 block
    break;

  case HOR_PRED:                        // horisontal prediction from left block
    for(j=0;j<BLOCK_SIZE;j++)
      for(i=0;i<BLOCK_SIZE;i++)
        img->mpr[i+ioff][j+joff]=imgY[img_y+j][img_x-1]; // store predicted 4x4 block
    break;

  case DIAG_PRED_LR_45:       // diagonal prediction from left to right 45 degree
    ia[0]=(imgY[img_y+3][img_x-1]+2*imgY[img_y+2][img_x-1]+imgY[img_y+1][img_x-1]+2)/4;
    ia[1]=(imgY[img_y+2][img_x-1]+2*imgY[img_y+1][img_x-1]+imgY[img_y+0][img_x-1]+2)/4;
    ia[2]=(imgY[img_y+1][img_x-1]+2*imgY[img_y+0][img_x-1]+imgY[img_y-1][img_x-1]+2)/4;
    ia[3]=(imgY[img_y+0][img_x-1]+2*imgY[img_y-1][img_x-1]+imgY[img_y-1][img_x+0]+2)/4;
    ia[4]=(imgY[img_y-1][img_x-1]+2*imgY[img_y-1][img_x+0]+imgY[img_y-1][img_x+1]+2)/4;
    ia[5]=(imgY[img_y-1][img_x+0]+2*imgY[img_y-1][img_x+1]+imgY[img_y-1][img_x+2]+2)/4;
    ia[6]=(imgY[img_y-1][img_x+1]+2*imgY[img_y-1][img_x+2]+imgY[img_y-1][img_x+3]+2)/4;
    for(i=0;i<BLOCK_SIZE;i++)
      for(j=0;j<BLOCK_SIZE;j++)
        img->mpr[i+ioff][j+joff]=ia[i-j+3];
    break;

  case DIAG_PRED_RL: // diagonal prediction 22.5 deg to vertical plane
    ia[0]=(imgY[img_y-1][img_x+0]+imgY[img_y-1][img_x+1])/2;
    ia[1]= imgY[img_y-1][img_x+1];
    ia[2]=(imgY[img_y-1][img_x+1]+imgY[img_y-1][img_x+2])/2;
    ia[3]= imgY[img_y-1][img_x+2];
    ia[4]=(imgY[img_y-1][img_x+2]+imgY[img_y-1][img_x+3])/2;
    ia[5]= imgY[img_y-1][img_x+3];
    for(i=0;i<BLOCK_SIZE;i++)
      for(j=0;j<BLOCK_SIZE;j++)
        img->mpr[i+ioff][j+joff]=ia[MAP[j][i]];
    break;

  case  DIAG_PRED_LR:// diagonal prediction -22.5 deg to horizontal plane
    ia[0]=(imgY[img_y+0][img_x-1]+imgY[img_y+1][img_x-1])/2;
    ia[1]= imgY[img_y+1][img_x-1];
    ia[2]=(imgY[img_y+1][img_x-1]+imgY[img_y+2][img_x-1])/2;
    ia[3]= imgY[img_y+2][img_x-1];
    ia[4]=(imgY[img_y+2][img_x-1]+imgY[img_y+3][img_x-1])/2;
    ia[5]= imgY[img_y+3][img_x-1];
    for(i=0;i<BLOCK_SIZE;i++)
      for(j=0;j<BLOCK_SIZE;j++)
        img->mpr[i+ioff][j+joff]=ia[MAP[i][j]];
    break;

  default:
    printf("Error: illegal prediction mode input: %d\n",predmode);
    return SEARCH_SYNC;
    break;
  }
  return DECODING_OK;
}

#endif

/*!
 ***********************************************************************
 * \return
 *    best SAD
 ***********************************************************************
 */
int intrapred_luma_2(struct img_par *img, //!< image parameters
                     int predmode)        //!< prediction mode
{
  int s0=0,s1,s2;

  int i,j;

  int ih,iv;
  int ib,ic,iaa;

  int mb_nr = img->current_mb_nr;
  int mb_width = img->width/16;
  int mb_available_up = (img->mb_y == 0) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width].slice_nr);
  int mb_available_left = (img->mb_x == 0) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-1].slice_nr);

  if(img->UseConstrainedIntraPred)
  {
    if (mb_available_up   && (img->intra_block[mb_nr-mb_width][2]==0 || img->intra_block[mb_nr-mb_width][3]==0))
      mb_available_up   = 0;
    if (mb_available_left && (img->intra_block[mb_nr-       1][1]==0 || img->intra_block[mb_nr       -1][3]==0))
      mb_available_left = 0;
  }

  s1=s2=0;

  switch (predmode)
  {
  case VERT_PRED_16:                       // vertical prediction from block above
    for(j=0;j<MB_BLOCK_SIZE;j++)
      for(i=0;i<MB_BLOCK_SIZE;i++)
        img->mpr[i][j]=imgY[img->pix_y-1][img->pix_x+i];// store predicted 16x16 block
    break;

  case HOR_PRED_16:                        // horisontal prediction from left block
    for(j=0;j<MB_BLOCK_SIZE;j++)
      for(i=0;i<MB_BLOCK_SIZE;i++)
        img->mpr[i][j]=imgY[img->pix_y+j][img->pix_x-1]; // store predicted 16x16 block
    break;

  case DC_PRED_16:                         // DC prediction
    s1=s2=0;
    for (i=0; i < MB_BLOCK_SIZE; i++)
    {
      if (mb_available_up)
        s1 += imgY[img->pix_y-1][img->pix_x+i];    // sum hor pix
      if (mb_available_left)
        s2 += imgY[img->pix_y+i][img->pix_x-1];    // sum vert pix
    }
    if (mb_available_up && mb_available_left)
      s0=(s1+s2+16)/(2*MB_BLOCK_SIZE);       // no edge
    if (!mb_available_up && mb_available_left)
      s0=(s2+8)/MB_BLOCK_SIZE;              // upper edge
    if (mb_available_up && !mb_available_left)
      s0=(s1+8)/MB_BLOCK_SIZE;              // left edge
    if (!mb_available_up && !mb_available_left)
      s0=128;                            // top left corner, nothing to predict from
    for(i=0;i<MB_BLOCK_SIZE;i++)
      for(j=0;j<MB_BLOCK_SIZE;j++)
      {
        img->mpr[i][j]=s0;
      }
    break;
  case PLANE_16:// 16 bit integer plan pred
    ih=0;
    iv=0;
    for (i=1;i<9;i++)
    {
      ih += i*(imgY[img->pix_y-1][img->pix_x+7+i] - imgY[img->pix_y-1][img->pix_x+7-i]);
      iv += i*(imgY[img->pix_y+7+i][img->pix_x-1] - imgY[img->pix_y+7-i][img->pix_x-1]);
    }
    ib=5*(ih/4)/16;
    ic=5*(iv/4)/16;

    iaa=16*(imgY[img->pix_y-1][img->pix_x+15]+imgY[img->pix_y+15][img->pix_x-1]);
    for (j=0;j< MB_BLOCK_SIZE;j++)
    {
      for (i=0;i< MB_BLOCK_SIZE;i++)
      {
        img->mpr[i][j]=max(0,min((iaa+(i-7)*ib +(j-7)*ic + 16)/32,255));
      }
    }// store plane prediction
    break;

  default:
    {                                    // indication of fault in bitstream,exit
      printf("Error: illegal prediction mode input: %d\n",predmode);
      return SEARCH_SYNC;
    }
  }

  return DECODING_OK;
}

/*!
 ***********************************************************************
 * \brief
 *    Inverse 4x4 transformation, transforms cof to m7
 ***********************************************************************
 */
void itrans(struct img_par *img, //!< image parameters
            int ioff,            //!< index to 4x4 block
            int joff,            //!<
            int i0,              //!<
            int j0)              //!<
{
  int i,j,i1,j1;
  int m5[4];
  int m6[4];

  // horizontal
  for (j=0;j<BLOCK_SIZE;j++)
  {
    for (i=0;i<BLOCK_SIZE;i++)
    {
      m5[i]=img->cof[i0][j0][i][j];
    }
    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (i=0;i<2;i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }
  // vertical
  for (i=0;i<BLOCK_SIZE;i++)
  {
    for (j=0;j<BLOCK_SIZE;j++)
      m5[j]=img->m7[i][j];

    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (j=0;j<2;j++)
    {
      j1=3-j;
      img->m7[i][j] =mmax(0,mmin(255,(m6[j]+m6[j1]+(img->mpr[i+ioff][j+joff] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS));
      img->m7[i][j1]=mmax(0,mmin(255,(m6[j]-m6[j1]+(img->mpr[i+ioff][j1+joff]<<DQ_BITS)+DQ_ROUND)>>DQ_BITS));
    }
  }

}


/*!
 ***********************************************************************
 * \brief
 *    invers  transform
 ***********************************************************************
 */
void itrans_2(
   struct img_par *img) //!< image parameters
{
  int i,j,i1,j1;
  int M5[4];
  int M6[4];

  int qp_per = (img->qp-MIN_QP)/6;
  int qp_rem = (img->qp-MIN_QP)%6;

  // horizontal
  for (j=0;j<4;j++)
  {
    for (i=0;i<4;i++)
      M5[i]=img->cof[i][j][0][0];

    M6[0]=M5[0]+M5[2];
    M6[1]=M5[0]-M5[2];
    M6[2]=M5[1]-M5[3];
    M6[3]=M5[1]+M5[3];

    for (i=0;i<2;i++)
    {
      i1=3-i;
      img->cof[i ][j][0][0]= M6[i]+M6[i1];
      img->cof[i1][j][0][0]=M6[i]-M6[i1];
    }
  }

  // vertical
  for (i=0;i<4;i++)
  {
    for (j=0;j<4;j++)
      M5[j]=img->cof[i][j][0][0];

    M6[0]=M5[0]+M5[2];
    M6[1]=M5[0]-M5[2];
    M6[2]=M5[1]-M5[3];
    M6[3]=M5[1]+M5[3];

    for (j=0;j<2;j++)
    {
      j1=3-j;
      img->cof[i][j][0][0] = ((M6[j]+M6[j1])*dequant_coef[qp_rem][0][0]<<qp_per)/4;
      img->cof[i][j1][0][0]= ((M6[j]-M6[j1])*dequant_coef[qp_rem][0][0]<<qp_per)/4;
    }
  }
}


void itrans_sp(struct img_par *img,  //!< image parameters
               int ioff,             //!< index to 4x4 block
               int joff,             //!<
               int i0,               //!<
               int j0)               //!<
{
  int i,j,i1,j1;
  int m5[4];
  int m6[4];
  int predicted_block[BLOCK_SIZE][BLOCK_SIZE],ilev;
  
  int qp_per = (img->qp-MIN_QP)/6;
  int qp_rem = (img->qp-MIN_QP)%6;
  int q_bits    = Q_BITS+qp_per;
  int qp_per_sp = (img->qpsp-MIN_QP)/6;
  int qp_rem_sp = (img->qpsp-MIN_QP)%6;
  int q_bits_sp    = Q_BITS+qp_per_sp;

  int qp_const2=(1<<q_bits_sp)/2;  //sp_pred

  for (j=0; j< BLOCK_SIZE; j++)
    for (i=0; i< BLOCK_SIZE; i++)
    {
      predicted_block[i][j]=img->mpr[i+ioff][j+joff];
    }
  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=predicted_block[i][j]+predicted_block[i1][j];
      m5[i1]=predicted_block[i][j]-predicted_block[i1][j];
    }
    predicted_block[0][j]=(m5[0]+m5[1]);
    predicted_block[2][j]=(m5[0]-m5[1]);
    predicted_block[1][j]=m5[3]*2+m5[2];
    predicted_block[3][j]=m5[3]-m5[2]*2;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=predicted_block[i][j]+predicted_block[i][j1];
      m5[j1]=predicted_block[i][j]-predicted_block[i][j1];
    }
    predicted_block[i][0]=(m5[0]+m5[1]);
    predicted_block[i][2]=(m5[0]-m5[1]);
    predicted_block[i][1]=m5[3]*2+m5[2];
    predicted_block[i][3]=m5[3]-m5[2]*2;
  }


  for (j=0;j<BLOCK_SIZE;j++)
    for (i=0;i<BLOCK_SIZE;i++)
    {
      img->cof[i0][j0][i][j]=(img->cof[i0][j0][i][j]>>qp_per)/dequant_coef[qp_rem][i][j]; // recovering coefficient since they are already dequantized earlier
      ilev=(sign(((abs(img->cof[i0][j0][i][j])<<q_bits)/quant_coef[qp_rem][i][j]),img->cof[i0][j0][i][j])+predicted_block[i][j] );
      img->cof[i0][j0][i][j]=sign((abs(ilev)* quant_coef[qp_rem_sp][i][j]+qp_const2)>> q_bits_sp,ilev)*dequant_coef[qp_rem_sp][i][j]<<qp_per_sp;
    }
  // horizontal
  for (j=0;j<BLOCK_SIZE;j++)
  {
    for (i=0;i<BLOCK_SIZE;i++)
    {
      m5[i]=img->cof[i0][j0][i][j];
    }
    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (i=0;i<2;i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }
  // vertical
  for (i=0;i<BLOCK_SIZE;i++)
  {
    for (j=0;j<BLOCK_SIZE;j++)
      m5[j]=img->m7[i][j];

    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (j=0;j<2;j++)
    {
      j1=3-j;
      img->m7[i][j] =mmax(0,mmin(255,(m6[j]+m6[j1]+DQ_ROUND)>>DQ_BITS));
      img->m7[i][j1]=mmax(0,mmin(255,(m6[j]-m6[j1]+DQ_ROUND)>>DQ_BITS));
    }
  }
}

/*!
 ***********************************************************************
 * \brief
 *    The routine performs transform,quantization,inverse transform, adds the diff.
 *    to the prediction and writes the result to the decoded luma frame. Includes the
 *    RD constrained quantization also.
 *
 * \par Input:
 *    block_x,block_y: Block position inside a macro block (0,4,8,12).
 *
 * \par Output:
 *    nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels. \n
 *    coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
 ************************************************************************
 */
void copyblock_sp(struct img_par *img,int block_x,int block_y)
{
  int sign(int a,int b);

  int i,j,i1,j1,m5[4],m6[4];

  int predicted_block[BLOCK_SIZE][BLOCK_SIZE];
  int qp_per = (img->qpsp-MIN_QP)/6;
  int qp_rem = (img->qpsp-MIN_QP)%6;
  int q_bits    = Q_BITS+qp_per;
  int qp_const2=(1<<q_bits)/2;  //sp_pred


  //  Horizontal transform
  for (j=0; j< BLOCK_SIZE; j++)
    for (i=0; i< BLOCK_SIZE; i++)
    {
      predicted_block[i][j]=img->mpr[i+block_x][j+block_y];
    }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=predicted_block[i][j]+predicted_block[i1][j];
      m5[i1]=predicted_block[i][j]-predicted_block[i1][j];
    }
    predicted_block[0][j]=(m5[0]+m5[1]);
    predicted_block[2][j]=(m5[0]-m5[1]);
    predicted_block[1][j]=m5[3]*2+m5[2];
    predicted_block[3][j]=m5[3]-m5[2]*2;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=predicted_block[i][j]+predicted_block[i][j1];
      m5[j1]=predicted_block[i][j]-predicted_block[i][j1];
    }
    predicted_block[i][0]=(m5[0]+m5[1]);
    predicted_block[i][2]=(m5[0]-m5[1]);
    predicted_block[i][1]=m5[3]*2+m5[2];
    predicted_block[i][3]=m5[3]-m5[2]*2;
  }

  // Quant
  for (j=0;j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
       img->m7[i][j]=sign((abs(predicted_block[i][j])* quant_coef[qp_rem][i][j]+qp_const2)>> q_bits,predicted_block[i][j])*dequant_coef[qp_rem][i][j]<<qp_per;

  //     IDCT.
  //     horizontal

  for (j=0;j<BLOCK_SIZE;j++)
  {
    for (i=0;i<BLOCK_SIZE;i++)
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (i=0;i<2;i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }
  // vertical
  for (i=0;i<BLOCK_SIZE;i++)
  {
    for (j=0;j<BLOCK_SIZE;j++)
      m5[j]=img->m7[i][j];

    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (j=0;j<2;j++)
    {
      j1=3-j;
      img->m7[i][j] =mmax(0,mmin(255,(m6[j]+m6[j1]+DQ_ROUND)>>DQ_BITS));
      img->m7[i][j1]=mmax(0,mmin(255,(m6[j]-m6[j1]+DQ_ROUND)>>DQ_BITS));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];

}

void itrans_sp_chroma(struct img_par *img,int ll)
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,qp_const;
  int m5[BLOCK_SIZE];
  int predicted_chroma_block[MB_BLOCK_SIZE/2][MB_BLOCK_SIZE/2],mp1[BLOCK_SIZE];
  int qp_per,qp_rem,q_bits;
  int qp_per_sp,qp_rem_sp,q_bits_sp,qp_const2;

  qp_per    = ((img->qp<0?img->qp:QP_SCALE_CR[img->qp])-MIN_QP)/6;
  qp_rem    = ((img->qp<0?img->qp:QP_SCALE_CR[img->qp])-MIN_QP)%6;
  q_bits    = Q_BITS+qp_per;
  qp_const=(1<<q_bits)/6;    // inter
  qp_per_sp    = ((img->qpsp<0?img->qpsp:QP_SCALE_CR[img->qpsp])-MIN_QP)/6;
  qp_rem_sp    = ((img->qpsp<0?img->qpsp:QP_SCALE_CR[img->qpsp])-MIN_QP)%6;
  q_bits_sp    = Q_BITS+qp_per_sp;
  qp_const2=(1<<q_bits_sp)/2;  //sp_pred

  for (j=0; j < MB_BLOCK_SIZE/2; j++)
    for (i=0; i < MB_BLOCK_SIZE/2; i++)
    {
      predicted_chroma_block[i][j]=img->mpr[i][j];
      img->mpr[i][j]=0;
    }
  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      //  Horizontal transform.
      for (j=0; j < BLOCK_SIZE; j++)
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++)
        {
          i1=3-i;
          m5[i]=predicted_chroma_block[i+n1][mb_y]+predicted_chroma_block[i1+n1][mb_y];
          m5[i1]=predicted_chroma_block[i+n1][mb_y]-predicted_chroma_block[i1+n1][mb_y];
        }
        predicted_chroma_block[n1][mb_y]  =(m5[0]+m5[1]);
        predicted_chroma_block[n1+2][mb_y]=(m5[0]-m5[1]);
        predicted_chroma_block[n1+1][mb_y]=m5[3]*2+m5[2];
        predicted_chroma_block[n1+3][mb_y]=m5[3]-m5[2]*2;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=predicted_chroma_block[j1][n2+j]+predicted_chroma_block[j1][n2+j2];
          m5[j2]=predicted_chroma_block[j1][n2+j]-predicted_chroma_block[j1][n2+j2];
        }
        predicted_chroma_block[j1][n2+0]=(m5[0]+m5[1]);
        predicted_chroma_block[j1][n2+2]=(m5[0]-m5[1]);
        predicted_chroma_block[j1][n2+1]=m5[3]*2+m5[2];
        predicted_chroma_block[j1][n2+3]=m5[3]-m5[2]*2;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  mp1[0]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]+predicted_chroma_block[0][4]+predicted_chroma_block[4][4]);
  mp1[1]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]+predicted_chroma_block[0][4]-predicted_chroma_block[4][4]);
  mp1[2]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]-predicted_chroma_block[0][4]-predicted_chroma_block[4][4]);
  mp1[3]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]-predicted_chroma_block[0][4]+predicted_chroma_block[4][4]);

  ilev=(sign(((abs(img->cof[0+ll][4][0][0])<<(q_bits+1))/quant_coef[qp_rem][0][0]),img->cof[0+ll][4][0][0])+mp1[0] );
  mp1[0]=sign((abs(ilev)* quant_coef[qp_rem_sp][0][0]+qp_const2)>> (q_bits_sp+1),ilev)*dequant_coef[qp_rem_sp][0][0]<<qp_per_sp;
  ilev=(sign(((abs(img->cof[1+ll][4][0][0])<<(q_bits+1))/quant_coef[qp_rem][0][0]),img->cof[1+ll][4][0][0])+mp1[1] );
  mp1[1]=sign((abs(ilev)* quant_coef[qp_rem_sp][0][0]+qp_const2)>> (q_bits_sp+1),ilev)*dequant_coef[qp_rem_sp][0][0]<<qp_per_sp;
  ilev=(sign(((abs(img->cof[0+ll][5][0][0])<<(q_bits+1))/quant_coef[qp_rem][0][0]),img->cof[0+ll][5][0][0])+mp1[2] );
  mp1[2]=sign((abs(ilev)* quant_coef[qp_rem_sp][0][0]+qp_const2)>> (q_bits_sp+1),ilev)*dequant_coef[qp_rem_sp][0][0]<<qp_per_sp;
  ilev=(sign(((abs(img->cof[1+ll][5][0][0])<<(q_bits+1))/quant_coef[qp_rem][0][0]),img->cof[1+ll][5][0][0])+mp1[3] );
  mp1[3]=sign((abs(ilev)* quant_coef[qp_rem_sp][0][0]+qp_const2)>> (q_bits_sp+1),ilev)*dequant_coef[qp_rem_sp][0][0]<<qp_per_sp;

  img->cof[0+ll][4][0][0]=(mp1[0]+mp1[1]+mp1[2]+mp1[3])/2;
  img->cof[1+ll][4][0][0]=(mp1[0]-mp1[1]+mp1[2]-mp1[3])/2;
  img->cof[0+ll][5][0][0]=(mp1[0]+mp1[1]-mp1[2]-mp1[3])/2;
  img->cof[1+ll][5][0][0]=(mp1[0]-mp1[1]-mp1[2]+mp1[3])/2;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
      for (i=0;i< BLOCK_SIZE; i++)
        for (j=0;j< BLOCK_SIZE; j++)
        if ((i!=0) || (j!=0))
        {
          img->cof[n1/BLOCK_SIZE+ll][4+n2/BLOCK_SIZE][i][j]=(img->cof[n1/BLOCK_SIZE+ll][4+n2/BLOCK_SIZE][i][j]>>qp_per)/dequant_coef[qp_rem][i][j];
          ilev=(sign(((abs(img->cof[n1/BLOCK_SIZE+ll][4+n2/BLOCK_SIZE][i][j])<<q_bits)/quant_coef[qp_rem][i][j]),img->cof[n1/BLOCK_SIZE+ll][4+n2/BLOCK_SIZE][i][j])+predicted_chroma_block[n1+i][n2+j] )* quant_coef[qp_rem_sp][i][j];
          img->cof[n1/BLOCK_SIZE+ll][4+n2/BLOCK_SIZE][i][j]=sign((abs(ilev)+qp_const2)>> q_bits_sp,ilev)*dequant_coef[qp_rem_sp][i][j]<<qp_per_sp;
        }
}

int sign(int a , int b)
{
  int x;

  x=abs(a);
  if (b>0)
    return(x);
  else return(-x);
}
