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

#include "clientsocket.h"

CClientSocket::CClientSocket(char *host, unsigned int port, BOOL bDebug)
        : CSocket(bDebug) {
    m_bStop = false;
    m_bStart = false;
    strcpy(m_host, host);
    m_port = port;

    if (pthread_mutex_init(&m_hMutex, NULL) == 0) {
        pthread_attr_t attr;
        if (pthread_attr_init(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_init failed");
        if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
            if (m_bDebug) puts("ERROR:pthread_attr_setdetachstate failed");
        if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM))
            if (m_bDebug) puts("ERROR:pthread_attr_setscope failed");
        if (pthread_create(&m_threadid, &attr, ClientSocketThread, this))
            if (m_bDebug) puts("ERROR:pthread_create failed");
        if (pthread_attr_destroy(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_destroy failed");
    }
}

CClientSocket::~CClientSocket() {
}

BOOL CClientSocket::Lock() {
    pthread_mutex_lock(&m_hMutex);
    return true;
}

void CClientSocket::Unlock() {
    pthread_mutex_unlock(&m_hMutex);
}

BOOL CClientSocket::Connect() {
    BOOL bRes = CSocket::Connect(m_host, m_port);
    if (bRes) {
        SetUnblockingMode();
        OnConnect();
    }
    return bRes;
}

void CClientSocket::Close() {
    CSocket::Close();
    OnClose();
}

BOOL CClientSocket::Start() {
    if (pthread_mutex_init(&m_hMutex, NULL) == 0) {
        pthread_attr_t attr;
        if (pthread_attr_init(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_init failed");
        if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))
            if (m_bDebug) puts("ERROR:pthread_attr_setdetachstate failed");
        if (pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM))
            if (m_bDebug) puts("ERROR:pthread_attr_setscope failed");
        if (pthread_create(&m_threadid, &attr, ClientSocketThread, this))
            if (m_bDebug) puts("ERROR:pthread_create failed");
        if (pthread_attr_destroy(&attr))
            if (m_bDebug) puts("ERROR:pthread_attr_destroy failed");
        return true;
    }
    return false;
}

int CClientSocket::Send(UCHAR *buf, int len) {
    if (m_bDebug) puts("CClientSocket::Send");
    Lock();
    int res = CSocket::Send(buf, len);
    Unlock();
    return res;
}

void CClientSocket::Stop() {
    m_bStop = true;
    pthread_join(GetThreadId(), NULL);
    pthread_mutex_destroy(&m_hMutex);
}

void *ClientSocketThread(void *param) {
    CClientSocket *pCS = (CClientSocket *) param;
    BOOL bDebug = pCS->m_bDebug;
    //
    pCS->m_bStart = pCS->StartUp();
    if (pCS->m_bStart == false) {
        pthread_exit(pCS);
        return NULL;
    }
    //
    pCS->Connect();
    //
    UCHAR buf[2048];
    int res;
    while (!pCS->IsStopped()) {
        if (!pCS->IsConnected()) {
            pCS->Connect();
            if (!pCS->IsConnected()) {
                sleep(1);
                continue;
            }
        }
        memset(buf, 0, 2048);
        res = pCS->ReceiveEx(buf, 2048);
        if (res == 0) continue;
        if (res < 0) {
            pCS->Close();
            continue;
        }
        pCS->ProcessIncoming(buf, res);
    }
    //
    pCS->CleanUp();
    //
    pthread_exit(pCS);
    return NULL;
}

