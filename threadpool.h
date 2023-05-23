#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <list>
#include <iostream>
#include <exception>
#include <pthread.h>
#include "lock.h"

template<class T>
class threadpool
{
public:
    threadpool(int thread_number = 8,int max_requests = 1000);
    ~threadpool();
    bool append(T* request);
private:
    static void* worker(void* arg);
    void run();
private:
    int thread_number;          // 线程池中的线程数
    int max_requests;           // 请求队列允许的最大请求数
    pthread_t* threads;         // 描述线程池的数组，大小为 thread_number
    std::list<T*> workqueue;    // 请求队列
    locker queuelock;           // 请求队列的互斥锁
    sem queuestat;              // 是否有任务需要处理
    bool stop;                  // 是否结束线程
};

// 工作线程运行的函数，不断从工作队列中取出任务并执行
template<class T>
void* threadpool<T>::worker(void* arg)
{
    threadpool* pool = (threadpool*) arg;
    pool->run();
    return pool;
}
#endif