/************************************************************************
*
*  macroblock.h for H.26L encoder. 
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
#ifndef _MACROBLOCK_H_
#define _MACROBLOCK_H_


/* just to make new temp intra mode table */ 
const int  MODTAB[3][2]= 
{
  { 0, 4},
  {16,12},
  { 8,20}
};

/* gives codeword number from CBP value, both for intra and inter */
const int NCBP[48][2]=
{
  { 3, 0},{29, 2},{30, 3},{17, 7},{31, 4},{18, 8},{37,17},{ 8,13},{32, 5},{38,18},{19, 9},{ 9,14},
  {20,10},{10,15},{11,16},{ 2,11},{16, 1},{33,32},{34,33},{21,36},{35,34},{22,37},{39,44},{ 4,40},
  {36,35},{40,45},{23,38},{ 5,41},{24,39},{ 6,42},{ 7,43},{ 1,19},{41, 6},{42,24},{43,25},{25,20},
  {44,26},{26,21},{46,46},{12,28},{45,27},{47,47},{27,22},{13,29},{28,23},{14,30},{15,31},{ 0,12},
}; 

/*  
Return prob.(0-5) for the input intra prediction mode(0-5), depending on previous (right/above) pred mode(0-6).  
NA values are set to 0 in the array. 
The modes for the neighbour blocks are signalled:
0 = outside
1 = DC
2 = diagonal vert 22.5 deg
3 = vertical
4 = diagonal 45 deg
5 = horizontal
6 = diagonal horizontal -22.5 deg
*/
/* prob order=PRED_IPRED[B(block left)][A(block above)][intra mode] */
const byte PRED_IPRED[7][7][6]=
{
  {
    {0,0,0,0,0,0},
    {0,0,0,0,1,0},
    {0,0,0,0,0,0},
    {0,0,0,0,0,0},  
    {0,0,0,0,0,0},
    {1,0,0,0,0,0},
    {0,0,0,0,0,0},
  },
  {
    {0,0,1,0,0,0},
    {0,2,5,3,1,4},
    {0,1,4,3,2,5},
    {0,1,2,3,4,5},
    {1,3,5,0,2,4},
    {1,4,5,2,0,3},
    {2,4,5,3,1,0},
  },
  {
    {0,0,0,0,0,0},
    {1,0,4,3,2,5},
    {1,0,2,4,3,5},
    {1,0,2,3,4,5},
    {2,1,4,0,3,5},
    {1,2,5,4,0,3},
    {0,1,5,4,3,2},      
  },
  {        
    {1,0,0,0,0,0},
    {2,4,0,1,3,5},
    {1,3,0,2,4,5},
    {2,1,0,3,4,5},
    {3,2,0,1,5,4},
    {2,5,0,3,1,4},
    {1,2,0,5,3,4},    
  },
  {      
    {0,0,0,0,0,0},
    {1,4,3,0,2,5},
    {0,3,2,1,4,5},
    {1,3,2,0,4,5},
    {1,4,3,0,2,5},
    {2,4,5,1,0,3},
    {2,4,5,1,3,0},
  },
  {          
    {0,0,0,0,0,0},
    {0,3,5,2,1,4},
    {0,2,4,3,1,5},
    {0,3,2,4,1,5},
    {1,4,5,2,0,3},
    {1,4,5,2,0,3},
    {2,4,5,3,0,1},         
  },
  {
    {0,0,0,0,0,0},
    {0,4,5,2,1,3},
    {0,1,5,3,2,4},
    {0,1,3,2,4,5},
    {1,4,5,0,3,2},
    {1,4,5,3,0,2},
    {1,3,5,4,2,0},  
  }
};
/*
  return codeword number from two combined intra prediction blocks 
*/

const int IPRED_ORDER[6][6]= 
{
  { 0, 2, 3, 9,10,20},
  { 1, 4, 8,11,19,21},
  { 5, 7,12,18,22,29},
  { 6,13,17,23,28,30},
  {14,16,24,27,31,34},
  {15,25,26,32,33,35},
};
 
extern int JQ[32][2];
extern int QP2QUANT[32];

#endif