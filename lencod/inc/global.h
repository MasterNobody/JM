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
#include "defines.h"


/***********************************************************************
 * T y p e    d e f i n i t i o n s    f o r    T M L  
 ***********************************************************************
 */

typedef unsigned char byte;

/* Introduced by TOM */
/* Data Partitioning Modes */
typedef enum
{
	PAR_DP_1,    /* no data partitioning is supported    */
	PAR_DP_2,    /* data partitioning with 2 partitions  */
	PAR_DP_4    /* data partitioning with 4 partitions  */
} PAR_DP_TYPE;


/* Output File Types */
typedef enum
{
	PAR_OF_26L,   /* Current TML description             */
	PAR_OF_SLICE,  /* Slice File Format				     */
	PAR_OF_NAL    /* Current NAL Format                  */
} PAR_OF_TYPE;
/* End Introduced by TOM */


typedef enum {
	FALSE,
	TRUE
} Boolean;

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
	SE_BFRAME,
	SE_EOS,					
	SE_MAX_ELEMENTS	/* number of maximum syntax elements */
} SE_type;				/* substituting the definitions in element.h */


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
	SINGLE_SCAN,
	DOUBLE_SCAN
} ScanMode;

typedef enum {
	NO_SLICES,
	FIXED_MB,
	FIXED_RATE,
	CALLBACK
} SliceMode;

typedef enum {
	UVLC,
	CABAC
} SymbolMode;


/***********************************************************************
 * D a t a    t y p e s   f o r  C A B A C  
 ***********************************************************************
 */


typedef struct 
{								
    unsigned int   Elow, Ehigh;
    unsigned int   Ebuffer;
    unsigned int   Ebits_to_go;
    unsigned int   Ebits_to_follow;
    byte					 *Ecodestrm;
    int						 *Ecodestrm_len;
} EncodingEnvironment;						/* struct to characterize the state of the arithmetic coding engine */

typedef EncodingEnvironment *EncodingEnvironmentPtr;


typedef struct 
{
    unsigned int	cum_freq[2];           /* cumulated frequency counts	*/
    Boolean				in_use;                /* flag for context in use			*/
		unsigned int	max_cum_freq;					 /* maximum frequency count			*/ 

} BiContextType;							 /* struct for context management */

typedef BiContextType *BiContextTypePtr;


/**********************************************************************
 * C O N T E X T S   F O R   T M L   S Y N T A X   E L E M E N T S
 **********************************************************************
 */

#define NUM_MB_TYPE_CTX  10
#define NUM_MV_RES_CTX   10
#define NUM_REF_NO_CTX   6

typedef struct
{
	BiContextTypePtr mb_type_contexts[2];
	BiContextTypePtr mv_res_contexts[2];
	BiContextTypePtr ref_no_contexts;
} MotionInfoContexts;

#define NUM_IPR_CTX    2
#define NUM_CBP_CTX    4
#define NUM_TRANS_TYPE 9
#define NUM_LEVEL_CTX  4
#define NUM_RUN_CTX    2

typedef struct 
{
	BiContextTypePtr ipr_contexts[6]; 
	BiContextTypePtr cbp_contexts[2][3]; 
	BiContextTypePtr level_context[NUM_TRANS_TYPE];
	BiContextTypePtr run_context[NUM_TRANS_TYPE];
} TextureInfoContexts;

/************************ end of data type definition for CABAC ********************/


/***********************************************************************
 * N e w   D a t a    t y p e s   f o r    T M L  
 ***********************************************************************
 */

typedef struct syntaxelement {
	int							type;				/* type of syntax element for data part.*/
	int							value1;			/* numerical value of syntax element		*/
	int							value2;			/* for blocked symbols, e.g. run/level	*/
	int							len;				/* length of code												*/
	int							inf;				/* info part of UVLC code								*/
	unsigned int		bitpattern;	/* UVLC bitpattern                      */
	int							context;		/* CABAC context												*/
#if TRACE
	char						tracestring[100];			/* trace string								*/
#endif

	void			(*mapping)(int value1, int value2, int* len_ptr, int* info_ptr); // for mapping of syntaxElement to UVLC
  void			(*writing)(struct syntaxelement *, struct inp_par *, struct img_par *, EncodingEnvironmentPtr);
																														/* used for CABAC: refers to actual coding method of each
																														 individual syntax element type */
}	SyntaxElement; 

