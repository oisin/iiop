/*
 * conxt.c - 
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *	GIOP API connection specific functions.
 * 
 *	Refer  to the giop.c source for  a description of the complete
 *	GIOP API.
 *
 *  Copyright 1995 Iona Technologies Ltd.
 * 	ALL RIGHTS RESERVED.
 */
#define CONXT_EXTERN
#include "conxt.h"


static 
GIOPStatusT
giop_connect_any(GIOPStateT *);


/*
 * GIOPConnect()
 *
 * Purpose:
 *
 *	Client only call.
 *
 *	Connect to a   server. The  first   IOR  entry  to provide   a
 *	successful connection is chosen.
 *
 * Returns:
 *	GIOP_OK		connected ok
 *	else		transport error or bad param.
 */
GIOPStatusT
GIOPConnect(GIOPStateT *giop, IORT *ior_p, short policy)
{
	if (!giop)
		return GIOP_ENULL_STATE;

	if (giop->gs_is_server)
		return GIOP_EINV_AGENT;
	
	if (!ior_p)
		return GIOP_ENULL_IOR;

	/*
	 *	Must  normally   be in the INIT (pre-IDLE)
	 *	state. Otherwise, if the current status is a trasnport
	 *	error or closed connection, we reset the state to INIT
	 *	assuming another connect call is needed.
	 */
	if (giop->gs_state != GIOP_SINIT)
	{
		if (
			(giop->gs_state >= GIOP_SIDLE)
			&& ((giop->gs_status == GIOP_ETRANSPORT)
			    || (giop->gs_status == GIOP_ECLOSED)
			)
		)
			giop->gs_state = GIOP_SINIT;
		else
			return GIOP_EINV_CALL;
	}

	/* Init the control block */

	giop_init_ctrl_blk(&giop->gs_ctrl_blk, ior_p, 0, 0);
	
#if 0
done in init_ctrl_blk call..
	/*
	 *	Set the connection Policy in the giop state. The policy
	 *	determines the  behaviour  when a connection  is  lost
	 *	(subsequent to this call). It may either automatically
	 *	retry  (AUTO_RETRY)  to  connect   or  just bomb   out
	 *	(TRY_ONE) if the inital connection is ever lost.
	 */
        giop->gs_ctrl_blk.cb_ior_p = ior_p;
	giop->gs_ctrl_blk.cb_ior_profile = 0;
#endif
	
	/*
	 *	Set the connection Policy in the giop state. The policy
	 *	determines the  behaviour  when a connection  is  lost
	 *	(subsequent to this call). It may either automatically
	 *	retry  (AUTO_RETRY)  to  connect   or  just bomb   out
	 *	(TRY_ONE) if the inital connection is ever lost.
	 */
 	giop->gs_ctrl_blk.cb_ior_policy = policy;

	/*
	 *	Connect using  any server entry   we have in  the IOR.
	 *	Note  that  the transport    level  will use   the IOR
	 *	revision  (from  the  transport specific  Profile Body
	 *	etc) to set the GIOP revision we  will use, by setting
	 *	the gs_major and gs_minor values in the GIOPStateT.
	 */
	if ((giop->gs_status = giop_connect_any(giop)) != GIOP_OK)
		return giop->gs_status;

	/* IDLE - ready to send messages */

	giop->gs_state = GIOP_SIDLE;

	return GIOP_OK;
}


/*
 * GIOPListen()
 *
 * Purpose:
 *
 *	Server call only.
 *
 *	Listen for  any attempts made by clients  to connect.  If such
 *	attempts  are   detected, the  server  may  either accept() or
 *	reject() them.
 *
 *	The IOR is passed  in for the server  to register in  the GIOP
 *	state. The  tag param indicates the   transport over which the
 *	server will listen. This requires that such a tag type profile
 *	exists in the specified IOR.
 *
 * Returns:
 *	GIOP_OK		detected client connect attempts.
 *	else		no clients, timeout or transport error.
 */
GIOPStatusT
GIOPListen(GIOPStateT *giop, IORT *ior_p, IORProfileIdT tag)
{
	unsigned long	entry;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Invalid IOR */

	if (!ior_p || !ior_p->ior_profiles_p)
		return GIOP_ENULL_IOR;

	if (giop->gs_state != GIOP_SINIT)
		return GIOP_EINV_CALL;

	/* Register the IOR */

        giop->gs_ctrl_blk.cb_ior_p = ior_p;

#if 0
may not be the first listen (ie the call will be ignored). So, dont trash 
the control block!

	/* Init the control block */

	giop_init_ctrl_blk(&giop->gs_ctrl_blk, ior_p, 0, 0);
#endif
	
	/*
	 *	Simple check that the tag is a valid transport tag. If
	 *	not the GIOP static transport  table will not  contain
	 *	transport entries  for  the  tag,  eg if  the  tag was
	 *	MULTIPLE_COMPONENT or other stupid tag.
	 */
	if (!GIOPProtTab[tag].pc_listen)
		return GIOP_EINV_TAG;
	
	/* Look for a 'tag' type transport profile body */

	for (entry = 0; entry < ior_p->ior_profiles_len; entry++)
		if (ior_p->ior_profiles_p[entry].tp_tag == tag)
			break;

	/*
	 *	If  we can't find a tagged  profile with the specified
	 *	tag, return GIOP_ENOPROFILE which signals that no such
	 *	tag entry exists. 
	 */
	if (entry == ior_p->ior_profiles_len)
		return GIOP_ENOPROFILE;
	
	/* found the required profile  - use this to listen on */

	giop->gs_ctrl_blk.cb_ior_profile = entry;
	
	/* Call the transport listen() function */

	if ((giop->gs_status = (*GIOPProtTab[tag].pc_listen)(giop)) != GIOP_OK)
		return giop->gs_status;
	
	/* Ready to accept connections.. */

	giop->gs_state = GIOP_SOPEN;
	
	return GIOP_OK;
}


/*
 * GIOPStopListen()
 *
 * Purpose:
 *
 *	This is a  server side call to stop  any further listening for
 *	client connection attempts. 
 *
 * Returns:
 *	GIOP_OK		ok
 */
GIOPStatusT
GIOPStopListen(GIOPStateT *giop)
{
	IORProfileIdT	tag;
	unsigned long	prof;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must at least be already listening */

	if (giop->gs_state < GIOP_SOPEN)
		return GIOP_EINV_CALL;

	giop_profile(giop, &prof, &tag);

	if (
		(giop->gs_status = (*GIOPProtTab[tag].pc_stoplisten)(giop))
		!= GIOP_OK
	)
		return giop->gs_status;

	giop->gs_state = GIOP_SIDLE;

	return GIOP_OK;
}


/*
 * GIOPAccept()
 *
 * Purpose:
 *
 *	Server call only.
 *
 *	Accept a connection attempt from a  client. This will follow a
 *	successful call to GIOPListen(). The connection will be opened
 *	to the client so that it may begin sending requests.
 *
 * Returns:
 *	GIOP_OK		opened connection to client ok
 *	else		bad param or transport error.
 */
GIOPStatusT
GIOPAccept(GIOPStateT *giop)
{
	unsigned long	prof;
	unsigned long	tag;

	if (!giop)
		return GIOP_ENULL_STATE;
	
	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;
	
	/*
	 *	Must  normally   be in the   OPEN (post-INIT/pre-IDLE)
	 *	state. Otherwise, if the current status is a trasnport
	 *	error or closed connection, we reset the state to OPEN
	 *	assuming another accept call is needed.
	 */
	if (giop->gs_state != GIOP_SOPEN)
	{
		if (
			(giop->gs_state >= GIOP_SIDLE)
			&& ((giop->gs_status == GIOP_ETRANSPORT)
			    || (giop->gs_status == GIOP_ECLOSED)
			)
		)
			giop->gs_state = GIOP_SOPEN;
		else
			return GIOP_EINV_CALL;
	}
	
	giop_profile(giop, &prof, &tag);

	if ((giop->gs_status = (*GIOPProtTab[tag].pc_accept)(giop)) != GIOP_OK)
		return giop->gs_status;

	/* Idle state - ready to accept messages */

	giop->gs_state = GIOP_SIDLE;

	return GIOP_OK;
}


/*
 * GIOPReject()
 *
 * Purpose:
 *
 *	Server call only.
 *
 *	Reject an attempt from a client to connect.
 *
 * Returns:
 *	GIOP_OK		call succeeded
 *	else		bad param or transport error.
 */
GIOPStatusT
GIOPReject(GIOPStateT *giop)
{
	unsigned long	prof;
	unsigned long	tag;

	if (!giop)
		return GIOP_ENULL_STATE;
	
	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the OPEN (post-INIT/pre-IDLE) state */

	if (giop->gs_state != GIOP_SOPEN)
		return GIOP_EINV_CALL;

	giop_profile(giop, &prof, &tag);

	/* Call the transport reject() function */

	giop->gs_status = (*GIOPProtTab[tag].pc_reject)(giop);

	return giop->gs_status;
}


/*
 * GIOPCloseConnectionSend()
 *
 * Purpose:
 *
 *	Client or Server call.
 *
 *	Close the connection which was previously established and is
 *	currently still valid. 
 *
 * Returns:
 *	GIOP_OK		closed connection ok
 *	else		transport error, no connection to close etc
 */
GIOPStatusT
GIOPCloseConnectionSend(GIOPStateT *giop)
{
	unsigned long	prof;
	unsigned long	tag;

	if (!giop)
		return GIOP_ENULL_STATE;
	
	/*
	 *	This happens if  they didn't first call  listen, prior
	 *	to the accept.
	 */
	if (!giop->gs_ctrl_blk.cb_ior_p)
		return GIOP_ENULL_IOR;

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;
	
	/* Get the current profile */

	giop_profile(giop, &prof, &tag);
	
	/*
	 *	If we are  the server, then  a CloseConnection message
	 *	is first sent  to  the  client  before we invoke   the
	 *	close() transport call.
	 */
	if (giop->gs_is_server)
	{
		giop_hdr_cr(
			giop, 
			GIOPCloseConnection, 
			giop->gs_major, 
			giop->gs_minor
		);
		
		if ((giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop)) != GIOP_OK)
			return giop->gs_status;
	}

	/* Call the transport close() function */

	if ((giop->gs_status = (*GIOPProtTab[tag].pc_close)(giop)) != GIOP_OK)
		return giop->gs_status;

	/* Back to the INIT/OPEN state */

	giop->gs_state = (short)(giop->gs_is_server ? GIOP_SOPEN : GIOP_SINIT);
	
	return GIOP_OK;
}



/*---------------------------------------------------------
 *	Internal Connection specific calls
 */


/* Try all ior profiles until one succeeds in a connect. */

static GIOPStatusT
giop_connect_any(GIOPStateT *giop)
{
	unsigned long 		num_profs;
	unsigned long		prof;		/* profile index being used*/
	
	IORT			*ior_p;
	IORTaggedProfileT	*prof_p;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	ior_p = giop->gs_ctrl_blk.cb_ior_p;
	num_profs = ior_p->ior_profiles_len;
	
	giop->gs_status = GIOP_ECONNECT;	/* if we return from 'for' */

	for (prof = 0; prof < num_profs; prof++)
	{
		if (!(prof_p = &ior_p->ior_profiles_p[prof]))
			return GIOP_ENULL_PROFILE;
		
		/* Valid tag? TAG_MAXIMUM is defined in ior.h */

		if (prof_p->tp_tag > TAG_MAXIMUM)
			return GIOP_EINV_TAG;
		
		/*
		 *	If the transport type   name is  null,  assume
		 *	it's    a  non-transport   tag   id, such   as
		 *	TAG_MULTIPLE_COMPONENTS etc.
		 */
		if (!GIOPProtTab[prof_p->tp_tag].pc_name_p)
			continue;

		/*
		 *	Set the current profile  index and attempt the
		 *	connect(). 
		 */
		giop->gs_ctrl_blk.cb_ior_profile = prof;

		if (
			(giop->gs_status = 
                           (*GIOPProtTab[prof_p->tp_tag].pc_connect)(giop)
                        ) 
			== GIOP_OK
		)
			return GIOP_OK;
	}
	
	giop->gs_status = GIOP_ECONNECT;

	return GIOP_ECONNECT;	/* every profile failed */
}

/*
 * ctrl.c - Control calls
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *	GIOP Control functions. These  are miscellaneous functions for
 *	admin of the GIOP Manager instance.
 *
 *	Refer  to the giop.c source for  a description of the complete
 *	GIOP API.
 * 
 *  Copyright (c) 1993-7 Iona Technologies Ltd.
 *             All Rights Reserved
 *  This is unpublished proprietary source code of Iona Technologies
 *  The copyright  notice  above  does not  evidence  any  actual or
 *  intended publication of such source code.
 */

#include "giopP.h"

#define CTRL_EXTERN
#include "ctrl.h"

/*
 * static prototypes 
 */
static void
giop_init_msginfo(
	GIOPMsgInfoT 	*msg_p, 
	GIOPMsgType 	typ, 
	unsigned long 	sz, 
	octet 		major, 
	octet 		minor
);

/*
 * End static prototypes 
 */


/*
 * GIOPInit()
 *
 * Purpose:
 *
 *	Initialise the GIOP state. This installs a  CDR coder and sets
 *	the agent type (client or server).  The default setting is for
 *	no automatic fragmentation. The request  id begins at zero and
 *	the  engine  will be  in   the  'SINIT' state.  This  means no
 *	connection has been established  and only  connection specific
 *	calls may therfore be called (before messages may be created).
 *
 * Returns:
 *	GIOP_OK		initialised ok
 *	else		bad params
 */
GIOPStatusT
GIOPInit(
	GIOPStateT 	*giop, 
	CDRCoderT 	*coder_p, 
	boolean 	is_server
)
{
	if (!giop)
		return GIOP_ENULL_STATE;

	giop->gs_state = GIOP_SUNINIT;		/* nothing initialised.. */

	if (!coder_p)
		return GIOP_ENULL_CODER;

	/* Install the coder */

	giop->gs_coder_p = coder_p;
	
	/* fragment settings */

	giop->gs_afrag_flags = 0;
	giop->gs_frag_nbyte = 0;
	giop->gs_frag_def_nbyte = GIOP_DEF_FRAGSZ;

	giop->gs_is_server = is_server;		/* Am I the server? */

	/* Lowest revision initially (until 1st listen/connect) */

	giop->gs_major = 1;
	giop->gs_minor = 0;

	giop->gs_status = GIOP_OK;
	giop->gs_state = GIOP_SINIT;		/* has been initialised.. */
	
	giop->gs_request_id = 0;

	/* Init IOR/Transport control block */

	giop_init_ctrl_blk(
		&giop->gs_ctrl_blk, 0, 0, 0
	);
	
	/* Init in/our message info */

	giop_init_msginfo(
		&giop->gs_msg_in, GIOPUnknown, 0, 
		GIOP_MAJOR, GIOP_MINOR
	);
	giop_init_msginfo(
		&giop->gs_msg_out, GIOPUnknown, 0, 
		GIOP_MAJOR, GIOP_MINOR
	);

	return GIOP_OK;
}


/*--------------------------------------------------------------------
 *	Internal GIOP calls used by the control api.
 */


static void
giop_init_msginfo(
	GIOPMsgInfoT 	*msg_p, 
	GIOPMsgType 	typ, 
	unsigned long 	sz, 
	octet 		major, 
	octet 		minor
)
{
	if (!msg_p)
		return;
	
	/* version */

	msg_p->mi_major = major;
	msg_p->mi_minor = minor;

	/* Message type/length */

	msg_p->mi_type = typ;
	msg_p->mi_size = sz;

	msg_p->mi_is_frag = 0;
}


/* Init the GIOPCtrlBlkT structure */

void
giop_init_ctrl_blk(
	GIOPCtrlBlkT 	*obj_p,
	IORT		*ior_p,
	unsigned long	objkey_len,
	octet	 	*objkey_p
)
{
	TCPCtrlBlkT	*tcp_p;
	
	if (!obj_p)
		return;
	
	obj_p->cb_ior_p = ior_p;
	obj_p->cb_ior_profile = 0;
	obj_p->cb_ior_policy = IOR_MP_TRYONE;

	/*obj_p->cb_profile_p = 0;*/

	obj_p->cb_objkey_len = objkey_len;
	obj_p->cb_objkey_p = objkey_p;

	/* Init the supported transport control blocks */

	tcp_p = &obj_p->cb_profile[TAG_INTERNET_IOP].cp_tcp;
	
	tcp_p->tcp_major = TCPCB_MAJOR;
	tcp_p->tcp_minor = TCPCB_MINOR;
	tcp_p->tcp_status = IIOP_OK;
	tcp_p->tcp_unknown_status = 0L;
	tcp_p->tcp_msg_fd = -1;
	tcp_p->tcp_listen_fd = -1;
	tcp_p->tcp_nbytes = 0;
	tcp_p->tcp_rd_bufsz = 0;
	tcp_p->tcp_wr_bufsz = 0;
}




/*
 * frag.c - Fragment message calls.
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *	Fragment  Message API.  The fragment  size  may be set by  the
 *	calling application if automatic fragmentation is active.
 *
 *	For  manual/explicit  creating  and sending of FragmentMessage
 *	types, the size is set when the fragment is sent.
 * 
 *	Automatic fragmentation is only implemented if the macro 
 *	GIOP_NO_AUTOFRAG is *NOT* set.
 *
 *  Copyright (c) 1993-7 Iona Technologies Ltd.
 *             All Rights Reserved
 *  This is unpublished proprietary source code of Iona Technologies
 *  The copyright  notice  above  does not  evidence  any  actual or
 *  intended publication of such source code.
 */

#include "giopP.h"

#define FRAG_EXTERN
#include "frag.h"


#ifndef GIOP_NO_AUTOFRAG

GIOPStatusT
GIOPAutoFrag(GIOPStateT *giop, octet flags, unsigned long nbyte)
{
	if (!giop)
		return GIOP_ENULL_STATE;

	if (!giop->gs_coder_p)
		return GIOP_ENULL_CODER;
	
	/* Not supported before GIOP 1.1 */

	if (
		(giop->gs_major < FRAG_MIN_MAJOR)
		|| (giop->gs_minor < FRAG_MIN_MINOR)
	)
		return GIOP_EREVISION;
	
	/* Set the nbyte value (zero means preserve current value) */

	if (nbyte)
	{
		/* Can't fragment less than GIOP Header size + <long> ! */

		if (nbyte < GIOP_MIN_FRAGSZ)
			return GIOP_EINV_FRAGSZ;

		/* Coder maxlen is only set if the giop frag_nbyte is */

		giop->gs_frag_nbyte = nbyte;
		giop->gs_coder_p->cdr_maxlen = nbyte;
	}
	
	/* Set/unset the flags setting */

	if (flags&GIOP_AFRAG_CLR)
		giop->gs_afrag_flags &= ~flags;
	else
		giop->gs_afrag_flags |= flags;

	/* REVISIT: AFRAG_STR and AFRAG_MSGHDR not supported in beta. */
	/* remove this for GA */

	giop->gs_afrag_flags &= ~(GIOP_AFRAG_STR|GIOP_AFRAG_MSGHDR);
	
	giop->gs_coder_p->cdr_giop_p = giop;

	return GIOP_OK;
}

unsigned long
GIOPAutoFragGetSize(GIOPStateT *giop)
{
	if (!giop)
		return GIOP_ENULL_STATE;

	return giop->gs_frag_nbyte;
}
#endif


/*
 * GIOPFragCreate()
 *
 * Purpose:
 *
 *	Create a Fragment message. This is the  manual method by which
 *	the    calling application  may   send FragmentMessages.  With
 *	automatic  fragmentation,  Request    and Reply  messages  are
 *	automatically fragmented beyond a set message size.
 *
 *	Manual  fragment message create  and  send calls may also make
 *	use of the  automatic setting to reduce  the need for multiple
 *	fragment create/sends, to one call, by letting the Engine  
 *	handle this.
 *
 * Returns:
 *	GIOP_OK		fragmented created
 *	else		error; null state or call not allowed at this time
 */
