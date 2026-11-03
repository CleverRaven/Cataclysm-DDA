#if defined( TILES )
#include "sdl_font.h"

#include <optional>

#include "font_loader.h"
#include "glyph_shelf_packer.h"
#include "output.h"
#include "sdl_quad_batch.h"
#include "sdl_utils.h"
#include "ui_manager.h"

#if defined(_WIN32)
#   if 1 // HACK: Hack to prevent reordering of #include "platform_win.h" by IWYU
#       include "platform_win.h"
#   endif
#   include <shlwapi.h>
#   if !defined(strcasecmp)
#       define strcasecmp StrCmpI
#   endif
#else
#   include <strings.h> // for strcasecmp
#endif

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "


    // bitmap font size test
    // return face index that has this size or below
    static int test_face_size( const std::string &f, int size, int faceIndex )
{
    const TTF_Font_Ptr fnt = OpenFontIndex( f.c_str(), size, faceIndex );
    if( fnt ) {
        const char *style = FontFaceStyleName( fnt );
        if( style != nullptr ) {
            int faces = FontFaces( fnt );
            for( int i = faces - 1; i >= 0; i-- ) {
                const TTF_Font_Ptr tf = OpenFontIndex( f.c_str(), size, i );
                const char *ts = nullptr;
                if( tf ) {
                    if( nullptr != ( ts = FontFaceStyleName( tf ) ) ) {
                        if( 0 == strcasecmp( ts, style ) && FontHeight( tf ) <= size ) {
                            return i;
                        }
                    }
                }
            }
        }
    }

    return faceIndex;
}

// power-of-two side of a regular atlas page; 512x512 RGBA is 1 MiB, holds
// ~1600 cells of 8x16, and is below every SDL3 renderer's texture limit
static constexpr int glyph_page_size = 512;

static int glyph_page_failures_armed = 0;

void arm_glyph_page_create_failures( const int count )
{
    glyph_page_failures_armed = count;
}

static bool glyph_page_create_fails()
{
    if( glyph_page_failures_armed <= 0 ) {
        return false;
    }
    --glyph_page_failures_armed;
    return true;
}

// opacity truncated to 8 bits, as an alpha mod of opacity * 255
static float quantized_opacity( const float opacity )
{
    return static_cast<Uint8>( opacity * 255.0f ) / 255.0f;
}

std::unique_ptr<Font> Font::load_font( SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
                                       const std::string &typeface, int fontsize, int width,
                                       int height,
                                       const palette_array &palette,
                                       const bool fontblending )
{
    if( is_bitmap_typeface( typeface ) ) {
        // Seems to be an image file, not a font.
        // Try to load as bitmap font from user font dir, then from font dir.
        try {
            return std::unique_ptr<Font>( std::make_unique<BitmapFont>( renderer, pixel_format, width, height,
                                          palette,
                                          typeface ) );
        } catch( std::exception & ) {
            try {
                return std::unique_ptr<Font>( std::make_unique<BitmapFont>( renderer, pixel_format, width, height,
                                              palette,
                                              PATH_INFO::user_font() + typeface ) );
            } catch( std::exception & ) {
                try {
                    return std::unique_ptr<Font>( std::make_unique<BitmapFont>( renderer, pixel_format, width, height,
                                                  palette,
                                                  PATH_INFO::fontdir() + typeface ) );
                } catch( std::exception &err ) {
                    dbg( D_ERROR ) << "Failed to load font " << typeface << ": " << err.what();
                    // Continue to load as truetype font
                }
            }
        }
    }
    // Not loaded as bitmap font (or it failed), try to load as truetype
    try {
        return std::unique_ptr<Font>( std::make_unique<CachedTTFFont>( width, height,
                                      palette, typeface, fontsize, fontblending ) );
    } catch( std::exception &err ) {
        dbg( D_ERROR ) << "Failed to load font " << typeface << ": " << err.what();
    }
    return nullptr;
}

static std::optional<bool> text_atlas_override;

bool text_atlas_enabled( const SDL_Renderer_Ptr &renderer )
{
    return text_atlas_override ? *text_atlas_override : !IsRendererSoftware( renderer );
}

