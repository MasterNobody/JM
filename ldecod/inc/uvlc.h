/************************************************************************
*
*  UVLC.h for H.26L decoder. 
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

#ifndef _UVLC_H
#define _UVLC_H

/* Note that all NA values are filled with 0 */

/* for the linfo_levrun_inter routine */
const byte NTAB1[4][8][2] =                                
{
  {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,1},{1,2},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{2,0},{1,3},{1,4},{1,5},{0,0},{0,0},{0,0},{0,0}},
  {{3,0},{2,1},{2,2},{1,6},{1,7},{1,8},{1,9},{4,0}},
};
const byte LEVRUN1[16]=
{
  4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0,
};

/* for the linfo_levrun_intra routine */
const byte LEVRUN2[8]=                                      
{
  9,3,1,1,1,0,0,0,
};
const byte NTAB2[4][8][2] = 
{
  {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,1},{2,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,2},{3,0},{4,0},{5,0},{0,0},{0,0},{0,0},{0,0}},
  {{1,3},{1,4},{2,1},{3,1},{6,0},{7,0},{8,0},{9,0}},
};

/* for the linfo_levrun__c2x2 routine */
const byte LEVRUN3[4] =                                    
{
  2,1,0,0
};
const byte NTAB3[2][2][2] = 
{
  {{1,0},{0,0}},
  {{2,0},{1,1}},
};


#endif
