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
 *************************************************************************************
 * \file block.c
 *
 * \brief
 *    Process one block
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Lang�y               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *    - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *    - Jani Lainema                    <jani.lainema@nokia.com>
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *    - Ragip Kurceren                  <ragip.kurceren@nokia.com>
 *************************************************************************************
 */

#include "contributors.h"


#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "block.h"
#include "refbuf.h"


#define Q_BITS          16
#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))


static const int quant_coef[6][4][4] = {
  {{16384,10486,16384,10486},{10486, 6453,10486, 6453},{16384,10486,16384,10486},{10486, 6453,10486, 6453}},
  {{14564, 9118,14564, 9118},{ 9118, 5785, 9118, 5785},{14564, 9118,14564, 9118},{ 9118, 5785, 9118, 5785}},
  {{13107, 8389,13107, 8389},{ 8389, 5243, 8389, 5243},{13107, 8389,13107, 8389},{ 8389, 5243, 8389, 5243}},
  {{11916, 7490,11916, 7490},{ 7490, 4660, 7490, 4660},{11916, 7490,11916, 7490},{ 7490, 4660, 7490, 4660}},
  {{10486, 6554,10486, 6554},{ 6554, 4194, 6554, 4194},{10486, 6554,10486, 6554},{ 6554, 4194, 6554, 4194}},
  {{9362,  5825, 9362, 5825},{ 5825, 3728, 5825, 3728},{ 9362, 5825, 9362, 5825},{ 5825, 3728, 5825, 3728}}
};

static const int dequant_coef[6][4][4] = {
  {{16,20,16,20},{20,26,20,26},{16,20,16,20},{20,26,20,26}},
  {{18,23,18,23},{23,29,23,29},{18,23,18,23},{23,29,23,29}},
  {{20,25,20,25},{25,32,25,32},{20,25,20,25},{25,32,25,32}},
  {{22,28,22,28},{28,36,28,36},{22,28,22,28},{28,36,28,36}},
  {{25,32,25,32},{32,40,32,40},{25,32,25,32},{32,40,32,40}},
  {{28,36,28,36},{36,45,36,45},{28,36,28,36},{36,45,36,45}},
};


/*!
 ************************************************************************
 * \brief
 *    Make intra 4x4 prediction according to all 6 prediction modes.
 *    The routine uses left and upper neighbouring points from
 *    previous coded blocks to do this (if available). Notice that
 *    inaccessible neighbouring points are signalled with a negative
 *    value i the predmode array .
 *
 *  \para Input:
 *     Starting point of current 4x4 block image posision
 *
 *  \para Output:
 *      none
 ************************************************************************
 */
void intrapred_luma(int img_x,int img_y)
{
  int i,j,s0=0,s1,s2,ia[7][3],s[4][2];

  int block_available_up = (img->ipredmode[img_x/BLOCK_SIZE+1][img_y/BLOCK_SIZE] >=0);
  int block_available_left = (img->ipredmode[img_x/BLOCK_SIZE][img_y/BLOCK_SIZE+1] >=0);

  s1=0;
  s2=0;

  // make DC prediction
  for (i=0; i < BLOCK_SIZE; i++)
  {
    if (block_available_up)
      s1 += imgY[img_y-1][img_x+i];    // sum hor pix
    if (block_available_left)
      s2 += imgY[img_y+i][img_x-1];    // sum vert pix
  }
  if (block_available_up && block_available_left)
    s0=(s1+s2+4)/(2*BLOCK_SIZE);      // no edge
  if (!block_available_up && block_available_left)
    s0=(s2+2)/BLOCK_SIZE;             // upper edge
  if (block_available_up && !block_available_left)
    s0=(s1+2)/BLOCK_SIZE;             // left edge
  if (!block_available_up && !block_available_left)
    s0=128;                           // top left corner, nothing to predict from

  for (i=0; i < BLOCK_SIZE; i++)
  {
    // vertical prediction
    if (block_available_up)
      s[i][0]=imgY[img_y-1][img_x+i];
    // horizontal prediction
    if (block_available_left)
      s[i][1]=imgY[img_y+i][img_x-1];
  }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      img->mprr[DC_PRED][i][j]=s0;      // store DC prediction
      img->mprr[VERT_PRED][i][j]=s[j][0]; // store vertical prediction
      img->mprr[HOR_PRED][i][j]=s[i][1]; // store horizontal prediction
    }
  }

  //  Prediction according to 'diagonal' modes
  if (block_available_up && block_available_left)
  {
    int A = imgY[img_y-1][img_x];
    int B = imgY[img_y-1][img_x+1];
    int C = imgY[img_y-1][img_x+2];
    int D = imgY[img_y-1][img_x+3];
    int E = imgY[img_y  ][img_x-1];
    int F = imgY[img_y+1][img_x-1];
    int G = imgY[img_y+2][img_x-1];
    int H = imgY[img_y+3][img_x-1];
    int I = imgY[img_y-1][img_x-1];
    ia[0][0]=(H+2*G+F+2)/4;
    ia[1][0]=(G+2*F+E+2)/4;
    ia[2][0]=(F+2*E+I+2)/4;
    ia[3][0]=(E+2*I+A+2)/4;
    ia[4][0]=(I+2*A+B+2)/4;
    ia[5][0]=(A+2*B+C+2)/4;
    ia[6][0]=(B+2*C+D+2)/4;
    for (i=0;i<4;i++)
      for (j=0;j<4;j++)
        img->mprr[DIAG_PRED_LR_45][i][j]=ia[j-i+3][0];
  }
  if (block_available_up)
  { // Do prediction 1
    int A = imgY[img_y-1][img_x+0];
    int B = imgY[img_y-1][img_x+1];
    int C = imgY[img_y-1][img_x+2];
    int D = imgY[img_y-1][img_x+3];

    img->mprr[DIAG_PRED_RL][0][0] = (A+B)/2; // a
    img->mprr[DIAG_PRED_RL][1][0] = B;       // e
    img->mprr[DIAG_PRED_RL][0][1] = img->mprr[DIAG_PRED_RL][2][0] = (B+C)/2; // b i
    img->mprr[DIAG_PRED_RL][1][1] = img->mprr[DIAG_PRED_RL][3][0] = C;       // f m
    img->mprr[DIAG_PRED_RL][0][2] = img->mprr[DIAG_PRED_RL][2][1] = (C+D)/2; // c j
    img->mprr[DIAG_PRED_RL][3][1] =
        img->mprr[DIAG_PRED_RL][1][2] =
            img->mprr[DIAG_PRED_RL][2][2] =
                img->mprr[DIAG_PRED_RL][3][2] =
                    img->mprr[DIAG_PRED_RL][0][3] =
                        img->mprr[DIAG_PRED_RL][1][3] =
                            img->mprr[DIAG_PRED_RL][2][3] =
                                img->mprr[DIAG_PRED_RL][3][3] = D; // d g h k l n o p
  }

  if (block_available_left)
  { // Do prediction 5
    int E = imgY[img_y+0][img_x-1];
    int F = imgY[img_y+1][img_x-1];
    int G = imgY[img_y+2][img_x-1];
    int H = imgY[img_y+3][img_x-1];

    img->mprr[DIAG_PRED_LR][0][0] = (E+F)/2; // a
    img->mprr[DIAG_PRED_LR][0][1] = F;       // b
    img->mprr[DIAG_PRED_LR][1][0] = img->mprr[DIAG_PRED_LR][0][2] = (F+G)/2; // e c
    img->mprr[DIAG_PRED_LR][1][1] = img->mprr[DIAG_PRED_LR][0][3] = G;       // f d
    img->mprr[DIAG_PRED_LR][2][0] = img->mprr[DIAG_PRED_LR][1][2] = (G+H)/2; // i g

    img->mprr[DIAG_PRED_LR][1][3] =
        img->mprr[DIAG_PRED_LR][2][1] =
            img->mprr[DIAG_PRED_LR][2][2] =
                img->mprr[DIAG_PRED_LR][2][3] =
                    img->mprr[DIAG_PRED_LR][3][0] =
                        img->mprr[DIAG_PRED_LR][3][1] =
                            img->mprr[DIAG_PRED_LR][3][2] =
                                img->mprr[DIAG_PRED_LR][3][3] = H;
  }
}

