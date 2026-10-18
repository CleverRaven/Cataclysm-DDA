#include <optional>
#include <string>
#include <vector>

#if defined(TILES)
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iterator>

#include <SDL3_ttf/SDL_ttf.h>

#include "cata_scope_helpers.h"
#endif

#include "cata_catch.h"
#include "game_constants.h"
#include "point.h"
#include "terminal_layout.h"

TEST_CASE( "terminal_layout_orientation_follows_window_shape", "[terminal_layout]" )
{
    CHECK( terminal_layout::is_portrait( point( 1080, 2400 ) ) );
    CHECK_FALSE( terminal_layout::is_portrait( point( 2400, 1080 ) ) );
    // square window keeps the landscape terminal
    CHECK_FALSE( terminal_layout::is_portrait( point( 1000, 1000 ) ) );
}

TEST_CASE( "terminal_layout_portrait_reserve", "[terminal_layout]" )
{
    GIVEN( "shortcut strip overlaps the view" ) {
        THEN( "reserve is the percentage of the height" ) {
            CHECK( terminal_layout::portrait_reserved_bottom( 2400, 35, 0 ) == 840 );
            CHECK( terminal_layout::portrait_reserved_bottom( 2400, 0, 0 ) == 0 );
        }
    }
    GIVEN( "shortcut strip is taller than the percentage" ) {
        THEN( "strip wins" ) {
            CHECK( terminal_layout::portrait_reserved_bottom( 2400, 1, 130 ) == 130 );
        }
    }
    GIVEN( "percentage that doesn't divide the height" ) {
        THEN( "reserve rounds down" ) {
            CHECK( terminal_layout::portrait_reserved_bottom( 1001, 33, 0 ) == 330 );
        }
    }
}

TEST_CASE( "terminal_layout_text_size_ids", "[terminal_layout]" )
{
    GIVEN( "preset list" ) {
        THEN( "Terminus bitmap strikes, smallest first" ) {
            CHECK( terminal_layout::text_size_presets() == std::vector<point> {
                point( 6, 12 ), point( 8, 14 ), point( 8, 16 ), point( 10, 18 ), point( 10, 20 ),
                point( 11, 22 ), point( 12, 24 ), point( 14, 28 ), point( 16, 32 )
            } );
        }
        THEN( "every id parses back to its cell" ) {
            for( const point &cell : terminal_layout::text_size_presets() ) {
                CAPTURE( cell );
                CHECK( terminal_layout::parse_text_size( terminal_layout::text_size_id( cell ) )
                       .value_or( point::zero ) == cell );
            }
        }
    }
    WHEN( "id names a preset" ) {
        THEN( "parses to that cell" ) {
            CHECK( terminal_layout::text_size_id( point( 8, 16 ) ) == "8x16" );
            CHECK( terminal_layout::parse_text_size( "10x20" ).value_or( point::zero ) == point( 10, 20 ) );
        }
    }
    WHEN( "id is not a preset" ) {
        THEN( "nothing parses" ) {
            CHECK_FALSE( terminal_layout::parse_text_size( "auto" ).has_value() );
            CHECK_FALSE( terminal_layout::parse_text_size( "custom" ).has_value() );
            CHECK_FALSE( terminal_layout::parse_text_size( "9x18" ).has_value() );
            CHECK_FALSE( terminal_layout::parse_text_size( "8x16 " ).has_value() );
            CHECK_FALSE( terminal_layout::parse_text_size( "" ).has_value() );
        }
    }
}

TEST_CASE( "terminal_layout_custom_text_size", "[terminal_layout]" )
{
    GIVEN( "an even height" ) {
        THEN( "width is half of it" ) {
            CHECK( terminal_layout::custom_text_size( 16 ) == point( 8, 16 ) );
        }
    }
    GIVEN( "odd height" ) {
        THEN( "width rounds up" ) {
            CHECK( terminal_layout::custom_text_size( 13 ) == point( 7, 13 ) );
            CHECK( terminal_layout::custom_text_size( 25 ) == point( 13, 25 ) );
        }
    }
    GIVEN( "a height below one" ) {
        THEN( "cell is one pixel" ) {
            CHECK( terminal_layout::custom_text_size( 0 ) == point::south_east );
            CHECK( terminal_layout::custom_text_size( -5 ) == point::south_east );
        }
    }
}

