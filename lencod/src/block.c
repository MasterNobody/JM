/************************************************************************
*
*  block.c for H.26L encoder. 
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
#include <stdlib.h> 
#include <stdio.h>

#include "global.h"
#include "block.h" 


/************************************************************************
*
*  Routine    : intrapred_luma()
*
*  Description: Make intra 4x4 prediction according to all 6 prediction modes.
*               The routine uses left and upper neighbouring points from 
*               previous coded blocks to do this (if available). Notice that
*               inaccessible neighbouring points are signalled with a negative 
*               value i the predmode array .     
*
*  Input      : Starting point of current 4x4 block image posision
*
*  Output     : none
*                    
************************************************************************/
void intrapred_luma(struct img_par *img,int img_x,int img_y)
{
  int i,j,s0,s1,s2,ia[7][3],s[4][2];
  int img_block_x,img_block_y;        /* indexes to the prediction mode array, 
                                        addressing mode for one 4x4 intra block */
  int l,l2;

  
  img_block_x=img_x/BLOCK_SIZE;
  img_block_y=img_y/BLOCK_SIZE;
  s1=0;
  s2=0;
  
  /* make DC prediction */
  for (i=0; i < BLOCK_SIZE; i++) 
  {
    if (img->ipredmode[img_block_x+1][img_block_y] >= 0) 
      s1 += imgY[img_y-1][img_x+i];    /* sum hor pix */ 
    if (img->ipredmode[img_block_x][img_block_y+1] >= 0) 
      s2 += imgY[img_y+i][img_x-1];    /* sum vert pix */ 
  }
  if (img->ipredmode[img_block_x+1][img_block_y] >= 0 && img->ipredmode[img_block_x][img_block_y+1] >= 0) 
    s0=(s1+s2+4)/(2*BLOCK_SIZE);     /* no edge */
  if (img->ipredmode[img_block_x+1][img_block_y]< 0 && img->ipredmode[img_block_x][img_block_y+1] >= 0) 
    s0=(s2+2)/BLOCK_SIZE;             /* upper edge */
  if (img->ipredmode[img_block_x+1][img_block_y] >= 0 && img->ipredmode[img_block_x][img_block_y+1] < 0) 
    s0=(s1+2)/BLOCK_SIZE;             /* left edge */
  if (img->ipredmode[img_block_x+1][img_block_y] < 0 && img->ipredmode[img_block_x][img_block_y+1] < 0) 
    s0=128;                            /* top left corner, nothing to predict from */
  
  for (i=0; i < BLOCK_SIZE; i++) 
  {
    /* vertical prediction */
    if (img->ipredmode[img_block_x+1][img_block_y] >= 0) 
      s[i][0]=imgY[img_y-1][img_x+i];
    /* horizontal prediction */
    if (img->ipredmode[img_block_x][img_block_y+1] >= 0) 
      s[i][1]=imgY[img_y+i][img_x-1];
  }
  
  for (j=0; j < BLOCK_SIZE; j++) 
  {
    for (i=0; i < BLOCK_SIZE; i++) 
    {
      img->mprr[DC_PRED][i][j]=s0;      /* store DC prediction */                  
      img->mprr[VERT_PRED][i][j]=s[j][0]; /* store vertical prediction */
      img->mprr[HOR_PRED][i][j]=s[i][1]; /* store horizontal prediction */
    }
  }
  
  /*  Prediction according to 'diagonal' modes */    
  if (img->ipredmode[img_block_x+1][img_block_y] >= 0 && img->ipredmode[img_block_x][img_block_y+1] >= 0) 
  {
    ia[0][0]=(imgY[img_y+3][img_x-1]+2*imgY[img_y+2][img_x-1]+imgY[img_y+1][img_x-1]+2)/4;
    ia[1][0]=(imgY[img_y+2][img_x-1]+2*imgY[img_y+1][img_x-1]+imgY[img_y+0][img_x-1]+2)/4;
    ia[2][0]=(imgY[img_y+1][img_x-1]+2*imgY[img_y+0][img_x-1]+imgY[img_y-1][img_x-1]+2)/4;
    ia[3][0]=(imgY[img_y+0][img_x-1]+2*imgY[img_y-1][img_x-1]+imgY[img_y-1][img_x+0]+2)/4;
    ia[4][0]=(imgY[img_y-1][img_x-1]+2*imgY[img_y-1][img_x+0]+imgY[img_y-1][img_x+1]+2)/4;
    ia[5][0]=(imgY[img_y-1][img_x+0]+2*imgY[img_y-1][img_x+1]+imgY[img_y-1][img_x+2]+2)/4;
    ia[6][0]=(imgY[img_y-1][img_x+1]+2*imgY[img_y-1][img_x+2]+imgY[img_y-1][img_x+3]+2)/4;
    
    for (l=0;l<3;l++)
    {
      l2=l*2;
      ia[l2  ][1]=(imgY[img_y-1  ][img_x+l  ]+imgY[img_y-1][img_x+l+1])/2;
      ia[l2+1][1]= imgY[img_y-1  ][img_x+l+1];
      ia[l2  ][2]=(imgY[img_y+l  ][img_x-1  ]+imgY[img_y+l+1][img_x-1])/2;
      ia[l2+1][2]= imgY[img_y+l+1][img_x-1  ];
    }
   
    
    for (i=0;i<4;i++)
    {
      for (j=0;j<4;j++)
      {
        img->mprr[DIAG_PRED_LR_45][i][j]=ia[j-i+3][0];
        img->mprr[DIAG_PRED_RL   ][i][j]=ia[MAP[i][j]][1];
        img->mprr[DIAG_PRED_LR   ][i][j]=ia[MAP[j][i]][2];
      }
    } 
    
  }
}

/************************************************************************
*
*  Routine    : intrapred_luma_2()
*
*  Description: 16x16 based luma prediction
*
*  Input      : Image parameters
*
*  Output     : none
*                    
************************************************************************/
void intrapred_luma_2(struct img_par *img)
{
  int s0,s1,s2;
  int i,j;
  int s[16][2];
  
  int ih,iv;
  int ib,ic,iaa;
  
  s1=s2=0;
  /* make DC prediction */
  for (i=0; i < MB_BLOCK_SIZE; i++) 
  {
    if (img->pix_y > 0)                          /* hor edge?       */
      s1 += imgY[img->pix_y-1][img->pix_x+i];    /* sum hor pix     */ 
    if (img->pix_x > 0)                          /* vert edge?      */
      s2 += imgY[img->pix_y+i][img->pix_x-1];    /* sum vert pix    */ 
  }
  if (img->pix_y > 0 && img->pix_x > 0) 
    s0=(s1+s2+16)/(2*MB_BLOCK_SIZE);             /* no edge         */
  if (img->pix_y == 0 && img->pix_x > 0) 
    s0=(s2+8)/MB_BLOCK_SIZE;                     /* upper edge      */
  if (img->pix_y > 0 && img->pix_x == 0) 
    s0=(s1+8)/MB_BLOCK_SIZE;                     /* left edge       */
  if (img->pix_y == 0 && img->pix_x == 0) 
    s0=128;                                      /* top left corner, nothing to predict from */
  
  for (i=0; i < MB_BLOCK_SIZE; i++) 
  {
    /* vertical prediction */
    if (img->pix_y > 0) 
      s[i][0]=imgY[img->pix_y-1][img->pix_x+i];
    /* horizontal prediction */
    if (img->pix_x > 0) 
      s[i][1]=imgY[img->pix_y+i][img->pix_x-1];
  }
  
  for (j=0; j < MB_BLOCK_SIZE; j++) 
  {
    for (i=0; i < MB_BLOCK_SIZE; i++) 
    {                     
      img->mprr_2[VERT_PRED_16][j][i]=s[i][0]; /* store vertical prediction */
      img->mprr_2[HOR_PRED_16 ][j][i]=s[j][1]; /* store horizontal prediction */
      img->mprr_2[DC_PRED_16  ][j][i]=s0;      /* store DC prediction */   
    }
  }
  if (img->pix_y == 0 || img->pix_x == 0)      /* edge */
    return;
  
  /* 16 bit integer plan pred */
  
  ih=0;
  iv=0;
  for (i=1;i<9;i++)
  {
    ih += i*(imgY[img->pix_y-1][img->pix_x+7+i] - imgY[img->pix_y-1][img->pix_x+7-i]);
    iv += i*(imgY[img->pix_y+7+i][img->pix_x-1] - imgY[img->pix_y+7-i][img->pix_x-1]);
  }
  ib=5*(ih/4)/16;
  ic=5*(iv/4)/16;
  
  iaa=16*(imgY[img->pix_y-1][img->pix_x+15]+imgY[img->pix_y+15][img->pix_x-1]);
  for (j=0;j< MB_BLOCK_SIZE;j++)
  {
    for (i=0;i< MB_BLOCK_SIZE;i++)
    {
      img->mprr_2[PLANE_16][j][i]=(iaa+(i-7)*ib +(j-7)*ic + 16)/32;/* store plane prediction */
    }
  }
}

