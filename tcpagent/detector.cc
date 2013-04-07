// Implementation of the IIOPDetector generic class for use with 
// socket listeners and connections.
//
// History list:
//
// 14nov97   omh    file creation
// 17nov97   omh    addition of functionality to deal with IOR's

#include <detector.h>
#include <selectLib.h>
#include <hostLib.h>
#include <cdr.h>

IIOPDetector:: IIOPDetector (char *name) : m_selecter (name)
{
    m_state = m_selecter.state ();
    if (m_state != ERROR)
    {
	lstInit (&m_active);
    }
}

IIOPDetector:: ~IIOPDetector ()
{
    ListNode *sckt;


    {   // Artificial scope for guard
	IT_scoped_lock guard (m_lock);
	
	while ((sckt = (ListNode*)lstGet (&m_active)) != 0)
	{
	    if (sckt->m_socket)
	    {
		m_selecter.remove_descriptor (sckt->m_socket->descriptor());
		delete sckt->m_socket;
	    }
	}
    
    lstFree (&m_active);

    }
}

IIOPDetector::HNDL
IIOPDetector:: register_detector (unsigned short port)
{
    char hostn[128];
    int ipaddr = 0;
    
    // Make a socket on the supplied port, ensuring reusable addresses
    // Call listen on it
    // Add this information into the m_active list
    
    // 1. check that there isn't already a detector on that port

    ListNode* lptr = (ListNode*) lstFirst (&m_active);
    
    while (lptr != 0)
    {
	if (lptr->m_socket->port() == port)
	{
	    IT_scoped_lock x (m_lock);
	    
	    lptr->m_refcount++;
	    return lptr;
	}

	lptr = (ListNode*) lstNext ((NODE*) lptr);
    }

    // 2. make the socket

    if (::gethostname (hostn, sizeof (hostn)) != ERROR)
    {
	if ((ipaddr = hostGetByName (hostn)) == ERROR)
	{
	    ipaddr = INADDR_ANY;
	}
    }
    else
    {
	ipaddr = INADDR_ANY; 
    }
    
    IT_INETsckt *skt = new IT_INETsckt (port, ipaddr);
    
    if (skt->state () == ERROR)
    {
	delete skt;
	return 0;
    }

    // 3. start the socket listening

    if (skt->listen () == ERROR)
    {
	delete skt;
	return 0;
    }

    // 4. put the socket in the list
    
    IIOPDetector::ListNode *new_node = new IIOPDetector::ListNode (this);
    new_node->m_socket = skt;
    
    new_node->m_refcount = 1;

    if (1)
    {   // Artificial scope for lock
	IT_scoped_lock x (m_lock);    
	lstAdd (&m_active, (NODE*)new_node);
    }
    
    // 5. put the socket fd in the select loop

    m_selecter.add_descriptor (skt->descriptor());
    return new_node;
}

IIOPDetector::HNDL
IIOPDetector:: register_detector (int fd)
{
    char hostn[128];
    int ipaddr = 0;
 
    // Make a socket object using the supplied fd
    // Add this information into the m_active_readers list
 
    // 1. make the socket
 
    IT_INETsckt *skt = new IT_INETsckt (fd);
 
    if (skt->state () == ERROR)
    {
        delete skt;
        return 0;
    }
 
    // 2. put the socket in the list
 
    IIOPDetector::ListNode *new_node = new IIOPDetector::ListNode (this);
    new_node->m_socket = skt;
 
    new_node->m_refcount = 1;
 
    if (1)
    {   // Artificial scope for lock
        IT_scoped_lock guard (m_lock);
        lstAdd (&m_active, (NODE*)new_node);
    }
 
    // 3. put the socket fd in the select loop
 
    m_selecter.add_descriptor (skt->descriptor());
    return new_node;
}