TEST_CASE( "terminal_layout_auto_text_size", "[terminal_layout]" )
{
    GIVEN( "1080 pixel narrow side" ) {
        THEN( "80 columns of 12x24 fit" ) {
            CHECK( terminal_layout::auto_text_size( 1080 ) == point( 12, 24 ) );
        }
    }
    GIVEN( "narrow side at a preset boundary" ) {
        THEN( "preset that exactly fits is picked" ) {
            CHECK( terminal_layout::auto_text_size( 1120 ) == point( 14, 28 ) );
            CHECK( terminal_layout::auto_text_size( 1119 ) == point( 12, 24 ) );
            CHECK( terminal_layout::auto_text_size( 960 ) == point( 12, 24 ) );
            CHECK( terminal_layout::auto_text_size( 959 ) == point( 11, 22 ) );
        }
    }
    GIVEN( "two same-width presets fit" ) {
        THEN( "the taller one is picked" ) {
            CHECK( terminal_layout::auto_text_size( 640 ) == point( 8, 16 ) );
            CHECK( terminal_layout::auto_text_size( 800 ) == point( 10, 20 ) );
        }
    }
    GIVEN( "very wide narrow side" ) {
        THEN( "the largest preset is picked" ) {
            CHECK( terminal_layout::auto_text_size( 4000 ) == point( 16, 32 ) );
        }
    }
    GIVEN( "a narrow side too small for any preset" ) {
        THEN( "smallest preset is picked" ) {
            CHECK( terminal_layout::auto_text_size( 479 ) == point( 6, 12 ) );
            CHECK( terminal_layout::auto_text_size( 0 ) == point( 6, 12 ) );
        }
    }
}

TEST_CASE( "terminal_layout_sizing_reference", "[terminal_layout]" )
{
    GIVEN( "a rotation with system bars insetting the content" ) {
        // portrait: nav bar takes 53 at the bottom; landscape: status bar takes 72 off the
        // short side, so the content surfaces are not transposes of each other
        const point portrait_window( 1080, 2357 );
        const point landscape_window( 2410, 1008 );
        const point portrait_outer( 1080, 2410 );
        const point landscape_outer( 2410, 1080 );
        THEN( "content short sides differ" ) {
            CHECK( terminal_layout::narrow_side( portrait_window ) !=
                   terminal_layout::narrow_side( landscape_window ) );
        }
        THEN( "outer window gives same reference and text size" ) {
            const int portrait = terminal_layout::sizing_reference( portrait_window, portrait_outer );
            const int landscape = terminal_layout::sizing_reference( landscape_window, landscape_outer );
            CHECK( portrait == 1080 );
            CHECK( landscape == 1080 );
            CHECK( terminal_layout::auto_text_size( portrait ) ==
                   terminal_layout::auto_text_size( landscape ) );
        }
    }
    GIVEN( "split-screen" ) {
        THEN( "the smaller outer window lowers the reference" ) {
            CHECK( terminal_layout::sizing_reference( point( 1080, 1180 ), point( 1080, 1205 ) ) ==
                   1080 );
            CHECK( terminal_layout::sizing_reference( point( 1180, 1080 ), point( 1205, 1080 ) ) ==
                   1080 );
            CHECK( terminal_layout::sizing_reference( point( 700, 1000 ), point( 720, 1000 ) ) == 720 );
        }
    }
    GIVEN( "no outer window report yet" ) {
        THEN( "SDL window is used" ) {
            CHECK( terminal_layout::sizing_reference( point( 2410, 1008 ), point::zero ) == 1008 );
            CHECK( terminal_layout::sizing_reference( point( 2410, 1008 ), point( 0, 1080 ) ) == 1008 );
        }
    }
}

