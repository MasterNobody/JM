Notice
======

Tml 5.9 includes the following changes from Tml 5.2:
- improved loop filter
- B picture
 (postfilter, boundary constraints for direct MVs, residual coding at direct mode)
- improved sub-pel interpolation (direct interpolation)
- NO NAL stuff

Tml 5.9 does not contain the work of NAL and slice modes yet. 
So their parameters at lencod.dat(and ldecod.dat) must be OFF.


Changes for loop filter
=======================

1. New lower complexity loop filter added. 

2. Fixed two bugs in macroblock.c:
   - Corrected the calculation of motion vector differences
     when determining filtering strengths
   - Corrected the calculation of chrominance filtering
     strengths in the case of nonzero chroma AC coefficients


Encoder changes for B picture
=============================

1. Two input parameters for B picture are added to lencod.dat file.
One is the number of B pictures inserted, which is equal to or smaller 
than the number of frames to be skipped in P pictures. 
The other is QP used in B pictures. 
For the reference, the number of multiple reference frames used in B pictures 
is identical to that in P pictures.

2. Some variables and constants are added to global.h file for B pictures.

3. Some functions in main() of lencod.c are moved into image() of image.c, 
such as loopfilter, oneforthpix, and find_snr. 
But this modification results in the same output as tml 5.2.
Especially oneforthpix(), which is executed only at I picture, is divided into 
two functions oneforthpix_1() and oneforthpix_2(). 
oneforthpix_1() copies the predicted image of P picture to 1/4 pel-resolution buffer 
and oneforthpix_2() moves buffer contents to mref/mcef array after B picture encoding is done.
find_snr() is a little bit modified to calculate correct PSNR value 
in considering predicted B pictures.

4. Since B picture is not used as reference frame, the predicted image can be directly 
written to recon file without applying loopfilter().

5. In image(), if the number of B pictures inserted is equal to or more than 1, 
then image() does B picture coding after P picture coding is done. 

6. B_frame.c file contains new functions for B picture coding. 

7. When a sequence is encoded as IBPBP, img->number is 

	               I   B   P   B   P   B   P ......
	img->tr        0   1   2   3   4   5   6
	img->number    0   1   1   2   2   3   3

8. Residual coding at direct mode is done.


Decoder changes for B picture
=============================

1. While loop in main() of ldecod.c and decode_slice() of image.c are modified 
because the decoding order is I, P, B, P, B. 

2. As encoder does, oneforthpix() is divided into oneforthpix_1() and oneforthpix_2(). 
And also find_snr() is a little bit modified to calculate correct PSNR value.

3. loop filter in encoder is used as postfilter of B picture. 

4. Residual coding at direct mode is done.

5. B_frame.c file contains new functions for B picture decoding.

6. decoded I picture is enlarged into 1/4 pel resolution and written to recon file. 
If current decoded picture is P, then it is copied to 1/4 pel-resolution buffer temporarily 
to decode B picture and is written to recon file/mref array when all inserted B pictures are 
decoded and another P picture is decoded.
B picture is decoded from previous decoded P pictures.

Overall decoding flow is as follows when decoding IBPBP...

I picture :
	nal_find_startcode();
	decode_slice();
		- macroblock(); // decoding
		- loopfilter_new();
		- oneforthpix(); // decoded picture is written to mref/mcef array
	find_snr();
	write_frame(); // decoded picture is written to recon file
	img->number=1;
P picture (first P) :
	nal_find_startcode();
	decode_slice();
		- macroblock(); // decoding
		- loopfilter_new();
		- oneforthpix_1(); // decoded picture is copied to 1/4pel-resolution buffer
	find_snr();
	copy_Pframe(); // decoded picture is copied to image buffer
	img->number=2;
B picture :
	nal_find_startcode();
	decode_slice();
		- Bframe_ctr=1;
		- initialize_Bframe();
		- decode_Bframe(); // decoding
	find_snr();
	write_frame(); // decoded picture is written to recon file
P picture :
	nal_find_startcode();
	decode_slice();
		- write_prev_Pframe();	// image buffer is written to recon file
		- oneforthpix_2();  // 1/4pel-resolution buffer is written to mref/mcef array
		- macroblock(); // decoding
		- loopfilter_new();
		- oneforthpix_1(); // decoded picture is copied to 1/4pel-resolution buffer
	find_snr();
	copy_Pframe(); // decoded picture is copied to image buffer
	img->number=3;
B picture :
	nal_find_startcode();
	decode_slice();
		- Bframe_ctr=2;
		- initialize_Bframe();
		- decode_Bframe(); // decoding
	find_snr();
	write_frame(); // decoded picture is written to recon file

	               I   B   P   B   P   B   P ......
	img->tr        0   1   2   3   4   5   6
	img->number    0   2   1   3   2   4   3 


Encoder changes for direct interpolation
========================================

 - In principle the encoder is not changed.
 - Only the upsampling function (oneforthpix) is slightly changed. This is
   done to avoid mismatches between the different interpolation schemes 
   caused by rounding effects.

Decoder changes for direct interpolation
========================================

 - The reference frames are not sampled up by the factor of 4 any more.
   This means that the frames are not stored in the multiframe buffer at
   the big resolution but at the original resolution.
 - Only the subpels that are used in the motion compensated prediction
   are interpolated. Therefore a direct interpolation filter is used 
   (see VCEG-L20).

