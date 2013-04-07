#define USE_EXCEPTIONS

#include <OTCB.h>
#include <Config.h>
#include <stdlib.h>
#include <stdio.h>
#include <msgQLib.h>
#include <semLib.h>
#include <qMap.h>
#include <commsMsg.h>
#include <iiopMsg.h>
#include <iiopagent.h>
#include <giop.h>
#include <skts.h>
#include <reader.h>
#include <ior.h>

#include <ObjRef.h>

#ifdef MEMCHECKRT
#include <itrtlib.h>
#endif

/* Cache contains a mapping from a file descriptor to
 * a structure representing the queue and semaphore pair
 * associated with an Orbix server
 */

FD_to_QTag IT_IIOP_cache;  

#define IIOP_NAME_LIM  512

extern "C" char* strstr (const char *, const char *);

/*
 * MACRO: extracts the giop state, message identifier, Q message
 * and the IIOP message from an opaque closure as delivered by the
 * ORB. This macro is for code health!
 */

#define SPLIT_REQUEST_CLOSURE(closure, msg, iiopmsg, msg_id, giop) \
        msg = (QCommsMsgT*) closure;                              \
        iiopmsg = (IIOPMsg::MsgT*)msg->_data_p;    \
	msg_id=((IT_IIOPMsgHdrT*)iiopmsg->_header_p)->msg.request.request_id;\
        giop = ((IT_IIOPAgent::HNDL)msg->_commsChannel)->giop_state ();

#define SPLIT_LOCATEREQUEST_CLOSURE(closure, msg, iiopmsg, msg_id, giop) \
        msg = (QCommsMsgT*) closure;                              \
        iiopmsg = (IIOPMsg::MsgT*)msg->_data_p;    \
	msg_id=((IT_IIOPMsgHdrT*)iiopmsg->_header_p)->msg.locate_request.request_id;\
        giop = ((IT_IIOPAgent::HNDL)msg->_commsChannel)->giop_state ();

/* 
 * Convenience function: used by the ORB when it wishes to get stuff 
 * from the opaque, without having to know the exact structure of the
 * opaque.
 *
 */

CDRCoderT*
IT_IIOP_get_coder (void *closure)
{
    if (closure)
    {
	// extract the handle pointer from out the closure and
	// get the GIOP state from there, then get the coder :)

        unsigned long msg_id;
        QCommsMsgT* msg;
        GIOPStateT* giop;
        IIOPMsg::MsgT* iiopmsg;
 
	SPLIT_REQUEST_CLOSURE(closure,msg,iiopmsg,msg_id,giop);

        // printf ("(stub) Retrieved GIOP coder = 0x%x\n", giop->gs_coder_p);

        // Changing the returned coder from the one in the giop state -- ie
        // giop->gs_coder_p -- to the one hept in the message.
        // The giop state one seems corrupt.
        // ASK Feb 98
        // 
	//return giop->gs_coder_p;
        return (CDRCoderT*)iiopmsg->_body_p;
        

    }

    return 0;
}

GIOPStateT *
IT_IIOP_get_giop( void *closure )
{
    if (closure)
    {
	// extract the handle pointer from out the closure and
	// get the GIOP state from there, then get the coder :)

        unsigned long msg_id;
        QCommsMsgT* msg;
        GIOPStateT* giop;
        IIOPMsg::MsgT* iiopmsg;
 
	SPLIT_REQUEST_CLOSURE(closure,msg,iiopmsg,msg_id,giop);

        // printf ("(stub) Retrieved GIOP coder = 0x%x\n", giop->gs_coder_p);
	return giop;
    }

    return 0;

}


/*
 * Called by:  server side (FRRInterface::dispatch)
 * In response To: a valid IIOP Request
 * Function :  send a pre-prepared IIOP reply message
 * Parameters: opaque datum which represents the Q message which was
 *             passed to the server as a request.          
 *
 */

int
IT_IIOP_send_reply (void* closure)
{
    if (closure)
    {
	// extract the handle pointer from out the closure and
	// get the GIOP state from there.

        unsigned long msg_id;
        QCommsMsgT* msg;
        GIOPStateT* giop;
        IIOPMsg::MsgT* iiopmsg;
 
	SPLIT_REQUEST_CLOSURE(closure,msg,iiopmsg,msg_id,giop);

	if (GIOPReplySend (giop, 0) == OK)
	{
	    return OK;
	}
    }

    return ERROR;
}

/*
 * Called by:  server side (CORBA_RequestS::convertToReply)
 * In response To: a valid IIOP Request
 * Function :  prepare a reply message with no return and out/inout
 *             parameters and GIOP_NO_EXCEPTION. 
 * Parameters: opaque datum which represents the Q message which was
 *             passed to the server as a request.          
 *
 */

int
IT_IIOP_prepare_reply_ok (void *closure)
{
    if (closure)
    {
	// extract the handle pointer from out the closure and
	// get the GIOP state from there.

        unsigned long msg_id;
        QCommsMsgT* msg;
        GIOPStateT* giop;
        IIOPMsg::MsgT* iiopmsg;
 
	SPLIT_REQUEST_CLOSURE(closure,msg,iiopmsg,msg_id,giop);
	// Set coder to encoding
	CDRMode (giop->gs_coder_p, cdr_encoding);

	if (GIOPReplyCreate (giop, 1, 0, GIOP_NO_EXCEPTION, msg_id, 0) == OK)
	{
		return OK;
	}
    }

    return ERROR;
}


