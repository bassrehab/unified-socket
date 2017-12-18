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

/** ListeningSocket.cpp: implementation of the CListeningSocket class.
*/

//#include "stdafx.h"
#include "listeningsocket.h"

CListeningSocket::CListeningSocket(BOOL bDebug)
        : CObject(bDebug) {
    m_socket = 0;
    m_bOpen = false;
}

CListeningSocket::~CListeningSocket() {
    if (m_socket) VS_CloseSocket(m_socket);
}

void CListeningSocket::Close() {
    if (m_socket) VS_CloseSocket(m_socket);
    m_socket = 0;
    m_bOpen = false;
}

int CListeningSocket::StartListening(int port) {
    if (m_bOpen) return 0;
    int result = VS_Create_Server_Socket(&m_socket, port);
    if (result < 0) {
        m_socket = 0;
        return result;
    }
    unsigned long arg = 1;
    VS_IoCtlSocket(m_socket, FIONBIO, &arg); // set unblocking mode
    m_bOpen = true;
    return (0);
}

int CListeningSocket::Accept(unsigned long *ip) {
    if (m_bOpen == false) return -1;
    int result = VS_Accept(m_socket, ip);
    if (result > 0) {
        unsigned long arg = 1;
        VS_IoCtlSocket(result, FIONBIO, &arg); // set unblocking mode
    }
    return result;
}

BOOL CListeningSocket::IsOpen() {
    return m_bOpen;
}

void CListeningSocket::StopListening() {
    close(m_socket);
    m_bOpen = false;
}
