/************************************************************************
*
*  global.h for H.26L decoder. 
*  Copyright (C) 1999  Telenor Satellite Services, Norway
*  
*  Contacts: 
*  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
*  
*
*  Telenor Satellite Services 
*  P.O.Box 6914 St.Olavs plass                        
*  N-0130 Oslo, Norway                  
*  
************************************************************************/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>                              /* for FILE */

#define FALSE           0
#define TRUE            1

typedef unsigned char   byte;                   /*  8 bit unsigned */

#define LEN_STARTCODE   31                      /* uniqe length of startcode, defines new picture */  

#define INTER_IMG_1     0               
#define INTER_IMG_MULT  1       
#define INTRA_IMG       2       

/* mb_mode constants */
#define COPY_MB         0                       /* just copy last MB, no motion vectors */  
#define M16x16_MB       1                       /* block sizes for inter motion search  */
#define M16x8_MB        2   
#define M8x16_MB        3
#define M8x8_MB         4
#define M8x4_MB         5   
#define M4x8_MB         6
#define M4x4_MB         7
#define INTRA_MB        8                       /* intra coded MB in inter frame */     
/* imod constants */
#define INTRA_MB_OLD    0       /* new intra prediction mode in inter frame         */
#define INTRA_MB_NEW    1       /* 'old' intra prediction mode in inter frame       */
#define INTRA_MB_INTER  2       /* Intra MB in inter frame, use the constants above */ 

/* 4x4 intra prediction modes */
/*#define DC_PRED         0
#define VERT_PRED       1
#define HOR_PRED        2
#define DIAG_PRED_LR    3
#define DIAG_PRED_RL    4  */
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


#define BLOCK_SIZE      4
#define MB_BLOCK_SIZE   16

#define MAX_MULT_PRED    5                      /* max number of reference frames

/* QCIF format */
#define IMG_WIDTH       176                     /* luma */
#define IMG_HEIGHT      144
#define IMG_WIDTH_CR    88                      /* chroma */
#define IMG_HEIGHT_CR   72

#define QCIF            0                       /* image format */
#define CIF             1

#define EOS_MASK        0x01                    /* mask for end of sequence (bit 1)         */
#define ICIF_MASK       0x02                    /* mask for image format (bit 2)            */
#define QP_MASK         0x7C                    /* mask for quant.parameter (bit 3->7)      */
#define TR_MASK         0x7f80                  /* mask for temporal referance (bit 8->15)  */

#define DECODING_OK     0
#define SEARCH_SYNC     1

#define MIN_PIX_VAL     0                       /* lowest legal values for 8 bit sample     */ 
#define MAX_PIX_VAL     255                     /* highest legal values for 8 bit sample    */ 

#define mmax(a, b)      ((a) > (b) ? (a) : (b)) /* Macro returning max value                */
#define mmin(a, b)      ((a) < (b) ? (a) : (b)) /* Macro returning min value                */


byte imgY[288][352];                            /* array for the decoded luma component     */  
byte imgY_pf[288][352];                         /* Post filter luma image */
byte imgUV[2][144][176];                        /* array for the chroma component           */
byte imgUV_pf[2][144][176];                     /* Post filter luma image */

byte mref[MAX_MULT_PRED][1152][1408];     /* 1/4 pix luma */
byte mcef[MAX_MULT_PRED][2][352][288];    /*  pix chroma */

/* fix from ver 4.1 */
byte loopb[91][75];
/* fix from ver 4.1 */
byte loopc[47][39];

/* image parameters */
struct img_par
{ 
  int number;                                  /* frame number                              */    
  int format;                                  /* frame format CIF or QCIF                  */   
  int tr;                                      /* temporal reference, 8 bit, wrapps at 255  */
  int frame_cycle;                             /* cycle through all stored reference frames */
  int qp;                                      /* quant for the current frame               */
  int type;                                    /* image type INTER/INTRA                    */   
  int width;
  int height;
  int width_cr;                                /* width chroma */
  int height_cr;                               /* height chroma */
  int mb_y;
  int mb_x;     
  int block_y;
  int pix_y;
  int pix_x;
  int pix_c_y;  
  int block_c_x;
  int block_x;
  int pix_c_x;
  int mb_mode;
  int imod;                                    /* MB mode */ 
  int mv[92][72][3];    
  int mpr[16][16];                             /* predicted block */                    
 
  int m7[4][4];                                /* final 4x4 block                         */
  int cof[4][6][4][4];                         /* correction coefficients from predicted  */ 
  int cofu[4];    
  int ipredmode[90][74];                       /* prediction type  *//* fix from ver 4.1 */
  int quad[256];
  int lpfilter[6][601];
  
  byte li[8];                                 /* 8 pix input to loopfilter routine */
  byte lu[8];                                 /* 8 pix output from loopfilter routine */
};

/* signal to noice ratio parameters */
struct snr_par         
{
  float snr_y;                                 /* current Y SNR                           */      
  float snr_u;                                 /* current U SNR                           */
  float snr_v;                                 /* current V SNR                           */
  float snr_y1;                                /* SNR Y(dB) first frame                   */
  float snr_u1;                                /* SNR U(dB) first frame                   */
  float snr_v1;                                /* SNR V(dB) first frame                   */
  float snr_ya;                                /* Average SNR Y(dB) remaining frames      */
  float snr_ua;                                /* Average SNR U(dB) remaining frames      */
  float snr_va;                                /* Average SNR V(dB) remaining frames      */
};

/* input parameters from configuration file */
struct inp_par                    
{
  char infile[100];                             /* Telenor H.26L input                     */
  char outfile[100];                            /* Decoded YUV 4:2:0 output                */
  char reffile[100];                            /* Optional YUV 4:2:0 reference file for SNR measurement */
  int  postfilter;                              /* postfilter (0=OFF,1=ON)                 */
};

/* prototypes */
int     read_frame(FILE *p_in);
int     get_info(int *len);
void    write_frame(struct img_par *img,int,FILE *p_out); 
void    itrans(struct img_par *img,int ioff,int joff,int i0,int j0);
int      intrapred(struct img_par *img,int ioff,int joff,int i4,int j4);
void    onethirdpix(struct img_par *img);
void    onethirdpix_separable(struct img_par *img);
void    postfilter(struct img_par *img);
void    loopfilter(struct img_par *img);
int     macroblock(struct img_par *img);
int     linfo_mvd(int len,int info);
int     image(struct img_par *img,struct inp_par *inp);
void    linfo_levrun_inter(int len,int info,int *level,int *irun);
void    linfo_levrun_intra(int len,int info,int *level,int *irun);
void    linfo_levrun_c2x2(int len,int info,int *level,int *irun);
void    find_snr(struct snr_par *snr,struct img_par *img, FILE *p_ref, int postfilter);
void    init(struct img_par *img);
void    loopfilter_new(struct img_par *img);

void oneforthpix(struct img_par *img);

void itrans_2(struct img_par *img);
int intrapred_luma_2(struct img_par *img,int predmode);
void loopfiler_new(struct img_par *img);
void loop(struct img_par *img,int ibl,int ibr);

#endif