/*
 * Called by:  server side
 * In response to: an IIOP request which has caused an exception to be raised
 * Function :  send a reply message with GIOP_SYSTEM_EXCEPTION and 
 *             the required parameters:
 *                      string exception_id;
 *                      unsigned long minor_code;
 *                      unsigned long completed;
 * 
 * Parameters: opaque datum which represents the Q message which was
 *             passed to the server as a request,
 *             minor code of the exception
 *             exception identifier
 *
 * Notes:  completed is always NO
 *         no vendor exception code is included in the minor_code
 */

int
IT_IIOP_system_exception (void *details,
			  unsigned long minor_code,
			  char *exception_id)
{
    unsigned long complete = 1, msg_id;
    QCommsMsgT* msg;
    GIOPStateT* giop;
    IIOPMsg::MsgT* iiopmsg;

    SPLIT_REQUEST_CLOSURE(details, msg, iiopmsg, msg_id, giop);
    if (GIOPReplyCreate (giop, 1, 0, GIOP_SYSTEM_EXCEPTION, 
			 msg_id, 0) == GIOP_OK)
    {
	CDRCodeString (giop->gs_coder_p, &exception_id);
	CDRCodeULong (giop->gs_coder_p, &minor_code);
	CDRCodeULong (giop->gs_coder_p, &complete);
	if (GIOPReplySend (giop, 0) == GIOP_OK)
	    return OK;
    }

    return ERROR;
}


/*
 * Called by:  Request.cc
 * In response to: sending a request to a server and getting back a 
 * LOCATE_FORWARD message
 * Function :  make a backup copy of a coder
 * 
 * Parameters: GIOPState of the source coder and the new coder
 */

CDRStatusT
IT_IIOP_copy_coder(GIOPStateT *giop, CDRCoderT* new_coder)
{
	return GIOP_OK;
	CDRBufferT*			cdr_buf_p;
	CDRBufferT*		buf_p;
	CDRCoderT*		old_coder;
	unsigned long     buf_len;
	unsigned long     total = 0;
	unsigned long     total_so_far = 0;
	old_coder = giop->gs_coder_p;
	/* 
	total bytes to copy, and init the new coder.
	*/
	total = old_coder->cdr_buflen;
	buf_p	= old_coder->cdr_buf_p;
	CDRInit(new_coder, old_coder->cdr_bytesex, 512);
    CDRAlloc(new_coder, (CDRBufferT* (*)(unsigned int))IT_IIOPAgent::_newbuf);
    CDRDealloc(new_coder, (void (*)(CDRBufferT*))IT_IIOPAgent::_freebuf);

	new_coder->cdr_bytesex = old_coder->cdr_bytesex;

	while (total > total_so_far )
	{
		CDRCodeNOctet( new_coder, (octet**)buf_p,  &buf_p->cdrb_len );
		total_so_far += buf_p->cdrb_len;

	/*
	 get the next buffer pointer 
	*/
	
		buf_p = buf_p->cdrb_next_p;
	}
	if (  total == total_so_far )
		return GIOP_OK;
}







/*
 * Called by:  IIOPAgent
 * In response to: an IIOP request for which no server exists
 * Function :  send a reply message with GIOP_SYSTEM_EXCEPTION and 
 *             the required parameters:
 *                      string exception_id = relevant error string;
 *                      unsigned long minor_code =xxxxx 
 *                      unsigned long completed = NO;
 * 
 * Parameters: GIOPState of the client connection
 *             message identifier of the request that caused the exception
 *
 * Notes:  parameters are fixed
 *         no vendor exception code is included in the minor_code
 *         exception is Orbix specific
 */

int
IT_IIOP_exception (GIOPStateT* giop, char* exception_id,unsigned long minor_code )
{
    unsigned long complete = 1;
    if (GIOPReplyCreate (giop, 1, 0, GIOP_SYSTEM_EXCEPTION, giop->gs_request_id, 0) == GIOP_OK)
    {
	CDRCodeString (giop->gs_coder_p, &exception_id);
	CDRCodeULong (giop->gs_coder_p, &minor_code);
	CDRCodeULong (giop->gs_coder_p, &complete);
	if (GIOPReplySend (giop, 0) == GIOP_OK)
	    return OK;
    }

    return ERROR;
}

/*
 * Called by:  IIOPAgent
 * In response to: an IIOP request for which no server exists
 * Function :  send a reply message with GIOP_SYSTEM_EXCEPTION and 
 *             the required parameters:
 *                      string exception_id = "VxWorks IIOP server not up";
 *                      unsigned long minor_code = 10582;
 *                      unsigned long completed = NO;
 * 
 * Parameters: GIOPState of the client connection
 *             message identifier of the request that caused the exception
 *
 * Notes:  parameters are fixed
 *         no vendor exception code is included in the minor_code
 *         exception is Orbix specific
 */

int
IT_IIOP_exception (GIOPStateT* giop, unsigned long minor_code)
{
    unsigned long complete = 1;
    char *exception_id = "VxWorks IIOP server not up";

    if (GIOPReplyCreate (giop, 1, 0, GIOP_SYSTEM_EXCEPTION, giop->gs_request_id, 0) == GIOP_OK)
    {
	CDRCodeString (giop->gs_coder_p, &exception_id);
	CDRCodeULong (giop->gs_coder_p, &minor_code);
	CDRCodeULong (giop->gs_coder_p, &complete);
	if (GIOPReplySend (giop, 0) == GIOP_OK)
	    return OK;
    }

    return ERROR;
}

