/*!
 *************************************************************************************
 * \file rdoq.c
 *
 * \brief
 *    Rate Distortion Optimized Quantization based on VCEG-AH21
 *************************************************************************************
 */

#include "contributors.h"

#include <math.h>
#include <float.h>

#include "global.h"
#include "image.h"
#include "fmo.h"
#include "macroblock.h"
#include "mb_access.h"
#include "rdopt.h"
#include "rdoq.h"
#include "mv_search.h"

#define RDOQ_BASE 0


/*!
****************************************************************************
* \brief
*    Initialize the parameters related to RDO_Q in slice level
****************************************************************************
*/
void init_rdoq_slice(Slice *currSlice)
{
  //norm_factor_4x4 = (double) ((int64) 1 << (2 * DQ_BITS + 19)); // norm factor 4x4 is basically (1<<31)
  //norm_factor_8x8 = (double) ((int64) 1 << (2 * Q_BITS_8 + 9)); // norm factor 8x8 is basically (1<<41)
  currSlice->norm_factor_4x4 = pow(2, (2 * DQ_BITS + 19));
  currSlice->norm_factor_8x8 = pow(2, (2 * Q_BITS_8 + 9));
}
/*!
****************************************************************************
* \brief
*    Initialize levelData for Chroma DC
****************************************************************************
*/
int init_trellis_data_DC_cr_CAVLC(Macroblock *currMB, int **tblock, int qp_per, int qp_rem, 
                         LevelQuantParams *q_params_4x4, const byte *p_scan, 
                         levelDataStruct *dataLevel)
{
  Slice *currSlice = currMB->p_slice;
  ImageParameters *p_Img = currMB->p_Img;
  int i, j, coeff_ctr, end_coeff_ctr = p_Img->num_cdc_coeff;
  int *m7;
  int q_bits = Q_BITS + qp_per + 1; 
  int q_offset = ( 1 << (q_bits - 1) );
  double err; 
  int level, lowerInt, k;
  double estErr = (double) estErr4x4[qp_rem][0][0] / currSlice->norm_factor_4x4; // note that we could also use int64

  for (coeff_ctr = 0; coeff_ctr < end_coeff_ctr; coeff_ctr++)
  {
    j = *p_scan++;  // horizontal position
    i = *p_scan++;  // vertical position

    m7 = &tblock[j][i];
    if (*m7 == 0)
    {
      dataLevel->levelDouble = 0;
      dataLevel->level[0] = 0;
      dataLevel->noLevels = 1;
      err = 0.0;
      dataLevel->errLevel[0] = 0.0;
      dataLevel->pre_level = 0;
      dataLevel->sign = 0;
    }
    else
    {
      dataLevel->levelDouble = iabs(*m7 * q_params_4x4->ScaleComp);
      level = (dataLevel->levelDouble >> q_bits);

      lowerInt=( ((int)dataLevel->levelDouble - (level << q_bits)) < q_offset )? 1 : 0;
      dataLevel->level[0] = 0;
      if (level == 0 && lowerInt == 1)
      {
        dataLevel->noLevels = 1;
      }
      else if (level == 0 && lowerInt == 0)
      {
        dataLevel->level[1] = level + 1;
        dataLevel->noLevels = 2;
      }
      else if (level > 0 && lowerInt == 1)
      {
        dataLevel->level[1] = level;
        dataLevel->noLevels = 2;
      }
      else
      {
        dataLevel->level[1] = level;
        dataLevel->level[2] = level + 1;
        dataLevel->noLevels = 3;
      }

      for (k = 0; k < dataLevel->noLevels; k++)
      {
        err = (double)(dataLevel->level[k] << q_bits) - (double)dataLevel->levelDouble;
        dataLevel->errLevel[k] = (err * err * estErr); 
      }

      if(dataLevel->noLevels == 1)
        dataLevel->pre_level = 0;
      else
        dataLevel->pre_level = (iabs (*m7) * q_params_4x4->ScaleComp + q_params_4x4->OffsetComp) >> q_bits;
      dataLevel->sign = isign(*m7);
    }
    dataLevel++;
  }
  return 0;
}


