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

/** ListeningSocket.h: interface for the CListeningSocket class.
*/

#if !defined(AFX_LISTENINGSOCKET_H__DFE7AF80_49D0_4E5E_9025_FD1A3556204E__INCLUDED_)
#define AFX_LISTENINGSOCKET_H__DFE7AF80_49D0_4E5E_9025_FD1A3556204E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "object.h"
#include "vsock2.h"

class CListeningSocket : public CObject {
private:
    SOCKET m_socket;
    BOOL m_bOpen;
public:
    int Accept(unsigned long *ip);

    int StartListening(int port);

    void StopListening();

    void Close();

    BOOL IsOpen();

    CListeningSocket(BOOL bDebug = false);

    virtual ~CListeningSocket();
};

#endif // !defined(AFX_LISTENINGSOCKET_H__DFE7AF80_49D0_4E5E_9025_FD1A3556204E__INCLUDED_)
