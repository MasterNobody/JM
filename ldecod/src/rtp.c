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
 ************************************************************************
 * \file  rtp.c
 *
 * \brief
 *    Network Adaptation layer for RTP packets
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Stephan Wenger   <stewe@cs.tu-berlin.de>
 ************************************************************************
 */

#include "contributors.h"

#include <assert.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "global.h"
#include "elements.h"
#include "bitsbuf.h"
#include "rtp.h"

extern void tracebits(const char *trace_str,  int len,  int info,int value1,
    int value2) ;

extern FILE *bits;
#define MAX_PARAMETER_STRINGLEN 1000

static int CurrentParameterSet = -1;

typedef struct
{
  int FramesToBeEncoded;
  int FrameSkip;
  char SequenceFileName[MAX_PARAMETER_STRINGLEN];
} InfoSet_t;

static InfoSet_t InfoSet;

ParameterSet_t ParSet[RTP_MAX_PARAMETER_SET];



/*!
 ************************************************************************
 * \brief
 *    read all Partitions of one Slice from RTP packets
 * \return
 *    SOS if this is not a new frame                                    \n
 *    SOP if this is a new frame                                        \n
 *    EOS if end of sequence reached
 ************************************************************************
 */
int readSliceRTP (struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;
  int PartitionMask;

  assert (currSlice != NULL);

  PartitionMask = ReadRTPPacket (img, inp, bits);

  if(PartitionMask == -4711)
    return EOS;

  if (PartitionMask == 1)     // got a full slice, PAR_DP_1
  {
    
    if(currSlice->start_mb_nr != 0)
      return SOS;
    else
      return SOP;
  }

  printf ("readSlice_RTP -- we shouldn't be here\n");
  assert ("Data Partitioning not yet supported\n");
}

/*!
 ************************************************************************
 * \brief
 *    Input file containing Partition structures
 * \return
 *    TRUE if a startcode follows (meaning that all symbols are used).  \n
 *    FALSE otherwise.
 ************************************************************************/
