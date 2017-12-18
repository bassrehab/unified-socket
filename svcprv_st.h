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

#if !defined(AFX_SVCPRV_ST_H__D678252D_FBD6_4913_A120_8AE2B1B2D37C__INCLUDED_)
#define AFX_SVCPRV_ST_H__D678252D_FBD6_4913_A120_8AE2B1B2D37C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "listeningsocket.h"
#include "ptrlist.h"

class CSocket;

class CServiceProviderSt : public CListeningSocket {
public:
    int m_port;
    int m_portcount;
private:
    UINT m_clientId;
    CPtrList m_connections;
public:
    void CleanUp();

    int Send(int id, UCHAR *buf, int len);

    int Send(CSocket *pSocket, UCHAR *buf, int len);

    int BroadCast(UCHAR *buf, int len);

    int Peek(CSocket *pSocket);

    int Run();

    void Receive();

    int ReceiveEx();

    CSocket *GetConnectionByID(UINT Id);

    UINT GetNextClientID();

    void AcceptPendingConnectionRequest();

    CServiceProviderSt(int port, int portcount, BOOL bDebug);

    virtual ~CServiceProviderSt();

    virtual void ProcessIncoming(CSocket *pSocket, UCHAR *s, int l)=0;

    virtual void AcceptInfo(CSocket *pSocket);

    virtual void DisconnectInfo(CSocket *pSocket);
};

#endif // !defined(AFX_SVCPRV_ST_H__D678252D_FBD6_4913_A120_8AE2B1B2D37C__INCLUDED_)
