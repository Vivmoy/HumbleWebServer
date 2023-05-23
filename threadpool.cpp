#include "threadpool.h"

template<class T>
threadpool<T>::threadpool(int thread_number,int max_requests) : 
        thread_number(thread_number),max_requests(max_requests),stop(false),threads(NULL)
{
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

template<class T>
threadpool<T>::~threadpool()
{
    delete[] threads;
    stop = true;
}

template<class T>
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

template<class T>
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