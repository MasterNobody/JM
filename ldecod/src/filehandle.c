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

#include "global.h"
#include "elements.h"
#if TRACE
#include <string.h>    // strncpy
#endif

/*!
 ************************************************************************
 * \brief
 *    Error handling procedure. Print error message to stderr and exit
 *    with supplied code.
 * \param text
 *    Error message
 ************************************************************************
 */
void error(char *text, int code)
{
  fprintf(stderr, "%s\n", text);
  exit(code);
}


/*!
 ************************************************************************
 *  \brief
 *     This function generates the appropriate slice
 *     header
 ************************************************************************
 */
void start_slice(struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;
  int i;

  switch(inp->of_mode)
  {
    case PAR_OF_26L:

      if (inp->symbol_mode == UVLC)
      {
        // Current TML File Format
        nal_startcode_follows = uvlc_startcode_follows;
        currSlice->readSlice = readSliceUVLC;
        currSlice->partArr[0].readSyntaxElement = readSyntaxElement_UVLC;
      }
      else
      {
        // CABAC File Format
        nal_startcode_follows = cabac_startcode_follows;
        currSlice->readSlice = readSliceCABAC;
        currSlice->partArr[0].readSyntaxElement = readSyntaxElement_CABAC;
      }
      break;
    case PAR_OF_SLICE:
      if (inp->symbol_mode == UVLC)
      {
        nal_startcode_follows = uvlc_startcode_follows;
        currSlice->readSlice = readSliceSLICE;
        for (i=0; i<currSlice->max_part_nr; i++)
          currSlice->partArr[i].readSyntaxElement = readSyntaxElement_SLICE;
      }
      else
      {
        // CABAC File Format
        nal_startcode_follows = cabac_startcode_follows;
        currSlice->readSlice = readSliceSLICE;
        for (i=0; i<currSlice->max_part_nr; i++)
          currSlice->partArr[i].readSyntaxElement = readSyntaxElement_CABAC;
      }
      break;
    default:
      snprintf(errortext, ET_SIZE, "Output File Mode %d not supported", inp->of_mode);
      error(errortext,1);
      break;
  }
}


