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
 *     global.h
 *  \brief
 *     global definitions for for H.26L decoder.
 *  \author
 *     Copyright (C) 1999  Telenor Satellite Services,Norway
 *                         Ericsson Radio Systems, Sweden
 *
 *     Inge Lille-Lang�y               <inge.lille-langoy@telenor.com>
 *
 *     Telenor Satellite Services
 *     Keysers gt.13                       tel.:   +47 23 13 86 98
 *     N-0130 Oslo,Norway                  fax.:   +47 22 77 79 80
 *
 *     Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *
 *     Ericsson Radio Systems
 *     KI/ERA/T/VV
 *     164 80 Stockholm, Sweden
 *
 ************************************************************************
 */
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>                              //!< for FILE
#include "defines.h"

#ifdef WIN32
  #define  snprintf _snprintf
#endif


typedef unsigned char   byte;                   //!<  8 bit unsigned
typedef int             int32;
typedef unsigned int    u_int32;


// global picture format dependend buffers, mem allocation in decod.c ******************
int  **refFrArr;                                //!< Array for reference frames of each block
byte **imgY;                                    //!< array for the decoded luma component
byte **imgY_pf;                                 //!< Post filter luma image
byte ***imgUV;                                  //!< array for the chroma component
byte ***imgUV_pf;                               //!< Post filter luma image

int   **moving_block;           //<! stationary block buffer
int   **moving_block_frm;       //<! stationary block buffer - frame
int   **moving_block_top;       //<! stationary block buffer - field
int   **moving_block_bot;       //<! stationary block buffer - field


// B pictures
byte **imgY_prev;
byte ***imgUV_prev;

byte **imgY_ref;                                //!< reference frame find snr
byte ***imgUV_ref;

// B pictures
int  Bframe_ctr;
byte prevP_tr, nextP_tr, P_interval;
int  frame_no;
int  ***colB8mode;                              //!< ABT: Array containing the modes of the collocated 8x8 Blocks.

int  **refFrArr_frm;
int  **refFrArr_top;
int  **refFrArr_bot;
byte **imgY_frm;
byte **imgY_top;
byte **imgY_bot;
byte ***imgUV_frm;
byte ***imgUV_top;
byte ***imgUV_bot;

byte ***mref_frm;                               //!< 1/1 pix luma for direct interpolation
byte ****mcef_frm;                              //!< pix chroma

byte ***mref_fld;                               //!< 1/1 pix luma for direct interpolation
byte ****mcef_fld;     
byte nextP_tr_frm, nextP_tr_fld;
int  **field_mb;

// For MB level frame/field coding
int  TopFieldForSkip_Y[16][16];
int  TopFieldForSkip_UV[2][16][16];


#define ET_SIZE 300      //!< size of error text buffer
char errortext[ET_SIZE]; //!< buffer for error message for exit with error()

/***********************************************************************
 * T y p e    d e f i n i t i o n s    f o r    T M L
 ***********************************************************************
 */

//! Data Partitioning Modes
typedef enum
{
  PAR_DP_1,    //<! no data partitioning is supported
  PAR_DP_3,    //<! data partitioning with 3 partitions
} PAR_DP_TYPE;


//! Output File Types
typedef enum
{
  PAR_OF_26L,   //<! Current TML description
  PAR_OF_RTP,   //<! RTP Packet Output format
  PAR_OF_IFF    //<! Interim File Format
} PAR_OF_TYPE;

//! Boolean Type
typedef enum {
  FALSE,
  TRUE
} Boolean;

