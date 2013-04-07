/*
 * Author:          Iona Technologies Plc
 *
 * Description:
 *
 *	General Inter-ORB Protocol (GIOP) message formats and related
 *	macros etc.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef GIOP_H
#define	GIOP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GIOP_EXTERN
#define	GIOP_EXTERN	extern
#endif

#include "ver.h"

/*
 *	GIOP API specific status values. Used to indicate API
 *	success/failure.
 */
typedef unsigned long	GIOPStatusT;

struct GIOPStateS;

#include "typ.h"
#include "cdr.h"
#include "ior.h"
#include "encap.h"
#include "tcpcb.h"

/* Current GIOP and IIOP versions */

#include "ver.h"

/*
 *	Misc. GIOP lengths and limits
 */
#define GIOP_HDR_SZ		12		/* GIOP Header size */


/*
 *	Offsets in the GIOP Header flags field.
 */
#define GIOP_FLAGS_ENDIAN	0x1
#define GIOP_FLAGS_MOREFRAGS	0x2

/* 
 * 	Min Frag size is GIOP Header length, plus at least one 
 *	aligned datum of the longest length (8 bytes).
 */
#define GIOP_MIN_FRAGSZ		GIOP_HDR_SZ + CDR_SIZEOF_LONG

/* 
 *	Default Frag size 
 */
#define GIOP_DEF_FRAGSZ		1024


/*
 *	Auto   Fragment  flag  settings   (stored in    the GIOPStateT
 *	gs_afrag_flags member).
 */
#define GIOP_AFRAG_CLR		0x1	/* clear the specified flag(s) */

#define GIOP_AFRAG_ON		0x2	/* turn auto fragmentation on */
#define GIOP_AFRAG_STR		0x4	/* allow strings to be fragmented */
#define GIOP_AFRAG_MSGHDR	0x8	/* allow message headers to be frag. */


/*
 *	How many transport TAG types do we support?
 */
#define GIOP_TAG_MAX		1


/*-------------------------------------------------------------------------
 *	GIOP Status values
 */
#define GIOP_OK			0	/* successful return */

/*
 *	Error status values for params/limits etc
 */
#define GIOP_ENULL_STATE	1	/* null giop passed in */
#define GIOP_ENULL_CODER	2	/* null coder passed in */
#define	GIOP_ENULL_IOR		3	/* null IOR reference given */
#define GIOP_ENULL_TYP		4	/* null typ (out) param */
#define GIOP_ENULL_CB		5	/* null callback given */
#define GIOP_EINV_FRAGSZ	6	/* invalid frag sz given (too small) */
#define GIOP_ENULL_PARAM	7	/* null param supplied */

/*
 *	Error in call semantics
 */
#define GIOP_EINV_AGENT		20	/* invalid call for agent type */
#define GIOP_EINV_CALL		21	/* Invalid api call at this time */

/*
 *	General connection/transport errors.
 */
#define GIOP_ECLOSED		30	/* connection is closed */
#define GIOP_ENOSPACE		31	/* unable to obtain buffer space */
#define GIOP_EBADMAGIC		32	/* not a GIOP msg; bad magic string */
#define GIOP_EREVISION		33	/* unknown GIOP revision */
#define GIOP_ECONNECT		34	/* failed to connect */
#define	GIOP_ETRANSPORT		35	/* transport specific error */
#define GIOP_ENULL_PROFILE	36	/* bad profile */
#define	GIOP_ENULL_CTRLBLK	37	/* bad/null transport ctrl blk */
#define GIOP_ENOPROFILE		38	/* no profile to listen on */
#define GIOP_EINV_TAG		39	/* Invalid TAG specified */

/*
 *	Message specific errors.
 */
#define GIOP_EINV_MSGTYP	50	/* Invalid message type in giop hdr */
#define GIOP_EINV_MSGSZ		51	/* Inv. msg size, based on msg type */
#define GIOP_EEXCEPT		52	/* Exception returned in Reply */


/*--------------------------------------------------------------------
 *	Various   GIOP states. These apply    to incoming or  outgoing
 *	messages. GIOP uses this state  to determine legal calls  able
 *	to be made at any given time. Most calls are only allowed once
 *	the engine is in the IDLE state. This is  reached once it  has
 *	been initialised  (ie reached INIT state)  and then connected, 
 *	after which it is idle.
 */
