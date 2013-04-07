// IIOP message processor class
//
// 24nov97    omh    file creation

#include <FRRIf.h>
#include <RequestS.h>
#include <iiopmsgp.h>
#include <giop.h>
#include <iiopMsg.h>
#include <protoReg.h>
#include <reader.h>
#include <stdio.h>
#include <OTCB.h>

IT_IIOPMsgProcessor* IT_IIOPMsgProcessor::_instance = 0;
IT_mutex IT_IIOPMsgProcessor::_lock;

IT_IIOPMsgProcessor*
IT_IIOPMsgProcessor:: instance ()
{
    if (_instance == 0)
    {
	IT_scoped_lock guard (_lock);
	if (_instance == 0)
	{
	    _instance = new IT_IIOPMsgProcessor (COMMS_IIOP_MSG);
	}
    }

    return _instance;
}

void
IT_IIOPMsgProcessor:: release ()
{
    if (_instance != 0)
    {
	IT_scoped_lock guard (_lock);
	if (_instance != 0)
	{
	    delete _instance;
	    _instance = 0;
	}
    }
}

IT_IIOPMsgProcessor::IT_IIOPMsgProcessor (unsigned int id)
    : IT_MsgProcessor (id)
{
}

IT_IIOPMsgProcessor::~IT_IIOPMsgProcessor ()
{
    IT_ProtocolRegistry::instance()->unregister_p (m_id);
}

int
IT_IIOPMsgProcessor::process (QCommsMsgT msg, unsigned int &errval)
{
    RequestS *serverRequest = 0;

    if (msg._messageType != COMMS_IIOP_MSG)
    {
	errval = IIOP_BAD_MSG;
	return -1;
    }
    
    // Message is valid for this processor so.....
    // Need to unify all these message types
    IIOPMsg::MsgT *message = (IIOPMsg::MsgT *)msg._data_p;
    IT_IIOPMsgHdrT *hdr    = (IT_IIOPMsgHdrT*)message->_header_p;

#if 0
    // 
    // debugging aides: (can be removed)
    // 
    CDRCoderT* temp   	 = (CDRCoderT*)message->_body_p;
    GIOPStateT* gs   	 = ((IT_IIOPAgent::HNDL)msg._commsChannel)->giop_state();
    logMsg( "\n[process] GIOP = %p Msg Coder = %p; GIOP Coder: %p", 
            gs, temp, gs->gs_coder_p,
            0,0, 0 
          );
#endif 

    if (message == 0)
    {
	errval = IIOP_NUL_MSG;
	return -1;
    }
    
    switch (message->_msgType)
    {
        case GIOPRequest:
        {
            GIOPRequestHeader_1_0T *rhdr = &hdr->msg.request;
            // printf ("\n\nRequest message received.\n");
            // Request message: 
            // Split out the fields from the request header which
            // are required to construct a RequestS object. Call
            // FRRIf::dispatch_iiop_request with the RequestS as
            // argument.
            
            //
            // ASK -- for a laugh, change the _body_p to be the giop
            // state's coder.  This may be causing problems...
            // TODO --nvestigate why this worked!!! 
            // gs->gs_coder_p = message->_body_p ; //= gs->gs_coder_p;

            serverRequest = new RequestS ((const char*)hdr->object_key,
                                          (const char*)rhdr->operation_p,
                                          rhdr->response_expected,
                                          (void*)&msg
                                          );
	  
            OTCB->m_FRR->dispatch (serverRequest);
        }   
        break;
	  
        case GIOPReply:
            // Reply message:
            // Put the message contents into the reply cache for
            // retrieval later.
            // printf ("reply message\n");
            break;
	    
        case GIOPCancelRequest:
            // Ignore this
            // printf ("cancel request message\n");
            break;
	  
        case GIOPLocateRequest:
            // Ignore this, first version locate requests will not
            // get as far as the server
            // printf ("locate request message\n");
            break;
	
        case GIOPLocateReply:
            // Not sure what to do here :)
            // printf ("locate reply message\n");
            break;
            
        case GIOPCloseConnection:
            // These should be trapped in the IIOP Agent, but shutdown
            // the handle anyway.
            // printf ("close connection message\n");
            break;
            
        case GIOPMessageError:
            // printf ("message error message\n");
            break;
            
        case GIOPFragment:
            // printf ("fragment message\n");
            break;
            
        default:
            // printf ("some weird message\n");
            break;
    } 

    errval = 0;
    return 0;
}

    



