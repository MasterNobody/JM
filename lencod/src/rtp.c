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
 *****************************************************************************
 *
 * \file rtp.c
 *
 * \brief
 *    Functions to handle RTP headers and packets per RFC1889 and RTP NAL spec
 *    Functions support little endian systems only (Intel, not Motorola/Sparc)
 *
 * \date
 *    30 September 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <memory.h>
#include <malloc.h>

#include "rtp.h"
#include "elements.h"
#include "defines.h"
#include "header.h"

#include "global.h"
#include "fmo.h"
#include "encodeiff.h"
#include "sei.h"

// A little trick to avoid those horrible #if TRACE all over the source code
#if TRACE
#define SYMTRACESTRING(s) strncpy(sym.tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // to nothing
#endif


int CurrentRTPTimestamp = 0;      //! The RTP timestamp of the current packet,
                                  //! incremented with all P and I frames
int CurrentRTPSequenceNumber = 0; //! The RTP sequence number of the current packet
                                  //! incremented by one for each sent packet

/*!
 *****************************************************************************
 *
 * \brief 
 *    ComposeRTPpacket composes the complete RTP packet using the various
 *    structure members of the RTPpacket_t structure
 *
 * \return
 *    0 in case of success
 *    negative error code in case of failure
 *
 * \para Parameters
 *    Caller is responsible to allocate enough memory for the generated packet
 *    in parameter->packet. Typically a malloc of 12+paylen bytes is sufficient
 *
 * \para Side effects
 *    none
 *
 * \para Other Notes
 *    Function contains assert() tests for debug purposes (consistency checks
 *    for RTP header fields
 *
 * \date
 *    30 Spetember 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int ComposeRTPPacket (RTPpacket_t *p)

{
  // Consistency checks through assert, only used for debug purposes
  assert (p->v == 2);
  assert (p->p == 0);
  assert (p->x == 0);
  assert (p->cc == 0);    // mixer designers need to change this one
  assert (p->m == 0 || p->m == 1);
  assert (p->pt < 128);
  assert (p->seq < 65536);
  assert (p->payload != NULL);
  assert (p->paylen < 65536 - 40);  // 2**16 -40 for IP/UDP/RTP header
  assert (p->packet != NULL);

  // Compose RTP header, little endian

  p->packet[0] = (   (p->v)
                  |  (p->p << 2)
                  |  (p->x << 3)
                  |  (p->cc << 4) );
  p->packet[1] = (   (p->m)
                  |  (p->pt << 1) );
  p->packet[2] = p->seq & 0xff;
  p->packet[3] = (p->seq >> 8) & 0xff;

  memcpy (&p->packet[4], &p->timestamp, 4);  // change to shifts for unified byte sex
  memcpy (&p->packet[8], &p->ssrc, 4);// change to shifts for unified byte sex

  // Copy payload 

  memcpy (&p->packet[12], p->payload, p->paylen);
  p->packlen = p->paylen+12;
  return 0;
}



/*!
 *****************************************************************************
 *
 * \brief 
 *    WriteRTPPacket writes the supplied RTP packet to the output file
 *
 * \return
 *    0 in case of access
 *    <0 in case of write failure (typically fatal)
 *
 * \para Parameters
 *    p: the RTP packet to be written (after ComposeRTPPacket() )
 *    f: output file
 *
 * \para Side effects
 *    none
 *
 * \date
 *    October 23, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

int WriteRTPPacket (RTPpacket_t *p, FILE *f)

{
  int intime = -1;

  assert (f != NULL);
  assert (p != NULL);


  if (1 != fwrite (&p->packlen, 4, 1, f))
    return -1;
  if (1 != fwrite (&intime, 4, 1, f))
    return -1;
  if (1 != fwrite (p->packet, p->packlen, 1, f))
    return -1;
  return 0;
}

#if 0

// OLD CODE, delete StW
//
// Note: this old code does not support FMO, but is also buggy (slen is
// incorrect, sprintf does not return the number of byte sin the string!
//

/*!
 *****************************************************************************
 *
 * \brief 
 *    ComposeHeaderPacketPayload generates the payload for a header packet
 *    using the inp-> structure contents.  The header packet contains 
 *    definitions for a single parameter set 0, which is used for all 
 *    slices of the picture
 *
 * \return
 *    len of the genberated payload in bytes
 *    <0 in case of failure (typically fatal)
 *
 * \para Parameters
 *    p: the payload of the RTP packet to be written
 *
 * \para Side effects
 *    none
 *
 * \note
 *    This function should be revisited and checked in case of additional
 *    bit stream parameters that affect the picture header (or higher
 *    entitries).  Typical examples are more entropy coding schemes, other
 *    motion vector resolutiuon, and optional elements
 *
 * \date
 *    October 23, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int ComposeHeaderPacketPayload (byte *payload)
{
  int slen=0;
  int multpred;

  assert (img->width%16 == 0);
  assert (img->height%16 == 0);

#ifdef _ADDITIONAL_REFERENCE_FRAME_
  if (input->no_multpred <= 1 && input->add_ref_frame == 0)
#else
  if (input->no_multpred <= 1)
#endif
    multpred=FALSE;
  else
    multpred=TRUE;               // multiple reference frames in motion search

  
  slen = snprintf (payload, MAXRTPPAYLOADLEN, 
             "a=H26L (0) MaxPicID %d\
            \na=H26L (0) UseMultpred %d\
            \na=H26L (0) MaxPn %d\
            \na=H26L (0) BufCycle %d\
            \na=H26L (0) PixAspectRatioX 1\
            \na=H26L (0) PixAspectRatioY 1\
            \na=H26L (0) DisplayWindowOffsetTop 0\
            \na=H26L (0) DisplayWindowOffsetBottom 0\
            \na=H26L (0) DisplayWindowOffsetRight 0\
            \na=H26L (0) DisplayWindowOffsetLeft 0\
            \na=H26L (0) XSizeMB %d\
            \na=H26L (0) YSizeMB %d\
            \na=H26L (0) EntropyCoding %s\
            \na=H26L (0) PartitioningType %s\
            \na=H26L (0) IntraPredictionType %s\
            \na=H26L (0) HRCParameters 0\
            \
            \na=H26L (-1) FramesToBeEncoded %d\
            \na=H26L (-1) FrameSkip %d\
            \na=H26L (-1) SequenceFileName %s\
            \na=H26L (0) NumberBFrames %d\
            %c%c",

            256,
            multpred==TRUE?1:0,
            input->no_multpred,
            input->no_multpred,
            input->img_width/16,
            input->img_height/16,
            input->symbol_mode==UVLC?"UVLC":"CABAC",
            input->partition_mode==0?"one":"three",
            input->UseConstrainedIntraPred==0?"Unconstrained":"Constrained",
            
            input->no_frames,
            input->jumpd,
            input->infile,
            input->successive_Bframe,
            4,     // Unix Control D EOF symbol
            26);  // MS-DOS/Windows Control Z EOF Symbol
  return slen;
}


#endif


/*!
 *****************************************************************************
 *
 * \brief 
 *    ComposeHeaderPacketPayload generates the payload for a header packet
 *    using the inp-> structure contents.  The header packet contains 
 *    definitions for a single parameter set 0, which is used for all 
 *    slices of the picture
 *
 * \return
 *    len of the genberated payload in bytes
 *    <0 in case of failure (typically fatal)
 *
 * \para Parameters
 *    p: the payload of the RTP packet to be written
 *
 * \para Side effects
 *    none
 *
 * \note
 *    This function should be revisited and checked in case of additional
 *    bit stream parameters that affect the picture header (or higher
 *    entitries).  Typical examples are more entropy coding schemes, other
 *    motion vector resolutiuon, and optional elements
 *
 * \date
 *    October 23, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int ComposeHeaderPacketPayload (byte *payload)
{
  int slen=0;
  int multpred;
  int y, x, i;

  assert (img->width%16 == 0);
  assert (img->height%16 == 0);

#ifdef _ADDITIONAL_REFERENCE_FRAME_
  if (input->no_multpred <= 1 && input->add_ref_frame == 0)
#else
  if (input->no_multpred <= 1)
#endif
    multpred=FALSE;
  else
    multpred=TRUE;               // multiple reference frames in motion search

  
  snprintf (payload, MAXRTPPAYLOADLEN, 
             "a=H26L (0) MaxPicID %d\
            \na=H26L (0) UseMultpred %d\
            \na=H26L (0) MaxPn %d\
            \na=H26L (0) BufCycle %d\
            \na=H26L (0) PixAspectRatioX 1\
            \na=H26L (0) PixAspectRatioY 1\
            \na=H26L (0) DisplayWindowOffsetTop 0\
            \na=H26L (0) DisplayWindowOffsetBottom 0\
            \na=H26L (0) DisplayWindowOffsetRight 0\
            \na=H26L (0) DisplayWindowOffsetLeft 0\
            \na=H26L (0) XSizeMB %d\
            \na=H26L (0) YSizeMB %d\
            \na=H26L (0) FilterParametersFlag %d\
            \na=H26L (0) EntropyCoding %s\
            \na=H26L (0) PartitioningType %s\
            \na=H26L (0) IntraPredictionType %s\
            \na=H26L (0) HRCParameters 0\
            \
            \na=H26L (-1) FramesToBeEncoded %d\
            \na=H26L (-1) FrameSkip %d\
            \na=H26L (-1) SequenceFileName %s\
            \na=H26L (0) NumberBFrames %d",

            256,
            multpred==TRUE?1:0,
            input->no_multpred,
            input->no_multpred,
            input->img_width/16,
            input->img_height/16,
            input->LFSendParameters,
            input->symbol_mode==UVLC?"UVLC":"CABAC",
            input->partition_mode==0?"one":"three",
            input->UseConstrainedIntraPred==0?"Unconstrained":"Constrained",
            
            input->no_frames,
            input->jumpd,
            input->infile,
            input->successive_Bframe);
  slen = strlen (payload);
  snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) FMOmap \\\n");
  slen = strlen(payload);

  for (y=0; y<img->height/16; y++)
  {
    for (x=0; x<img->width/16; x++)
    {
      snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "%d ", FmoMB2SliceGroup (y*(img->width/16)+x));
      slen = strlen (payload);
    }
    snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\\\n");
    slen = strlen (payload);
  }

  // JVT-D095, JVT-D097
  // the orignal implementation of FMO is left intact. 
  snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) num_slice_groups_minus1 %d", 
            input->num_slice_groups_minus1);
  slen = strlen(payload);
  if(input->num_slice_groups_minus1 > 0)
  {
    snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) mb_allocation_map_type %d", 
              input->mb_allocation_map_type);
    slen = strlen(payload);

    if(input->mb_allocation_map_type == 3)
    {
      assert(input->num_slice_groups_minus1 == 1);
      for(i=0; i<input->num_slice_groups_minus1; i++)
      {
        snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) top_left_mb %d", 
                  input->top_left_mb);
        slen = strlen(payload);
        snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) bottom_right_mb %d", 
                  input->bottom_right_mb);
        slen = strlen(payload);
      }
    }
    if(input->mb_allocation_map_type == 4 || input->mb_allocation_map_type == 5 || input->mb_allocation_map_type == 6)
    {
      assert(input->num_slice_groups_minus1 == 1);
      snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) slice_group_change_direction %d", 
                input->slice_group_change_direction);
      slen = strlen(payload);
      snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) slice_group_change_rate_minus1 %d", 
                input->slice_group_change_rate_minus1);
      slen = strlen(payload);
    }
  }
  // End JVT-D095, JVT-D097

  // JVT-D101
  snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\na=H26L (0) redundant_slice_flag %d", 
            input->redundant_slice_flag);
  slen = strlen(payload);
  // End JVT-D101


  snprintf (&payload[slen], MAXRTPPAYLOADLEN-slen, "\n%c%c%c\n",
            4,     // Unix Control D EOF symbol
            26,    // MS-DOS/Windows Control Z EOF Symbol
            0);   // End of String
  slen = strlen (payload);
  //printf ("Parameter Set Packet '\n%s\n'\n", payload);
  return slen;
}




/*!
 *****************************************************************************
 *
 * \brief 
 *    void RTPSequenceHeader (FILE *out) write the RTP Sequence header in the
 *       form of a header packet into the outfile.  It is assumed that the
 *       input-> structure is already filled (config file is parsed)
 *
 * \return
 *    length of sequence header
 *
 * \para Parameters
 *    out: fiel pointer of the output file
 *
 * \para Side effects
 *    header written, statistics are updated, RTP sequence number updated
 *
 * \para Note
 *    This function uses alloca() to reserve memry on the stack -- no freeing and
 *    no return value check necessary, error lead to stack overflows
 *
 * \date
 *    October 24, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

int RTPSequenceHeader (FILE *out)
{
  RTPpacket_t *p;

  assert (out != NULL);

  // Set RTP structure elements and alloca() memory foor the buffers
  p = (RTPpacket_t *) alloca (sizeof (RTPpacket_t));
  p->packet=alloca (MAXRTPPACKETSIZE);
  p->payload=alloca (MAXRTPPACKETSIZE);
  p->v=2;
  p->p=0;
  p->x=0;
  p->cc=0;
  p->m=1;
  p->pt=H26LPAYLOADTYPE;
  p->seq=CurrentRTPSequenceNumber++;
  p->timestamp=CurrentRTPTimestamp;
  p->ssrc=H26LSSRC;

  // Set First Byte of RTP payload, see VCEG-N72r1
  p->payload[0] = 
    6 |               // Parameter Update Packet
    0 |               // no know error
    0;                // reserved

  // Generate the Parameter update SDP syntax, donm't overwrite First Byte
  p->paylen = 1 + ComposeHeaderPacketPayload (&p->payload[1]);

  // Generate complete RTP packet
  if (ComposeRTPPacket (p) < 0)
  {
    printf ("Cannot compose RTP packet, exit\n");
    exit (-1);
  }

  if (WriteRTPPacket (p, out) < 0)
  {
    printf ("Cannot write %d bytes of RTP packet to outfile, exit\n", p->packlen);
    exit (-1);
  }
  return (p->packlen);
}



/*!
 *****************************************************************************
 *
 * \brief 
 *    int RTPSliceHeader () write the Slice header for the RTP NAL      
 *
 * \return
 *    Number of bits used by the slice header
 *
 * \para Parameters
 *    none
 *
 * \para Side effects
 *    Slice header as per VCEG-N72r2 is written into the appropriate partition
 *    bit buffer
 *
 * \para Limitations/Shortcomings/Tweaks
 *    The current code does not support the change of picture parameters within
 *    one coded sequence, hence there is only one parameter set necessary.  This
 *    is hard coded to zero.
 *    As per Email discussions on 10/24/01 we decided to include the SliceID
 *    into both the Full Slice pakcet and Partition A packet
 *   
 * \date
 *    October 24, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int RTPSliceHeader()
{
  int dP_nr;
  Bitstream *currStream; 
  DataPartition *partition;
  SyntaxElement sym;

  int len = 0;
  int RTPSliceType;

  if(img->type==BS_IMG && img->type!=B_IMG)
    dP_nr= assignSE2partition[input->partition_mode][SE_BFRAME];
  else
    dP_nr= assignSE2partition[input->partition_mode][SE_HEADER];

  currStream = ((img->currentSlice)->partArr[dP_nr]).bitstream;
  partition = &((img->currentSlice)->partArr[dP_nr]);

  assert (input->of_mode == PAR_OF_RTP);
  sym.type = SE_HEADER;         // This will be true for all symbols generated here
  sym.mapping = n_linfo2;       // Mapping rule: Simple code number to len/info


  SYMTRACESTRING("RTP-SH: Parameter Set");
  sym.value1 = 0;
  len += writeSyntaxElement_UVLC (&sym, partition);
  
  // picture structure, (0: progressive, 1: top field, 2: bottom field, 
  // 3: top field first, 4: bottom field first)
  sym.value1 = img->pstruct;
  SYMTRACESTRING("Picture Stucture");
  len += writeSyntaxElement_UVLC (&sym, partition);

  SYMTRACESTRING("RTP-SH: Picture ID");
  sym.value1 = img->currentSlice->picture_id;

  len += writeSyntaxElement_UVLC (&sym, partition);

  /*! StW:
  // Note: we currently do not support mixed slice types in the encoder, hence
  // we use the picture type here (although there is no such thing as a picture
  // type any more)
  //
  // This needs to be double checked against the document.  Email inquiery on
  // 10/24 evening: is there a need for distinguishing the multpred P from regular
  // P?  if so, this has to be revisited! 
  //
  // StW bug fix: write Slice type according to document
  */

  switch (img->type)
  {
    case INTER_IMG:
      if (img->types==SP_IMG)
        RTPSliceType = 2;
      else
        RTPSliceType = 0;
      break;
    case INTRA_IMG:
      RTPSliceType = 3;
      break;
    case B_IMG:
    case BS_IMG:
      RTPSliceType = 1;
      break;
    case SP_IMG:
      // That's what I would expect to be the case. But img->type contains INTER_IMG 
      // here (see handling above)
      //! \todo make one variable from img->type and img->types
      RTPSliceType = 2;
      break;
    default:
      printf ("Panic: unknown picture type %d, exit\n", img->type);
      exit (-1);
  }


  SYMTRACESTRING("RTP-SH: Slice Type");
  sym.value1 = RTPSliceType;
  len += writeSyntaxElement_UVLC (&sym, partition);

  // NOTE: following syntax elements have to be moved into nal_unit() and changed from e(v) to u(1)
  if(1)
  {
    sym.value1 = img->disposable_flag;
    SYMTRACESTRING("RTP-SH: NAL disposable_flag");
    len += writeSyntaxElement_UVLC (&sym, partition);
  }
  
  // NOTE: following syntax elements have to be moved into picture_layer_rbsp()
  // weighted_bipred_implicit_flag
  if(img->type==B_IMG || img->type==BS_IMG)
  {
    sym.value1 = (input->BipredictiveWeighting > 0)? (input->BipredictiveWeighting-1) : 0;
    SYMTRACESTRING("RTP-SH: weighted_bipred_implicit_flag");
    len += writeSyntaxElement_UVLC (&sym, partition);
  }

  if (img->type==INTER_IMG || img->type==B_IMG || img->type==BS_IMG)
  {
    sym.value1 = img->num_ref_pic_active_fwd_minus1;
    SYMTRACESTRING("RTP-SH: num_ref_pic_active_fwd_minus1");
    len += writeSyntaxElement_UVLC (&sym, partition);
    if (img->type==B_IMG || img->type==BS_IMG)
    {
      sym.value1 = img->num_ref_pic_active_bwd_minus1;
      SYMTRACESTRING("RTP-SH: num_ref_pic_active_bwd_minus1");
      len += writeSyntaxElement_UVLC (&sym, partition);
    }
  }

  SYMTRACESTRING("RTP-SH: FirstMBInSliceX");
  sym.value1 = img->mb_x;
  len += writeSyntaxElement_UVLC (&sym, partition);

  SYMTRACESTRING("RTP-SH: FirstMBinSlinceY");
  sym.value1 = img->mb_y;
  len += writeSyntaxElement_UVLC (&sym, partition);

  // JVT-D101: redundant_pic_cnt should be a ue(v) instead of u(1). However since currently we allow 
  // only 1 redundant slice for each slice, therefore herein encoding it as u(1) is OK.
  if(input->redundant_slice_flag) 
  {
    SYMTRACESTRING("RTP-SH: redundant_pic_cnt");
    rpc_bytes_to_go = currStream->byte_pos;
    rpc_bits_to_go = currStream->bits_to_go;
    sym.bitpattern = img->redundant_pic_cnt = 0; //may be modifed before being written into bitstream file
    sym.len = 1;
    len += writeSyntaxElement_fixed(&sym, partition);
  }
  // End JVT-D101

  if (img->type==B_IMG || img->type==BS_IMG)
  {
    SYMTRACESTRING("RTP-SH DirectSpatialFlag");
    sym.bitpattern = input->direct_type;  // 1 for Spatial Direct, 0 for temporal Direct
    sym.len = 1;
    len += writeSyntaxElement_fixed(&sym, partition);
  }

