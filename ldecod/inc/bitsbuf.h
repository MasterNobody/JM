/*!
 *************************************************************************************
 * \file bitsbuf.h
 *
 * \brief
 *    Bit Stream Buffer handling.
 *
 *************************************************************************************
 */

#ifndef _BITSBUF_H_
#define _BITSBUF_H_


void InitializeSourceBitBuffer();
int  GetOneSliceIntoSourceBitBuffer(byte *Buf);
int  OpenBitstreamFile (char *fn);
void CloseBitstreamFile();
void get_last_mb(struct img_par *img, struct inp_par *inp);

#endif