void override_text_atlas( const std::optional<bool> enabled )
{
    text_atlas_override = enabled;
}

void Font::OutputChar( const SDL_Renderer_Ptr &renderer, const std::string &ch, const point &p,
                       const unsigned char color, const float opacity )
{
    static text_batch one_glyph;
    one_glyph.begin_pass( renderer, !text_atlas_enabled( renderer ) );
    queue_char( one_glyph, renderer, ch, p, color, opacity );
    one_glyph.flush( renderer );
}

// line_id is one of the LINE_*_C constants
// color is a curses color
void Font::queue_ascii_lines( text_batch &batch, const unsigned char line_id, const point &p,
                              const unsigned char color ) const
{
    const int horizontal_thickness = 1;
    const int vertical_thickness = 2;
    const int starting_x_offset = ( width - vertical_thickness ) / 2;
    const int starting_y_offset = ( height - horizontal_thickness ) / 2;
    const SDL_Color sdl_color = palette[color];
    const auto hline = [&]( const point & from, const int x2 ) {
        batch.add_rect( SDL_Rect{ from.x, from.y, x2 - from.x, horizontal_thickness }, sdl_color );
    };
    const auto vline = [&]( const point & from, const int y2 ) {
        batch.add_rect( SDL_Rect{ from.x, from.y, vertical_thickness, y2 - from.y }, sdl_color );
    };
    switch( line_id ) {
        // box bottom/top side (horizontal line)
        case LINE_OXOX_C:
            hline( p + point( 0, starting_y_offset ), p.x + width );
            break;
        // box left/right side (vertical line)
        case LINE_XOXO_C:
            vline( p + point( starting_x_offset, 0 ), p.y + height );
            break;
        // box top left
        case LINE_OXXO_C:
            hline( p + point( starting_x_offset, starting_y_offset ), p.x + width );
            vline( p + point( starting_x_offset, starting_y_offset ), p.y + height );
            break;
        // box top right
        case LINE_OOXX_C:
            hline( p + point( 0, starting_y_offset ), p.x + starting_x_offset + vertical_thickness );
            vline( p + point( starting_x_offset, starting_y_offset ), p.y + height );
            break;
        // box bottom right
        case LINE_XOOX_C:
            hline( p + point( 0, starting_y_offset ), p.x + starting_x_offset + vertical_thickness );
            vline( p + point( starting_x_offset, 0 ), p.y + starting_y_offset + horizontal_thickness );
            break;
        // box bottom left
        case LINE_XXOO_C:
            hline( p + point( starting_x_offset, starting_y_offset ), p.x + width );
            vline( p + point( starting_x_offset, 0 ), p.y + starting_y_offset + horizontal_thickness );
            break;
        // box bottom north T (left, right, up)
        case LINE_XXOX_C:
            hline( p + point( 0, starting_y_offset ), p.x + width );
            vline( p + point( starting_x_offset, 0 ), p.y + starting_y_offset );
            break;
        // box bottom east T (up, right, down)
        case LINE_XXXO_C:
            vline( p + point( starting_x_offset, 0 ), p.y + height );
            hline( p + point( starting_x_offset, starting_y_offset ), p.x + width );
            break;
        // box bottom south T (left, right, down)
        case LINE_OXXX_C:
            hline( p + point( 0, starting_y_offset ), p.x + width );
            vline( p + point( starting_x_offset, starting_y_offset ), p.y + height );
            break;
        // box X (left down up right)
        case LINE_XXXX_C:
            hline( p + point( 0, starting_y_offset ), p.x + width );
            vline( p + point( starting_x_offset, 0 ), p.y + height );
            break;
        // box bottom east T (left, down, up)
        case LINE_XOXX_C:
            vline( p + point( starting_x_offset, 0 ), p.y + height );
            hline( p + point( 0, starting_y_offset ), p.x + starting_x_offset );
            break;
        default:
            break;
    }
}

