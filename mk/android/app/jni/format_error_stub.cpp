// NDK 26 + -fexperimental-library: <format> is usable at compile time but
// format_error's key functions are missing from libc++_shared. Define them.

#include <format>
#include <stdexcept>

namespace std {

format_error::~format_error() noexcept = default;

} // namespace std
