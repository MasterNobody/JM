/************************************************************************
*
*  vlc.h for H.26L encoder. 
*  Copyright (C) 1999  Telenor Satellite Services, Norway
*  
*  Contacts: 
*  Inge Lille-Langøy               <inge.lille-langoy@telenor.com>
* 
*
*  Telenor Satellite Services 
*  P.O.Box 6914 St.Olavs plass                       
*  N-0130 Oslo, Norway                 
*  
************************************************************************/

#include <math.h>
#include "global.h"
/************************************************************************
*
*  Routine      n_linfo
*
*  Description:   
*               Input: Number in the code table
*               Output: lenght and info
*                    
************************************************************************/
void n_linfo(int n,int *len,int *info)
{
  int i,nn;
  
  nn=(n+1)/2;
  
  for (i=0; i < 16 && nn != 0; i++)
  {
    nn /= 2;
  }
  *len= 2*i + 1;
  *info=n+1-(int)pow(2,i);
} 
/************************************************************************
*
*  Routine      mvd_linfo
*
*  Description:   
*               Input: motion vector differense
*               Output: lenght and info
*                    
************************************************************************/
void mvd_linfo(int mvd,int *len,int *info)
{  
  int i,n,sign,nn;
  
  sign=0;
  
  if (mvd <= 0) 
  {
    sign=1;
  }
  n=abs(mvd) << 1;
  
  /*  
  n+1 is the number in the code table.  Based on this we find length and info 
  */
  
  nn=n/2;
  for (i=0; i < 16 && nn != 0; i++)
  {
    nn /= 2;
  }
  *len=i*2 + 1;
  *info=n - (int)pow(2,i) + sign;
} 

/************************************************************************
*
*  Routine      levrun_linfo_c2x2
*
*  Description: 2x2 transform of chroma DC
*               Input: level and run for coefficiets
*               Output: lenght and info
*               see ITU document for bit assignment
*
************************************************************************/
void levrun_linfo_c2x2(int level,int run,int *len,int *info)
{   
  const int NTAB[2][2]=
  {
    {1,5},{3,0}
  };
  const int LEVRUN[4]=
  { 
    2,1,0,0 
  };
  
  int levabs,i,n,sign,nn;
  
  if (level == 0) /*  check if the coefficient sign EOB (level=0) */
  {
    *len=1;
    return;
  }
  sign=0;
  if (level <= 0) 
  {
    sign=1;
  }
  levabs=abs(level);
  if (levabs <= LEVRUN[run]) 
  {
    n=NTAB[levabs-1][run]+1;
  } 
  else 
  {
    n=(levabs-LEVRUN[run])*8 + run*2;
  }
  
  nn=n/2;
  
  for (i=0; i < 16 && nn != 0; i++)
  {
    nn /= 2;
  }
  *len= 2*i + 1;
  *info=n-(int)pow(2,i)+sign;
} 

/************************************************************************
*
*  Routine      levrun_linfo_inter
*
*  Description: Single scan coefficients
*               Input: level and run for coefficiets
*               Output: lenght and info
*               see ITU document for bit assignment
*
************************************************************************/
void levrun_linfo_inter(int level,int run,int *len,int *info)
{    
  const byte LEVRUN[16]=
  { 
    4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0 
  };
  const byte NTAB[4][10]=
  {
    { 1, 3, 5, 9,11,13,21,23,25,27},
    { 7,17,19, 0, 0, 0, 0, 0, 0, 0},
    {15, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {29, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  };
  
  int levabs,i,n,sign,nn;
  
  if (level == 0)           /*  check for EOB  */
  {
    *len=1;
    return;
  }
  
  if (level <= 0) 
    sign=1;
  else
    sign=0;
  
  levabs=abs(level);
  if (levabs <= LEVRUN[run]) 
  {
    n=NTAB[levabs-1][run]+1;
  } 
  else 
  {
    n=(levabs-LEVRUN[run])*32 + run*2;
  }
  
  nn=n/2;
  
  for (i=0; i < 16 && nn != 0; i++)
  {
    nn /= 2;
  }
  *len= 2*i + 1;
  *info=n-(int)pow(2,i)+sign;
  
} 

/************************************************************************
*
*  Routine      levrun_linfo_intra
*
*  Description: Double scan coefficients
*               Input: level and run for coefficiets
*               Output: lenght and info
*               see ITU document for bit assignment
*
************************************************************************/
void levrun_linfo_intra(int level,int run,int *len,int *info)
{
  const byte LEVRUN[8]=
  { 
    9,3,1,1,1,0,0,0 
  };
  
  const byte NTAB[9][5] =
  {
    { 1, 3, 7,15,17},
    { 5,19, 0, 0, 0},
    { 9,21, 0, 0, 0},
    {11, 0, 0, 0, 0},
    {13, 0, 0, 0, 0},
    {23, 0, 0, 0, 0},
    {25, 0, 0, 0, 0},
    {27, 0, 0, 0, 0},
    {29, 0, 0, 0, 0},
  };
  
  int levabs,i,n,sign,nn;
  
  if (level == 0)     /*  check for EOB */
  {
    *len=1;
    return;
  }
  if (level <= 0) 
    sign=1;
  else
    sign=0;
   
  levabs=abs(level);
  if (levabs <= LEVRUN[run]) 
  {
    n=NTAB[levabs-1][run]+1;
  } 
  else 
  {
    n=(levabs-LEVRUN[run])*16 + 16 + run*2;
  }
  
  nn=n/2;
  
  for (i=0; i < 16 && nn != 0; i++)
  {
    nn /= 2;
  }
  *len= 2*i + 1;
  *info=n-(int)pow(2,i)+sign; 
}