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


// Defines the initialization routines for the DLL.
//

#include "vsock2.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef WIN32

#include <sys/ioctl.h>

#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SO_REUSEPORT    15

#ifdef WIN32
BOOL VS_WSAStartup()
{
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;

        wVersionRequested = MAKEWORD( 2, 2 );

        err = WSAStartup( wVersionRequested, &wsaData );
        if ( err != 0 ) {
                return false;
        }

        if ( LOBYTE( wsaData.wVersion ) != 2 ||
                HIBYTE( wsaData.wVersion ) != 2 ) {
                WSACleanup( );
                return false;
        }
        return true;
}

void VS_WSACleanup()
{
        WSACleanup();
}

#endif

int VS_Create_Server_Socket(
        SOCKET *sock,
        int port) {
    int length;
    struct sockaddr_in server;
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }
    memset((char *) &server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    fcntl(*sock, F_SETFL, O_NONBLOCK);
    fcntl(*sock, F_SETFL, O_NDELAY);
    if (bind(*sock, (struct sockaddr *) &server, (int) sizeof(server)) < 0) {
#ifndef WIN32
        if (errno != EADDRINUSE) {
            close(*sock);
            return (-2);
        }
#ifdef SUNOS
        length = 2;
        setsockopt(*sock,SOL_SOCKET,SO_REUSEPORT,&length,sizeof(int));
#endif
        length = 1;
        setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &length, sizeof(int));
        if (bind(*sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
            close(*sock);
            return (-4);
        }
#else
        close(*sock);
        return (-2);
#endif
    }
    length = (int) sizeof(server);
    if (getsockname(*sock, (struct sockaddr *) &server, (socklen_t * ) & length) < 0) {
        close(*sock);
        return (-3);
    }
    listen(*sock, 10);
    return (0);
}

int VS_Create_Client_Socket(
        unsigned int *sock,
        const char *server_ip,
        int port) {
    struct sockaddr_in server;
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }
    memset((char *) &server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_port = htons(port);
    if (connect(*sock, (struct sockaddr *) &server, (int) sizeof(server)) < 0) {
        close(*sock);
        return (-2);
    }
    return (0);
}

int VS_Peek(int sock, char *buf, int len) {
    return (recv(sock, buf, len, MSG_PEEK));
}

int VS_ReadSocket(
        int RSsockfd,
        char *RSpacket,
        int RSlen,
        int RSheader) {
    int Result;
    unsigned short Len;
    unsigned char lupper[1], llower[1];
    if (RSheader) {
        Result = recv(RSsockfd, (char *) &lupper, 1, 0);
        if (Result <= 0) return (Result);
        Result = recv(RSsockfd, (char *) &llower, 1, 0);
        if (Result <= 0) return (Result);
        Len = (((unsigned short) lupper[0] << 8) + llower[0]);
    } else {
        Len = RSlen;
        Result = 1;
    }
    if (Len > 1024) return (2);
    if (Result > 0) {
        Result = recv(RSsockfd, RSpacket, Len, 0);
    }
    return (Result);
}

int VS_WriteSocket(
        int WSsockfd,
        char *WSpacket,
        int WSlen,
        int WSwithlen) {
    int Result;
    unsigned short Len;
    unsigned char lupper, llower;
    short Offset;
    char SendBuf[10240];
    Len = (unsigned short) WSlen;
    Offset = 0;
    if (WSwithlen) {
        lupper = ((unsigned char) ((Len & 0xFF00) >> 8));
        llower = ((unsigned char) Len & 0x00FF);
        SendBuf[0] = lupper;
        SendBuf[1] = llower;
        Offset += 2;
        Len += 2;
    }
    memcpy(SendBuf + Offset, WSpacket, WSlen);
    if ((Result = send(WSsockfd, SendBuf, Len, 0)) <= 0) {
//              perror("socklib");
        return (-1);
    }
    return (Result);
}