/************************************************************************
*
*  Routine    : find_sad2()
*
*  Description: Find best 16x16 based intra mode
*
*
*  Input      : Image parameters, pointer to best 16x16 intra mode
*
*  Output     : best 16x16 based SAD
*                    
************************************************************************/
int find_sad2(struct img_par *img,int *intra_mode)
{
  int current_intra_sad_2,best_intra_sad2;
  int M1[16][16],M0[4][4][4][4],M3[4],M4[4][4];
  
  int i,j,k;
  int ii,jj;
  
  best_intra_sad2=MAX_VALUE;
  
  for (k=0;k<4;k++)
  {
    /*check if there are neighbours to predict from */
    if ((k==0 && img->pix_y == 0) || (k==1 && img->pix_x == 0) || (k==3 && (img->pix_x==0 || img->pix_y==0))) 
    {
      ; /* edge, do nothing */
    }
    else
    {
      for (j=0;j<16;j++)
      {
        for (i=0;i<16;i++)
        {
          M1[i][j]=imgY_org[img->pix_y+j][img->pix_x+i]-img->mprr_2[k][j][i];
          M0[i%4][i/4][j%4][j/4]=M1[i][j];
        }
      }
      current_intra_sad_2=0;              /* no SAD start handicap here */
      for (jj=0;jj<4;jj++)
      {
        for (ii=0;ii<4;ii++)
        {
          for (j=0;j<4;j++)
          {
            M3[0]=M0[0][ii][j][jj]+M0[3][ii][j][jj];
            M3[1]=M0[1][ii][j][jj]+M0[2][ii][j][jj];
            M3[2]=M0[1][ii][j][jj]-M0[2][ii][j][jj];
            M3[3]=M0[0][ii][j][jj]-M0[3][ii][j][jj];
            
            M0[0][ii][j][jj]=M3[0]+M3[1];
            M0[2][ii][j][jj]=M3[0]-M3[1];
            M0[1][ii][j][jj]=M3[2]+M3[3];
            M0[3][ii][j][jj]=M3[3]-M3[2];
          }
          
          for (i=0;i<4;i++)
          {
            M3[0]=M0[i][ii][0][jj]+M0[i][ii][3][jj];
            M3[1]=M0[i][ii][1][jj]+M0[i][ii][2][jj];
            M3[2]=M0[i][ii][1][jj]-M0[i][ii][2][jj];
            M3[3]=M0[i][ii][0][jj]-M0[i][ii][3][jj];
            
            M0[i][ii][0][jj]=M3[0]+M3[1];
            M0[i][ii][2][jj]=M3[0]-M3[1];
            M0[i][ii][1][jj]=M3[2]+M3[3];
            M0[i][ii][3][jj]=M3[3]-M3[2];
            for (j=0;j<4;j++)
              if ((i+j)!=0)
                current_intra_sad_2 += abs(M0[i][ii][j][jj]);
          }
        }
      }
      
      for (j=0;j<4;j++)
        for (i=0;i<4;i++)
          M4[i][j]=M0[0][i][0][j]/4;
        
        /* Hadamard of DC koeff */
        for (j=0;j<4;j++)
        {
          M3[0]=M4[0][j]+M4[3][j];
          M3[1]=M4[1][j]+M4[2][j];
          M3[2]=M4[1][j]-M4[2][j];
          M3[3]=M4[0][j]-M4[3][j];
          
          M4[0][j]=M3[0]+M3[1];
          M4[2][j]=M3[0]-M3[1];
          M4[1][j]=M3[2]+M3[3];
          M4[3][j]=M3[3]-M3[2];
        }
        
        for (i=0;i<4;i++)
        {
          M3[0]=M4[i][0]+M4[i][3];
          M3[1]=M4[i][1]+M4[i][2];
          M3[2]=M4[i][1]-M4[i][2];
          M3[3]=M4[i][0]-M4[i][3];
          
          M4[i][0]=M3[0]+M3[1];
          M4[i][2]=M3[0]-M3[1];
          M4[i][1]=M3[2]+M3[3];
          M4[i][3]=M3[3]-M3[2];
          
          for (j=0;j<4;j++)
            current_intra_sad_2 += abs(M4[i][j]);
        }
        if(current_intra_sad_2 < best_intra_sad2)
        {
          best_intra_sad2=current_intra_sad_2;
          *intra_mode = k; /* update best intra mode */
          
        }
    }
  }
  best_intra_sad2 = best_intra_sad2/2;
  
  return best_intra_sad2;   
  
}
/************************************************************************
*
*  Routine:     dct_luma2()
*
*  Description: For new intra pred routines
*               
*               
*  Input:       Image par, 16x16 based intra mode 
*
*  Output:      
*                    
************************************************************************/
void dct_luma2(struct img_par *img,int new_intra_mode)
{
  int jq0;
  int i,j;
  int ii,jj;
  int i1,j1;
  
  int M1[16][16];
  int M4[4][4];
  int M5[4],M6[4];
  int M0[4][4][4][4];
  int coeff[16];
  
  int quant_set,run,scan_pos,coeff_ctr,level;
  
  jq0=JQQ3;
  
  for (j=0;j<16;j++)
  {
    for (i=0;i<16;i++)
    {
      M1[i][j]=imgY_org[img->pix_y+j][img->pix_x+i]-img->mprr_2[new_intra_mode][j][i];
      M0[i%4][i/4][j%4][j/4]=M1[i][j];
    }
  }
  
  for (jj=0;jj<4;jj++)
  {
    for (ii=0;ii<4;ii++)
    {
      for (j=0;j<4;j++)
      {
        for (i=0;i<2;i++)
        {
          i1=3-i;
          M5[i]=  M0[i][ii][j][jj]+M0[i1][ii][j][jj];
          M5[i1]= M0[i][ii][j][jj]-M0[i1][ii][j][jj];
        }
        M0[0][ii][j][jj]=(M5[0]+M5[1])*13;
        M0[2][ii][j][jj]=(M5[0]-M5[1])*13;
        M0[1][ii][j][jj]=M5[3]*17+M5[2]*7;
        M0[3][ii][j][jj]=M5[3]*7-M5[2]*17;
      }
      /* vertical */
      for (i=0;i<4;i++)
      {
        for (j=0;j<2;j++)
        {
          j1=3-j;
          M5[j] = M0[i][ii][j][jj]+M0[i][ii][j1][jj];
          M5[j1]= M0[i][ii][j][jj]-M0[i][ii][j1][jj];
        }
        M0[i][ii][0][jj]=(M5[0]+M5[1])*13;
        M0[i][ii][2][jj]=(M5[0]-M5[1])*13;
        M0[i][ii][1][jj]= M5[3]*17+M5[2]*7;
        M0[i][ii][3][jj]= M5[3]*7 -M5[2]*17;
      }
    }
  }
  
  /* pick out DC coeff */
  
  for (j=0;j<4;j++)
    for (i=0;i<4;i++)
      M4[i][j]= 49 * M0[0][i][0][j]/32768;

    for (j=0;j<4;j++)
    {
      for (i=0;i<2;i++)
      {
        i1=3-i;
        M5[i]= M4[i][j]+M4[i1][j];
        M5[i1]=M4[i][j]-M4[i1][j];
      }
      M4[0][j]=(M5[0]+M5[1])*13;
      M4[2][j]=(M5[0]-M5[1])*13;
      M4[1][j]= M5[3]*17+M5[2]*7;
      M4[3][j]= M5[3]*7 -M5[2]*17;
    }
    
    /* vertical */
    
    for (i=0;i<4;i++)
    {
      for (j=0;j<2;j++)
      {
        j1=3-j;
        M5[j]= M4[i][j]+M4[i][j1];
        M5[j1]=M4[i][j]-M4[i][j1];
      }
      M4[i][0]=(M5[0]+M5[1])*13;
      M4[i][2]=(M5[0]-M5[1])*13;
      M4[i][1]= M5[3]*17+M5[2]*7;
      M4[i][3]= M5[3]*7 -M5[2]*17;
    }

    /* quant , we use RD quant here as well*/
    
    quant_set=img->qp;
    run=-1;
    scan_pos=0;
    for (coeff_ctr=0;coeff_ctr<16;coeff_ctr++)
    {
      i=SNGL_SCAN[coeff_ctr][0];
      j=SNGL_SCAN[coeff_ctr][1];
      coeff[coeff_ctr]=M4[i][j];
    }
    rd_quant(img,QUANT_LUMA_SNG,coeff);

    for (coeff_ctr=0;coeff_ctr<16;coeff_ctr++)
    {
      i=SNGL_SCAN[coeff_ctr][0];
      j=SNGL_SCAN[coeff_ctr][1];
      
      run++;
     
      level=abs(coeff[coeff_ctr]);

      if (level != 0)
      {
        img->cof[0][0][scan_pos][0][1]=sign(level,M4[i][j]);
        img->cof[0][0][scan_pos][1][1]=run;
        ++scan_pos;
        run=-1;
      }
      M4[i][j]=sign(level,M4[i][j]);
    }
    img->cof[0][0][scan_pos][0][1]=0;
    
    /* invers DC transform */
    
    for (j=0;j<4;j++)
    {
      for (i=0;i<4;i++)
        M5[i]=M4[i][j];
      
      M6[0]=(M5[0]+M5[2])*13;
      M6[1]=(M5[0]-M5[2])*13;
      M6[2]= M5[1]*7 -M5[3]*17;
      M6[3]= M5[1]*17+M5[3]*7;
      
      for (i=0;i<2;i++)
      {
        i1=3-i;
        M4[i][j]= M6[i]+M6[i1];
        M4[i1][j]=M6[i]-M6[i1];
      }
    }
    
    for (i=0;i<4;i++)
    {
      for (j=0;j<4;j++)
        M5[j]=M4[i][j];
      
      M6[0]=(M5[0]+M5[2])*13;
      M6[1]=(M5[0]-M5[2])*13;
      M6[2]= M5[1]*7 -M5[3]*17;
      M6[3]= M5[1]*17+M5[3]*7;
      
      for (j=0;j<2;j++)
      {
        j1=3-j;
        M0[0][i][0][j] = ((M6[j]+M6[j1])/8) *JQ[quant_set][1];
        M0[0][i][0][j1]= ((M6[j]-M6[j1])/8) *JQ[quant_set][1];
      } 
    }
    for (j=0;j<4;j++) 
    {
      for (i=0;i<4;i++)
      {
        M0[0][i][0][j] = 3 * M0[0][i][0][j]/256;
      }
    }
    
    /* AC invers trans/quant for MB  */
    img->kac=0;
    for (jj=0;jj<4;jj++)
    {
      for (ii=0;ii<4;ii++)
      {
        run=-1;
        scan_pos=0;
        for (coeff_ctr=1;coeff_ctr<16;coeff_ctr++) /* set in AC coeff */
        {
          i=SNGL_SCAN[coeff_ctr][0];
          j=SNGL_SCAN[coeff_ctr][1];
          coeff[coeff_ctr-1]=M0[i][ii][j][jj];
        }
        rd_quant(img,QUANT_LUMA_AC,coeff);

        for (coeff_ctr=1;coeff_ctr<16;coeff_ctr++) /* set in AC coeff */
        {
          i=SNGL_SCAN[coeff_ctr][0];
          j=SNGL_SCAN[coeff_ctr][1];
          run++;
          
          level=abs(coeff[coeff_ctr-1]);

          if (level != 0)
          {
            img->kac=1;
            img->cof[ii][jj][scan_pos][0][0]=sign(level,M0[i][ii][j][jj]);
            img->cof[ii][jj][scan_pos][1][0]=run;
            ++scan_pos;
            run=-1;
          }
          M0[i][ii][j][jj]=sign(level*JQ[quant_set][1],M0[i][ii][j][jj]);
        }
        img->cof[ii][jj][scan_pos][0][0]=0;
        
        /* IDCT horizontal */
        
        for (j=0;j<4;j++)
        {
          for (i=0;i<4;i++)
          {
            M5[i]=M0[i][ii][j][jj];
          }
          
          M6[0]=(M5[0]+M5[2])*13;
          M6[1]=(M5[0]-M5[2])*13;
          M6[2]=M5[1]*7 -M5[3]*17;
          M6[3]=M5[1]*17+M5[3]*7;
          
          for (i=0;i<2;i++)
          {
            i1=3-i;
            M0[i][ii][j][jj] =M6[i]+M6[i1];
            M0[i1][ii][j][jj]=M6[i]-M6[i1];
          }
        }
        
        /* vert */
        for (i=0;i<4;i++)
        {
          for (j=0;j<4;j++)
            M5[j]=M0[i][ii][j][jj];
          
          M6[0]=(M5[0]+M5[2])*13;
          M6[1]=(M5[0]-M5[2])*13;
          M6[2]=M5[1]*7 -M5[3]*17;
          M6[3]=M5[1]*17+M5[3]*7;
          
          for (j=0;j<2;j++)
          {
            j1=3-j;
            M0[i][ii][ j][jj]=M6[j]+M6[j1];
            M0[i][ii][j1][jj]=M6[j]-M6[j1];
            
          }
        }
        
      }
    }
    
    for (j=0;j<16;j++)
    {
      for (i=0;i<16;i++)
      {
        M1[i][j]=M0[i%4][i/4][j%4][j/4];     
      }
    }
    
    for (j=0;j<16;j++)
      for (i=0;i<16;i++)
        imgY[img->pix_y+j][img->pix_x+i]=min(255,max(0,(M1[i][j]+img->mprr_2[new_intra_mode][j][i]*JQQ1+JQQ2)/JQQ1));
      
}




