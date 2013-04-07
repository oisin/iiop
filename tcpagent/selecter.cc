// Utility selecter object for use with select() based event systems
// The IT_Selecter is intended for use as the main select() loop in a TCP based
// event delivery system. It provides an encapsulation of the loop itself and
// an API to allow manipulation of the event sources which the loop is 
// maintaining as well as a portable interruption mechanism. Internally, the
// class depends on the IT_fd_set and TCPNotifier classes which are used 
// respectively to record the event sources and to allow control of the 
// blocking internal thread. Each instance of the class requires one thread.
//
// Caveats:
// This version does not implement callback routines to the outside world
// to indicate event occurrences. The class is not fully thread safe too.
// This implementation presumes a non-protected memory model and as such
// is only workable on platforms such as VxWorks and pSOS.
//
// Change Log:
//
//  7nov97   omh   file creation
// 10nov97   omh   addition of file comments and callback infrastructure
//                 added semaphore controlled startup of select_thread
//                 made ~IT_Selector semaphore controlled to prevent race
//                 initialised m_cbVec to all zeros
// 13nov97   omh   added ERROR callback return option to remove fd from select

#include <selecter.h>
#include <taskLib.h>
#include <selectLib.h>
#include <unistd.h>
#include <stdio.h>

extern "C" void bzero (void *, int);

// int IT_Selecter:: select_thread (IT_Selecter*)
//
// This is the code body of the internal thread. It is passed the this
// pointer to allow access to the the internal state of the object where
// information pertinent to the select() is stored. When the thread exits,
// either through an unrecoverable error or through instruction from the
// internal TCPNotifier when the IT_Selecter object is destructed, it
// returns an integer for its state: 0 (OK) object is being destructed and
// no error condition exists, -1 (ERROR) the thread is exiting with an 
// error condition outstanding.
//
// When a file descriptor is found to be ready for read and there is an
// IT_Selecter::ActiveFD callback available the callback will be called
// in the context of the IT_Selecter:: select_thread() thread.