IIOPDetector::HNDL
IIOPDetector:: register_detector (IORT* ior_p)
{
    char hostn[128];
    unsigned short port;
    int ipaddr = 0, count = 0;
    unsigned long objkey_len;
    IORTaggedProfileT *profile;
    CDRCoderT coder;
    CDRBufferT coder_buf;
    IIOPBody_1_0T           iiop_pb;
    octet _d1, _d2, *_d6;
    char * _d3;
    

    // Extract the port number from the IOR TAG_INTERNET_IOP
    // profile.
    
    if ((count = ior_p->ior_profiles_len) == 0)
	return 0;
    
    profile = ior_p->ior_profiles_p;
    while (count > 0 && (profile[count].tp_tag != TAG_INTERNET_IOP))
	count--;

    if (count == 0) // did not find the profile
	return 0;

    // Take out the port and check we are not already using it
    
   
    CDREncapInit (&coder, &coder_buf, profile[count].tp_profile_data_p,
		  profile[count].tp_profile_len, cdr_decoding);
    IOREncapIIOP (&coder, &_d1, &_d2, &_d3, &port, &objkey_len, &_d6); 
    
    // Make a socket on the port, ensuring reusable addresses
    // Call listen on it.
    // Add this information into the m_active list
    
    // 1. check that there isn't already a detector on that port

    ListNode* lptr = (ListNode*) lstFirst (&m_active);
    
    while (lptr != 0)
    {
	if (lptr->m_socket->port() == port)
	{
	    IT_scoped_lock x (m_lock);
	    
	    lptr->m_refcount++;
	    return lptr;
	}

	lptr = (ListNode*) lstNext ((NODE*) lptr);
    }

    // 2. make the socket

    if (::gethostname (hostn, sizeof (hostn)) != ERROR)
    {
	if ((ipaddr = hostGetByName (hostn)) == ERROR)
	{
	    ipaddr = INADDR_ANY;
	}
    }
    else
    {
	ipaddr = INADDR_ANY; 
    }
    
    IT_INETsckt *skt = new IT_INETsckt (port, ipaddr);
    
    if (skt->state () == ERROR)
    {
	delete skt;
	return 0;
    }

    // 3. start the socket listening

    if (skt->listen () == ERROR)
    {
	delete skt;
	return 0;
    }

    // 4. put the socket in the list
    
    IIOPDetector::ListNode *new_node = new IIOPDetector::ListNode (this);
    new_node->m_socket = skt;
    
    new_node->m_refcount = 1;

    if (1)
    {   // Artificial scope for lock
	IT_scoped_lock x (m_lock);    
	lstAdd (&m_active, (NODE*)new_node);
    }
    
    // 5. put the socket fd in the select loop

    m_selecter.add_descriptor (skt->descriptor());
    return new_node;
}

int
IIOPDetector:: shutdown_detector (HNDL handle)
{
    if (handle)
    {
	// TODO: this needs to be mutexed! Need to increase critsec size?

	if (handle->m_refcount > 1)
	    handle->m_refcount--;
	else
	{
	    m_selecter.remove_descriptor (handle->socket()->descriptor());
	    if (1)
	    {  // Artificial scope to reduce critical section size
		IT_scoped_lock guard (m_lock);
		lstDelete (&m_active, (NODE*)handle);
	    }
	    // explicitly shutdown the socket
	    handle->socket()->shutdown();
	    delete handle->socket ();
	    delete handle;
	}
	return OK;
    }
    
    return ERROR;
}

int
IIOPDetector:: register_callback (IT_Selecter::callbackFnT fn)
{
    if (m_selecter.state () != ERROR)
	return m_selecter.add_callback (IT_Selecter::ActiveFD, fn);
    else
	return ERROR;
}

int
IIOPDetector:: shutdown_callback ()
{
    if (m_selecter.state () != ERROR)
	return m_selecter.remove_callback (IT_Selecter::ActiveFD);
    else
	return ERROR;
}

int 
IIOPDetector:: state ()
{
    return m_state;
}
