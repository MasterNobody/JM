/*!
 *
 *  \file	errorconcealment.c
 *	\brief	Implements error concealment scheme for H.26L decoder
 *	\date	6.10.2000
 *	\version	
 *			1.0
 *
 *	\note	This simple error concealment implemented in this decoder uses 
 *			the existing dependencies of syntax elements.
 *			In case that an element is detected as false this elements and all
 *			dependend elements are marked as elements to conceal in the ec_flag[]
 *			array. If the decoder requests a new element by the function 
 *			get_symbol() this array is checked first if an error concealment has 
 *			to be applied on this element. 
 *			In case that an error occured a concealed element is given to the 
 *			decoding function in macroblock().
 *
 *	\author Main contributors (see contributors.h for copyright, address and affiliation details)
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>		
 *
 *	\note	tags are used for document system "doxygen"
 *			available at http://www.stack.nl/~dimitri/doxygen/index.html
 */

#include <stdio.h>

#include "contributors.h"
#include "elements.h"


/*!
 *	\fn		set_ec_flag
 *  \brief	set concealment for all elements in same partition
 *			and dependend syntax elements
 *	\return EC_REQ,	elements of same type or depending type need error concealment.
 *			EX_SYNC, sync on next header
 */
int set_ec_flag(
	int se)		/*!< type of syntax element to conceal */
{

	if (ec_flag[se] == NO_EC)
		printf("Error concealment on element %s\n",SEtypes[se]);

	switch (se)
	{
	case SE_HEADER :
		ec_flag[SE_HEADER] = EC_REQ;
	case SE_PTYPE :
		ec_flag[SE_PTYPE] = EC_REQ;
	case SE_MBTYPE :
		ec_flag[SE_MBTYPE] = EC_REQ;

	case SE_REFFRAME :
		ec_flag[SE_REFFRAME] = EC_REQ;
		ec_flag[SE_MVD] = EC_REQ;	/* set all motion vectors to zero length */
		se = SE_CBP_INTER;			/* conceal also Inter texture elements */
		break;

	case SE_INTRAPREDMODE :
		ec_flag[SE_INTRAPREDMODE] = EC_REQ;
		se = SE_CBP_INTRA;			/* conceal also Intra texture elements */
		break;
	case SE_MVD :
		ec_flag[SE_MVD] = EC_REQ;
		se = SE_CBP_INTER;			/* conceal also Inter texture elements */
		break;

	default:
		break;
	}

	switch (se)
	{
	case SE_CBP_INTRA :
		ec_flag[SE_CBP_INTRA] = EC_REQ;
	case SE_LUM_DC_INTRA :
		ec_flag[SE_LUM_DC_INTRA] = EC_REQ;
	case SE_CHR_DC_INTRA :
		ec_flag[SE_CHR_DC_INTRA] = EC_REQ;
	case SE_LUM_AC_INTRA :
		ec_flag[SE_LUM_AC_INTRA] = EC_REQ;
	case SE_CHR_AC_INTRA :
		ec_flag[SE_CHR_AC_INTRA] = EC_REQ;
		break;

	case SE_CBP_INTER :
		ec_flag[SE_CBP_INTER] = EC_REQ;
	case SE_LUM_DC_INTER :
		ec_flag[SE_LUM_DC_INTER] = EC_REQ;
	case SE_CHR_DC_INTER :
		ec_flag[SE_CHR_DC_INTER] = EC_REQ;
	case SE_LUM_AC_INTER :
		ec_flag[SE_LUM_AC_INTER] = EC_REQ;
	case SE_CHR_AC_INTER :
		ec_flag[SE_CHR_AC_INTER] = EC_REQ;
		break;

	default:
		break;

	}
	return EC_REQ;
}

/*!
 *	\fn		reset_ec_flags
 *  \brief	reset concealment flags for all elements
 */

void reset_ec_flags()
{
	int i;
	for (i=0; i<SE_MAX_ELEMENTS; i++)
		ec_flag[i] = NO_EC;
}


/*!
 *	\fn		get_concealed_element
 *  \brief	get error concealed element in dependence of syntax
 *			element se.
 *			This function implements the error concealment.
 *  \return NO_EC if no error concealment required
 *          EC_REQ if element requires error concealment
 */

int get_concealed_element(
	int *len,		/*!< length of element (modified) */
	int *info,		/*!< infoword of element (modified) */
	int se)			/*!< type of syntax element */
{
	if (ec_flag[se] == NO_EC)
		return NO_EC;

#if TRACE
	printf("TRACE: get concealed element for %s!!!\n", SEtypes[se]);
#endif

	switch (se)
	{
	case SE_HEADER :
		*len = 31;
		*info = 0;
		break;

	case SE_PTYPE :	/* inter_img_1 */
	case SE_MBTYPE :
	case SE_REFFRAME :
		*len = 1;   /* set COPY_MB */
		*info = 0;
		break;

	case SE_INTRAPREDMODE :
	case SE_MVD :
		*len = 1;
		*info = 0;  /* set vector to zero length */
		break;

	case SE_CBP_INTRA :
		*len = 5;
		*info = 0; /* codenumber 3 <=> no CBP information for INTRA images */
		break;

	case SE_LUM_DC_INTRA :
	case SE_CHR_DC_INTRA :
	case SE_LUM_AC_INTRA :
	case SE_CHR_AC_INTRA :
		*len = 1;
		*info = 0;  /* return EOB */
		break;

	case SE_CBP_INTER :
		*len = 1;
		*info = 0; /* codenumber 1 <=> no CBP information for INTER images */
		break;

	case SE_LUM_DC_INTER :
	case SE_CHR_DC_INTER :
	case SE_LUM_AC_INTER :
	case SE_CHR_AC_INTER :
		*len = 1;
		*info = 0;  /* return EOB */
		break;

	default:
		break;
	}

	return EC_REQ;
}