//! definition of H.26L syntax elements
typedef enum {
  SE_HEADER,
  SE_PTYPE,
  SE_MBTYPE,
  SE_REFFRAME,
  SE_INTRAPREDMODE,
  SE_MVD,
  SE_CBP_INTRA,
  SE_LUM_DC_INTRA,
  SE_CHR_DC_INTRA,
  SE_LUM_AC_INTRA,
  SE_CHR_AC_INTRA,
  SE_CBP_INTER,
  SE_LUM_DC_INTER,
  SE_CHR_DC_INTER,
  SE_LUM_AC_INTER,
  SE_CHR_AC_INTER,
  SE_DELTA_QUANT_INTER,
  SE_DELTA_QUANT_INTRA,
  SE_BFRAME,
  SE_EOS,
  SE_MAX_ELEMENTS //!< number of maximum syntax elements, this MUST be the last one!
} SE_type;        // substituting the definitions in element.h


typedef enum {
  INTER_MB,
  INTRA_MB_4x4,
  INTRA_MB_16x16
} IntraInterDecision;

typedef enum {
  BITS_TOTAL_MB,
  BITS_HEADER_MB,
  BITS_INTER_MB,
  BITS_CBP_MB,
  BITS_COEFF_Y_MB,
  BITS_COEFF_UV_MB,
  MAX_BITCOUNTER_MB
} BitCountType;

typedef enum {
  NO_SLICES,
  FIXED_MB,
  FIXED_RATE,
  CALLBACK,
  FMO
} SliceMode;


typedef enum {
  UVLC,
  CABAC
} SymbolMode;


typedef enum {        // ABT
  NO_ABT,
  INTER_ABT,
  INTER_INTRA_ABT
} ABTMode;            // ABT

typedef enum {        // ABT
  B8x8=0,
  B8x4,
  B4x8,
  B4x4
} ABTBlockMode;       // ABT

typedef enum {
  FRAME,
  TOP_FIELD,
  BOTTOM_FIELD
} PictureType;           //!< New enum for field processing

/***********************************************************************
 * D a t a    t y p e s   f o r  C A B A C
 ***********************************************************************
 */

//! struct to characterize the state of the arithmetic coding engine
typedef struct
{
  unsigned int    Dlow, Drange;
  unsigned int    Dvalue;
  unsigned int    Dbuffer;
  int             Dbits_to_go;
  byte            *Dcodestrm;
  int             *Dcodestrm_len;
} DecodingEnvironment;

typedef DecodingEnvironment *DecodingEnvironmentPtr;

//! struct for context management
typedef struct
{
  unsigned short state;         // index into state-table CP  
  unsigned char  MPS;           // Least Probable Symbol 0/1 CP
} BiContextType;

typedef BiContextType *BiContextTypePtr;


/**********************************************************************
 * C O N T E X T S   F O R   T M L   S Y N T A X   E L E M E N T S
 **********************************************************************
 */

#define NUM_ABT_MODE_CTX 3
#define NUM_MB_TYPE_CTX  11
#define NUM_B8_TYPE_CTX  9
#define NUM_MV_RES_CTX   10
#define NUM_REF_NO_CTX   6
#define NUM_DELTA_QP_CTX 4
#define NUM_MB_AFF_CTX 4


typedef struct
{
  BiContextType ABT_mode_contexts[2][NUM_ABT_MODE_CTX];
  BiContextType mb_type_contexts [4][NUM_MB_TYPE_CTX];
  BiContextType b8_type_contexts [2][NUM_B8_TYPE_CTX];
  BiContextType mv_res_contexts  [2][NUM_MV_RES_CTX];
  BiContextType ref_no_contexts  [2][NUM_REF_NO_CTX];
  BiContextType delta_qp_contexts[NUM_DELTA_QP_CTX];
  BiContextType mb_aff_contexts  [NUM_MB_AFF_CTX];
} MotionInfoContexts;

#define NUM_IPR_CTX    2
#define NUM_CIPR_CTX   4
#define NUM_CBP_CTX    4
#define NUM_BCBP_CTX   4
#define NUM_MAP_CTX   15
#define NUM_LAST_CTX  15
#define NUM_ONE_CTX    5
#define NUM_ABS_CTX    5


