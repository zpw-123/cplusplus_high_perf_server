#ifndef __FIBER_H__
#define __FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace zpw {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    std::shared_ptr<Fiber> ptr;
    //enable_shared_from_this的作用是可以在类内部获取this指针，而不是通过参数传递

    enum State {
        INIT,//初始化
        HOLD,//暂停
        EXEC,//执行
        TERM,//结束
        READY//可执行
    };
private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0);//cb是协程函数，stacksize是协程栈大小
    ~Fiber();

    //重置协程函数，并重置状态
    //INIT，TERM
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();
public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static zpw::Fiber::ptr GetThis();
    //协程切换到后台，并且设置为Ready状态
    static void YieldToReady();
    //协程切换到后台，并且设置为Hold状态
    static void YieldToHold();
    //总协程数
    static uint64_t TotalFibers();

    static MainFunc();

private:
    uint64_t m_id = 0;//携程id
    uint32_t m_stacksize = 0;//协程栈大小
    State m_state = INIT;//协程状态 

    ucontext_t m_ctx;//上下文
    void* m_stack = nullptr;//协程栈指针

    std::function<void()> m_cb;//协程执行函数
};

}

#endif