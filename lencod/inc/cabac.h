/***************************************************************************
 *
 * Modul       :  cabac.h
 *
 * Author      :  Detlev Marpe
 *
 * Date        :  21. Oct 2000
 *
 * Description :  Headerfile for entropy coding routines
 *                
 *      Copyright (C) 2000 HEINRICH HERTZ INSTITUTE All Rights Reserved.
 *		  
 **************************************************************************/


#ifndef _CABAC_H_
#define _CABAC_H_

#include "global.h"


/*******************************************************************************************
 * l o c a l    c o n s t a n t s   f o r   i n i t i a l i z a t i o n   o f   m o d e l s
 *******************************************************************************************
 */

static const int MB_TYPE_Ini[2][10][3]=
{
	{{8,1,50},	{2,1,50}, {1,1,50},	{1,5,50},	{1,1,50},	{1,1,50},	{2,1,50}, {2,1,50},	{1,1,50}, {1,1,50}},
	{{8,1,50},	{1,1,50}, {1,2,50},	{1,10,50},	{1,1,50},	{10,1,50},	{2,1,50},	{2,1,50}, {2,1,50},	{2,1,50}}
};

static const int MV_RES_Ini[2][10][3]=
{
	{{3,1,50},	{1,1,50},	{1,3,50},	{1,2,50},	{1,1,50},  {3,1,50},	{1,1,50},	{1,3,50},	{1,2,50}, {1,1,50}},
	{{2,1,50},	{1,2,50},	{1,3,50},	{1,7,50},	{1,1,50},  {2,1,50},	{1,2,50},	{1,3,50},	{1,7,50}, {1,1,50}}
};

static const int REF_NO_Ini[6][3]=
{
	{10,1,50},	{2,1,50},	{1,1,50},	{1,3,50},	{2,1,50}, {1,1,50}
};

static const int CBP_Ini[2][3][4][3]=
{
	{ {{1,7,50},	{1,2,50},	{1,2,50},	{2,3,50}},
		{{1,2,50},	{1,2,50},	{1,2,50},	{1,7,50}},
		{{2,1,50},	{1,2,50},	{1,2,50},	{1,2,50}} }, //intra cbp
	{ {{1,1,50},	{2,1,50},	{2,1,50},	{5,1,50}},
		{{2,1,50},	{2,1,50},	{1,1,50},	{1,1,50}},
		{{1,1,50},	{2,3,50},	{2,3,50},	{2,3,50}} }	//inter cbp
};


static const int IPR_Ini[6][2][3]=
{
	{{2,1,50},	{1,1,50}},
	{{3,2,50},	{1,1,50}},
	{{1,1,50},	{2,3,50}},
	{{1,1,50},	{2,3,50}},
	{{1,1,50},	{1,1,50}},
	{{2,3,50},	{1,1,50}}
};

static const int Run_Ini[9][2][3]=
{
	{{3,1,50},	{3,1,50}},// double scan
	{{6,5,50},	{2,3,50}},// single scan, inter
	{{2,1,50},	{3,4,50}},// single scan, intra
	{{2,1,50},	{2,1,50}},// 16x16 DC
	{{3,2,50},	{1,1,50}},// 16x16 AC
	{{2,1,50},	{3,1,50}},// chroma inter DC
	{{4,1,50},	{3,1,50}},// chroma intra DC
	{{3,2,50},	{4,5,50}},// chroma inter AC
	{{4,3,50},	{2,1,50}}// chroma intra AC
};

static const int Level_Ini[9][4][3]=
{
	{{2,3,50},	{3,1,50},	{3,2,50},		{1,1,50}},// double scan
	{{2,4,50},	{6,1,50},	{1,1,50},		{1,1,50}},// single scan, inter
	{{1,1,50},	{6,1,50},	{3,1,50},		{1,1,50}},// single scan, intra
	{{1,2,50},	{2,1,50},	{1,1,50},		{1,1,50}},// 16x16 DC
	{{6,1,50},	{9,1,50},	{1,1,50},		{1,1,50}},// 16x16 AC
	{{4,3,50},	{7,1,50},	{2,1,50},		{1,1,50}},// chroma inter DC
	{{3,4,50},	{8,3,50},	{1,1,50},		{1,1,50}}, // chroma intra DC
	{{7,6,50},	{5,1,50},	{2,1,50},		{1,1,50}},// chroma inter AC
	{{2,1,50},	{5,1,50},	{3,1,50},		{1,1,50}}// chroma intra AC
};


/***********************************************************************
 * L O C A L L Y   D E F I N E D   F U N C T I O N   P R O T O T Y P E S   
 ***********************************************************************
 */



void unary_bin_encode(EncodingEnvironmentPtr eep_frame,
											unsigned int symbol,
											BiContextTypePtr ctx,
											int ctx_offset);

void unary_bin_max_encode(EncodingEnvironmentPtr eep_frame,
											unsigned int symbol,
											BiContextTypePtr ctx,
											int ctx_offset,
											unsigned int max_symbol);

void unary_level_encode(EncodingEnvironmentPtr eep_frame,
											unsigned int symbol,
											BiContextTypePtr ctx);

void unary_mv_encode(EncodingEnvironmentPtr eep_frame,
												unsigned int symbol,
												BiContextTypePtr ctx,
												unsigned int max_bin);


#endif 	/* CABAC_H */

