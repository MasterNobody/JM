/*! 
 * 	\file 	
 *			NALencH223.h
 * 	\brief	
 *			Header file for NAL-encoder
 *	\date	
 *			22.10.2000
 *	\version
 *			1.0
 * 	\author	
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 */

#ifndef _NAL_ENC_4_H223_
#define _NAL_ENC_4_H223_

FILE	*in, *out;
int		finished = 0;

int 	NumberOfBytes (int bits);
int 	ReadPicture();
void 	WritePicture(int mode);
void 	Initialize();

#endif