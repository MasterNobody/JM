/*!
 * 	\file 	
 *			NAL4H223.h
 * 	\brief	
 *			Header file for NAL
 *	\date	
 *			22.10.2000
 *	\version	
 *			1.0
 * 	\author
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 */

#ifndef _NAL_4_H223_H_
#define _NAL_4_H223_H_

typedef unsigned char byte;

#define TRUE 	1
#define FALSE 	0

#define NO_PARTITION_CONTENT	111		/* no contnet in partition indicator */

#define MAXPARTITION		17		/* Maximum partition ID */
#define MAXSIZEPARTITION	10000	/* Maximum bytes per partition */

#define EOS_MASK			0x01	/* mask for end of sequence (bit 1)         */
#define ICIF_MASK			0x02	/* mask for image format (bit 2)            */
#define QP_MASK				0x7C	/* mask for quant.parameter (bit 3->7)      */
#define TR_MASK				0x7f80	/* mask for temporal referance (bit 8->15)  */

/*!
 *	\brief defines the interim fileformat of TML5.2 encoded files
 */

typedef struct {
  	int partitionsizebits;			/*!< 4 bytes; length of VLC coded data stream d in bits */
	int partitionID;				/*!< 4 bytes; Partition ID*/
  	int picID;                  	/*!< 4 bytes; Picture ID */
	int sliceID;                	/*!< 4 bytes; Slice ID */
  	int startMB;                	/*!< 4 bytes; Macroblock number where slice begins */
  	int qp;                     	/*!< 4 bytes; quantization parameter over slice */
  	int picFormat;					/*!< 4 bytes; picture format 0=CIF, 1=QCIF */
  	byte data[MAXSIZEPARTITION];	/*!< partitionsizebits/8 bytes; VLC coded data stream */
} PartitionInfo;

PartitionInfo p[MAXPARTITION];

/*!
 * assignment of partitionID to a logical channel
 */
static int partition2logicalchannel[2][MAXPARTITION] =
{
	//	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10,	11,	13,	14,	15,	16;		= partition number
	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0},		/*!< H.223 Annex B */
	{	0,	1,	2,	3,	4,	5,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0}		/*!< H.223 Annex D */
};

#endif