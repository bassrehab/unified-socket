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

#ifndef _VSOCK2_H_25671906_12F1_4337_8E4D_2162E85C4148_INCLUDED
#define _VSOCK2_H_25671906_12F1_4337_8E4D_2162E85C4148_INCLUDED

#ifdef WIN32

#include <windows.h>

#else // WIN32

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef SUNOS
#include <sys/filio.h>
#endif

#endif //WIN32


BOOL VS_WSAStartup();

void VS_WSACleanup();

int VS_Create_Server_Socket(
        SOCKET *sock,
        int port);

int VS_Create_Client_Socket(
        SOCKET *sock,
        const char *server_ip,
        int port);

int VS_ReadSocket(
        int RSsockfd,
        char *RSpacket,
        int RSlen,
        int RSheader);

int VS_ReadSocket(
        int RSsockfd,
        char *RSpacket,
        int *RSsource,
        int *RSdestination
);

int VS_WriteSocket(
        int WSsockfd,
        char *WSpacket,
        int WSlen,
        int WSwithlen);

int VS_WriteSocket(
        int WSsockfd,
        char *WSpacket,
        int WSlen,
        int WSsource,
        int WSdestination
);

int VS_Communication(
        char *wbuf,
        int wlen,
        char *rbuf,
        int rtimeout);

int VS_CommunicationEx(
        char *hostaddr,
        int service,
        char *wbuf,
        int wlen,
        char *rbuf,
        int rtimeout,
        int hdr_exist);

int VS_SendGetWTimer(
        SOCKET sck,
        char *wbuf,
        int wlen,
        char *rbuf,
        int rtimeout,
        int hdr_exist);

int VS_GetService(
        char *service);

void VS_GetHostAddress(
        char *addr);

void VS_CloseSocket(
        int sck);

int VS_Accept(
        SOCKET sck, unsigned long *cliIP = NULL);

int VS_IoCtlSocket(
        int sck,
        long cmd,
        unsigned long *arg);

int VS_Select(
        fd_set *readfs,
        fd_set *writefs,
        fd_set *errorfs,
        struct timeval *timeout);

void VS_IpStr2LongStr(
        char *Dest,
        char *Ip);

void VS_LongStr2IpStr(
        char *Dest,
        char *LongStr,
        int len);

void VS_Long2IpStr(
        char *ipstr,
        unsigned long ip);

int VS_Peek(int sock, char *buf, int len);

int VS_CheckConnection(int conn);

int VS_ConnectEx(unsigned int *sock, char *server_name, int port, int seconds);

#endif // _VSOCK2_H_25671906_12F1_4337_8E4D_2162E85C4148_INCLUDED