/*!
 ************************************************************************
 * \brief
 *    16x16 based luma prediction
 *
 * \para Input:
 *    Image parameters
 *
 * \para Output:
 *    none
 ************************************************************************
 */
void intrapred_luma_2()
{
  int s0=0,s1,s2;
  int i,j;
  int s[16][2];

  int ih,iv;
  int ib,ic,iaa;

  int mb_nr = img->current_mb_nr;
  int mb_width = img->width/16;
  int mb_available_up = (img->mb_y == 0) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width].slice_nr);
  int mb_available_left = (img->mb_x == 0) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-1].slice_nr);

  if(input->UseConstrainedIntraPred)
  {
    if (mb_available_up   && (img->intra_block[mb_nr-mb_width][2]==0 || img->intra_block[mb_nr-mb_width][3]==0))
      mb_available_up   = 0;
    if (mb_available_left && (img->intra_block[mb_nr-       1][1]==0 || img->intra_block[mb_nr       -1][3]==0))
      mb_available_left = 0;
  }

  s1=s2=0;
  // make DC prediction
  for (i=0; i < MB_BLOCK_SIZE; i++)
  {
    if (mb_available_up)
      s1 += imgY[img->pix_y-1][img->pix_x+i];    // sum hor pix
    if (mb_available_left)
      s2 += imgY[img->pix_y+i][img->pix_x-1];    // sum vert pix
  }
  if (mb_available_up && mb_available_left)
    s0=(s1+s2+16)/(2*MB_BLOCK_SIZE);             // no edge
  if (!mb_available_up && mb_available_left)
    s0=(s2+8)/MB_BLOCK_SIZE;                     // upper edge
  if (mb_available_up && !mb_available_left)
    s0=(s1+8)/MB_BLOCK_SIZE;                     // left edge
  if (!mb_available_up && !mb_available_left)
    s0=128;                                      // top left corner, nothing to predict from

  for (i=0; i < MB_BLOCK_SIZE; i++)
  {
    // vertical prediction
    if (mb_available_up)
      s[i][0]=imgY[img->pix_y-1][img->pix_x+i];
    // horizontal prediction
    if (mb_available_left)
      s[i][1]=imgY[img->pix_y+i][img->pix_x-1];
  }

  for (j=0; j < MB_BLOCK_SIZE; j++)
  {
    for (i=0; i < MB_BLOCK_SIZE; i++)
    {
      img->mprr_2[VERT_PRED_16][j][i]=s[i][0]; // store vertical prediction
      img->mprr_2[HOR_PRED_16 ][j][i]=s[j][1]; // store horizontal prediction
      img->mprr_2[DC_PRED_16  ][j][i]=s0;      // store DC prediction
    }
  }
  if (!mb_available_up || !mb_available_left) // edge
    return;

  // 16 bit integer plan pred

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
      img->mprr_2[PLANE_16][j][i]=max(0,min(255,(iaa+(i-7)*ib +(j-7)*ic + 16)/32));// store plane prediction
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    For new intra pred routines
 *
 * \para Input:
 *    Image par, 16x16 based intra mode
 *
 * \para Output:
 *    none
 ************************************************************************
 */
