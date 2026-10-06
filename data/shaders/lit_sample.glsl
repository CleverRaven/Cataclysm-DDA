// light for the smooth lighting sprite shaders, per pixel: the tile's own
// light, or the filtered light lit_prefilter.frag stored round the pixel.
// smooth_lighting::reference_prefiltered_sample is this file's sample_light in
// C++.
//
// include memory_presets.glsl before this file: lit_memory_rgb calls into it.
//
// vertex colors carry light map coordinates in texels, rows counted over the
// whole stacked texture: xy is where the vertex falls on the ground plane, z
// the column of the sprite's own cell plus the standing marker for a standing
// sprite, w the row of its own cell.

#include "lit_common.glsl"

struct lit_sample {
    // 0 at vision threshold to 1 at full light
    float light;
    // illumination as a channel multiplier, brightest channel 1
    vec3 hue;
    // how far in sight, 0 to 1
    float visible;
};

vec3 chroma_hue(vec3 chroma)
{
    return chroma / max(max(chroma.r, chroma.g), max(chroma.b, MIN_WEIGHT));
}

// `rgb` mixed toward the light's color at its own brightness, or as near it
// as the screen shows; `hue` is 1 - w + w * color, w the light's colored share
vec3 mix_in_hue(vec3 rgb, vec3 hue)
{
    float strength = 1.0 - min(min(hue.r, hue.g), hue.b);
    if (strength <= 0.0) {
        return rgb;
    }
    vec3 color = (hue - 1.0 + strength) / strength;
    // the pixel's brightness in the light's color, as far as that fits on
    // screen: color's brightest channel is 1, so a scale over 1 would clip
    float scale = min((rgb.r + rgb.g + rgb.b) / max(color.r + color.g + color.b, 3.0 * MIN_WEIGHT),
                      1.0);
    return mix(rgb, color * scale, u_look.y * strength);
}

// memory overlay look out-of-sight light fades into
vec3 lit_memory_rgb(vec3 rgb)
{
    if (u_mode.x == u_flags.z) {
        return memory_mixer(rgb, u_custom_dark.rgb, u_custom_light.rgb, u_custom_dark.w);
    }
    return memory_preset(u_mode.x, rgb);
}

// prefiltered light at `local` in light map cell `cell` on `level`; the
// manual lookup clamps its second texel to the block, as
// smooth_lighting::prefilter_lookup_texels does
vec4 prefiltered(ivec2 cell, int level, vec2 local)
{
    // outside the layout no anchor is seen: dark, neutral chroma
    ivec2 at = ivec2(cell.x, cell.y - level * u_size.z) - u_prefilter.xy;
    if (level < u_prefilter.w || level >= u_prefilter.w + u_prefilter_grid.z || at.x < 0 ||
        at.x >= u_prefilter_grid.y || at.y < 0 || at.y >= u_prefilter.z) {
        return vec4(0.0, 1.0 / 3.0, 1.0 / 3.0, 0.0);
    }
    int grid = u_prefilter_grid.x;
    ivec2 origin = ivec2(at.x, (level - u_prefilter.w) * u_prefilter.z + at.y) * (grid + 1);
    vec2 q = vec2(origin) + local * float(grid);
    if (u_prefilter_grid.w == 0) {
        // texel centers sit at half steps, and q stays inside its block
        return texture(u_light, (q + 0.5) / vec2(textureSize(u_light, 0)));
    }
    ivec2 i0 = ivec2(floor(q));
    vec2 w = q - vec2(i0);
    ivec2 i1 = min(i0 + 1, origin + grid);
    return mix(mix(texelFetch(u_light, i0, 0), texelFetch(u_light, ivec2(i1.x, i0.y), 0), w.x),
               mix(texelFetch(u_light, ivec2(i0.x, i1.y), 0), texelFetch(u_light, i1, 0), w.x),
               w.y);
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
    lit_sample s;
    if (u_mode.y != 0) {
        lit_texel own = fetch_light(cell, level);
        s.light = own.light;
        s.hue = chroma_hue(own.chroma);
        s.visible = own.detail ? 1.0 : 0.0;
        return s;
    }
    // light, red, green chroma shares, in_sight
    vec4 r = prefiltered(cell, level, local);
    s.light = r.x;
    s.hue = chroma_hue(vec3(r.y, r.z, max(1.0 - r.y - r.z, 0.0)));
    // sight is per tile, so its edge is a staircase; the half contour of its
    // bilinear over same class cells cuts the corners, and a narrow band around
    // it fades the edge without dimming whole tiles
    s.visible = smoothstep(SIGHT_EDGE_START, SIGHT_EDGE_END, r.w);
    return s;
}