typedef struct macroblock
{
	int							currSEnr;					/* number of current syntax element */
	int							slice_nr;					/* slice number to which the MB belongs */
	int							qp;							/* for future use */
	int							intraOrInter;
	int							bitcounter[MAX_BITCOUNTER_MB];
	struct macroblock			*mb_available[3][3];		/* pointer to neighboring MBs in a 3x3 window of current MB, which is located at [1][1] */
																						/* NULL pointer identifies neighboring MBs which are unavailable */ 
		
	/* some storage of macroblock syntax elements for global access */
	int									mb_type;
	int									mb_imode;
	int									ref_frame;
	int									mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];			/* indices correspond to [forw,backw][block_y][block_x][x,y] */
	int									intra_pred_modes[BLOCK_MULTIPLE*BLOCK_MULTIPLE];
	int									cbp;

} Macroblock;

typedef struct {
	int				byte_pos;						// current position in bitstream;
	int				bits_to_go;					// current bitcounter
	byte			byte_buf;						// current buffer for last written byte
	int				stored_byte_pos;		// storage for position in bitstream;
	int				stored_bits_to_go;	// storage for bitcounter
	byte			stored_byte_buf;		// storage for buffer of last written byte

	byte			*streamBuffer;			//  actual buffer for written bytes

} Bitstream;

typedef struct datapartition {

	Bitstream						*bitstream;
	EncodingEnvironment ee_cabac;
	
	int									(*writeSyntaxElement)(SyntaxElement *, struct img_par *, struct inp_par *, struct datapartition *); 
															// virtual function;
															// actual method depends on chosen data partition and 
															// entropy coding method 
} DataPartition;

typedef struct {
	int							picture_id;
	int							slice_nr;    // not necessary but o.k.
	int							qp;
	int							format;
	int							picture_type; // picture type
	int							start_mb_nr;
	int							dp_mode;     // data partioning mode
	int							max_part_nr; // number of different partitions
	int							eos_flag;	 // end of sequence flag
	DataPartition				*partArr;    // array of partitions
	MotionInfoContexts			*mot_ctx;    // pointer to struct of context models for use in CABAC
	TextureInfoContexts			*tex_ctx;	 // pointer to struct of context models for use in CABAC


	Boolean						(*slice_too_big)(int bits_slice); // for use of callback functions

} Slice;
/******************************* ~DM ************************************/

/* GH: old fixed mem allocation for picture sizes up to CIF*/
/*int  refFrArr[72][88];*/            /* Array for reference frames of each block */
/*byte imgY[288][352]; */             /* Encoded luma images */
/*byte imgY_org[288][352]; */         /* Reference luma image */
/*byte imgY_pf[288][352];*/           /* Post filter luma image */
/*byte imgUV[2][144][176];   */       /* Encoded chroma images  */
/*byte imgUV_org[2][144][176]; */     /* Reference chroma images  */
/*byte imgUV_pf[2][144][176];  */     /* Post filter chroma images  */
/* fix from ver 4.1 */
/*byte loopb[91][75]; */              /* array containing filter strenght for 4x4 luma block */
/*byte loopc[47][39]; */              /* array containing filter strenght for 4x4 chroma block */
/*byte mref[MAX_MULT_PRED][1152][1408];  */   /* 1/4 pix luma */
/*byte mcef[MAX_MULT_PRED][2][352][288]; */   /*  pix chroma */
/*byte mref_P[1152][1408];      1/4 pix luma for next P picture*/
/*byte mcef_P[2][352][288];     pix chroma for next P picture*/
/*int  imgY_tmp[576][704];*/

/* GH: global picture format dependend buffers, mem allocation in image.c*/ 
byte   **imgY;               /* Encoded luma images  */
byte  ***imgUV;              /* Encoded croma images */
byte   **imgY_org;           /* Reference luma image */
byte  ***imgUV_org;          /* Reference croma image */
byte   **imgY_pf;            /* Post filter luma image */
byte  ***imgUV_pf;           /* Post filter croma image */
byte   **loopb;              /* array containing filter strenght for 4x4 luma block */
byte   **loopc;              /* array containing filter strenght for 4x4 chroma block */
byte  ***mref;               /* 1/4 pix luma */
byte ****mcef;               /* pix chroma */
int    **img4Y_tmp;			 /* for quarter pel interpolation */
byte   **imgY_tmp;           /* for loop filter */  
byte  ***imgUV_tmp;
int   ***tmp_mv;             /* motion vector buffer */
int    **refFrArr;           /* Array for reference frames of each block */

/* B pictures */
// motion vector : forward, backward, direct
int  ***tmp_fwMV;    //[2][72][92];
int  ***tmp_bwMV;    //[2][72][92];
int  ***dfMV;        //[2][72][92];
int  ***dbMV;        //[2][72][92];
int   **fw_refFrArr; //[72][88];
int   **bw_refFrArr; //[72][88];
byte  **mref_P;      //for B-frames: 1/4 pix luma for next P picture 
byte ***mcef_P;      //for B-frames: pix chroma for next P picture
byte  **nextP_imgY;
byte ***nextP_imgUV;