/************************************************************************
*
*  Routine:     intrapred_chroma()
*
*  Description: Intra prediction for chroma.  There is only one prediction mode, 
*               corresponding to 'DC prediction' for luma. However,since 2x2 transform    
*               of DC levels are used,all predictions are made from neighbouring MBs.    
*               Prediction also depends on whether the block is at a frame edge.      
*               
*  Input:       Starting point of current chroma macro block image posision
*
*  Output:      8x8 array with DC intra chroma prediction and diff array   
*                    
************************************************************************/

void intrapred_chroma(struct img_par *img,int img_c_x,int img_c_y,int uv)
{
  int s[2][2],s0,s1,s2,s3;
  int i,j;
  int img_block_c_x,img_block_c_y;    
  
  img_block_c_x=img_c_x/BLOCK_SIZE;
  img_block_c_y=img_c_y/BLOCK_SIZE;
  
  s0=s1=s2=s3=0;          /* reset counters */
  
  for (i=0; i < BLOCK_SIZE; i++) 
  {
    if (img_block_c_y > 0)  /* no vertical frame edge */
    {
      s0 += imgUV[uv][img_c_y-1][img_c_x+i];
      s1 += imgUV[uv][img_c_y-1][img_c_x+i+BLOCK_SIZE];
    }
    if (img_block_c_x > 0)  /* no horizontal frame edge */
    {
      s2 += imgUV[uv][img_c_y+i][img_c_x-1];
      s3 += imgUV[uv][img_c_y+i+BLOCK_SIZE][img_c_x-1];
    }
  }
  
  if (img_block_c_x > 0 && img_block_c_y > 0)         /* free macro block */
  {
    s[0][0]=(s0+s2+4)/(2*BLOCK_SIZE);
    s[1][0]=(s1+2)/BLOCK_SIZE;
    s[0][1]=(s3+2)/BLOCK_SIZE;
    s[1][1]=(s1+s3+4)/(2*BLOCK_SIZE);
  }
  else if (img_block_c_x == 0 && img_block_c_y > 0)   /* left edge */
  {
    s[0][0]=(s0+2)/BLOCK_SIZE;
    s[1][0]=(s1+2)/BLOCK_SIZE;
    s[0][1]=(s0+2)/BLOCK_SIZE;
    s[1][1]=(s1+2)/BLOCK_SIZE;
  }
  else if (img_block_c_x > 0 && img_block_c_y == 0)   /* upper edge */
  {
    s[0][0]=(s2+2)/BLOCK_SIZE;
    s[1][0]=(s2+2)/BLOCK_SIZE;
    s[0][1]=(s3+2)/BLOCK_SIZE;
    s[1][1]=(s3+2)/BLOCK_SIZE;
  }
  else if (img_block_c_x == 0 && img_block_c_y == 0) /* top left corner */
  {
    s[0][0]=128;
    s[1][0]=128;
    s[0][1]=128;
    s[1][1]=128;
  }
  for (j=0; j < MB_BLOCK_SIZE/2; j++) 
  {
    for (i=0; i < MB_BLOCK_SIZE/2; i++) 
    {
      img->mpr[i][j]=s[i/BLOCK_SIZE][j/BLOCK_SIZE];
      img->m7[i][j]=imgUV_org[uv][img_c_y+j][img_c_x+i]-img->mpr[i][j];
    }
  }    
}