#ifdef _ABT_FLAG_IN_SLICE_HEADER_
  SYMTRACESTRING("RTP-SH ABTMode");
  sym.value1 = input->abt;
  len += writeSyntaxElement_UVLC (&sym, partition);
#endif

  SYMTRACESTRING("RTP-SH: InitialQP");
  sym.mapping = dquant_linfo;
  sym.value1 = img->qp - (MAX_QP - MIN_QP +1)/2;
  len += writeSyntaxElement_UVLC (&sym, partition);

  if (img->types==SP_IMG)
  {

    if (img->type!=INTRA_IMG) // Switch Flag only for SP pictures
    {
      SYMTRACESTRING("RTP-SH SWITCH FLAG");
      sym.bitpattern = 0;  // 1 for switching SP, 0 for normal SP
      sym.len = 1;
      len += writeSyntaxElement_fixed(&sym, partition);
    }

    SYMTRACESTRING("RTP-SH: SP InitialQP");
    sym.value1 = img->qpsp - (MAX_QP - MIN_QP +1)/2;
    len += writeSyntaxElement_UVLC (&sym, partition);
  }

  if (input->LFSendParameters)
  {
    SYMTRACESTRING("RTP-SH LF_DISABLE FLAG");
    sym.bitpattern = input->LFDisable;  /* Turn loop filter on/off on slice basis */
    sym.len = 1;
    len += writeSyntaxElement_fixed(&sym, partition);

    if (!input->LFDisable)
    {
      sym.mapping = dquant_linfo;           // Mapping rule: Signed integer
      SYMTRACESTRING("RTP-SH LFAlphaC0OffsetDiv2");
      sym.value1 = input->LFAlphaC0Offset>>1; /* Convert from offset to code */
      len += writeSyntaxElement_UVLC (&sym, partition);

      SYMTRACESTRING("RTP-SH LFBetaOffsetDiv2");
      sym.value1 = input->LFBetaOffset>>1; /* Convert from offset to code */
      len += writeSyntaxElement_UVLC (&sym, partition);
    }
  }

  sym.mapping = n_linfo2;


  /*! StW:
  // Note: VCEG-N72 says that the slice type UVLC is not present in single
  // slice packets.  Generally, the NAL allows to mix Single Slice and Partitioning,
  // however, the software does not.  Hence it is sufficient here to express
  // the dependency by checkling the input parameter
  */

  if (input->partition_mode == PAR_DP_3 && img->type != B_IMG && img->type != BS_IMG)
  {
    SYMTRACESTRING("RTP-SH: Slice ID");
    sym.value1 = img->current_slice_nr;
    len += writeSyntaxElement_UVLC (&sym, partition);
  }

  /* KS: add Annex U Syntax elements */
  len += writeERPS(&sym, partition);

  // JVT-D097
  if( input->num_slice_groups_minus1 > 0  &&  
      input->mb_allocation_map_type >= 4  &&  
		  input->mb_allocation_map_type <= 6)
  {
    SYMTRACESTRING("RTP-SH: slice_group_change_cycle");
    sym.value1 = slice_group_change_cycle;
    len += writeSyntaxElement_UVLC (&sym, partition);
  }
  // End JVT-D097

  // After this, and when in CABAC mode, terminateSlice() may insert one more
  // UVLC codeword for the number of coded MBs

  return len;
}


