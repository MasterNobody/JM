/*!
 *************************************************************************************
 * \file biaridecod.c
 *
 * \brief
 *   Binary arithmetic decoder routines.
 *
 *   This modified implementation of the M Coder is based on JVT-U084 
 *   with the choice of M_BITS = 16.
 *
 * \date
 *    21. Oct 2000
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Detlev Marpe                    <marpe@hhi.de>
 *    - Gabi Blaettermann
 *    - Gunnar Marten
 *************************************************************************************
 */

#include "global.h"
#include "memalloc.h"
#include "biaridecod.h"


#define B_BITS    10      // Number of bits to represent the whole coding interval
#define HALF      0x01FE  //(1 << (B_BITS-1)) - 2
#define QUARTER   0x0100  //(1 << (B_BITS-2))

/*!
 ************************************************************************
 * \brief
 *    Allocates memory for the DecodingEnvironment struct
 * \return DecodingContextPtr
 *    allocates memory
 ************************************************************************
 */
DecodingEnvironmentPtr arideco_create_decoding_environment()
{
  DecodingEnvironmentPtr dep;

  if ((dep = calloc(1,sizeof(DecodingEnvironment))) == NULL)
    no_mem_exit("arideco_create_decoding_environment: dep");
  return dep;
}


/*!
 ***********************************************************************
 * \brief
 *    Frees memory of the DecodingEnvironment struct
 ***********************************************************************
 */
void arideco_delete_decoding_environment(DecodingEnvironmentPtr dep)
{
  if (dep == NULL)
  {
    snprintf(errortext, ET_SIZE, "Error freeing dep (NULL pointer)");
    error (errortext, 200);
  }
  else
    free(dep);
}

/*!
 ************************************************************************
 * \brief
 *    finalize arithetic decoding():
 ************************************************************************
 */
void arideco_done_decoding(DecodingEnvironmentPtr dep)
{
  (*dep->Dcodestrm_len)++;
#if(TRACE==2)
  fprintf(p_trace, "done_decoding: %d\n", *dep->Dcodestrm_len);
#endif
}

/*!
 ************************************************************************
 * \brief
 *    read one byte from the bitstream
 ************************************************************************
 */
unsigned int getbyte(DecodingEnvironmentPtr dep)
{     
#if(TRACE==2)
  fprintf(p_trace, "get_byte: %d\n", (*dep->Dcodestrm_len));
#endif
  return dep->Dcodestrm[(*dep->Dcodestrm_len)++];
}

/*!
 ************************************************************************
 * \brief
 *    read two bytes from the bitstream
 ************************************************************************
 */
unsigned int getword(DecodingEnvironmentPtr dep)
{
  int d = *dep->Dcodestrm_len;
#if(TRACE==2)
  fprintf(p_trace, "get_byte: %d\n", d);
  fprintf(p_trace, "get_byte: %d\n", d + 1);
#endif
  *dep->Dcodestrm_len += 2;
  return ((dep->Dcodestrm[d]<<8) | dep->Dcodestrm[d+1]);
}
/*!
 ************************************************************************
 * \brief
 *    Initializes the DecodingEnvironment for the arithmetic coder
 ************************************************************************
 */
void arideco_start_decoding(DecodingEnvironmentPtr dep, unsigned char *code_buffer,
                            int firstbyte, int *code_len)
{

  dep->Dcodestrm      = code_buffer;
  dep->Dcodestrm_len  = code_len;
  *dep->Dcodestrm_len = firstbyte;

  dep->Dvalue = getbyte(dep);
  dep->Dvalue = (dep->Dvalue << 16) | getword(dep); // lookahead of 2 bytes: always make sure that bitstream buffer
                                        // contains 2 more bytes than actual bitstream
  dep->DbitsLeft = 15;
  dep->Drange = HALF;

#if (2==TRACE)
  fprintf(p_trace, "value: %d firstbyte: %d code_len: %d\n", dep->Dvalue >> dep->DbitsLeft, firstbyte, *code_len);
#endif
}


/*!
 ************************************************************************
 * \brief
 *    arideco_bits_read
 ************************************************************************
 */
int arideco_bits_read(DecodingEnvironmentPtr dep)
{ 
  int tmp = ((*dep->Dcodestrm_len) << 3) - dep->DbitsLeft;

#if (2==TRACE)
  fprintf(p_trace, "tmp: %d\n", tmp);
#endif
  return tmp;
}


