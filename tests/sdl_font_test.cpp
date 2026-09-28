#if defined(TILES)

#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "catacharset.h"
#include "cursesdef.h"
#include "cursesport.h"
#include "options_helpers.h"
#include "path_info.h"
#include "point.h"
#include "sdl_renderer_recovery.h"
#include "sdl_utils.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

// Characterization: pins what draw_window paints for TrueType and bitmap fonts.

namespace
{

using palette_t = std::remove_reference_t<decltype( windowsPalette )>;

constexpr int cell_w = 8;
constexpr int cell_h = 16;
constexpr int font_size = 16;
constexpr int win_cols = 8;
constexpr int win_rows = 4;
constexpr int canvas_w = win_cols * cell_w;
constexpr int canvas_h = win_rows * cell_h;
constexpr SDL_Color sentinel = { 0xFF, 0x00, 0xFF, 0xFF };

const std::string cjk_middle = "中";     // two cells wide
const std::string box_horizontal = "─"; // LINE_OXOX
const std::string box_vertical = "│";   // LINE_XOXO

// Unifont glyphs rasterize to opaque/clear pixels only, even blended; Roboto
// has antialiased edges, so blended runs exercise partial alpha
const std::string typeface_unifont = "unifont.ttf";
const std::string typeface_roboto = "Roboto-Medium.ttf";

palette_t test_palette()
{
    palette_t pal{};
    for( size_t i = 0; i < pal.size(); ++i ) {
        pal[i] = SDL_Color{ static_cast<Uint8>( 20 + 14 * i ),
                            static_cast<Uint8>( 230 - 11 * i ),
                            static_cast<Uint8>( ( 60 + 67 * i ) % 256 ), 0xFF };
    }
    return pal;
}

Uint32 rgb_of( const SDL_Color &c )
{
    return ( static_cast<Uint32>( c.r ) << 16 ) | ( static_cast<Uint32>( c.g ) << 8 ) | c.b;
}

struct cell_spec {
    point pos;
    std::string ch;
    int fg;
    int bg;
};

const std::vector<cell_spec> &test_cells()
{
    static const std::vector<cell_spec> cells = {
        { point::zero, "A", 15, 0 },
        { point::east, " ", 7, 4 },
        { point( 2, 0 ), "g", 2, 1 },
        { point( 3, 0 ), cjk_middle, 11, 0 },
        { point( 4, 0 ), "", 11, 0 },
        { point( 5, 0 ), "x", 7, 5 },
        { point( 6, 0 ), box_horizontal, 14, 0 },
        { point( 7, 0 ), box_vertical, 9, 3 },
        { point::south, "@", 12, 6 },
        { point::south_east, "#", 10, 0 },
        { point( 2, 1 ), cjk_middle, 3, 2 },
        { point( 3, 1 ), "", 3, 2 },
        { point( 4, 1 ), box_vertical, 13, 0 },
        { point( 5, 1 ), box_horizontal, 8, 7 },
        { point( 0, 2 ), " ", 7, 9 },
        { point( 1, 2 ), "Q", 1, 14 },
        // stays under the right half of the wide glyph the wrap writes below
        { point( 1, 3 ), "k", 5, 6 },
        { point( 7, 3 ), "z", 4, 0 },
    };
    return cells;
}

void put_cell( const catacurses::window &w, const cell_spec &c )
{
    cata_cursesport::cursecell &cell =
        w.get<cata_cursesport::WINDOW>()->line[c.pos.y].chars[c.pos.x];
    cell.ch = c.ch;
    cell.FG = static_cast<catacurses::base_color>( c.fg );
    cell.BG = static_cast<catacurses::base_color>( c.bg );
}

// wide glyph in the last column wraps: printstring moves it to column 0 of the
// next row and leaves column 1 as is, so that cell draws over the glyph's right
// half
void wrap_wide_glyph( const catacurses::window &w )
{
    catacurses::mvwprintw( w, point( win_cols - 1, 2 ), cjk_middle );
}

// Software canvas composed with SDL's own blitter in draw_window's order.
class oracle_canvas
{
    public:
        oracle_canvas( const std::string &typeface, const palette_t &pal, const bool blending )
            : pal_( pal ), blending_( blending ),
              canvas_( CreateRGBSurface( 0, canvas_w, canvas_h, 32, 0x00FF0000, 0x0000FF00,
                                         0x000000FF, 0xFF000000 ) ),
              font_( OpenFontIndex( typeface.c_str(), font_size, 0 ) ) {
            fill( SDL_Rect{ 0, 0, canvas_w, canvas_h }, sentinel );
        }

