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
 * \file mv-search.c
 *
 * \brief
 *    Motion Vector Search, unified for B and P Pictures
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *      - Inge Lille-Lang�y               <inge.lille-langoy@telenor.com>
 *      - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *      - Stephan Wenger                  <stewe@cs.tu-berlin.de>
 *      - Jani Lainema                    <jani.lainema@nokia.com>
 *      - Detlev Marpe                    <marpe@hhi.de>
 *      - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *      - Heiko Schwarz                   <hschwarz@hhi.de>
 *
 *************************************************************************************
 * \todo
 *   6. Unite the 1/2, 1/4 and1/8th pel search routines into a single routine       \n
 *   7. Clean up parameters, use sensemaking variable names                         \n
 *   9. Implement fast leaky search, such as UBCs Diamond search
 *
 *************************************************************************************
*/

#include "contributors.h"

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "global.h"
#include "mv-search.h"
#include "refbuf.h"
#include "memalloc.h"
#include "abt.h"


// These procedure pointers are used by motion_search() and one_eigthpel()
static pel_t (*PelY_18) (pel_t**, int, int);
static pel_t (*PelY_14) (pel_t**, int, int);
static pel_t *(*PelYline_11) (pel_t *, int, int);

// Statistics, temporary
int    max_mvd;
int*   spiral_search_x;
int*   spiral_search_y;
int*   mvbits;
int*   refbits;
int*   byte_abs;
int*** motion_cost;


void SetMotionVectorPredictor (int  pmv[2],
                               int  **refFrArr,
                               int  ***tmp_mv,
                               int  ref_frame,
                               int  mb_x,
                               int  mb_y,
                               int  blockshape_x,
                               int  blockshape_y);

#ifdef _FAST_FULL_ME_

/*****
 *****  static variables for fast integer motion estimation
 *****
 */
static int  *search_setup_done;  //!< flag if all block SAD's have been calculated yet
static int  *search_center_x;    //!< absolute search center for fast full motion search
static int  *search_center_y;    //!< absolute search center for fast full motion search
static int  *pos_00;             //!< position of (0,0) vector
static int  ****BlockSAD;        //!< SAD for all blocksize, ref. frames and motion vectors
static int  *max_search_range;


/*!
 ***********************************************************************
 * \brief
 *    function creating arrays for fast integer motion estimation
 ***********************************************************************
 */