#if defined(TILES)
// preset cells are bitmap strikes of the bundled Terminus, drawn at a point size equal
// to the cell height; check against the font's EBLC strike sizes and SDL_ttf advances
TEST_CASE( "terminal_layout_presets_are_terminus_strikes", "[terminal_layout]" )
{
    const std::string path = "data/font/Terminus.ttf";
    std::ifstream file( path, std::ios::binary );
    REQUIRE( file.good() );
    const std::vector<unsigned char> data( ( std::istreambuf_iterator<char>( file ) ),
                                           std::istreambuf_iterator<char>() );
    const auto u16 = [&]( size_t at ) {
        return static_cast<uint32_t>( data.at( at ) << 8 | data.at( at + 1 ) );
    };
    const auto u32 = [&]( size_t at ) {
        return u16( at ) << 16 | u16( at + 2 );
    };
    // table directory: numTables at 4, records of 16 bytes from 12 (tag, checksum, offset, length)
    size_t eblc = 0;
    for( uint32_t i = 0; i < u16( 4 ); ++i ) {
        const size_t rec = 12 + i * 16;
        if( std::string( data.begin() + rec, data.begin() + rec + 4 ) == "EBLC" ) {
            eblc = u32( rec + 8 );
        }
    }
    REQUIRE( eblc != 0 );
    // EBLC: version (4), numSizes (4), then 48 byte BitmapSize records; ppemY at byte 45
    std::vector<int> strike_heights;
    strike_heights.reserve( u32( eblc + 4 ) );
    for( uint32_t i = 0; i < u32( eblc + 4 ); ++i ) {
        strike_heights.push_back( data.at( eblc + 8 + i * 48 + 45 ) );
    }
    std::sort( strike_heights.begin(), strike_heights.end() );
    std::vector<int> preset_heights;
    for( const point &cell : terminal_layout::text_size_presets() ) {
        preset_heights.push_back( cell.y );
    }
    CHECK( strike_heights == preset_heights );

    // balance our own TTF_Init, so this test can't hide a fixture that forgets to init
    const bool acquired_ttf = TTF_WasInit() == 0;
    REQUIRE( ( !acquired_ttf || TTF_Init() ) );
    on_out_of_scope release_ttf( [acquired_ttf]() {
        if( acquired_ttf ) {
            TTF_Quit();
        }
    } );
    for( const point &cell : terminal_layout::text_size_presets() ) {
        CAPTURE( cell );
        TTF_Font *font = TTF_OpenFont( path.c_str(), static_cast<float>( cell.y ) );
        REQUIRE( font != nullptr );
        int advance = 0;
        CHECK( TTF_GetGlyphMetrics( font, 'M', nullptr, nullptr, nullptr, nullptr, &advance ) );
        CHECK( advance == cell.x );
        CHECK( TTF_GetFontHeight( font ) == cell.y );
        TTF_CloseFont( font );
    }
}
#endif

TEST_CASE( "terminal_layout_resolve_text_size", "[terminal_layout]" )
{
    WHEN( "choice is auto" ) {
        THEN( "narrow side picks the cell" ) {
            CHECK( terminal_layout::resolve_text_size( "auto", 30, 1080 ) == point( 12, 24 ) );
        }
    }
    WHEN( "choice is a preset" ) {
        THEN( "the preset is used whatever the screen" ) {
            CHECK( terminal_layout::resolve_text_size( "8x14", 30, 1080 ) == point( 8, 14 ) );
        }
    }
    WHEN( "choice is custom" ) {
        THEN( "custom height is used" ) {
            CHECK( terminal_layout::resolve_text_size( "custom", 30, 1080 ) == point( 15, 30 ) );
        }
    }
    WHEN( "choice is unknown" ) {
        THEN( "falls back to auto" ) {
            CHECK( terminal_layout::resolve_text_size( "bogus", 30, 1080 ) == point( 12, 24 ) );
        }
    }
}

TEST_CASE( "terminal_layout_fit_grid", "[terminal_layout]" )
{
    const point no_caps;
    const point cell( 12, 24 );
    // 2340x1080 landscape less 130-pixel shortcut strip
    const point landscape( 2340, 950 );
    GIVEN( "a portrait area" ) {
        THEN( "cells fill it" ) {
            CHECK( terminal_layout::fit_grid( point( 1080, 1560 ), cell, no_caps ) == point( 90, 65 ) );
        }
        THEN( "a partial cell is dropped" ) {
            CHECK( terminal_layout::fit_grid( point( 1091, 1583 ), cell, no_caps ) == point( 90, 65 ) );
        }
    }
    GIVEN( "a landscape area" ) {
        THEN( "cells fill it" ) {
            CHECK( terminal_layout::fit_grid( landscape, cell, no_caps ) == point( 195, 39 ) );
        }
        WHEN( "columns are capped" ) {
            THEN( "width stops at the cap" ) {
                CHECK( terminal_layout::fit_grid( landscape, cell, point( 100, 0 ) ) == point( 100, 39 ) );
            }
        }
        WHEN( "rows are capped" ) {
            THEN( "height stops at the cap" ) {
                CHECK( terminal_layout::fit_grid( landscape, cell, point( 0, 30 ) ) == point( 195, 30 ) );
            }
        }
        WHEN( "caps are above what fits" ) {
            THEN( "they change nothing" ) {
                CHECK( terminal_layout::fit_grid( landscape, cell, point( 500, 500 ) ) == point( 195, 39 ) );
            }
        }
        WHEN( "caps are below the minimum terminal" ) {
            THEN( "minimum wins" ) {
                CHECK( terminal_layout::fit_grid( landscape, cell, point( 60, 10 ) ) ==
                       point( EVEN_MINIMUM_TERM_WIDTH, EVEN_MINIMUM_TERM_HEIGHT ) );
            }
        }
    }
    GIVEN( "area too small for 80x24" ) {
        THEN( "grid is the minimum and overflows the area" ) {
            CHECK( terminal_layout::fit_grid( point( 400, 200 ), point( 8, 16 ), no_caps ) ==
                   point( EVEN_MINIMUM_TERM_WIDTH, EVEN_MINIMUM_TERM_HEIGHT ) );
        }
    }
    GIVEN( "an area beyond the maximum terminal" ) {
        THEN( "grid is the maximum" ) {
            CHECK( terminal_layout::fit_grid( point( 100000, 100000 ), point( 6, 12 ), no_caps ) ==
                   point( MAXIMUM_TERM_WIDTH, MAXIMUM_TERM_HEIGHT ) );
        }
    }
    GIVEN( "empty cell or area" ) {
        THEN( "grid is the minimum" ) {
            CHECK( terminal_layout::fit_grid( point( 1080, 1560 ), point( 0, 24 ), no_caps ) ==
                   point( EVEN_MINIMUM_TERM_WIDTH, EVEN_MINIMUM_TERM_HEIGHT ) );
            CHECK( terminal_layout::fit_grid( point( -5, -5 ), point( 8, 16 ), no_caps ) ==
                   point( EVEN_MINIMUM_TERM_WIDTH, EVEN_MINIMUM_TERM_HEIGHT ) );
        }
    }
}

