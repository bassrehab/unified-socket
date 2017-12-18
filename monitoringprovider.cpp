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

/** MonitoringProvider.cpp: implementation of the CMonitoringProvider class.
*/
#include "monitoringprovider.h"
#include "socket.h"


CMonitoringProvider::CMonitoringProvider(int service, BOOL bDebug)
        : CListeningSocket(bDebug) {
    m_clientIndex = 1;
    m_bDebug = bDebug;
    m_service = service;
}

CMonitoringProvider::~CMonitoringProvider() {
    CSocket *pSocket;
    pSocket = (CSocket *) m_clients.GetHead();
    while (pSocket) {
        pSocket->Close();
        delete pSocket;
        pSocket = NULL;
        pSocket = (CSocket *) m_clients.GetNext();
    }
    m_clients.RemoveAll();
}

BOOL CMonitoringProvider::Start() {
    int res = StartListening(m_service);
    if (res < 0) {
        if (m_bDebug) printf("can not start monitoring listener\r\n");
        return false;
    } else {
        if (m_bDebug) printf("monitoring listener started at port [%d]\r\n", m_service);
    }
    ////////////////////////////////
    return true;
}

void CMonitoringProvider::DoAccept() {
    unsigned long ip;
    int res = Accept(&ip);
    if (res > 0) {
        CSocket *pSocket;
        pSocket = new CSocket((SOCKET) res, GetClientIndex(), ip, true, true, m_bDebug);
        if (pSocket) {
            pSocket->SetUnblockingMode();
            m_clients.AddHead(pSocket);
        }
    }
}

UINT CMonitoringProvider::GetClientIndex() {
    CSocket *pSocket;
    pSocket = (CSocket *) m_clients.GetHead();
    while (pSocket) {
        if ((UINT) pSocket->GetID() == m_clientIndex) {
            m_clientIndex++;
            pSocket = (CSocket *) m_clients.GetHead();
            continue;
        }
        pSocket = (CSocket *) m_clients.GetNext();
    }
    return m_clientIndex;
}

int CMonitoringProvider::BroadCast(UCHAR *s, int l) {
    int res;
    CSocket *pSocket;
    pSocket = (CSocket *) m_clients.GetHead();
    while (pSocket) {
        res = pSocket->Send(s, l);
        pSocket = (CSocket *) m_clients.GetNext();
    }
    return res;
}

int CMonitoringProvider::Send(UINT destinationId, UCHAR *s, int l) {
    int res;
    CSocket *pSocket;
    pSocket = (CSocket *) m_clients.GetHead();
    while (pSocket) {
        if ((UINT) pSocket->GetID() == destinationId) {
            res = pSocket->Send(s, l);
            break;
        }
        pSocket = (CSocket *) m_clients.GetNext();
    }
    return res;
}

void CMonitoringProvider::Commander() {
    int res;
    UCHAR buf[1024];
    CSocket *pSocket;
    pSocket = (CSocket *) m_clients.GetHead();
    while (pSocket) {
        memset(buf, 0, 1024);
        res = pSocket->Recv(buf);
        switch (res) {
            case 0:
            case -1 :
            case -2 :
                break;
            case -3 :
            case -4 :
                pSocket->Close();
                delete pSocket;
                pSocket = NULL;
                m_clients.RemoveCurrent();
                continue;
            default :
                ProcessIncoming(pSocket, buf, res);
                break;
        }
        pSocket = (CSocket *) m_clients.GetNext();
    }
}

void CMonitoringProvider::Run() {
    if (!IsOpen()) {
        Start();
    }
    DoAccept();
    Commander();
}