/************************************************************************
*
*  Routine:     motion_search()
*
*  Description: In this routine motion search (integer pel+1/3 pel) and mode selection 
*               is performed. Since we treat all 4x4 blocks before coding/decoding the 
*               prediction may not be based on decoded pixels (except for some of the blocks). 
*               This will result in too good prediction.  To compensate for this the SAD for 
*               intra(tot_intra_sad) is given a 'handicap' depending on QP.
*
*  Input:       Best intra SAD value.
*
*  Output:      Reference image.  
*
************************************************************************/
int motion_search(struct inp_par *inp_par,struct img_par *img,int tot_intra_sad)
{
  int predframe_no;
  int numc,j,ref_frame,i;
  int vec0_x,vec0_y,vec1_x,vec1_y,vec2_x,vec2_y;
  int pic_block_x,pic_block_y,block_x,ref_inx,block_y,pic_pix_y,pic4_pix_y,pic_pix_x,pic4_pix_x,i22,lv,hv;
  int all_mv[4][4][5][9][2]; 
  int center_x,s_pos_x,ip0,ip1,ip2,center_y,blocktype,s_pos_y,iy0,jy0,center_h2,curr_search_range,center_v2;
  int s_pos_x1,s_pos_y1,s_pos_x2,s_pos_y2;
  int abort_search;
  int mb_y,mb_x;
  int min_inter_sad,tot_inter_sad,best_inter_sad,current_inter_sad;
  int mo[16][16];
  int blockshape_x,blockshape_y;
  int tmp0,tmp1;
  
  
  min_inter_sad=MAX_VALUE; /* start value for inter SAD search */
  
  /*  Loop through all reference frames under consideration */  
  for (ref_frame=0; ref_frame < min(img->number,inp_par->no_multpred); ref_frame++) 
  {
    /* find  reference image */
    ref_inx=(img->number-ref_frame-1)%MAX_MULT_PRED; 
    
    /*  Looping through all the chosen block sizes: */        
    blocktype=1;      
    while (inp_par->blc_size[blocktype][0]==0 && blocktype<=7) /* skip blocksizes not chosen */
      blocktype++;
    
    for (;blocktype <= 7;) 
    {   
      blockshape_x=inp_par->blc_size[blocktype][0];/* inp_par->blc_size has information of the 7 blocksizes */
      blockshape_y=inp_par->blc_size[blocktype][1];/* array iz stores offset inside one MB */           
      
                                                   /*  curr_search_range is the actual search range used depending on block size and reference frame.  
                                                   It may be reduced by 1/2 or 1/4 for smaller blocks and prediction from older frames due to compexity
      */
      curr_search_range=inp_par->search_range/((min(ref_frame,1)+1) * min(2,blocktype));

      tot_inter_sad=QP2QUANT[img->qp] * min(ref_frame,1) * 2; /* start 'handicap ' */

      /*  Loop through the hole MB with all block sizes  */             
      for (mb_y=0; mb_y < MB_BLOCK_SIZE; mb_y += blockshape_y) 
      {
        block_y=mb_y/BLOCK_SIZE;
        pic_block_y=img->block_y+block_y;
        pic_pix_y=img->pix_y+mb_y;
        pic4_pix_y=pic_pix_y*4;
        
        for (mb_x=0; mb_x < MB_BLOCK_SIZE; mb_x += blockshape_x) 
        {
          block_x=mb_x/BLOCK_SIZE;
          pic_block_x=img->block_x+block_x;
          pic_pix_x=img->pix_x+mb_x;
          pic4_pix_x=pic_pix_x*4;
          
          /*  Prediction of vectors. vec0, 1 and 2 are the coordinates of vectors to be used for prediction.  
          As in H.263 the prediction is the median of three vectors.  
          Generally the coordinates of the three vectors are:
          */
          
          vec0_x=pic_block_x-1;
          vec0_y=pic_block_y;
          vec1_x=pic_block_x;
          vec1_y=pic_block_y-1;
          vec2_x=pic_block_x+blockshape_x/4;
          vec2_y=pic_block_y-1;
          
          /*  Corrections if we are at the upper edge of the frame (see similar correction in H.263) */
          
          if (pic_block_y == 0) 
          {
            vec1_x=vec0_x;
            vec2_x=vec0_x;
            vec1_y=vec0_y;
            vec2_y=vec0_y;
          }
          
          /*  Correction if a vector to the right is not yet available or to the right of the frame */       
          
          if (mb_x == MB_BLOCK_SIZE-blockshape_x && mb_y != 0 || vec2_x > img->width_cr/2-1) 
          {
            vec2_x=pic_block_x-1;
          }
          
          /*  Prediction by calculating the median */
          
          for (hv=0; hv < 2; hv++) 
          {
            ip0=tmp_mv[hv][vec0_y][vec0_x+4];
            ip1=tmp_mv[hv][vec1_y][vec1_x+4];
            ip2=tmp_mv[hv][vec2_y][vec2_x+4];
            img->mv[block_x][block_y][ref_frame][blocktype][hv]=ip0+ip1+ip2-min(min(ip0,ip1),ip2)-max(max(ip0,ip1),ip2);
          }
          ip0=img->mv[block_x][block_y][ref_frame][blocktype][0];
          ip1=img->mv[block_x][block_y][ref_frame][blocktype][1];
          
          /*  Integer pel search.  center_x,center_y is the 'search center'*/
          
          center_x=ip0/4;
          center_y=ip1/4;
          

          /*  Limitation of center_x,center_y so that the search wlevel ow contains the (0,0) vector*/
          center_x=max(-(inp_par->search_range),min(inp_par->search_range,center_x));
          center_y=max(-(inp_par->search_range),min(inp_par->search_range,center_y));
          
          /*  Search center corrected to prevent vectors outside the frame, this is not permitted in this model. */
          
          center_x=max(min(center_x,img->width-1-pic_pix_x-(blockshape_x-1)-curr_search_range),curr_search_range-pic_pix_x);
          center_y=max(min(center_y,img->height_err-pic_pix_y-(blockshape_y-1)-curr_search_range),curr_search_range-pic_pix_y);
          
          /*  mo() is the the original block to be used in the search process */
          
          for (i=0; i < blockshape_x; i++) 
            for (j=0; j < blockshape_y; j++) 
              mo[i][j]=imgY_org[pic_pix_y+j][pic_pix_x+i];
            
            best_inter_sad=MAX_VALUE;
            
            numc=(curr_search_range*2+1)*(curr_search_range*2+1);
            
            for (lv=0; lv < numc; lv++) 
            {
              s_pos_x=(center_x+img->spiral_search[0][lv])*4;
              s_pos_y=(center_y+img->spiral_search[1][lv])*4;
             
              iy0=pic4_pix_x+s_pos_x;
              jy0=pic4_pix_y+s_pos_y;
           
              /*  
              The initial setting of current_inter_sad reflects the number of bits used to signal the vector.  
              It also depends on the QP. (See documented approach of RD constrained motion search) 
              */
              tmp0=s_pos_x-ip0;
              tmp1=s_pos_y-ip1;
              current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));
              
              if (s_pos_x == 0 && s_pos_y == 0 && blocktype == 1) /* no motion, 16x16 blocktype */
              {
                current_inter_sad -= QP2QUANT[img->qp] * 16;
              } 
              abort_search=FALSE;
              for (j=0; j < blockshape_y && !abort_search; j++) 
              {
                vec2_y=jy0+j*4;
                
                for (i=0; i < blockshape_x ; i++)
                {
                  tmp0=mo[i][j]-mref[ref_inx][vec2_y][iy0+i*4];
                  
                  current_inter_sad += absm(tmp0);
                }
                if (current_inter_sad >= best_inter_sad) 
                {
                  abort_search=TRUE;
                }
              }
              if(!abort_search)
              {
                center_h2=s_pos_x;
                center_v2=s_pos_y;
                best_inter_sad=current_inter_sad;
              }
            }
            
            /*  
            center_h2, center_v2 is the search center to be used for 1/2 pixel search.  
            It is adjusted to prevent vectors pointing outside the frame.
            */
            
            center_h2=max(2-pic4_pix_x,min(center_h2,(img->width-1-pic_pix_x-(blockshape_x-1))*4-2));
            center_v2=max(2-pic4_pix_y,min(center_v2,(img->height_err-pic_pix_y-(blockshape_y-1))*4-2));
            

            /*  1/2 pixel search.  In this version the search is over 9 vector positions. */
            
            best_inter_sad=MAX_VALUE;
            for (lv=0; lv < 9; lv++) 
            {
              s_pos_x=center_h2+img->spiral_search[0][lv]*2;
              s_pos_y=center_v2+img->spiral_search[1][lv]*2;
              iy0=pic4_pix_x+s_pos_x;
              jy0=pic4_pix_y+s_pos_y;
              tmp0=s_pos_x-ip0;
              tmp1=s_pos_y-ip1;
              current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));
              
              if (s_pos_x == 0 && s_pos_y == 0 && blocktype == 1) /* no motion, blocktype 16x16 */  
              {
                current_inter_sad -= QP2QUANT[img->qp] * 16;
              }
              for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4) 
              {
                for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4) 
                {
                  for (i=0; i < BLOCK_SIZE; i++) 
                  {
                    vec2_x=i+vec1_x;
                    i22=iy0+vec2_x*4;
                    for (j=0; j < BLOCK_SIZE; j++) 
                    {
                      vec2_y=j+vec1_y;
                      img->m7[i][j]=mo[vec2_x][vec2_y]-mref[ref_inx][jy0+vec2_y*4][i22];
                    }
                  }
                  current_inter_sad += find_sad(img,inp_par->hadamard);
                }
              }
              if (current_inter_sad < best_inter_sad) 
              {
                /*  Vectors are saved in all_mv[] to be able to assign correct vectors to each block after mode selection.
                    tmp_mv[] is a 'temporary' assignment of vectors to be used to estimate 'bit cost' in vector prediction.
                */
                s_pos_x1=s_pos_x;
                s_pos_y1=s_pos_y;
                s_pos_x2=s_pos_x;
                s_pos_y2=s_pos_y;
                
                for (i=0; i < blockshape_x/BLOCK_SIZE; i++) 
                {
                  for (j=0; j < blockshape_y/BLOCK_SIZE; j++) 
                  {
                    all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=s_pos_x1;
                    all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=s_pos_y1;

                    tmp_mv[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_x1;
                    tmp_mv[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_y1;
                  }
                }
                best_inter_sad=current_inter_sad;
              }
            }
            
            /*  1/4 pixel search.   */
             
            s_pos_x2=max(1-pic4_pix_x,min(s_pos_x2,(img->width-1-pic_pix_x-(blockshape_x-1))*4-1));
            s_pos_y2=max(1-pic4_pix_y,min(s_pos_y2,(img->height_err-pic_pix_y-(blockshape_y-1))*4-1));
            for (lv=1; lv < 9; lv++) 
            {
              s_pos_x=s_pos_x2+img->spiral_search[0][lv];
              s_pos_y=s_pos_y2+img->spiral_search[1][lv];
             
              iy0=pic4_pix_x+s_pos_x;
              jy0=pic4_pix_y+s_pos_y;
              tmp0=s_pos_x-ip0;
              tmp1=s_pos_y-ip1;
              current_inter_sad=(QP2QUANT[img->qp]*(img->mv_bituse[absm(tmp0)]+img->mv_bituse[absm(tmp1)]));
              
              if (s_pos_x == 0 && s_pos_y == 0 && blocktype == 1) /* no motion, blocktype 16x16 */  
              {
                current_inter_sad -= QP2QUANT[img->qp] * 16;
              }
              for (vec1_x=0; vec1_x < blockshape_x; vec1_x += 4) 
              {
                for (vec1_y=0; vec1_y < blockshape_y; vec1_y += 4) 
                {
                  for (i=0; i < BLOCK_SIZE; i++) 
                  {
                    vec2_x=i+vec1_x;
                    i22=iy0+vec2_x*4;
                    for (j=0; j < BLOCK_SIZE; j++) 
                    {
                      vec2_y=j+vec1_y;
                      img->m7[i][j]=mo[vec2_x][vec2_y]-mref[ref_inx][jy0+vec2_y*4][i22];
                    }
                  }
                  current_inter_sad += find_sad(img,inp_par->hadamard);
                }
              }
              if (current_inter_sad < best_inter_sad) 
              {
              /*  Vectors are saved in all_mv[] to be able to assign correct vectors to each block after mode selection.
                  tmp_mv[] is a 'temporary' assignment of vectors to be used to estimate 'bit cost' in vector prediction.
                */

                s_pos_x1=s_pos_x;
                s_pos_y1=s_pos_y;
                
                for (i=0; i < blockshape_x/BLOCK_SIZE; i++) 
                {
                  for (j=0; j < blockshape_y/BLOCK_SIZE; j++) 
                  {
                    all_mv[block_x+i][block_y+j][ref_frame][blocktype][0]=s_pos_x1;
                    all_mv[block_x+i][block_y+j][ref_frame][blocktype][1]=s_pos_y1;
                    tmp_mv[0][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_x1;
                    tmp_mv[1][pic_block_y+j][pic_block_x+i+BLOCK_SIZE]=s_pos_y1;
                  }
                }
                best_inter_sad=current_inter_sad;
              }
            }
            
            tot_inter_sad += best_inter_sad;
        }
      }
      
      /*  
        Here we update which is the best inter mode. At the end we have the best inter mode.
        predframe_no:  which reference frame to use for prediction
        img->multframe_no:  Index in the mref[] matrix used for prediction 
      */
      
      if (tot_inter_sad <= min_inter_sad) 
      {
        img->mb_mode=blocktype;
        img->blc_size_h=inp_par->blc_size[blocktype][0];
        img->blc_size_v=inp_par->blc_size[blocktype][1];
        predframe_no=ref_frame;
        img->multframe_no=ref_inx;
        min_inter_sad=tot_inter_sad;
      } 
      
      while (inp_par->blc_size[++blocktype][0]==0 && blocktype<=7); /* only go through chosen blocksizes */ 
    }   
  }
  
  /*  
  tot_intra_sad is now the minimum SAD for intra.  min_inter_sad is the best (min) SAD for inter (see above).  
  Inter/intra is determined depending on which is smallest 
  */
  if (tot_intra_sad < min_inter_sad) 
  {
    img->mb_mode=img->imod+8*img->type; /* set intra mode in inter frame */
    for (hv=0; hv < 2; hv++) 
      for (i=0; i < 4; i++) 
        for (j=0; j < 4; j++) 
          tmp_mv[hv][img->block_y+j][img->block_x+i+4]=0;
  } 
  else 
  {
    img->imod=INTRA_MB_INTER; /* Set inter mode */
    for (hv=0; hv < 2; hv++) 
      for (i=0; i < 4; i++) 
        for (j=0; j < 4; j++) 
          tmp_mv[hv][img->block_y+j][img->block_x+i+4]=all_mv[i][j][predframe_no][img->mb_mode][hv];
  }
  return predframe_no;
}

