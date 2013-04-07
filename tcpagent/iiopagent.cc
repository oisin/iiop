// The IIOP Agent is implemented by this class which is instantiated
// as a singleton on the board, using a pointer-based singleton pattern and 
// double-checked locking. Don not change this to a reference based
// singleton pattern as this is not thread safe.
//
// The IIOP Agent API is used by any server thread which wishes to listen
// for IIOP connections and process IIOP messages. The server thread 
// registers with the API which delegates the registration to the listener
// segment of the Agent. This object assumes that there exists a lookup
// procedure to access information about a server based on the server name.
// In the case of the Orbix 1.3.5 for VxWorks product, this is implemented
// by the qAddressMap global object.
//
// Caveats:
// The IT_IIOPAgent object can be allocated and deleted, but only one
// instance may exist at the one time.
//
// Change Log:
//
// 11nov97    omh    file creation
// 12nov97    omh    added IIOPReader and callback functions
// 13nov97    omh    changed read_callback to make socket non-blocking
//                   to allow parallel connection processing
// 14nov97    omh    change to use IIOPDetector class
//                   addition of local HNDL cache to match up FD's
//                   addition of cache consistency callbacks and Detector
//                   storage consistency callbacks.
// 18nov97    omh    added GIOPStateT faking in read_callback

#include <iiopagent.h>
#include <locks.h>
#include <stdio.h>
#include <ioLib.h>
#include <sockLib.h>
#include <skts.h>
#include <cdr.h>
#include <giop.h>

#include <itrtlib.h>

IT_IIOPAgent*      IT_IIOPAgent::_instance = 0;
IT_mutex           IT_IIOPAgent::_lock;
IORTaggedProfileT  IT_IIOPAgent::_dummyProfile = { TAG_INTERNET_IOP, 0,0,0 };
IORT               IT_IIOPAgent::_dummyIOR = { 0, 0, 0, 
					       &IT_IIOPAgent::_dummyProfile };


extern "C" void bzero (void*, int);

CDRBufferT *
IT_IIOPAgent:: _newbuf (size_t min_bytes)
{
    size_t          len;
    CDRBufferT      *buf_p;
    
    /* Allocate the buffer struct */
    
    buf_p = (CDRBufferT *) ::ITRT_MALLOC(sizeof(CDRBufferT));
    
    if (buf_p != 0)
    {
	len = min_bytes;
	
	if (min_bytes < CDR_MIN_BUFSZ)
	    len = CDR_MIN_BUFSZ;
    
	/* Allocate the raw buffer */
	
	buf_p->cdrb_buffer_p =
	    buf_p->cdrb_pos_p = (unsigned char *)::ITRT_MEMALIGN (8, len);

	if (buf_p->cdrb_buffer_p == 0)
	{
	    ITRT_FREE(buf_p);
	    buf_p = 0;
	}
	else
	    buf_p->cdrb_len = len;
    }
    return buf_p;
}    

void
IT_IIOPAgent:: _freebuf (CDRBufferT *buf_p)
{
    if (!buf_p)
	return;
    
    if (buf_p->cdrb_buffer_p)
	ITRT_FREE(buf_p->cdrb_buffer_p);
    
    buf_p->cdrb_next_p = 0;
    
    ITRT_FREE(buf_p);
    buf_p = 0;
}

int
IT_IIOPAgent:: _sink (IT_IIOPAgent::HNDL hndl)
{
    int result;
    char memblk[1024];

    // just read and send to stdio!
    // have to make the fd non-blocking temporarily
    // so that this won't block and starve other connections

    hndl->socket()->blocking (1);
    
    while ((result = hndl->socket()->recv (1024, memblk)) > 0)
    {
	// printf ("reader has %d bytes\n", result);
	write (1, memblk, result);
    }
    
    hndl->socket()->blocking (0);
    
    if (result == 0)
    {
	// If fd is a blocking(?) file descriptor then the zero
	// means that the connection has been shutdown in 
	// an orderly manner.
	// TODO: this is temporary, need to do shutdown_Detector.

	IT_IIOPAgent::instance()->shutdown (hndl);
	// printf ("reader has 0 bytes\n");
	return ERROR;
    }
    else
    {
	if (result == ERROR && errno != EWOULDBLOCK)
	{
	    // TODO: this is temporary, need to do shutdown_Detector.
	    IT_IIOPAgent::instance()->shutdown (hndl);
	    // printf ("reader has -1 bytes!!\n");
	    // remove from the cache
	    return ERROR;
	}
	return OK;
    }
}

