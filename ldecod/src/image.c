/*!
 ***********************************************************************
 * \file image.c
 *
 * \brief
 *    Decode a Slice
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
 *    - Rickard Sjoberg                 <rickard.sjoberg@era.ericsson.se>
 *    - Jani Lainema                    <jani.lainema@nokia.com>
 *    - Sebastian Purreiter             <sebastian.purreiter@mch.siemens.de>
 *    - Byeong-Moon Jeon                <jeonbm@lge.com>
 *    - Thomas Wedi                     <wedi@tnt.uni-hannover.de>
 *    - Gabi Blaettermann               <blaetter@hhi.de>
 ***********************************************************************
 */

#include "contributors.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>


#include "global.h"
#include "elements.h"
#include "image.h"


#ifdef _ADAPT_LAST_GROUP_
int *last_P_no;
#endif



/*!
 ***********************************************************************
 * \brief
 *    decodes one I- or P-frame
 *
 ***********************************************************************
 */

int decode_one_frame(struct img_par *img,struct inp_par *inp, struct snr_par *snr)
{

  int current_header;
  Slice *currSlice = img->currentSlice;

  time_t ltime1;                  // for time measurement
  time_t ltime2;

#ifdef WIN32
  struct _timeb tstruct1;
  struct _timeb tstruct2;
#else
  struct timeb tstruct1;
  struct timeb tstruct2;
#endif

  int tmp_time;                   // time used by decoding the last frame

#ifdef WIN32
  _ftime (&tstruct1);             // start time ms
#else
  ftime (&tstruct1);              // start time ms
#endif
  time( &ltime1 );                // start time s

  currSlice->next_header = 0;
  while (currSlice->next_header != EOS && currSlice->next_header != SOP)
  {

    // set the  corresponding read functions
    start_slice(img, inp);

    // read new slice
    current_header = read_new_slice(img, inp);
    img->current_mb_nr = currSlice->start_mb_nr;
    if (inp->symbol_mode == CABAC)
    {
      init_contexts_MotionInfo(img, currSlice->mot_ctx, 1);
      init_contexts_TextureInfo(img,currSlice->tex_ctx, 1);
    }

    if (current_header == EOS)
      return EOS;
    // init new frame
    if (current_header == SOP)
      init_frame(img, inp);

    // decode main slice information
    if (current_header == SOP || current_header == SOS)
      decode_one_slice(img,inp);
    img->current_slice_nr++;

  }
#if !defined LOOP_FILTER_MB
  loopfilter(img);
#endif
  if(img->number == 0)
    copy2mref(img);

  if (p_ref)
    find_snr(snr,img,p_ref,inp->postfilter);      // if ref sequence exist

#ifdef WIN32
  _ftime (&tstruct2);   // end time ms
#else
  ftime (&tstruct2);    // end time ms
#endif
  time( &ltime2 );                                // end time sec
  tmp_time=(ltime2*1000+tstruct2.millitm) - (ltime1*1000+tstruct1.millitm);
  tot_time=tot_time + tmp_time;

  if(img->type == INTRA_IMG) // I picture
    fprintf(stdout,"%3d(I) %3d %5d %7.4f %7.4f %7.4f %5d\n",
        frame_no, img->tr, img->qp,snr->snr_y,snr->snr_u,snr->snr_v,tmp_time);
  else if(img->type == INTER_IMG_1 || img->type == INTER_IMG_MULT) // P pictures
    fprintf(stdout,"%3d(P) %3d %5d %7.4f %7.4f %7.4f %5d\n",
    frame_no, img->tr, img->qp,snr->snr_y,snr->snr_u,snr->snr_v,tmp_time);
  else if(img->type == SP_IMG_1 || img->type == SP_IMG_MULT) // SP pictures
    fprintf(stdout,"%3d(P) %3d %5d %7.4f %7.4f %7.4f %5d\n",
    frame_no, img->tr, img->qp,snr->snr_y,snr->snr_u,snr->snr_v,tmp_time);
  else // B pictures
    fprintf(stdout,"%3d(B) %3d %5d %7.4f %7.4f %7.4f %5d\n",
        frame_no, img->tr, img->qp,snr->snr_y,snr->snr_u,snr->snr_v,tmp_time);

  fflush(stdout);
  // getchar();
  if(img->type == INTER_IMG_1 || img->type == INTER_IMG_MULT) // P pictres
    copy_Pframe(img, inp->postfilter);  // imgY-->imgY_prev, imgUV-->imgUV_prev
  else if(img->type == SP_IMG_1 || img->type == SP_IMG_MULT) // SP pictres
    copy_Pframe(img, inp->postfilter);  // imgY-->imgY_prev, imgUV-->imgUV_prev
  else // I or B pictures
    write_frame(img,inp->postfilter,p_out);         // write image to output YUV file

  if(img->type <= INTRA_IMG || img->type >= SP_IMG_1)   // I or P pictures
    img->number++;
  else
    Bframe_ctr++;    // B pictures

  exit_frame(img, inp);

  img->current_mb_nr = 0;
  return (SOP);
}



