/*!
 *************************************************************************************
 * \file header.c
 *
 * \brief
 *    H.26L Slice and Picture headers
 *
 *************************************************************************************
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "global.h"
#include "elements.h"
#include "header.h"


#if TRACE
#define SYMTRACESTRING(s) strncpy(sym.tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // to nothing
#endif


/*!
 ************************************************************************
 * \brief
 *    read the whole Picture header without the Startcode UVLC
 * \return
 *    the number of bits used by the header exclusing start code
 ************************************************************************
 */
int PictureHeader(struct img_par *img, struct inp_par *inp)
{

  int dP_nr = assignSE2partition[inp->partition_mode][SE_HEADER];
  Slice *currSlice = img->currentSlice;
  Bitstream *currStream = (currSlice->partArr[dP_nr]).bitstream;
  DataPartition *partition = &(currSlice->partArr[dP_nr]);
  SyntaxElement sym;
  int UsedBits=31;

  sym.type = SE_HEADER;
  sym.mapping = linfo;

  // 1. TRType = 0
  SYMTRACESTRING("PH TemporalReferenceType");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  // currSlice-> ?????????
  // Currently, only the value 0 is supported, hence a simple assert to
  // catch evenntual  encoder problems
  if (sym.value1 != 0)
  {
    snprintf (errortext, ET_SIZE, "Unsupported value %d in Picture Header TemporalReferenceType, len %d info %d", sym.value1, sym.len, sym.inf);
    error(errortext, 600);
  }
  UsedBits += sym.len;

  // 2. TR, variable length
  SYMTRACESTRING("PH TemporalReference");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  currSlice->picture_id = img->tr = sym.value1;
  UsedBits += sym.len;

  // 3. Size Type
  SYMTRACESTRING ("PH WhichSize...");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  UsedBits += sym.len;
  if (sym.value1 == 0)
    SYMTRACESTRING("PH:    Size Information Unchanged");
  else
  {
    // width * 16
    SYMTRACESTRING("PH FullSize-X");
    readSyntaxElement_UVLC (&sym,img,inp,partition);
    img->width = sym.value1 * MB_BLOCK_SIZE;
    img->width_cr = img->width/2;
    UsedBits+=sym.len;
    SYMTRACESTRING("PH FullSize-Y");
    readSyntaxElement_UVLC (&sym,img,inp,partition);
    img->height = sym.value1 * MB_BLOCK_SIZE;
    img->height_cr = img->height/2;
    UsedBits+= sym.len;
  }

  // 4. Picture Type indication (I, P, Mult_P, B , Mult_B)
  SYMTRACESTRING("PHPictureType");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  currSlice->picture_type = img->type = sym.value1;
  UsedBits += sym.len;

  // Finally, read Reference Picture ID (same as TR here).  Note that this is an
  // optional field that is not present if the input parameters do not indicate
  // multiframe prediction ??

  // Ok, the above comment is nonsense.  There is no way how a decoder could
  // know that we use multiple reference frames (except probably through a
  // sequence header).  Hence, it's now an if (1) -- PHRefPicID is always present

  // Of course, the decoder can know. It is indicated by the previously decoded
  // parameter "PHPictureType". So, I changed the if-statement again to be
  // compatible with the encoder.

  if (1)
    // if ((img->type == INTER_IMG_MULT) || (img->type == B_IMG_MULT))
  { // more than one reference frames
    // TR, variable length
    SYMTRACESTRING("PHRefPicID");
    readSyntaxElement_UVLC (&sym,img,inp,partition);
    UsedBits += sym.len;
  }

  // Now get the first Slice Header without a start code

  UsedBits += SliceHeader (img, inp);

  return UsedBits;
}

/*!
 ************************************************************************
 * \brief
 *    read the whole Slice header without the Startcode UVLC
 ************************************************************************
 */
int SliceHeader(struct img_par *img, struct inp_par *inp)
{
  int dP_nr = assignSE2partition[inp->partition_mode][SE_HEADER];
  Slice *currSlice = img->currentSlice;
  Bitstream *currStream = (currSlice->partArr[dP_nr]).bitstream;
  DataPartition *partition = &(currSlice->partArr[dP_nr]);
  SyntaxElement sym;
  int UsedBits=0;

  sym.type = SE_HEADER;
  sym.mapping = linfo;

  // 1. Get MB-Adresse
  SYMTRACESTRING("SH FirstMBInSlice");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  currSlice->start_mb_nr = sym.value1;
  UsedBits += sym.len;

  // 2. Get Quant.
  SYMTRACESTRING("SH SliceQuant");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  currSlice->qp = img->qp = 31 - sym.value1;
  UsedBits += sym.len;

  if(img->type==SP_IMG_1 || img->type==SP_IMG_MULT)
  {
    SYMTRACESTRING("SH SP SliceQuant");
    readSyntaxElement_UVLC (&sym,img,inp,partition);
    img->qpsp = 31 - sym.value1;
  }
  // 2. Get MVResolution
  SYMTRACESTRING("SH MVResolution");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  img->mv_res = sym.value1;
  UsedBits += sym.len;

  img->max_mb_nr = (img->width * img->height) / (MB_BLOCK_SIZE * MB_BLOCK_SIZE);
  if (inp->symbol_mode ==CABAC)
  {
    // 3. Get number of MBs in this slice
    SYMTRACESTRING("SH Last MB in Slice");
    readSyntaxElement_UVLC (&sym,img,inp,partition);
    currSlice->last_mb_nr = currSlice->start_mb_nr+sym.value1;
    // Note: if one slice == one frame the number of MBs in this slice is coded as 0
    if (currSlice->last_mb_nr == currSlice->start_mb_nr)
      currSlice->last_mb_nr = img->max_mb_nr;
    UsedBits += sym.len;
  }
  return UsedBits;
}

/*!
 ************************************************************************
 * \brief
 *    get info of SliceHeader (interim file format) according to VCEG-M52
 ************************************************************************
 */
void DP_SliceHeader(int last_mb, struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;

  switch (currSlice->dp_mode)
  {
    case 0:
      currSlice->max_part_nr = 1;
      break;
    case 1:
      currSlice->max_part_nr = 2;
      break;
    case 2:
      currSlice->max_part_nr = 4;
      break;
    default:
      currSlice->max_part_nr = 1;
  }

  img->tr = currSlice->picture_id;
  img->type = currSlice->picture_type;
  img->qp = currSlice->qp;
  img->height_cr = (img->height)/2;
  img->width_cr = (img->width)/2;
  img->max_mb_nr = (img->width * img->height) / (MB_BLOCK_SIZE * MB_BLOCK_SIZE);

  if (inp->symbol_mode ==CABAC)
  {
    currSlice->last_mb_nr = currSlice->start_mb_nr+last_mb;
    // Note: if the last MB in this slice == last MB in frame then the number of MBs in this slice is coded as 0
    if (currSlice->last_mb_nr == currSlice->start_mb_nr)
      currSlice->last_mb_nr = img->max_mb_nr;
  }
}