IT_IIOPAgent*
IT_IIOPAgent:: instance ()
{
    // Use double-checked locking to dodge race conditions
    if (_instance == 0)
    {
	IT_scoped_lock guard (_lock);
	if (_instance == 0)
	{
	    _instance = new IT_IIOPAgent() ;
	}
    }

    return _instance;
}

void
IT_IIOPAgent:: release ()
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

IT_IIOPAgent:: IT_IIOPAgent ()
{
    bzero (m_hndl_cache, sizeof (m_hndl_cache));
    
    interpreter (IT_IIOPAgent::_sink);

    m_listen_agent = new IIOPDetector (IT_IIOPLAGENT_NAME);
    if (m_listen_agent)
	m_listen_agent->register_callback ((IT_Selecter::callbackFnT)listen_callback);

    // printf ("IIOPAgent ctor: this = 0x%x, listener = 0x%x\n",
    //	    this, m_listen_agent);

    m_reader_agents[0] = new IIOPDetector (IT_IIOPRAGENT0_NAME);
    if (m_reader_agents[0])
	m_reader_agents[0]->register_callback ((IT_Selecter::callbackFnT)read_callback);
    //printf ("IIOPAgent ctor: this = 0x%x, reader = 0x%x\n",
    //	    this, m_reader_agents[0]);

    m_reader_agents[1] = new IIOPDetector (IT_IIOPRAGENT1_NAME);
    if (m_reader_agents[1])
	m_reader_agents[1]->register_callback ((IT_Selecter::callbackFnT)read_callback);
    // printf ("IIOPAgent ctor: this = 0x%x, reader = 0x%x\n",
    //	    this, m_reader_agents[1]);
}


IT_IIOPAgent:: ~IT_IIOPAgent ()
{
  if (m_listen_agent)
    delete m_listen_agent;

  if (m_reader_agents[0])
    delete m_reader_agents[0];

  if (m_reader_agents[1])
    delete m_reader_agents[1];
}

// Are these over-locked??

IT_IIOPAgent::HNDL
IT_IIOPAgent:: register_port (char *servername, unsigned short port)
{
    IT_IIOPAgent::HNDL result;
    IT_scoped_lock guard (_lock);

    result = m_listen_agent->register_detector (port);
    if (result)
	m_hndl_cache[result->socket()->descriptor()] = result;
	
    return result;
}

// Are these over-locked??

IT_IIOPAgent::HNDL
IT_IIOPAgent:: register_socket (int fd)
{
    IT_IIOPAgent::HNDL result;
    IT_scoped_lock guard (_lock);

    // printf ("Allocating reader thread %d\n", fd % 2);
    result = m_reader_agents[fd % 2]->register_detector (fd);
    if (result)
	m_hndl_cache[result->socket()->descriptor()] = result;
	
    return result;
}


IT_IIOPAgent::HNDL
IT_IIOPAgent:: register_IOR (IORT* ior_p)
{
    IT_IIOPAgent::HNDL result;
    IT_scoped_lock guard (_lock);

    
    // printf ("Allocating IOR listener\n");
    result = m_listen_agent->register_detector (ior_p);
    if (result)
	m_hndl_cache[result->socket()->descriptor()] = result;
	
    return result;
}

int
IT_IIOPAgent:: shutdown (IT_IIOPAgent::HNDL handle)
{
    IT_scoped_lock guard (_lock);

    if (handle->refs () == 1)
	m_hndl_cache[handle->socket()->descriptor()] = 0;
    return handle->shutdown ();
}

