/*! 
 * 	\file 	NALdec4H223.h
 * 	\brief	Header file for NAL-decoder
 *	\date	22.10.2000
 *	\version
 *			1.0
 * 	\author	
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 */

#ifndef _NAL_DEC_4_H223_
#define _NAL_DEC_4_H223_

FILE	*in, *out, *partinf;

int		finished = 0, i;

int		GetVLCSymbol (byte buffer[], int totbitoffset, int *info);
int 	NumberOfBytes (int bits);
int 	ReadPicture();
void 	WritePicture();

#endif