void
InitializeFastFullIntegerSearch ()
{
  int  i, j, k;
  int  search_range = input->search_range;
  int  max_pos      = (2*search_range+1) * (2*search_range+1);

  if(input->InterlaceCodingOption != FRAME_CODING) 
    img->buf_cycle *= 2;

  if ((BlockSAD = (int****)malloc ((img->buf_cycle+1) * sizeof(int***))) == NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
  for (i = 0; i <= img->buf_cycle; i++)
  {
    if ((BlockSAD[i] = (int***)malloc (8 * sizeof(int**))) == NULL)
      no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
    for (j = 1; j < 8; j++)
    {
      if ((BlockSAD[i][j] = (int**)malloc (16 * sizeof(int*))) == NULL)
        no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
      for (k = 0; k < 16; k++)
      {
        if ((BlockSAD[i][j][k] = (int*)malloc (max_pos * sizeof(int))) == NULL)
          no_mem_exit ("InitializeFastFullIntegerSearch: BlockSAD");
      }
    }
  }

  if ((search_setup_done = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: search_setup_done");
  if ((search_center_x = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: search_center_x");
  if ((search_center_y = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: search_center_y");
  if ((pos_00 = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: pos_00");
  if ((max_search_range = (int*)malloc ((img->buf_cycle+1)*sizeof(int)))==NULL)
    no_mem_exit ("InitializeFastFullIntegerSearch: max_search_range");

  // assign max search ranges for reference frames
  if (input->full_search == 2)
  {
    for (i=0; i<=img->buf_cycle; i++)  max_search_range[i] = search_range;
  }
  else
  {
    max_search_range[0] = max_search_range[img->buf_cycle] = search_range;
    for (i=1; i< img->buf_cycle; i++)  max_search_range[i] = search_range / 2;
  }

  if(input->InterlaceCodingOption != FRAME_CODING) 
    img->buf_cycle /= 2;
}



/*!
 ***********************************************************************
 * \brief
 *    function for deleting the arrays for fast integer motion estimation
 ***********************************************************************
 */
void
ClearFastFullIntegerSearch ()
{
  int  i, j, k;

  if(input->InterlaceCodingOption != FRAME_CODING) 
    img->buf_cycle *= 2;

  for (i = 0; i <= img->buf_cycle; i++)
  {
    for (j = 1; j < 8; j++)
    {
      for (k = 0; k < 16; k++)
      {
        free (BlockSAD[i][j][k]);
      }
      free (BlockSAD[i][j]);
    }
    free (BlockSAD[i]);
  }
  free (BlockSAD);

  free (search_setup_done);
  free (search_center_x);
  free (search_center_y);
  free (pos_00);
  free (max_search_range);

  if(input->InterlaceCodingOption != FRAME_CODING) 
    img->buf_cycle /= 2;
}


/*!
 ***********************************************************************
 * \brief
 *    function resetting flags for fast integer motion estimation
 *    (have to be called in start_macroblock())
 ***********************************************************************
 */
void
ResetFastFullIntegerSearch ()
{
  int i;
  for (i = 0; i <= img->buf_cycle; i++)
    search_setup_done [i] = 0;
}

/*!
 ***********************************************************************
 * \brief
 *    calculation of SAD for larger blocks on the basis of 4x4 blocks
 ***********************************************************************
 */
void
SetupLargerBlocks (int refindex, int max_pos)
{
#define ADD_UP_BLOCKS()   _o=*_bo; _i=*_bi; _j=*_bj; for(pos=0;pos<max_pos;pos++) _o[pos] = _i[pos] + _j[pos];
#define INCREMENT(inc)    _bo+=inc; _bi+=inc; _bj+=inc;

  int    pos, **_bo, **_bi, **_bj;
  register int *_o,   *_i,   *_j;

  //--- blocktype 6 ---
  _bo = BlockSAD[refindex][6];
  _bi = BlockSAD[refindex][7];
  _bj = _bi + 4;
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(5);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS(); INCREMENT(1);
  ADD_UP_BLOCKS();

  //--- blocktype 5 ---
  _bo = BlockSAD[refindex][5];
  _bi = BlockSAD[refindex][7];
  _bj = _bi + 1;
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS();

  //--- blocktype 4 ---
  _bo = BlockSAD[refindex][4];
  _bi = BlockSAD[refindex][6];
  _bj = _bi + 1;
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS(); INCREMENT(6);
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS();

  //--- blocktype 3 ---
  _bo = BlockSAD[refindex][3];
  _bi = BlockSAD[refindex][4];
  _bj = _bi + 8;
  ADD_UP_BLOCKS(); INCREMENT(2);
  ADD_UP_BLOCKS();

  //--- blocktype 2 ---
  _bo = BlockSAD[refindex][2];
  _bi = BlockSAD[refindex][4];
  _bj = _bi + 2;
  ADD_UP_BLOCKS(); INCREMENT(8);
  ADD_UP_BLOCKS();

  //--- blocktype 1 ---
  _bo = BlockSAD[refindex][1];
  _bi = BlockSAD[refindex][3];
  _bj = _bi + 2;
  ADD_UP_BLOCKS();
}


/*!
 ***********************************************************************
 * \brief
 *    Setup the fast search for an macroblock
 ***********************************************************************
 */
void
SetupFastFullPelSearch (int   ref)  // <--  reference frame parameter (0... or -1 (backward))
{
  int     pmv[2];
  pel_t   orig_blocks[256], *orgptr=orig_blocks, *refptr;
  int     offset_x, offset_y, x, y, range_partly_outside, ref_x, ref_y, pos, abs_x, abs_y, bindex, blky;
  int     LineSadBlk0, LineSadBlk1, LineSadBlk2, LineSadBlk3;

  int     mvshift       = (input->mv_res ? 3 : 2);
  int     refframe      = (ref>=0 ? ref             : 0);
  int     refindex      = (ref>=0 ? ref             : img->buf_cycle);
  pel_t*  ref_pic       = img->type==B_IMG? Refbuf11 [ref+((mref==mref_fld)) +1] : Refbuf11[ref];
  int**   ref_array     = ((img->type!=B_IMG && img->type!=BS_IMG)? refFrArr : ref>=0 ? fw_refFrArr : bw_refFrArr);
  int***  mv_array      = ((img->type!=B_IMG && img->type!=BS_IMG)? tmp_mv   : ref>=0 ? tmp_fwMV    : tmp_bwMV);
  int**   block_sad     = BlockSAD[refindex][7];
  int     max_width     = img->width  - 17;
  int     max_height    = img->height - 17;
  int     search_range  = max_search_range[refindex];
  int     max_pos       = (2*search_range+1) * (2*search_range+1);
  byte**  imgY_orig     = imgY_org;
  int     pix_y         = img->pix_y;
  
  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    ref_pic       = img->type==B_IMG? Refbuf11 [ref+(img->field_mode) +1] : Refbuf11[ref];
    block_sad     = BlockSAD[refindex][7];
    pix_y     = img->field_pix_y;
    if(img->top_field)
    {
      ref_array     = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr_top : ref>=0 ? fw_refFrArr_top : bw_refFrArr_top);
      mv_array      = ((img->type!=B_IMG && img->type!=BS_IMG) ? tmp_mv_top   : ref>=0 ? tmp_fwMV_top    : tmp_bwMV_top);
      imgY_orig   = imgY_org_top;
    }
    else
    {
      ref_array     = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr_bot : ref>=0 ? fw_refFrArr_bot : bw_refFrArr_bot);
      mv_array      = ((img->type!=B_IMG && img->type!=BS_IMG) ? tmp_mv_bot   : ref>=0 ? tmp_fwMV_bot    : tmp_bwMV_bot);
      imgY_orig   = imgY_org_bot;
    }

  }



  //===== get search center: predictor of 16x16 block =====
  SetMotionVectorPredictor (pmv, ref_array, mv_array, refframe, 0, 0, 16, 16);
  search_center_x[refindex] = pmv[0] / (1 << mvshift);
  search_center_y[refindex] = pmv[1] / (1 << mvshift);
  if (!input->rdopt)
  {
    //--- correct center so that (0,0) vector is inside ---
    search_center_x[refindex] = max(-search_range, min(search_range, search_center_x[refindex]));
    search_center_y[refindex] = max(-search_range, min(search_range, search_center_y[refindex]));
  }
  search_center_x[refindex] += img->pix_x;
  search_center_y[refindex] += pix_y;
  offset_x = search_center_x[refindex];
  offset_y = search_center_y[refindex];


  //===== copy original block for fast access =====
  for   (y = pix_y; y < pix_y+16; y++)
    for (x = img->pix_x; x < img->pix_x+16; x++)
      *orgptr++ = imgY_orig [y][x];


  //===== check if whole search range is inside image =====
  if (offset_x >= search_range && offset_x <= max_width  - search_range &&
      offset_y >= search_range && offset_y <= max_height - search_range   )
  {
    range_partly_outside = 0; PelYline_11 = FastLine16Y_11;
  }
  else
  {
    range_partly_outside = 1;
  }


  //===== determine position of (0,0)-vector =====
  if (!input->rdopt)
  {
    ref_x = img->pix_x - offset_x;
    ref_y = pix_y - offset_y;

    for (pos = 0; pos < max_pos; pos++)
    {
      if (ref_x == spiral_search_x[pos] &&
          ref_y == spiral_search_y[pos])
      {
        pos_00[refindex] = pos;
        break;
      }
    }
  }


  //===== loop over search range (spiral search): get blockwise SAD =====
  for (pos = 0; pos < max_pos; pos++)
  {
    abs_y = offset_y + spiral_search_y[pos];
    abs_x = offset_x + spiral_search_x[pos];

    if (range_partly_outside)
    {
      if (abs_y >= 0 && abs_y <= max_height &&
          abs_x >= 0 && abs_x <= max_width    )
      {
        PelYline_11 = FastLine16Y_11;
      }
      else
      {
        PelYline_11 = UMVLine16Y_11;
      }
    }

    orgptr = orig_blocks;
    bindex = 0;
    for (blky = 0; blky < 4; blky++)
    {
      LineSadBlk0 = LineSadBlk1 = LineSadBlk2 = LineSadBlk3 = 0;
      for (y = 0; y < 4; y++)
      {
        refptr = PelYline_11 (ref_pic, abs_y++, abs_x);

        LineSadBlk0 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk0 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk0 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk0 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk1 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk1 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk1 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk1 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk2 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk2 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk2 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk2 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk3 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk3 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk3 += byte_abs [*refptr++ - *orgptr++];
        LineSadBlk3 += byte_abs [*refptr++ - *orgptr++];
      }
      block_sad[bindex++][pos] = LineSadBlk0;
      block_sad[bindex++][pos] = LineSadBlk1;
      block_sad[bindex++][pos] = LineSadBlk2;
      block_sad[bindex++][pos] = LineSadBlk3;
    }
  }


  //===== combine SAD's for larger block types =====
  SetupLargerBlocks (refindex, max_pos);


  //===== set flag marking that search setup have been done =====
  search_setup_done[refindex] = 1;
}
#endif // _FAST_FULL_ME_


/*!
 ***********************************************************************
 * \brief
 *    setting the motion vector predictor
 ***********************************************************************
 */
void
SetMotionVectorPredictor (int  pmv[2],
                          int  **refFrArr,
                          int  ***tmp_mv,
                          int  ref_frame,
                          int  mb_x,
                          int  mb_y,
                          int  blockshape_x,
                          int  blockshape_y)
{
  int pic_block_x          = img->block_x + (mb_x>>2);
  int pic_block_y          = img->block_y + (mb_y>>2);
  int mb_nr                = img->current_mb_nr;
  int mb_width             = img->width/16;
  int mb_available_up      = (img->mb_y == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width  ].slice_nr);
  int mb_available_left    = (img->mb_x == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-1         ].slice_nr);
  int mb_available_upleft  = (img->mb_x == 0 ||
                              img->mb_y == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr);
  int mb_available_upright = (img->mb_x >= mb_width-1 ||
                              img->mb_y == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr);
  int block_available_up, block_available_left, block_available_upright, block_available_upleft;
  int mv_a, mv_b, mv_c, mv_d, pred_vec=0;
  int mvPredType, rFrameL, rFrameU, rFrameUR;
  int hv;

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    pic_block_y          = img->field_block_y + (mb_y>>2);
    mb_available_up      = (img->field_mb_y == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width  ].slice_nr);
    mb_available_left    = (img->mb_x == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-1         ].slice_nr);
    mb_available_upleft  = (img->mb_x == 0 ||
                            img->field_mb_y == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr);
    mb_available_upright = (img->mb_x >= mb_width-1 ||
                            img->field_mb_y == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr);
  }
  
  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive)
  {
    if(img->field_mode && !img->top_field)
      mb_available_upright=0; // set mb_available_upright to 0 for bottom MBs in a MB pair
    else if(!img->field_mode && img->mb_y%2)
      mb_available_upright=0; // set mb_available_upright to 0 for bottom MBs in a MB pair

  }

  /* D B C */
  /* A X   */
  
  /* 1 A, B, D are set to 0 if unavailable       */
  /* 2 If C is not available it is replaced by D */
  block_available_up   = mb_available_up   || (mb_y > 0);
  block_available_left = mb_available_left || (mb_x > 0);

  if (mb_y > 0)
  {
    if (mb_x < 8)  // first column of 8x8 blocks
    {
      if (mb_y==8)
      {
        if (blockshape_x == 16)      block_available_upright = 0;
        else                         block_available_upright = 1;
      }
      else
      {
        if (mb_x+blockshape_x != 8)  block_available_upright = 1;
        else                         block_available_upright = 0;
      }
    }
    else
    {
      if (mb_x+blockshape_x != 16)   block_available_upright = 1;
      else                           block_available_upright = 0;
    }
  }
  else if (mb_x+blockshape_x != MB_BLOCK_SIZE)
  {
    block_available_upright = block_available_up;
  }
  else
  {
    block_available_upright = mb_available_upright;
  }
  
  if (mb_x > 0)
  {
    block_available_upleft = (mb_y > 0 ? 1 : block_available_up);
  }
  else if (mb_y > 0)
  {
    block_available_upleft = block_available_left;
  }
  else
  {
    block_available_upleft = mb_available_upleft;
  }
  
//  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive)
//    block_available_upright = 0;      // temp fix for MB level field/frame coding

  mvPredType = MVPRED_MEDIAN;
  rFrameL    = block_available_left    ? refFrArr[pic_block_y]  [pic_block_x-1] : -1;
  rFrameU    = block_available_up      ? refFrArr[pic_block_y-1][pic_block_x]   : -1;
  rFrameUR   = block_available_upright ? refFrArr[pic_block_y-1][pic_block_x+blockshape_x/4] :
               block_available_upleft  ? refFrArr[pic_block_y-1][pic_block_x-1] : -1;
  
  
  
  /* Prediction if only one of the neighbors uses the reference frame
  * we are checking
  */
  
  if(rFrameL == ref_frame && rFrameU != ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_L;
  else if(rFrameL != ref_frame && rFrameU == ref_frame && rFrameUR != ref_frame)
    mvPredType = MVPRED_U;
  else if(rFrameL != ref_frame && rFrameU != ref_frame && rFrameUR == ref_frame)
    mvPredType = MVPRED_UR;
  // Directional predictions 
  else if(blockshape_x == 8 && blockshape_y == 16)
  {
    if(mb_x == 0)
    {
      if(rFrameL == ref_frame)
        mvPredType = MVPRED_L;
    }
    else
    {
      if(rFrameUR == ref_frame)
        mvPredType = MVPRED_UR;
    }
  }
  else if(blockshape_x == 16 && blockshape_y == 8)
  {
    if(mb_y == 0)
    {
      if(rFrameU == ref_frame)
        mvPredType = MVPRED_U;
    }
    else
    {
      if(rFrameL == ref_frame)
        mvPredType = MVPRED_L;
    }
  }
  
#define MEDIAN(a,b,c)  (a>b?a>c?b>c?b:c:a:b>c?a>c?a:c:b)
  
  for (hv=0; hv < 2; hv++)
  {
    mv_a = block_available_left    ? tmp_mv[hv][pic_block_y  ][4+pic_block_x-1]              : 0;
    mv_b = block_available_up      ? tmp_mv[hv][pic_block_y-1][4+pic_block_x]                : 0;
    mv_d = block_available_upleft  ? tmp_mv[hv][pic_block_y-1][4+pic_block_x-1]              : 0;
    mv_c = block_available_upright ? tmp_mv[hv][pic_block_y-1][4+pic_block_x+blockshape_x/4] : mv_d;
    
    switch (mvPredType)
    {
    case MVPRED_MEDIAN:
      if(!(block_available_upleft || block_available_up || block_available_upright))
        pred_vec = mv_a;
      else
        pred_vec = MEDIAN (mv_a, mv_b, mv_c);
      break;
      
    case MVPRED_L:
      pred_vec = mv_a;
      break;
    case MVPRED_U:
      pred_vec = mv_b;
      break;
    case MVPRED_UR:
      pred_vec = mv_c;
      break;
    default:
      break;
    }

    pmv[hv] = pred_vec;
  }
#undef MEDIAN
}


/*!
 ************************************************************************
 * \brief
 *    Initialize the motion search
 ************************************************************************
 */
void
Init_Motion_Search_Module ()
{
  int bits, i, imin, imax, k, l;

  int search_range               = input->search_range;
  int number_of_reference_frames = img->buf_cycle;
  int max_search_points          = (2*search_range+1)*(2*search_range+1);
  int max_ref_bits               = 1 + 2 * (int)floor(log(max(16,number_of_reference_frames+1)) / log(2) + 1e-10);
  int max_ref                    = (1<<((max_ref_bits>>1)+1))-1;
  int number_of_subpel_positions = (input->mv_res?8:4) * (2*search_range+3);
  int max_mv_bits                = 3 + 2 * (int)ceil (log(number_of_subpel_positions+1) / log(2) + 1e-10);
  max_mvd                        = (1<<( max_mv_bits >>1)   )-1;


  //=====   CREATE ARRAYS   =====
  //-----------------------------
  if ((spiral_search_x = (int*)calloc(max_search_points, sizeof(int))) == NULL)
    no_mem_exit("Init_Motion_Search_Module: spiral_search_x");
  if ((spiral_search_y = (int*)calloc(max_search_points, sizeof(int))) == NULL)
    no_mem_exit("Init_Motion_Search_Module: spiral_search_y");
  if ((mvbits = (int*)calloc(2*max_mvd+1, sizeof(int))) == NULL)
    no_mem_exit("Init_Motion_Search_Module: mvbits");
  if ((refbits = (int*)calloc(max_ref, sizeof(int))) == NULL)
    no_mem_exit("Init_Motion_Search_Module: refbits");
  if ((byte_abs = (int*)calloc(512, sizeof(int))) == NULL)
    no_mem_exit("Init_Motion_Search_Module: byte_abs");
  get_mem3Dint (&motion_cost, 8, 2*(img->buf_cycle+1), 4);

  //--- set array offsets ---
  mvbits   += max_mvd;
  byte_abs += 256;


  //=====   INIT ARRAYS   =====
  //---------------------------
  //--- init array: motion vector bits ---
  mvbits[0] = 1;
  for (bits=3; bits<=max_mv_bits; bits+=2)
  {
    imax = 1    << (bits >> 1);
    imin = imax >> 1;

    for (i = imin; i < imax; i++)   mvbits[-i] = mvbits[i] = bits;
  }
  //--- init array: reference frame bits ---
  refbits[0] = 1;
  for (bits=3; bits<=max_ref_bits; bits+=2)
  {
    imax = (1   << ((bits >> 1) + 1)) - 1;
    imin = imax >> 1;

    for (i = imin; i < imax; i++)   refbits[i] = bits;
  }
  //--- init array: absolute value ---
  byte_abs[0] = 0;
  for (i=1; i<256; i++)   byte_abs[i] = byte_abs[-i] = i;
  //--- init array: search pattern ---
  spiral_search_x[0] = spiral_search_y[0] = 0;
  for (k=1, l=1; l<=max(1,search_range); l++)
  {
    for (i=-l+1; i< l; i++)
    {
      spiral_search_x[k] =  i;  spiral_search_y[k++] = -l;
      spiral_search_x[k] =  i;  spiral_search_y[k++] =  l;
    }
    for (i=-l;   i<=l; i++)
    {
      spiral_search_x[k] = -l;  spiral_search_y[k++] =  i;
      spiral_search_x[k] =  l;  spiral_search_y[k++] =  i;
    }
  }

#ifdef _FAST_FULL_ME_
  InitializeFastFullIntegerSearch ();
#endif
}


/*!
 ************************************************************************
 * \brief
 *    Free memory used by motion search
 ************************************************************************
 */
void
Clear_Motion_Search_Module ()
{
  //--- correct array offset ---
  mvbits   -= max_mvd;
  byte_abs -= 256;

  //--- delete arrays ---
  free (spiral_search_x);
  free (spiral_search_y);
  free (mvbits);
  free (refbits);
  free (byte_abs);
  free_mem3Dint (motion_cost, 8);

#ifdef _FAST_FULL_ME_
  ClearFastFullIntegerSearch ();
#endif
}



/*!
 ***********************************************************************
 * \brief
 *    Full pixel block motion search
 ***********************************************************************
 */
int                                               //  ==> minimum motion cost after search
FullPelBlockMotionSearch (pel_t**   orig_pic,     // <--  original pixel values for the AxB block
                          int       ref,          // <--  reference frame (0... or -1 (backward))
                          int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
                          int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
                          int       blocktype,    // <--  block type (1-16x16 ... 7-4x4)
                          int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
                          int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
                          int*      mv_x,         // <--> in: search center (x) / out: motion vector (x) - in pel units
                          int*      mv_y,         // <--> in: search center (y) / out: motion vector (y) - in pel units
                          int       search_range, // <--  1-d search range in pel units
                          int       min_mcost,    // <--  minimum motion cost (cost for center or huge value)
                          double    lambda)       // <--  lagrangian parameter for determining motion cost
{
  int   pos, cand_x, cand_y, y, x4, mcost;
  pel_t *orig_line, *ref_line;
  pel_t *(*get_ref_line)(int, pel_t*, int, int);

  pel_t*  ref_pic       = img->type==B_IMG? Refbuf11 [ref+((mref==mref_fld)) +1] : Refbuf11[ref];
  int   best_pos      = 0;                                        // position with minimum motion cost
  int   max_pos       = (2*search_range+1)*(2*search_range+1);    // number of search positions
  int   lambda_factor = LAMBDA_FACTOR (lambda);                   // factor for determining lagragian motion cost
  int   mvshift       = (input->mv_res ? 3 : 2);                  // motion vector shift for getting sub-pel units
  int   blocksize_y   = input->blc_size[blocktype][1];            // vertical block size
  int   blocksize_x   = input->blc_size[blocktype][0];            // horizontal block size
  int   blocksize_x4  = blocksize_x >> 2;                         // horizontal block size in 4-pel units
  int   pred_x        = (pic_pix_x << mvshift) + pred_mv_x;       // predicted position x (in sub-pel units)
  int   pred_y        = (pic_pix_y << mvshift) + pred_mv_y;       // predicted position y (in sub-pel units)
  int   center_x      = pic_pix_x + *mv_x;                        // center position x (in pel units)
  int   center_y      = pic_pix_y + *mv_y;                        // center position y (in pel units)
  int   check_for_00  = (blocktype==1 && !input->rdopt && img->type!=B_IMG && ref==0);

  //===== set function for getting reference picture lines =====
  if ((center_x > search_range) && (center_x < img->width -1-search_range-blocksize_x) &&
      (center_y > search_range) && (center_y < img->height-1-search_range-blocksize_y)   )
  {
     get_ref_line = FastLineX;
  }
  else
  {
     get_ref_line = UMVLineX;
  }


  //===== loop over all search positions =====
  for (pos=0; pos<max_pos; pos++)
  {
    //--- set candidate position (absolute position in pel units) ---
    cand_x = center_x + spiral_search_x[pos];
    cand_y = center_y + spiral_search_y[pos];

    //--- initialize motion cost (cost for motion vector) and check ---
    mcost = MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_x, pred_y);
    if (check_for_00 && cand_x==pic_pix_x && cand_y==pic_pix_y)
    {
      mcost -= WEIGHTED_COST (lambda_factor, 16);
    }
    if (mcost >= min_mcost)   continue;

    //--- add residual cost to motion cost ---
    for (y=0; y<blocksize_y; y++)
    {
      ref_line  = get_ref_line (blocksize_x, ref_pic, cand_y+y, cand_x);
      orig_line = orig_pic [y];

      for (x4=0; x4<blocksize_x4; x4++)
      {
        mcost += byte_abs[ *orig_line++ - *ref_line++ ];
        mcost += byte_abs[ *orig_line++ - *ref_line++ ];
        mcost += byte_abs[ *orig_line++ - *ref_line++ ];
        mcost += byte_abs[ *orig_line++ - *ref_line++ ];
      }

      if (mcost >= min_mcost)
      {
        break;
      }
    }

    //--- check if motion cost is less than minimum cost ---
    if (mcost < min_mcost)
    {
      best_pos  = pos;
      min_mcost = mcost;
    }
  }


  //===== set best motion vector and return minimum motion cost =====
  if (best_pos)
  {
    *mv_x += spiral_search_x[best_pos];
    *mv_y += spiral_search_y[best_pos];
  }
  return min_mcost;
}


