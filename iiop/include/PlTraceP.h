/*
// PlTraceP.h -  Trace API, private include
//
// Author	Craig Ryan
//
// Purpose
//	Defines private trace stuff.
*/

#if	!defined(PLTRACEP_H)
#define	PLTRACEP_H

#define PLTRACEP_EXTERN
#include	"PlTrace.h"

/*#include 	<std/iona.h>*/

#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>

typedef int	Bool;

/*
// Default trace flag recognised by the trace API.  
*/
#define _TRACE_FLAG     	"-t"
#define _TRACE_FLAGLEN  	2


/* Reserved trace identifiers.
// 	'all'	to specify ALL trace modules are active for the
// 		given trace level(s). Eg 'app -t a1' will activate 
// 		tracing for level 1 in all modules.
// 	'mod'	prefix all output with '<mod_id>: '.
// 	'info'	list all trace information. This is typically used 
// 		to determine all trace flags available.
// 	'exit'	exit program (eg after a 'list')
*/  
#define _TRACE_ALL		"all"
#define _TRACE_MOD		"mod"
#define _TRACE_INFO		"info"
#define _TRACE_EXIT		"exit"
#define _TRACE_ON		"on"
#define _TRACE_OFF		"off"
#define _TRACE_STAT		"stat"

/* Environ. var for setting trace levels
*/
#define _TRACE_FLAGS		"PL_TRACE"


/* Process PL_TRACE_INIT macro's. This will scan argv. Default is
// to ignore argv in case pogram has an equiv. option.
*/
#define _TRACE_INIT		"PL_TRACE_INIT"


/* -t is the default trace option. This alters the option to the
// specified string.
*/
#define _TRACE_NEW_FLAG		"PL_TRACE_FLAG"


/* Trace level range. 0 indicates all levels (1-9) are activated.
*/  
#define MIN_TRACE_LEVEL		0
#define MAX_TRACE_LEVEL		9


/* Trace level bitmask - bit 1 (level 0) to bit 10 (level 9).
*/
#define _TRACE_ALL_MASK	(1<<0)


/* TRACES 		max number of active traces.
// TRACE_IDLEN		max len of trace module identifier.
//			Note: must be the same as the '%<len>s' format
//			specified in the PL_TRACE_INFO macro.
// TRACE_OPTLEN		max length of each trace option string.
// TRACE_FLAGLEN	max length of the actual trace option (def. is '-t').
*/  
#define MAX_TRACES      	10
#define MAX_TRACE_IDLEN		8
#define MAX_TRACE_OPTLEN	64
#define MAX_TRACE_FLAGLEN	16


/* Trace table entry structure.
*/
#define TRACE_ENABLE	1
#define TRACE_DISABLE	2

typedef struct 
{
	char  		te_mod_id_p[MAX_TRACE_IDLEN];  /* module id string */
	char   		te_mod_len;		       /* mod id string len */
	unsigned int   	te_trace_mask;		       /* active levels */

	/*
	 * Status mask. An SET bit represents 
	 * that that bit/level is ENABLED.
	 */  
	unsigned int	te_stat_mask;
}
        TraceEntryT;

char	TraceFlag[MAX_TRACE_FLAGLEN]	= { '\0' };
int	TraceFlagLen			= 0;

/* Trace table and top index.  
*/  
TraceEntryT       TraceTab[MAX_TRACES];
int               TraceTop;


/* Used when _TRACE_ALL is active.  
*/  
Bool   	TraceAll;
int	TraceAllMask;


/* If _TRACE_MOD, _TRACE_INFO, _TRACE_EXIT specified.
*/  
Bool	TraceModPrefix;
Bool	TraceInfo;
Bool	TraceExit;


/* Valid trace option symbols. 
//	'-' 	is a range (1-3 etc). Alternate char ':' can be used.
//	'+' 	is inclusive (1+2 etc)
//	',' 	seperates module/level pairs (x1,y2-4 etc).
*/  
#define TRC_RANGE	'-'
#define TRC_ALT_RANGE	':'
#define TRC_INCLUDE	'+'
#define TRC_SEP		','


/* Trace option handling symbol values. Used by state table parser.
*/  
typedef enum {
	ST_TMODID = 0, 	/* Module id string */
	ST_TDIGIT,	/* Level digit */
	KS_RANGE, 	/* '-'/':' symbol */
	KS_INCLUDE,	/* '+' symbol */
	KS_EOL, 	/* End-of-line */
	KS_ERROR	/* Illegal/unknown symbol */
}
        TraceSymbolsT;

#endif 	/* PLTRACEP_H - This must be the LAST line in this file */
