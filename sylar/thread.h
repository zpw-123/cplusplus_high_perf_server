#ifndef _THREAD_H__
#define _THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>

namespace zpw {

class Semaphore {//信号量的类
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();//就是有一个消息队列，有消息就消费，没有就wait
    void notify();
private:
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
private:
    sem_t m_semaphore;//本质是一个长整型的数
};

//通用的模板类，传进来都是互斥量，只是负责锁和解锁这几个方法就行
template<class T>
struct ScopedLockImpl {
public://上面这个T的模板 应该只是限于 互斥量，自旋锁，原子锁
    ScopedLockImpl(T& mutex)//互斥量不可以复制，所以引用传参数
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }

    ~ScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct ReadScopedLockImpl {
public://封装了一个读锁
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl() {
        unlock();
    }

    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;
};

class Mutex {
public:
    typedef ScopedLockImpl<Mutex> Lock;
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

class NullMutex {
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

//封装一下读写锁
class RWMutex {
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);//初始化一个读写锁对象的指针
    }

    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);//用于获取读写锁上面的共享读取的权限
    }

    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};
class NullRWMutex {
public:
    typedef ReadScopedLockImpl<NullMutex> ReadLock;
    typedef WriteScopedLockImpl<NullMutex> WriteLock;

    NullRWMutex() {}
    ~NullRWMutex() {}

    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

class Spinlock {//这个是自旋锁，不会让进程进入睡眠，从而避免了线程进入退出睡眠的开销
public:
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};
class NullSpinlock {
public:
    typedef ScopedLockImpl<NullSpinlock> Lock;
    NullSpinlock() {}
    ~NullSpinlock() {}
    void lock() {}
    void unlock() {}
};

class CASLock {//原子锁
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();//原子操作无需担心竞争；clear()函数会将给定的原子标志位清除或者重置为未设置状态
    }
    ~CASLock() {
    }

    void lock() {
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
        //就是一个原子操作，如果原子标志位是设置的，就返回true，否则设置为true，返回false
        //std::memory_order_acquire 是一种内存序，指定原子操作的同步和内存顺序
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;//给定的原子标志位
};

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();
    pid_t getId() const { return m_id;}//常成员函数，不会修改成员变量
    const std::string& getName() const { return m_name;}

    void join();

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    Thread(const Thread&) = delete;//禁止拷贝构造
    Thread(const Thread&&) = delete;//禁止移动构造,避免线程安全的问题
    Thread& operator=(const Thread&) = delete;

    static void* run(void* arg);
private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;//就是线程的抽象，api函数都操作这个变量
    std::function<void()> m_cb;
    std::string m_name;
    Semaphore m_semaphore;//加入信号量

};
}

#endif