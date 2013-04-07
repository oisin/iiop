// Utility classes used for locking 
// 
// IT_scoped_mutex   uses the ctor/dtor wait/signal model with native VxWorks
//                   priority based (with inversion protection) Mutex objects.
//                   This is an infinite mutex implementation.
// History list:
//
// 10nov97   omh    file creation
// 11nov97   omh    added IT_mutex class and changed IT_scoped_lock to use it

#ifndef _locks_h
#define _locks_h

#include <semLib.h>

class IT_mutex
{
    SEM_ID m_mutex;
    
    public:
    
    IT_mutex ()
    {	
	m_mutex = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
        //printf( "in it_mutex ctor; mutex val is %p.\n", m_mutex );//ask 
        
    }
    
    ~IT_mutex ()
    {
	if (m_mutex != (SEM_ID)ERROR)
	{
	    semDelete (m_mutex);
	}
    }
    
    int state () 
    {
	if (m_mutex != (SEM_ID)ERROR)
	    return OK;
	else
	    return ERROR;
    }

    int wait (int timeout)
    {
	return semTake (m_mutex, timeout);
    }
    
    int signal ()
    {
	return semGive (m_mutex);
    }

    friend class IT_scoped_lock;
};
       
class IT_scoped_lock
{
    IT_mutex *m_mutex;
    int m_state;
    
    public:
    
    IT_scoped_lock (IT_mutex& lock)
    {
	if (lock.m_mutex != (SEM_ID)ERROR)
	{
	    lock.wait (WAIT_FOREVER);
	    m_mutex = &lock;
	    m_state = 0;
	}
	else
	{
	    m_mutex = 0;
	    m_state = -1;
	}
    }
    
    ~IT_scoped_lock ()
    {
	if (m_mutex)
	    m_mutex->signal ();
    }
};

#endif _locks_h
