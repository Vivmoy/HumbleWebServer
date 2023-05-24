#ifndef _LOCK_H
#define _LOCK_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
    sem();
    ~sem();
    // 以原子操作将信号量值-1。若信号量值为0，则阻塞直到信号量有非0值
    bool wait();
    // 以原子操作将信号量值+1。若信号量值大于0，唤醒正在调用wait()等待信号量的线程
    bool post();
private:
    sem_t m_sem;
};

class locker
{
public:
    locker();
    ~locker();
    // 以原子操作给一个互斥锁加锁。若互斥锁已被锁上，则阻塞直到互斥锁的占有者将其解锁
    bool lock();
    // 以原子操作为一个互斥锁解锁。若有其他线程正在等待这个互斥锁，则唤醒其中一个
    bool unlock();
private:
    pthread_mutex_t m_mutex;
};
#endif