/*!
 *****************************************************************************
 *
 * \brief 
 *    int RTPWriteBits (int marker) write the Slice header for the RTP NAL      
 *
 * \return
 *    Number of bytes written to output file
 *
 * \para Parameters
 *    marker: markber bit,
 *
 * \para Side effects
 *    Packet written, RTPSequenceNumber and RTPTimestamp updated
 *   
 * \date
 *    October 24, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int RTPWriteBits (int Marker, int PacketType, void * bitstream, 
                    int BitStreamLenInByte, FILE *out)
{
  RTPpacket_t *p;

  assert (out != NULL);
  assert (BitStreamLenInByte < 65000);
  assert (bitstream != NULL);
  assert ((PacketType&0xf) < 4);

  // Set RTP structure elements and alloca() memory foor the buffers
  p = (RTPpacket_t *) alloca (sizeof (RTPpacket_t));
  p->packet=alloca (MAXRTPPACKETSIZE);
  p->payload=alloca (MAXRTPPACKETSIZE);
  p->v=2;
  p->p=0;
  p->x=0;
  p->cc=0;
  p->m=Marker&1;
  p->pt=H26LPAYLOADTYPE;
  p->seq=CurrentRTPSequenceNumber++;
  p->timestamp=CurrentRTPTimestamp;
  p->ssrc=H26LSSRC;

  p->payload[0] = PacketType;

  p->paylen = 1 + BitStreamLenInByte;


  memcpy (&p->payload[1], bitstream, BitStreamLenInByte);

  // Generate complete RTP packet
  if (ComposeRTPPacket (p) < 0)
  {
    printf ("Cannot compose RTP packet, exit\n");
    exit (-1);
  }
  if (WriteRTPPacket (p, out) < 0)
  {
    printf ("Cannot write %d bytes of RTP packet to outfile, exit\n", p->packlen);
    exit (-1);
  }
  return (p->packlen);

}

/*!
 *****************************************************************************
 *
 * \brief 
 *    int aggregationRTPWriteBits (int marker) write the Slice header for the RTP NAL      
 *
 * \return
 *    Number of bytes written to output file
 *
 * \para Parameters
 *    marker: markber bit,
 *
 * \para Side effects
 *    Packet written, RTPSequenceNumber and RTPTimestamp updated
 *   
 * \date
 *    September 10, 2002
 *
 * \author
 *    Dong Tian   tian@cs.tut.fi
 *****************************************************************************/

