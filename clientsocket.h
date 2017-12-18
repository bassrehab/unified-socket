/*
  Copyright (c) 2017 Subhadip Mitra. All rights reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _CLIENTSOCKET_H
#define _CLIENTSOCKET_H

#include "socket.h"

#ifndef WIN32

#include <pthread.h>

#endif


class CClientSocket : public CSocket {
public:
    char m_host[16];
private:
    BOOL m_bStop;
    BOOL m_bStart;
#ifdef WIN32
    HANDLE m_hMutex;
    DWORD m_threadid;
public:
    DWORD GetThreadId() {return m_threadid;};
#else
    pthread_mutex_t m_hMutex;
    pthread_t m_threadid;
public:
    pthread_t GetThreadId() { return m_threadid; };
#endif
public:
    CClientSocket(char *host, unsigned int port, BOOL bDebug = false);

    virtual ~CClientSocket();

public:
    BOOL Connect();

    virtual void OnConnect() {};

    void Close();

    virtual void OnClose() {};

    BOOL IsStopped() { return m_bStop == true; };

    BOOL IsStartupFailed() { return m_bStart == false; };

    BOOL Start();

    BOOL Lock();

    void Unlock();

    void Stop();

    virtual int Send(UCHAR *buf, int len);

    virtual void ProcessIncoming(UCHAR *s, int l)=0;

    virtual BOOL StartUp() {
        if (m_bDebug) puts("CClientSocket:StartUp");
        return false;
    };

    virtual void CleanUp() {};
#ifdef WIN32
    friend DWORD __stdcall ClientSocketThread(void* param);
#else

    friend void *ClientSocketThread(void *param);

#endif
};

#endif // _CLIENTSOCKET_H
