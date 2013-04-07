// initialise iiop for vxworks

#include <iiopagent.h>
#include <reader.h>
#include <protoReg.h>
#include <iiopmsgp.h>

extern "C"
int
IT_iiopInit (unsigned short port)
{
    // make protocol registry
    
    IT_ProtocolRegistry *pr = IT_ProtocolRegistry::instance ();

    if (pr == 0)
	return ERROR;

    IT_IIOPAgent *ia = IT_IIOPAgent::instance ();

    if (ia == 0)
    {
	pr->release ();
	return ERROR;
    }

    IT_IIOPMsgProcessor *msgp = IT_IIOPMsgProcessor::instance ();

    if (msgp == 0)
    {
	ia->release ();
	pr->release ();
	return ERROR;
    }

    // register the IIOP reader with the Agent

    ia->interpreter (IT_IIOP_reader);

    // register the IIOP protocol processor with the protocol registry

    if (pr->register_p (COMMS_IIOP_MSG, msgp) == (IT_IIOPMsgProcessor*)ERROR)
	return ERROR;

    // register the port as a listener if required
    if (port)
    {
	return ia->register_port ("ORB", port);
    }

    return OK;
}

extern "C"
int
IT_iiopExit (int hndl)
{
    IT_IIOPAgent::HNDL handle;

    if (hndl != 0)
    {
	handle = (IT_IIOPAgent::HNDL) hndl;
	return IT_IIOPAgent::instance()->shutdown (handle);
    }
    return ERROR;
}
