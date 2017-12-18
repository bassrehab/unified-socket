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

/** Socket.cpp: implementation of the CSocket class.
*/

#include "vsock2.h"
#include "socket.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

CSocket::CSocket(BOOL bDebug)
        : CObject(bDebug) {
    m_Socket = 0;
    m_ip = 0;
    m_port = 0;
    m_connectionId = 0;
    m_readHeader = false;
    m_writeHeader = false;
}

CSocket::CSocket(
        SOCKET Socket,
        int id,
        unsigned long ip,
        BOOL readheader,
        BOOL writeheader,
        BOOL bDebug
)
        : CObject(bDebug) {
    m_Socket = Socket;
    m_ip = ip;
    m_port = 0;
    m_connectionId = id;
    m_readHeader = readheader;
    m_writeHeader = writeheader;
    m_bDebug = bDebug;
}

CSocket::~CSocket() {
    if (m_Socket) VS_CloseSocket(m_Socket);
}

int CSocket::ReceiveEx(UCHAR *rbuf, int rlen) {
    int res;
    CSocket *pSocket;
    fd_set rset;
    fd_set eset;
    int maxfd = 0;
    struct timeval tv;
    if (!IsConnected()) return 0;
    FD_ZERO(&rset);
    FD_ZERO(&eset);
    FD_SET(m_Socket, &rset);
    FD_SET(m_Socket, &eset);
    maxfd = m_Socket;
    //
    tv.tv_sec = 0;
    tv.tv_usec = 100;
#ifndef SUNOS
    int selectres = select(maxfd + 1, &rset, NULL, &eset, &tv);
#else
    int selectres = select(FD_SETSIZE,&rset,NULL,&eset,&tv);
#endif
    if (selectres == -1) return -1; // error on select
    if (selectres == 0) return 0;  // timeout
    // something happened
    if (FD_ISSET(m_Socket, &eset)) { // is error set
        return -2;
    }
    if (FD_ISSET(m_Socket, &rset)) { // is incoming data exist
        res = VS_ReadSocket(m_Socket, (char *) rbuf, rlen, m_readHeader);
        if (res == 0) {
            return -3; // shutdown detected
        }
        if (res < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0; // no data found
            return -4; // error detected on socket
        }
        return res; // return recvd data length
    }
    return 0; // retry later
}

int CSocket::Recv(UCHAR *rbuf) {
    int res;
    struct timeval mtval;
    fd_set mysockset;
    if (m_bDebug) printf("try to read from conn. %d\r\n", m_connectionId);
    mtval.tv_sec = 0;
    mtval.tv_usec = 250000;
    FD_ZERO(&mysockset);
    FD_SET(m_Socket, &mysockset);
    res = VS_Select(&mysockset, NULL, NULL, &mtval);
    /**/
// it waits but does not return the exact status
    if (m_bDebug) printf("\r\nselect res %d", res);
//      if (res==0) {
//              if (m_bDebug) printf("timeout ");
//              return(-1); // timeout
//      }
//      if (res<0) {
//              if (m_bDebug) printf("error %d ",res);
//              return(-2); // read error
//      }
//      ULONG   arg;
//      res=VS_IoCtlSocket(m_Socket,FIONREAD,&arg);
//      if (m_bDebug) printf("\r\nioctl res %d",res);
    /**/
    char pbuf[1024];
    res = VS_Peek(m_Socket, pbuf, 1024);
    if (res == 0) {
        return (-3);
    }
    if (res == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return (-1);
        return (-4);
    }
    int bres = res;
    res = VS_ReadSocket(m_Socket, (char *) rbuf, 1024, m_readHeader);
    if (res <= 0) {
//              if (m_bDebug) printf("read error %d ",res);
        return (-4); // error
    }
//      if (m_bDebug) printf("%d-%d\r\n",bres,res);
    return (res);
}

int CSocket::Send(UCHAR *buf, int len) {
    if (m_Socket == 0) return (-1);
    return (VS_WriteSocket(m_Socket, (char *) buf, len, m_writeHeader));
}

void CSocket::Close() {
//      if (m_bDebug) printf("conn. %d closed\r\n",m_connectionId);
//      if (m_bDebug) printf("conn. %d closed\r\n",m_connectionId);
    if (m_Socket) VS_CloseSocket(m_Socket);
    m_Socket = 0;
}

BOOL CSocket::IsConnected() {
    return (m_Socket != 0);
}