/*
 * Called by:  IIOPAgent
 * In response to: an IIOP locate request for which no server exists
 * Function :  send a locate reply message with GIOP_UNKNOWN_OBJECT
 * Parameters: GIOPState of the client connection
 *             message identifier of the request that caused the exception
 *
 * Notes:  does not attempt to offer an alternative object key
 */

int 
IT_IIOP_not_here (GIOPStateT* giop, unsigned long id)
{
    // printf ("IT_IIOP: returning not here locate reply\n");
    if (GIOPLocateReplyCreate (giop, 1, 0,
			       GIOP_UNKNOWN_OBJECT, id) == GIOP_OK)
    {
	if (GIOPLocateReplySend (giop) == GIOP_OK)
	    return OK;
    }
    // printf ("IT_IIOP: locate reply error\n");
    return ERROR;
}

/*
 * Called by:  IIOPAgent
 * In response to: an IIOP request for which the server exists
 * Function :  send a reply message with GIOP_OBJECT_HERE 
 * Parameters: GIOPState of the client connection
 *             message identifier of the request that caused the exception
 *
 */

int 
IT_IIOP_object_here (GIOPStateT* giop, unsigned long id)
{
    // printf ("returning object here locate reply\n");
    if (GIOPLocateReplyCreate (giop, 1, 0,
			       GIOP_OBJECT_HERE, id) == GIOP_OK)
    {
	if (GIOPLocateReplySend (giop) == GIOP_OK)
	    return OK;
    }
    return ERROR;
}

int	
IT_IIOP_getIIOPDetails (GIOPStateS *giop)
{
    CDRCoderT coder;
    CDRBufferT buf;
    char* target_server;
    char* marker_name;
    char* method_name;
    char* activationPolicy = "shared";
    char* iiopPort = "dummyval";
    SEM_ID      s = 0;
    MSG_Q_ID    q = 0;
    unsigned short port;
    
    // printf("\nIN the getIIOPdetails funcion\n");
    
    
    
    CDRMode (giop->gs_coder_p, cdr_decoding);
    
    CDRCodeString (giop->gs_coder_p, &target_server);
    CDRCodeString (giop->gs_coder_p, &marker_name);
    CDRCodeString (giop->gs_coder_p, &method_name);
    // printf("\ntargetname is %s\n",target_server);
    // printf("\nmarker_name is %s\n",marker_name);
    // printf("\nmethod_name is %s\n",method_name);
    
    if (IT_qAddressMap::instance()->lookupServer((const char*)target_server, 
						 s, q, port) == OK)
    {
	char buf[8];
	sprintf(buf,"%lu",port);
	strcpy(&iiopPort[0],buf);
	CDRMode (giop->gs_coder_p, cdr_decoding);
	if (GIOPReplyCreate (giop, 1, 0, GIOP_NO_EXCEPTION,
			     giop->gs_request_id , 0) == GIOP_OK)
	{
	    CDRCodeString (giop->gs_coder_p, &iiopPort);
	    CDRCodeString (giop->gs_coder_p, &activationPolicy);
	    if (GIOPReplySend (giop, 0) == GIOP_OK)
	    {
		return OK;
	    }
	    else
	    {
		IT_IIOP_exception (giop, 10140);
		return ERROR;
	    }
	}
	else
	{
	    IT_IIOP_exception (giop, 10140);
	    return ERROR;
	}
    }
    else
    {
	//  Otherwise there is no entry in the qAddressmap for a
	//  server by this name so again return server not active 
	IT_IIOP_exception (giop, 10582);
	    return ERROR;
    }
}




/***************************************************************************
	
	Function:	makeRequestHeader(GIOPStateS *giop)
	
	We shall enter this functin in response to the handler function getting a
	GIOPRequest.  If this is the case then we must extract two pieces of
	data from the LocateRequest header
	
	1)	The Request id	
	2)	Does the caller expect a reply	
	3)	The Object key length and value	
	4)	The operation name	
	5)	And the principal	

******************************************************************************/