int VS_Communication(char *wbuf, int wlen, char *rbuf, int rtimeout) {
    char hostaddr[128];
    VS_GetHostAddress(hostaddr);
///////////////////////////////////
    struct servent serviceentry;
    struct servent *pserviceentry;
    int service = 6931;
    pserviceentry = getservbyname("someservercom", NULL);
    if (pserviceentry != NULL) {
        memcpy((char *) &serviceentry, pserviceentry, sizeof(struct hostent));
        service = ntohs(serviceentry.s_port);
    }
///////////////////////////////////
    return (VS_CommunicationEx(hostaddr, service, wbuf, wlen, rbuf, rtimeout, 1));
}

int VS_CommunicationEx(
        char *hostaddr, int service,
        char *wbuf, int wlen,
        char *rbuf, int rtimeout, int hdr_exist
) {
    unsigned int sck;
    int res;
    fd_set mysockset;
    struct timeval mtval;
    FD_ZERO(&mysockset);
    if (VS_Create_Client_Socket(&sck, hostaddr, service)) {
        sprintf(rbuf, "Error : Couldn't create socket");
        return (-1);
    }
    FD_SET(sck, &mysockset);
    if (VS_WriteSocket(sck, wbuf, wlen, hdr_exist) <= 1) {
        VS_CloseSocket(sck);
        sprintf(rbuf, "Error : write error");
        return (-2);
    }
    mtval.tv_sec = rtimeout;
    mtval.tv_usec = 0;
    res = select(FD_SETSIZE, &mysockset, 0, 0, &mtval);
    if (res == 0) {
        VS_CloseSocket(sck);
        sprintf(rbuf, "Error : Timeout");
        return (-3);
    }
    if (res < 0) {
        VS_CloseSocket(sck);
        sprintf(rbuf, "Error : select error");
        return (-4);
    }
    if ((res = VS_ReadSocket(sck, rbuf, 1024, hdr_exist)) <= 0) {
        VS_CloseSocket(sck);
        sprintf(rbuf, "Error : read error");
        return (-5);
    }
    VS_CloseSocket(sck);
    return (res);
}

int VS_SendGetWTimer(
        int sck, char *wbuf, int wlen, char *rbuf, int rtimeout, int hdr_exist
) {
    int res;
    fd_set mysockset;
    struct timeval mtval;
    FD_ZERO(&mysockset);
#ifdef _WINDOWS_
    FD_SET((UINT)sck,&mysockset);
#else
    FD_SET(sck, &mysockset);
#endif
    if (VS_WriteSocket(sck, wbuf, wlen, hdr_exist) <= 1) {
        sprintf(rbuf, "Error : write error");
        return (-1);
    }
    mtval.tv_sec = rtimeout;
    mtval.tv_usec = 0;
    res = select(FD_SETSIZE, &mysockset, 0, 0, &mtval);
    if (res == 0) {
        sprintf(rbuf, "Error : Timeout");
        return (-2);
    }
    if (res < 0) {
        sprintf(rbuf, "Error : select error");
        return (-3);
    }
    if ((res = VS_ReadSocket(sck, rbuf, 1024, hdr_exist)) <= 0) {
        sprintf(rbuf, "Error : read error");
        return (-4);
    }
    return (res);
}

int VS_GetService(char *service) {
    struct servent *sp;
    sp = getservbyname(service, "tcp");
    if (sp == NULL) {
        return -1;
    }
    return ((int) ntohs(sp->s_port));
}

void VS_GetHostAddress(char *addr) {
    char localhostname[128];
    char hostaddr[1024];
    char addrstr[1024];
    strcpy(hostaddr, "");
    if (gethostname(localhostname, 128) == 0) {
        struct hostent *pHost;
        int i;
        pHost = gethostbyname(localhostname);
        for (i = 0; pHost != NULL && pHost->h_addr_list[i] != NULL; i++) {
            int j;
            for (j = 0; j < pHost->h_length; j++) {
                if (j > 0)
                    sprintf(hostaddr, "%s.", hostaddr);
                sprintf(addrstr, "%u", (UINT)((UCHAR *) pHost->h_addr_list[i])[j]);
                sprintf(hostaddr, "%s%s", addrstr);
            }
            break;
        }
    }
    sprintf(addr, "%s", hostaddr);
    return;
}