int dct_luma2(int new_intra_mode)
{
  int qp_const;
  int i,j;
  int ii,jj;
  int i1,j1;
  int M1[16][16];
  int M4[4][4];
  int M5[4],M6[4];
  int M0[4][4][4][4];
  int run,scan_pos,coeff_ctr,level;
  int qp_per,qp_rem,q_bits;
  int ac_coef = 0;

  int   b8, b4;
  int*  DCLevel = img->cofDC[0][0];
  int*  DCRun   = img->cofDC[0][1];
  int*  ACLevel;
  int*  ACRun;

  qp_per    = (img->qp-MIN_QP)/6;
  qp_rem    = (img->qp-MIN_QP)%6;
  q_bits    = Q_BITS+qp_per;
  qp_const  = (1<<q_bits)/3;


  for (j=0;j<16;j++)
  {
    for (i=0;i<16;i++)
    {
      M1[i][j]=imgY_org[img->pix_y+j][img->pix_x+i]-img->mprr_2[new_intra_mode][j][i];
      M0[i%4][i/4][j%4][j/4]=M1[i][j];
    }
  }

  for (jj=0;jj<4;jj++)
  {
    for (ii=0;ii<4;ii++)
    {
      for (j=0;j<4;j++)
      {
        for (i=0;i<2;i++)
        {
          i1=3-i;
          M5[i]=  M0[i][ii][j][jj]+M0[i1][ii][j][jj];
          M5[i1]= M0[i][ii][j][jj]-M0[i1][ii][j][jj];
        }
        M0[0][ii][j][jj]=M5[0]+M5[1];
        M0[2][ii][j][jj]=M5[0]-M5[1];
        M0[1][ii][j][jj]=M5[3]*2+M5[2];
        M0[3][ii][j][jj]=M5[3]-M5[2]*2;
      }
      // vertical
      for (i=0;i<4;i++)
      {
        for (j=0;j<2;j++)
        {
          j1=3-j;
          M5[j] = M0[i][ii][j][jj]+M0[i][ii][j1][jj];
          M5[j1]= M0[i][ii][j][jj]-M0[i][ii][j1][jj];
        }
        M0[i][ii][0][jj]=M5[0]+M5[1];
        M0[i][ii][2][jj]=M5[0]-M5[1];
        M0[i][ii][1][jj]=M5[3]*2+M5[2];
        M0[i][ii][3][jj]=M5[3] -M5[2]*2;
      }
    }
  }

  // pick out DC coeff

  for (j=0;j<4;j++)
    for (i=0;i<4;i++)
      M4[i][j]= M0[0][i][0][j];

  for (j=0;j<4;j++)
  {
    for (i=0;i<2;i++)
    {
      i1=3-i;
      M5[i]= M4[i][j]+M4[i1][j];
      M5[i1]=M4[i][j]-M4[i1][j];
    }
    M4[0][j]=M5[0]+M5[1];
    M4[2][j]=M5[0]-M5[1];
    M4[1][j]=M5[3]+M5[2];
    M4[3][j]=M5[3]-M5[2];
  }

  // vertical

  for (i=0;i<4;i++)
  {
    for (j=0;j<2;j++)
    {
      j1=3-j;
      M5[j]= M4[i][j]+M4[i][j1];
      M5[j1]=M4[i][j]-M4[i][j1];
    }
    M4[i][0]=(M5[0]+M5[1])/2;
    M4[i][2]=(M5[0]-M5[1])/2;
    M4[i][1]=(M5[3]+M5[2])/2;
    M4[i][3]=(M5[3]-M5[2])/2;
  }

  // quant

  run=-1;
  scan_pos=0;

  for (coeff_ctr=0;coeff_ctr<16;coeff_ctr++)
  {
    i=SNGL_SCAN[coeff_ctr][0];
    j=SNGL_SCAN[coeff_ctr][1];

    run++;

    level= (abs(M4[i][j]) * quant_coef[qp_rem][0][0] + 2*qp_const)>>(q_bits+1);

    if (level != 0)
    {
      DCLevel[scan_pos] = sign(level,M4[i][j]);
      DCRun  [scan_pos] = run;
      ++scan_pos;
      run=-1;
    }
    M4[i][j]=sign(level,M4[i][j]);
  }
  DCLevel[scan_pos]=0;

  // invers DC transform
  for (j=0;j<4;j++)
  {
    for (i=0;i<4;i++)
      M5[i]=M4[i][j];

    M6[0]=M5[0]+M5[2];
    M6[1]=M5[0]-M5[2];
    M6[2]=M5[1]-M5[3];
    M6[3]=M5[1]+M5[3];

    for (i=0;i<2;i++)
    {
      i1=3-i;
      M4[i][j]= M6[i]+M6[i1];
      M4[i1][j]=M6[i]-M6[i1];
    }
  }

  for (i=0;i<4;i++)
  {
    for (j=0;j<4;j++)
      M5[j]=M4[i][j];

    M6[0]=M5[0]+M5[2];
    M6[1]=M5[0]-M5[2];
    M6[2]=M5[1]-M5[3];
    M6[3]=M5[1]+M5[3];

    for (j=0;j<2;j++)
    {
      j1=3-j;
      M0[0][i][0][j] = ((M6[j]+M6[j1])*dequant_coef[qp_rem][0][0]<<qp_per)/4;
      M0[0][i][0][j1]= ((M6[j]-M6[j1])*dequant_coef[qp_rem][0][0]<<qp_per)/4;
    }
  }

  // AC invers trans/quant for MB
  for (jj=0;jj<4;jj++)
  {
    for (ii=0;ii<4;ii++)
    {
      run      = -1;
      scan_pos =  0;
      b8       = 2*(jj/2) + (ii/2);
      b4       = 2*(jj%2) + (ii%2);
      ACLevel  = img->cofAC [b8][b4][0];
      ACRun    = img->cofAC [b8][b4][1];

      for (coeff_ctr=1;coeff_ctr<16;coeff_ctr++) // set in AC coeff
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        run++;

        level= ( abs( M0[i][ii][j][jj]) * quant_coef[qp_rem][i][j] + qp_const) >> q_bits;

        if (level != 0)
        {
          ac_coef = 15;
          ACLevel[scan_pos] = sign(level,M0[i][ii][j][jj]);
          ACRun  [scan_pos] = run;
          ++scan_pos;
          run=-1;
        }
        M0[i][ii][j][jj]=sign(level*dequant_coef[qp_rem][i][j]<<qp_per,M0[i][ii][j][jj]);
      }
      ACLevel[scan_pos] = 0;


      // IDCT horizontal

      for (j=0;j<4;j++)
      {
        for (i=0;i<4;i++)
        {
          M5[i]=M0[i][ii][j][jj];
        }

        M6[0]= M5[0]+M5[2];
        M6[1]= M5[0]-M5[2];
        M6[2]=(M5[1]>>1) -M5[3];
        M6[3]= M5[1]+(M5[3]>>1);

        for (i=0;i<2;i++)
        {
          i1=3-i;
          M0[i][ii][j][jj] =M6[i]+M6[i1];
          M0[i1][ii][j][jj]=M6[i]-M6[i1];
        }
      }

      // vert
      for (i=0;i<4;i++)
      {
        for (j=0;j<4;j++)
          M5[j]=M0[i][ii][j][jj];

        M6[0]= M5[0]+M5[2];
        M6[1]= M5[0]-M5[2];
        M6[2]=(M5[1]>>1) -M5[3];
        M6[3]= M5[1]+(M5[3]>>1);

        for (j=0;j<2;j++)
        {
          j1=3-j;
          M0[i][ii][ j][jj]=M6[j]+M6[j1];
          M0[i][ii][j1][jj]=M6[j]-M6[j1];

        }
      }

    }
  }

  for (j=0;j<16;j++)
  {
    for (i=0;i<16;i++)
    {
      M1[i][j]=M0[i%4][i/4][j%4][j/4];
    }
  }

  for (j=0;j<16;j++)
    for (i=0;i<16;i++)
      imgY[img->pix_y+j][img->pix_x+i]=(byte)min(255,max(0,(M1[i][j]+(img->mprr_2[new_intra_mode][j][i]<<DQ_BITS)+DQ_ROUND)>>DQ_BITS));

    return ac_coef;
}


