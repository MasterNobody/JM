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
 **************************************************************************
 * \file defines.h
 *
 * \brief
 *    Headerfile containing some useful global definitions
 *
 * \author
 *    Detlev Marpe                                                        \n
 *    Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *
 * \date
 *    21. March 2001
 **************************************************************************
 */

#ifndef _DEFINES_H_
#define _DEFINES_H_


#define _EXP_GOLOMB

#define _ERROR_CONCEALMENT_   1   //!< 0: off; 1: on
#define _ADAPT_LAST_GROUP_

#define MAX_INFO_WORD  300000               //!< for one frame
#define MAX_CODED_FRAME_SIZE 400000         //!< bytes for one frame
#define MAXIMUM_UVLC_CODEWORD_PER_HEADER 20 //!< UVLC codewords per combined picture/slice header maximum
#define TRACE           0                   //!< 0:Trace off 1:Trace on
#define _LEAKYBUCKET_

#define absm(A) ((A)<(0) ? (-(A)):(A))      //!< abs macro, faster than procedure
#define MAX_VALUE       999999              //!< used for start value for some variables


#define P8x8    8
#define I4MB    9
#define I16MB   10
#define IBLOCK  11
#define MAXMODE 12


#define IS_INTRA(MB)    ((MB)->mb_type==I4MB  || (MB)->mb_type==I16MB)
#define IS_NEWINTRA(MB) ((MB)->mb_type==I16MB)
#define IS_OLDINTRA(MB) ((MB)->mb_type==I4MB)
#define IS_INTER(MB)    ((MB)->mb_type!=I4MB  && (MB)->mb_type!=I16MB)
#define IS_INTERMV(MB)  ((MB)->mb_type!=I4MB  && (MB)->mb_type!=I16MB       && (MB)->mb_type!=0)
#define IS_DIRECT(MB)   ((MB)->mb_type==0     && (img->   type==    B_IMG_1 ||  img->   type==    B_IMG_MULT))
#define IS_COPY(MB)     ((MB)->mb_type==0     && (img->   type==INTER_IMG_1 ||  img->   type==INTER_IMG_MULT || img->type==SP_IMG_1 || img->type==SP_IMG_MULT))
#define IS_P8x8(MB)     ((MB)->mb_type==P8x8)


// Quantization parameter range
#define MIN_QP          -8
#define MAX_QP          39

#define INTER_IMG_1     0
#define INTER_IMG_MULT  1
#define INTRA_IMG       2

// B pictures
#define B_IMG_1   3
#define B_IMG_MULT  4

// SP Pictures
#define SP_IMG_1  5
#define SP_IMG_MULT 6


#define BLOCK_SIZE      4
#define MB_BLOCK_SIZE   16

#define NO_INTRA_PMODE  6        //!< #intra prediction modes
// 4x4 intra prediction modes
#define DC_PRED         0
#define DIAG_PRED_RL    1
#define VERT_PRED       2
#define DIAG_PRED_LR_45 3
#define HOR_PRED        4
#define DIAG_PRED_LR    5

// 16x16 intra prediction modes
#define VERT_PRED_16    0
#define HOR_PRED_16     1
#define DC_PRED_16      2
#define PLANE_16        3

// QCIF format
#define IMG_WIDTH       176
#define IMG_HEIGHT      144
#define IMG_WIDTH_CR    88
#define IMG_HEIGHT_CR   72

#define INIT_FRAME_RATE 30
#define LEN_STARTCODE   31                      //!< length of start code
#define EOS             1                       //!< End Of Sequence
#define SOP             2                       //!< Start Of Picture
#define SOS             3                       //!< Start Of Slice

#define EOS_MASK        0x01                    //!< mask for end of sequence (bit 1)
#define ICIF_MASK       0x02                    //!< mask for image format (bit 2)
#define QP_MASK         0x7C                    //!< mask for quant.parameter (bit 3->7)
#define TR_MASK         0x7f80                  //!< mask for temporal referance (bit 8->15)

#define DECODING_OK     0
#define SEARCH_SYNC     1
#define PICTURE_DECODED 2

#define MIN_PIX_VAL     0                       //!< lowest legal values for 8 bit sample
#define MAX_PIX_VAL     255                     //!< highest legal values for 8 bit sample

#ifndef WIN32
#define max(a, b)      ((a) > (b) ? (a) : (b))  //!< Macro returning max value
#define min(a, b)      ((a) < (b) ? (a) : (b))  //!< Macro returning min value
#endif
#define mmax(a, b)      ((a) > (b) ? (a) : (b)) //!< Macro returning max value
#define mmin(a, b)      ((a) < (b) ? (a) : (b)) //!< Macro returning min value


#define MVPRED_MEDIAN   0
#define MVPRED_L        1
#define MVPRED_U        2
#define MVPRED_UR       3

#define DECODE_COPY_MB  0
#define DECODE_MB       1
//#define DECODE_MB_BFRAME 2

#define BLOCK_MULTIPLE      (MB_BLOCK_SIZE/BLOCK_SIZE)

#define MAX_SYMBOLS_PER_MB  600  //!< Maximum number of different syntax elements for one MB

#define MAX_PART_NR     3        /*!< Maximum number of different data partitions.
                                      Some reasonable number which should reflect
                                      what is currently defined in the SE2Partition
                                      map (elements.h) */

// Interim File Format: define the following macro to identify which version is 
//                      used in the implementation

#define WORKING_DRAFT_MAJOR_NO 0    // inidicate the working draft version number
#define WORKING_DRAFT_MINOR_NO 4
#define INTERIM_FILE_MAJOR_NO 0     // indicate interim file format version number
#define INTERIM_FILE_MINOR_NO 0

#endif