typedef struct
{
  BiContextType  ipr_contexts [NUM_IPR_CTX];
  BiContextType  cipr_contexts[NUM_CIPR_CTX]; 
  BiContextType  cbp_contexts [3][NUM_CBP_CTX];
  BiContextType  bcbp_contexts[NUM_BLOCK_TYPES][NUM_BCBP_CTX];
  BiContextType  map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
  BiContextType  last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
  BiContextType  one_contexts [NUM_BLOCK_TYPES][NUM_ONE_CTX];
  BiContextType  abs_contexts [NUM_BLOCK_TYPES][NUM_ABS_CTX];
} TextureInfoContexts;


//*********************** end of data type definition for CABAC *******************

/***********************************************************************
 * N e w   D a t a    t y p e s   f o r    T M L
 ***********************************************************************
 */

struct img_par;
struct inp_par;
struct stat_par;

/*! Buffer structure for RMPNI commands */
typedef struct RMPNIbuffer_s
{
  int RMPNI;
  int Data;
  struct RMPNIbuffer_s *Next;
} RMPNIbuffer_t;

/*! Buffer structure for MMCO commands */
typedef struct MMCObuffer_s
{
  int MMCO;
  int DPN;
  int LPIN;
  int MLIP1;
  struct MMCObuffer_s *Next;
} MMCObuffer_t;

//! Syntaxelement
typedef struct syntaxelement
{
  int           type;                  //!< type of syntax element for data part.
  int           value1;                //!< numerical value of syntax element
  int           value2;                //!< for blocked symbols, e.g. run/level
  int           len;                   //!< length of code
  int           inf;                   //!< info part of UVLC code
  unsigned int  bitpattern;            //!< UVLC bitpattern
  int           context;               //!< CABAC context
  int           k;                     //!< CABAC context for coeff_count,uv
  int           golomb_grad;           //needed if type is a golomb element (ABT)
  int           golomb_maxlevels;      // (ABT)
#if TRACE
  #define       TRACESTRING_SIZE 100           //!< size of trace string
  char          tracestring[TRACESTRING_SIZE]; //!< trace string
#endif

  //! for mapping of UVLC to syntaxElement
  void    (*mapping)(int len, int info, int *value1, int *value2);
  //! used for CABAC: refers to actual coding method of each individual syntax element type
  void  (*reading)(struct syntaxelement *, struct inp_par *, struct img_par *, DecodingEnvironmentPtr);

} SyntaxElement;

//! Macroblock
typedef struct macroblock
{
  int           qp;
  int           slice_nr;
  int           delta_quant;          //!< for rate control
  struct macroblock   *mb_available[3][3]; /*!< pointer to neighboring MBs in a 3x3 window of current MB, which is located at [1][1]
                                                NULL pointer identifies neighboring MBs which are unavailable */
  
  struct macroblock   *skip_mb_available[3][3]; /*!< pointer to neighboring MBs in a 3x3 window of current MB, which is located at [1][1]
                                                NULL pointer identifies neighboring MBs which are unavailable */

  struct macroblock   *field_available[2];

  // some storage of macroblock syntax elements for global access
  int           mb_type;
  int           mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];      //!< indices correspond to [forw,backw][block_y][block_x][x,y]
  int           cbp, cbp_blk ;
  unsigned long cbp_bits;

  int           i16mode;
  int           b8mode[4];
  int           b8pdir[4];
  int           ei_flag;

  int           useABT[4];          //!< ABT flag, indicating application of ABT for each 8x8 block (1=ABT)
  int           abt_mode[4];        //!< ABT transform mode used for ABT.
  int           abt_pred_mode[4];   //!< ABT mode used for ABT block prediction.

  int           bipred_weighting_type[4];  //!< bi-predictive weighting type, 0:average 1:extrapolation

  int           lf_disable;
  int           lf_alpha_c0_offset;
  int           lf_beta_offset;

  int           c_ipred_mode;       //!< chroma intra prediction mode
  int           mb_field;
} Macroblock;

