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

/** MonitoringProvider.h: interface for the CMonitoringProvider class.
*/
#if !defined(AFX_MONITORINGPROVIDER_H__8D8D5D40_890F_41E4_AFCE_AE40E5308CAC__INCLUDED_)
#define AFX_MONITORINGPROVIDER_H__8D8D5D40_890F_41E4_AFCE_AE40E5308CAC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "listeningsocket.h"
#include "ptrlist.h"

class CSocket;

class CMonitoringProvider : public CListeningSocket {
public:
    void Run();

    virtual void ProcessIncoming(CSocket *pSocket, UCHAR *s, int l)=0;

    void Commander();

    int Send(UINT destinationId, UCHAR *s, int l);

    int BroadCast(UCHAR *s, int l);

    UINT GetClientIndex();

    void DoAccept();

    BOOL Start();

    CMonitoringProvider(int service, BOOL bDebug);

    virtual ~CMonitoringProvider();

private:
    UINT m_clientIndex;
    int m_service;
    CPtrList m_clients;
};

#endif // !defined(AFX_MONITORINGPROVIDER_H__8D8D5D40_890F_41E4_AFCE_AE40E5308CAC__INCLUDED_)