int aggregationRTPWriteBits (int Marker, int PacketType, int subPacketType, void * bitstream, 
                    int BitStreamLenInByte, FILE *out)
{
  RTPpacket_t *p;
  int offset;

//  printf( "writing aggregation packet...\n");
  assert (out != NULL);
  assert (BitStreamLenInByte < 65000);
  assert (bitstream != NULL);
  assert ((PacketType&0xf) == 4);

  // Set RTP structure elements and alloca() memory foor the buffers
  p = (RTPpacket_t *) alloca (sizeof (RTPpacket_t));
  p->packet=alloca (MAXRTPPACKETSIZE);
  p->payload=alloca (MAXRTPPACKETSIZE);
  p->v=2;
  p->p=0;
  p->x=0;
  p->cc=0;
  p->m=Marker&1;
  p->pt=H26LPAYLOADTYPE;
  p->seq=CurrentRTPSequenceNumber++;
  p->timestamp=CurrentRTPTimestamp;
  p->ssrc=H26LSSRC;

  offset = 0;
  p->payload[offset++] = PacketType; // This is the first byte of the compound packet

  // FIRST, write the sei message to aggregation packet, if it is available
  if ( HaveAggregationSEI() )
  {
    p->payload[offset++] = sei_message[AGGREGATION_SEI].subPacketType; // this is the first byte of the first subpacket
    *(short*)&(p->payload[offset]) = sei_message[AGGREGATION_SEI].payloadSize;
    offset += 2;
    memcpy (&p->payload[offset], sei_message[AGGREGATION_SEI].data, sei_message[AGGREGATION_SEI].payloadSize);
    offset += sei_message[AGGREGATION_SEI].payloadSize;

    clear_sei_message(AGGREGATION_SEI);
  }

  // SECOND, write other payload to the aggregation packet
  // to do ...

  // LAST, write the slice data to the aggregation packet
  p->payload[offset++] = subPacketType;  // this is the first byte of the second subpacket
  *(short*)&(p->payload[offset]) = BitStreamLenInByte;
  offset += 2;
  memcpy (&p->payload[offset], bitstream, BitStreamLenInByte);
  offset += BitStreamLenInByte;

  p->paylen = offset;  // 1 +3 +seiPayload.payloadSize +3 +BitStreamLenInByte

  // Now the payload is ready, we can ...
  // Generate complete RTP packet
  if (ComposeRTPPacket (p) < 0)
  {
    printf ("Cannot compose RTP packet, exit\n");
    exit (-1);
  }
  if (WriteRTPPacket (p, out) < 0)
  {
    printf ("Cannot write %d bytes of RTP packet to outfile, exit\n", p->packlen);
    exit (-1);
  }
  return (p->packlen);

}


