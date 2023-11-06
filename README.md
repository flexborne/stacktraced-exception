# stacktraced-exception
Extended cxa_allocate_exception and cxa_free_exception for Windows 

# Library for extracting stacktrace from exception
An alternative implementation of https://github.com/axolm/libsfe for Windows, without breaking the ABI.

# Motivation
The Standard Library does not provide stacktrace capture at the throw statement, although this information would be very useful for bug fixing and debugging.
https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2370r0.html

# How does it work
Functionality of the Standard Library is extended using detours: the library functions from ABI are redirected to the overridden ones (allocate_exception_with_stacktrace, free_exception_with_stacktrace). 
allocate_exception_with_stacktrace writes the dumped stacktrace near an exception and puts the dump into the map. 
The map is then used in exception::get_current_exception_stacktrace. 
Unfortunately, the ABI-changing method does not provide an overridden function call from libstdc++, but hooking does. Tested for libc++ and libstdc++ (clang and gcc).

# Usage
There are two functions: 
1) void exception::capture_stacktraces_at_throw(bool) - defines whether we want to capture a stacktrace for the caller thread.
2) boost::stacktrace::stacktrace exception::get_current_exception_stacktrace() - retrieves a stacktrace if it was recorded.


# Example
    void throwable() {
        std::unordered_map<int, int> a;
        a.at(10);
    }
    
    int main() {
        std::async([]{
            exception::capture_stacktraces_at_throw(true);
            try {
                throwable();
            } catch(const std::exception& ex) {
                std::cout << "caught from thread " << ex.what() << "\n";
                std::cout << exception::get_current_exception_stacktrace() << "\n";
            }
        });
    
        exception::capture_stacktraces_at_throw(true);
        try {
            throwable();
        } catch(const std::exception& ex) {
            std::cout << "caught " << ex.what() << "\n";
            std::cout << exception::get_current_exception_stacktrace() << "\n";
        }
    
        return 0;
    }


# Deps
1) https://github.com/TsudaKageyu/minhook
2) [boost/stacktrace](https://github.com/boostorg/stacktrace)https://github.com/boostorg/stacktrace