#define GIOP_SUNINIT	0	/* Unitialised */
#define GIOP_SINIT	1 	/* Engine has been initialised */
#define GIOP_SOPEN	4	/* Engine is listening (pre accept state) */
#define GIOP_SIDLE	2	/* Engine is idle (ready for msg xfers) */
#define GIOP_SREQCR	3	/* Request being created */
#define GIOP_SREPCR	5	/* Reply being created */
#define GIOP_SLREPCR	6	/* LocateReply being created */
#define GIOP_SFRAGCR	7	/* Fragment being created */
#define GIOP_SMSGCR	8	/* Giop hdr created; now creating message */


/*
 * 	GIOP Message Type
 */
typedef enum GIOPMsgTypeE
{
	GIOPRequest 		= 0,	/* client */
	GIOPReply 		= 1,	/* server */
	GIOPCancelRequest 	= 2,	/* client */
	GIOPLocateRequest 	= 3,	/* client */
	GIOPLocateReply 	= 4,	/* server */
	GIOPCloseConnection 	= 5,    /* server */
	GIOPMessageError 	= 6,	/* both */
	GIOPFragment		= 7, 	/* both */
	GIOPUnknown		= 99
}
        GIOPMsgType;



/*
 *	GIOP Message Headers
 */
typedef struct GIOPMessageHeader_1_0S
{
	char	        magic[4];		/* "GIOP" */

	octet 		major;
	octet 		minor; 
	boolean		byte_order;       	/* 0 = big, 1 = little */
	octet		message_type;	       
	unsigned long	message_size;          
}
        GIOPMessageHeader_1_0T;


typedef struct GIOPMessageHeader_1_1S
{
	char	        magic[4];		/* "GIOP" */

	octet 		major;
	octet 		minor; 
	octet		flags;       	
	octet		message_type;	       
	unsigned long	message_size;          
}
        GIOPMessageHeader_1_1T;

/*
 *	Request Message format
 */
typedef struct GIOPRequestHeader_1_0S
{
	unsigned long		services_max;
	unsigned long		services_len;
	IORServiceContextT 	*services_p;

	unsigned long		request_id;
	unsigned char		response_expected;

	unsigned long		objkey_len;
	octet			*objkey_p;

	char 			*operation_p;

	unsigned long  		principal_max;
	unsigned long  		principal_len;
	octet	  		*principal_p;
}
        GIOPRequestHeader_1_0T;

typedef struct GIOPRequestHeader_1_1S
{
	unsigned long		services_max;
	unsigned long		services_len;
	IORServiceContextT 	*services_p;

	unsigned long		request_id;
	unsigned char		response_expected;
	octet			reserved[3];

	unsigned long		objkey_len;
	octet			*objkey_p;

	char 			*operation_p;

	unsigned long  		principal_max;
	unsigned long  		principal_len;
	octet	  		*principal_p;
}
        GIOPRequestHeader_1_1T;

/*
 *	Reply Message formats
 */
typedef enum
{
	GIOP_NO_EXCEPTION,
	GIOP_USER_EXCEPTION,
	GIOP_SYSTEM_EXCEPTION,
	GIOP_LOCATION_FORWARD
}
        GIOPReplyStatusType;

typedef struct GIOPReplyHeaderS
{
	unsigned long		num_services;
	IORServiceContextT 	*services_p;

	unsigned long		request_id;
	GIOPReplyStatusType	reply_status;
}
        GIOPReplyHeaderT;


/*
 *	CancelRequest Message format for both Request/LocateRequest.
 */
typedef struct GIOPCancelRequestHeaderS
{
	unsigned long	request_id;
}
        GIOPCancelRequestHeaderT;


/*
 *	LocateRequest Message format
 */
typedef struct GIOPLocateRequestHeaderS
{
	unsigned long	request_id;
	unsigned long	objkey_len;
	octet 		*objkey_p;
}
        GIOPLocateRequestHeaderT;


/*
 *	LocateReply Message format
 */
typedef enum
{
	GIOP_UNKNOWN_OBJECT,
	GIOP_OBJECT_HERE,
	GIOP_OBJECT_FORWARD
}
        GIOPLocateStatusType;

typedef struct GIOPLocateReplyHeaderS
{
	unsigned long		request_id;
	GIOPLocateStatusType	locate_status;
}
        GIOPLocateReplyHeaderT;


/*
 *	CloseConnection,  MessageError  and  Fragment  Messages do not
 *	have a message header. The  GIOP  Message header contains  the
 *	message type.
 */


/*------------------------------------------------------------------
 *	GIOP/IIOP Engine internal declarations.
 */