GIOPStatusT
GIOPFragCreate(GIOPStateT *giop)
{
	if (!giop)
		return GIOP_ENULL_STATE;

	/* Not supported before GIOP 1.1 */

	if (
		(giop->gs_major < FRAG_MIN_MAJOR)
		|| (giop->gs_minor < FRAG_MIN_MINOR)
	)
		return GIOP_EREVISION;

	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;

	/*
	 *	Code the GIOP Header.
	 */
	giop_hdr_cr(giop, GIOPFragment, giop->gs_major, giop->gs_minor);

	giop->gs_state = GIOP_SFRAGCR;

	return GIOP_OK;
}


/*
 * GIOPFragSend()
 *
 * Purpose:
 *
 *	Send the current fragment message. The more_fragments argument
 *	is true if there are more fragment messages to follow.
 *
 * Returns:
 *	GIOP_OK		sent ok
 *	else		bad param or invalid call at this time
 */
GIOPStatusT
GIOPFragSend(GIOPStateT *giop, boolean more_fragments)
{
	unsigned long	prof;
	unsigned long	tag;
	unsigned long	buf_len;

	if (!giop)
		return GIOP_ENULL_STATE;

	/* Must be in the middle of a FragmentCreate */

	if (giop->gs_state != GIOP_SFRAGCR)
		return GIOP_EINV_CALL;

	/* 
	 *	set the more_fragments bit. Initial value is 
	 *	already false so only set it if true.
	 */
	if (more_fragments)
		giop_set_frags(giop, more_fragments);

	/*
	 *	Set the message size in the GIOP header
	 */
	buf_len = CDRBuflen(giop->gs_coder_p, false) - GIOP_HDR_SZ;
	giop_set_msgsz(giop, buf_len);

	/* Update the current message info (req_id doesn't change if 0) */

	giop_msg_info(
		&giop->gs_msg_out, buf_len, more_fragments
	);

	/* Call the transport send() function */

	giop_profile(giop, &prof, &tag);

	giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop);

	giop->gs_state = GIOP_SIDLE;

	return giop->gs_status;
}

/*
 * giop.c - 
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *	GIOP API. The API consists of the following function groups:
 *
 *	     Message functions
 *		     create/send giop messages
 *	     Connection functions
 *		     initialise and control connection calls to the
 *		     transport.
 *	     Control functions
 *		     Misc calls to manage giop behaviour.
 *
 *	This file contains  the  get-message functionality, MessageError
 *	and the internal GIOP API.
 *
 *	Message specific APIs are contained in the Request (req.c),
 *	Reply (rep.c), Locate Request (locreq.c), LocateReply (locrep.c)
 *	and Fragment (frag.c) source files. 
 *
 *	The    connection functions are  located in the conxt.c   source 
 *	and the control	functions in ctrl.c source.
 * 
 *  Copyright 1995 Iona Technologies Ltd.
 * 	ALL RIGHTS RESERVED.
 */
#define GIOP_EXTERN
#define GIOPP_EXTERN
#include "giopP.h"

#ifdef TEST_DRIVER
#include "conxt.h"
#include "ctrl.h"
#endif

#include "PlTrace.h"

/*
 * GIOPGetNextMsg()
 *
 * Purpose:
 *
 *	Read the      next  incoming  message.    This    is   done in
 *	stages. Firstly, the giop header magic values is read ("GIOP")
 *	followed by the rest of the giop  header. Finally, the message
 *	contents are read. The coder is rewound for each stage.
 *
 *	Incoming messages are expected to be in 1.1 GIOP format unless
 *	we are a server receiving a 1.0 message from a 1.0 based client.
 *
 * Returns:
 *	GIOP_OK		read the message ok
 *	else		invalid message or tranpsort problem
 */
GIOPStatusT
GIOPGetNextMsg(GIOPStateT *giop, GIOPMsgType *typ_p)
{
	unsigned long	prof;	/* Relevent IOR profile */
	unsigned long	tag;	/* Profile tag */

	unsigned long	nbyte;	/* num bytes to code */

	GIOPRecvFpT	recv;
	CDRCoderT	*coder_p;
	octet		mag[4];
	octet		*magic_p;
	octet		flags;
	octet		msg_typ;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Init these, in case we return in error */

	if (typ_p)
		*typ_p = GIOPUnknown;

	giop->gs_msg_in.mi_type = GIOPUnknown;
	giop->gs_msg_in.mi_size = 0;
	
	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;
	
	/* 
	 *	Get the GIOP (IOR) profile entry and tag type 
	 *	we are dealing with.
	 */
	giop_profile(giop, &prof, &tag);

	/* 
	 *	Pointer to the transport specific recv() 
	 */
	recv = GIOPProtTab[tag].pc_recv;

	coder_p = giop->gs_coder_p;
	
	if (!coder_p->cdr_start_buf_p)
	{
		if (CDRAddBuffer(coder_p, 0) != CDR_OK)
			return GIOP_ENOSPACE;
	}
	else
	{
		CDRRewind(coder_p, true);
		CDRReset(coder_p, false);
	}
	
	/*
	 *	Get the GIOP Header magic string and and check it's valid
	 */
	if ((giop->gs_status = (*recv)(giop, 4)) != GIOP_OK)
		return giop->gs_status;
	
	CDRMode(coder_p, cdr_decoding);
	
	/* Is the message magic string "GIOP" ? */

	nbyte = 4;
	magic_p = mag;
	CDRCodeNOctet(coder_p, (octet **)&magic_p, &nbyte);
	
	if (
		(magic_p[0] == 'G') 
		&& (magic_p[1] == 'I') 
		&& (magic_p[2] == 'O') 
		&& (magic_p[3] == 'P')
	)
		;
	else
		return GIOP_EBADMAGIC;
	
	(void)CDRReset(coder_p, true);
	
	/*
	 *	Get the remainder of the  GIOP Header to establish the
	 *	message type and size. Since we have at least one buffer
	 *	by now, it will be at least 8 bytes long.
	 */
	if ((giop->gs_status = (*recv)(giop, 8)) != GIOP_OK)
		return giop->gs_status;

	CDRCodeOctet(coder_p, &giop->gs_msg_in.mi_major);
	CDRCodeOctet(coder_p, &giop->gs_msg_in.mi_minor);

	/* 
	 * Check compatability - we support up to our major/minor revision
	 *
	 * Note that if we are the client, we expect all reply messages to
	 * be returned with 1.1 giop version. This is an implementation 
	 * descision (not to support 1.0 only servers).
	 */
	if (giop->gs_msg_in.mi_major != GIOP_MAJOR)
		return GIOP_EREVISION;

	if (giop->gs_msg_in.mi_minor > GIOP_MINOR)
		return GIOP_EREVISION;

	/*
	 *	Either flags or byte_order bool follows 
	 */
	CDRCodeOctet(coder_p, &flags);

	if (giop->gs_msg_in.mi_minor == GIOP_MINOR)
	{
		boolean		bit_set;
		
		/* GIOP 1.1 - byte order bit */

		bit_set = (boolean) (flags & (octet) GIOP_ORDER_BIT ? 1 : 0);
		CDRByteSex(coder_p, bit_set);

		/* indicates if message is fragmented */

		bit_set = (boolean) (flags & (octet) GIOP_FRAG_BIT ? 1 : 0);
		giop->gs_msg_in.mi_is_frag = bit_set;
	}
	else
	{
		/* Assume GIOP 1.0 message */

	        CDRByteSex(coder_p, flags);

		/*
		 *	We  could  be  a 1.1  client  recieving  a 1.0
		 *	message from the server (MessageError etc). We
		 *	need to ensure we are using  1.0 if the server
		 *	is.
		 */
		if (!giop->gs_is_server)
			giop->gs_minor = giop->gs_msg_in.mi_minor;
	}	

	/*
	 *	Get the message type. Also assigns the out param type.
	 */
	CDRCodeOctet(coder_p, &msg_typ);

	/* Is it a valid message type? */

	if ((GIOPMsgType)msg_typ <= GIOPFragment)
	{
		/* Make sure the message type is valid for the revision */

		if (
			(giop->gs_minor < GIOP_MINOR)
			&& ((GIOPMsgType)msg_typ == GIOPFragment)
		)
			return GIOP_EINV_MSGTYP;

		/* Ok type - set the state settings and return typ */

		giop->gs_msg_in.mi_type = (GIOPMsgType)msg_typ;

		if (typ_p)
			*typ_p = (GIOPMsgType)msg_typ;
	}
	else
		return GIOP_EINV_MSGTYP;

	/*
	 *	Get the message size in bytes.
	 */
	CDRCodeULong(coder_p, &giop->gs_msg_in.mi_size);

	/* 
	 *	Get the message header and body (if any) 
	 */
	if (giop->gs_msg_in.mi_size)
	{
		long		pad;
		
		/* MessageError and CloseConnection must have a size of 0 */

		if (
			((GIOPMsgType)msg_typ == GIOPMessageError) 
			|| ((GIOPMsgType)msg_typ == GIOPCloseConnection)
		)
			return GIOP_EINV_MSGSZ;
		
		/* Get a large enough buffer */

		if (CDRNeedBuffer(
			coder_p, giop->gs_msg_in.mi_size + 4
		    ) != CDR_OK
		)
			return GIOP_ENOSPACE;

		/*
		 *	The 12 byte GIOP  header has been  read. Since
		 *	we rewind the  buffer, and this read needs  to
		 *	read from byte 13 to remain aligned, we add  a
		 *	4 byte pad value.
		 */
		CDRMode(coder_p, cdr_encoding);
		pad = 0;
		CDRCodeLong(coder_p, &pad);
		CDRMode(coder_p, cdr_decoding);
		
		giop->gs_status = (*recv)(giop, giop->gs_msg_in.mi_size);
	}
	
	return giop->gs_status;
}


/*
 * GIOPMessageErrorSend()
 *
 * Purpose:
 *	Send a MessageError message with our (1.1) revision. 
 *
 * Returns:
 *	GIOP_OK		sent ok
 *	else		error condition
 */
GIOPStatusT
GIOPMessageErrorSend(GIOPStateT *giop)
{
	unsigned long	prof;
	unsigned long	tag;

	PL_TRACE("req1",("GIOPMessageErrorSend(): Enter"));

	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;

	/*
	 *	Code the GIOP Header.
	 */
	giop_hdr_cr(giop, GIOPMessageError, giop->gs_major, giop->gs_minor);

	/* Call the transport send() function */

	giop_profile(giop, &prof, &tag);

	giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop);

	PL_TRACE("req1",("GIOPMessageErrorSend(): Exit"));

	return giop->gs_status;
}


/*---------------------------------------------------------------
 *	Internal GIOP calls, shared amongst all the GIOP source.
 *	Others may exist, but static to each file.
 */


/*
 * giop_profile()
 *
 * Purpose:
 *
 *	Determine  the  current   (transport) profile   in  use.  Also
 *	determines the tagged profile tag (eg TAG_INTERNET_IOP) of the
 *	entry.
 *
 * Returns:
 *	void
 */
void
giop_profile(GIOPStateT *giop, unsigned long *prof_p, unsigned long *tag_p)
{
	/* Relevent IOR profile */

	*prof_p = giop->gs_ctrl_blk.cb_ior_profile;

	/* Profile tag */

	*tag_p  = giop->gs_ctrl_blk.cb_ior_p->ior_profiles_p[*prof_p].tp_tag;
}


/*
 * giop_hdr_cr()
 *
 * Purpose:
 *	Code the GIOP message  header.   This consists of  the  "GIOP"
 *	magic  value followed  by message type,   size and byte  order
 *	information.
 *
 * Returns:
 *	void
 */
void
giop_hdr_cr(
	GIOPStateT 	*giop, 
	GIOPMsgType 	typ, 
	octet	 	major, 
	octet	 	minor
)
{
	CDRCoderT	*cod_p;
	unsigned char	flags;
	unsigned long	nbyte;
	const char 	*mag_p;
	octet		typ_oct;
	
	PL_TRACE("giop1",("giop_hdr_cr(): Enter"));

	cod_p = giop->gs_coder_p;
	
	CDRMode(cod_p, cdr_encoding);

	CDRRewind(cod_p, true);
	CDRReset(cod_p, false);
	
	/* Magic string */

	PL_TRACE("giop1",("giop_hdr_cr(): code magic string"));

	nbyte = 4;
	mag_p = "GIOP";
	CDRCodeNOctet(cod_p, (octet **)&mag_p, &nbyte);
	
	/* Version */

	PL_TRACE("giop1",("giop_hdr_cr(): code version"));

	CDRCodeOctet(cod_p, &major);
	CDRCodeOctet(cod_p, &minor);
	
	/* Code flags value */

	PL_TRACE("giop1",("giop_hdr_cr(): code flags"));

	flags = CDR_BYTE_ORDER;
	CDRCodeUChar(cod_p, &flags);
	giop->gs_flags_p = cod_p->cdr_buf_p->cdrb_pos_p - CDR_SIZEOF_UCHAR;
	
	/* Message type */

	PL_TRACE("giop1",("giop_hdr_cr(): code message type"));

	typ_oct = (octet)typ;
	CDRCodeOctet(cod_p, &typ_oct);
	
	PL_TRACE("giop1",("giop_hdr_cr(): code message size"));

	/* Store message size address and set initial size to zero */

	nbyte = 0L;
	CDRCodeULong(cod_p, &nbyte);
	giop->gs_msgsz_p = cod_p->cdr_buf_p->cdrb_pos_p - CDR_SIZEOF_LONG;

	/* Store the msg info */

	giop->gs_msg_out.mi_major = major;
	giop->gs_msg_out.mi_minor = minor;
	giop->gs_msg_out.mi_type = typ;

	giop->gs_msg_out.mi_size = 0;
	giop->gs_msg_out.mi_is_frag = false;

	PL_TRACE("giop1",("giop_hdr_cr(): Exit"));
}


/*
 * giop_serv_cxt()
 *
 * Purpose:
 *	Code the service context list. This may be used by the
 *	security or other levels.
 *
 * Returns:
 *	void
 */
void
giop_serv_cxt(GIOPStateT *giop, IORServiceContextListT *scxt_p)
{
	unsigned long		n, num;
	IORServiceContextT	*cxt_p;
	
	if (scxt_p)
		num = scxt_p->scl_contexts_len;
	else
		num = 0;
	
	/* (sequence) length */

	CDRCodeULong(giop->gs_coder_p, &num);
	
	if (!num)
		return;		       /* zero length sequence */
	
	/* code all contexts */

	for (n = 0; n < num; n++)
	{
		cxt_p = &(scxt_p->scl_contexts_p[n]);

		/* context id */

		CDRCodeULong(giop->gs_coder_p, &cxt_p->sc_context_id);
		
		/* context data length */

		CDRCodeULong(giop->gs_coder_p, &cxt_p->sc_context_len);

		/* context data */

		CDRCodeNOctet(
			giop->gs_coder_p, 
			&cxt_p->sc_context_data_p, 
			&cxt_p->sc_context_len
		);
	}
}


/*
 * giop_set_noresp()
 *
 * Purpose:
 *	Set the response_expected flag. This determines if it is a
 *	one-way operation or not.
 *
 * Returns:
 *	void
 */
void
giop_set_noresp(GIOPStateT *giop, const boolean no_resp)
{
	if (!giop->gs_noresp_p)
		return;
	
	/* Actually points to 'response_expected' flag */
	/* REVISIT - rename gs_noresp_p to resp_expected.. */

	*giop->gs_noresp_p = (unsigned char) !no_resp;
}


/* set the message size in the GIOP header */

void
giop_set_msgsz(GIOPStateT *giop, const unsigned long msg_sz)
{
	if (!giop->gs_msgsz_p)
		return;
	
	*(unsigned long *)giop->gs_msgsz_p = msg_sz;
}


/* Set the more_fragments bit. Assumes giop 1.1 flag field in header */

void
giop_set_frags(GIOPStateT *giop, const boolean more_frags)
{
	octet	flags;
	
	if (!giop->gs_flags_p)
		return;
	
	flags = *(octet *)giop->gs_flags_p;

	if (more_frags)
		GIOP_BITON(flags, GIOP_FRAG_BIT);
	else
		GIOP_BITOFF(flags, GIOP_FRAG_BIT);

	*(octet *)giop->gs_flags_p = flags;
}


/*
 * giop_inc_id()
 *
 * Purpose:
 *
 *	Increment  the Request Id value.  This  is increased from 0 to
 *	the MAX_LONG value and then   wraps. Highly unlikely to  clash
 *	:-)
 *
 * Returns:
 *	void
 */
void
giop_inc_id(unsigned long *req_id_p)
{
	if (!req_id_p)
		return;
	
	if (*req_id_p == GIOP_MAX_REQID)
		*req_id_p = 0;			/* wrap */
	else
		(*req_id_p)++;
}


/* Fill in the remaining msg info */

void
giop_msg_info(
	GIOPMsgInfoT 	*msg_p, 
	unsigned long 	msg_sz, 
	boolean 	is_frag
)
{
	if (!msg_p)
		return;
	
	/* GIOP state maintained message info */

	msg_p->mi_size = msg_sz;
	msg_p->mi_is_frag = is_frag;
}


/* Only if auto fragmentation is supported.. */

#ifndef GIOP_NO_AUTOFRAG


/*
 * giop_auto_frag()
 *
 * Purpose:
 *	Send the  current   message   and create a    new   (Fragment)
 *	message. Coding will continue  transparent to the caller.  The
 *	final RequestSend (etc) will then send the current fragment.
 *
 *	Note that if a Request was auto-fragmented, it is not possible
 *	to specify   no_response (ie  no-wait for one-way  operations)
 *	with the RequestSend() call since the original Request message
 *	has already been sent.
 *
 * Side Effects:
 *	Resets the coder and fills in the Fragment message header.
 *
 * Returns:
 *	GIOP_OK		send/create process completed ok
 *	else		error condition
 */
GIOPStatusT
giop_auto_frag(GIOPStateT *giop)
{
	unsigned long	prof;
	unsigned long	tag;
	unsigned long	buf_len;
	unsigned long	giop_state;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	/*
	 *	Return if fragmentation doesn't  apply to this message
	 *	type.
	 */
	switch (giop->gs_msg_out.mi_type)
	{
	case GIOPCancelRequest:
	case GIOPCloseConnection:
	case GIOPMessageError:
	case GIOPUnknown:
		return GIOP_OK;
	}
	
	/*
	 *	Set the message size in the GIOP header and set 
	 *	more_fragments bit on.
	 */
	buf_len = CDRBuflen(giop->gs_coder_p, false) - GIOP_HDR_SZ;
	giop_set_msgsz(giop, buf_len);
	giop_set_frags(giop, true);

	/* Call the transport send() function */

	giop_profile(giop, &prof, &tag);

	if ((giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop)) != GIOP_OK)
		return giop->gs_status;

	/*
	 *	Save the current giop state.
	 */
	giop_state = giop->gs_state;
	giop->gs_state = GIOP_SIDLE;
	
	/*
	 *	Create a new fragment message
	 */
	if ((giop->gs_status = GIOPFragCreate(giop)) != GIOP_OK)
		return giop->gs_status;

	giop->gs_msg_out.mi_type = GIOPFragment;
	
	/*
	 *	Restore the current giop state, so the fragment message
	 *	is transparent to the caller.
	 */
	giop->gs_state = (short) giop_state;

	return GIOP_OK;
}
#endif
/*
 * locrep.c - GIOP LocateReply message API.
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 * 
 *  Copyright 1995 Iona Technologies Ltd.
 * 	ALL RIGHTS RESERVED.
 */

#include "giopP.h"

/* #define LOCREP_EXTERN
#include "locrep.h" */


/*
 * GIOPLocateReplyCreate()
 *
 * Purpose:
 *
 *	Create a LocateReply Message.
 *
 * Returns:
 *	GIOP_OK		ok
 *	else		failed
 */
GIOPStatusT
GIOPLocateReplyCreate(
	GIOPStateT 	*giop, 
	octet		major,
	octet		minor,
	GIOPLocateStatusType 
	                status, 
	unsigned long 	request_id
)
{
	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be the server */

	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;

	/*
	 *	Code the GIOP Header and LocateReply Header.
	 */
	giop_hdr_cr(giop, GIOPLocateReply, major, minor);

	/*
	 *	Request id
	 */
	CDRCodeULong(giop->gs_coder_p, &request_id);

	/* Store the current request id in the giop state */

	giop->gs_request_id = request_id;

	/*
	 *	LocateReply status
	 */
	CDRCodeEnum(giop->gs_coder_p, (unsigned long *)&status);

	giop->gs_state = GIOP_SLREPCR;

	return GIOP_OK;	
}


