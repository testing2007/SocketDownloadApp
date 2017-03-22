//
//  ActiveObj.h
//  DownloadApp
//
//  Created by 魏志强 on 15/8/28.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#ifndef __DownloadApp__ActiveObj__
#define __DownloadApp__ActiveObj__

#include <stdio.h>
//#include "CMemPool.h"
#include <queue>
#include <pthread/pthread.h>

class ActiveObj
{
public:
    ActiveObj();
    virtual ~ActiveObj();
    
    virtual int Start();
    int Stop();
    
    void Dispatch(void* reqParam);
    
    bool Isrunning();

private:
    ActiveObj(const ActiveObj& obj);//禁止拷贝
    static void* _ActiveThread(void* param);
    int _Init();
    void* _Dequeue();
    void _Enqueue(void* reqParam);

protected:
//    virtual void _AllocateQueue() = 0; //初始化内存池,链表的首尾指针
    virtual void _OnThreadStart() = 0; //主动对象线程启动回调
    virtual void _OnRequest(void* reqParam) = 0;
    virtual void _OnThreadEnd() = 0; //主动对象线程结束回调

//    CMemPool* _mempool;
//    MemoryItemNode* _head;
//    MemoryItemNode* _tail;

    volatile bool _brunning;

private:
    std::queue<void*> _queue;
    pthread_mutex_t _lock;
    pthread_mutexattr_t _mutexattr;
    pthread_cond_t _notempty;
    pthread_cond_t _notfull;
    pthread_t _thread;
};

#endif /* defined(__DownloadApp__ActiveObj__) */
