#include "engine/common/utils.hpp"

namespace engine {

bool is_power_of_two(unsigned int n)
{
    if (n == 0) {
        return false;
    }

    while (!(n & 1)) {
        n >>= 1;
    }
    n >>= 1;
    return !n;
}

unsigned int log2_of_pot(unsigned int n)
{
    return __builtin_ctz(n);
}

}