/*!
************************************************************************
* \brief
*    biari_decode_symbol():
* \return
*    the decoded symbol
************************************************************************
*/
unsigned int biari_decode_symbol(DecodingEnvironmentPtr dep, BiContextTypePtr bi_ct )
{
  unsigned int state = bi_ct->state;
  unsigned int bit   = bi_ct->MPS;
  unsigned int value = dep->Dvalue;
  unsigned int range = dep->Drange;
  int renorm = 1;
  unsigned int rLPS  = rLPS_table_64x4[state][(range>>6) & 0x03];

  range -= rLPS;

  if(value < (range << dep->DbitsLeft))   //MPS
  {
    bi_ct->state = AC_next_state_MPS_64[state]; // next state 

    if( range >= QUARTER )
    {
      dep->Drange = range;
      return (bit);
    }
    else 
      range <<= 1;

  }
  else         // LPS 
  {
    value -= (range << dep->DbitsLeft);
    bit ^= 0x01;

    if (!state)          // switch meaning of MPS if necessary 
      bi_ct->MPS ^= 0x01; 

    bi_ct->state = AC_next_state_LPS_64[state]; // next state 

    renorm = renorm_table_32[(rLPS>>3) & 0x1F]; 
    range = (rLPS << renorm);
  }

  dep->Drange = range;
  dep->DbitsLeft -= renorm;
  if( dep->DbitsLeft > 0 )
  { 
    dep->Dvalue = value;
    return(bit);
  } 

  dep->Dvalue = (value << 16) | getword(dep);    // lookahead of 2 bytes: always make sure that bitstream buffer
                                                 // contains 2 more bytes than actual bitstream
  dep->DbitsLeft += 16;

  return(bit);
}


/*!
 ************************************************************************
 * \brief
 *    biari_decode_symbol_eq_prob():
 * \return
 *    the decoded symbol
 ************************************************************************
 */
unsigned int biari_decode_symbol_eq_prob(DecodingEnvironmentPtr dep)
{
   int tmp_value;
   int value = dep->Dvalue;

  if(--(dep->DbitsLeft) == 0)  
  {
    value = (value << 16) | getword( dep );  // lookahead of 2 bytes: always make sure that bitstream buffer
                                             // contains 2 more bytes than actual bitstream
    dep->DbitsLeft = 16;
  }
  tmp_value  = value - (dep->Drange << dep->DbitsLeft);

  if (tmp_value < 0)
  {
    dep->Dvalue = value;
    return 0;
  }
  else
  {
    dep->Dvalue = tmp_value;
    return 1;
  }
}

/*!
 ************************************************************************
 * \brief
 *    biari_decode_symbol_final():
 * \return
 *    the decoded symbol
 ************************************************************************
 */
unsigned int biari_decode_final(DecodingEnvironmentPtr dep)
{
  unsigned int range  = dep->Drange - 2;
  int value  = dep->Dvalue;
  value -= (range << dep->DbitsLeft);

  if (value < 0) 
  {
    if( range >= QUARTER )
    {
      dep->Drange = range;
      return 0;
    }
    else 
    {   
      dep->Drange = (range << 1);
      if( --(dep->DbitsLeft) > 0 )
        return 0;
      else
      {
        dep->Dvalue = (dep->Dvalue << 16) | getword( dep ); // lookahead of 2 bytes: always make sure that bitstream buffer
                                                            // contains 2 more bytes than actual bitstream
        dep->DbitsLeft = 16;
        return 0;
      }
    }
  }
  else
  {
    return 1;
  }
}

/*!
 ************************************************************************
 * \brief
 *    Initializes a given context with some pre-defined probability state
 ************************************************************************
 */
void biari_init_context (int qp, BiContextTypePtr ctx, const char* ini)
{
  int pstate = ((ini[0]* qp )>>4) + ini[1];

  if ( pstate >= 64 )
  {
    pstate = imin(126, pstate);
    ctx->state = (uint16) (pstate - 64);
    ctx->MPS   = 1;
  }
  else
  {
    pstate = imax(1, pstate);
    ctx->state = (uint16) (63 - pstate);
    ctx->MPS   = 0;
  }
}

