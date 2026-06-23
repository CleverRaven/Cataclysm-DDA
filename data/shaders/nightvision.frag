// Per-pixel nightvision variant. Float-domain port of
// color_pixel_nightvision.

#version 450

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    float av = (sample_color.r + sample_color.g + sample_color.b) / 3.0;

    // result = min(av * (av*3/4 + 64/255) + 16/255, 1.0)
    float result = min(av * (av * 0.75 + 64.0 / 255.0) + 16.0 / 255.0, 1.0);
    vec3 nv_rgb = vec3(result * 0.25, result, result * 0.125);

    out_color = vec4(nv_rgb * v_vertex_color.rgb,
                     sample_color.a * v_vertex_color.a);
}
