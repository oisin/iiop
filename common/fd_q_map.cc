#include <reader.h>
#include <memLib.h>
#include <itrtlib.h>

IT_mutex FD_to_QTag::_lock;

extern "C"
{
    char *strncpy  (char*, const char*, unsigned int);
}

FD_to_QTag:: FD_to_QTag() 
{
    int i;
    for (i = 0;i < FD_SETSIZE; i++)
    {
	fd_map[i]._qEntry = 0;
	fd_map[i]._qSem = 0;
	fd_map[i]._server = 0;
    }
}

FD_to_QTag:: ~FD_to_QTag() 
{
    int i;
    for (i = 0;i < FD_SETSIZE; i++)
	if (fd_map[i]._server) 
	    ITRT_FREE(fd_map[i]._server);
}		

int 
FD_to_QTag:: isZero(int i) 
{
    IT_scoped_lock guard (_lock);
    if (fd_map[i]._qEntry == 0) 
	return true;
    else
	return false;
}

FD_to_QS& 
FD_to_QTag:: details(int i) 
{
    IT_scoped_lock guard (_lock);
    return fd_map[i];
}

void 
FD_to_QTag:: newEntry(MSG_Q_ID newQ, SEM_ID sem, int fd,char *name,int len)
{
    IT_scoped_lock guard (_lock);
    fd_map[fd]._qEntry = newQ;
    fd_map[fd]._qSem = sem;
    fd_map[fd]._server = (char *)::ITRT_MEMALIGN(4,len+1);
    ::strncpy(fd_map[fd]._server,name,len);
    fd_map[fd]._server[len] = '\0';
}

void 
FD_to_QTag:: delEntry(int fd)
{
    IT_scoped_lock guard (_lock);
    fd_map[fd]._qEntry =  0;
    if (fd_map[fd]._server) 
	ITRT_FREE(fd_map[fd]._server);
    fd_map[fd]._server = 0;
}





