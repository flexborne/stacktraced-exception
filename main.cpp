#include <iostream>


#include <unordered_map>
#include "get_current_exception.h"

int main() {
    try {
        //throw std::runtime_error("err)");

        std::unordered_map<int, int> a;
        a.at(10);
    } catch(const std::exception& ex) {
        std::cout << "caught " << ex.what() << "\n";
        std::cout << exception::get_current_exception_stacktrace() << "\n";
    }
    return 0;
}
