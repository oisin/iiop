/*
 * Author:          Craig Ryan
 *
 * Description:
 *
 *	CDR   Coder private definitions.     These definitions are not
 *	accessible by the calling  application.   Refer to the   cdr.c
 *	source for details on the coder functionality.
 *
 * Copyright 1995 Iona Technologies Ltd.
 *	ALL RIGHTS RESERVED
 */
#ifndef CDRP_H
#define	CDRP_H

#ifndef CDRP_EXTERN
#define	CDRP_EXTERN	extern
#endif

#include "cdr.h"
#include "giopP.h"
#include "PlTrace.h"

/* Included for memcpy in following macros and strlen*/
#include <string.h>

/* 
 * Byte swapping macro's 
 */
#define CDR_SWAP_2(x) { \
	unsigned short	_cdr_swap_sh_tmp; \
	swab(((char *)&x),   ((char *)&_cdr_swap_sh_tmp), 2); \
	x = (unsigned short) _cdr_swap_sh_tmp; \
	}

#define CDR_SWAP_4(x) { \
	unsigned long   _cdr_swap_tmp; \
	swab(((char *)&x) +0,  ((char *)&_cdr_swap_tmp) +2, 2); \
	swab(((char *)&x) +2,  ((char *)&_cdr_swap_tmp) +0, 2); \
	memcpy((char *)&x, (char *)&_cdr_swap_tmp, 4); \
	}

#define CDR_SWAP_8(x) { \
	unsigned long   _cdr_swap_tmp[2]; \
	swab(((char *)&x) +0,  ((char *)_cdr_swap_tmp) +6, 2); \
	swab(((char *)&x) +2,  ((char *)_cdr_swap_tmp) +4, 2); \
	swab(((char *)&x) +4,  ((char *)_cdr_swap_tmp) +2, 2); \
	swab(((char *)&x) +6,  ((char *)_cdr_swap_tmp) +0, 2); \
	memcpy((char *)&x, (char *)_cdr_swap_tmp, 8); \
	}

/* 
 *	Alignment offset of the 'sz' datum from current buffer 'pos_p'
 */
#define CDR_ALIGN_DELTA(pos_p,sz) ((((unsigned long)(pos_p + sz-1)) 	\
				 &~(sz-1))-((unsigned long)pos_p))


/* 
 *	Increment buffer by sz bytes
 */
#define CDR_INC(buf,sz)		buf += sz;


/*
 * Prototypes for module: CDRP
 */
static CDRStatusT
cdr_align(CDRCoderT *cod_p, unsigned long sz);

static CDRStatusT
cdr_encode_nbytes(
	CDRCoderT 	*cod_p, 
	char 		*str_p, 
	unsigned long 	sz, 
	unsigned long 	free, 
	boolean 	is_string
);
/*
 * End of Prototypes.
 */
#undef	CDRP_EXTERN

#endif	/* !CDRP_H	This MUST be the last line in this file! */
