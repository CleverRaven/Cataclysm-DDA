// Per-pixel memory darken variant. Float-domain port of color_pixel_darken
// (multiplies RGB by 85/256 ~ 1/3).

#version 450

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    vec3 rgb = sample_color.rgb;

    // Preserve pure black: source short-circuits when r==g==b==0.
    float black = step(rgb.r + rgb.g + rgb.b, 0.0);

    // 85/256 dimming with a 1/255 floor on each channel.
    vec3 dim = max(rgb * (85.0 / 256.0), vec3(1.0 / 255.0));
    vec3 result_rgb = mix(dim, rgb, black);

    out_color = vec4(result_rgb * v_vertex_color.rgb,
                     sample_color.a * v_vertex_color.a);
}
