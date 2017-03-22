//
//  CHttp.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "CHttp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <atlstr.h>
#else
#include <sys/socket.h>
#endif

#include <sstream>

#include "CMySocket.h"
#include "CHttp.h"
#include "Internal.h"
#include "CUtil.h"
#include <assert.h>

#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')

//-------------------------------------------------------
const char requestfilesizehdr[] = "HEAD %s HTTP/1.1\r\n"
"Accept: */*\r\n"
"Host: %s\r\n"
"Connection: close\r\n\r\n";

const char requestfilesizecookiehdr[] = "HEAD %s HTTP/1.1\r\n"
"Host: %s\r\n"
"User-Agent: iTunes/11.1.3 (Windows; Microsoft Windows 7 x64 Ultimate Edition Service Pack 1 (Build 7601)) AppleWebKit/536.30.1\r\n"
"Accept: */*\r\n"
"Cookie: %s\r\n"
"Connection: close\r\n"
"Connection: close\r\n\r\n";

const char requestfilehdr[] = "GET %s HTTP/1.1\r\n"
"Accept: */*\r\n"
"Host: %s\r\n"
"RANGE: bytes=%d-%d\r\n"
"Connection: close\r\n\r\n";

const char requestcookiefilehdr[] = "GET %s HTTP/1.1\r\n"
"Host: %s\r\n"
"User-Agent: iTunes/11.0.1 (Windows; (Build 9200)) AppleWebKit/536.27.1\r\n"
"Accept: */*\r\n"
"RANGE: bytes=%d-%d\r\n"
"Cookie: %s\r\n"
"Connection: close\r\n"
"Connection: close\r\n\r\n";

const char requestcontent[] = "%s %s HTTP/1.1\r\n"
"Accept: */*\r\n"
"Host: %s\r\n"
"Content-Type:application/x-www-form-urlencoded\r\n"
"Content-Length: %d\r\n";

const char requestsinfhdr[] = "GET /sc?id=%s&time=%d HTTP/1.1\r\n"
"Host: %s\r\n"
"Accept: */*\r\n"
"Connection: close\r\n\r\n";

//-------------------------------------------------------
//将字符串url解析成url_t结构体

CHttp::CHttp(const char* URL)
{
    _urlinfo = NULL;
    this->_ParserUrl(URL);
}

CHttp::CHttp(struct url_t* url_t)
{
    printf("constructor execute\n");
    int urllen = sizeof(struct url_t);
    _urlinfo = (struct url_t*)malloc(urllen);
    memset(_urlinfo, 0, urllen);
    memcpy(_urlinfo, url_t, urllen);
}

CHttp::~CHttp()
{
    this->_ReleaseUrl();
}

