// Sprite shading for the smooth lighting modes.

#version 450
#extension GL_GOOGLE_include_directive : require

#include "memory_presets.glsl"
#include "lit_sample.glsl"

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

// light level from which sprites keep their full color
const float FULL_COLOR_LIGHT = 0.75;
// shade at the vision threshold, full light being 1
const float THRESHOLD_SHADE = 0.35;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    lit_sample s = sample_light(v_vertex_color);
    // dim light drains color just like the shadow variant does outright
    float gray = (sample_color.r + sample_color.g + sample_color.b) / 3.0;
    vec3 lit_rgb = mix(vec3(gray), sample_color.rgb, smoothstep(0.0, FULL_COLOR_LIGHT, s.light)) * s.hue;
    vec3 seen_rgb = lit_rgb * mix(THRESHOLD_SHADE, 1.0, s.light);
    vec3 unseen_rgb = u_mode.z != 0 ? lit_memory_rgb(sample_color.rgb) : vec3(0.0);
    out_color = vec4(mix(unseen_rgb, seen_rgb, s.visible), sample_color.a);
}
