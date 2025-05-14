#pragma once
#include <cstdint>

namespace lz::math
{
    static uint32_t previous_power_of_two(uint32_t value)
    {
        uint32_t result = 1;
        while (result < value)
        {
            result <<= 1;
        }
        return result;
    }

    static uint32_t get_mip_levels(uint32_t width, uint32_t height)
    {
        uint32_t result = 1;
        while (width > 1 || height > 1)
        {
            width >>= 1;
            height >>= 1;
            result++;
        }
        return result;
    }
}