#ifdef _FAST_FULL_ME_
/*!
 ***********************************************************************
 * \brief
 *    Fast Full pixel block motion search
 ***********************************************************************
 */
int                                                   //  ==> minimum motion cost after search
FastFullPelBlockMotionSearch (pel_t**   orig_pic,     // <--  not used
                              int       ref,          // <--  reference frame (0... or -1 (backward))
                              int       pic_pix_x,    // <--  absolute x-coordinate of regarded AxB block
                              int       pic_pix_y,    // <--  absolute y-coordinate of regarded AxB block
                              int       blocktype,    // <--  block type (1-16x16 ... 7-4x4)
                              int       pred_mv_x,    // <--  motion vector predictor (x) in sub-pel units
                              int       pred_mv_y,    // <--  motion vector predictor (y) in sub-pel units
                              int*      mv_x,         //  --> motion vector (x) - in pel units
                              int*      mv_y,         //  --> motion vector (y) - in pel units
                              int       search_range, // <--  1-d search range in pel units
                              int       min_mcost,    // <--  minimum motion cost (cost for center or huge value)
                              double    lambda)       // <--  lagrangian parameter for determining motion cost
{
  int   pos, offset_x, offset_y, cand_x, cand_y, mcost;

  int   refindex      = (ref!=-1 ? ref : img->buf_cycle);                   // reference frame index (array entry)
  int   max_pos       = (2*search_range+1)*(2*search_range+1);              // number of search positions
  int   lambda_factor = LAMBDA_FACTOR (lambda);                             // factor for determining lagragian motion cost
  int   mvshift       = (input->mv_res ? 3 : 2);                            // motion vector shift for getting sub-pel units
  int   best_pos      = 0;                                                  // position with minimum motion cost
  int   block_index   = (pic_pix_y-img->pix_y)+((pic_pix_x-img->pix_x)>>2); // block index for indexing SAD array
  int*  block_sad     = BlockSAD[refindex][blocktype][block_index];         // pointer to SAD array
  int   pix_y         = img->pix_y;

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    pix_y = img->field_pix_y;
    block_index   = (pic_pix_y-pix_y)+((pic_pix_x-img->pix_x)>>2); 
    block_sad     = BlockSAD[refindex][blocktype][block_index];
  }

  //===== set up fast full integer search if needed / set search center =====
  if (!search_setup_done[refindex])
  {
    SetupFastFullPelSearch (ref);
  }
  offset_x = search_center_x[refindex] - img->pix_x;
  offset_y = search_center_y[refindex] - pix_y;


  //===== cost for (0,0)-vector: it is done before, because MVCost can be negative =====
  if (!input->rdopt)
  {
    mcost = block_sad[pos_00[refindex]] + MV_COST (lambda_factor, mvshift, 0, 0, pred_mv_x, pred_mv_y);

    if (mcost < min_mcost)
    {
      min_mcost = mcost;
      best_pos  = pos_00[refindex];
    }
  }

  //===== loop over all search positions =====
  for (pos=0; pos<max_pos; pos++, block_sad++)
  {
    //--- check residual cost ---
    if (*block_sad < min_mcost)
    {
      //--- get motion vector cost ---
      cand_x = offset_x + spiral_search_x[pos];
      cand_y = offset_y + spiral_search_y[pos];
      mcost  = *block_sad;
      mcost += MV_COST (lambda_factor, mvshift, cand_x, cand_y, pred_mv_x, pred_mv_y);

      //--- check motion cost ---
      if (mcost < min_mcost)
      {
        min_mcost = mcost;
        best_pos  = pos;
      }
    }
  }

  //===== set best motion vector and return minimum motion cost =====
  *mv_x = offset_x + spiral_search_x[best_pos];
  *mv_y = offset_y + spiral_search_y[best_pos];
  return min_mcost;
}
#endif


/*!
 ***********************************************************************
 * \brief
 *    Calculate SA(T)D
 ***********************************************************************
 */
int
SATD (int* diff, int use_hadamard)
{
  int k, satd = 0, m[16], dd, *d=diff;
  
  if (use_hadamard)
  {
    /*===== hadamard transform =====*/
    m[ 0] = d[ 0] + d[12];
    m[ 4] = d[ 4] + d[ 8];
    m[ 8] = d[ 4] - d[ 8];
    m[12] = d[ 0] - d[12];
    m[ 1] = d[ 1] + d[13];
    m[ 5] = d[ 5] + d[ 9];
    m[ 9] = d[ 5] - d[ 9];
    m[13] = d[ 1] - d[13];
    m[ 2] = d[ 2] + d[14];
    m[ 6] = d[ 6] + d[10];
    m[10] = d[ 6] - d[10];
    m[14] = d[ 2] - d[14];
    m[ 3] = d[ 3] + d[15];
    m[ 7] = d[ 7] + d[11];
    m[11] = d[ 7] - d[11];
    m[15] = d[ 3] - d[15];
    
    d[ 0] = m[ 0] + m[ 4];
    d[ 8] = m[ 0] - m[ 4];
    d[ 4] = m[ 8] + m[12];
    d[12] = m[12] - m[ 8];
    d[ 1] = m[ 1] + m[ 5];
    d[ 9] = m[ 1] - m[ 5];
    d[ 5] = m[ 9] + m[13];
    d[13] = m[13] - m[ 9];
    d[ 2] = m[ 2] + m[ 6];
    d[10] = m[ 2] - m[ 6];
    d[ 6] = m[10] + m[14];
    d[14] = m[14] - m[10];
    d[ 3] = m[ 3] + m[ 7];
    d[11] = m[ 3] - m[ 7];
    d[ 7] = m[11] + m[15];
    d[15] = m[15] - m[11];
    
    m[ 0] = d[ 0] + d[ 3];
    m[ 1] = d[ 1] + d[ 2];
    m[ 2] = d[ 1] - d[ 2];
    m[ 3] = d[ 0] - d[ 3];
    m[ 4] = d[ 4] + d[ 7];
    m[ 5] = d[ 5] + d[ 6];
    m[ 6] = d[ 5] - d[ 6];
    m[ 7] = d[ 4] - d[ 7];
    m[ 8] = d[ 8] + d[11];
    m[ 9] = d[ 9] + d[10];
    m[10] = d[ 9] - d[10];
    m[11] = d[ 8] - d[11];
    m[12] = d[12] + d[15];
    m[13] = d[13] + d[14];
    m[14] = d[13] - d[14];
    m[15] = d[12] - d[15];
    
    d[ 0] = m[ 0] + m[ 1];
    d[ 1] = m[ 0] - m[ 1];
    d[ 2] = m[ 2] + m[ 3];
    d[ 3] = m[ 3] - m[ 2];
    d[ 4] = m[ 4] + m[ 5];
    d[ 5] = m[ 4] - m[ 5];
    d[ 6] = m[ 6] + m[ 7];
    d[ 7] = m[ 7] - m[ 6];
    d[ 8] = m[ 8] + m[ 9];
    d[ 9] = m[ 8] - m[ 9];
    d[10] = m[10] + m[11];
    d[11] = m[11] - m[10];
    d[12] = m[12] + m[13];
    d[13] = m[12] - m[13];
    d[14] = m[14] + m[15];
    d[15] = m[15] - m[14];
    
    /*===== sum up =====*/
    for (dd=diff[k=0]; k<16; dd=diff[++k])
    {
      satd += (dd < 0 ? -dd : dd);
    }
    satd >>= 1;
  }
  else
  {
    /*===== sum up =====*/
    for (k = 0; k < 16; k++)
    {
      satd += byte_abs [diff [k]];
    }
  }
  
  return satd;
}



/*!
 ***********************************************************************
 * \brief
 *    Sub pixel block motion search
 ***********************************************************************
 */
