// Refbuf.c		Declarations of teh reference frame buffer types and functions

#define HACK


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include "refbuf.h"
#include "global.h"

#define CACHELINESIZE	32

#ifdef HACK


// Reference buffer write routines
//
//

void PutPel_14 (pel_t **Pic, int y, int x, pel_t val) {
	Pic [y][x] = val;
}

void PutPel_11 (pel_t *Pic, int y, int x, pel_t val) {
	Pic [y*img->width+x] = val;
}


// Reference buffer read, Full pel
//
pel_t	FastPelY_11 (pel_t *Pic, int y, int x) {

	return Pic [y*img->width+x];
}


pel_t	*FastLine16Y_11 (pel_t *Pic, int y, int x) {
	return &Pic [y*img->width+x];
}

pel_t	UMVPelY_11 (pel_t *Pic, int y, int x) {
	if (x < 0) {										
		if (y < 0)
			return Pic [0];
		if (y >= img->height)
			return Pic [(img->height-1) * img->width];
		return Pic [y*img->width];
	}

	if (x >= img->width) {
		if (y < 0)
			return Pic [img->width-1];
		if (y >= img->height)
			return Pic [img->height * img->width -1];
		return Pic [(y+1)*img->width -1 ];
	}

	if (y < 0)		// note: corner pixels were already processed
		return Pic [x];
	if (y >= img->height)
		return Pic [(img->height-1)*img->width+x];

	return Pic [y*img->width+x];
}

// Note: the follonwing function is NOT reentrant!  Use a buffer 
// provided by the caller to change that (but it costs a memcpy()...

static pel_t line[16];
pel_t	*UMVLine16Y_11 (pel_t *Pic, int y, int x) {
	int i;

	for (i=0; i<16; i++)
		line[i] = UMVPelY_11 (Pic, y, x+i);
	return line;
}


// Reference buffer read, 1/2 pel
//
pel_t	FastPelY_12 (pel_t **Pic, int y, int x) {
	return Pic [y<<1][x<<1];
}


pel_t	UMVPelY_12 (pel_t **Pic, int y, int x) {
	return UMVPelY_14 (Pic, y*2, x*2);
}


// Reference buffer, 1/4 pel
//
pel_t	UMVPelY_14 (pel_t **Pic, int y, int x) {

	int width4	= (img->width<<2)-1;
	int height4 = (img->height<<2)-1;
	
	if (x < 0) {
		if (y < 0)
			return Pic [0][0];
		if (y > height4)
			return Pic [height4][0];
		return Pic [y][0];
	}

	if (x > width4) {
		if (y < 0)
			return Pic [0][width4];
		if (y > height4)
			return Pic [height4][width4];
		return Pic [y][width4];
	}

	if (y < 0)		// note: corner pixels were already processed
		return Pic [0][x];
	if (y > height4)
		return Pic [height4][x];


	return Pic [y][x];
}

pel_t	FastPelY_14 (pel_t**Pic, int y, int x) {
	return Pic [y][x];
}


// reference buffer, 1/8th pel
	
pel_t	UMVPelY_18_old (pel_t **Pic, int y, int x)
{
	byte out;
	int yfloor, xfloor, x_max, y_max;

	x = max (0, min (x, (img->width *8-2)));
	y = max (0, min (y, (img->height*8-2)));

	xfloor=x/2;
	yfloor=y/2;


	x_max=img->width *4-1;
	y_max=img->height*4-1;

	if(xfloor<0 || xfloor > x_max)
	  {
	    printf("\n WARNING(get_eigthpix_pel): xfloor = %d is out of range",xfloor);
	    xfloor=min(x_max,max(0,xfloor));
	    printf(", set to %d\n",xfloor);
	  }

	if(yfloor<0 || yfloor > y_max)
	  {
	    printf("\n WARNING(get_eigthpix_pel): yfloor = %d is out of range",yfloor);
	    yfloor=min(y_max,max(0,yfloor));
	    printf(", set to %d\n",yfloor);

	  }


	if( y == img->height*8-1 )
	  {
	    if (x%2 && x != img->width*8-1)
	      {
		out=( Pic[yfloor  ][xfloor  ] +
		      Pic[yfloor  ][xfloor+1] + 1 ) / 2;
	      }
	    else
	      out=  Pic[yfloor  ][xfloor  ];
	      
	  }
	else 	if( x == img->width*8-1 )
	  {
	    if (y%2)
	      {
		out=( Pic[yfloor  ][xfloor  ] +
		      Pic[yfloor+1][xfloor  ] + 1 ) / 2;
	      }
	    else
	      out=  Pic[yfloor  ][xfloor  ];
	      
	  }
	else if( x%2 && y%2 )
	  {
	    out=( Pic[yfloor  ][xfloor  ] +
		  Pic[yfloor+1][xfloor  ] +
		  Pic[yfloor  ][xfloor+1] +
		  Pic[yfloor+1][xfloor+1] + 2 ) / 4;
	  }
	else if (x%2)
	  {
	    out=( Pic[yfloor  ][xfloor  ] +
		  Pic[yfloor  ][xfloor+1] + 1 ) / 2;
	  }
	else if (y%2)
	  {
	    out=( Pic[yfloor  ][xfloor  ] +
		  Pic[yfloor+1][xfloor  ] + 1 ) / 2;
	  }
	else
	  out=  Pic[yfloor  ][xfloor  ];
	
	return(out);

}

