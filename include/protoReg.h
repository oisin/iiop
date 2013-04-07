// Protocol registry
//
// 24nov97    omh    file creation

#ifndef _preg_h
#define _preg_h

#include <itmsgp.h>
#include <locks.h>
#include <commsMsg.h>

#define COMMS_PROTOCOL_MAX    5

#define COMMS_POOP_MSG  0
#define COMMS_IIOP_MSG  1

class IT_MsgProcessor;

class IT_ProtocolRegistry
{
  private:
    struct EntryT
    {
	char * name;
	IT_MsgProcessor*  handler;
    }; 

    EntryT m_protocol_table[COMMS_PROTOCOL_MAX];

    static IT_ProtocolRegistry* _instance;
    static IT_mutex _lock;

  protected: 
    IT_ProcotolRegistry ();
    ~IT_ProtocolRegistry ();

  public: 
    static IT_ProtocolRegistry* instance ();
    static void release ();

    IT_MsgProcessor* processor (unsigned int);
    IT_MsgProcessor* register_p (unsigned int, IT_MsgProcessor*);
    IT_MsgProcessor* unregister_p (unsigned int);

};    

#endif // _preg_h