int                                               //  ==> minimum motion cost after search
SubPelBlockMotionSearch (pel_t**   orig_pic,      // <--  original pixel values for the AxB block
                         int       ref,           // <--  reference frame (0... or -1 (backward))
                         int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                         int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                         int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
                         int       pred_mv_x,     // <--  motion vector predictor (x) in sub-pel units
                         int       pred_mv_y,     // <--  motion vector predictor (y) in sub-pel units
                         int*      mv_x,          // <--> in: search center (x) / out: motion vector (x) - in pel units
                         int*      mv_y,          // <--> in: search center (y) / out: motion vector (y) - in pel units
                         int       search_pos2,   // <--  search positions for    half-pel search  (default: 9)
                         int       search_pos4,   // <--  search positions for quarter-pel search  (default: 9)
                         int       search_pos8,   // <--  search positions for  eighth-pel search  (default: 9)
                         int       min_mcost,     // <--  minimum motion cost (cost for center or huge value)
                         double    lambda,        // <--  lagrangian parameter for determining motion cost
                         int       useABT)        // <--  use ABT SATD calculation
{
  int   diff[16], *d;
  int   pos, best_pos, mcost, abort_search;
  int   y0, x0, ry0, rx0, ry;
  int   cand_mv_x, cand_mv_y;
  pel_t *orig_line;
  int   incr            = ref==-1 ? ((!img->fld_type)&&(mref==mref_fld)&&(img->type==B_IMG)) : (mref==mref_fld)&&(img->type==B_IMG) ;
  pel_t **ref_pic       = img->type==B_IMG? mref [ref+1+incr] : mref [ref];
  int   lambda_factor   = LAMBDA_FACTOR (lambda);
  int   mv_shift        = (input->mv_res ? 1 : 0);
  int   check_position0 = (blocktype==1 && *mv_x==0 && *mv_y==0 && input->hadamard && !input->rdopt && img->type!=B_IMG && ref==0);
  int   blocksize_x     = input->blc_size[blocktype][0];
  int   blocksize_y     = input->blc_size[blocktype][1];
  int   pic4_pix_x      = (pic_pix_x << 2);
  int   pic4_pix_y      = (pic_pix_y << 2);
  int   max_pos_x4      = ((img->width -blocksize_x+1)<<2);
  int   max_pos_y4      = ((img->height-blocksize_y+1)<<2);
  int   min_pos2        = (input->hadamard ? 0 : 1);
  int   max_pos2        = (input->hadamard ? max(1,search_pos2) : search_pos2);
  int   curr_diff[MB_BLOCK_SIZE][MB_BLOCK_SIZE]; // for ABT SATD calculation
  int   xx,yy,kk;                                // indicees for curr_diff

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode && img->type == B_IMG)
  {
    incr    =  ref == -1 ? (img->top_field && img->field_mode):(img->field_mode);
    ref_pic =  mref[ref+1+incr];
  }


  
  /*********************************
   *****                       *****
   *****  HALF-PEL REFINEMENT  *****
   *****                       *****
   *********************************/
  //===== convert search center to quarter-pel units =====
  *mv_x <<= 2;
  *mv_y <<= 2;
  //===== set function for getting pixel values =====
  if ((pic4_pix_x + *mv_x > 1) && (pic4_pix_x + *mv_x < max_pos_x4 - 2) &&
      (pic4_pix_y + *mv_y > 1) && (pic4_pix_y + *mv_y < max_pos_y4 - 2)   )
  {
    PelY_14 = FastPelY_14;
  }
  else
  {
    PelY_14 = UMVPelY_14;
  }
  //===== loop over search positions =====
  for (best_pos = 0, pos = min_pos2; pos < max_pos2; pos++)
  {
    cand_mv_x = *mv_x + (spiral_search_x[pos] << 1);    // quarter-pel units
    cand_mv_y = *mv_y + (spiral_search_y[pos] << 1);    // quarter-pel units

    //----- set motion vector cost -----
    mcost = MV_COST (lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y);
    if (check_position0 && pos==0)
    {
      mcost -= WEIGHTED_COST (lambda_factor, 16);
    }

    //----- add up SATD -----
    for (y0=0, abort_search=0; y0<blocksize_y && !abort_search; y0+=4)
    {
      ry0 = ((pic_pix_y+y0)<<2) + cand_mv_y;

      for (x0=0; x0<blocksize_x; x0+=4)
      {
        rx0 = ((pic_pix_x+x0)<<2) + cand_mv_x;
        d   = diff;

        orig_line = orig_pic [y0  ];    ry=ry0;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d++      = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        orig_line = orig_pic [y0+1];    ry=ry0+4;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d++      = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        orig_line = orig_pic [y0+2];    ry=ry0+8;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d++      = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        orig_line = orig_pic [y0+3];    ry=ry0+12;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d        = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        if (!useABT)
        {
          if ((mcost += SATD (diff, input->hadamard)) > min_mcost)
          {
            abort_search = 1;
            break;
          }
        }
        else  // copy diff to curr_diff for ABT SATD calculation
        {
          for (yy=y0,kk=0; yy<y0+4; yy++)
            for (xx=x0; xx<x0+4; xx++, kk++)
              curr_diff[yy][xx] = diff[kk];
        }
      }
    }
    if (useABT)
      mcost += find_sad_abt(input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff);

    if (mcost < min_mcost)
    {
      min_mcost = mcost;
      best_pos  = pos;
    }
  }
  if (best_pos)
  {
    *mv_x += (spiral_search_x [best_pos] << 1);
    *mv_y += (spiral_search_y [best_pos] << 1);
  }


  /************************************
   *****                          *****
   *****  QUARTER-PEL REFINEMENT  *****
   *****                          *****
   ************************************/
  //===== set function for getting pixel values =====
  if ((pic4_pix_x + *mv_x > 1) && (pic4_pix_x + *mv_x < max_pos_x4 - 1) &&
      (pic4_pix_y + *mv_y > 1) && (pic4_pix_y + *mv_y < max_pos_y4 - 1)   )
  {
    PelY_14 = FastPelY_14;
  }
  else
  {
    PelY_14 = UMVPelY_14;
  }
  //===== loop over search positions =====
  for (best_pos = 0, pos = 1; pos < search_pos4; pos++)
  {
    cand_mv_x = *mv_x + spiral_search_x[pos];    // quarter-pel units
    cand_mv_y = *mv_y + spiral_search_y[pos];    // quarter-pel units

    //----- set motion vector cost -----
    mcost = MV_COST (lambda_factor, mv_shift, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y);

    //----- add up SATD -----
    for (y0=0, abort_search=0; y0<blocksize_y && !abort_search; y0+=4)
    {
      ry0 = ((pic_pix_y+y0)<<2) + cand_mv_y;

      for (x0=0; x0<blocksize_x; x0+=4)
      {
        rx0 = ((pic_pix_x+x0)<<2) + cand_mv_x;
        d   = diff;

        orig_line = orig_pic [y0  ];    ry=ry0;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d++      = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        orig_line = orig_pic [y0+1];    ry=ry0+4;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d++      = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        orig_line = orig_pic [y0+2];    ry=ry0+8;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d++      = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        orig_line = orig_pic [y0+3];    ry=ry0+12;
        *d++      = orig_line[x0  ]  -  PelY_14 (ref_pic, ry, rx0   );
        *d++      = orig_line[x0+1]  -  PelY_14 (ref_pic, ry, rx0+ 4);
        *d++      = orig_line[x0+2]  -  PelY_14 (ref_pic, ry, rx0+ 8);
        *d        = orig_line[x0+3]  -  PelY_14 (ref_pic, ry, rx0+12);

        if (!useABT)
        {
          if ((mcost += SATD (diff, input->hadamard)) > min_mcost)
          {
            abort_search = 1;
            break;
          }
        }
        else  // copy diff to curr_diff for ABT SATD calculation
        {
          for (yy=y0,kk=0; yy<y0+4; yy++)
            for (xx=x0; xx<x0+4; xx++, kk++)
              curr_diff[yy][xx] = diff[kk];
        }
      }
    }
    if (useABT)
      mcost += find_sad_abt(input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff);

    if (mcost < min_mcost)
    {
      min_mcost = mcost;
      best_pos  = pos;
    }
  }
  if (best_pos)
  {
    *mv_x += spiral_search_x [best_pos];
    *mv_y += spiral_search_y [best_pos];
  }


  /***********************************
   *****                         *****
   *****  EIGHTH-PEL REFINEMENT  *****
   *****                         *****
   ***********************************/
  if (input->mv_res)
  {
    //===== convert search center to eighth-pel units =====
    *mv_x <<= 1;
    *mv_y <<= 1;
    //===== set function for getting pixel values =====
    if ((2*pic4_pix_x + *mv_x > 1) && (2*pic4_pix_x + *mv_x < 2*max_pos_x4 - 2) &&
        (2*pic4_pix_y + *mv_y > 1) && (2*pic4_pix_y + *mv_y < 2*max_pos_y4 - 2)   )
    {
      PelY_18 = FastPelY_18;
    }
    else
    {
      PelY_18 = UMVPelY_18;
    }
    //===== loop over search positions =====
    for (best_pos = 0, pos = 1; pos < search_pos8; pos++)
    {
      cand_mv_x = *mv_x + spiral_search_x[pos];    // eighth-pel units
      cand_mv_y = *mv_y + spiral_search_y[pos];    // eighth-pel units

      //----- set motion vector cost -----
      mcost = MV_COST (lambda_factor, 0, cand_mv_x, cand_mv_y, pred_mv_x, pred_mv_y);

      //----- add up SATD -----
      for (y0=0, abort_search=0; y0<blocksize_y && !abort_search; y0+=4)
      {
        ry0 = ((pic_pix_y+y0)<<3) + cand_mv_y;

        for (x0=0; x0<blocksize_x; x0+=4)
        {
          rx0 = ((pic_pix_x+x0)<<3) + cand_mv_x;
          d   = diff;

          orig_line = orig_pic [y0  ];    ry=ry0;
          *d++      = orig_line[x0  ]  -  PelY_18 (ref_pic, ry, rx0   );
          *d++      = orig_line[x0+1]  -  PelY_18 (ref_pic, ry, rx0+ 8);
          *d++      = orig_line[x0+2]  -  PelY_18 (ref_pic, ry, rx0+16);
          *d++      = orig_line[x0+3]  -  PelY_18 (ref_pic, ry, rx0+24);

          orig_line = orig_pic [y0+1];    ry=ry0+8;
          *d++      = orig_line[x0  ]  -  PelY_18 (ref_pic, ry, rx0   );
          *d++      = orig_line[x0+1]  -  PelY_18 (ref_pic, ry, rx0+ 8);
          *d++      = orig_line[x0+2]  -  PelY_18 (ref_pic, ry, rx0+16);
          *d++      = orig_line[x0+3]  -  PelY_18 (ref_pic, ry, rx0+24);

          orig_line = orig_pic [y0+2];    ry=ry0+16;
          *d++      = orig_line[x0  ]  -  PelY_18 (ref_pic, ry, rx0   );
          *d++      = orig_line[x0+1]  -  PelY_18 (ref_pic, ry, rx0+ 8);
          *d++      = orig_line[x0+2]  -  PelY_18 (ref_pic, ry, rx0+16);
          *d++      = orig_line[x0+3]  -  PelY_18 (ref_pic, ry, rx0+24);

          orig_line = orig_pic [y0+3];    ry=ry0+24;
          *d++      = orig_line[x0  ]  -  PelY_18 (ref_pic, ry, rx0   );
          *d++      = orig_line[x0+1]  -  PelY_18 (ref_pic, ry, rx0+ 8);
          *d++      = orig_line[x0+2]  -  PelY_18 (ref_pic, ry, rx0+16);
          *d        = orig_line[x0+3]  -  PelY_18 (ref_pic, ry, rx0+24);

          if (!useABT)
          {
            if ((mcost += SATD (diff, input->hadamard)) > min_mcost)
            {
              abort_search = 1;
              break;
            }
          }
          else  // copy diff to curr_diff for ABT SATD calculation
          {
            for (yy=y0,kk=0; yy<y0+4; yy++)
              for (xx=x0; xx<x0+4; xx++, kk++)
                curr_diff[yy][xx] = diff[kk];
          }
        }
      }
      if (useABT)
        mcost += find_sad_abt(input->hadamard, blocksize_x, blocksize_y, 0, 0, curr_diff);

      if (mcost < min_mcost)
      {
        min_mcost = mcost;
        best_pos  = pos;
      }
    }
    if (best_pos)
    {
      *mv_x += spiral_search_x [best_pos];
      *mv_y += spiral_search_y [best_pos];
    }
  }

  //===== return minimum motion cost =====
  return min_mcost;
}



/*!
 ***********************************************************************
 * \brief
 *    Block motion search
 ***********************************************************************
 */
