/*!
 *************************************************************************************
 * \file b_frame.h
 *
 * \brief
 *    Header file for B picture coding
 *
 * \author
 *    Main contributor and Contact Information
 *    - Byeong-Moon Jeon      <jeonbm@lge.com>
 *    - Yoon-Seong Soh        <yunsung@lge.com>                                     \n
 *      LG Electronics Inc., Digital Media Research Lab.                            \n
 *      16 Woomyeon-Dong, Seocho-Gu, Seoul, 137-724, Korea
 *************************************************************************************
 */
#ifndef _B_FRAME_H_
#define _B_FRAME_H_

#define MAX_DIR_SAD     100000000

extern int  ONE_FOURTH_TAP[3][2];
extern int  MODTAB[3][2];
extern int  NCBP[48][2];
extern byte PRED_IPRED[7][7][6];
extern int  IPRED_ORDER[6][6];
extern int  JQ[32][2];
extern int  QP2QUANT[32];


#endif

