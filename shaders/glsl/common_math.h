vec3 random_color(uint seed)
{
	// generate random color
	seed = (seed ^ 61u) ^ (seed >> 16u);
	seed *= 9u;
	seed = seed ^ (seed >> 4u);
	seed *= 0x27d4eb2du;
	seed = seed ^ (seed >> 15u);

	// to rgb color
	float r = float((seed & 0xFFu)) / 255.0;
	float g = float((seed >> 8u) & 0xFFu) / 255.0;
	float b = float((seed >> 16u) & 0xFFu) / 255.0;

	return vec3(r, g, b);
}

bool cone_cull(vec3 center, float radius, vec3 cone_axis, float cone_cutoff, vec3 camera_position)
{
	// return false;
	return dot(center - camera_position, cone_axis) >= cone_cutoff * length(center - camera_position) + radius;
}