int                                         //  ==> minimum motion cost after search
BlockMotionSearch (int       ref,           // <--  reference frame (0... or -1 (backward))
                   int       pic_pix_x,     // <--  absolute x-coordinate of regarded AxB block
                   int       pic_pix_y,     // <--  absolute y-coordinate of regarded AxB block
                   int       blocktype,     // <--  block type (1-16x16 ... 7-4x4)
                   int       search_range,  // <--  1-d search range for integer-position search
                   double    lambda,        // <--  lagrangian parameter for determining motion cost
                   int       useABT)        // <--  use ABT SATD calculation
{
  static pel_t   orig_val [256];
  static pel_t  *orig_pic  [16] = {orig_val,     orig_val+ 16, orig_val+ 32, orig_val+ 48,
                                   orig_val+ 64, orig_val+ 80, orig_val+ 96, orig_val+112,
                                   orig_val+128, orig_val+144, orig_val+160, orig_val+176,
                                   orig_val+192, orig_val+208, orig_val+224, orig_val+240};

#ifndef _FAST_FULL_ME_
  int       mvshift;
#endif
  int       pred_mv_x, pred_mv_y, mv_x, mv_y, i, j;

  int       max_value = (1<<20);
  int       min_mcost = max_value;
  int       mb_x      = pic_pix_x-img->pix_x;
  int       mb_y      = pic_pix_y-img->pix_y;
  int       block_x   = (mb_x>>2);
  int       block_y   = (mb_y>>2);
  int       bsx       = input->blc_size[blocktype][0];
  int       bsy       = input->blc_size[blocktype][1];
  int       refframe  = (ref==-1 ? 0 : ref);
  int*      pred_mv;
  int**     ref_array = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr : ref>=0 ? fw_refFrArr : bw_refFrArr);
  int***    mv_array  = ((img->type!=B_IMG && img->type!=BS_IMG) ? tmp_mv   : ref>=0 ? tmp_fwMV    : tmp_bwMV);
  int*****  all_bmv   = img->all_bmv;
  int*****  all_mv    = (ref==-1 ? img->all_bmv : img->all_mv);
  byte**    imgY_org_pic = imgY_org;

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    mb_y    = pic_pix_y - img->field_pix_y;
    block_y = mb_y >> 2;
    if(img->top_field)
    {
      pred_mv   = ((img->type!=B_IMG && img->type!=BS_IMG) ? img->mv_top  : ref>=0 ? img->p_fwMV_top : img->p_bwMV_top)[mb_x>>2][mb_y>>2][refframe][blocktype];
      ref_array = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr_top : ref>=0 ? fw_refFrArr_top : bw_refFrArr_top);
      mv_array  = ((img->type!=B_IMG && img->type!=BS_IMG) ? tmp_mv_top   : ref>=0 ? tmp_fwMV_top    : tmp_bwMV_top);
      all_bmv   = img->all_bmv_top;
      all_mv    = (ref==-1 ? img->all_bmv_top    : img->all_mv_top);
      imgY_org_pic = imgY_org_top;
    }
    else
    {
      pred_mv   = ((img->type!=B_IMG && img->type!=BS_IMG) ? img->mv_bot   : ref>=0 ? img->p_fwMV_bot : img->p_bwMV_bot)[mb_x>>2][mb_y>>2][refframe][blocktype];
      ref_array = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr_bot  : ref>=0 ? fw_refFrArr_bot : bw_refFrArr_bot);
      mv_array  = ((img->type!=B_IMG && img->type!=BS_IMG) ? tmp_mv_bot    : ref>=0 ? tmp_fwMV_bot    : tmp_bwMV_bot);
      all_bmv   = img->all_bmv_bot;
      all_mv    = (ref==-1      ? img->all_bmv_bot  : img->all_mv_bot);
      imgY_org_pic = imgY_org_bot;
    }
  }
  else
    pred_mv = ((img->type!=B_IMG && img->type!=BS_IMG) ? img->mv  : ref>=0 ? img->p_fwMV : img->p_bwMV)[mb_x>>2][mb_y>>2][refframe][blocktype];



  //==================================
  //=====   GET ORIGINAL BLOCK   =====
  //==================================
  for (j = 0; j < bsy; j++)
  {
    for (i = 0; i < bsx; i++)
    {
      orig_pic[j][i] = imgY_org_pic[pic_pix_y+j][pic_pix_x+i];
    }
  }


  //===========================================
  //=====   GET MOTION VECTOR PREDICTOR   =====
  //===========================================
  SetMotionVectorPredictor (pred_mv, ref_array, mv_array, refframe, mb_x, mb_y, bsx, bsy);
  pred_mv_x = pred_mv[0];
  pred_mv_y = pred_mv[1];


  //==================================
  //=====   INTEGER-PEL SEARCH   =====
  //==================================
#ifndef _FAST_FULL_ME_

  mvshift   = (input->mv_res ? 3 : 2);

  //--- set search center ---
  mv_x = pred_mv_x / (1 << mvshift);
  mv_y = pred_mv_y / (1 << mvshift);
  if (!input->rdopt)
  {
    //--- adjust search center so that the (0,0)-vector is inside ---
    mv_x = max (-search_range, min (search_range, mv_x));
    mv_y = max (-search_range, min (search_range, mv_y));
  }

  //--- perform motion search ---
  min_mcost = FullPelBlockMotionSearch     (orig_pic, ref, pic_pix_x, pic_pix_y, blocktype,
                                            pred_mv_x, pred_mv_y, &mv_x, &mv_y, search_range,
                                            min_mcost, lambda);

#else

  // comments:   - orig_pic is not used  -> be careful
  //             - search center is automatically determined
  min_mcost = FastFullPelBlockMotionSearch (orig_pic, ref, pic_pix_x, pic_pix_y, blocktype,
                                            pred_mv_x, pred_mv_y, &mv_x, &mv_y, search_range,
                                            min_mcost, lambda);

#endif

  //==============================
  //=====   SUB-PEL SEARCH   =====
  //==============================
  if (input->hadamard)
  {
    min_mcost = max_value;
  }
  min_mcost =  SubPelBlockMotionSearch (orig_pic, ref, pic_pix_x, pic_pix_y, blocktype,
                                        pred_mv_x, pred_mv_y, &mv_x, &mv_y, 9, 9, 9,
                                        min_mcost, lambda, useABT);


  if (!input->rdopt)
  {
    // Get the skip mode cost
    if (blocktype == 1 && img->type == INTER_IMG)
    {
      int cost;

      FindSkipModeMotionVector ();

      cost  = GetSkipCostMB (lambda);
      cost -= (int)floor(8*lambda+0.4999);

      if (cost < min_mcost)
      {
        min_mcost = cost;
        mv_x      = img->all_mv [0][0][0][0][0];
        mv_y      = img->all_mv [0][0][0][0][1];
      }
    }
  }

  //===============================================
  //=====   SET MV'S AND RETURN MOTION COST   =====
  //===============================================
  for (i=0; i < (bsx>>2); i++)
  {
    for (j=0; j < (bsy>>2); j++)
    {
      all_mv[block_x+i][block_y+j][refframe][blocktype][0] = mv_x;
      all_mv[block_x+i][block_y+j][refframe][blocktype][1] = mv_y;
    }
  }
  if (img->type==BS_IMG)
  {
    for (i=0; i < (bsx>>2); i++)
    for (j=0; j < (bsy>>2); j++)
    {
      //  Backward
      all_bmv[block_x+i][block_y+j][ref][blocktype][0] = mv_x;
      all_bmv[block_x+i][block_y+j][ref][blocktype][1] = mv_y;
    }
  }
  return min_mcost;
}


/*!
 ***********************************************************************
 * \brief
 *    Motion Cost for Bidirectional modes
 ***********************************************************************
 */
int
BIDPartitionCost (int   blocktype,
                  int   block8x8,
                  int   fw_ref,
                  int   lambda_factor,
                  int   useABT)
{
  static int  bx0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,2,0,2}};
  static int  by0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,0,0,0}, {0,0,2,2}};

  int   diff[16];
  int   pic_pix_x, pic_pix_y, block_x, block_y;
  int   v, h, mcost, i, j, k;
  int   mvd_bits  = 0;
  int   parttype  = (blocktype<4?blocktype:4);
  int   step_h0   = (input->blc_size[ parttype][0]>>2);
  int   step_v0   = (input->blc_size[ parttype][1]>>2);
  int   step_h    = (input->blc_size[blocktype][0]>>2);
  int   step_v    = (input->blc_size[blocktype][1]>>2);
  int   curr_blk[MB_BLOCK_SIZE][MB_BLOCK_SIZE]; // ABT pred.error buffer
  int   bxx, byy;                               // indexing curr_blk
  int   bsx       = min(input->blc_size[blocktype][0],8);
  int   bsy       = min(input->blc_size[blocktype][1],8);
  byte** imgY_original  = imgY_org;
  int    pix_y    =   img->pix_y;
  int    *****all_mv = img->all_mv;
  int   *****all_bmv = img->all_bmv;
  int   *****p_fwMV  = img->p_fwMV;
  int   *****p_bwMV  = img->p_bwMV;

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    pix_y     = img->field_pix_y;
    if(img->top_field)
    {
      imgY_original = imgY_org_top;
      all_mv = img->all_mv_top;
      all_bmv = img->all_bmv_top;
      p_fwMV  = img->p_fwMV_top;
      p_bwMV   = img->p_bwMV_top;
    }
    else
    {
      imgY_original = imgY_org_bot;
      all_mv = img->all_mv_bot;
      all_bmv = img->all_bmv_bot;
      p_fwMV  = img->p_fwMV_bot;
      p_bwMV   = img->p_bwMV_bot;

    }
  }

  //----- cost for motion vector bits -----
  for (v=by0[parttype][block8x8]; v<by0[parttype][block8x8]+step_v0; v+=step_v)
  for (h=bx0[parttype][block8x8]; h<bx0[parttype][block8x8]+step_h0; h+=step_h)
  {
    mvd_bits += mvbits[ all_mv [h][v][fw_ref][blocktype][0] - p_fwMV[h][v][fw_ref][blocktype][0] ];
    mvd_bits += mvbits[ all_mv [h][v][fw_ref][blocktype][1] - p_fwMV[h][v][fw_ref][blocktype][1] ];
    mvd_bits += mvbits[ all_bmv[h][v][     0][blocktype][0] - p_bwMV[h][v][     0][blocktype][0] ];
    mvd_bits += mvbits[ all_bmv[h][v][     0][blocktype][1] - p_bwMV[h][v][     0][blocktype][1] ];
  }
  mcost = WEIGHTED_COST (lambda_factor, mvd_bits);

  //----- cost of residual signal -----
  for (byy=0, v=by0[parttype][block8x8]; v<by0[parttype][block8x8]+step_v0; byy+=4, v++)
  {
    pic_pix_y = pix_y + (block_y = (v<<2));

    for (bxx=0, h=bx0[parttype][block8x8]; h<bx0[parttype][block8x8]+step_h0; bxx+=4, h++)
    {
      pic_pix_x = img->pix_x + (block_x = (h<<2));

      LumaPrediction4x4 (block_x, block_y, blocktype, blocktype, fw_ref);

      for (k=j=0; j<4; j++)
      for (  i=0; i<4; i++, k++)
      {
        diff[k] = curr_blk[byy+j][bxx+i] =
                  imgY_original[pic_pix_y+j][pic_pix_x+i] - img->mpr[i+block_x][j+block_y];
      }
      if (!useABT)
        mcost += SATD (diff, input->hadamard);
    }
  }
  if (useABT)
  {
    for (byy=0; byy<8 ; byy+=bsy)
      for (bxx=0; bxx<8 ; bxx+=bsx)
        mcost += find_sad_abt(input->hadamard,bsx,bsy,bxx,byy,curr_blk);
  }

  return mcost;
}



/*!
 ***********************************************************************
 * \brief
 *    Motion Cost for ABidirectional modes
 ***********************************************************************
 */
