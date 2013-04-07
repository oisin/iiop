/*
 * Author:          Iona Technologies Plc
 *
 * Description:
 *
 *	CDR  Coder public definitions. Refer  to the  cdr.c source for
 *	details on the coder functionality.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef CDR_H
#define	CDR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CDR_EXTERN
#define	CDR_EXTERN	extern
#endif

#include "typ.h"
#include "ver.h"

/* Need the GIOPStateT */

#ifndef GIOP_NO_AUTOFRAG
struct GIOPStateS;
#endif

/* CDR lengths, Limits */

#define CDR_MIN_BUFSZ		8	/* Min per-buffer size required */

/* Byte Order of this machine */

#ifdef CDR_BIG_ENDIAN
#define CDR_BYTE_ORDER      	0	/* BIG ENDIAN */
#else
#define CDR_BYTE_ORDER      	1	/* LITTLE ENDIAN */
#endif



/* Type sizes for CDR as the number of bytes */

#define CDR_SIZEOF_CHAR       1
#define CDR_SIZEOF_BYTE       1
#define CDR_SIZEOF_UCHAR      1
#define CDR_SIZEOF_OCTET      1

#define CDR_SIZEOF_SHORT      2
#define CDR_SIZEOF_USHORT     2

#define CDR_SIZEOF_ENUM       4
#define CDR_SIZEOF_LONG       4
#define CDR_SIZEOF_ULONG      4
#define CDR_SIZEOF_FLOAT      4

#define CDR_SIZEOF_DOUBLE     8
#define CDR_SIZEOF_LLONG      8


/* 
 *	CDR coder return status values
 */
#define CDR_OK			0
#define CDR_FAIL		1	/* general failure */

#define CDR_ENULL_CODER		2	/* null coder specified */
#define CDR_EINV_LEN		3	/* coded length is > buffer length */
#define CDR_EINV_MODE		4	/* invalid cdr mode (cdr_unknown) */
#define CDR_ENOSPACE		5	/* failed to allocate memory */
#define CDR_ENULL_RETURN	6	/* null return (out) param specified */
#define CDR_ENULL_DATA		7	/* null data (in) param specified */
#define CDR_EAUTO_FRAG		8	/* automatic fragmentation failed */
#define CDR_ESTR_FRAG		9	/* partial string (frag) found */

/* Return status type */

typedef unsigned long	    CDRStatusT;


/* 
 *	Coding mode 
 */
typedef enum
{
	cdr_encoding, cdr_decoding, cdr_unknown
}
        CDRModeT;


/*
 *	A  single coder buffer. This  contains a  memory block for the
 *	buffer and various settings as follows:
 *
 *	_buffer_p
 *			physical start of buffer address.
 *
 *	_pos_p
 *			is the current position  in the buffer for the
 *	next alignment or read/write
 *
 *	_len
 *			this is the total buffer length, including
 *	data space and (trailing) padding.
 *
 *	_used
 *			this is  the number  of bytes containing valid
 *	pad  and data  bytes.
 *
 *	_next_p, _prev_p
 *			next/previous buffer in the list.
 */
typedef struct CDRBufferS
{
	unsigned char *
		        cdrb_buffer_p;	/* start of buffer */
	unsigned char *
		        cdrb_pos_p;	/* current buffer position/index */

	unsigned long	cdrb_len;	/* physical buffer length */
	unsigned long	cdrb_used;	/* nbytes used so far */
	
	struct CDRBufferS
		        *cdrb_next_p;	/* next buffer */
	struct CDRBufferS
		        *cdrb_prev_p;	/* previous buffer */
}
        CDRBufferT;


/* 
 *	Function pointer types for automatic buffer [de-]allocation 
 */
typedef CDRBufferT 	*(*CDRAllocFpT)(size_t);
typedef void 		(*CDRDeallocFpT)(CDRBufferT *);


/*
 *	Main coder object. Manages the  buffer list for the coder  and
 *	contains callbacks for buffer allocation/de-allocation.   Also
 *	maintains total buffer usage and the byte order setting.
 */
