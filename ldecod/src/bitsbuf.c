/*!
 *************************************************************************************
 * \file bitsbuf.c
 *
 * \brief
 *    Bit Stream Buffer Management
 *
 *  This module is introduced to facilitate the handling of the bit stream
 *  reading.  It assumes a start code taht is a UVLC code word with len 31
 *  and info 0 for Picture Start code, and info = 1 for Slice start code.
 *  The module should be used for all entropy coding schemes, including UVLC
 *  and CABAC.
 *************************************************************************************
 */

#include <stdlib.h>
#include <assert.h>

#include "global.h"
#include "bitsbuf.h"

static int SourceBitBufferPtr;    //!< the position for the next byte-write into the bit buffer
static FILE *bits;                //!< the bit stream file


/*!
 ************************************************************************
 * \brief
 *    Initializes the Source Bit Buffer
 ************************************************************************
 */
void InitializeSourceBitBuffer()
{
  SourceBitBufferPtr = 0;
}


/*!
 ************************************************************************
 * \brief
 *    returns the type of the start code at byte aligned position buf.
 *
 *  \return
 *     the info field of a start code (len == 31) or                      \n
 *     -1, indicating that there is no start code here
 *
 *  This function could be optimized considerably by checking for zero bytes first,
 *  but the current implementation gives us more freedom to define what a
 *  start code actually is
 *  Note that this function could be easily extended to search for non
 *  byte alogned start codes, by simply checking all 8 bit positions
 *  (and not only the zero position
 ************************************************************************
 */
static int TypeOfStartCode (byte *Buf)
{
  int info;

  // A bit of optimization first: a start code starts always with 3 zero bytes
  if ((Buf[0] != 0) || (Buf[1] != 0) || (Buf[2] != 0))
    return -1;

  if (31 != GetVLCSymbol (Buf, 0, &info, MAX_CODED_FRAME_SIZE))
  {
    return -1;
  }
  if (info != 0 && info != 1)   // the only two start codes currently allowed
    return -1;
  return info;
}

/*!
 ************************************************************************
 * \brief
 *    Returns the number of bytes in copied into the buffer
 *    The buffer includes the own start code, but excludes the
 *    next slice's start code
 * \return
 *     0 if there is nothing any more to read (EOF)                         \n
 *    -1 in case of any error
 *
 * \note
 *   GetOneSliceIntoSourceBitBuffer() expects start codes as follows:       \n
 *   Slice start code: UVLC, len == 31, info == 1                           \n
 *   Picture Start code: UVLC, len == 31, info == 0                         \n
 * \note
 *   getOneSliceIntoSourceBitBuffer expects Slice and Picture start codes
 *   at byte aligned positions in the file
 *
 ************************************************************************
 */
int GetOneSliceIntoSourceBitBuffer(byte *Buf)
{
  int info, pos;
  int StartCodeFound;

  InitializeSourceBitBuffer();
  // read the first 32 bits (which MUST contain a start code, or eof)
  if (4 != fread (Buf, 1, 4, bits))
  {
    return 0;
  }
  info = TypeOfStartCode (Buf);
  if (info < 0)
  {
    printf ("GetOneSliceIntoSourceBitBuffer: no Start Code at the begin of the slice, return -1\n");
    return -1;
  }
  if (info != 0 && info != 1)
  {
    printf ("GetOneSliceIntoSourceBitBuffer: found start code with invalid info %d, return -1\n", info);
    return -1;
  }
  pos = 4;
  StartCodeFound = 0;

  while (!StartCodeFound)
  {
    if (feof (bits))
      return pos;
    Buf[pos++] = fgetc (bits);

    info = TypeOfStartCode(&Buf[pos-4]);
    StartCodeFound = (info == 0 || info == 1);
  }

  // Here, we have found another start code (and read four bytes more than we should
  // have.  Hence, go back in the file

  if (0 != fseek (bits, -4, SEEK_CUR))
  {
    snprintf (errortext, ET_SIZE, "GetOneSliceIntoSourceBitBuffer: Cannot fseek -4 in the bit stream file");
    error(errortext, 600);
  }

  return (pos-4);
}




/*!
 ************************************************************************
 * \brief
 *    Attempts to open the bit stream file named fn
 * \return
 *    0 on success,                                                    \n
 *    -1 in case of error
 ************************************************************************
 */
