/*===========================================================================
  ITU-T Telecommunications Standardization Sector     Document
  Study Group 16                                      Filename
  Video Coding Experts Group (Question 15)            Generated: 20 May. '00
  ----------------------------------------

  Source:
  ~~~~~~~
  Sebastian Purreiter
  purreiter@lnt.ei.tum.de
  Thomas Stockhammer
  stockhammer@ei.tum.de

  pseudomux10.c
  ~~~~~~~~~~~~~
  Simple video packet mux simulator.  Approximates AL3 mux of H.223 Annex D.

  Based on pseudomux approximating H.223 Annex B
  Document Q15-G-38, Q15-G-42r2, Q15-G-42r3, Q15-G-42r4
  written by: Gary Sullivan, Stephan Wenger, Ari Hourunranta, Simao Campos-Neto,
  Maximilian Luttrell and Bernhard Wimmer

  Purpose: Proposal
  ~~~~~~~~

  History:
  ~~~~~~~~
  27.Jul.00  V.1.0  Author: Sebastian Purreiter

  ===========================================================================*/
/*===========================================================================

  Application Layer

  Maps SDUs on logical channels.

  Video packet consists of the following format
  pseudomux input format at encoder and
  pseudomux output format at decoder

           +---------+----+--------+---------------+
  content  | EI      | lc | l(sdu) | pl(sdu)       |
           +---------+----+--------+---------------+
  bytes    | 1       | 1  | 4      | l(sdu)        |
           +---------+----+--------+---------------+

  EI :    : Error Indication flag at decoder only, at encoder unused
  lc      : logical channel number used for transmission (16 channels possible)
  l(sdu)  : video packet length transmitted/received over logical channel lc
  pl(sdu) : payload of transmitted video packet (octett-aligned)

  ===========================================================================

  Adaptation Layer

  Segmentation and Reassembly (AL-SDU -> AL-SDU*s -> AL-SDU).
  Error correction and detection (RS codes, CRCs).
  Framed transfer mode.
  Only I-PDUs are sent (no S-PDU with zero length).
  No optional control field used.
  FEC_ONLY mode, no ARQ
  AL-SDU* with a 16bit CRC is RS encoded with a rate smaller than 1.0

  ===========================================================================

  Multiplex Layer

  The 2 byte sync is 0xE14D (binary 11100001 followed by 01001101)

  The 3 byte basic mux header we are using contains the following:
  bits    contents     explanation
  0-3     MC           Multiplex Code, describes multiplex scheme of packet
  4-11    MPL          Multiplex Payload Length, the length of the data field
  12-23   golay        (24,12) Golay block code which codes combined MC+MPL

  ===========================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <malloc.h>

#define byte      unsigned char
#define unt16     unsigned short
#define int16     short
#define int32     long

/* initialization value of CRC */
#define INIT_CRC_C  ((unt16) 0xFFFF)

/*===========================================================================

  unt16 updcrc (unt16 icrc, unsigned char *icp, int icnt, unt16 *crctab,
  ~~~~~~~~~~~~  char swapped);

  Description:
  ~~~~~~~~~~~~
  Calculate, intelligently, the CRC-16 of a dataset incrementally given a
  buffer full at a time. The parameters on which the function relies are:

  Parameter             CCITT   XMODEM  ARC
  ---------             ------  ------  ----
  the polynomial P      0x1021  0x1021  A001
  initial CRC value:    0xFFFF  0       0
  bit-order swap        No (0)  Yes (1) Yes (1)
  bits in CRC:          16      16      16

  Usage:
  ~~~~~~
  newcrc = updcrc( oldcrc, bufadr, buflen, crctab, swapped)
                   unt16 oldcrc;
                   int buflen;
                   char *bufadr;
                   unt16 *crctab;
                   char swapped;

  Notes:
   Regards the data stream as an integer whose MSB is the MSB of the first
   byte recieved.  This number is 'divided' (using xor instead of subtraction)
   by the crc-polynomial P.
   XMODEM does things a little differently, essentially treating the LSB of
   the first data byte as the MSB of the integer.

  Original Author:
  ~~~~~~~~~~~~~~~~
  Mark G. Mendel, 7/86
  UUCP: ihnp4!umn-cs!hyper!mark, GEnie: mgm
  [ Original code available in comp.sources.unix volume 6 (1989) ]

  History:
  ~~~~~~~~
  xx.Jul.86 v1.0  Release by the author [posted to c.s.misc in 1989]
  01.Sep.93 v1.0a Adapted by Simao <simao@cpqd.ansp.br> to be used in the
                  x{en,de}code.c.
  08.Oct.98 v1.0b Adopted into pseudo-mux simulator by Gary (garys@pictel.com)

  ===========================================================================*/

