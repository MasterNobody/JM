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
#include "parsetcommon.h"

#ifdef WIN32
  #define  snprintf _snprintf
#endif


typedef unsigned char   byte;                   //!<  8 bit unsigned
typedef int             int32;
typedef unsigned int    u_int32;

unsigned int toprefpoc[MAX_REFERENCE_PICTURES];
unsigned int bottomrefpoc[MAX_REFERENCE_PICTURES];

pic_parameter_set_rbsp_t *active_pps;
seq_parameter_set_rbsp_t *active_sps;

// global picture format dependend buffers, mem allocation in decod.c ******************
int  **refFrArr;                                //!< Array for reference frames of each block

int   **moving_block;           //<! stationary block buffer
int   **moving_block_frm;       //<! stationary block buffer - frame
int   **moving_block_top;       //<! stationary block buffer - field
int   **moving_block_bot;       //<! stationary block buffer - field


byte **imgY_ref;                                //!< reference frame find snr
byte ***imgUV_ref;

int  ReMapRef[20];
// B pictures
int  Bframe_ctr;
byte prevP_tr, nextP_tr, P_interval;
int  frame_no;

int  **refFrArr_frm;
int  **refFrArr_top;
int  **refFrArr_bot;

byte ***mref_frm;                               //!< 1/1 pix luma for direct interpolation
byte ****mcef_frm;                              //!< pix chroma

byte ***mref_fld;                               //!< 1/1 pix luma for direct interpolation
byte ****mcef_fld;     
byte nextP_tr_frm, nextP_tr_fld;

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
  PAR_OF_ANNEXB,   //<! Current TML description
  PAR_OF_RTP,   //<! RTP Packet Output format
//  PAR_OF_IFF    //<! Interim File Format
} PAR_OF_TYPE;

//! Boolean Type
/*typedef enum {
  FALSE,
  TRUE
} Boolean;
*/
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


typedef enum {
  FRAME,
  TOP_FIELD,
  BOTTOM_FIELD
} PictureStructure;           //!< New enum for field processing


typedef enum {
  P_SLICE = 0,
  B_SLICE,
  I_SLICE,
  SP_SLICE,
  SI_SLICE
} SliceType;

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

#define NUM_MB_TYPE_CTX  11
#define NUM_B8_TYPE_CTX  9
#define NUM_MV_RES_CTX   10
#define NUM_REF_NO_CTX   6
#define NUM_DELTA_QP_CTX 4
#define NUM_MB_AFF_CTX 4


typedef struct
{
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
  BiContextType  fld_map_contexts [NUM_BLOCK_TYPES][NUM_MAP_CTX];
  BiContextType  fld_last_contexts[NUM_BLOCK_TYPES][NUM_LAST_CTX];
} TextureInfoContexts;


//*********************** end of data type definition for CABAC *******************

/***********************************************************************
 * N e w   D a t a    t y p e s   f o r    T M L
 ***********************************************************************
 */

struct img_par;
struct inp_par;
struct stat_par;

/*! Buffer structure for decoded referenc picture marking commands */
typedef struct DecRefPicMarking_s
{
  int memory_management_control_operation;
  int difference_of_pic_nums_minus1;
  int long_term_pic_num;
  int long_term_frame_idx;
  int max_long_term_frame_idx_plus1;
  struct DecRefPicMarking_s *Next;
} DecRefPicMarking_t;

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
  
  struct macroblock   *mb_available_up;   //!< pointer to neighboring MB (CABAC)
  struct macroblock   *mb_available_left; //!< pointer to neighboring MB (CABAC)

  // some storage of macroblock syntax elements for global access
  int           mb_type;
  int           mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];      //!< indices correspond to [forw,backw][block_y][block_x][x,y]
  int           cbp, cbp_blk ;
  unsigned long cbp_bits;

  int           i16mode;
  int           b8mode[4];
  int           b8pdir[4];
  int           ei_flag;

  int           LFDisableIdc;
  int           LFAlphaC0Offset;
  int           LFBetaOffset;

  int           c_ipred_mode;       //!< chroma intra prediction mode
  int           mb_field;

  int mbAddrA, mbAddrB, mbAddrC, mbAddrD;
  int mbAvailA, mbAvailB, mbAvailC, mbAvailD;

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
  PictureStructure    structure;     //!< Identify picture structure type
  int                 start_mb_nr;   //!< MUST be set by NAL even in case of ei_flag == 1
  int                 max_part_nr;
  int                 dp_mode;       //!< data partioning mode
  int                 next_header;
