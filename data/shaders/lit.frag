// Sprite shading for the smooth lighting modes.

#version 450
#extension GL_GOOGLE_include_directive : require

#include "memory_presets.glsl"
#include "lit_sample.glsl"

layout(set = 2, binding = 0) uniform sampler2D u_atlas;

layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    lit_sample s = sample_light(v_vertex_color);

    // dim light drains color just like shadow variant does outright
    float level = max(s.light.r, max(s.light.g, s.light.b));
    float gray = (sample_color.r + sample_color.g + sample_color.b) / 3.0;
    float saturation = smoothstep(0.3, 0.85, level);
    vec3 lit_rgb = mix(vec3(gray), sample_color.rgb, saturation) * s.light;

    if (u_mode.x < 0) {
        out_color = vec4(lit_rgb * s.visible, sample_color.a);
        return;
    }
    vec3 memory_rgb = memory_preset(u_mode.x, sample_color.rgb);
    out_color = vec4(mix(memory_rgb, lit_rgb, s.visible), sample_color.a);
}
