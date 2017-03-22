//
//  CLock.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/24.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__CLock__
#define __DownloadApp__CLock__

#include <stdio.h>
#include <pthread.h>

class CLock
{
public:
    CLock()
    {
        pthread_mutex_init(&m_lock, NULL);
    }
    ~CLock()
    {
        pthread_mutex_destroy(&m_lock);
    }
    void Lock()
    {
        pthread_mutex_lock(&m_lock);
    }
    void Unlock()
    {
        pthread_mutex_unlock(&m_lock);
    }
private:
    pthread_mutex_t m_lock;
};

class CAutoLock
{
public:
    CAutoLock(CLock &lock):m_wrapperLock(lock)
    {
        m_wrapperLock.Lock();
    }
    ~CAutoLock()
    {
        m_wrapperLock.Unlock();
    }
private:
    CLock &m_wrapperLock;
};

#endif /* defined(__DownloadApp__CLock__) */
