#ifndef _LOCK_H
#define _LOCK_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
    sem()
    {
        if(sem_init(&m_sem,0,0) != 0)
            throw std::exception();
    }

    ~sem()
    {
        sem_destroy(&m_sem);
    }

    // 以原子操作将信号量值-1。若信号量值为0，则阻塞直到信号量有非0值
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }

    // 以原子操作将信号量值+1。若信号量值大于0，唤醒正在调用wait()等待信号量的线程
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
private:
    sem_t m_sem;
};


class locker
{
public:
    locker()
    {
        if(pthread_mutex_init(&m_mutex,NULL) != 0)
            throw std::exception();
    }

    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    // 以原子操作给一个互斥锁加锁。若互斥锁已被锁上，则阻塞直到互斥锁的占有者将其解锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    // 以原子操作为一个互斥锁解锁。若有其他线程正在等待这个互斥锁，则唤醒其中一个
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
private:
    pthread_mutex_t m_mutex;
};
#endif