int 
IT_IIOP_make_header (GIOPStateT *giop, 
		     IT_IIOPMsgHdrT* msghdr,
		     GIOPMsgType msg_type)
{
    CDRCoderT coder;
    CDRBufferT buf;
    
    // Set the coder into decoding mode
    CDRMode (giop->gs_coder_p, cdr_decoding);

    msghdr->msg_type = msg_type;
    switch (msg_type)
    {
	case GIOPRequest:
	{
	    GIOPRequestHeader_1_0T* hdr = &msghdr->msg.request;
	    unsigned long scl_length;

	    CDRCodeULong (giop->gs_coder_p, &scl_length);
	    if (scl_length != 0L) // don't support service context lists
	    {
                // ASK Jan 98 - just ignore service context lists
                // (we may need to read it from the coder) TODO
                
                
		printf ("Service context lists not supported\n");
		return ERROR;
	    }

	    /* Then the Request id */
	    
	    CDRCodeULong(giop->gs_coder_p, &hdr->request_id);
	    
	    /* Then the response_expected flag */
	    
	    CDRCodeBool(giop->gs_coder_p,&hdr->response_expected);
	    
	    // We don't support 1.1, so no reserved octets


	    //	Now take out the Object key length and value unencapsulate 
	    //	it if we need to.
	    
	    CDRCodeULong(giop->gs_coder_p, &hdr->objkey_len);
	    CDRCodeNOctet(giop->gs_coder_p, &hdr->objkey_p,
                          &hdr->objkey_len);  
            
            // ASK
            msghdr->object_key = (unsigned char*)ITRT_MALLOC( hdr->objkey_len +1 );
            memcpy( msghdr->object_key, hdr->objkey_p, hdr->objkey_len );
            
	    msghdr->object_key[hdr->objkey_len-1] = '\0';
            //printf("\nRequest: obj key = [%s]\n",msghdr->object_key);
	    
    
	    /* Lets now get the operation name */
	    
	    CDRCodeString(giop->gs_coder_p, &hdr->operation_p);

#if 0
            // ASK Debug output -- for tracing coder usage
            logMsg( "\n[make_header]Request: "
                    "operation name [%s]; coder [%p] giop: [%p]\n",
                    hdr->operation_p, giop->gs_coder_p, giop
                  );
#endif 

	    /* and the value for the principal */
	    
	    CDRCodeULong(giop->gs_coder_p, &hdr->principal_len);
	    CDRCodeNOctet(giop->gs_coder_p,
			  (octet**)&hdr->principal_p, 
			  &hdr->principal_len);
	    break;
	}
	    
	case GIOPLocateRequest:
	{
	    GIOPLocateRequestHeaderT* hdr = &msghdr->msg.locate_request;

	    /*	Set the coder into decoding mode */
	    
	    CDRMode(giop->gs_coder_p, cdr_decoding);
	    
	    /*	Extract the Request id nto newHeader._request_id	*/
	    
	    CDRCodeULong(giop->gs_coder_p, &hdr->request_id);
	    
	    //	Now take out the Object key length and value
            // unencapsulate it 
	    //  if we need to	
	    
	    CDRCodeULong(giop->gs_coder_p, &hdr->objkey_len);
	    CDRCodeNOctet(giop->gs_coder_p, (octet **)&hdr->objkey_p, 
			  &hdr->objkey_len);

            // copy the object key into the msghdr structure
            msghdr->object_key = (unsigned char*)ITRT_MALLOC( hdr->objkey_len +1 );
            memcpy( msghdr->object_key, hdr->objkey_p, hdr->objkey_len );
        
	    msghdr->object_key [hdr->objkey_len-1] = '\0';
//            printf("\nLocateRequest: obj key = [%s]",msghdr->object_key);
	    break;
	}

	case GIOPCancelRequest:
	{
	    GIOPCancelRequestHeaderT* hdr = &msghdr->msg.cancel_request;

	    /*	Set the coder into decoding mode */
	    
	    CDRMode(giop->gs_coder_p, cdr_decoding);
	    
	    /*	Extract the Request id nto newHeader._request_id	*/
	    
	    CDRCodeULong(giop->gs_coder_p, &hdr->request_id);
	    
	    break;
	}

	case GIOPCloseConnection:
	case GIOPMessageError:
	case GIOPFragment:
	    return ERROR;

	default:
	    printf ("Bad message type!\n");
	    return ERROR; 	
     }
	    
    return OK;
}

