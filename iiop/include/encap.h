/*
 * Author:          Iona Technologies Plc.
 *
 * Description:
 *
 *	Encapsulation function calls. These  are  part of the IOR  API
 *	and are IOR specific encapsulation calls. General purpose 
 *	encapsulation calls are provided in the CDR API.
 *
 *	An IIOP specific call is  also provided which encapsulates the
 *	given IIOP Profile Body data, which is generic to both 1.0 and
 *	1.1 versions of  IIOP. Following this  call the caller may (if
 *	an IIOP 1.1 profile) then directly  CDR code the remaining 1.1
 *	specific Tagged Component data.
 *
 * Copyright (c) 1993-7 Iona Technologies Plc.
 *            All Rights Reserved
 *
 */
#ifndef ENCAP_H
#define	ENCAP_H

#include "ior.h"
#include "cdr.h"

#ifndef ENCAP_EXTERN
#define	ENCAP_EXTERN	extern
#endif


/*
 * Prototypes for module: ENCAP
 */

DLL_LINKAGE IORStatusT
IOREncapIIOP(
	CDRCoderT 	*cod_p,
	octet 		*major_p, 
	octet 		*minor_p, 
	char 		**host_pp, 
	unsigned short 	*port_p, 
	unsigned long 	*objkey_len_p, 
	octet 		**objkey_pp
);

/*
 * End of Prototypes.
 */
#undef	ENCAP_EXTERN

#endif	/* !ENCAP_H	This MUST be the last line in this file! */