GIOPStatusT
GIOPLocateReplySend(GIOPStateT 	*giop)
{
	unsigned long	prof;
	unsigned long	tag;
	unsigned long	buf_len;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be the server */

	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the middle of creating a LocateReply */

	if (giop->gs_state != GIOP_SLREPCR)
		return GIOP_EINV_CALL;

	/* 
	 *	Set the message size in the GIOP header
	 */
	buf_len = CDRBuflen(giop->gs_coder_p, false) - GIOP_HDR_SZ;
	giop_set_msgsz(giop, buf_len);

	/* Update the current message info */

	giop_msg_info(&giop->gs_msg_out, buf_len, false);

	/* 
	 *	Call the transport send() function 
	 */
	giop_profile(giop, &prof, &tag);

	giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop);

	giop->gs_state = GIOP_SIDLE;

	return giop->gs_status;
}
/*
 * locreq.c - GIOP LocateRequest message API.
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *      These are sent from client to server to determine the following
 *	with respect to an object reference:
 *	a) is the object valid?
 *	b) can the server accept request for this object
 *	c) or else what address can the object be found at
 * 
 *  Copyright 1995 Iona Technologies Ltd.
 * 	ALL RIGHTS RESERVED.
 */

#include "giopP.h"

/* #define LOCREQ_EXTERN
#include "locreq.h" */


/*
 * GIOPLocateRequestCreate()
 *
 * Purpose:
 *	Create a LocateRequest Message. This contains a LocateRequest
 *	header which is a request_id and object key.
 *
 * Returns:
 *	GIOP_OK		ok
 *	else		failed
 */
GIOPStatusT
GIOPLocateRequestSend(
	GIOPStateT *giop, unsigned long objkey_len, octet *objkey_p
)
{
	unsigned long	buf_len;
	unsigned long	prof;
	IORProfileIdT	tag;

	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be the client */

	if (giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;

	/*
	 *	Code the GIOP Header and LocateRequest Header.
	 */
	giop_hdr_cr(giop, GIOPLocateRequest, giop->gs_major, giop->gs_minor);

	/* Increment request id */

	giop_inc_id(&giop->gs_request_id);

	/* 
	 *	request id 
	 */
	CDRCodeULong(giop->gs_coder_p, &giop->gs_request_id);

	/* 
	 *	object key 
	 */
	CDRCodeULong(giop->gs_coder_p, &objkey_len);
	CDRCodeNOctet(giop->gs_coder_p, &objkey_p, &objkey_len);

	/* 
	 *	Set the message size.
	 */
	buf_len = CDRBuflen(giop->gs_coder_p, false) - GIOP_HDR_SZ;
	giop_set_msgsz(giop, buf_len);

	/* Update the current message info */

	giop_msg_info(&giop->gs_msg_out, buf_len, false);

	/*
	 *	Call the transport send() function 
	 */
	giop_profile(giop, &prof, &tag);

	giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop);

	return giop->gs_status;
}




/*
 * prottab.c - GIOP to Transport linkage table
 * 
 * Author:    Iona Technologies Ltd.
 * 
 * Description:
 *
 *	A static  table  of  supported GIOP transports.   Index values
 *	correspond to the  TAG_XXX_IOP macro  values defined in  CORBA
 *	2.0, in the Interoperable Object Reference section (10.6.2).
 * 
 *  Copyright 1995 Iona Technologies Ltd.
 * 	ALL RIGHTS RESERVED.
 */
#define PROTTAB_EXTERN

#include "giop.h"
#include "iiop.h"

/*
 *	If included into some other   source  file, the table may   be
 *	renamed. The default is to name it GIOPProtTab.
 */
#ifndef GIOP_PROTTAB
#define GIOP_PROTTAB	GIOPProtTab
#endif


/*
 *	GIOP transport linkage table.
 */
GIOPProtCallsT GIOP_PROTTAB[] = 
{
    /* transport index, name and table */

    {
	    "IIOP",
	    iiop_connect, 	iiop_accept,	iiop_reject, 	
	    iiop_listen,	iiop_stoplisten, iiop_close,	
	    iiop_send,		iiop_recv
    },

    /* NULL entry for TAG_MULTIPLE_COMPONENTS */

    {0,	0, 0, 0, 0, 0, 0, 0}
};


/*
 * rep.c - GIOP Reply message API.
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *      These are sent from client to server and they encode 
 *	object invocations.
 * 
 *  Copyright 1995 Iona Technologies Ltd.
 * 	ALL RIGHTS RESERVED.
 */

#include "giopP.h"

/* #define REP_EXTERN
#include "rep.h" */


/*
 * GIOPReplyCreate()
 *
 * Purpose:
 *
 *	Create a Reply Message. The GIOP version of the reply
 *	is matched to the Request version sent by the client.
 *
 * Returns:
 *	GIOP_OK		ok
 *	else		failed
 */
GIOPStatusT
GIOPReplyCreate(
	GIOPStateT 		*giop, 
	octet			major,
	octet			minor,
	GIOPReplyStatusType 	status,  
	unsigned long 		request_id,
	IORServiceContextListT  *scxt_p
)
{
	octet		aflags;		/* auto frag flags */
	unsigned char	check_sz;

	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be the server */

	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;
	
	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;

	/*
	 *	Code the GIOP Header and Reply Header.
	 */
	giop_hdr_cr(giop, GIOPReply, major, minor);

	aflags = giop->gs_afrag_flags;
	check_sz = false;
	
	/* Don't fragment message headers? */

	if ((aflags&GIOP_AFRAG_ON) && !(aflags&GIOP_AFRAG_MSGHDR))
	{
		/* 
		 *	Need to check message size before returning.
		 *	If > frag size, we fragment after the Request
		 *	Header.
		 */
		check_sz = true;
		giop->gs_afrag_flags &= ~GIOP_AFRAG_ON;
	}

	giop->gs_status = GIOP_OK;
	
	/*
	 *	Service Context list
	 */
	(void)giop_serv_cxt(giop, scxt_p);

	/*
	 *	Request id
	 */
	CDRCodeULong(giop->gs_coder_p, &request_id);

	/* Store the current request id in the giop state */

	giop->gs_request_id = request_id;
	
	/*
	 *	Reply status. One of:
	 *
	 *	      NO_EXCEPTION	params and op. results follow
	 *	      USER_EXCEPTION	encoded user exception follows
	 *	      SYSTEM_EXCEPTION	system exception follows
	 *	      LOCATION_FORWARD	location forward info follows
	 */
	if (CDRCodeEnum(giop->gs_coder_p, (unsigned long *)&status) != CDR_OK)
		giop->gs_status = GIOP_ENOSPACE;
	else
	{
#ifndef GIOP_NO_AUTOFRAG
		if (
			check_sz 
			&& (
				giop->gs_coder_p->cdr_buflen 
				> giop->gs_coder_p->cdr_maxlen
			)
		)
			giop->gs_status = giop_auto_frag(giop);
#endif
	}

	/* Restore auto frag flags */

	giop->gs_afrag_flags = aflags;

	if (giop->gs_status == GIOP_OK)
		giop->gs_state = GIOP_SREPCR;

	return giop->gs_status;
}


GIOPStatusT
GIOPReplySend(GIOPStateT *giop, boolean more_fragments)
{
	unsigned long	prof;
	unsigned long	tag;
	unsigned long	buf_len;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be the server */

	if (!giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the create-reply state */

	if (giop->gs_state != GIOP_SREPCR)
		return GIOP_EINV_CALL;

	/* 
	 * could now be either creating a Reply or creating 
	 * an automatic fragment. Check the message type..
	 */
	if (giop->gs_msg_out.mi_type == GIOPFragment)
	{
		giop->gs_state = GIOP_SFRAGCR;
		return GIOPFragSend(giop, more_fragments);
	}	

	/* 
	 *	set the more_fragments bit. Initial value is 
	 *	already false so only set it if true.
	 */
	if (more_fragments)
	{
		if (
			(giop->gs_major == GIOP_MAJOR)
			&& (giop->gs_minor == GIOP_MINOR)
		)
			giop_set_frags(giop, more_fragments);
	}
	
	/*
	 *	Set the message size in the GIOP header
	 */
	buf_len = CDRBuflen(giop->gs_coder_p, false) - GIOP_HDR_SZ;
	giop_set_msgsz(giop, buf_len);

	/* Update the current message info */

	giop_msg_info(&giop->gs_msg_out, buf_len, false);

	/* 
	 *	Call the transport send() function 
	 */
	giop_profile(giop, &prof, &tag);

	giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop);
	
	giop->gs_state = GIOP_SIDLE;

	return giop->gs_status;
}
/*
 * req.c - GIOP Request message API.
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *      These are sent from client to server and they encode 
 *	object invocations.
 * 
 *  Copyright 1995 Iona Technologies Ltd.
 * 	ALL RIGHTS RESERVED.
 */

#include "giopP.h"
#include "PlTrace.h"


/*
 * GIOPRequestCreate()
 *
 * Purpose:
 *
 *	Create a Request Message. GIOP 1.1 requests are used.
 *
 * Returns:
 *	GIOP_OK		ok
 *	else		failed
 */
