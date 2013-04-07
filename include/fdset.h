// Utility file descriptor set class
//
// Change Log:
//
//  5nov97   omh    file creation
// 11nov97   omh    added code to zero the fd_set on construction
//                  added assignment operator 

#ifndef _fdset_h
#define _fdset_h

#include <types/vxTypesOld.h>

struct IT_fd_set : fd_set
{
    private:
    int iterator_inx;
    int max_fd;
    
    public:
    IT_fd_set () : iterator_inx (0), max_fd (0) { zero (); };

    int iterate ();
    int isset (int);
    int maxfd ();

    void zero ();
    void set (int);
    void reset ();
    void clear (int);

    IT_fd_set& operator= (IT_fd_set&);
};

#endif
