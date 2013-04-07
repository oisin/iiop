/*
 * Author:          Iona Technologies Plc.
 *
 * Description:
 *
 *	General IIOP Engine types and definitions. These are used by
 *	all interfaces.
 *
 *	Various  components may be  excluded  by defining the relevant
 *	'_EXCLUDE' macro prior to #including this header.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef TYP_H
#define	TYP_H

#include <stdlib.h>



#ifdef _WIN32
#include "WinWarn.h"
#endif



/*
 *	Standard IDL types for C.
 */
#ifndef IDL_EXCLUDE

typedef unsigned char	octet;
typedef unsigned char	boolean;

#endif

#ifdef _WIN32
typedef int		FdT;	/* standard file descriptor type */
#else
typedef short		FdT;	/* standard file descriptor type */
#endif

/*
 *	Global memory (de-)allocation function type signature
 */
#ifdef MEMCHECKRT
/*
 *	To coorospond to the itrt memfunctions
 *
 */
typedef void *	(*GIOPAllocFpT)(size_t size,char*,int);
typedef void 	(*GIOPDeallocFpT)(void *data_p,char*,int);
#else
typedef void *	(*GIOPAllocFpT)(size_t size);
typedef void 	(*GIOPDeallocFpT)(void *data_p);
#endif
/*
 *	True/false
 */
#ifndef true
#define true	1
#define false	0
#endif

/* size_t settings for memory allocation */

#ifndef GIOP_SIZET_MAX
#define	GIOP_SIZET_MAX		4294967295U 	/* max value of a "uint" */
#endif

/* May be either (QNX uses the _DEFINED_ macro) */

#ifdef _SIZE_T_DEFINED_
#define _SIZE_T
#endif

#ifdef _TYPE_size_t
#define _SIZE_T
#endif

#ifndef _SIZE_T
#define _SIZE_T
#ifndef TORNADO
typedef unsigned int    size_t;
#endif
#endif

/* Windows DLL macros */

#ifdef LIB_IIOP_AS_DLL
#ifdef BUILDING_IIOP_LIB
#define DLL_LINKAGE __declspec(dllexport)
#else
#define DLL_LINKAGE __declspec(dllimport)
#endif
#else
#define DLL_LINKAGE
#endif

#endif	/* !TYP_H	This MUST be the last line in this file! */
