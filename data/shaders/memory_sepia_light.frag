// Per-pixel memory sepia-light variant. Float-domain port of
// color_pixel_sepia_light (color_pixel_mixer with sepia tints, gamma 1.6).

#version 450

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

const vec3 COLOR_DARK = vec3(39.0, 23.0, 19.0) / 255.0;
const vec3 COLOR_LIGHT = vec3(241.0, 220.0, 163.0) / 255.0;
const float GAMMA = 1.6;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    vec3 rgb = sample_color.rgb;

    // Preserve pure black: source short-circuits when r==g==b==0.
    float black = step(rgb.r + rgb.g + rgb.b, 0.0);

    float av = (rgb.r + rgb.g + rgb.b) / 3.0;
    float p = clamp(pow(av, GAMMA) * 1.5, 0.0, 1.0);
    vec3 mixed = mix(COLOR_DARK, COLOR_LIGHT, p);
    vec3 result_rgb = mix(mixed, rgb, black);

    out_color = vec4(result_rgb * v_vertex_color.rgb,
                     sample_color.a * v_vertex_color.a);
}
