// Message processor abstract class
//
// 24nov97   omh    file creation

#ifndef _itmsgp_h
#define _itmsgp_h

#include <commsMsg.h>

class IT_MsgProcessor
{
  protected:

    unsigned int m_id;
    
  public:
      
    IT_MsgProcessor (unsigned int id) 
	: m_id (id) {}
    virtual int process (QCommsMsgT, unsigned int&) = 0;
    unsigned int& id () 
    {
	return m_id;
    }
};

#endif 