/*!
 ************************************************************************
 * \brief
 *    The routine performs transform,quantization,inverse transform, adds the diff.
 *    to the prediction and writes the result to the decoded luma frame. Includes the
 *    RD constrained quantization also.
 *
 * \para Input:
 *    block_x,block_y: Block position inside a macro block (0,4,8,12).
 *
 * \para Output_
 *    nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels.             \n
 *    coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
 ************************************************************************
 */
int dct_luma(int block_x,int block_y,int *coeff_cost, int old_intra_mode)
{
  int sign(int a,int b);

  int i,j,i1,j1,ilev,m5[4],m6[4],coeff_ctr,scan_loop_ctr;
  int qp_const,level,scan_pos,run;
  int nonzero;
  int idx;
  int scan_mode;
  int loop_rep;
  int qp_per,qp_rem,q_bits;

  int   pos_x   = block_x/BLOCK_SIZE;
  int   pos_y   = block_y/BLOCK_SIZE;
  int   b8      = 2*(pos_y/2) + (pos_x/2);
  int   b4      = 2*(pos_y%2) + (pos_x%2);
  int*  ACLevel = img->cofAC[b8][b4][0];
  int*  ACRun   = img->cofAC[b8][b4][1];

  qp_per    = (img->qp-MIN_QP)/6;
  qp_rem    = (img->qp-MIN_QP)%6;
  q_bits    = Q_BITS+qp_per;

  if (img->type == INTRA_IMG)
    qp_const=(1<<q_bits)/3;    // intra
  else
    qp_const=(1<<q_bits)/6;    // inter

  //  Horizontal transform
  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=img->m7[i][j]+img->m7[i1][j];
      m5[i1]=img->m7[i][j]-img->m7[i1][j];
    }
    img->m7[0][j]=(m5[0]+m5[1]);
    img->m7[2][j]=(m5[0]-m5[1]);
    img->m7[1][j]=m5[3]*2+m5[2];
    img->m7[3][j]=m5[3]-m5[2]*2;
  }

  //  Vertival transform
  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=img->m7[i][j]+img->m7[i][j1];
      m5[j1]=img->m7[i][j]-img->m7[i][j1];
    }
    img->m7[i][0]=(m5[0]+m5[1]);
    img->m7[i][2]=(m5[0]-m5[1]);
    img->m7[i][1]=m5[3]*2+m5[2];
    img->m7[i][3]=m5[3]-m5[2]*2;
  }

  // Quant

  nonzero=FALSE;

  if (old_intra_mode && (img->qp<24 || input->symbol_mode == CABAC)) // for CABAC double scan always
  {
    scan_mode=DOUBLE_SCAN;
    loop_rep=2;
    idx=1;
  }
  else
  {
    scan_mode=SINGLE_SCAN;
    loop_rep=1;
    idx=0;
  }

  for(scan_loop_ctr=0;scan_loop_ctr<loop_rep;scan_loop_ctr++) // 2 times if double scan, 1 normal scan
  {
    run=-1;
    scan_pos=scan_loop_ctr*9;

    for (coeff_ctr=0;coeff_ctr < 16/loop_rep;coeff_ctr++)     // 8 times if double scan, 16 normal scan
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }

      run++;
      ilev=0;

      level = (abs (img->m7[i][j]) * quant_coef[qp_rem][i][j] + qp_const) >> q_bits;

      if (level != 0)
      {
        nonzero=TRUE;
        if (level > 1)
          *coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
        else
          *coeff_cost += COEFF_COST[run];
        ACLevel[scan_pos] = sign(level,img->m7[i][j]);
        ACRun  [scan_pos] = run;
        ++scan_pos;
        run=-1;                     // reset zero level counter
        ilev=level*dequant_coef[qp_rem][i][j]<<qp_per;
      }
      img->m7[i][j]=sign(ilev,img->m7[i][j]);
    }
    ACLevel[scan_pos] = 0;
  }


  //     IDCT.
  //     horizontal

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (i=0; i < 2; i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }

  //  vertical

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < BLOCK_SIZE; j++)
    {
      m5[j]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (j=0; j < 2; j++)
    {
      j1=3-j;
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+(img->mpr[i+block_x][j+block_y] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS));
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+(img->mpr[i+block_x][j1+block_y]<<DQ_BITS)+DQ_ROUND)>>DQ_BITS));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];


  return nonzero;
}



