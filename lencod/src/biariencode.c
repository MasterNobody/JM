// *************************************************************************************
// *************************************************************************************
// Biariencode.c	 Routines for binary arithmetic encoding
//
// Main contributors (see contributors.h for copyright, address and affiliation details)
//
// Detlev Marpe                    <marpe@hhi.de>
// Gabi Blaettermann               <blaetter@hhi.de>
// *************************************************************************************
// *************************************************************************************
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "global.h"
#include "biariencode.h"


/************************************************************************
 * M a c r o    f o r    w r i t i n g    b y t e s    o f    c o d e   
 ***********************************************************************
 */

#define put_byte() { \
								Ecodestrm[(*Ecodestrm_len)++] = Ebuffer; \
                Ebits_to_go = 8; \
										}



/************************************************************************
*
*  Name :       arienco_create_encoding_environment()
*
*  Description: Allocates memory for the EncodingEnvironment struct
*
************************************************************************/
EncodingEnvironmentPtr arienco_create_encoding_environment()
{
	EncodingEnvironmentPtr eep;
	
	if ( (eep = (EncodingEnvironmentPtr) calloc(1,sizeof(EncodingEnvironment))) == NULL)
		no_mem_exit(1);
	
	return eep;
}



/************************************************************************
*
*  Name :				arienco_delete_encoding_environment()
*
*  Description: Frees memory of the EncodingEnvironment struct
*
************************************************************************/
void arienco_delete_encoding_environment(EncodingEnvironmentPtr eep)
{
	if (eep == NULL)
		no_mem_exit(1);
	else
		free(eep);
}



/************************************************************************
*
*  Name :				arienco_start_encoding()
*
*  Description: Initializes the EncodingEnvironment for the arithmetic coder
*
************************************************************************/
void arienco_start_encoding(EncodingEnvironmentPtr eep, 
							unsigned char *code_buffer, 
							int *code_len )
{
	
	Elow = 0;
	Ehigh = TOP_VALUE;
	Ebits_to_follow = 0;
	Ebuffer = 0;
	Ebits_to_go = 8;
		
	Ecodestrm = code_buffer;
	Ecodestrm_len = code_len;
		
}   

/************************************************************************
*
*  Name :				arienco_bits_written()
*
*  Description: Returns the number of currently written bits 
*
************************************************************************/
int arienco_bits_written(EncodingEnvironmentPtr eep)
{
   return (8 * (*Ecodestrm_len) + Ebits_to_follow + 8 + 2 - Ebits_to_go);
}


/************************************************************************
*
*  Name :				arienco_done_encoding()
*
*  Description: Terminates the arithmetic coder and writes the trailing bits
*
************************************************************************/
void arienco_done_encoding(EncodingEnvironmentPtr eep)
{

		Ebits_to_follow ++;
		if (Elow < FIRST_QTR)								  //  output_bit(0)
		{     
			Ebuffer >>= 1;
			if (--Ebits_to_go == 0)
				put_byte();
			
			while (Ebits_to_follow > 0) {
				Ebuffer >>= 1;
				Ebuffer |= 0x80;
				if (--Ebits_to_go == 0) 
					put_byte();
				Ebits_to_follow--;
			}
		}
		else																 //  output_bit(1)
		{                    
			Ebuffer >>= 1;
			Ebuffer |= 0x80;
			if (--Ebits_to_go == 0)
				put_byte();	
			
			while (Ebits_to_follow > 0)
			{
				Ebuffer >>= 1;
				if (--Ebits_to_go == 0) 
					put_byte();
				Ebits_to_follow--;
			}
    }
		if (Ebits_to_go != 8)
			Ecodestrm[(*Ecodestrm_len)++] = (Ebuffer >> Ebits_to_go);
				
}  


