#pragma once

namespace lz
{
constexpr size_t k_max_vertices  = 64;
constexpr size_t k_max_triangles = 124;        // we use 4 bytes to store indices, so the max triangle count is 124 that is divisible by 4
constexpr float  k_cone_weight   = 0.5f;
}        // namespace lz