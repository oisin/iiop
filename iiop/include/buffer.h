/*
 * Author:          Iona Technologies Plc.
 *
 * Description:
 *
 *	Default allocation/de-allocation routines for CDR Coder buffer
 *	memory management.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 */
#ifndef BUFFER_H
#define	BUFFER_H

#include "cdr.h"
#include "../demo/lib/demo.h"  /* If this contrib source is moved
								  this include path must be updated. */

/*
 *	This is only defined by the buffer.c source itself in order to
 *	define  the function  prototypes. All other  source (including
 *	customer source) which includes  this file does not  define it
 *	so that all function prototypes will be 'extern'.
 */
#ifndef BUFFER_EXTERN
#define	BUFFER_EXTERN	extern
#endif


/*
 *	Make use of memalign(UNIX)/getmem_aligned(Windows) to guarantee 8 byte alignment.
 */

#ifndef _WIN32
#define BUF_MALLOC(sz)	ITRT_MEMALIGN(8, sz)
#define BUF_FREE(buf)	ITRT_FREE(buf)
#else
#define BUF_MALLOC(sz)  ((void *)getmem_aligned(sz))
#define BUF_FREE(buf)	 delmem_aligned(buf)
#endif



/*
 * Prototypes for module: BUFFER
 */

BUFFER_EXTERN CDRBufferT *
buf_allocate(unsigned long min_bytes);

BUFFER_EXTERN void
buf_free(CDRBufferT *buf_p);

/*
 * End of Prototypes.
 */
#undef	BUFFER_EXTERN

#endif	/* !BUFFER_H	This MUST be the last line in this file! */
