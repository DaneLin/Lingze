#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform MipLevelBuilderData
{
  float filter_type;
};

layout(set = 0, binding = 1) uniform sampler2D prev_level_sampler;

layout(location = 0) in vec2 frag_screen_coord;

layout(location = 0) out vec4 out_color;



void main() 
{
  ivec2 offsets[] = ivec2[](ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(1, 1));
  vec4 sum = vec4(0.0f);
  
  if(filter_type < 0.5f)
  {
    for(int i = 0; i < 4; i++)
    {
      sum += texelFetch(prev_level_sampler, ivec2(gl_FragCoord.xy) * 2 + offsets[i], 0);
    }
    out_color = sum / 4.0f;
  }else
  {
    float min_depth = 1e5f;
    float max_depth = -1e5f;
    float total_mass = 0.0f;
    for(int i = 0; i < 4; i++)
    {
      vec4 cur_sample = texelFetch(prev_level_sampler, ivec2(gl_FragCoord.xy) * 2 + offsets[i], 0);
      min_depth = min(min_depth, cur_sample.x);
      max_depth = max(max_depth, cur_sample.y);
      total_mass += (cur_sample.y - cur_sample.x) * cur_sample.z;
    }
    out_color = vec4(min_depth, max_depth, total_mass / (max_depth - min_depth) / 4.0f, 0.0f);
  }
}