// Night vision looks: float-domain ports of color_pixel_nightvision and
// color_pixel_overexposed.

vec3 nightvision_green(float result)
{
    return vec3(result * 0.25, result, result * 0.125);
}

// what night vision shows in low light
vec3 nightvision_rgb(vec3 rgb)
{
    float av = (rgb.r + rgb.g + rgb.b) / 3.0;
    // result = min(av * (av*3/4 + 64/255) + 16/255, 1.0)
    return nightvision_green(min(av * (av * 0.75 + 64.0 / 255.0) + 16.0 / 255.0, 1.0));
}

// what night vision shows in light
vec3 overexposed_rgb(vec3 rgb)
{
    float av = (rgb.r + rgb.g + rgb.b) / 3.0;
    // result = min(64/255 + av * (av/4 + 192/255), 1.0)
    return nightvision_green(min(64.0 / 255.0 + av * (av * 0.25 + 192.0 / 255.0), 1.0));
}
