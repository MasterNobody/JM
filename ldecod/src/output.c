/*!
 ************************************************************************
 * \file output.c
 *
 * \brief
 *    Output an image and Trance support
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 ************************************************************************
 */

#include "contributors.h"

#include <stdlib.h>

#include "global.h"

/*!
 ************************************************************************
 * \brief
 *    Write decoded frame to output file
 ************************************************************************
 */
void write_frame(
  struct img_par *img,  //!< image parameter
  int postfilter,       //!< postfilter on (=1) or off (=0)
  FILE *p_out)          //!< filestream to output file
{
  int i,j;

  /*
   * the mmin, mmax macros are taken out, because it makes no sense due to limited range of data type
   */

  if(postfilter)
  {
    for(i=0;i<img->height;i++)
      for(j=0;j<img->width;j++)
      {
        fputc(imgY_pf[i][j],p_out);
      }
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)
      {
        fputc(imgUV_pf[0][i][j],p_out);
      }
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)
      {
        fputc(imgUV_pf[1][i][j],p_out);
      }
  }
  else
  {
    for(i=0;i<img->height;i++)
      for(j=0;j<img->width;j++)
      {
        fputc(imgY[i][j],p_out);
      }
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)
      {
        fputc(imgUV[0][i][j],p_out);
      }
    for(i=0;i<img->height_cr;i++)
      for(j=0;j<img->width_cr;j++)
      {
        fputc(imgUV[1][i][j],p_out);
      }
  }
}


#if TRACE

/*!
 ************************************************************************
 * \brief
 *    Tracing bitpatterns for symbols
 *    A code word has the following format: 0 Xn...0 X2 0 X1 0 X0 1
 ************************************************************************
 */
void tracebits(
    const char *trace_str,  //!< tracing information, char array describing the symbol
    int len,                //!< length of syntax element in bits
    int info,               //!< infoword of syntax element
    int value1,
    int value2)
{
  static int bitcounter = 0;

  int i, chars;
  // int outint = 1;

  if(len>=34)
  {
    snprintf(errortext, ET_SIZE, "Length argument to put too long for trace to work");
    error (errortext, 600);
  }


  putc('@', p_trace);
  chars = fprintf(p_trace, "%i", bitcounter);
  while(chars++ < 6)
    putc(' ',p_trace);
  chars += fprintf(p_trace, "%s", trace_str);
  while(chars++ < 30)
    putc(' ',p_trace);

  // Align bitpattern
  if(len<15)
    for(i=0 ; i<15-len ; i++)
      fputc(' ', p_trace);


  // Print bitpattern
  for(i=0 ; i<len-1 ; i++)
  {
    if(i%2 == 0)
    {
      fputc('0', p_trace);
    }
    else
    {
      if (0x01 & ( info >> ((len-i)/2-1)))
        fputc('1', p_trace);
      else
        fputc('0', p_trace);
    }
  }

  // put out the last 1
  fprintf(p_trace, "1\n");

  bitcounter += len;
}
#endif