CachedTTFFont::CachedTTFFont(
    const int w, const int h,
    const palette_array &palette,
    std::string typeface, int fontsize,
    const bool fontblending )
    : Font( w, h, palette )
    , packer( glyph_page_size, h, 1 )
    , fontblending( fontblending )
{
    int faceIndex = 0;
    std::vector<std::string> typefaces;
    std::vector<std::string> known_suffixes = { ".ttf", ".otf", ".ttc", ".fon" };
    bool add_suffix = true;
    for( const std::string &ks : known_suffixes ) {
        if( string_ends_with( typeface, ks ) ) {
            add_suffix = false;
            break;
        }
    }
    bool add_prefix = true;
    std::vector<std::string> known_prefixes = {
        PATH_INFO::user_font(), PATH_INFO::fontdir()
    };
#if defined(_WIN32)
    constexpr UINT max_dir_len = 256;
    char buf[max_dir_len];
    const UINT dir_len = GetSystemWindowsDirectory( buf, max_dir_len );
    if( dir_len == 0 ) {
        throw std::runtime_error( "GetSystemWindowsDirectory failed" );
    } else if( dir_len >= max_dir_len ) {
        throw std::length_error( "GetSystemWindowsDirectory failed due to insufficient buffer" );
    }
    known_prefixes.emplace_back( buf + std::string( "\\fonts\\" ) );
#elif defined(_APPLE_) && defined(_MACH_)
    /*
    // Well I don't know how osx actually works ....
    known_prefixes.emplace_back( "/System/Library/Fonts/" );
    known_prefixes.emplace_back( "/Library/Fonts/" );
    wordexp_t exp;
    wordexp( "~/Library/Fonts/", &exp, 0 );
    known_prefixes.emplace_back( exp.we_wordv[0] );
    wordfree( &exp );
    */
#else // Other POSIX-ish systems
    known_prefixes.emplace_back( "/usr/share/fonts/" );
    known_prefixes.emplace_back( "/usr/local/share/fonts/" );
    char *home;
    if( ( home = getenv( "HOME" ) ) ) {
        std::string userfontdir = home;
        userfontdir += "/.fonts/";
        known_prefixes.emplace_back( userfontdir );
    }
#endif

    for( const std::string &kp : known_prefixes ) {
        if( string_starts_with( typeface, kp ) ) {
            add_prefix = false;
            break;
        }
    }

    for( const std::string &ks : known_suffixes ) {
        for( const std::string &kp : known_prefixes ) {
            if( add_prefix ) {
                typefaces.emplace_back( kp + typeface + ( add_suffix ? ks : "" ) );
            }
            typefaces.emplace_back( typeface + ( add_suffix ? ks : "" ) );
        }
    }
    if( add_suffix ) {
        typefaces.emplace_back( typeface );
    }
    ensure_unifont_loaded( typefaces );

    for( const std::string &tf : typefaces ) {
        if( !file_exist( tf ) ) {
            dbg( D_INFO ) << "Not found " << tf;
            continue;
        }
        dbg( D_INFO ) << "Loading truetype font " << tf;
        typeface = tf;
        break;
    }

    if( fontsize <= 0 ) {
        fontsize = height - 1;
    }
    // SDL_ttf handles bitmap fonts size incorrectly
    if( typeface.length() > 4 &&
        strcasecmp( typeface.substr( typeface.length() - 4 ).c_str(), ".fon" ) == 0 ) {
        faceIndex = test_face_size( typeface, fontsize, faceIndex );
    }
    font = OpenFontIndex( typeface.c_str(), fontsize, faceIndex );
    if( !font ) {
        throw std::runtime_error( SDL_GetError() );
    }
    SetFontStyle( font, TTF_STYLE_NORMAL );
}