int 
IT_IIOP_reader (IT_IIOPAgent::HNDL handle)
{
    GIOPMsgType	msg_typ;
    GIOPStateT	*giop;
    GIOPStatusT	st;
    SEM_ID sem;
    IT_IIOPMsgHdrT *messageHdr = 0;

    unsigned long messageId;
    char serverName[IIOP_NAME_LIM];
    int  serverNameLen;

    SEM_ID 		s;
    MSG_Q_ID  	q;

    int index = 0;		
	
    if (handle->socket()->state() == ERROR)
	return ERROR;

    // The message header is allocated on the heap since there
    // will be an ownership change of the memory

    messageHdr = new IT_IIOPMsgHdrT;
    messageHdr->object_key = 0;

    // We need to extract the giop state from the HNDL structure that has
    // been passed into this function.  We can do this by calling the 
    // giop_state()  method on the HNDL. 
    
    giop = handle->giop_state ();
    
    // We use the value of the HNDL file discriptor as an index into the 
    // array of FD_to_Q object that will map the file discriptor(socket) 
    // to the right queue for
    // the server.  We can do this by calling the descriptor() method on 
    // the HNDL's IT_INETsckt object.
    
    index = handle->socket()->descriptor();	
    
    
    /* The giop message that we have now received in in a ready state for 
       reading. This reading should result in finding out what type of 
       message it is.  This can
       be one of 9 types . . . .
       
       GIOPRequest             = 0,     client 
       GIOPReply               = 1,     server
       GIOPCancelRequest       = 2,     client
       GIOPLocateRequest       = 3,     client 
       GIOPLocateReply         = 4,     server 
       GIOPCloseConnection     = 5,     server 
       GIOPMessageError        = 6,     both 
       GIOPFragment            = 7,     both 
       GIOPUnknown             = 99	
     */
    
    if ((st = GIOPGetNextMsg(giop, &msg_typ)) != GIOP_OK)
    {
	// Failed to get the message, connection is bad
	//printf ("\nServer failed to get next message - it was %d\n",msg_typ);
	delete messageHdr;
	return ERROR;
    }
    
    // Interested in all GIOP messages
    
    if (IT_IIOP_make_header (giop, messageHdr, msg_typ) == ERROR)
    {
        /* Commenting out printfs ASK
		if (msg_typ == GIOPCloseConnection)
		    printf ("received close connection\n");
		else
		    printf ("Failed to make a GIOP header\n");
        */
	if ( messageHdr->object_key) 
		ITRT_FREE( messageHdr->object_key);

	delete messageHdr;
	return ERROR;
    }

    switch(msg_typ)
    {
      case GIOPRequest:
	  
	  /* make a new Request header structure and extract 	
	     the servername from the object key */
	  
	  if (IT_IIOP_extract_server (messageHdr->object_key, 
				     serverName, serverNameLen) == ERROR)
	  {
	      printf ("server extraction failed!\n");
	      delete messageHdr;
	      return ERROR;
	  }

	  /* 
	     DK- If this request is for the daemon then we will process the request 
	     in case the operation is getIIOPdetails which we will process and send back
	     the correct paramaters else return an error/exception 
	     */
	  
	  if ( strcmp(serverName, "IT_daemon") == 0)
	  {
	      if ( strcmp( messageHdr->msg.request.operation_p, "getIIOPDetails") != 0)
	      {
		  IT_IIOP_exception (giop, 10581);
		  delete messageHdr;
		  return ERROR;
	      }
	      return IT_IIOP_getIIOPDetails (giop);
	  }

	/*
	We are not looking for the daemon so continue as normal.
	*/

	  messageId = messageHdr->msg.request.request_id;
	  // printf ("\nServer name in question is %s.\n",serverName);
	  break;
	  
	case GIOPLocateRequest:
	    
	    /* 	make a new Locate Request header structure and extract 	
		the servername from the object key */
	    
	    if (IT_IIOP_extract_server (messageHdr->object_key, 
					serverName, serverNameLen) == ERROR)
	    {
		printf ("server extraction failed!\n");
		delete messageHdr;
		return ERROR;
	    }
	    messageId = messageHdr->msg.locate_request.request_id;
	  
	  break;
	  
      };		//	End of switch statement
    
	/* 
	If there is no entry in the IT_IIOP_cache object at position
	index = handle->socket()->descriptor() i.e. the value of the
	_qEntry is zero Then we must ask the orbix daemon if he has an
	entry for a server named serverName in the qAdressMap and if so
	to return us the valid q address
	*/
    index = handle->socket()->descriptor ();
    
    if (IT_IIOP_cache.isZero (index))
    {
	unsigned short port;
	// 	If we get a non null value back from the
	// 	getServerMapping
	//	call then we can assume that this server name hase an active 
	//	Queue associated with it.  A structure in the
        //      IT_IIOP_cache object is 
	//	then filled in with the relevant details about this server. 
	//	This structure entry
	//	is referanced using the fd value as an index.	
	
	if (IT_qAddressMap::instance()->lookupServer ((const char*)serverName,
						      sem, q, port) == OK)
	{
	    IT_IIOP_cache.newEntry (q, sem, index, 
				    serverName, serverNameLen);	
	}
	else
	{
	    // Otherwise there is no entry in the qAddressmap for a 
	    // server by  this name so again return -1 to the caller
	    
	    switch (msg_typ)
	    {
	      case GIOPRequest:
		  IT_IIOP_exception (giop,serverName, 10582);
		  delete messageHdr;
		  return ERROR;
		  break;
		case GIOPLocateRequest:
		  return IT_IIOP_not_here (giop, messageId);
	    } // end of switch
	}
    }
    else
    {
	// There is an entry in the IT_IIOP_cache structure for the positin 
	// indicated by index.  Assign this value to q variable saving us
	// the getServerMapping call
	
	q = IT_IIOP_cache.details(index)._qEntry;
	sem = IT_IIOP_cache.details(index)._qSem;
    }
    
    /*	Create the message structure that will be put onto the servers Q. */
    
    if (msg_typ == GIOPLocateRequest)
	return IT_IIOP_object_here (giop, messageId);
    else
	return IT_IIOP_queue_msg(handle, messageHdr, q, sem);
    
}



/******************************************************************************
	
	Function:	IT_IIOP_extract_server (unsigned char *objref,
	                                                             char *serverName,
							            int &serverNameLen)

	This function isolates the servername out of the object referance
	that is received from the GIOPState.  this object referance is 
	typically in the form  
	\place.domain.iona.ie:IT_daemon:::IR:IT_daemon	

******************************************************************************/

int 
IT_IIOP_extract_server (unsigned char *objref, 
			char *serverName, 
			int &serverNameLen)
{
    char * place;

    if (*objref == ':') // sometimes the objref begins with a ':'
	objref++; 

    place = ::strstr((char *)objref,":") + 1;

    for(serverNameLen = 0; 
	*place != ':' && serverNameLen < IIOP_NAME_LIM; 
	place++, serverNameLen++)
    {
	serverName[serverNameLen] = *place;
    }

    if (serverNameLen >= IIOP_NAME_LIM)
    {
	return ERROR;
    }
    else
	serverName[serverNameLen] = '\0';

    return OK;
}

int
IT_IIOP_queue_msg (IT_IIOPAgent::HNDL handle,
		   IT_IIOPMsgHdrT *header_p,
		   MSG_Q_ID q,
		   SEM_ID sem)
		   
