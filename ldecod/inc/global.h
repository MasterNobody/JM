/************************************************************************
*
*  global.h for H.26L decoder.
*  Copyright (C) 1999  Telenor Satellite Services, Norway
*                      Ericsson Radio Systems, Sweden
*
*  Contacts:
*  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
*
*
*  Telenor Satellite Services
*  P.O.Box 6914 St.Olavs plass
*  N-0130 Oslo, Norway
*
*  Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
*
*  Ericsson Radio Systems
*  KI/ERA/T/VV
*  164 80 Stockholm, Sweden
************************************************************************/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <stdio.h>                              /* for FILE */
#include "defines.h"

typedef unsigned char   byte;                   /*  8 bit unsigned */


/* GH: global picture format dependend buffers, mem allocation in decod.c *******************/ 
int **refFrArr;                                 /* Array for reference frames of each block */
byte **imgY;                                    /* array for the decoded luma component     */
byte **imgY_pf;                                 /* Post filter luma image                   */
byte ***imgUV;                                  /* array for the chroma component           */
byte ***imgUV_pf;                               /* Post filter luma image                   */

byte ***mref;                                   /* 1/1 pix luma for direct interpolation	*/
byte ****mcef;		    	                    /* pix chroma								*/

byte **loopb;                                   /* Array for loop filter strength luma      */
byte **loopc;                                   /* Array for loop filter strength chroma    */

// B pictures
byte **imgY_prev;
byte ***imgUV_prev;
// byte **mref_P;                                  /* 1/4 pix luma for next P picture*/
// byte ***mcef_P;                                 /* pix chroma for next P picture*/

byte **mref_P_small;                            /* 1/4 pix luma for next P picture*/

byte **imgY_ref;                                /* reference frame find snr */
byte ***imgUV_ref;

byte **imgY_tmp;                                /* temp luma image loop_filter*/
byte ***imgUV_tmp;                              /* temp chroma image loop_filter*/

//B pictures
int  Bframe_ctr;
byte prevP_tr, nextP_tr, P_interval;
int  frame_no;

char errortext[300];

/***********************************************************************
 * T y p e    d e f i n i t i o n s    f o r    T M L  
 ***********************************************************************
 */

//typedef unsigned char byte;

/* Introduced by TOM */
/* Data Partitioning Modes */
typedef enum
{
	PAR_DP_1    /* no data partitioning is supported    */
} PAR_DP_TYPE;


/* Output File Types */
typedef enum
{
	PAR_OF_26L   /* Current TML description             */
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
    unsigned int    Dlow, Dhigh;
    unsigned int    Dvalue;
    unsigned int    Dbuffer;
    int             Dbits_to_go;
    byte            *Dcodestrm;
    int             *Dcodestrm_len; 
} DecodingEnvironment;                 /* struct to characterize the state of the arithmetic coding engine */

typedef DecodingEnvironment *DecodingEnvironmentPtr;

typedef struct 
{
    unsigned int	cum_freq[2];            /* cumulated frequency counts */
    Boolean			in_use;                 /* flag for context in use */
	unsigned int	max_cum_freq;		    /* maximum frequency count */ 
} BiContextType;							/* struct for context management */

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

struct img_par;
struct inp_par;
struct stat_par;

typedef struct syntaxelement 
{
	int						type;				/* type of syntax element for data part.*/
	int						value1;			    /* numerical value of syntax element */
	int						value2;			    /* for blocked symbols, e.g. run/level */
	int						len;				/* length of code */
	int						inf;				/* info part of UVLC code */
	unsigned int		    bitpattern;	        /* UVLC bitpattern */
	int					    context;		    /* CABAC context */
#if TRACE
	char					tracestring[100];	/* trace string	*/
#endif
    /* for mapping of UVLC to syntaxElement */
	void    (*mapping)(int len, int info, int *value1, int *value2); 
    /* used for CABAC: refers to actual coding method of each individual syntax element type */				
    void	(*reading)(struct syntaxelement *, struct inp_par *, struct img_par *, DecodingEnvironmentPtr);	
}	SyntaxElement; 