int
ABIDPartitionCost (int   blocktype,
                   int   block8x8,
                   int*  fw_ref,
                   int*  bw_ref,
                   int   lambda_factor,
                   int*  abp_type,
                   int   useABT)
{
  static int  bx0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,2,0,2}};
  static int  by0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,0,0,0}, {0,0,2,2}};

  int   diff[16];
  int   pic_pix_x, pic_pix_y, block_x, block_y;
  int   v, h, mcost, mcost0, mcost1, i, j, k;
  int   mvd_bits  = 0;
  int   parttype  = (blocktype<4?blocktype:4);
  int   step_h0   = (input->blc_size[ parttype][0]>>2);
  int   step_v0   = (input->blc_size[ parttype][1]>>2);
  int   step_h    = (input->blc_size[blocktype][0]>>2);
  int   step_v    = (input->blc_size[blocktype][1]>>2);
  int   curr_blk[MB_BLOCK_SIZE][MB_BLOCK_SIZE]; // ABT pred.error buffer
  int   bxx, byy;                               // indexing curr_blk
  int   bsx       = min(input->blc_size[blocktype][0],8);
  int   bsy       = min(input->blc_size[blocktype][1],8);
  byte** imgY_original  = imgY_org;
  int    pix_y    =   img->pix_y;
  int    *****all_mv = img->all_mv;
  int   *****all_bmv = img->all_bmv;
  int   *****abp_all_dmv = img->abp_all_dmv;
  int   *****p_fwMV  = img->p_fwMV;
  int   *****p_bwMV  = img->p_bwMV;
  int   mv_scale;

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    pix_y     = img->field_pix_y;
    if(img->top_field)
    {
      imgY_original = imgY_org_top;
      all_mv = img->all_mv_top;
      all_bmv = img->all_bmv_top;
      p_fwMV  = img->p_fwMV_top;
      p_bwMV   = img->p_bwMV_top;
      abp_all_dmv = img->abp_all_dmv_top;
    }
    else
    {
      imgY_original = imgY_org_bot;
      all_mv = img->all_mv_bot;
      all_bmv = img->all_bmv_bot;
      p_fwMV  = img->p_fwMV_bot;
      p_bwMV   = img->p_bwMV_bot;
      abp_all_dmv = img->abp_all_dmv_bot;

    }
  }


  if ((input->explicit_B_prediction==0 || input->explicit_B_prediction==1) && img->type==BS_IMG)
  {
    //
    //  Interpolation (1/2,1/2,0)
    //
    //----- cost for motion vector bits -----
    if ((input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode) || input->InterlaceCodingOption == FIELD_CODING) 
    {
      *fw_ref = 3;
      *bw_ref = 1;
    }
    else
    {
      *fw_ref = 1;
      *bw_ref = 0;
    }
    mvd_bits = 0;
    mv_scale = 256*((*bw_ref)+1)/((*fw_ref)+1);
    for (v=by0[parttype][block8x8]; v<by0[parttype][block8x8]+step_v0; v+=step_v)
    for (h=bx0[parttype][block8x8]; h<bx0[parttype][block8x8]+step_h0; h+=step_h)
    {
      abp_all_dmv[h][v][*bw_ref][blocktype][0] = all_bmv [h][v][*bw_ref][blocktype][0] - ((mv_scale*all_mv [h][v][*fw_ref][blocktype][0]+128)>>8);
      abp_all_dmv[h][v][*bw_ref][blocktype][1] = all_bmv [h][v][*bw_ref][blocktype][1] - ((mv_scale*all_mv [h][v][*fw_ref][blocktype][1]+128)>>8);

      mvd_bits += mvbits[ all_mv [h][v][*fw_ref][blocktype][0] - p_fwMV[h][v][*fw_ref][blocktype][0] ];
      mvd_bits += mvbits[ all_mv [h][v][*fw_ref][blocktype][1] - p_fwMV[h][v][*fw_ref][blocktype][1] ];
      mvd_bits += mvbits[ abp_all_dmv[h][v][*bw_ref][blocktype][0] ];
      mvd_bits += mvbits[ abp_all_dmv[h][v][*bw_ref][blocktype][1] ];
    }
    mcost0 = WEIGHTED_COST (lambda_factor, mvd_bits);

    //----- cost of residual signal -----
    for (byy=0, v=by0[parttype][block8x8]; v<by0[parttype][block8x8]+step_v0; byy+=4, v++)
    {
      pic_pix_y = pix_y + (block_y = (v<<2));

      for (bxx=0, h=bx0[parttype][block8x8]; h<bx0[parttype][block8x8]+step_h0; bxx+=4, h++)
      {
        pic_pix_x = img->pix_x + (block_x = (h<<2));

        AbpLumaPrediction4x4 (block_x, block_y, blocktype, blocktype, *fw_ref, *bw_ref, 1);

        for (k=j=0; j<4; j++)
        for (  i=0; i<4; i++, k++)
        {
          diff[k] = curr_blk[byy+j][bxx+i] =
                    imgY_original[pic_pix_y+j][pic_pix_x+i] - img->mpr[i+block_x][j+block_y];
        }
        if (!useABT)
          mcost0 += SATD (diff, input->hadamard);
      }

    }
    if (useABT)
    {
      for (byy=0; byy<8 ; byy+=bsy)
        for (bxx=0; bxx<8 ; bxx+=bsx)
          mcost0 += find_sad_abt(input->hadamard,bsx,bsy,bxx,byy,curr_blk);
    }

    if (input->explicit_B_prediction==0 && img->type==BS_IMG)
    {
      *abp_type = 1;
      mcost = mcost0;
      if ((input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode) || input->InterlaceCodingOption == FIELD_CODING) 
      {
        *fw_ref = 3;
        *bw_ref = 1;
      }
      else
      {
        *fw_ref = 1;
        *bw_ref = 0;
      }
    }
  }
  if (input->explicit_B_prediction==1 && img->type==BS_IMG)
  {
    //
    //  Extrapolation (2,-1,0)
    //  
    //----- cost for motion vector bits -----
    if ((input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode) || input->InterlaceCodingOption == FIELD_CODING) 
    {
      *fw_ref = 1;
      *bw_ref = 3;
    }
    else
    {
      *fw_ref = 0;
      *bw_ref = 1;
    }
    mvd_bits = 0;
    mv_scale = 256*((*bw_ref)+1)/((*fw_ref)+1);
    for (v=by0[parttype][block8x8]; v<by0[parttype][block8x8]+step_v0; v+=step_v)
    for (h=bx0[parttype][block8x8]; h<bx0[parttype][block8x8]+step_h0; h+=step_h)
    {
      abp_all_dmv[h][v][*bw_ref][blocktype][0] = all_bmv [h][v][*bw_ref][blocktype][0] - ((mv_scale*all_mv [h][v][*fw_ref][blocktype][0]+128)>>8);
      abp_all_dmv[h][v][*bw_ref][blocktype][1] = all_bmv [h][v][*bw_ref][blocktype][1] - ((mv_scale*all_mv [h][v][*fw_ref][blocktype][1]+128)>>8);

      mvd_bits += mvbits[ all_mv [h][v][*fw_ref][blocktype][0] - p_fwMV[h][v][*fw_ref][blocktype][0] ];
      mvd_bits += mvbits[ all_mv [h][v][*fw_ref][blocktype][1] - p_fwMV[h][v][*fw_ref][blocktype][1] ];
      mvd_bits += mvbits[ abp_all_dmv[h][v][*bw_ref][blocktype][0] ];
      mvd_bits += mvbits[ abp_all_dmv[h][v][*bw_ref][blocktype][1] ];

    }
    mcost1 = WEIGHTED_COST (lambda_factor, mvd_bits);

    //----- cost of residual signal -----
    for (byy=0, v=by0[parttype][block8x8]; v<by0[parttype][block8x8]+step_v0; byy+=4, v++)
    {
      pic_pix_y = pix_y + (block_y = (v<<2));

      for (bxx=0, h=bx0[parttype][block8x8]; h<bx0[parttype][block8x8]+step_h0; bxx+=4, h++)
      {
        pic_pix_x = img->pix_x + (block_x = (h<<2));

        AbpLumaPrediction4x4 (block_x, block_y, blocktype, blocktype, *fw_ref, *bw_ref, 2);

        for (k=j=0; j<4; j++)
          for (  i=0; i<4; i++, k++)
          {
            diff[k] = curr_blk[byy+j][bxx+i] =
              imgY_original[pic_pix_y+j][pic_pix_x+i] - img->mpr[i+block_x][j+block_y];
          }
        if (!useABT)
          mcost1 += SATD (diff, input->hadamard);
      }

    }
    if (useABT)
    {
      for (byy=0; byy<8 ; byy+=bsy)
        for (bxx=0; bxx<8 ; bxx+=bsx)
          mcost1 += find_sad_abt(input->hadamard,bsx,bsy,bxx,byy,curr_blk);
    }

    if (mcost0<=mcost1)
    {
      *abp_type = 1;
      mcost = mcost0;
      if ((input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode) || input->InterlaceCodingOption == FIELD_CODING) 
      {
        *fw_ref = 3;
        *bw_ref = 1;
      }
      else
      {
        *fw_ref = 1;
        *bw_ref = 0;
      }
    }
    else
    {
      *abp_type = 2;
      mcost = mcost1;
      if ((input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode) || input->InterlaceCodingOption == FIELD_CODING) 
      {
        *fw_ref = 1;
        *bw_ref = 3;
      }
      else
      {
        *fw_ref = 0;
        *bw_ref = 1;
      }
    }
  }

  return mcost;
}


/*!
 ************************************************************************
 * \brief
 *    Get cost for skip mode for an macroblock
 ************************************************************************
 */
int GetSkipCostMB (double lambda)
{
  int block_y, block_x, pic_pix_y, pic_pix_x, i, j, k;
  int diff[16];
  int cost = 0;

  for (block_y=0; block_y<16; block_y+=4)
  {
    pic_pix_y = img->pix_y + block_y;

    for (block_x=0; block_x<16; block_x+=4)
    {
      pic_pix_x = img->pix_x + block_x;

      //===== prediction of 4x4 block =====
      if (img->type==BS_IMG) 
      {
        AbpLumaPrediction4x4 (block_x, block_y, 0, 0, 0, 0, 0);   
      }
      else
      {
        LumaPrediction4x4 (block_x, block_y, 0, 0, 0);
      }

      //===== get displaced frame difference ======                
      for (k=j=0; j<4; j++)
        for (i=0; i<4; i++, k++)
        {
          diff[k] = imgY_org[pic_pix_y+j][pic_pix_x+i] - img->mpr[i+block_x][j+block_y];
        }
      cost += SATD (diff, input->hadamard);
    }
  }

  return cost;
}

/*!
 ************************************************************************
 * \brief
 *    Find motion vector for the Skip mode
 ************************************************************************
 */
void FindSkipModeMotionVector ()
{
  int bx, by;
  int mb_available_up   = (img->mb_y == 0)  ? 0 : (img->mb_data[img->current_mb_nr].slice_nr == img->mb_data[img->current_mb_nr-img->width/16].slice_nr);
  int mb_available_left = (img->mb_x == 0)  ? 0 : (img->mb_data[img->current_mb_nr].slice_nr == img->mb_data[img->current_mb_nr-1                    ].slice_nr);
  int zeroMotionAbove   = !mb_available_up  ? 1 : refFrArr[img->block_y-1][img->block_x] == 0 && tmp_mv[0][img->block_y-1][4+img->block_x]   == 0 && tmp_mv[1][img->block_y-1][4+img->block_x]   == 0 ? 1 : 0;
  int zeroMotionLeft    = !mb_available_left? 1 : refFrArr[img->block_y][img->block_x-1] == 0 && tmp_mv[0][img->block_y  ][4+img->block_x-1] == 0 && tmp_mv[1][img->block_y  ][4+img->block_x-1] == 0 ? 1 : 0;
  int mb_y = img->mb_y;
  int block_y = img->block_y;
  int **refar = refFrArr;
  int ***tmpmv = tmp_mv;
  int *****all_mv = img->all_mv;
  int *****mv  = img->mv;

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    mb_y  =img->field_mb_y;
    block_y = img->field_block_y;
    if(img->top_field)
    {
      all_mv=img->all_mv_top;
      mv  =img->mv_top;
      tmpmv =tmp_mv_top;
      refar =refFrArr_top;
    }
    else
    {
      all_mv=img->all_mv_bot;
      mv  =img->mv_bot;
      tmpmv =tmp_mv_bot;
      refar =refFrArr_bot;

    }
  }


  if(mb_adaptive)
  {
    if(!img->field_mode)    // if current MB is in frame mode
    {
      if(mb_available_up)
        zeroMotionAbove = field_mb[img->mb_y-1][img->mb_x] ? 1:zeroMotionAbove; // if top MB Pair is frame then motion MV ok, else set it to 0
      
      if(mb_available_left)
        zeroMotionLeft = field_mb[img->mb_y][img->mb_x-1] ? 1:zeroMotionLeft;     // if left MB Pair is frame then motion MV ok, else set it to 1
    }
    else            // else if current MB is in field mode
    {
      
      mb_available_up   = (mb_y == 0)  ? 0 : (img->mb_data[img->current_mb_nr].slice_nr == img->mb_data[img->current_mb_nr-img->width/32].slice_nr);
      mb_available_left = (img->mb_x == 0)  ? 0 : (img->mb_data[img->current_mb_nr].slice_nr == img->mb_data[img->current_mb_nr-1                    ].slice_nr);
      zeroMotionAbove   = !mb_available_up  ? 1 : refar[block_y-1][img->block_x] == 0 && tmpmv[0][block_y-1][4+img->block_x]   == 0 && tmpmv[1][block_y-1][4+img->block_x]   == 0 ? 1 : 0;
      zeroMotionLeft    = !mb_available_left? 1 : refar[block_y][img->block_x-1] == 0 && tmpmv[0][block_y  ][4+img->block_x-1] == 0 && tmpmv[1][block_y  ][4+img->block_x-1] == 0 ? 1 : 0;
      
      if(mb_available_up)
        zeroMotionAbove = field_mb[img->mb_y-1][img->mb_x] ? zeroMotionAbove:1; // if top MB Pair is field then motion MV ok, else set it to 0
      
      if(mb_available_left)
        zeroMotionLeft = field_mb[img->mb_y][img->mb_x-1] ? zeroMotionLeft:1;     // if left MB Pair is field then motion MV ok, else set it to 1
      
    }
  }
  
  if (zeroMotionAbove || zeroMotionLeft)
  {
    for (by = 0;by < 4;by++)
      for (bx = 0;bx < 4;bx++)
      {
        all_mv [bx][by][0][0][0] = 0;
        all_mv [bx][by][0][0][1] = 0;
      }
  }
  else
  {
    for (by = 0;by < 4;by++)
      for (bx = 0;bx < 4;bx++)
      {
        all_mv [bx][by][0][0][0] = mv[0][0][0][1][0];
        all_mv [bx][by][0][0][1] = mv[0][0][0][1][1];
      }
  }
}

