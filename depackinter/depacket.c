	

// De-packetizer, reads H.26L Internet packet structure and produces H.26L data partitioned
// bitstream. Two packets are used: packet 1 includes all header information, whereas
// packet2 includes only CBP and coefficients.  Whenever packet 1 is lost, then ignore
// packet 2 too.  If packet 2 is lost, then decode packet 1 and add 'dummy' packet 2 data,
// (all zero coefficients).


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

#define MAXPACKETSIZE 100000

int ProcessPicture();
int ReadPacket(unsigned char *buf);

int getptype (int);
int getplen (int);
int getnumberofbytes (int);
int getpartitionlen (int, unsigned char *, int);

FILE *in, *out;
int finished = 0, picid, packet1picid, sliceid, startmb, qp, format, type, header;
unsigned char p[MAXPACKETSIZE];

main (int ac, char *av[]) {

	int i=0;

	if (ac != 3) {
		printf ("Usage: depacket <infile.pac> outfile.26L\n");
		exit (-1);
	}
	if ((in = fopen(av[1], "rb")) == NULL) {
		printf ("Can't open infile %s, exiting\n", av[1]);
		exit (-2);
	}
	if ((out = fopen (av[2], "wb")) == NULL) {
		printf ("Can't open outfile %s, exiting\n", av[2]);
		exit (-3);
	}

	finished = 0;
	picid = packet1picid = sliceid = startmb = qp = format = type = -1;
	while (!finished) {
		finished = ProcessPicture();
//	getchar();
	}
//	getchar();
	return 0;

/*
	// Check integrity of the packet structure
	while ((finished = ReadPacket(p)) > 0) {
		printf ("%d\t%d\t", i++, finished); 
	}
	getchar();
*/
}


int ProcessPicture() {

// very simple algorithm: read first packet.  Packet can be either #1 (containing partition 0) or
// #2 (not containing partition 0).  It is at the moment assumed that packet #1 starts with
// partition 0.  If it is packert #2, then drop and conrtinue until packet #1 is found.
// 
// Now we found a packet#1.  Read next packet.  If it is packet #2 (i.e. not containing partition
// 0, then believe that we found a complete picture.  Otherwise 'repair' by generate 0-coefficient
// data for packet #2

	int packetlen, plen, ptype, packetindex, bytecount;

/*
	ptype = -1;
	while (ptype != 0) {
		if (! (packetlen = ReadPacket(p)))
			return (1);		// packet size = 0: finished

		// Check first partition header

		ptype = getptype (0);
		plen = getplen (0);
	}
	packet1picid = picid;	// Note that picid was set as a side effect of ReadPacket()

	printf ("Found First Picture plen %d ptype %d as the first part of partition \n", plen, ptype);

	// Here, we have read one complete packet and are sure that it is packet #1.  
	// Now interprete partitions and write them to outfile

	// getplen returns the 12 bit value stored for each part-of-partiton packet of
	// the IP packet.  To determine the size of the partition do the following:
	// collect all pop-packets for each partition (check the headers).  Size is
	// if (first pop-packet and plen > 0) then size - plen  // only one pop packet
	// if (first pop-packet and plen = 0) then 
	//    there is more then one pop-packet for that partition
	//    plen = 4096
	//    for (all pop-packets of that partition) do
	//      if (plen > 0) 
	//         plen += plen;  This is exact psize
	//      if (plen = 0)
	//        if this is the very last pop-pakcet of that partiton
	//          plen += 0, end of partition, special case where 0 does not mean 4096 bits
	//        else
	//          plen += 4096

*/

	if (! (packetlen = ReadPacket(p)))
		return (1);		// packet size = 0: finished

	for (ptype = 0; ptype < 8; ptype++) {
		plen = getpartitionlen (ptype, p, packetlen);
		bytecount = getnumberofbytes (plen);
		if (bytecount > 0) {
			assert (1 == fwrite (&plen, 4, 1, out));
			assert (1 == fwrite (&ptype, 4, 1, out));
			assert (1 == fwrite (&picid, 4, 1, out));
			// process header
			sliceid = header & 0x3ff;
			startmb = (header >> 10) & 0x3fff;
			// No way to determine qp, format, but they were initially set to -1
			assert (1 == fwrite (&sliceid, 4, 1, out));
			assert (1 == fwrite (&startmb, 4, 1, out));
			assert (1 == fwrite (&qp, 4, 1, out));
			assert (1 == fwrite (&format, 4, 1, out));
printf ("Writing partition header plen %d, ptype %d, picid %d, bytecount %d\n", plen, ptype, picid, bytecount);
getchar();
			// Header is writtten.  Now go through the whole packet and collect pop-packets
			packetindex = 0;
			while (packetindex < packetlen) {
				if (ptype == getptype (packetindex))
					assert (1 == fwrite (&p[packetindex+2], getnumberofbytes (getplen (packetindex)), 1, out));
				packetindex += 2 + getnumberofbytes (getplen (packetindex));
			}
/*			if (getpartitionlen (ptype, p, packetlen) % 4096 == 0)	// we are here only for non empty partitions
				assert (fseek (out, -4096 / 8, SEEK_CUR));
*/		}
	}

//	assert (packetlen == packetindex);	// Check for all bytes in packet processed, oetherwise violation
	
	// Read the packet #2.  This packet should NOT start with partition 0.  If it does, unread and
	// generate zero coefficients.  Otherwise assume that packet is ok and write partitions
/*
	if (! (packetlen = ReadPacket (p)))
		return (1);		// packet size == 0: 
// printf ("Read pakcet 2 (hopefully), %d bytes\n", packetlen);

	
	if ((ptype = getptype (0)) == 0) {		// found a partition 0: unread packet and conceal
		// Here we know taht packet #2 was lost, as the second packet starts with a picture header partition
		// ideally, the depacketizer would now generate zero coefficients to generate a correct
		// bitstream.  This is however not possible due to the context-sensitive nature of the
		// coefficient coding.  Therefore, this concealmehnt is done in the patched decoder
		// itself.
		// writezerocoefficients();
		assert (0 == fseek(in, -(packetlen+8), SEEK_CUR));  // Packetlen + 4 bytes header + 4 byte picid
// printf ("Found ptype 0, fseeking back %d\n", -packetlen);
	} else {						// found another partition, assume all is ok
// printf ("Found ptype %d, and know its packet 2\n", getptype (0));
		if (packet1picid == picid) {
		
			packetindex = 0;

			for (ptype = 0; ptype < 16; ptype++) {
				plen = getpartitionlen (ptype, p, packetlen);
				bytecount = getnumberofbytes (plen);
				if (bytecount > 0) {
					assert (1 == fwrite (&plen, 4, 1, out));
					assert (1 == fwrite (&ptype, 4, 1, out));
					assert (1 == fwrite (&picid, 4, 1, out));
// printf ("Writing partition header plen %d, ptype %d, picid %d, bytecount %d\n", plen, ptype, picid, bytecount);
// getchar();				
				// Header is writtten.  Now go through the whole packet and collect pop-packets
					packetindex = 0;
					while (packetindex < packetlen) {
						if (ptype == getptype (packetindex))
							assert (1 == fwrite (&p[packetindex+2], getnumberofbytes (getplen (packetindex)), 1, out));
						packetindex += 2 + getnumberofbytes (getplen (packetindex));
					}
					if (getpartitionlen (ptype, p, packetlen) % 4096 == 0)	// we are here only for non empty partitions
						assert (fseek (out, -4096 / 8, SEEK_CUR));
				}
	
			}
//			assert (packetlen == packetindex);	// Check for all bytes in packet processed, oetherwise violation
		}
		 else {
			printf ("Found packet1id %d != packet2id, do nothing %d\n", packet1picid, picid);
		}
	} */
	return 0;
}

