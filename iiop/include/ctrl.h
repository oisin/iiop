/*
 * Author:          Iona Technologies Plc
 *
 * Description:
 *	Control specific GIOP API. Includes initialisation of GIOP
 *	State and associated calls to change settings etc.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef CTRL_H
#define	CTRL_H

#ifndef CTRL_EXTERN
#define	CTRL_EXTERN	extern
#endif

/*
 * Prototypes for module: CTRL
 */
#ifndef GIOP_H
CTRL_EXTERN GIOPStatusT
GIOPInit(
	GIOPStateT 	*giop, 
	CDRCoderT 	*coder_p, 
	boolean 	is_server
);
#endif
/*
 * End of Prototypes.
 */
#undef	CTRL_EXTERN

#endif	/* !CTRL_H	This MUST be the last line in this file! */