TEST_CASE( "terminal_layout_bitmap_scale", "[terminal_layout]" )
{
    GIVEN( "8-pixel-wide bitmap cell" ) {
        THEN( "zoom is the whole multiple that fits 80 columns" ) {
            CHECK( terminal_layout::bitmap_scale( 1080, 8 ) == 1 );
            CHECK( terminal_layout::bitmap_scale( 1279, 8 ) == 1 );
            CHECK( terminal_layout::bitmap_scale( 1280, 8 ) == 2 );
            CHECK( terminal_layout::bitmap_scale( 2560, 8 ) == 4 );
        }
        THEN( "zoom never drops below one" ) {
            CHECK( terminal_layout::bitmap_scale( 300, 8 ) == 1 );
        }
    }
    GIVEN( "6-pixel-wide bitmap cell" ) {
        THEN( "smaller cells zoom more" ) {
            CHECK( terminal_layout::bitmap_scale( 1080, 6 ) == 2 );
        }
    }
    GIVEN( "empty cell" ) {
        THEN( "zoom is one" ) {
            CHECK( terminal_layout::bitmap_scale( 1080, 0 ) == 1 );
        }
    }
}

TEST_CASE( "terminal_layout_present_size", "[terminal_layout]" )
{
    GIVEN( "a buffer that fits at 1:1" ) {
        THEN( "it's drawn unscaled" ) {
            CHECK( terminal_layout::present_size( point( 1080, 1560 ), point( 1080, 1560 ), 1 ) ==
                   point( 1080, 1560 ) );
            CHECK( terminal_layout::present_size( point( 1080, 1560 ), point( 1090, 1600 ), 1 ) ==
                   point( 1080, 1560 ) );
        }
    }
    GIVEN( "a zoom that fits" ) {
        THEN( "the buffer is drawn at that zoom" ) {
            CHECK( terminal_layout::present_size( point( 640, 384 ), point( 1440, 2000 ), 2 ) ==
                   point( 1280, 768 ) );
        }
    }
    GIVEN( "a zoom too large for the bounds" ) {
        THEN( "largest fitting zoom is used" ) {
            CHECK( terminal_layout::present_size( point( 640, 384 ), point( 1440, 2000 ), 3 ) ==
                   point( 1280, 768 ) );
            CHECK( terminal_layout::present_size( point( 800, 400 ), point( 1200, 1000 ), 2 ) ==
                   point( 800, 400 ) );
        }
    }
    GIVEN( "a zoom below one" ) {
        THEN( "it counts as one" ) {
            CHECK( terminal_layout::present_size( point( 640, 384 ), point( 1440, 2000 ), 0 ) ==
                   point( 640, 384 ) );
        }
    }
    GIVEN( "a buffer larger than the bounds" ) {
        THEN( "a too wide buffer shrinks to the width" ) {
            CHECK( terminal_layout::present_size( point( 640, 384 ), point( 480, 1000 ), 1 ) ==
                   point( 480, 288 ) );
        }
        THEN( "a too tall buffer shrinks to the height" ) {
            CHECK( terminal_layout::present_size( point( 640, 384 ), point( 2000, 192 ), 1 ) ==
                   point( 320, 192 ) );
        }
    }
    GIVEN( "empty buffer or bounds" ) {
        THEN( "nothing is drawn" ) {
            CHECK( terminal_layout::present_size( point( 0, 384 ), point( 1440, 2000 ), 1 ) == point::zero );
            CHECK( terminal_layout::present_size( point( 640, 384 ), point::zero, 1 ) == point::zero );
        }
    }
}
