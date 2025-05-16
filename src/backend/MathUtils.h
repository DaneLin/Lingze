#pragma once
#include <cstdint>

namespace lz::math
{
    static uint32_t previous_power_of_two(uint32_t value)
    {
	    uint32_t r = 1;

	    while (r * 2 < value)
		    r *= 2;

	    return r;
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

	static uint32_t get_group_count(uint32_t threadCount, uint32_t localSize)
    {
	    return (threadCount + localSize - 1) / localSize;
    }
    }