/*!
 *
 *  \file	nal_part.h
 *	\brief	Header fiel for nal_part.c
 *	\version	
 *			1.0
 *
 *	\author Main contributors (see contributors.h for copyright, address and affiliation details)
 *		Stephan Wenger			<stewe@cs.tu-berlin.de>
 */

#ifndef _NAL_PART_H_
#define _NAL_PART_H_

#define MAXPARTITION			23		/*!< Maximum partitions */
#define MAXSIZEPARTITION		10000	/*!< Maximum bytes per partition */
#define MAXSYMBOLS				30000	/*!< Maximum symbols per partition */

#define NO_ASSIGNMENT_POSSIBLE	111		/*!< no assignment to partition-id possible */

int partition_assignment[MAXPARTITION];	/*!< array resolving incoming partition number with partition-ID*/

static void MallocPartition (int PType);
static void FreePartition (int PType);
static int GetSlice (FILE *in);
static void ConvertPartition (int dt);
static int ReadSlice (FILE *p_in);

/*!
 * \brief	struct defining partition information
 */

typedef struct {
  int PicID;					/*!< Picture ID */
  int SliceID;					/*!< Slice ID */
  int StartMB;					/*!< Macroblock number where slice begins */
  int QP;						/*!< quantization parameter over slice */
  int Format;					/*!< picture format 0=CIF, 1=QCIF */
  int bytecount;				/*!< length of VLC coded data stream d in bytes */
  int bitcount;					/*!< length of VLC coded data stream d in bits */
  byte d [MAXSIZEPARTITION];	/*!< VLC coded data stream */
  int len [MAXSYMBOLS];			/*!< array containing length of all elements in partition */
  int info [MAXSYMBOLS];		/*!< array containing infoword of all elements in partition */
  int symbols;					/*!< Number of written symbols */
  int symptr;					/*!< Number of read symbols */
  int PartID;					/*!< PartitionID */
} PInfo;

static PInfo *p[MAXPARTITION];			/*!< Main data structure to collect part. data */

#endif