int 
IT_IIOPAgent:: listen_callback (int fd)
{

    if (fd != ERROR)
    {
	int afd;
	IT_IIOPAgent::HNDL acceptor;

	if ((acceptor = IT_IIOPAgent::instance()->m_hndl_cache[fd]) != 0)
	{
	    /* got the acceptor */
	    
	    // printf ("Successfully got the acceptor in the cache\n");
	    afd = acceptor->socket()->accept ();
	}
	else // make a temporary socket
	{
	    IT_INETsckt acceptor_socket (fd);
	    
	    afd = acceptor_socket.accept ();
	}

	if (afd != ERROR)
	{
	    IT_IIOPAgent::instance()->register_socket (afd);
	    return OK;
	}
    }
    return ERROR;
}

int
IT_IIOPAgent:: read_callback (int fd)
{

    //
    // dynamically allocating coder now.
    // ASK Feb 98 vxiiop
    // 
    int result, nbstate, size;
    GIOPStateT  giop;
    CDRCoderT   *new_coder_p = new CDRCoderT;
    IORTaggedProfileT   prof;
    
    if (fd != ERROR)
    {
	// printf ("reader activity on %d\n", fd);
	
	IT_IIOPAgent::HNDL h;
	
	if ((h = IT_IIOPAgent::instance()->m_hndl_cache[fd]) != 0)
	{
	    // printf ("Read callback got cached hndl %d -> 0x%x\n",
		   // fd, h);
	}
 	CDRInit( new_coder_p, CDR_BYTE_ORDER, 256);
	/* Add buffer [de-]allocation routines */
 
        CDRAlloc( new_coder_p, (CDRBufferT* (*)(unsigned int))IT_IIOPAgent::_newbuf);
        CDRDealloc( new_coder_p, (void (*)(CDRBufferT*))IT_IIOPAgent::_freebuf);
 
        /* Init giop - install coder, is_server is FALSE */
 
        GIOPInit(&giop, new_coder_p, 1);	
	
	// printf ("Faking a GIOP state, ha ha ha\n");
	giop.gs_afrag_flags = 0;
	giop.gs_is_server = 1;
	giop.gs_status = GIOP_OK;

	giop.gs_state = GIOP_SIDLE;

	giop.gs_ctrl_blk.cb_ior_p = &IT_IIOPAgent::_dummyIOR;
	giop.gs_ctrl_blk.cb_ior_profile = 0;
	giop.gs_ctrl_blk.cb_profile[0].cp_tcp.tcp_msg_fd = fd;
	giop.gs_ctrl_blk.cb_profile[0].cp_tcp.tcp_listen_fd = ERROR;

	size = sizeof (unsigned long);
	::getsockopt (fd, SOL_SOCKET, SO_SNDBUF, (char*)
		      &giop.gs_ctrl_blk.cb_profile[0].cp_tcp.tcp_wr_bufsz,
		      &size);

	::getsockopt (fd, SOL_SOCKET, SO_RCVBUF, (char*)
		      &giop.gs_ctrl_blk.cb_profile[0].cp_tcp.tcp_rd_bufsz,
		      &size);

	// Free up any old memory being used before re-assigning the
        // giop state for the handle
        // 
//	delete h->giop_state();
	h->giop_state(giop);

	if (IT_IIOPAgent::instance()->m_interpreter)
	{
	    if ( (IT_IIOPAgent::instance()->m_interpreter)(h) == ERROR)
	    {
                /*
                  DK - free up some stuff not needed in an error condition.
                */
                h->socket()->shutdown();
                delete h->socket();
                CDRFree(giop.gs_coder_p);
                delete h->giop_state();
                
                IT_IIOPAgent::instance()->m_hndl_cache[fd] == 0;
                return ERROR;
            }
	    else
            {
                return OK;
            }
	}
    }
    
    return ERROR;
}    

IT_IIOPAgent::interpreterFnT
IT_IIOPAgent:: interpreter (IT_IIOPAgent::interpreterFnT fn)
{
    IT_IIOPAgent::interpreterFnT tmp = m_interpreter;
	
    m_interpreter = fn;
    return tmp;
}

extern "C" void mm()
{
memShow(1);
}
