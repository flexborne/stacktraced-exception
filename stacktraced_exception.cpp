#include <exception>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <boost/stacktrace.hpp>
#include <iostream>
#include "stacktraced_exception.h"

#include <MinHook.h>
#include <windows.h>

#ifdef __GLIBCXX__
const char* lib = "libstdc++-6.dll";
#else
const char* lib = "libc++.dll";
#endif


using namespace boost::stacktrace;

namespace {
    const size_t kStacktraceDumpSize = 4096;

    std::unordered_map<void*, const char*> stacktrace_dump_by_exc;
    std::mutex mutex;

    thread_local bool want_stacktracing = true;

    using cxa_allocate_exception_t =  void* (*)(size_t);
    using cxa_free_exception_t = void (*)(void*);

    constinit cxa_allocate_exception_t orig_cxa_allocate_exception = nullptr;

    constinit cxa_free_exception_t orig_cxa_free_exception = nullptr;

    void* allocate_exception_with_stacktrace(size_t thrown_size) throw() {
        if (!want_stacktracing) {
            return orig_cxa_allocate_exception(thrown_size);
        }
        static thread_local bool already_in_allocate_exception = false;
        if (std::exchange(already_in_allocate_exception, true)) {  // 'bad alloc'
            std::terminate();
        }

        static constexpr size_t kAlign = alignof(std::max_align_t);
        thrown_size = (thrown_size + kAlign - 1) & (~(kAlign - 1));  // round up

        void* user_obj_ptr = orig_cxa_allocate_exception(thrown_size + kStacktraceDumpSize);

        char* stacktrace_dump_ptr = ((char*)user_obj_ptr + thrown_size);

        safe_dump_to(1, stacktrace_dump_ptr, kStacktraceDumpSize);
        {
            std::lock_guard lg{mutex};
            stacktrace_dump_by_exc[user_obj_ptr] = stacktrace_dump_ptr;
        }

        return already_in_allocate_exception = false, user_obj_ptr;
    }

    /// @todo not working with libc++ cause its inlined there
    void free_exception_with_stacktrace(void* thrown_object) throw() {
        if (!want_stacktracing) {
            return orig_cxa_free_exception(thrown_object);
        }
        static thread_local bool already_in_free_exception = false;
        if (std::exchange(already_in_free_exception, true)) {
            std::terminate();
        }

        orig_cxa_free_exception(thrown_object);

        {
            std::lock_guard lg{mutex};
            stacktrace_dump_by_exc.erase(thrown_object);
        }

        already_in_free_exception = false;
    }

    // init
    auto _ = [] () -> int {
        if (auto res = MH_Initialize(); res != MH_OK)
        {
            std::cerr << "Could not init hook. Err: " << res;
            std::terminate();
        }

        HMODULE standard_lib = LoadLibrary(lib);
        if (!standard_lib) {
            std::cerr << "Could not load library";
            return 1;
        }

        // original functions
        auto orig_cxa_allocate_exception_target = (cxa_allocate_exception_t)GetProcAddress(standard_lib, "__cxa_allocate_exception");
        auto orig_cxa_free_exception_target = (cxa_free_exception_t)GetProcAddress(standard_lib, "__cxa_free_exception");

        if (!orig_cxa_allocate_exception_target || !orig_cxa_free_exception_target) {
            std::cerr << "Could not find library functions. Terminating";
            std::terminate();
        }

        auto res = MH_CreateHook(reinterpret_cast<LPVOID>(orig_cxa_allocate_exception_target),
                                   reinterpret_cast<LPVOID>(allocate_exception_with_stacktrace),
                                   reinterpret_cast<LPVOID*>(&orig_cxa_allocate_exception));
        if (res != MH_OK) {
            std::cerr << "Could not create hook for allocate exception. Err: " << res;
            return 1;
        }
        res = MH_EnableHook(reinterpret_cast<LPVOID>(orig_cxa_allocate_exception_target));
        if (res != MH_OK) {
            std::cerr << "Could not enable hook for allocate exception. Err: " << res;
            return 1;
        }
        res = MH_CreateHook(reinterpret_cast<LPVOID>(orig_cxa_free_exception_target),
                                   reinterpret_cast<LPVOID>(free_exception_with_stacktrace),
                                   reinterpret_cast<LPVOID*>(&orig_cxa_free_exception));
        if (res != MH_OK) {
            std::cerr << "Could not create hook for allocate exception. Err: " << res;
            std::terminate();
        }
        res = MH_EnableHook(reinterpret_cast<LPVOID>(orig_cxa_free_exception_target));

        if (res != MH_OK) {
            std::cerr << "Could not enable hook for free exception. Err: " << res;
            std::terminate();
        }

        return 1;
    }();

    __attribute__((always_inline)) void* get_current_exception_raw_ptr() {
        auto exc_ptr = std::current_exception();
        return *static_cast<void**>((void*)&exc_ptr);
    }
}

stacktrace exception::get_current_exception_stacktrace() {
    static const stacktrace empty{0, 0};

    void* exc_raw_ptr = get_current_exception_raw_ptr();
    if (!exc_raw_ptr) {
        return empty;
    }

    const char* stacktrace_dump_ptr;
    {
        std::lock_guard lg{mutex};
        auto it = stacktrace_dump_by_exc.find(exc_raw_ptr);
        if (it == stacktrace_dump_by_exc.end()) {
            return empty;
        }
        stacktrace_dump_ptr = it->second;
    }

    return stacktrace::from_dump(stacktrace_dump_ptr, kStacktraceDumpSize);
}

void exception::capture_stacktraces_at_throw(bool want_stacktracing_new) {
    want_stacktracing = want_stacktracing_new;
}
