/************************************************************************
*
*  BitSBuf.h for H.26L decoder: Bit Stream Buffer handling. 
*  
************************************************************************/

#ifndef _BITSBUF_H
#define _BITSBUF_H


void InitializeSourceBitBuffer();
int GetOneSliceIntoSourceBitBuffer(byte *Buf);
int OpenBitstreamFile (char *fn);
void CloseBitstreamFile();



#endif