#define BPS   8  /* bits per sample */
#define CRCW 16  /* crc width */

/* Table for CCITT CRCs */
static unt16 crctab_ccitt[1 << BPS] = {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
  0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
  0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
  0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
  0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
  0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
  0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
  0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
  0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
  0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
  0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
  0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
  0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
  0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
  0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
  0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
  0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
  0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
  0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
  0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
  0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
  0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
  0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
};

/* -------------- Begin of function updcrc() ---------------------- */
unt16
updcrc(icrc, icp, icnt, crctab, swapped)
  unt16           icrc, *crctab;
  unsigned char  *icp;
  int             icnt;
  char            swapped;
{
  register unt16          crc = icrc;
  register unsigned char *cp  = icp;
  register int            cnt = icnt;

  if (swapped)

    while (cnt--)
      crc = (crc >> BPS) ^ crctab[(crc & ((1 << BPS) - 1)) ^ *cp++];
  else
    while (cnt--)
      crc = (crc << BPS) ^ crctab[(crc >> (CRCW - BPS)) ^ *cp++];

  return crc;
}
/* .......................... End of updcrc() ............................ */

/* The maximum value of maximumAL3SDUSize negotiable by H.245 is 65535. */
#define maximumAL3SDUSize    65535        /* bytes */
#define MAX_MUX_PACKET_SIZE  254          /* bytes */

#define AUDIO_TIME_INTERVAL  0.030        /* seconds */
#define AUDIO_PACKET_SIZE    24 /* bytes (not including overhead) */

#define USE_OPTIONAL_HEADER  1            /* 1=Use it, 0=Don't use it */

#define AL2_CRC_SIZE         1            /* bytes */
#define AL3_CRC_SIZE         2            /* bytes */

#define SYNC_SIZE            2            /* bytes */
#define BASIC_HEADER_SIZE    3            /* bytes */
#define OPTIONAL_HEADER_SIZE 1            /* bytes */
#define TOTAL_HEADER_SIZE    (BASIC_HEADER_SIZE + USE_OPTIONAL_HEADER * OPTIONAL_HEADER_SIZE)
#define MUX_PACKET_OVERHEAD  (SYNC_SIZE + TOTAL_HEADER_SIZE)

#define MAX_SYNC_ERRORS             3
#define MAX_HEADER_ERRORS           2
#define MAX_OPTIONAL_HEADER_ERRORS  0

#define MAX_LOGIC_CHANNELS          16

/*===========================================================================*/

/**
 * global variables
 */
char  *pname;
FILE  *ErrorFile, *InputFile, *OutputFile, *MessageFile;

/* bytes for Reed-Solomon coding for different logical channels */
int rscode[MAX_LOGIC_CHANNELS]; /* RS-protection in logical channels */

/*===========================================================================*/

/**
 * splits AL-SDU into Al-SDU* of a maximum length of MAX_MUX_PACKET_SIZE bytes
 */
