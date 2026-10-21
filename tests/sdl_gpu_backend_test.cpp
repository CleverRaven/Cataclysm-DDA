#if defined(TILES)

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "cata_tiles.h"
#include "options.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

TEST_CASE( "gpu_backend_hint_follows_the_option_unless_the_environment_sets_one",
           "[tiles][renderer]" )
{
    GIVEN( "option left on automatic" ) {
        THEN( "game sets no GPU driver hint" ) {
            CHECK( gpu_backend_hint( "auto", false ) == std::nullopt );
        }
    }
    GIVEN( "option set to vulkan" ) {
        WHEN( "environment names no GPU driver" ) {
            THEN( "hint asks for vulkan" ) {
                CHECK( gpu_backend_hint( "vulkan", false ) == std::optional<std::string>( "vulkan" ) );
            }
        }
        WHEN( "environment names a GPU driver" ) {
            THEN( "environment wins, game sets no hint" ) {
                CHECK( gpu_backend_hint( "vulkan", true ) == std::nullopt );
            }
        }
    }
}

TEST_CASE( "gpu_backend_choices_start_with_automatic_and_list_sdl_drivers",
           "[tiles][renderer]" )
{
    std::set<std::string> sdl_drivers;
    for( int i = 0; i < GetNumGPUDrivers(); ++i ) {
        sdl_drivers.insert( GetGPUDriverName( i ) );
    }
    const std::vector<options_manager::id_and_option> choices = cata_tiles::build_gpu_backend_list();
    REQUIRE( !choices.empty() );
    CHECK( choices.front().first == "auto" );
    CHECK( choices.size() == sdl_drivers.size() + 1 );
    for( size_t i = 1; i < choices.size(); ++i ) {
        CAPTURE( choices[i].first );
        CHECK( sdl_drivers.count( choices[i].first ) == 1 );
    }
}

#endif // TILES
