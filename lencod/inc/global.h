/************************************************************************
*
*  global.h for H.26L encoder. 
*  Copyright (C) 1999  Telenor Satellite Services,Norway
*  
*  Contacts: 
*  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
*
*  Telenor Satellite Services 
*  Keysers gt.13                       tel.:   +47 23 13 86 98
*  N-0130 Oslo,Norway                  fax.:   +47 22 77 79 80
*
************************************************************************/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>

#define TRUE            1
#define FALSE           0

typedef unsigned char byte;

#define TRACE           0       /* Trace off */

#define absm(A) ((A)<(0) ? (-A):(A)) /* abs macro, faster than procedure */
#define MAX_VALUE       999999   /* used for start value for some variables */

 /* Picture types  */       
#define INTRA_IMG       0         
#define INTER_IMG       1
    
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

#define BLOCK_SIZE      4
#define MB_BLOCK_SIZE   16

#define NO_INTRA_PMODE  6		     /* #intra prediction modes */
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

byte imgY[288][352];              /* Encoded luma images */
byte imgY_org[288][352];				  /* Reference luma image */
byte imgY_pf[288][352];				    /* Post filter luma image */

byte imgUV[2][144][176];				  /* Encoded chroma images  */
byte imgUV_org[2][144][176];            /* Reference chroma images  */
byte imgUV_pf[2][144][176];				/* Post filter chroma images  */

/* fix from ver 4.1 */
byte loopb[91][75];               /* array containing filter strenght for 4x4 luma block */
/* fix from ver 4.1 */
byte loopc[47][39];               /* array containing filter strenght for 4x4 chroma block */

byte mref[MAX_MULT_PRED][1152][1408];     /* 1/4 pix luma */
byte mcef[MAX_MULT_PRED][2][352][288];    /*  pix chroma */

char tracestring[50];

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
struct inp_par					 /* all input parameters                */
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
  char infile[100];             /* YUV 4:2:0 input format*/      
  char outfile[100];            /* H.26L compressed output bitstream*/
};

struct img_par
{   
  int number;                  /* current image number to be encoded*/
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
  int mb_y_intra;	             /* which GOB to intra code */ 
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

  byte li[8];                  /* 8 pix input for new filter routine loop() */
  byte lu[6];                  /* 4 pix output for new filter routine loop() */

  int lpfilter[6][602];        /* for use in loopfilter */ 

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
  
  int   bit_use_mode_inter[33]; /* statistics of bit usage */
  int   mode_use_intra[25];     /* Macroblock mode usage for Intra frames */
  int   mode_use_inter[33];
  
  int   bit_use_head_mode[2];
  int   tmp_bit_use_cbp[2];
  int   bit_use_coeffY[2];
  int   bit_use_coeffC[2];

};

/* prototypes */
void put(char *tracestring,int len,int info);
void tracebits(char *trace_str, int len, int info);
void macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat);
void intrapred_luma(struct img_par *img,int CurrPixX,int CurrPixY);
void init();
void find_snr(struct snr_par *snr,struct img_par *img,struct inp_par *inp);
void onethirdpix(struct img_par *img);
void onethirdpix_separable(struct img_par *img);
void image(struct img_par *img,struct inp_par *inp,struct stat_par *stat);
int  find_sad(struct img_par *img,int hadamard);
int  dct_luma(int pos_mb1,int pos_mb2,int *cnt_nonz,struct img_par *img);
int  dct_chroma(int uv,int i11,struct img_par *img);
int  motion_search(struct inp_par *inp,struct img_par *img, int isi);
void n_linfo(int n,int *len,int *info);
void mvd_linfo(int mvd,int *len,int *info);
void levrun_linfo_c2x2(int level,int run,int *len,int *info);
void levrun_linfo_intra(int level,int run,int *len,int *info);
void levrun_linfo_inter(int level,int run,int *len,int *info);

void read_input(struct inp_par *inp,char config_filename[]);
void intrapred_chroma(struct img_par *img,int,int,int uv);
void rd_quant(struct img_par *img, int no_coeff,int *coeff);

void postfilter_cube(struct img_par *img);
void intrapred_luma_2(struct img_par *img);
int find_sad2(struct img_par *img,int *intra_mode);
void dct_luma2(struct img_par *img, int);

void oneforthpix(struct img_par *img);
void loop(struct img_par *img,int ibl,int ibr);
void loopfilter_new(struct img_par *img);
void loopfilter(struct img_par *img);

/* files */
FILE *p_dec;        /* internal decoded image for debugging*/
FILE *p_out;        /* H.26L output bitstream */ 
FILE *p_stat;       /* status file for the last encoding session */
FILE *p_log;        /* SNR file */  
FILE *p_in;         /* YUV */
FILE *p_trace;

#endif







