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
 ************************************************************************
 *  \file
 *     parset.c
 *  \brief
 *     Parameter Sets
 *  \author
 *     Main contributors (see contributors.h for copyright, address and affiliation details)
 *     - Stephan Wenger          <stewe@cs.tu-berlin.de>
 *
 ***********************************************************************
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "parsetcommon.h"
#include "parset.h"
#include "nalu.h"
#include "global.h"
#include "memalloc.h"
#include "fmo.h"
#include "vlc.h"

#if TRACE
#define SYMTRACESTRING(s) strncpy(sym->tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // do nothing
#endif

int UsedBits;      // for internal statistics, is adjusted by se_v, ue_v, u_1

static seq_parameter_set_rbsp_t SeqParSet[MAXSPS];
static pic_parameter_set_rbsp_t PicParSet[MAXPPS];
                          
// fill sps with contrent of p

int InterpretSPS (DataPartition *p, seq_parameter_set_rbsp_t *sps)
{
  unsigned i;

  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (sps != NULL);

  UsedBits = 0;

  sps->profile_idc                            = u_v  (8, "SPS: profile_idc"                           , p);
  sps->level_idc                              = u_v  (8, "SPS: level_idc"                             , p);
  sps->more_than_one_slice_group_allowed_flag = u_1  ("SPS: more_than_one_slice_group_allowed_flag"   , p);
  sps->arbitrary_slice_order_allowed_flag     = u_1  ("SPS: arbitrary_slice_order_allowed_flag"       , p);
  sps->redundant_slices_allowed_flag          = u_1  ("SPS: redundant_slices_allowed_flag"            , p);
  sps->seq_parameter_set_id                   = ue_v ("SPS: seq_parameter_set_id"                     , p);
  sps->log2_max_frame_num_minus4              = ue_v ("SPS: log2_max_frame_num_minus4"                , p);
  sps->pic_order_cnt_type                     = ue_v ("SPS: pic_order_count_type"                     , p);
  if (sps->pic_order_cnt_type == 1)
    sps->num_ref_frames_in_pic_order_cnt_cycle = ue_v ("SPS: num_ref_frames_in_pic_order_cnt_cycle", p);
  if (sps->pic_order_cnt_type < 2)
  {
    sps->delta_pic_order_always_zero_flag      = u_1  ("SPS: delta_pic_order_always_zero_flag"       , p);
    sps->offset_for_non_ref_pic                = se_v ("SPS: offset_for_non_ref_pic"                 , p);
    sps->offset_for_top_to_bottom_field        = se_v ("SPS: offset_for_top_to_bottom_field"         , p);
  }
  if (sps->pic_order_cnt_type == 0)
    sps->offset_for_ref_frame[0]               = se_v ("SPS: offset_for_ref_frame[0]"                , p);
  else if (sps->pic_order_cnt_type == 1)
  {
    for(i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      sps->offset_for_ref_frame[i]               = se_v ("SPS: offset_for_ref_frame[i]"                 , p);
  }
  sps->num_ref_frames                        = ue_v ("SPS: num_ref_frames"                         , p);
  sps->required_frame_num_update_behaviour_flag = u_1 ("SPS: required_frame_num_update_behaviour_flag", p);
  sps->frame_width_in_mbs_minus1             = ue_v ("SPS: frame_width_in_mbs_minus1"              , p);
  sps->frame_height_in_mbs_minus1            = ue_v ("SPS: frame_height_in_mbs_minus1"             , p);
  sps->frame_mbs_only_flag                   = u_1  ("SPS: frame_mbs_only_flag"                    , p);
  if (!sps->frame_mbs_only_flag)
  {
    sps->mb_adaptive_frame_field_flag          = u_1  ("SPS: mb_adaptive_frame_field_flag"           , p);
  }
  sps->direct_8x8_inference_flag             = u_1  ("SPS: direct_8x8_inference_flag"       , p);
  sps->vui_parameters_present_flag           = u_1  ("SPS: vui_parameters_present_flag"             , p);
  if (sps->vui_parameters_present_flag)
  {
    printf ("VUI sequence parameters present but not supported, ignored, proceeding to next NALU\n");
  }
  sps->Valid = TRUE;
  return UsedBits;
}


int InterpretPPS (DataPartition *p, pic_parameter_set_rbsp_t *pps)
{
  unsigned i;
  int NumberBitsPerSliceGroupId;
  
  assert (p != NULL);
  assert (p->bitstream != NULL);
  assert (p->bitstream->streamBuffer != 0);
  assert (pps != NULL);

  UsedBits = 0;

  pps->pic_parameter_set_id                  = ue_v ("PPS: pic_parameter_set_id"                   , p);
  pps->seq_parameter_set_id                  = ue_v ("PPS: seq_parameter_set_id"                   , p);
  pps->entropy_coding_mode                   = u_1  ("PPS: entropy_coding_mode"                    , p);

  //! Note: as per JVT-F078 the following bit is unconditional.  If F078 is not accepted, then
  //! one has to fetch the correct SPS to check whether the bit is present (hopefully there is
  //! no consistency problem :-(
  //! The current encoder code handles this in the same way.  When you change this, don't forget
  //! the encoder!  StW, 12/8/02
  pps->pic_order_present_flag                = u_1  ("PPS: pic_order_present_flag"                 , p);

  pps->num_slice_groups_minus1               = ue_v ("PPS: num_slice_groups_minus1"                , p);

  //! InterpretSPS is able to process all different FMO modes, but UseParameterSet() is not (yet
  //! Hence, make sure FMO is not used.

  assert (pps->num_slice_groups_minus1 == 0);

  // FMO stuff begins here
  if (pps->num_slice_groups_minus1 > 0)
  {
    pps->mb_slice_group_map_type               = ue_v ("PPS: mb_slice_group_map_type"                , p);
    if (pps->mb_slice_group_map_type == 0)
      for (i=0; i<pps->num_slice_groups_minus1; i++)
        pps->run_length [i]                        = ue_v ("PPS: run_length [i]"                         , p);
    else if (pps->mb_slice_group_map_type == 2)
      for (i=0; i<pps->num_slice_groups_minus1; i++)
      {
        //! JVT-F078: avoid reference of SPS by using ue(v) instead of u(v)
        pps->top_left_mb [i]                       = ue_v ("PPS: top_left_mb [i]"                        , p);
        pps->bottom_right_mb [i]                   = ue_v ("PPS: bottom_right_mb [i]"                    , p);
      }
    else if (pps->mb_slice_group_map_type == 3 ||
             pps->mb_slice_group_map_type == 4 ||
             pps->mb_slice_group_map_type == 5)
    {
      pps->slice_group_change_direction_flag     = u_1  ("PPS: slice_group_change_direction_flag"      , p);
      pps->slice_group_change_rate_minus1        = ue_v ("PPS: slice_group_change_rate_minus1"         , p);
    }
    else if (pps->mb_slice_group_map_type == 6)
    {
      if (pps->num_slice_groups_minus1 >= 4)
        NumberBitsPerSliceGroupId = 3;
      else if (pps->num_ref_idx_l0_active_minus1 >= 2)
        NumberBitsPerSliceGroupId = 2;
      else if (pps->num_ref_idx_l0_active_minus1 >= 1)
        NumberBitsPerSliceGroupId = 1;
      else
        NumberBitsPerSliceGroupId = 0;
      //! JVT-F078, exlicitly signal number of MBs in the map
      pps->slice_group_id_cnt_minus1              = ue_v ("PPS: slice_group_id_cnt_minus1"               , p);
      for (i=0; i<=pps->slice_group_id_cnt_minus1; i++)
        pps->slice_group_id[i] = u_v (NumberBitsPerSliceGroupId, "slice_group_id[i]", p);
    }
  }
  // End of FMO stuff

  pps->num_ref_idx_l0_active_minus1          = ue_v ("PPS: num_ref_idx_l0_active_minus1"           , p);
  pps->num_ref_idx_l1_active_minus1          = ue_v ("PPS: num_ref_idx_l1_active_minus1"           , p);
  pps->weighted_pred_flag                    = u_1  ("PPS: wheighted prediction flag"              , p);
  pps->weighted_bipred_idc                   = u_v  ( 2, "PPS: weighted_bipred_idc"                , p);
  pps->pic_init_qp_minus26                   = se_v ("PPS: pic_init_qp_minus26"                    , p);
  pps->pic_init_qs_minus26                   = se_v ("PPS: pic_init_qs_minus26"                    , p);
  pps->chroma_qp_index_offset                = se_v ("PPS: chroma_qp_index_offset"                 , p);
  pps->deblocking_filter_parameters_present_flag = u_1 ("PPS: deblocking_filter_parameters_present_flag", p);
  pps->constrained_intra_pred_flag           = u_1  ("PPS: constrained_intra_pred_flag"            , p);
  pps->redundant_pic_cnt_present_flag        = u_1  ("PPS: vui_pic_parameters_flag"                 , p);
  pps->frame_cropping_flag                   = u_1  ("PPS: frame_cropping_flag"                , p);

  if (pps->frame_cropping_flag)
  {
    pps->frame_cropping_rect_left_offset      = ue_v ("PPS: frame_cropping_rect_left_offset"           , p);
    pps->frame_cropping_rect_right_offset     = ue_v ("PPS: frame_cropping_rect_right_offset"          , p);
    pps->frame_cropping_rect_top_offset       = ue_v ("PPS: frame_cropping_rect_top_offset"            , p);
    pps->frame_cropping_rect_bottom_offset    = ue_v ("PPS: frame_cropping_rect_bottom_offset"         , p);
  }
  pps->Valid = TRUE;
  return UsedBits;
}


void DumpSPS (seq_parameter_set_rbsp_t *sps)
{
  printf ("Dumping a sequence parset, to be implemented\n");
};

void DumpPPS (pic_parameter_set_rbsp_t *pps)
{
  printf ("Dumping a picture parset\n");
}

void PPSConsistencyCheck (pic_parameter_set_rbsp_t *pps)
{
  printf ("Consistency checking a picture parset\n");
//  if (pps->seq_parameter_set_id invalid then do something)
}

void SPSConsistencyCheck (seq_parameter_set_rbsp_t *sps)
{
  printf ("Consistency checkinga sequence parset\n");
}

void MakePPSavailable (int id, pic_parameter_set_rbsp_t *pps)
{
  assert (pps->Valid == TRUE);

  if (PicParSet[id].Valid == TRUE && PicParSet[id].slice_group_id != NULL)
    free (PicParSet[id].slice_group_id);

  memcpy (&PicParSet[id], pps, sizeof (pic_parameter_set_rbsp_t));
  if ((PicParSet[id].slice_group_id = calloc (PicParSet[id].slice_group_id_cnt_minus1+1, sizeof(int))) == NULL)
    no_mem_exit ("MakePPSavailable: Cannot calloc slice_group_id");
  
  memcpy (PicParSet[id].slice_group_id, pps->slice_group_id, (pps->slice_group_id_cnt_minus1+1)*sizeof(int));
}

void MakeSPSavailable (int id, seq_parameter_set_rbsp_t *sps)
{
  assert (sps->Valid == TRUE);
  memcpy (&SeqParSet[id], sps, sizeof (seq_parameter_set_rbsp_t));
}


void ProcessSPS (NALU_t *nalu)
{
  DataPartition *dp = AllocPartition(1);
  seq_parameter_set_rbsp_t *sps = AllocSPS();
  int dummy;

  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->code_len = dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);
  dp->bitstream->ei_flag = 0;
  dp->bitstream->read_len = dp->bitstream->frame_bitoffset = 0;
  dummy = InterpretSPS (dp, sps);
  // DumpSPS (sps);
  // SPSConsistencyCheck (pps);
  MakeSPSavailable (sps->seq_parameter_set_id, sps);
  FreePartition (dp, 1);
  FreeSPS (sps);
}


