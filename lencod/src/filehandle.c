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
*  Routine      int start_sequence()
*
*  Description: This function opens the output files and generates the 
*								appropriate sequence header
*
************************************************************************/

int start_sequence(struct inp_par *inp)
{	
	char string[255];

	switch(inp->of_mode)
	{
		case PAR_OF_26L:
			if (inp->symbol_mode == UVLC)
			{ 
				/* Current TML File Format */
				if ((p_out=fopen(inp->outfile,"wb"))==0)   /* open TML bitstream file */
				{
					sprintf(errortext, "Error open file %s  \n",inp->outfile);
					error(errortext);
				}
			}
			else
			{
				/* CABAC File Format */
				if ((p_out=fopen(inp->outfile,"wb"))==0)    
				{
					sprintf(errortext, "Error open file %s \n",inp->outfile);  /* open CABAC bitstream file */
					error(errortext);
				}
			}
			return 0;
		case PAR_OF_SLICE:
			/* Slice File Format */
			if ((p_out=fopen(inp->outfile,"wb"))==0)   /* open interim file */
			{
				sprintf(errortext, "Error open file %s  \n",inp->outfile);
				error(errortext);
			}

			return 0;
		case PAR_OF_NAL:
			/* Slice File Format */
			if ((p_out=fopen(inp->outfile,"wb"))==0)   /* open interim file */
			{
				sprintf(errortext, "Error open file %s  \n",inp->outfile);
				error(errortext);
			}

			/* open file to write partition id and lengths as
					we do not have a "real" data departitioner at 
					the decoding side, which requires parsing the stream  */
			sprintf(string, "%s.dp", inp->outfile);
			if ((p_datpart=fopen(string,"wb"))==0)    
			{
				sprintf(errortext, "Error open file %s \n",string);
				error(errortext);
			}
			return 0;
		default: 
			sprintf(errortext, "Output File Mode %d not supported", inp->of_mode);
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

int terminate_sequence(struct img_par *img, struct inp_par *inp)
{
	Bitstream *currStream;
	int stuffing=0xff;
	int active_bitpos;


	/* Mainly flushing of everything */
	/* Add termination symbol, etc.  */

	switch(inp->of_mode)
	{
		case PAR_OF_26L:
			currStream = ((img->currentSlice)->partArr[0]).bitstream;
			if (inp->symbol_mode == UVLC)
			{ 
				/* Current TML File Format */
				/* Get the trailing bits of the last slice */
				currStream->bits_to_go	= currStream->stored_bits_to_go;
				currStream->byte_pos		= currStream->stored_byte_pos;
				currStream->byte_buf		= currStream->stored_byte_buf;
				/* Write EOS to bitstream buffer */
				writeEOS2buffer(img, inp);
				/* Stuff with ones and put out the last byte */
				active_bitpos = 1 << (currStream->bits_to_go-1);
				currStream->byte_buf <<= currStream->bits_to_go;
				while (stuffing>active_bitpos)
					stuffing=stuffing>>1;
				currStream->byte_buf |= stuffing;
				currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
				/* Write all remaining bits to output bitstream file */
				fwrite (currStream->streamBuffer, 1, currStream->byte_pos, p_out);
				fclose(p_out);
			}
			else
			{
				/* CABAC File Format */
				currStream->byte_pos = 0;
				writeEOSToBuffer(inp, img);
				/* Write all remaining bits to output bitstream file */
				fwrite (currStream->streamBuffer, 1, currStream->byte_pos, p_out);
				fclose(p_out);
			}
			return 0;
		case PAR_OF_SLICE:
			/* Slice Format          */
			fclose(p_out);
			return 0;		
		case PAR_OF_NAL:
			/* NAL Format          */
			fclose(p_out);
			fclose(p_datpart);
			return 0;
		default: 
			sprintf(errortext, "Output File Mode %d not supported", inp->of_mode);
			error(errortext);
			return 1;
	}						
}

/************************************************************************
*
*  Routine      int start_slice()
*
*  Description: This function generates the appropriate slice
*								header
*
************************************************************************/

int start_slice(struct img_par *img, struct inp_par *inp, SyntaxElement *sym)
{
	EncodingEnvironmentPtr eep;
	Slice *currSlice = img->currentSlice;
	Bitstream *currStream;
	int header_len;

	switch(inp->of_mode)
	{
		case PAR_OF_26L:
			if (inp->symbol_mode == UVLC)
				/* Current TML File Format */
				return writeStartcode2buffer(img, inp, sym);
			else
			{
				/* CABAC File Format */
				/* initialize arithmetic coder */
				eep = &((currSlice->partArr[0]).ee_cabac);
				header_len = 4 + ( (inp->img_format == QCIF) ? 0 : 1);
				currStream = (currSlice->partArr[0]).bitstream;
				currStream->byte_pos = header_len;
				arienco_start_encoding(eep, currStream->streamBuffer, &(currStream->byte_pos));
				/* initialize context models */
				init_contexts_MotionInfo(img, currSlice->mot_ctx, 1);	
				init_contexts_TextureInfo(img,currSlice->tex_ctx, 1);	
				return ( 8*header_len ); /* header will be written when coding of slice is finished */
			}
		case PAR_OF_SLICE:
			/* Slice Format */
			/* do not do anything in general now */
			/* but here the Startcode is written to the buffer */
			/* return writeStartcode2buffer(img, inp, sym);; */
			/* More exact to match the decoder stuff */
			if (inp->symbol_mode == UVLC)
				currSlice->picture_type = sym->value1;
			else
			{
				/* CABAC Symbol Mode */
				sprintf(errortext, "Output File Mode %d in Symbol Mode %d not YET supported", inp->of_mode, inp->symbol_mode);
				error(errortext);
			}
			return 0;
		case PAR_OF_NAL:
			/* NAL File Format */
			if (inp->symbol_mode == UVLC)
				return writeStartcode2buffer(img, inp, sym);
			else
			{
				/* CABAC Symbol Mode */
				sprintf(errortext, "Output File Mode %d in Symbol Mode %d not YET supported", inp->of_mode, inp->symbol_mode);
				error(errortext);
			}
		default: 
			sprintf(errortext, "Output File Mode %d not supported", inp->of_mode);
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

int terminate_slice(struct img_par *img, struct inp_par *inp, struct stat_par  *stat)
{
	int i,bytes_written,bits_written;
	Bitstream *currStream;
	Slice *currSlice = img->currentSlice;
	EncodingEnvironmentPtr eep;

	/* Mainly flushing of everything */
	/* Add termination symbol, etc.  */
	switch(inp->of_mode)
	{
		case PAR_OF_26L:	
			if (inp->symbol_mode == UVLC)
			{ 
				/* Current TML File Format */
				currStream = (currSlice->partArr[0]).bitstream;
				bytes_written = currStream->byte_pos; /* number of written bytes */
				fwrite (currStream->streamBuffer, 1, bytes_written, p_out);
				/* store the trailing bits in the beginning of the stream belonging to the next slice */
				currStream->stored_bits_to_go = currStream->bits_to_go; /* store bits_to_go */
				currStream->stored_byte_buf		= currStream->byte_buf;		/* store current byte */
				currStream->stored_byte_pos		= 0; /* reset byte position */
			}
			else
			{
				/* CABAC File Format */
				eep = &((currSlice->partArr[0]).ee_cabac);
				currStream = (currSlice->partArr[0]).bitstream;
				/* terminate the arithmetic code */
				arienco_done_encoding(eep);
				/* write header to code buffer */
				writeHeaderToBuffer(inp, img);
				bytes_written = currStream->byte_pos; /* number of written bytes */
				stat->bit_ctr += 8*bytes_written;			/* actually written bits */
				fwrite (currStream->streamBuffer, 1, bytes_written, p_out);
			}
			return 0;
		case PAR_OF_SLICE:
			if (inp->symbol_mode == UVLC)
			{ 
				/* Slice File Format */
				/* Write all Slice relevant information to file */
				fwrite (&(currSlice->picture_id), 4, 1, p_out);
				fwrite (&(currSlice->start_mb_nr), 4, 1, p_out);
				fwrite (&(currSlice->qp), 4, 1, p_out);
				fwrite (&(currSlice->format), 4, 1, p_out);
				fwrite (&(currSlice->picture_type), 4, 1, p_out);
				fwrite (&(currSlice->eos_flag), 4, 1, p_out);
				fwrite (&(currSlice->max_part_nr), 4, 1, p_out);
				fwrite (&(currSlice->dp_mode), 4, 1, p_out);

				for (i=0; i<currSlice->max_part_nr; i++)
				{
					currStream = (currSlice->partArr[i]).bitstream;
					bytes_written = currStream->byte_pos; /* number of written bytes */
					bits_written = 8*bytes_written+8-currStream->bits_to_go; 
					if (currStream->bits_to_go < 8) /* trailing bits to process */
					{
						currStream->byte_buf <<= currStream->bits_to_go;
						currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
						bytes_written++;
					}

					/* write all bits to the output file */
					/* write the next bit vector with length and bits */
					fwrite (&bits_written, 4, 1, p_out);
					fwrite (currStream->streamBuffer, 1, bytes_written, p_out);

					/* Provide the next partition with a 'fresh' buffer */
					currStream->stored_bits_to_go = 8; 
					currStream->stored_byte_buf		= 0;	
					currStream->stored_byte_pos		= 0; 
				}
			}
			else
			{
				/* CABAC Symbol Mode */
				sprintf(errortext, "Output File Mode %d in Symbol Mode %d not YET supported", inp->of_mode, inp->symbol_mode);
				error(errortext);
			}
			return 0;
		case PAR_OF_NAL:
			if (inp->symbol_mode == UVLC)
			{ 
				/* NAL File Format */
				for (i=0; i<currSlice->max_part_nr; i++)
				{
					currStream = (currSlice->partArr[i]).bitstream;
					bytes_written = currStream->byte_pos; /* number of written bytes */
					bits_written = 8*bytes_written+8-currStream->bits_to_go; 
					if (currStream->bits_to_go < 8) /* trailing bits to process */
					{
						currStream->byte_buf <<= currStream->bits_to_go;
						currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
						bytes_written++;
					}
					/* write all bits to the output file */
					if (bits_written != 0)
					{
						/* Write info to dp-file */			
						fprintf(p_datpart,"%d\n%d\n",i , bits_written);

						fwrite (&bits_written, 4, 1, p_out);
						fwrite (&i, 4, 1, p_out);   // DataType
						fwrite (&(currSlice->picture_id), 4, 1, p_out);
						fwrite (&(currSlice->slice_nr), 4, 1, p_out);
						fwrite (&(currSlice->start_mb_nr), 4, 1, p_out);
						fwrite (&(currSlice->qp), 4, 1, p_out);
						fwrite (&(currSlice->format), 4, 1, p_out);
						fwrite (currStream->streamBuffer, 1, bytes_written, p_out);

						/* Provide the next slice with a 'fresh' buffer */
						currStream->stored_bits_to_go = 8; 
						currStream->stored_byte_buf		= 0;	
						currStream->stored_byte_pos		= 0; 
					}
				}
			}
			else
			{
				/* CABAC Symbol Mode */
				sprintf(errortext, "Output File Mode %d in Symbol Mode %d not YET supported", inp->of_mode, inp->symbol_mode);
				error(errortext);
			}
			return 0;
		default: 
			sprintf(errortext, "Output File Mode %d not supported", inp->of_mode);
			error(errortext);
			return 1;
	}									
}