pel_t	UMVPelY_18 (pel_t **Pic, int y, int x) {
	
	byte out;
	int yfloor, xfloor;
	
	int width8  = (img->width<<3) - 2;		// Width and Height of the 1/4 buffer, measure in 1/8 pel
	int height8 = (img->height<<3) -2;		// the -2 is for the bound checking, since arrays start at 0
	

	// The following two are macros and not variables, because they need to be calculated only
	// rarely, and hence variables that are always evaluated would be inappropriate.  The macros
	// are more easily readable.

#define	width4	((img->width<<2)-1)
#define height4	((img->height<<2)-1)

	if (x < 0) {
		if (y < 0)
			return Pic [0][0];
		if (y > height8)
			return Pic [height4][0];
		x=0;
	}

	if (x > width8) {
		if (y < 0)
			return Pic [0][width4];
		if (y > height8)
			return Pic [height4][width4];
		x=width8;
	}

	if (y < 0)		// note: corner pixels were already processed
		y=0;
	if (y > height8)
		y=height8;

#undef width4
#undef height4


	xfloor=x>>1;
	yfloor=y>>1;

	if( x%2 && y%2 )
	{
		out=( Pic[yfloor  ][xfloor  ] +
		      Pic[yfloor+1][xfloor  ] +
			  Pic[yfloor  ][xfloor+1] +
			  Pic[yfloor+1][xfloor+1] + 2 ) / 4;
	}
	else if (x%2)
	{
		out=( Pic[yfloor  ][xfloor  ] +
			  Pic[yfloor  ][xfloor+1] + 1 ) / 2;
	}
	else if (y%2)
	{
		out=( Pic[yfloor  ][xfloor  ] +
		      Pic[yfloor+1][xfloor  ] + 1 ) / 2;
	}
	else
		out=  Pic[yfloor  ][xfloor  ];

	return(out);

}

pel_t	FastPelY_18 (pel_t **Pic, int y, int x) {
	
	byte out;
	int yfloor, xfloor;

	//int width8  = (img->width<<3) - 2;		// Width and Height of the 1/4 buffer, measure in 1/8 pel
	//int height8 = (img->height<<3) -2;		// the -2 is for the bound checking, since arrays start at 0

	xfloor=x>>1;
	yfloor=y>>1;

	if( x%2 && y%2 )
	{
		out=( Pic[yfloor  ][xfloor  ] +
		      Pic[yfloor+1][xfloor  ] +
			  Pic[yfloor  ][xfloor+1] +
			  Pic[yfloor+1][xfloor+1] + 2 ) / 4;
	}
	else if (x%2)
	{
		out=( Pic[yfloor  ][xfloor  ] +
			  Pic[yfloor  ][xfloor+1] + 1 ) / 2;
	}
	else if (y%2)
	{
		out=( Pic[yfloor  ][xfloor  ] +
		      Pic[yfloor+1][xfloor  ] + 1 ) / 2;
	}
	else
		out=  Pic[yfloor  ][xfloor  ];

	return(out);

}

void InitRefbuf () {
	int width  = img->width;
	int height = img->height;
	int num_frames = img->buf_cycle;
	int i;

	if (NULL == (Refbuf11_P = malloc ((width * height + 4711) * sizeof (pel_t))))
		perror ("InitRefbuf: cannot allocate memory");
	if (NULL == (Refbuf11 = malloc (num_frames * sizeof (pel_t *))))
		perror ("InitRefbuf, cannot allocate memory\n");
	for (i=0; i<num_frames; i++)
		if (NULL == (Refbuf11[i] = malloc ((width * height + 4711) * sizeof (pel_t))))
			perror ("InitRefbuf: cannot allocate memory");

}



/************************************************************************
*  Name :    copy2mref()
*
*  Description: Substitutes function oneforthpix_2. It should be worked 
*								out how this copy procedure can be avoided.
*
************************************************************************/
void copy2mref()
{
	int j, uv;

	img->frame_cycle=img->number % img->buf_cycle;  /*GH img->no_multpred used insteadof MAX_MULT_PRED
		                                                frame buffer size = img->no_multpred+1*/
//	printf ("Copy2MRef: copying mref-P to ref buffer %d\n", img->frame_cycle);
	/* Luma */
	for (j=0; j < img->height*4; j++)
		memcpy(mref[img->frame_cycle][j],mref_P[j], img->width*4);


	/*  Chroma: */
	for (uv=0; uv < 2; uv++)
		for (j=0; j < img->height_cr; j++)
				memcpy(mcef[img->frame_cycle][uv][j],mcef_P[uv][j],img->width_cr);

	// Full pel represnetation for MV search

	memcpy (Refbuf11[img->frame_cycle], Refbuf11_P, (img->width*img->height));
}






