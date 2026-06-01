#if defined(TILES)
#include "sdl_wrappers.h"

#if SDL_MAJOR_VERSION >= 3
#include "cata_catch.h"
#include "cata_shader.h"
#include "cata_tiles.h"
#include "lightmap.h"

TEST_CASE( "compute_variant_kind_dispatch_table", "[tiles][gpu]" )
{
    using cata_shader::variant_kind;

    SECTION( "memorized always maps to MEMORY regardless of nightvision" ) {
        CHECK( compute_variant_kind( lit_level::MEMORIZED, false ) == variant_kind::MEMORY );
        CHECK( compute_variant_kind( lit_level::MEMORIZED, true ) == variant_kind::MEMORY );
    }

    SECTION( "nightvision active picks NIGHT at LOW, OVEREXPOSED otherwise" ) {
        CHECK( compute_variant_kind( lit_level::LOW, true ) == variant_kind::NIGHT );
        CHECK( compute_variant_kind( lit_level::DARK, true ) == variant_kind::OVEREXPOSED );
        CHECK( compute_variant_kind( lit_level::BRIGHT_ONLY, true ) == variant_kind::OVEREXPOSED );
        CHECK( compute_variant_kind( lit_level::LIT, true ) == variant_kind::OVEREXPOSED );
        CHECK( compute_variant_kind( lit_level::BRIGHT, true ) == variant_kind::OVEREXPOSED );
        CHECK( compute_variant_kind( lit_level::BLANK, true ) == variant_kind::OVEREXPOSED );
    }

    SECTION( "no nightvision: LOW is SHADOW, everything else NORMAL" ) {
        CHECK( compute_variant_kind( lit_level::LOW, false ) == variant_kind::SHADOW );
        CHECK( compute_variant_kind( lit_level::DARK, false ) == variant_kind::NORMAL );
        CHECK( compute_variant_kind( lit_level::BRIGHT_ONLY, false ) == variant_kind::NORMAL );
        CHECK( compute_variant_kind( lit_level::LIT, false ) == variant_kind::NORMAL );
        CHECK( compute_variant_kind( lit_level::BRIGHT, false ) == variant_kind::NORMAL );
        CHECK( compute_variant_kind( lit_level::BLANK, false ) == variant_kind::NORMAL );
    }
}

TEST_CASE( "memory_preset_from_option_value_mapping", "[tiles][gpu]" )
{
    using cata_shader::memory_preset;
    using cata_shader::memory_preset_from_option_value;

    SECTION( "named MEMORY_MAP_MODE values map to their preset" ) {
        CHECK( memory_preset_from_option_value( "color_pixel_darken" )
               == memory_preset::DARKEN );
        CHECK( memory_preset_from_option_value( "color_pixel_sepia_light" )
               == memory_preset::SEPIA_LIGHT );
        CHECK( memory_preset_from_option_value( "color_pixel_sepia_dark" )
               == memory_preset::SEPIA_DARK );
        CHECK( memory_preset_from_option_value( "color_pixel_blue_dark" )
               == memory_preset::BLUE_DARK );
    }

    SECTION( "custom and unknown values map to nullopt" ) {
        CHECK_FALSE( memory_preset_from_option_value( "color_pixel_custom" ).has_value() );
        CHECK_FALSE( memory_preset_from_option_value( "" ).has_value() );
        CHECK_FALSE( memory_preset_from_option_value( "garbage" ).has_value() );
    }
}
#endif // SDL_MAJOR_VERSION >= 3
#endif // TILES
