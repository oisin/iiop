/*
 * Author:          Iona Technologies Plc.
 *
 * Description:
 *
 *	TCP specific  control block. This  is used  by iiop and stores
 *	file descriptor,  buffer  size and  status  info  about a  tcp
 *	connection.
 *
 *  Copyright (c) 1993-7 Iona Technologies Plc.
 *             All Rights Reserved
 *
 */
#ifndef TCPCB_H
#define	TCPCB_H

#include "iiop.h"

/* TCPCtrlBlkS version */

#define TCPCB_MAJOR	1
#define TCPCB_MINOR	1


/*
 *	TCP  specific   control  block.     This  is stored   in   the
 *	GIOPCtrlBlkT.cb_profile_p member.
 *
 *	This is the only transport currently supported.
 */
typedef struct TCPCtrlBlkS
{
	octet		tcp_major;		/* TCP Ctrl Blk version */
	octet		tcp_minor;
	
	IIOPStatusT	tcp_status;		/* return status */

	/* unknown status value (if tcp_status = IIOP_EUNKNOWN) */

	unsigned long	tcp_unknown_status;	


	/* Listen and Read/Write sockets */

	FdT		tcp_msg_fd;
	FdT		tcp_listen_fd;

	/* Number of bytes last read/written */

	unsigned long	tcp_nbytes;

	/* Read/Write TCP internal buffer sizes */

	unsigned long	tcp_rd_bufsz;
	unsigned long	tcp_wr_bufsz;
}
        TCPCtrlBlkT;

#endif	/* !TCPCB_H	This MUST be the last line in this file! */
