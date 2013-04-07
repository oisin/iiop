#ifndef _READER_H
#define _READER_H

// Cache for the IT_IIOP_reader

#include <vxWorks.h>
#include <msgQLib.h>
#include <semLib.h>
#include <giop.h>
#include <locks.h>
#include <iiopagent.h>

typedef struct FD_to_QS
{
    MSG_Q_ID _qEntry;
    SEM_ID   _qSem;
    char *   _server;
}FD_to_QT;

typedef struct IT_IIOPMsgHdrS 
{
    GIOPMsgType  msg_type;
    unsigned char *object_key;
    union IT_IIOPMsgHdrU
    {
	GIOPRequestHeader_1_0T  request;
	GIOPReplyHeaderT        reply;
	GIOPCancelRequestHeaderT      cancel_request;
	GIOPLocateRequestHeaderT      locate_request;
	GIOPLocateReplyHeaderT        locate_reply;
    } msg;
} IT_IIOPMsgHdrT;

class FD_to_QTag
{
  private: 
      FD_to_QT  fd_map[FD_SETSIZE];  // a little space inefficient :)
      static IT_mutex _lock;

  public:	
      FD_to_QTag();
      ~FD_to_QTag();
      FD_to_QT& details(int i);
      int isZero(int i);
      void newEntry(MSG_Q_ID, SEM_ID, int fd, char* a, int);
      void delEntry(int fd);
};

/* Functions */
extern "C"
{
    int IT_IIOP_extract_server (unsigned char*, char*, int&);
    int IT_IIOP_queue_msg (IT_IIOPAgent::HNDL, IT_IIOPMsgHdrT*, 
			   MSG_Q_ID, SEM_ID);
    int IT_IIOP_reader (IT_IIOPAgent::HNDL);
}

#endif _READER_H
