//
//  ActiveObj.cpp
//  DownloadApp
//
//  Created by 魏志强 on 15/8/28.
//  Copyright (c) 2015年 yeelion. All rights reserved.
//

#include "ActiveObj.h"
#include "CUtil.h"

static const int RQUESTQEUESIZE = 1024;//请求队列大小

ActiveObj::ActiveObj()
{
    
}

ActiveObj::~ActiveObj()
{
}

int ActiveObj::_Init(){
    int ret = pthread_mutexattr_init(&_mutexattr);
    ret = pthread_mutexattr_settype(&_mutexattr, PTHREAD_MUTEX_RECURSIVE);
    ret = pthread_mutex_init(&_lock, NULL);
    //ret = pthread_mutex_init(&_lock, &_mutexattr);//设置递归锁
    ret = pthread_mutexattr_destroy(&_mutexattr);
    ret = pthread_cond_init(&_notempty, NULL);
    ret = pthread_cond_init(&_notfull, NULL);
    _brunning = false;
    return ret;
}

int ActiveObj::Start(){
    int ret = this->_Init();
    ret = pthread_create(&_thread, NULL, ActiveObj::_ActiveThread, this);
    while (!_brunning)//wait for start
        Sleep(0);
    return ret;
}

int ActiveObj::Stop(){
    _brunning = false;
    _Enqueue(0);
    int ret = pthread_join(_thread, 0);
    ret = pthread_cond_destroy(&_notempty);
    ret = pthread_cond_destroy(&_notfull);
    ret = pthread_mutex_destroy(&_lock);
    return ret;
}

void ActiveObj::Dispatch(void *reqParam){
    _Enqueue(reqParam);
}

void* ActiveObj::_ActiveThread(void* param){
    ActiveObj* pThis = (ActiveObj*)param;
    pThis->_OnThreadStart();
    pThis->_brunning = true;
    while (pThis->_brunning) {
        void* ret = pThis->_Dequeue();
        pThis->_OnRequest(ret);
    }
    pThis->_OnThreadEnd();
    return 0;
}

void ActiveObj::_Enqueue(void* reqParam){
    pthread_mutex_lock(&_lock);
    while (_queue.size() >= RQUESTQEUESIZE) {
        pthread_cond_wait(&_notfull, &_lock);//在此处一直等待直到队列没满的时候返回
        Sleep(100);//请求太多,稍等一会。
    }
    _queue.push(reqParam);
    pthread_cond_signal(&_notempty);
    pthread_mutex_unlock(&_lock);
}

void* ActiveObj::_Dequeue(){
    pthread_mutex_lock(&_lock);
    while (_queue.empty()) {
        pthread_cond_wait(&_notempty, &_lock);//在此处一直等待直到队列不为空的时候返回
    }
    void* ret = _queue.front();
    _queue.pop();
    pthread_cond_signal(&_notfull);
    pthread_mutex_unlock(&_lock);
    return ret;
}