SDL_Surface_Ptr CachedTTFFont::render_glyph_cell( const std::string &ch, int &cell_width,
        const SDL_Color &color ) const
{
    SDL_Surface_Ptr sglyph = fontblending ? RenderUTF8_Blended( font, ch.c_str(), color )
                             : RenderUTF8_Solid( font, ch.c_str(), color );
    if( !sglyph ) {
        dbg( D_ERROR ) << "Failed to create glyph for " << ch << ": " << SDL_GetError();
        return nullptr;
    }
    cell_width = width * utf8_width( ch );
    if( cell_width <= 0 ) {
        return nullptr;
    }
    SDL_Surface_Ptr surface = create_surface_32( cell_width, height );
    SDL_Rect src_rect = { 0, 0, sglyph->w, sglyph->h };
    SDL_Rect dst_rect = { 0, 0, cell_width, height };
    if( src_rect.w < dst_rect.w ) {
        dst_rect.x = ( dst_rect.w - src_rect.w ) / 2;
        dst_rect.w = src_rect.w;
    } else if( src_rect.w > dst_rect.w ) {
        src_rect.x = ( src_rect.w - dst_rect.w ) / 2;
        src_rect.w = dst_rect.w;
    }
    if( src_rect.h < dst_rect.h ) {
        dst_rect.y = ( dst_rect.h - src_rect.h ) / 2;
        dst_rect.h = src_rect.h;
    } else if( src_rect.h > dst_rect.h ) {
        src_rect.y = ( src_rect.h - dst_rect.h ) / 2;
        src_rect.h = dst_rect.h;
    }
    // Copy without altering the source
    SetSurfaceBlendMode( sglyph, SDL_BLENDMODE_NONE );
    if( printErrorIf( BlitSurface( sglyph, &src_rect, surface, &dst_rect ) != 0,
                      "SDL_BlitSurface failed" ) ) {
        return nullptr;
    }
    return surface;
}

bool CachedTTFFont::ensure_page( const SDL_Renderer_Ptr &renderer, const int page,
                                 const int w, const int h )
{
    if( page < static_cast<int>( pages.size() ) ) {
        return true;
    }
    // Pages are committed in order, so a missing page is always the next one
    cata_assert( page == static_cast<int>( pages.size() ) );
    if( glyph_page_create_fails() ) {
        return false;
    }
    SDL_Texture_Ptr tex = CreateTexture( renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_STATIC, w, h );
    if( !tex ) {
        return false;
    }
    SetTextureBlendMode( tex, SDL_BLENDMODE_BLEND );
    // Static textures start undefined, so gaps between cells must be clear
    if( !UpdateTexture( tex, SDL_Rect{ 0, 0, w, h }, create_surface_32( w, h ) ) ) {
        return false;
    }
    pages.push_back( std::move( tex ) );
    return true;
}

const CachedTTFFont::glyph_entry &CachedTTFFont::glyph_for( const SDL_Renderer_Ptr &renderer,
        const std::string &ch )
{
    const auto found = glyphs.find( ch );
    if( found != glyphs.end() ) {
        return found->second;
    }
    // returned, not cached, when GPU work failed, so the next draw retries
    static const glyph_entry not_drawn;
    int cell_width = 0;
    // white glyphs: vertex color supplies the palette color when drawn
    constexpr SDL_Color white = { 255, 255, 255, 255 };
    const SDL_Surface_Ptr cell = render_glyph_cell( ch, cell_width, white );
    const std::optional<glyph_shelf_packer::placement> p =
        cell ? packer.plan( cell_width ) : std::nullopt;
    if( !p ) {
        // either SDL_ttf can't render it, or it has no width. that won't
        // change.
        return glyphs.emplace( ch, glyph_entry() ).first->second;
    }
    if( !ensure_page( renderer, p->at.page, p->page_w, p->page_h ) ) {
        return not_drawn;
    }
    const SDL_Rect rect = { p->at.pos.x, p->at.pos.y, cell_width, height };
    if( !UpdateTexture( pages[p->at.page], rect, cell ) ) {
        return not_drawn;
    }
    packer.commit( *p );
    glyph_entry entry;
    entry.page = p->at.page;
    entry.cell = rect;
    return glyphs.emplace( ch, entry ).first->second;
}

bool CachedTTFFont::isGlyphProvided( const std::string &ch ) const
{
    // Just return false if the glyph is not provided by the font
    if( !CanRenderGlyph( font, UTF8_getch( ch ) ) ) {
        return false;
    }

    // Test whether the glyph can actually be rendered
    constexpr SDL_Color white{255, 255, 255, 0};
    SDL_Surface_Ptr surface = RenderUTF8_Solid( font, ch.c_str(), white );
    return static_cast<bool>( surface );
}

