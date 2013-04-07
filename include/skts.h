// Utility socket wrapper
//
// Change Log:
//
//  6nov97   omh   file creation
// 14nov97   omh   added block on/off API call

#ifndef _sckts_h
#define _sckts_h

#include <netinet/in.h>

#define SCKT_EXISTS 1
#define SCKT_OPEN   2

// TODO: require locking on this data structure to serialise
// reads and writes?

class IT_INETsckt
{
    sockaddr_in m_address;
    sockaddr_in m_peer;
    
    int         m_fd;
    short       m_state;
    short       m_rsrc;

    public:

    IT_INETsckt (unsigned short, unsigned long);
    IT_INETsckt (int);
    
    ~IT_INETsckt ();
    
    int listen ();
    int accept ();
    
    int recv (int, void*);
    int send (int, void*);
    void shutdown ();
    void blocking (int);

    int state ();
    int descriptor ();
    unsigned short port ();

};

#endif