int  Bframe_ctr, frame_no, nextP_tr;
int  tot_time;

char errortext[300];

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
  int img_format;               /* GH: 0=SUB_QCIF, 1=QCIF; 2=CIF, 3=CIF_4. 4=CIF_16, 5=CUSTOM                  */
  int img_width;                /* GH: if CUSTOM image format is chosen, use this size                         */
  int img_height;               /* GH: width and height must be a multiple of 16 pels                          */
  int yuv_format;               /* GH: YUV format (0=4:0:0, 1=4:2:0, 2=4:2:2, 3=4:4:4,currently only 4:2:0 is supported)*/
  int color_depth;              /* GH: YUV color depth per component in bit/pel (currently only 8 bit/pel is supported) */
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
  int successive_Bframe;        /* number of B frames that will be used */
  int qpB;                      /* QP of B frames */

  /* Introduced by TOM */
  int symbol_mode;			/* Specifies the mode the symbols are mapped on bits */
  int of_mode;					/* Specifies the mode of the output file             */
	int partition_mode;		/* Specifies the mode of data partitioning */
};

struct img_par
{
  int number;                  /* current image number to be encoded*/
  int current_mb_nr;
  int current_slice_nr;
  int type;
	int no_multpred;             /* 1: prediction from the last frame only. 2: prediction from the last or      */
                               /* second last frame etc.  Maximum MAX_MULT_PRED frames                        */
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
	int subblock_y;              /* DM: current subblock vertical   */
  int subblock_x;              /* DM: current subblock horizontal */
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
  int **ipredmode;             /* GH ipredmode[90][74];prediction mode for inter frames */ /* fix from ver 4.1 */

	/* some temporal buffers */
  int mprr[6][4][4];           /* all 5 prediction modes */
  int mprr_2[5][16][16];       /* all 4 new intra prediction modes */
  int mv[4][4][MAX_MULT_PRED][9][2];       /* motion vectors for all block types and all reference frames */
  int mpr[16][16];             /* current best prediction mode */
  int m7[16][16];              /* the diff pixel values between orginal image and prediction */
  int cof[4][6][18][2][2];     /* coefficients */
  int cofu[5][2][2];           /* coefficients chroma */
	/* DM: 030501 */
	byte tmp_loop_Y[BLOCK_MULTIPLE+2][BLOCK_MULTIPLE+2];			/* temporal storage for LUMA loop-filter strength */
	byte tmp_loop_UV[BLOCK_MULTIPLE/2+2][BLOCK_MULTIPLE/2+2];	/* temporal storage for CHROMA loop-filter strength */
	/* ~DM */
 
	/* DM: 030501 */
	Slice				*currentSlice;			/* pointer to current Slice data struct */
	Macroblock	*mb_data;			      /* array containing all MBs of a whole frame */ 
	SyntaxElement MB_SyntaxElements[MAX_SYMBOLS_PER_MB];	/* temporal storage for all chosen syntax elements of one MB */
	/* ~DM */

	int quad[256];               /* Array containing square values,used for snr computation  */                                         /* Values are limited to 5000 for pixel differences over 70 (sqr(5000)). */
  int mv_bituse[512];
  int *slice_numbers;

  byte li[8];                  /* 8 pix input for new filter routine loop() */
  byte lu[6];                  /* 4 pix output for new filter routine loop() */

  int lpfilter[6][602];        /* for use in loopfilter */

