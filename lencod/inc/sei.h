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
 *     sei.h
 *  \brief
 *     definitions for Supplemental Enhanced Information
 *  \author(s)
 *      - Dong Tian                             <tian@cs.tut.fi>
 *      - TBD
 *
 * ************************************************************************
 */

#ifndef SEI_H
#define SEI_H

//! definition of SEI payload type
typedef enum {
  SEI_ZERO,        //!< 0 is undefined, useless
  SEI_TEMPORAL_REF,
  SEI_CLOCK_TIMESTAMP,
  SEI_PANSCAN_RECT,
  SEI_BUFFERING_PERIOD,
  SEI_HRD_PICTURE,
  SEI_FILLER_PAYLOAD,
  SEI_USER_DATA_REGISTERED_ITU_T_T35,
  SEI_USER_DATA_UNREGISTERED,
  SEI_RANDOM_ACCESS_POINT,
  SEI_REF_PIC_BUFFER_MANAGEMENT_REPETITION,
  SEI_SPARE_PICTURE,
  SEI_SCENE_INFORMATION,
  SEI_SUBSEQ_INFORMATION,
  SEI_SUBSEQ_LAYER_CHARACTERISTICS,
  SEI_SUBSEQ_CHARACTERISTICS,
  SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
} SEI_type;

#define MAX_FN 256

#define AGGREGATION_PACKET_TYPE 4
#define SEI_PACKET_TYPE 5  // Tian Dong: See VCEG-N72, it need updates

#define NORMAL_SEI 0
#define AGGREGATION_SEI 1

//! SEI structure
typedef struct
{
  Boolean available;
  int payloadSize;
  unsigned char subPacketType;
  byte* data;
} sei_struct;

//!< sei_message[0]: this struct is to store the sei message packtized independently 
//!< sei_message[1]: this struct is to store the sei message packtized together with slice data
extern sei_struct sei_message[2];

void InitSEIMessages();
void CloseSEIMessages();
Boolean HaveAggregationSEI();
void write_sei_message(int id, byte* payload, int payload_size, int payload_type);
void finalize_sei_message(int id);
void clear_sei_message(int id);
void AppendTmpbits2Buf( Bitstream* dest, Bitstream* source );

void PrepareAggregationSEIMessage();


//! Spare Picture
typedef struct
{
  int target_frame_num;
  int num_spare_pics;
  int payloadSize;
  Bitstream* data;
} spare_picture_struct;

extern Boolean seiHasSparePicture;
//extern Boolean sei_has_sp;
extern spare_picture_struct seiSparePicturePayload;

void InitSparePicture();
void CloseSparePicture();
void CalculateSparePicture();
void ComposeSparePictureMessage(int delta_spare_frame_num, int ref_area_indicator, Bitstream *tmpBitstream);
Boolean CompressSpareMBMap(unsigned char **map_sp, Bitstream *bitstream);
void FinalizeSpareMBMap();

//! Subseq Information
typedef struct
{
  int subseq_layer_num;
  int subseq_id;
  unsigned int last_picture_flag;
  unsigned int stored_frame_cnt;

  int payloadSize;
  Bitstream* data;
} subseq_information_struct;

extern Boolean seiHasSubseqInfo;
extern subseq_information_struct seiSubseqInfo[MAX_LAYER_NUMBER];

void InitSubseqInfo(int currLayer);
void UpdateSubseqInfo(int currLayer);
void FinalizeSubseqInfo(int currLayer);
void ClearSubseqInfoPayload(int currLayer);
void CloseSubseqInfo(int currLayer);

//! Subseq Layer Information
typedef struct
{
  unsigned short bit_rate[MAX_LAYER_NUMBER];
  unsigned short frame_rate[MAX_LAYER_NUMBER];
  byte data[4*MAX_LAYER_NUMBER];
  int layer_number;
  int payloadSize;
} subseq_layer_information_struct;

extern Boolean seiHasSubseqLayerInfo;
extern subseq_layer_information_struct seiSubseqLayerInfo;

void InitSubseqLayerInfo();
void CloseSubseqLayerInfo();
void FinalizeSubseqLayerInfo();

//! Subseq Characteristics
typedef struct
{
  int subseq_layer_num;
  int subseq_id;
  int duration_flag;
  unsigned int subseq_duration;
  unsigned int average_rate_flag;
  unsigned int average_bit_rate;
  unsigned int average_frame_rate;
  int num_referenced_subseqs;
  int ref_subseq_layer_num[MAX_DEPENDENT_SUBSEQ];
  int ref_subseq_id[MAX_DEPENDENT_SUBSEQ];

  Bitstream* data;
  int payloadSize;
} subseq_char_information_struct;

extern Boolean seiHasSubseqChar;
extern subseq_char_information_struct seiSubseqChar;

void InitSubseqChar();
void ClearSubseqCharPayload();
void UpdateSubseqChar();
void FinalizeSubseqChar();
void CloseSubseqChar();

#endif