//! Bitstream
typedef struct
{
  // CABAC Decoding
  int           read_len;           //!< actual position in the codebuffer, CABAC only
  int           code_len;           //!< overall codebuffer length, CABAC only
  // UVLC Decoding
  int           frame_bitoffset;    //!< actual position in the codebuffer, bit-oriented, UVLC only
  int           bitstream_length;   //!< over codebuffer lnegth, byte oriented, UVLC only
  // ErrorConcealment
  byte          *streamBuffer;      //!< actual codebuffer for read bytes
  int           ei_flag;            //!< error indication, 0: no error, else unspecified error
} Bitstream;

//! DataPartition
typedef struct datapartition
{

  Bitstream           *bitstream;
  DecodingEnvironment de_cabac;

  int     (*readSyntaxElement)(SyntaxElement *, struct img_par *, struct inp_par *, struct datapartition *);
          /*!< virtual function;
               actual method depends on chosen data partition and
               entropy coding method  */
} DataPartition;

//! Slice
typedef struct
{
  int                 ei_flag;       //!< 0 if the partArr[0] contains valid information
  int                 picture_id;    //!< MUST be set by NAL even in case ei_flag == 1
  int                 qp;
  int                 picture_type;  //!< picture type
  int                 structure;     //!< Identify picture structure type
  int                 start_mb_nr;   //!< MUST be set by NAL even in case of ei_flag == 1
  int                 max_part_nr;
  int                 dp_mode;       //!< data partioning mode
  int                 eos_flag;      //!< end of sequence flag
  int                 next_header;
  int                 next_eiflag;
  int                 last_mb_nr;    //!< only valid when entropy coding == CABAC
  DataPartition       *partArr;      //!< array of partitions
  MotionInfoContexts  *mot_ctx;      //!< pointer to struct of context models for use in CABAC
  TextureInfoContexts *tex_ctx;      //!< pointer to struct of context models for use in CABAC
  RMPNIbuffer_t        *rmpni_buffer; //!< stores the slice temporary buffer remapping commands
  int     (*readSlice)(struct img_par *, struct inp_par *);
  int                 abt;           //!< ABT Flag

  int                 LFDisable;        //!< Disable loop filter on slice
  int                 LFAlphaC0Offset;  //!< Alpha and C0 offset for filtering slice
  int                 LFBetaOffset;     //!< Beta offset for filtering slice

} Slice;

//****************************** ~DM ***********************************