/*!
 ************************************************************************
 * \brief
 *    store reconstructed frames in multiple
 *    reference buffer
 ************************************************************************
 */
void copy2mref(struct img_par *img)
{
  int i,j,uv;

  img->frame_cycle=img->number % img->buf_cycle;
  for (j=0; j < img->height; j++)
  for (i=0; i < img->width; i++)
     mref[img->frame_cycle][j][i]=imgY[j][i];

  for (uv=0; uv < 2; uv++)
  for (i=0; i < img->width_cr; i++)
    for (j=0; j < img->height_cr; j++)
    mcef[img->frame_cycle][uv][j][i]=imgUV[uv][j][i];// just copy 1/1 pix, interpolate "online"
}

/*!
 ************************************************************************
 * \brief
 *    Find PSNR for all three components.Compare decoded frame with
 *    the original sequence. Read inp->jumpd frames to reflect frame skipping.
 ************************************************************************
 */
void find_snr(
  struct snr_par *snr,   //!< pointer to snr parameters
  struct img_par *img,   //!< pointer to image parameters
  FILE *p_ref,           //!< filestream to reference YUV file
  int postfilter)        //!< postfilterin on (=1) or off (=1)
{
  int i,j;
  int diff_y,diff_u,diff_v;
  int uv;
  int  status;
#ifndef _ADAPT_LAST_GROUP_
  byte       diff;
#else
  static int p_frame_no;
#endif

#ifndef _ADAPT_LAST_GROUP_
  if(img->type<=INTRA_IMG || img->type >= SP_IMG_1) // I, P pictures
    frame_no=img->number*P_interval;
  else // B pictures
  {
    diff=nextP_tr-img->tr;
    frame_no=(img->number-1)*P_interval-diff;
  }
#else
  if (img->type <= INTRA_IMG || img->type >= SP_IMG_1) // I, P
  {
    if (img->number > 0)
      frame_no = p_frame_no += (256 + img->tr - last_P_no[0]) % 256;
    else
      frame_no = p_frame_no  = 0;
  }
  else // B
  {
    frame_no = p_frame_no - (256 + nextP_tr - img->tr) % 256;
  }
#endif

  rewind(p_ref);
  status = fseek (p_ref, frame_no*img->height*img->width*3/2, 0);
  if (status != 0)
  {
    snprintf(errortext, ET_SIZE, "Error in seeking img->tr: %d", img->tr);
    error(errortext, 500);
  }
  for (j=0; j < img->height; j++)
    for (i=0; i < img->width; i++)
      imgY_ref[j][i]=fgetc(p_ref);
  for (uv=0; uv < 2; uv++)
    for (j=0; j < img->height_cr ; j++)
      for (i=0; i < img->width_cr; i++)
        imgUV_ref[uv][j][i]=fgetc(p_ref);

  img->quad[0]=0;
  diff_y=0;
  for (i=0; i < img->width; ++i)
  {
    for (j=0; j < img->height; ++j)
    {
      diff_y += img->quad[abs(imgY[j][i]-imgY_ref[j][i])];
    }
  }

  // Chroma
  diff_u=0;
  diff_v=0;

  for (i=0; i < img->width_cr; ++i)
  {
    for (j=0; j < img->height_cr; ++j)
    {
      diff_u += img->quad[abs(imgUV_ref[0][j][i]-imgUV[0][j][i])];
      diff_v += img->quad[abs(imgUV_ref[1][j][i]-imgUV[1][j][i])];
    }
  }

  // Collecting SNR statistics
  if (diff_y != 0)
  {
    snr->snr_y=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)diff_y));        // luma snr for current frame
  }

  if (diff_u != 0)
  {
    snr->snr_u=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_u)));    //  chroma snr for current frame
    snr->snr_v=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_v)));    //  chroma snr for current frame
  }

  if (img->number == 0) // first
  {
    snr->snr_y1=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)diff_y));       // keep luma snr for first frame
    snr->snr_u1=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_u)));   // keep chroma snr for first frame
    snr->snr_v1=(float)(10*log10(65025*(float)(img->width)*(img->height)/(float)(4*diff_v)));   // keep chroma snr for first frame
    snr->snr_ya=snr->snr_y1;
    snr->snr_ua=snr->snr_u1;
    snr->snr_va=snr->snr_v1;
  }
  else
  {
    snr->snr_ya=(float)(snr->snr_ya*(img->number+Bframe_ctr)+snr->snr_y)/(img->number+Bframe_ctr+1); // average snr chroma for all frames
    snr->snr_ua=(float)(snr->snr_ua*(img->number+Bframe_ctr)+snr->snr_u)/(img->number+Bframe_ctr+1); // average snr luma for all frames
    snr->snr_va=(float)(snr->snr_va*(img->number+Bframe_ctr)+snr->snr_v)/(img->number+Bframe_ctr+1); // average snr luma for all frames
  }
}

