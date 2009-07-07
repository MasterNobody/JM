/*!
 ***************************************************************************
 * \file
 *    md_distortion.h
 *
 * \author
 *    Alexis Michael Tourapis        <alexis.tourapis@dolby.com>
 *
 * \date
 *    7. February 2009
 *
 * \brief
 *    Headerfile for distortion functions
 **************************************************************************
 */

#ifndef _MD_DISTORTION_H_
#define _MD_DISTORTION_H_

#include "global.h"

// Functions
extern void  setupDistortion (Slice *currSlice);
extern int64 compute_SSE     (imgpel **imgRef, imgpel **imgSrc, int xRef, int xSrc, int ySize, int xSize);
extern int64 compute_SSE_cr  (imgpel **imgRef, imgpel **imgSrc, int xRef, int xSrc, int ySize, int xSize);
extern int64 compute_SSE16x16(imgpel **imgRef, imgpel **imgSrc, int xRef, int xSrc);
extern int64 compute_SSE8x8  (imgpel **imgRef, imgpel **imgSrc, int xRef, int xSrc);
extern int64 compute_SSE4x4  (imgpel **imgRef, imgpel **imgSrc, int xRef, int xSrc);
#endif
