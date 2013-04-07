// Protocol registry
//
// 24nov97    omh    file creation

#include <protoReg.h>
#include <locks.h>
#include <stdio.h>

IT_ProtocolRegistry* IT_ProtocolRegistry::_instance = 0;

IT_mutex IT_ProtocolRegistry::_lock;

IT_ProtocolRegistry::IT_ProtocolRegistry ()
{
    int loop = COMMS_PROTOCOL_MAX - 1;
    
    while (loop >= 0)
    {
	m_protocol_table[loop].name = 0;
	m_protocol_table[loop].handler = 0;
	loop--;
    }
}

IT_ProtocolRegistry::~IT_ProtocolRegistry ()
{
}

IT_ProtocolRegistry*
IT_ProtocolRegistry:: instance ()
{
    if (_instance == 0)
    {
        IT_scoped_lock guard (_lock);
	if (_instance == 0)
	{
	    // printf ("making a registry\n");
	    _instance = new IT_ProtocolRegistry ();
	}
    }
    
    return _instance;
}

void
IT_ProtocolRegistry:: release ()
{
    if (_instance != 0)
    {
        IT_scoped_lock guard (_lock);
	if (_instance != 0)
	{
	    // printf ("removing a registry\n");
	    delete _instance;
	    _instance = 0;
	}
    }
}    
	
IT_MsgProcessor*
IT_ProtocolRegistry:: register_p (unsigned int protocol_id, 
				  IT_MsgProcessor* handler)
{
    IT_MsgProcessor* tmp;
      
    if (protocol_id >= COMMS_PROTOCOL_MAX)
    {
	return (IT_MsgProcessor*) -1;
    }
    else
    {
	IT_scoped_lock guard (_lock);
	
	tmp = m_protocol_table[protocol_id].handler;
	m_protocol_table[protocol_id].handler = handler;
	handler->id() = protocol_id;
	return tmp;
    }
}

IT_MsgProcessor*
IT_ProtocolRegistry:: unregister_p (unsigned int protocol_id)
{
    IT_MsgProcessor* tmp;
      
    if (protocol_id >= COMMS_PROTOCOL_MAX)
    {
	return (IT_MsgProcessor*) -1;
    }
    else
    {
	IT_scoped_lock guard (_lock);
	
	tmp = m_protocol_table[protocol_id].handler;
	m_protocol_table[protocol_id].handler = 0;
	return tmp;
    }
}    
    
IT_MsgProcessor*
IT_ProtocolRegistry:: processor (unsigned int protocol_id)
{
    if (protocol_id >= COMMS_PROTOCOL_MAX)
    {
	return (IT_MsgProcessor*) -1;
    }
    else
    {
	IT_scoped_lock guard (_lock);
	return m_protocol_table[protocol_id].handler;
    }
}    