#if !defined LOOP_FILTER_MB


static unsigned char
  overallActivity[256], qp_overallActivity = 255;
static int
  beta;

/*!
 ************************************************************************
 * \brief
 *    Filter to reduce blocking artifacts. The filter strengh
 *    is QP dependent.
 ************************************************************************
 */
void loopfilter(struct img_par *img)
{
  static int MAP[32];

  int x,y,y4,x4,k,uv,str1,str2;

  if (qp_overallActivity != img->qp)
  {
    int
      i, alpha,
      ALPHA_TABLE[32] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        3,  3,  3,  7,  7,  7, 12, 17,
       17, 22, 28, 34, 40, 47, 60, 67,
       82, 89,112,137,153,187,213,249
      },
      BETA_TABLE[32] = {
        0,  0,  0,  0,  0,  0,  0,  0,
        3,  3,  3,  4,  4,  4,  6,  6,
        6,  7,  8,  8,  9,  9, 10, 10,
       11, 11, 12, 12, 13, 13, 14, 14
      };

    for (i=0; i<32; i++)
      MAP[i] = QP2QUANT[i]-1;

    /*
     * ALPHA(img->qp) = ceil(2.0 * MAP(img->qp) * ln(MAP(img->qp)))
     */
    alpha = ALPHA_TABLE[img->qp];

    /*
     * BETA(img->qp) = floor(4.0 * log(MAP[img->qp]) + 0.5)
     */
    beta = BETA_TABLE[img->qp];

    for (k=0; k<256; k++)
      if (2*k >= alpha+(alpha/2))
        overallActivity[k] = 1;
      else if (2*k >= alpha)
        overallActivity[k] = 2;
      else
        overallActivity[k] = 3;

    qp_overallActivity = img->qp;

  }

  // Luma hor
  for(y=0;y<img->height;y++)
    for(x=0;x<img->width;x++)
      imgY_tmp[y][x]=imgY[y][x];

  for(y=0;y<img->height;y++)
  {
    y4=y/4;
    for(x=4;x<img->width;x=x+4)
    {
      x4=x/4;

      str1 = loopb[x4][y4+1];
      str2 = loopb[x4+1][y4+1];

      if((str1!=0)&&(str2!=0))
      {
        /*
         * Copy one line of 4+4 reconstruction pels into the filtering buffer.
         */
        for(k=0;k<8;k++)
          img->li[k]=imgY_tmp[y][x-4+k];

        /*
         * Check for longer filtering on macroblock border if either block is intracoded.
         */
        if(!(x4%4) && (str1 == 3 || str2 == 3))
        {
          if(loop(img,str1,str2, 1, 0))
          {
            imgY_tmp[y][x-4+6] = img->lu[6];
            imgY[y][x-3]       = img->lu[1];
            imgY[y][x+2]       = img->lu[6];
          }
        }
        else
          loop(img,str1,str2, 0, 0);

        /*
         * Place back the filtered values into the filtered reconstruction buffer.
         */
        for (k=2;k<6;k++)
          imgY[y][x-4+k]=img->lu[k];
      }
    }
  }

  // Luma vert
  for(y=0;y<img->height;y++)
    for(x=0;x<img->width;x++)
      imgY_tmp[y][x]=imgY[y][x];

  for(x=0;x<img->width;x++)
  {
    x4=x/4;
    for(y=4;y<img->height;y=y+4)
    {
      y4=y/4;

      str1 = loopb[x4+1][y4];
      str2 = loopb[x4+1][y4+1];

      if((str1 != 0) && (str2 != 0))
      {
        /*
         * Copy one line of 4+4 reconstruction pels into the filtering buffer.
         */
        for(k=0;k<8;k++)
          img->li[k]=imgY_tmp[y-4+k][x];

        /*
         * Check for longer filtering on macroblock border if either block is intracoded.
         */
        if(!(y4%4) && (str1 == 3 || str2 == 3))
        {
          if(loop(img,str1,str2, 1, 0))
          {
            imgY_tmp[y-4+6][x] = img->lu[6];
            imgY[y-3][x]       = img->lu[1];
            imgY[y+2][x]       = img->lu[6];
          }
        }
        else
          loop(img,str1,str2, 0, 0);

        /*
         * Place back the filtered values into the filtered reconstruction buffer.
         */
        for (k=2;k<6;k++)
          imgY[y-4+k][x]=img->lu[k];
      }
    }
  }

  // chroma

  for (uv=0;uv<2;uv++)
  {
    // horizontal chroma
    for(y=0;y<img->height_cr;y++)
      for(x=0;x<img->width_cr;x++)
        imgUV_tmp[uv][y][x]=imgUV[uv][y][x];

    for(y=0;y<img->height_cr;y++)
    {
      y4=y/4;
      for(x=4;x<img->width_cr;x=x+4)
      {
        x4=x/4;

        str1 = loopc[x4][y4+1];
        str2 = loopc[x4+1][y4+1];

        if((str1 != 0) && (str2 != 0))
        {
          /*
           * Copy one line of 4+4 reconstruction pels into the filtering buffer.
           */
          for(k=0;k<8;k++)
            img->li[k]=imgUV_tmp[uv][y][x-4+k];

          /*
           * Check for longer filtering on macroblock border if either block is intracoded.
           */
          if(!(x4%2) && (loopb[2*x4][2*y4+1] == 3 || loopb[2*x4+1][2*y4+1] == 3))
            loop(img,str1,str2, 1, 1);
          else
            loop(img,str1,str2, 0, 1);

          /*
           * Place back the filtered values into the filtered reconstruction buffer.
           */
          for (k=2;k<6;k++)
            imgUV[uv][y][x-4+k]=img->lu[k];
        }
      }
    }

    // vertical chroma
    for(y=0;y<img->height_cr;y++)
      for(x=0;x<img->width_cr;x++)
        imgUV_tmp[uv][y][x]=imgUV[uv][y][x];

    for(x=0;x<img->width_cr;x++)
    {
      x4=x/4;
      for(y=4;y<img->height_cr;y=y+4)
      {
        y4=y/4;

        str1 = loopc[x4+1][y4];
        str2 = loopc[x4+1][y4+1];

        if((str1 != 0) && (str2 != 0))
        {
          /*
           * Copy one line of 4+4 reconstruction pels into the filtering buffer.
           */
          for(k=0;k<8;k++)
            img->li[k]=imgUV_tmp[uv][y-4+k][x];

          /*
           * Check for longer filtering on macroblock border if either block is intracoded.
           */
          if(!(y4%2) && (loopb[2*x4+1][2*y4] == 3 || loopb[2*x4+1][2*y4+1] == 3))
            loop(img,str1,str2, 1, 1);
          else
            loop(img,str1,str2, 0, 1);

          /*
           * Place back the filtered values into the filtered reconstruction buffer.
           */
          for (k=2;k<6;k++)
            imgUV[uv][y-4+k][x]=img->lu[k];
        }
      }
    }
  }
}