static int oldtr = -1;

void RTPUpdateTimestamp (int tr)
{
  int delta;

  if (oldtr == -1)            // First invocation
  {
    CurrentRTPTimestamp = 0;  //! This is a violation of the security req. of
                              //! RTP (random timestamp), but easier to debug
    oldtr = 0;
    return;
  }

  /*! The following code assumes a wrap around of TR at 256, and
      needs to be changed as soon as this is no more true.
      
      The support for B frames is a bit tricky, because it is not easy to distinguish
      between a natural wrap-around of the tr, and the intentional going back of the
      tr because of a B frame.  It is solved here by a heuristic means: It is assumed that
      B frames are never "older" than 10 tr ticks.  Everything higher than 10 is considered
      a wrap around.
      I'm a bit at loss whether this is a fundamental problem of B frames.  Does a decoder
      have to change its TR interpretation depenent  on the picture type?  Horrible -- we
      would have a fundamental problem when mixing slices.  This needs careful review.
  */

  delta = tr - oldtr;

  if (delta < -10)        // wrap-around
    delta+=256;

  CurrentRTPTimestamp += delta * RTP_TR_TIMESTAMP_MULT;
  oldtr = tr;
}

/*!
 *****************************************************************************
 *
 * \brief 
 *    int RTPPartition_BC_Header () write the Partition type B, C header
 *
 * \return
 *    Number of bits used by the partition header
 *
 * \para Parameters
 *    PartNo: Partition Number to which the header should be written
 *
 * \para Side effects
 *    Partition header as per VCEG-N72r2 is written into the appropriate 
 *    partition bit buffer
 *
 * \para Limitations/Shortcomings/Tweaks
 *    The current code does not support the change of picture parameters within
 *    one coded sequence, hence there is only one parameter set necessary.  This
 *    is hard coded to zero.
 *   
 * \date
 *    October 24, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/


int RTPPartition_BC_Header(int PartNo)
{
  DataPartition *partition = &((img->currentSlice)->partArr[PartNo]);
  SyntaxElement sym;

  int len = 0;

  assert (input->of_mode == PAR_OF_RTP);
  assert (PartNo > 0 && PartNo < img->currentSlice->max_part_nr);

  sym.type = SE_HEADER;         // This will be true for all symbols generated here
  sym.mapping = n_linfo2;       // Mapping rule: Simple code number to len/info


  SYMTRACESTRING("RTP-PH: Picture ID");
  sym.value1 = img->currentSlice->picture_id;
  len += writeSyntaxElement_UVLC (&sym, partition);

  SYMTRACESTRING("RTP-PH: Slice ID");
  sym.value1 = img->current_slice_nr;
  len += writeSyntaxElement_UVLC (&sym, partition);

  return len;
}

/*!
 *****************************************************************************
 * \isAggregationPacket
 * \brief 
 *    Determine if current packet is normal packet or compound packet (aggregation
 *    packet)
 *
 * \return
 *    return TRUE, if it is compound packet.
 *    return FALSE, otherwise.
 *   
 * \date
 *    September 10, 2002
 *
 * \author
 *    Dong Tian   tian@cs.tut.fi
 *****************************************************************************/
