#define HACK

#include "global.h"

// Refbuf.h		Declarations of the reference frame buffer types and functions
#ifdef HACK
//typedef unsigned char	pel_t;		// Only 8 bit per pixel for now

pel_t	UMVPelY_14 (pel_t **Pic, int y, int x);
pel_t	FastPelY_14 (pel_t **Pic, int y, int x);

pel_t	UMVPelY_18 (pel_t **Pic, int y, int x);
pel_t	FastPelY_18 (pel_t **Pic, int y, int x);

pel_t	UMVPelY_11 (pel_t *Pic, int y, int x);
pel_t	FastPelY_11 (pel_t *Pic, int y, int x);
pel_t	*FastLine16Y_11 (pel_t *Pic, int y, int x);
pel_t	*UMVLine16Y_11 (pel_t *Pic, int y, int x);

void PutPel_14 (pel_t **Pic, int y, int x, pel_t val);
void PutPel_11 (pel_t *Pic, int y, int x, pel_t val);
#endif


























































#ifndef HACK

typedef struct {
	int		Id;			// reference buffer ID, TR or Annex U style
	// The planes
	pel_t	*y;			// Buffer of Y, U, V pels, Semantic is hidden in refbuf.c
	pel_t	*u;			// Currently this is a 1/4 pel buffer
	pel_t	*v;

	// Plane geometric/storage characteristics
	int		x_ysize;	// Size of Y buffer in columns, should be aligned to machine
	int		y_ysize;	// chracateristics such as word and cache line size, will be
						// set by the alloc routines
	int		x_uvsize;
	int		y_uvsize;
	
	// Active pixels in plane
	int		x_yfirst;	// First active column, measured in 1/1 pel
	int		x_ylast;	// Last active column
	int		y_yfirst;	// First active row, measure in 1/1 pel
	int		y_ylast;	// Last acrive row

	int		x_uvfirst;
	int		x_uvlast;
	int		y_uvfirst;
	int		y_uvlast;

} refpic_t;


// Alloc and free for reference buffers

refpic_t *AllocRefPic (int Id, 
					  int NumCols, 
					  int NumRows,
					  int MaxMotionVectorX,		// MV Size may be used to allocate additional
					  int MaxMotionVectorY);	// memory around boundaries fro UMV search

int FreeRefPic (refpic_t *Pic);

// Access functions for full pel (1/1 pel)

pel_t	PelY_11 (refpic_t *Pic, int y, int x);
pel_t	PelU_11 (refpic_t *Pic, int y, int x);
pel_t	PelV_11 (refpic_t *Pic, int y, int x);

pel_t	*MBLineY_11 (refpic_t *Pic, int y, int x);

// Access functions for half pel (1/2 pel)

pel_t	PelY_12 (refpic_t *Pic, int y, int x);


// Access functions for quater pel (1/4 pel)

pel_t	PelY_14 (refpic_t *Pic, int y, int x);


// Access functions for one-eigths pel (1/8 pel)

pel_t	PelY_18 (refpic_t *Pic, int y, int x);


#endif