#endif


#ifndef HACK
// Alloc and free for reference buffers

refpic_t *AllocRefPic (int Id, 
					  int NumCols, 
					  int NumRows,
					  int MaxMotionVectorX,		// MV Size may be used to allocate additional
					  int MaxMotionVectorY) {	// memory around boundaries fro UMV search

	refpic_t *pic;
	int xs, ys;
	
	if (NULL == (pic = malloc (sizeof (refpic_t)))) {
		perror ("Malloc (sizeof (refpic_t)) in AllocRefPic, exiting\n");
	}
	
	if (NumCols %2 != 0)
		perror ("AllocRefPic: Number of columns must be divisible by two, exiting\n");
	if (NumRows %2 != 0)
		perror ("AllocRefPic: Number of rows must be divisible by two, exiting\n");

	pic->Id = Id;
	xs = NumCols + MaxMotionVectorX + MaxMotionVectorX;
	if (xs % (CACHELINESIZE/sizeof (pel_t)) != 0)
		xs = ((xs / (CACHELINESIZE/sizeof (pel_t)))+1) * (CACHELINESIZE/sizeof (pel_t));
	ys = NumRows + MaxMotionVectorY + MaxMotionVectorY;
	pic->x_ysize = xs * 4;
	pic->y_ysize = (NumRows + MaxMotionVectorY + MaxMotionVectorY) * 4;

	pic->x_yfirst	= MaxMotionVectorX * 4;
	pic->x_ylast	= (NumCols + MaxMotionVectorX) * 4;
	pic->y_yfirst	= MaxMotionVectorY * 4;
	pic->x_ylast	= (NumRows + MaxMotionVectorY) * 4;

	pic->x_uvsize	= xs/2;
	pic->y_uvsize	= ys/2;

	pic->x_uvfirst	= MaxMotionVectorX/2;
	pic->x_uvlast	= (NumCols + MaxMotionVectorX)/2;
	pic->y_uvfirst	= MaxMotionVectorY/2;
	pic->y_uvlast	= (NumRows + MaxMotionVectorY)/2;

	if (NULL == (pic->y = malloc (16 * sizeof (pel_t)* xs * ys)))
		perror ("AllocRefPic: cannot alloc memory for Y-plane, exiting");
	if (NULL == (pic->u = malloc (sizeof (pel_t) * (xs/2) * (ys/2))))
		perror ("AllocRefPic: cannot alloc memory for U-plane, exiting");
	if (NULL == (pic->v = malloc (sizeof (pel_t) * (xs/2) * (ys/2))))
		perror ("AllocRefPic: cannot alloc memory for V-plane, exiting");

	return pic;
}


int FreeRefPic (refpic_t *Pic) {
	if (Pic == NULL)
		return 0;
	if (Pic->y != NULL)
		free (Pic->y);
	if (Pic->u != NULL)
		free (Pic->u);
	if (Pic->v != NULL)
		free (Pic->v);
	free (Pic);
	return (0);
}






// Access functions for full pel (1/1 pel)

pel_t	PelY_11 (refpic_t *Pic, int x, int y) {
	register int pos;

	pos = x<<2+Pic->x_yfirst;		// Y Structures are 1/4 pel, hence *4
	pos += (Pic->x_ysize * (Pic->y_yfirst + y<<2));

	return Pic->y[pos];
}


pel_t	PelU_11 (refpic_t *Pic, int x, int y) {
	register int pos;

	pos = x+Pic->x_uvfirst;		// UV Structures are 1/1 pel
	pos += Pic->x_uvsize * (Pic->y_uvfirst + y);

	return Pic->u[pos];
}

pel_t	PelV_11 (refpic_t *Pic, int x, int y) {
	register int pos;

	pos = x+Pic->x_uvfirst;		// UV Structures are 1/1 pel
	pos += Pic->x_uvsize * (Pic->y_uvfirst + y);

	return Pic->v[pos];
}


pel_t	*MBLineY_11 (refpic_t *Pic, int x, int y) {
	perror ("MBLine_11: net yet implemented, exit");
}


// Access functions for half pel (1/2 pel)

pel_t	PelY_12 (refpic_t *Pic, int x, int y) {
	register int pos;
	
	pos = (x<<1+Pic->x_yfirst);		// Structures are 1/4 pel, hence *4
	pos += (Pic->x_ysize * (Pic->y_yfirst + y<<1));

	return (Pic->v[pos]);
};


// Access functions for quater pel (1/4 pel)

pel_t	PelY_14 (refpic_t *Pic, int x, int y) {
	return (Pic->y[ (Pic->y_yfirst+y)*Pic->x_ysize  + x + Pic->x_yfirst]);
}


// Access functions for one-eigths pel (1/8 pel)

pel_t	PelY_18 (refpic_t *Pic, int x, int y) {
	perror ("No 1/8th pel support yet");
}

#endif

