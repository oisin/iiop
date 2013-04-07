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
//
// Change Log:
//
// 11nov97    omh    file creation
// 12nov97    omh    added IIOPReader object
// 14nov97    omh    added m_hndl_cache for storing HNDLs
//                   modified to use IIOPDetector class.
// 17nov97    omh    modifications to use IIOP Engine rather than raw sockets
// 18nov97    omh    added new static members for IOR dummy

#ifndef _iiopagent_h
#define _iiopagent_h

#include <detector.h>
#include <semLib.h>
#include <locks.h>
#include <skts.h>
#include <giop.h>
#include <ior.h>

#define  IT_IIOPLAGENT_NAME "iopL"
#define  IT_IIOPRAGENT0_NAME "iopR0"
#define  IT_IIOPRAGENT1_NAME "iopR1"

class IT_IIOPAgent
{
  public: 

    typedef IIOPDetector::HNDL  HNDL;
    typedef int (*interpreterFnT)(HNDL);
    typedef int (*callbackFnT)(int);
  
  private: 
    
    IIOPDetector  *m_listen_agent;
    IIOPDetector  *m_reader_agents[2];

    // TODO: us a less wasteful data structure
    HNDL  m_hndl_cache[FD_SETSIZE];  
    
    interpreterFnT m_interpreter;
    
    static IT_mutex      _lock;
    static IT_IIOPAgent* _instance;
    static IORT          _dummyIOR;
    static IORTaggedProfileT _dummyProfile;
    static int read_callback (int);
    static int listen_callback (int);

  protected: 

    IT_IIOPAgent ();
    ~IT_IIOPAgent ();
    
  public: 
    
    HNDL register_port (char* name, unsigned short port);
    HNDL register_socket (int fd);
    HNDL register_IOR (IORT*);

    int shutdown (HNDL);
    interpreterFnT interpreter (interpreterFnT);

    static IT_IIOPAgent* instance ();
    static void release ();
    static int _sink (HNDL);   // default handler

    // memory allocation routines
    static CDRBufferT * _newbuf (size_t);
    static void         _freebuf (CDRBufferT *);
};


#endif _iiopagent_h
