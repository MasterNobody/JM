/*!
 **************************************************************************************
 * \file
 *    filehandle.c
 * \brief
 *    Handles the operations how to write
 *    the generated symbols on the interim file format or some
 *    other specified output formats
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *      - Thomas Stockhammer            <stockhammer@ei.tum.de>
 *      - Detlev Marpe                  <marpe@hhi.de>
 ***************************************************************************************
 */

#include "contributors.h"

#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "global.h"
#include "header.h"


// Global Varibales

static FILE *out;   //!< output file

/*
   The implemented solution for a unified picture header
*/

/*!
 ************************************************************************
 * \brief
 *    Error handling procedure. Print error message to stderr and exit
 *    with supplied code.
 * \param text
 *    Error message
 * \param code
 *    Exit code
 ************************************************************************
 */
void error(char *text, int code)
{
  fprintf(stderr, "%s\n", text);
  exit(code);
}


/*!
 ************************************************************************
 * \brief
 *    This function opens the output files and generates the
 *    appropriate sequence header
 ************************************************************************
 */
int start_sequence()
{

  switch(input->of_mode)
  {
    case PAR_OF_26L:
      if ((out=fopen(input->outfile,"wb"))==NULL)
      {
          snprintf(errortext, ET_SIZE, "Error open file %s  \n",input->outfile);
          error(errortext,1);
      }
      SequenceHeader(out);
      return 0;
    case PAR_OF_SLICE:
      // Slice File Format
      if ((out=fopen(input->outfile,"wb"))==0)   // open interim file
      {
        snprintf(errortext, ET_SIZE, "Error open file %s  \n",input->outfile);
        error(errortext,1);
      }
      // No sequence headers in interim File Format?
      return 0;
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}

/*!
 ************************************************************************
 * \brief
 *     This function terminates the sequence and closes the
 *     output files
 ************************************************************************
 */
int terminate_sequence()
{
  Bitstream *currStream;

  // Mainly flushing of everything
  // Add termination symbol, etc.

  switch(input->of_mode)
  {
    case PAR_OF_26L:
      currStream = ((img->currentSlice)->partArr[0]).bitstream;
      if (input->symbol_mode == UVLC)
      {

        // Current TML File Format
        // Get the trailing bits of the last slice
        currStream->bits_to_go  = currStream->stored_bits_to_go;
        currStream->byte_pos    = currStream->stored_byte_pos;
        currStream->byte_buf    = currStream->stored_byte_buf;

        if (currStream->bits_to_go < 8)   // there are bits left in the last byte
          currStream->streamBuffer[currStream->byte_pos++] = currStream->byte_buf;
        // Write all remaining bits to output bitstream file
        fwrite (currStream->streamBuffer, 1, currStream->byte_pos, out);
        fclose(out);
      }
      else
      {
        // CABAC File Format
        fclose(out);
      }
      return 0;
    case PAR_OF_SLICE:
      // Slice Format
      fclose(out);
      return 0;
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}

/*!
 ************************************************************************
 *  \brief
 *     This function generates the appropriate slice
 *     header
 *
 *  \return number of bits used for the picture header, including the PSC.
 *
 *  \par Side effects:
 *      Adds picture header symbols to the symbol buffer
 *
 *  \par Remarks:
 *      THIS IS AN INTERIM SOLUTION FOR A PICTURE HEADER, see VCEG-M79
 *                                                                               \par
 *      Generates the Picture Header out of the information in img_par and inp-par,
 *      and writes it to the symbol buffer.  The structure of the Picture Header
 *      is discussed in VCEG-M79.  It is implemented as discussed there.  In addition,
 *      it is preceeded by a picture start code, a UVLC-codeword of LEN=31 and INFO = 0.
 *      It would have been possible to put information into this codeword, similar
 *      to designs earlier than TML 5.9.  But it is deemed that the waste of 15
 *      data bits is acceptable considering the advantages of being able to search
 *      for a picture header quickly and easily, by looking for 30 consecutive, byte-
 *      aligned zero bits.
 *                                                                               \par
 *      The accounting of the header length (variable len) relies on the side effect
 *      of writeUVLCSymbol() that sets len and info in the symbol variable parameter
 ************************************************************************
*/

int start_slice(SyntaxElement *sym)
{
  EncodingEnvironmentPtr eep;
  Slice *currSlice = img->currentSlice;
  Bitstream *currStream;
  int header_len;
  int i;

  switch(input->of_mode)
  {
    case PAR_OF_26L:
      if (input->symbol_mode == UVLC)
      {
        currStream = (currSlice->partArr[0]).bitstream;
        // the information given in the SliceHeader is far not enought if we want to be able
        // to do some errorconcealment.
        // So write PictureHeader and SliceHeader in any case.
        if (img->current_mb_nr == 0)
        {
          header_len = PictureHeader();   // Picture Header
          header_len += SliceHeader (0);  // Slice Header without Start Code
        }
        else
        {
          header_len = PictureHeader();   // Picture Header
          header_len += SliceHeader (0);  // Slice Header without Start Code
        }

        return header_len;
      }
      else
      {                   // H.26: CABAC File Format
        eep = &((currSlice->partArr[0]).ee_cabac);
        currStream = (currSlice->partArr[0]).bitstream;

        assert (currStream->bits_to_go == 8);
        assert (currStream->byte_buf == 0);
        assert (currStream->byte_pos == 0);
        memset(currStream->streamBuffer, 0, 12);    // fill first 12 bytes with zeros (debug only)

        // the information given in the SliceHeader is far not enought if we want to be able
        // to do some errorconcealment.
        // So write PictureHeader and SliceHeader in any case.
        if (img->current_mb_nr == 0)
        {
          header_len = PictureHeader(); // Picture Header
          header_len += SliceHeader (0);  // Slice Header without Start Code
        }
        else
        {
          header_len = PictureHeader();   // Picture Header
          header_len += SliceHeader (0);  // Slice Header without Start Code
        }

        // Note that PictureHeader() and SliceHeader() set the buffer pointers as a side effect
        // Hence no need for adjusting it manually (and preserving space to be patched later

        // reserve bits for d_MB_Nr
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
        // initialize context models
        init_contexts_MotionInfo(currSlice->mot_ctx, 1);
        init_contexts_TextureInfo(currSlice->tex_ctx, 1);

        return header_len;

      }
    // as we do not have a Picture- ore Sliceheader in SLICE-Mode
    // we need to get the picture_type here.
    case PAR_OF_SLICE:
      select_picture_type (sym);
      currSlice->picture_type = sym->value1;
      if (input->symbol_mode == CABAC)
      {
        for (i=0; i<currSlice->max_part_nr; i++)
        {
          eep = &((currSlice->partArr[i]).ee_cabac);
          currStream = (currSlice->partArr[i]).bitstream;

          assert (currStream->bits_to_go == 8);
          assert (currStream->byte_buf == 0);
          assert (currStream->byte_pos == 0);
          memset(currStream->streamBuffer, 0, 12);    // fill first 12 bytes with zeros (debug only)

          arienco_start_encoding(eep, currStream->streamBuffer, &(currStream->byte_pos));
        }
        init_contexts_MotionInfo(currSlice->mot_ctx, 1);
        init_contexts_TextureInfo(currSlice->tex_ctx, 1);
      }
      return 0;
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}


/*!
 ************************************************************************
 * \brief
 *    This function terminates a slice
 * \return
 *    0 if OK,                                                         \n
 *    1 in case of error
 ************************************************************************
 */
int terminate_slice()
{
  int bytes_written;
  Bitstream *currStream;
  Slice *currSlice = img->currentSlice;
  EncodingEnvironmentPtr eep;
  int i;
  int byte_pos, bits_to_go, start_data;
  byte buffer;

  // Mainly flushing of everything
  // Add termination symbol, etc.
  switch(input->of_mode)
  {
    case PAR_OF_26L:


      if (input->symbol_mode == UVLC)
      {
        // Current TML File Format
        // Enforce byte alignment of next header: zero bit stuffing
        currStream = (currSlice->partArr[0]).bitstream;

        if (currStream->bits_to_go < 8)
        { // trailing bits to process
          currStream->byte_buf <<= currStream->bits_to_go;
          currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
          currStream->bits_to_go = 8;
        }

        bytes_written = currStream->byte_pos;
        stat->bit_ctr += 8*bytes_written;     // actually written bits
        fwrite (currStream->streamBuffer, 1, bytes_written, out);

        currStream->stored_bits_to_go = 8; // store bits_to_go
        currStream->stored_byte_buf   = currStream->byte_buf;   // store current byte
        currStream->stored_byte_pos   = 0; // reset byte position

      }
      else
      {

        // CABAC File Format
        eep = &((currSlice->partArr[0]).ee_cabac);
        currStream = (currSlice->partArr[0]).bitstream;

        // terminate the arithmetic code
        arienco_done_encoding(eep);

        // Add Number of MBs of this slice to the header
        // Save current state of Bitstream
        currStream = (currSlice->partArr[0]).bitstream;
        byte_pos = currStream->byte_pos;
        bits_to_go = currStream->bits_to_go;
        buffer = currStream->byte_buf;

        // Go to the reserved bits
        currStream->byte_pos = (currStream->header_len)/8;
        currStream->bits_to_go = 8-(currStream->header_len)%8;
        currStream->byte_buf = currStream->header_byte_buffer;

        // Ad Info about last MB
        LastMBInSlice();

        // And write the header to the output
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

        // Go back to the end of the stream
        currStream->byte_pos = byte_pos;
        currStream->bits_to_go = bits_to_go;
        currStream->byte_buf = buffer;


        // Find startposition of databitstream
        start_data = (currStream->header_len+31)/8;
        if ((currStream->header_len+31)%8 != 0)
          start_data++;
        bytes_written = currStream->byte_pos - start_data; // number of written bytes

        stat->bit_ctr += 8*bytes_written;     // actually written bits


        fwrite ((currStream->streamBuffer+start_data), 1, bytes_written, out);

      }
      return 0;
    case PAR_OF_SLICE:
      for (i=0; i<currSlice->max_part_nr; i++)
      {
        if (input->symbol_mode == CABAC)
        {
          eep = &((currSlice->partArr[i]).ee_cabac);
          arienco_done_encoding(eep);
        }
        currStream = (currSlice->partArr[i]).bitstream;
        bytes_written = currStream->byte_pos; // number of written bytes

        if (currStream->bits_to_go < 8) // trailing bits to process
        {
          currStream->byte_buf <<= currStream->bits_to_go;
          currStream->streamBuffer[currStream->byte_pos++]=currStream->byte_buf;
          bytes_written++;
        }

        if (!i)
          DP_SliceHeader(out, bytes_written, i);
        else
          DP_PartHeader(out, bytes_written, i);

        if (bytes_written != 0)
        {
          // write the next bit vector with length and bits
          fwrite (currStream->streamBuffer, 1, bytes_written, out);

          stat->bit_ctr += 8*bytes_written;
          // Provide the next partition with a 'fresh' buffer
          currStream->stored_bits_to_go = 8;
          currStream->stored_byte_buf   = 0;
          currStream->stored_byte_pos   = 0;
        }
      }
      return 0;
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", input->of_mode);
      error(errortext,1);
      return 1;
  }
}
