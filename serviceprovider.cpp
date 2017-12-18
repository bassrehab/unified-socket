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
#ifndef WIN32
#error "this source file has been written for win32 platform"
#endif

#include "serviceprovider.h"
#include "socket.h"
#include "errno.h"

DWORD __stdcall ServiceProviderThread(void *param);

CServiceProvider::CServiceProvider(int port, BOOL bDebug)
        : CListeningSocket(bDebug) {
    m_clientId = 1;
    m_port = port;
    m_bDebug = bDebug;
    m_bStop = false;
    //
    m_hMutex = CreateMutex(NULL, FALSE, "SocketLib::CServiceProvider");
    //
    CreateThread(NULL, 0, ServiceProviderThread, this, 0, &m_threadid);
}

CServiceProvider::~CServiceProvider() {
    CloseHandle(m_hMutex);
    //
    CSocket *pSocket;
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        pSocket->Close();
        delete pSocket;
        pSocket = NULL;
        pSocket = (CSocket *) m_connections.GetNext();
    }
    m_connections.RemoveAll();
}

void CServiceProvider::AcceptPendingConnectionRequest() {
    unsigned long ip;
    int res = Accept(&ip);
    if (res > 0) {
        if (m_bDebug) puts("new connection accepted");
        CSocket *pSocket;
        pSocket = new CSocket((SOCKET) res, GetNextClientID(), ip, true, true, m_bDebug);
        if (pSocket) {
            pSocket->SetUnblockingMode();
            m_connections.AddHead(pSocket);
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

void CServiceProvider::Receive() {
    int res;
    UCHAR buf[1024];
    CSocket *pSocket;
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        memset(buf, 0, 1024);
        res = Peek(pSocket);
        if (res == 0) {
            if (m_bDebug) printf("Close 0 %s\r\n", pSocket->GetIp((char *) buf));
            pSocket->Close();
            delete pSocket;
            pSocket = NULL;
            m_connections.RemoveCurrent();
            continue;
        }
        if (res == -1) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) continue;
            if (m_bDebug) printf("Close 1 %s\r\n", pSocket->GetIp((char *) buf));
            pSocket->Close();
            delete pSocket;
            pSocket = NULL;
            m_connections.RemoveCurrent();
            continue;
        }

        if (Lock(1000) == false) continue;
        res = pSocket->Recv(buf);
        Unlock();
        switch (res) {
            case 0  :
            case -1 :
            case -2 :
                break;
            case -3 :
            case -4 :
                pSocket->Close();
                delete pSocket;
                pSocket = NULL;
                m_connections.RemoveCurrent();
                continue;
            default :
                ProcessIncoming(pSocket, buf, res);
                break;
        }
        pSocket = (CSocket *) m_connections.GetNext();
    }
}

int CServiceProvider::Peek(CSocket *pSocket) {
    char buf[16];
    return pSocket->Peek(buf, 15);
}

void CServiceProvider::Stop() {
    m_bStop = true;
}

BOOL CServiceProvider::IsStopped() {
    return (m_bStop == true);
}

BOOL CServiceProvider::Lock(DWORD miliseconds) {
    DWORD dwRes;
    dwRes = WaitForSingleObject(m_hMutex, miliseconds);
    if (dwRes == WAIT_OBJECT_0) return true;
    return false;
}

void CServiceProvider::Unlock() {
    ReleaseMutex(m_hMutex);
}

int CServiceProvider::Send(CSocket *pSocket, UCHAR *buf, int len) {
    if (Lock(2000) == false) return -1;
    int res = pSocket->Send(buf, len);
    Unlock();
    return res;
}

int CServiceProvider::Send(int id, UCHAR *buf, int len) {
    CSocket *pSocket = GetConnectionByID(id);
    if (pSocket == NULL) return -2;
    return Send(pSocket, buf, len);
}

void CServiceProvider::AcceptInfo(CSocket *pSocket) {
    if (m_bDebug) {
        char temp[32];
        printf("new connection from %s\r\n", pSocket->GetIp(temp));
    }
}

void CServiceProvider::DisconnectInfo(CSocket *pSocket) {
    if (m_bDebug) {
        char temp[32];
        printf("%s connection closed\r\n", pSocket->GetIp(temp));
    }
}

DWORD __stdcall ServiceProviderThread(void *param) {
    CServiceProvider *pSP = (CServiceProvider *) param;

    BOOL bDebug = pSP->m_bDebug;
    int res = pSP->StartListening(pSP->m_port);
    if (res < 0) {
        if (bDebug) printf("can not start service on port %d [%d]\r\n", pSP->m_port, res);
        return -1;
    }
    if (bDebug) printf("listener started at port [%d]\r\n", pSP->m_port);

    while (!pSP->IsStopped()) {
        pSP->AcceptPendingConnectionRequest();
        pSP->Receive();
    }
    return 0;
}