//  int                 last_mb_nr;    //!< only valid when entropy coding == CABAC
  DataPartition       *partArr;      //!< array of partitions
  MotionInfoContexts  *mot_ctx;      //!< pointer to struct of context models for use in CABAC
  TextureInfoContexts *tex_ctx;      //!< pointer to struct of context models for use in CABAC
  
  int                 ref_pic_list_reordering_flag_l0;
  int                 *remapping_of_pic_nums_idc_l0;
  int                 *abs_diff_pic_num_minus1_l0;
  int                 *long_term_pic_idx_l0;
  int                 ref_pic_list_reordering_flag_l1;
  int                 *remapping_of_pic_nums_idc_l1;
  int                 *abs_diff_pic_num_minus1_l1;
  int                 *long_term_pic_idx_l1;

  int     (*readSlice)(struct img_par *, struct inp_par *);

  int                 LFDisableIdc;     //!< Disable loop filter on slice
  int                 LFAlphaC0Offset;  //!< Alpha and C0 offset for filtering slice
  int                 LFBetaOffset;     //!< Beta offset for filtering slice

  int                 pic_parameter_set_id;   //!<the ID of the picture parameter set the slice is reffering to

} Slice;

//****************************** ~DM ***********************************

// image parameters
typedef struct img_par
{
  int number;                                 //<! frame number
  unsigned current_mb_nr; // bitstream order
  unsigned num_dec_mb;
  int current_slice_nr;
  int *intra_block;
  int tr;                                     //<! temporal reference, 8 bit, wrapps at 255
  int tr_old;                                     //<! old temporal reference, for detection of a new frame, added by WYK
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
  int mpr[16][16];                            //<! predicted block

  int m7[16][16];                             //<! final 4x4 block. Extended to 16x16 for ABT
  int cof[4][6][4][4];                        //<! correction coefficients from predicted
  int cofu[4];
  int **ipredmode;                            //<! prediction type [90][74]
  int quad[256];
  int constrained_intra_pred_flag;            //<! if 1, prediction only from other Intra MBs
  int ***nz_coeff;
  int **siblock;
  int cod_counter;                            //<! Current count of number of skipped macroblocks in a row

  int structure;                               //<! Identify picture structure type
  int structure_old;                           //<! temp fix for multi slice per picture
  int pstruct_next_P;
  int imgtr_next_P;
  int imgtr_last_P;
  int tr_frm;
  int tr_fld;

  // B pictures
  Slice       *currentSlice;                   //<! pointer to current Slice data struct
  Macroblock          *mb_data;                //<! array containing all MBs of a whole frame
  int subblock_x;
  int subblock_y;
  int is_intra_block;
  int is_v_block;

  // For MB level frame/field coding
  int MbaffFrameFlag;

  int **field_anchor;

  DecRefPicMarking_t *dec_ref_pic_marking_buffer;                    //<! stores the memory management control operations

  int disposable_flag;                          //!< flag for disposable frame, 1:disposable
  int num_ref_idx_l0_active;             //!< number of forward reference
  int num_ref_idx_l1_active;             //!< number of backward reference

  int slice_group_change_cycle;
  // JVT-D101
  int redundant_slice_flag; 
  int redundant_pic_cnt; 
  int last_decoded_pic_id; 

  int explicit_B_prediction;



  // End JVT-D101
  // POC200301: from unsigned int to int
           int toppoc;      //poc for this top field // POC200301
           int bottompoc;   //poc of bottom field of frame
           int framepoc;    //poc of this frame // POC200301
  unsigned int frame_num;   //frame_num for this frame
  unsigned int field_pic_flag;
  unsigned int bottom_field_flag;
  
  //the following should probably go in sequence parameters
  // unsigned int log2_max_frame_num_minus4;
  unsigned int pic_order_cnt_type;
  // for poc mode 0, POC200301
  // unsigned int log2_max_pic_order_cnt_lsb_minus4;  
  // for poc mode 1, POC200301
  unsigned int delta_pic_order_always_zero_flag;
           int offset_for_non_ref_pic;
           int offset_for_top_to_bottom_field;
  unsigned int num_ref_frames_in_pic_order_cnt_cycle;
           int offset_for_ref_frame[MAX_LENGTH_POC_CYCLE];

  // POC200301
  //the following is for slice header syntax elements of poc
  // for poc mode 0.
  unsigned int pic_order_cnt_lsb;
           int delta_pic_order_cnt_bottom;
  // for poc mode 1.
           int delta_pic_order_cnt[2];

  // ////////////////////////
  // for POC mode 0:
    signed int PicOrderCntMsb;
  unsigned int PrevPicOrderCntLsb;
    signed int CurrPicOrderCntMsb;
  // for POC mode 1:
  unsigned int AbsFrameNum;
    signed int ExpectedPicOrderCnt, PicOrderCntCycleCnt, FrameNumInPicOrderCntCycle;
  unsigned int PreviousFrameNum, FrameNumOffset, ExpectedDeltaPerPicOrderCntCycle;
  unsigned int Previousfield_pic_flag,Previousbottom_field_flag,Previousnal_reference_idc,FirstFieldType;
           int Previousdelta_pic_order_cnt[2], PreviousPOC, ThisPOC;
  // /////////////////////////


  //weighted prediction
  unsigned int weighted_pred_flag;
  unsigned int weighted_bipred_idc;
  unsigned int luma_log2_weight_denom;
  unsigned int chroma_log2_weight_denom;
  int ***wp_weight;  // weight in [list][index][component] order
  int ***wp_offset;  // offset in [list][index][component] order
  int ****wbp_weight; //weight in [list][fw_index][bw_index][component] order
  int wp_round_luma;
  int wp_round_chroma;
  unsigned int apply_weights;

  unsigned int pic_order_present_flag;
  
  int idr_flag;
  int nal_reference_idc;                       //!< nal_reference_idc from NAL unit

  int idr_pic_id;

  int MaxFrameNum;

  unsigned PicWidthInMbs;
  unsigned PicHeightInMapUnits;
  unsigned FrameHeightInMbs;
  unsigned PicHeightInMbs;
  unsigned PicSizeInMbs;
  unsigned FrameSizeInMbs;

  int no_output_of_prior_pics_flag;
  int long_term_reference_flag;
  int adaptive_ref_pic_buffering_flag;

  int model_number;

} ImageParameters;