/*!
 ************************************************************************
 * \brief
 *    Transform,quantization,inverse transform for chroma.
 *    The main reason why this is done in a separate routine is the
 *    additional 2x2 transform of DC-coeffs. This routine is called
 *    ones for each of the chroma components.
 *
 * \para Input:
 *    uv    : Make difference between the U and V chroma component  \n
 *    cr_cbp: chroma coded block pattern
 *
 * \para Output:
 *    cr_cbp: Updated chroma coded block pattern.
 ************************************************************************
 */
int dct_chroma(int uv,int cr_cbp)
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,coeff_ctr,qp_const,level ,scan_pos,run;
  int m1[BLOCK_SIZE],m5[BLOCK_SIZE],m6[BLOCK_SIZE];
  int coeff_cost;
  int cr_cbp_tmp;
  int nn0,nn1;
  int DCcoded=0 ;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  int qp_per,qp_rem,q_bits;

  int   b4;
  int*  DCLevel = img->cofDC[uv+1][0];
  int*  DCRun   = img->cofDC[uv+1][1];
  int*  ACLevel;
  int*  ACRun;

  qp_per    = ((img->qp<0?img->qp:QP_SCALE_CR[img->qp])-MIN_QP)/6;
  qp_rem    = ((img->qp<0?img->qp:QP_SCALE_CR[img->qp])-MIN_QP)%6;
  q_bits    = Q_BITS+qp_per;

  if (img->type == INTRA_IMG)
    qp_const=(1<<q_bits)/3;    // intra
  else
    qp_const=(1<<q_bits)/6;    // inter

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
          m5[i]=img->m7[i+n1][mb_y]+img->m7[i1+n1][mb_y];
          m5[i1]=img->m7[i+n1][mb_y]-img->m7[i1+n1][mb_y];
        }
        img->m7[n1][mb_y]  =(m5[0]+m5[1]);
        img->m7[n1+2][mb_y]=(m5[0]-m5[1]);
        img->m7[n1+1][mb_y]=m5[3]*2+m5[2];
        img->m7[n1+3][mb_y]=m5[3]-m5[2]*2;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=img->m7[j1][n2+j]+img->m7[j1][n2+j2];
          m5[j2]=img->m7[j1][n2+j]-img->m7[j1][n2+j2];
        }
        img->m7[j1][n2+0]=(m5[0]+m5[1]);
        img->m7[j1][n2+2]=(m5[0]-m5[1]);
        img->m7[j1][n2+1]=m5[3]*2+m5[2];
        img->m7[j1][n2+3]=m5[3]-m5[2]*2;
      }
    }
  }

  //     2X2 transform of DC coeffs.
  m1[0]=(img->m7[0][0]+img->m7[4][0]+img->m7[0][4]+img->m7[4][4]);
  m1[1]=(img->m7[0][0]-img->m7[4][0]+img->m7[0][4]-img->m7[4][4]);
  m1[2]=(img->m7[0][0]+img->m7[4][0]-img->m7[0][4]-img->m7[4][4]);
  m1[3]=(img->m7[0][0]-img->m7[4][0]-img->m7[0][4]+img->m7[4][4]);

  //     Quant of chroma 2X2 coeffs.
  run=-1;
  scan_pos=0;

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
  {
    run++;
    ilev=0;

    level =(abs(m1[coeff_ctr]) * quant_coef[qp_rem][0][0] + 2*qp_const) >> (q_bits+1);

    if (level  != 0)
    {
      currMB->cbp_blk |= 0xf0000 << (uv << 2) ;    // if one of the 2x2-DC levels is != 0 set the
      cr_cbp=max(1,cr_cbp);                     // coded-bit all 4 4x4 blocks (bit 16-19 or 20-23)
      DCcoded = 1 ;
      DCLevel[scan_pos] = sign(level ,m1[coeff_ctr]);
      DCRun  [scan_pos] = run;
      scan_pos++;
      run=-1;
      ilev=level*dequant_coef[qp_rem][0][0]<<qp_per;
    }
    m1[coeff_ctr]=sign(ilev,m1[coeff_ctr]);
  }
  DCLevel[scan_pos] = 0;

  //  Invers transform of 2x2 DC levels

  img->m7[0][0]=(m1[0]+m1[1]+m1[2]+m1[3])/2;
  img->m7[4][0]=(m1[0]-m1[1]+m1[2]-m1[3])/2;
  img->m7[0][4]=(m1[0]+m1[1]-m1[2]-m1[3])/2;
  img->m7[4][4]=(m1[0]-m1[1]-m1[2]+m1[3])/2;

  //     Quant of chroma AC-coeffs.
  coeff_cost=0;
  cr_cbp_tmp=0;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      b4      = 2*(n2/4) + (n1/4);
      ACLevel = img->cofAC[uv+4][b4][0];
      ACRun   = img->cofAC[uv+4][b4][1];
      run=-1;
      scan_pos=0;

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)// start change rd_quant
      {
        i = SNGL_SCAN[coeff_ctr][0];
        j = SNGL_SCAN[coeff_ctr][1];
        ++run;
        ilev=0;

        level=(abs(img->m7[n1+i][n2+j])*quant_coef[qp_rem][i][j]+qp_const)>>q_bits;

        if (level  != 0)
        {
          currMB->cbp_blk |= 1 << (16 + (uv << 2) + ((n2 >> 1) + (n1 >> 2))) ;
          if (level > 1)
            coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
          else
            coeff_cost += COEFF_COST[run];

          cr_cbp_tmp=2;
          ACLevel[scan_pos] = sign(level,img->m7[n1+i][n2+j]);
          ACRun  [scan_pos] = run;
          ++scan_pos;
          run=-1;
          ilev=level*dequant_coef[qp_rem][i][j]<<qp_per;
        }
        img->m7[n1+i][n2+j]=sign(ilev,img->m7[n1+i][n2+j]); // for use in IDCT
      }
      ACLevel[scan_pos] = 0;
    }
  }

  // * reset chroma coeffs
  if(coeff_cost<7)
  {
    cr_cbp_tmp = 0 ;
    for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
    {
      for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
      {
        b4      = 2*(n2/4) + (n1/4);
        ACLevel = img->cofAC[uv+4][b4][0];
        ACRun   = img->cofAC[uv+4][b4][1];
        if( DCcoded == 0) currMB->cbp_blk &= ~(0xf0000 << (uv << 2));  // if no chroma DC's: then reset coded-bits of this chroma subblock
        nn0 = (n1>>2) + (uv<<1);
        nn1 = 4 + (n2>>2) ;
        ACLevel[0] = 0;
        for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)// ac coeff
        {
          i=SNGL_SCAN[coeff_ctr][0];
          j=SNGL_SCAN[coeff_ctr][1];
          img->m7[n1+i][n2+j]=0;
          ACLevel[coeff_ctr] = 0;
        }
      }
    }
  }
  if(cr_cbp_tmp==2)
    cr_cbp = 2;
  //     IDCT.

      //     Horizontal.
  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      for (j=0; j < BLOCK_SIZE; j++)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          m5[i]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2]);
        m6[1]=(m5[0]-m5[2]);
        m6[2]=(m5[1]>>1)-m5[3];
        m6[3]=m5[1]+(m5[3]>>1);

        for (i=0; i < 2; i++)
        {
          i1=3-i;
          img->m7[n1+i][n2+j]=m6[i]+m6[i1];
          img->m7[n1+i1][n2+j]=m6[i]-m6[i1];
        }
      }

      //     Vertical.
      for (i=0; i < BLOCK_SIZE; i++)
      {
        for (j=0; j < BLOCK_SIZE; j++)
        {
          m5[j]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2]);
        m6[1]=(m5[0]-m5[2]);
        m6[2]=(m5[1]>>1)-m5[3];
        m6[3]=m5[1]+(m5[3]>>1);

        for (j=0; j < 2; j++)
        {
          j2=3-j;
          img->m7[n1+i][n2+j] =min(255,max(0,(m6[j]+m6[j2]+(img->mpr[n1+i][n2+j] <<DQ_BITS)+DQ_ROUND)>>DQ_BITS));
          img->m7[n1+i][n2+j2]=min(255,max(0,(m6[j]-m6[j2]+(img->mpr[n1+i][n2+j2]<<DQ_BITS)+DQ_ROUND)>>DQ_BITS));
        }
      }
    }
  }

  //  Decoded block moved to memory
  for (j=0; j < BLOCK_SIZE*2; j++)
    for (i=0; i < BLOCK_SIZE*2; i++)
      imgUV[uv][img->pix_c_y+j][img->pix_c_x+i]= img->m7[i][j];

  return cr_cbp;
}