GIOPStatusT
GIOPRequestCreate(
	GIOPStateT 	*giop, 
	char 		*operation_p, 
	unsigned long	prin_len,
	octet		*prin_p,
	IORServiceContextListT
	                *scxt_p,  
	boolean 	no_response
)
{
	unsigned long	objkey_len;
	CDRCoderT	*cod_p;
	boolean		response_expected;
	octet		res[3];
	octet		*res_p;
	octet		aflags;		/* auto frag flags */
	unsigned char	check_sz;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	PL_TRACE("req1",\
		("GIOPRequestCreate(): Enter; operation is '%s'",\
	        operation_p?operation_p:pl_null));

	/* Must be the client */

	if (giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;

	if (!operation_p)
		return GIOP_ENULL_PARAM;
	
	if (!(cod_p = giop->gs_coder_p))
		return GIOP_ENULL_CODER;
	
	response_expected = (boolean) !no_response;

	/*
	 *	Code the GIOP Header.
	 */
	giop_hdr_cr(giop, GIOPRequest, giop->gs_major, giop->gs_minor);

	aflags = giop->gs_afrag_flags;
	check_sz = false;
	
	/* Don't fragment message headers? */

	if ((aflags&GIOP_AFRAG_ON) && !(aflags&GIOP_AFRAG_MSGHDR))
	{
		/* 
		 *	Need to check message size before returning.
		 *	If > frag size, we fragment after the Request
		 *	Header.
		 */
		check_sz = true;
		giop->gs_afrag_flags &= ~GIOP_AFRAG_ON;
	}
	
	giop->gs_status = GIOP_OK;
	
	/* 
	 *	service context 
	 */
	(void)giop_serv_cxt(giop, scxt_p);

	/* Increment current giop request id */

	giop_inc_id(&giop->gs_request_id);

	PL_TRACE("req1",("GIOPRequestCreate(): code request id"));

	/* 
	 *	request id 
	 */
	CDRCodeULong(cod_p, &giop->gs_request_id);
	
	/* 
	 * 	response_expected field 
	 */
	CDRCodeBool(cod_p, &response_expected);

	/* 
	 *	save response_expected address (also settable during the Send) 
	 */
	giop->gs_noresp_p 
		= cod_p->cdr_buf_p->cdrb_pos_p - CDR_SIZEOF_BYTE;
	
	/*
	 *	Reserved 3 octets for GIOP 1.1 ONLY
	 */
	if (
		(giop->gs_major == GIOP_MAJOR)
		&& (giop->gs_minor == GIOP_MINOR)
	)
	{
		res[0] = res[1] = res[2] = 0;
		res_p = res;	/* casting res to octet ** causes core dump! */
		objkey_len = 3;	/* saves using another variable .. */
	
		PL_TRACE("req1",\
			("GIOPRequestCreate(): code 3 octet reserved field"));

		CDRCodeNOctet(cod_p, &res_p, &objkey_len);
	}

	/* 
	 *	Object Key (sequence<octet> encapsulation)
	 */
	objkey_len = giop->gs_ctrl_blk.cb_objkey_len;

	PL_TRACE("req1",\
		("GIOPRequestCreate(): code object key (len %d)",objkey_len));

	CDRCodeULong(cod_p, &objkey_len);
	CDRCodeNOctet(cod_p, &giop->gs_ctrl_blk.cb_objkey_p, &objkey_len);

	/* 
	 *	operation name 
	 */
	CDRCodeString(cod_p, &operation_p);
	
	CDRCodeULong(giop->gs_coder_p, &prin_len);
	if (CDRCodeNOctet(giop->gs_coder_p, &prin_p, &prin_len) != CDR_OK)
		giop->gs_status = GIOP_ENOSPACE;
	else
	{
#ifndef GIOP_NO_AUTOFRAG
		if (
			check_sz 
			&& (
				giop->gs_coder_p->cdr_buflen 
				> giop->gs_coder_p->cdr_maxlen
			)
		)
			giop->gs_status = giop_auto_frag(giop);
#endif
	}
	
	/* Restore auto frag flags */

	giop->gs_afrag_flags = aflags;

	if (giop->gs_status == GIOP_OK)
		giop->gs_state = GIOP_SREQCR;

	PL_TRACE("req1",("GIOPRequestCreate(): Exit"));

	return giop->gs_status;	
}


GIOPStatusT
GIOPRequestSend(GIOPStateT *giop, boolean no_response, boolean more_fragments)
{
	unsigned long	prof;
	unsigned long	tag;
	unsigned long	buf_len;
	
	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be the client */

	if (giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the middle of creating a Request */

	if (giop->gs_state != GIOP_SREQCR)
		return GIOP_EINV_CALL;
	
	PL_TRACE("req1",\
              ("GIOPRequestSend(): Enter; more_frags is %d", more_fragments));

	/* 
	 * Could now be either creating a request or creating 
	 * an automatic fragment. Check the message type.
	 *
	 * With automatic fragmentation set, and if the current
	 * message is actually a Fragment msg, no_response is
	 * ignored since the initial request has already been 
	 * sent!
	 */
	if (giop->gs_msg_out.mi_type == GIOPFragment)
	{
		giop->gs_state = GIOP_SFRAGCR;
		return GIOPFragSend(giop, more_fragments);
	}
	
	/*
	 *	Set no_response bit in GIOP Header. If it is a
	 *	fragment message, it's too late to set this in
	 *	the (already sent) Request message.
	 */
	giop_set_noresp(giop, no_response);
	
	/* 
	 *	set the more_fragments bit. Initial value is 
	 *	already false so only set it if true.
	 *
	 *	if its a 1.0 Request, ignore the setting.
	 */
	if (more_fragments)
	{
		if (
			(giop->gs_major == GIOP_MAJOR)
			&& (giop->gs_minor == GIOP_MINOR)
		)
			giop_set_frags(giop, more_fragments);
	}

	/*
	 *	Set the message size in the GIOP header
	 */
	buf_len = CDRBuflen(giop->gs_coder_p, false) - GIOP_HDR_SZ;
	giop_set_msgsz(giop, buf_len);

	/* Update the current message info */

	giop_msg_info(&giop->gs_msg_out, buf_len, more_fragments);

	/* Call the transport send() function */

	giop_profile(giop, &prof, &tag);

	giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop);

	PL_TRACE("req1",("GIOPRequestSend(): Exit"));

	giop->gs_state = GIOP_SIDLE;

	return giop->gs_status;
}


GIOPStatusT
GIOPCancelRequestSend(GIOPStateT *giop, unsigned long request_id)
{
	unsigned long	prof;
	unsigned long	tag;
	unsigned long	buf_len;

	if (!giop)
		return GIOP_ENULL_STATE;
	
	/* Must be the client */

	if (giop->gs_is_server)
		return GIOP_EINV_AGENT;

	/* Must be in the idle state */

	if (giop->gs_state != GIOP_SIDLE)
		return GIOP_EINV_CALL;

	giop_hdr_cr(giop, GIOPCancelRequest, giop->gs_major, giop->gs_minor);

	/* Code the request id */

	CDRCodeULong(giop->gs_coder_p, &request_id);
	
	/* 
	 *	Set message size in GIOP Header. This will just be
	 *	the request id.
	 */
	buf_len = CDRBuflen(giop->gs_coder_p, false) - GIOP_HDR_SZ;
	giop_set_msgsz(giop, buf_len);
	
	/* Update the current message info */

	giop_msg_info(&giop->gs_msg_out, buf_len, false);

	/* Call the transport send() function */

	giop_profile(giop, &prof, &tag);

	giop->gs_status = (*GIOPProtTab[tag].pc_send)(giop);
	
	return giop->gs_status;
}

/*
 * ior.c - IOR API.
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *	IOR API functions.  These  allow the  caller to create  an IOR
 *	structure and add Tagged Profile  data to it. It also provides
 *	calls  to convert   the  IOR to   the  stringified  form   and
 *	back.  This  is defined  in  the CORBA  2.0 spec  (see section
 *	10.6.5).
 *
 *	Memory allocation is done  using a caller supplied  allocation
 *	function pointer. This may be null in which case it is assumed
 *	the caller has pre-allocated space prior to the API call.
 * 
 *  Copyright (c) 1993-7 Iona Technologies Ltd.
 *             All Rights Reserved
 *  This is unpublished proprietary source code of Iona Technologies
 *  The copyright  notice  above  does not  evidence  any  actual or
 *  intended publication of such source code.
 */

#ifdef QNX
#include <mem.h>
#else
#ifndef TORNADO
#include <memory.h>
#endif /* TORNADO */
#endif /* QNX */

#include <string.h>

#define IOR_EXTERN
#include "ior.h"

#include "cdr.h"
#include "ver.h"
#include "PlTrace.h"


/* 
 *	If the (unsigned long) data length is greater 
 *	than the max size_t value, then the malloc(or
 *	memalign etc) will fail. We trap this.
 */
/* REVISIT - size_t is a long. May need to have this check for 8 byte
   long machines (osf/1 etc) */

#if 0
#define IOR_CHECK_SIZET(data_len)		\
		if (data_len > GIOP_SIZET_MAX)	\
			return IOR_ENOSPACE;
#endif

#define IOR_CHECK_SIZET(n)
	
/* static functions */

static unsigned char	
ior_hex_to_nyb(const char ch);

static void
ior_byte_to_hex(unsigned char byt, char *buf_p);

static IORStatusT
ior_to_ascii(const unsigned char *str_p, const unsigned long len);

#if 0

not needed..

static void
ior_code_iiop(CDRCoderT *cod_p, octet *body_p, unsigned long body_len);

static void
ior_code_tcomp(
	CDRCoderT *cod_p, unsigned long tlen, IORTaggedComponentT *tcomp_p
);

static void
ior_code_mcomp(
	CDRCoderT 			*cod, 
	IORMultipleComponentProfileT 	*mcomp_p
);
#endif


static unsigned long
ior_length(IORT *ior_p);



/* length of leading "IOR:" prefix in stringified IOR */	

#define IORHDR_LEN	4


/*
 * IORCreateIor()
 *
 * Purpose:
 *	Creates a new IOR structure, and  ensures enough space for the
 *	IOR to contain max_profiles Tagged Profiles.
 *
 *	The getmem  param specifies the  memory  allocation routine to
 *	use    for the  allocation  of  the  Tagged  Profile structures. 
 *
 * Returns:
 *	IOR_OK		created IOR OK
 *	IOR_ENULL_IOR	null ior pointer passed
 *	IOR_ENULL_TYPID	a type id was specified but a zero getmem was
 *			specified and the IOR type id field is null.
 */
IORStatusT 
IORCreateIor(
	IORT 		*ior_p, 
	char 		*typid_p, 
	unsigned long 	max_profiles, 
	GIOPAllocFpT 	getmem
)
{
	unsigned long		full_len;
	
	if (!ior_p)
		return IOR_ENULL_IOR;
	
    	if (!getmem) 
		return IOR_ENOSPACE;
	
	if (!max_profiles)
		return IOR_EZERO_PROFILE;


	full_len =  sizeof(IORTaggedProfileT) * max_profiles;

	/* Too large for malloc etc? */

	IOR_CHECK_SIZET(full_len);

#if 0
	/* allocate IORT structure */

	ior_p = (IORT *) getmem(sizeof(IORT));
#endif

	/* allocate Tagged Profile pointer */
#ifdef MEMCHECKRT
	ior_p->ior_profiles_p = (IORTaggedProfileT *) getmem(full_len,__FILE__,__LINE__);
#else
	ior_p->ior_profiles_p = (IORTaggedProfileT *) getmem(full_len);
#endif
	ior_p->ior_profiles_len = 0;
	ior_p->ior_profiles_max = max_profiles;

	/* Init the type id field */

	if (typid_p)
	{
#ifdef MEMCHECKRT
		ior_p->ior_type_id_p = (char *) getmem(strlen(typid_p) + 1,__FILE__,__LINE__);
#else
		ior_p->ior_type_id_p = (char *) getmem(strlen(typid_p) + 1);
#endif
		strcpy(ior_p->ior_type_id_p, typid_p);
	}
	else
		ior_p->ior_type_id_p = 0;

	return IOR_OK;
}


/*
 * IORAddTaggedProfile()
 *
 * Purpose:
 *
 *	Adds a  Tagged Profile to the  IOR.  The data specified  is an
 *	encapsulation octet stream of either an IIOP Profile Body or a
 *	Multiple Component  Profile.   The  caller  will  have   first
 *	encapsulated the data using the IOREncapXX calls.
 *
 *	If the getmem param  is null, the data  (address) will be used
 *	directly,  otherwise  the  data will be   copied into  a newly
 *	allocated memory  block   pointed  to by the    Tagged Profile
 *	profile_data.
 *
 * Returns:
 *	IOR_OK		added Tagged Profile ok
 *	IOR_ENULL_IOR	null IOR specified
 *	IOR_ENULL_DATA	null data specified
 *	IOR_EINV_TAG	invalid Tagged Profile tag specified
 *	IOR_ENOSPACE	no space for any more Tagged Profiles
 */
IORStatusT 
IORAddTaggedProfile(
	IORT 		*ior_p,
	IORProfileIdT 	tag,
	unsigned long 	data_len,
	octet 		*data_p,
	GIOPAllocFpT 	getmem
)
{
	IORTaggedProfileT	*tprof_p;

	if (!ior_p)
		return IOR_ENULL_IOR;

	if (!data_len || !data_p)
		return IOR_ENULL_DATA;
	
	if (tag > TAG_MAXIMUM)
		return IOR_EINV_TAG;

	/* No more room for the new profile */

	if (ior_p->ior_profiles_len >= ior_p->ior_profiles_max) 
		return IOR_ENOSPACE;

	/* update the current profile index and get profile pointer */

	tprof_p = &ior_p->ior_profiles_p[ior_p->ior_profiles_len++];

	tprof_p->tp_tag = tag;
	tprof_p->tp_profile_len = tprof_p->tp_profile_max = data_len;

	/*
	 *	If an GIOPAllocFpT is passed in,  this implies that the
	 *	profile data  is   to   be  copied into    the  Tagged
	 *	Profile. Otherwise  we  simply  use the  actual data_p
	 *	passed in.
	 */
	if (getmem)
	{
		/* Too large for malloc etc? */

		IOR_CHECK_SIZET(data_len);
#ifdef MEMCHECKRT
		tprof_p->tp_profile_data_p = (octet *) getmem((size_t)data_len,__FILE__,__LINE__);
#else
		tprof_p->tp_profile_data_p = (octet *) getmem((size_t)data_len);
#endif		
		memcpy(tprof_p->tp_profile_data_p, data_p, (size_t)data_len);
	}
	else
		tprof_p->tp_profile_data_p = data_p;

	return IOR_OK;
}


/*
 * IORToString()
 *
 * Purpose:
 *
 *	Converts   an  IOR  structure into  the  stringified  IOR form
 *	("IOR:<hex string>").  We  use the  getmem  param to  allocate 
 *	memory which  iorstr_pp  will  point to and  will contain  the 
 *	resulting string.
 *
 *	The approach taken is to CDR code the IOR  into the buffer and
 *	then,  starting from the last  byte,  coding each binary value
 *	into a  two char hex (string) value;  "A8" etc.  This means we
 *	will      end up with     twice     the  buffer length    when
 *	finished. Starting at the last byte means we  can use the same
 *	buffer by coding byte N into (2 * N)-1 and  2*N and so on down
 *	to byte 0.
 *
 * Returns:
 *	IOR_OK		converted IOR to string without error
 *	IOR_ENULL_IOR	null IOR passed
 *	IOR_ENOSPACE	string not big enough or getmem is null.
 *	IOR_EINV_TAG	invalid/unknown tag in IOR
 *	else return value from ior_to_ascii()
 */
IORStatusT
IORToString(IORT *ior_p, unsigned char **iorstr_pp, GIOPAllocFpT getmem)
{
	CDRCoderT	cod;
	CDRBufferT	cdr_buf;
	octet sex;
	unsigned long	len;
	register unsigned long	
		        idx;

	/* "IOR:" prefix in stringified IOR */

	octet		pref[IOR_PREFIX_LEN];
	octet		*pref_p;
	unsigned long	pref_len;
	

	if (!ior_p)
		return IOR_ENULL_IOR;
	
	if (!iorstr_pp || !getmem)
		return IOR_ENOSPACE;
	
	/* 
	 *	Allocate the string buffer. This is based on the 
	 *	calculated length of the CDR coded IOR.
	 */
	len = ior_length(ior_p);
	
	/*
	 *	The space required is double the IOR coded length 
	 *	(since we need two chars for each octet, for the 
	 *	ascii hex strings) plus the 'IOR:' prefix + 
	 *	terminating null.
	 */
	len += len + IORHDR_LEN + 1;

	/* Too large for ITRT_MALLOC etc? */

	IOR_CHECK_SIZET(len);
#ifdef MEMCHECKRT
	*iorstr_pp = (unsigned char *) getmem((size_t)len,__FILE__,__LINE__);
#else
	*iorstr_pp = (unsigned char *) getmem((size_t)len);
#endif

	CDREncapInit(&cod, &cdr_buf, *iorstr_pp, len, cdr_encoding);

	/* 
	 *	Add the "IOR:" prefix. Do not use CDRCodeNString() 
	 *	because we don't want a string length coded.
	 */
	pref_p = &pref[0];
	pref_len = IOR_PREFIX_LEN;
	memcpy((void *)pref_p, (void *)IOR_PREFIX, IOR_PREFIX_LEN);
	CDRCodeNOctet(&cod, &pref_p, &pref_len);
	
#if 0
	strcpy(*iorstr_pp, "IOR:");
	cdr_buf.cdrb_pos_p += IORHDR_LEN;
	cod.cdr_buflen = cdr_buf.cdrb_used = IORHDR_LEN;
#endif

	/* Byte order of the coded IOR */

	sex = CDR_BYTE_ORDER;
	CDRCodeOctet(&cod, &sex);

	/* code the IOR contents */

	CDRCodeString(&cod, &ior_p->ior_type_id_p);

	len = ior_p->ior_profiles_len;
	CDRCodeULong(&cod, &len);
	
	/*
	 *	Code the sequence of Tagged Profiles.
	 */
	for (idx = 0; idx < len; idx++)
	{
		
		/* Tagged Profile tag */

		CDRCodeULong(&cod, &ior_p->ior_profiles_p[idx].tp_tag);

		/* Profile data (sequence) length */

		CDRCodeULong(&cod, &ior_p->ior_profiles_p[idx].tp_profile_len);
		
		/*
		 *	Code  the profile_data  as  is,  since this is
		 *	already   an   encapsulation. This   will be a
		 *	nested encapsulation.
		 */
		CDRCodeNOctet(
			&cod, 
			&ior_p->ior_profiles_p[idx].tp_profile_data_p, 
			&ior_p->ior_profiles_p[idx].tp_profile_len
		);

#if 0

this is crap - the profile_data is already encapsulated..


		/* code the byte sex since this is an encapsulation */

		CDRCodeOctet(&cod, &sex);
		
		/*
		 *	Encode   profile    body   (IIOP)  or multiple
		 *	component. For internal  IOR use,  we set  the
		 *	profile_data   to  the address of  the profile
		 *	body  or multi  component profile. Stringified
		 *	IOR's  on the other   hand must be constructed
		 *	using the  rules for encapsulated  equivalents
		 *	since they will probably be used externally.
		 */
		switch (ior_p->ior_profiles_p[idx].tp_tag)
		{
		case TAG_INTERNET_IOP:
			ior_code_iiop(
				&cod, 
				ior_p->ior_profiles_p[idx].tp_profile_data_p,
				ior_p->ior_profiles_p[idx].tp_profile_len
			);
			break;
			
		case TAG_MULTIPLE_COMPONENTS:
			ior_code_mcomp(
				&cod,
				(IORMultipleComponentProfileT *)
				   ior_p->ior_profiles_p[idx].tp_profile_data_p
			);
			break;

		default:
			return IOR_EINV_TAG;
		}
#endif
	}

	/*
	 *	We  now convert the raw buffer  (beyond the 'IOR:') to
	 *	an ascii hex pair string.  This will double the buffer
	 *	length.
	 */
	return ior_to_ascii(
		*iorstr_pp + IORHDR_LEN, 
		cod.cdr_buflen - IORHDR_LEN
	);
}


/*
 * IORFromString()
 *
 * Purpose:
 *	Convert a stringified IOR to an IORT structure. The format of
 *	the string must be "IOR:<hex string>". 
 *
 *	We  need the  getmem  param to  allocate  the  IOR struct and 
 *	Tagged Profiles.
 *
 *	The  IOR  string is first   converted from the  ascii hex pair
 *	values  into  a binary  octet   stream.  We therfore  need  to
 *	allocate  an aligned buffer  to hold  the binary octet  stream
 *	from which we will decode the IOR data. Once finished, we need
 *	to  use the delmem  param to  release  it. Only  the allocated
 *	memory for the IOR itself is returned.
 *
 * Returns:
 *	IOR_OK		succeeded
 *	IOR_FAIL	failed
 */
IORStatusT
IORFromString(
	IORT 		*ior_p, 
	unsigned char 	*iorstr_p, 
	GIOPAllocFpT 	getmem,
	GIOPDeallocFpT 	delmem
)
{
#define MAX_IORLEN	256

	octet		*buf_p;
	CDRCoderT	cod;
	CDRBufferT	cdr_buf;
	char		*typid_p;
	
	unsigned long	len;
	register unsigned long	
		        idx;
	IORTaggedProfileT
		        *prof_p;
	
	if (!ior_p)
		return IOR_ENULL_IOR;
	
	if (!iorstr_p)
		return IOR_ENULL_DATA;
	
	if (!getmem || !delmem)
		return IOR_ENOSPACE;
	
	/* Must begin with "IOR:" */

	if (
		(iorstr_p[0] == 'I')
		&& (iorstr_p[1] == 'O') 
		&& (iorstr_p[2] == 'R')
		&& (iorstr_p[3] == ':')
	)
		;
	else
		return IOR_EINV_IORSTR;
	
	/* 
	 *	Length of binary octet stream, ignoring the "IOR:" prefix.
	 *	Divide by two because each binary octet is represented as
	 *	a two byte ascii hex pair ("A8" etc).
	 */
	iorstr_p += IORHDR_LEN;
	len = (unsigned long)strlen((char *)iorstr_p) / 2L;

	/* Too large for ITRT_MALLOC etc? */

	IOR_CHECK_SIZET(len);

#ifdef MEMCHECKRT
	buf_p = (octet *) getmem((size_t)len,__FILE__,__LINE__);
#else
	buf_p = (octet *) getmem((size_t)len);
#endif

	/* 
	 * Convert the string into a binary octet stream. This involves
	 * converting the ascii hex pair string into binary.
	 */
	for (idx = 0; idx < len; idx++)
	{
		buf_p[idx] = (octet) (ior_hex_to_nyb(iorstr_p[0]) << 4);
		buf_p[idx] |= ior_hex_to_nyb(iorstr_p[1]);
		
		iorstr_p += 2;
	}

	/* Set up a coder with the encapsulation data */

	CDREncapInit(&cod, &cdr_buf, buf_p, len, cdr_decoding);

#if 0
	/* 
	 *	Now decode the string as an encapsulation
	 */
	CDRCodeUChar(&cod, &sex);
	CDRByteSex(&cod, sex);
#endif
	
	typid_p = 0;

	/* type-id string (also, re-use len) */

	CDRCodeNString(&cod, &typid_p, &len);

#if 0
	if (!*ior_pp)
		*ior_pp = (IORT *) getmem(sizeof(IORT));
#endif

	if (typid_p)
	{
#ifdef MEMCHECKRT
		ior_p->ior_type_id_p = (char *) getmem(strlen(typid_p) + 1,__FILE__,__LINE__); 
#else
		ior_p->ior_type_id_p = (char *) getmem(strlen(typid_p) + 1); 
#endif

		memcpy(ior_p->ior_type_id_p, typid_p, len);
	}
	else
		ior_p->ior_type_id_p = 0;


	/* Num of profiles (also re-use len variable) */

	len = 0;
	CDRCodeULong(&cod, &len);
	ior_p->ior_profiles_len = len;

	/* Just in case we have no profiles.. */

	if (len)
	{
		unsigned long	full_len;
		
		full_len = len * sizeof(IORTaggedProfileT);

		/* Too large for ITRT_MALLOC etc? */

		IOR_CHECK_SIZET(full_len);

#ifdef MEMCHECKRT
		prof_p = ior_p->ior_profiles_p = (IORTaggedProfileT *) getmem((size_t)full_len,__FILE__,__LINE__) ;
#else
		prof_p = ior_p->ior_profiles_p = (IORTaggedProfileT *) getmem((size_t)full_len) ;
#endif
	}
	
	/* 
	 *	Sequence of Tagged Profiles 
	 */
	for (idx = 0; idx < len; idx++) 
	{
		octet		*oct_p;
		unsigned long	plen;
		
		/* tag */

		CDRCodeULong(&cod, &prof_p[idx].tp_tag);

		/* profile data (sequence) len */

		CDRCodeULong(&cod, &plen);
		prof_p[idx].tp_profile_len = prof_p[idx].tp_profile_max = plen;
		
		/* profile data */

		CDRCodeNOctet(&cod, &oct_p, &plen);

		/* Too large for ITRT_MALLOC etc? */

		IOR_CHECK_SIZET(plen);

#ifdef MEMCHECKRT
		prof_p[idx].tp_profile_data_p = (octet *)getmem((size_t)plen,__FILE__,__LINE__); 
#else
		prof_p[idx].tp_profile_data_p = (octet *)getmem((size_t)plen); 
#endif
		memcpy(prof_p[idx].tp_profile_data_p, oct_p, (size_t)plen);
	}

	/* Release the encapsulation buffer we used to decode the IOR */

#ifdef MEMCHECKRT
	delmem(buf_p,__FILE__,__LINE__);
#else
	delmem(buf_p);
#endif
	
	return IOR_OK;
}


void
IORFree(IORT *ior_p, GIOPDeallocFpT delmem)
{
	unsigned long i;
	
	if (!ior_p || !delmem)
		return;
	
	if (ior_p->ior_type_id_p)
#ifdef MEMCHECKRT
		delmem(ior_p->ior_type_id_p,__FILE__,__LINE__);
#else
		delmem(ior_p->ior_type_id_p);
#endif
	
	if (!ior_p->ior_profiles_p)
		return;
	
	for (i = 0; i < ior_p->ior_profiles_len; i++)
	{
		if (ior_p->ior_profiles_p[i].tp_profile_data_p)
#ifdef MEMCHECKRT
			delmem(ior_p->ior_profiles_p[i].tp_profile_data_p,__FILE__,__LINE__);
#else
			delmem(ior_p->ior_profiles_p[i].tp_profile_data_p);
#endif
	}
	
#ifdef MEMCHECKRT
	delmem(ior_p->ior_profiles_p,__FILE__,__LINE__);
#else
	delmem(ior_p->ior_profiles_p);
#endif
}


/* ------------- static functions --------------- */

/*
 *	internal  call  to convert  hex   strings (2  chars) to binary
 *	values.
 */
static unsigned char
ior_hex_to_nyb(const char ch)
{
	register unsigned char out;
	
	out = 0;
	
	switch (ch)
	{
	case '1': case '2': case '3': case '4': case '5':
	case '6': case '7': case '8': case '9': case '0':
		out = (unsigned char) (ch - '0');
		break;

	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		out = (unsigned char) (ch - 'A' + 10);
		break;

	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		out = (unsigned char) (ch - 'a' + 10);
		break;

	default:
		return 0;
	}

	return out;
}

/* convert a byte value to a 2 char hex string */

static void
ior_byte_to_hex(unsigned char byt, char *str_p)
{
	const char *hex_chars = "0123456789abcdef";

	str_p[0] = hex_chars[ (byt >> 4) & 0x0f ];
	str_p[1] = hex_chars[ byt & 0x0f];
}


#if 0
/*
 *	Code an IIOP 1.1 Profile Body into the  coder. This is part of
 *	the stringified IOR.
 */
static void
ior_code_iiop(CDRCoderT *cod_p, octet *body_p, unsigned long body_len)
{
	CDRCoderT	cod;
	CDRBufferT	buf;
	
	octet		major, minor;
	char		*host_p;
	unsigned short	port;
	unsigned long	objkey_len;
	octet		*objkey_p;

	unsigned long	tc_len;
	IORTaggedComponentT
	   	        *tc_p;
	
	unsigned long	len, idx;

	if (!body_p)
		return;

	/* Create a temp coder with the existing encapsulation */

	CDREncapInit(&cod, &buf, body_p, body_len, cdr_decoding);

	/* Decode the IIOP Profile Body data */

	IOREncapIIOP(
		&cod, 
		&major, &minor, 
		&host_p, &port, 
		&objkey_len, &objkey_p 
	);

	if (major != IIOP_MAJOR)
		return;

	if (minor > IIOP_MINOR)
		return;
      
	/* Code the generic IIOP data into the existing coder */

	CDRCodeUChar(cod_p, &major);
	CDRCodeUChar(cod_p, &minor);

	CDRCodeString(cod_p, &host_p);
	CDRCodeUShort(cod_p, &port);

	CDRCodeULong(cod_p, &objkey_len);
	CDRCodeNOctet(cod_p, &objkey_p, &objkey_len);

	/* Additional data depending on the IIOP version */

	switch (minor)
	{
	case 0:
		/* done.. */
		break;
		
	case 1:
		/* Tagged Components to code */

		CDRCodeULong(&cod, &tc_len);

		if (tc_len)
			CDRCodeNOctet(&cod, &tc_p, &tc_len);
		else
			tc_p = 0;
		
		ior_code_tcomp(cod_p, tc_len, tc_p);
	}
}


/*
 *	Code a Tagged Component into the buffer. 
 */
static void
ior_code_tcomp(
	CDRCoderT *cod_p, unsigned long tlen, IORTaggedComponentT *tcomp_p
)
{
	register unsigned long	idx;
	
	if (!cod_p)
		return;
	
	/* Code the (sequence) length) */

	CDRCodeULong(cod_p, &tlen);

	/*
	 *	Code the sequence of Tagged Components.
	 */
	for (idx = 0; idx < tlen; idx++)
	{
		CDRCodeULong(cod_p, &tcomp_p[idx].tc_tag);

		CDRCodeULong(cod_p, &tcomp_p[idx].tc_component_len);
		CDRCodeNOctet(
			cod_p, 
			&tcomp_p[idx].tc_component_data_p, 
			&tcomp_p[idx].tc_component_len
		);
	}
}


/*
 *	Code a MultipleComponentProfile.  This is a sequence of Tagged
 *	Components.
 */
static void
ior_code_mcomp(CDRCoderT *cod_p, IORMultipleComponentProfileT *mcomp_p)
{
	if (!mcomp_p)
		return;
	
	ior_code_tcomp(
		cod_p, mcomp_p->mcp_components_len, mcomp_p->mcp_components_p
	);
}

#endif


/*
 *	Calculate  the  maximum   possible CDR  coded  length  of  the
 *	IOR.  This represents the encapsulated   data length of an IOR
 *	which we use to calculate the length of a stringified IOR.
 */
static unsigned long
ior_length(IORT *ior_p)
{
	unsigned long	len;
	unsigned long	i;
	
	if (!ior_p)
		return 0;
	
	/* string length + string + max padding for next long */

	len = 4 + 3 + ior_p->ior_type_id_p ? strlen(ior_p->ior_type_id_p) : 1;
	
	/* Tagged Profile sequence length */

	len += 4;
	
	/* Tagged Profiles */

	for (i = 0; i < ior_p->ior_profiles_len; i++)
	{
		/* tag (ulong) + sequence length (ulong) */

		len += 8;

		/* octet stream + max padding for next profile */

		len += ior_p->ior_profiles_p[i].tp_profile_len + 3;
	}
	
	/* Add 4 to be sure */

	return len + 4;
}


/*
 *	Convert  the binary data in the  stringified IOR to a hex pair
 *	string equivalent. For example, a 1 byte binary value of 5 (ie
 *	00000101) would  be coded  as the  ascii  pair '05'. This will
 *	double the buffer size.
 */
static IORStatusT
ior_to_ascii(const unsigned char *str_p, const unsigned long len)
{
	unsigned char	*old_p;
	char		*new_p;
	
	if (!str_p || !len)
		return IOR_ENULL_DATA;
	
	old_p = (unsigned char *)str_p + len - 1; /* last byte */
	new_p = (char *)old_p + len - 1;	 /* start of last ascii pair */
	
	new_p[2] = '\0';
	
	for (; old_p >= str_p; old_p--)
	{
		ior_byte_to_hex(*old_p, new_p);
		new_p -= 2;
	}
	
	PL_TRACE("ior2",("ior_to_ascii(): hex string is %s", str_p));

	return IOR_OK;
}
/*
 * encap.c - 
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *	Functions to create encapsulated   data and return the   octet
 *	stream   of   the same data.  Also    defines an IIOP specific
 *	encapsulation function to create an initial IIOP Profile Body.
 * 
 *  Copyright (c) 1993-7 Iona Technologies Ltd.
 *             All Rights Reserved
 *  This is unpublished proprietary source code of Iona Technologies
 *  The copyright  notice  above  does not  evidence  any  actual or
 *  intended publication of such source code.
 */

#define ENCAP_EXTERN
#include "encap.h"


/*
 * IOREncapIIOP()
 *
 * Purpose:
 *	Encapsulate the generic IIOP Profile Body,  common to both 1.0
 *	and 1.1 versions of IIOP. The 1.1 specifics are coded manually
 *	by the application, following this call.
 *
 * Returns:
 *	IOR_OK		coded Profile data ok 
 *	IOR_ENULL_CODER	null coder specified
 */
IORStatusT
IOREncapIIOP(
	CDRCoderT 	*cod_p,
	octet 		*major_p, 
	octet 		*minor_p, 
	char 		**host_pp, 
	unsigned short 	*port_p, 
	unsigned long 	*objkey_len_p, 
	octet 		**objkey_pp
)
{
	if (!cod_p)
		return IOR_ENULL_CODER;

	CDRCodeOctet(cod_p, major_p);
	CDRCodeOctet(cod_p, minor_p);
	CDRCodeString(cod_p, host_pp);
	CDRCodeUShort(cod_p, port_p);
	CDRCodeULong(cod_p, objkey_len_p);
	CDRCodeNOctet(cod_p, objkey_pp, objkey_len_p);
	
	return IOR_OK;
}
/*
 * cdr.c -  CDR Coder implementation
 * 
 * Author:          Craig Ryan
 * 
 * Description:
 *
 *	This api implements  a coder  which can encode/decode   simple
 *	(IDL)  types into a buffer, aligning   all data on its natural
 *	boundary.
 *
 *	Buffer space is not allocated locally; the calling application
 *	must provide  a alloc  callback function to  return additional
 *	buffer space  as required. This space  is then returned to the
 *	caller via the dealloc callback.
 *
 *  Copyright (c) 1993-7 Iona Technologies Ltd.
 *             All Rights Reserved
 *  This is unpublished proprietary source code of Iona Technologies
 *  The copyright  notice  above  does not  evidence  any  actual or
 *  intended publication of such source code.
 */
#define CDR_EXTERN

#include "cdrP.h"

#ifdef TEST_DRIVER
#include <stdio.h>
#include <fcntl.h>
#endif


/*
 * CDRInit()
 *
 * Purpose:
 *
 *	Initialise  the coder  settings. Also provides  for installing
 *	alloc/de-alloc callbacks  for memory allocation,  and sets the
 *	expected minimum buffer size.
 *
 * Returns:
 *	void
 */
CDRStatusT
CDRInit(CDRCoderT *cod_p, unsigned short bytesex, size_t alloc_min)
{
	if (!cod_p)
		return CDR_FAIL;
	
	cod_p->cdr_mode = cdr_unknown;

	/* Current and start buffer pointers */

	cod_p->cdr_buf_p = cod_p->cdr_start_buf_p = (CDRBufferT *)0;

	/* Buffer space [de-]allocation callbacks. */

	cod_p->cdr_alloc_fp = 0;
	cod_p->cdr_alloc_min = 
		alloc_min < CDR_MIN_BUFSZ ? CDR_MIN_BUFSZ : alloc_min;

	cod_p->cdr_dealloc_fp = 0;

	/* Total buffer length and byte order */

	cod_p->cdr_buflen = 0;
	cod_p->cdr_bytesex = bytesex;		/* big/little endian */

#ifndef GIOP_NO_AUTOFRAG
	cod_p->cdr_maxlen = 0;
	cod_p->cdr_giop_p = 0;
#endif

	return CDR_OK;
}


/*
 * CDRFree()
 *
 * Purpose:
 *
 *	Free all  space  used by the coder.   This  only  happens if a
 *	de-allocation callback  has  been  registered.  All space   is
 *	returned to the caller.
 *
 * Returns:
 *	void
 */
void
CDRFree(CDRCoderT *cod_p)
{
	CDRBufferT *buf_p, *tmp_buf_p;
	
	if (!cod_p)
		return;
	/*
	 *	If there is no   dealloc callback registered, we   can
	 *	assume that the buffer memory does not need reeIng.
	 */
	if (!cod_p->cdr_dealloc_fp)
		return;

	/*
	 *	Free  each  buffer   struct  by returning it   to  the
	 *	caller. Only the current buffer  is returned with each
	 *	call, so we first detach each from the buffer list.
	 */
	for (buf_p = cod_p->cdr_start_buf_p; buf_p; buf_p = tmp_buf_p)
	{
		tmp_buf_p = buf_p->cdrb_next_p;
		buf_p->cdrb_next_p = 0;
		
		cod_p->cdr_dealloc_fp(buf_p);
	}

	cod_p->cdr_start_buf_p = cod_p->cdr_buf_p = 0;
	cod_p->cdr_buflen = 0;
}


/*
 * CDRMode()
 *
 * Purpose:
 *	Set the coding mode - encoding or decoding and return
 *	the current setting.
 *
 * Returns:
 *	current mode
 */
CDRModeT
CDRMode(CDRCoderT *cod_p, CDRModeT mode)
{
	CDRModeT	old_mode;
	
	if (!cod_p)
		return cdr_unknown;
	
	old_mode = cod_p->cdr_mode;
	cod_p->cdr_mode = mode;

	return old_mode;
}


/*
 * CDRReset()
 *
 * Purpose:
 *	Reset the current buffer settings including buffer pointer and
 *	buffer usage.
 *
 *	in_use indicates that the buffer is part of the active (in use)
 *	set. Therefore, it's _used bytes must be deducted from the total
 *	coder length.
 *
 * Returns:
 *	CDR_FAIL	no current buffer to reset
 *	CDR_OK		otherwise
 */
CDRStatusT
CDRReset(CDRCoderT *cod_p, unsigned char in_use)
{
	PL_TRACE("cdr1",("CDRReset(): Enter"));

	if (!cod_p || !cod_p->cdr_start_buf_p)
		return CDR_FAIL;

	/* Reduce total coder length */

	if (in_use && (cod_p->cdr_buflen >= cod_p->cdr_buf_p->cdrb_used))
		cod_p->cdr_buflen -= cod_p->cdr_buf_p->cdrb_used;

	/* Reset the buffer pointer */

	cod_p->cdr_buf_p->cdrb_used = 0;
	cod_p->cdr_buf_p->cdrb_pos_p = cod_p->cdr_buf_p->cdrb_buffer_p;

	PL_TRACE("cdr1",("CDRReset(): Exit"));

	return CDR_OK;
}


/*
 * CDRRewind()
 *
 * Purpose:
 *	Rewind the  coder to the start  of the first buffer. The total
 *	used buffer space in the coder is preserved.
 *
 * Returns:
 *	CDR_FAIL	no buffers
 *	CDR_OK		otherwise
 */
CDRStatusT
CDRRewind(CDRCoderT *cod_p, unsigned char reset_length)
{
	PL_TRACE("cdr1",("CDRRewind(): Enter"));

	if (!cod_p)	/* || !cod_p->cdr_start_buf_p) */
		return CDR_FAIL;
	
	cod_p->cdr_buf_p = cod_p->cdr_start_buf_p;
	
	if (reset_length)
		cod_p->cdr_buflen = 0;
	
	PL_TRACE("cdr1",("CDRRewind(): Exit"));

	return CDR_OK;
}


/*
 * CDRByteSex()
 *
 * Purpose:
 *	Set the byte sex (or order) for this buffer. This can be
 *	either big endian (sex = 0) or little endian.
 *
 * Returns:
 *	void
 */
void
CDRByteSex(CDRCoderT *cod_p, unsigned char sex)
{
	if (!cod_p)
		return;
       
	cod_p->cdr_bytesex = sex;
}


/*
 * CDRNeedBuffer()
 *
 * Purpose:
 *
 *	Find a single buffer in the coder of the minimum length.  Each
 *	intermediate buffer that is  not long enough is  released back
 *	the allocator. If a buffer is found it will be at the start of
 *	the buffer list.
 *
 *	If no such buffer is found, a new one is allocated at the head
 *	of the list.
 *
 * Returns:
 *	CDR_OK		found or allocated a buffer
 *	else		failed to allocate or dealloc a buffer
 */
CDRStatusT
CDRNeedBuffer(CDRCoderT *cod_p, size_t min_len)
{
	CDRBufferT	*buf_p, *next_p;
	
	if (!cod_p)
		return CDR_FAIL;
      
	PL_TRACE("cdr1",("CDRNeedBuffer(): Enter; min len is %d",min_len));

	next_p = cod_p->cdr_start_buf_p;
	
	for (buf_p = cod_p->cdr_start_buf_p; buf_p;)
	{
		if (buf_p->cdrb_len >= min_len)
		{
			PL_TRACE("cdr1",\
			("CDRNeedBuffer(): entry len of %d found..",\
		        buf_p->cdrb_len));

			break;
		}		

		PL_TRACE("cdr1",\
		     ("CDRNeedBuffer(): free entry %x; len %d too small",\
		     buf_p->cdrb_buffer_p, buf_p->cdrb_len));

		/* Free it up because it's no good */
		
		next_p = buf_p->cdrb_next_p;
		buf_p->cdrb_next_p = 0;

		/* Call the de-alloc callback */

		if (cod_p->cdr_dealloc_fp)
			(*cod_p->cdr_dealloc_fp)(buf_p);
		else	
			/* REVISIT - ok to just drop the buffer instead?? */
			return CDR_FAIL;

		buf_p = next_p;
	}

	/* Reset the coder settings */

	cod_p->cdr_start_buf_p = cod_p->cdr_buf_p = buf_p;
	cod_p->cdr_buflen = 0;

	/* No buffer of that length - add one */

	if (!buf_p)
	{
		unsigned long 	save_min;
		CDRStatusT	st;
		
		PL_TRACE("cdr1",\
		("CDRNeedBuffer(): all buffers too small - add new one"));

		save_min = cod_p->cdr_alloc_min;
		cod_p->cdr_alloc_min = min_len;

		st = CDRAddBuffer(cod_p, 0);
		cod_p->cdr_alloc_min = save_min;

		return st;
	}
	else
		CDRReset(cod_p, false);
	
	PL_TRACE("cdr1",("CDRNeedBuffer(): Exit"));

	return CDR_OK;
}


/* Add a buffer to the coder */

CDRStatusT
CDRAddBuffer(CDRCoderT *cod_p, CDRBufferT *buf_p)
{
	CDRBufferT 	*new_buf_p;
	unsigned long	delta;
	
	PL_TRACE("cdr1",("CDRAddBuffer: Enter"));

	if (!cod_p)
		return CDR_FAIL;

	if (buf_p)
		new_buf_p = buf_p;
	else
	{
		/* Try to auto alloc a new buffer */

		PL_TRACE("cdr1",\
		("CDRAddBuffer(): call alloc with %d",cod_p->cdr_alloc_min));

		if (!cod_p->cdr_alloc_fp)
			return CDR_FAIL;

		/*
		 *	Determine an 8 byte multiple minimum size. Use
		 *	delta to avoid an extra variable :-)
		 */
		if ((delta = cod_p->cdr_alloc_min) < CDR_MIN_BUFSZ)
			delta = CDR_MIN_BUFSZ;
		else
			delta += CDR_ALIGN_DELTA(delta, CDR_MIN_BUFSZ);

		new_buf_p = (*cod_p->cdr_alloc_fp)(delta);
	}

	if (!new_buf_p)
		return CDR_FAIL;

	if (cod_p->cdr_mode == cdr_encoding) 
	{
		/* Must be at least 8 bytes long */

		if (new_buf_p->cdrb_len < CDR_MIN_BUFSZ)
			return CDR_FAIL;
		
		/*
		 *	Ensure the buffer is a multiple of 8 in length.
		 */
		if (delta = CDR_ALIGN_DELTA( \
			      new_buf_p->cdrb_pos_p + new_buf_p->cdrb_len, \
			      CDR_MIN_BUFSZ ), delta
		)
			new_buf_p->cdrb_len -= CDR_MIN_BUFSZ - delta;
	}

	PL_TRACE("cdr1",("CDRAddBuffer: New buf %x[ ... %x]",\
		new_buf_p->cdrb_buffer_p, \
		new_buf_p->cdrb_buffer_p + new_buf_p->cdrb_len));

	/*
	 *	Add the buffer to the coder.
	 */
	if (!cod_p->cdr_start_buf_p)
	{
		PL_TRACE("cdr1",\
			("CDRAddBuffer(): new buffer is first in list"));

		/* this is the first buffer */

		cod_p->cdr_start_buf_p = cod_p->cdr_buf_p = new_buf_p;
		cod_p->cdr_buflen = 0;

		new_buf_p->cdrb_next_p = new_buf_p->cdrb_prev_p = 0;
	}
	else
	{
		CDRBufferT	*next_p;

		/* 
		 * Add the new buffer after the current one. Any trailing
		 * buffer will follow the new buffer.
		 */
		next_p = cod_p->cdr_buf_p->cdrb_next_p;
		cod_p->cdr_buf_p->cdrb_next_p = new_buf_p;

		new_buf_p->cdrb_next_p = next_p;
		new_buf_p->cdrb_prev_p = cod_p->cdr_buf_p;
		cod_p->cdr_buf_p = new_buf_p;
	}

	/* Reset the new buffer settings */

	CDRReset(cod_p, false);
	
	PL_TRACE("cdr1",("CDRAddBuffer: Exit"));
	return CDR_OK;
}


/*
 * CDRAlloc()
 *
 * Purpose:
 *	Register an automatic buffer allocation callback.
 *
 * Returns:
 *	 void
 */
void
CDRAlloc(CDRCoderT *cod_p, CDRAllocFpT buf_alloc_fp)
{
	if (!cod_p || !buf_alloc_fp)
		return;
	
	cod_p->cdr_alloc_fp = buf_alloc_fp;
}


/*
 * CDRDealloc()
 *
 * Purpose:
 *	Register an automatic buffer de-allocation callback.
 *
 * Returns:
 *	 void
 */
void
CDRDealloc(CDRCoderT *cod_p, CDRDeallocFpT buf_dealloc_fp)
{
	if (!cod_p || !buf_dealloc_fp)
		return;
	
	cod_p->cdr_dealloc_fp = buf_dealloc_fp;
}


/*
 * CDRBuflen()
 *
 * Purpose:
 *	Return the total buffer space in use. Once called, the
 *	buffer usage setting is reset if 'reset' is true.
 *
 * Returns:
 *	buffer length in bytes
 */
unsigned long
CDRBuflen(CDRCoderT *cod_p, unsigned char reset)
{
	unsigned long buflen = cod_p->cdr_buflen;

	if (reset)
		cod_p->cdr_buflen = 0;

	return buflen;
}


/*-------------------------------------------------------------------
 * Coding routines for simple types.
 */
CDRStatusT
CDRCodeOctet(CDRCoderT *cod_p, octet *oct_p)
{
	return CDRCodeUChar(cod_p, oct_p);
}

CDRStatusT
CDRCodeNOctet(CDRCoderT *cod_p, octet **oct_pp, unsigned long *len_p)
{
	unsigned long free;
	
	if (!oct_pp || !len_p)
		return CDR_FAIL;

	if (cdr_align(cod_p, 0) != CDR_OK)
		return CDR_FAIL;

	PL_TRACE("cdr1",\
	("CDRCodeNOctet(): octet len, free bytes are %d, %d",*len_p,free));

	if (cod_p->cdr_mode == cdr_decoding)
	{
		CDRStatusT	ret;
		
		*oct_pp = 0;
		ret = CDR_OK;
		
		free = cod_p->cdr_buflen - cod_p->cdr_buf_p->cdrb_used;

		/* REVISIT - if we ever decode from multiple buffers, 
		 * this needs to change to a copy
		 */
		*oct_pp = cod_p->cdr_buf_p->cdrb_pos_p;

		/* Assume octets are fragmented over >1 message. */

		if (free < *len_p)
		{
			/* Return actual length */

			*len_p = free;
			ret = CDR_ESTR_FRAG;
		}			

		/* skip octets */

		CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, *len_p);
		cod_p->cdr_buf_p->cdrb_used += *len_p;

		return ret;
	}
	
	/* Encode the octet stream */

	/* nothing to code */

	if (!*len_p)
		return CDR_OK;

	/* must be non null if the length is >0 */

	if (!*oct_pp)
		return CDR_FAIL;
	
	return cdr_encode_nbytes(
		cod_p, 
		(char *)*oct_pp, 
		*len_p,
 		cod_p->cdr_buf_p->cdrb_len - cod_p->cdr_buf_p->cdrb_used,
	        false
	);
}

