/*
 * Author:          Iona Technologies Plc
 *
 * Description:
 *	Connection specific GIOP API calls.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef CONXT_H
#define	CONXT_H

#include "giopP.h"

#ifndef CONXT_EXTERN
#define	CONXT_EXTERN	extern
#endif


/*
 * Prototypes for module: CONXT
 */
#ifndef GIOP_H
CONXT_EXTERN GIOPStatusT
GIOPConnect(GIOPStateT *giop, IORT *ior_p, short policy);

CONXT_EXTERN GIOPStatusT
GIOPListen(GIOPStateT *giop, IORT *ior_p, IORProfileIdT tag);

CONXT_EXTERN GIOPStatusT
GIOPAccept(GIOPStateT *giop);

CONXT_EXTERN GIOPStatusT
GIOPReject(GIOPStateT *giop);

CONXT_EXTERN GIOPStatusT
GIOPCloseConnectionSend(GIOPStateT *giop);
#endif
/*
 * End of Prototypes.
 */
#undef	CONXT_EXTERN

#endif	/* !CONXT_H	This MUST be the last line in this file! */
