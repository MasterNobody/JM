/************************************************************************
*
*  global.h for H.26L encoder.
*  Copyright (C) 1999  Telenor Satellite Services,Norway
*                      Ericsson Radio Systems, Sweden
*
*  Contacts:
*  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
*
*  Telenor Satellite Services
*  Keysers gt.13                       tel.:   +47 23 13 86 98
*  N-0130 Oslo,Norway                  fax.:   +47 22 77 79 80
*
*  Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
*
*  Ericsson Radio Systems
*  KI/ERA/T/VV
*  164 80 Stockholm, Sweden
*
************************************************************************/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>

#define TRUE            1
#define FALSE           0

typedef unsigned char byte;

#define TRACE           0       /* 0:Trace off 1:Trace on*/

#define absm(A) ((A)<(0) ? (-A):(A)) /* abs macro, faster than procedure */
#define MAX_VALUE       999999   /* used for start value for some variables */

/* Picture types  */
#define INTRA_IMG       0
#define INTER_IMG       1
/* B pictures */
#define B_IMG			2

//<pu>
/* coding of MBtypes */
#define INTRA_CODED_MB  0
#define INTER_CODED_MB  1
//<\pu>

/* inter MB modes */
#define COPY_MB         0        /* just copy last MB, no motion vectors */
#define M16x16_MB       1        /* 16 x 16 block */
#define M16x8_MB        2
#define M8x16_MB        3
#define M8x8_MB         4
#define M8x4_MB         5
#define M4x8_MB         6
#define M4x4_MB         7

/* imod constants */
#define INTRA_MB_OLD    0       /* new intra prediction mode in inter frame         */
#define INTRA_MB_NEW    1       /* 'old' intra prediction mode in inter frame       */
#define INTRA_MB_INTER  2       /* Intra MB in inter frame, use the constants above */

/* B pictures : img->imod */
#define B_Forward       3
#define B_Backward      4
#define B_Bidirect      5
#define B_Direct        6

#define BLOCK_SIZE      4
#define MB_BLOCK_SIZE   16

#define NO_INTRA_PMODE  6        /* #intra prediction modes */
/* 4x4 intra prediction modes */
#define DC_PRED         0
#define DIAG_PRED_RL    1
#define VERT_PRED       2
#define DIAG_PRED_LR_45 3
#define HOR_PRED        4
#define DIAG_PRED_LR    5

/* 16x16 intra prediction modes */
#define VERT_PRED_16    0
#define HOR_PRED_16     1
#define DC_PRED_16      2
#define PLANE_16        3

/* image formats*/
#define QCIF            0
#define CIF             1

/* QCIF format */
#define IMG_WIDTH       176
#define IMG_HEIGHT      144
#define IMG_WIDTH_CR    88
#define IMG_HEIGHT_CR   72

/* coefficient scan order */
#define SINGLE_SCAN     0
#define DOUBLE_SCAN     1

#define INIT_FRAME_RATE 30
#define LEN_STARTCODE   31        /* length of start code */
#define MAX_MULT_PRED   5         /* max number of frames to make inter prediction from */
#define EOS             1         /* End Of Sequence */

/* symbol types */
//<!pu>#define TYPE_HEADER     0
//<!pu>#define TYPE_MBHEADER   1
//<!pu>#define TYPE_MVD        2
//<!pu>#define TYPE_CBP        3
//<!pu>#define TYPE_2x2DC      4
//<!pu>#define TYPE_COEFF_Y    5
//<!pu>#define TYPE_COEFF_C    6
//<!pu>#define TYPE_EOS        7

#define MVPRED_MEDIAN   0
#define MVPRED_L        1
#define MVPRED_U        2
#define MVPRED_UR       3

int  refFrArr[72][88];            /* Array for reference frames of each block */
byte imgY[288][352];              /* Encoded luma images */
byte imgY_org[288][352];          /* Reference luma image */
byte imgY_pf[288][352];           /* Post filter luma image */

byte imgUV[2][144][176];          /* Encoded chroma images  */
byte imgUV_org[2][144][176];      /* Reference chroma images  */
byte imgUV_pf[2][144][176];       /* Post filter chroma images  */

int imgY_tmp[576][704];           /* temporary solution: */

