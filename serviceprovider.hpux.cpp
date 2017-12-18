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

/** ServiceProvider.cpp: implementation of the CServiceProvider class.
*/
#include "serviceprovider.h"
#include "socket.h"

#include <stdio.h>
#include <errno.h>

void *ServiceProviderThread(void *param);

CServiceProvider::CServiceProvider(int port, BOOL bDebug)
        : CListeningSocket(bDebug) {
    m_clientId = 1;
    m_port = port;
    m_bDebug = bDebug;
    m_bStop = false;
    m_bStart = false;

    if (pthread_mutex_init(&m_hMutex, NULL) == 0) {
        pthread_attr_t attr;
        if (pthread_attr_init(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_init failed");
        if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
            if (m_bDebug) puts("ERROR:pthread_attr_setdetachstate failed");
        if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM))
            if (m_bDebug) puts("ERROR:pthread_attr_setscope failed");
        if (pthread_create(&m_threadid, &attr, ServiceProviderThread, this))
            if (m_bDebug) puts("ERROR:pthread_create failed");
        if (pthread_attr_destroy(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_destroy failed");
    }
}

CServiceProvider::~CServiceProvider() {
    pthread_join(m_threadid, NULL);
    pthread_mutex_destroy(&m_hMutex);
    //
    CSocket *pSocket;
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        if (pSocket->IsConnected()) pSocket->Close();
        delete pSocket;
        pSocket = NULL;
        pSocket = (CSocket *) m_connections.GetNext();
    }
    m_connections.RemoveAll();
}

BOOL CServiceProvider::Start() {
    if (pthread_mutex_init(&m_hMutex, NULL) == 0) {
        pthread_attr_t attr;
        if (pthread_attr_init(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_init failed");
        if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
            if (m_bDebug) puts("ERROR:pthread_attr_setdetachstate failed");
        if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM))
            if (m_bDebug) puts("ERROR:pthread_attr_setscope failed");
        if (pthread_create(&m_threadid, &attr, ServiceProviderThread, this))
            if (m_bDebug) puts("ERROR:pthread_create failed");
        if (pthread_attr_destroy(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_destroy failed");
        return true;
    }
    return false;
}

void CServiceProvider::CleanUp() {
    StopListening();
    CSocket *pSocket;
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        pSocket->Shutdown();
        pSocket = (CSocket *) m_connections.GetNext();
    }
    sleep(2);
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        pSocket->Close();
        DisconnectInfo(pSocket);
        pSocket = (CSocket *) m_connections.GetNext();
    }
}

void CServiceProvider::AcceptPendingConnectionRequest() {
    unsigned long ip;
    int res = Accept(&ip);
    if (res > 0) {
        CSocket *pSocket;
        pSocket = new CSocket((SOCKET) res, GetNextClientID(), ip, true, true, m_bDebug);
        if (pSocket) {
            pSocket->SetUnblockingMode();
            m_connections.AddTail(pSocket);
            if (m_bDebug) {
                cout << "new connection from ";
                cout << pSocket->GetAddr().GetBuffer() << endl;
            }
            AcceptInfo(pSocket);
        } else {
            if (m_bDebug) puts("failed to create new connection object");
        }
    }
}

UINT CServiceProvider::GetNextClientID() {
    while (GetConnectionByID(m_clientId)) {
        m_clientId++;
        if (m_clientId == 99999)
            m_clientId = 1;
    }
    return m_clientId;
}

CSocket *CServiceProvider::GetConnectionByID(UINT Id) {
    CSocket *pSocket;
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        if ((UINT) pSocket->GetID() == Id) {
            return pSocket;
        }
        pSocket = (CSocket *) m_connections.GetNext();
    }
    return NULL;
}

void CServiceProvider::ReceiveEx() {
    int res;
    UCHAR buf[10240];
    CSocket *pSocket;
    fd_set rset;
    fd_set eset;
    int maxfd = 0;
    struct timeval tv;
    if (m_connections.GetCount() == 0) {
        sleep(1);
        return;
    }
    FD_ZERO(&rset);
    FD_ZERO(&eset);
    pSocket = (CSocket *) m_connections.GetHead();
    for (; pSocket; pSocket = (CSocket *) m_connections.GetNext()) {
        FD_SET(pSocket->m_Socket, &rset);
        FD_SET(pSocket->m_Socket, &eset);
        if (maxfd < pSocket->m_Socket) maxfd = pSocket->m_Socket;
    }
    tv.tv_sec = 1;
    tv.tv_usec = 0;
#ifndef SUNOS
    int selectres = select(maxfd + 1, &rset, NULL, &eset, &tv);
#else
    int selectres = select(FD_SETSIZE,&rset,NULL,&eset,&tv);
#endif
    if (selectres == -1) return; // error on select
    if (selectres == 0) return;  // timeout
    // something happened
    pSocket = (CSocket *) m_connections.GetHead();
    for (; pSocket; pSocket = (CSocket *) m_connections.GetNext()) {
        if (FD_ISSET(pSocket->m_Socket, &eset)) { // is error set
            DisconnectInfo(pSocket);
            pSocket->Close();
            delete pSocket;
            pSocket = NULL;
            m_connections.RemoveCurrent();
            continue;
        }
        if (FD_ISSET(pSocket->m_Socket, &rset)) { // is incoming data exist
            if (Lock() == false) continue;
            res = pSocket->Recv(buf);
            Unlock();
            switch (res) {
                case 0  :
                case -1 :
                case -2 :
                    break;
                case -3 :
                    DisconnectInfo(pSocket);
                    pSocket->Close();
                    delete pSocket;
                    pSocket = NULL;
                    m_connections.RemoveCurrent();
                    continue;
                case -4 :
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    DisconnectInfo(pSocket);
                    pSocket->Close();
                    delete pSocket;
                    pSocket = NULL;
                    m_connections.RemoveCurrent();
                    continue;
                default :
                    ProcessIncoming(pSocket, buf, res);
                    break;
            }
        }
    }
    return;
}

