/*!
 *
 *  \file	nal_part.h
 *	\brief	Header file for nal_part.c
 *	\version	
 *			1.0
 *
 *	\author Main contributors (see contributors.h for copyright, address and affiliation details)
 *		Stephan Wenger			<stewe@cs.tu-berlin.de>
 *		Rickard Sjoberg			<rickard.sjoberg@era.ericsson.se>
 */

#define MAXPARTITION		23		/* Maximum partition ID */
#define MAXSIZEPARTITION	10000	/* Maximum bytes per partition */
#define MAXSYMBOLS			30000

/*!
 * \brief	struct defining partition information
 */

typedef struct {
	int PicID;
	int SliceID;
	int StartMB;
	unsigned int byteptr;
	unsigned int bitcount;
	unsigned int Symbols;
	int byte;			    
	int bitcounter;			
	byte d[MAXSIZEPARTITION];
} PInfo;