const CachedTTFFont::immediate_glyph &CachedTTFFont::immediate_glyph_for(
    const SDL_Renderer_Ptr &renderer, const std::string &ch, const unsigned char color )
{
    std::string key = ch;
    key += static_cast<char>( color );
    const auto found = immediate_glyphs.find( key );
    if( found != immediate_glyphs.end() ) {
        return found->second;
    }
    immediate_glyph entry;
    const SDL_Surface_Ptr cell = render_glyph_cell( ch, entry.width, palette[color] );
    if( cell ) {
        SetSurfaceBlendMode( cell, SDL_BLENDMODE_BLEND );
        entry.texture = CreateTextureFromSurface( renderer, cell );
    }
    return immediate_glyphs.emplace( std::move( key ), std::move( entry ) ).first->second;
}

void CachedTTFFont::queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                                const std::string &ch, const point &p,
                                const unsigned char color, const float opacity )
{
    if( batch.immediate() ) {
        const immediate_glyph &g = immediate_glyph_for( renderer, ch, color & 0xf );
        if( !g.texture ) {
            return;
        }
        const SDL_Rect rect = { p.x, p.y, g.width, height };
        if( opacity != 1.0f ) {
            SetTextureAlphaMod( g.texture, opacity * 255.0f );
        }
        RenderCopy( renderer, g.texture, nullptr, &rect );
        if( opacity != 1.0f ) {
            SetTextureAlphaMod( g.texture, 255 );
        }
        return;
    }
    const glyph_entry &glyph = glyph_for( renderer, ch );
    if( glyph.page < 0 ) {
        return;
    }
    const SDL_Color &c = palette[color & 0xf];
    batch.add_glyph( pages[glyph.page].get(), packer.page_width( glyph.page ),
                     packer.page_height( glyph.page ),
                     SDL_Rect{ p.x, p.y, glyph.cell.w, glyph.cell.h }, glyph.cell,
                     SDL_FColor{ c.r / 255.0f, c.g / 255.0f, c.b / 255.0f,
                                 quantized_opacity( opacity ) } );
}

BitmapFont::BitmapFont(
    SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
    const int w, const int h,
    const palette_array &palette,
    const std::string &typeface_path )
    : Font( w, h, palette ), typeface_path( typeface_path ), source_pixel_format( pixel_format )
{
    build_textures( renderer, pixel_format );
}

void BitmapFont::build_textures( const SDL_Renderer_Ptr &renderer, const Uint32 pixel_format )
{
    dbg( D_INFO ) << "Loading bitmap font [" + typeface_path + "].";
    SDL_Surface_Ptr asciiload = load_image( typeface_path.c_str() );
    cata_assert( asciiload );
    if( asciiload->w * asciiload->h < ( width * height * 256 ) ) {
        throw std::runtime_error( "bitmap for font is to small" );
    }
    tilewidth = asciiload->w / width;
    Uint32 key = MapRGB( asciiload, 0xFF, 0, 0xFF );
    SetColorKey( asciiload, 1, key );
    std::array<SDL_Surface_Ptr, std::tuple_size_v<decltype( ascii )>> ascii_surf;
    ascii_surf[0] = ConvertSurfaceFormat( asciiload, pixel_format );
    atlas_size = point( ascii_surf[0]->w, ascii_surf[0]->h );
    SetSurfaceRLE( ascii_surf[0], 1 );
    asciiload.reset();

    for( size_t a = 1; a < std::tuple_size_v<decltype( ascii )>; ++a ) {
        ascii_surf[a] = ConvertSurfaceFormat( ascii_surf[0], pixel_format );
        SetSurfaceRLE( ascii_surf[a], 1 );
    }

    for( size_t a = 0; a < std::tuple_size_v<decltype( ascii )> - 1; ++a ) {
        throwErrorIf( LockSurface( ascii_surf[a] ) != 0, "SDL_LockSurface failed" );
        int size = ascii_surf[a]->h * ascii_surf[a]->w;
        Uint32 *pixels = static_cast<Uint32 *>( ascii_surf[a]->pixels );
        Uint32 color = ( windowsPalette[a].r << 16 ) | ( windowsPalette[a].g << 8 ) | windowsPalette[a].b;
        for( int i = 0; i < size; i++ ) {
            if( pixels[i] == 0xFFFFFF ) {
                pixels[i] = color;
            }
        }
        UnlockSurface( ascii_surf[a] );
    }
    //convert ascii_surf to SDL_Texture
    for( size_t a = 0; a < std::tuple_size_v<decltype( ascii )>; ++a ) {
        ascii[a] = CreateTextureFromSurface( renderer, ascii_surf[a] );
    }
}

