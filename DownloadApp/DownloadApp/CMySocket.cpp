//
//  CMySocket.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CMySocket.h"
#include "CUtil.h"
#include "Internal.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/errno.h>
#include <assert.h>

CMySocket::CMySocket()
{
    _socketid = INVALID_SOCKET;
}

CMySocket::~CMySocket()
{
    if(_socketid!=INVALID_SOCKET)
    {
        Close();
    }
}

bool CMySocket::Connect(const char* host, int port)
{
    SOCKET tempSocketID = INVALID_SOCKET;
    char strport[10];
    int err;
    
    struct addrinfo hints, *res, *res0;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    
    snprintf(strport, sizeof(strport), "%d", port);
    err = getaddrinfo(host, strport, &hints, &res0);
    if(err != 0)
    {
        return false;
    }
    
    for(tempSocketID=INVALID_SOCKET, res=res0; res; tempSocketID=INVALID_SOCKET, res=res->ai_next)
    {
        tempSocketID = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if(tempSocketID != -1)
        {
            this->SetNonBlock();
            err = connect(tempSocketID, res->ai_addr, res->ai_addrlen);
            if(err == -1)
            {
                struct timeval tm;
                fd_set set;
                tm.tv_sec = 3;
                tm.tv_usec = 0;
                FD_ZERO(&set);
                FD_SET(tempSocketID, &set);
                if(select(tempSocketID+1, NULL, &set, NULL, &tm) > 0)
                {
                    int len = sizeof(int);
                    getsockopt(tempSocketID, SOL_SOCKET, SO_ERROR, &err, (socklen_t*)&len);
                    if(err)
                    {
                        DEBUGOUT("%s[%d] err=%d\n", __FUNCTION__, __LINE__, err);
                    }
                }
                else
                {
                    err = -1;
                }
            }
            
            if(err == 0)
            {
                int set =1;
                int nRecvBufSize = (2 * 1024 * 1024);
                struct timeval timeout = {1, 0};
                setsockopt(tempSocketID, SOL_SOCKET, SO_RCVBUF, &nRecvBufSize, sizeof(int));
                setsockopt(tempSocketID, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
                setsockopt(tempSocketID, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));
                setsockopt(tempSocketID, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int));
                
                freeaddrinfo(res0);
                this->_socketid = tempSocketID;
                this->SetBlock();
                return this->_socketid!=INVALID_SOCKET;
            }//end if(err == 0)
            
            close(tempSocketID);
            
        }//end if(tempSocketID != -1)
    }//end for
    
    freeaddrinfo(res0);
    this->_socketid = tempSocketID;
    return this->_socketid!=INVALID_SOCKET;
}

int CMySocket::SendData(unsigned char* buf, int len)
{
    assert(this->_socketid!=INVALID_SOCKET);
    if (buf==NULL || len==0) {
        return 0;
    }
    
    int need_send_len = len;
    int cur_send_len = 0;
    int errcnt = 0;
    while(need_send_len>0)
    {
        if(need_send_len>1024)
        {
            cur_send_len = send(this->_socketid, buf, 1024, 0);
        }
        else
        {
            cur_send_len = send(this->_socketid, buf, need_send_len, 0);
        }
        
        if(cur_send_len==need_send_len)
        {
            return 0;
        }
        
        if(cur_send_len>=0)
        {
            errcnt = 0;
            need_send_len -= cur_send_len;
            buf += cur_send_len;
            continue;
        }
        
        if(errno==EINTR || errno==EAGAIN)
        {
            errcnt++;
            if(errcnt > 10)
            {
                return -1;
            }
            Sleep(100);
            continue;
        }
        return -1;
    }

    return 0;
}

int CMySocket::ReceiveData(unsigned char* buf, int len)
{
    assert(buf!=NULL);
    int errcnt = 0;
    int rcvlen = 0;
start:
    rcvlen = recv(this->_socketid, buf, len, 0);
    if(rcvlen<0)
    {
        if(errno==EINTR || errno==EAGAIN || errno==ECONNRESET)
        {
            errcnt++;
            if(errcnt > 10)
            {
                return -1;
            }
            Sleep(100);
            goto start;
        }
        DEBUGOUT("%s[%d] fail to recv data, the error number is=%d\n", __FUNCTION__, __LINE__, errno);
        assert(false);
        return -1;
    }
    return rcvlen;
}

int CMySocket::Close()
{
    close(_socketid);
    _socketid = INVALID_SOCKET;
    return 0;
}

int CMySocket::SetNonBlock()
{
    int flags;
    flags = fcntl(_socketid, F_GETFL, 0);
    return fcntl(_socketid, F_SETFL, flags|O_NONBLOCK);
}

int CMySocket::SetBlock()
{
    int flags;
    flags = fcntl(_socketid, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    return fcntl(_socketid, F_SETFL, flags);
}