{
    
    QCommsMsgT	   msgQinfo;
    GIOPStateT    *giop;
    IIOPMsg::MsgT *message = new IIOPMsg::MsgT;
    
    giop = handle->giop_state ();
    //printf ("Queuing GIOP state = 0x%x\n", giop);
    
    /* header_p will point to the RequestHeader structure */
    
    message->_header_p = (void *)header_p;
    
    /* body_p will point to the parameter list */
    
    message->_body_p = giop->gs_coder_p; // TODO: investigate copy here??

#if 0
    //
    // ASK -- problem with coders, see where they diverge:
    // (these should be the same)
    logMsg( "[q_msg]: msg_coder: %p; giop coder: %p", 
            message->_body_p,
            giop->gs_coder_p 
          );
#endif

    /* In this case message can be anything */
    
    message->_msgType = (IIOPMsg::MsgTypeT)header_p->msg_type;

    /* Now Create the Message structure */
    
    msgQinfo._data_p = (void*) message;	
    msgQinfo._len = 0;	
    msgQinfo._messageType =  COMMS_IIOP_MSG;
    msgQinfo._commsChannel = (unsigned int)handle;

#if 0

    /*	Send the message to the server represented by q	*/
    {
		/*
		 * DK - print out info for debugging
		 *
		 */ 
    	logMsg("\n[read]operation \"%s\" Req %d GIOP %p Coder %p\n", 
               header_p->msg.request.operation_p,
               giop->gs_request_id,giop,giop->gs_coder_p); 
//    	logMsg("[reader]OP %s Req %d GIOP %p Coder %p\n", 
//				header_p->msg.request.operation_p,
//				giop->gs_request_id,giop,giop->gs_coder_p); 
    }
#endif

    if ( msgQSend(q, (char*)&msgQinfo, 
                  sizeof (msgQinfo), WAIT_FOREVER,
                  MSG_PRI_NORMAL) != ERROR)
    {
        //printf ("IT_IIOP:: msgQsend succeeded\n");
            //printf ("IT_IIOP:: activating server semaphore 0x%x\n", sem);
        semGive (sem);
    }
    else
    {
        //printf ("IT_IIOP:: msgQsend succeeded\n");
        return ERROR;
    }
    // DK - the recipient task musk delete the memory held by _data_p
    //
    msgQinfo._data_p = 0;    
    return OK;
}

IORT*
IT_IIOP_create_ior (unsigned short port, char *type, char *key)
{
    
    /* IOR data */
    
    IORT            *ior_p;
    
    /* IIOP data */
    
    octet           *iiop_data_p;
    unsigned long   iiop_len;
    
    unsigned long   objkey_len;             /* Object key length */
    unsigned long   num_tc;                 /* # Tagged Components */
    
    octet           *odata_p;               /* Object key encap. */
    unsigned long   olen;
    char            *host_p;
    
    /* Coder for data encapsulation */
    
    CDRCoderT       cod;
    
    ior_p = (IORT *)ITRT_MALLOC(sizeof(IORT));
    host_p = OTCB->m_config->getHostName ();
    /*
     *      Init CDR coder
     */
    CDRInit(&cod, CDR_BYTE_ORDER, 512);
    CDRAlloc(&cod, (CDRBufferT* (*)(unsigned int))IT_IIOPAgent::_newbuf);
    CDRDealloc(&cod, (void (*)(CDRBufferT*))IT_IIOPAgent::_freebuf);
    
    /*
     *      Create an IOR with room for 1 Tagged Profile
     */
#ifdef MEMCHECKRT
    IORCreateIor(ior_p, type, 1, itrt_malloc);
#else
    IORCreateIor(ior_p, type, 1, malloc);
#endif    
    // omh - add one to prevent object key from being chopped
    objkey_len = key ? (strlen(key) + 1) : 0;


#if 0
    //
    // ASK Jan 98 - Encapsulated Object keys are causing Orbix
    // Problems (core dumps); therefore, removing encap
    //

     /*
      *      Encapsulate the object key
      */
    
    //ASK CDREncapCreate(&cod, CDR_BYTE_ORDER);
    //ASK CDRCodeNOctet(&cod, (octet **)&key, &objkey_len);
    // ASK CDREncapEnd(&cod, &odata_p, &olen, ITRT_MALLOC);

#endif

    char ** obj_key_pp = key ? (char**)&key : 0;
    CDRCodeNString(&cod, obj_key_pp , &objkey_len);
    odata_p = (octet*)key;    // ASK
    olen = objkey_len;
    
    /*
     *      Encapsulate an IIOP 1.x Profile Body
     */
    CDREncapCreate(&cod, CDR_BYTE_ORDER);

    octet major = 1, minor = 0;

    IOREncapIIOP(
	&cod,
	&major, &minor, (char **)&host_p, &port, &olen, &odata_p
	);
    
    /* Object key is encapsulated, so free it */
    
    //ASK ITRT_FREE(odata_p);
#ifdef MEMCHECKRT
    CDREncapEnd(&cod, &iiop_data_p, &iiop_len, itrt_malloc);
#else
    CDREncapEnd(&cod, &iiop_data_p, &iiop_len, malloc);
#endif
    
    /*
     *      Add the IIOP encapsulation to the IOR
     */
#ifdef MEMCHECKRT
    IORAddTaggedProfile
		( ior_p, TAG_INTERNET_IOP, iiop_len, iiop_data_p, itrt_malloc);
#else
    IORAddTaggedProfile
		( ior_p, TAG_INTERNET_IOP, iiop_len, iiop_data_p, malloc);
#endif
    
    /* iiop data is encapsulated, so free it */
    
    ITRT_FREE(iiop_data_p);
    CDRFree(&cod);
    
    return ior_p;
}



