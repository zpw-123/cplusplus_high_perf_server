#include "thread.h"
#include "log.h"
#include "util.h"

namespace zpw {
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";//thread_local 线程局部变量 线程的存储周期，线程结束时销毁

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        //初始化信号量，pshared=0表示信号量在进程内共享, count表示信号量的初始值
        throw std::logic_error("sem_init error");
    }
}
Semaphore::~Semaphore() {
    //销毁信号量
    sem_destroy(&m_semaphore);
}
void Semaphore::wait() {
    //等待信号量，信号量大于0，就减一，否则阻塞
    if(sem_wait(&m_semaphore)) {//信号量为0就会线程阻塞，停止在这里面
        throw std::logic_error("sem_wait error");//这个情况是信号量操作出现了问题，而不是需要重新等待
    }
}
void Semaphore::notify() {
    //获取信号量，信号量加一，让阻塞在sem_wait的线程继续执行
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}



Thread* Thread::GetThis() {
    return t_thread;
}
const std::string& Thread::GetName() {
    return t_thread_name;
}
void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}
Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    m_semaphore.wait();//在出构造函数之前，要等待线程真正的创建完成并且运行起来
}
Thread::~Thread() {//线程分离，线程结束后自动回收资源
    if(m_thread) {
        pthread_detach(m_thread);
    }
}
void Thread::join() {//等待指定线程终止，并且获取线程的返回值
    if(m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}
void* Thread::run(void* arg) {
    //创建的线程，线程的入口函数, 这个地方我们跑到真正的线程里面了
    Thread* thread = (Thread*)arg;
    t_thread = thread;
    t_thread_name = thread->m_name;
    thread->m_id = sylar::GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
    //上面这俩就可以将top里面的 线程的名字改成我们自己设置的名字了


    std::function<void()> cb;
    cb.swap(thread->m_cb);//在安全情况下，传递资源给线程函数，不必担心资源在主线程被释放

    thread->m_semaphore.notify();//这个地方就是通知构造函数，线程已经创建完成了，也就是初始化好了

    cb();//执行cb里面存储的函数
    return 0;

}

}