/**************************************************************************************
**************************************************************************************
* Filehandle.c	 Handles the operations how to read
*								 symbols of the interim file format or some
*								 other specified input formats
*
* Main contributors (see contributors.h for copyright, address and affiliation details)
*
* Thomas Stockhammer			<stockhammer@ei.tum.de>
* Detlev Marpe                  <marpe@hhi.de>
***************************************************************************************
***************************************************************************************/

#include "contributors.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdlib.h>

#include "global.h"
#include "elements.h"
#if TRACE
#include <string.h>    /* strncpy */
#endif

/************************************************************************
*
*  Routine      void error(char *text)
*
*  Description: Error Handling Procedure
*
************************************************************************/

void error(char *text)
{
	fprintf(stderr, "%s\n", errortext);
	exit(1);
}



/************************************************************************
*
*  Routine      int start_slice()
*
*  Description: This function initializes the appropriate methods for 
*               decoding a slice in the given symbol mode
*			
*
************************************************************************/

void start_slice(struct img_par *img, struct inp_par *inp)
{
	Slice *currSlice = img->currentSlice;
	
	switch(inp->of_mode)
	{
		case PAR_OF_26L:
			
			if (inp->symbol_mode == UVLC)
			{
				/* Current TML File Format */
				nal_startcode_follows = uvlc_startcode_follows;
				currSlice->readSlice = readSliceUVLC;
				currSlice->partArr[0].readSyntaxElement = readSyntaxElement_UVLC; 
			}
			else
			{
				/* CABAC File Format */
				nal_startcode_follows = cabac_startcode_follows;
				currSlice->readSlice = readSliceCABAC;
				currSlice->partArr[0].readSyntaxElement = readSyntaxElement_CABAC;
			}
			break;
		default: 
			sprintf(errortext, "Output File Mode %d not supported", inp->of_mode);
			error(errortext);
			break;
	}							
}