/*!
 ************************************************************************
 * \brief
 *    Get cost for direct mode for an 8x8 block
 ************************************************************************
 */
int Get_Direct_Cost8x8 (int block, double lambda, int abt_mode)
{
  int block_y, block_x, pic_pix_y, pic_pix_x, i, j, k;
  int diff[16];
  int cost  = 0;
  int mb_y  = (block/2)<<3;
  int mb_x  = (block%2)<<3;
  int curr_blk[MB_BLOCK_SIZE][MB_BLOCK_SIZE]; // ABT pred.error buffer
  int bxx, byy;                               // indexing curr_blk
  int bsx   = ABT_TRSIZE[abt_mode][PIX];
  int bsy   = ABT_TRSIZE[abt_mode][LIN];
  byte **imgY_original = imgY_org;
  int pix_y = img->pix_y;

  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    imgY_original =  (img->top_field ?  imgY_org_top:imgY_org_bot);
    pix_y         =  img->field_pix_y;
  }

  for (byy=0, block_y=mb_y; block_y<mb_y+8; byy+=4, block_y+=4)
  {
    pic_pix_y = pix_y + block_y;

    for (bxx=0, block_x=mb_x; block_x<mb_x+8; bxx+=4, block_x+=4)
    {
      pic_pix_x = img->pix_x + block_x;

      //===== prediction of 4x4 block =====
      if (img->type==BS_IMG) 
      {
        AbpLumaPrediction4x4 (block_x, block_y, 0, 0, 0, 0, 0);   
      }
      else
      {
        LumaPrediction4x4 (block_x, block_y, 0, 0, max(0,refFrArr[pic_pix_y>>2][pic_pix_x>>2]));
      }

      //===== get displaced frame difference ======                
      for (k=j=0; j<4; j++)
        for (i=0; i<4; i++, k++)
        {
          diff[k] = curr_blk[byy+j][bxx+i] =
                    imgY_original[pic_pix_y+j][pic_pix_x+i] - img->mpr[i+block_x][j+block_y];

        }
      if (abt_mode == ABT_OFF)
        cost += SATD (diff, input->hadamard);
    }
  }

  if (abt_mode != ABT_OFF)
  {
    for (byy=0; byy<8 ; byy+=bsy)
      for (bxx=0; bxx<8 ; bxx+=bsx)
        cost += find_sad_abt(input->hadamard,bsx,bsy,bxx,byy,curr_blk);
  }

  return cost;
}



/*!
 ************************************************************************
 * \brief
 *    Get cost for direct mode for an macroblock
 ************************************************************************
 */
int Get_Direct_CostMB (double lambda)
{
  int i;
  int cost = 0;
  
  for (i=0; i<4; i++)
  {
    cost += Get_Direct_Cost8x8 (i, lambda, (input->abt)? getDirectModeABT(i) : ABT_OFF);
  }
  return cost;
}


/*!
 ************************************************************************
 * \brief
 *    Motion search for a partition
 ************************************************************************
 */
void
PartitionMotionSearch (int    blocktype,
                       int    block8x8,
                       double lambda,
                       int    useABT)
{
  static int  bx0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,2,0,2}};
  static int  by0[5][4] = {{0,0,0,0}, {0,0,0,0}, {0,2,0,0}, {0,0,0,0}, {0,0,2,2}};

  int   **ref_array, ***mv_array, *****all_mv;
  int   ref, refinx, refframe, v, h, mcost, search_range, i, j;
  int   pic_block_x, pic_block_y;
  int   bframe    = (img->type==B_IMG);
  int   max_ref   = bframe? img->nb_references-1 : img->nb_references;
  int   parttype  = (blocktype<4?blocktype:4);
  int   step_h0   = (input->blc_size[ parttype][0]>>2);
  int   step_v0   = (input->blc_size[ parttype][1]>>2);
  int   step_h    = (input->blc_size[blocktype][0]>>2);
  int   step_v    = (input->blc_size[blocktype][1]>>2);
  int   block_y   = img->block_y;
  int   **bref_array, ***bwMV;

  if(mref==mref_fld)   //field coding, 
    max_ref = min (img->number-((mref==mref_fld)&&img->fld_type&&bframe), img->buf_cycle);
  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
      max_ref   = min (2*img->number, img->buf_cycle);
      if(img->top_field)
        block_y = 4*(img->mb_y/2)+(img->block_y-4*img->mb_y);  // seems to be un-necessary, can use img->field_block_y instead 
      else
        block_y = 4*((img->mb_y-1)/2)+(img->block_y-4*img->mb_y);
  }

  //===== LOOP OVER REFERENCE FRAMES =====
  for (ref=(bframe?-1:0); ref<max_ref; ref++)
  {
    refinx    = ref+1;
    refframe  = (ref<0?0:ref);
#ifdef _ADDITIONAL_REFERENCE_FRAME_
    if ((ref<input->no_multpred) || (ref==input->add_ref_frame))
    {
#endif
      //----- set search range ---
#ifdef _FULL_SEARCH_RANGE_
      if      (input->full_search == 2) search_range = input->search_range;
      else if (input->full_search == 1) search_range = input->search_range /  (min(refframe,1)+1);
      else                              search_range = input->search_range / ((min(refframe,1)+1) * min(2,blocktype));
#else
      search_range = input->search_range / ((min(refframe,1)+1) * min(2,blocktype));
#endif

      //----- set arrays -----
      ref_array = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr : ref<0 ? bw_refFrArr : fw_refFrArr);
      mv_array  = ((img->type!=B_IMG && img->type!=BS_IMG) ?   tmp_mv : ref<0 ?    tmp_bwMV :    tmp_fwMV);
      if (img->type==BS_IMG)
      {
        bref_array = bw_refFrArr;
        bwMV = tmp_bwMV;
      }
      all_mv    = (ref<0 ? img->all_bmv : img->all_mv);
      if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
      {
        if(img->top_field)
        {
          ref_array = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr_top : ref<0 ? bw_refFrArr_top : fw_refFrArr_top);
          mv_array  = ((img->type!=B_IMG && img->type!=BS_IMG) ?   tmp_mv_top : ref<0 ?    tmp_bwMV_top :    tmp_fwMV_top);
          if (img->type==BS_IMG)
          {
            bref_array = bw_refFrArr_top;
            bwMV = tmp_bwMV_top;
          }
          all_mv    = (ref<0 ? img->all_bmv_top : img->all_mv_top);
        }
        else
        {
          ref_array = ((img->type!=B_IMG && img->type!=BS_IMG) ? refFrArr_bot : ref<0 ? bw_refFrArr_bot : fw_refFrArr_bot);
          mv_array  = ((img->type!=B_IMG && img->type!=BS_IMG) ?   tmp_mv_bot : ref<0 ?    tmp_bwMV_bot :    tmp_fwMV_bot);
          if (img->type==BS_IMG)
          {
            bref_array = bw_refFrArr_bot;
            bwMV = tmp_bwMV_bot;
          }
          all_mv    = (ref<0 ? img->all_bmv_bot : img->all_mv_bot);
        }
      }

      //----- init motion cost -----
      motion_cost[blocktype][refinx][block8x8] = 0;

      //===== LOOP OVER BLOCKS =====
      for (v=by0[parttype][block8x8]; v<by0[parttype][block8x8]+step_v0; v+=step_v)
      {
        pic_block_y = block_y + v;

        for (h=bx0[parttype][block8x8]; h<bx0[parttype][block8x8]+step_h0; h+=step_h)
        {
          pic_block_x = img->block_x + h;

          //--- motion search for block ---
          mcost = BlockMotionSearch (ref, 4*pic_block_x, 4*pic_block_y, blocktype, search_range, lambda, useABT);
          motion_cost[blocktype][refinx][block8x8] += mcost;

          //--- set motion vectors and reference frame (for motion vector prediction) ---
          for (j=0; j<step_v; j++)
          for (i=0; i<step_h; i++)
          {
            mv_array[0][pic_block_y+j][pic_block_x+i+4] = all_mv[h][v][refframe][blocktype][0];
            mv_array[1][pic_block_y+j][pic_block_x+i+4] = all_mv[h][v][refframe][blocktype][1];
            ref_array  [pic_block_y+j][pic_block_x+i  ] = refframe;
            if (img->type==BS_IMG)
            {
              bwMV    [0][pic_block_y+j][pic_block_x+i+4] = all_mv[h][v][refframe][blocktype][0];
              bwMV    [1][pic_block_y+j][pic_block_x+i+4] = all_mv[h][v][refframe][blocktype][1];
              bref_array [pic_block_y+j][pic_block_x+i  ] = refframe;
            }
          }
        }
      }
#ifdef _ADDITIONAL_REFERENCE_FRAME_
    }
    else
    {
      motion_cost[blocktype][refinx][block8x8] = (1<<20);
    }
#endif
  }
}





extern int* last_P_no;
/*********************************************
 *****                                   *****
 *****  Calculate Direct Motion Vectors  *****
 *****                                   *****
 *********************************************/