int ReadPacket (unsigned char *buf) {
	int packetlen;

	if (1 != fread (&packetlen, 4, 1, in))
		return 0;
// printf ("ReadPacket: packetlen = %d\n");
	assert (1 == fread (&picid, 4, 1, in));
	assert (1 == fread (&header, 4, 1, in));
	assert (1 == fread (buf, packetlen-8, 1, in));
	return packetlen-8;	// Not the picid and header, only the partition data
}

int getplen (int index) {
	int plen = 0;

	memcpy (&plen, &p[index], 2);

// printf ("In getplen: after memcpy plen 0x%x\n", plen);
	plen &= 0x0fff;	// mask ptype
	if (plen == 0)
		plen = 4096;
	return plen;
}

int getptype (int index) {
	int ptype = 0, plen = 0;

	memcpy (&plen, &p[index], 2);
    ptype = (plen >> 12) & 0xf;
	return ptype;
}

int getnumberofbytes (int bits) {
	if (bits % 8 == 0)
		return bits/8;
	else
		return bits/8 + 1;
}

int getpartitionlen (int reqptype, unsigned char *buf, int bufsize) {
	int i = 0, finished = 0, plen, ptype, psize = 0, foundzero = 0;

	while (!finished) {			// Walk through al pops
		ptype = getptype (i);
		plen = getplen (i);
		i += getnumberofbytes (plen) + 2;
		if (i >= bufsize)
			finished = 1;
// printf ("Found ptype %d plen %d\n", ptype, plen);		
		if (ptype == reqptype)
			if (plen < 4096) {
				psize += plen;
				finished = 1;
				foundzero=0;
			} else {	// plen == 0
				psize += 4096;
				foundzero = 1;
			}
	}
	if (foundzero)		// the last pop-packet of the partitrion was 0 -> 4096 bits too much
		psize -= 4096;
	return psize;
}