/*!
 ************************************************************************
 * \brief
 *    Filter 4 or 6 from 8 pix input
 ************************************************************************
 */
int loop(struct img_par *img, int ibl, int ibr, int longFilt, int chroma)
{
  int
    delta, halfLim, dr, dl, i, truncLimLeft, truncLimRight,
    diff, clip, clip_left, clip_right;

  /* Limit the difference between filtered and nonfiltered based
   * on QP and strength
   */
  clip_left  = FILTER_STR[img->qp][ibl];
  clip_right = FILTER_STR[img->qp][ibr];

  // The step across the block boundaries
  delta   = abs(img->li[3]-img->li[4]);

  // Find overall activity parameter (n/2)
  halfLim = overallActivity[delta];

  // Truncate left limit to 2 for small stengths
  if (ibl <= 1)
    truncLimLeft = (halfLim > 2) ? 2 : halfLim;
  else
    truncLimLeft = halfLim;

  // Truncate right limit to 2 for small stengths
  if (ibr <= 1)
    truncLimRight = (halfLim > 2) ? 2 : halfLim;
  else
    truncLimRight = halfLim;

  // Find right activity parameter dr
  for (dr=1; dr<truncLimRight; dr++)
  {
    if (dr*abs(img->li[4]-img->li[4+dr]) > beta)
      break;
  }

  // Find left activity parameter dl
  for (dl=1; dl<truncLimLeft; dl++)
  {
    if (dl*abs(img->li[3]-img->li[3-dl]) > beta)
      break;
  }

  if(dr < 2 || dl < 2)
  {
    // no filtering when either one of activity params is below two
    img->lu[2] = img->li[2];
    img->lu[3] = img->li[3];
    img->lu[4] = img->li[4];
    img->lu[5] = img->li[5];

    return 0;
  }

  if(longFilt)
  {
    if(dr    == 3 && dl    == 3 &&
       delta >= 2 && delta <  img->qp/4)
    {
      if(chroma)
      {
        img->lu[3] = (25*(img->li[1] + img->li[5]) + 26*(img->li[2] + img->li[3] + img->li[4]) + 64) >> 7;
        img->lu[2] = (25*(img->li[0] + img->li[4]) + 26*(img->li[1] + img->li[2] + img->lu[3]) + 64) >> 7;
        img->lu[4] = (25*(img->li[2] + img->li[6]) + 26*(img->li[3] + img->li[4] + img->li[5]) + 64) >> 7;
        img->lu[5] = (25*(img->li[3] + img->li[7]) + 26*(img->lu[4] + img->li[5] + img->li[6]) + 64) >> 7;

        return 0;
      }
      else
      {
        img->lu[3] = (25*(img->li[1] + img->li[5]) + 26*(img->li[2] + img->li[3] + img->li[4]) + 64) >> 7;
        img->lu[2] = (25*(img->li[0] + img->li[4]) + 26*(img->li[1] + img->li[2] + img->lu[3]) + 64) >> 7;
        img->lu[1] = (25*(img->li[1] + img->lu[3]) + 26*(img->li[0] + img->li[1] + img->lu[2]) + 64) >> 7;
        img->lu[4] = (25*(img->li[2] + img->li[6]) + 26*(img->li[3] + img->li[4] + img->li[5]) + 64) >> 7;
        img->lu[5] = (25*(img->li[3] + img->li[7]) + 26*(img->lu[4] + img->li[5] + img->li[6]) + 64) >> 7;
        img->lu[6] = (25*(img->lu[4] + img->li[6]) + 26*(img->lu[5] + img->li[6] + img->li[7]) + 64) >> 7;

        return 1;
      }
    }
  }

  // Filter pixels at the edge
  img->lu[4] = (21*(img->li[3] + img->li[5]) + 22*img->li[4] + 32) >> 6;
  img->lu[3] = (21*(img->li[2] + img->li[4]) + 22*img->li[3] + 32) >> 6;

  if(dr == 3)
    img->lu[5] = (21*(img->lu[4] + img->li[6]) + 22*img->li[5] + 32) >> 6;
  else
    img->lu[5] = img->li[5];

  if(dl == 3)
    img->lu[2] = (21*(img->li[1] + img->lu[3]) + 22*img->li[2] + 32) >> 6;
  else
    img->lu[2] = img->li[2];

  // Clipping parameter depends on one table and left and right act params
  clip = (clip_left + clip_right + dl + dr) / 2;

  // Pixels at the edge are clipped differently
  for (i=3; i<=4; i++)
  {
    diff = (int)img->lu[i] - (int)img->li[i];

    if (diff)
    {
      if (diff > clip)
        diff = clip;
      else if (diff < -clip)
        diff = -clip;

      img->lu[i] = img->li[i] + diff;
    }
  }

  // pixel from left is clipped
  diff = (int)img->lu[2] - (int)img->li[2];

  if (diff)
  {
    if (diff > clip_left)
      diff = clip_left;
    else if (diff < -clip_left)
      diff = -clip_left;

    img->lu[2] = img->li[2] + diff;
  }

  // pixel from right is clipped
  diff = (int)img->lu[5] - (int)img->li[5];

  if (diff)
  {
    if (diff > clip_right)
      diff = clip_right;
    else if (diff < -clip_right)
      diff = -clip_right;

    img->lu[5] = img->li[5] + diff;
  }

  return 0;
}