int
IT_IIOP_bind( GIOPStateT *giop, CDRCoderT *coder, IORT & ior  )
{
    GIOPStatusT         st;
    unsigned long	req_id, 
                        rep_id;

    /*
     *	Initialise the GIOP  state.  This installs a coder  in
     *	the giop state and  places  giop in  a  pre-connection
     *	state
     */
    /* Add buffer [de-]allocation routines */

    CDRInit(coder, CDR_BYTE_ORDER, 512);
    CDRAlloc(coder, (CDRBufferT* (*)(unsigned int))IT_IIOPAgent::_newbuf);
    CDRDealloc(coder, (void (*)(CDRBufferT*))IT_IIOPAgent::_freebuf);
	
    /* Init giop - install coder, is_server is FALSE */

    GIOPInit(giop, coder, FALSE);

    /*
     *	Connect to the server.
     */
    //printf("\nAttempting to bind to the server\n");
	
    if ((st = GIOPConnect(giop, &ior, IOR_MP_TRYONE)) != GIOP_OK)
    {        
        printf("\nClient failed on GIOPConnect()");
        return ERROR;
    }
    
    return OK;
}

//
// IOR coding area below
//
//
// Coding the IOR:
//
//  IOR {
//          string type_id;
//          ulong  profile_len; // ie num profiles
//          code each profile, i :
//                  ulong profile_id
//                  profile_data (encapsulated IIOP 1.0 Profile Body)
//  }
//  IIOP 1.0 Profile Body {
//          (octets) major =1; minor = 0;
//          string host;
//          (short) port;
//          object_key (encapsulated octet seq) (note: do not encapsulate the
//              object key as Orbix will have a fit)
//  }
//


// allocates a new string (using operator new) and returns the orbix object
// key
// 
inline char*
IT_IIOP_ObtainObjectKey( CORBA(ObjectRef) object_p )
{
    CORBA(PPTR)                 *obj_pptr_p     = object_p?object_p->_pptr():
                                                  0; 
    ObjectReference             *obj_ref_p      = obj_pptr_p ? 
                                                  obj_pptr_p->getOR() : 0;
    char                        *obj_key_p      = obj_ref_p ? 
                                                  obj_ref_p->object_to_string()
                                                  : 0;
    return obj_key_p;    
}

inline IORT*
IT_IIOP_Extract_IOR( CORBA(ObjectRef) object_p )
{
    CORBA(PPTR)                 *obj_pptr_p     = object_p?object_p->_pptr():
                                                  0; 
    ObjectReference             *obj_ref_p      = obj_pptr_p ?
                                                  obj_pptr_p->getOR()
                                                  : 0; 
    if( obj_ref_p && obj_ref_p->isIOR() )
    {
        
        IORT                    &ior            = obj_ref_p->getIOR();
        return &ior;        
    }
    else
        return 0;
}



static int
IT_IIOP_CodeInternetProfile( CDRCoderT          *dest_cod_p, 
                             IORTaggedProfileT  &tag_profile,
                             char               *orbix_obj_key_p 
                            )
{
    octet               *profile_data_p = tag_profile.tp_profile_data_p;
    unsigned long       profile_len     = tag_profile.tp_profile_len;

    CDRCodeULong( dest_cod_p, &profile_len );
    CDRCodeNOctet( dest_cod_p, &profile_data_p, &profile_len );
    return OK;
    
    // this code is not really needed; just code the tag_profile
    // 

    CDRCoderT           local_coder;
    CDRBufferT          local_buffer;
    

    octet               major_version   = 1;
    octet               minor_version   = 0;
    char                *host_nam_p     = OTCB->m_config->getHostName();
    unsigned short      port            = OTCB->m_iiopListenerPort;
    unsigned long       obj_key_len     = orbix_obj_key_p ? 
                                          strlen( orbix_obj_key_p ) + 1 : 0; 
    
    
    if( !port || !dest_cod_p || ! obj_key_len )
        return ERROR;


    //
    // setup the local coder
    // 
    CDRInit( &local_coder, CDR_BYTE_ORDER, 128 );
    CDRAlloc(  &local_coder, (CDRBufferT* (*)(unsigned
                                              int))IT_IIOPAgent::_newbuf ); 
    CDRDealloc( &local_coder, (void (*)(CDRBufferT*))IT_IIOPAgent::_freebuf );

    //
    // encapsulate the profile
    // 
    CDREncapCreate( &local_coder, CDR_BYTE_ORDER );
    // code the IIOP 1.0 body
    //
    IOREncapIIOP( dest_cod_p, &major_version, &minor_version, &host_nam_p,
                  &port, &obj_key_len, (octet**)&orbix_obj_key_p
                 );
    
    // end the encap -- this will put the sequence of octets of the
    // encapsulated data into profile_data_p
    //
    CDREncapEnd( &local_coder, &profile_data_p, &profile_len,
                 (void*)IT_IIOPAgent::_newbuf  
                );
    
    // now encode the encapsulated profile into the main coder
    //
    CDRCodeULong( dest_cod_p, &profile_len );
    CDRCodeNOctet( dest_cod_p, &profile_data_p, &profile_len );

    return OK;
    
}

