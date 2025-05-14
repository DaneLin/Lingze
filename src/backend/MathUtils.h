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

        uint32_t prev_width = previous_power_of_two(width);
	    uint32_t prev_height = previous_power_of_two(height);

        while (prev_width > 1 || prev_height > 1)
        {
		    prev_width >>= 1;
		    prev_height >>= 1;
            result++;
        }
        return result;
    }

	static uint32_t get_group_count(uint32_t threadCount, uint32_t localSize)
    {
	    return (threadCount + localSize - 1) / localSize;
    }
    }