/************************************************************************
*
*  Routine      find_sad
*
*  Description: Do hadamard transform or normal SAD calculation. 
*               If Hadamard=1 4x4 Hadamard transform is performed and SAD of transform 
*               levels is calculated
*
*  Input:       hadamard=0 : normal SAD
*               hadamard=1 : 4x4 Hadamard transform is performed and SAD of transform 
*                            levels is calculated
*
*  Output:      SAD/hadamard transform value
*
*                    
************************************************************************/
int find_sad(struct img_par *img,int hadamard)
{
  int i,j,m1[BLOCK_SIZE][BLOCK_SIZE],best_sad,current_sad;
  
  current_sad=0;
  if (hadamard != 0) 
  {
    best_sad=0;
    for (j=0; j < BLOCK_SIZE; j++) 
    {
      m1[0][j]=img->m7[0][j]+img->m7[3][j];
      m1[1][j]=img->m7[1][j]+img->m7[2][j];
      m1[2][j]=img->m7[1][j]-img->m7[2][j];
      m1[3][j]=img->m7[0][j]-img->m7[3][j];
      
      img->m7[0][j]=m1[0][j]+m1[1][j];
      img->m7[2][j]=m1[0][j]-m1[1][j];
      img->m7[1][j]=m1[2][j]+m1[3][j];
      img->m7[3][j]=m1[3][j]-m1[2][j];
    }
    for (i=0; i < BLOCK_SIZE; i++) 
    {
      m1[i][0]=img->m7[i][0]+img->m7[i][3];
      m1[i][1]=img->m7[i][1]+img->m7[i][2];
      m1[i][2]=img->m7[i][1]-img->m7[i][2];
      m1[i][3]=img->m7[i][0]-img->m7[i][3];
      
      img->m7[i][0]=m1[i][0]+m1[i][1];
      img->m7[i][1]=m1[i][0]-m1[i][1];
      img->m7[i][2]=m1[i][2]+m1[i][3];
      img->m7[i][3]=m1[i][3]-m1[i][2];
      
      for (j=0; j < BLOCK_SIZE; j++)
        best_sad += absm(img->m7[i][j]);
    }
    /*  
    Calculation of normalized Hadamard transforms would require divison by 4 here.  
    However,we flevel  that divison by 2 is better (assuming we give the same penalty for 
    bituse for Hadamard=0 and 1) 
    */
    current_sad += best_sad/2;
  } 
  else 
  {
    for (i=0; i < BLOCK_SIZE; i++) 
    {
      for (j=0; j < BLOCK_SIZE; j++) 
      {
        current_sad += absm(img->m7[i][j]);
      }
    }
  }
  return current_sad;
} 