int GetVideoMuxPacket(int32 *pLastMuxPacketOfVideoPacket,
                      int32 *pVideoPacketSize,
                      byte  *VideoPacketData,
                      byte  *MuxPacketData,
                      int32  dposition,
                      byte  *logicchannel)
{
  int   MuxPacketSize=0, i;
  unt16 transmitted_crc, msb_crc;

  if(*pLastMuxPacketOfVideoPacket) {
                                          /* read logic channel number */
    if (fread(logicchannel, sizeof(*logicchannel), 1, InputFile) != 1)
      MuxPacketSize = -1;                      /* indicate end of session */

    if (*logicchannel >= MAX_LOGIC_CHANNELS) {
      fprintf(MessageFile, "logical channel number (%d) out of range!\n",
              (int)*logicchannel );
      exit(-5);
    }
                                          /* read AL-SDU packet length */
    if ( fread(pVideoPacketSize, sizeof(*pVideoPacketSize), 1, InputFile) != 1) {
      MuxPacketSize = -1;             /* indicate end of session */
    }
    else {
      if(*pVideoPacketSize < 0 ||
         *pVideoPacketSize > maximumAL3SDUSize) {
        fprintf(MessageFile, "%s: packet size (%ld) out of bounds\n",
                pname, *pVideoPacketSize);
        exit(-7);
      }
                                          /* read AL-SDU payload */
      if(fread(VideoPacketData, sizeof(byte), *pVideoPacketSize, InputFile)
         != (size_t)*pVideoPacketSize) {
        fprintf(MessageFile, "%s: error reading video packet of %ld bytes\n",
                pname, *pVideoPacketSize);
        exit(-8);
      }
	  }
  }

  if (MuxPacketSize != -1) {
    /* calculate Mux packet size */
    if ( (*pVideoPacketSize - dposition) <= (MAX_MUX_PACKET_SIZE-AL3_CRC_SIZE-2*rscode[*logicchannel]) )
      MuxPacketSize = *pVideoPacketSize - dposition;
    else
      MuxPacketSize = MAX_MUX_PACKET_SIZE-AL3_CRC_SIZE-2*rscode[*logicchannel];

    /* map SDU content into PDU */
    for (i=0; i<MuxPacketSize; i++)
      MuxPacketData[i] = VideoPacketData[dposition+i];

    /* check if last AL-SDU* is transmitted */
    *pLastMuxPacketOfVideoPacket = ( (*pVideoPacketSize - dposition) <= MuxPacketSize );

    /* apply CRC check to AL-SDU* payload */
    /* calculate packet CRC to be transmitted */
    transmitted_crc = updcrc(INIT_CRC_C, MuxPacketData, MuxPacketSize, crctab_ccitt, 0);

    /* Technically, each byte of the CRC should be bit-reversed (according to the last */
    /* paragraph of Section 3 of H.223), however an equality test on a transmitted and */
    /* received  crc value will get the same result regardless of the bit order,       */
    /* provided the transmitter and receiver use the same bit order.                   */

    /* put CRC into data buffer to be sent (manipulating in endian-neutral fashion)    */
    msb_crc = transmitted_crc >> 8;
    MuxPacketData[MuxPacketSize++] = (unsigned char)msb_crc;
    MuxPacketData[MuxPacketSize++] = transmitted_crc - (msb_crc << 8);

    /* add necessary space for RS-encoding */
    MuxPacketSize += (2*rscode[*logicchannel]);
  }

  return MuxPacketSize;
}

/*===========================================================================*/

/**
 * pseudomux version 2.0
 *
 */