  /* B pictures */
  int tr;
  int b_interval;
  int p_interval;
  int b_frame_to_code;
  int fw_mb_mode;
  int fw_multframe_no;
  int fw_blc_size_h;
  int fw_blc_size_v;
  int bw_mb_mode;
  int bw_multframe_no;
  int bw_blc_size_h;
  int bw_blc_size_v;
  int p_fwMV[4][4][MAX_MULT_PRED][9][2]; // for MVDFW
  int p_bwMV[4][4][MAX_MULT_PRED][9][2]; // for MVDBW
  int blk_bituse[10];        // it is included when getting bid_sad
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

/* files */
FILE *p_dec;        /* internal decoded image for debugging*/
FILE *p_out;        /* H.26L output bitstream */
FILE *p_stat;       /* status file for the last encoding session */
FILE *p_log;        /* SNR file */
FILE *p_in;         /* YUV */
FILE *p_datpart;		/* file to write bitlength and id of all partitions */

#if TRACE
FILE *p_trace;     /* Trace file */
#endif


/***********************************************************************
 * P r o t o t y p e s   f o r    T M L  
 ***********************************************************************
 */


void intrapred_luma(struct img_par *img,int CurrPixX,int CurrPixY);
void init(struct img_par *img);
void find_snr(struct snr_par *snr,struct img_par *img);
void oneforthpix(struct img_par *img, struct inp_par *inp);
void oneforthpix_2(struct img_par *img);
int  encode_oneIorP_Frame(struct img_par *img,struct inp_par *inp,struct stat_par *stat, struct snr_par *snr);
int  encode_oneB_Frame(struct img_par *img,struct inp_par *inp,struct stat_par *stat, struct snr_par *snr);
int  find_sad(struct img_par *img,int hadamard);
int  dct_luma(int pos_mb1,int pos_mb2,int *cnt_nonz,struct img_par *img);
int  dct_chroma(int uv,int i11,struct img_par *img);
int  motion_search(struct inp_par *inp,struct img_par *img, int isi);
void levrun_linfo_c2x2(int level,int run,int *len,int *info);
void levrun_linfo_intra(int level,int run,int *len,int *info);
void levrun_linfo_inter(int level,int run,int *len,int *info);
int  sign(int a,int b);
void intrapred_chroma(struct img_par *img,int,int,int uv);
void rd_quant(struct img_par *img, int no_coeff,int *coeff);
void intrapred_luma_2(struct img_par *img);
int	 find_sad2(struct img_par *img,int *intra_mode);
void dct_luma2(struct img_par *img, int);

void init_conf(struct inp_par *inp, char *config_filename);
void init_img(struct inp_par *inp, struct img_par *img);
void init_stat(struct inp_par *inp, struct stat_par *stat);
void report(struct inp_par *inp, struct img_par *img, struct stat_par *stat, struct snr_par *snr);
void information_init(struct inp_par *inp, char *config_filename);
void init_frame(struct img_par *img, struct inp_par *inp, struct stat_par *stat);
void select_picture_type(struct img_par *img, struct inp_par *inp, SyntaxElement *symbol);
void read_one_new_frame(struct img_par *img, struct inp_par *inp);
void write_reconstructed_image(struct img_par *img, struct inp_par *inp);
void init_loop_filter(struct img_par *img);
int loop(struct img_par *img, int ibl, int ibr, int longFilt, int chroma);
void loopfilter(struct img_par *img);

/* GH: Added for dynamic mem allocation*/
int  get_mem4global_buffers(struct inp_par *inp, struct img_par *img); 
void free_mem4global_buffers(struct inp_par *inp, struct img_par *img); 
int  get_mem2D(byte ***array2D, int rows, int columns);
int  get_mem2Dint(int ***array2D, int rows, int columns);
int  get_mem3D(byte ****array2D, int frames, int rows, int columns);
int  get_mem3Dint(int ****array3D, int frames, int rows, int columns);
void no_mem_exit(int code);
/* end GH*/


/* DM: Added for (re-) structuring the TML soft */
int		encode_one_frame(struct img_par *img,struct inp_par *inp,struct stat_par *stat, struct snr_par *snr);
void	encode_one_slice(struct img_par *img,struct inp_par *inp,struct stat_par *stat, SyntaxElement *se);
void	malloc_slice(struct inp_par *inp, struct img_par *img);
void	free_slice(struct inp_par *inp, struct img_par *img);
void	init_slice(struct img_par *img, struct inp_par *inp);
void	encode_one_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat);
void	start_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat);

void	terminate_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat,
														Boolean *end_of_slice, Boolean *recode_macroblock);
void	write_one_macroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat);
void	proceed2nextMacroblock(struct img_par *img,struct inp_par *inp,struct stat_par *stat);
void	LumaResidualCoding_P(struct img_par *img,struct inp_par *inp);
void	ChromaCoding_P(int *cr_cbp, struct img_par *img);
void	SetRefFrameInfo_P(struct img_par *img);
void	SetLoopfilterStrength_P(struct img_par *img);
int		MakeIntraPrediction(struct img_par *img,struct inp_par *inp, int *intra_pred_mode_2);
void	CheckAvailabilityOfNeighbors(struct img_par *img);
void	writeMotionInfo2NAL_Pframe(struct img_par *img,struct inp_par *inp);
void	writeCBPandCoeffs2NAL(struct img_par *img,struct inp_par *inp);
int		writeStartcode2buffer(struct img_par *img, struct inp_par *inp, SyntaxElement *se);
void	writeEOS2buffer(struct img_par *img, struct inp_par *inp);
void	writeUVLC2buffer(SyntaxElement *se,  struct img_par *img, struct inp_par *inp, Bitstream *currStream);
int		writeSyntaxElement_UVLC(SyntaxElement *se, struct img_par *img, struct inp_par *inp, DataPartition *this_dataPart);
int		symbol2uvlc(SyntaxElement *se);
void	n_linfo(int n, int *len,int *info);
void	n_linfo2(int n, int dummy, int *len,int *info);
void	intrapred_linfo(int ipred1, int ipred2, int *len,int *info);
void	mvd_linfo2(int mvd, int dummy, int *len,int *info);
void	cbp_linfo_intra(int cbp, int dummy, int *len,int *info);
void	cbp_linfo_inter(int cbp, int dummy, int *len,int *info);
#if TRACE
void	trace2out(SyntaxElement *se);
#endif
Boolean	dummy_slice_too_big(int bits_slice); 
/* ~DM */