/*!
****************************************************************************
* \brief
*    Initialize levelData for Chroma DC
****************************************************************************
*/
int init_trellis_data_DC_cr_CABAC(Macroblock *currMB, int **tblock, int qp_per, int qp_rem, 
                         LevelQuantParams *q_params_4x4, const byte *p_scan, 
                         levelDataStruct *dataLevel, int* kStart, int* kStop)
{
  Slice *currSlice = currMB->p_slice;
  ImageParameters *p_Img = currMB->p_Img;
  int noCoeff = 0;
  int i, j, coeff_ctr, end_coeff_ctr = p_Img->num_cdc_coeff;
  int *m7;
  int q_bits = Q_BITS + qp_per + 1; 
  int q_offset = ( 1 << (q_bits - 1) );
  double err; 
  int level, lowerInt, k;
  double estErr = (double) estErr4x4[qp_rem][0][0] / currSlice->norm_factor_4x4; // note that we could also use int64

  for (coeff_ctr = 0; coeff_ctr < end_coeff_ctr; coeff_ctr++)
  {
    j = *p_scan++;  // horizontal position
    i = *p_scan++;  // vertical position

    m7 = &tblock[j][i];
    if (*m7 == 0)
    {
      dataLevel->levelDouble = 0;
      dataLevel->level[0] = 0;
      dataLevel->noLevels = 1;
      err = 0.0;
      dataLevel->errLevel[0] = 0.0;
    }
    else
    {
      dataLevel->levelDouble = iabs(*m7 * q_params_4x4->ScaleComp);
      level = (dataLevel->levelDouble >> q_bits);

      lowerInt=( ((int)dataLevel->levelDouble - (level << q_bits)) < q_offset )? 1 : 0;
      dataLevel->level[0] = 0;
      if (level == 0 && lowerInt == 1)
      {
        dataLevel->noLevels = 1;
      }
      else if (level == 0 && lowerInt == 0)
      {
        dataLevel->level[1] = level + 1;
        dataLevel->noLevels = 2;
        *kStop = coeff_ctr;
        noCoeff++;
      }
      else if (level > 0 && lowerInt == 1)
      {
        dataLevel->level[1] = level;
        dataLevel->noLevels = 2;
        *kStop = coeff_ctr;
        noCoeff++;
      }
      else
      {
        dataLevel->level[1] = level;
        dataLevel->level[2] = level + 1;
        dataLevel->noLevels = 3;
        *kStop  = coeff_ctr;
        *kStart = coeff_ctr;
        noCoeff++;
      }

      for (k = 0; k < dataLevel->noLevels; k++)
      {
        err = (double)(dataLevel->level[k] << q_bits) - (double)dataLevel->levelDouble;
        dataLevel->errLevel[k] = (err * err * estErr); 
      }
    }
    dataLevel++;
  }
  return (noCoeff);
}

void get_dQP_table(Slice *currSlice)
{
  int   qp_offset = (currSlice->slice_type == B_SLICE) ? (currSlice->RDOQ_QP_Num / 3): (currSlice->RDOQ_QP_Num >> 1);
  int   deltaQPCnt, deltaQP;

  memset(currSlice->deltaQPTable, 0, sizeof(int)*9);

  for(deltaQPCnt = 0; deltaQPCnt < currSlice->RDOQ_QP_Num; deltaQPCnt++)
  {
    if (deltaQPCnt == 0)
      deltaQP = 0;
    else
    {
      if (deltaQPCnt <= qp_offset)
        deltaQP = deltaQPCnt - 1 - qp_offset;
      else
        deltaQP = deltaQPCnt - qp_offset;
    }
    currSlice->deltaQPTable[deltaQPCnt] = deltaQP; 
  }
}