//获取url文件内容长度
size_t CHttp::GetContentLength(const char* cookie)
{
    assert(_urlinfo!=NULL);
    
    int ret;
    char httpbuf[4096] = {0};
    char *p;
    size_t len = 0;
    SOCKET sock = -1;
    char doctime[2048] = {0};
    
    struct url_t tmpurlinfo = *(this->_urlinfo);
    CSmartPtr<CMySocket> smtsocket;
    CMySocket *mysocket = NULL;
redirect:
    if(mysocket!=NULL)
    {
        delete mysocket;
        mysocket = NULL;
    }
    mysocket =  new CMySocket();
    smtsocket.Assign(mysocket);

    bool bsucc = smtsocket->Connect(tmpurlinfo._host, tmpurlinfo._port);
    if(!bsucc)
    {
        DEBUGOUT("%s[%d] host[%s] port[%d] Connect failure\n", __FUNCTION__, __LINE__, tmpurlinfo._host, tmpurlinfo._port);
        return 0;
    }
    
    if(strchr(tmpurlinfo._doc, '?'))
    {
        if(cookie && cookie[0])
        {
            //snprintf(doctime, sizeof(doctime), "http://%s%s&time=%d", tmpurlinfo.host, tmpurlinfo.doc, GetTime() / (1000 * 60));
            snprintf(doctime, sizeof(doctime), "http://%s%s", tmpurlinfo._host, tmpurlinfo._doc);
        }
        else
        {
            //snprintf(doctime, sizeof(doctime), "%s&time=%d", tmpurlinfo.doc, GetTime() / (1000 * 60));
            snprintf(doctime, sizeof(doctime), "%s", tmpurlinfo._doc);
        }
    }
    else
    {
        if(cookie && cookie[0])
        {
            //snprintf(doctime, sizeof(doctime), "http://%s%s?time=%d", tmpurlinfo.host, tmpurlinfo.doc, GetTime() / (1000 * 60));
            snprintf(doctime, sizeof(doctime), "http://%s%s", tmpurlinfo._host, tmpurlinfo._doc);
        }
        else
        {
            //snprintf(doctime, sizeof(doctime), "%s?time=%d", tmpurlinfo.doc, GetTime() / (1000 * 60));
            snprintf(doctime, sizeof(doctime), "%s", tmpurlinfo._doc);
        }
    }
    
    
    if(cookie && cookie[0])
    {
        snprintf(httpbuf, sizeof(httpbuf), requestfilesizecookiehdr, doctime, tmpurlinfo._host, cookie);
    }
    else
    {
        snprintf(httpbuf, sizeof(httpbuf), requestfilesizehdr,  doctime, tmpurlinfo._host);
    }
    
    DEBUGOUT("%s[%d] Request[%s]\n", __FUNCTION__, __LINE__, httpbuf);
    
    ret = smtsocket->SendData((unsigned char*)httpbuf, strlen(httpbuf));
    
    if(ret != 0)
    {
        DEBUGOUT("http Send data failure\n");
        goto stop;
    }
    
    memset(httpbuf, 0, sizeof(httpbuf));
    Sleep(HTTP_RECEIVE_SLEEPTIME_MS);
    ret = smtsocket->ReceiveData((unsigned char*)httpbuf, sizeof(httpbuf));
    
    if(ret <= 0)
    {
        DEBUGOUT("%s[%d] ret[%d] errno[%d] Error[%u]\n", __FUNCTION__, __LINE__, ret, errno, errno);
        goto stop;
    }
    
    DEBUGOUT("%s[%d] Respond[%s]\n", __FUNCTION__, __LINE__, httpbuf);
    ret = _GetResponseStatusCode(httpbuf, &p);
    if(ret != 200)
    {
        smtsocket->Close();
        if(ret == 301 || ret == 302)
        {
            char* szlocation = _GetRedirectLocaltion(httpbuf);
            if(szlocation)
            {
                this->_ReleaseUrl();//先释放之前的
                ret = _ParserUrl(szlocation);
                if(ret == 0)
                {
                    tmpurlinfo =  *_urlinfo;
                    DEBUGOUT("Location[%s]\n", _urlinfo->_host);
                    goto redirect;
                }
                DEBUGOUT("%s[%d]\n", __FUNCTION__, __LINE__);
            }
        }
        DEBUGOUT("%s[%d] http reply value = %d host[%s] doc[%s]\n", __FUNCTION__, __LINE__, ret, tmpurlinfo._host, tmpurlinfo._doc);
        return -1;
    }
    
    len = this->_GetHttpResponsePackLength(p);
    if(len <= 0)
    {
        DEBUGOUT("%s[%d] GetHttpReplayPackLength = %zd\n", __FUNCTION__, __LINE__, len);
    }
    
stop:
    smtsocket->Close();
    return len;
}

