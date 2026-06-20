#pragma once

#include <android/log.h>

#define ZSC_LOG_TAG "ZygiskSecureCapture"
#define ZSC_LOGI(...) __android_log_print(ANDROID_LOG_INFO, ZSC_LOG_TAG, __VA_ARGS__)
#define ZSC_LOGW(...) __android_log_print(ANDROID_LOG_WARN, ZSC_LOG_TAG, __VA_ARGS__)
#define ZSC_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, ZSC_LOG_TAG, __VA_ARGS__)
