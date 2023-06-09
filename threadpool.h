#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <list>
#include <iostream>
#include <exception>
#include <pthread.h>
#include "lock.h"

template<typename T>
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
template<typename T>
void* threadpool<T>::worker(void* arg)
{
    threadpool* pool = (threadpool*) arg;
    pool->run();
    return pool;
}

template<typename T>
threadpool<T>::threadpool(int thread_number,int max_requests) : thread_number(thread_number),max_requests(max_requests)
{
    stop = false;
    if((thread_number <= 0) || (max_requests <= 0))
        throw std::exception();

    threads = new pthread_t[thread_number];
    if(threads == NULL)
        throw std::exception();

    for(int i = 0;i < thread_number;++i)
    {
        std::cout << "creating the " << (i + 1) << "th thread" << std::endl;
        if(pthread_create(threads + i,NULL,worker,this) != 0)
        {
            delete []threads;
            throw std::exception();
        }

        // 线程主动与主线程断开同步，脱离线程在退出时将自行释放其占有的系统资源
        if(pthread_detach(threads[i]))
        {
            delete []threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete[] threads;
    stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request)
{
    queuelock.lock();
    if(workqueue.size() >= max_requests)
    {
        queuelock.unlock();
        return false;
    }
    workqueue.push_back(request);
    queuelock.unlock();
    queuestat.post();
    return true;
}

template<typename T>
void threadpool<T>::run()
{
    while(!stop)
    {
        queuestat.wait();
        queuelock.lock();
        if(workqueue.empty())
        {
            queuelock.unlock();
            continue;
        }
        T* request = workqueue.front();
        workqueue.pop_front();
        queuelock.unlock();
        if(!request)
            continue;
        request->process();
    }
}
#endif