#endif


/*!
 ************************************************************************
 * \brief
 *    Direct interpolation of a specific subpel position
 *
 * \author
 *    Thomas Wedi, 12.01.2001   <wedi@tnt.uni-hannover.de>
 *
 * \para Remarks:
 *    A further significant complexity reduction is possible
 *    if the direct interpolation is not performed on pixel basis
 *    but on block basis,
 *
 ************************************************************************
 */
byte get_pixel(int ref_frame,int x_pos, int y_pos, struct img_par *img)
{

  switch(img->mv_res)
  {
  case 0:

    return(get_quarterpel_pixel(ref_frame,x_pos,y_pos,img));

  case 1:

    return(get_eighthpel_pixel(ref_frame,x_pos,y_pos,img));

  default:

    snprintf(errortext, ET_SIZE, "wrong mv-resolution: %d",img->mv_res);
    error(errortext, 600);
    return -1;
  }

}

byte get_quarterpel_pixel(int ref_frame,int x_pos, int y_pos, struct img_par *img)
{

  int dx=0, x=0;
  int dy=0, y=0;
  int maxold_x=0,maxold_y=0;

  int result=0;
  int pres_x=0;
  int pres_y=0;


  x_pos = max (0, min (x_pos, img->width *4-1));
  y_pos = max (0, min (y_pos, img->height*4-1));


  //********************
  //  applied filters
  //
  //  X  2  3  2  X
  //  2  6  7  6
  //  3  7  5  7
  //  2  6  7  6
  //  X           X
  //
  //  X-fullpel positon
  //
  //********************

 dx = x_pos%4;
 dy = y_pos%4;
 maxold_x=img->width-1;
 maxold_y=img->height-1;

 if(dx==0&&dy==0) // fullpel position
   {
     pres_y=y_pos/4;
     pres_x=x_pos/4;
     result=mref[ref_frame][pres_y][pres_x];
     return result;
   }
 else if(dx==3&&dy==3) // funny position
   {
     pres_y=y_pos/4;
     pres_x=x_pos/4;
     result=(mref[ref_frame][pres_y                ][pres_x                ]+
       mref[ref_frame][pres_y                ][min(maxold_x,pres_x+1)]+
       mref[ref_frame][min(maxold_y,pres_y+1)][min(maxold_x,pres_x+1)]+
       mref[ref_frame][min(maxold_y,pres_y+1)][pres_x                ]+2)/4;
     return result;
   }
 else // other positions
  {
    if(x_pos==img->width*4-1)
      dx=2;
    if(y_pos==img->height*4-1)
      dy=2;

    if(dx==1&&dy==0)
      {
  pres_y=y_pos/4;
  for(x=-2;x<4;x++)
    {
      pres_x=max(0,min(maxold_x,x_pos/4+x));
      result+=mref[ref_frame][pres_y][pres_x]*two[x+2];
    }
      }
    else if(dx==0&&dy==1)
      {
  pres_x=x_pos/4;
  for(y=-2;y<4;y++)
    {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      result+=mref[ref_frame][pres_y][pres_x]*two[y+2];
    }
      }
    else if(dx==2&&dy==0)
      {
  pres_y=y_pos/4;
  for(x=-2;x<4;x++)
    {
      pres_x=max(0,min(maxold_x,x_pos/4+x));
      result+=mref[ref_frame][pres_y][pres_x]*three[x+2];
    }
      }
    else if(dx==0&&dy==2)
      {
  pres_x=x_pos/4;
  for(y=-2;y<4;y++)
    {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      result+=mref[ref_frame][pres_y][pres_x]*three[y+2];
    }
      }
    else if(dx==3&&dy==0)
      {
  pres_y=y_pos/4;
  for(x=-2;x<4;x++)
    {
      pres_x=max(0,min(maxold_x,x_pos/4+x));
      result+=mref[ref_frame][pres_y][pres_x]*two[3-x];   // four
    }
      }
    else if(dx==0&&dy==3)
      {
  pres_x=x_pos/4;
  for(y=-2;y<4;y++)
    {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      result+=mref[ref_frame][pres_y][pres_x]*two[3-y];   // four
    }
      }
    else if(dx==1&&dy==1)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*six[y+2][x+2];
        }
      }
      }
    else if(dx==2&&dy==1)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*seven[y+2][x+2];
        }
      }
      }
    else if(dx==1&&dy==2)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*seven[x+2][y+2]; // eight
        }
      }
      }
    else if(dx==3&&dy==1)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*six[y+2][3-x];   // nine
        }
      }
      }
    else if(dx==1&&dy==3)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*six[3-y][x+2];   // ten
        }
      }
      }
    else if(dx==2&&dy==2)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*five[y+2][x+2];
        }
      }
      }
    else if(dx==3&&dy==2)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*seven[3-x][3-y]; // eleven
        }
      }
      }
    else if(dx==2&&dy==3)
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*seven[3-y][x+2]; // twelve
        }
      }
      }
    else if(dx==3&&dy==3) // not used if funny position is on
      {
    for(y=-2;y<4;y++)
      {
      pres_y=max(0,min(maxold_y,y_pos/4+y));
      for(x=-2;x<4;x++)
        {
        pres_x=max(0,min(maxold_x,x_pos/4+x));
        result+=mref[ref_frame][pres_y][pres_x]*six[3-y][3-x]; // thirteen
        }
      }
      }

  }

 return max(0,min(255,(result+2048)/4096));

}