int main(int ac, char *av[]) {

  byte   VideoPacketData[maximumAL3SDUSize+AL3_CRC_SIZE];
  byte   MuxPacketData[MAX_MUX_PACKET_SIZE];
  byte   *errors, ebyte, FailFlag;
  int32  esize, eposition, j, k, SyncErrorCount, HeaderErrorCount, TotalBytes;
  int32  LastMuxPacketOfVideoPacket, VideoPacketSize, dposition;
  int32  TotalBitRate, TotalRScodeBytes, VideoPacketNumber, TotalVideoBytes;
  int32  MuxPacketSize, MuxPacketNumber;
  unt16  received_crc, rcvr_computed_crc, muxpacketlost;
  double time_since_audio_sent, time_duration_of_audio_packet;
  int32  ai = 0, loss = 0 ;
  int    bad_usage = 0;

  byte   PreviousMplFieldWasOk = 0; /* == 0 : FALSE*/
  int32  OptionalHeaderErrorCount;
  int32  TestSyncErrorCount;
  int32  OldEposition = 0;

  byte   logicchannel=0;
	int32  err_count=0;

  int32  assembledVideoPacketSize=0, frames=0;
  int    assembleuntillost=0, nosyncerrors=0;
  byte   buffer[MAX_MUX_PACKET_SIZE];

  char errorfilename[255], inputfilename[255];
  char outputfilename[255], msgfilename[255];
  FILE *ConfigFile;

  /* -------------------------------------------- */

  pname = av[ai++];
  MessageFile = stderr;

  if((ac > ai) && (!bad_usage)) {
    if((bad_usage = ((ConfigFile = fopen(av[ai++], "rb"))) == NULL)) {
      fprintf(MessageFile, "%s: Cannot find config file (%s)\n", pname, av[--ai]);
      bad_usage = 1;
    }
    else {
      /* read config file */
      fscanf(ConfigFile,"%s",errorfilename);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%s",inputfilename);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%s",outputfilename);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%s",msgfilename);
      fscanf(ConfigFile,"%*[^\n]");

      fscanf(ConfigFile,"%ld",&TotalBitRate);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%ld",&eposition);
      fscanf(ConfigFile,"%*[^\n]");

      fscanf(ConfigFile,"%d",&assembleuntillost);
      fscanf(ConfigFile,"%*[^\n]");

      fscanf(ConfigFile,"%d",&nosyncerrors);
      fscanf(ConfigFile,"%*[^\n]");

      fscanf(ConfigFile,"%d",&rscode[0]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[1]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[2]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[3]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[4]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[5]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[6]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[7]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[8]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[9]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[10]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[11]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[12]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[13]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[14]);
      fscanf(ConfigFile,"%*[^\n]");
      fscanf(ConfigFile,"%d",&rscode[15]);
      fscanf(ConfigFile,"%*[^\n]");

      if ((bad_usage = ((ErrorFile = fopen(errorfilename, "rb"))) == NULL )) {
        fprintf(MessageFile, "%s: Cannot open error file (%s)\n", pname, errorfilename);
        bad_usage = 1;
      }

      if( strcmp(inputfilename, "stdin") == 0)
        InputFile = stdin;
      else if((InputFile = fopen(inputfilename, "rb")) == NULL) {
        fprintf(MessageFile, "%s: Cannot open input file (%s)\n", pname, inputfilename);
        bad_usage = 1;
      }
      if( strcmp(outputfilename, "stdout") == 0)
        OutputFile = stdout;
      else if((OutputFile = fopen(outputfilename, "wb")) == NULL) {
        fprintf(MessageFile, "%s: Cannot open output file (%s)\n", pname, outputfilename);
        bad_usage = 1;
      }
      if( strcmp(msgfilename, "stderr") == 0)
        MessageFile = stderr;
      else if((MessageFile = fopen(msgfilename, "wb")) == NULL) {
        fprintf(MessageFile, "%s: Cannot open output file (%s)\n", pname, msgfilename);
        bad_usage = 1;
      }
    }
  }
  else
    bad_usage = 1;

  if(ac > ai || bad_usage) {
    fprintf(stderr, "\n");
    fprintf(stderr, "USAGE: %s <Configfile>\n", pname);
    fprintf(stderr, "\n");
    fprintf(stderr, "The Configfile consists of the following:\n");
    fprintf(stderr, "Each .. represents a new line in the configfile\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "..Errorpatternfile (string)\n");
    fprintf(stderr, "   Error patterns are binary-format files in which the least-significant \n");
    fprintf(stderr, "   bit of the first byte signifies the error state of the temporally \n");
    fprintf(stderr, "   first bit on the channel.\n");
    fprintf(stderr, "..Inputfile (string)\n");
    fprintf(stderr, "   Input packets (from stdin if not existant or InputFile if specified)\n");
    fprintf(stderr, "   are each prefaced by a logical channel number (byte) and a packet length \n");
    fprintf(stderr, "   in bytes (integer)\n");
    fprintf(stderr, "..Outputfile (string)\n");
    fprintf(stderr, "  Output (to stdout or OutputFile) for each packet contains:\n");
    fprintf(stderr, "    Partly loss flag (byte)\n");
    fprintf(stderr, "    logical channel number (byte)\n");
    fprintf(stderr, "    packet length (integer), and\n");
    fprintf(stderr, "    packet data (with errors applied)\n");
    fprintf(stderr, "..Messagefile (string)\n");
    fprintf(stderr, "   Total number of video bytes used including packet overhead and other \n");
    fprintf(stderr, "   information is provided. (to stderr or Messagefile)\n");
    fprintf(stderr, "..Totalbitrate (integer) \n");
    fprintf(stderr, "   is used to determine when to insert periodic audio packets.\n");
    fprintf(stderr, "..ErrStart (integer) is used to specify the starting\n");
    fprintf(stderr, "   byte number in the error pattern file. Note that all bytes are used when \n");
    fprintf(stderr, "   wrapping, but ErrStart bytes are jumped over at start-up.\n");
    fprintf(stderr, "..assemble SDU until first lost of SDU* (0=off 1=on)\n");
    fprintf(stderr, "   Reassemble until first lost AL-SDU*\n");
    fprintf(stderr, "..no errors in Sync bytes and Mux-Header in Mux Layer possible (0=off 1=on)\n");
    fprintf(stderr, "..Reed-Solomon correction ability of logical channel 0-15 (integer)\n");
    fprintf(stderr, "   16 lines determining the correction ability of the Reed-Solomon Code \n");
    fprintf(stderr, "   for each logical channel in bytes. The first line is assigned to the \n");
    fprintf(stderr, "   logical channel 0, the second to channel 1, and so on.\n");
    fprintf(stderr, "   The maximum value for a %d byte AL_SDU* is %d bytes.\n",
                    MAX_MUX_PACKET_SIZE, (MAX_MUX_PACKET_SIZE-AL3_CRC_SIZE-1)/2);
    fprintf(stderr, "\n");
    exit(-1);
  }

  /* determine size of error pattern file and read it into errors buffer */
  if(fseek(ErrorFile, 0L, SEEK_END)) {
    fprintf(MessageFile, "%s: fseek failed\n", pname);
    exit(-3);
  }
  if((esize = ftell(ErrorFile)) == -1) {
    fprintf(MessageFile, "%s: ftell returns -1", pname);
    exit(-4);
  }
  fseek(ErrorFile, 0L, SEEK_SET);
  if((errors = (byte *)malloc(esize)) == NULL) {
    fprintf(MessageFile, "%s: cannot malloc %ld bytes for error patterns\n",
            pname, esize);
    exit(-5);
  }
  if(fread(errors, esize, 1, ErrorFile) != 1) {
    fprintf(MessageFile, "%s: error while reading error patterns\n",
            pname);
    exit(-6);
  }
  fclose(ErrorFile);

  if (eposition > esize)
    eposition=esize;

  time_duration_of_audio_packet =
    (AUDIO_PACKET_SIZE + MUX_PACKET_OVERHEAD + AL2_CRC_SIZE) * 8.0 / TotalBitRate;

  for(VideoPacketNumber = 0, MuxPacketNumber = 0,
			LastMuxPacketOfVideoPacket = 1,
      TotalBytes = 0, TotalVideoBytes = 0, TotalRScodeBytes = 0,
			rcvr_computed_crc = INIT_CRC_C,
			dposition = 0,
      time_since_audio_sent = AUDIO_TIME_INTERVAL;

        (MuxPacketSize = GetVideoMuxPacket(&LastMuxPacketOfVideoPacket,
            &VideoPacketSize, VideoPacketData, MuxPacketData, dposition, &logicchannel)) > 0 ;

          MuxPacketNumber++, VideoPacketNumber += LastMuxPacketOfVideoPacket )
  {
		err_count  = 0;
		if (logicchannel==0)
		  frames++;

    TotalVideoBytes += (MuxPacketSize-AL3_CRC_SIZE-(2*rscode[logicchannel]));
    TotalRScodeBytes += (2*rscode[logicchannel]);
    TotalBytes += (MuxPacketSize + MUX_PACKET_OVERHEAD);
    time_since_audio_sent += ((MuxPacketSize + MUX_PACKET_OVERHEAD) * 8.0 / TotalBitRate);

    /* send audio when necessary (and account for time to send it) */
    while(time_since_audio_sent >= AUDIO_TIME_INTERVAL) {
      TotalBytes += MUX_PACKET_OVERHEAD + AUDIO_PACKET_SIZE + AL2_CRC_SIZE;
      eposition  += MUX_PACKET_OVERHEAD + AUDIO_PACKET_SIZE + AL2_CRC_SIZE;
      if(eposition >= esize) eposition -= esize;
      time_since_audio_sent -= AUDIO_TIME_INTERVAL;
      time_since_audio_sent += time_duration_of_audio_packet;
    }
    /* discard SYNC_SIZE bytes while counting errors in them */
    for(j=0, SyncErrorCount=0; j<SYNC_SIZE; j++) {
      ebyte  = errors[eposition++];
      if(eposition == esize) eposition = 0;
      if(ebyte) for(k=0; k<8; k++)
        if((ebyte >> k) & 1) SyncErrorCount++;
    }
    /* discard BASIC_HEADER_SIZE bytes while counting errors in them */
    for(j=0, HeaderErrorCount=0; j<BASIC_HEADER_SIZE; j++) {
      ebyte = errors[eposition++];
      if(eposition == esize) eposition = 0;
      if(ebyte) for(k=0; k<8; k++)
        if((ebyte >> k) & 1) HeaderErrorCount++;
    }

    /* discard optional mux header if present */
    if(USE_OPTIONAL_HEADER == 1) {
      eposition += OPTIONAL_HEADER_SIZE;
      if(eposition >= esize) eposition -= esize;
    }

    /* use optional mux header */
    if(USE_OPTIONAL_HEADER == 1) {
      /* the optional header can only be correctly found if the following */
      /* synchronization flag is ok!!! */
      OldEposition = eposition;

      /* Count errors in the following sync flag */
      for(j=0, TestSyncErrorCount=0; j<SYNC_SIZE; j++) {
        ebyte  = errors[eposition++];
        if(eposition == esize) eposition = 0;
        if(ebyte) for(k=0; k<8; k++)
          if((ebyte >> k) & 1) TestSyncErrorCount++;
      }
      /* skip over basic header of next packet to find optional header */
      eposition += BASIC_HEADER_SIZE;
      if(eposition >= esize) eposition -= esize;

      /* count errors in the optional header */
      for(j=0, OptionalHeaderErrorCount=0; j<OPTIONAL_HEADER_SIZE; j++) {
        ebyte  = errors[eposition++];
        if(eposition == esize) eposition = 0;
        if(ebyte) for(k=0; k<8; k++)
          if((ebyte >> k) & 1) OptionalHeaderErrorCount++;
      }
      eposition = OldEposition;
    }

    /* If previous MPL field is ok do not discard entire mux-frame */
    if(PreviousMplFieldWasOk != 0)
      SyncErrorCount = 0;

    PreviousMplFieldWasOk = (HeaderErrorCount <= MAX_HEADER_ERRORS);

    /* If optional header is ok but mux header is in error then still allow*/
    /* payload decoding, because MC code is available from optional header */
    if(USE_OPTIONAL_HEADER == 1)
      if((OptionalHeaderErrorCount <= MAX_OPTIONAL_HEADER_ERRORS)
         && (TestSyncErrorCount <= MAX_SYNC_ERRORS))
        HeaderErrorCount = 0;

    /* add errors to video packet */
    for(j=0; j < MuxPacketSize; j++) {
	  	/*count byte errors in videopacket */
  	  if (errors[eposition] != 0)
				err_count++;
			buffer[j] = MuxPacketData[dposition+j] ^ errors[eposition++];  /* add errors to buffer */
      if(eposition == esize) eposition = 0; /* rewind */
    }

    muxpacketlost=0;

    if(MuxPacketSize > AL3_CRC_SIZE) { /* if something got through */
      MuxPacketSize -= (2*rscode[logicchannel] + AL3_CRC_SIZE);

      if( nosyncerrors==1 ) {
        HeaderErrorCount=SyncErrorCount=0;
      }

      if( SyncErrorCount > MAX_SYNC_ERRORS || HeaderErrorCount > MAX_HEADER_ERRORS ) {
        muxpacketlost++;
      }
      else {
        /* calculate new CRC computed by receiver */
        rcvr_computed_crc = updcrc(INIT_CRC_C, buffer, MuxPacketSize, crctab_ccitt, 0);

        for(j=AL3_CRC_SIZE, received_crc=0; j>0; j--)
          received_crc = (received_crc << 8) + buffer[MuxPacketSize+AL3_CRC_SIZE-j];

        if ((received_crc != rcvr_computed_crc) && (err_count > (int)rscode[logicchannel])) {
          muxpacketlost++;
        }
      }
    }

    if (muxpacketlost > 0) {
      fprintf(MessageFile,
              "                  Video Mux Packet %4ld Was Lost",
              MuxPacketNumber);
      fprintf(MessageFile,
              "     (SyncErrorCount: %2ld, HeaderErrorCount: %2ld, Error: %d, RScode: %d)\n",
              SyncErrorCount, HeaderErrorCount, (int)err_count, rscode[logicchannel]);

      /* throw away the data so it doesn't get to the receiver */
      for(j=dposition; j<MuxPacketSize; j++)
        VideoPacketData[j] = VideoPacketData[j+MuxPacketSize];

      VideoPacketSize -= MuxPacketSize;

      if (loss>0 && assembleuntillost>0)
        assembledVideoPacketSize = dposition; 	/* save position until correct delivered AL-SDU* */
      loss++;
    }
    else{
      fprintf(MessageFile,
              "                  Video Mux Packet %4ld Was Received\n",
              MuxPacketNumber);
      dposition += MuxPacketSize;
    }

    if(LastMuxPacketOfVideoPacket) {

      if (loss > 0 && assembleuntillost == 0) {
        VideoPacketSize = 0;
        fprintf(MessageFile,
          "Video Packet %4ld                       Was Lost\n",
          VideoPacketNumber);
      }
      else {

        if ( (FailFlag = ((loss != 0 ) && assembleuntillost != 0)) == 1) {
       	  VideoPacketSize = assembledVideoPacketSize;
          fprintf(stderr,
            "Video Packet %4ld                       Was Received (Partly Lost = %d)\n",
            VideoPacketNumber, (int)FailFlag);
        }
        else {
          fprintf(MessageFile,
            "Video Packet %4ld                       Was Received (Partly Lost = %d)\n",
            VideoPacketNumber, (int)FailFlag);
        }
      }
      /* write out the crc cross-check flag, packet size, and packet */
      fwrite(&FailFlag, sizeof(FailFlag), 1, OutputFile);
      fwrite(&logicchannel, sizeof(logicchannel), 1, OutputFile);
      fwrite(&VideoPacketSize, sizeof(VideoPacketSize), 1, OutputFile);
      fwrite(VideoPacketData, sizeof(byte), VideoPacketSize, OutputFile);
      dposition = 0;
      loss=0;
    }
  } /* end of for */

  fprintf(MessageFile, "Processing Complete.\n");
  fprintf(MessageFile, "TotalVideoPackets:               %ld\n", VideoPacketNumber);
  fprintf(MessageFile, "TotalVideoBytes:                 %ld\n", TotalVideoBytes);

  /* Entire Overhead now includes optional header*/
  fprintf(MessageFile, "PayloadAndOverheadBytesForVideo: %ld\n",
          (TotalVideoBytes + MuxPacketNumber * ( MUX_PACKET_OVERHEAD + AL3_CRC_SIZE)
          + TotalRScodeBytes ));

  fprintf(MessageFile, "VideoPayloadPlusOverheadBitRate: %f bps\n",
          (TotalVideoBytes + MuxPacketNumber * ( MUX_PACKET_OVERHEAD + AL3_CRC_SIZE)
          + TotalRScodeBytes ) * (double) TotalBitRate / (double) TotalBytes);

  fprintf(MessageFile, "TotalTimeElapsed:                %f sec\n",
          TotalBytes * 8.0 / TotalBitRate);

  fprintf(MessageFile, "End of program.\n");

  fprintf(stdout,"%d\n",(int)eposition); 		/* return actual position in errorfile */

  exit(0);
}