        bool ready() const {
            return canvas_ && font_;
        }

        void fill( const SDL_Rect &r, const SDL_Color &c ) {
            FillRect( canvas_, &r, MapRGBA( canvas_, c.r, c.g, c.b, c.a ) );
        }

        // glyph cell rule: each axis centred in the cw x h cell, a larger
        // source cropped from its centre, copied without blending.
        void glyph( const std::string &ch, const point &at, const int fg ) {
            const SDL_Color color = pal_[fg & 0xf];
            SDL_Surface_Ptr src = blending_ ? RenderUTF8_Blended( font_, ch.c_str(), color )
                                  : RenderUTF8_Solid( font_, ch.c_str(), color );
            REQUIRE( src );
            const int cw = cell_w * utf8_width( ch );
            SDL_Surface_Ptr cell = create_surface_32( cw, cell_h );
            SDL_Rect s = { 0, 0, src->w, src->h };
            SDL_Rect d = { 0, 0, cw, cell_h };
            if( s.w < d.w ) {
                d.x = ( d.w - s.w ) / 2;
                d.w = s.w;
            } else if( s.w > d.w ) {
                s.x = ( s.w - d.w ) / 2;
                s.w = d.w;
            }
            if( s.h < d.h ) {
                d.y = ( d.h - s.h ) / 2;
                d.h = s.h;
            } else if( s.h > d.h ) {
                s.y = ( s.h - d.h ) / 2;
                s.h = d.h;
            }
            SetSurfaceBlendMode( src, SDL_BLENDMODE_NONE );
            REQUIRE( BlitSurface( src, &s, cell, &d ) == 0 );
            count_partial_alpha( cell );
            SetSurfaceBlendMode( cell, SDL_BLENDMODE_BLEND );
            SDL_Rect dst = { at.x, at.y, cw, cell_h };
            REQUIRE( BlitSurface( cell, nullptr, canvas_, &dst ) == 0 );
        }

        Uint32 rgb_at( const point &p ) {
            REQUIRE( LockSurface( canvas_ ) == 0 );
            const Uint32 v = static_cast<const Uint32 *>( canvas_->pixels )[
                      p.y * ( canvas_->pitch / 4 ) + p.x];
            UnlockSurface( canvas_ );
            return v & 0x00FFFFFF;
        }

        const palette_t &pal_;
        bool blending_;
        // texels of composed glyph cells with 0 < alpha < 255
        int partial_alpha_texels = 0;

    private:
        void count_partial_alpha( const SDL_Surface_Ptr &cell ) {
            REQUIRE( LockSurface( cell ) == 0 );
            const Uint32 *px = static_cast<const Uint32 *>( cell->pixels );
            for( int y = 0; y < cell->h; ++y ) {
                for( int x = 0; x < cell->w; ++x ) {
                    Uint8 r = 0;
                    Uint8 g = 0;
                    Uint8 b = 0;
                    Uint8 a = 0;
                    GetRGBA( px[y * ( cell->pitch / 4 ) + x], cell, r, g, b, a );
                    partial_alpha_texels += a > 0 && a < 255 ? 1 : 0;
                }
            }
            UnlockSurface( cell );
        }