//重新建立http文件指定文件块下载socket连接
int CHttp::RequestStartDownload(CMySocket **mysocket,
                                const char* host,
                                const char* cookie,
                                PosInfo *posinfo,
                                char *buff,
                                int* len)
{
    int ret;
    char httpbuf[4096] = {0};
//    SOCKET cursock;
    struct url_t tmpurlinfo;
    
    int tmplen;
    char *ptr;
    char *tmpbuff;
    int readcount;
    int totalcount;
    size_t packlen;
    int replycode;
    char doctime[4096] = {0};
    
    *mysocket = NULL;
    if(buff == NULL || len == 0)
    {
        return -1;
    }
    
    if(posinfo->_offset >= posinfo->_end)
    {
        DEBUGOUT("%s[%d] **offset[%d] end[%d]  failure***\n", __FUNCTION__, __LINE__, posinfo->_offset, posinfo->_end);
        *len = 0;
        return -1;
    }
    
    tmpurlinfo = *_urlinfo;
    snprintf(tmpurlinfo._host, MAX_HOST_NAME_LEN, "%s", host);
    
redirect:
    if(*mysocket!=NULL)
    {
        delete *mysocket;
        *mysocket = NULL;
    }
    *mysocket =  new CMySocket();
//    CMySocket* smtsocket = *mysocket;
    
    tmplen = *len;
    tmpbuff = buff;
    
    if(!(*mysocket)->Connect(tmpurlinfo._host, tmpurlinfo._port))
    {
        (*mysocket)->Close();
        delete *mysocket;
        *mysocket = NULL;
        return -1;
    }
    if(cookie && cookie[0])
    {
        snprintf(doctime, sizeof(doctime), "http://%s%s", tmpurlinfo._host, tmpurlinfo._doc);
    }
    else
    {
        snprintf(doctime, sizeof(doctime), "%s", tmpurlinfo._doc);
    }
    
    if(cookie && cookie[0])
    {
        snprintf(httpbuf, sizeof(httpbuf), requestcookiefilehdr, doctime, tmpurlinfo._host, posinfo->_offset, posinfo->_end - 1, cookie);
    }
    else
    {
        snprintf(httpbuf, sizeof(httpbuf), requestfilehdr, doctime, tmpurlinfo._host, posinfo->_offset, posinfo->_end - 1);
    }
    
    DEBUGOUT("%s[%d] Request[%s]\n", __FUNCTION__, __LINE__, httpbuf);
    ret = (*mysocket)->SendData((unsigned char *)httpbuf, strlen(httpbuf));
    if(ret != 0)
    {
        DEBUGOUT("%s[%d] http Send data failure\n", __FUNCTION__, __LINE__);
        (*mysocket)->Close();
        delete *mysocket;
        *mysocket = NULL;
        return -1;
    }
    
    Sleep(HTTP_RECEIVE_SLEEPTIME_MS);
    totalcount = 0;
    while(tmplen)
    {
        readcount = (*mysocket)->ReceiveData((unsigned char *)tmpbuff, tmplen);
        if(readcount <= 0)
        {
            DEBUGOUT("%s[%d] ReceiveData[%d]\n", __FUNCTION__, __LINE__, readcount);
            (*mysocket)->Close();
            delete *mysocket;
            *mysocket = NULL;
            return -1;
        }
        
        DEBUGOUT("%s[%d] readcount=%d Respond[%s]\n", __FUNCTION__, __LINE__, readcount, tmpbuff);
        totalcount += readcount;
        tmpbuff += readcount;
        tmplen -= readcount;
        ptr = strstr(buff, "\r\n\r\n");
        if(ptr != NULL)
        {
            //*ptr = 0;
            break;
        }
    }
    
    if(tmplen == 0 && ptr == NULL)
    {
        DEBUGOUT("%s[%d] ReceiveData error!!\n", __FUNCTION__, __LINE__);
        (*mysocket)->Close();
        //assert(0);
        delete *mysocket;
        *mysocket = NULL;
        return -1;
    }
    
    replycode = _GetResponseStatusCode(buff, NULL);
    if(replycode != 200 && replycode != 206)
    {
        (*mysocket)->Close();
        if(replycode == 301 || replycode == 302)
        {
            char* szlocation = _GetRedirectLocaltion(buff);
            if(szlocation)
            {
                this->_ReleaseUrl();//先释放之前的
                ret = _ParserUrl(szlocation);
                if(ret == 0)
                {
                    tmpurlinfo =  *_urlinfo;
                    DEBUGOUT("Localtion[%s]\n", _urlinfo->_host);
                    goto redirect;
                }
            }
            DEBUGOUT("%s[%d] redirect url error![%s]\n", __FUNCTION__, __LINE__, httpbuf);
        }
        
        DEBUGOUT("%s[%d] replycode=[%d]\n", __FUNCTION__, __LINE__, replycode);
        if(replycode == 403)
        {
            (*mysocket)->Close();
            delete *mysocket;
            *mysocket = NULL;
            return -2;
        }
        delete *mysocket;
        *mysocket = NULL;
        return -3;
    }
    
    tmplen = totalcount - (ptr + 4 - buff);
    if(tmplen == 0)
    {
        *len = 0;
//        *sock = smtsocket->_socketid;
        return 0;
    }
    
    packlen = _GetHttpResponsePackLength(buff);
    if(packlen <= 0)
    {
        (*mysocket)->Close();
        delete *mysocket;
        *mysocket = NULL;
        DEBUGOUT("%s[%d] GetHttpReplayPackLength = %lld error!\n", __FUNCTION__, __LINE__, packlen);
        return -1;
    }
    
    memcpy(buff, ptr + 4, tmplen);
    if(tmplen == packlen)
    {
        *len = tmplen;
//        *sock = cursock;
        return 0;
    }
    
    readcount = (*mysocket)->ReceiveData((unsigned char *)buff + tmplen, *len - tmplen);
    
    if(readcount <= 0)
    {
        DEBUGOUT("%s[%d] ReceiveData[%d]\n", __FUNCTION__, __LINE__, readcount);
        (*mysocket)->Close();
        delete *mysocket;
        *mysocket = NULL;
        return -1;
    }
    *len = readcount + tmplen;
//    *sock = smtsocket->_socketid;
    return 0;
}

