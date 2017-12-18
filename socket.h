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

/** Socket.h: interface for the CSocket class.
*/
#if !defined(AFX_SOCKET_H__2C6A338C_D4CA_4A07_B7DF_D14C0BA5C42F__INCLUDED_)
#define AFX_SOCKET_H__2C6A338C_D4CA_4A07_B7DF_D14C0BA5C42F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "object.h"
#include "vsock2.h"
#include "vstring.h"
#include "vtime.h"

class CSocket : public CObject {
protected:
    unsigned short m_port;
protected:
    unsigned long m_ip;
    int m_connectionId;
    BOOL m_readHeader;
    BOOL m_writeHeader;
public:
    CTime m_connecttime;

    CSocket(BOOL bDebug = false);

    CSocket(SOCKET Socket, int id, unsigned long ip,
            BOOL readheader, BOOL writeheader, BOOL bDebug = false);

    virtual ~CSocket();

public:
    SOCKET m_Socket;

    void SetTTY();

    CString GetAddr();

    char *GetIp(char *ip);

    unsigned long GetIp();

    int Peek(char *buf, int len);

    int RecvByLen(UCHAR *rbuf, int len, int sec = 0, int usec = 500);

    void SetUnblockingMode();

    int GetID();

    virtual void Close();

    virtual int Send(UCHAR *buf, int len);

    virtual int Recv(UCHAR *rbuf);

    int ReceiveEx(UCHAR *rbuf, int len = 1024);

    int Shutdown();

    BOOL IsConnected();

    BOOL Connect(char *pAddress, int port, int sec = 1);

    void SetHeaders(BOOL readHeader = true, BOOL writeHeader = true);
};

#endif // !defined(AFX_SOCKET_H__2C6A338C_D4CA_4A07_B7DF_D14C0BA5C42F__INCLUDED_)
