#include <exception>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <boost/stacktrace.hpp>
#include <iostream>
#include "Detours-main/src/detours.h"
#include "get_current_exception.h"

#define libc

#ifdef libc
std::string lib = "libc++.dll";
#else
std::string lib = "libstdc++-6.dll";
#endif


using namespace boost::stacktrace;

namespace {
    const size_t kStacktraceDumpSize = 4096;

    std::unordered_map<void*, const char*> stacktrace_dump_by_exc;
    std::mutex mutex;

    using cxa_allocate_exception_t =  void* (*)(size_t);
    using cxa_free_exception_t = void (*)(void*);

    constinit cxa_allocate_exception_t orig_cxa_allocate_exception = nullptr;
    constinit cxa_free_exception_t orig_cxa_free_exception = nullptr;

    void* allocate_exception_with_stacktrace(size_t thrown_size) throw() {
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
        orig_cxa_allocate_exception = (cxa_allocate_exception_t)DetourFindFunction(lib.c_str(), "__cxa_allocate_exception");
        orig_cxa_free_exception = (cxa_free_exception_t)DetourFindFunction(lib.c_str(), "__cxa_free_exception");

        std::cout << "found alloc++:  " << (orig_cxa_allocate_exception != nullptr) << "\n";
        std::cout << "found free++:  " << (orig_cxa_free_exception != nullptr) << "\n";

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)orig_cxa_allocate_exception, &(PVOID&) allocate_exception_with_stacktrace);
        DetourAttach(&(PVOID&)orig_cxa_free_exception, &(PVOID&) free_exception_with_stacktrace);
        DetourTransactionCommit();

        return 1;
    }();

    __attribute__((always_inline)) void* get_current_exception_raw_ptr() {
        auto exc_ptr = std::current_exception();
        void* exc_raw_ptr = *static_cast<void**>((void*)&exc_ptr);
        return exc_raw_ptr;
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