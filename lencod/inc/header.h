/*!
 *************************************************************************************
 * \file header.h
 *
 * \brief
 *    Prototypes for header.c
 *************************************************************************************
 */

#ifndef _HEADER_H_
#define _HEADER_H_

int  SliceHeader(int UseStartCode);
int  PictureHeader();
int  SequenceHeader(FILE *outf);
void LastMBInSlice();

#endif
