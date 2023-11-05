#pragma once

#include <boost/stacktrace.hpp>

namespace exception {
    /// @brief gets stacktrace from the current exception
    boost::stacktrace::stacktrace get_current_exception_stacktrace();

    /// @brief defines whether all stacktraces for the caller thread should be recorded
    void capture_stacktraces_at_throw(bool want_stacktracing);
}