void CServiceProvider::Receive() {
    int res;
    int pcount;
    UCHAR buf[10240];
    CSocket *pSocket;

    pcount = 0;
    for (pSocket = (CSocket *) m_connections.GetHead(); pSocket; pSocket = (CSocket *) m_connections.GetNext()) {
        memset(buf, 0, 10240);
        res = Peek(pSocket);
        if (res == 0) {
            if (m_bDebug) printf("Close 0 %s\r\n", pSocket->GetIp((char *) buf));
            DisconnectInfo(pSocket);
            pSocket->Close();
            delete pSocket;
            pSocket = NULL;
            m_connections.RemoveCurrent();
            continue;
        }
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            if (m_bDebug) printf("Close 1 %s\r\n", pSocket->GetIp((char *) buf));
            DisconnectInfo(pSocket);
            pSocket->Close();
            delete pSocket;
            pSocket = NULL;
            m_connections.RemoveCurrent();
            continue;
        }


        if (Lock() == false) continue;
        res = pSocket->Recv(buf);
        Unlock();
        switch (res) {
            case 0  :
            case -1 :
            case -2 :
                break;
            case -3 :
                DisconnectInfo(pSocket);
                pSocket->Close();
                delete pSocket;
                pSocket = NULL;
                m_connections.RemoveCurrent();
                continue;
            case -4 :
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                DisconnectInfo(pSocket);
                pSocket->Close();
                delete pSocket;
                pSocket = NULL;
                m_connections.RemoveCurrent();
                continue;
            default :
                ProcessIncoming(pSocket, buf, res);
                pcount++;
                break;
        }
    }
    if (pcount == 0) usleep(50000);
}

int CServiceProvider::Peek(CSocket *pSocket) {
    char buf[16];
    return pSocket->Peek(buf, 15);
}

void CServiceProvider::Stop() {
    m_bStop = true;
    pthread_join(GetThreadId(), NULL);
}

BOOL CServiceProvider::IsStopped() {
    return (m_bStop == true);
}

BOOL CServiceProvider::Lock() {
    pthread_mutex_lock(&m_hMutex);
    return true;
}

void CServiceProvider::Unlock() {
    pthread_mutex_unlock(&m_hMutex);
}

int CServiceProvider::Send(CSocket *pSocket, UCHAR *buf, int len) {
    if (Lock() == false) return -1;
    int res = pSocket->Send(buf, len);
    Unlock();
    return res;
}

int CServiceProvider::BroadCast(UCHAR *buf, int len) {
    if (Lock() == false) return -1;
    CSocket *pSocket;
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        pSocket->Send(buf, len);
        pSocket = (CSocket *) m_connections.GetNext();
    }
    Unlock();
    return 0;
}

int CServiceProvider::Send(int id, UCHAR *buf, int len) {
    CSocket *pSocket = GetConnectionByID(id);
    if (pSocket == NULL) return -2;
    return Send(pSocket, buf, len);
}

pthread_t CServiceProvider::GetThreadId() {
    return m_threadid;
}

void CServiceProvider::AcceptInfo(CSocket *pSocket) {
    if (m_bDebug) {
        char temp[32];
        printf("new connection from %s with id %d\r\n",
               pSocket->GetIp(temp), pSocket->GetID());
    }
}

void CServiceProvider::DisconnectInfo(CSocket *pSocket) {
    if (m_bDebug) {
        char temp[32];
        printf("%s connection closed\r\n", pSocket->GetIp(temp));
    }
}

void *ServiceProviderThread(void *param) {
    CServiceProvider *pSP = (CServiceProvider *) param;

    BOOL bDebug = pSP->m_bDebug;

    pSP->m_bStart = pSP->StartUp();
    if (pSP->m_bStart == false) {
        if (bDebug) printf("ServiceProviderThread startup failed\r\n");
        pthread_exit(pSP);
        return NULL;
    }

    while (!pSP->IsStopped()) {
        int res = pSP->StartListening(pSP->m_port);
        if (res == 0) break;
        if (bDebug)
            printf("cannot start service on port %d [%d]\r\n", pSP->m_port, res);
        sleep(1);
    }
    if (bDebug) printf("listener started at port [%d]\r\n", pSP->m_port);

    while (!pSP->IsStopped()) {
        pSP->AcceptPendingConnectionRequest();
        pSP->ReceiveEx();
    }

    pSP->CleanUp();

    pthread_exit(pSP);
    return NULL;
}
