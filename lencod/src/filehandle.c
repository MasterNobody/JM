/**************************************************************************************
**************************************************************************************
* Filehandle.c	 Handles the operations how to write
*								 the generated symbols on the interim file format or some
*								 other specified output formats
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
#include <assert.h>
#include "string.h"

#include "defines.h"
#include "global.h"
#include "header.h"


// Global Varibales

static FILE *out;		// output file
//static int bytepos;		// byte position in output file
//static int bitpos;		// bit position in output file

//
//
//
//
//
//
//
/*

The implemented solution for a unified picture header 

*/
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
*  Routine      int start_sequence()
*
*  Description: This function opens the output files and generates the 
*								appropriate sequence header
*
************************************************************************/

int start_sequence()
{	

	switch(input->of_mode)
	{
		case PAR_OF_26L:
			if ((out=fopen(input->outfile,"wb"))==NULL) {
					sprintf(errortext, "Error open file %s  \n",input->outfile);
					error(errortext);
			}
			SequenceHeader(out);
			return 0;
		default: 
			sprintf(errortext, "Output File Mode %d not supported", input->of_mode);
			error(errortext);
			return 1;
	}				
}

/************************************************************************
*
*  Routine      int terminate_sequence()
*
*  Description: This function terminates the sequence and closes the 
*								output files
*
************************************************************************/

int terminate_sequence()
{
	Bitstream *currStream;
	//int stuffing=0xff;


	/* Mainly flushing of everything */
	/* Add termination symbol, etc.  */

	switch(input->of_mode)
	{
		case PAR_OF_26L:
			currStream = ((img->currentSlice)->partArr[0]).bitstream;
			if (input->symbol_mode == UVLC)
			{ 

				/* Current TML File Format */
				/* Get the trailing bits of the last slice */
				currStream->bits_to_go	= currStream->stored_bits_to_go;
				currStream->byte_pos		= currStream->stored_byte_pos;
				currStream->byte_buf		= currStream->stored_byte_buf;

				if (currStream->bits_to_go < 8)		// there are bits left in the last byte
					currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
				/* Write all remaining bits to output bitstream file */
				fwrite (currStream->streamBuffer, 1, currStream->byte_pos, out);
				fclose(out);
			}
			else
			{
				/* CABAC File Format */
				fclose(out);
			}
			return 0;
		default: 
			sprintf(errortext, "Output File Mode %d not supported", input->of_mode);
			error(errortext);
			return 1;
	}						
}





/*!
 *	\fn		start_slice()
 *
 *	\return	number of bits used for the picture header, including the PSC.
 
 *	\note
 *
 *	-Side effects:
 *			Adds picture header symbols to the symbol buffer
 *  -Remarks:
 *			THIS IS AN INTERIM SOLUTION FOR A PICTURE HEADER, see VCEG-M79
 *
 *			Generates the Picture Header out of the information in img_par and inp-par,
 *			and writes it to the symbol buffer.  The structure of the Picture Header
 *			is discussed in VCEG-M79.  It is implemented as discussed there.  In addition,
 *			it is preceeded by a picture start code, a UVLC-codeword of LEN=31 and INFO = 0.
 *          It would have been possible to put information into this codeword, similar
 *			to designs earlier than TML 5.9.  But it is deemed that the waste of 15
 *			data bits is acceptable considering the advantages of being able to search
 *			for a picture header quickly and easily, by looking for 30 consecutive, byte-
 *			aligned zero bits.
 *
 *			The accounting of the header length (variable len) relies on the side effect
 *			of writeUVLCSymbol() that sets len and info in the symbol variable parameter
*/





/************************************************************************
*
*  Routine      int start_slice()
*
*  Description: This function generates the appropriate slice
*								header
*
************************************************************************/