void BitmapFont::queue_ascii_lines( text_batch &batch, const unsigned char line_id,
                                    const point &p, const unsigned char color ) const
{
    switch( line_id ) {
        // box bottom/top side (horizontal line)
        case LINE_OXOX_C:
            queue_index( batch, 0xcd, p, color, 1.0f );
            break;
        // box left/right side (vertical line)
        case LINE_XOXO_C:
            queue_index( batch, 0xba, p, color, 1.0f );
            break;
        // box top left
        case LINE_OXXO_C:
            queue_index( batch, 0xc9, p, color, 1.0f );
            break;
        // box top right
        case LINE_OOXX_C:
            queue_index( batch, 0xbb, p, color, 1.0f );
            break;
        // box bottom right
        case LINE_XOOX_C:
            queue_index( batch, 0xbc, p, color, 1.0f );
            break;
        // box bottom left
        case LINE_XXOO_C:
            queue_index( batch, 0xc8, p, color, 1.0f );
            break;
        // box bottom north T (left, right, up)
        case LINE_XXOX_C:
            queue_index( batch, 0xca, p, color, 1.0f );
            break;
        // box bottom east T (up, right, down)
        case LINE_XXXO_C:
            queue_index( batch, 0xcc, p, color, 1.0f );
            break;
        // box bottom south T (left, right, down)
        case LINE_OXXX_C:
            queue_index( batch, 0xcb, p, color, 1.0f );
            break;
        // box X (left down up right)
        case LINE_XXXX_C:
            queue_index( batch, 0xce, p, color, 1.0f );
            break;
        // box bottom east T (left, down, up)
        case LINE_XOXX_C:
            queue_index( batch, 0xb9, p, color, 1.0f );
            break;
        default:
            break;
    }
}

bool BitmapFont::isGlyphProvided( const std::string &ch ) const
{
    const uint32_t t = UTF8_getch( ch );
    switch( t ) {
        case LINE_XOXO_UNICODE:
        case LINE_OXOX_UNICODE:
        case LINE_XXOO_UNICODE:
        case LINE_OXXO_UNICODE:
        case LINE_OOXX_UNICODE:
        case LINE_XOOX_UNICODE:
        case LINE_XXXO_UNICODE:
        case LINE_XXOX_UNICODE:
        case LINE_XOXX_UNICODE:
        case LINE_OXXX_UNICODE:
        case LINE_XXXX_UNICODE:
            return true;
        default:
            return t < 256;
    }
}

void BitmapFont::queue_char( text_batch &batch, const SDL_Renderer_Ptr &,
                             const std::string &ch, const point &p,
                             const unsigned char color, const float opacity )
{
    const int t = UTF8_getch( ch );
    if( t <= 256 ) {
        queue_index( batch, t, p, color, opacity );
    } else {
        unsigned char uc = 0;
        switch( t ) {
            case LINE_XOXO_UNICODE:
                uc = LINE_XOXO_C;
                break;
            case LINE_OXOX_UNICODE:
                uc = LINE_OXOX_C;
                break;
            case LINE_XXOO_UNICODE:
                uc = LINE_XXOO_C;
                break;
            case LINE_OXXO_UNICODE:
                uc = LINE_OXXO_C;
                break;
            case LINE_OOXX_UNICODE:
                uc = LINE_OOXX_C;
                break;
            case LINE_XOOX_UNICODE:
                uc = LINE_XOOX_C;
                break;
            case LINE_XXXO_UNICODE:
                uc = LINE_XXXO_C;
                break;
            case LINE_XXOX_UNICODE:
                uc = LINE_XXOX_C;
                break;
            case LINE_XOXX_UNICODE:
                uc = LINE_XOXX_C;
                break;
            case LINE_OXXX_UNICODE:
                uc = LINE_OXXX_C;
                break;
            case LINE_XXXX_UNICODE:
                uc = LINE_XXXX_C;
                break;
            default:
                return;
        }
        queue_ascii_lines( batch, uc, p, color );
    }
}

