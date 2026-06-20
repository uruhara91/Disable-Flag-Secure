#pragma once

#include <android/log.h>
#include <stdint.h>

#ifndef ZSC_INFO_LOGS
#define ZSC_INFO_LOGS 0
#endif

#define ZSC_LOG_TAG "ZygiskSecureCapture"

#if ZSC_INFO_LOGS
#define ZSC_LOGI(fmt, ...) \
    __android_log_print(ANDROID_LOG_INFO, ZSC_LOG_TAG, "[%d] " fmt, \
                        __LINE__ __VA_OPT__(,) __VA_ARGS__)
#else
#define ZSC_LOGI(...) ((void)0)
#endif

#define ZSC_LOGE(fmt, ...) \
    __android_log_print(ANDROID_LOG_ERROR, ZSC_LOG_TAG, "[%d] " fmt, \
                        __LINE__ __VA_OPT__(,) __VA_ARGS__)

#define ZSC_LOGE_ONCE(flag, fmt, ...)                                           \
    do {                                                                         \
        if (__atomic_exchange_n(&(flag), uint32_t{1}, __ATOMIC_RELAXED) == 0) { \
            ZSC_LOGE(fmt __VA_OPT__(,) __VA_ARGS__);                             \
        }                                                                        \
    } while (0)
