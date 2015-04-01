#ifndef SCC_ENGINE_COMMON_UTILS_HPP
#define SCC_ENGINE_COMMON_UTILS_HPP

#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace engine {

static inline void raise_last_os_error()
{
    const int err = errno;
    if (err != 0) {
        throw std::system_error(err, std::system_category());
    }
}

}

#endif // SCC_ENGINE_COMMON_UTILS_HPP