//获取http的响应头的返回值，ptr是去除http头信息后的数据指针
int CHttp::_GetResponseStatusCode(const char *httphead, char** ptr)
{
    int value = 0;
    char *p = strstr((char *)httphead, "HTTP/");
    if(p == NULL)
    {
        return -1;
    }
    
    p += 5;
    if (p[0] != '1' || p[1] != '.' || (p[2] != '0' && p[2] != '1'))
    {
        return -2;
    }
    
    p += 4;
    if (!ISDIGIT(p[0]) || !ISDIGIT(p[1]) || !ISDIGIT(p[2]))
    {
        return -3;
    }
    
    value = (p[0] - '0') * 100 + (p[1] - '0') * 10 + (p[2] - '0');
    if(ptr)
    {
        *ptr = p + 4;
    }
    return value;
}

//解析出http头信息中的包长度信息
size_t CHttp::_GetHttpResponsePackLength(const char* httphead)
{
    size_t len;
    char *p = strstr((char*)httphead, "Content-Length");
    if(p == NULL)
    {
        return 0;
    }
    
    p += strlen("Content-Length") + 1;
    while(!ISDIGIT(*p))
    {
        if(*p == 0)
        {
            return 0;
        }
        p++;
    }
    
    for(len = 0; *p && ISDIGIT(*p); ++p)
    {
        len = len * 10 + (*p - '0');
    }
    
    if(*p == 0)
    {
        return 0;
    }
    return len;
}

//获取http头信息中的重定向的url
char* CHttp::_GetRedirectLocaltion(const char *httphead)
{
    char* szlocation = strstr((char *)httphead, "Location:");
    if(szlocation)
    {
        char* tmpptr = strstr(szlocation, "\r\n");
        szlocation += strlen("Location:");
        if(tmpptr)
        {
            *tmpptr = 0;
            if(szlocation[0] == ' ')
            {
                szlocation++;
            }
            return szlocation;
        }
    }
    return 0;
}

//释放url_t中的内存资源
void CHttp::_ReleaseUrl()
{
    //todo
    if(_urlinfo && _urlinfo->_strurl)
    {
//        free(_urlinfo->_strurl);
//        _urlinfo->_strurl = NULL;
        free(this->_urlinfo);
        this->_urlinfo =  NULL;
    }
}

