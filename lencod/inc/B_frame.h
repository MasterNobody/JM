/****************************************************************
 *
 *  File B_frame.h :	Header file for B picture coding 
 *
 *	Main contributor and Contact Information
 *
 *		Byeong-Moon Jeon			<jeonbm@lge.com>
 *      LG Electronics Inc., Digital Media Research Lab.
 *      16 Woomyeon-Dong, Seocho-Gu, Seoul, 137-724, Korea
 *
 *****************************************************************/

#define SINGLE_SCAN			0
#define DOUBLE_SCAN			1

#define MAX_DIR_SAD			100000000
#define SEB					100   // used at put_symbol(....., SEB);

extern int ONE_FOURTH_TAP[3][2];
extern int  tmp_mv[2][72][92];
extern int  MODTAB[3][2];
extern int NCBP[48][2];
extern byte PRED_IPRED[7][7][6];
extern int IPRED_ORDER[6][6]; 
extern int JQ[32][2];
extern int QP2QUANT[32];

int **fw_refFrArr, ** bw_refFrArr;
byte mref_P[1152][1408];     /* 1/4 pix luma for next P picture*/
byte mcef_P[2][352][288];    /* pix chroma for next P picture*/

extern void ResetSlicePredictions(struct img_par *img);

// motion vector : forward, backward, direct
int ***tmp_fwMV; //[2][72][92];
int ***tmp_bwMV; //[2][72][92];
int ***dfMV;  //[2][72][92];
int ***dbMV;  //[2][72][92];