// image parameters
typedef struct img_par
{
  int number;                                 //<! frame number
  int pn;                                     //<! short term picture number
  int current_mb_nr; // bitstream order
  int map_mb_nr;  //related to mb_data
  int max_mb_nr;
  int current_slice_nr;
  int **intra_block;
  int tr;                                     //<! temporal reference, 8 bit, wrapps at 255
  int tr_old;                                     //<! old temporal reference, for detection of a new frame, added by WYK
  int refPicID;                         //<! temporal reference for reference frames (non-B frames), 4 bit, wrapps at 15, added by WYK
  int refPicID_old;                  //<! to detect how many reference frames are lost, added by WYK
  int qp;                                     //<! quant for the current frame
  int qpsp;                                   //<! quant for SP-picture predicted frame
  int sp_switch;                              //<! 1 for switching sp, 0 for normal sp
  int direct_type;                          //<! 1 for Spatial Direct, 0 for Temporal
  int type;                                   //<! image type INTER/INTRA
  int width;
  int height;
  int width_cr;                               //<! width chroma
  int height_cr;                              //<! height chroma
  int mb_y;
  int mb_x;
  int block_y;
  int pix_y;
  int pix_x;
  int pix_c_y;
  int block_x;
  int pix_c_x;

  int allrefzero;
  int ***mv;                                  //<! [92][72][3]
  int mpr[16][16];                            //<! predicted block

  int m7[16][16];                             //<! final 4x4 block. Extended to 16x16 for ABT
  int cof[4][6][4][4];                        //<! correction coefficients from predicted
  int cofu[4];
  int **ipredmode;                            //<! prediction type [90][74]
  int quad[256];
  int UseConstrainedIntraPred;
  int ****nz_coeff;
  int **siblock;
  int cod_counter;                            //<! Current count of number of skipped macroblocks in a row

  int ***dfMV;                                //<! [92][72][3];
  int ***dbMV;                                //<! [92][72][3];
  int **fw_refFrArr;                          //<! [72][88];
  int **bw_refFrArr;                          //<! [72][88];
 

  int ***mv_top;
  int ***mv_bot;
  int ***mv_frm;
  int **fw_refFrArr_frm;                          //<! [72][88];
  int **bw_refFrArr_frm;                          //<! [72][88];
  int **fw_refFrArr_top;                          //<! [72][88];
  int **bw_refFrArr_top;                          //<! [72][88];
  int **fw_refFrArr_bot;                          //<! [72][88];
  int **bw_refFrArr_bot;                          //<! [72][88];


  int structure;                               //<! Identify picture structure type
  int structure_old;                           //<! temp fix for multi slice per picture
  int pstruct_next_P;
  int imgtr_next_P;
  int imgtr_last_P;
  int tr_frm;
  int tr_fld;

  // B pictures
  int ***fw_mv;                                //<! [92][72][3];
  int ***bw_mv;                                //<! [92][72][3];
  Slice       *currentSlice;                   //<! pointer to current Slice data struct
  Macroblock          *mb_data;                //<! array containing all MBs of a whole frame
  int subblock_x;
  int subblock_y;
  int is_intra_block;
  int is_v_block;

  int buf_cycle;

  // For MB level frame/field coding
  int mb_frame_field_flag;
  int mb_field;
  int **ipredmode_frm;
  int **ipredmode_top;
  int **ipredmode_bot;
  int ***fw_mv_frm;
  int ***fw_mv_top;
  int ***fw_mv_bot;
  int ***bw_mv_frm;
  int ***bw_mv_top;
  int ***bw_mv_bot;
  int ***dfMV_top;                                //<! [92][72][3];
  int ***dbMV_top;                                //<! [92][72][3];
  int ***dfMV_bot;                                //<! [92][72][3];
  int ***dbMV_bot;                                //<! [92][72][3];
  int **field_anchor;

  MMCObuffer_t *mmco_buffer;                    //<! stores the memory management control operations

  int disposable_flag;                          //!< flag for disposable frame, 1:disposable
  int num_ref_pic_active_fwd;                   //!< number of forward reference
  int num_ref_pic_active_bwd;                   //!< number of backward reference
  int explicit_B_prediction;                    //!< type of weight for bi-pred, 0:average 1:implicit 

  // JVT-D095, JVT-D097
  int num_slice_groups_minus1; 
  int mb_allocation_map_type; 
  int top_left_mb; 
  int bottom_right_mb; 
  int slice_group_change_direction; 
  int slice_group_change_rate_minus1; 
  int slice_group_change_cycle;
  // End JVT-D095, JVT-D097

  // JVT-D101
  int redundant_slice_flag; 
  int redundant_pic_cnt; 
  int last_decoded_pic_id; 
  // End JVT-D101

} ImageParameters;

// signal to noice ratio parameters
struct snr_par
{
  float snr_y;                                 //<! current Y SNR
  float snr_u;                                 //<! current U SNR
  float snr_v;                                 //<! current V SNR
  float snr_y1;                                //<! SNR Y(dB) first frame
  float snr_u1;                                //<! SNR U(dB) first frame
  float snr_v1;                                //<! SNR V(dB) first frame
  float snr_ya;                                //<! Average SNR Y(dB) remaining frames
  float snr_ua;                                //<! Average SNR U(dB) remaining frames
  float snr_va;                                //<! Average SNR V(dB) remaining frames
};

int tot_time;

