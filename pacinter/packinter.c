#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAXPARTITION 30
#define MAXPARTITIONSIZE 10000

typedef struct
{
	int psizebits;
	unsigned char *pbuf;
}
PartitionInfo;

PartitionInfo p[MAXPARTITION];

FILE *in, *out;
int finished = 0, i, picid, sliceid, startmb;

int NumberOfBytes (int bits);
int ReadPicture();
void WritePicture();
void Initialize();

main (int ac, char *av[])
{
	if (ac != 3)
	{
		printf ("Usage: packinter <tml1dp.26l> <packet-file.pac>\n");
		exit (-1);
	}
	if ((in = fopen (av[1], "rb")) == NULL)
	{
		printf ("packinter: can't open %s for reading... exit\n", av[1]);
		exit (-2);
	}
	if ((out = fopen (av[2], "wb")) == NULL)
	{
		printf ("packinter: can't open %s for writing... exit\n", av[2]);
		exit (-3);
	}

	while (!finished)
	{
		Initialize();
		finished = ReadPicture();
		WritePicture();
		// getchar();
	}
	// getchar();

	return 0;
}


int ReadPicture ()
{
	int pdt, psizebits, psizebytes, ppicid, psliceid, pstartmb, pqp, pformat, i;

	for (i=0; i<MAXPARTITION; i++)
		p[i].psizebits = 0;

	// Read Picture Header
	if (1 != fread (&psizebits, 4, 1, in))
		return 1;		// Finished, EOF

	assert (1 == fread (&pdt, 4, 1, in));
	assert (1 == fread (&ppicid, 4, 1, in));
	assert (1 == fread (&psliceid, 4, 1, in));
	assert (1 == fread (&pstartmb, 4, 1, in));
	assert (1 == fread (&pqp, 4, 1, in));
	assert (1 == fread (&pformat, 4, 1, in));

	printf ("ReadPicture, got dt %d   picid %d   sliceid %d   startmb %d   pqp %d   pformat %d\n",
	        pdt, ppicid, psliceid, pstartmb, pqp, pformat);

	picid = ppicid;
	sliceid = psliceid;
	startmb = pstartmb;
	assert (pdt == 0);

	psizebytes = NumberOfBytes (psizebits);

	assert (1 == fread (p[pdt].pbuf, psizebytes, 1, in));
	p[pdt].psizebits = psizebits;
	// Read Partition

	while (1)
	{
		if (1 != fread (&psizebits, 4, 1, in))
			return (1);
		assert (1 == fread (&pdt, 4, 1, in));
		assert (1 == fread (&ppicid, 4, 1, in));
		assert (1 == fread (&psliceid, 4, 1, in));
		assert (1 == fread (&pstartmb, 4, 1, in));
		assert (1 == fread (&pqp, 4, 1, in));
		assert (1 == fread (&pformat, 4, 1, in));

		// printf ("Got Partition type %d size %d picture %d\n", pdt, psizebits, ppicid);
		// getchar();
		if (pdt == 0)
		{	// New picture header found -> new picture
			assert (0 == fseek (in, -28, SEEK_CUR));
			return 0;
		}
		psizebytes = NumberOfBytes (psizebits);

		assert (1 == fread (p[pdt].pbuf, psizebytes, 1, in));
		p[pdt].psizebits = psizebits;
	}
}


// This is the very first idea of a packetization scheme for the Internet
// Currently, we use 2 packets per picture.  Pakcet one, with the higher
// priority, includes all 'header' information, whereas packet 2 contains
// CBP and all coefficient information.
//
// De-Packetization Idea: In order to present the decoder an uncorrupted
// bitstream even in case of a packet loss, the de-packetizer follows
// the algorithm: if packet 1 is lost, then loose packet 2.  If packet 2
// is lost, present packet 1 (containing the MVs) and 'dummy CBPs' indicating
// no coefficients.

