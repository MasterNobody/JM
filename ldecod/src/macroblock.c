/************************************************************************
*
*  macroblock.c for H.26L decoder. 
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

#include <global.h>
#include <math.h>                   
#include <macroblock.h>
#include <stdlib.h>

#include <stdio.h>

/************************************************************************
*
*  Routine:     macroblock()
*
*  Description: Decode one macroblock
*                              
*  Input:       Image parameters
*
*  Output:      SEARCH_SYNC : error in bitstream 
*               DECODING_OK : no error was detected 
*                    
************************************************************************/
int macroblock(struct img_par *img)
{
  int js[2][2];
  int i,j,ii,jj,iii,jjj,i1,j1,ie,j4,i4,k,i0,pred_vec,ip0,ip1,ip2,j0;    
  int js0,js1,js2,js3,jf;
  int coef_ctr,vec,cbp,uv,ll;
  int scan_loop_ctr;
  int level,run;
  int ref_frame;
  int len,info;
  int predframe_no;                           /* frame number which current MB is predicted from */
  int block_x,block_y;
  int cbp_idx,dbl_ipred_word;
  int vec0_x,vec0_y,vec1_x,vec1_y,vec2_x,vec2_y;
  int scan_mode;

  const int ICBPTAB[6] = {0,16,32,15,31,47};
  int mod0;
  int kmod;
  int start_scan;
  int ioff,joff;
  int i3,j3; 

  int ii0,jj0,ii1,jj1,if1,jf1,if0,jf0;

  int m2;
  int jg2;
    
  if (img->type==INTER_IMG_1 || (img->type==INTER_IMG_MULT))         /* inter frame */
  {   
    info=get_info(&len);
    img->mb_mode = (int)pow(2,(len/2))+info-1;          /* inter prediction mode (block shape) */                                    
    if (img->mb_mode>32) /* 8 inter + 24 new intra */
      return SEARCH_SYNC;
  
    if (img->mb_mode == INTRA_MB) /* 4x4 intra */
      img->imod=INTRA_MB_OLD;
    if (img->mb_mode > INTRA_MB)  /* 16x16 intra */
    {
      img->imod=INTRA_MB_NEW;
      mod0=img->mb_mode - INTRA_MB-1;
      kmod=mod0 & 3;
      cbp = ICBPTAB[mod0/4];
    }
    if (img->mb_mode < INTRA_MB)  /* intra in inter frame */ 
      img->imod=INTRA_MB_INTER;

  }
  else if ((img->type==INTRA_IMG))                      
  {
    info=get_info(&len);
    img->mb_mode = (int)pow(2,(len/2))+info-1;
    if (img->mb_mode>24) /* 24 new intra */
      return SEARCH_SYNC;
    
    if (img->mb_mode == 0)
      img->imod=INTRA_MB_OLD; /* 4x4 intra */
    else
    {
      img->imod=INTRA_MB_NEW;/* new */
      mod0=img->mb_mode-1;
      kmod=mod0 & 3;
      cbp = ICBPTAB[mod0/4];
    }
  }
  else
  {
    printf("Wrong image type, exiting\n");
    return SEARCH_SYNC;
  }

  img->mv[img->block_x+4][img->block_y][2]=img->number;
  
  for (i=0;i<BLOCK_SIZE;i++)                            /* reset vectors and pred. modes  */
    for(j=0;j<BLOCK_SIZE;j++)           
    {
      img->mv[img->block_x+i+4][img->block_y+j][0] = 0;
      img->mv[img->block_x+i+4][img->block_y+j][1] = 0;
      img->ipredmode[img->block_x+i+1][img->block_y+j+1] = 0;
    }
    ref_frame=img->frame_cycle;
    predframe_no=1;
    
    if (img->imod==INTRA_MB_INTER && img->mb_mode==COPY_MB) /*keep last macroblock*/
    {
     for (i=0;i<4;i++)
      {
        ii=img->block_x+i;
        i3=ii/2;
        for (j=0;j<4;j++)
        { 
          jj=img->block_y+j;
          j3=jj/2;
          
          if ((img->mv[ii-1+4][jj][0]/4)!=0||(img->mv[ii-1+4][jj][1]/4!=0))
          {
            loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
            loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
            /*if (img->block_x>0 ) fix from ver 4.1 */
            loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1); 
            loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
          }
          if ((img->mv[ii+4][jj-1][0]/4!=0)||(img->mv[ii+4][jj-1][1]/4!=0))
          {
            loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],1);
            loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
            /*if (img->block_x>0 )  fix from ver 4.1 */
            loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
            loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
          }
        }
      }
      return DECODING_OK;
    }
    
    if (img->imod==INTRA_MB_OLD)                 /* find two and two intra prediction mode for a macroblock */    
    {
      for(i=0;i<MB_BLOCK_SIZE/2;i++)            
      {
        info=get_info(&len);        
        dbl_ipred_word = (int) pow(2,(len/2))+info-1;        
        if (dbl_ipred_word>35)                                 
        {              
          return SEARCH_SYNC;                   /* bitfault, only 36 possible combinations*/ 
        } 
        
        i1=img->block_x + 2*(i&0x01);
        j1=img->block_y + i/2;
        
        /* find intra prediction mode for two blocks */ 
        img->ipredmode[i1+1][j1+1] = PRED_IPRED[img->ipredmode[i1+1][j1]+1][img->ipredmode[i1][j1+1]+1][IPRED_ORDER[dbl_ipred_word][0]];
        img->ipredmode[i1+2][j1+1] = PRED_IPRED[img->ipredmode[i1+2][j1]+1][img->ipredmode[i1+1][j1+1]+1][IPRED_ORDER[dbl_ipred_word][1]];
      }
    }
    if(img->imod==INTRA_MB_INTER)               /* inter frame, read vector data */
    {     
      if(img->type==INTER_IMG_MULT)     
      {
        info=get_info(&len);
        predframe_no=(int)pow(2,len/2)+info;      /* find frame number to copy blocks from (if mult.pred mode)*/
        if (predframe_no>MAX_MULT_PRED)
            return SEARCH_SYNC;
        ref_frame=(img->frame_cycle+6-predframe_no)%5;
        img->mv[img->block_x+4][img->block_y][2]=img->number-predframe_no;
      }
      ie=4-BLOCK_STEP[img->mb_mode][0];
      for(j=0;j<4;j=j+BLOCK_STEP[img->mb_mode][1])
      {
        j4=img->block_y+j;
        for(i=0;i<4;i=i+BLOCK_STEP[img->mb_mode][0])
        {
          i4=img->block_x+i;
          for (k=0;k<2;k++)                       /* vector prediction. find first koordinate set for pred */
          {
            vec0_x=i4-1;
            vec0_y=j4;
            vec1_x=i4;
            vec1_y=j4-1;
            vec2_x=i4+BLOCK_STEP[img->mb_mode][0];
            vec2_y=j4-1;
            /* correction for upper edge */
            if(j4==0)
            {
              vec1_x=vec0_x;
              vec2_x=vec0_x;
              vec1_y=vec0_y;
              vec2_y=vec0_y;
            }
            /* correction for unknown vector or to the right for pic */
            if(((i==ie)&&(j!=0))||(vec2_x>(img->width_cr/2-1)))
              vec2_x=i4-1;
            
            ip0=img->mv[vec0_x+BLOCK_SIZE][vec0_y][k];
            ip1=img->mv[vec1_x+BLOCK_SIZE][vec1_y][k];
            ip2=img->mv[vec2_x+BLOCK_SIZE][vec2_y][k];
            
            pred_vec=ip0+ip1+ip2-mmin(ip0,mmin(ip1,ip2))-mmax(ip0,mmax(ip1,ip2));
            
            info=get_info(&len);
            if (len>17)
              return SEARCH_SYNC;
            
            vec=linfo_mvd(len,info)+pred_vec;           /* find motion vector */
            for(ii=0;ii<BLOCK_STEP[img->mb_mode][0];ii++)
              for(jj=0;jj<BLOCK_STEP[img->mb_mode][1];jj++)
              {
                img->mv[i4+ii+BLOCK_SIZE][j4+jj][k]=vec;
              }
          }  
        }
      }      
    }
    /* read CBP if not new intra mode */
    if (img->imod !=INTRA_MB_NEW)
    {
      info=get_info(&len);
      cbp_idx = (int) pow(2,(len/2))+info-1;      /* find cbp index in VLC table */
      if (cbp_idx>47)                             /* illegal value, exit to find next sync word */
      {
        printf("Error in index n to array cbp\n");
        return SEARCH_SYNC;        
      }
      cbp=NCBP[cbp_idx][img->imod/INTRA_MB_INTER];/* find coded block pattern for intra or inter */
    }
    
    for (i=0;i<BLOCK_SIZE;i++)
      for (j=0;j<BLOCK_SIZE;j++)
        for(iii=0;iii<BLOCK_SIZE;iii++)
          for(jjj=0;jjj<BLOCK_SIZE;jjj++)
            img->cof[i][j][iii][jjj]=0;/* reset luma coeffs */
          
    if(img->imod==INTRA_MB_NEW) /* read DC coeffs for new intra modes */
    {
      for (i=0;i<BLOCK_SIZE;i++)
        for (j=0;j<BLOCK_SIZE;j++)
          img->ipredmode[img->block_x+i+1][img->block_y+j+1]=0;
        
        
      coef_ctr=-1;
      len = 0;                            /* just to get inside the loop */ 
      for(k=0;(k<17) && (len!=1);k++)
      {
        info=get_info(&len);
        
        if(len>29)
          return SEARCH_SYNC;             /* illegal length, start_scan sync search */
        if (len != 1)                     /* leave if len=1 */
        {
          linfo_levrun_inter(len,info,&level,&run);
          coef_ctr=coef_ctr+run+1;
          
          i0=SNGL_SCAN[coef_ctr][0];
          j0=SNGL_SCAN[coef_ctr][1];
          
          if(coef_ctr > 15)
            return SEARCH_SYNC; 
          
          img->cof[i0][j0][0][0]=level;/* add new intra DC coeff */          
        }
      }
     itrans_2(img);/* transform new intra DC  */
   }

  if (img->imod == INTRA_MB_OLD || img->imod == INTRA_MB_NEW)
  {    
    for (i=0;i<4;i++)
    {
      ii=img->block_x+i;
      i3=ii/2;
      for (j=0;j<4;j++)
      {
        jj=img->block_y+j;
        j3=jj/2;
        loopb[ii+1][jj+1]=3;
        loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],2);
        loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],2);
        loopb[ii+2][jj+1]=max(loopb[ii+2][jj+1],2);
        loopb[ii+1][jj+2]=max(loopb[ii+1][jj+2],2);

        loopc[i3+1][j3+1]=2;
        loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
        loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
        loopc[i3+2][j3+1]=max(loopc[i3+2][j3+1],1);
        loopc[i3+1][j3+2]=max(loopc[i3+1][j3+2],1);
      }
    }
  }
  else /* inter */ 
  {
    
    for (i=0;i<4;i++)
    {
      ii=img->block_x+i;
      i3=ii/2;
      for (j=0;j<4;j++)
      { 
        jj=img->block_y+j;
        j3=jj/2;
        if ((img->mv[ii+4][jj][0]/4!=img->mv[ii-1+4][jj][0]/4)||(img->mv[ii+4][jj][1]/4!=img->mv[ii-1+4][jj][1]/4))
        {
          loopb[ii  ][jj+1]=max(loopb[ii  ][jj+1],1);
          loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
          /* if (img->block_x>0 )fix from ver 4.1 */
          loopc[i3  ][j3+1]=max(loopc[i3  ][j3+1],1);
          loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
        }
        if ((img->mv[ii+4][jj][0]/4!=img->mv[ii+4][jj-1][0]/4)||(img->mv[ii+4][jj][1]/4!=img->mv[ii+4][jj-1][1]/4))
        {
          loopb[ii+1][jj  ]=max(loopb[ii+1][jj  ],1);
          loopb[ii+1][jj+1]=max(loopb[ii+1][jj+1],1);
           /*if (img->block_x>0 ) fix from ver 4.1 */
          loopc[i3+1][j3  ]=max(loopc[i3+1][j3  ],1);
          loopc[i3+1][j3+1]=max(loopc[i3+1][j3+1],1);
        }
      }
    }
  }
             
    if (img->imod == INTRA_MB_OLD && img->qp < 24) 
      scan_mode=DOUBLE_SCAN;
    else
      scan_mode=SINGLE_SCAN;

 
    /* luma coefficients */
    for (block_y=0; block_y < 4; block_y += 2) /* all modes */
    {
      for (block_x=0; block_x < 4; block_x += 2)
      {       
        for (j=block_y; j < block_y+2; j++)
        {
          jj=j/2;
          for (i=block_x; i < block_x+2; i++)
          {
            ii=i/2;
            if (img->imod == INTRA_MB_NEW)
              start_scan = 1; /* skip DC coeff */
            else
              start_scan = 0; /* take all coeffs */
            
            if((cbp & (int)pow(2,(ii+2*jj))) != 0)  /* are there any coeff in current block at all */  
            {
              if (scan_mode==SINGLE_SCAN)
              {
                coef_ctr=start_scan-1;
                len = 0;                            /* just to get inside the loop */ 
                for(k=start_scan;(k<17) && (len!=1);k++)
                {
                  info=get_info(&len);

                  if(len>29)
                    return SEARCH_SYNC;             /* illegal length, start_scan sync search */
                  if (len != 1)                     /* leave if len=1 */
                  {
                    linfo_levrun_inter(len,info,&level,&run);
                    coef_ctr += run+1;
                    
                    i0=SNGL_SCAN[coef_ctr][0];
                    j0=SNGL_SCAN[coef_ctr][1];
                    
                    if(coef_ctr > 15)
                      return SEARCH_SYNC; 
                    
                    img->cof[i][j][i0][j0]=level*JQ1[img->qp];
                    if (level=!0)
                    {
                      loopb[img->block_x+i+1][img->block_y+j+1]=max(loopb[img->block_x+i+1][img->block_y+j+1],2);

                      loopb[img->block_x+i  ][img->block_y+j+1]=max(loopb[img->block_x+i  ][img->block_y+j+1],1);
                      loopb[img->block_x+i+1][img->block_y+j  ]=max(loopb[img->block_x+i+1][img->block_y+j  ],1);
                      loopb[img->block_x+i+2][img->block_y+j+1]=max(loopb[img->block_x+i+2][img->block_y+j+1],1);
                      loopb[img->block_x+i+1][img->block_y+j+2]=max(loopb[img->block_x+i+1][img->block_y+j+2],1);
                    }
                   }
                }                
              }
              else    /* double scan (old intra with QP<24*/
              {
                for(scan_loop_ctr=0;scan_loop_ctr<2;scan_loop_ctr++)
                {
                  coef_ctr=start_scan-1;
                  len=0;                          /* just to get inside the loop */ 
                  for(k=0; k<9 && len!=1;k++)
                  {
                    info=get_info(&len);

                    if(len>29)
                      return SEARCH_SYNC;         /* illegal length, start_scan sync search */
                    if (len != 1)                 /* leave if len=1 */
                    {
                      
                      linfo_levrun_intra(len,info,&level,&run);
                      coef_ctr=coef_ctr+run+1;
                      
                      i0=DBL_SCAN[coef_ctr][0][scan_loop_ctr];
                      j0=DBL_SCAN[coef_ctr][1][scan_loop_ctr];
                      
                      if(coef_ctr > 7)
                        return SEARCH_SYNC; 
                      img->cof[i][j][i0][j0]=level*JQ1[img->qp];

                    if (level=!0)
                    {
                      loopb[img->block_x+i+1][img->block_y+j+1]=max(loopb[img->block_x+i+1][img->block_y+j+1],2);

                      loopb[img->block_x+i  ][img->block_y+j+1]=max(loopb[img->block_x+i  ][img->block_y+j+1],1);
                      loopb[img->block_x+i+1][img->block_y+j  ]=max(loopb[img->block_x+i+1][img->block_y+j  ],1);
                      loopb[img->block_x+i+2][img->block_y+j+1]=max(loopb[img->block_x+i+2][img->block_y+j+1],1);
                      loopb[img->block_x+i+1][img->block_y+j+2]=max(loopb[img->block_x+i+1][img->block_y+j+2],1);
                    }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    
    for (j=4;j<6;j++) /* reset all chroma coeffs before read */
      for (i=0;i<4;i++)
        for (iii=0;iii<4;iii++)
          for (jjj=0;jjj<4;jjj++)
            img->cof[i][j][iii][jjj]=0;

   m2=img->mb_x*2;
   jg2=img->mb_y*2;

   /* chroma 2x2 DC coeff */   
    if(cbp>15)
    {
      for (ll=0;ll<3;ll+=2)
      {
        for (i=0;i<4;i++)
          img->cofu[i]=0;
        
        coef_ctr=-1;
        len=0;
        for(k=0;(k<5)&&(len!=1);k++)
        {      
          info=get_info(&len);
          if (len>29)
            return SEARCH_SYNC; 
          if (len != 1)                         
          { 
            linfo_levrun_c2x2(len,info,&level,&run);
            coef_ctr=coef_ctr+run+1;
            if(coef_ctr>3)
              return SEARCH_SYNC;
            img->cofu[coef_ctr]=level*JQ1[QP_SCALE_CR[img->qp]];
            
         /* } fix from ver 4.1 */
          
            if (level != 0 );
            { 
              for (j=0;j<2;j++)
              {
                for (i=0;i<2;i++)
                {
                  loopc[m2+i+1][jg2+j+1]=max(loopc[m2+i+1][jg2+j+1],2);
                }
              }
            
              for (i=0;i<2;i++)
              {
                loopc[m2+i+1][jg2    ]=max(loopc[m2+i+1][jg2    ],1);
                loopc[m2+i+1][jg2+3  ]=max(loopc[m2+i+1][jg2+3  ],1);
                loopc[m2    ][jg2+i+1]=max(loopc[m2    ][jg2+i+1],1);
                loopc[m2+3  ][jg2+i+1]=max(loopc[m2+3  ][jg2+i+1],1);
              }
            }
          }
        }/* fix from ver 4.1 */
        img->cof[0+ll][4][0][0]=(img->cofu[0]+img->cofu[1]+img->cofu[2]+img->cofu[3])/2;
        img->cof[1+ll][4][0][0]=(img->cofu[0]-img->cofu[1]+img->cofu[2]-img->cofu[3])/2;
        img->cof[0+ll][5][0][0]=(img->cofu[0]+img->cofu[1]-img->cofu[2]-img->cofu[3])/2;
        img->cof[1+ll][5][0][0]=(img->cofu[0]-img->cofu[1]-img->cofu[2]+img->cofu[3])/2;
      }
    }        
        
    /* chroma AC coeff, all zero fram start_scan */
    if (cbp>31)
    {
      block_y=4;
      for (block_x=0; block_x < 4; block_x += 2)
      {       
        for (j=block_y; j < block_y+2; j++) 
        {
          jj=j/2;
          j1=j-4;
          for (i=block_x; i < block_x+2; i++) 
          {
            ii=i/2;
            i1=i%2;
            {
              coef_ctr=0;
              len=0;
              for(k=0;(k<16)&&(len!=1);k++)
              {
                info=get_info(&len);

                if (len>29)
                  return SEARCH_SYNC; 
                if (len != 1)            
                {
                  linfo_levrun_inter(len,info,&level,&run);
                  coef_ctr=coef_ctr+run+1;
                  i0=SNGL_SCAN[coef_ctr][0];
                  j0=SNGL_SCAN[coef_ctr][1];
                  if(coef_ctr>15)
                    return SEARCH_SYNC;
                  img->cof[i][j][i0][j0]=level*JQ1[QP_SCALE_CR[img->qp]]; 
            
                  if (level!=0) 
                  {              
                    loopc[m2+i1+1][jg2+j1+1]=max(loopc[m2+i1+1][jg2+j1+1],2);
                    
                    loopc[m2+i1  ][jg2+j1+1]=max(loopc[m2+i1  ][jg2+j1+1],1);
                    loopc[m2+i1+1][jg2+j1  ]=max(loopc[m2+i1+1][jg2+j1  ],1);
                    loopc[m2-i1+2][jg2+j1+1]=max(loopc[m2-i1+2][jg2+j1+1],1);
                    loopc[m2+i1+1][jg2+j1+2]=max(loopc[m2+i1+1][jg2+j1+2],1); 
                  } 
                }
                
               
              }
            }
          }
        }
      }
    }
 
    /* start_scan decoding */ 
    
    /* luma */
    if (img->imod==INTRA_MB_NEW)
      intrapred_luma_2(img,kmod);

    for(j=0;j<MB_BLOCK_SIZE/BLOCK_SIZE;j++)
    {
      joff=j*4;
      j4=img->block_y+j;
      for(i=0;i<MB_BLOCK_SIZE/BLOCK_SIZE;i++)
      {
        ioff=i*4;
        i4=img->block_x+i;
        if(img->imod==INTRA_MB_OLD)
        {
          if (intrapred(img,ioff,joff,i4,j4)==SEARCH_SYNC)  /* make 4x4 prediction block mpr from given prediction img->mb_mode */
            return SEARCH_SYNC;                   /* bit error */
        }
        else if(img->imod==INTRA_MB_INTER) 
        {
          for(ii=0;ii<BLOCK_SIZE;ii++)
          {
            vec2_x=(i4*4+ii)*4;
            vec1_x=vec2_x+img->mv[i4+BLOCK_SIZE][j4][0];
            for(jj=0;jj<MB_BLOCK_SIZE/BLOCK_SIZE;jj++)
            {
              vec2_y=(j4*4+jj)*4;
              vec1_y=vec2_y+img->mv[i4+4][j4][1];
              img->mpr[ii+ioff][jj+joff]=mref[ref_frame][vec1_y][vec1_x];
            }

          }
        } 
        
        itrans(img,ioff,joff,i,j);      /* use DCT transform and make 4x4 block m7 from prediction block mpr */
        
        for(ii=0;ii<BLOCK_SIZE;ii++)
          for(jj=0;jj<BLOCK_SIZE;jj++)
          {
            imgY[j4*BLOCK_SIZE+jj][i4*BLOCK_SIZE+ii]=img->m7[ii][jj]; /* contruct picture from 4x4 blocks*/
          }           
      }
    }
    /* chroma */   
    
    for(uv=0;uv<2;uv++)
    {
      if (img->imod==INTRA_MB_OLD || img->imod==INTRA_MB_NEW)/* intra mode */
      {
        js0=0;
        js1=0;
        js2=0;
        js3=0;
        for(i=0;i<4;i++)
        {
          if(img->mb_y >0)
          {
            js0=js0+imgUV[uv][img->pix_c_y-1][img->pix_c_x+i];
            js1=js1+imgUV[uv][img->pix_c_y-1][img->pix_c_x+i+4];
          }
          if(img->mb_x>0)
          {
            js2=js2+imgUV[uv][img->pix_c_y+i][img->pix_c_x-1];
            js3=js3+imgUV[uv][img->pix_c_y+i+4][img->pix_c_x-1];
          }
        }
        if((img->mb_x>=0)&&(img->mb_y >=0))
        {
          js[0][0]=(js0+js2+4)/8;
          js[1][0]=(js1+2)/4;
          js[0][1]=(js3+2)/4;
          js[1][1]=(js1+js3+4)/8;
        }
        if((img->mb_x==0)&&(img->mb_y >=0))
        {
          js[0][0]=(js0+2)/4;
          js[1][0]=(js1+2)/4;
          js[0][1]=(js0+2)/4;
          js[1][1]=(js1+2)/4;
        }
        if((img->mb_x>=0)&&(img->mb_y ==0))
        {
          js[0][0]=(js2+2)/4;
          js[1][0]=(js2+2)/4;
          js[0][1]=(js3+2)/4;
          js[1][1]=(js3+2)/4;
        }
        if((img->mb_x==0)&&(img->mb_y ==0))
        {
          js[0][0]=128;
          js[1][0]=128;
          js[0][1]=128;
          js[1][1]=128;
        }
      }
      
      for (j=4;j<6;j++)
      {
        joff=(j-4)*4;
        j4=img->pix_c_y+joff;
        for(i=0;i<2;i++)
        {
          ioff=i*4;
          i4=img->pix_c_x+ioff;
          /* make pred */
          if(img->imod==INTRA_MB_OLD|| img->imod==INTRA_MB_NEW)/* intra */
          {
            for(ii=0;ii<4;ii++)
              for(jj=0;jj<4;jj++)
              {
                img->mpr[ii+ioff][jj+joff]=js[i][j-4];
              }
          }
          else
          {
            for(jj=0;jj<4;jj++)
            {
              jf=(j4+jj)/2;
              for(ii=0;ii<4;ii++)
              {
                if1=(i4+ii)/2;
                i1=(img->pix_c_x+ii+ioff)*8+img->mv[if1+4][jf][0];
                j1=(img->pix_c_y+jj+joff)*8+img->mv[if1+4][jf][1];
                
                ii0=i1/8;
                jj0=j1/8;
                ii1=(i1+7)/8;
                jj1=(j1+7)/8;

                if1=(i1 & 7);
                jf1=(j1 & 7);
                if0=8-if1;
                jf0=8-jf1;
                img->mpr[ii+ioff][jj+joff]=(if0*jf0*mcef[ref_frame][uv][jj0][ii0]+
                          if1*jf0*mcef[ref_frame][uv][jj0][ii1]+
                          if0*jf1*mcef[ref_frame][uv][jj1][ii0]+
                          if1*jf1*mcef[ref_frame][uv][jj1][ii1]+32)/64; 
               }              
            }
          }   
          itrans(img,ioff,joff,2*uv+i,j);
          for(ii=0;ii<4;ii++)
            for(jj=0;jj<4;jj++)
            {
              imgUV[uv][j4+jj][i4+ii]=img->m7[ii][jj];
            }
        }
      }
    }  
    return DECODING_OK;
}