Boolean isAggregationPacket()
{
  if (HaveAggregationSEI())
  {
    return TRUE;
  }
  // Until Sept 2002, the JM will produce aggregation packet only for some SEI messages

  return FALSE;
}

/*!
 *****************************************************************************
 * \PrepareAggregationSEIMessage
 * \brief 
 *    Prepare the aggregation sei message.
 *    
 * \date
 *    September 10, 2002
 *
 * \author
 *    Dong Tian   tian@cs.tut.fi
 *****************************************************************************/
void PrepareAggregationSEIMessage()
{
  Boolean has_aggregation_sei_message = FALSE;
  // prepare the sei message here
  // write the spare picture sei payload to the aggregation sei message
  if (seiHasSparePicture && img->type != B_IMG)
  {
    FinalizeSpareMBMap();
    assert(seiSparePicturePayload.data->byte_pos == seiSparePicturePayload.payloadSize);
    write_sei_message(AGGREGATION_SEI, seiSparePicturePayload.data->streamBuffer, seiSparePicturePayload.payloadSize, SEI_SPARE_PICTURE);
    has_aggregation_sei_message = TRUE;
  }
  // write the sub sequence information sei paylaod to the aggregation sei message
  if (seiHasSubseqInfo)
  {
    FinalizeSubseqInfo(img->layer);
    write_sei_message(AGGREGATION_SEI, seiSubseqInfo[img->layer].data->streamBuffer, seiSubseqInfo[img->layer].payloadSize, SEI_SUBSEQ_INFORMATION);
    ClearSubseqInfoPayload(img->layer);
    has_aggregation_sei_message = TRUE;
  }
  // write the sub sequence layer information sei paylaod to the aggregation sei message
  if (seiHasSubseqLayerInfo && img->number == 0)
  {
    FinalizeSubseqLayerInfo();
    write_sei_message(AGGREGATION_SEI, seiSubseqLayerInfo.data, seiSubseqLayerInfo.payloadSize, SEI_SUBSEQ_LAYER_CHARACTERISTICS);
    seiHasSubseqLayerInfo = FALSE;
    has_aggregation_sei_message = TRUE;
  }
  // write the sub sequence characteristics payload to the aggregation sei message
  if (seiHasSubseqChar)
  {
    FinalizeSubseqChar();
    write_sei_message(AGGREGATION_SEI, seiSubseqChar.data->streamBuffer, seiSubseqChar.payloadSize, SEI_SUBSEQ_CHARACTERISTICS);
    ClearSubseqCharPayload();
    has_aggregation_sei_message = TRUE;
  }
  // write the pan scan rectangle info sei playload to the aggregation sei message
  if (seiHasPanScanRectInfo)
  {
    FinalizePanScanRectInfo();
    write_sei_message(AGGREGATION_SEI, seiPanScanRectInfo.data->streamBuffer, seiPanScanRectInfo.payloadSize, SEI_PANSCAN_RECT);
    ClearPanScanRectInfoPayload();
    has_aggregation_sei_message = TRUE;
  }
  // write the arbitrary (unregistered) info sei playload to the aggregation sei message
  if (seiHasUser_data_unregistered_info)
  {
    FinalizeUser_data_unregistered();
    write_sei_message(AGGREGATION_SEI, seiUser_data_unregistered.data->streamBuffer, seiUser_data_unregistered.payloadSize, SEI_USER_DATA_UNREGISTERED);
    ClearUser_data_unregistered();
    has_aggregation_sei_message = TRUE;
  }
  // write the arbitrary (unregistered) info sei playload to the aggregation sei message
  if (seiHasUser_data_registered_itu_t_t35_info)
  {
    FinalizeUser_data_registered_itu_t_t35();
    write_sei_message(AGGREGATION_SEI, seiUser_data_registered_itu_t_t35.data->streamBuffer, seiUser_data_registered_itu_t_t35.payloadSize, SEI_USER_DATA_REGISTERED_ITU_T_T35);
    ClearUser_data_registered_itu_t_t35();
    has_aggregation_sei_message = TRUE;
  }
  //write RandomAccess info sei payload to the aggregation sei message
  if (seiHasRandomAccess_info)
  {
    FinalizeRandomAccess();
    write_sei_message(AGGREGATION_SEI, seiRandomAccess.data->streamBuffer, seiRandomAccess.payloadSize, SEI_RANDOM_ACCESS_POINT);
    ClearRandomAccess();
    has_aggregation_sei_message = TRUE;
  }
  // more aggregation sei payload is written here...

  // JVT-D099 write the scene information SEI payload
  if (seiHasSceneInformation)
  {
    FinalizeSceneInformation();
    write_sei_message(AGGREGATION_SEI, seiSceneInformation.data->streamBuffer, seiSceneInformation.payloadSize, SEI_SCENE_INFORMATION);
    has_aggregation_sei_message = TRUE;
  }
  // End JVT-D099

  // after all the sei payload is written
  if (has_aggregation_sei_message)
    finalize_sei_message(AGGREGATION_SEI);
}