/************************************************************************
*
*  Name :				biari_encode_symbol()
*
*  Description: Actually arithmetic encoding of one binary symbol by using 
*								the symbol counts of its associated context model 
*
************************************************************************/
void biari_encode_symbol(EncodingEnvironmentPtr eep, signed short symbol, BiContextTypePtr bi_ct )
{

		int Elow_m1 = Elow - 1; 

		if( symbol != 0) 
		{

#if AAC_FRAC_TABLE
			Ehigh = Elow_m1 + ( ( ( Ehigh - Elow_m1 ) * 
													((bi_ct->cum_freq[1]*ARITH_CUM_FREQ_TABLE[bi_ct->cum_freq[0]])>>16))>>10);
#else
			Ehigh = Elow_m1 + ( ( Ehigh - Elow_m1 ) * bi_ct->cum_freq[1]) / bi_ct->cum_freq[0];
#endif

			bi_ct->cum_freq[1]++;

		}
		else
		{
#if AAC_FRAC_TABLE
				Elow += ((( Ehigh - Elow_m1 )  * ((bi_ct->cum_freq[1]*ARITH_CUM_FREQ_TABLE[bi_ct->cum_freq[0]])>>16))>>10);
#else
				Elow += ( ( Ehigh - Elow_m1 ) * bi_ct->cum_freq[1]) / bi_ct->cum_freq[0];
#endif
		}

		if (++bi_ct->cum_freq[0] >= bi_ct->max_cum_freq) 
			rescale_cum_freq(bi_ct);

		do
		{
			if (Ehigh < HALF)  //  output_bit(0)
			{    
				Ebuffer >>= 1;
				if (--Ebits_to_go == 0) 
					put_byte();
				
				while (Ebits_to_follow > 0) 
				{
					Ebits_to_follow--;
					Ebuffer >>= 1;
					Ebuffer |= 0x80;
					if (--Ebits_to_go == 0) 
						put_byte();
					
				}
			} 
			else 
				if (Elow >= HALF)  // output_bit(1)
				{     
					Ebuffer >>= 1;
					Ebuffer |= 0x80;
					if (--Ebits_to_go == 0) 
						put_byte();
					
					while (Ebits_to_follow > 0) 
					{
						Ebits_to_follow--;
						Ebuffer >>= 1;
						if (--Ebits_to_go == 0) 
							put_byte();
					}
					
					Ehigh -= HALF;
					Elow -= HALF;
				} 
				else 
					if (Elow >= FIRST_QTR && Ehigh < THIRD_QTR) 
					{
									Ebits_to_follow++;
									Ehigh -= FIRST_QTR;
									Elow -= FIRST_QTR;								
					} 
					else 								
						break;
								
			Elow <<= 1;
			Ehigh += Ehigh+1;
		}
		while (1);     
} 



/************************************************************************
*
*  Name :				biari_init_context()
*
*  Description: Initializes a given context with some pre-defined probabilities 
*								and a maximum symbol count for triggering the rescaling 
*
************************************************************************/
void biari_init_context( BiContextTypePtr ctx, int ini_count_0, int ini_count_1, int max_cum_freq )
{
	  
	ctx->in_use       = TRUE;
	ctx->max_cum_freq = max_cum_freq;
		
		
	ctx->cum_freq[1]  = ini_count_1;
	ctx->cum_freq[0]  = ini_count_0 + ini_count_1;
	
}  

/************************************************************************
*
*  Name :				biari_copy_context()
*
*  Description: Copies the content (symbol counts) of a given context 
*
************************************************************************/
void biari_copy_context( BiContextTypePtr ctx_orig, BiContextTypePtr ctx_dest )
{

	ctx_dest->in_use     =  ctx_orig->in_use;
	ctx_dest->max_cum_freq = ctx_orig->max_cum_freq;

	ctx_dest->cum_freq[1] = ctx_orig->cum_freq[1];
	ctx_dest->cum_freq[0] = ctx_orig->cum_freq[0];

}   

/************************************************************************
*
*  Name :				biari_copy_context()
*
*  Description: Prints the content (symbol counts) of a given context model 
*
************************************************************************/
void biari_print_context( BiContextTypePtr ctx )
{

	printf("0: %4d\t",ctx->cum_freq[0] - ctx->cum_freq[1]);
  printf("1: %4d",ctx->cum_freq[1]);

}  
 

/************************************************************************
*
*  Name :				rescale_cum_freq()
*
*  Description: Rescales a given context model by halvening the symbol counts
*
************************************************************************/
void rescale_cum_freq( BiContextTypePtr   bi_ct)
{

	int old_cum_freq_of_one = bi_ct->cum_freq[1];

  bi_ct->cum_freq[1] = (bi_ct->cum_freq[1] + 1) >> 1;
  bi_ct->cum_freq[0] = bi_ct->cum_freq[1] + 
												 ( ( bi_ct->cum_freq[0] - old_cum_freq_of_one + 1 ) >> 1);
}
 