extern ImageParameters *img;
extern struct snr_par  *snr;
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
  int FileFormat;                         //<! File format of the Input file, PAR_OF_ANNEXB or PAR_OF_RTP
  int dpb_size;                          //<! Frame buffer size

#ifdef _LEAKYBUCKET_
  unsigned long R_decoder;                //<! Decoder Rate in HRD Model
  unsigned long B_decoder;                //<! Decoder Buffer size in HRD model
  unsigned long F_decoder;                //<! Decoder Inital buffer fullness in HRD model
  char LeakyBucketParamFile[100];         //<! LeakyBucketParamFile
#endif

};

extern struct inp_par *input;

typedef struct pix_pos
{
  int available;
  int mb_addr;
  int x;
  int y;
  int pos_x;
  int pos_y;
} PixelPos;

// files
FILE *p_out;                    //<! pointer to output YUV file
//FILE *p_out2;                    //<! pointer to debug output YUV file
FILE *p_ref;                    //<! pointer to input original reference YUV file file
FILE *p_log;                    //<! SNR file

#if TRACE
FILE *p_trace;
#endif

// prototypes
void init_conf(struct inp_par *inp, char *config_filename);
void report(struct inp_par *inp, struct img_par *img, struct snr_par *snr);
void init(struct img_par *img);