int VS_Select(fd_set *readfs,
              fd_set *writefs,
              fd_set *errorfs,
              struct timeval *timeout) {
    return (select(FD_SETSIZE, readfs, writefs, errorfs, timeout));
}

int VS_Accept(SOCKET sck, unsigned long *cliIP) {
    struct sockaddr_in cli_addr;
    int clilen, res;
    clilen = (int) sizeof(struct sockaddr);
    res = accept(sck, (struct sockaddr *) &cli_addr, (socklen_t * ) & clilen);
    if (res == -1) return -1;
    if (cliIP != NULL) {
        *cliIP = cli_addr.sin_addr.s_addr;
    }
    return (res);
}

int VS_IoCtlSocket(int sck, long cmd, unsigned long *arg) {
#ifdef WIN32
    return(ioctlsocket(sck,(int)cmd,arg));
#else
#ifdef SUNOS
#ifdef FIOSNBIO
    if (cmd==FIOSNBIO)
#else
    if (cmd==FIONBIO)
#endif
    {
            fcntl(sck,F_SETFL,O_NONBLOCK);
            return(fcntl(sck,F_SETFL,O_NDELAY));
    }
#else
    return (ioctl(sck, (int) cmd, (long *) arg));
#endif
#endif
}

void VS_CloseSocket(int sck) {
#ifdef WIN32
    closesocket(sck);
#else
    close(sck);
#endif
}

void VS_IpStr2LongStr(char *Dest, char *Ip) {
    unsigned long l;
    if (!memchr(Ip, '.', strlen(Ip))) {
        strcpy(Dest, Ip);
        return;
    }
    l = inet_addr(Ip);
    sprintf(Dest, "%012u", l);
    return;
}

void VS_LongStr2IpStr(char *Dest, char *LongStr) {
    if (LongStr[0] != '0') {
        strcpy(Dest, LongStr);
        return;
    }
    VS_Long2IpStr(Dest, (unsigned long) atol(LongStr));
}

void VS_Long2IpStr(char *ipstr, unsigned long ip) {
    struct in_addr instrc;
    instrc.s_addr = ip;
    strcpy(ipstr, inet_ntoa(instrc));
}

int VS_CheckConnection(int conn) {
    int res;
    char buf[10];
    if (conn <= 0) return (-1);
    res = recv(conn, buf, 9, MSG_PEEK);
    if (res == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) return (0);
        else return (-2);
    }
    if (res == 0) return (-3);
    return (0);
}

int VS_ConnectEx(unsigned int *sock, char *server_name, int port, int seconds) {
    struct sockaddr_in server;
    fd_set wset;
    fd_set eset;
    struct timeval tv;
    int res;
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(server_name);
    server.sin_port = htons(port);
    if (connect(*sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        if (errno != EINPROGRESS) {
            close(*sock);
            return (-2);
        }
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        FD_ZERO(&wset);
        FD_ZERO(&eset);
        FD_SET(*sock, &wset);
        FD_SET(*sock, &eset);
#ifndef SUNOS
        res = select((*sock) + 1, NULL, &wset, &eset, &tv);
#else
        res = select(FD_SETSIZE,NULL,&wset,&eset,&tv);
#endif
        if (res <= 0) {
            close(*sock);
            return (-3);
        }
        if (!FD_ISSET(*sock, &wset)) {
            close(*sock);
            return (-4);
        }
        if (FD_ISSET(*sock, &eset)) {
            close(*sock);
            return (-5);
        }
        res = VS_CheckConnection(*sock);
        if (res == -2 && errno == ECONNRESET) {
            close(*sock);
            return (-6);
        }
        if (res == -3) {
            if (errno == EINPROGRESS) return 0;
            close(*sock);
            return (-7);
        }
        if (res < 0) return (-8);
    }
    return (0);
}
