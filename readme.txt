                          TML H 26L codec
                          ===================

                        Version 4.5, Sep 23, 2000

                   (C) Telenor Broadband Services 
                       Ericsson Radio Systems AB
                       TELES AG			

lcodec4.5 is an implementation of the TML4 with a few divagations from the description(see below). 
It consists of C source code for both encoder and decoder. We have used MS Visual C++, but the source code should be portable to other platforms.
For simplicity we have supplied executable codec in addition to the C source code. 

This software is according to Test Model Long Term Number 4 (TML4) with the following exceptions:

-No B-Pictures

=================================================================
 Ericsson implemented the changes between version 4.3 and 4.4
 The code was modified between Aug 31 and Sep 15 2000
 Responsible for these changes: rickard.sjoberg@era.ericsson.se
 See text file changes_v44.txt for list of modifications
=================================================================
TELES added Data Partitioning support, see changes_v45.txt, stewe@cs.tu-berlin.de
=================================================================

Bugfixes from version 4.1
===============
1)
ipredmode, loopb and loopc too small for CIF images:
int ipredmode[89+1][73+1];// change
byte loopb[90+1][74+1];               
byte loopc[46+1][38+1];

2) Under /*  Bits for chroma 2x2 DC transform coefficients */ :

encoder
... (some code deleted)
        if (level != 0); // ; shall be removed!


        if (level != 0)   // like this     
        {
          for (j=0;j<2;j++)
          {
           ...
decoder
if (len != 1)                         
          { 
            linfo_levrun_c2x2(len,info,&level,&run);
            coef_ctr=coef_ctr+run+1;
            if(coef_ctr>3)
              return SEARCH_SYNC;
            img->cofu[coef_ctr]=level*JQ1[QP_SCALE_CR[img->qp]];
            
         /* } fix from ver 4.1 */
          
            if (level != 0 );
            { 
              for (j=0;j<2;j++)
              {
                for (i=0;i<2;i++)
                {
                  loopc[m2+i+1][jg2+j+1]=max(loopc[m2+i+1][jg2+j+1],2);
                }
              }
            
              for (i=0;i<2;i++)
              {
                loopc[m2+i+1][jg2    ]=max(loopc[m2+i+1][jg2    ],1);
                loopc[m2+i+1][jg2+3  ]=max(loopc[m2+i+1][jg2+3  ],1);
                loopc[m2    ][jg2+i+1]=max(loopc[m2    ][jg2+i+1],1);
                loopc[m2+3  ][jg2+i+1]=max(loopc[m2+3  ][jg2+i+1],1);
              }
            }
          }
        }/* fix from ver 4.1 */


3) two line marked /* CIF mismatch */ in macroblock() encoder/decoder removed:
    if (img->block_x>0 ) /* CIF mismatch */ // lines shall be removed

    loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
    loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1); 
    //if (img->block_x>0 ) /* CIF mismatch */  // removed
    //{
    loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
    loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
    //}


Usage encoder:
===============
From DOS command prompt:
lencod <lencod.dat>
For the format of the configuration file, see description in lencod.dat.
During execution, the encoder will produce a log file, a status file and a H.26L bitstream.
 
Usage decoder:
===============
From DOS command prompt:
ldecod <ldecod.dat>
For the format of the configuration file, see description in ldecod.dat.
During execution, the encoder will produce a log file and a decoded sequence.

TML4:
=====================
Output document from Osaka meeting.

Files
=====
Following files and directories are part of ldecode.zip:

lencod/         : dir with source code for the encoder
ldecod/         : dir with source code for the decoder
readme.txt      : this description file
disclaimer.txt  : disclaimer of warranty
ldecod.exe      : executable decoder (release build)
lencod.exe      : executable encoder (release build)
lencod.dat      : example encoder configuration file
ldecod.dat      : example decoder configuration file
foreman5.yuv    : test input yuv file for the encoder

Following files are generated during execution(using the given config files):
log.dat         : encoder log file, new run are appended 
stat.dat        : encoder status file for the last run
test.dec        : encoder decoded yuv sequence (for debugging)
foreman5.dec    : decoder decoded yuv sequence
foreman5.26l    : encoder output coded bitstream

Contact for this software
=========================
inge.lille-langoy@telenor.com

Contact for Telenor proposed description
========================================
gisle.bjontegaard@telenor.com