/* DM: Added for CABAC */
void arienco_start_encoding(EncodingEnvironmentPtr eep, unsigned char *code_buffer, int *code_len );
int	 arienco_bits_written(EncodingEnvironmentPtr eep);
void arienco_done_encoding(EncodingEnvironmentPtr eep);
void biari_init_context( BiContextTypePtr ctx, int ini_count_0, int ini_count_1, int max_cum_freq );
void biari_copy_context( BiContextTypePtr ctx_orig, BiContextTypePtr ctx_dest );
void biari_print_context( BiContextTypePtr ctx );
void rescale_cum_freq(BiContextTypePtr bi_ct);
void biari_encode_symbol(EncodingEnvironmentPtr eep, signed short symbol, BiContextTypePtr bi_ct );
MotionInfoContexts* create_contexts_MotionInfo(void);
TextureInfoContexts* create_contexts_TextureInfo(void);
void init_contexts_MotionInfo(struct img_par *img, MotionInfoContexts *enco_ctx, int ini_flag);
void init_contexts_TextureInfo(struct img_par *img, TextureInfoContexts *enco_ctx, int ini_flag);
void delete_contexts_MotionInfo(MotionInfoContexts *enco_ctx);
void delete_contexts_TextureInfo(TextureInfoContexts *enco_ctx);
void writeHeaderToBuffer(struct inp_par *inp, struct img_par *img);
void writeEOSToBuffer(struct inp_par *inp, struct img_par *img);
int	 writeSyntaxElement_CABAC(SyntaxElement *se, struct img_par *img, struct inp_par *inp, DataPartition *this_dataPart);
void writeMB_typeInfo2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, EncodingEnvironmentPtr eep_dp);
void writeIntraPredMode2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img,	EncodingEnvironmentPtr eep_dp);
void writeRefFrame2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, EncodingEnvironmentPtr eep_dp);
void writeMVD2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, EncodingEnvironmentPtr eep_dp);
void writeCBP2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img,	EncodingEnvironmentPtr eep_dp);
void writeRunLevel2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, EncodingEnvironmentPtr eep_dp);
void writeBiDirBlkSize2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, EncodingEnvironmentPtr eep_dp);
void writeBiMVD2Buffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, EncodingEnvironmentPtr eep_dp);
/* ~DM */

/* Introduced by TOM  */
void error(char *text);
int start_sequence(struct inp_par *inp);
int terminate_sequence(struct img_par *img, struct inp_par *inp);
int start_slice(struct img_par *img, struct inp_par *inp, SyntaxElement *sym);
int terminate_slice(struct img_par *img, struct inp_par *inp, struct stat_par  *stat);

/* B pictures */
void copy2mref(struct img_par *img);
int motion_search_Bframe(int tot_intra_sad, struct img_par *img, struct inp_par *inp);
int get_fwMV(int *min_fw_sad, struct img_par *img, struct inp_par *inp, int tot_intra_sad);
void get_bwMV(int *min_bw_sad, struct img_par *img, struct inp_par *inp_par);
void get_bid(int *bid_sad, int fw_predframe_no, struct img_par *img, struct inp_par *inp);
void get_dir(int *dir_sad, struct img_par *img, struct inp_par *inp);
void compare_sad(int tot_intra_sad, int fw_sad, int bw_sad, int bid_sad, int dir_sad, struct img_par *img);
void LumaResidualCoding_B(struct img_par *img);
void ChromaCoding_B(int *cr_cbp, struct img_par *img);
void SetLoopfilterStrength_B(struct img_par *img);
void SetRefFrameInfo_B(struct img_par *img);
void writeMotionInfo2NAL_Bframe(struct img_par *img,struct inp_par *inp);
int	 BlkSize2CodeNumber(int blc_size_h, int blc_size_v);


#endif
