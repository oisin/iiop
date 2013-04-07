/**************************************************************************
		 Copyright (c) 1993-5  IONA Technologies Ltd.
			  All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Iona Technologies

	The copyright notice above does not evidence any
	actual or intended publication of such source code.

	The information contained herein is TRADE SECRET and 
	PROPRIETARY INFORMATION of IONA Technologies Ltd.and is not to
	be disclosed without the express written consent of IONA
	Technologies Ltd.

*****************************************************************************/

#ifndef IIOPP_h
#define IIOPP_h


#include "PlTrace.h"
#include "iiop.h"
#include "tcpcb.h"
#include "encap.h"



#ifndef TORNADO
#ifdef QNX
#include <mem.h>
#else
#include <sys/types.h>
#include <memory.h>
#endif
#endif


#ifdef _WIN32
#include <time.h>
#include <errno.h>
#include <winsock.h>
#else  

#if     defined(QNX)
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#if     defined(TORNADO)
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
extern int errno;

#endif /* !TORNADO */
#endif /* !QNX */
#endif /* !WIN32 */


/* Macros for system socket calls */

#ifdef _WIN32

/* We have to define this to disable sends 
   that the SO_LINGER prooperties on the socket will work properly. 
 */
#define IIOP_SHUTDOWN(fd)   shutdown(fd,1)  /* 1 disables sends */   
#define IIOP_CLOSE 			closesocket

#define IIOP_CREATESOCKET 			socket
#define IIOP_BIND			bind
#define IIOP_CONNECT 		connect
#define IIOP_WRITE(fd,buf,sz) send(fd,buf,sz,0)
#define IIOP_READ(fd,buf,sz)		recv(fd, buf, sz, 0)
#define IIOP_SEND 			send
#define IIOP_RECV 			recv
#define IIOP_SELECT 			select
#define IIOP_ACCEPT 			accept
#define IIOP_LISTEN(fd,sz)		listen(fd, sz)

#define IIOP_GETHOSTBYNAME 		gethostbyname
#define E_INVALID_SOCKET 	INVALID_SOCKET

#else

#ifdef NOKIA9K

#define AF_INET			0
#define SOCK_STREAM		SDT_STREAM

/* The close is enough - no shutdown needed */
#define IIOP_SHUTDOWN(fd)			      
#define IIOP_CLOSE 			SocketCloseSend

#define IIOP_CREATESOCKET(fam,typ,n)	SocketCreate(typ)
#define IIOP_BIND			SocketBind
#define IIOP_CONNECT 		SocketConnect
#define IIOP_WRITE(fd,buf,sz) 	SocketSend(fd, buf, sz, 0, (SocketAddress *)0)
#define IIOP_READ(fd,buf,sz)		SocketRecv(fd, buf, sz, SOCKET_NO_TIMEOUT, \
					(SocketAddress *)0)
#define IIOP_SEND 			send
#define IIOP_RECV 			recv
#define IIOP_SELECT 			select
#define IIOP_ACCEPT(fd,addr,alen)	SocketAccept(fd, SOCKET_NO_TIMEOUT)
#define IIOP_LISTEN(fd,sz)		SocketListen(fd, sz)

#else

/* shutdown socket for send and recv */
#define IIOP_SHUTDOWN(fd)		shutdown(fd, 2)
#define IIOP_CLOSE 			close

#define IIOP_CREATESOCKET		socket
#define IIOP_BIND			bind
#define IIOP_CONNECT 		connect

#ifdef  TORNADO
#define IIOP_WRITE_TCP 			write
#define IIOP_READ_TCP 			read
#else
#define IIOP_WRITE 			write
#define IIOP_READ 			read
#endif

#define IIOP_SEND 			send
#define IIOP_RECV 			recv
#define IIOP_SELECT 			select
#define IIOP_ACCEPT 			accept
#define IIOP_LISTEN(fd,sz)		listen(fd, sz)

#define IIOP_GETHOSTBYNAME 		gethostbyname
#define E_INVALID_SOCKET 	0

#endif 
#endif 

#ifdef DEC
#define CHAR_CAST char*
#define SELECT_CAST int*
#else
#define CHAR_CAST const char*
#define SELECT_CAST fd_set*
#endif

#endif