void trellis_mp(Macroblock *currMB, Boolean prev_recode_mb)
{
  Slice *currSlice = currMB->p_slice;
  ImageParameters *p_Img = currMB->p_Img;
  InputParameters *p_Inp = currMB->p_Inp;

  int masterQP = 0, deltaQP;
  int qp_left, qp_up;
#if RDOQ_BASE
  const int deltaQPTabB[] = {0,  1, -1,  2, 3, -2, 4,  5, -3};
  const int deltaQPTabP[] = {0, -1,  1, -2, 2, -3, 3, -4,  4};
#endif
  int   deltaQPCnt; 
  int   qp_anchor;

  masterQP = p_Img->masterQP = p_Img->qp;
  p_Img->Motion_Selected = 0;
  currSlice->rddata_trellis_best.min_rdcost = 1e30;

  if (p_Inp->symbol_mode == CABAC)
  {
    estRunLevel_CABAC(currMB, LUMA_4x4); 
    estRunLevel_CABAC(currMB, LUMA_16AC);
    estRunLevel_CABAC(currMB, LUMA_16DC);
    if (p_Inp->Transform8x8Mode)
      estRunLevel_CABAC(currMB, LUMA_8x8);
    if (p_Img->yuv_format != YUV400)
    {
      estRunLevel_CABAC(currMB, CHROMA_AC);
      if (p_Img->yuv_format == YUV420)
        estRunLevel_CABAC(currMB, CHROMA_DC);
      else
        estRunLevel_CABAC(currMB, CHROMA_DC_2x4);
    }
  }

  qp_left   = (currMB->mb_left) ? currMB->mb_left->qp : p_Img->masterQP;
  qp_up     = (currMB->mb_up)   ? currMB->mb_up->qp   : p_Img->masterQP;
  qp_anchor = (qp_left + qp_up + 1)>>1;

  for (deltaQPCnt=0; deltaQPCnt < currSlice->RDOQ_QP_Num; deltaQPCnt++)
  {
    currSlice->rddata = &currSlice->rddata_trellis_curr;
    currSlice->rddata->min_dcost = 1e30;
#if RDOQ_BASE
    if (currSlice->slice_type == B_SLICE)
      deltaQP = deltaQPTabB[deltaQPCnt];      
    else
      deltaQP = deltaQPTabP[deltaQPCnt];
#else

    // It seems that pushing the masterQP as first helps things when fast me is enabled. 
    // Could there be an issue with motion estimation?
    deltaQP = currSlice->deltaQPTable[deltaQPCnt]; 
#endif

    p_Img->qp = iClip3(-p_Img->bitdepth_luma_qp_scale, 51, masterQP + deltaQP);
    deltaQP = p_Img->qp - masterQP; 


#if RDOQ_BASE
    if(deltaQP != 0 && !(p_Img->qp - qp_anchor >= -2 && p_Img->qp - qp_anchor <= 1) && currMB->mb_left && currMB->mb_up && currSlice->slice_type == P_SLICE)
      continue; 
    if(deltaQP != 0 && !(p_Img->qp - qp_anchor >= -1 && p_Img->qp - qp_anchor <= 2) && currMB->mb_left && currMB->mb_up && currSlice->slice_type == B_SLICE)
      continue;
#endif

    reset_macroblock(currMB);
    currMB->qp       = (short) p_Img->qp;
    update_qp (currMB);

    currSlice->encode_one_macroblock (currMB);
    end_encode_one_macroblock(currMB);


    if ( currSlice->rddata_trellis_curr.min_rdcost < currSlice->rddata_trellis_best.min_rdcost)
      copy_rddata_trellis(currMB, &currSlice->rddata_trellis_best, currSlice->rddata);

    if (p_Inp->RDOQ_CP_MV)
      p_Img->Motion_Selected = 1;

#if (!RDOQ_BASE)
    if (p_Inp->RDOQ_Fast == 1)
    {
      if ((p_Img->qp - currSlice->rddata_trellis_best.qp > 1))
        break;
      if ((currSlice->rddata_trellis_curr.cbp == 0) && (currSlice->rddata_trellis_curr.mb_type != 0))
        break;
      if ((currSlice->rddata_trellis_best.mb_type == 0) && (currSlice->rddata_trellis_best.cbp == 0))
        break;
    }
#endif
  }

  reset_macroblock(currMB);
  currSlice->rddata = &currSlice->rddata_trellis_best;

  copy_rdopt_data  (currMB);  // copy the MB data for Top MB from the temp buffers
  write_macroblock (currMB, 1, prev_recode_mb);
  p_Img->qp = masterQP;
}

void trellis_sp(Macroblock *currMB, Boolean prev_recode_mb)
{
  ImageParameters *p_Img     = currMB->p_Img;
  InputParameters *p_Inp     = currMB->p_Inp;
  Slice           *currSlice = currMB->p_slice;

  p_Img->masterQP = p_Img->qp;

  if (currSlice->symbol_mode == CABAC)
  {
    estRunLevel_CABAC(currMB, LUMA_4x4); 
    estRunLevel_CABAC(currMB, LUMA_16AC);

    estRunLevel_CABAC(currMB, LUMA_16DC);
    if (p_Inp->Transform8x8Mode)
      estRunLevel_CABAC(currMB, LUMA_8x8);

    if (p_Img->yuv_format != YUV400)
    {
      estRunLevel_CABAC(currMB, CHROMA_AC);

      if (p_Img->yuv_format == YUV420)
        estRunLevel_CABAC(currMB, CHROMA_DC);
      else
        estRunLevel_CABAC(currMB, CHROMA_DC_2x4);
    }
  }

  currSlice->encode_one_macroblock (currMB);
  end_encode_one_macroblock(currMB);



  write_macroblock (currMB, 1, prev_recode_mb);    
}

void trellis_coding(Macroblock *currMB, Boolean prev_recode_mb)
{
  if (currMB->p_slice->RDOQ_QP_Num > 1)
  {
    trellis_mp(currMB, prev_recode_mb);   
  }
  else
  {
    trellis_sp(currMB, prev_recode_mb);   
  }
}

