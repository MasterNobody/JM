/*!
 * 	\file 	
 *			NALenc4H223.c
 * 	\brief	
 *			NAL-Encoder for datapartitioned H.26L bitstreams to H.223 Annex B/D
 *	\date	
 *			22.10.2000
 *	\version	
 *			1.0
 * 	\author
 *			Stephan Wenger			<stewe@cs.tu-berlin.de>
 *			Sebastian Purreiter		<sebastian.purreiter@mch.siemens.de>
 *	\note	
 *			assignment of partitions to logical channels can be varied with 
 *			the variable "mode" in main. \par
 *
 *			Further modes can be enhanced in NAL4H223.h 
 *	\sa		
 *			NAL4H223.h, main
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "NAL4H223.h"
#include "NALenc4H223.h"

/*!
 * 	\fn 	main()
 *	\brief	main function
 */
int main (int ac, char *av[])
{
	/*!
	 * mode to assign partitions to different logical channels as
	 * required for H.223 Annex D
	 * mode = 0 <=> H.223 Annex B only one logical channel available
	 */
	int mode = 0;

	if (ac != 3)
	{
		printf ("Usage: %s <infile.h26l> <outfile.pac>\n",av[0]);
		printf ("   infile.h26l =Data Partitioned H.26L bitstream\n");
		printf ("   outfile.pac =Packetized H.26L bitstream for H.223 mux\n");
		exit (-1);
	}
	if ((in = fopen (av[1], "rb")) == NULL)
	{
		printf ("%s: can't open %s for reading... exit\n",av[0], av[1]);
		exit (-2);
	}
	if ((out = fopen (av[2], "wb")) == NULL)
	{
		printf ("%s: can't open %s for writing... exit\n",av[0], av[2]);
		exit (-3);
	}

	while (!finished)
	{
		finished = ReadPicture();
		WritePicture(mode);
	}
	return 0;
}


/*!
 *	\fn		ReadPicture
 *	\brief	Reads all partitions of one slice/frame
 *	\return	status if input file end is reached or not
 *			(0=no EOF; 1=EOF)
 */

int ReadPicture ()
{
	int i;

	for (i=0; i<MAXPARTITION; i++)
		p[i].picFormat = NO_PARTITION_CONTENT;	/* required to detect last partition of slice/frame */

	i = 0;

	/* Read Picture Header */
	if (1 != fread (&p[i].partitionsizebits, 4, 1, in))
		return 1;		// Finished, EOF
	assert (1 == fread (&p[i].partitionID, 4, 1, in));
	assert (1 == fread (&p[i].picID, 4, 1, in));
	assert (1 == fread (&p[i].sliceID, 4, 1, in));
	assert (1 == fread (&p[i].startMB, 4, 1, in));
	assert (1 == fread (&p[i].qp, 4, 1, in));
	assert (1 == fread (&p[i].picFormat, 4, 1, in));
	assert (1 == fread (&p[i].data[0], NumberOfBytes(p[i].partitionsizebits), 1, in));

	printf ("ReadPicture,  id:%2d  picid:%3d  sliceid:%3d  startmb:%3d  pqp:%2d  pformat:%1d  size:%6d  bytes:%4d\n",
		p[i].partitionID,	p[i].picID, p[i].sliceID,	p[i].startMB,
		p[i].qp, p[i].picFormat, p[i].partitionsizebits, NumberOfBytes(p[i].partitionsizebits));

	/* Read all partitions in one slice */
	while (1)
	{
		i++;
		if (1 != fread (&p[i].partitionsizebits, 4, 1, in))
			return 1;		// Finished, EOF
		assert (1 == fread (&p[i].partitionID, 4, 1, in));
		assert (1 == fread (&p[i].picID, 4, 1, in));
		assert (1 == fread (&p[i].sliceID, 4, 1, in));
		assert (1 == fread (&p[i].startMB, 4, 1, in));
		assert (1 == fread (&p[i].qp, 4, 1, in));
		assert (1 == fread (&p[i].picFormat, 4, 1, in));

		if (p[i].partitionID == 0)
		{	/* New picture header found . new picture */
			p[i].picFormat = NO_PARTITION_CONTENT;
			assert (0 == fseek (in, -28, SEEK_CUR));
			return 0;
		}
		printf ("ReadPicture,  id:%2d  picid:%3d  sliceid:%3d  startmb:%3d  pqp:%2d  pformat:%1d  size:%6d  bytes:%4d\n",
			p[i].partitionID,	p[i].picID, p[i].sliceID,	p[i].startMB,
			p[i].qp, p[i].picFormat, p[i].partitionsizebits, NumberOfBytes(p[i].partitionsizebits));

		assert (1 == fread (&p[i].data[0], NumberOfBytes(p[i].partitionsizebits), 1, in));
	}
}


/*!
 *	\fn 	WritePicture
 *	\brief 	Writes partitions of slice/frame and sends
 *			it to different logical channels
 *			Output format is in accordance to document Q15-K-15
 *	\param	mode, mode for assignment of partitions to logical channels
 */

void WritePicture (int mode)
{
	int i = 0;
	int num = 0;
	int logicalchannel = 0;
	int bytes;

	while (p[i].picFormat != NO_PARTITION_CONTENT) /* write until picFormat in current partition is NOCONTENT */
	{
		/* partitions with zero length are not written to multiplexer */
		if (p[i].partitionsizebits > 0)
		{
			bytes = NumberOfBytes(p[i].partitionsizebits);
			logicalchannel = partition2logicalchannel[mode][p[i].partitionID];

			//printf("logic %d \t bytes %d \t part %d \n", logicalchannel, bytes, i);
			//printf ("writepos %6d\n", ftell(out));
			assert (1 == fwrite (&logicalchannel, 1, 1, out));
			assert (1 == fwrite (&bytes, 4, 1, out));
			assert (1 == fwrite (&p[i].data[0], bytes, 1, out));
		}
		i++;
	}
}


/*!
 *	\fn 	NumberOfBytes
 *	\brief	Calculates number of bytes from number of bits
 *	\param	bits, number of bits
 *	\return number of necessary bytes
 */

int NumberOfBytes (int bits)
{
	int bytes;

	bytes = bits / 8;
	if (bits % 8)
		bytes++;
	return bytes;
}


