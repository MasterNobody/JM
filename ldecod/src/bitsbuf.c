//
// BitSBuf.c		Bit Stream Buffer Management
//
// this module is introduced to facilitate the handling of the bit stream
// reading.  It assumes a start code taht is a UVLC code word with len 31
// and info 0 for Picture Start code, and info = 1 for Slice start code.
// The module should be used for all entropy coding schemes, including UVLC
// and CABAC.



#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "global.h"
#include "bitsbuf.h"

static int SourceBitBufferPtr;		// the position for the next byte-write into the bit buffer
static FILE *bits;					// the bit stream file


//
// InitializeSourceBitBuffer()
//
// No Paranmeters
// Side effects: Initializes teh Source Bit Buffer

void InitializeSourceBitBuffer() {
	SourceBitBufferPtr = 0;
}


//
// static int TypeOfStartCode (byte *buf) returns the type of the start code
//   at byte aligned position buf.  Possible returns are 
//   -- the info field of a start code (len == 31) or
//   -- -1, inidicating that there is no start code here
// This function could be optimized considerably by checking for zero bytes first,
// but the current implementation gives us mroe freedom to define what a
// start code actually is
// Note that this function could be easily extended to search for non
// byte alogned start codes, by simply checking all 8 bit positions
// (and not only the zero position

static int TypeOfStartCode (byte *Buf) {
	int info;

	// A bit of optimization first: a start code starts always with 3 zero bytes
	if ((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 0))
		return -1;

	if (31 != GetVLCSymbol (Buf, 0, &info, MAX_CODED_FRAME_SIZE)) { 
//		printf ("TypeOfStartcode: GetVLCSymbolreturns %d\n", GetVLCSymbol (Buf, 0, &info));
		return -1;
	}
//	printf ("TypeOfStartCode: found info %d\n", info);
	if (info != 0 && info != 1)		// the only two start codes currently allowed
		return -1;
	return info;
}

//
// int GetOneSliceIntoSourceBitBuffer (int, int *)
//
// Returns the number of bytes in copied into the buffer
// The buffer includes the own start code, but excludes the
// next slice's start code
// returns 0, if there is nothing any more to read (EOF)
// returns -1 in case of any error
//
// Note: GetOneSliceIntoSourceBitBuffer() expects start codes as follows:
//   Slice start code: UVLC, len == 31, info == 1
//   Picture Start code: UVLC, len == 31, info == 0
// Note: getOneSliceIntoSourceBitBuffer expects Slice and Picture start codes
//   at byte aligned positions in the file
//

int GetOneSliceIntoSourceBitBuffer(byte *Buf) {
	int info, pos;
	int StartCodeFound;

	InitializeSourceBitBuffer();
	// read the first 32 bits (which MUST contain a start code, or eof)
	if (4 != fread (Buf, 1, 4, bits)) {
// printf ("GetOneSliceIntoSourceBitBuffer: cannot read four bytes header, return 0\n");
		return 0;
	}
	info = TypeOfStartCode (Buf);
	if (info < 0) {
		printf ("GetOneSliceIntoSourceBitBuffer: no Start Code at the begin of the slice, return -1\n");
		return -1;
	}
	if (info != 0 && info != 1) {
		printf ("GetOneSliceIntoSourceBitBuffer: found start code with invalid info %d, return -1\n", info);
		return -1;
	}
// printf ("GetOneSliceIntoSourceBitBuffer: got a correct start code\n");
	pos = 4;
	StartCodeFound = 0;
	
	while (!StartCodeFound) {
		if (feof (bits))
			return pos;
		Buf[pos++] = fgetc (bits);
		
		info = TypeOfStartCode(&Buf[pos-4]);
		StartCodeFound = (info == 0 || info == 1);
	}

	// Here, we have found another start code (and read four bytes more than we should
	// have.  Hence, go back in the file

	if (0 != fseek (bits, -4, SEEK_CUR)) {
		printf ("GetOneSliceIntoSourceBitBuffer: Cannot fseek -4 in the bit stream file, exit\n");
		exit (-1);
	}

// { int i; for (i=0;i<12;i++) printf ("%02x ", Buf[i]); printf ("\n"); }
	return (pos-4);
}





//
// int OpenBitstreamFile (char *fn)
//
// Attempts to open the bit stream file named fn
// returns 0 on success, -1 in case of error
//
int OpenBitstreamFile (char *fn) {
	if (NULL == (bits=fopen(fn, "rb")))
		return -1;
//	printf ("OpenBitstreamFile: sucessfully opened file\n");
	return 0;
}


//
// void CloseBitstreamFile()
//
// Closes the bit stream file
//

void CloseBitstreamFile() {
	fclose (bits);
}