typedef struct macroblock
{
	int						slice_nr;			/* slice number to which the MB belongs */
	int						qp;					/* for future use */
	int						intraOrInter;
	struct macroblock		*mb_available[3][3];/* pointer to neighboring MBs in a 3x3 window of current MB, which is located at [1][1] */
												/* NULL pointer identifies neighboring MBs which are unavailable */ 
		
	/* some storage of macroblock syntax elements for global access */
	int						mb_type;
    int						mb_imode;
	int						ref_frame;
    int                     predframe_no;
	int						mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];			/* indices correspond to [forw,backw][block_y][block_x][x,y] */
	int						intra_pred_modes[BLOCK_MULTIPLE*BLOCK_MULTIPLE];
	int						cbp;
} Macroblock;


typedef struct 
{
    /* CABAC Decoding */
    int             read_len;                   // actual position in the codebuffer
    int             code_len;                   // overall codebuffer length
	byte			*streamBuffer;			    // actual codebuffer for read bytes
    /* UVLC Decoding */
	int				frame_bitoffset;
	int				bitstram_length;
} Bitstream;


typedef struct datapartition 
{

	Bitstream						*bitstream;
	DecodingEnvironment de_cabac;
	
	int     (*readSyntaxElement)(SyntaxElement *, struct img_par *, struct inp_par *, struct datapartition *); 

} DataPartition;

typedef struct 
{
	int							picture_id;
	int							slice_nr;    // not necessary but o.k.
	int							qp;
	int							picture_type; // picture type
	int							start_mb_nr;
	int							dp_mode;     // data partioning mode
	int							max_part_nr; // number of different partitions
	int							eos_flag;	 // end of sequence flag
	int                         next_header; 
	int							last_mb_nr;
	DataPartition				*partArr;    // array of partitions
	MotionInfoContexts			*mot_ctx;    // pointer to struct of context models for use in CABAC
	TextureInfoContexts			*tex_ctx;	 // pointer to struct of context models for use in CABAC
   	int     (*readSlice)(struct img_par *, struct inp_par *);  												


} Slice;
/******************************* ~DM ************************************/

/* image parameters */
struct img_par
{
    int number;                                  /* frame number                              */
    int current_mb_nr;
    int max_mb_nr;
    int current_slice_nr;
    int *slice_numbers;
    int tr;                                     /* temporal reference, 8 bit, wrapps at 255  */
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
    int block_x;
    int pix_c_x;
    int mb_mode;
    int imod;                                    /* MB mode */
    int ***mv;                                   //[92][72][3];
    int mpr[16][16];                             /* predicted block */

    int m7[4][4];                                /* final 4x4 block                         */
    int cof[4][6][4][4];                         /* correction coefficients from predicted  */
    int cofu[4];
    int **ipredmode;                             //[90][74];/* prediction type  *//* fix from ver 4.1 */
    int quad[256];
//    int lpfilter[6][601];

    byte li[8];                                 /* 8 pix input to loopfilter routine */
    byte lu[8];                                 /* 8 pix output from loopfilter routine */

    int ***dfMV;                                //[92][72][3]; 
    int ***dbMV;                                //[92][72][3];
    int **fw_refFrArr;                          //[72][88];
    int **bw_refFrArr;                          //[72][88];
    int ref_frame;

    /* B pictures */
    int ***fw_mv;                                //[92][72][3];
    int ***bw_mv;                                //[92][72][3];
    int fw_multframe_no;
    int fw_blc_size_h;
    int fw_blc_size_v;
	int bw_multframe_no;
	int bw_blc_size_h;
	int bw_blc_size_v;
    /* DM: 030501 */
	Slice				*currentSlice;			    /* pointer to current Slice data struct */
	Macroblock	        *mb_data;			        /* array containing all MBs of a whole frame */ 
    int subblock_x;
    int subblock_y;
	/* ~DM */

