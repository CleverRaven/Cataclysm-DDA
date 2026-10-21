// smooth_filtered's prefilter: filter_light at every grid point of every cell
// the prefilter layout holds, drawn into a float target once per light
// change. smooth_lighting::build_prefilter_table is this file in C++.

#version 450
#extension GL_GOOGLE_include_directive : require

// light map is the draw's own texture
#define LIGHT_BINDING 0
#include "lit_common.glsl"
#include "lit_filter.glsl"

// target pixel coordinates; at pixel ( x, y ) the center ( x + 0.5, y + 0.5 )
layout(location = 0) in vec4 v_vertex_color;
layout(location = 1) in vec2 v_uv;
layout(location = 0) out vec4 out_color;

void main()
{
    ivec2 t = ivec2(floor(v_vertex_color.xy));
    int side = u_prefilter_grid.x + 1;
    ivec2 block = t / side;
    ivec2 sub = t - block * side;
    int level = u_prefilter.w + block.y / u_prefilter.z;
    ivec2 own = ivec2(u_prefilter.x + block.x,
                      level * u_size.z + u_prefilter.y + block.y % u_prefilter.z);
    filter_result r = filter_light(own, level, vec2(sub) / float(u_prefilter_grid.x));
    out_color = vec4(r.light, r.chroma.r, r.chroma.g, r.in_sight);
}