int CHttp::_ParserUrl(const char *URL)
{
    char *p, *q;
    int i;
    char *source;
    int len;
    char *newurl;
    struct url_t urlinfo;
    
    _ReleaseUrl();

    /* allocate struct url */
    urlinfo._strurl = strdup(URL);
    urlinfo._port = 80;
    
    newurl = urlinfo._strurl;
    /* scheme name */
    p = strstr(newurl, ":/");
    if(p)
    {
        urlinfo._scheme = newurl;
        *p = '\0';
        newurl = ++p;
        if(newurl[1] == '/')
        {
            newurl = (p += 2);
        }
    }
    else
    {
        p = newurl;
    }
    
    if(*newurl == 0 || *newurl == '/' || *newurl == '.' ||
       (urlinfo._scheme && urlinfo._scheme[0] == '\0' &&
        strchr(newurl, '/') == NULL && strchr(newurl, ':') == NULL))
    {
        goto nohost;
    }
    //ex: http://weizhiqiang@172.17.60.120/svn/iOS/branches/Modular/iPad_WeiXuGuang/tags/3.7.2
    p = strpbrk(newurl, "/@");
    if(p && *p == '@')
    {
        /* username */
        *p = '\0';
        urlinfo._user = newurl;
        for(q = newurl, i = 0; (*q != ':') && (*q != '@'); q++);
        /* password */
        if(*q == ':')
        {
            *q = '\0';
            urlinfo._pwd = ++q;
            for( i = 0; (*q != ':') && (*q != '@'); q++);
        }
        p++;
    }
    else
    {
        p = newurl;
    }
    
    for(i = 0; *p && (*p != '/') && (*p != ':'); p++)
    {
        if (i < MAX_HOST_NAME_LEN)
        {
            urlinfo._host[i++] = *p;
        }
    }
    
    urlinfo._host[i] = 0;
    /* port */
    if(*p == ':')
    {
        *p = '\0';
        urlinfo._port = 0;
        for(q = ++p; *q && (*q != '/'); q++)
        {
            if(ISDIGIT(*q))
            {
                urlinfo._port = urlinfo._port * 10 + (*q - '0');
            }
            else
            {
                /* invalid port */
                urlinfo._port = 80;
                free(urlinfo._strurl);
                int urllen_ = sizeof(struct url_t);
                _urlinfo = (url_t*)malloc(urllen_);
                memset(_urlinfo, 0, sizeof(struct url_t));
                return -1;
            }
        }
        p = q;
    }
    
nohost:
    /* document */
    if(!*p)
    {
        p = "/";
    }
    
    urlinfo._doc = p;
    len = strlen(urlinfo._doc);
    if(len > 0)
    {
        source = urlinfo._doc;
        source += len;
        while(*(source --) != '/')
        {
            len --;
        }
        urlinfo._filename = source + 2;
    }
    
#if 0
    fprintf(stderr,
            "scheme:   [%s]\n"
            "user:     [%s]\n"
            "password: [%s]\n"
            "host:     [%s]\n"
            "port:     [%d]\n"
            "refoffset:[%d]\n"
            "document: [%s]\n",
            urlinfo.scheme, urlinfo.user, urlinfo.pwd,
            urlinfo.host, urlinfo.port, urlinfo.refoffset, urlinfo。doc);
#endif
    int urllen = sizeof(struct url_t);
    _urlinfo = (url_t*)malloc(urllen);
    memcpy(_urlinfo, &urlinfo, urllen);
//    free(urlinfo._strurl);
    return 0;
}

//http相应头中的文件尺寸
//int CHttp::ParseHttpReplayDownloadDataHead(const char *httphead, size_t* filesize)
//{
//    size_t fsize;
//    char* ptr;
//    int replycode = _GetResponseStatusCode(httphead, &ptr);
//    if(replycode != 200 && replycode != 206)
//    {
//        return -1;
//    }
//    
//    ptr = strstr(ptr + 1, "Content-Range: bytes");
//    if(ptr == NULL)
//    {
//        return replycode;
//    }
//    
//    ptr += strlen("Content-Range: bytes");
//    
//    ptr = strchr(ptr, '/');
//    if(ptr == NULL)
//    {
//        return replycode;
//    }
//    
//    ptr++;
//    for(fsize = 0; *ptr && ISDIGIT(*ptr); ++ptr)
//    {
//        fsize = fsize * 10 + (*ptr - '0');
//    }
//    *filesize = fsize;
//    return replycode;
//}