// input parameters from configuration file
struct inp_par
{
  char infile[100];                       //<! Telenor H.26L input
  char outfile[100];                      //<! Decoded YUV 4:2:0 output
  char reffile[100];                      //<! Optional YUV 4:2:0 reference file for SNR measurement
  int  postfilter;                        //<! postfilter (0=OFF,1=ON)
  int symbol_mode;                        //<! Specifies the mode the symbols are mapped on bits
  int UseConstrainedIntraPred;            //<! 0: Inter MB pixels are allowed for intra prediction 1: Not allowed
  int of_mode;                            //<! Specifies the mode of the output file
  int partition_mode;                     //<! Specifies the mode of data partitioning
  int buf_cycle;                          //<! Frame buffer size
#ifdef _LEAKYBUCKET_
  unsigned long R_decoder;                //<! Decoder Rate in HRD Model
  unsigned long B_decoder;                //<! Decoder Buffer size in HRD model
  unsigned long F_decoder;                //<! Decoder Inital buffer fullness in HRD model
  char LeakyBucketParamFile[100];         //<! LeakyBucketParamFile
#endif
  int  abt;                               //<! Use Adaptive Block Transforms (ABT) 0:Off, 1: Inter ABT, 2: Inter & Intra ABT
  int LFParametersFlag;                   //<! Specifies that loop filter parameters are included in bitstream

};


// files
FILE *p_out;                    //<! pointer to output YUV file
FILE *p_ref;                    //<! pointer to input original reference YUV file file
FILE *p_log;                    //<! SNR file
FILE *p_datpart;                //<! file to write bitlength and id of all partitions

#if TRACE
FILE *p_trace;
#endif

int USEABT; // Global Flag indicating ABT mode

// prototypes
void init_conf(struct inp_par *inp, char *config_filename);
void report(struct inp_par *inp, struct img_par *img, struct snr_par *snr);
void find_snr(struct snr_par *snr,struct img_par *img, FILE *p_ref, int postfilter);
void init(struct img_par *img);

void malloc_slice(struct inp_par *inp, struct img_par *img);
void free_slice(struct inp_par *inp, struct img_par *img);

int  decode_one_frame(struct img_par *img,struct inp_par *inp, struct snr_par *snr);
void init_frame(struct img_par *img, struct inp_par *inp);
void exit_frame(struct img_par *img, struct inp_par *inp);
void DeblockFrame(struct img_par *img, byte **imgY, byte ***imgUV ) ;

void write_frame(struct img_par *img,int,FILE *p_out);
void write_prev_Pframe(struct img_par *img,FILE *p_out);// B pictures
void copy_Pframe(struct img_par *img,int);// B pictures


int  read_new_slice(struct img_par *img, struct inp_par *inp);
void decode_one_slice(struct img_par *img,struct inp_par *inp);

void start_macroblock(struct img_par *img,struct inp_par *inp, int CurrentMBInScanOrder);
void init_macroblock_Bframe(struct img_par *img);// B pictures
int  read_one_macroblock(struct img_par *img,struct inp_par *inp);
void read_ipred_modes(struct img_par *img,struct inp_par *inp);
int  read_one_macroblock_Bframe(struct img_par *img,struct inp_par *inp);// B pictures
int  decode_one_macroblock(struct img_par *img,struct inp_par *inp);
int  decode_one_macroblock_Bframe(struct img_par *img);// B pictures
int  exit_macroblock(struct img_par *img,struct inp_par *inp, int eos_bit);

void readMotionInfoFromNAL (struct img_par *img,struct inp_par *inp);

void readMotionInfoFromNAL_Bframe(struct img_par *img,struct inp_par *inp);// B pictures
void readMotionInfoFromNAL_Pframe(struct img_par *img,struct inp_par *inp);
void readCBPandCoeffsFromNAL(struct img_par *img,struct inp_par *inp);