void init_poc();
void push_poc(unsigned int topvalue, unsigned int bottomvalue, unsigned int ref_frame_ind );


void malloc_slice(struct inp_par *inp, struct img_par *img);
void free_slice(struct inp_par *inp, struct img_par *img);

int  decode_one_frame(struct img_par *img,struct inp_par *inp, struct snr_par *snr);
void init_frame(struct img_par *img, struct inp_par *inp);
void exit_frame(struct img_par *img, struct inp_par *inp);
void DeblockFrame(struct img_par *img, byte **imgY, byte ***imgUV ) ;

int  read_new_slice();
void decode_one_slice(struct img_par *img,struct inp_par *inp);

void start_macroblock(struct img_par *img,struct inp_par *inp, int CurrentMBInScanOrder);
int  read_one_macroblock(struct img_par *img,struct inp_par *inp);
void read_ipred_modes(struct img_par *img,struct inp_par *inp);
int  decode_one_macroblock(struct img_par *img,struct inp_par *inp);
int  exit_macroblock(struct img_par *img,struct inp_par *inp, int eos_bit);

void readMotionInfoFromNAL (struct img_par *img,struct inp_par *inp);
void readCBPandCoeffsFromNAL(struct img_par *img,struct inp_par *inp);

void copyblock_sp(struct img_par *img,int block_x,int block_y);
void itrans_sp_chroma(struct img_par *img,int ll);
void itrans(struct img_par *img,int ioff,int joff,int i0,int j0);
void itrans_sp(struct img_par *img,int ioff,int joff,int i0,int j0);
int  intrapred(struct img_par *img,int ioff,int joff,int i4,int j4);
void itrans_2(struct img_par *img);
int  intrapred_luma_16x16(struct img_par *img,int predmode);
void intrapred_chroma(struct img_par *img, int uv);
int  sign(int a , int b);

// SLICE function pointers
int  (*nal_startcode_follows) ();

// NAL functions TML/CABAC bitstream
int  uvlc_startcode_follows();
int  cabac_startcode_follows();
void free_Partition(Bitstream *currStream);

// ErrorConcealment
void reset_ec_flags();

void error(char *text, int code);

// dynamic mem allocation
int  init_global_buffers(struct inp_par *inp, struct img_par *img);
void free_global_buffers(struct inp_par *inp, struct img_par *img);

void frame_postprocessing(struct img_par *img, struct inp_par *inp);
void field_postprocessing(struct img_par *img, struct inp_par *inp);
int  bottom_field_picture(struct img_par *img,struct inp_par *inp);
void init_top(struct img_par *img, struct inp_par *inp);
void init_bottom(struct img_par *img, struct inp_par *inp);
void decode_frame_slice(struct img_par *img,struct inp_par *inp, int current_header);
void decode_field_slice(struct img_par *img,struct inp_par *inp, int current_header);

#define PAYLOAD_TYPE_IDERP 8
int RBSPtoSODB(byte *streamBuffer, int last_byte_pos);
int EBSPtoRBSP(byte *streamBuffer, int end_bytepos, int begin_bytepos);

// For MB level frame/field coding
void init_super_macroblock(struct img_par *img,struct inp_par *inp);
void exit_super_macroblock(struct img_par *img,struct inp_par *inp);
int  decode_super_macroblock(struct img_par *img,struct inp_par *inp);
void decode_one_Copy_topMB(struct img_par *img,struct inp_par *inp);

void SetOneRefMV(struct img_par* img);
int peekSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP);

void fill_wp_params(struct img_par *img);

void reset_wp_params(struct img_par *img);

int poc_distance( int refa, int refb);

void FreePartition (DataPartition *dp, int n);
DataPartition *AllocPartition();

void tracebits2(const char *trace_str, int len, int info);



#endif