void RDOQ_update_mode(Slice *currSlice, RD_PARAMS *enc_mb)
{
  InputParameters *p_Inp = currSlice->p_Inp;

  int bslice = (currSlice->slice_type == B_SLICE);
  int mb_type = currSlice->rddata_trellis_best.mb_type;
  int i;
  for(i=0; i<MAXMODE; i++)
    enc_mb->valid[i] = 0;

    enc_mb->valid[mb_type] = 1;  

    if(mb_type  == P8x8)
    {            
      enc_mb->valid[4] = (short) (p_Inp->InterSearch[bslice][4]);
      enc_mb->valid[5] = (short) (p_Inp->InterSearch[bslice][5] && !(p_Inp->Transform8x8Mode==2));
      enc_mb->valid[6] = (short) (p_Inp->InterSearch[bslice][6] && !(p_Inp->Transform8x8Mode==2));
      enc_mb->valid[7] = (short) (p_Inp->InterSearch[bslice][7] && !(p_Inp->Transform8x8Mode==2));
    }
}

void copy_rddata_trellis (Macroblock *currMB, RD_DATA *dest, RD_DATA *src)
{
  ImageParameters *p_Img = currMB->p_Img;
  int j; 

  dest->min_rdcost = src->min_rdcost;
  dest->min_dcost  = src->min_dcost;
  dest->min_rate   = src->min_rate;

  memcpy(&dest->rec_mb[0][0][0],&src->rec_mb[0][0][0], MB_PIXELS * sizeof(imgpel));

  if (p_Img->yuv_format != YUV400) 
  {
    memcpy(&dest->rec_mb[1][0][0],&src->rec_mb[1][0][0], 2 * MB_PIXELS * sizeof(imgpel));
  }

  memcpy(&dest->cofAC[0][0][0][0], &src->cofAC[0][0][0][0], (4 + p_Img->num_blk8x8_uv) * 4 * 2 * 65 * sizeof(int));
  memcpy(&dest->cofDC[0][0][0], &src->cofDC[0][0][0], 3 * 2 * 18 * sizeof(int));

  dest->mb_type = src->mb_type;

  memcpy(dest->b8x8, src->b8x8, BLOCK_MULTIPLE * sizeof(Info8x8));

  dest->cbp  = src->cbp;
  dest->mode = src->mode;
  dest->i16offset = src->i16offset;
  dest->c_ipred_mode = src->c_ipred_mode;
  dest->luma_transform_size_8x8_flag = src->luma_transform_size_8x8_flag;
  dest->NoMbPartLessThan8x8Flag = src->NoMbPartLessThan8x8Flag;
  dest->qp = src->qp;

  dest->prev_qp  = src->prev_qp;
  dest->prev_dqp = src->prev_dqp;
  dest->prev_cbp = src->prev_cbp;
  dest->cbp_blk  = src->cbp_blk;
 

  if (p_Img->type != I_SLICE)
  {
    // note that this is not copying the bipred mvs!!!
    memcpy(&dest->all_mv [0][0][0][0][0][0], &src->all_mv [0][0][0][0][0][0], p_Img->max_num_references * 576 * sizeof(short)); // 2 * ref * 9 * 4 * 4 * 2
  }

  memcpy(dest->intra_pred_modes,src->intra_pred_modes, MB_BLOCK_PARTITIONS * sizeof(char));
  memcpy(dest->intra_pred_modes8x8,src->intra_pred_modes8x8, MB_BLOCK_PARTITIONS * sizeof(char));
  for(j = currMB->block_y; j < currMB->block_y + BLOCK_MULTIPLE; j++)
    memcpy(&dest->ipredmode[j][currMB->block_x],&src->ipredmode[j][currMB->block_x], BLOCK_MULTIPLE * sizeof(char));

  memcpy(&dest->refar[LIST_0][0][0], &src->refar[LIST_0][0][0], 2 * BLOCK_MULTIPLE * BLOCK_MULTIPLE * sizeof(char));

}                     

void updateMV_mp(Macroblock *currMB, int *m_cost, short ref, int list, int h, int v, int blocktype, int block8x8)
{
  InputParameters *p_Inp = currMB->p_Inp;
  Slice       *currSlice = currMB->p_slice;

  int       i, j;
  int       bsx       = block_size[blocktype][0] >> 2;
  int       bsy       = block_size[blocktype][1] >> 2;
  MotionVector  all_mv;

  if ( (p_Inp->Transform8x8Mode == 1) && (blocktype == 4) && currMB->luma_transform_size_8x8_flag)
  {
    all_mv  = currSlice->tmp_mv8[list][ref][v][h];
    *m_cost = currSlice->motion_cost8[list][ref][block8x8];
  }
  else
  {
    all_mv.mv_x = currSlice->rddata_trellis_best.all_mv[list][ref][blocktype][v][h][0];
    all_mv.mv_y = currSlice->rddata_trellis_best.all_mv[list][ref][blocktype][v][h][1];
  }

  for (j = 0; j < bsy; j++)
  {
    for (i = 0; i < bsx; i++) 
    {
      currSlice->all_mv[list][ref][blocktype][v+j][h+i][0] = all_mv.mv_x;
      currSlice->all_mv[list][ref][blocktype][v+j][h+i][1] = all_mv.mv_y;
    }
  }
}



