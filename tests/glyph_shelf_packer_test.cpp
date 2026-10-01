#include <optional>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "glyph_shelf_packer.h"
#include "point.h"

TEST_CASE( "next_power_of_two_rounds_up", "[sdl_font]" )
{
    const std::vector<std::pair<int, int>> cases = {
        { 1, 1 }, { 2, 2 }, { 3, 4 }, { 16, 16 }, { 17, 32 }, { 100, 128 }, { 512, 512 }
    };
    for( const std::pair<int, int> &c : cases ) {
        CAPTURE( c.first );
        CHECK( next_power_of_two( c.first ) == c.second );
    }
}

TEST_CASE( "glyph_shelf_packer_fills_rows_then_pages", "[sdl_font]" )
{
    GIVEN( "64 texel pages with 16 texel rows and 1 texel gap" ) {
        glyph_shelf_packer packer( 64, 16, 1 );
        WHEN( "two 8 texel cells are placed" ) {
            const std::optional<glyph_shelf_packer::slot> first = packer.place( 8 );
            const std::optional<glyph_shelf_packer::slot> second = packer.place( 8 );
            THEN( "side by side past the gap on page 0" ) {
                REQUIRE( first );
                REQUIRE( second );
                CHECK( first->page == 0 );
                CHECK( first->pos.x == 0 );
                CHECK( first->pos.y == 0 );
                CHECK( second->pos.x == 9 );
                CHECK( packer.page_width( 0 ) == 64 );
            }
        }
        WHEN( "a row of seven cells is full" ) {
            for( int i = 0; i < 7; ++i ) {
                CAPTURE( i );
                const std::optional<glyph_shelf_packer::slot> s = packer.place( 8 );
                REQUIRE( s );
                CHECK( s->pos.x == 9 * i );
            }
            const std::optional<glyph_shelf_packer::slot> next = packer.place( 8 );
            THEN( "eighth starts the next row" ) {
                REQUIRE( next );
                CHECK( next->pos.x == 0 );
                CHECK( next->pos.y == 17 );
                CHECK( next->page == 0 );
            }
        }
        WHEN( "three rows are full" ) {
            for( int i = 0; i < 21; ++i ) {
                CAPTURE( i );
                REQUIRE( packer.place( 8 ) );
            }
            const std::optional<glyph_shelf_packer::slot> next = packer.place( 8 );
            THEN( "next cell opens page 1 at its origin" ) {
                REQUIRE( next );
                CHECK( next->page == 1 );
                CHECK( next->pos.x == 0 );
                CHECK( next->pos.y == 0 );
                CHECK( packer.page_count() == 2 );
            }
        }
        WHEN( "a cell wider than a page is placed between regular cells" ) {
            REQUIRE( packer.place( 8 ) );
            const std::optional<glyph_shelf_packer::slot> wide = packer.place( 100 );
            const std::optional<glyph_shelf_packer::slot> after = packer.place( 8 );
            THEN( "it gets its own power of two page and the shelf continues" ) {
                REQUIRE( wide );
                REQUIRE( after );
                CHECK( wide->page == 1 );
                CHECK( packer.page_width( 1 ) == 128 );
                CHECK( packer.page_height( 1 ) == 16 );
                CHECK( after->page == 0 );
                CHECK( after->pos.x == 9 );
            }
        }
        WHEN( "a zero width cell is placed" ) {
            THEN( "no slot" ) {
                CHECK_FALSE( packer.place( 0 ) );
            }
        }
        WHEN( "a slot planned twice without a commit" ) {
            REQUIRE( packer.place( 8 ) );
            const std::optional<glyph_shelf_packer::placement> a = packer.plan( 8 );
            const std::optional<glyph_shelf_packer::placement> b = packer.plan( 8 );
            THEN( "both plans name the same free slot, nothing is taken" ) {
                REQUIRE( a );
                REQUIRE( b );
                CHECK( a->at.pos.x == 9 );
                CHECK( b->at.pos.x == 9 );
                CHECK( packer.page_count() == 1 );
            }
            AND_WHEN( "one plan is committed" ) {
                packer.commit( *a );
                const std::optional<glyph_shelf_packer::slot> next = packer.place( 8 );
                THEN( "next cell goes after it" ) {
                    REQUIRE( next );
                    CHECK( next->pos.x == 18 );
                }
            }
        }
        WHEN( "first cell's plan names a page that doesn't exist yet" ) {
            const std::optional<glyph_shelf_packer::placement> p = packer.plan( 8 );
            THEN( "it carries the new page's size and opens nothing until committed" ) {
                REQUIRE( p );
                CHECK( p->opens_page );
                CHECK( p->page_w == 64 );
                CHECK( p->page_h == 64 );
                CHECK( packer.page_count() == 0 );
            }
        }
        WHEN( "packer is cleared" ) {
            REQUIRE( packer.place( 8 ) );
            packer.clear();
            const std::optional<glyph_shelf_packer::slot> s = packer.place( 8 );
            THEN( "placement restarts on page 0" ) {
                REQUIRE( s );
                CHECK( s->page == 0 );
                CHECK( s->pos.x == 0 );
                CHECK( packer.page_count() == 1 );
            }
        }
    }
}
