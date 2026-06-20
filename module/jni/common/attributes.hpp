#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define ZSC_LIKELY(value) __builtin_expect(!!(value), 1)
#define ZSC_UNLIKELY(value) __builtin_expect(!!(value), 0)
#define ZSC_HOT __attribute__((hot))
#define ZSC_COLD __attribute__((cold, noinline))
#define ZSC_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define ZSC_LIKELY(value) (value)
#define ZSC_UNLIKELY(value) (value)
#define ZSC_HOT
#define ZSC_COLD
#define ZSC_ALWAYS_INLINE inline
#endif