int RTP_startcode_follows(struct img_par *img, struct inp_par *inp)
{
  Slice *currSlice = img->currentSlice;
  int dp_Nr = assignSE2partition[inp->partition_mode][SE_MBTYPE];
  DataPartition *dP = &(currSlice->partArr[dp_Nr]);
  Bitstream   *currStream = dP->bitstream;
  byte *buf = currStream->streamBuffer;
  int frame_bitoffset = currStream->frame_bitoffset;
  int info;

  if (currStream->ei_flag)
  {
    printf ("ei_flag set, img->current_mb_nr %d, currSlice->last_mb_nr %d\n", img->current_mb_nr, currSlice->last_mb_nr);
    return (img->current_mb_nr == currSlice->last_mb_nr);
  }
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
int readSyntaxElement_RTP(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dP)
{
  Bitstream   *currStream = dP->bitstream;

  if (RTP_symbols_available(currStream))    // check on existing elements in partition
  {
    RTP_get_symbol(sym, currStream);
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
int RTP_symbols_available (Bitstream *currStream)
{
  byte *buf = currStream->streamBuffer;
  int frame_bitoffset = currStream->frame_bitoffset;
  int info;

  if (currStream->ei_flag) {
    printf ("RTP_symbols_available returns FALSE: ei_flag set\n");
    return FALSE;
  }
  if (-1 == GetVLCSymbol (buf, frame_bitoffset, &info, currStream->bitstream_length))
  {
    printf ("RTP_symbols_available returns FALSE: GETVLCSymbol returns -1\n");
    return FALSE;
  }
  else
    return TRUE;
};

/*!
 ************************************************************************
 * \brief
 *    gets info and len of symbol
 ************************************************************************
 */
void RTP_get_symbol(SyntaxElement *sym, Bitstream *currStream)
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
 *    Interrepts a parameter set packet
 ************************************************************************
 */

#define EXPECT_ATTR 0
#define EXPECT_PARSET_LIST 1
#define EXPECT_PARSET_NO 2
#define EXPECT_STRUCTNAME 3
#define EXPECT_STRUCTVAL_INT 4
#define EXPECT_STRUCTVAL_STRING 5

#define INTERPRET_COPY 100
#define INTERPRET_ENTROPY_CODING 101
#define INTERPRET_MOTION_RESOLUTION 102
#define INTERPRET_INTRA_PREDICTION 103
#define INTERPRET_PARTITIONING_TYPE 104


int RTPInterpretParameterSetPacket (char *buf, int buflen)

{
  // The dumbest possible parser that updates the parameter set variables 
  // static to this module

  int bufp = 0;
  int state = EXPECT_ATTR;
  int interpreter;
  int minus, number;
  int ps;
  char s[MAX_PARAMETER_STRINGLEN];
  void *destin;

  while (bufp < buflen)
  {
// printf ("%d\t%d\t %c%c%c%c%c%c%c%c  ", bufp, state, buf[bufp], buf[bufp+1], buf[bufp+2], buf[bufp+3], buf[bufp+4], buf[bufp+5], buf[bufp+6], buf[bufp+7], buf[bufp+8], buf[bufp+9]);

    switch (state)
    {
    case EXPECT_ATTR:
      if (buf[bufp] == '\004')      // Found UNIX EOF, this is the end marker
        return 0;
      if (strncmp ("a=H26L ", &buf[bufp], 7))
      {
        printf ("Parsing error EXPECT_ATTR in Header Packet: position %d, packet %s\n",
                 bufp, &buf[bufp]);
        return -1;
      }
      bufp += 7;
      state = EXPECT_PARSET_LIST;
      break;

    case EXPECT_PARSET_LIST:
      if (buf[bufp] != '(')
      {
        printf ("Parsing error EXPECT_PARSET in Header Packet: position %d, packet %s\n",
                bufp, &buf[bufp]);
        return -2;
      }
      bufp++;
      state = EXPECT_PARSET_NO;
      break;

    case EXPECT_PARSET_NO:
      number = 0;
      minus = 1;
      if (buf[bufp] == '-')
      {
        minus = -1;
        bufp++;
      }
      while (isdigit (buf[bufp]))
        number = number * 10 + ( (int)buf[bufp++] - (int) '0');
      if (buf[bufp] == ',')
      {
        printf ("Update of more than one prameter set not yet supported\n");
        return -1;
      }
      if (buf[bufp] != ')')
      {
        printf ("Parsing error NO PARSET LISTEND in Header Packet: position %d, packet %s\n",
          bufp, &buf[bufp]);
        return -1;
      }
      if (minus > 0)      // not negative
        ps = number;

      bufp+= 2;     // skip ) and blank
      state = EXPECT_STRUCTNAME;
      break;

    case EXPECT_STRUCTNAME:
      if (1 != sscanf (&buf[bufp], "%100s", s))   // Note the 100, which si MAX_PARAMETER_STRLEN
      {
        printf ("Parsing error EXPECT STRUCTNAME STRING in Header packet: position %d, packet %s\n",
               bufp, &buf[bufp]);
        return -1;
      }
      bufp += strlen (s);
      bufp++;       // Skip the blank


      if (!strncmp (s, "MaxPicID", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].MaxPicID;
        break;
      }
      if (!strncmp (s, "BufCycle", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].BufCycle;
        break;
      }
      if (!strncmp (s, "UseMultpred", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].UseMultpred;
        break;
      }

      if (!strncmp (s, "PixAspectRatioX", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].PixAspectRatioX;
        break;
      }
      if (!strncmp (s, "PixAspectRatioY", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].PixAspectRatioY;
        break;
      }
      if (!strncmp (s, "DisplayWindowOffsetTop", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].DisplayWindowOffsetTop;
        break;
      }
      if (!strncmp (s, "DisplayWindowOffsetBottom", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].DisplayWindowOffsetBottom;
        break;
      }
      if (!strncmp (s, "DisplayWindowOffsetRight", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].DisplayWindowOffsetRight;
        break;
      }
      if (!strncmp (s, "DisplayWindowOffsetLeft", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].DisplayWindowOffsetLeft;
        break;
      }
      if (!strncmp (s, "XSizeMB", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].XSizeMB;
        break;
      }
      if (!strncmp (s, "YSizeMB", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].YSizeMB;
        break;
      }
      if (!strncmp (s, "EntropyCoding", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_STRING;
        interpreter = INTERPRET_ENTROPY_CODING;
        destin = &ParSet[ps].EntropyCoding;
        break;
      }
      if (!strncmp (s, "MotionResolution", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_STRING;
        interpreter = INTERPRET_MOTION_RESOLUTION;
        destin = &ParSet[ps].MotionResolution;
        break;
      }
      if (!strncmp (s, "PartitioningType", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_STRING;
        interpreter = INTERPRET_PARTITIONING_TYPE;
        destin = &ParSet[ps].PartitioningType;
        break;
      }
      if (!strncmp (s, "IntraPredictionType", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_STRING;
        interpreter = INTERPRET_INTRA_PREDICTION;
        destin = &ParSet[ps].IntraPredictionType;
        break;
      }
      if (!strncmp (s, "HRCParameters", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &ParSet[ps].HRCParameters;
        break;
      }
      if (!strncmp (s, "FramesToBeEncoded", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &InfoSet.FramesToBeEncoded;
        break;
      }
      if (!strncmp (s, "FrameSkip", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_INT;
        interpreter = INTERPRET_COPY;
        destin = &InfoSet.FrameSkip;
        break;
      }
      if (!strncmp (s, "SequenceFileName", MAX_PARAMETER_STRINGLEN))
      {
        state = EXPECT_STRUCTVAL_STRING;
        interpreter = INTERPRET_COPY;
        destin = &InfoSet.SequenceFileName;
        break;
      }
     
      // Here, all defined Parameter names are checked.  Anything else is a syntax error
      printf ("Syntax Error: unknown Parameter %s\n", s);
      printf ("Parsing error in Header Packet: position %d, packet %s\n",
          bufp, buf);
      return -3;
      break;        // to make lint happy
    
    case EXPECT_STRUCTVAL_INT:
      if (1!=sscanf (&buf[bufp], "%d", destin))
      {
        printf ("Parsing error EXPECT STRUCTVAL INT in Header Packet: position %d, packet %s\n",
          bufp, &buf[bufp]);
        return -4;
      }
// printf ("EXPECT_STRCUTVAL_INT: write %d\n", * (int *)destin);
      while (bufp < buflen && buf[bufp] != '\n')    // Skip any trailing whitespace and \n
        bufp++;
      bufp++;
      state=EXPECT_ATTR;
      break;
      
    case EXPECT_STRUCTVAL_STRING:
      if (1 != sscanf (&buf[bufp], "%100s", s))
      {
        printf ("Parsing error EXPECT STRUCTVAL STRING in Header Packet: position %d, packet %s\n",
          bufp, &buf[bufp]);
        return -5;
      }
      while (bufp < buflen && buf[bufp] != '\n')   // Skip any trailing whitespace and \n
        bufp++;
      bufp++;
      state=EXPECT_ATTR;

      switch (interpreter)
      {
      case INTERPRET_COPY:
        // nothing -- handled where it occurs
        break;
      case INTERPRET_ENTROPY_CODING:
        if (!strncmp (s, "UVLC", 4))
          * (int *)destin = 0;
        else
          * (int *)destin = 1;
//        printf ("in INterpret, Entropy COding :%s: results in %d\n", s, *(int *)destin);
        break;
      case INTERPRET_MOTION_RESOLUTION:
        if (!strncmp (s, "quater", 6))
          * (int *)destin = 0;
        else
          * (int *)destin = 1;
        break;
      case INTERPRET_INTRA_PREDICTION:
        if (!strncmp (s, "InterPredicted", 14))
          * (int *)destin = 0;
        else
          * (int *)destin = 1;
        break;
      case INTERPRET_PARTITIONING_TYPE:
        if (!strncmp (s, "one", 3))
          * (int *)destin = 0;
        else
          * (int *)destin = 1;
        break;
      default:
        assert (0==1);
      }
    break;

    default:
      printf ("Parsing error UNDEFINED SYNTAX in Header Packet: position %d, packet %s\n",
        bufp, &buf[bufp]);
      return -1;
    }
//  printf ("\t\t%d\n", bufp);
  }

//  printf ("CurrentParameterSet %d, ps %d\n", CurrentParameterSet, ps);
//  printf ("RTPInterpretParameterPacket: xsize x Ysize, %d x %d, Entropy %d, Motion %d  MaxPicId %d\n",
//    ParSet[ps].XSizeMB, ParSet[ps].YSizeMB, ParSet[ps].EntropyCoding, ParSet[ps].MotionResolution, ParSet[ps].MaxPicID);
  ParSet[ps].Valid = 1;
  return 0;
}


/*!
 ************************************************************************
 * \brief
 *    Update img->xxx with the content of a parameter set, called for 
 *    every slice 
 ************************************************************************
 */



int RTPUseParameterSet (int n, struct img_par *img, struct inp_par *inp)
{
  int status;
  
  if (n == CurrentParameterSet)
    return RTP_PARAMETER_SET_OK;   // no change

//  printf ("Use a new parameter set: old %d, new %d\n", CurrentParameterSet, n);
  CurrentParameterSet = n;

  status = RTP_PARAMETER_SET_OK;

  if (CurrentParameterSet < 0 || (CurrentParameterSet > RTP_MAX_PARAMETER_SET))
  {
    printf ("Parameter Set %d out of range, conceal to 0\n", CurrentParameterSet);
    CurrentParameterSet = 0;    // and pray that it works...
    status = RTP_PARAMETER_SET_OUT_OF_RANGE;
  }

  if (!ParSet[CurrentParameterSet].Valid)
  {
    printf ("Try to use uninitialized Parameter Set %d, conceal to 0\n", CurrentParameterSet);
    CurrentParameterSet = 0;
    status = RTP_PARAMETER_SET_INVALID;
  }

  // Now updates global decoder variables with the appropriate parameter set.

  // A full-fledged decoder would make some consistency checks.  For example,
  // it makes sense to change the MotionResolution or the EntropyCode within
  // a picture (img->current_mb_nr != 0).  It doesn't make sense to change
  // the pixel aspect ratio.
  // There is no need to do those checks in an error free environment -- an
  // encoder is simply not supposed to do so.  In error prone environments,
  // however, this is an additional means for error detection.

  // Note: Many parameters are available in both the input-> and img-> structures.
  // Some people seem to use the input-> parameters, others the img-> parameters.
  // This should be cleaned up one day.
  // For now simply copy any updated variables into both structures.

  // MaxPicID: doesn't exist in pinput-> or img->
  inp->buf_cycle = ParSet[CurrentParameterSet].BufCycle;
  img->buf_cycle = inp->buf_cycle+1;      // see get_mem4global_buffers()

  // PixAspectRatioX: doesn't exist
  // PixAspectRatioY: doesn't exist
  // DisplayWindowOffset*: doesn't exist

  // XSizeMB
  img->width = ParSet[CurrentParameterSet].XSizeMB*16;
  img->width_cr = ParSet[CurrentParameterSet].XSizeMB*8;

  //YSizeMB
  img->height = ParSet[CurrentParameterSet].YSizeMB*16;
  img->height_cr = ParSet[CurrentParameterSet].YSizeMB*8;

  // EntropyCoding
  if (ParSet[CurrentParameterSet].EntropyCoding == 0)
    inp->symbol_mode = UVLC;
  else
    inp->symbol_mode = CABAC;

  // MotionResolution
  img->mv_res = ParSet[CurrentParameterSet].MotionResolution;

  // PartitioningType
  inp->partition_mode = ParSet[CurrentParameterSet].PartitioningType;

  // IntraPredictionType
  inp->UseConstrainedIntraPred = ParSet[CurrentParameterSet].IntraPredictionType;
  
  //! img->type: This is calculated by using ParSet[CurrentParameterSet].UseMultpred
  //! and the slice type from the slice header.  It is set in RTPSetImgInp()

  // HRCParameters: Doesn't exist
}

  
/*!
 ************************************************************************
 * \brief
 *    read all partitions of one slice from RTP packet stream, also handle
 *    any Parameter Update packets and SuUPP-Packets
 * \return
 *    -1 for EOF                                              \n
 *    Partition-Bitmask otherwise:
 *    Partition Bitmask: 0x1: Full Slice, 0x2: type A, 0x4: Type B 
 *                       0x8: Type C
 ************************************************************************
 */
int ReadRTPPacket (struct img_par *img, struct inp_par *inp, FILE *bits)
{
  Slice *currSlice = img->currentSlice;
  DecodingEnvironmentPtr dep;
  byte *buf;
  int TotalPackLen;
  int i=0, dt=0;
  unsigned int last_mb=0, picid=0;
  int eiflag=1;
  static int first=1;
  RTPpacket_t *p, *nextp;
  RTPSliceHeader_t *sh, *nextsh;
  int DoUnread = 0;
  int MBDataIndex;
  int PartitionMask = 0;
  int done = 0;
  int *read_len;        
  int err, back=0;
  int intime=0;
  int ei_flag;
  static unsigned int old_seq=0;

  assert (currSlice != NULL);
  assert (bits != 0);

  if (first)
  {
    currSlice->max_part_nr = MAX_PART_NR;   // Just to get a start
    first = FALSE;
  }
  p=alloca (sizeof (RTPpacket_t));
  p->packet=alloca (MAXRTPPACKETSIZE);
  p->payload=alloca (MAXRTPPAYLOADLEN);
  nextp=alloca (sizeof (RTPpacket_t));
  sh=alloca(sizeof (RTPSliceHeader_t));
  nextsh=alloca(sizeof (RTPSliceHeader_t));

  while (!done)
  {

    // the alloca() used to be here
    if (4 != fread (&TotalPackLen,1, 4, bits))
      return -4711;    // EOF inidication

    p->packlen = TotalPackLen;
    if (4 != fread (&intime, 1, 4, bits))
    {
      printf ("RTP File corruption, unexpected end of file, tried to read intime\n");
      return -4711;
    }

    if (p->packlen != fread (p->packet, 1, p->packlen, bits))
    {
      // The corruption of a packet file is not a case we should handle.
      // In a real-world system, RTP packets may get lost, but they will
      // never get shortened.  Hence, the error checked here cannot occur.
      printf ("RTP File corruption, unexpected end of file, tried to read %d bytes\n", p->packlen);
      return -4711;    // EOF
    }

    // Here we have the complete RTP packet in p->packet,a nd p->packlen set
    p->paylen = p->packlen-12;             // 12 bytes RTP header

    if (DecomposeRTPpacket (p) < 0)
    {
      // this should never happen, hence exit() is ok.  We probably do not want to attempt
      // to decode a packet that obviously wasn't generated by RTP
      printf ("Errors reported by DecomposePacket(), exit\n");
      exit (-700);
    }

    if (p->seq != old_seq+1)
      ei_flag=1;
    else
    {
      eiflag=0;
      old_seq=p->seq;
    }

    if (eiflag)
    {
      currSlice->ei_flag=1;
      back=p->packlen+8;                //! TO 4.11.2001 we read a packet that we can 
      fseek (bits, -back, SEEK_CUR);    //! not use now, so go back
      currSlice->start_mb_nr = img->current_mb_nr; //! This is true in any case
      //! Now get the information of the next packet header
      //! As this is the same packet we just read this may seem
      //! not very logical but it will be if we allow more than one Partition
      currSlice->next_header = RTPGetFollowingSliceHeader (img, nextp, nextsh); 
      old_seq = get_lastMB (img, nextsh, nextp);  //! get the last MB that has to be concealed
      return 1;                                   //! same return as for Full Slice partition
    }

    // Here the packet is ready for interpretation

    assert (p->pt == H26LPAYLOADTYPE);
    assert (p->ssrc == 0x12345678);

    switch (p->payload[0] & 0xf)    // the type field
    {
    case 0:   // Full Slice


      //! This is the place to insert code top generate empty (lost) slice info with ei_flag set,
      //! based on information in nextp and nextsh

      currSlice->ei_flag = 0;
      MBDataIndex = RTPInterpretSliceHeader (&p->payload[1], p->paylen-1, 0, sh);
      MBDataIndex++;    // Skip First Byte
   
      RTPUseParameterSet (sh->ParameterSet, img, inp);

      currSlice->dp_mode = PAR_DP_1;
      currSlice->max_part_nr=1;

      RTPSetImgInp(img, inp, sh);

      free_Partition (currSlice->partArr[0].bitstream);
      
      read_len = &(currSlice->partArr[0].bitstream->read_len);
      *read_len = p->paylen - MBDataIndex;
      assert (p->paylen-MBDataIndex > 0);
      currSlice->partArr[0].bitstream->bitstream_length = p->paylen-MBDataIndex;

      memcpy (currSlice->partArr[0].bitstream->streamBuffer, &p->payload[MBDataIndex],p->paylen-MBDataIndex);
      buf = currSlice->partArr[0].bitstream->streamBuffer;

      if(inp->symbol_mode == CABAC)
      {
        dep = &((currSlice->partArr[0]).de_cabac);
        arideco_start_decoding(dep, buf, 0, read_len);
      }

      // At this point the slice is ready for decoding. 
      
      currSlice->next_header = RTPGetFollowingSliceHeader (img, nextp, nextsh); // no use for the info in nextp, nextsh yet. 

      return 1;

      break;

    case 1:   // Type A Partition (Header and MVs)
      printf ("Found Type A Patrition\n");
      assert (0==1);
      MBDataIndex = RTPInterpretSliceHeader (&p->payload[1], p->paylen-1, 1, sh);
      MBDataIndex++;    // Skip First Byte

      currSlice->dp_mode = PAR_DP_3;
      img->qp = currSlice->qp = sh->InitialQP;
      currSlice->start_mb_nr = (img->width/16)*sh->FirstMBInSliceY+sh->FirstMBInSliceX;
      currSlice->max_part_nr=1;
      img->tr = currSlice->picture_id = sh->PictureID;
      img->type = currSlice->picture_type = sh->SliceType;
      currSlice->last_mb_nr = currSlice->start_mb_nr + sh->CABAC_LastMB;
      if (currSlice->last_mb_nr == currSlice->start_mb_nr)
        currSlice->last_mb_nr = img->max_mb_nr;

      free_Partition (currSlice->partArr[0].bitstream);

      currSlice->partArr[0].bitstream->read_len = p->paylen-MBDataIndex;
      read_len = &(currSlice->partArr[0].bitstream->read_len);
      currSlice->partArr[0].bitstream->bitstream_length = p->paylen-MBDataIndex;

      memcpy (currSlice->partArr[0].bitstream, &p->payload[MBDataIndex],p->paylen-MBDataIndex);
      buf = currSlice->partArr[0].bitstream->streamBuffer;

      if(inp->symbol_mode == CABAC)
      {
        dep = &((currSlice->partArr[0]).de_cabac);
        arideco_start_decoding(dep, buf, 0, read_len);
      }

      return 1;

      break;
    case 2:   // Type B Partition (Intra CBPs and Coefficients)
      printf ("Found Type B Partition\n");
      assert (0==1);
      MBDataIndex = RTPInterpretPartitionHeader (&p->payload[1], p->paylen-1, sh);
      free_Partition (currSlice->partArr[1].bitstream);

      currSlice->partArr[1].bitstream->read_len = p->paylen-MBDataIndex;
      read_len = &(currSlice->partArr[0].bitstream->read_len);
      currSlice->partArr[1].bitstream->bitstream_length = p->paylen-MBDataIndex;

      memcpy (currSlice->partArr[1].bitstream, &p->payload[MBDataIndex],p->paylen-MBDataIndex);
      buf = currSlice->partArr[0].bitstream->streamBuffer;

      if(inp->symbol_mode == CABAC)
      {
        dep = &((currSlice->partArr[1]).de_cabac);
        arideco_start_decoding(dep, buf, 0, read_len);
      }

      return 2;

      break;
    case 3:   // Type C Partition (Inter CBP and coefficients)
      printf ("Foudn type C partition\n");
      assert (0==1);
      MBDataIndex = RTPInterpretPartitionHeader (&p->payload[1], p->paylen-1, sh);
      free_Partition (currSlice->partArr[2].bitstream);

      currSlice->partArr[2].bitstream->read_len = p->paylen-MBDataIndex;
      read_len = &(currSlice->partArr[0].bitstream->read_len);
      currSlice->partArr[2].bitstream->bitstream_length = p->paylen-MBDataIndex;

      memcpy (currSlice->partArr[2].bitstream, &p->payload[MBDataIndex],p->paylen-MBDataIndex);
      buf = currSlice->partArr[0].bitstream->streamBuffer;

      if(inp->symbol_mode == CABAC)
      {
        dep = &((currSlice->partArr[1]).de_cabac);
        arideco_start_decoding(dep, buf, 0, read_len);
      }

      return 4;

      // interpret partition header
      // check timestamp
      // if yes: copy.  if no: unread, return with indication of missing info
      break;

    case 4:   // Compound packet
      printf ("Comound packets not yet implemented, exit()\n");
      exit (-700);
      break;
    case 5:   // SUPP packet
      printf ("SUPP packets not yet implemented, exit()\n");
      exit (-700);
      break;
    case 6:   // Header packet
      printf ("Found header packet\n");
      if ((err = RTPInterpretParameterSetPacket (&p->payload[1], p->paylen-1)) < 0)
      {
        printf ("RTPInterpretParameterSetPacket returns error %d\n", err);
      }

      break;
    default:
      printf ("RTP First Byte 0x%x, type %d not defined, exit\n", p->payload[0], p->payload[0]&0xf);
      exit (-1);
    }
  }
    

  return FALSE;
}


/*!
 *****************************************************************************
 *
 * \brief 
 *    DecomposeRTPpacket interprets the RTP packet and writes the various
 *    structure members of the RTPpacket_t structure
 *
 * \return
 *    0 in case of success
 *    negative error code in case of failure
 *
 * \para Parameters
 *    Caller is responsible to allocate enough memory for the generated payload
 *    in parameter->payload. Typically a malloc of paclen-12 bytes is sufficient
 *
 * \para Side effects
 *    none
 *
 * \para Other Notes
 *    Function contains assert() tests for debug purposes (consistency checks
 *    for RTP header fields)
 *
 * \date
 *    30 Spetember 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

int DecomposeRTPpacket (RTPpacket_t *p)

{
  // consistency check 
  assert (p->packlen < 65536 - 28);  // IP, UDP headers
  assert (p->packlen >= 12);         // at least a complete RTP header
  assert (p->payload != NULL);
  assert (p->packet != NULL);

  // Extract header information

  p->v  = p->packet[0] & 0x3;
  p->p  = (p->packet[0] & 0x4) >> 2;
  p->x  = (p->packet[0] & 0x8) >> 3;
  p->cc = (p->packet[0] & 0xf0) >> 4;

  p->m  = p->packet[1] & 0x1;
  p->pt = (p->packet[1] & 0xfe) >> 1;

  p->seq = p->packet[2] | (p->packet[3] << 8);

  memcpy (&p->timestamp, &p->packet[4], 4);// change to shifts for unified byte sex
  memcpy (&p->ssrc, &p->packet[8], 4);// change to shifts for unified byte sex

  // header consistency checks
  if (     (p->v != 2)
        || (p->p != 0)
        || (p->x != 0)
        || (p->cc != 0) )
  {
    printf ("DecomposeRTPpacket, RTP header consistency problem, header follows\n");
    DumpRTPHeader (p);
    return -1;
  }
  p->paylen = p->packlen-12;
  memcpy (p->payload, &p->packet[12], p->paylen);
  return 0;
}

/*!
 *****************************************************************************
 *
 * \brief 
 *    DumpRTPHeader is a debug tool that dumps a human-readable interpretation
 *    of the RTP header
 *
 * \return
 *    n.a.
 * \para Parameters
 *    the RTP packet to be dumped, after DecompositeRTPpacket()
 *
 * \para Side effects
 *    Debug output to stdout
 *
 * \date
 *    30 Spetember 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

void DumpRTPHeader (RTPpacket_t *p)

{
  int i;
  for (i=0; i< 30; i++)
    printf ("%02x ", p->packet[i]);
  printf ("Version (V): %d\n", p->v);
  printf ("Padding (P): %d\n", p->p);
  printf ("Extension (X): %d\n", p->x);
  printf ("CSRC count (CC): %d\n", p->cc);
  printf ("Marker bit (M): %d\n", p->m);
  printf ("Payload Type (PT): %d\n", p->pt);
  printf ("Sequence Number: %d\n", p->seq);
  printf ("Timestamp: %d\n", p->timestamp);
  printf ("SSRC: %d\n", p->ssrc);
}

/*!
 *****************************************************************************
 *
 * \brief 
 *    Parses and interprets the UVLC-coded slice header
 *
 * \return
 *    negative in case of errors, the byte-index where the UVLC/CABAC MB data
 *    starts otherwise
 *
 * \date
 *    27 October, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

int RTPInterpretSliceHeader (byte *buf, int bufsize, int ReadSliceId, RTPSliceHeader_t *sh)
{
  int len, info, bytes, dummy, bitptr=0;
  
  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->ParameterSet, &dummy);
  bitptr+=len;

  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->PictureID, &dummy);
  bitptr+=len;

  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->SliceType, &dummy);
  bitptr+=len;

  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->FirstMBInSliceX, &dummy);
  bitptr+=len;

  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->FirstMBInSliceY, &dummy);
  bitptr+=len;

  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->InitialQP, &dummy);
  bitptr+=len;
  sh->InitialQP = 31-sh->InitialQP;

  if (sh->SliceType==2) // SP Picture
  {
    len = GetVLCSymbol(buf, bitptr, &info, bufsize);
    linfo (len, info, &sh->InitialSPQP, &dummy);
    bitptr+=len;
    sh->InitialSPQP = 31-sh->InitialSPQP;
  }

  assert (sh->ParameterSet == 0);     // only for testing, should be deleted as soon as more than one parameter set is generated by trhe encoder
  assert (sh->SliceType > 0 || sh->SliceType < 5);
  assert (sh->InitialQP >=0 && sh->InitialQP < 32);
  assert (sh->InitialSPQP >=0 && sh->InitialSPQP < 32);


  if (ReadSliceId)
  {
    len = GetVLCSymbol(buf, bitptr, &info, bufsize);
    linfo (len, info, &sh->SliceID, &dummy);
    bitptr+=len;
  }

  if (ParSet[sh->ParameterSet].EntropyCoding == 1)   // CABAC in use, need to get LastMB
  {
    len = GetVLCSymbol(buf, bitptr, &info, bufsize);
    linfo (len, info, &sh->CABAC_LastMB, &dummy);
    bitptr+=len;
  }

  bytes = bitptr/8;
  if (bitptr%8)
    bytes++;

  return bytes;

}



/*!
 *****************************************************************************
 *
 * \brief 
 *    Parses and interprets the UVLC-coded partition header (Type B and C packets only)
 *
 * \return
 *    negative in case of errors, the byte-index where the UVLC/CABAC MB data
 *    starts otherwise
 * Side effects:
 *    sh->PictureID und sh->SliceID set, all other values unchanged
 *
 * \date
 *    27 October, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

int RTPInterpretPartitionHeader (byte *buf, int bufsize, RTPSliceHeader_t *sh)
{
  int len, info, bytes, dummy, bitptr=0;
  
  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->PictureID, &dummy);
  bitptr+=len;

  len = GetVLCSymbol(buf, bitptr, &info, bufsize);
  linfo (len, info, &sh->SliceID, &dummy);
  bitptr+=len;

  printf ("InterpretPartitionHeader: PicId %d, SliceID %d \n",
    sh->PictureID, sh->SliceID);

  bytes = bitptr/8;
  if (bitptr%8)
    bytes++;

  return bytes;

}

/*!
 *****************************************************************************
 *
 * \brief 
 *    Reads and interprets the RTP sequence header, expects a type 6 packet
 *
 * \return
 *
 * Side effects:
 *   sets several fields in the img-> and inp-> structure, see RTPUseParameterSet
 *
 * \date
 *    27 October, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

// Each RTP File is supposed to start with a type 6 (Header) packet.  It is necessary
// to read this early on in order to allocate the memory for the decoder.  This should
// be fixed some day in such a way that the decoder allocates memory as needed, and
// not statically at the first frame.

int RTPSequenceHeader (struct img_par *img, struct inp_par *inp, FILE *bits)
{
  int TotalPackLen;
  int i=0, dt=0;
  unsigned int last_mb=0, picid=0;
  int eiflag=1;
  static int first=1;
  RTPpacket_t *p;
  RTPSliceHeader_t *sh;
  int DoUnread = 0;
  int PartitionMask = 0;
  int done = 0;
  int err;
  int intime=0;

  assert (bits != NULL);

  p=alloca (sizeof (RTPpacket_t));
  sh=alloca(sizeof (RTPSliceHeader_t));

  if (4 != fread (&TotalPackLen,1, 4, bits))
    return -4711;    // EOF inidication
  if (4 != fread (&intime, 1, 4, bits))
    return -4712;

  p->packlen = TotalPackLen;
  p->packet = alloca (p->packlen);
  if (p->packlen != fread (p->packet, 1, p->packlen, bits))
    {
      // The corruption of a packet file is not a case we should handle.
      // In a real-world system, RTP packets may get lost, but they will
      // never get shortened.  Hence, the error checked here cannot occur.
      printf ("RTP File corruption, unexpected end of file, tried to read %d bytes\n", p->packlen);
      return -4713;    // EOF
    }

  p->paylen = p->packlen - 12;          // 12 bytes RTP header
  p->payload = alloca (p->paylen);   

  if (DecomposeRTPpacket (p) < 0)
    {
      // this should never happen, hence exit() is ok.  We probably do not want to attempt
      // to decode a packet that obviously wasn't generated by RTP
      printf ("Errors reported by DecomposePacket(), exit\n");
      exit (-700);
    }

    // Here the packet is ready for interpretation

  assert (p->pt == H26LPAYLOADTYPE);
  assert (p->ssrc == 0x12345678);

  if (p->payload[0] != 6)
  {
    printf ("RTPSequenceHeader: Expect Header Packet (FirstByet = 6), found packet type %d\n", p->payload[0]);
    exit (-1);
  }

  if ((err = RTPInterpretParameterSetPacket (&p->payload[1], p->paylen-1)) < 0)
    {
      printf ("RTPInterpretParameterSetPacket returns error %d\n", err);
    }

  RTPUseParameterSet (0, img, inp);
  img->number = 0;
  
}


/*!
 *****************************************************************************
 *
 * \brief 
 *    Sets various img->, inp-> and currSlice->struct members according to 
 *    the contents of the sh-> slice header structure
 *
 * \return
 *
 * Side effects:
 *    Set img->       qp, current_slice_nr, type, tr
 *    Set inp->
 *    Set currSlice-> qp, start_mb_nr, slice_nr, picture_type, picture_id (CABAC only: last_mb_nr)
 *
 * \date
 *    27 October, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/

void RTPSetImgInp (struct img_par *img, struct inp_par *inp, RTPSliceHeader_t *sh)
{
  static int ActualPictureType;
  Slice *currSlice = img->currentSlice;
 
  img->qp = currSlice->qp = sh->InitialQP;

  if (sh->SliceType==2)
    img->qpsp = sh->InitialSPQP;

  currSlice->start_mb_nr = (img->width/16)*sh->FirstMBInSliceY+sh->FirstMBInSliceX;

  if (currSlice->start_mb_nr == 0)
    currSlice->slice_nr=0;
  else                        
    currSlice->slice_nr++; //! TO 2.10.2001 changed (was currSlice->slice_nr=0;)
  img->current_slice_nr = currSlice->slice_nr;


  switch (sh->SliceType)
  {
  //! Potential BUG: do we need to distinguish between INTER_IMG_MULT and INTER_IMG?
  //!    similar with B_IMG_! and B_IMG_MULT
  //! also: need to define Slice types for SP images
  //! see VCEG-N72r1 for the Slice types, which are mapped here to img->type
  case 0:
    img->type = currSlice->picture_type = ParSet[CurrentParameterSet].UseMultpred?INTER_IMG_MULT:INTER_IMG_1;
    break;
  case 1:
    img->type = currSlice->picture_type = ParSet[CurrentParameterSet].UseMultpred?B_IMG_MULT:B_IMG_1;
    break;
  case 2:
    img->type = currSlice->picture_type = ParSet[CurrentParameterSet].UseMultpred?SP_IMG_MULT:SP_IMG_1;
    break;
  case 3:
    img->type = currSlice->picture_type = INTRA_IMG;
    break;
  default:
    printf ("Panic: unknown Slice type %d, conceal by loosing slice\n", sh->SliceType);
    currSlice->ei_flag = 1;
  }  
  

  //! The purpose of the following is to check for mixed Slices in one picture.
  //! According to VCEG-N72r1 and common sense this is allowed.  However, the
  //! current software seems to have a problem of some kind, to be checked.  Hence,
  //! printf a warning

  if (currSlice->start_mb_nr == 0)
    ActualPictureType = img->type;
  else
    if (ActualPictureType != img->type)
    {
      printf ("WARNING: mixed Slice types in a single picture -- interesting things may happen :-(\n");
    }

  img->tr = currSlice->picture_id = sh->PictureID;

  currSlice->last_mb_nr = currSlice->start_mb_nr + sh->CABAC_LastMB;

  if (currSlice->last_mb_nr == currSlice->start_mb_nr)
    currSlice->last_mb_nr = img->max_mb_nr;
}


/*!
 *****************************************************************************
 *
 * \brief 
 *    Function returns the next header type (SOS SOP, EOS)
 *    p-> and sh-> are filled.  p-> does not need memory for payload and packet
 *
 * \return
 *
 * Side effects:
 *
 * \date
 *    27 October, 2001
 *
 * \author
 *    Stephan Wenger   stewe@cs.tu-berlin.de
 *****************************************************************************/



int RTPGetFollowingSliceHeader (struct img_par *img, RTPpacket_t *p, RTPSliceHeader_t *sh)
{
  long int Filepos;
  int TotalPackLen;
  int done=0;
  int intime=0;
  Slice *currSlice = img->currentSlice;
  static unsigned int old_seq=0;
  static int first=0;
  static int i=2;
  
  RTPpacket_t *newp, *nextp;
  RTPSliceHeader_t *nextsh;
  
  assert (p != NULL);
  assert (sh != NULL);

  newp = alloca (sizeof (RTPpacket_t));
  newp->packet = alloca (MAXRTPPACKETSIZE);
  newp->payload = alloca(MAXRTPPAYLOADLEN);
  nextp=alloca (sizeof (RTPpacket_t));
  nextsh=alloca(sizeof (RTPSliceHeader_t));

  Filepos = ftell (bits);

  while (!done)
  {
    if (4 != fread (&TotalPackLen,1, 4, bits))
    {
      fseek (bits, Filepos, SEEK_SET);
      return EOS;    // EOF inidication
    }
    
    if (4 != fread (&intime, 1, 4, bits))
    {
      fseek (bits, Filepos, SEEK_SET);
      printf ("RTPGetFollowingSliceHeader: File corruption, could not read Timestamp\n");
      return EOS;
    }

    newp->packlen = TotalPackLen;
    assert (newp->packlen < MAXRTPPACKETSIZE);
    if (newp->packlen != fread (newp->packet, 1, newp->packlen, bits))
    {
      fseek (bits, Filepos, SEEK_SET);
      return EOS;    // EOF inidication
    }
    DecomposeRTPpacket (newp);
    if (newp->payload [0] == 0 || newp->payload[0] == 1)   // Full Slice or Partition A
      done = 1;
  }
  fseek (bits, Filepos, SEEK_SET);
  p->cc = newp->cc;
  p->m = newp->m;
  p->p = newp->p;
  p->pt = newp->pt;
  p->seq = newp->seq;
  p->ssrc = newp->ssrc;
  p->timestamp = newp->timestamp;
  p->v = newp->v;
  p->x = newp->x;

  if (p->seq != old_seq+i)
    currSlice->next_eiflag =1;
  else
  {
    currSlice->next_eiflag =0;
    old_seq=p->seq;
  }

  if(!first)
  {
    first=1;
    i=1;
  }

  RTPInterpretSliceHeader (&newp->payload[1], newp->packlen, newp->payload[0]==0?0:1, sh);
  if(currSlice->picture_id != sh->PictureID) 
    return (SOP);
  else
    return (SOS);
}

/*!
 *****************************************************************************
 *
 * \brief 
 *    Evaluates the lastMB of one slice that has to be concealed
 *    
 * \return
 *   sequence number of the last packed that has been concealed
 *
 * Side effects:
 *
 * \date
 *    03 November, 2001
 *
 * \author
 *    Tobis Oelbaum drehvial@gmx.net
 *****************************************************************************/

int get_lastMB(struct img_par *img, RTPSliceHeader_t *sh, RTPpacket_t *p)
{
  Slice *currSlice = img->currentSlice;
  int packets = p->seq;
  int lost_frames = 0;
  
  //! start_MB Entry of the next slice 
  currSlice->last_mb_nr = (img->width/16)*sh->FirstMBInSliceY+sh->FirstMBInSliceX; 
  
  //! Adjust the number of packets that are concealed 
  //! Next Slice does not belong to the current frame 
  //! AND is the first slice of the next frame AND we are not at the beginning of a new frame
  if(currSlice->picture_id != sh->PictureID) 
  {
    lost_frames = ((sh->PictureID - currSlice->picture_id) + 256)%256;
    lost_frames /= (InfoSet.FrameSkip+1); 
    if(!img->current_mb_nr)
      lost_frames--;
    if(!currSlice->last_mb_nr)
      packets = p->seq - lost_frames;                                                 
    else
      packets = p->seq - (lost_frames+1);    
  }
  else
      packets = p->seq - 1;

  if(!currSlice->last_mb_nr && lost_frames > 0)
    lost_frames--;
  
  if(!currSlice->start_mb_nr && img->number) 
    currSlice->picture_id += (InfoSet.FrameSkip+1);

  if(currSlice->picture_id > (256 - (InfoSet.FrameSkip+1)))
    currSlice->picture_id -= 256;

  img->tr = currSlice->picture_id;
  
  //! Adjust the last MB for the slice that has to be concealed
  //! the next received slice starts in one of the next frames
  if(lost_frames || !currSlice->last_mb_nr)
  {
    currSlice->last_mb_nr = img->max_mb_nr-1; //! Changed TO 12.11.2001        
    currSlice->next_header = SOP;
  }
  else
  {
    currSlice->last_mb_nr--; 
    currSlice->next_header = SOS;
  }
  currSlice->next_eiflag = 0;
  return packets;
}
  
  