byte get_eighthpel_pixel(int ref_frame,int x_pos,int y_pos, struct img_par *img)
{
 int dx=0, x=0;
 int dy=0, y=0;
 int pres_x=0;
 int pres_y=0;
 int max_x=0,max_y=0;

 byte tmp1[8];
 byte tmp2[8];
 byte tmp3[8];

 double result=0;

 x_pos = max (0, min (x_pos, (img->width *8-2)));
 y_pos = max (0, min (y_pos, (img->height*8-2)));

 dx = x_pos%8;
 dy = y_pos%8;
 pres_x=x_pos/8;
 pres_y=y_pos/8;
 max_x=img->width-1;
 max_y=img->height-1;

 // choose filter depending on subpel position
 if(dx==0 && dy==0)                 // fullpel position
   {
     return(mref[ref_frame][pres_y][pres_x]);
   }
 else if(dx%2==0 && dy==0)
   {
     for(x=-3;x<5;x++)
       {
     tmp3[x+3]= mref[ref_frame][pres_y][max(0,min(pres_x+x,max_x))];
       }
     return(interpolate(tmp3,dx/2) );
   }
 else if(dx==0 && dy%2==0)
   {
     for(y=-3;y<5;y++)
       {
     tmp1[y+3]= mref[ref_frame][max(0,min(pres_y+y,max_y))][pres_x];
       }
     return( interpolate(tmp1,dy/2) );
   }
 else if(dx%2==0 && dy%2==0)
   {
     for(y=-3;y<5;y++)
       {
     for(x=-3;x<5;x++)
     {
         tmp3[x+3]=mref[ref_frame][max(0,min(pres_y+y,max_y))][max(0,min(pres_x+x,max_x))];
     }
     tmp1[y+3]= interpolate(tmp3,dx/2);
       }
     return( interpolate(tmp1,dy/2) );
   }
 else if((dx==1 && dy==0)||(dx==7 && dy==0))
   {
     for(x=-3;x<5;x++)
     {
    tmp1[x+3]= mref[ref_frame][pres_y][max(0,min(pres_x+x,max_x))];
     }
     if(dx==1)
       return( (byte) ((interpolate(tmp1,1) + mref[ref_frame][pres_y][pres_x] +1)/2 ));
     else
       return( (byte) ((interpolate(tmp1,3) + mref[ref_frame][pres_y][max(0,min(pres_x+1,max_x))] +1)/2 ));
   }
 else
   {
     for(y=-3;y<5;y++)
       {
     for(x=-3;x<5;x++)
     {
      tmp3[x+3]= mref[ref_frame][max(0,min(pres_y+y,max_y))][max(0,min(pres_x+x,max_x))];
     }
     tmp1[y+3]= interpolate(tmp3,dx/2);
     tmp2[y+3]= interpolate(tmp3,(dx+1)/2);
       }
     return( (byte) (( interpolate(tmp1,dy/2) + interpolate(tmp2,dy/2) + interpolate(tmp1,(dy+1)/2) + interpolate(tmp2,(dy+1)/2) + 2 )/4 ));
   }
}