void ProcessPPS (NALU_t *nalu)
{
  DataPartition *dp;
  pic_parameter_set_rbsp_t *pps = AllocPPS();
  int dummy;

  dp = AllocPartition(1);
  pps = AllocPPS();
  memcpy (dp->bitstream->streamBuffer, &nalu->buf[1], nalu->len-1);
  dp->bitstream->code_len = dp->bitstream->bitstream_length = RBSPtoSODB (dp->bitstream->streamBuffer, nalu->len-1);
  dp->bitstream->ei_flag = 0;
  dp->bitstream->read_len = dp->bitstream->frame_bitoffset = 0;
  dummy = InterpretPPS (dp, pps);
  // DumpPPS (pps);
  // PPSConsistencyCheck (pps);
  MakePPSavailable (pps->pic_parameter_set_id, pps);
  FreePartition (dp, 1);
  FreePPS (pps);
}






void UseParameterSet (int PicParsetId)
{
  seq_parameter_set_rbsp_t *sps = &SeqParSet[PicParSet[PicParsetId].seq_parameter_set_id];
  pic_parameter_set_rbsp_t *pps = &PicParSet[PicParsetId];
  static unsigned int ExpectedDeltaPerPicOrderCntCycle;
  int i;


  if (PicParSet[PicParsetId].Valid != TRUE)
    printf ("Trying to use an invalid (uninitialized) Picture Parameter Set with ID %d, expect the unexpected...\n", PicParsetId);
  if (SeqParSet[PicParSet[PicParsetId].seq_parameter_set_id].Valid != TRUE)
    printf ("PicParset %d references an invalid (uninitialized) Sequence Parameter Set with ID %d, expect the unexpected...\n", PicParsetId, PicParSet[PicParsetId].seq_parameter_set_id);

  sps =  &SeqParSet[PicParSet[PicParsetId].seq_parameter_set_id];

  active_sps = sps;
  active_pps = pps;

  // In theory, and with a well-designed software, the lines above
  // are everything necessary.  In practice, we need to patch many values
  // in img-> (but no more in inp-> -- these have been taken care of)


  // Sequence Parameter Set Stuff first

//  printf ("Using Picture Parameter set %d and associated Sequence Parameter Set %d\n", PicParsetId, PicParSet[PicParsetId].seq_parameter_set_id);

//  img->log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
  img->MaxFrameNum = 1<<(sps->log2_max_frame_num_minus4+4);
  
  img->pic_order_cnt_type = sps->pic_order_cnt_type;
  if (img->pic_order_cnt_type != 1)
  {
    printf ("sps->pic_order_cnt_type %d, expected 1, exoect the unexpected...\n", sps->pic_order_cnt_type);
    assert (sps->pic_order_cnt_type == 1);
    error ("pic_order_cnt_type != 1", -1000);
  }

  img->num_ref_frames_in_pic_order_cnt_cycle = sps->num_ref_frames_in_pic_order_cnt_cycle;
  if(img->num_ref_frames_in_pic_order_cnt_cycle != 1)
    error("num_ref_frames_in_pic_order_cnt_cycle != 1",-1001);
  if(img->num_ref_frames_in_pic_order_cnt_cycle >= MAX_LENGTH_POC_CYCLE)
    error("num_ref_frames_in_pic_order_cnt_cycle too large",-1011);
  
  img->delta_pic_order_always_zero_flag = sps->delta_pic_order_always_zero_flag;
  if(img->delta_pic_order_always_zero_flag != 0)
    error ("delta_pic_order_always_zero_flag != 0",-1002);
 
  img->offset_for_non_ref_pic = sps->offset_for_non_ref_pic;
  
  img->offset_for_top_to_bottom_field = sps->offset_for_top_to_bottom_field;
  

  ExpectedDeltaPerPicOrderCntCycle=0;
  if (sps->num_ref_frames_in_pic_order_cnt_cycle)
    for(i=0;i<(int)sps->num_ref_frames_in_pic_order_cnt_cycle;i++) 
    {
      img->offset_for_ref_frame[i] = sps->offset_for_ref_frame[i];
      ExpectedDeltaPerPicOrderCntCycle += sps->offset_for_ref_frame[i];
    }

  img->width = (sps->frame_width_in_mbs_minus1+1) * MB_BLOCK_SIZE;
  img->width_cr = img->width /2;
  img->height = (sps->frame_height_in_mbs_minus1+1) * MB_BLOCK_SIZE;
  img->height_cr = img->height / 2;

  // Picture Parameter Stuff

  img->weighted_pred_flag = pps->weighted_pred_flag;
  img->weighted_bipred_explicit_flag = (pps->weighted_bipred_idc==1);
  img->weighted_bipred_implicit_flag = (pps->weighted_bipred_idc==2);

  img->pic_order_present_flag = pps->pic_order_present_flag;
  if(img->pic_order_present_flag != 0)
    error ("pic_order_present_flag != 0",-1004);

  img->constrained_intra_pred_flag = pps->constrained_intra_pred_flag;


  // currSlice->dp_mode is set by read_new_slice (NALU first byte available there)
  switch (pps->entropy_coding_mode)
  {
  case UVLC:
    nal_startcode_follows = uvlc_startcode_follows;
    for (i=0; i<3; i++)
    {
      img->currentSlice->partArr[i].readSyntaxElement = readSyntaxElement_UVLC;
    }
    break;
  case CABAC:
    nal_startcode_follows = cabac_startcode_follows;
//        currSlice->readSlice = readSliceCABAC;
    for (i=0; i<3; i++)
    {
      img->currentSlice->partArr[i].readSyntaxElement = readSyntaxElement_CABAC;
    }
    break;
  default:
    printf ("Unknown PPS:entropy_coding_mode %d\n", pps->entropy_coding_mode);
    assert (0==1);
  }

}