	int mv_res;
	int buf_cycle;
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

int tot_time;

/* input parameters from configuration file */
struct inp_par
{
    char infile[100];                             /* Telenor H.26L input                     */
    char outfile[100];                            /* Decoded YUV 4:2:0 output                */
    char reffile[100];                            /* Optional YUV 4:2:0 reference file for SNR measurement */
    int  postfilter;                              /* postfilter (0=OFF,1=ON)                 */
    /* Introduced by TOM */
    int symbol_mode;			                  /* Specifies the mode the symbols are mapped on bits */
    int of_mode;					              /* Specifies the mode of the output file             */
	int partition_mode;		                      /* Specifies the mode of data partitioning */
	int buf_cycle;				/* Frame buffer size */
};


/* files */
FILE *p_out;                    /* pointer to output YUV file                       */
FILE *p_ref;                    /* pointer to input original reference YUV file file*/
FILE *p_log;                    /* SNR file */
FILE *p_datpart;		        /* file to write bitlength and id of all partitions */

#if TRACE
FILE *p_trace;
#endif

/* prototypes */
void    init_conf(struct inp_par *inp, char *config_filename);
void    report(struct inp_par *inp, struct img_par *img, struct snr_par *snr);
void    find_snr(struct snr_par *snr,struct img_par *img, FILE *p_ref, int postfilter);
void    init(struct img_par *img);

void	malloc_slice(struct inp_par *inp, struct img_par *img);
void	free_slice(struct inp_par *inp, struct img_par *img);

int     decode_one_frame(struct img_par *img,struct inp_par *inp, struct snr_par *snr);
void    init_frame(struct img_par *img, struct inp_par *inp);
void    exit_frame(struct img_par *img, struct inp_par *inp);
void    init_loop_filter(struct img_par *img);
void    loopfilter(struct img_par *img);
int     loop(struct img_par *img, int ibl, int ibr, int longFilt, int chroma);
void    write_frame(struct img_par *img,int,FILE *p_out);
void    write_prev_Pframe(struct img_par *img,FILE *p_out);// B pictures
void    copy_Pframe(struct img_par *img,int);// B pictures


int     read_new_slice(struct img_par *img, struct inp_par *inp);
void    decode_one_slice(struct img_par *img,struct inp_par *inp);

void    start_macroblock(struct img_par *img,struct inp_par *inp);
void    init_macroblock_Bframe(struct img_par *img);// B pictures
int     read_one_macroblock(struct img_par *img,struct inp_par *inp);
int     read_one_macroblock_Bframe(struct img_par *img,struct inp_par *inp);// B pictures
int     decode_one_macroblock(struct img_par *img,struct inp_par *inp);
int     decode_one_macroblock_Bframe(struct img_par *img);// B pictures
void    decode_one_CopyMB(struct img_par *img,struct inp_par *inp);
int     exit_macroblock(struct img_par *img,struct inp_par *inp);

void    readMotionInfoFromNAL_Bframe(struct img_par *img,struct inp_par *inp);// B pictures
void    readMotionInfoFromNAL_Pframe(struct img_par *img,struct inp_par *inp);
void    readCBPandCoeffsFromNAL(struct img_par *img,struct inp_par *inp);

void    itrans(struct img_par *img,int ioff,int joff,int i0,int j0);
int     intrapred(struct img_par *img,int ioff,int joff,int i4,int j4);
void	itrans_2(struct img_par *img);
int		intrapred_luma_2(struct img_par *img,int predmode);


/* UVLC mapping */
void    linfo(int len, int info, int *value1, int *dummy);
void    linfo_mvd(int len,int info, int *signed_mvd, int *dummy);
void    linfo_cbp_intra(int len,int info,int *cbp, int *dummy);
void    linfo_cbp_inter(int len,int info,int *cbp, int *dummy);
void    linfo_levrun_inter(int len,int info,int *level,int *irun);
void    linfo_levrun_intra(int len,int info,int *level,int *irun);
void    linfo_levrun_c2x2(int len,int info,int *level,int *irun);

int     readSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dp);
int     readSliceUVLC(struct img_par *img, struct inp_par *inp);


