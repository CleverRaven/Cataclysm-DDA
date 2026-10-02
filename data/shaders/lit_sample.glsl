// light for the smooth lighting shaders, from the map-space light map the CPU
// fills each frame. smooth_lighting.h holds the C++ side of this contract, and
// smooth_lighting::reference_sample is this file's sample_light in C++.
//
// include memory_presets.glsl before this file: lit_memory_rgb calls into it.
//
// vertex colors carry light map coordinates in texels, rows counted over the
// whole stacked texture: xy is where the vertex falls on the ground plane, z
// the column of the sprite's own cell plus the standing marker for a standing
// sprite, w the row of its own cell.
//
// light texels, left of u_size.w: r light, 0 at the vision threshold to 1 at
// full light; g and b red and green shares of the illumination color; a flag
// bits, see u_flags. reach texels, from u_size.w on: the 25 bit mask of cells
// within two of a cell its light may be filtered from, little endian.

layout(set = 2, binding = 1) uniform sampler2D u_lightmap;

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
    // x: light over which night vision hands over to the overexposed look
    vec4 u_look;
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

struct lit_sample {
    // 0 at vision threshold to 1 at full light
    float light;
    // illumination as a channel multiplier, brightest channel 1
    vec3 hue;
    // how far in sight, 0 to 1
    float visible;
};

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
    vec4 t = texelFetch(u_lightmap, cell, 0);
    int flags = int(t.a * 255.0 + 0.5);
    o.valid = true;
    o.light = t.r;
    o.chroma = vec3(t.g, t.b, max(1.0 - t.g - t.b, 0.0));
    o.detail = (flags & u_flags.x) != 0;
    o.barrier = (flags & u_flags.y) != 0;
    return o;
}

// reach mask of a cell fetch_light found valid
uint fetch_reach(ivec2 cell)
{
    uvec4 b = uvec4(texelFetch(u_lightmap, ivec2(cell.x + u_size.w, cell.y), 0) * 255.0 + 0.5);
    return b.r | (b.g << 8) | (b.b << 16) | (b.a << 24);
}

bool reaches(uint mask, ivec2 d)
{
    return (mask & (1u << uint((d.y + FILTER_REACH) * FILTER_WINDOW + d.x + FILTER_REACH))) != 0u;
}

// cubic B-spline kernel, zero from two texels out
float bspline(float x)
{
    x = abs(x);
    if (x >= 2.0) {
        return 0.0;
    }
    if (x >= 1.0) {
        float a = 2.0 - x;
        return a * a * a / 6.0;
    }
    return (4.0 - 6.0 * x * x + 3.0 * x * x * x) / 6.0;
}

vec3 chroma_hue(vec3 chroma)
{
    return chroma / max(max(chroma.r, chroma.g), max(chroma.b, MIN_WEIGHT));
}

// memory overlay look out-of-sight light fades into
vec3 lit_memory_rgb(vec3 rgb)
{
    if (u_mode.x == u_flags.z) {
        return memory_mixer(rgb, u_custom_dark.rgb, u_custom_light.rgb, u_custom_dark.w);
    }
    return memory_preset(u_mode.x, rgb);
}

lit_sample sample_light(vec4 coords)
{
    float column = coords.z + 0.5 * u_tone.y;
    ivec2 cell = ivec2(int(floor(column)), int(floor(coords.w + 0.5)));
    bool standing = fract(column) >= u_tone.y;
    int level = cell.y / u_size.z;
    vec2 local = coords.xy - vec2(cell);
    if (standing) {
        // light along the sprite's base line, the same all the way up: the
        // left to right corner diagonal of an iso tile, the middle row of an
        // ortho one. its ground mapping would reach tiles behind it
        if (u_mode.w != 0) {
            local = vec2((local.x + local.y) * 0.5);
        } else {
            local = vec2(local.x, 0.5);
        }
    }
    local = clamp(local, 0.0, 1.0);
    lit_texel own = fetch_light(cell, level);
    lit_sample s;
    if (u_mode.y != 0) {
        s.light = own.light;
        s.hue = chroma_hue(own.chroma);
        s.visible = own.detail ? 1.0 : 0.0;
        return s;
    }
    // texel space: centers at whole numbers
    vec2 st = vec2(cell) + local - 0.5;
    ivec2 base = ivec2(floor(st));
    vec2 f = st - vec2(base);
    lit_texel taps[16];
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            taps[j * 4 + i] = fetch_light(base + ivec2(i - 1, j - 1), level);
        }
    }
    float light_sum = 0.0;
    vec3 chroma_sum = vec3(0.0);
    float blend_weight = 0.0;
    float sight_sum = 0.0;
    float sight_weight = 0.0;
    // 2x2 cells around the sample point that join the own cell through cells of
    // its class: a diagonal walled on both sides is not joined
    bool joined[4];
    for (int k = 0; k < 4; ++k) {
        lit_texel ct = taps[(k / 2 + 1) * 4 + k % 2 + 1];
        joined[k] = ct.valid && ct.barrier == own.barrier;
    }
    int own_k = (cell.y - base.y) * 2 + cell.x - base.x;
    // diagonal joins through either side cell
    joined[3 - own_k] = joined[3 - own_k] && (joined[own_k ^ 1] || joined[own_k ^ 2]);
    // each joined cell filters with its own mask and their results blend
    // bilinearly, so neighbours of one class agree
    for (int cj = 0; cj <= 1; ++cj) {
        for (int ci = 0; ci <= 1; ++ci) {
            if (!joined[cj * 2 + ci]) {
                continue;
            }
            lit_texel ct = taps[(cj + 1) * 4 + ci + 1];
            float bw = (ci == 1 ? f.x : 1.0 - f.x) * (cj == 1 ? f.y : 1.0 - f.y);
            sight_weight += bw;
            if (!ct.detail) {
                continue;
            }
            sight_sum += bw;
            ivec2 c = base + ivec2(ci, cj);
            uint mask = fetch_reach(c);
            float w_sum = 0.0;
            float l_sum = 0.0;
            vec3 ch_sum = vec3(0.0);
            for (int j = -1; j <= 2; ++j) {
                for (int i = -1; i <= 2; ++i) {
                    lit_texel t = taps[(j + 1) * 4 + i + 1];
                    ivec2 tap = base + ivec2(i, j);
                    if (!t.detail || !reaches(mask, tap - c)) {
                        continue;
                    }
                    float w = bspline(st.x - float(tap.x)) * bspline(st.y - float(tap.y));
                    w_sum += w;
                    l_sum += w * t.light;
                    // chroma by filter weight alone: weighting it by light
                    // would give a faint colored cell its full hue
                    ch_sum += w * t.chroma;
                }
            }
            // a detail cell admits itself, at most one cell from the point
            light_sum += bw * l_sum / w_sum;
            chroma_sum += bw * ch_sum / w_sum;
            blend_weight += bw;
        }
    }
    s.light = blend_weight > MIN_WEIGHT ? light_sum / blend_weight : 0.0;
    s.hue = chroma_hue(blend_weight > MIN_WEIGHT ? chroma_sum / blend_weight : vec3(1.0 / 3.0));
    // sight is per tile, so its edge is a staircase; the half contour of its
    // bilinear over same class cells cuts the corners, and a narrow band around
    // it fades the edge without dimming whole tiles
    float in_sight = sight_weight > MIN_WEIGHT ? sight_sum / sight_weight : 0.0;
    s.visible = smoothstep(SIGHT_EDGE_START, SIGHT_EDGE_END, in_sight);
    return s;
}
