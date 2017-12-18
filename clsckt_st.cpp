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

#ifdef ENABLE_3RD_CONNECTIONS
#include "clsckt_st.h"

CClientSocketSt::CClientSocketSt(char* host,UINT port,BOOL bDebug,char* name)
:CSocket(bDebug)
{
        strcpy(m_host,host);
        m_port = port;
        if (name) m_name = name;
        else m_name = "NAMELESS";
}

CClientSocketSt::~CClientSocketSt()
{
}

BOOL CClientSocketSt::Connect()
{
        BOOL bRes = CSocket::Connect(m_host,m_port);
        if (bRes) {
                SetUnblockingMode();
                if (m_bDebug) {
                        char temp[128];
                        sprintf(temp,"CONNECTED TO %s SERVER [%s:%d]                       ",
                                                            m_name.GetBuffer(),m_host,m_port);
                        puts(temp);
                }
                OnConnect();
        }
        return bRes;
}

void CClientSocketSt::Close()
{
        if (!IsConnected()) return;
        CSocket::Close();
        if (m_bDebug) {
                printf("%s",m_name.GetBuffer());
                puts(" CONNECTION CLOSED");
        }
        OnClose();
}

int CClientSocketSt::Send(UCHAR* buf,int len)
{
        if (m_bDebug) puts("CClientSocketSt::Send");
        int res = CSocket::Send(buf,len);
        return res;
}

int CClientSocketSt::Run()
{
        UCHAR buf[2048];
        int res;
        if (!IsConnected()) {
                Connect();
                if (!IsConnected()) return -1;
        }
        memset(buf,0,2048);
        res = ReceiveEx(buf,2048);
        if (res == 0) return -2;
        if (res < 0) {
                Close();
                return -3;
        }
        ProcessIncoming(buf,res);
        return 0;
}
#endif