/* Direct interpolation */
byte    get_pixel(int ref_frame,int x_pos, int y_pos, struct img_par *img);
byte    get_quarterpel_pixel(int ref_frame,int x_pos, int y_pos, struct img_par *img);
byte    get_eighthpel_pixel(int ref_frame,int x_pos, int y_pos, struct img_par *img);
byte    interpolate(byte container[8],int modus);
void    copy2mref(struct img_par *img);
void    copy2mref_1(struct img_par *img);
void    copy2mref_2(struct img_par *img);

int	GetVLCSymbol (byte buffer[],int totbitoffset,int *info, int bytecount);
int PictureHeader(struct img_par *img, struct inp_par *inp);
int SliceHeader(struct img_par *img, struct inp_par *inp);

//int		inter_intra(struct img_par *img);

/* SLICE function pointers */
int  (*nal_startcode_follows) ();

/* NAL functions TML/CABAC bitstream*/
int  uvlc_startcode_follows();
int cabac_startcode_follows();

/* Error concealment */
void reset_ec_flags();

/* DM: Added for CABAC */
void    arideco_start_decoding(DecodingEnvironmentPtr dep, unsigned char *code_buffer, int firstbyte, int *code_len );
int     arideco_bits_read(DecodingEnvironmentPtr dep);
void    arideco_done_decoding(DecodingEnvironmentPtr dep);
void    biari_init_context( BiContextTypePtr ctx, int ini_count_0, int ini_count_1, int max_cum_freq );
void    biari_copy_context( BiContextTypePtr ctx_orig, BiContextTypePtr ctx_dest );
void    biari_print_context( BiContextTypePtr ctx );
void    rescale_cum_freq(BiContextTypePtr bi_ct);
unsigned int biari_decode_symbol(DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct );
MotionInfoContexts* create_contexts_MotionInfo(void);
TextureInfoContexts* create_contexts_TextureInfo(void);
void    init_contexts_MotionInfo(struct img_par *img, MotionInfoContexts *enco_ctx, int ini_flag);
void    init_contexts_TextureInfo(struct img_par *img, TextureInfoContexts *enco_ctx, int ini_flag);
void    delete_contexts_MotionInfo(MotionInfoContexts *enco_ctx);
void    delete_contexts_TextureInfo(TextureInfoContexts *enco_ctx);


void    readMB_typeInfoFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void    readIntraPredModeFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp,struct img_par *img, DecodingEnvironmentPtr dep_dp);
void    readRefFrameFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void    readMVDFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void    readCBPFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void    readRunLevelFromBuffer_CABAC(SyntaxElement *se, struct inp_par *inp, struct img_par *img,	DecodingEnvironmentPtr dep_dp);

void    readMVD2Buffer_CABAC(SyntaxElement *se,	struct inp_par *inp, struct img_par *img, DecodingEnvironmentPtr dep_dp);
void    readBiMVD2Buffer_CABAC(SyntaxElement *se,struct inp_par *inp,struct img_par *img,DecodingEnvironmentPtr dep_dp);
void    readBiDirBlkSize2Buffer_CABAC(SyntaxElement *se,struct inp_par *inp,struct img_par *img,DecodingEnvironmentPtr dep_dp);

int     readSliceCABAC(struct img_par *img, struct inp_par *inp);
int	    readSyntaxElement_CABAC(SyntaxElement *se, struct img_par *img, struct inp_par *inp, DataPartition *this_dataPart);
/* ~DM */

/* Introduced by TOM  */
void    error(char *text);
void    start_slice(struct img_par *img, struct inp_par *inp);
int     terminate_slice(struct img_par *img, struct inp_par *inp, struct stat_par  *stat);

/* GH: Added for dynamic mem allocation*/
int     get_mem4global_buffers(struct inp_par *inp, struct img_par *img); 
void    free_mem4global_buffers(struct inp_par *inp, struct img_par *img); 
int     get_mem2D(byte ***array2D, int rows, int columns);
int     get_mem2Dint(int ***array2D, int rows, int columns);
int     get_mem3D(byte ****array2D, int frames, int rows, int columns);
int     get_mem3Dint(int ****array3D, int frames, int rows, int columns);
void    no_mem_exit(int code);
/* end GH*/


#endif

