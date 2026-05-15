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
#endif // SDL_MAJOR_VERSION >= 3
#endif // TILES
