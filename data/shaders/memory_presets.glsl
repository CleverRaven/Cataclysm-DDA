// Memory map overlay presets: float-domain ports of the color_pixel_* atlas
// functions. Preset numbers follow cata_shader::memory_preset.

// Pure black stays black: the atlas functions short-circuit on r==g==b==0.
vec3 memory_keep_black(vec3 rgb, vec3 result_rgb)
{
    return mix(result_rgb, rgb, step(rgb.r + rgb.g + rgb.b, 0.0));
}

// color_pixel_darken: 85/256 dimming with 1/255 floor per channel
vec3 memory_darken(vec3 rgb)
{
    return memory_keep_black(rgb, max(rgb * (85.0 / 256.0), vec3(1.0 / 255.0)));
}

// color_pixel_mixer: gray level through gamma, mapped between two tints.
vec3 memory_mixer(vec3 rgb, vec3 dark, vec3 light, float gamma)
{
    float av = (rgb.r + rgb.g + rgb.b) / 3.0;
    float p = clamp(pow(av, gamma) * 1.5, 0.0, 1.0);
    return memory_keep_black(rgb, mix(dark, light, p));
}

vec3 memory_sepia_light(vec3 rgb)
{
    return memory_mixer(rgb, vec3(39.0, 23.0, 19.0) / 255.0,
                        vec3(241.0, 220.0, 163.0) / 255.0, 1.6);
}

vec3 memory_sepia_dark(vec3 rgb)
{
    return memory_mixer(rgb, vec3(39.0, 23.0, 19.0) / 255.0,
                        vec3(70.0, 66.0, 60.0) / 255.0, 1.0);
}

vec3 memory_blue_dark(vec3 rgb)
{
    return memory_mixer(rgb, vec3(19.0, 23.0, 39.0) / 255.0,
                        vec3(60.0, 66.0, 70.0) / 255.0, 1.0);
}

vec3 memory_preset(int preset, vec3 rgb)
{
    if (preset == 0) {
        return memory_darken(rgb);
    }
    if (preset == 1) {
        return memory_sepia_light(rgb);
    }
    if (preset == 2) {
        return memory_sepia_dark(rgb);
    }
    return memory_blue_dark(rgb);
}