CDRStatusT
CDRCodeBool(CDRCoderT *cod_p, unsigned char *bool_p)
{
	return CDRCodeUChar(cod_p, bool_p);
}

CDRStatusT
CDRCodeShort(CDRCoderT *cod_p, short *short_p)
{
	if (!short_p)
		return CDR_FAIL;

	if (cdr_align(cod_p, CDR_SIZEOF_USHORT) != CDR_OK)
		return CDR_FAIL;

	if (cod_p->cdr_mode == cdr_encoding)
		*(short *)cod_p->cdr_buf_p->cdrb_pos_p = *short_p;
	else
	{
		if (cod_p->cdr_bytesex == CDR_BYTE_ORDER)
			*short_p = *(short *)cod_p->cdr_buf_p->cdrb_pos_p;
		else
		{  
			((unsigned char *)short_p)[0] =
				cod_p->cdr_buf_p->cdrb_pos_p[1];;
			
			((unsigned char *)short_p)[1] =
				cod_p->cdr_buf_p->cdrb_pos_p[0];
		}
		/* CDR_SWAP_2(*short_p);*/
	}
	
	CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, CDR_SIZEOF_USHORT);

	return CDR_OK;
}

CDRStatusT
CDRCodeUShort(CDRCoderT *cod_p, unsigned short *ushort_p)
{
	return CDRCodeShort(cod_p, (short *)ushort_p);
}


CDRStatusT
CDRCodeEnum(CDRCoderT *cod_p, unsigned long *enum_p)
{
	return CDRCodeLong(cod_p, (long *)enum_p);
}


CDRStatusT
CDRCodeLong(CDRCoderT *cod_p, long *long_p)
{
	if (!long_p)
		return CDR_FAIL;

	if (cdr_align(cod_p, CDR_SIZEOF_LONG) != CDR_OK)
		return CDR_FAIL;

	if (cod_p->cdr_mode == cdr_encoding)
		*(long *)cod_p->cdr_buf_p->cdrb_pos_p = *long_p;
	else
	{
		if (cod_p->cdr_bytesex == CDR_BYTE_ORDER)
			*long_p = *(long *)cod_p->cdr_buf_p->cdrb_pos_p;
		else
		{
			short i;
			
			for (i = 0; i < 4; i ++)
			{
				((unsigned char *)long_p)[i] =
					cod_p->cdr_buf_p->cdrb_pos_p[3-i];
			}
			
			/* CDR_SWAP_4(*long_p);*/
		}
	}
	
	CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, CDR_SIZEOF_LONG);

	return CDR_OK;
}

