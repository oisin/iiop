// Utility TCP Notifier class
// A TCP Notifier is used to allow control over blocking TCP mechanisms.
// It is typically used as a means to interrupt and pass messages to a 
// thread which is implementing a blocking select. This method of control
// using native file descriptors is more portable than the use of POSIX
// signals or native messaging techniques.
//
// A more primitive version of this class was introduced to Orbix 1.3.5
// for VxWorks to fix a bug (PR25293) which caused a race condition when
// registering new ORB connections and immediately sending a CORBA callback.
// The race condition was between  the main server thread and the TCP
// event detector thread. Which just shows what an inappropriate architecture
// the product is suffering from.
//
// Caveats:
// Despite the comment about portability above, this implementation is purely
// VxWorks dependent and requires the named pipe facility which the OS
// provides.
//
// History list:
//
// 21aug97   omh    initial class authoring for Orbix 1.3.5 codebase
//  5nov97   omh    creation of this file
//  7nov97   omh    addition of name-based constructor 
// 10nov97   omh    addition of file comments
//                  added msg size and pipe size defaults
//                  corrected file descriptor leak in destructor

#include <tcpnotify.h>
#include <ioLib.h>
#include <iosLib.h>
#include <pipeDrv.h>
#include <stdio.h>
#include <itrtlib.h>

// CTOR
//
// Construct the TCPNotifier object with an ascii name. The name string
// is limited to nine characters in length and is copied. The name is
// used to identify the particular notifier and acts as the basis for the
// internal named pipe which is created on the mythical /pipe filesystem.
// There is no policing of the names at this level: if a name clash occurs
// the state of the notifier will be set to ERROR and a state check followed
// by an inspection of errno will show what problem has occurred.

TCPNotifier:: TCPNotifier (char *name, int pipeSize)
{
    sprintf (m_name, "/pipe/%s", name);

    if (pipeSize <= 1)
	pipeSize = 16;   // this is default size
    
    if (pipeDevCreate (m_name, pipeSize, sizeof (TCPNotifier::CtrlBlk)) == OK)
    {
        m_descriptor = open (m_name, O_RDWR, 0);
	m_state = 0;
    }
    else
    {
        m_descriptor = ERROR;
	m_state = ERROR;
    }
}

// DTOR
//
// Find and remove the internal named pipe from the mythical file system.
// and free the associated header memory which was allocated internally
// by the pipe creation procedure.
 
TCPNotifier:: ~TCPNotifier ()
{
    char restOfName[32];
    DEV_HDR *pDevHdr;
 
    if (m_descriptor != ERROR)
	close (m_descriptor);

    if (m_state != ERROR)
    {
	pDevHdr = iosDevFind (m_name, (char**)&restOfName);
	if (pDevHdr != 0)
	{
	    iosDevDelete (pDevHdr);
	    ITRT_FREE(pDevHdr);
	}
    }
}
 
// int TCPNotifier:: notify (TCPNotifier::CtrlBlk)
// 
// Calling the notify() method will cause the passed TCPNotifier::CtrlBlk to
// be written to the internal named pipe. The pipe will then be ready to read
// which will cause an attached blocking TCP mechanism, such as select, to 
// fire. The previously blocked thread can then read the TCPNotifier::CtrlBlk
// from the live notifier file descriptor and take appropriate action. This
// mechanism is especially useful for such actions as adding and removing
// file descriptors from a fd_set used in a select and for informing a blocking
// thread that it is now time to exit cleanly.

int
TCPNotifier:: notify (TCPNotifier::CtrlBlk ctrlblk)
{
    if (m_descriptor == ERROR)
        return ERROR;
    else
    {
        if (write (m_descriptor, (char *)ctrlblk, 8) == ERROR)
	{
            return ERROR;
	}
        else
            return OK;
    }
}
 
// int TCPNotfier:: descriptor ()
//
// Returns the file descriptor currently being used by the built-in
// named pipe.

int
TCPNotifier::descriptor ()
{
    return m_descriptor;
}

// int TCPNotifier:: state ()
//
// Return the current error state of the notifier. Values are 0 (OK) and
// -1 (ERROR). Inspecting the errno thread specific variable immediately
// after a failed state check should indicate the root cause of the error.

int 
TCPNotifier:: state ()
{
    return m_state;
}