int OpenBitstreamFile (char *fn)
{
  if (NULL == (bits=fopen(fn, "rb")))
    return -1;
  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    Closes the bit stream file
 ************************************************************************
 */
void CloseBitstreamFile()
{
  fclose (bits);
}

/*!
 ************************************************************************
 * \brief
 *    read bytes from input
 * \return
 *    Number of bytes read from partition
 ************************************************************************
 */
int GetOnePartitionIntoSourceBitBuffer(int PartitionSize, byte *Buf)
{
  int pos;
  InitializeSourceBitBuffer();
  for (pos=0; pos<PartitionSize; pos++)
  {
    if (feof (bits))
      return pos;
    Buf[pos] = fgetc (bits);
  }
  return pos;
}

/*!
 ************************************************************************
 * \brief
 *    read all partitions of one slice
 * \return
 *    TRUE if EOF reached                                               \n
 *    FALSE Else
 ************************************************************************
 */
int ReadPartitionsOfSlice (struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;
  Bitstream *currStream;
  DecodingEnvironmentPtr dep;
  byte *buf;
  int *read_len;
  int PartitionSize;
  int i=0, dt=0, j;
  unsigned int last_mb=0, picid=0;
  int eiflag=1;
  static int first=1;


  if (first)
  {
    currSlice->max_part_nr = MAX_PART_NR;
    first = FALSE;
  }

  for (i=0; i<currSlice->max_part_nr; i++)
  {
    if (fread (&eiflag, 1, 1, bits) !=1)  // delivered by the NAL
      return TRUE;    // EOF indication
    if (!eiflag)
    {
      fread (&PartitionSize, 4, 1, bits);   // delivered by the NAL
      // header in interim file format
      fread (&currSlice->picture_id, 4, 1, bits);
      fread (&dt, 4, 1, bits);
      if (!dt)
      {
        fread (&currSlice->picture_type, 4, 1, bits);
        fread (&currSlice->dp_mode, 4, 1, bits);
        fread (&img->height, 4, 1, bits);
        fread (&img->width, 4, 1, bits);
        fread (&currSlice->qp, 4, 1, bits);
        fread (&currSlice->start_mb_nr, 4, 1, bits);
        fread (&img->mv_res, 4, 1, bits);
        if (inp->symbol_mode == CABAC)
        {
          fread (&last_mb, 4, 1, bits);
        }
        DP_SliceHeader(last_mb,img,inp);
      }
      currStream = currSlice->partArr[dt].bitstream;
      read_len= &(currStream->read_len);

      free_Partition(currStream);

      buf = currStream->streamBuffer;
      currStream->bitstream_length = GetOnePartitionIntoSourceBitBuffer(PartitionSize, buf);
      if(inp->symbol_mode == CABAC)
      {
        dep = &((currSlice->partArr[dt]).de_cabac);
        arideco_start_decoding(dep, buf, 0, read_len);
      }
    }
    else
    {
      for(j=i; j<currSlice->max_part_nr; j++)
      {
        currStream = currSlice->partArr[j].bitstream;
        currStream->ei_flag =1;
      }
      if (!i)
        ec_header(img);
    }
  }
  currStream = currSlice->partArr[0].bitstream;
  if(currStream->ei_flag && inp->symbol_mode != CABAC)
    get_last_mb(img, inp);

  return FALSE;
}

/*!
 ************************************************************************
 * \brief
 *    go back to start of current slice (in case of lost partitions/slices)
 ************************************************************************
 */
void seek_back(int back)
{
  fseek (bits, -back, SEEK_CUR);
}

/*!
 ************************************************************************
 * \brief
 *    find last mb of slice
 ************************************************************************
 */
void get_last_mb(struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;
  int back=0;
  int PartitionSize, picture_id;
  int eiflag=1;
  int dt=0;
  int height, width, picture_type, dp_mode, mv_res;

  for (;;)
  {
    if (fread (&eiflag, 1, 1, bits) !=1)  // delivered by the NAL
      break;    // EOF indication
    if(!eiflag)
    {
      fread (&PartitionSize, 4, 1, bits);   // delivered by the NAL
      fread (&picture_id, 4, 1, bits);
      fread (&dt, 4, 1, bits);
      back += 12;
      if(!dt)
      {
        fread (&height, 4, 1, bits);
        fread (&width, 4, 1, bits);
        fread (&picture_type, 4, 1, bits);
        fread (&dp_mode, 4, 1, bits);
        fread (&currSlice->last_mb_nr, 4, 1, bits);
        fread (&mv_res, 4, 1, bits);
        back += 24;
      }
      else
      {
        fread (&currSlice->last_mb_nr, 4, 1, bits);
        back += 4;
      }

      if(picture_id != img->tr)
        currSlice->last_mb_nr = img->max_mb_nr;

      break;
    }
    back += 1;
  }
  seek_back(back); // go back the header we have read
}
