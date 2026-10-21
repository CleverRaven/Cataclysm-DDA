// what the smooth lighting shaders share: the light map the CPU fills each
// frame, the uniform block and the texel decoding. smooth_lighting.h holds the
// C++ side of this contract.
//
// light texels, left of u_size.w: r light, 0 at the vision threshold to 1 at
// full light; g and b red and green shares of the illumination color; a flag
// bits, see u_flags. reach texels, from u_size.w on: the 25 bit mask of cells
// within two of a cell its light may be filtered from, little endian.
//
// define LIGHT_BINDING before including this to bind the light map to another
// sampler slot

#ifndef LIGHT_BINDING
#define LIGHT_BINDING 1
#endif

layout(set = 2, binding = LIGHT_BINDING) uniform sampler2D u_light;

layout(set = 3, binding = 0) uniform lit_params {
    // x, y: light map size in texels; z: rows per z level; w: first reach column
    ivec4 u_size;
    // x: memory look, 0 to 3 the named presets, u_flags.z custom;
    // y: 1 to take each tile's own light rather than filter;
    // z: 1 to fade out of sight into the memory look, 0 into darkness;
    // w: 1 for iso
    ivec4 u_mode;
    // x: flag of a tile seen in detail; y: flag of a light barrier;
    // z: memory look id of the custom preset
    ivec4 u_flags;
    // x: brightness at the vision threshold; y: standing marker; z: light
    // from which sprites keep their full color; w: share of night vision's
    // look low light keeps
    vec4 u_tone;
    // custom memory look: rgb dark color, w gamma; rgb light color
    vec4 u_custom_dark;
    vec4 u_custom_light;
    // x: light over which night vision hands over to the overexposed look;
    // y: how far a fully colored light mixes a pixel toward its color
    vec4 u_look;
    // prefilter layout, see smooth_lighting::prefilter_layout: x, y first
    // cell; z: cells per level; w: first level
    ivec4 u_prefilter;
    // x: grid steps per cell side; y: cells per level row; z: levels; w: 1
    // for the manual lookup
    ivec4 u_prefilter_grid;
};

// cells the cubic B-spline reads each side of the sample point, and the side
// of the square the reach masks cover
const int FILTER_REACH = 2;
const int FILTER_WINDOW = 2 * FILTER_REACH + 1;
// sight edge fades over this band of the in-sight fraction
const float SIGHT_EDGE_START = 0.3;
const float SIGHT_EDGE_END = 0.7;
// filter weights below this count as nothing admitted
const float MIN_WEIGHT = 1.0e-4;

struct lit_texel {
    bool valid;
    float light;
    vec3 chroma;
    bool detail;
    bool barrier;
};

// light texel at `cell`, empty outside the light columns or the rows of
// z level `level`: a sample never leaves its own level
lit_texel fetch_light(ivec2 cell, int level)
{
    lit_texel o;
    o.valid = false;
    o.light = 0.0;
    o.chroma = vec3(0.0);
    o.detail = false;
    o.barrier = false;
    int top = level * u_size.z;
    if (cell.x < 0 || cell.x >= u_size.w || cell.y < top || cell.y >= top + u_size.z) {
        return o;
    }
    vec4 t = texelFetch(u_light, cell, 0);
    int flags = int(t.a * 255.0 + 0.5);
    o.valid = true;
    o.light = t.r;
    o.chroma = vec3(t.g, t.b, max(1.0 - t.g - t.b, 0.0));
    o.detail = (flags & u_flags.x) != 0;
    o.barrier = (flags & u_flags.y) != 0;
    return o;
}
