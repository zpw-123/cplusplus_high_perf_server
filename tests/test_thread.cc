#include "../sylar/zpw.h"
#include <unistd.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
int count = 0;
//zpw::RWMutex s_mutex;
zpw::Mutex s_mutex;
void fun1() {
    SYLAR_LOG_INFO(g_logger) << "name: " << zpw::Thread::GetName()
                             << " this.name: " << zpw::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id: " << zpw::Thread::GetThis()->getId();
    //sleep(1);
    for(int i = 0; i < 100000; ++i) {
        //zpw::RWMutex::WriteLock lock(s_mutex);//不需要解锁是因为，锁是一个对象，离开作用域就会销毁 同时解锁!!!!
        zpw::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        SYLAR_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv) {
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/zpw/cplusplus_high_perf_server/bin/conf/log2.yml");
    sylar::Config::LoadFromYaml(root);
    std::vector<zpw::Thread::ptr> thrs;
    for(int i = 0; i < 2; ++i) {
        zpw::Thread::ptr thr(new zpw::Thread(&fun2, "name_" + std::to_string(i * 2)));
        zpw::Thread::ptr thr2(new zpw::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;
    return 0;
}