/*!
 * \file
 *			VLC.c	
 *	\brief	
 *			VLC support functions
 *	\author
 *			Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 */
#include "contributors.h"

#include <math.h>
#include "global.h"
#include "vlc.h"

/*!
 *	\fn		linfo_levrun_inter()
 *  \return	Level and run for inter (through pointers)
 */
void linfo_levrun_inter(
	int len,		/*!< length of element in bits */
	int info,		/*!< infoword of element */
	int *level,		/*!< level value */
	int *irun)		/*!< run value */
{
	int l2;
	int inf;
	if (len<=9)
	{
		l2=mmax(0,len/2-1);
		inf=info/2;
		*level=NTAB1[l2][inf][0];
		*irun=NTAB1[l2][inf][1];
		if ((info&0x01)==1)
			*level=-*level;                   /* make sign */
	}
	else                                  /* if len > 9, skip using the array */
	{
		*irun=(info&0x1e)>>1;
		*level = LEVRUN1[*irun] + info/32 + (int)pow(2,len/2 - 5);
		if ((info&0x01)==1)
			*level=-*level;
	}
}


/*!
 *	\fn		linfo_levrun_intra()
 *  \return	Level and run for intra (through pointers)
 */

void linfo_levrun_intra(
	int len,		/*!< length of element in bits */
	int info,		/*!< infoword of element */
	int *level,		/*!< level value */
	int *irun)		/*!< run value */
{
	int l2;
	int inf;

	if (len<=9)
	{
		l2=mmax(0,len/2-1);
		inf=info/2;
		*level=NTAB2[l2][inf][0];
		*irun=NTAB2[l2][inf][1];
		if ((info&0x01)==1)
			*level=-*level;                 /* make sign */
	}
	else                                  /* if len > 9, skip using the array */
	{
		*irun=(info&0x0e)>>1;
		*level = LEVRUN2[*irun] + info/16 + (int)pow(2,len/2-4) -1;
		if ((info&0x01)==1)
			*level=-*level;
	}
}


/*!
 *	\fn		linfo_levrun_c2x2()
 *  \return	Level and run for 2x2 chroma (through pointers)
 */

void linfo_levrun_c2x2(
	int len,		/*!< length of element in bits */
	int info,		/*!< infoword of element */
	int *level,		/*!< level value */
	int *irun)		/*!< run value */
{
	int l2;
	int inf;

	if (len<=5)
	{
		l2=mmax(0,len/2-1);
		inf=info/2;
		*level=NTAB3[l2][inf][0];
		*irun=NTAB3[l2][inf][1];
		if ((info&0x01)==1)
			*level=-*level;                 /* make sign */
	}
	else                                  /* if len > 5, skip using the array */
	{
		*irun=(info&0x06)>>1;
		*level = LEVRUN3[*irun] + info/8 + (int)pow(2,len/2 - 3);
		if ((info&0x01)==1)
			*level=-*level;
	}
}


/*!
 *	\fn		linfo_mvd()
 *	\brief	Make signed value
 *  \return	signed motion vector
 */
int linfo_mvd(
	int len,		/*!< length of element in bits */
	int info)		/*!< infoword of element */
{
	int n;
	int signed_mvd;

	n = (int)pow(2,(len/2))+info-1;
	signed_mvd = (n+1)/2;
	if((n & 0x01)==0)                           /* lsb is signed bit */
		signed_mvd = -signed_mvd;

	return signed_mvd;
}