        SDL_Surface_Ptr canvas_;
        TTF_Font_Ptr font_;
};

// font's box line rects, for the two line glyphs used here.
void oracle_line( oracle_canvas &o, const std::string &ch, const point &at, const int fg )
{
    const SDL_Color c = o.pal_[fg];
    if( ch == box_horizontal ) {
        o.fill( SDL_Rect{ at.x, at.y + ( cell_h - 1 ) / 2, cell_w, 1 }, c );
    } else {
        o.fill( SDL_Rect{ at.x + ( cell_w - 2 ) / 2, at.y, 2, cell_h }, c );
    }
}

// draw_window order: per row a black line clear, then per cell its background
// and then its glyph or line
void compose_expected( oracle_canvas &o, const catacurses::window &w, const bool ascii_routine )
{
    const cata_cursesport::WINDOW *win = w.get<cata_cursesport::WINDOW>();
    for( int j = 0; j < win_rows; ++j ) {
        o.fill( SDL_Rect{ 0, j * cell_h, win_cols * cell_w, cell_h }, o.pal_[0] );
        for( int i = 0; i < win_cols; ++i ) {
            const cata_cursesport::cursecell &cell = win->line[j].chars[i];
            const point at( i * cell_w, j * cell_h );
            if( cell.ch.empty() ) {
                continue;
            }
            if( cell.ch == " " ) {
                if( cell.BG != catacurses::black ) {
                    o.fill( SDL_Rect{ at.x, at.y, cell_w, cell_h }, o.pal_[cell.BG] );
                }
                continue;
            }
            const int cw = utf8_width( cell.ch );
            if( cell.BG != catacurses::black ) {
                o.fill( SDL_Rect{ at.x, at.y, cell_w * cw, cell_h }, o.pal_[cell.BG] );
            }
            const bool is_line = cell.ch == box_horizontal || cell.ch == box_vertical;
            if( is_line && ascii_routine ) {
                oracle_line( o, cell.ch, at, cell.FG );
            } else {
                o.glyph( cell.ch, at, cell.FG );
            }
        }
    }
}

int channel_delta( const Uint32 a, const Uint32 b )
{
    int worst = 0;
    for( int shift = 0; shift <= 16; shift += 8 ) {
        worst = std::max( worst, std::abs( static_cast<int>( ( a >> shift ) & 0xFF ) -
                                           static_cast<int>( ( b >> shift ) & 0xFF ) ) );
    }
    return worst;
}

std::vector<Uint32> read_canvas( const int w, const int h )
{
    std::vector<Uint32> px( static_cast<size_t>( w ) * h );
    const SDL_Rect all = { 0, 0, w, h };
    REQUIRE( RenderReadPixels( get_sdl_renderer(), &all, SDL_PIXELFORMAT_ARGB8888, px.data(),
                               w * 4 ) );
    for( Uint32 &p : px ) {
        p &= 0x00FFFFFF;
    }
    return px;
}

void clear_to_sentinel()
{
    SetRenderDrawColor( get_sdl_renderer(), sentinel.r, sentinel.g, sentinel.b, sentinel.a );
    RenderClear( get_sdl_renderer() );
}

} // namespace