////发起简单的http请求，并获取服务器端的响应数据
//char* CHttp::GetUrlRespondContent(const char *URL, const char* postparaminfo, int agentflag, int* errcode)
//{
//    struct url_t urlinfo;
//    int iret;
//    char* ptrhttpbuf = NULL;
//    char *p;
//    char *bodyptr = NULL;
//    int contentlen = 0;
//    SOCKET sock = -1;
//    int revlen;
//    char* retbuff;
//    //CStringA sRevdata;
//    std::string sRevdata;
//
//    if(URL == NULL)
//    {
//        *errcode = -1;
//        return NULL;
//    }
//
//    if(g_exitflag)
//    {
//        return NULL;
//    }
//
//    iret = ParserUrl(&urlinfo, URL);
//    if(iret != 0)
//    {
//        DEBUGOUT("%s[%d] ParseAppFileInfo error!\n", __FUNCTION__, __LINE__);
//        *errcode = -2;
//        return NULL;
//    }
//
//    sock = Connect(urlinfo.host, urlinfo.port);
//    if(sock == -1)
//    {
//        DEBUGOUT("%s[%d] Connect failure\n", __FUNCTION__, __LINE__);
//        *errcode = -3;
//        ReleaseUrl(&urlinfo);
//        return NULL;
//    }
//
//    if(g_exitflag)
//    {
//        ReleaseUrl(&urlinfo);
//        Close(sock);
//        return NULL;
//    }
//
//    contentlen = strlen(requestcontent) + strlen(URL) + 2048;
//    if(postparaminfo)
//    {
//        contentlen += strlen(postparaminfo);
//    }
//
//    if(contentlen < 2048)
//    {
//        contentlen = 2048;
//    }
//
//    ptrhttpbuf = (char *)calloc(1, contentlen);
//    if(ptrhttpbuf == NULL)
//    {
//        Close(sock);
//        *errcode = -1;
//        ReleaseUrl(&urlinfo);
//        return NULL;
//    }
//
//    if(postparaminfo)
//    {
//        snprintf(ptrhttpbuf, contentlen, requestcontent, "POST", urlinfo.doc, urlinfo.host, strlen(postparaminfo));
//        if(g_useragent && agentflag)
//        {
//            strcat(ptrhttpbuf, "User-Agent: ");
//            strcat(ptrhttpbuf, g_useragent);
//            strcat(ptrhttpbuf, "\r\n");
//        }
//        strcat(ptrhttpbuf, "Connection: close\r\n\r\n");
//        strcat(ptrhttpbuf, postparaminfo);
//    }
//    else
//    {
//        snprintf(ptrhttpbuf, contentlen, requestcontent, "GET", urlinfo.doc, urlinfo.host, 0);
//        if(g_useragent && agentflag)
//        {
//            strcat(ptrhttpbuf, "User-Agent: ");
//            strcat(ptrhttpbuf, g_useragent);
//            strcat(ptrhttpbuf, "\r\n");
//        }
//        strcat(ptrhttpbuf, "Connection: close\r\n\r\n");
//    }
//
//    if(g_exitflag)
//    {
//        Close(sock);
//        free(ptrhttpbuf);
//        ReleaseUrl(&urlinfo);
//        return NULL;
//    }
//
//    iret = SendData(sock, (unsigned char *)ptrhttpbuf, strlen(ptrhttpbuf));
//    ReleaseUrl(&urlinfo);
//    //DEBUGOUT("%s[%d] ret[%d] send[%s]\n", __FUNCTION__, __LINE__, iret, ptrhttpbuf);
//
//    if(g_exitflag)
//    {
//        Close(sock);
//        free(ptrhttpbuf);
//        return NULL;
//    }
//
//    if(iret != 0)
//    {
//        DEBUGOUT("http Send data failure\n");
//        Close(sock);
//        free(ptrhttpbuf);
//        *errcode = -3;
//        return NULL;
//    }
//
//    //sRevdata.Empty();
//    sRevdata.clear();
//    memset(ptrhttpbuf, 0, contentlen);
//    Sleep(10);
//    do
//    {
//        if(g_exitflag)
//        {
//            Close(sock);
//            free(ptrhttpbuf);
//            return NULL;
//        }
//
//        revlen = ReceiveData(sock, (unsigned char *)ptrhttpbuf, contentlen - 1);
//        if(g_exitflag)
//        {
//            Close(sock);
//            free(ptrhttpbuf);
//            return NULL;
//        }
//
//        if(revlen < 0)
//        {
//            DEBUGOUT("%s[%d] ret[%d] errno[%d] WSAGetLastError[%u]\n", __FUNCTION__, __LINE__, iret, errno, errno);
//            Close(sock);
//            free(ptrhttpbuf);
//            *errcode = -3;
//            return NULL;
//        }
//
//        if(revlen > 0)
//        {
//            ptrhttpbuf[revlen] = 0;
//            sRevdata.append(ptrhttpbuf);
//            //sRevdata.Append(ptrhttpbuf);
//            if(sRevdata.size() > 1024 * 1024)
//            {
//                Close(sock);
//                free(ptrhttpbuf);
//                sRevdata.clear();
//                DEBUGOUT("%s[%d] http reply length too max\n", __FUNCTION__, __LINE__, iret);
//                *errcode = -4;
//                return NULL;
//            }
//        }
//    }while(revlen > 0);
//
//    Close(sock);
//    free(ptrhttpbuf);
//    if(g_exitflag)
//    {
//        return NULL;
//    }
//
//    const char* ptrbuff = sRevdata.c_str();
//    //DEBUGOUT("%s[%d] ReceiveData[%s]\n", __FUNCTION__, __LINE__, ptrbuff);
//    //DEBUGOUT("%s[%d]\n", __FUNCTION__, __LINE__);
//    iret = GetHttpReplyValue(ptrbuff, &p);
//    if(iret != 200)
//    {
//        DEBUGOUT("%s[%d] http reply value = %d\n", __FUNCTION__, __LINE__, iret);
//        *errcode = -4;
//        return NULL;
//    }
//
//    //DEBUGOUT("%s[%d]\n", __FUNCTION__, __LINE__);
//#if 0
//    contentlen = ParseHttpReplayPackLength(p);
//    if(contentlen <= 0)
//    {
//        DEBUGOUT("%s[%d] GetHttpReplayPackLength = %d p[%s]\n", __FUNCTION__, __LINE__, contentlen, p);
//        *errcode = -5;
//        return NULL;
//    }
//#endif
//
//    p = strstr(p, "\r\n\r\n");
//    if(p == NULL)
//    {
//        DEBUGOUT("can't find http body\n");
//        *errcode = -5;
//        return NULL;
//    }
//    *p = 0;
//    bodyptr = p + 4;
//
//    int isChunkedmode = 0;
//    if(strstr(ptrbuff, "Transfer-Encoding: chunked") != NULL)
//    {
//        //DEBUGOUT("%s[%d] chunked mode\n", __FUNCTION__, __LINE__);
//        isChunkedmode = 1;
//    }
//
//    contentlen = ptrbuff + sRevdata.size() - bodyptr + 1;
//    retbuff = (char *)calloc(1, contentlen);
//    if(retbuff == NULL)
//    {
//        DEBUGOUT("malloc downloadbufinfo failure\n");
//        *errcode = -6;
//        return NULL;
//    }
//
//    if(isChunkedmode == 0)
//    {
//        memcpy(retbuff, bodyptr, ptrbuff + sRevdata.size() - bodyptr);
//        sRevdata.clear();
//        *errcode = 0;
//        return retbuff;
//    }
//
//    int tmpretbuff = 0;
//    long chunkedlen;
//    do
//    {
//        chunkedlen = 0;
//        sscanf(bodyptr, "%lx", &chunkedlen);
//        if(chunkedlen > 0)
//        {
//            bodyptr = strstr(bodyptr, "\r\n");
//            if(bodyptr == NULL)
//            {
//                DEBUGOUT("%s[%d] error http data chunked\n");
//                free(retbuff);
//                *errcode = -6;
//                return NULL;
//            }
//
//            bodyptr += 2;
//            p = strstr(bodyptr, "\r\n");
//            if(p == NULL)
//            {
//                break;
//            }
//
//            memcpy(retbuff + tmpretbuff, bodyptr, chunkedlen);
//            tmpretbuff += chunkedlen;
//            bodyptr = p + 2;
//        }
//    }while(chunkedlen > 0);
//    sRevdata.clear();
//    *errcode = 0;
//    return retbuff;
//}
