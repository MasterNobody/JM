/*!
 ************************************************************************
 * \file  nal_part.c
 *
 * \brief
 *    Network Adaptation layer for partition file
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Tobias Oelbaum <oelbaum@hhi.de, oelbaum@drehvial.de>
 ************************************************************************
 */

#include "contributors.h"

#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#include "global.h"
#include "elements.h"
#include "bitsbuf.h"

extern void tracebits(const char *trace_str,  int len,  int info,int value1,
    int value2) ;

/*!
 ************************************************************************
 * \brief
 *    read all Partitions of one Slice
 * \return
 *    SOS if this is not a new frame                                    \n
 *    SOP if this is a new frame                                        \n
 *    EOS if end of sequence reached
 ************************************************************************
 */
int readSliceSLICE(struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;
  int eos;

  eos = ReadPartitionsOfSlice (img, inp);

  if(eos)
    return EOS;

  if(currSlice->start_mb_nr)
    return SOS;
  else
    return SOP;
}

/*!
 ************************************************************************
 * \brief
 *    Input file containing Partition structures
 * \return
 *    TRUE if a startcode follows (meaning that all symbols are used).  \n
 *    FALSE otherwise.
 ************************************************************************/
int slice_startcode_follows(struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;
  int dp_Nr = assignSE2partition[inp->partition_mode][SE_MBTYPE];
  DataPartition *dP = &(currSlice->partArr[dp_Nr]);
  Bitstream   *currStream = dP->bitstream;
  byte *buf = currStream->streamBuffer;
  int frame_bitoffset = currStream->frame_bitoffset;
  int info;

  if (currStream->ei_flag)
    return (img->current_mb_nr == currSlice->last_mb_nr);
  else
  {
    if (-1 == GetVLCSymbol (buf, frame_bitoffset, &info, currStream->bitstream_length))
      return TRUE;
    else
      return FALSE;
  }
}

/*!
 ************************************************************************
 * \brief
 *    read next UVLC codeword from SLICE-partition and
 *    map the corresponding syntax element
 *     Add Errorconceilment if necessary
 ************************************************************************
 */
int readSyntaxElement_SLICE(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP)
{
  Bitstream   *currStream = dP->bitstream;

  if (slice_symbols_available(currStream))    // check on existing elements in partition
  {
    slice_get_symbol(sym, currStream);
  }
  else
  {
    set_ec_flag(sym->type);           // otherwise set error concealment flag
  }
  get_concealed_element(sym);

  sym->mapping(sym->len,sym->inf,&(sym->value1),&(sym->value2));

#if TRACE
  tracebits(sym->tracestring, sym->len, sym->inf, sym->value1, sym->value2);
#endif

  return 1;
}

/*!
 ************************************************************************
 * \brief
 *    checks if thererare symbols to read in the
 *    appropriate partition
 ************************************************************************
 */
int slice_symbols_available (Bitstream *currStream)
{
  byte *buf = currStream->streamBuffer;
  int frame_bitoffset = currStream->frame_bitoffset;
  int info;

  if (currStream->ei_flag)
    return FALSE;
  if (-1 == GetVLCSymbol (buf, frame_bitoffset, &info, currStream->bitstream_length))
    return FALSE;
  else
    return TRUE;
};

/*!
 ************************************************************************
 * \brief
 *    gets info and len of symbol
 ************************************************************************
 */
void slice_get_symbol(SyntaxElement *sym, Bitstream *currStream)
{
  int frame_bitoffset = currStream->frame_bitoffset;
  byte *buf = currStream->streamBuffer;
  int BitstreamLengthInBytes = currStream->bitstream_length;

  sym->len =  GetVLCSymbol (buf, frame_bitoffset, &(sym->inf), BitstreamLengthInBytes);
  currStream->frame_bitoffset += sym->len;
}

/*!
 ************************************************************************
 * \brief
 *    Resets the entries in the bitstream struct
 ************************************************************************
 */
void free_Partition(Bitstream *currStream)
{
  byte *buf = currStream->streamBuffer;

  currStream->bitstream_length = 0;
  currStream->frame_bitoffset = 0;
  currStream->ei_flag =0;
  memset (buf, 0x00, MAX_CODED_FRAME_SIZE);
}
