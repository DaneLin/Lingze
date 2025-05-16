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

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool project_sphere(vec3 C, float r, float znear, float P00, float P11, out vec4 aabb)
{
	if (C.z < r + znear)
		return false;

	vec2 cx = -C.xz;
	vec2 vx = vec2(sqrt(dot(cx, cx) - r * r), r) / length(cx);
	vec2 minx = mat2(vx.x, vx.y, -vx.y, vx.x) * cx;
	vec2 maxx = mat2(vx.x, -vx.y, vx.y, vx.x) * cx;

	vec2 cy = -C.yz;
	vec2 vy = vec2(sqrt(dot(cy, cy) - r * r), r) / length(cy);
	vec2 miny = mat2(vy.x, -vy.y, vy.y, vy.x) * cy;
	vec2 maxy = mat2(vy.x, vy.y, -vy.y, vy.x) * cy;

	aabb = vec4(minx.x / minx.y * P00, miny.x / miny.y * P11, maxx.x / maxx.y * P00, maxy.x / maxy.y * P11) * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f);

	return true;
}