/* fix from ver 4.1 */
byte loopb[91][75];               /* array containing filter strenght for 4x4 luma block */
/* fix from ver 4.1 */
byte loopc[47][39];               /* array containing filter strenght for 4x4 chroma block */

byte mref[MAX_MULT_PRED][1152][1408];     /* 1/4 pix luma */
byte mcef[MAX_MULT_PRED][2][352][288];    /*  pix chroma */

/* B pictures */
byte nextP_imgY[288][352];              /* Encoded next luma images */
byte nextP_imgUV[2][144][176];				  /* Encoded next chroma images  */
int  Bframe_ctr, frame_no, P_interval, nextP_tr;
int  tot_time;

char tracestring[100];

struct snr_par
{
  float snr_y;                 /* current Y SNR                       */
  float snr_u;                 /* current U SNR                       */
  float snr_v;                 /* current V SNR                       */
  float snr_y1;                /* SNR Y(dB) first frame               */
  float snr_u1;                /* SNR U(dB) first frame               */
  float snr_v1;                /* SNR V(dB) first frame               */
  float snr_ya;                /* Average SNR Y(dB) remaining frames  */
  float snr_ua;                /* Average SNR U(dB) remaining frames  */
  float snr_va;                /* Average SNR V(dB) remaining frames  */
};

struct inp_par           /* all input parameters                */
{
  int no_frames;                /* number of frames to be encoded                                              */
  int qp0;                      /* QP of first frame                                                           */
  int qpN;                      /* QP of remaining frames                                                      */
  int jumpd;                    /* number of frames to skip in input sequence (e.g 2 takes frame 0,3,6,9...)   */
  int hadamard;                 /* 0: 'normal' SAD in 1/3 pixel search.  1: use 4x4 Haphazard transform and '  */
                                /* Sum of absolute transform difference' in 1/3 pixel search                   */
  int search_range;             /* search range - integer pel search and 16x16 blocks.  The search window is   */
                                /* generally around the predicted vector. Max vector is 2xmcrange.  For 8x8    */
                                /* and 4x4 block sizes the search range is 1/2 of that for 16x16 blocks.       */
  int no_multpred;              /* 1: prediction from the last frame only. 2: prediction from the last or      */
                                /* second last frame etc.  Maximum 5 frames                                    */
  int postfilter;               /* Not used                                      */
  int img_format;               /* 0: QCIF. 1: CIF                                                             */
  int intra_upd;                /* For error robustness. 0: no special action. 1: One GOB/frame is intra coded */
                                /* as regular 'update'. 2: One GOB every 2 frames is intra coded etc.          */
                                /* In connection with this intra update, restrictions is put on motion vectors */
                                /* to prevent errors to propagate from the past*/
  int write_dec;                /* write decoded image to file, for debugging*/
  int blc_size[8][2];           /* array for different block sizes */
  int slice_mode;               /* Indicate what algorithm to use for setting slices */
  int slice_argument;           /* Argument to the specified slice algorithm */
  char infile[100];             /* YUV 4:2:0 input format*/
  char outfile[100];            /* H.26L compressed output bitstream*/

  /* B pictures */
  char dec_file[100], dec_file2[100];
  int successive_Bframe;       /* number of B frames that will be used */
  int qpB;                      /* QP of B frames */
};