/*
 *	Message Info structure. This contains the version, type, size,
 *	request id and is_fragmented? flag  for the given message. The
 *	GIOP state maintains one of these for the most recent incoming
 *	and outgoing messages to be processed.
 */
typedef struct GIOPMsgInfoS
{
	octet		mi_major;	/* GIOP Message version */
        octet		mi_minor;
	
	GIOPMsgType	mi_type;	/* Msg type */
	unsigned long	mi_size;	/* Msg size in bytes */

	boolean		mi_is_frag;	/* Is this fragmented? */
}
        GIOPMsgInfoT;


/*
 *	The  control   profile contains a  transport  specific control
 *	block for each supported  transport. It is  stored in the GIOP
 *	Control Block as an  array, one entry  per transport TAG.  The
 *	currently used tag determines  which array entry and therefore
 *	which union member to use.
 */
typedef union GIOPCtrlProfileU
{
	TCPCtrlBlkT	cp_tcp;

	/* future transports.. */
}
        GIOPCtrlProfileT;


/*
 *	GIOP  Control Block.  This   holds the IOR,  object  key and a
 *	transport specific control block  for each transport supported
 *	(only IIOP at  present). These  transport ctrl blocks  contain
 *	connection specifics  (file descriptor,  buffer sizes etc  for
 *	TCP for example).
 */
typedef struct GIOPCtrlBlkS
{
	IORT		*cb_ior_p;	/* IOR */
	unsigned long	cb_ior_profile; /* Current ior profile index */

	unsigned long	cb_ior_policy;	/* try one or auto retry */

	GIOPCtrlProfileT		/* Transport specific ctrl blk(s) */
		        cb_profile[GIOP_TAG_MAX];

	unsigned long	cb_objkey_len;	/* Object key len/value */
	octet 		*cb_objkey_p;
}
        GIOPCtrlBlkT;


/*
 *	Main GIOP   structure  which contains all  information  for an
 *	agent (eg client or server application) to connect to and pass
 *	messages to/from another agent.  An instance of this structure
 *	(giop state) is defined in the  calling application and passed
 *	to each GIOP API call.
 */
typedef struct GIOPStateS
{
	/*
	 *	Current  GIOP version of  the connection. This is used
	 *	by client side  ONLY  to determine the version  of the
	 *	server, for example to allow a 1.1 client to talk to a
	 *	1.0 server.
	 */
	octet		gs_major;
        octet		gs_minor;

	/*
	 *	Message info for the most recent in and out message.
	 */
	GIOPMsgInfoT	gs_msg_in,
	                gs_msg_out;

	/*
	 *	auto_fragment indicates auto fragmentation of  Request
	 *	and Reply messages.
	 */
	octet		gs_afrag_flags;

	/*
	 *	frag_nbyte  is  the    caller  supplied       fragment
	 *	size. def_frag_nbyte is the Engine default value. This
	 *	is determined by the accept/connect api calls so that
	 *	transport specific optimal lengths can be specified.
	 *
	 *	These settings are used by the auto-fragmentor and may 
	 *	be used by calling code for explicit Fragment messages.
	 */
	unsigned long	gs_frag_nbyte;
	unsigned long	gs_frag_def_nbyte;
	
	boolean		gs_is_server;	/* Am I the server agent? */

	GIOPStatusT	gs_status;	/* Last known GIOP status */
	short		gs_state;	/* Current state of processing */
	
	unsigned long	gs_request_id;	/* current request Id */
	
	GIOPCtrlBlkT	gs_ctrl_blk;	/* IOR/Transport Connection info */

	CDRCoderT	*gs_coder_p;	/* CDR coder */

	/*
	 * Pointers into the coder - yuk.
	 */
	unsigned char	*gs_flags_p; 	/* Message flags field */
	unsigned char	*gs_msgsz_p; 	/* Message size field */
	unsigned char  	*gs_noresp_p; 	/* response_expected field */
}
        GIOPStateT;


/*
 *	Signature of transport specific calls which implement 
 *	the generic GIOP transport API.
 */
typedef GIOPStatusT	(*GIOPConnectFpT)(GIOPStateT *);
typedef GIOPStatusT	(*GIOPAcceptFpT)(GIOPStateT *);
typedef GIOPStatusT	(*GIOPRejectFpT)(GIOPStateT *);
typedef GIOPStatusT	(*GIOPListenFpT)(GIOPStateT *);
typedef GIOPStatusT	(*GIOPCloseFpT)(GIOPStateT *);

typedef GIOPStatusT	(*GIOPRecvFpT)(GIOPStateT *, unsigned long nbyte);
typedef GIOPStatusT	(*GIOPSendFpT)(GIOPStateT *);