int start_slice(SyntaxElement *sym)
{
	EncodingEnvironmentPtr eep;
	Slice *currSlice = img->currentSlice;
	Bitstream *currStream; 
	int header_len;

	switch(input->of_mode)
	{
		case PAR_OF_26L:
			if (input->symbol_mode == UVLC) {
				currStream = (currSlice->partArr[0]).bitstream;
				if (img->current_mb_nr == 0) {
					header_len = PictureHeader();		// Picture Header
					header_len += SliceHeader (0);	// Slice Header without Start Code
				} else {
					header_len = SliceHeader (1);	// Slice Header with Start Code
				}
				
				return header_len;
			}
			  else {										// H.26: CABAC File Format
				eep = &((currSlice->partArr[0]).ee_cabac);
				currStream = (currSlice->partArr[0]).bitstream;

				assert (currStream->bits_to_go == 8);
				assert (currStream->byte_buf == 0);
				assert (currStream->byte_pos == 0);
				memset(currStream->streamBuffer, 0, 12);		// fill first 12 bytes with zeros (debug only)

				if (img->current_mb_nr == 0) {
					header_len = PictureHeader();		// Picture Header
					header_len += SliceHeader (0);	// Slice Header without Start Code
				} else {
					header_len = SliceHeader (1);	// Slice Header with Start Code
				}
				
				// Note that PictureHeader() and SLiceHeader() set the buffer pointers as a side effect
				// Hence no need for adjusting it manually (and preserving space to be patched later
				
				//reserve bits for d_MB_Nr 
				currStream->header_len = header_len;
				currStream->header_byte_buffer = currStream->byte_buf;
				
				currStream->byte_pos += ((31-currStream->bits_to_go)/8);
				if ((31-currStream->bits_to_go)%8 != 0)
					currStream->byte_pos++;
				currStream->bits_to_go = 8;
				currStream->byte_pos++;

				// If there is an absolute need to communicate the partition size, this would be
				// the space to insert it
				arienco_start_encoding(eep, currStream->streamBuffer, &(currStream->byte_pos));
				/* initialize context models */
				init_contexts_MotionInfo(currSlice->mot_ctx, 1);	
				init_contexts_TextureInfo(currSlice->tex_ctx, 1);

				return header_len;

			}
		default: 
			sprintf(errortext, "Output File Mode %d not supported", input->of_mode);
			error(errortext);
			return 1;
	}							
}






/************************************************************************
*
*  Routine      int terminate_slice()
*
*  Description: This function terminates a slice
*
************************************************************************/

int terminate_slice()
{
	int bytes_written;
	Bitstream *currStream;
	Slice *currSlice = img->currentSlice;
	EncodingEnvironmentPtr eep;
	int byte_pos, bits_to_go, start_data;
	byte buffer;

int null = 0;

	/* Mainly flushing of everything */
	/* Add termination symbol, etc.  */
	switch(input->of_mode)
	{
		case PAR_OF_26L:

			
			if (input->symbol_mode == UVLC)
			{ 
				// Current TML File Format 
				// Enforce byte alignment of next header: zero bit stuffing
				currStream = (currSlice->partArr[0]).bitstream;
				
				if (currStream->bits_to_go < 8) { // trailing bits to process 
					currStream->byte_buf <<= currStream->bits_to_go;
					currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
					currStream->bits_to_go = 8;
				}
				
				bytes_written = currStream->byte_pos;
				fwrite (currStream->streamBuffer, 1, bytes_written, out);

				currStream->stored_bits_to_go = 8; // store bits_to_go
				currStream->stored_byte_buf		= currStream->byte_buf;		// store current byte 
				currStream->stored_byte_pos		= 0; // reset byte position 

			}
			else
			{

				/* CABAC File Format */
				eep = &((currSlice->partArr[0]).ee_cabac);
				currStream = (currSlice->partArr[0]).bitstream;

				/* terminate the arithmetic code */
				arienco_done_encoding(eep);
				
				//Add Number of MBs of this slice to the header
				//Save current state of Bitstream
				currStream = (currSlice->partArr[0]).bitstream;
				byte_pos = currStream->byte_pos;
				bits_to_go = currStream->bits_to_go;
				buffer = currStream->byte_buf; 
				
				//Go to the reserved bits
				currStream->byte_pos = (currStream->header_len)/8;
				currStream->bits_to_go = 8-(currStream->header_len)%8;
				currStream->byte_buf = currStream->header_byte_buffer;
				
				//Add Info about last MB
				LastMBInSlice();
				
				//And write the header to the output
				bytes_written = currStream->byte_pos;
				if (currStream->bits_to_go < 8) // trailing bits to process 
				{ 
						currStream->byte_buf <<= currStream->bits_to_go;
						currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
						bytes_written++;
						currStream->bits_to_go = 8;
				}
				fwrite (currStream->streamBuffer, 1, bytes_written, out);
				stat->bit_ctr += 8*bytes_written;
				
				//Go back to the end of the stream
				currStream->byte_pos = byte_pos; 
				currStream->bits_to_go = bits_to_go;
				currStream->byte_buf = buffer;


				//Find start position of data bitstream
				start_data = (currStream->header_len+31)/8;
				if ((currStream->header_len+31)%8 != 0)
					start_data++;
				bytes_written = currStream->byte_pos - start_data; // number of written bytes

				stat->bit_ctr += 8*bytes_written;			/* actually written bits */

				
				fwrite ((currStream->streamBuffer+start_data), 1, bytes_written, out);

			}
			return 0;
		default: 
			sprintf(errortext, "Output File Mode %d not supported", input->of_mode);
			error(errortext);
			return 1;
	}									
}