void BitmapFont::queue_index( text_batch &batch, const int t, const point &p,
                              const unsigned char color, const float opacity ) const
{
    const SDL_Rect src = { ( t % tilewidth ) *width, ( t / tilewidth ) *height, width, height };
    if( batch.immediate() ) {
        const SDL_Rect rect = { p.x, p.y, width, height };
        if( opacity != 1.0f ) {
            SetTextureAlphaMod( ascii[color], opacity * 255 );
        }
        RenderCopy( batch.renderer(), ascii[color], &src, &rect );
        if( opacity != 1.0f ) {
            SetTextureAlphaMod( ascii[color], 255 );
        }
        return;
    }
    batch.add_glyph( ascii[color].get(), atlas_size.x, atlas_size.y,
                     SDL_Rect{ p.x, p.y, width, height }, src,
                     SDL_FColor{ 1.0f, 1.0f, 1.0f, quantized_opacity( opacity ) } );
}

FontFallbackList::FontFallbackList(
    SDL_Renderer_Ptr &renderer, Uint32 pixel_format,
    const int w, const int h,
    const palette_array &palette,
    const std::vector<font_config> &typefaces,
    const int fontsize, const bool fontblending )
    : Font( w, h, palette )
{
    for( const font_config &font_config : typefaces ) {
        std::unique_ptr<Font> font = Font::load_font( renderer, pixel_format, font_config.path, fontsize, w,
                                     h,
                                     palette, fontblending );
        if( !font ) {
            throw std::runtime_error( "Cannot load font " + font_config.path );
        }
        fonts.emplace_back( std::move( font ) );
    }
    if( fonts.empty() ) {
        throw std::runtime_error( "Typeface list is empty" );
    }
}

bool FontFallbackList::isGlyphProvided( const std::string & ) const
{
    return true;
}

void FontFallbackList::queue_char( text_batch &batch, const SDL_Renderer_Ptr &renderer,
                                   const std::string &ch, const point &p,
                                   const unsigned char color, const float opacity )
{
    auto cached = glyph_font.find( ch );
    if( cached == glyph_font.end() ) {
        for( auto it = fonts.begin(); it != fonts.end(); ++it ) {
            if( std::next( it ) == fonts.end() || ( *it )->isGlyphProvided( ch ) ) {
                cached = glyph_font.emplace( ch, it ).first;
                break;
            }
        }
    }
    ( *cached->second )->queue_char( batch, renderer, ch, p, color, opacity );
}

void CachedTTFFont::release_gpu_resources()
{
    glyphs.clear();
    pages.clear();
    packer.clear();
    immediate_glyphs.clear();
}

void BitmapFont::release_gpu_resources()
{
    for( SDL_Texture_Ptr &tex : ascii ) {
        tex.reset();
    }
}

void BitmapFont::rebuild_for_renderer( const SDL_Renderer_Ptr &renderer )
{
    build_textures( renderer, source_pixel_format );
}

void FontFallbackList::release_gpu_resources()
{
    for( std::unique_ptr<Font> &child : fonts ) {
        if( child ) {
            child->release_gpu_resources();
        }
    }
}

void FontFallbackList::rebuild_for_renderer( const SDL_Renderer_Ptr &renderer )
{
    for( std::unique_ptr<Font> &child : fonts ) {
        if( child ) {
            child->rebuild_for_renderer( renderer );
        }
    }
}

#endif // TILES
