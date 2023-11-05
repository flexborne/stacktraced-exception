#pragma once

#include <boost/stacktrace.hpp>

namespace exception {
    extern boost::stacktrace::stacktrace get_current_exception_stacktrace();
}  // namespace sfe