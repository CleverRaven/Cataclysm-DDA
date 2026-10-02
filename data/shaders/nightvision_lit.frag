// Night vision scaled by the smooth lighting light, so night vision keeps
// some of the light falloff it amplifies.

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
    float av = (sample_color.r + sample_color.g + sample_color.b) / 3.0;
    float result = min(av * (av * 0.75 + 64.0 / 255.0) + 16.0 / 255.0, 1.0);
    vec3 nv_rgb = vec3(result * 0.25, result, result * 0.125);

    lit_sample s = sample_light(v_vertex_color);
    float level = max(s.light.r, max(s.light.g, s.light.b));
    nv_rgb *= mix(0.55, 1.0, clamp(level / 0.8, 0.0, 1.0));

    if (u_mode.x < 0) {
        out_color = vec4(nv_rgb * s.visible, sample_color.a);
        return;
    }
    vec3 memory_rgb = memory_preset(u_mode.x, sample_color.rgb);
    out_color = vec4(mix(memory_rgb, nv_rgb, s.visible), sample_color.a);
}
