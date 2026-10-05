#pragma once
/** System information banner for benchmark output (OS, compiler, CPU, cores) */
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // windows.h min/max macros break std::min/std::max in any header included after this one
#endif
#include <windows.h>
#elif defined(__linux__)
#include <fstream>
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

namespace {

inline void printSystemInfo()
{
    std::cout << "== System Info ==" << std::endl;

    // OS
#ifdef _WIN32
    std::cout << "OS: Windows";
    OSVERSIONINFOEXW osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    std::cout << std::endl;
#elif defined(__APPLE__)
    std::cout << "OS: macOS" << std::endl;
#elif defined(__linux__)
    std::cout << "OS: Linux" << std::endl;
#endif

    // Compiler
#if defined(_MSC_VER)
    std::cout << "Compiler: MSVC " << _MSC_VER << std::endl;
#elif defined(__clang__)
    std::cout << "Compiler: Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__ << std::endl;
#elif defined(__GNUC__)
    std::cout << "Compiler: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << std::endl;
#endif

    // CPU
#ifdef _WIN32
    {
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        // Processor name from registry
        char cpuName[256] = "Unknown";
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD size = sizeof(cpuName);
            RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr,
                reinterpret_cast<LPBYTE>(cpuName), &size);
            RegCloseKey(hKey);
        }
        std::cout << "CPU: " << cpuName << std::endl;
    }
#elif defined(__linux__)
    {
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                auto pos = line.find(':');
                if (pos != std::string::npos)
                    std::cout << "CPU:" << line.substr(pos + 1) << std::endl;
                break;
            }
        }
    }
#elif defined(__APPLE__)
    {
        char buf[256];
        size_t len = sizeof(buf);
        if (sysctlbyname("machdep.cpu.brand_string", buf, &len, nullptr, 0) == 0)
            std::cout << "CPU: " << buf << std::endl;
    }
#endif

    // Logical cores
    std::cout << "Threads: " << std::thread::hardware_concurrency() << std::endl;

    // RAM
#ifdef _WIN32
    {
        MEMORYSTATUSEX mem{};
        mem.dwLength = sizeof(mem);
        if (GlobalMemoryStatusEx(&mem))
            std::cout << "RAM: " << (mem.ullTotalPhys / (1024 * 1024 * 1024)) << " GB" << std::endl;
    }
#elif defined(__linux__)
    {
        long pages = sysconf(_SC_PHYS_PAGES);
        long pageSize = sysconf(_SC_PAGE_SIZE);
        if (pages > 0 && pageSize > 0)
            std::cout << "RAM: " << ((long long)pages * pageSize / (1024 * 1024 * 1024)) << " GB" << std::endl;
    }
#elif defined(__APPLE__)
    {
        int64_t mem = 0;
        size_t len = sizeof(mem);
        if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) == 0)
            std::cout << "RAM: " << (mem / (1024 * 1024 * 1024)) << " GB" << std::endl;
    }
#endif

    // Build config
#ifdef NDEBUG
    std::cout << "Build: Release" << std::endl;
#else
    std::cout << "Build: Debug" << std::endl;
#endif

    std::cout << "==" << std::endl << std::endl;
}

} // namespace
