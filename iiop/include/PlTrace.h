/* PlTrace.h - Tracing API.
//
// Author	Craig Ryan.
//
// Purpose
// 	Enable Tracing if PL_TRACING has been defined. If the
// 	global PL_DEBUG is defined, turn tracing on as well.
// 	If enabled, the PL_TRACE and PL_TRACE_INIT macro's enable 
// 	the API and print trace statements during app execution.
//
// 	The function PlTraceSet(char *flag_p) is provided in order
// 	for tracing to be explictly turned on during a debugging
// 	session. This would be the case if the main application does
// 	not have tracing activated, yet links with a library with
// 	tracing compiled in.
//
// Entry Points
//
// 	PlTraceInit()	Init trace settings.
// 	PlTraceSet()	Activate a trace.
// 	PlTraceOn()	True if a specific trace is set.
// 	PlTraceInfo()	True if trace info (help) has been defined.
*/

#if	!defined(PLTRACE_H)
#define	PLTRACE_H

#include	<stdio.h>
#include	"PlGlob.h"

#if 	defined(PL_DEBUG) || defined(TEST_DRIVER)
#ifndef PL_TRACING
#define	PL_TRACING
#endif
#endif

#ifndef PLTRACEP_EXTERN
#define PL_INIT(v)
#define PLTRACEP_EXTERN	extern
#else
#define PL_INIT(v)		= v
#endif

#ifndef PL_INIT
#endif

#if	defined(PL_TRACING)
			
PL_PUBLIC const char	*pl_null	PL_INIT("<null>");

PL_PUBLIC void	PlTraceInit(
			int *argc_p, char **argv_pp, 
			const char *trace_flag 
#ifdef CXX_COMPILER
			    = 0
#endif
		);

PL_PUBLIC int	PlTraceInfo(const char *mod_p);
PL_PUBLIC int	PlTraceOn(char *flag_p, char *file_p, int line_no);
PL_PUBLIC int	PlTraceSet(char *flag_p);

#define PL_TRACE_INIT(argc_p,argv_pp)   PlTraceInit(argc_p, argv_pp);
#define PL_TRACE_INIT_FLAG(argc_p,argv_pp,trace_flag)   \
	PlTraceInit(argc_p, argv_pp, trace_flag);

#define PL_TRACE(trc_flag,trc_stmt)     \
if (PlTraceOn(trc_flag, __FILE__, __LINE__)) \
        {printf("<TRC:(%s)> ",trc_flag); printf trc_stmt; printf("\n");}

#define PL_TRACE_INFO(mod_p,info_p)     if (PlTraceInfo(mod_p)) \
        printf("  %-8s\t%s\n", mod_p, info_p);

#else	/* !PL_TRACING */

#define PL_TRACE_INIT(argc_p,argv_pp)
#define PL_TRACE_INIT_FLAG(argc_p,argv_pp,trace_flag)
#define PL_TRACE(trc_flag,trc_stmt)

#endif	/* !PL_TRACING */

#endif 	/* PLTRACE_H - This must be the LAST line in this file */
