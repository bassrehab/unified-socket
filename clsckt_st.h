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
#ifndef _CLSCKT_ST_H
#define _CLSCKT_ST_H

#include "socket.h"
#include "vstring.h"

class CClientSocketSt : public CSocket
{
public:
        char m_host[16];
        CString m_name;
public:
        CClientSocketSt(char* host,UINT port,BOOL bDebug=false,char* name=NULL);
        virtual ~CClientSocketSt();
public:
        BOOL Connect();
        virtual void OnConnect(){};
        void Close();
        int Run();
        virtual void OnClose(){};
        virtual int Send(UCHAR* buf,int len);
        virtual void ProcessIncoming(UCHAR *s,int l)=0;
};
#endif // _CLSCKT_ST_H
#endif
