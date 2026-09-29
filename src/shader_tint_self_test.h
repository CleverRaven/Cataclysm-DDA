#pragma once
#ifndef CATA_SRC_SHADER_TINT_SELF_TEST_H
#define CATA_SRC_SHADER_TINT_SELF_TEST_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "point.h"
#include "tile_tint.h"

// expected result of the shader tint self-test: five sprites drawn in order from
// one 4x2 source texture into a target cleared to clear_color(). each unrotated
// destination rect is 4x2, each draw has its own tint mod, and the footprints
// are disjoint
namespace shader_tint_self_test
{

struct rgba {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

enum class transform {
    none,
    flip_horizontal,
    // 90 degrees clockwise about the destination center
    rotate_90,
};

struct draw_case {
    char name = ' ';
    // Top-left corner of the unrotated destination rect
    point dst;
    transform xform = transform::none;
    tint_texture_mod mod;
};

constexpr int source_width = 4;
constexpr int source_height = 2;
constexpr size_t texel_count = source_width * source_height;
constexpr int target_size = 32;
constexpr int channel_tolerance = 4;
constexpr size_t case_count = 5;
// one check per case, then one for the uncovered background
constexpr size_t check_count = case_count + 1;
constexpr size_t background_index = case_count;

// row major: distinct opaque colors, one texel at alpha 128 and one transparent
// texel
const std::array<rgba, texel_count> &source_texels();
// in draw order
const std::array<draw_case, case_count> &draw_cases();
rgba clear_color();

struct coverage {
    size_t case_index = 0;
    point texel;
};
// the case whose footprint holds the pixel center, and the source texel it
// samples; nullopt when no footprint holds it
std::optional<coverage> covering_case( const point &px );

// target pixel after every draw: the tint mix of the sampled texel,
// alpha-blended over the clear color. only the RGB channels are meaningful.
rgba expected_pixel( const point &px );

struct mismatch {
    point px;
    rgba expected;
    rgba actual;
};
// for each check, the first pixel in row-major order whose RGB differs from
// expected_pixel by more than channel_tolerance in any channel. pixels holds
// target_size * target_size entries in row-major order
std::array<std::optional<mismatch>, check_count> compare_readback(
    const std::vector<rgba> &pixels );

// "(r, g, b)" for log lines
std::string rgb_string( const rgba &c );

} // namespace shader_tint_self_test

#endif // CATA_SRC_SHADER_TINT_SELF_TEST_H
