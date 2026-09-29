// Per-pixel grayscale variant. Float-domain port of color_pixel_grayscale;
// integer truncation in the source produces 1-2 LSB drift versus the
// pre-baked atlas output.

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
    float av = (rgb.r + rgb.g + rgb.b) / 3.0;
    float result = max(av * 0.625, 1.0 / 255.0);
    vec3 grayscale_rgb = mix(vec3(result), rgb, black);

    // Vertex color carries the colored-light tint: rgb = hue, a = 1 - strength.
    // The renderer default (1,1,1,1) is the identity.
    float tint_strength = 1.0 - v_vertex_color.a;
    out_color = vec4(mix(grayscale_rgb, v_vertex_color.rgb, tint_strength), sample_color.a);
}
