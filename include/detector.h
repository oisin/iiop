// Header file for Generic IIOPDetector object. 
// This class is a unification of the older IIOPReader and IIOPListener 
// classes
//
// History list:
//
// 14nov97   omh    file creation

#ifndef _detector_h
#define _detector_h

#include <semLib.h>
#include <lstLib.h>
#include <tcpnotify.h>
#include <selecter.h>
#include <skts.h>
#include <locks.h>
#include <ior.h>
#include <giop.h>

class IIOPDetector
{
    private:
    
    LIST         m_active;
    IT_Selecter  m_selecter;
    int          m_state;
    IT_mutex     m_lock;
    
    struct ListNode : NODE
    {
	IT_INETsckt * m_socket;
	int m_refcount;
	GIOPStateT  *m_giop;
	
	ListNode (IIOPDetector* r) 
                : m_giop(0), m_owner(r) , m_socket(0), m_refcount(0)
	{	    
	}
	
	~ListNode ()
	{
		if (m_giop)
			CDRFree(m_giop->gs_coder_p);
	}

	IT_INETsckt * socket() 
	{
	    return m_socket;
	}
	
	int refs ()
	{
	    return m_refcount;
	}


	int shutdown ()
	{
	    if (m_owner)
	    {
		return m_owner->shutdown_detector (this);
	    }
	    else
		return ERROR;
	}

	GIOPStateT*& giop_state ()
        {
	    return m_giop;
	}

	void giop_state (GIOPStateT& giop)
        {
	    // COPY
	    m_giop = new GIOPStateT;
	    *m_giop = giop;
	}

	private: 
	
	IIOPDetector *m_owner;
	
    };
    
    public: 

    typedef ListNode *HNDL;
    
    IIOPDetector (char *);
    ~IIOPDetector ();
    
    HNDL register_detector (unsigned short port);
    HNDL register_detector (int);
    HNDL register_detector (IORT*);

    int shutdown_detector (HNDL);
    int register_callback (IT_Selecter::callbackFnT);
    int shutdown_callback (); 
    
    int state ();

};


#endif // _detector_h
