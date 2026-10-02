// night vision for the smooth lighting modes: the night look in low light,
// turning into the overexposed look as the light passes the lit threshold, so a
// light's edge is a gradient rather than a tile staircase

#version 450
#extension GL_GOOGLE_include_directive : require

#include "memory_presets.glsl"
#include "nightvision_presets.glsl"
#include "lit_sample.glsl"

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

// share of night vision's look that low light keeps
const float NIGHT_FLOOR = 0.55;
// light level over which night vision hands over to the overexposed look; at
// full light classic tiles show the overexposed variant
const float OVEREXPOSE_START = 0.85;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    lit_sample s = sample_light(v_vertex_color);
    vec3 night_rgb = nightvision_rgb(sample_color.rgb) * mix(NIGHT_FLOOR, 1.0, s.light);
    vec3 nv_rgb = mix(night_rgb, overexposed_rgb(sample_color.rgb),
                      smoothstep(OVEREXPOSE_START, 1.0, s.light));
    vec3 unseen_rgb = u_mode.z != 0 ? lit_memory_rgb(sample_color.rgb) : vec3(0.0);
    out_color = vec4(mix(unseen_rgb, nv_rgb, s.visible), sample_color.a);
}
