// smooth_filtered's light filter: a cubic B-spline over the 4x4 cells round
// the sample point, each of the 2x2 nearest cells filtering only what its reach
// mask admits, blended bilinearly. smooth_lighting::reference_filter is this
// file's filter_light in C++.
//
// include lit_common.glsl before this file.

// reach mask of a cell fetch_light found valid
uint fetch_reach(ivec2 cell)
{
    uvec4 b = uvec4(texelFetch(u_light, ivec2(cell.x + u_size.w, cell.y), 0) * 255.0 + 0.5);
    return b.r | (b.g << 8) | (b.b << 16) | (b.a << 24);
}

// 0 mask admits nothing
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

struct filter_result {
    float light;
    // weighted chroma shares
    vec3 chroma;
    // before the sight edge smoothstep
    float in_sight;
};

// filtered light of light map cell `own` on `level` at `local` inside it
filter_result filter_light(ivec2 own, int level, vec2 local)
{
    lit_texel own_texel = fetch_light(own, level);
    // texel space: centers at whole numbers
    vec2 st = vec2(own) + local - 0.5;
    ivec2 base = ivec2(floor(st));
    vec2 f = st - vec2(base);
    // kernel weights depend only on the point and the tap, so each axis takes
    // four
    float wx[4];
    float wy[4];
    for (int k = 0; k < 4; ++k) {
        wx[k] = bspline(st.x - float(base.x + k - 1));
        wy[k] = bspline(st.y - float(base.y + k - 1));
    }
    // 2x2 anchors around the sample point: (0,0) (1,0) (0,1) (1,1) from base
    lit_texel c0 = fetch_light(base, level);
    lit_texel c1 = fetch_light(base + ivec2(1, 0), level);
    lit_texel c2 = fetch_light(base + ivec2(0, 1), level);
    lit_texel c3 = fetch_light(base + ivec2(1, 1), level);
    // anchors joined to the own cell through cells of its class
    bool j0 = c0.valid && c0.barrier == own_texel.barrier;
    bool j1 = c1.valid && c1.barrier == own_texel.barrier;
    bool j2 = c2.valid && c2.barrier == own_texel.barrier;
    bool j3 = c3.valid && c3.barrier == own_texel.barrier;
    int own_k = (own.y - base.y) * 2 + own.x - base.x;
    // the diagonal of the own cell joins through either side cell, own_k ^ 1
    // or own_k ^ 2; selects keep this out of an indexed array, which Mali
    // compilers put on the stack
    bool side_x = own_k == 0 ? j1 : own_k == 1 ? j0 : own_k == 2 ? j3 : j2;
    bool side_y = own_k == 0 ? j2 : own_k == 1 ? j3 : own_k == 2 ? j0 : j1;
    bool diagonal_ok = side_x || side_y;
    j0 = j0 && (own_k != 3 || diagonal_ok);
    j1 = j1 && (own_k != 2 || diagonal_ok);
    j2 = j2 && (own_k != 1 || diagonal_ok);
    j3 = j3 && (own_k != 0 || diagonal_ok);
    uint m0 = j0 && c0.detail ? fetch_reach(base) : 0u;
    uint m1 = j1 && c1.detail ? fetch_reach(base + ivec2(1, 0)) : 0u;
    uint m2 = j2 && c2.detail ? fetch_reach(base + ivec2(0, 1)) : 0u;
    uint m3 = j3 && c3.detail ? fetch_reach(base + ivec2(1, 1)) : 0u;
    // one pass over the taps into each anchor's sums: light and chroma, and
    // weight
    vec4 a0 = vec4(0.0);
    vec4 a1 = vec4(0.0);
    vec4 a2 = vec4(0.0);
    vec4 a3 = vec4(0.0);
    float w0 = 0.0;
    float w1 = 0.0;
    float w2 = 0.0;
    float w3 = 0.0;
    for (int j = -1; j <= 2; ++j) {
        for (int i = -1; i <= 2; ++i) {
            lit_texel t = fetch_light(base + ivec2(i, j), level);
            if (!t.detail) {
                continue;
            }
            float w = wx[i + 1] * wy[j + 1];
            // chroma by filter weight alone: weighting it by light would give
            // a faint colored cell its full hue
            vec4 v = w * vec4(t.light, t.chroma);
            ivec2 d = ivec2(i, j);
            if (reaches(m0, d)) {
                a0 += v;
                w0 += w;
            }
            if (reaches(m1, d - ivec2(1, 0))) {
                a1 += v;
                w1 += w;
            }
            if (reaches(m2, d - ivec2(0, 1))) {
                a2 += v;
                w2 += w;
            }
            if (reaches(m3, d - ivec2(1, 1))) {
                a3 += v;
                w3 += w;
            }
        }
    }
    // each joined anchor filters with its own mask and their results blend
    // bilinearly, so neighbours of one class agree; a detail anchor admits
    // itself, so its weight is never zero
    vec4 sum = vec4(0.0);
    float blend_weight = 0.0;
    float sight_sum = 0.0;
    float sight_weight = 0.0;
    float b0 = (1.0 - f.x) * (1.0 - f.y);
    float b1 = f.x * (1.0 - f.y);
    float b2 = (1.0 - f.x) * f.y;
    float b3 = f.x * f.y;
    if (j0) {
        sight_weight += b0;
        if (c0.detail) {
            sight_sum += b0;
            sum += b0 * a0 / w0;
            blend_weight += b0;
        }
    }
    if (j1) {
        sight_weight += b1;
        if (c1.detail) {
            sight_sum += b1;
            sum += b1 * a1 / w1;
            blend_weight += b1;
        }
    }
    if (j2) {
        sight_weight += b2;
        if (c2.detail) {
            sight_sum += b2;
            sum += b2 * a2 / w2;
            blend_weight += b2;
        }
    }
    if (j3) {
        sight_weight += b3;
        if (c3.detail) {
            sight_sum += b3;
            sum += b3 * a3 / w3;
            blend_weight += b3;
        }
    }
    filter_result r;
    r.light = blend_weight > MIN_WEIGHT ? sum.x / blend_weight : 0.0;
    r.chroma = blend_weight > MIN_WEIGHT ? sum.yzw / blend_weight : vec3(1.0 / 3.0);
    r.in_sight = sight_weight > MIN_WEIGHT ? sight_sum / sight_weight : 0.0;
    return r;
}