static int 
IT_IIOP_EncodeIOR( CDRCoderT* dest_cod_p, CORBA(ObjectRef) obj_ref_p )
{
    IORT                *ior_p          = 0;
    unsigned long       num_profiles    = 0;    
    octet               sex             = CDR_BYTE_ORDER;    
    octet               v_major         = 1;
    octet               v_minor         = 0;

    if( ! (ior_p = IT_IIOP_Extract_IOR( obj_ref_p )) )
        return ERROR;

    num_profiles = ior_p->ior_profiles_len;
    CDRCodeString( dest_cod_p, &ior_p->ior_type_id_p );
    CDRCodeULong( dest_cod_p, &num_profiles );

    //
    // now code each profile (note: only tagged_internet_iiop profiles are    
    // supported)
    int i = 0;
    for(i=0; i < num_profiles; i++ )
    {   
        //
        // IIOP 1.0 only support TAG_INTERNET_IIOP.
        // We only support IIOP 1.0 (so far)
        // 
	IORTaggedProfileT &cur_profile_p = ior_p->ior_profiles_p[i];

        if( cur_profile_p.tp_tag == TAG_INTERNET_IOP )
        {
            char * orbix_obj_key_p = IT_IIOP_ObtainObjectKey( obj_ref_p );
            
            CDRCodeULong( dest_cod_p, &cur_profile_p.tp_tag );

            IT_IIOP_CodeInternetProfile( dest_cod_p, cur_profile_p,
                                         orbix_obj_key_p  
                                        );
            if( orbix_obj_key_p )
                delete [] orbix_obj_key_p;
        }
    }
}

static int 
IT_IIOP_DecodeIOR( CDRCoderT* dest_cod_p, void *& obj_ref_p )
{

    IORT        new_ior;
    CDRCoderT   ior_coder;
    CDRBufferT  cdr_buf;
    

    //
    // decode type_id
    CDRCodeString( dest_cod_p, &new_ior.ior_type_id_p );
    
    //
    // decode number of profiles
    CDRCodeULong( dest_cod_p, &new_ior.ior_profiles_len );
    
    if( new_ior.ior_profiles_len )
    {
        new_ior.ior_profiles_p =  (IORTaggedProfileT*)ITRT_MALLOC(
                                                               sizeof(IORTaggedProfileT) 
                                                               *
                                                               new_ior.ior_profiles_len
                                                              );

        new_ior.ior_profiles_max = new_ior.ior_profiles_len; 

        //
        // decode each tagged profile
        for( unsigned long i = 0; i < new_ior.ior_profiles_len; i++ )
        {

            IORTaggedProfileT *cur_prof_p = &new_ior.ior_profiles_p[i];

            //
            // decode the tag type
            CDRCodeULong( dest_cod_p, &cur_prof_p->tp_tag );
            
            if( cur_prof_p->tp_tag != TAG_INTERNET_IOP )
            {
                // cannot handle any other profiles
                // panic, and quit
                // 
                ITRT_FREE( new_ior.ior_profiles_p );
                return ERROR;
            }
            
            // decode the number of elements in the profile
            // 
            CDRCodeULong( dest_cod_p, &cur_prof_p->tp_profile_len );
            
            //
            // now read the data; note this is an encap.  If we do
            // need to interpret it, use the coders above and pass, as
            // data, the octets read off the wire: -
            //            CDREncapInit( &ior_coder, &cdr_buf,
            //              cur->len, cur->tp_data_p,
            //              cdr_decoding
            //            );
            // 
            CDRCodeNOctet( dest_cod_p, 
                           &cur_prof_p->tp_profile_data_p,
                           &cur_prof_p->tp_profile_len  );
        }
        octet * ior_str_p = 0;
#ifdef MEMCHECKRT
        IORToString( &new_ior, &ior_str_p, itrt_malloc );
#else
        IORToString( &new_ior, &ior_str_p, malloc );
#endif
        CORBA(Object) * obj_p = CORBA(Orbix).string_to_object(
                                                              (char*)ior_str_p );
        if( obj_p )
        {
            CORBA(PPTR)*        pptr_p = obj_p->_pptr();
            void *              proxy_p = pptr_p ? pptr_p->getObj() :
                                          0;

            &obj_ref_p = proxy_p ? &proxy_p : 0;
            
            /*
            &obj_ref_p = obj_p->_pptr() ? obj_p->_pptr()->getObj() :
                         0;
            */
        }
        
        if( !obj_ref_p )
            return ERROR;
 
    }
    
    return ERROR;
}

// The void * reference that is delivered from this function from the
// generated code is a pointer to the *proxy* object for decoding
// and for encoding is a CORBA_Object. Sez Aman.
//
int
CORBA(IT_IIOP_CodeIOR)(CDRCoderT* dest_cod_p, void* & v_obj_ref_p, 
		       CORBA(Environment) &env)
{
    // in the generated code, call this method by obtaining the target
    // from the request:
    // 	CORBA_(ObjectRef) target () 
    
    //
    // initalisation of locals
    // 

    if( !dest_cod_p )
        return ERROR;


    if( dest_cod_p->cdr_mode == cdr_encoding )
    {
        if( !v_obj_ref_p )
            return ERROR;
        
 	CORBA(ObjectRef) obj_ref_p = (CORBA(ObjectRef)) v_obj_ref_p;
        IT_IIOP_EncodeIOR( dest_cod_p, obj_ref_p );        
    }
    else
    {
        IT_IIOP_DecodeIOR( dest_cod_p, v_obj_ref_p );        
    }
    return OK;    
}

