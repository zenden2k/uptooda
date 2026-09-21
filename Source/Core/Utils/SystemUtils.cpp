#include "SystemUtils.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    #define ENABLE_CPU_FEATURES_TEST
#endif

#ifdef ENABLE_CPU_FEATURES_TEST
    // Useful link:
    // https://code.google.com/p/chromium/codesearch#chromium/src/base/cpu.cc&sq=package:chromium&type=cs
    #ifdef _WIN32
        #include <intrin.h>
        #define __xgetbv _xgetbv
        //  Windows
        #define cpuid(info,x)    __cpuidex(info,x,0)
    #else
        //  GCC Inline Assembly
        void cpuid(int CPUInfo[4], int InfoType){
            __asm__ __volatile__(
                "cpuid":
            "=a" (CPUInfo[0]),
                "=b" (CPUInfo[1]),
                "=c" (CPUInfo[2]),
                "=d" (CPUInfo[3]) :
                "a" (InfoType), "c" (0)
                );
        }
        #if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 4
            static inline unsigned long long __xgetbv(unsigned int index){
            unsigned int eax, edx;
            __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(index));
            return ((unsigned long long)edx << 32) | eax;

            }
        #else
            #define __xgetbv() 0
        #endif
    #endif
    #include <immintrin.h>

    #ifndef _XCR_XFEATURE_ENABLED_MASK
        #define _XCR_XFEATURE_ENABLED_MASK
    #endif
#endif