/*!
 *****************************************************************************
 * \begin_sub_sequence_rtp
 * \brief 
 *    do some initialization for sub-sequence under rtp
 *    
 * \date
 *    September 10, 2002
 *
 * \author
 *    Dong Tian   tian@cs.tut.fi
 *****************************************************************************/

void begin_sub_sequence_rtp()
{
  if ( input->of_mode != PAR_OF_RTP || input->NumFramesInELSubSeq == 0 ) 
    return;

  // begin to encode the base layer subseq
  if ( IMG_NUMBER == 0 )
  {
//    printf("begin to encode the base layer subseq\n");
    InitSubseqInfo(0);
    if (1)
      UpdateSubseqChar();
  }
  // begin to encode the enhanced layer subseq
  if ( IMG_NUMBER % (input->NumFramesInELSubSeq+1) == 1 )
  {
//    printf("begin to encode the enhanced layer subseq\n");
    InitSubseqInfo(1);  // init the sub-sequence in the enhanced layer
//    add_dependent_subseq(1);
    if (1)
      UpdateSubseqChar();
  }
}

/*!
 *****************************************************************************
 * \end_sub_sequence_rtp
 * \brief 
 *    do nothing
 *    
 * \date
 *    September 10, 2002
 *
 * \author
 *    Dong Tian   tian@cs.tut.fi
 *****************************************************************************/
void end_sub_sequence_rtp()
{
  // end of the base layer:
  if ( img->number == input->no_frames-1 )
  {
//    printf("end of encoding the base layer subseq\n");
    CloseSubseqInfo(0);
//    updateSubSequenceBox(0);
  }
  // end of the enhanced layer:
  if ( ((IMG_NUMBER%(input->NumFramesInELSubSeq+1)==0) && (input->successive_Bframe != 0) && (IMG_NUMBER>0)) || // there are B frames
    ((IMG_NUMBER%(input->NumFramesInELSubSeq+1)==input->NumFramesInELSubSeq) && (input->successive_Bframe==0))   // there are no B frames
    )
  {
//    printf("end of encoding the enhanced layer subseq\n");
    CloseSubseqInfo(1);
//    add_dependent_subseq(1);
//    updateSubSequenceBox(1);
  }
}
