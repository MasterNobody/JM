/*
***********************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2001, International Telecommunications Union, Geneva
*
* DISCLAIMER OF WARRANTY
*
* These software programs are available to the user without any
* license fee or royalty on an "as is" basis. The ITU disclaims
* any and all warranties, whether express, implied, or
* statutory, including any implied warranties of merchantability
* or of fitness for a particular purpose.  In no event shall the
* contributor or the ITU be liable for any incidental, punitive, or
* consequential damages of any kind whatsoever arising from the
* use of these programs.
*
* This disclaimer of warranty extends to the user of these programs
* and user's customers, employees, agents, transferees, successors,
* and assigns.
*
* The ITU does not represent or warrant that the programs furnished
* hereunder are free of infringement of any third-party patents.
* Commercial implementations of ITU-T Recommendations, including
* shareware, may be subject to royalty fees to patent holders.
* Information regarding the ITU-T patent policy is available from
* the ITU Web site at http://www.itu.int.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE ITU-T PATENT POLICY.
************************************************************************
*/

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


#if TRACE
#define SYMTRACESTRING(s) strncpy(sym.tracestring,s,TRACESTRING_SIZE)
#else
#define SYMTRACESTRING(s) // to nothing
#endif


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
  int UsedBits=31;

  sym.type = SE_HEADER;
  sym.mapping = linfo;

  // 1. TRType = 0
  SYMTRACESTRING("PH TemporalReferenceType");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
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

  // 4. Picture Type indication (I, P, Mult_P, B , Mult_B, SP, Mult_SP)
  SYMTRACESTRING("PHPictureType");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  currSlice->picture_type = img->type = sym.value1;
  UsedBits += sym.len;

  // 5. Finally, read Reference Picture ID (same as TR here).  Note that this is an
  // optional field that is not present if the input parameters do not indicate
  // multiframe prediction ??

  // Ok, the above comment is nonsense.  There is no way how a decoder could
  // know that we use multiple reference frames (except probably through a
  // sequence header).  Hence, it's now an if (1) -- PHRefPicID is always present

  // Of course, the decoder can know. It is indicated by the previously decoded
  // parameter "PHPictureType". So, I changed the if-statement again to be
  // compatible with the encoder.

  // WYK: Oct. 16, 2001. Now I use this for the reference frame ID (non-B frame ID). 
  // Thus, we can know how many  non-B frames are lost, and then we can adjust 
  // the reference frame buffers correctly.
  if (1)
  {
    // refPicID, variable length
    SYMTRACESTRING("PHRefPicID");
    readSyntaxElement_UVLC (&sym,img,inp,partition);
    if (img->refPicID != sym.value1)
    {
      img->refPicID_old = img->refPicID;
      img->refPicID = sym.value1;
    }
    UsedBits += sym.len;
  }

  // 6. Get MB-Adresse
  SYMTRACESTRING("SH FirstMBInSlice");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  currSlice->start_mb_nr = sym.value1;
  UsedBits += sym.len;

  // 7. Get Quant.
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
  // 8. Get MVResolution
  SYMTRACESTRING("SH MVResolution");
  readSyntaxElement_UVLC (&sym,img,inp,partition);
  img->mv_res = sym.value1;
  UsedBits += sym.len;

  img->max_mb_nr = (img->width * img->height) / (MB_BLOCK_SIZE * MB_BLOCK_SIZE);
  if (inp->symbol_mode ==CABAC)
  {
    // 9. Get number of MBs in this slice
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