byte interpolate(byte container[8],int modus)
{

 int i=0;
 int sum=0;

 static int h1[8] = {  -3,    12,   -37,   229,   71,   -21,     6,    -1 };
 static int h2[8] = {  -3,    12,   -39,   158,  158,   -39,    12,    -3 };
 static int h3[8] = {  -1,     6,   -21,    71,  229,   -37,    12,    -3 };

 switch(modus)
   {
   case 0:
     return(container[3]);
   case 1:
     for(i=0;i<8;i++)
       {
         sum+=h1[i]*container[i];
       }
     return( (byte) max(0,min((sum+128)/256,255)) );
   case 2:
     for(i=0;i<8;i++)
       {
         sum+=h2[i]*container[i];
       }
     return ((byte) max(0,min((sum+128)/256,255)) );
   case 3:
     for(i=0;i<8;i++)
       {
     sum+=h3[i]*container[i];
       }
     return((byte) max(0,min((sum+128)/256,255)) );
   case 4:
     return( (byte) container[4] );

   default: return(-1);
   }

}



/*!
 ************************************************************************
 * \brief
 *    Reads new slice (picture) from bit_stream
 ************************************************************************
 */
int read_new_slice(struct img_par *img, struct inp_par *inp)
{

    int current_header;
    Slice *currSlice = img->currentSlice;

    // read new slice
    current_header = currSlice->readSlice(img,inp);
    return  current_header;
}


/*!
 ************************************************************************
 * \brief
 *    Initializes the parameters for a new frame
 ************************************************************************
 */
