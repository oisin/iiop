/*
 * Author:          Craig Ryan
 *
 * Description:
 *
 *	Private GIOP definitions, types etc.
 *
 * Copyright 1995 Iona Technologies Ltd.
 *	ALL RIGHTS RESERVED
 */
#ifndef GIOPP_H
#define	GIOPP_H

#ifndef GIOPP_EXTERN
#define	GIOPP_EXTERN	extern
#endif

#include "giop.h"

/*
 *	GIOP 1.1 only  -  the flags byte   contains a  byte_order  and
 *	more_fragments setting. The byte offset of the flags byte in  
 *	the GIOP header is also defined.
 */
#define GIOP_FLAGS_OFFSET	6
#define GIOP_ORDER_BIT		0x1	/* 0 value = big endian */
#define GIOP_FRAG_BIT		0x2	/* 0 value = no more frags */

#define GIOP_BITON(mask,bit)	mask |= bit
#define GIOP_BITOFF(mask,bit)	mask &= ~bit


/* Maximum request id value is the largest ulong number */

#define GIOP_MAX_REQID		4294967295UL

/*
 * Prototypes for module: 
 */
GIOPP_EXTERN void
giop_hdr_cr(
	GIOPStateT 	*giop, 
	GIOPMsgType 	typ, 
	octet	 	major, 
	octet		minor
);

GIOPP_EXTERN void
giop_serv_cxt(GIOPStateT *, IORServiceContextListT *);

GIOPP_EXTERN void
giop_profile(GIOPStateT *giop, unsigned long *prof_p, unsigned long *tag_p);

GIOPP_EXTERN void
giop_set_frags(GIOPStateT *giop, const boolean more_frags);

GIOPP_EXTERN void
giop_set_msgsz(GIOPStateT *giop, const unsigned long msg_sz);

GIOPP_EXTERN void
giop_msg_info(
	GIOPMsgInfoT 	*msg_p, 
	unsigned long 	msg_sz, 
	boolean 	is_frag
);

GIOPP_EXTERN void
giop_inc_id(unsigned long *req_id_p);

GIOPP_EXTERN void
giop_set_noresp(GIOPStateT *giop, const boolean no_resp);

#ifndef GIOP_NO_AUTOFRAG

GIOPP_EXTERN GIOPStatusT
giop_auto_frag(GIOPStateT *giop);

#endif

GIOPP_EXTERN void
giop_init_ctrl_blk(
	GIOPCtrlBlkT 	*obj_p,
	IORT		*ior_p,
	unsigned long	objkey_len,
	octet	 	*objkey_p
);

/*
 * End of Prototypes.
 */

#undef	GIOPP_EXTERN

#endif	/* !GIOPP_H	This MUST be the last line in this file! */