TEST_CASE( "draw_window_matches_ttf_glyph_oracle", "[tiles][sdl_font]" )
{
    const std::string face = GENERATE( typeface_unifont, typeface_roboto );
    const bool blending = GENERATE( false, true );
    const bool ascii_routine = GENERATE( false, true );
    CAPTURE( face, blending, ascii_routine );

    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    restore_on_out_of_scope<palette_t> restore_palette( windowsPalette );
    windowsPalette = test_palette();
    override_option ascii_lines( "USE_DRAW_ASCII_LINES_ROUTINE", ascii_routine ? "true" : "false" );
    const std::string typeface = PATH_INFO::fontdir() + face;

    GIVEN( "8x4 window of colored cells, spaces, wide glyphs, box lines and a wrapped wide glyph" ) {
        REQUIRE( renderer_recovery_test_support::install_test_font( typeface, cell_w, cell_h,
                 font_size, blending ) );
        const catacurses::window w = catacurses::newwin( win_rows, win_cols, point::zero );
        for( const cell_spec &c : test_cells() ) {
            put_cell( w, c );
        }
        wrap_wide_glyph( w );
        const cata_cursesport::WINDOW *win = w.get<cata_cursesport::WINDOW>();
        // layout this case rests on: the writer's wrap branch left the cell
        // under the glyph's right half occupied
        REQUIRE( win->line[3].chars[0].ch == cjk_middle );
        REQUIRE( win->line[3].chars[1].ch == "k" );
        oracle_canvas expected( typeface, windowsPalette, blending );
        REQUIRE( expected.ready() );
        compose_expected( expected, w, ascii_routine );
        if( blending && face == typeface_roboto ) {
            // otherwise blended runs never reach the partial-alpha math
            REQUIRE( expected.partial_alpha_texels > 0 );
        }

        WHEN( "window drawn over a sentinel-cleared target" ) {
            clear_to_sentinel();
            REQUIRE( renderer_recovery_test_support::draw_test_window( w ) );
            const std::vector<Uint32> actual = read_canvas( canvas_w, canvas_h );

            THEN( "every pixel matches the oracle" ) {
                // Solid glyph texels are opaque or clear, so exact. Blended
                // edges go through SDL's blitter here and the renderer's blend
                // in the draw, which may round one step apart.
                const int tolerance = blending ? 1 : 0;
                int mismatches = 0;
                point first_bad = point::north_west;
                Uint32 got = 0;
                Uint32 want = 0;
                for( int y = 0; y < canvas_h; ++y ) {
                    for( int x = 0; x < canvas_w; ++x ) {
                        const Uint32 a = actual[static_cast<size_t>( y ) * canvas_w + x];
                        const Uint32 e = expected.rgb_at( point( x, y ) );
                        if( channel_delta( a, e ) > tolerance ) {
                            if( mismatches == 0 ) {
                                first_bad = point( x, y );
                                got = a;
                                want = e;
                            }
                            ++mismatches;
                        }
                    }
                }
                CAPTURE( first_bad.x, first_bad.y, got, want );
                CHECK( mismatches == 0 );
            }
        }
    }
}

TEST_CASE( "draw_window_paints_a_npot_bitmap_font_glyph", "[tiles][sdl_font]" )
{
    // window format on common desktops; build_textures recolors only texels
    // that read 0xFFFFFF in it
    const Uint32 format = SDL_PIXELFORMAT_XRGB8888;
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    restore_on_out_of_scope<palette_t> restore_palette( windowsPalette );
    windowsPalette = test_palette();
    GIVEN( "6x10 bitmap font from 100x170 image + two A cells" ) {
        REQUIRE( renderer_recovery_test_support::install_test_font(
                     "tests/data/bitmap_font_npot.png", 6, 10, 0, false, format ) );
        const catacurses::window w = catacurses::newwin( 4, 8, point::zero );
        put_cell( w, cell_spec{ point::zero, "A", 12, 4 } );
        put_cell( w, cell_spec{ point::east, "A", 3, 0 } );
        WHEN( "the window is drawn" ) {
            clear_to_sentinel();
            REQUIRE( renderer_recovery_test_support::draw_test_window( w ) );
            const std::vector<Uint32> px = read_canvas( 48, 40 );
            const auto want = [&]( const point & at ) {
                const SDL_Color &c = at.y >= 10 ? windowsPalette[0]
                                     : at.x < 3 ? windowsPalette[12]
                                     : at.x < 6 ? windowsPalette[4]
                                     : at.x < 9 ? windowsPalette[3] : windowsPalette[0];
                return rgb_of( c );
            };
            THEN( "each glyph's white texels take its color, the rest shows what is below" ) {
                int mismatches = 0;
                point first_bad = point::north_west;
                Uint32 got = 0;
                Uint32 expected = 0;
                for( int y = 0; y < 40; ++y ) {
                    for( int x = 0; x < 48; ++x ) {
                        if( px[static_cast<size_t>( y ) * 48 + x] != want( point( x, y ) ) ) {
                            if( mismatches == 0 ) {
                                first_bad = point( x, y );
                                got = px[static_cast<size_t>( y ) * 48 + x];
                                expected = want( point( x, y ) );
                            }
                            ++mismatches;
                        }
                    }
                }
                CAPTURE( first_bad.x, first_bad.y, got, expected );
                CHECK( mismatches == 0 );
            }
        }
    }
}

#endif // TILES
