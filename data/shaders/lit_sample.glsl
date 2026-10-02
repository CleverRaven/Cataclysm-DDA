// Light for smooth lighting shaders, from a map-space light map sampled per
// pixel. Each vertex color carries map-space texture coordinates: xy is where
// the vertex falls on the ground plane, zw the corner of its own tile's cell.
// Map texels: rgb = light * visible, a = visible, where visible is 1 for a tile
// drawn lit and 0 for one out of sight.

layout(set = 2, binding = 1) uniform sampler2D u_lightmap;

layout(set = 3, binding = 0) uniform lit_params {
    // xy: size of one texel in texture coordinates
    vec4 u_texel;
    // x: memory preset out-of-sight texels blend into, or -1 to fade into
    // darkness; y: 1 to take each tile's own light rather than blend;
    // z: unused; w: 1 for iso
    ivec4 u_mode;
};

// light of the tiles in sight, and how far the pixel is in sight, 0 to 1:
// out of sight it fades into memory or darkness per u_mode.x
struct lit_sample {
    vec3 light;
    float visible;
};

// Cubic B-spline filtering from four bilinear taps. Plain bilinear changes
// slope at every texel center, which reads as a grid across the map; the
// B-spline keeps the slope continuous, at the cost of softening peaks.
vec4 texture_bspline(vec2 uv)
{
    vec2 st = uv / u_texel.xy - 0.5;
    vec2 i = floor(st);
    vec2 f = st - i;
    vec2 f2 = f * f;
    vec2 f3 = f2 * f;
    vec2 w0 = (-f3 + 3.0 * f2 - 3.0 * f + 1.0) / 6.0;
    vec2 w1 = (3.0 * f3 - 6.0 * f2 + 4.0) / 6.0;
    vec2 w2 = (-3.0 * f3 + 3.0 * f2 + 3.0 * f + 1.0) / 6.0;
    vec2 w3 = f3 / 6.0;
    vec2 g0 = w0 + w1;
    vec2 g1 = w2 + w3;
    vec2 p0 = (i - 1.0 + w1 / g0 + 0.5) * u_texel.xy;
    vec2 p1 = (i + 1.0 + w3 / g1 + 0.5) * u_texel.xy;
    return g0.y * (g0.x * texture(u_lightmap, p0) + g1.x * texture(u_lightmap, vec2(p1.x, p0.y))) +
           g1.y * (g0.x * texture(u_lightmap, vec2(p0.x, p1.y)) + g1.x * texture(u_lightmap, p1));
}

lit_sample sample_light(vec4 coords)
{
    // a cell u past 1 marks a standing sprite: a wall, furniture, a creature
    bool standing = coords.z >= 1.0;
    vec2 cell_min = coords.zw - vec2(standing ? 1.0 : 0.0, 0.0);
    vec2 local = (coords.xy - cell_min) / u_texel.xy;
    if (standing) {
        // Light along the sprite's base line, the same all the way up: the
        // left to right corner diagonal of an iso tile, the middle row of
        // an ortho one. Its ground mapping would reach tiles behind it.
        if (u_mode.w != 0) {
            local = vec2(clamp((local.x + local.y) * 0.5, 0.0, 1.0));
        } else {
            local = vec2(clamp(local.x, 0.0, 1.0), 0.5);
        }
    }
    lit_sample s;
    if (u_mode.y != 0) {
        vec4 texel = texture(u_lightmap, cell_min + 0.5 * u_texel.xy);
        s.light = texel.rgb;
        s.visible = texel.a;
        return s;
    }
    vec2 uv = cell_min + clamp(local, 0.0, 1.0) * u_texel.xy;
    // light of the tiles in sight only, so it keeps its level up to the edge
    // of sight
    vec4 smooth_texel = texture_bspline(uv);
    s.light = smooth_texel.a > 0.001 ? smooth_texel.rgb / smooth_texel.a : vec3(0.0);
    // sight is per tile, so its edge is a staircase. bilinear is 1 at the
    // center of each tile in sight and 0 on the rest; its half contour cuts the
    // staircase corners, and a narrow step around that contour keeps the fade
    // to a band at the edge instead of dimming whole tiles
    s.visible = smoothstep(0.3, 0.7, texture(u_lightmap, uv).a);
    return s;
}