typedef struct CDRCoderS
{
	CDRModeT	cdr_mode;	/* encoding, decoding, unknown? */

	CDRBufferT	*cdr_start_buf_p; /* start of buffer list */
	CDRBufferT	*cdr_buf_p;	/* current buffer */

	CDRAllocFpT	cdr_alloc_fp;   /* buffer allocation function */
	unsigned long	cdr_alloc_min;	/* Min nbytes required from alloc */
	CDRDeallocFpT	cdr_dealloc_fp; /* buffer deallocation function */

	unsigned long	cdr_buflen;	/* total buffer space in use */

	unsigned short	cdr_bytesex;	/* endian */

#ifndef GIOP_NO_AUTOFRAG
	unsigned long	cdr_maxlen;	/* Max buffer length before fragment */
	struct GIOPStateS
		        *cdr_giop_p;	/* My giop state (to fragment) */
#endif
}
        CDRCoderT;


#ifndef GIOP_NO_AUTOFRAG
#include "giop.h"
#endif


/* 
 *	Init flags (not used)
 */
#define	CDR_MANAGE_MEM	0x01


/*
 * Prototypes for module: CDR
 */

/* Init call */

CDR_EXTERN DLL_LINKAGE CDRStatusT
CDRInit(CDRCoderT *cod_p, unsigned short byte_sex, size_t alloc_min);

/* 
 * 	Control calls 
 */
CDR_EXTERN DLL_LINKAGE CDRModeT
CDRMode(CDRCoderT *cod_p, CDRModeT mode);

/* Reset buffer */

CDR_EXTERN DLL_LINKAGE CDRStatusT
CDRReset(CDRCoderT *cod_p, unsigned char in_use);

CDR_EXTERN DLL_LINKAGE CDRStatusT
CDRRewind(CDRCoderT *cod_p, unsigned char reset_length);

CDR_EXTERN DLL_LINKAGE void
CDRByteSex(CDRCoderT *cod_p, unsigned char order);

CDR_EXTERN DLL_LINKAGE CDRStatusT
CDRNeedBuffer(CDRCoderT *cod_p, size_t min_len);

CDR_EXTERN DLL_LINKAGE CDRStatusT
CDRAddBuffer(CDRCoderT *cod_p, CDRBufferT * buf_p);

CDR_EXTERN DLL_LINKAGE void
CDRAlloc(CDRCoderT *cod_p, CDRAllocFpT alloc_fp);

CDR_EXTERN DLL_LINKAGE void
CDRFree(CDRCoderT *cod_p);

CDR_EXTERN DLL_LINKAGE void
CDRDealloc(CDRCoderT *cod_p, CDRDeallocFpT dealloc_fp);

/* Buffer length */

CDR_EXTERN DLL_LINKAGE unsigned long
CDRBuflen(CDRCoderT *cod_p, unsigned char do_reset);


/* 
 * All of these encoding/decoding routines return CDR_OK
 * on success or an error return status.
 */

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeEnum(CDRCoderT *, unsigned long *);

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeOctet(CDRCoderT *cod_p, octet *oct_p);
CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeNOctet(
      CDRCoderT *cod_p, octet **oct_pp, unsigned long *oct_len_p
);

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeBool(CDRCoderT *cod_p, unsigned char *bool_p);

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeShort(CDRCoderT *cod_p, short *short_p);
CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeUShort(
      CDRCoderT *cod_p, unsigned short *ushort_p
);

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeLong(CDRCoderT *cod_p, long *long_p);
CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeULong(CDRCoderT *cod_p, unsigned long *ulong_p);

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeFloat(CDRCoderT *cod_p, float *flt_p);
CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeDouble(CDRCoderT *cod_p, double *dbl_p);

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeChar(CDRCoderT *cod_p, char *ch_p);
CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeUChar(CDRCoderT *cod_p, unsigned char *uch_p);

CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeString(CDRCoderT *cod_p, char **str_pp);
CDR_EXTERN DLL_LINKAGE CDRStatusT CDRCodeNString(
      CDRCoderT *cod_p, char **str_pp, unsigned long *strlen_p
);

DLL_LINKAGE CDRStatusT
CDREncapCreate(CDRCoderT *cod_p, octet byte_sex);

DLL_LINKAGE CDRStatusT
CDREncapEnd(
	CDRCoderT 	*cod_p, 
	octet 		**oct_pp, 
	unsigned long 	*octlen_p,
        GIOPAllocFpT	getmem
);

DLL_LINKAGE CDRStatusT
CDREncapInit(
	CDRCoderT 	*cod_p, 
	CDRBufferT 	*buf_p, 
	octet 		*data_p, 
	unsigned long 	data_len,
        CDRModeT	mode
);




/*
 * End of Prototypes.
 */
#undef	CDR_EXTERN

#ifdef __cplusplus
}
#endif

#endif	/* !CDR_H	This MUST be the last line in this file! */
