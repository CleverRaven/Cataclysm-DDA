// Per-pixel memory sepia-dark variant.

#version 450
#extension GL_GOOGLE_include_directive : require

#include "memory_presets.glsl"

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    out_color = vec4(memory_sepia_dark(sample_color.rgb) * v_vertex_color.rgb,
                     sample_color.a * v_vertex_color.a);
}
