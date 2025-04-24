#pragma once

#include <string>

namespace lz
{
	namespace Colors
	{
		//https://flatuicolors.com/palette/defo
#define RGBA_LE(col) (((col & 0xff000000) >> (3 * 8)) + ((col & 0x00ff0000) >> (1 * 8)) + ((col & 0x0000ff00) << (1 * 8)) + ((col & 0x000000ff) << (3 * 8)))
		static constexpr uint32_t turqoise = RGBA_LE(0x1abc9cffu);
		static constexpr uint32_t greenSea = RGBA_LE(0x16a085ffu);

		static constexpr uint32_t emerald = RGBA_LE(0x2ecc71ffu);
		static constexpr uint32_t nephritis = RGBA_LE(0x27ae60ffu);

		static constexpr uint32_t peter_river = RGBA_LE(0x3498dbffu); //blue
		static constexpr uint32_t belize_hole = RGBA_LE(0x2980b9ffu);

		static constexpr uint32_t amethyst = RGBA_LE(0x9b59b6ffu);
		static constexpr uint32_t wisteria = RGBA_LE(0x8e44adffu);

		static constexpr uint32_t sun_flower = RGBA_LE(0xf1c40fffu);
		static constexpr uint32_t orange = RGBA_LE(0xf39c12ffu);

		static constexpr uint32_t carrot = RGBA_LE(0xe67e22ffu);
		static constexpr uint32_t pumpkin = RGBA_LE(0xd35400ffu);

		static constexpr uint32_t alizarin = RGBA_LE(0xe74c3cffu);
		static constexpr uint32_t pomegranate = RGBA_LE(0xc0392bffu);

		static constexpr uint32_t clouds = RGBA_LE(0xecf0f1ffu);
		static constexpr uint32_t silver = RGBA_LE(0xbdc3c7ffu);
		static constexpr uint32_t imgui_text = RGBA_LE(0xF2F5FAFFu);
	}

	struct ProfilerTask
	{
		double start_time;
		double end_time;
		std::string name;
		uint32_t color;

		double get_length() const
		{
			return end_time - start_time;
		}
	};
}