int CSocket::GetID() {
    return m_connectionId;
}

void CSocket::SetUnblockingMode() {
    if (IsConnected()) {
        unsigned long arg = 1;
        VS_IoCtlSocket(m_Socket, FIONBIO, &arg);
    }
}

BOOL CSocket::Connect(char *pAddress, int port, int sec) {
    SOCKET d_Socket;
    if (IsConnected()) return true;
    if (pAddress == NULL) return false;
    if (port == 0) return false;
    int res = VS_ConnectEx(&d_Socket, pAddress, port, sec);
    //int res=VS_Create_Client_Socket(&d_Socket,pAddress,port);
    if (res < 0) {
        if (m_bDebug) {
            char temp[16];
            time_t t;
            time(&t);
            strftime(temp, 16, "%H:%M:%S", localtime(&t));
            printf("%s connect to [%s:%d] failed [%d][%d]\r",
                   temp, pAddress, port, res, errno);
            fflush(stdout);
        }
        return false;
    }
    m_Socket = d_Socket;
    m_connecttime.Reset();
    return true;
}

void CSocket::SetHeaders(BOOL readHeader, BOOL writeHeader) {
    m_readHeader = readHeader;
    m_writeHeader = writeHeader;
}

/*
int CSocket::RecvByLen(UCHAR *rbuf, int len)
{
int             res;
struct  timeval  mtval;
fd_set  mysockset;
        mtval.tv_sec=0;
        mtval.tv_usec=250000;
        FD_ZERO(&mysockset);
        FD_SET(m_Socket,&mysockset);
        res=VS_Select(&mysockset,NULL,NULL,&mtval);
        char pbuf[1024];
        res=VS_Peek(m_Socket,pbuf,1024);
        if (res==0) {
                return(-3);
        }
        if (res==-1) {
                if (errno==EAGAIN || errno==EWOULDBLOCK)
                        return(-1);
                return(-4);
        }
        res=VS_ReadSocket(m_Socket,(char*)rbuf,len,m_readHeader);
        if (res<=0) {
                return(-4); // error
        }
        return(res);
}
*/

int CSocket::RecvByLen(UCHAR *rbuf, int rlen, int sec, int usec) {
    int res;
    CSocket *pSocket;
    fd_set rset;
    fd_set eset;
    int maxfd = 0;
    struct timeval tv;
    if (!IsConnected()) return 0;
    FD_ZERO(&rset);
    FD_ZERO(&eset);
    FD_SET(m_Socket, &rset);
    FD_SET(m_Socket, &eset);
    maxfd = m_Socket;
    //
    tv.tv_sec = sec;
    tv.tv_usec = usec;
#ifndef SUNOS
    int selectres = select(maxfd + 1, &rset, NULL, &eset, &tv);
#else
    int selectres = select(FD_SETSIZE,&rset,NULL,&eset,&tv);
#endif
    if (selectres == -1) return -1; // error on select
    if (selectres == 0) return 0;  // timeout
    // something happened
    if (FD_ISSET(m_Socket, &eset)) { // is error set
        return -2;
    }
    if (FD_ISSET(m_Socket, &rset)) { // is incoming data exist
        res = VS_Peek(m_Socket, (char *) rbuf, rlen);
        if (res <= 0) return -5;
        if (res > 0 && res < rlen) {
            return res;
        }
        res = VS_ReadSocket(m_Socket, (char *) rbuf, rlen, m_readHeader);
        if (res == 0) {
            return -3; // shutdown detected
        }
        if (res < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0; // no data found
            return -4; // error detected on socket
        }
        return res; // return recvd data length
    }
    return 0; // retry later
}

int CSocket::Peek(char *buf, int len) {
    return VS_Peek(m_Socket, buf, len);
}

unsigned long CSocket::GetIp() {
    return m_ip;
}

char *CSocket::GetIp(char *ip) {
    VS_Long2IpStr(ip, m_ip);
    return ip;
}

CString CSocket::GetAddr() {
    char ip[16];
    VS_Long2IpStr(ip, m_ip);
    return CString(ip);
}

void CSocket::SetTTY() {
#ifndef WIN32
    ioctl(m_Socket, F_SETFL, O_NOCTTY);
    //fcntl(m_Socket,F_SETFL,O_NOCTTY);
#endif
}

int CSocket::Shutdown() {
    return shutdown(m_Socket, SHUT_WR);
}

