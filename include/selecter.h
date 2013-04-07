// Utility selecter object for use with select() based event systems
//
// Change Log:
//
//  7nov97   omh   file creation
// 10nov97   omh   addition of the exit() method and callback infrastructure
//                 removed exit() and made exit_and_wait() private
//                 added semaphore for dtor control and thread exit

#ifndef _selecter_h
#define _selecter_h

#include <semLib.h>
#include <tcpnotify.h>
#include <fdset.h>

class IT_Selecter
{
    public:
    
    typedef unsigned int callbackT;
    typedef int (*callbackFnT)(int);

    enum { ActiveFD = 0, Pad, MaxCallback = 2 };
    
    private:

    int  m_tid;
    int  m_state;
    int  m_rsrc;
    int  m_running;
    TCPNotifier m_notifier;
    IT_fd_set   m_fds;
    SEM_ID  m_thread_sem;
    callbackFnT m_cbVec[MaxCallback];
    
    enum { Interrupt, Add, Remove, Exit	};

    static int select_thread (IT_Selecter*, SEM_ID);
    void exit_and_wait (int);

    public:
    
    IT_Selecter (char *);
    ~IT_Selecter ();
    
    void interrupt ();
    void add_descriptor (int);
    void remove_descriptor (int);
    int state ();

    // callback registration

    int add_callback (callbackT, callbackFnT);
    int remove_callback (callbackT);
};

#endif