/*!
 ************************************************************************
 * \brief
 *    The routine performs transform,quantization,inverse transform, adds the diff.
 *    to the prediction and writes the result to the decoded luma frame. Includes the
 *    RD constrained quantization also.
 *
 * \para Input:
 *    block_x,block_y: Block position inside a macro block (0,4,8,12).
 *
 * \para Output:
 *    nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels.              \n
 *    coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
 *
 *
 ************************************************************************
 */
int dct_luma_sp(int block_x,int block_y,int *coeff_cost)
{
  int sign(int a,int b);

  int i,j,i1,j1,ilev,m5[4],m6[4],coeff_ctr,scan_loop_ctr;
  int qp_const,level,scan_pos,run;
  int nonzero;
  int idx;

  int scan_mode;
  int loop_rep;
  int predicted_block[BLOCK_SIZE][BLOCK_SIZE],c_err,qp_const2;
  int qp_per,qp_rem,q_bits;
  int qp_per_sp,qp_rem_sp,q_bits_sp;

  int   pos_x   = block_x/BLOCK_SIZE;
  int   pos_y   = block_y/BLOCK_SIZE;
  int   b8      = 2*(pos_y/2) + (pos_x/2);
  int   b4      = 2*(pos_y%2) + (pos_x%2);
  int*  ACLevel = img->cofAC[b8][b4][0];
  int*  ACRun   = img->cofAC[b8][b4][1];

  qp_per    = (img->qp-MIN_QP)/6;
  qp_rem    = (img->qp-MIN_QP)%6;
  q_bits    = Q_BITS+qp_per;
  qp_per_sp    = (img->qpsp-MIN_QP)/6;
  qp_rem_sp    = (img->qpsp-MIN_QP)%6;
  q_bits_sp    = Q_BITS+qp_per_sp;

  qp_const=(1<<q_bits)/6;    // inter
  qp_const2=(1<<q_bits_sp)/2;  //sp_pred

  //  Horizontal transform
  for (j=0; j< BLOCK_SIZE; j++)
    for (i=0; i< BLOCK_SIZE; i++)
    {
      img->m7[i][j]+=img->mpr[i+block_x][j+block_y];
      predicted_block[i][j]=img->mpr[i+block_x][j+block_y];
    }

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < 2; i++)
    {
      i1=3-i;
      m5[i]=img->m7[i][j]+img->m7[i1][j];
      m5[i1]=img->m7[i][j]-img->m7[i1][j];
    }
    img->m7[0][j]=(m5[0]+m5[1]);
    img->m7[2][j]=(m5[0]-m5[1]);
    img->m7[1][j]=m5[3]*2+m5[2];
    img->m7[3][j]=m5[3]-m5[2]*2;
  }

  //  Vertival transform

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < 2; j++)
    {
      j1=3-j;
      m5[j]=img->m7[i][j]+img->m7[i][j1];
      m5[j1]=img->m7[i][j]-img->m7[i][j1];
    }
    img->m7[i][0]=(m5[0]+m5[1]);
    img->m7[i][2]=(m5[0]-m5[1]);
    img->m7[i][1]=m5[3]*2+m5[2];
    img->m7[i][3]=m5[3]-m5[2]*2;
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
  nonzero=FALSE;
  scan_mode=SINGLE_SCAN;
  loop_rep=1;
  idx=0;

  for(scan_loop_ctr=0;scan_loop_ctr<loop_rep;scan_loop_ctr++) // 2 times if double scan, 1 normal scan
  {
  run=-1;
  scan_pos=scan_loop_ctr*9;

    for (coeff_ctr=0;coeff_ctr < 16/loop_rep;coeff_ctr++)     // 8 times if double scan, 16 normal scan
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }

      run++;
      ilev=0;
   //   level_pred = (abs (predicted_block[i][j]) * quant_coef[qp_rem][i][j] + qp_const) >> q_bits;
   //   c_err=img->m7[i][j]-sign((level_pred<<q_bits)/quant_coef[qp_rem][i][j],predicted_block[i][j]);
      c_err=img->m7[i][j]-predicted_block[i][j];
      level = (abs (c_err) * quant_coef[qp_rem][i][j] + qp_const) >> q_bits;

      if (level != 0)
      {
        nonzero=TRUE;
        if (level > 1)
          *coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
        else
          *coeff_cost += COEFF_COST[run];
        ACLevel[scan_pos] = sign(level,c_err);
        ACRun  [scan_pos] = run;
        ++scan_pos;
        run=-1;                     // reset zero level counter
        ilev=level;
      }
      ilev=(sign(((ilev<<q_bits)/quant_coef[qp_rem][i][j]),c_err)+predicted_block[i][j] );
      img->m7[i][j]=sign((abs(ilev)* quant_coef[qp_rem_sp][i][j]+qp_const2)>> q_bits_sp,ilev)*dequant_coef[qp_rem_sp][i][j]<<qp_per_sp;
    }
    ACLevel[scan_pos] = 0;
  }
  //     IDCT.
  //     horizontal

  for (j=0; j < BLOCK_SIZE; j++)
  {
    for (i=0; i < BLOCK_SIZE; i++)
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (i=0; i < 2; i++)
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }

  //  vertical

  for (i=0; i < BLOCK_SIZE; i++)
  {
    for (j=0; j < BLOCK_SIZE; j++)
    {
      m5[j]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2]);
    m6[1]=(m5[0]-m5[2]);
    m6[2]=(m5[1]>>1)-m5[3];
    m6[3]=m5[1]+(m5[3]>>1);

    for (j=0; j < 2; j++)
    {
      j1=3-j;
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+DQ_ROUND)>>DQ_BITS));
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+DQ_ROUND)>>DQ_BITS));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];


  return nonzero;
}