CDRStatusT
CDRCodeULong(CDRCoderT *cod_p, unsigned long *ulong_p)
{
	return CDRCodeLong(cod_p, (long *)ulong_p);
}


CDRStatusT
CDRCodeFloat(CDRCoderT *cod_p, float *flt_p)
{
	if (!flt_p)
		return CDR_FAIL;

	if (cdr_align(cod_p, CDR_SIZEOF_FLOAT) != CDR_OK)
		return CDR_FAIL;

	if (cod_p->cdr_mode == cdr_encoding)
		*(float *)cod_p->cdr_buf_p->cdrb_pos_p = *flt_p;
	else
	{
		if (cod_p->cdr_bytesex == CDR_BYTE_ORDER)
			*flt_p = *(float *)cod_p->cdr_buf_p->cdrb_pos_p;
		else
		{
			short i;

			for (i = 0; i < 4; i ++)
			{
				((unsigned char *)flt_p)[i] =
					cod_p->cdr_buf_p->cdrb_pos_p[3-i];
			}
		}
		
		/* CDR_SWAP_4(*flt_p); */
	}
	
	CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, CDR_SIZEOF_FLOAT);

	return CDR_OK;
}


CDRStatusT
CDRCodeDouble(CDRCoderT *cod_p, double *dbl_p)
{
	if (!dbl_p)
		return CDR_FAIL;

	if (cdr_align(cod_p, CDR_SIZEOF_DOUBLE) != CDR_OK)
		return CDR_FAIL;

	if (cod_p->cdr_mode == cdr_encoding)
		*(double *)cod_p->cdr_buf_p->cdrb_pos_p = *dbl_p;
	else
	{
		if (cod_p->cdr_bytesex == CDR_BYTE_ORDER)
			*dbl_p = *(double *)cod_p->cdr_buf_p->cdrb_pos_p;
		else
		{
			short i;

			for (i = 0; i < 8; i ++)
			{
				((unsigned char *)dbl_p)[i] =
					cod_p->cdr_buf_p->cdrb_pos_p[7-i];
			}
		}

		/* CDR_SWAP_8(*dbl_p); */
	}
	
	CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, CDR_SIZEOF_DOUBLE);

	return CDR_OK;
}


CDRStatusT
CDRCodeChar(CDRCoderT *cod_p, char *ch_p)
{
	return CDRCodeUChar(cod_p, (unsigned char *)ch_p);
}


CDRStatusT
CDRCodeUChar(CDRCoderT *cod_p, unsigned char *uch_p)
{
	if (!uch_p)
		return CDR_FAIL;

	if (cdr_align(cod_p, CDR_SIZEOF_BYTE) != CDR_OK)
		return CDR_FAIL;
	
	if (cod_p->cdr_mode == cdr_encoding)
		*cod_p->cdr_buf_p->cdrb_pos_p = *uch_p;
	else
		*uch_p = *cod_p->cdr_buf_p->cdrb_pos_p;
	
	cod_p->cdr_buf_p->cdrb_pos_p++;
	
	return CDR_OK;
}


CDRStatusT
CDRCodeString(CDRCoderT *cod_p, char **str_pp)
{
	unsigned long len;

	PL_TRACE("cdr1",("CDRCodeString: Enter"));

	if (!cod_p)
		return CDR_FAIL;
	
	/*
	 *	If the string  in null,  length  is 0. If  it is empty
	 *	("")  length is 1  (ie the null  char only). Otherwise
	 *	the length  is coded  as   the string length  + 1   to
	 *	include to trailing null.
	 */
	if (cod_p->cdr_mode == cdr_encoding)
		len = str_pp 
			? (*str_pp ? (unsigned long)strlen(*str_pp) + 1 : 1) 
			: 0L;

	PL_TRACE("cdr1",("CDRCodeString: call CodeNString and Exit:"));
	return CDRCodeNString(cod_p, str_pp, &len);
}


CDRStatusT
CDRCodeNString(CDRCoderT *cod_p, char **str_pp, unsigned long *str_len_p)
{
	unsigned long	free;		/* free bytes in current buffer */
	CDRStatusT	ret;
	
	if (!str_len_p)
		return CDR_FAIL;
	
	PL_TRACE("cdr1",\
	("CDRCodeNString: Enter; coder offset (%d), string is (len:%d) >%s<", \
	cod_p->cdr_buflen, str_len_p?*str_len_p:-1, *str_pp?*str_pp:pl_null));

	if ((ret = cdr_align(cod_p, CDR_SIZEOF_LONG)) != CDR_OK)
		return ret;

	if (cod_p->cdr_mode == cdr_encoding)
	{
		long		sz;

		/* Free space remaining */

		free = cod_p->cdr_buf_p->cdrb_len
			- cod_p->cdr_buf_p->cdrb_used;

		/*
		 * 3 cases (and coding are):
		 *	null string:	<long:0>
		 *	empty string:	<long:1><'\0'>
		 *	normal string:	<long:length><"...\0">
		 */
		sz = *str_len_p ? *str_len_p
			: ((str_pp && *str_pp) 
			   ? (*str_len_p = 1, CDR_SIZEOF_UCHAR) : 0);

		/* Code length (long) */

		PL_TRACE("cdr2",\
		       ("CDRCodeNString: code length %d into (aligned) %x",\
		       *str_len_p,cod_p->cdr_buf_p->cdrb_pos_p));

		*(long *)cod_p->cdr_buf_p->cdrb_pos_p = *str_len_p;

		PL_TRACE("cdr1",\
			("CDRCodeNString(): used bytes is %d",\
			cod_p->cdr_buf_p->cdrb_used));

		/* Increment buffer */

		CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, CDR_SIZEOF_ULONG);
		
		/* Code string - may use multiple buffers.. */

		return cdr_encode_nbytes(cod_p, *str_pp, sz, free, true);
	}
	else
	{
		/* Decode.. */

		long 		real_len;
		
		/* Must be non-null at this stage */

		if (!str_pp)
			return CDR_FAIL;

		*str_pp = 0;
		
		/* Decode length */

		real_len = *(long *)cod_p->cdr_buf_p->cdrb_pos_p;

		/* REVISIT - should call CodeULong?? */
		/* Swap length if need be */

		if (cod_p->cdr_bytesex != CDR_BYTE_ORDER)
			CDR_SWAP_4(real_len);
		
		/* Increment buffer to start of string */

		CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, CDR_SIZEOF_LONG);

		PL_TRACE("cdr2",\
		       ("CDRCodeNString: decoded length %d from (aligned) %x",\
		       real_len,cod_p->cdr_buf_p->cdrb_pos_p));

		/*
		 *	Corner case; if length is  0, then the  buffer
		 *	contains no string.  The return  string is set
		 *	to null.
		 */
		if (real_len == 0)
		{
			*str_len_p = 0L;

			PL_TRACE("cdr1",\
			("CDRCodeNString: Exit Ok; coded length 0"));

			return CDR_OK;
		}
					
		/* Free space remaining */

		free = cod_p->cdr_buflen - cod_p->cdr_buf_p->cdrb_used;

		/*
		 *	Is there enough space in the buffer to contain
		 *	the string of the specified length? If not, we
		 *	assume its fragmented across messages.
		 */
		if (!free || (free < (unsigned long) real_len))
		{
			*str_len_p = free;
			ret = CDR_ESTR_FRAG;
		}
		else
			*str_len_p = real_len;

		PL_TRACE("cdr2",\
			 ("CDRCodeNString: expected/coded lengths %d / %d",\
			  *str_len_p, real_len));

		/* Decode string */

		*str_pp = (char *)cod_p->cdr_buf_p->cdrb_pos_p;

		PL_TRACE("cdr2",\
		   ("CDRCodeNString: decode string >%s< from %x\n",\
		   *str_pp, cod_p->cdr_buf_p->cdrb_pos_p));

		/* skip string */

		CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, real_len);

		cod_p->cdr_buf_p->cdrb_used += *str_len_p;
	}
	
	PL_TRACE("cdr2",("CDRCodeNString: Exit"));

	return ret;
}


/*
 * CDREncapCreate()
 *
 * Purpose:
 *
 *	Initialise the CDR coder ready for encoding a sequence<octet>
 *	encapsulation. This is coded as:
 *
 *	        <sex:1><data:n>
 *
 *	where sex is the byte order and	data which will be manually 
 *	CDR coded following this API call.
 *
 *	The coder is reset prior to using it.
 *
 * Returns:
 *	CDR_OK		all ok
 *	CDR_ENULL_CODER	null CDR coder given
 */
CDRStatusT
CDREncapCreate(CDRCoderT *cod_p, octet byte_sex)
{
	if (!cod_p)
		return CDR_ENULL_CODER;

	/* Reset the coder settings */

	CDRRewind(cod_p, true);
	CDRReset(cod_p, true);

	CDRMode(cod_p, cdr_encoding);
	CDRByteSex(cod_p, byte_sex);
	
	/* Code the initial octet byte sex */

	CDRCodeOctet(cod_p, &byte_sex);

	return CDR_OK;
}


/*
 * CDREncapEnd()
 *
 * Purpose:
 *
 *	This signals  that  the encapsulation  is  complete. This will
 *	cause   the initial   length  field  (see  the  CDREncapCreate
 *	comments) to  be  set to   the final sequence   length and the
 *	actual buffer containing the   resulting octet stream  is then
 *	returned along with it's physical length.
 *
 * Returns:
 *	CDR_OK			all ok
 *	CDR_ENULL_CODER		null coder specified
 *	CDR_ENULL_RETURN	a null return (out) param was specified
 */
CDRStatusT
CDREncapEnd(
	CDRCoderT 	*cod_p, 
	octet 		**oct_pp, 
	unsigned long 	*octlen_p,
        GIOPAllocFpT	getmem
)
{
	unsigned long 	len;
	register unsigned long 	
		        tot;
	CDRBufferT	*cdr_buf_p;
	octet		*stream_p;
	
	if (!cod_p)
		return CDR_ENULL_CODER;

	if (!oct_pp || !octlen_p)
		return CDR_ENULL_RETURN;
	
	if (!getmem)
		return CDR_ENOSPACE;
	
	tot = 0;
	len = cod_p->cdr_buflen;

	/* we return the octet stream and length */

	*octlen_p = len;
#ifdef MEMCHECKRT
	stream_p = *(octet **)oct_pp = (octet *)getmem(len,__FILE__,__LINE__);
#else
	stream_p = *(octet **)oct_pp = (octet *)getmem(len);
#endif
	
	/*
	 *	Copy  all the  CDR  buffers   into one  large  buffer,
	 *	pointed to by  stream_p. This will  be return  out via
	 *	the out_pp parameter.
	 */
	for (
		cdr_buf_p = cod_p->cdr_start_buf_p; 
		cdr_buf_p && (tot < len);
		cdr_buf_p = cdr_buf_p->cdrb_next_p
	)
	{
		unsigned long	seg_len;
		
		seg_len = cdr_buf_p->cdrb_used;
		
		if ((tot + seg_len) > len)
			seg_len = len - tot;
		
		memcpy(stream_p, cdr_buf_p->cdrb_buffer_p, seg_len);

		stream_p += seg_len;
		tot += seg_len;
	}
	
	return CDR_OK;
}


/*
 * CDREncapInit()
 *
 * Purpose:
 *
 *	Sets up a  CDR coder with an  existing buffer.  By default the
 *	coder is set to decoding mode which the caller may change with
 *	a CDRMode() call on return from here.
 *
 *	The     resulting  coder will either be      used to encode an
 *	encapsulation   or  decode an existing  encapsulation  already
 *	contained in the buffer.
 *
 * Returns:
 *	CDR_OK			all ok
 *	CDR_ENULL_CODER		null coder specified
 *	CDR_ENULL_DATA		null data (raw buffer) specified
 */
CDRStatusT
CDREncapInit(
	CDRCoderT 	*cod_p, 
	CDRBufferT 	*buf_p, 
	octet 		*data_p, 
	unsigned long 	data_len,
        CDRModeT	mode
)
{
	if (!cod_p || !buf_p || (mode == cdr_unknown))
		return CDR_ENULL_CODER;
	
	if (!data_p || !data_len)
		return CDR_ENULL_DATA;
	
	CDRInit(cod_p, CDR_BYTE_ORDER, 0L);
	
	/* Construct a Coder with the given buffer */

	buf_p->cdrb_buffer_p = data_p;
	buf_p->cdrb_len = data_len;
	buf_p->cdrb_next_p = buf_p->cdrb_prev_p = 0;
	
	cod_p->cdr_start_buf_p = cod_p->cdr_buf_p = buf_p;

	CDRReset(cod_p, false);
	CDRMode(cod_p, mode);
	
	if (mode == cdr_decoding)
	{
		octet	sex;
		
		cod_p->cdr_buflen = data_len;

		/* Decode the byte sex and set the coder byte order */

		CDRCodeOctet(cod_p, &sex);
		CDRByteSex(cod_p, sex);
	}
	else
		cod_p->cdr_buflen = 0;
		
	return CDR_OK;
}


/*--------------------------------------------------------------------
 *	CDR static functions
 */

/*
 * cdr_align()
 *
 * Purpose:
 *
 *	Align the buffer for the current data  type. Since all buffers
 *	are  a multiple of 8  in length, the   start of each buffer is
 *	also aligned with the start of the first buffer.
 *
 *	If the  current  buffer   does  not  hold  the  data  item the
 *	remaining bytes become  padding   bytes, and a new   buffer is
 *	added to the  list. Note that  the used bytes  setting for the
 *	buffer are set to include  the current data  size so that each
 *	caller to   this routine does  not have  to update   the usage
 *	setting each time.
 *
 * Returns:
 *	CDR_OK		buffer is ready for writing the item
 *	CDR_FAIL	failed to get buffer space
 */
static CDRStatusT
cdr_align(CDRCoderT *cod_p, unsigned long data_sz)
{
	register unsigned char	
		        is_dec;		/* are we decoding? */
	
	unsigned long	sz;
	long 		from_curr, 
	                free;
	
	if (!cod_p)
		return CDR_FAIL;
	
	if (cod_p->cdr_mode == cdr_unknown)
		return CDR_EINV_MODE;

	from_curr = 0;
	is_dec = (unsigned char) (cod_p->cdr_mode == cdr_decoding);

	/*
	 * 	How many bytes do we skip from the current buffer 
	 *	position to align the item? The data_sz may be zero
	 *	if we want to align on any byte without updating
	 *	the coder settings (pos, used bytes etc).
	 */
	sz = data_sz ? data_sz : 1;

	if (cod_p->cdr_buf_p)
		from_curr = CDR_ALIGN_DELTA(cod_p->cdr_buf_p->cdrb_pos_p, sz);


#ifndef GIOP_NO_AUTOFRAG
	/*
	 *	If the buffer   has reached the maximum  fragmentation
	 *	size, then call back to giop  to send  the message and
	 *	reset the coder before we continue coding.
	 *
	 *	If the  giop  pointer  (_giop_p)  is  nil, assume auto 
	 *	fragmentation is not active.
	 */
	if (
		!is_dec
		&& cod_p->cdr_giop_p 
		&& (cod_p->cdr_giop_p->gs_afrag_flags&GIOP_AFRAG_ON)
		&& cod_p->cdr_buf_p
	)
	{
		/* ie if buffer len + aligned data space > max frag size */

		if (cod_p->cdr_buflen + from_curr + sz > cod_p->cdr_maxlen)
		{	
			if (giop_auto_frag(cod_p->cdr_giop_p) != GIOP_OK)
				return CDR_EAUTO_FRAG;

			/* 
			 *	Re-eval padding with new coder contents. 
			 *	The coder will contain a Fragment msg hdr.
			 */
			from_curr = CDR_ALIGN_DELTA(\
					cod_p->cdr_buf_p->cdrb_pos_p, sz);
		}
	}
#endif
	
	PL_TRACE("cdr1",("cdr_align(): Enter, size is %d",data_sz));

	/*
	 *	First buffer - add it.
	 */
	if (!cod_p->cdr_buf_p)
	{
		if (is_dec)
			return CDR_FAIL;

		if (CDRAddBuffer(cod_p, 0) != CDR_OK)
			return CDR_FAIL;

		cod_p->cdr_buf_p->cdrb_used += data_sz;
		cod_p->cdr_buflen += data_sz;

		return CDR_OK;
	}

	/*
	 *	Free bytes remaining in the current buffer
	 */
	free = (is_dec ? cod_p->cdr_buflen : cod_p->cdr_buf_p->cdrb_len)
		- cod_p->cdr_buf_p->cdrb_used;

	/*
	 *	Need to fit the alignment padding plus data. Add a
	 *	buffer if this doesn't fit. Pad the (now previous)
	 *	buffer in this case. Use 'sz' in case data_sz is 0.
	 */
	if (from_curr + (long)sz > free)
	{
		/* We expect a single buffer when decoding */
		
		if (is_dec)
			return CDR_FAIL;

		/*
		 *	The   previous  buffer  is   padded using  the
		 *	trailing bytes.  This ensures   alignment  for
		 *	this buffer relative to it.
		 */
		cod_p->cdr_buf_p->cdrb_used += from_curr;
		cod_p->cdr_buflen += from_curr;
		
		/* Add a new buffer */

		if (CDRAddBuffer(cod_p, 0) != CDR_OK)
			return CDR_FAIL;

		from_curr = 0;
	}

	/* Update buffer/coder settings */

	cod_p->cdr_buf_p->cdrb_pos_p += from_curr;
	cod_p->cdr_buf_p->cdrb_used += from_curr + data_sz;

	/* buflen is fixed for decoding */

	if (!is_dec)
		cod_p->cdr_buflen += from_curr + data_sz;
	
	PL_TRACE("cdr1",("cdr_align(): Exit (write addr will be %x)",\
		 cod_p->cdr_buf_p->cdrb_pos_p));

	return CDR_OK;
}


/* Encode nbytes into the coder */

static CDRStatusT
cdr_encode_nbytes(
	CDRCoderT 	*cod_p, 
	char 		*str_p, 
	unsigned long 	sz,
	unsigned long 	free,
        boolean		is_string
)
{
	unsigned long	remainder;
	unsigned long	seglen;
	char		*seg_p;
	unsigned char	do_auto_frag;
	unsigned char	auto_frag_enabled;
	CDRStatusT	ret;

#ifndef GIOP_NO_AUTOFRAG
	GIOPStateT	*cdr_giop_p;
#endif
	
	remainder = sz;
	seg_p = str_p;
	do_auto_frag = false;
	ret = CDR_OK;

#ifndef GIOP_NO_AUTOFRAG
	cdr_giop_p = cod_p->cdr_giop_p;
	
	auto_frag_enabled = (unsigned char)(cod_p->cdr_giop_p 
		? (cod_p->cdr_giop_p->gs_afrag_flags&GIOP_AFRAG_ON) 
				: false);

	/*
	 *	If the GIOP_AFRAG_STR flag  is NOT set, we can't split
	 *	strings across fragments.  Turn auto_frag off  if this
	 *	is the case.
	 */
	if (
		auto_frag_enabled 
		&& is_string 
		&& !(cod_p->cdr_giop_p->gs_afrag_flags&GIOP_AFRAG_STR)
	)
	{
		auto_frag_enabled = false;

		/* Null the giop so cdr_align calls (below) dont auto_frag */

		cod_p->cdr_giop_p = 0;
	}
#endif

	PL_TRACE("cdr1",("cdr_encode_nbytes(): Enter; Encoding string now.."));

	for (; remainder > 0;)
	{
		PL_TRACE("cdr1",\
			("cdr_encode_nbytes(): free bytes now %d", free));

		/* Need another buffer ? */

		if (!free)
		{
			if ((ret = cdr_align(cod_p, 0)) != CDR_OK)
				break;
			
			free = cod_p->cdr_buf_p->cdrb_len;
		}
			
		seglen = (remainder <= free) ? remainder : free;
		
		if (!seglen)
			break;
		
#ifndef GIOP_NO_AUTOFRAG
		/* 
		 *	Segment too big for the fragment - only code
		 * 	up to the frag size, and send the fragment.
		 */
		if (
			auto_frag_enabled
			&& (cod_p->cdr_buflen + seglen > cod_p->cdr_maxlen)
		)
		{
			seglen = cod_p->cdr_maxlen - cod_p->cdr_buflen;
			do_auto_frag = true;
		}
#endif

		PL_TRACE("cdr1",\
		("cdr_encode_nbytes(): cpy %d bytes of '%s' into [%x - %x]",\
			seglen, seg_p, cod_p->cdr_buf_p->cdrb_pos_p,\
			cod_p->cdr_buf_p->cdrb_pos_p+seglen));
		
		/* 
		 * Write the min of the remaining string / remaining
		 * space in the buffer.
		 */
		memcpy(
			(void *)cod_p->cdr_buf_p->cdrb_pos_p, 
			(const void *)seg_p, 
			seglen
		);
		
		seg_p += seglen;
		remainder -= seglen;
		free -= seglen;

		/* Increment the coder */
		
		CDR_INC(cod_p->cdr_buf_p->cdrb_pos_p, seglen);
		cod_p->cdr_buf_p->cdrb_used += seglen;
		cod_p->cdr_buflen += seglen;

#ifndef GIOP_NO_AUTOFRAG
		if (!do_auto_frag)
			continue;
		
		/* Send the fragment */

		do_auto_frag = false;
			
		if (giop_auto_frag(cod_p->cdr_giop_p) != GIOP_OK)
		{
			ret = CDR_EAUTO_FRAG;
			break;
		}

		free = cod_p->cdr_buf_p->cdrb_len 
			- cod_p->cdr_buf_p->cdrb_used;
#endif
	}

	PL_TRACE("cdr1",("cdr_encode_nbytes(): Exit; Encoding done..."));

	/* Note ONLY return from here, so we can restore the giop */

#ifndef GIOP_NO_AUTOFRAG
	cod_p->cdr_giop_p = cdr_giop_p;
#endif
	return ret;
}