/************************************************************************
*
*  Routine:     dct_luma
*
*  Description: The routine performs transform,quantization,inverse transform, adds the diff. 
*               to the prediction and writes the result to the decoded luma frame. Includes the
*               RD constrained quantization also.
*               
*               
*  Input:       block_x,block_y: Block position inside a macro block (0,4,8,12).              
*
*  Output:      nonzero: 0 if no levels are nonzero.  1 if there are nonzero levels.
*               coeff_cost: Counter for nonzero coefficients, used to discard expencive levels.
*
*                    
************************************************************************/
int dct_luma(int block_x,int block_y,int *coeff_cost, struct img_par *img)
{ 
  int sign(int a,int b);
  
  int i,j,i1,j1,ilev,m5[4],m6[4],coeff_ctr,scan_loop_ctr;
  int qp_const,pos_x,pos_y,quant_set,level,scan_pos,run;
  int nonzero;
  int idx;

  int scan_mode;
  int loop_rep;
  int coeff[16];
  
  if (img->type == INTRA_IMG) 
    qp_const=JQQ3;    /* intra */
  else                
    qp_const=JQQ4;    /* inter */
  
  pos_x=block_x/BLOCK_SIZE;
  pos_y=block_y/BLOCK_SIZE;
  
  /*  Horizontal transform */
  
  for (j=0; j < BLOCK_SIZE; j++) 
  {
    for (i=0; i < 2; i++) 
    {
      i1=3-i;
      m5[i]=img->m7[i][j]+img->m7[i1][j];
      m5[i1]=img->m7[i][j]-img->m7[i1][j];
    }
    img->m7[0][j]=(m5[0]+m5[1])*13;
    img->m7[2][j]=(m5[0]-m5[1])*13;
    img->m7[1][j]=m5[3]*17+m5[2]*7;
    img->m7[3][j]=m5[3]*7-m5[2]*17;
  }
  
  /*  Vertival transform */
  
  for (i=0; i < BLOCK_SIZE; i++) 
  {
    for (j=0; j < 2; j++) 
    {
      j1=3-j;
      m5[j]=img->m7[i][j]+img->m7[i][j1];
      m5[j1]=img->m7[i][j]-img->m7[i][j1];
    }
    img->m7[i][0]=(m5[0]+m5[1])*13;
    img->m7[i][2]=(m5[0]-m5[1])*13;
    img->m7[i][1]=m5[3]*17+m5[2]*7;
    img->m7[i][3]=m5[3]*7-m5[2]*17;
  }
 
  /* Do prescan to decide one or two Q-levels for RD constrained quantization*/

  quant_set=img->qp;
  nonzero=FALSE;
  
  if (img->imod == INTRA_MB_OLD && img->qp < 24) 
  {
    scan_mode=DOUBLE_SCAN;
    loop_rep=2;
    idx=1;              
  }
  else
  {
    scan_mode=SINGLE_SCAN;
    loop_rep=1;
    idx=0;             
  }
  
  for(scan_loop_ctr=0;scan_loop_ctr<loop_rep;scan_loop_ctr++) /* 2 times if double scan, 1 normal scan */
  {
    for (coeff_ctr=0;coeff_ctr < 16/loop_rep;coeff_ctr++)     /* 8 times if double scan, 16 normal scan */
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }
      coeff[coeff_ctr]=img->m7[i][j];
    }
    if (scan_mode==DOUBLE_SCAN)
      rd_quant(img,QUANT_LUMA_DBL,coeff);
    else
      rd_quant(img,QUANT_LUMA_SNG,coeff);

    
    run=-1;
    scan_pos=scan_loop_ctr*9;   /* for double scan; set first or second scan posision */
    for (coeff_ctr=0; coeff_ctr<16/loop_rep; coeff_ctr++)
    {
      if (scan_mode==DOUBLE_SCAN)
      {
        i=DBL_SCAN[coeff_ctr][0][scan_loop_ctr];
        j=DBL_SCAN[coeff_ctr][1][scan_loop_ctr];
      }
      else
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
      }
      run++;
      ilev=0;

      level= absm(coeff[coeff_ctr]);
      if (level != 0) 
      {
        nonzero=TRUE;
        if (level > 1)
          *coeff_cost += MAX_VALUE;                /* set high cost, shall not be discarded */  
        else 
          *coeff_cost += COEFF_COST[run];   
        img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=sign(level,img->m7[i][j]);
        img->cof[pos_x][pos_y][scan_pos][1][scan_mode]=run;
        ++scan_pos;
        run=-1;                     /* reset zero level counter */ 
        ilev=level*JQ[quant_set][1];
      }
      img->m7[i][j]=sign(ilev,img->m7[i][j]);
    }
    img->cof[pos_x][pos_y][scan_pos][0][scan_mode]=0;  /* end of block */
  }
 
  /*     IDCT. */
  /*     horizontal */
  
  for (j=0; j < BLOCK_SIZE; j++) 
  {
    for (i=0; i < BLOCK_SIZE; i++) 
    {
      m5[i]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;
    
    for (i=0; i < 2; i++) 
    {
      i1=3-i;
      img->m7[i][j]=m6[i]+m6[i1];
      img->m7[i1][j]=m6[i]-m6[i1];
    }
  }
  
  /*  vertical */
  
  for (i=0; i < BLOCK_SIZE; i++) 
  {
    for (j=0; j < BLOCK_SIZE; j++) 
    {
      m5[j]=img->m7[i][j];
    }
    m6[0]=(m5[0]+m5[2])*13;
    m6[1]=(m5[0]-m5[2])*13;
    m6[2]=m5[1]*7-m5[3]*17;
    m6[3]=m5[1]*17+m5[3]*7;
    
    for (j=0; j < 2; j++) 
    {
      j1=3-j;       
      img->m7[i][j] =min(255,max(0,(m6[j]+m6[j1]+img->mpr[i+block_x][j+block_y] *JQQ1+JQQ2)/JQQ1));      
      img->m7[i][j1]=min(255,max(0,(m6[j]-m6[j1]+img->mpr[i+block_x][j1+block_y] *JQQ1+JQQ2)/JQQ1));
    }
  }
  
  /*  Decoded block moved to frame memory */ 
  
  for (j=0; j < BLOCK_SIZE; j++) 
    for (i=0; i < BLOCK_SIZE; i++) 
      imgY[img->pix_y+block_y+j][img->pix_x+block_x+i]=img->m7[i][j];
    
    
    return nonzero;
}