void
Get_Direct_Motion_Vectors ()
{
  int  block_x, block_y, pic_block_x, pic_block_y;
  int  refframe, refP_tr, TRb, TRp, TRd;
  int  frame_no_next_P, frame_no_B, delta_P;
  int  pix_y    = img->pix_y;
  int  **refarr = refFrArr;
  int  ***tmpmvs = tmp_mv;
  int  *****all_mvs = img->all_mv;
  int  *****all_bmvs= img->all_bmv;
  int  *****abp_all_dmv= img->abp_all_dmv;
  int  prev_mb_is_field = 0;  // if the collocated MB is field/frame  
  int  mv_scale;

  int  **moving_block_dir = moving_block; 
  int  **fwrefarr = fw_refFrArr;
  int  **bwrefarr = bw_refFrArr;   
  int  ***tmpmvfw = tmp_fwMV;
  int  ***tmpmvbw = tmp_bwMV;

  int MB_AFF_mode= (input->InterlaceCodingOption >= MB_CODING && mb_adaptive );  
  
  if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
  {
    pix_y    = img->field_pix_y;
    tmpmvs   = img->top_field ? tmp_mv_top : tmp_mv_bot;
    all_mvs  = img->top_field ? img->all_mv_top : img->all_mv_bot;
    all_bmvs = img->top_field ? img->all_bmv_top : img->all_bmv_bot;
    abp_all_dmv= img->top_field ? img->abp_all_dmv_top : img->abp_all_dmv_bot;
    prev_mb_is_field = field_mb[img->mb_y][img->mb_x];
    refarr   = (img->top_field ? refFrArr_top:refFrArr_bot);

    if (input->direct_type)
    {
      fwrefarr   = (img->top_field ? fw_refFrArr_top:fw_refFrArr_bot);
      bwrefarr   = (img->top_field ? bw_refFrArr_top:bw_refFrArr_bot);
      moving_block_dir=img->top_field ? moving_block_top : moving_block_bot;
      tmpmvfw   = img->top_field ? tmp_fwMV_top : tmp_fwMV_bot;
      tmpmvbw   = img->top_field ? tmp_bwMV_top : tmp_bwMV_bot;
    }

  }
  
  if (input->direct_type)
//   if(input->direct_type && !(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode))
  {
    
    int pic_blockx            = img->block_x;
    int pic_blocky            = pix_y>>2; 
    int mb_nr                 = img->current_mb_nr;
    int mb_width              = img->width/16;
    int mb_available_up       = (img->mb_y == 0 || pic_blocky == 0 ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width  ].slice_nr);
    int mb_available_left     = (img->mb_x == 0          ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-1         ].slice_nr);
    int mb_available_upleft  = (img->mb_x == 0 ||
      img->mb_y == 0  || pic_blocky == 0  ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width-1].slice_nr);
    int mb_available_upright = (MB_AFF_mode && img->mb_y%2)? 0 : (img->mb_x >= mb_width-1 ||
      img->mb_y == 0  || pic_blocky == 0  ) ? 0 : (img->mb_data[mb_nr].slice_nr == img->mb_data[mb_nr-mb_width+1].slice_nr);

    int fw_rFrameL              = mb_available_left    ? fwrefarr[pic_blocky]  [pic_blockx-1]   : -1;
    int fw_rFrameU              = mb_available_up      ? fwrefarr[pic_blocky-1][pic_blockx]     : -1;
    int fw_rFrameUL             = mb_available_upleft  ? fwrefarr[pic_blocky-1][pic_blockx-1]   : -1;
    int fw_rFrameUR             = mb_available_upright ? fwrefarr[pic_blocky-1][pic_blockx+4]   : fw_rFrameUL; 

    int bw_rFrameL              = mb_available_left    ? bwrefarr[pic_blocky]  [pic_blockx-1]   : -1;
    int bw_rFrameU              = mb_available_up      ? bwrefarr[pic_blocky-1][pic_blockx]     : -1;
    int bw_rFrameUL             = mb_available_upleft  ? bwrefarr[pic_blocky-1][pic_blockx-1]   : -1;
    int bw_rFrameUR             = mb_available_upright ? bwrefarr[pic_blocky-1][pic_blockx+4]   : bw_rFrameUL; 
    
    int fw_rFrame,bw_rFrame;
    int pmvfw[2]={0,0},pmvbw[2]={0,0};
    if (!fw_rFrameL || !fw_rFrameU || !fw_rFrameUR)
      fw_rFrame=0;
    else
      fw_rFrame=min(fw_rFrameL&15,min(fw_rFrameU&15,fw_rFrameUR&15));

    if (img->type==BS_IMG)
    {
      if (bw_rFrameL==1 || bw_rFrameU==1 || bw_rFrameUR==1)
        bw_rFrame=1;
      else
        bw_rFrame=min(bw_rFrameL&15,min(bw_rFrameU&15,bw_rFrameUR&15));
    }
    else
    {
      if (!bw_rFrameL || !bw_rFrameU || !bw_rFrameUR)
        bw_rFrame=0;
      else
        bw_rFrame=min(bw_rFrameL&15,min(bw_rFrameU&15,bw_rFrameUR&15));
    }

    if (fw_rFrame !=15)
      SetMotionVectorPredictor(pmvfw, fwrefarr, tmpmvfw, fw_rFrame, 0, 0, 16, 16);
    if (bw_rFrame !=15)
      SetMotionVectorPredictor(pmvbw, bwrefarr, tmpmvbw, bw_rFrame, 0, 0, 16, 16);
    
    for (block_y=0; block_y<4; block_y++)
    {
      pic_block_y = (pix_y>>2) + block_y;
      
      for (block_x=0; block_x<4; block_x++)
      {
        pic_block_x = (img->pix_x>>2) + block_x;
        if (fw_rFrame !=15)
        {                   
          if  (fw_rFrame == 0 && !moving_block_dir[pic_block_y][pic_block_x])       
          {
            all_mvs [block_x][block_y][0][0][0] = 0;
            all_mvs [block_x][block_y][0][0][1] = 0;            
            fwdir_refFrArr[pic_block_y][pic_block_x]=0;            
          }
          else 
          {
            all_mvs [block_x][block_y][fw_rFrame][0][0] = pmvfw[0];
            all_mvs [block_x][block_y][fw_rFrame][0][1] = pmvfw[1];
            fwdir_refFrArr[pic_block_y][pic_block_x]=fw_rFrame;              
          }
        }
        else
        {          
          fwdir_refFrArr[pic_block_y][pic_block_x]=-1;          
          all_mvs [block_x][block_y][0][0][0] = 0;
          all_mvs [block_x][block_y][0][0][1] = 0;
        }
        if (bw_rFrame !=15)
        {
//          if (refframe==0 && (abs( tmpmvs[0][pic_block_y][pic_block_x+4])>>1) == 0 && (abs(tmpmvs[1][pic_block_y][pic_block_x+4])>>1) == 0)
          if (img->type==BS_IMG)
          {
            if  (bw_rFrame == 1 && !moving_block_dir[pic_block_y][pic_block_x]) 
            {
              all_bmvs [block_x][block_y][1][0][0] = 0;
              all_bmvs [block_x][block_y][1][0][1] = 0;
              bwdir_refFrArr[pic_block_y][pic_block_x]=1;            
            }
            else 
            {
              bwdir_refFrArr[pic_block_y][pic_block_x]=bw_rFrame;
              all_bmvs[block_x][block_y][bw_rFrame][0][0] = pmvbw[0];
              all_bmvs[block_x][block_y][bw_rFrame][0][1] = pmvbw[1];
            }            
          }
          else
          {
            if  (!moving_block_dir[pic_block_y][pic_block_x])       
            {
              all_bmvs [block_x][block_y][0][0][0] = 0;
              all_bmvs [block_x][block_y][0][0][1] = 0;
              bwdir_refFrArr[pic_block_y][pic_block_x]=0;            
            }
            else 
            {
              all_bmvs[block_x][block_y][0][0][0] = pmvbw[0];
              all_bmvs[block_x][block_y][0][0][1] = pmvbw[1];
              bwdir_refFrArr[pic_block_y][pic_block_x]=bw_rFrame;
            }            
          }
        }
        else
        {
          bwdir_refFrArr[pic_block_y][pic_block_x]=-1;
          if (img->type==BS_IMG)
          {
            all_bmvs [block_x][block_y][1][0][0] = 0;
            all_bmvs [block_x][block_y][1][0][1] = 0;
          }
          else
          {
            all_bmvs [block_x][block_y][0][0][0] = 0;
            all_bmvs [block_x][block_y][0][0][1] = 0;
          }
        }
        if (fw_rFrame ==15 && bw_rFrame ==15)
        {
          if (img->type==BS_IMG)
          {
            fwdir_refFrArr[pic_block_y][pic_block_x] = 0;
            bwdir_refFrArr[pic_block_y][pic_block_x] = 1;
          }
          else
          {
            fwdir_refFrArr[pic_block_y][pic_block_x] = 
              bwdir_refFrArr[pic_block_y][pic_block_x] = 0;
          }
        }
      }      
    }
  }
  else
  {
    for (block_y=0; block_y<4; block_y++)
    {
      pic_block_y = (pix_y>>2) + block_y;
      
      for (block_x=0; block_x<4; block_x++)
      {
        pic_block_x = (img->pix_x>>2) + block_x;


        // next P is intra mode
        if((refframe=refarr[pic_block_y][pic_block_x]) == -1)
        {
          all_mvs [block_x][block_y][0][0][0] = 0;
          all_mvs[block_x][block_y][0][0][1] = 0;
          all_bmvs[block_x][block_y][0][0][0] = 0;
          all_bmvs[block_x][block_y][0][0][1] = 0;
        }
        /*    else if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode && !prev_mb_is_field)
        {
        all_mvs [block_x][block_y][0][0][0] = 0;
        all_mvs [block_x][block_y][0][0][1] = 0;
        all_bmvs[block_x][block_y][0][0][0] = 0;
        all_bmvs[block_x][block_y][0][0][1] = 0;
        }
        else if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && !img->field_mode && prev_mb_is_field)
        {
        all_mvs [block_x][block_y][0][0][0] = 0;
        all_mvs [block_x][block_y][0][0][1] = 0;
        all_bmvs[block_x][block_y][0][0][0] = 0;
        all_bmvs[block_x][block_y][0][0][1] = 0;      
      } */
        // next P is skip or inter mode
        else 
        {

#ifdef _ADAPT_LAST_GROUP_
          refP_tr = last_P_no [refframe];
#else
          refP_tr = nextP_tr - ((refframe+1)*img->p_interval);
#endif
          
          frame_no_next_P = (mref==mref_fld) ? img->imgtr_next_P_fld+img->fld_type : 2*img->imgtr_next_P_frm;
          frame_no_B = (mref==mref_fld) ? img->tr : 2*img->tr;
          delta_P = (mref==mref_fld)?(img->imgtr_next_P_fld - img->imgtr_last_P_fld):2*(img->imgtr_next_P_frm - img->imgtr_last_P_frm);
          if((mref==mref_fld) && !img->fld_type)  // top field
          {
            TRp = delta_P*(refframe/2+1)-(refframe+1)%2;
          }
          else if((mref==mref_fld) && img->fld_type)  // bot field
          {
            TRp = 1+delta_P*(refframe+1)/2-refframe%2;
          }
          else  // frame
          {
            TRp  = (refframe+1)*delta_P;
            if(input->InterlaceCodingOption >= MB_CODING && mb_adaptive && img->field_mode)
            {
              delta_P = 2*(img->imgtr_next_P_frm - img->imgtr_last_P_frm);
              if(img->top_field)
                TRp = delta_P*(refframe/2+1)-(refframe+1)%2;
              else
                TRp = 1+delta_P*(refframe+1)/2-refframe%2;
            }
            
          }
          
          TRb = TRp - (frame_no_next_P - frame_no_B);
          
          TRd    = TRb      - TRp;
          mv_scale = (TRb * 256) / TRp;       //! Note that this could be precomputed at the frame/slice level. 
                                              //! In such case, an array is needed for each different reference.
          // forward
          all_mvs [block_x][block_y][refframe][0][0] = (mv_scale * tmpmvs[0][pic_block_y][pic_block_x+4] + 128) >> 8;
          all_mvs [block_x][block_y][refframe][0][1] = (mv_scale * tmpmvs[1][pic_block_y][pic_block_x+4] + 128) >> 8;
          // backward
          all_bmvs[block_x][block_y][       0][0][0] = ((mv_scale - 256)* tmpmvs[0][pic_block_y][pic_block_x+4] + 128) >> 8;
          all_bmvs[block_x][block_y][       0][0][1] = ((mv_scale - 256)* tmpmvs[1][pic_block_y][pic_block_x+4] + 128) >> 8;
          
          /*
          all_mvs [block_x][block_y][refframe][0][0] = TRb * tmpmvs[0][pic_block_y][pic_block_x+4] / TRp;
          all_mvs [block_x][block_y][refframe][0][1] = TRb * tmpmvs[1][pic_block_y][pic_block_x+4] / TRp;
          // backward
          all_bmvs[block_x][block_y][       0][0][0] = TRd * tmpmvs[0][pic_block_y][pic_block_x+4] / TRp;
          all_bmvs[block_x][block_y][       0][0][1] = TRd * tmpmvs[1][pic_block_y][pic_block_x+4] / TRp;
          */
        }
      }
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    control the sign of a with b
 ************************************************************************
 */
int sign(int a,int b)
{
  int x;
  x=absm(a);
  if (b >= 0)
    return x;
  else
    return -x;
}