struct img_par
{
  int number;                  /* current image number to be encoded*/
  int current_mb_nr;
  int current_slice_nr;
  int type;
  int multframe_no;
  int qp;                      /* quant for the current frame */
  int frame_cycle;
  int framerate;
  int width;                   /* Number of pels */
  int width_cr;                /* Number of pels chroma */
  int height;                  /* Number of lines */
  int height_cr;               /* Number of lines  chroma */
  int height_err;              /* For y vector limitation for error robustness*/
  int mb_y;                    /* current MB vertical   */
  int mb_x;                    /* current MB horizontal */
  int block_y;                 /* current block vertical   */
  int block_x;                 /* current block horizontal */
  int pix_y;                   /* current pixel vertical   */
  int pix_x;                   /* current pixel horizontal */
  int mb_y_upd;
  int mb_y_intra;              /* which GOB to intra code */
  int mb_mode;                 /* MB mode relevant for inter images, for intra images are all MB intra*/
  int imod;                    /* new/old intra modes + inter */
  int pix_c_y;                 /* current pixel chroma vertical   */
  int block_c_x;               /* current block chroma vertical   */
  int pix_c_x;                 /* current pixel chroma horizontal   */
  int blc_size_h;              /* block size for motion search, horizontal */
  int blc_size_v;              /* block size for motion search, vertical */
  int kac;                     /* any AC coeffs */
  int spiral_search[2][6561];  /* To obtain spiral search */
  int ipredmode[90][74];       /* prediction mode for inter frames */ /* fix from ver 4.1 */
  int mprr[6][4][4];           /* all 5 prediction modes */
  int mprr_2[5][16][16];       /* all 4 new intra prediction modes */
  int mv[4][4][5][9][2];       /* motion vectors for all block types and all reference frames */
  int mpr[16][16];             /* current best prediction mode */
  int m7[16][16];              /* the diff pixel values between orginal image and prediction */
  int cof[4][6][18][2][2];     /* coefficients */
  int cofu[5][2][2];           /* coefficients chroma */
  int quad[256];               /* Array containing square values,used for snr computation  */                                         /* Values are limited to 5000 for pixel differences over 70 (sqr(5000)). */
  int mv_bituse[512];
  int slice_numbers[396];

  byte li[8];                  /* 8 pix input for new filter routine loop() */
  byte lu[6];                  /* 4 pix output for new filter routine loop() */

  int lpfilter[6][602];        /* for use in loopfilter */

	/* B pictures */
	int tr;
	int fw_mb_mode;
	int fw_multframe_no;
	int fw_blc_size_h;
	int fw_blc_size_v;
	int bw_mb_mode;
	int bw_multframe_no;
	int bw_blc_size_h;
	int bw_blc_size_v;
	int p_fwMV[4][4][5][9][2]; // for MVDFW
	int p_bwMV[4][4][5][9][2]; // for MVDBW
	int blk_bituse[10];   // it is included when getting bid_sad
};

struct stat_par                 /* statistics */
{
  int   quant0;                 /* quant for the first frame */
  int   quant1;                 /* average quant for the remaining frames */
  float bitr;                   /* bit rate for current frame, used only for output til terminal*/
  float bitr0;                  /* stored bit rate for the first frame*/
  float bitrate;                /* average bit rate for the sequence except first frame*/
  int   bit_ctr;                /* counter for bit usage */
  int   bit_ctr_0;              /* stored bit use for the first frame*/
  int   bit_ctr_n;              /* bit usage for the current frame*/
  int   bit_slice;              /* number of bits in current slice */
  int   bit_use_mode_inter[33]; /* statistics of bit usage */
  int   mode_use_intra[25];     /* Macroblock mode usage for Intra frames */
  int   mode_use_inter[33];

	/* B pictures */  
	int   *mode_use_Bframe;
	int   *bit_use_mode_Bframe; 
	int   bit_ctr_P;
	int   bit_ctr_B;
	float bitrate_P;
	float bitrate_B;
	int   bit_use_head_mode[3];
  int   tmp_bit_use_cbp[3];
  int   bit_use_coeffY[3];
  int   bit_use_coeffC[3];
};

/* prototypes */
#if TRACE
void tracebits(char *tracestring, int len, int bitpattern);
#endif

void put_symbol(char *tracestring,int len,int info, int sliceno, int type);
void store_symbol(char *tracestring,int len,int info, int sliceno, int type);
void put_stored_symbols();
void flush_macroblock_symbols();

void macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat);
void intrapred_luma(struct img_par *img,int CurrPixX,int CurrPixY);
void init(struct img_par *img);
void find_snr(struct snr_par *snr,struct img_par *img);
void onethirdpix(struct img_par *img);
void onethirdpix_separable(struct img_par *img); 
void  image(struct img_par *img,struct inp_par *inp,struct stat_par *stat, struct snr_par *snr);
int  find_sad(struct img_par *img,int hadamard);
int  dct_luma(int pos_mb1,int pos_mb2,int *cnt_nonz,struct img_par *img);
int  dct_chroma(int uv,int i11,struct img_par *img);
int  motion_search(struct inp_par *inp,struct img_par *img, int isi);
void n_linfo(int n,int *len,int *info);
void mvd_linfo(int mvd,int *len,int *info);
void levrun_linfo_c2x2(int level,int run,int *len,int *info);
void levrun_linfo_intra(int level,int run,int *len,int *info);
void levrun_linfo_inter(int level,int run,int *len,int *info);
//<pu> 10/6/2000
int inter_intra(struct img_par *img);
//<\pu>

