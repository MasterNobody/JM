/*!
 *
 *  \file	elements.h
 *	\brief	Header file for elements in H.26L streams
 *	\date	6.10.2000
 *	\version	
 *			1.0
 *
 *  \author	Sebastian Purreiter     <sebastian.purreiter@mch.siemens.de>
 *
 *			Siemens AG, Information and Communication Mobile
 *			P.O.Box 80 17 07
 *			D-81617 Munich, Germany
 */

#ifndef _ELEMENTS_H_
#define _ELEMENTS_H_

/*!  
 *	definition of H.26L syntaxelements
 *  order of elements follow dependencies for picture reconstruction
 */
/*!
 * \brief	Assignment of old TYPE or partition elements to new
 *			elements
 *
 *	old element		| new elements
 *  ----------------+-------------------------------------------------------------------
 *	TYPE_HEADER     | SE_HEADER, SE_PTYPE
 *  TYPE_MBHEADER   | SE_MBTYPE, SE_REFFRAME, SE_INTRAPREDMODE
 *	TYPE_MVD		| SE_MVD			
 *	TYPE_CBP		| SE_CBP_INTRA, SE_CBP_INTER
 *	TYPE_COEFF_Y    | SE_LUM_DC_INTRA, SE_LUM_AC_INTRA, SE_LUM_DC_INTER, SE_LUM_AC_INTER
 *  TYPE_2x2DC      | SE_CHR_DC_INTRA, SE_CHR_DC_INTER
 *  TYPE_COEFF_C    | SE_CHR_AC_INTRA, SE_CHR_AC_INTER
 *  TYPE_EOS        | SE_EOS
*/

#define SE_HEADER			0	
#define SE_PTYPE			1
#define SE_MBTYPE			2
#define SE_REFFRAME			3
#define SE_INTRAPREDMODE	4
#define SE_MVD				5
#define SE_CBP_INTRA		6
#define SE_LUM_DC_INTRA		7
#define SE_CHR_DC_INTRA		8
#define SE_LUM_AC_INTRA		9
#define SE_CHR_AC_INTRA		10
#define SE_CBP_INTER		11
#define SE_LUM_DC_INTER		12
#define SE_CHR_DC_INTER		13
#define SE_LUM_AC_INTER		14
#define SE_CHR_AC_INTER		15
#define SE_EOS				16	/*!< End of Slice => use same partition as SE_PTYPE */
#define SE_MAX_ELEMENTS		17  /*!< number of maximum syntax elements */

#define MAXPARTITIONMODES	3	/*!< maximum possible partition modes as defined in assignSE2partition[][] */	

/*!
 *	\brief	lookup-table to assign different elements to partition
 *
 *  \note	here we defined up to 6 different partitions similar to
 *			document Q15-k-18 described in the PROGFRAMEMODE.
 *			The Sliceheader contains the PSYNC information. \par
 *		
 *			Elements inside a partition are not ordered. They are
 *			ordered by occurence in the stream.
 *			Assumption: Only partitionlosses are considered. \par
 *		
 *			The texture elements luminance and chrominance are
 *			not ordered in the progressive form
 *			This may be changed in image.c \par
 *
 *			We also defined the proposed internet partition mode
 *			of Stephan Wenger here. To select the desired mode
 *			uncomment one of the two following lines. \par
 *
 *	-IMPORTANT:
 *			Picture- or Sliceheaders must be assigned to partition 0. \par
 *			Furthermore partitions must follow syntax dependencies as
 *			outlined in document Q15-J-23.
 */

static int assignSE2partition[][SE_MAX_ELEMENTS] = 
{
	// 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16		// elementnumber (no not uncomment)
	{  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },		/*!< all elements in one partition */
	{  0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 0 },		/*!< H.223 partition mode */
	{  0, 0, 1, 1, 1, 2, 3, 4, 5, 4, 6, 3, 4, 5, 4, 6, 7 }		/*!< internet partition mode */
};

int PartitionMode;

#endif