void WritePicture ()
{
	// Data structure expected by ploss is 4 byte length (in bytes) and n bytes data

	// Packet 1: consists of partitions 0, 1, 2, 7
	// Packet 2: consists of partitions 3, 4, 5, 6

	// Compressed partition header is 4 bit partition type and 12 bit length (in bits)
	// if 2 partitions of same type follow then they are to be concatenated
	// Length == 0 means 4096 bits, except for the last sub-packet of a partition, where
	// it means 0 bits.  That is if, for a given partition type, len is 0, 0, 0, then
	// the partition contains 4096+4096+0 bits.  If len is 0, 0, 234, then the part.
	// contains 4096+4096+234 bits.

	int packet1[] = {0, 1, 2, 7, -1};
	int packet2[] = {3, 4, 5, 6, -1};

	unsigned int phead, i, packetsize, bits, writebits, header;

	// Write packet 1

	// Calculate packet size
	// printf ("WritePicture\n");

	packetsize = 0;
	i=0;
	while (packet1[i] != -1)
	{
		// printf ("Checking Partition %d \t", packet1[i]);
		if (p[packet1[i]].psizebits > 0)
		{
			packetsize += NumberOfBytes (p[packet1[i]].psizebits);
			// printf ("psizebits = %d\n", p[packet1[i]].psizebits);
			packetsize += 2 * ((p[packet1[i]].psizebits / 4096)+1);		// 2 bytes for the header(s)
		}

		i++;
	}
	packetsize += 8;		// for picid and header
	// printf ("Calculated Packet 1 size %d\n", packetsize);
	// Here is a problem: 4096 bits in packet!
	// Fix later

	assert (1 == fwrite (&packetsize, 4, 1, out));		// packet size


	// Hack: write PicID instead of getting it from the RTP header
	assert (1 == fwrite (&picid, 4, 1, out));			// Picture Id

	header = ((sliceid & 0x3ff)    <<  0) |
	         ((startmb & 0x3fff)   << 10) |
	         (((startmb == 0) & 1) << 30) |   // Slice header == Pakcet 1 AND NOT StartMB == 0
	         (((startmb != 0) & 1) << 31);	  // Picture header == Pakcet 1 AND StartMB == 0
	// printf ("Writing packet 1 header 0x%x\n", header);
	// getchar();
	assert (1 == fwrite (&header, 4, 1, out));

	// printf ("Writing packet size %d for picid %d\n", packetsize, picid);
	for (i=0; packet1[i] != -1; i++)
		if (p[packet1[i]].psizebits > 0)
			for (bits = 0; bits < p[packet1[i]].psizebits; bits+=4096)
			{
				assert (p[packet1[i]].psizebits % 4096 != 0);
				writebits = p[packet1[i]].psizebits - bits > 4096 ? 4096:p[packet1[i]].psizebits - bits;
				// printf ("writebits %d\n", writebits);
				phead = packet1[i] * 4096 + writebits % 4096;	// 4096 is signalled as '0'
				// printf ("Phead = 0x%x for partition %d bits %d\n", phead, packet1[i], writebits);
				assert (1 == fwrite(&phead, 2, 1, out));		// write only 16 bits
				assert (1 == fwrite (&p[packet1[i]].pbuf[bits/8], NumberOfBytes(writebits), 1, out));
			}

	// getchar();
	// Write packet 2
	// Calculate packet size
	packetsize = 0;
	i=0;
	while (packet2[i] != -1)
	{
		if (p[packet2[i]].psizebits > 0)
		{
			packetsize += NumberOfBytes (p[packet2[i]].psizebits);
			// printf ("psizebits = %d\n", p[packet2[i]].psizebits);
			packetsize += 2 * ((p[packet2[i]].psizebits / 4096)+1);
		}
		i++;
	}
	packetsize += 8;		// for picid and header

	// printf ("Calculated Packet 2 size %d\n", packetsize);

	assert (1 == fwrite (&packetsize, 4, 1, out));		// packet size
	// Hack: write PicID instead of getting it from the RTP header
	assert (1 == fwrite (&picid, 4, 1, out));			// Picture Id

	header =(((sliceid & 0x3ff)    <<  0) |
	         ((startmb & 0x3fff)   << 10) |
	         ((0 & 1) << 30) |				// Packet 2 never contains Picture pe7r Slice header
	         ((0 & 1) << 31));
	printf ("Writing packet 2 header 0x%x\n", header);
	assert (1 == fwrite (&header, 4, 1, out));

	// printf ("Writing packet size %d for picid %d\n", packetsize, picid);
	for (i=0; packet2[i] != -1; i++)
		if (p[packet2[i]].psizebits > 0)
			for (bits = 0; bits < p[packet2[i]].psizebits; bits+=4096)
			{
				assert (p[packet2[i]].psizebits % 4096 != 0);
				writebits = p[packet2[i]].psizebits - bits > 4096 ? 4096:p[packet2[i]].psizebits - bits;
				phead = packet2[i] * 4096 + writebits % 4096;	// 4096 is signalled as '0'
				assert (1 == fwrite(&phead, 2, 1, out));		// write only 16 bits
				assert (1 == fwrite (&p[packet2[i]].pbuf[bits/8], NumberOfBytes (writebits), 1, out));
				// printf ("write phead 0x%x, writebits %d, bytes %d, partitiopn %d\n", phead, writebits, NumberOfBytes (writebits), packet2[i]);
				// getchar();
			}

	//	getchar();

}

/*
void WriteRTPpacket (FILE *out, uchar *data, int len, int psize, int pebit, int Timestamp, int Marker) {
  static int RTPSequence = 0;
  headerinfo h;
  int packetsize;
  
  memset (h.IPheader, 0, 20);			// zero IP-header
  memset (h.UDPheader, 0, 8);			// zero UDPheader
  h.RTPhflags = Marker?0:0x100;		// all zero, except the marker bit
  h.RTPseqno = RTPSequence++;
  h.RTPtime = Timestamp;
  h.RTPssrc = 0;
  h.RFC2429 = 0x40					// set V-bit
    |   (psize << 7)			// PLEN
    // MDG Fix:
    // Shift up 13, not 12
    // |   (pebit << 12);			// PEBIT
    |   (pebit << 13);			// PEBIT
  packetsize = sizeof (headerinfo) + len;
  fwrite (&packetsize, 4, 1, out);		
  fwrite (&h, sizeof (headerinfo),1, out);
  fwrite (data, len, 1, out);
}
 
*/

int NumberOfBytes (int bits)
{
	int bytes;

	bytes = bits / 8;
	if (bits % 8)
		bytes++;
	return bytes;
}

void Initialize()
{
	int i;
	static int Firsttime = 1;

	for (i=0;i<MAXPARTITION;i++)
		p[i].psizebits = 0;
	if (Firsttime)
	{
		Firsttime = 0;
		for (
		    i=0; i<MAXPARTITION; i++)
			assert (NULL != (p[i].pbuf = malloc (MAXPARTITIONSIZE)));
	}


}
