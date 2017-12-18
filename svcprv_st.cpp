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

#include "svcprv_st.h"
#include "socket.h"

#include <stdio.h>
#include <errno.h>

// Constructors/destructors
CServiceProviderSt::CServiceProviderSt(int port, int portcount, BOOL bDebug)
        : CListeningSocket(bDebug) {
    m_clientId = 1;
    m_port = port;
    m_portcount = portcount;
    m_bDebug = bDebug;
}

CServiceProviderSt::~CServiceProviderSt() {
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

void CServiceProviderSt::CleanUp() {
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

void CServiceProviderSt::AcceptPendingConnectionRequest() {
    unsigned long ip;
    int res = Accept(&ip);
    if (res > 0) {
        CSocket *pSocket;
        pSocket = new CSocket((SOCKET) res, GetNextClientID(), ip, true, true, m_bDebug);
        if (pSocket) {
            pSocket->SetUnblockingMode();
            m_connections.AddTail(pSocket);
            if (m_bDebug) {
                printf("new connection from [");
                printf("%s", pSocket->GetAddr().GetBuffer());
                puts("]                                 ");
            }
            AcceptInfo(pSocket);
        } else {
            if (m_bDebug) puts("failed to create new connection object");
        }
    }
}

UINT CServiceProviderSt::GetNextClientID() {
    while (GetConnectionByID(m_clientId)) {
        m_clientId++;
        if (m_clientId == 99999)
            m_clientId = 1;
    }
    return m_clientId;
}

CSocket *CServiceProviderSt::GetConnectionByID(UINT Id) {
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

int CServiceProviderSt::ReceiveEx() {
    int res;
    UCHAR buf[10240];
    CSocket *pSocket;
    fd_set rset;
    fd_set eset;
    int maxfd = 0;
    struct timeval tv;
    if (m_connections.GetCount() == 0) {
        return -1;
    }
    FD_ZERO(&rset);
    FD_ZERO(&eset);
    pSocket = (CSocket *) m_connections.GetHead();
    for (; pSocket; pSocket = (CSocket *) m_connections.GetNext()) {
        FD_SET(pSocket->m_Socket, &rset);
        FD_SET(pSocket->m_Socket, &eset);
        if (maxfd < pSocket->m_Socket) maxfd = pSocket->m_Socket;
    }
    tv.tv_sec = 0;
    tv.tv_usec = 100;
#ifndef SUNOS
    int selectres = select(maxfd + 1, &rset, NULL, &eset, &tv);
#else
    int selectres = select(FD_SETSIZE,&rset,NULL,&eset,&tv);
#endif
    if (selectres == -1) return -2; // error on select
    if (selectres == 0) return -3;  // timeout
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
            res = pSocket->Recv(buf);
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
                    return res;
            }
        }
    }
    return 0;
}

void CServiceProviderSt::Receive() {
    int res;
    int pcount;
    UCHAR buf[10240];
    CSocket *pSocket;

    pcount = 0;
    for (pSocket = (CSocket *) m_connections.GetHead(); pSocket; pSocket = (CSocket *) m_connections.GetNext()) {
        memset(buf, 0, 10240);
        ////
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
        ///
        //
        res = pSocket->Recv(buf);
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

int CServiceProviderSt::Peek(CSocket *pSocket) {
    char buf[16];
    return pSocket->Peek(buf, 15);
}

int CServiceProviderSt::Send(CSocket *pSocket, UCHAR *buf, int len) {
    int res = pSocket->Send(buf, len);
    return res;
}

int CServiceProviderSt::BroadCast(UCHAR *buf, int len) {
    CSocket *pSocket;
    pSocket = (CSocket *) m_connections.GetHead();
    while (pSocket) {
        pSocket->Send(buf, len);
        pSocket = (CSocket *) m_connections.GetNext();
    }
    return 0;
}

int CServiceProviderSt::Send(int id, UCHAR *buf, int len) {
    CSocket *pSocket = GetConnectionByID(id);
    if (pSocket == NULL) return -2;
    return Send(pSocket, buf, len);
}

void CServiceProviderSt::AcceptInfo(CSocket *pSocket) {
    if (m_bDebug) {
        char temp[32];
        printf("new connection from %s with id %d\r\n",
               pSocket->GetIp(temp), pSocket->GetID());
    }
}

void CServiceProviderSt::DisconnectInfo(CSocket *pSocket) {
    if (m_bDebug) {
        printf("%s connection closed!", pSocket->GetAddr().GetBuffer());
        printf("                                    \r\n");
    }
}

int CServiceProviderSt::Run() {
    if (!IsOpen()) {
        int res = StartListening(m_port);
        if (res) {
            if (m_bDebug)
                printf("cannot start service on port %d [%d]\r\n", m_port, res);
            return -1;
        }
        if (m_bDebug) {
            printf("listener started at port [%d]\r\n", m_port);
        }
    }
    AcceptPendingConnectionRequest();
    int res = ReceiveEx();
    return res;
}
