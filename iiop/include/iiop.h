/*
 * Author:          Iona Technologies Plc
 *
 * Description:
 *
 *	Internet Inter-ORB Protocol (IIOP) transport specific functions
 *	used by the GIOP API for TCP/IP communication. These are a TCP/IP
 *	implementation of the generic GIOP Connection specific API calls
 *	and do not require direct application access.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef IIOP_H
#define IIOP_H

#ifdef __cplusplus
extern "C" {
#endif

/* GIOP/IIOP version macros */

#include "ver.h"


/* 
 *	IIOP return status values. These are stored in the tcp 
 *	control block of the GIOPStateT state. 
 *
 *	If a (TCP/IP) transport error occurs, the GIOP API calls 
 *	will return GIOP_ETRANSPORT and the caller can then access
 *	the tcp control block for the specific error status.
 */
#define IIOP_OK			0

#define IIOP_EINV_FD		1	/* invalid file descriptor */
#define IIOP_EINV_HOST		2	/* invalid host */
#define IIOP_EINV_IPADDR	3	/* invalid IP address */

#define IIOP_EBAD_SOCK		4	/* bad socket */
#define IIOP_EBAD_SOCKTYPE	5	/* bad socket type */
#define IIOP_EBAD_SOCKADDR	6	/* bad socket address */
#define IIOP_ESOCK_INUSE	7	/* socket already in use */
#define IIOP_EADDR_INUSE	8
#define IIOP_EBAD_INPUT		9	/* bad input */

#define IIOP_ECONN_REFUSED	10	/* connection refused */
#define IIOP_EIS_CONN		11	/* is already connected */	
#define IIOP_ECONN_CLOSED	12	/* conn. has been closed/reset */

#define IIOP_ENO_BUFFER		20
#define IIOP_EMSG_TOO_BIG	21	/* message is too large */
#define IIOP_EZERO_READ		22	/* zero read of data */

#define IIOP_ENOPERM		30	/* no permission to perform operation*/
#define IIOP_ETIMEDOUT		31	/* operation timed out */
#define IIOP_ENETWORK_ERROR	32	/* Network unreachable etc */
#define IIOP_EIO_ERROR		33
#define IIOP_EINTR		34	/* interrupt occured during operation*/
#define IIOP_ENO_BLOCK		35	/* O_NONBLOCK or O_NODELAY is set on
					   fd & operation would have blocked */

#define IIOP_EUNKNOWN		99	/* other unknown error */


/* IIOP return status type */

typedef unsigned long	IIOPStatusT;

#include "giop.h"		/* include giop structures */


/*--------------------------------------------------------------------
 *	IIOP function calls
 */

/*
 *	iiop_connect attempts to connect to a host and port. This info
 *	is  contained  in     the IOPIOR   structure   in   the   giop
 *	structure. Returns error status if it fails
 */
DLL_LINKAGE GIOPStatusT
iiop_connect(struct GIOPStateS *giop);
	
/*
 *	`iiop_send` attempts to write the  data contained in the Coder
 *	to   the  connected FD. All   info is  contained   in the giop
 *	structure.  Returns error status if it fails.
 */
DLL_LINKAGE GIOPStatusT 
iiop_send(struct GIOPStateS *giop);

/*
 *	`iiop_recv`  waits until  it    receives all the    characters
 *	expected from  the incoming  socket and  writes them  into the
 *	Coder.  Returns error status if it fails.
 */
DLL_LINKAGE GIOPStatusT
iiop_recv(struct GIOPStateS *giop, unsigned long charsToRead);


/*
 *	`iiop_accept` sets  up a new  socket and accepts  the incoming
 *	call on this socket. Returns error status if it fails
 */
DLL_LINKAGE GIOPStatusT
iiop_accept(struct GIOPStateS *giop);


/*
 *	`iiop_reject` rejects a new incoming connection. Returns error
 *	status if it fails
 */
DLL_LINKAGE GIOPStatusT
iiop_reject(struct GIOPStateS *giop);


/*
 *	`iiop_listen` sets   up  a socket   and sets  this to   be the
 *	listening  socket.  This socket   fires each time  an incoming
 *	connection occurs. Returns error status if it fails.
 */
DLL_LINKAGE GIOPStatusT
iiop_listen(struct GIOPStateS *giop);


/*     
 *	`iiop_stoplisten` closes the server side listen socket.
 */
DLL_LINKAGE GIOPStatusT
iiop_stoplisten(struct GIOPStateS *giop);


/*
 *	`iiop_close` function  attempts  to close  the connected  file
 *	descriptor  contained  in the  giop  structure.  Returns error
 *	status if input giop state has incorrect data  or in the wrong
 *	state.
 */
DLL_LINKAGE GIOPStatusT
iiop_close(struct GIOPStateS *giop);

#ifdef __cplusplus
}
#endif

#endif