int
IT_Selecter:: select_thread (IT_Selecter *ctrl, SEM_ID start_sem)
{
    TCPNotifier::CtrlBlk ctrlblk;
    int live_fd;
    IT_fd_set temp_fdset;

    ctrl->m_fds.set (ctrl->m_notifier.descriptor ());
    // printf ("Set fd %d\n", ctrl->m_notifier.descriptor ());
    
    ctrl->m_running = 1;
    
    // Synchronization for thread startup and exit
    semTake (ctrl->m_thread_sem, WAIT_FOREVER);
    semGive (start_sem);
    
    while (ctrl->m_running)
    {
	// printf ("IT_Selecter:: select thread running\n");
	
	// Copy the permanent fd set into a temporary one for use.
	temp_fdset = ctrl->m_fds;
	
	do
	{
	    ctrl->m_state = ::select(ctrl->m_fds.maxfd()+1, &temp_fdset,0,0,0);
	}
	while (errno == EINTR && ctrl->m_state == ERROR);
	
	// printf ("Select has an event\n");
	
	if (ctrl->m_state != ERROR)
	{
	    // printf ("There are %d sockets live\n", ctrl->m_state);
	    if (ctrl->m_state != 0)
	    {
		if (temp_fdset.isset (ctrl->m_notifier.descriptor ()))
		{
		    // Possible errors here, do recovery
		    int result;
		    
		    do
		    {
			result = ::read (ctrl->m_notifier.descriptor (), 
					 (char *)ctrlblk, sizeof(ctrlblk));
		    }
		    while (errno == EINTR && result == ERROR);
		    
		    // printf ("IT_Selecter:: got a control word 0x%x\n", ctrlblk[0]);
		    if (ctrlblk[0] == IT_Selecter::Interrupt)
			break;
		    else
		    {
			if (ctrlblk[0] == IT_Selecter::Add)
			{
			    if (ctrlblk[1] != ERROR)
				ctrl->m_fds.set (ctrlblk[1]);
			}
			else
			{
			    if (ctrlblk[0] == IT_Selecter::Remove)
			    {
				if (ctrlblk[1] != ERROR)
				    ctrl->m_fds.clear (ctrlblk[1]);
			    }			
			    else
			    {
				if (ctrlblk[0] == IT_Selecter::Exit)
				{
				    ctrl->m_running = 0;
				    break;
				}
			    }
			}
		    }
		    
		    ctrl->m_state--;
		}
		
		temp_fdset.reset ();
		while (ctrl->m_state && (live_fd = temp_fdset.iterate())!=-1)
		{
		    if (live_fd != ctrl->m_notifier.descriptor ()) 
		    {
			// GOT A LIVE ONE, DO SOMETHING!
			// printf ("IT_Selecter:: got a live one 0x%x\n", ctrlblk[1]);
			if (ctrl->m_cbVec[IT_Selecter::ActiveFD] != 0)
			{
			    if ((ctrl->m_cbVec[IT_Selecter::ActiveFD])(live_fd) == ERROR)
			    {
				// There's an error somewhere so remove
				// this file descriptor from the list to
				// prevent infinite loops
				ctrl->m_fds.clear (live_fd);
				// DK temp.  We will explicitly shutdown and close the socket
				//	to try and plug the socket leak.

				::shutdown(live_fd,2);
				::close(live_fd);				

			//	 printf ("removed %d from loop\n", live_fd);
			    }
			}
			
			ctrl->m_state--;
		    }
		}
	    }
	}
	else
	    ctrl->m_running = 0;
    }

    // This assignment is done because the ctrl object may have been
    // deleted by the time this task returns.

    int retval = ctrl->m_state;

    // Deletion synchronization -- allow the dtor to continue now as this
    // thread has finished with the data in the object.
    semGive (ctrl->m_thread_sem); 
    
    // printf ("IT_Selecter:: exiting...\n");
    return retval;
}

// CTOR
//
// Construction of the IT_Selector object causes the creation of a TCPNotifier
// and the creation of a lightweight thread with a small stack which will
// run the select code. The TCPNotifier internal file descriptor is registered
// with the internal file descriptor set of the selecter before the thread
// begins to run. The select() loop will initially pend on the TCPNotifier
// descriptor ready to receive information about additional descriptors.

IT_Selecter:: IT_Selecter (char *name) : m_notifier (name)
{
    SEM_ID start_sem;    
    m_rsrc = 0;
    ::bzero (m_cbVec, sizeof(m_cbVec));
    
    m_state = m_notifier.state ();
    
    if (m_state != ERROR)
    {
	// If this fails then the exit_and_wait() is compromised.
	m_thread_sem = semBCreate (SEM_Q_PRIORITY, SEM_FULL);

	// This temporary semaphore is used to pause this thread until
	// the select_thread has started up.
	start_sem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);

	m_fds.zero ();
	m_fds.set (m_notifier.descriptor ());
	m_tid = taskSpawn (name, 50, 0, 4096, (FUNCPTR)select_thread,
			     (int)this,
			     (int)start_sem, 
			     0, 0, 0, 0, 0, 0, 0, 0);

	// Wait for the thread to start
	if (m_tid != ERROR && start_sem != (SEM_ID)ERROR)
	{
	    semTake (start_sem, WAIT_FOREVER);
	    semDelete (start_sem);
	}
	m_state = 0;
    }
}

// DTOR
//
// Destruction of an IT_Selecter must ensure that the internal select()
// thread has been exitted correctly. Calling interrupt() on the local
// instance sends an interrupt message through the TCPNotifier associated
// with the IT_Selecter to the blocked thread which will read the message
// and break from the select loop.

IT_Selecter:: ~IT_Selecter ()
{
    if (m_tid != ERROR)
	exit_and_wait (WAIT_FOREVER);
}

