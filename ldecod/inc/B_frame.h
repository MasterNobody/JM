/****************************************************************
 *
 *  File B_frame.h :	Header file for B picture decoding 
 *
 *	Main contributor and Contact Information
 *
 *		Byeong-Moon Jeon			<jeonbm@lge.com>
 *      LG Electronics Inc., Digital Media Research Lab.
 *      16 Woomyeon-Dong, Seocho-Gu, Seoul, 137-724, Korea
 *
 *****************************************************************/

#define		SINGLE_SCAN 0
#define		DOUBLE_SCAN 1

//#define		SEB					100   // used at put_symbol(....., SEB);

extern int ONE_FOURTH_TAP[3][2];
extern const byte NCBP[48][2];
extern byte PRED_IPRED[7][7][6];
extern const byte IPRED_ORDER[36][2]; 
extern const int BLOCK_STEP[8][2];
extern const byte SNGL_SCAN[16][2];
extern const int JQ1[];
extern const byte DBL_SCAN[8][2][2];
extern const byte QP_SCALE_CR[32];

extern int two[6];
extern int three[6];
extern int five[6][6];
extern int six[6][6];
extern int seven[6][6];

  extern void ResetSlicePredictions(struct img_par *img);

int **fw_refFrArr, ** bw_refFrArr;
byte imgY_prev[288][352];
byte imgUV_prev[2][144][176];
byte mref_P[1152][1408];     /* 1/4 pix luma for next P picture*/
byte mcef_P[2][288][352];    /* pix chroma for next P picture*/

byte mref_P_small[288][352];     /* 1/4 pix luma for next P picture*/

int ***dfMV;  //[92][72][2] 
int ***dbMV;  //[92][72][2] 