/*!
 ************************************************************************
 * \brief
 *    Transform,quantization,inverse transform for chroma.
 *    The main reason why this is done in a separate routine is the
 *    additional 2x2 transform of DC-coeffs. This routine is called
 *    ones for each of the chroma components.
 *
 * \para Input:
 *    uv    : Make difference between the U and V chroma component               \n
 *    cr_cbp: chroma coded block pattern
 *
 * \para Output:
 *    cr_cbp: Updated chroma coded block pattern.
 ************************************************************************
 */
int dct_chroma_sp(int uv,int cr_cbp)
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,coeff_ctr,qp_const,c_err,level ,scan_pos,run;
  int m1[BLOCK_SIZE],m5[BLOCK_SIZE],m6[BLOCK_SIZE];
  int coeff_cost;
  int cr_cbp_tmp;
  int predicted_chroma_block[MB_BLOCK_SIZE/2][MB_BLOCK_SIZE/2],qp_const2,mp1[BLOCK_SIZE];
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  int qp_per,qp_rem,q_bits;
  int qp_per_sp,qp_rem_sp,q_bits_sp;

  int   b4;
  int*  DCLevel = img->cofDC[uv+1][0];
  int*  DCRun   = img->cofDC[uv+1][1];
  int*  ACLevel;
  int*  ACRun;

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
      img->m7[i][j]+=img->mpr[i][j];
      predicted_chroma_block[i][j]=img->mpr[i][j];
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
          m5[i]=img->m7[i+n1][mb_y]+img->m7[i1+n1][mb_y];
          m5[i1]=img->m7[i+n1][mb_y]-img->m7[i1+n1][mb_y];
        }
        img->m7[n1][mb_y]  =(m5[0]+m5[1]);
        img->m7[n1+2][mb_y]=(m5[0]-m5[1]);
        img->m7[n1+1][mb_y]=m5[3]*2+m5[2];
        img->m7[n1+3][mb_y]=m5[3]-m5[2]*2;
      }

      //  Vertical transform.

      for (i=0; i < BLOCK_SIZE; i++)
      {
        j1=n1+i;
        for (j=0; j < 2; j++)
        {
          j2=3-j;
          m5[j]=img->m7[j1][n2+j]+img->m7[j1][n2+j2];
          m5[j2]=img->m7[j1][n2+j]-img->m7[j1][n2+j2];
        }
        img->m7[j1][n2+0]=(m5[0]+m5[1]);
        img->m7[j1][n2+2]=(m5[0]-m5[1]);
        img->m7[j1][n2+1]=m5[3]*2+m5[2];
        img->m7[j1][n2+3]=m5[3]-m5[2]*2;
      }
    }
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
  m1[0]=(img->m7[0][0]+img->m7[4][0]+img->m7[0][4]+img->m7[4][4]);
  m1[1]=(img->m7[0][0]-img->m7[4][0]+img->m7[0][4]-img->m7[4][4]);
  m1[2]=(img->m7[0][0]+img->m7[4][0]-img->m7[0][4]-img->m7[4][4]);
  m1[3]=(img->m7[0][0]-img->m7[4][0]-img->m7[0][4]+img->m7[4][4]);

  //     2X2 transform of DC coeffs.
  mp1[0]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]+predicted_chroma_block[0][4]+predicted_chroma_block[4][4]);
  mp1[1]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]+predicted_chroma_block[0][4]-predicted_chroma_block[4][4]);
  mp1[2]=(predicted_chroma_block[0][0]+predicted_chroma_block[4][0]-predicted_chroma_block[0][4]-predicted_chroma_block[4][4]);
  mp1[3]=(predicted_chroma_block[0][0]-predicted_chroma_block[4][0]-predicted_chroma_block[0][4]+predicted_chroma_block[4][4]);

  run=-1;
  scan_pos=0;

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++)
  {
    run++;
    ilev=0;

    c_err=m1[coeff_ctr]-mp1[coeff_ctr];
    level =(abs(c_err) * quant_coef[qp_rem][0][0] + 2*qp_const) >> (q_bits+1);

    if (level  != 0)
    {
      currMB->cbp_blk |= 0xf0000 << (uv << 2) ;  // if one of the 2x2-DC levels is != 0 the coded-bit
      cr_cbp=max(1,cr_cbp);
      DCLevel[scan_pos] = sign(level ,c_err);
      DCRun  [scan_pos] = run;
      scan_pos++;
      run=-1;
      ilev=level;
    }
    ilev=(sign(((ilev<<(q_bits+1))/quant_coef[qp_rem][0][0]),c_err)+mp1[coeff_ctr] )* quant_coef[qp_rem_sp][0][0];
    m1[coeff_ctr]=sign((abs(ilev)+qp_const2)>> (q_bits_sp+1),ilev)*dequant_coef[qp_rem_sp][0][0]<<qp_per_sp;
  }
  DCLevel[scan_pos] = 0;

  //  Invers transform of 2x2 DC levels

  img->m7[0][0]=(m1[0]+m1[1]+m1[2]+m1[3])/2;
  img->m7[4][0]=(m1[0]-m1[1]+m1[2]-m1[3])/2;
  img->m7[0][4]=(m1[0]+m1[1]-m1[2]-m1[3])/2;
  img->m7[4][4]=(m1[0]-m1[1]-m1[2]+m1[3])/2;

  //     Quant of chroma AC-coeffs.
  coeff_cost=0;
  cr_cbp_tmp=0;

  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      b4      = 2*(n2/4) + (n1/4);
      ACLevel = img->cofAC[uv+4][b4][0];
      ACRun   = img->cofAC[uv+4][b4][1];

      run      = -1;
      scan_pos =  0;

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++)// start change rd_quant
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        ++run;
        ilev=0;

        c_err=img->m7[n1+i][n2+j]-predicted_chroma_block[n1+i][n2+j];
        level=(abs(c_err)*quant_coef[qp_rem][i][j]+qp_const)>>q_bits;

        if (level  != 0)
        {
          currMB->cbp_blk |=  1 << (16 + (uv << 2) + ((n2 >> 1) + (n1 >> 2))) ;
          if (level > 1)
            coeff_cost += MAX_VALUE;                // set high cost, shall not be discarded
          else
            coeff_cost += COEFF_COST[run];

          cr_cbp_tmp=2;
          ACLevel[scan_pos] = sign(level,c_err);
          ACRun  [scan_pos] = run;
          ++scan_pos;
          run=-1;
          ilev=level;
        }
        ilev=(sign(((ilev<<q_bits)/quant_coef[qp_rem][i][j]),c_err)+predicted_chroma_block[n1+i][n2+j] )* quant_coef[qp_rem_sp][i][j];
        img->m7[n1+i][n2+j]=sign((abs(ilev)+qp_const2)>> q_bits_sp,ilev)*dequant_coef[qp_rem_sp][i][j]<<qp_per_sp;
      }
      ACLevel[scan_pos] = 0;
    }
  }

  // * reset chroma coeffs

  if(cr_cbp_tmp==2)
      cr_cbp=2;
  //     IDCT.

      //     Horizontal.
  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE)
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE)
    {
      for (j=0; j < BLOCK_SIZE; j++)
      {
        for (i=0; i < BLOCK_SIZE; i++)
        {
          m5[i]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2]);
        m6[1]=(m5[0]-m5[2]);
        m6[2]=(m5[1]>>1)-m5[3];
        m6[3]=m5[1]+(m5[3]>>1);

        for (i=0; i < 2; i++)
        {
          i1=3-i;
          img->m7[n1+i][n2+j]=m6[i]+m6[i1];
          img->m7[n1+i1][n2+j]=m6[i]-m6[i1];
        }
      }

      //     Vertical.
      for (i=0; i < BLOCK_SIZE; i++)
      {
        for (j=0; j < BLOCK_SIZE; j++)
        {
          m5[j]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2]);
        m6[1]=(m5[0]-m5[2]);
        m6[2]=(m5[1]>>1)-m5[3];
        m6[3]=m5[1]+(m5[3]>>1);

        for (j=0; j < 2; j++)
        {
          j2=3-j;
          img->m7[n1+i][n2+j] =min(255,max(0,(m6[j]+m6[j2]+DQ_ROUND)>>DQ_BITS));
          img->m7[n1+i][n2+j2]=min(255,max(0,(m6[j]-m6[j2]+DQ_ROUND)>>DQ_BITS));
        }
      }
    }
  }

  //  Decoded block moved to memory
  for (j=0; j < BLOCK_SIZE*2; j++)
    for (i=0; i < BLOCK_SIZE*2; i++)
    {
      imgUV[uv][img->pix_c_y+j][img->pix_c_x+i]= img->m7[i][j];
    }

  return cr_cbp;
}


/*!
 ************************************************************************
 * \brief
 *    The routine performs transform,quantization,inverse transform, adds the diff.
 *    to the prediction and writes the result to the decoded luma frame. Includes the
 *    RD constrained quantization also.
 *
 * \para Input:
 *    block_x,block_y: Block position inside a macro block (0,4,8,12).
 *
 * \para Output:
 *    nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels.            \n
 *    coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
 ************************************************************************
 */
void copyblock_sp(int block_x,int block_y)
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
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+DQ_ROUND)>>DQ_BITS));
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+DQ_ROUND)>>DQ_BITS));
    }
  }

  //  Decoded block moved to frame memory

  for (j=0; j < BLOCK_SIZE; j++)
    for (i=0; i < BLOCK_SIZE; i++)
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];
}