/*
 *	Transport specific calls which implement the generic GIOP 
 *	transport API. A set will exist for each supported transport.
 */
typedef struct GIOPProtCallsS
{
	const char	*pc_name_p;	/* Transport type name */
	
	GIOPConnectFpT	pc_connect;
	GIOPAcceptFpT	pc_accept;
	GIOPRejectFpT	pc_reject;
	GIOPListenFpT	pc_listen;
	GIOPListenFpT	pc_stoplisten;
	GIOPCloseFpT	pc_close;

	GIOPSendFpT	pc_send;
	GIOPRecvFpT	pc_recv;
}
        GIOPProtCallsT;

/*
 * 	Global static transport mapping table.
 */
extern	 GIOPProtCallsT		GIOPProtTab[];


/*
 * Prototypes for module: GIOP
 */
GIOP_EXTERN DLL_LINKAGE GIOPStatusT
GIOPGetNextMsg(GIOPStateT *giop, GIOPMsgType *typ_p);

extern DLL_LINKAGE GIOPStatusT
GIOPInit(
	GIOPStateT 	*giop, 
	CDRCoderT 	*coder_p, 
	boolean 	is_server
);

/* Connection API */

extern DLL_LINKAGE GIOPStatusT
GIOPConnect(GIOPStateT *giop, IORT *ior_p, short policy);

extern DLL_LINKAGE GIOPStatusT
GIOPListen(GIOPStateT *giop, IORT *ior_p, IORProfileIdT tag);

extern DLL_LINKAGE GIOPStatusT
GIOPStopListen(GIOPStateT *giop);

extern DLL_LINKAGE GIOPStatusT
GIOPAccept(GIOPStateT *giop);

extern DLL_LINKAGE GIOPStatusT
GIOPReject(GIOPStateT *giop);

extern DLL_LINKAGE GIOPStatusT
GIOPCloseConnectionSend(GIOPStateT *giop);

/* Fragment API */

#ifndef GIOP_NO_AUTOFRAG
extern DLL_LINKAGE GIOPStatusT
GIOPAutoFrag(GIOPStateT *giop, octet flags, unsigned long nbyte);

extern DLL_LINKAGE unsigned long
GIOPAutoFragGetSize(GIOPStateT *giop);
#endif	/* !GIOP_NO_AUTOFRAG */

extern DLL_LINKAGE GIOPStatusT
GIOPFragCreate(GIOPStateT *giop);

extern DLL_LINKAGE GIOPStatusT
GIOPFragSend(GIOPStateT *giop, boolean more_fragments);

/* Reply API */

extern DLL_LINKAGE GIOPStatusT
GIOPLocateReplyCreate(
	GIOPStateT 	*giop, 
	octet		major,
	octet		minor,
	GIOPLocateStatusType 
	                status, 
	unsigned long 	request_id
);

extern DLL_LINKAGE GIOPStatusT
GIOPLocateReplySend(GIOPStateT 	*giop);

extern DLL_LINKAGE GIOPStatusT
GIOPReplyCreate(
	GIOPStateT 		*giop, 
	octet			major,
	octet			minor,
	GIOPReplyStatusType 	status,  
	unsigned long 		request_id,
	IORServiceContextListT  *scxt_p
);

extern DLL_LINKAGE GIOPStatusT
GIOPReplySend(GIOPStateT *giop, boolean more_fragments);

/* Request API */

extern DLL_LINKAGE GIOPStatusT
GIOPLocateRequestSend(
	GIOPStateT *giop, unsigned long objkey_len, octet *objkey_p
);

extern DLL_LINKAGE GIOPStatusT
GIOPRequestCreate(
	GIOPStateT 	*giop, 
	char 		*operation_p, 
	unsigned long	principal_len,
	octet		*principal_p,
	IORServiceContextListT
	                *scxt_p,  
	boolean 	no_response
);

extern DLL_LINKAGE GIOPStatusT
GIOPRequestSend(GIOPStateT *giop, boolean no_response, boolean more_fragments);

extern DLL_LINKAGE GIOPStatusT
GIOPCancelRequestSend(GIOPStateT *giop, unsigned long request_id);

extern DLL_LINKAGE GIOPStatusT
GIOPMessageErrorSend(GIOPStateT *giop);

/*
 * End of Prototypes.
 */
#undef	GIOP_EXTERN

#ifdef __cplusplus
}
#endif

#endif	/* !GIOP_H	This MUST be the last line in this file! */