/************************************************************************
*
*  Routine :    dct_chroma
*
*  Description:    
*               Transform,quantization,inverse transform for chroma.  
*               The main reason why this is done in a separate routine is the 
*               additional 2x2 transform of DC-coeffs. This routine is called 
*               ones for each of the chroma components.
*
*  Input:       uv    : Make difference between the U and V chroma component  
*               cr_cbp: chroma coded block pattern              
*
*  Output:      cr_cbp: Updated chroma coded block pattern.
*               
*
************************************************************************/
int dct_chroma(int uv,int cr_cbp,struct img_par *img) 
{
  int i,j,i1,j2,ilev,n2,n1,j1,mb_y,coeff_ctr,qp_const,pos_x,pos_y,quant_set,level ,scan_pos,run;
  int m1[BLOCK_SIZE],m5[BLOCK_SIZE],m6[BLOCK_SIZE];
  int coeff[16];
  
  if (img->type == INTRA_IMG) 
    qp_const=JQQ3;
  else
    qp_const=JQQ4;
  
  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE) 
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE) 
    {
      
      /*  Horizontal transform. */
      for (j=0; j < BLOCK_SIZE; j++) 
      {
        mb_y=n2+j;
        for (i=0; i < 2; i++) 
        {
          i1=3-i;
          m5[i]=img->m7[i+n1][mb_y]+img->m7[i1+n1][mb_y];
          m5[i1]=img->m7[i+n1][mb_y]-img->m7[i1+n1][mb_y];
        }
        img->m7[n1][mb_y]=(m5[0]+m5[1])*13;
        img->m7[n1+2][mb_y]=(m5[0]-m5[1])*13;
        img->m7[n1+1][mb_y]=m5[3]*17+m5[2]*7;
        img->m7[n1+3][mb_y]=m5[3]*7-m5[2]*17;
      }
      
      /*  Vertical transform. */
      
      for (i=0; i < BLOCK_SIZE; i++) 
      {
        j1=n1+i;
        for (j=0; j < 2; j++) 
        {
          j2=3-j;
          m5[j]=img->m7[j1][n2+j]+img->m7[j1][n2+j2];
          m5[j2]=img->m7[j1][n2+j]-img->m7[j1][n2+j2];
        }
        img->m7[j1][n2+0]=(m5[0]+m5[1])*13;
        img->m7[j1][n2+2]=(m5[0]-m5[1])*13;
        img->m7[j1][n2+1]=m5[3]*17+m5[2]*7;
        img->m7[j1][n2+3]=m5[3]*7-m5[2]*17;
      }
    }
  }
  
  /*     2X2 transform of DC coeffs. */    
  m1[0]=(img->m7[0][0]+img->m7[4][0]+img->m7[0][4]+img->m7[4][4])/2;
  m1[1]=(img->m7[0][0]-img->m7[4][0]+img->m7[0][4]-img->m7[4][4])/2;
  m1[2]=(img->m7[0][0]+img->m7[4][0]-img->m7[0][4]-img->m7[4][4])/2;
  m1[3]=(img->m7[0][0]-img->m7[4][0]-img->m7[0][4]+img->m7[4][4])/2;
  
  /*     Quant of chroma 2X2 coeffs.*/
  quant_set=QP_SCALE_CR[img->qp];
  run=-1;
  scan_pos=0;

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++) 
    coeff[coeff_ctr]=m1[coeff_ctr];
  
  rd_quant(img,QUANT_CHROMA_DC,coeff);

  for (coeff_ctr=0; coeff_ctr < 4; coeff_ctr++) 
  {
    run++;
    ilev=0;
    level =0;
    
    level =(absm(coeff[coeff_ctr]));
    if (level  != 0) 
    {
      cr_cbp=max(1,cr_cbp);
      img->cofu[scan_pos][0][uv]=sign(level ,m1[coeff_ctr]);
      img->cofu[scan_pos][1][uv]=run;
      scan_pos++;
      run=-1;
      ilev=level*JQ[quant_set][1];
    }
    m1[coeff_ctr]=sign(ilev,m1[coeff_ctr]);
  }
  img->cofu[scan_pos][0][uv]=0;
  
  /*  Invers transform of 2x2 DC levels */
  
  img->m7[0][0]=(m1[0]+m1[1]+m1[2]+m1[3])/2;
  img->m7[4][0]=(m1[0]-m1[1]+m1[2]-m1[3])/2;
  img->m7[0][4]=(m1[0]+m1[1]-m1[2]-m1[3])/2;
  img->m7[4][4]=(m1[0]-m1[1]-m1[2]+m1[3])/2;
  
  /*     Quant of chroma AC-coeffs. */
  
  for (n2=0; n2 <= BLOCK_SIZE; n2 += BLOCK_SIZE) 
  {
    for (n1=0; n1 <= BLOCK_SIZE; n1 += BLOCK_SIZE) 
    {
      pos_x=n1/BLOCK_SIZE + 2*uv;
      pos_y=n2/BLOCK_SIZE + BLOCK_SIZE;
      run=-1;
      scan_pos=0;

      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++) 
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        coeff[coeff_ctr-1]=img->m7[n1+i][n2+j];
      }
      rd_quant(img,QUANT_CHROMA_AC,coeff);
      
      for (coeff_ctr=1; coeff_ctr < 16; coeff_ctr++) 
      {
        i=SNGL_SCAN[coeff_ctr][0];
        j=SNGL_SCAN[coeff_ctr][1];
        ++run;
        ilev=0;
        
        level=absm(coeff[coeff_ctr-1]);
        if (level  != 0) 
        {
          cr_cbp=2;
          img->cof[pos_x][pos_y][scan_pos][0][0]=sign(level,img->m7[n1+i][n2+j]);
          img->cof[pos_x][pos_y][scan_pos][1][0]=run;
          ++scan_pos;
          run=-1;
          ilev=level*JQ[quant_set][1];
        }
        img->m7[n1+i][n2+j]=sign(ilev,img->m7[n1+i][n2+j]); /* for use in IDCT */
      }
      img->cof[pos_x][pos_y][scan_pos][0][0]=0; /* EOB */

    
      /*     IDCT. */
      
      /*     Horizontal. */
      for (j=0; j < BLOCK_SIZE; j++) 
      {
        for (i=0; i < BLOCK_SIZE; i++) 
        {
          m5[i]=img->m7[n1+i][n2+j];
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;
        
        for (i=0; i < 2; i++) 
        {
          i1=3-i;
          img->m7[n1+i][n2+j]=m6[i]+m6[i1];
          img->m7[n1+i1][n2+j]=m6[i]-m6[i1];
        }
      }
      
      /*     Vertical. */
      for (i=0; i < BLOCK_SIZE; i++) 
      {
        for (j=0; j < BLOCK_SIZE; j++) 
        {
          m5[j]=img->m7[n1+i][n2+j];         
        }
        m6[0]=(m5[0]+m5[2])*13;
        m6[1]=(m5[0]-m5[2])*13;
        m6[2]=m5[1]*7-m5[3]*17;
        m6[3]=m5[1]*17+m5[3]*7;
        
        for (j=0; j < 2; j++) 
        {
          j2=3-j;
          img->m7[n1+i][n2+j]=min(255,max(0,(m6[j]+m6[j2]+img->mpr[n1+i][n2+j]*JQQ1+JQQ2)/JQQ1));
          img->m7[n1+i][n2+j2]=min(255,max(0,(m6[j]-m6[j2]+img->mpr[n1+i][n2+j2]*JQQ1+JQQ2)/JQQ1));
        }
      }
    }
  }
  
  /*  Decoded block moved to memory */
  for (j=0; j < BLOCK_SIZE*2; j++) 
    for (i=0; i < BLOCK_SIZE*2; i++)
    {
      imgUV[uv][img->pix_c_y+j][img->pix_c_x+i]= img->m7[i][j];
    }
    
    return cr_cbp;
}

