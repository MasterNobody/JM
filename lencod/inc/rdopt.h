/*!
 ***************************************************************************
 * \file rdopt.h
 *
 * \brief
 *    Headerfile for RD-optimized mode decision
 * \author
 *    Heiko Schwarz  <hschwarz@hhi.de>
 * \date
 *    12. April 2001
 **************************************************************************
 */

#ifndef _RD_OPT_H_
#define _RD_OPT_H_

#include "global.h"
#include "elements.h"
#include "rdopt_coding_state.h"


//===== MACROBLOCK MODE CONSTANTS =====
#define  MBMODE_COPY            0
#define  MBMODE_INTER16x16      1
#define  MBMODE_INTER4x4        7
#define  MBMODE_BACKWARD16x16   8
#define  MBMODE_BACKWARD4x4    14
#define  MBMODE_BIDIRECTIONAL  15
#define  MBMODE_DIRECT         16
#define  MBMODE_INTRA16x16     17
#define  MBMODE_INTRA4x4       18
//-----------------------------
#define  NO_MAX_MBMODES        19
#define  NO_INTER_MBMODES       7



typedef struct {
  double  rdcost;
  long    distortion;
  long    distortion_UV;
  int     rate_mode;
  int     rate_motion;
  int     rate_cbp;
  int     rate_luma;
  int     rate_chroma;
} RateDistortion;



typedef struct {

  //=== lagrange multipliers ===
  double   lambda_intra;
  double   lambda_motion;
  double   lambda_mode;

  //=== cost, distortion and rate for 4x4 intra modes ===
  int      best_mode_4x4     [4][4];
  double   min_rdcost_4x4    [4][4];
  double   rdcost_4x4        [4][4][NO_INTRA_PMODE];
  long     distortion_4x4    [4][4][NO_INTRA_PMODE];
  int      rate_imode_4x4    [4][4][NO_INTRA_PMODE];
  int      rate_luma_4x4     [4][4][NO_INTRA_PMODE];

  //=== stored variables for "best" macroblock mode ===
  double   min_rdcost;
  int      best_mode;
  int      ref_or_i16mode;
  int      blocktype;
  int      blocktype_back;
  byte     rec_mb_Y     [16][16];
  byte     rec_mb_U     [ 8][ 8];
  byte     rec_mb_V     [ 8][ 8];
  int      ipredmode    [16];
  int      cbp;
  int      cbp_blk;
  int      kac;
  int      cof          [4][6][18][2][2];
  int      cofu         [5][2][2];
  int      mv           [4][4][2];
  int      bw_mv        [4][4][2];

} RDOpt;

#endif
