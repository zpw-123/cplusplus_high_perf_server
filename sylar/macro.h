#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"

//使用BacktraceToString 获取当前线程的调用堆栈信息(类似bt)，并且打印出来
#define SYLAR_ASSERT(x) \
    if(!(x)) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#define SYLAR_ASSERT2(x, w) \
    if(!(x)) { \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }

#endif