void read_input(struct inp_par *inp,char config_filename[]);
void intrapred_chroma(struct img_par *img,int,int,int uv);
void rd_quant(struct img_par *img, int no_coeff,int *coeff);

void postfilter_cube(struct img_par *img);
void intrapred_luma_2(struct img_par *img);
int find_sad2(struct img_par *img,int *intra_mode);
void dct_luma2(struct img_par *img, int);

void oneforthpix(struct img_par *img);
//int loop(struct img_par *img,int ibl,int ibr, int longFilt);
//void loopfilter_new(struct img_par *img);
int loop(struct img_par *img, int ibl, int ibr, int longFilt, int chroma);
void loopfilter(struct img_par *img);

/* B pictures */
void oneforthpix_1(struct img_par *img);
void oneforthpix_2(struct img_par *img);
void initialize_Bframe(int *B_interval, int successive_Bframe, int no_Bframe_to_code,
						struct img_par *img, struct inp_par *inp, struct stat_par *stat);
void ReadImage(int frame_no, struct img_par *img);
void code_Bframe(struct img_par *img, struct inp_par *inp, struct stat_par *stat);
void intra_4x4(int *cbp, int *tot_intra_sad, int *intra_pred_modes, 
							 struct img_par *img, struct inp_par *inp, struct stat_par *stat);
void intra_16x16(int *tot_intra_sad, int *intra_pred_mode_2, struct img_par *img);
int motion_search_Bframe(int tot_intra_sad, struct img_par *img, struct inp_par *inp);
int get_fwMV(int *min_fw_sad, struct img_par *img, struct inp_par *inp, int tot_intra_sad);
void get_bwMV(int *min_bw_sad, struct img_par *img, struct inp_par *inp_par);
void get_bid(int *bid_sad, int fw_predframe_no, struct img_par *img, struct inp_par *inp);
void get_dir(int *dir_sad, struct img_par *img, struct inp_par *inp);
void compare_sad(int tot_intra_sad, int fw_sad, int bw_sad, int bid_sad, int dir_sad, struct img_par *img);
void create_imgY_cbp(int *cbp, struct img_par *img);
void chroma_block(int *cbp, int *cr_cbp, struct img_par *img);
void writeimage(int Bframe_to_code, int no_Bframe, struct img_par *img, struct inp_par *inp);

/* files */
FILE *p_dec;        /* internal decoded image for debugging*/
FILE *p_out;        /* H.26L output bitstream */
FILE *p_stat;       /* status file for the last encoding session */
FILE *p_log;        /* SNR file */
FILE *p_in;         /* YUV */
FILE *p_dec2;

FILE *p_datpart;	/* file to write bitlength and id of all partitions */

#if TRACE
FILE *p_trace;     /* Trace file */
#endif

/* NAL function pointers */
void (*nal_init)();
void (*nal_finit)();
int  (*nal_put_startcode)(int tr, int mb_nr, int qp, int image_format, int sliceno, int type);
void (*nal_putbits)(int len, int bitpattern, int sliceno, int type);
int  (*nal_slice_too_big)(int bits_slice);

/* NAL functions TML bitstream */
void bits_tml_init();
void bits_tml_finit();
int  bits_tml_put_startcode(int tr, int mb_nr, int qp, int image_format, int sliceno, int type);
void bits_tml_putbits(int len, int bitpattern, int sliceno, int type);
int  bits_tml_slice_too_big(int bits_slice);

/* NAL functions TML Partition syntax */
void part_tml_init();
void part_tml_finit();
int  part_tml_put_startcode(int tr, int mb_nr, int qp, int image_format, int sliceno, int type);
void part_tml_putbits(int len, int bitpattern, int sliceno, int type);
int  part_tml_slice_too_big(int bits_slice);

#endif