#ifdef TEST_DRIVER

/*------------------------------------------------------------------------
 *	CDR Unit test driver.
 */

enum test_enum {red, green, blue};

/* Type values */

#define CDR_BYTE	'B'
#define CDR_ENUM	red
#define CDR_OCTET	'O'
#define CDR_BOOL	1
#define CDR_SHORT	-4321
#define CDR_USHORT	4321
#define CDR_LONG	-123456789
#define CDR_ULONG	123456789
#define CDR_FLOAT	-654.321
#define CDR_DOUBLE	-9876.54321
#define CDR_STRING	"string"

#define CDR_NSTRING	"string_n"
#define CDR_NSTRLEN	9		/* includes null */

/* Print failed coding of the type */

#define EFAIL(typ)	printf("** encoding of type %s FAILED\n",typ)
#define DFAIL(typ)	printf("** decoding of type %s FAILED\n",typ)

#define	OK		CDR_OK

/* Default buffer length */

#define BUFLEN		24


/* dummy buffer allocation */

CDRBufferT *
newBuf(size_t min_bytes)
{
	size_t		len;
	CDRBufferT	*buf_p	= (CDRBufferT *)ITRT_MALLOC(sizeof(CDRBufferT));

	len = min_bytes;

	if (!min_bytes)
		len = BUFLEN;
	else if (min_bytes < CDR_MIN_BUFSZ)
		len = CDR_MIN_BUFSZ;

	buf_p->cdrb_buffer_p =
#ifdef _WIN32
		buf_p->cdrb_pos_p = (unsigned char *)ITRT_MALLOC(len);
#else
		buf_p->cdrb_pos_p = (unsigned char *)GIOP_MALLOC(len);
#endif
	buf_p->cdrb_len = len;

	printf("cdr1,newBuf: return %d byte buffer at %x",len, buf_p));
			
	PL_TRACE("cdr1",("newBuf(): return %d byte buffer at %x",\
			len, buf_p));

	return buf_p;
}


/* dummy buffer de-allocation */

void
freeBuf(CDRBufferT *buf_p)
{
	if (!buf_p)
		return;
	
	if (buf_p->cdrb_buffer_p)
		GIOP_FREE(buf_p->cdrb_buffer_p);
	printf("\nFreeing buffer \n");
	PL_TRACE("cdr1",("freeBuf(): free at %x",buf_p?buf_p:0));

	buf_p->cdrb_next_p = 0;
	
	ITRT_FREE(buf_p);
}

void do_coding();
void do_swapping();

#ifdef _WIN32x
#include <io.h>
#endif

int
main()
{
	int	i;
	
	/* Coding test with various buffer sizes */

	for (i = 8; i < 32; i++)
	{
		do_coding(i);
	}

	/* Byte swapping calls */

	do_swapping();

	return 0;
}

/* Write coder contents to a data file (socket in the real case) */

unsigned long
write_coder(CDRCoderT *cod_p, int fd)
{
	unsigned long 	nbytes;
	unsigned long	len;
	CDRBufferT 	*buf_p;
	
	nbytes = 0;
	printf("\n xx  x x  warning mns fucked sssdsf ");	
	for (buf_p = cod_p->cdr_start_buf_p; buf_p; buf_p = buf_p->cdrb_next_p)
	{
	        /* len = buf_p->cdrb_len - buf_p->cdrb_free; */

	        len = (long) buf_p->cdrb_used;

		write (fd, buf_p->cdrb_buffer_p, len);
		
		nbytes+= len;

		/* Ignore unused trailing buffers */

		if (nbytes >= cod_p->cdr_buflen)
			break;
	}
	
	return nbytes;
}

/* Read data file contents into the coder */

unsigned long
read_coder(CDRCoderT *cod_p, int fd, unsigned long nbytes)
{
	return read(fd, cod_p->cdr_buf_p->cdrb_buffer_p, nbytes);
}


/*
 * do_coding()
 *
 * Purpose:
 *
 *	Encode and   decode each   type using  the    specified buffer
 *	size. The  data is written to  a data file  and re-read before
 *	decoding.
 *
 * Returns:
 *	void
 */
void
do_coding(int buflen)
{
	/* Items to code */

	unsigned char		c_byte 		= CDR_BYTE;
	enum test_enum		c_enum 		= CDR_ENUM;
	unsigned char		c_octet 	= CDR_OCTET;
	unsigned char		c_bool 		= (unsigned char)CDR_BOOL;
	short			c_short 	= CDR_SHORT;
	unsigned short		c_ushort 	= CDR_USHORT;
	long			c_long 		= CDR_LONG;
	unsigned long		c_ulong 	= CDR_ULONG;
	float			c_float 	= CDR_FLOAT;
	double			c_double 	= CDR_DOUBLE;
	char			c_string[30];
	char			c_nstring[30];

	char			*c_string_p,
	                        *c_nstring_p;
	unsigned long		str_len;

	unsigned long		nbytes;
	int			data_fd;
	
	CDRCoderT		coder;
	CDRBufferT		*buf_p;
	
	/* Init Coder */
	CDRInit(&coder, CDR_BYTE_ORDER, buflen);
	
	/* Auto buffer allocation/de-allocation functions */
	CDRAlloc(&coder, newBuf);
	CDRDealloc(&coder, freeBuf);

	/* Set mode to encode */
	CDRMode(&coder, cdr_encoding);
	
	/* Encode everything */

	if (OK != CDRCodeUChar(&coder, &c_byte))     EFAIL("byte");
	if (OK != CDRCodeEnum(&coder, (unsigned long *)&c_enum)) 
		                                    EFAIL("enum");
	if (OK != CDRCodeOctet(&coder, &c_octet))   EFAIL("octet");
	if (OK != CDRCodeBool(&coder, &c_bool))     EFAIL("boolean");
	if (OK != CDRCodeShort(&coder, &c_short))   EFAIL("short");
	if (OK != CDRCodeUShort(&coder, &c_ushort)) EFAIL("ushort");
	if (OK != CDRCodeLong(&coder, &c_long))     EFAIL("long");
	if (OK != CDRCodeULong(&coder, &c_ulong))   EFAIL("ulong");
	if (OK != CDRCodeFloat(&coder, &c_float))   EFAIL("float");
	if (OK != CDRCodeDouble(&coder, &c_double)) EFAIL("double");

	strcpy(c_string, CDR_STRING);
	c_string_p = c_string;
	if (OK != CDRCodeString(&coder, &c_string_p))  EFAIL("string");

	strcpy(c_nstring, CDR_NSTRING);
	c_nstring_p = c_nstring;
	str_len = CDR_NSTRLEN;

	if (OK != CDRCodeNString(&coder, &c_nstring_p, &str_len))  
		                                    EFAIL("string");

	/* Create a data file */

	if ((data_fd = open("cdrt.data", O_WRONLY|O_CREAT, 0666)) < 0)
	{
		perror("Open cdrt.data for writing failed: ");
		exit(1);
	}
	

	/* Write the buffer to the data file */

	CDRRewind(&coder, false);
	nbytes = write_coder(&coder, data_fd);
	
	close(data_fd);
	
	PL_TRACE("cdr1",("do_coding(): wrote %d bytes of coded data",nbytes));


	/* Open the data file for reading */

	if ((data_fd = open("cdrt.data", O_RDONLY)) < 0)
	{
		printf("Open cdrt.data for reading failed\n");
		exit(1);
	}

	if (CDRNeedBuffer(&coder, nbytes) != CDR_OK)
	{
		printf("Failed to get %d byte buffer\n", nbytes);
		exit(1);
	}
		
	/* Read in the buffer */

	if (read_coder(&coder, data_fd, nbytes) != nbytes)
	{
		printf("Failed to read %d bytes of data\n", nbytes);
		exit(1);
	}

	close(data_fd);

	/* Set mode to decode & rewind buffer */

	CDRMode(&coder, cdr_decoding);

	CDRReset(&coder, false);
	CDRRewind(&coder, true);
	
	c_string_p = c_nstring_p = (char *)0;

	/* Decode everything */

	if (OK != CDRCodeUChar(&coder, &c_byte))     DFAIL("byte");
	if (OK != CDRCodeEnum(&coder, (unsigned long *)&c_enum))
		                                    DFAIL("enum");
	if (OK != CDRCodeOctet(&coder, &c_octet))   DFAIL("octet");
	if (OK != CDRCodeBool(&coder, &c_bool))     DFAIL("boolean");
	if (OK != CDRCodeShort(&coder, &c_short))   DFAIL("short");
	if (OK != CDRCodeUShort(&coder, &c_ushort)) DFAIL("ushort");
	if (OK != CDRCodeLong(&coder, &c_long))     DFAIL("long");
	if (OK != CDRCodeULong(&coder, &c_ulong))   DFAIL("ulong");
	if (OK != CDRCodeFloat(&coder, &c_float))   DFAIL("float");

	PL_TRACE("cdr1",("do_coding(): READ %d BYTES so far",coder.cdr_buflen));

	if (OK != CDRCodeDouble(&coder, &c_double)) DFAIL("double");
	if (OK != CDRCodeString(&coder, &c_string_p))  DFAIL("string");
	if (OK != CDRCodeNString(&coder, &c_nstring_p, &str_len))  
		                                    DFAIL("string");

	/* Print results */

	printf("--------------------------------------------\n");
	printf("CDR coding test              (BUF_LEN = %d)\n\n",buflen);
	printf(" 		Encoded			Decoded\n");
	printf(" Type		Value			Value\n");
	printf(" ---------	-------			-------\n");
	printf(" UChar		%c			%c\n", 
	       CDR_BYTE, c_byte);
	printf(" Enum		%ld			%ld\n", 
	       CDR_ENUM, c_enum);
	printf(" Octet		%c			%c\n", 
	       CDR_OCTET, c_octet);
	printf(" Boolean	%d			%d\n", 
	       CDR_BOOL, (int)c_bool);
	printf(" Short		%d			%d\n", 
	       CDR_SHORT, c_short);
	printf(" UShort		%d			%d\n", 
	       CDR_USHORT, c_ushort);
	printf(" Long		%ld		%ld\n", CDR_LONG, c_long);
	printf(" ULong		%ld		%ld\n", CDR_ULONG, c_ulong);
	printf(" Float		%f		%f\n", CDR_FLOAT, c_float);
	printf(" Double		%g		%g\n", CDR_DOUBLE, c_double);
	printf(" String	       >%s<			>%s<\n", CDR_STRING, 
		c_string_p?c_string_p:"<null>");
	printf(" NString       >%s<		>%s<\n\n", CDR_NSTRING, 
		c_nstring_p?c_nstring_p:"<null>");

	printf("Total Byte encoded: %d\n", nbytes);
	printf("Total Byte decoded: %d\n\n", coder.cdr_buflen);
	
	CDRFree(&coder);
}


/*
 * do_swapping()
 *
 * Purpose:
 *	Simple byte swapping test. It swaps each value twice to return
 *	to the original value.
 *
 * Returns:
 *	void
 */
void
do_swapping()
{
	short			c_short_sw;
	long			c_long_sw;
	double			c_double_sw;

	c_short_sw = CDR_SHORT;
	c_long_sw = CDR_LONG;
	c_double_sw = CDR_DOUBLE;
	
	printf("Byte swapping test (value is doubly swapped)\n\n");

	CDR_SWAP_2(c_short_sw);
	CDR_SWAP_2(c_short_sw);

	CDR_SWAP_4(c_long_sw);
	CDR_SWAP_4(c_long_sw);

	CDR_SWAP_8(c_double_sw);
	CDR_SWAP_8(c_double_sw);

	printf(
	       " 2 byte: Orig val = %d, must = %d\n", 
	       CDR_SHORT, c_short_sw
	);

	printf(" 4 byte: Orig val = %ld, must = %ld\n",
	       CDR_LONG, c_long_sw
        );

	printf(" 8 byte: Orig val = %g, must = %g\n",
	       CDR_DOUBLE, c_double_sw
	);
	
	printf(" \nEnd of test.\n");
}


/* TEST_DRIVER */
#endif
/*
 * Author:          Iona Technologies Ltd
 *
 * Description:
 *
 *	Internet Inter-ORB Protocol (IIOP) transport specific functions
 *	used by the GIOP API for TCP/IP communication. These are a TCP/IP
 *	implementation of the generic GIOP Connection specific API calls
 *	and do not require direct application access.
 *
 *  Copyright (c) 1993-7 Iona Technologies Ltd.
 *             All Rights Reserved
 *  This is unpublished proprietary source code of Iona Technologies
 *  The copyright  notice  above  does not  evidence  any  actual or
 *  intended publication of such source code.
 */

#include "iiopP.h"


/* static functions */

static 
GIOPStatusT 
iiop_connectTo (TCPCtrlBlkT *tcp_p, const char* host, int port, int *fd);

static GIOPStatusT 
iiop_closefd (GIOPStateT *giop, boolean is_listen_fd) ;

static void
iiop_get_iiop(
	octet 		*data_p,
	unsigned long 	data_len,
	IIOPBody_1_0T	*iiop_p
);

static IIOPStatusT iiop_error_code();
static GIOPStatusT iiopError(TCPCtrlBlkT *tcp_p, IIOPStatusT iop_stat);


#ifdef TORNADO
extern "C"
{
    int taskDelay (int);
    int sysClkRateGet ();
}
#endif 

GIOPStatusT 
iiop_connect (GIOPStateT *giop) 
{
	IORTaggedProfileT 	*tprof_p;
	IIOPBody_1_0T 		iiop_pb; 
    	TCPCtrlBlkT	  	*tcp_p;	
	char 			*host_p;
	int 			port;
	int 			fd;

	GIOPStatusT 		errStatus;
	unsigned long 		attempt;
	unsigned long 		index;


	attempt = 3;
	index = 0;
	
	/* get the tcp control block */

	tcp_p = &giop->gs_ctrl_blk.cb_profile[TAG_INTERNET_IOP].cp_tcp;
	
	/* read the index of the current profile */

	index = giop->gs_ctrl_blk.cb_ior_profile;

	/* access this profile */

	tprof_p = &giop->gs_ctrl_blk.cb_ior_p->ior_profiles_p[index];

	/*
	 *	Get the  common   (1.0/1.1) IIOP  Profile  Body values
	 *	from the profile_data encapsulation.  We only need the
	 *	host and port here.
	 */
	if (!tprof_p->tp_profile_data_p)
		return GIOP_ENULL_PROFILE;

	iiop_get_iiop(
		tprof_p->tp_profile_data_p, 
		tprof_p->tp_profile_len,
	        &iiop_pb
	);

	/* access the parameters needed for this socket connection */

	host_p = iiop_pb.ib_host_p;
	port = iiop_pb.ib_port;

	for(;;)
	{
	       errStatus = iiop_connectTo (tcp_p, host_p, port, &fd);
	       attempt--;

	       if (errStatus != GIOP_OK) 
	       {
		      if (!attempt)
			  /*
			  // That was the very last try...
			  */
			  return errStatus;

#ifdef TORNADO
		      taskDelay (2 * sysClkRateGet ());
#else
#ifdef _WIN32		      
		      Sleep (2000);   /* sleep for 2 seconds */
#else
		      sleep (2);   /* sleep for 2 seconds */
#endif
#endif
	       }
	       else
	       {
		       /*
			// No exception -- everything OK !!
			*/

		       PL_TRACE("iiop2",\
		       ("iiop_connect(): new msg fd is %d", fd));

		       /* store the message socket descriptor */

		       tcp_p->tcp_msg_fd = fd;

		       /*
			*	Set the object key in the giop control block
			*/
		       giop->gs_ctrl_blk.cb_objkey_p = iiop_pb.ib_objkey_p;
		       giop->gs_ctrl_blk.cb_objkey_len = iiop_pb.ib_objkey_len;


		       /*
			*	Now    that we are connected, determine
			*	from the IOR the  IIOP revision and set
			*	the   GIOP     revision       we    use
			*	accordingly.  If the  server was    1.0
			*	based, we  will be able  to talk  to it
			*	using GIOP 1.0.
			*/
		       giop->gs_major = iiop_pb.ib_major;
		       giop->gs_minor = iiop_pb.ib_minor;
		       
		       return GIOP_OK;
	       }
       }  /* while */
}



GIOPStatusT 
iiop_accept (GIOPStateT *giop)
{
	int 			fd; 
	int 			conn_fd;
	IORTaggedProfileT 	*tprof_p;
	TCPCtrlBlkT	      	*tcp_p;

	IIOPBody_1_0T 		iiop_pb; 
	unsigned long		index;
	
	struct sockaddr_in 	dummy;
	int 			dummylen;
	struct linger l;



	dummylen = sizeof (dummy);
    
	/* get the socket FD from the tcp control block */

	tcp_p = &giop->gs_ctrl_blk.cb_profile[TAG_INTERNET_IOP].cp_tcp;
	

	/* Haven't done a _listen() yet! */

	if ((fd = tcp_p->tcp_listen_fd) < 0)
		return iiopError(tcp_p, IIOP_EINV_FD);
		
	PL_TRACE("iiop2",("iiop_accept(): listen fd is %d", fd));

	conn_fd = IIOP_ACCEPT (fd, (struct sockaddr*)&dummy, &dummylen);

	PL_TRACE("iiop2",("iiop_accept(): msg fd from accept is %d", conn_fd));

#ifdef _WIN32
	if (conn_fd == E_INVALID_SOCKET)
#else
	if (conn_fd < 1)
#endif
	{
	    return iiopError(tcp_p, iiop_error_code());
	}
	
	/* store the message FD in the tcp control block */

	tcp_p->tcp_msg_fd = conn_fd;

	/* get the current profile */

	index = giop->gs_ctrl_blk.cb_ior_profile;
	tprof_p = &giop->gs_ctrl_blk.cb_ior_p->ior_profiles_p[index];

	if (!tprof_p->tp_profile_data_p)
		return GIOP_ENULL_PROFILE;

	iiop_get_iiop(
		tprof_p->tp_profile_data_p, 
		tprof_p->tp_profile_len,
	        &iiop_pb
	);
	
	/*
	 *	Set the object key in the giop control block
	 */
	giop->gs_ctrl_blk.cb_objkey_p = iiop_pb.ib_objkey_p;
	giop->gs_ctrl_blk.cb_objkey_len = iiop_pb.ib_objkey_len;

	


#ifdef _WIN32   /*
				OPTION=SO_LINGER, INTERVAL=2, TYPE OF CLOSE=Graceful,
				WAIT FOR CLOSE=Yes.
				SO_LINGER used in conjunction with Shutdown should give
				us a blocking close that block until the remaining data is
				read or the timeout expires.
				*/

		l.l_onoff = 1;
		l.l_linger = 2;
		setsockopt(conn_fd, SOL_SOCKET, SO_LINGER, (const char *)&l, sizeof(struct linger));
	
#endif
	return GIOP_OK;
}


