/***************************************************************************
 *
 * Modul       :  rdopt_coding_state.h
 * Author      :  Heiko Schwarz
 * Date        :  17. April 2001
 *
 * Description :  Headerfile for storing/restoring coding state
 *                (for rd-optimized mode decision)
 * 	  
 **************************************************************************/

#ifndef _RD_OPT_CS_H_
#define _RD_OPT_CS_H_

#include "global.h"



typedef struct {

  /* important variables of data partition array */
  int                   no_part;
  EncodingEnvironment  *encenv;
  Bitstream            *bitstream;

  /* contexts for binary arithmetic coding */
  int                   symbol_mode;
  MotionInfoContexts   *mot_ctx;
  TextureInfoContexts  *tex_ctx;

  /* syntax element number and bitcounters */
  int                   currSEnr;
  int                   bitcounter[MAX_BITCOUNTER_MB];

  /* elements of current macroblock */
  int                   mvd[2][BLOCK_MULTIPLE][BLOCK_MULTIPLE][2];

} RDCodingState;



void  clear_coding_state   ();   /* delete structure   */
void  init_coding_state    ();   /* create structure   */
void  store_coding_state   ();   /* store parameters   */
void  restore_coding_state ();   /* restore parameters */

#endif