void copyblock_sp(struct img_par *img,int block_x,int block_y);
void itrans_sp_chroma(struct img_par *img,int ll);
void itrans(struct img_par *img,int ioff,int joff,int i0,int j0);
void itrans_sp(struct img_par *img,int ioff,int joff,int i0,int j0);
int  intrapred(struct img_par *img,int ioff,int joff,int i4,int j4);
void itrans_2(struct img_par *img);
int  intrapred_luma_2(struct img_par *img,int predmode);
int  sign(int a , int b);

// UVLC mapping
void linfo(int len, int info, int *value1, int *dummy);
void linfo_mvd(int len,int info, int *signed_mvd, int *dummy);
void linfo_cbp_intra(int len,int info,int *cbp, int *dummy);
void linfo_cbp_inter(int len,int info,int *cbp, int *dummy);
void linfo_dquant(int len,  int info, int *signed_dquant, int *dummy);
void linfo_levrun_inter(int len,int info,int *level,int *irun);
void linfo_levrun_intra(int len,int info,int *level,int *irun);
void linfo_levrun_c2x2(int len,int info,int *level,int *irun);

int  readSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dp);
int  readSliceUVLC(struct img_par *img, struct inp_par *inp);
int  readSyntaxElement_fixed(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP);
int  readSyntaxElement_Intra4x4PredictionMode(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dp);

// Direct interpolation
void get_block(int ref_frame,int x_pos, int y_pos, struct img_par *img, int block[BLOCK_SIZE][BLOCK_SIZE]);
void get_quarterpel_block(int ref_frame,int x_pos, int y_pos, struct img_par *img, int block[BLOCK_SIZE][BLOCK_SIZE]);

int  GetVLCSymbol (byte buffer[],int totbitoffset,int *info, int bytecount);
int  GetfixedSymbol (byte buffer[],int totbitoffset,int *info, int bytecount, int len);
// int   inter_intra(struct img_par *img);
int  GetVLCSymbol_IntraMode (byte buffer[],int totbitoffset,int *info, int bytecount);

int readSyntaxElement_FLC(SyntaxElement *sym, struct datapartition *dp);
int readSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *sym,  DataPartition *dP,
                                           char *type);
