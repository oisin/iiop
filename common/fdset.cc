// Utility file descriptor set class
// This class encapsulates the fd_set data structure to allow neater 
// access to the structure. It also provides extra functionality in the
// form of a method to return the arithemetic maximum file descriptor
// in the set (required by select(2)) and a method which allows iteration
// through the set to find set bits. 
//
// Caveats:
// This implementation is only useful on systems that implement the fd_set
// structure as a bit vector rather than a list of integers. This rules out
// Windows based systems.
//
// History list:
//
//  5nov97   omh    file creation
// 10nov97   omh    addition of file comments
// 11nov97   omh    fixed 2 bugs in iterate(): infinite loop and early exit
//                  fixed bug in zero() - now zeros all of the fd_set

#include <fdset.h>

extern "C" void bzero (void *, int);

// int IT_fd_set:: iterate ()
//
// Iterates through the file descriptor set and returns the next set
// file descriptor. Returns -1 when there are no more. The iterating
// action can be restarted at the beginning of the file descriptor set
// by calling reset ()

int 
IT_fd_set:: iterate ()
{
    // Not the most efficient version for the first cut :(
    
    while ((iterator_inx <= max_fd) && (!FD_ISSET (iterator_inx, this)))
	iterator_inx++;
    
    if (iterator_inx > max_fd)
    {
	iterator_inx = 0;
	return -1;   // finished iteration
    }
    else
    {
	return iterator_inx++; // return live descriptor and move onto next
    }
}

// void IT_fd_set:: reset ()
// 
// Sets the current location of the iterator in the file descriptor set
// back to the beginning ready to restart.

void
IT_fd_set:: reset ()
{
    iterator_inx = 0;
}

// void IT_fd_set:: clear (int)
//
// Unset the provided file descriptor bit in the file descriptor set

void 
IT_fd_set:: clear (int fd)
{
    FD_CLR(fd, this);
}

// int IT_fd_set:: isset (int)
//
// Return true if the provided file descriptor is set in the file 
// descriptor set

int 
IT_fd_set:: isset (int fd)
{
    return FD_ISSET(fd, this);
}

// void IT_fd_set:: zero ()
//
// Uses the C library bzero() routine to clear all of the bits in the
// file descriptor set.

void
IT_fd_set:: zero ()
{
    bzero (this, sizeof (fd_set));
    iterator_inx = 0;
    max_fd = 0;
}


// void IT_fd_set:: set (int)
//
// Set the provided file descriptor bit in the file descriptor set

void 
IT_fd_set:: set (int fd)
{
    FD_SET(fd, this);
    if (fd > max_fd)
	max_fd = fd;
}

// int IT_fd_set:: maxfd ()
// 
// Return the arithmetically highest file descriptor currently in the
// file descriptor set.

int
IT_fd_set:: maxfd ()
{
    return max_fd;
}

// IT_fd_set& IT_fd_set:: operator= (IT_fd_set&)
//
// Assignment operator for IT_fd_set class. When assigning these objects
// it is necessary to copy both m_maxfd member and the fd_set.

extern "C" void bcopy (void *, void *, int);

IT_fd_set&
IT_fd_set:: operator= (IT_fd_set& rhs)
{
    bcopy (&rhs, this, sizeof (fd_set));
    max_fd = rhs.maxfd ();
    iterator_inx = 0;
}
