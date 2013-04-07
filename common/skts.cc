// Utility socket wrapper
// Encapsulation of an Internet domain TCP/IP Stream socket. This utility class
// allows the creation of a socket from a specified host and port or from an
// existing file descriptor. The resulting socket object has a number of useful
// methods which allow listening and accepting of connections in a server 
// capacity, plus receive and send methods and accessors to get at the file
// descriptor of the socket, it's port and its error state.
//
// Caveats:
// This hasn't been reviewed for thread safety. The current version does not
// have a connect() method. If you give the ctor a port value of zero, the
// IT_INETsckt won't know what port it is on :)
//
// Change Log:
//
//  7nov97   omh   file creation
// 10nov97   omh   addition of file comments 
//                 removed reference counting to IIOPListener
// 13nov97   omh   changed semantics of fd ctor to ensure that the
//                 fd does not get shut when the socket is dtor'ed
// 14nov97   omh   added SO_KEEPALIVE option to socket and tweaked system
//                 vars to produce smaller connection-drop reaction time


#include <skts.h>
#include <sockLib.h>
#include <errno.h>
#include <unistd.h>
#include <ioLib.h>

extern "C" 
{
    void bzero (void *, int);
    int sysClkRateGet ();
}

// OS variables used to tune the SO_KEEPALIVE behaviour of the TCP stack
// Definitely not not portable! :)

extern int tcp_keepintvl, tcp_keepcnt, tcp_keepidle;

// CTOR
//
// Create an internet stream socket on the provided port and IP address.
// There is no sanity checking of the port and IP address done. While the
// value 0 is valid for the IP address (representing IN_ADDRANY), the value
// of 0 for the port will cause the system to pick a port number which this
// constructor cannot deal with correctly. The port number will be kept in
// this object as 0, rather than the system assigned one.

IT_INETsckt:: IT_INETsckt (unsigned short port, unsigned long ipaddr)
{
    m_state = 0;
    m_rsrc = 0;
    
    do 
    {
	m_fd = ::socket (PF_INET, SOCK_STREAM, 0);
    }
    while (errno = EINTR && m_fd == ERROR);
    
    if (m_fd != ERROR)
    {
	bzero ((char *) &m_address, sizeof (sockaddr_in));
	
	m_address.sin_family = PF_INET;
	m_address.sin_port = htons (port);
	m_address.sin_addr.s_addr = htonl (ipaddr);
	
	m_rsrc |= SCKT_EXISTS;
	
	do
	{
	    m_state = ::bind (m_fd,(sockaddr *)&m_address,sizeof(sockaddr_in));
	}
	while (errno == EINTR && m_state == ERROR);
	
	if (m_state != ERROR)
	{
	    int on = 1;
	    do 
	    {
		// For listening, make sure that the address is reusable
		m_state = ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, 
				       (char*)&on, sizeof(int));
	    }
	    while (errno == EINTR && m_state == ERROR);

	    if (m_state != ERROR)
	    {
		// Default behaviour for the keepalive is to declare
		// the connection dead if there has been 
		//   . no traffic on the socket for 2 minutes
		//   . no ack to 8 retries at 75 second intervals
		// Let's change these OS internal variables!!
		
		// Check after no traffic for 30 seconds
		// TODO
		tcp_keepidle = 30 * sysClkRateGet ();
		
		// Retry 10 times at 10 second intervals
		// TODO
		tcp_keepintvl = 10 * sysClkRateGet ();
		tcp_keepcnt   = 10;

		do
		{
		    // All sockets have KEEPALIVE
		    m_state = ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, 
					   (char*)&on, sizeof(int));

		}
		while (errno == EINTR && m_state == ERROR);
	    }
	}	    
    }
    else
    {
	m_state = ERROR;
    }
}

// CTOR
//
// This constructor takes an existing file descriptor and attempts to make
// an IT_INETsckt object from it. Note that if the descriptor is -1 or if
// the descriptor does not represent a valid stream socket, then the 
// constructor will fail and the object state set to ERROR. The port and
// IP address information is obtained when the socket is found to be valid.
//
// Note that when we receive the fd we haven't opened the socket so we 
// cannot assume that it is our job to close it - we just leave the existing
// socket alone on destruction. 