void init_frame(struct img_par *img, struct inp_par *inp)
{
  static int first_P = TRUE;
  int i,j;

  img->current_mb_nr=0;
  img->current_slice_nr=0;

  img->mb_y = img->mb_x = 0;
  img->block_y = img->pix_y = img->pix_c_y = 0; // define vertical positions
  img->block_x = img->pix_x = img->pix_c_x = 0; // define horizontal positions

  if(img->type == INTER_IMG_1 || img->type == INTER_IMG_MULT || img->type == SP_IMG_1 || img->type == SP_IMG_MULT)  // P pictures
  {
#ifdef _ADAPT_LAST_GROUP_
    for (i = img->buf_cycle-1; i > 0; i--)
      last_P_no[i] = last_P_no[i-1];
    last_P_no[0] = nextP_tr;
#endif
    nextP_tr=img->tr;
  }
  else if(img->type==INTRA_IMG) // I picture
    nextP_tr=prevP_tr=img->tr;

  if(img->type == INTER_IMG_1 || img->type == INTER_IMG_MULT || img->type == SP_IMG_1 || img->type == SP_IMG_MULT)
  {
    if(first_P) // first P picture
    {
      first_P = FALSE;
      P_interval=nextP_tr-prevP_tr;
    }
    else // all other P pictures
    {
      write_prev_Pframe(img, p_out);  // imgY_prev, imgUV_prev -> file
    }
  }

  if (img->type > SP_IMG_MULT)
  {
    set_ec_flag(SE_PTYPE);
    img->type = INTER_IMG_1;  // concealed element
  }

  img->max_mb_nr = (img->width * img->height) / (MB_BLOCK_SIZE * MB_BLOCK_SIZE);

  // allocate memory for frame buffers
  if (img->number == 0)  get_mem4global_buffers(inp, img);

#if !defined LOOP_FILTER_MB
  init_loop_filter(img);
#endif

  for(i=0;i<img->width/BLOCK_SIZE+1;i++)          // set edge to -1, indicate nothing to predict from
    img->ipredmode[i+1][0]=-1;
  for(j=0;j<img->height/BLOCK_SIZE+1;j++)
    img->ipredmode[0][j+1]=-1;

  if(img->UseConstrainedIntraPred)
  {
    for (i=0; i<img->width/MB_BLOCK_SIZE*img->height/MB_BLOCK_SIZE; i++)
      img->intra_mb[i] = 1; // default 1 = intra mb
  }
}


void exit_frame(struct img_par *img, struct inp_par *inp)
{
    if(img->type==INTRA_IMG || img->type == INTER_IMG_1 || img->type == INTER_IMG_MULT || img->type == SP_IMG_1 || img->type == SP_IMG_MULT)
        copy2mref(img);
}



/*!
 ************************************************************************
 * \brief
 *    decodes one slice
 ************************************************************************
 */
void decode_one_slice(struct img_par *img,struct inp_par *inp)
{

  Boolean end_of_slice = FALSE;
  int read_flag;
  Slice *currSlice = img->currentSlice;

  img->cod_counter=-1;

  reset_ec_flags();

  while (end_of_slice == FALSE) // loop over macroblocks
  {

#if TRACE
    fprintf(p_trace,"\n*********** Pic: %i (I/P) MB: %i Slice: %i **********\n\n", img->tr, img->current_mb_nr, img->slice_numbers[img->current_mb_nr]);
#endif

    // Initializes the current macroblock
    start_macroblock(img,inp);

    // Get the syntax elements from the NAL
    read_flag = read_one_macroblock(img,inp);

    // decode one macroblock
    switch(read_flag)
    {
    case DECODE_MB:
      decode_one_macroblock(img,inp);
      break;
    case DECODE_COPY_MB:
      decode_one_CopyMB(img,inp);
      break;
    case DECODE_MB_BFRAME:
      decode_one_macroblock_Bframe(img);
      break;
    default:
        printf("need to trigger error concealment or something here\n ");
    }
#if defined LOOP_FILTER_MB
    DeblockMb( img ) ;
#endif
    end_of_slice=exit_macroblock(img,inp);
  }
  reset_ec_flags();
}

/*!
 ************************************************************************
 * \brief
 *    initializes loop filter
 ************************************************************************
 */
void init_loop_filter(struct img_par *img)
{
  int i, j;

  for (i=0;i<img->width/4+2;i++)
    for (j=0;j<img->height/4+2;j++)
      loopb[i+1][j+1]=0;

  for (i=0;i<img->width_cr/4+2;i++)
    for (j=0;j<img->height_cr/4+2;j++)
      loopc[i+1][j+1]=0;
}

