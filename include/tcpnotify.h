// TCPNotifier helper object to allow control of blocking select()
// entities. Already part of Orbix for VxWorks code base.
//
// Change Log:
// 10nov97    omh    added defaulted pipe size parameter to constructor

#ifndef _tcpnotify_h
#define _tcpnotify_h

class TCPNotifier
{
private:
    int m_descriptor;
    char m_name[16];
    int m_state;
    
public:
    typedef unsigned long CtrlBlk[2];

    TCPNotifier (char *, int pipeSize = 16);
    ~TCPNotifier ();

    int notify (CtrlBlk);
    int descriptor ();
    int state ();
};

#endif //  _tcpnotify_h