IT_INETsckt:: IT_INETsckt (int fd)
{
    m_rsrc = 0;
    
    if (fd == ERROR)
    {
	m_state = ERROR;
    }
    else
    {
	int stype, size;
	
	m_state = ::getsockopt(fd, SOL_SOCKET, SO_TYPE, (char*)&stype, &size);
	if (m_state != ERROR && stype == SOCK_STREAM)
	{
	    int size;
	    
	    m_fd = fd;
	    m_state = 0;

	    ::getsockname (m_fd, (sockaddr*) &m_address, &size);
	}
    }
}

// DTOR
//
// The destructor uses the resource tick list method to perform actions of
// destruction dependent on the state of the current object. If the socket
// is open, that is created and ready to communicate or already communicating
// then it is shutdown(2) for reading and writing. It is then closed. If the
// construction has failed before the socket was created then this destructor
// does nothing with the user state of this object.

IT_INETsckt:: ~IT_INETsckt ()
{
    if (m_rsrc & SCKT_OPEN)
    {
	::shutdown (m_fd, 2);
    }
    
    if (m_rsrc & SCKT_EXISTS)
    {
	::close (m_fd);
    }	
}

// int IT_INETsckt:: listen ()
//
// Register this socket as listening for a connection. Up to 5 attempts
// can be queued waiting for a connection while one is being processed.
// Returns 0 for success, -1 if there is an error.

int
IT_INETsckt:: listen ()
{
    if (m_state != ERROR)
    {
	m_state = ::listen (m_fd, 5);
	return m_state;
    }

    return ERROR;
}

// int IT_INETsckt:: accept ()
// 
// Uses the accept(2) system call to accept a connection that has been 
// attempted by a peer on this socket. If the connection acceptance succeeds,
// the available information about the peer is stored in the socket object.
// Returns -1 on ERROR, otherwise the file descriptor of the new connection.
// This is a blocking call.

int
IT_INETsckt:: accept ()
{
    int new_sock, size;
    
    if (m_state != ERROR)
    {
	do
	{
	    m_state = ::accept (m_fd, (sockaddr*)&m_peer, &size);
	}
	while (errno == EINTR && m_state == ERROR);
	
	if (m_state != ERROR)
	{
	    new_sock = m_state;
	    m_state = 0;
	    return new_sock;
	}
    }
    
    return ERROR;
}

// int IT_INETsckt:: recv (int, void*)
//
// Reads a number of bytes of data from the socket into a supplied buffer.
// The buffer must exist and be of a guaranteed length to prevent undefined
// behaviour. Returns -1 on error, otherwise the number of bytes actually
// read from the socket. Note that this may not be the same as the length
// that was provided and it is up to the developer to try reading the 
// bytes which remain in the socket. This is a blocking call if the 
// socket has blocking switched on.

int
IT_INETsckt:: recv (int length, void * data)
{
    if (m_state != ERROR)
    {
	int result;
	
	if (length == 0)
	    return 0;
	
	do
	{
	    result = ::recv (m_fd, (char*)data, length, 0);
	}
	while (errno == EINTR && result == ERROR);
	
	return result;
	
    }
    return ERROR;
}

// int IT_INETsckt:: send (int, void*)
//
// Send a number of bytes of data to the socket from a supplied buffer.
// The buffer must exist and be of a guaranteed length to prevent undefined
// behaviour. Returns -1 on error, otherwise the number of bytes actually
// written to the socket. Note that this may not be the same as the length
// that was provided and it is up to the developer to try writing the 
// bytes which remain in the buffer. This is a blocking call.

int
IT_INETsckt:: send (int length, void * data)
{
    if (m_state != ERROR)
    {
	int result;
	
	if (length == 0)
	    return 0;
	
	do
	{
	    result = ::write (m_fd, (char*)data, length);
	}
	while (errno == EINTR && result == ERROR);
    }
    return ERROR;
}

void
IT_INETsckt:: shutdown ()
{
    if (m_state != ERROR)
    {
	::shutdown (m_fd, 2);
	::close (m_fd);
    }
}

void
IT_INETsckt:: blocking (int onoff)
{
    if (m_state != ERROR)
    {
	::ioctl (m_fd, FIONBIO, onoff);
    }
}

// int IT_INETsckt:: state ()
//
// Return the error state of the socket object: -1 (ERROR), 0 (OK) 

int
IT_INETsckt:: state ()
{
    return m_state;
}

// int IT_INETsckt:: descriptor ()
//
// Return the file descriptor that is being used by the socket.
 
int
IT_INETsckt:: descriptor ()
{
    return m_fd;
}

// unsigned short IT_INETsckt:: port ()
//
// Return the port that the socket is using.

unsigned short
IT_INETsckt:: port ()
{
    return ntohs(m_address.sin_port);
}

