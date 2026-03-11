/**
 * Benchmark: int[32e6][2] (contiguous) vs vector<vector<int>>(32e6, vector<int>(2))
 * Measures allocation time and memory usage.
 * On Windows uses QueryPerformanceCounter for high-resolution timing.
 */

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#else
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#endif

namespace {

constexpr size_t ROWS = 32ULL * 1000000;  // 32e6
constexpr size_t COLS = 2;

#if defined(_WIN32) || defined(_WIN64)
// Query Performance Counter: alta resolución en Windows
inline long long qpc_now() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

inline double qpc_elapsed_ms(long long start_ticks, long long end_ticks) {
    static LARGE_INTEGER freq = []() {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();
    return (end_ticks - start_ticks) * 1000.0 / static_cast<double>(freq.QuadPart);
}
#endif

struct MemoryUsage {
    double working_set_mb = -1.0;   // RAM en uso
    double private_bytes_mb = -1.0; // Bytes privados comprometidos
};

MemoryUsage get_current_memory() {
    MemoryUsage out;
#if defined(_WIN32) || defined(_WIN64)
    PROCESS_MEMORY_COUNTERS_EX pmc = {};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        out.working_set_mb = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
        out.private_bytes_mb = static_cast<double>(pmc.PrivateUsage) / (1024.0 * 1024.0);
    }
#else
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.compare(0, 6, "VmRSS:") == 0) {
            std::istringstream iss(line.substr(6));
            long kb;
            iss >> kb;
            out.working_set_mb = kb / 1024.0;
        } else if (line.compare(0, 10, "VmSize:") == 0) {
            std::istringstream iss(line.substr(10));
            long kb;
            iss >> kb;
            out.private_bytes_mb = kb / 1024.0;
        }
    }
#endif
    return out;
}

void touch_memory(int* p, size_t n) {
    volatile int sink = 0;
    for (size_t i = 0; i < n; ++i) {
        sink += p[i];
    }
    (void)sink;
}

void touch_memory(std::vector<std::vector<int>>& v) {
    volatile int sink = 0;
    for (size_t i = 0; i < v.size(); ++i) {
        for (size_t j = 0; j < v[i].size(); ++j) {
            sink += v[i][j];
        }
    }
    (void)sink;
}

}  // namespace

int main() {
    std::cout << "ROWS = " << ROWS << ", COLS = " << COLS << "\n";
    std::cout << "Theoretical: contiguous " << (ROWS * COLS * sizeof(int)) / (1024 * 1024)
              << " MB for data only.\n\n";

    // ---- Contiguous 2D array: int[32e6][2] (heap) ----
    std::cout << "=== Contiguous array: int[32e6][2] (heap) ===\n";
    MemoryUsage mem_before = get_current_memory();
#if defined(_WIN32) || defined(_WIN64)
    long long t0 = qpc_now();
#else
    auto t0 = std::chrono::high_resolution_clock::now();
#endif

    int (*arr)[2] = new int[ROWS][2];

#if defined(_WIN32) || defined(_WIN64)
    long long t1 = qpc_now();
#else
    auto t1 = std::chrono::high_resolution_clock::now();
#endif
    touch_memory(&arr[0][0], ROWS * COLS);
#if defined(_WIN32) || defined(_WIN64)
    long long t2 = qpc_now();
#else
    auto t2 = std::chrono::high_resolution_clock::now();
#endif
    MemoryUsage mem_after_array = get_current_memory();

#if defined(_WIN32) || defined(_WIN64)
    double alloc_time_ms = qpc_elapsed_ms(t0, t1);
    double touch_time_ms = qpc_elapsed_ms(t1, t2);
#else
    double alloc_time_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    double touch_time_ms =
        std::chrono::duration<double, std::milli>(t2 - t1).count();
#endif
    std::cout << "  Allocation time: " << alloc_time_ms << " ms";
#if defined(_WIN32) || defined(_WIN64)
    std::cout << " (QPC)";
#endif
    std::cout << "\n";
    std::cout << "  Touch time:      " << touch_time_ms << " ms\n";
    std::cout << "  Memory (working set): " << mem_after_array.working_set_mb << " MB (delta: "
              << (mem_after_array.working_set_mb - mem_before.working_set_mb) << " MB)\n";
    std::cout << "  Memory (private):     " << mem_after_array.private_bytes_mb << " MB (delta: "
              << (mem_after_array.private_bytes_mb - mem_before.private_bytes_mb) << " MB)\n\n";

    delete[] arr;

    MemoryUsage mem_after_free = get_current_memory();
    std::cout << "  After delete[] (private): " << mem_after_free.private_bytes_mb << " MB\n\n";

    // ---- vector<vector<int>> ----
    std::cout << "=== vector<vector<int>>(32e6, vector<int>(2)) ===\n";
    mem_before = get_current_memory();
#if defined(_WIN32) || defined(_WIN64)
    t0 = qpc_now();
#else
    t0 = std::chrono::high_resolution_clock::now();
#endif

    std::vector<std::vector<int>> v(ROWS, std::vector<int>(COLS));

#if defined(_WIN32) || defined(_WIN64)
    t1 = qpc_now();
#else
    t1 = std::chrono::high_resolution_clock::now();
#endif
    touch_memory(v);
#if defined(_WIN32) || defined(_WIN64)
    t2 = qpc_now();
#else
    t2 = std::chrono::high_resolution_clock::now();
#endif
    MemoryUsage mem_after_vec = get_current_memory();

#if defined(_WIN32) || defined(_WIN64)
    alloc_time_ms = qpc_elapsed_ms(t0, t1);
    touch_time_ms = qpc_elapsed_ms(t1, t2);
#else
    alloc_time_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    touch_time_ms =
        std::chrono::duration<double, std::milli>(t2 - t1).count();
#endif
    std::cout << "  Allocation time: " << alloc_time_ms << " ms";
#if defined(_WIN32) || defined(_WIN64)
    std::cout << " (QPC)";
#endif
    std::cout << "\n";
    std::cout << "  Touch time:      " << touch_time_ms << " ms\n";
    std::cout << "  Memory (working set): " << mem_after_vec.working_set_mb << " MB (delta: "
              << (mem_after_vec.working_set_mb - mem_before.working_set_mb) << " MB)\n";
    std::cout << "  Memory (private):     " << mem_after_vec.private_bytes_mb << " MB (delta: "
              << (mem_after_vec.private_bytes_mb - mem_before.private_bytes_mb) << " MB)\n\n";

    // Summary
    std::cout << "=== Resumen ===\n";
    std::cout << "Contiguous array: 1 bloque, cache-friendly, menos memoria.\n";
    std::cout << "vector<vector>:   " << ROWS << " bloques, mas overhead y fragmentacion.\n";
    std::cout << "Memoria teorica (solo datos): " << (ROWS * COLS * sizeof(int)) / (1024 * 1024)
              << " MB. vector<vector> suele usar mas por overhead de cada fila.\n";

    return 0;
}
