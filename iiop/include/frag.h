/*
 * Author:          Iona Technologies Plc
 *
 * Description:
 *	Fragmentation specific GIOP API calls.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef FRAG_H
#define	FRAG_H


/* Minimum GIOP revision in which Fragments are supported */

#define FRAG_MIN_MAJOR		1
#define FRAG_MIN_MINOR		1


#ifndef FRAG_EXTERN
#define	FRAG_EXTERN	extern
#endif

/*
 * Prototypes for module: FRAG
 */
#ifndef GIOP_H
FRAG_EXTERN GIOPStatusT
GIOPAutoFrag(GIOPStateT *giop, boolean on, unsigned long nbyte);


FRAG_EXTERN unsigned long
GIOPAutoFragGetSize(GIOPStateT *giop);

FRAG_EXTERN GIOPStatusT
GIOPFragCreate(GIOPStateT *giop);

FRAG_EXTERN GIOPStatusT
GIOPFragSend(GIOPStateT *giop, boolean more_fragments);
#endif
/*
 * End of Prototypes.
 */
#undef	FRAG_EXTERN

#endif	/* !FRAG_H	This MUST be the last line in this file! */
