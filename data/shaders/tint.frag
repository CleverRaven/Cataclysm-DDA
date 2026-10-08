// Untransformed sprite with the colored-light tint applied per pixel; identity
// at the default vertex color.

#version 450

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    float tint_strength = 1.0 - v_vertex_color.a;
    out_color = vec4(mix(sample_color.rgb, v_vertex_color.rgb, tint_strength), sample_color.a);
}
