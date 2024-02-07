#include "fiber.h"
#include "config.h"
#include "macro.h"
#include <atomic>

namespace zpw {

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};//原子量来统计协程数量

static thread_local Fiber* t_fiber = nullptr;//线程局部变量，每个线程都有一个t_fiber, 当前协程或者主协程
static thread_local std::shared_ptr<Fiber::ptr> t_threadFiber = nullptr;//主协程

//协程栈大小
static sylar::ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    sylar::Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

//负责创建和释放协程运行栈
class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    m_state = EXEC;//主协程构建
    SetThis(this);

    if(getcontext(&m_ctx)) {//获取当前上下文,保存到m_ctx
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize)
    :m_id(++s_fiber_id)
    ,m_cb(cb) {
    ++s_fiber_count;//子协程的构建
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;//关联是我结束 自动回到上一个上下文
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);//设置函数指针和堆栈 到对应保存context的寄存器中；设置m_ctx的上下文执行函数MainFunc
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {//子协程
        SYLAR_ASSERT(m_state == TERM
                || m_state == INIT);

        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {//主协程
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXEC);//主协程状态一直执行，不会有其他变化

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }
}

//重置协程函数，并重置状态
//INIT，TERM
void reset(std::function<void()> cb);
//切换到当前协程执行
void swapIn();
//切换到后台执行
void swapOut();
//设置当前协程
void SetThis(Fiber* f);
//返回当前协程
static Fiber::ptr GetThis();
//协程切换到后台，并且设置为Ready状态
static void YieldToReady();
//协程切换到后台，并且设置为Hold状态
static void YieldToHold();
//总协程数
static uint64_t TotalFibers();
static MainFunc();


}