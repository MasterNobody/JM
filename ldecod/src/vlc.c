/************************************************************************
*
*  vlc.c for H.26L decoder. 
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
#include "vlc.h"

/************************************************************************
*
*  Routine      linfo_levrun_inter()
*
*  Input:       lenght and info
*
*  Output:      Level and run for inter (through pointers)
*                    
************************************************************************/
void linfo_levrun_inter(int len,int info,int *level,int *irun)
{         
  int l2;    
  int inf;
  if (len<=9)
  {
    l2=mmax(0,len/2-1);                  
    inf=info/2;
    *level=NTAB1[l2][inf][0];     
    *irun=NTAB1[l2][inf][1];
    if ((info&0x01)==1)
      *level=-*level;                   /* make sign */
  }
  else                                  /* if len > 9, skip using the array */
  {
    *irun=(info&0x1e)>>1;
    *level = LEVRUN1[*irun] + info/32 + (int)pow(2,len/2 - 5);
    if ((info&0x01)==1)
      *level=-*level;
  } 
}
/************************************************************************
*
*  Routine      linfo_levrun_intra()
*
*  Input:       lenght and info
*
*  Output:      Level and run for intra (through pointers)
*                    
************************************************************************/
void linfo_levrun_intra(int len,int info,int *level,int *irun)
{     
  int l2;    
  int inf;
  
  if (len<=9)
  {
    l2=mmax(0,len/2-1);                  
    inf=info/2;
    *level=NTAB2[l2][inf][0];     
    *irun=NTAB2[l2][inf][1];
    if ((info&0x01)==1)
      *level=-*level;                 /* make sign */
  }
  else                                  /* if len > 9, skip using the array */
  {
    *irun=(info&0x0e)>>1;
    *level = LEVRUN2[*irun] + info/16 + (int)pow(2,len/2-4) -1;
    if ((info&0x01)==1)
      *level=-*level;
  }
}

/************************************************************************
*
*  Routine      linfo_levrun__c2x2()
* 
*  Input:       lenght and info
*  
*  Output:      Level and run for 2x2 chroma (through pointers)
*                    
************************************************************************/
void linfo_levrun_c2x2(int len,int info,int *level,int *irun)
{ 
  int l2;    
  int inf;
  
  if (len<=5)
  {
    l2=mmax(0,len/2-1);                  
    inf=info/2;
    *level=NTAB3[l2][inf][0];     
    *irun=NTAB3[l2][inf][1];
    if ((info&0x01)==1)
      *level=-*level;                 /* make sign */
  }
  else                                  /* if len > 5, skip using the array */
  {
    *irun=(info&0x06)>>1;
    *level = LEVRUN3[*irun] + info/8 + (int)pow(2,len/2 - 3);
    if ((info&0x01)==1)
      *level=-*level;
  }
}

/************************************************************************
*
*  Routine      linfo_levrun__c2x2()
*
*  Description: Make signed value 
* 
*  Input:       lenght and info
*
*  Output:      signed motion vector
*                    
************************************************************************/
int linfo_mvd(int len,int info)                
{
  int n;
  int signed_mvd;
  
  n = (int)pow(2,(len/2))+info-1;       
  signed_mvd = (n+1)/2;
  if((n & 0x01)==0)                           /* lsb is signed bit */
    signed_mvd = -signed_mvd;
  
  return signed_mvd;
}