namespace IuCoreUtils
{

std::string GetCpuFeatures() {
    //  Misc.
    bool HW_MMX = false;
    bool HW_x64 = false;
    bool HW_ABM = false;      // Advanced Bit Manipulation
    bool HW_RDRAND = false;
    bool HW_BMI1 = false;
    bool HW_BMI2 = false;
    bool HW_ADX = false;
    bool HW_PREFETCHWT1 = false;

    //  SIMD: 128-bit
    bool HW_SSE = false;
    bool HW_SSE2 = false;
    bool HW_SSE3 = false;
    bool HW_SSSE3 = false;
    bool HW_SSE41 = false;
    bool HW_SSE42 = false;
    bool HW_SSE4a = false;
    bool HW_AES = false;
    bool HW_SHA = false;

    //  SIMD: 256-bit
    bool HW_AVX = false;
    bool HW_XOP = false;
    bool HW_FMA3 = false;
    bool HW_FMA4 = false;
    bool HW_AVX2 = false;

    //  SIMD: 512-bit
    bool HW_AVX512F = false;    //  AVX512 Foundation
    bool HW_AVX512CD = false;   //  AVX512 Conflict Detection
    bool HW_AVX512PF = false;   //  AVX512 Prefetch
    bool HW_AVX512ER = false;   //  AVX512 Exponential + Reciprocal
    bool HW_AVX512VL = false;   //  AVX512 Vector Length Extensions
    bool HW_AVX512BW = false;   //  AVX512 Byte + Word
    bool HW_AVX512DQ = false;   //  AVX512 Doubleword + Quadword
    bool HW_AVX512IFMA = false; //  AVX512 Integer 52-bit Fused Multiply-Add
    bool HW_AVX512VBMI = false; //  AVX512 Vector Byte Manipulation Instructions
    bool avxSupported = false;

#ifdef ENABLE_CPU_FEATURES_TEST
    int info[4];
    cpuid(info, 0);
    int nIds = info[0];

    cpuid(info, 0x80000000);
    unsigned nExIds = info[0];

    //  Detect Features
    if (nIds >= 0x00000001) {
        cpuid(info, 0x00000001);
        HW_MMX = (info[3] & ((int)1 << 23)) != 0;
        HW_SSE = (info[3] & ((int)1 << 25)) != 0;
        HW_SSE2 = (info[3] & ((int)1 << 26)) != 0;
        HW_SSE3 = (info[2] & ((int)1 << 0)) != 0;

        HW_SSSE3 = (info[2] & ((int)1 << 9)) != 0;
        HW_SSE41 = (info[2] & ((int)1 << 19)) != 0;
        HW_SSE42 = (info[2] & ((int)1 << 20)) != 0;
        HW_AES = (info[2] & ((int)1 << 25)) != 0;

        HW_AVX = (info[2] & ((int)1 << 28)) != 0;
        HW_FMA3 = (info[2] & ((int)1 << 12)) != 0;

        HW_RDRAND = (info[2] & ((int)1 << 30)) != 0;
    }
    if (nIds >= 0x00000007) {
        cpuid(info, 0x00000007);
        HW_AVX2 = (info[1] & ((int)1 << 5)) != 0;

        HW_BMI1 = (info[1] & ((int)1 << 3)) != 0;
        HW_BMI2 = (info[1] & ((int)1 << 8)) != 0;
        HW_ADX = (info[1] & ((int)1 << 19)) != 0;
        HW_SHA = (info[1] & ((int)1 << 29)) != 0;
        HW_PREFETCHWT1 = (info[2] & ((int)1 << 0)) != 0;

        HW_AVX512F = (info[1] & ((int)1 << 16)) != 0;
        HW_AVX512CD = (info[1] & ((int)1 << 28)) != 0;
        HW_AVX512PF = (info[1] & ((int)1 << 26)) != 0;
        HW_AVX512ER = (info[1] & ((int)1 << 27)) != 0;
        HW_AVX512VL = (info[1] & ((int)1 << 31)) != 0;
        HW_AVX512BW = (info[1] & ((int)1 << 30)) != 0;
        HW_AVX512DQ = (info[1] & ((int)1 << 17)) != 0;
        HW_AVX512IFMA = (info[1] & ((int)1 << 21)) != 0;
        HW_AVX512VBMI = (info[2] & ((int)1 << 1)) != 0;
    }
    if (nExIds >= 0x80000001) {
        cpuid(info, 0x80000001);
        HW_x64 = (info[3] & ((int)1 << 29)) != 0;
        HW_ABM = (info[2] & ((int)1 << 5)) != 0;
        HW_SSE4a = (info[2] & ((int)1 << 6)) != 0;
        HW_FMA4 = (info[2] & ((int)1 << 16)) != 0;
        HW_XOP = (info[2] & ((int)1 << 11)) != 0;
    }

    // Checking for AVX requires 3 things:
    // 1) CPUID indicates that the OS uses XSAVE and XRSTORE
    //     instructions (allowing saving YMM registers on context
    //     switch)
    // 2) CPUID indicates support for AVX
    // 3) XGETBV indicates the AVX registers will be saved and
    //     restored on context switch
    //
    // Note that XGETBV is only available on 686 or later CPUs, so
    // the instruction needs to be conditionally run.
    int cpuInfo[4];
    cpuid(cpuInfo, 1);

    bool osUsesXSAVE_XRSTORE = cpuInfo[2] & (1 << 27) || false;
    bool cpuAVXSuport = cpuInfo[2] & (1 << 28) || false;

    if (osUsesXSAVE_XRSTORE && cpuAVXSuport) {
        // Check if the OS will save the YMM registers
        unsigned long long xcrFeatureMask = __xgetbv(/*_XCR_XFEATURE_ENABLED_MASK*/0);
        avxSupported = (xcrFeatureMask & 0x6) || false;
    }
#endif
    std::string res;
    if (HW_SSE2) {
        res += "sse2,";
    }

    if (HW_SSE3) {
        res += "sse3,";
    }

    if (HW_SSSE3){
        res += "ssse3,";
    }
    if (HW_SSE41) {
        res += "sse41,";
    }
    if (HW_SSE42) {
        res += "sse42,";
    }
    if (HW_SSE4a) {
        res += "sse4a,";
    }
    if (HW_AES) {
        res += "aes,";
    }
    if (HW_SHA) {
        res += "sha,";
    }

    if (avxSupported) {
        res += "avx,";
    }
    if (IsOs64Bit()) {
        res += "64bit,";
    }
    return res;
}

}
