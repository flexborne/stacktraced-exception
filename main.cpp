#include <iostream>
#include <unordered_map>
#include <future>

#include "stacktraced_exception.h"

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