// void IT_Selecter:: interrupt ()
//
// Interrupt the running of the select loop. Default action is to check the
// m_running variable and if is non-zero then to go running again.

void
IT_Selecter:: interrupt ()
{
    TCPNotifier::CtrlBlk ctrlblk;
    
    ctrlblk[0] = Interrupt;
    ctrlblk[1] = 0;
    
    m_notifier.notify (ctrlblk);
}

// void IT_Selecter:: exit_and_wait ()
//
// Exit the select loop. Sets the value of m_running to 0 then breaks 
// from the loop causing the thread to exit correctly. This method uses
// a simple binary semaphore to wait for the thread to exit. Be careful
// when using this function as it can only be called once by one thread!

void
IT_Selecter:: exit_and_wait (int timeout)
{
    TCPNotifier::CtrlBlk ctrlblk;
    
    ctrlblk[0] = Exit;
    ctrlblk[1] = 0;  // don't wait

    m_notifier.notify (ctrlblk);
    
    if (m_thread_sem != (SEM_ID)ERROR)
    {
	// If the wait for the semaphore times out then we will not
	// delete it as it is still taken by the task and it is not
	// wise to delete semaphores out from under tasks. Instead
	// the select_thread will have to delete it after giving it.
	if (semTake (m_thread_sem, timeout) == ERROR)
	{
	    if (errno != S_objLib_OBJ_UNAVAILABLE)
		semDelete (m_thread_sem);   // error, but not timed out
	}
	else
	    semDelete (m_thread_sem); // successfully taken
    }

    m_tid = ERROR;
    m_thread_sem = (SEM_ID)ERROR;
    m_state = ERROR;
}

// void IT_Selecter:: add_descriptor (int)
//
// Use the TCPNotifier to instruct the select thread to add the supplied file
// descriptor to the select() loop so that read activity conditions can be
// detected.

void
IT_Selecter:: add_descriptor (int fd)
{
    TCPNotifier::CtrlBlk ctrlblk;
    
    ctrlblk[0] = Add;
    ctrlblk[1] = fd;
    
    m_notifier.notify (ctrlblk);
}

// void IT_Selecter:: remove_descriptor (int)
//
// Use the TCPNotifier to instruct the select thread to remove the supplied 
// file descriptor from the select().

void
IT_Selecter:: remove_descriptor (int fd)
{
    TCPNotifier::CtrlBlk ctrlblk;
        
    ctrlblk[0] = Remove;
    ctrlblk[1] = fd;
    
    m_notifier.notify (ctrlblk);
}

// int IT_Selecter:: state ()
//
// Return the internal state of the selecter object: 0 (OK) or -1 (ERROR)

int
IT_Selecter:: state ()
{
    return m_state;
}

// int IT_Selecter:: add_callback (callbackT, callbackFnT)
//
// Add to the selection of callbacks available in the selecter object.
// Currently the only callback available is IT_Selecter::ActiveFD.
// When a file descriptor becomes ready to read, this callback is called
// from the context of the IT_Selecter::select_thread function.

int
IT_Selecter:: add_callback (IT_Selecter::callbackT cbtype, 
			    IT_Selecter::callbackFnT fn)
{
    if (cbtype >= IT_Selecter::MaxCallback)
	return ERROR;   // this callback type is not supported.

    m_cbVec[cbtype] = fn;
    return OK;
}

// int IT_Selecter:: remove_callback (callbackT)
//
// Removes the specified callback type from the list of callbacks currently
// active. Removal means that the callback resumes its specified default
// behaviour

int
IT_Selecter:: remove_callback (IT_Selecter::callbackT cbtype)
{
    if (cbtype >= IT_Selecter::MaxCallback)
	return ERROR;   // this callback type is not supported.

    m_cbVec[cbtype] = 0;
    return OK;
}    
    
