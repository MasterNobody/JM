/*!
 *************************************************************************************
 * \file b_frame.h
 *
 * \brief
 *    Header file for B picture coding
 *
 * \author
 *    Main contributor and Contact Information
 *    - Byeong-Moon Jeon      <jeonbm@lge.com>                                      \n
 *      LG Electronics Inc., Digital Media Research Lab.                            \n
 *      16 Woomyeon-Dong, Seocho-Gu, Seoul, 137-724, Korea
 *************************************************************************************
 */
#ifndef _B_FRAME_H_
#define _B_FRAME_H_


#define   SINGLE_SCAN 0
#define   DOUBLE_SCAN 1


extern int ONE_FOURTH_TAP[3][2];
extern const byte NCBP[48][2];
extern byte PRED_IPRED[7][7][6];
extern const byte IPRED_ORDER[36][2];
extern const int BLOCK_STEP[8][2];
extern const byte SNGL_SCAN[16][2];
extern const int JQ1[];
extern const byte DBL_SCAN[8][2][2];
extern const byte QP_SCALE_CR[32];

extern int two[6];
extern int three[6];
extern int five[6][6];
extern int six[6][6];
extern int seven[6][6];

int **fw_refFrArr, ** bw_refFrArr;

int ***dfMV;  // [92][72][2]
int ***dbMV;  // [92][72][2]


#endif