GIOPStatusT
iiop_reject (GIOPStateT *giop)
{
	GIOPStatusT errStatus;

	if ((errStatus = iiop_accept(giop)) != GIOP_OK)
		return errStatus;
	
	return iiop_close(giop);
}


GIOPStatusT
iiop_listen (GIOPStateT *giop) 
{
	short 			shortBindPort;
	int 			listenfd;
	int 			on;

	IORTaggedProfileT 	*tprof_p;
	IIOPBody_1_0T 		iiop_pb; 
    	TCPCtrlBlkT	      	*tcp_p;
	unsigned long 		index;
	unsigned long 		port;
	
#ifdef NOKIA9K
	SocketAddress		sockadr;
#else
	struct sockaddr_in 	sockadr;
#endif

	shortBindPort = 0;
	listenfd = 0;
	on = 1;
	
	/* read the index of the current profile */

	index = giop->gs_ctrl_blk.cb_ior_profile;

	/* access this profile */

	if (!(tprof_p = &giop->gs_ctrl_blk.cb_ior_p->ior_profiles_p[index]))
		return GIOP_ENULL_PROFILE;

	if (!tprof_p->tp_profile_data_p)
		return GIOP_ENULL_PROFILE;

	/*
	 *	get the profile data - only interested  in port so we can
	 *	merrily cast it to IIOPBody_1_0T structure even if its
	 *	really IIOPBody_1_1T
	 */
	iiop_get_iiop(
		tprof_p->tp_profile_data_p, 
		tprof_p->tp_profile_len, 
		&iiop_pb
	);

	port = iiop_pb.ib_port;

	/* get the tcp control block */

	tcp_p = &giop->gs_ctrl_blk.cb_profile[TAG_INTERNET_IOP].cp_tcp;

	/* Already have a listen socket */

	if (tcp_p->tcp_listen_fd != -1)
		return GIOP_OK;
	

    	if ((listenfd = IIOP_CREATESOCKET (AF_INET, SOCK_STREAM, 0)) == E_INVALID_SOCKET) 
	{
		PL_TRACE("iiop2",\
		     ("iiop_listen(): socket() failed; listen fd is bogus"));

		return iiopError(tcp_p, iiop_error_code());
	}      

	PL_TRACE("iiop2",\
		("iiop_listen(): listen fd from socket is %d", listenfd));

	shortBindPort = (short)port;

#ifdef NOKIA9K

#else
	sockadr.sin_family = AF_INET;
	sockadr.sin_addr.s_addr = htonl(INADDR_ANY);
	sockadr.sin_port = htons(shortBindPort);

	if (
		setsockopt(
		   listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)
		) < 0
	) 
	{
	    return iiopError(tcp_p, iiop_error_code());
        }
#endif

#ifdef NOKIA9K
	if (IIOP_BIND(fd, sockadr, 0) == ??)
#else
	if (IIOP_BIND(
		listenfd, (struct sockaddr *)&sockadr, sizeof(sockadr)
	    ) != 0
	) 
#endif
	{
		IIOPStatusT	ret_code;
		
		ret_code = iiop_error_code();

		/* 
		 *	bind() failed on the open socket - close it 
		 *	and return error 
		 */
		IIOP_CLOSE(listenfd);

		return iiopError(tcp_p, ret_code);
	}

	if (IIOP_LISTEN(listenfd, 5) != 0)
	{
		return iiopError(tcp_p, iiop_error_code());
	}

	/* store the listen fd in the tcp control block */

	tcp_p->tcp_listen_fd = listenfd;

	/*
	 *	Set   the GIOP revision    according to  the  revision
	 *	specified  in  the   IOR.   This way,   if  the server
	 *	application decides to use  1.0 IORs, we store this in
	 *	the GIOPStateT.
	 */
	giop->gs_major = iiop_pb.ib_major;
	giop->gs_minor = iiop_pb.ib_minor;

	return GIOP_OK;
}


/* Stop listening on the server side */

GIOPStatusT 
iiop_stoplisten (GIOPStateT *giop) 
{
	return iiop_closefd(giop, true);
}	


/* Close the message socket */

GIOPStatusT 
iiop_close (GIOPStateT *giop) 
{
	return iiop_closefd(giop, false);
}


GIOPStatusT 
iiop_send(GIOPStateT *giop)
{
    	TCPCtrlBlkT	  *tcp_p;	
	CDRBufferT	  *cdr_buf_p;    
   	int 		  numWritten;
	int 		  fd;
	unsigned char 	  *buf_p;
	unsigned long     buf_len;
	unsigned long     total;


	/* get the message socket FD */

	tcp_p = &giop->gs_ctrl_blk.cb_profile[TAG_INTERNET_IOP].cp_tcp;
	
	if ((fd = tcp_p->tcp_msg_fd) < 0)
		return iiopError(tcp_p, IIOP_EINV_FD);
	
	tcp_p->tcp_nbytes = 0;

	/* access first buffer in coder */ 

	if (!(cdr_buf_p = giop->gs_coder_p->cdr_start_buf_p))
		return GIOP_ENOSPACE;

	/* total bytes to write. Could be zero.. */

	total = giop->gs_coder_p->cdr_buflen;

	/* get buffer pointer and length from coder */ 

	buf_p = cdr_buf_p->cdrb_buffer_p;
	buf_len = cdr_buf_p->cdrb_used;
	 
	PL_TRACE("iiop2",("iiop_send (): send (msg) fd is %d",fd));

   	numWritten = 0;

	while ((buf_p != NULL) && (total > 0))
	{
        #ifdef TORNADO
		if ((numWritten = 
                       IIOP_WRITE_TCP (fd, (CHAR_CAST)buf_p, buf_len)) > 0
		)
        #else
		if ((numWritten = 
                       IIOP_WRITE (fd, (CHAR_CAST)buf_p, buf_len)) > 0
		)
        #endif
		{
			/* Update the tcp control block nbytes count */

			tcp_p->tcp_nbytes += numWritten;
		}
			
		PL_TRACE("iiop2",("numWritten - %d\n", numWritten));
		if ((unsigned long)numWritten < buf_len)
			return iiopError(tcp_p, iiop_error_code());

		total -= numWritten;
		
		cdr_buf_p = cdr_buf_p->cdrb_next_p;

		/* check if another buffer to go */

		if (cdr_buf_p)
		{
			/* next buffer */

			buf_p = cdr_buf_p->cdrb_buffer_p;
			buf_len = cdr_buf_p->cdrb_used;
		}
		else
			buf_p = NULL;
	}

    	return GIOP_OK;
}


GIOPStatusT 
iiop_recv(GIOPStateT *giop, unsigned long nbytes)
{
	TCPCtrlBlkT	      	*tcp_p;	
	CDRBufferT	      	*cdr_buf_p;
	unsigned char		*start_pos_p;
	unsigned char		*curr_pos_p;

	int 			fd;

	unsigned long 		numToRead;
	unsigned long 		numJustRead;
	unsigned long 		totalSoFar;
	unsigned long 		leftInSeg;

	/* read the socket FD */

	tcp_p = &giop->gs_ctrl_blk.cb_profile[TAG_INTERNET_IOP].cp_tcp;

	if ((fd = tcp_p->tcp_msg_fd) < 0)
		return iiopError(tcp_p, IIOP_EINV_FD);

	tcp_p->tcp_nbytes = 0;
	
	/* Now check for size error (after tcp_nbytes is set) */

	if (nbytes == 0)
		return GIOP_OK;

	PL_TRACE("iiop2",("iiop_recv (): recv (msg) fd is %d",fd));

	/* get the current buffer in the coder */

	if (!(cdr_buf_p = giop->gs_coder_p->cdr_buf_p))
		return GIOP_ENOSPACE;

	/*
	 *	Start at the current pos_p in the buffer. The read may
	 *	be done piece by piece so we must maintain alignment.
	 */
    	start_pos_p = cdr_buf_p->cdrb_pos_p;

	/* check size is big enough */

    	if (nbytes > (cdr_buf_p->cdrb_len - cdr_buf_p->cdrb_used))
		return iiopError(tcp_p, IIOP_EMSG_TOO_BIG);
	
	numToRead = numJustRead = totalSoFar = leftInSeg = 0;

	/*
	 // Now loop until read all data
	 */
	do
	{
		curr_pos_p = start_pos_p + totalSoFar;
		numToRead = nbytes - totalSoFar;
		do
		{
			errno = 0;
                #ifdef TORNADO
			numJustRead = IIOP_READ_TCP(fd,(char *)curr_pos_p, numToRead);
                #else
			numJustRead = IIOP_READ(fd,(char *)curr_pos_p, numToRead);
                #endif
			if (numJustRead > 0)
				tcp_p->tcp_nbytes += numJustRead;
		}
	
#ifdef _WIN32
		while (
			(numJustRead == SOCKET_ERROR) 
			&& (WSAGetLastError() == WSAEINTR)
		);
#else
		while ((errno == EINTR) && (numJustRead == -1));
#endif

#ifdef _WIN32
		if (numJustRead == SOCKET_ERROR)
#else
		if (numJustRead == -1)
#endif
		{
			return iiopError(tcp_p, iiop_error_code());
		}

		totalSoFar += numJustRead;

	} 
	while (numJustRead && (totalSoFar < nbytes));
	
	if (totalSoFar == nbytes)
	{
		/*
		 *	Set the coder total length and buffer length
		 *	REVISIT - if this code changes to reading into
		 *	>1 buffer, then the cdrb_used will have to be set
		 *	in the above loop, as each buffer is filled.
		 */

		/* if decoding, used should be zero after the read! REVISIT */
		/* cdr_buf_p->cdrb_used = */

		/* 
		 * buflen indicates the # bytes read and used starts at
		 * zero and is incremented as you decode 
		 */
		giop->gs_coder_p->cdr_buflen += nbytes;

		return GIOP_OK;
	}
	if (!numJustRead)
		return iiopError(tcp_p, IIOP_EZERO_READ);

	return iiopError(tcp_p, iiop_error_code());
}



/*-----------------------------------------------------------------------
 *	IIOP static functions
 */

static 
GIOPStatusT 
iiop_connectTo (TCPCtrlBlkT *tcp_p, const char* host, int port, int *fd)
{
	struct in_addr 	 	inAddr;
	struct sockaddr_in 	to_sock;
#ifndef TORNADO
	struct hostent 		*hp;
#endif
	int 			connecterr;
	unsigned long 		IPaddr;

#ifdef NOKIA9K

	
	/*
	 *	Establish an address structure.
	 */

int dave = 0;
	/*
	 *	Create the socket.
	 */
	if ((*fd = IIOP_CREATESOCKET(0, SDT_STREAM, 0)) == )
	{
		errno = ??;

		iiopError(tcp_p, iiop_error_code());
	}
	
	/*
	 *	Make the connection
	 */
	if (
		(connecterr = IIOP_CONNECT(
		        fd,
			(SocketAddres *)&theAddr,
			SOCKET_NO_TIMEOUT
		        )
                ) == ??)
	{
	}

#else
#ifdef TORNADO

	IPaddr = inet_addr(host);

	if ( IPaddr == 0xffffffff)
	{
		IPaddr = hostGetByName (host);
	}

	if (IPaddr !=  0xffffffff)
		inAddr.s_addr = IPaddr;
	else
		return iiopError(tcp_p, IIOP_EINV_HOST);

#else /* !TORNADO */
	IPaddr = inet_addr (host);

	if (IPaddr != -1) 
		inAddr.s_addr = IPaddr;
    	else 
	{
		if (!(hp = IIOP_GETHOSTBYNAME(host)))
			return iiopError(tcp_p, IIOP_EINV_HOST);

		memcpy(&inAddr, hp->h_addr, hp->h_length);
    	}
#endif /* !TORNADO */

   	to_sock.sin_family = AF_INET;
	to_sock.sin_addr = inAddr;
    	to_sock.sin_port = htons( (unsigned short) port);

    	if (
		(*fd = IIOP_CREATESOCKET (AF_INET, SOCK_STREAM, 0)) 
		== E_INVALID_SOCKET
	) 
		return iiopError(tcp_p, iiop_error_code());

        connecterr = IIOP_CONNECT(*fd, (struct sockaddr *) &to_sock, \
			sizeof(to_sock));

	if (connecterr < 0)
	{
		IIOP_SHUTDOWN(*fd);
		IIOP_CLOSE(*fd);

		return iiopError(tcp_p, iiop_error_code());
	}
#endif

#ifdef _WIN32x
	/* Set the LINGER option on the socket */

	{
		struct linger l;
		l.l_onoff = 1;
		l.l_linger = 0;
		setsockopt(
			*fd, 
			SOL_SOCKET, SO_LINGER, 
			(const char *)&l, sizeof(struct linger)
		);
	}
#endif
       
	return IIOP_OK;
}


static GIOPStatusT 
iiop_closefd (GIOPStateT *giop, boolean is_listen_fd) 
{
	FdT  		*fd_p;
	TCPCtrlBlkT	*tcp_p;
	
	/* get the tcp control block */
	
	tcp_p = &giop->gs_ctrl_blk.cb_profile[TAG_INTERNET_IOP].cp_tcp;
	
	/* read the socket FD to be closed (either listen or msg fd) */
	
	if (is_listen_fd)
		fd_p = &tcp_p->tcp_listen_fd;
	else
		fd_p = &tcp_p->tcp_msg_fd;
	
	if (*fd_p < 0)
		return iiopError(tcp_p, IIOP_EINV_FD);
	
	/* REVISIT - only shutdown on the msg socket?? */

	if (!is_listen_fd)
		IIOP_SHUTDOWN(*fd_p);

	IIOP_CLOSE(*fd_p);		/* perform TCP/IP operation */
	*fd_p = -1;			/* reset */
	
	return GIOP_OK;
}


/*
 * iiop_get_iiop()
 *
 * Purpose:
 *
 *	Given an encapsulation octet  stream, place it into a CDRCoder
 *	and decode it as IIOP Profile  Body data. The IIOP revision is
 *	not important  because  we  only  decode data common   to  all
 *	revisions.
 *
 * Returns:
 *	void
 */
static void
iiop_get_iiop(
	octet 		*data_p,
	unsigned long 	data_len,
	IIOPBody_1_0T	*iiop_p
)
{
	CDRCoderT	cod;
	CDRBufferT	cdr_buf;
	
	CDREncapInit(&cod, &cdr_buf, data_p, data_len, cdr_decoding);

	IOREncapIIOP(
		&cod, 
		&iiop_p->ib_major,
		&iiop_p->ib_minor,
		&iiop_p->ib_host_p,
		&iiop_p->ib_port,
		&iiop_p->ib_objkey_len,
		&iiop_p->ib_objkey_p
	);
}


/* Set the IIOP error status and return GIOP_ETRANSPORT */

static GIOPStatusT
iiopError(TCPCtrlBlkT *tcp_p, IIOPStatusT iop_stat)
{
	if ((tcp_p->tcp_status = iop_stat) == IIOP_EUNKNOWN)
#ifdef _WIN32
		/* warning: WSAGetLastError shouldn't clear the error but could
		** causing the second call to return noerror
		*/
		tcp_p->tcp_unknown_status = WSAGetLastError(); 
#else
		tcp_p->tcp_unknown_status = errno;
#endif

	return GIOP_ETRANSPORT;
}


static IIOPStatusT 
iiop_error_code()
{
	IIOPStatusT	errStatus;

#ifndef _WIN32
	switch (errno)
	{
	case EBADF:
		errStatus = IIOP_EINV_FD; 
		break; 
	case ENOTSOCK:
		errStatus = IIOP_EBAD_SOCK;
		break; 
	case EOPNOTSUPP:
		errStatus = IIOP_EBAD_SOCKTYPE; 
		break; 
	case EFAULT:
		errStatus = IIOP_EBAD_INPUT; 
		break; 
	case EADDRNOTAVAIL:
		errStatus = IIOP_EINV_IPADDR; 
		break; 
	case EINVAL:
		errStatus = IIOP_ESOCK_INUSE; 
		break; 
	case EACCES:
		errStatus = IIOP_ENOPERM; 
		break; 
	case EINTR:
		errStatus = IIOP_EINTR; 
		break; 
	case EIO:
		errStatus = IIOP_EIO_ERROR; 
		break; 
	case EMSGSIZE:
	case EFBIG:
		errStatus = IIOP_EMSG_TOO_BIG; 
		break; 
	case ENOBUFS:
		errStatus = IIOP_ENO_BUFFER; 
		break; 
	case EMFILE:
	case ENFILE:
		errStatus = IIOP_ENO_BUFFER; 
		break; 
	case EAFNOSUPPORT:
		errStatus = IIOP_EBAD_SOCKADDR; 
		break; 
	case EISCONN:
		errStatus = IIOP_EIS_CONN; 
		break; 
	case ETIMEDOUT:
		errStatus = IIOP_ETIMEDOUT; 
		break; 
	case ECONNREFUSED:
		errStatus = IIOP_ECONN_REFUSED; 
		break; 
	case ENETUNREACH:			/* network unreachable */
#ifndef TORNADO
	case ENOLINK:				/* link to host lost */
#endif
	case ENXIO:				/* hangup occured */
		errStatus = IIOP_ENETWORK_ERROR;
		break; 
	case EADDRINUSE:
		errStatus = IIOP_EADDR_INUSE; 
		break; 
	case EAGAIN:
		errStatus = IIOP_ENO_BLOCK;
		break;
	case ECONNRESET:
		errStatus = IIOP_ECONN_CLOSED; 
		break; 
		
	default:
		/* Unknown error */

		errStatus = IIOP_EUNKNOWN; 
		break; 
	}
#else	
	switch (WSAGetLastError())
	{
	case WSAEBADF:
		errStatus = IIOP_EINV_FD; 
		break; 
	case WSAENOTSOCK:
		errStatus = IIOP_EBAD_SOCK;
		break; 
	case WSAEOPNOTSUPP:
		errStatus = IIOP_EBAD_SOCKTYPE; 
		break; 
	case WSAEFAULT:
		errStatus = IIOP_EBAD_INPUT; 
		break; 
	case WSAEADDRNOTAVAIL:
		errStatus = IIOP_EINV_IPADDR; 
		break; 
	case WSAEINVAL:
		errStatus = IIOP_ESOCK_INUSE; 
		break; 
	case WSAEACCES:
		errStatus = IIOP_ENOPERM; 
		break; 
	case WSAEINTR:
		errStatus = IIOP_EINTR; 
		break; 
/*	case WSAEIO:*/
		errStatus = IIOP_EIO_ERROR; 
		break; 
	case WSAEMSGSIZE:
/*	case WSAEFBIG:*/
		errStatus = IIOP_EMSG_TOO_BIG; 
		break; 
	case WSAENOBUFS:
		errStatus = IIOP_ENO_BUFFER; 
		break; 
	case WSAEMFILE:
/*	case WSAENFILE:*/
		errStatus = IIOP_ENO_BUFFER; 
		break; 
	case WSAEAFNOSUPPORT:
		errStatus = IIOP_EBAD_SOCKADDR; 
		break; 
	case WSAEISCONN:
		errStatus = IIOP_EIS_CONN; 
		break; 
	case WSAETIMEDOUT:
		errStatus = IIOP_ETIMEDOUT; 
		break; 
	case WSAECONNREFUSED:
		errStatus = IIOP_ECONN_REFUSED; 
		break; 
	case WSAENETUNREACH:
		errStatus = IIOP_ENETWORK_ERROR;
		break; 
	case WSAEADDRINUSE:
		errStatus = IIOP_EADDR_INUSE; 
		break; 
/*	case WSAEAGAIN:*/
		errStatus = IIOP_ENO_BLOCK;
		break;
	case WSAECONNRESET:
		errStatus = IIOP_ECONN_CLOSED;
		break;

	default:
		/* Unknown error */

		errStatus = IIOP_EUNKNOWN; 
		break; 
	}
#endif	
	return errStatus;
}

