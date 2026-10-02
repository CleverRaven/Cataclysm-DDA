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

void main()
{
    vec4 sample_color = texture(u_atlas, v_uv);
    lit_sample s = sample_light(v_vertex_color);
    float level = max(s.light.r, max(s.light.g, s.light.b));

    // low light keeps some falloff; lit threshold is 0.8, see
    // smooth_light_brightness
    vec3 night_rgb = nightvision_rgb(sample_color.rgb) * mix(0.55, 1.0, clamp(level / 0.8, 0.0, 1.0));
    vec3 nv_rgb = mix(night_rgb, overexposed_rgb(sample_color.rgb), smoothstep(0.7, 0.9, level));

    if (u_mode.x < 0) {
        out_color = vec4(nv_rgb * s.visible, sample_color.a);
        return;
    }
    vec3 memory_rgb = memory_preset(u_mode.x, sample_color.rgb);
    out_color = vec4(mix(memory_rgb, nv_rgb, s.visible), sample_color.a);
}