int readSyntaxElement_NumCoeffTrailingOnesChromaDC(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_Level_VLC0(SyntaxElement *sym, struct datapartition *dP);
int readSyntaxElement_Level_VLCN(SyntaxElement *sym, int vlc, struct datapartition *dP);
int readSyntaxElement_TotalZeros(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_TotalZerosChromaDC(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_Run(SyntaxElement *sym,  DataPartition *dP);
int GetBits (byte buffer[],int totbitoffset,int *info, int bytecount, 
             int numbits);
int ShowBits (byte buffer[],int totbitoffset,int bytecount, int numbits);


// SLICE function pointers
int  (*nal_startcode_follows) ();

// NAL functions TML/CABAC bitstream
int  uvlc_startcode_follows();
int  cabac_startcode_follows();
void free_Partition(Bitstream *currStream);

// ErrorConcealment
void reset_ec_flags();

// CABAC
void arideco_start_decoding(DecodingEnvironmentPtr eep, unsigned char *code_buffer, int firstbyte, int *code_len, int slice_type);
int  arideco_bits_read(DecodingEnvironmentPtr dep);
void arideco_done_decoding(DecodingEnvironmentPtr dep);
void biari_init_context (struct img_par *img, BiContextTypePtr ctx, const int* ini);
void rescale_cum_freq(BiContextTypePtr bi_ct);
unsigned int biari_decode_symbol(DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct );
unsigned int biari_decode_symbol_eq_prob(DecodingEnvironmentPtr dep);
unsigned int biari_decode_final(DecodingEnvironmentPtr dep);
MotionInfoContexts* create_contexts_MotionInfo(void);
TextureInfoContexts* create_contexts_TextureInfo(void);
void init_contexts_MotionInfo(struct img_par *img, MotionInfoContexts *enco_ctx);
void init_contexts_TextureInfo(struct img_par *img, TextureInfoContexts *enco_ctx);
void delete_contexts_MotionInfo(MotionInfoContexts *enco_ctx);
void delete_contexts_TextureInfo(TextureInfoContexts *enco_ctx);


void readABTIntraBlkModeInfo2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readMB_typeInfoFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readB8_typeInfoFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readIntraPredModeFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp,struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readRefFrameFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readBwdRefFrameFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readMVDFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readCBPFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readRunLevelFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img,  DecodingEnvironmentPtr dep_dp);

void readMVD2Buffer_CABAC(SyntaxElement *se,  struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readBiMVD2Buffer_CABAC(SyntaxElement *se,struct inp_par *inp,struct img_par *img,DecodingEnvironmentPtr dep_dp);
void readBiDirBlkSize2Buffer_CABAC(SyntaxElement *se,struct inp_par *inp,struct img_par *img,DecodingEnvironmentPtr dep_dp);

int  readSliceCABAC(struct img_par *img, struct inp_par *inp);
int  readSyntaxElement_CABAC(SyntaxElement *se, struct img_par *img, struct inp_par *inp, DataPartition *this_dataPart);
void readDquant_FromBuffer_CABAC(SyntaxElement *se,struct inp_par *inp,struct img_par *img,DecodingEnvironmentPtr dep_dp);
void readCIPredMode_FromBuffer_CABAC(SyntaxElement *se,struct inp_par *inp,struct img_par *img,DecodingEnvironmentPtr dep_dp);

void readMB_skip_flagInfoFromBuffer_CABAC( SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void readFieldModeInfoFromBuffer_CABAC(SyntaxElement *se,struct inp_par *inp,struct img_par *img,DecodingEnvironmentPtr dep_dp); 
void setRealMB_nr (struct img_par *img); 
int  check_next_mb_and_get_field_mode_CABAC(SyntaxElement *se,struct img_par *img,struct inp_par *inp,DataPartition  *act_dp);
void CheckAvailabilityOfNeighborsForSkip(struct img_par *img); 
void CheckAvailabilityOfNeighborsForAff(struct img_par *img); 


void error(char *text, int code);
void start_slice(struct img_par *img, struct inp_par *inp);
int  terminate_slice(struct img_par *img, struct inp_par *inp, struct stat_par  *stat);

// dynamic mem allocation
int  init_global_buffers(struct inp_par *inp, struct img_par *img);
void free_global_buffers(struct inp_par *inp, struct img_par *img);

void split_field_top(struct img_par *img);
void split_field_bot(struct img_par *img);
void combine_field(struct img_par *img);
void frame_postprocessing(struct img_par *img, struct inp_par *inp);
void field_postprocessing(struct img_par *img, struct inp_par *inp);
int  bottom_field_picture(struct img_par *img,struct inp_par *inp);
void init_top(struct img_par *img, struct inp_par *inp);
void init_bottom(struct img_par *img, struct inp_par *inp);
void decode_frame_slice(struct img_par *img,struct inp_par *inp, int current_header);
void decode_field_slice(struct img_par *img,struct inp_par *inp, int current_header);
void store_field_MV(struct img_par *img);
void store_direct_moving_flag(struct img_par *img);
void store_field_colB8mode(struct img_par *img);

#define PAYLOAD_TYPE_IDERP 8
int RBSPtoSODB(byte *streamBuffer, int last_byte_pos);
int EBSPtoRBSP(byte *streamBuffer, int end_bytepos, int begin_bytepos);

// For MB level frame/field coding
void init_super_macroblock(struct img_par *img,struct inp_par *inp);
void exit_super_macroblock(struct img_par *img,struct inp_par *inp);
int  decode_super_macroblock(struct img_par *img,struct inp_par *inp);
void decode_one_Copy_topMB(struct img_par *img,struct inp_par *inp);
void copy_stored_B_motion_info(struct img_par *img);

void SetOneRefMV(struct img_par* img);
int peekSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP);


#endif