void rd_quant(struct img_par *img, int scan_type,int *coeff)
{
  int idx,coeff_ctr;
  int qp_const,intra_add;
  
  int dbl_coeff_ctr;
  int level0,level1;
  int snr0;
  int dbl_coeff,k,k1,k2,rd_best,best_coeff_comb,rd_curr,snr1;
  int k0;
  int quant_set;
  int ilev,run,level;
  int no_coeff;

  if (img->type == INTRA_IMG)
  {
    qp_const=JQQ3;
    intra_add=2;
  }
  else
  {
    qp_const=JQQ4;
    intra_add=0;
  }
   quant_set=img->qp;

   switch (scan_type)
   {
   case  QUANT_LUMA_SNG:
     quant_set=img->qp;
     idx=2;
     no_coeff=16;
     break;
   case  QUANT_LUMA_AC:
     quant_set=img->qp;
     idx=2;
     no_coeff=15;
     break;
   case QUANT_LUMA_DBL:
     quant_set=img->qp;
     idx=1;
     no_coeff=8;
     break;   
   case QUANT_CHROMA_DC:
     quant_set=QP_SCALE_CR[img->qp];
     idx=0;
     no_coeff=4;
     break;  
   case QUANT_CHROMA_AC:
     quant_set=QP_SCALE_CR[img->qp];
     idx=2;
     no_coeff=15;
     break;
   default:
     break;
   }

  dbl_coeff_ctr=0;
  for (coeff_ctr=0;coeff_ctr < no_coeff ;coeff_ctr++)
  { 
    k0=coeff[coeff_ctr];
    k1=abs(k0);

    if (dbl_coeff_ctr < MAX_TWO_LEVEL_COEFF)  /* limit the number of 'twin' levels */
    {
      level0 = (k0*JQ[quant_set][0])/J20;
      level1 = (k1*JQ[quant_set][0]+JQ4)/J20; /* make positive summation */
      level1 = sign(level1,k0);/* set back sign on level */
    }
    else              
    {
      level0 = (k1*JQ[quant_set][0]+qp_const)/J20;
      level0 = sign(level0,k0);
      level1 = level0;               
    }
    
    if (level0 != level1)
    {
      dbl_coeff = TRUE;     /* decision is still open */
      dbl_coeff_ctr++;      /* count number of coefficients with 2 possible levels */
    }
    else
      dbl_coeff = FALSE;    /* level is decided       */
    
    
    snr0 = (12+intra_add)*level0*(64*level0 - (JQ[quant_set][0]*coeff[coeff_ctr])/J13); /* find SNR improvement */
    
    level_arr[coeff_ctr][MTLC_POW]=0; /* indicates that all coefficients are decided */
    
    for (k=0; k< MTLC_POW; k++)
    {
      level_arr[coeff_ctr][k]=level0;
      snr_arr[coeff_ctr][k]=snr0;
    }
    if (dbl_coeff)
    {
      snr1 = (12+intra_add)*level1*(64*level1 - (JQ[quant_set][0]*coeff[coeff_ctr])/J13);
      ilev= (int)pow(2,dbl_coeff_ctr-1);
      for (k1=ilev; k1<MTLC_POW; k1+=ilev*2)
      {
        for (k2=k1; k2<k1+ilev; k2++)
        {
          level_arr[coeff_ctr][k2]=level1;
          snr_arr[coeff_ctr][k2]=snr1;
        }
      }
    }
  }
  
  rd_best=0;
  best_coeff_comb= MTLC_POW;      /* initial setting, used if no double decision coefficients */
  for (k=0; k < pow(2,dbl_coeff_ctr);k++) /* go through all combinations */ 
  {
    rd_curr=0;
    run=-1;
    for (coeff_ctr=0;coeff_ctr < no_coeff;coeff_ctr++)
    {
      run++;
      level=min(16,absm(level_arr[coeff_ctr][k]));
      if (level != 0)
      {
        rd_curr += 64*COEFF_BIT_COST[idx][run][level-1]+snr_arr[coeff_ctr][k];
        run = -1;
      }
    }
    if (rd_curr < rd_best)
    {
      rd_best=rd_curr;
      best_coeff_comb=k;
    }
  }
  for (coeff_ctr=0;coeff_ctr < no_coeff ;coeff_ctr++)
    coeff[coeff_ctr]=level_arr[coeff_ctr][best_coeff_comb];
  
  return;
}


/* control the sign of a with b */
int sign(int a,int b)
{
  int x;
  x=absm(a); 
  if (b >= 0)
    return x;
  else
    return -x;
}



