#if defined(TILES)
#include "sdl_font.h"

#include "font_loader.h"
#include "output.h"
#include "sdl_utils.h"

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
    const TTF_Font_Ptr fnt( TTF_OpenFontIndex( f.c_str(), size, faceIndex ) );
    if( fnt ) {
        const char *style = TTF_FontFaceStyleName( fnt.get() );
        if( style != nullptr ) {
            int faces = TTF_FontFaces( fnt.get() );
            for( int i = faces - 1; i >= 0; i-- ) {
                const TTF_Font_Ptr tf( TTF_OpenFontIndex( f.c_str(), size, i ) );
                const char *ts = nullptr;
                if( tf ) {
                    if( nullptr != ( ts = TTF_FontFaceStyleName( tf.get() ) ) ) {
                        if( 0 == strcasecmp( ts, style ) && TTF_FontHeight( tf.get() ) <= size ) {
                            return i;
                        }
                    }
                }
            }
        }
    }

    return faceIndex;
}

std::unique_ptr<Font> Font::load_font( SDL_Renderer_Ptr &renderer, SDL_PixelFormat_Ptr &format,
                                       const std::string &typeface, int fontsize, int width,
                                       int height,
                                       const palette_array &palette,
                                       const bool fontblending )
{
    if( string_ends_with( typeface, ".bmp" ) || string_ends_with( typeface, ".png" ) ) {
        // Seems to be an image file, not a font.
        // Try to load as bitmap font from user font dir, then from font dir.
        try {
            return std::unique_ptr<Font>( std::make_unique<BitmapFont>( renderer, format, width, height,
                                          palette,
                                          typeface ) );
        } catch( std::exception & ) {
            try {
                return std::unique_ptr<Font>( std::make_unique<BitmapFont>( renderer, format, width, height,
                                              palette,
                                              PATH_INFO::user_font() + typeface ) );
            } catch( std::exception & ) {
                try {
                    return std::unique_ptr<Font>( std::make_unique<BitmapFont>( renderer, format, width, height,
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

// line_id is one of the LINE_*_C constants
// FG is a curses color
void Font::draw_ascii_lines( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                             unsigned char line_id, const point &p, unsigned char color ) const
{
    const int horizontal_thickness = 1;
    const int vertical_thickness = 2;
    const int starting_x_offset = ( width - vertical_thickness ) / 2;
    const int starting_y_offset = ( height - horizontal_thickness ) / 2;
    SDL_Color sdl_color = palette[color];
    switch( line_id ) {
        // box bottom/top side (horizontal line)
        case LINE_OXOX_C:
            geometry->horizontal_line( renderer, p + point( 0, starting_y_offset ), p.x + width,
                                       horizontal_thickness,
                                       sdl_color );
            break;
        // box left/right side (vertical line)
        case LINE_XOXO_C:
            geometry->vertical_line( renderer, p + point( starting_x_offset, 0 ), p.y + height,
                                     vertical_thickness,
                                     sdl_color );
            break;
        // box top left
        case LINE_OXXO_C:
            geometry->horizontal_line( renderer, p + point( starting_x_offset, starting_y_offset ),
                                       p.x + width,
                                       horizontal_thickness,
                                       sdl_color );
            geometry->vertical_line( renderer, p + point( starting_x_offset, starting_y_offset ),
                                     p.y + height,
                                     vertical_thickness,
                                     sdl_color );
            break;
        // box top right
        case LINE_OOXX_C:
            geometry->horizontal_line( renderer, p + point( 0, starting_y_offset ),
                                       p.x + starting_x_offset + vertical_thickness,
                                       horizontal_thickness,
                                       sdl_color );
            geometry->vertical_line( renderer, p + point( starting_x_offset, starting_y_offset ),
                                     p.y + height,
                                     vertical_thickness,
                                     sdl_color );
            break;
        // box bottom right
        case LINE_XOOX_C:
            geometry->horizontal_line( renderer, p + point( 0, starting_y_offset ),
                                       p.x + starting_x_offset + vertical_thickness,
                                       horizontal_thickness,
                                       sdl_color );
            geometry->vertical_line( renderer, p + point( starting_x_offset, 0 ),
                                     p.y + starting_y_offset + horizontal_thickness,
                                     vertical_thickness, sdl_color );
            break;
        // box bottom left
        case LINE_XXOO_C:
            geometry->horizontal_line( renderer, p + point( starting_x_offset, starting_y_offset ),
                                       p.x + width,
                                       horizontal_thickness,
                                       sdl_color );
            geometry->vertical_line( renderer, p + point( starting_x_offset, 0 ),
                                     p.y + starting_y_offset + horizontal_thickness,
                                     vertical_thickness, sdl_color );
            break;
        // box bottom north T (left, right, up)
        case LINE_XXOX_C:
            geometry->horizontal_line( renderer, p + point( 0, starting_y_offset ), p.x + width,
                                       horizontal_thickness,
                                       sdl_color );
            geometry->vertical_line( renderer, p + point( starting_x_offset, 0 ), p.y + starting_y_offset,
                                     vertical_thickness,
                                     sdl_color );
            break;
        // box bottom east T (up, right, down)
        case LINE_XXXO_C:
            geometry->vertical_line( renderer, p + point( starting_x_offset, 0 ), p.y + height,
                                     vertical_thickness,
                                     sdl_color );
            geometry->horizontal_line( renderer, p + point( starting_x_offset, starting_y_offset ),
                                       p.x + width,
                                       horizontal_thickness,
                                       sdl_color );
            break;
        // box bottom south T (left, right, down)
        case LINE_OXXX_C:
            geometry->horizontal_line( renderer, p + point( 0, starting_y_offset ), p.x + width,
                                       horizontal_thickness,
                                       sdl_color );
            geometry->vertical_line( renderer, p + point( starting_x_offset, starting_y_offset ),
                                     p.y + height,
                                     vertical_thickness,
                                     sdl_color );
            break;
        // box X (left down up right)
        case LINE_XXXX_C:
            geometry->horizontal_line( renderer, p + point( 0, starting_y_offset ), p.x + width,
                                       horizontal_thickness,
                                       sdl_color );
            geometry->vertical_line( renderer, p + point( starting_x_offset, 0 ), p.y + height,
                                     vertical_thickness,
                                     sdl_color );
            break;
        // box bottom east T (left, down, up)
        case LINE_XOXX_C:
            geometry->vertical_line( renderer, p + point( starting_x_offset, 0 ), p.y + height,
                                     vertical_thickness,
                                     sdl_color );
            geometry->horizontal_line( renderer, p + point( 0, starting_y_offset ), p.x + starting_x_offset,
                                       horizontal_thickness,
                                       sdl_color );
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
    font.reset( TTF_OpenFontIndex( typeface.c_str(), fontsize, faceIndex ) );
    if( !font ) {
        throw std::runtime_error( TTF_GetError() );
    }
    TTF_SetFontStyle( font.get(), TTF_STYLE_NORMAL );
}

SDL_Texture_Ptr CachedTTFFont::create_glyph( const SDL_Renderer_Ptr &renderer,
        const std::string &ch,
        const int color )
{
    const auto function = fontblending ? TTF_RenderUTF8_Blended : TTF_RenderUTF8_Solid;
    SDL_Surface_Ptr sglyph( function( font.get(), ch.c_str(), windowsPalette[color] ) );
    if( !sglyph ) {
        dbg( D_ERROR ) << "Failed to create glyph for " << ch << ": " << TTF_GetError();
        return nullptr;
    }
    const int wf = utf8_width( ch );
    // Note: bits per pixel must be 8 to be synchronized with the surface
    // that TTF_RenderGlyph above returns. This is important for SDL_BlitScaled
    SDL_Surface_Ptr surface = create_surface_32( width * wf, height );
    SDL_Rect src_rect = { 0, 0, sglyph->w, sglyph->h };
    SDL_Rect dst_rect = { 0, 0, width * wf, height };
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
    SDL_SetSurfaceBlendMode( sglyph.get(), SDL_BLENDMODE_NONE );
    if( !printErrorIf( SDL_BlitSurface( sglyph.get(), &src_rect, surface.get(), &dst_rect ) != 0,
                       "SDL_BlitSurface failed" ) ) {
        sglyph = std::move( surface );
    }
    SDL_SetSurfaceBlendMode( sglyph.get(), SDL_BLENDMODE_BLEND );

    return CreateTextureFromSurface( renderer, sglyph );
}

bool CachedTTFFont::isGlyphProvided( const std::string &ch ) const
{
    // Just return false if the glyph is not provided by the font
    if( !TTF_GlyphIsProvided( font.get(), UTF8_getch( ch ) ) ) {
        return false;
    }

    // Test whether the glyph can actually be rendered
    constexpr SDL_Color white{255, 255, 255, 0};
    SDL_Surface_Ptr surface( TTF_RenderUTF8_Solid( font.get(), ch.c_str(), white ) );
    return static_cast<bool>( surface );
}

void CachedTTFFont::OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &,
                                const std::string &ch, const point &p,
                                unsigned char color, const float opacity )
{
    key_t    key {ch, static_cast<unsigned char>( color & 0xf )};

    auto it = glyph_cache_map.find( key );
    if( it == std::end( glyph_cache_map ) ) {
        cached_t new_entry {
            create_glyph( renderer, key.codepoints, key.color ),
            static_cast<int>( width * utf8_width( key.codepoints ) )
        };
        it = glyph_cache_map.insert( std::make_pair( std::move( key ), std::move( new_entry ) ) ).first;
    }
    const cached_t &value = it->second;

    if( !value.texture ) {
        // Nothing we can do here )-:
        return;
    }
    SDL_Rect rect {p.x, p.y, value.width, height};
    if( opacity != 1.0f ) {
        SDL_SetTextureAlphaMod( value.texture.get(), opacity * 255.0f );
    }
    RenderCopy( renderer, value.texture, nullptr, &rect );
    if( opacity != 1.0f ) {
        SDL_SetTextureAlphaMod( value.texture.get(), 255 );
    }
}

BitmapFont::BitmapFont(
    SDL_Renderer_Ptr &renderer, SDL_PixelFormat_Ptr &format,
    const int w, const int h,
    const palette_array &palette,
    const std::string &typeface_path )
    : Font( w, h, palette )
{
    dbg( D_INFO ) << "Loading bitmap font [" + typeface_path + "].";
    SDL_Surface_Ptr asciiload = load_image( typeface_path.c_str() );
    cata_assert( asciiload );
    if( asciiload->w * asciiload->h < ( width * height * 256 ) ) {
        throw std::runtime_error( "bitmap for font is to small" );
    }
    Uint32 key = SDL_MapRGB( asciiload->format, 0xFF, 0, 0xFF );
    SDL_SetColorKey( asciiload.get(), SDL_TRUE, key );
    std::array<SDL_Surface_Ptr, std::tuple_size<decltype( ascii )>::value> ascii_surf;
    ascii_surf[0].reset( SDL_ConvertSurface( asciiload.get(), format.get(), 0 ) );
    SDL_SetSurfaceRLE( ascii_surf[0].get(), 1 );
    asciiload.reset();

    for( size_t a = 1; a < std::tuple_size<decltype( ascii )>::value; ++a ) {
        ascii_surf[a].reset( SDL_ConvertSurface( ascii_surf[0].get(), format.get(), 0 ) );
        SDL_SetSurfaceRLE( ascii_surf[a].get(), 1 );
    }

    for( size_t a = 0; a < std::tuple_size<decltype( ascii )>::value - 1; ++a ) {
        SDL_LockSurface( ascii_surf[a].get() );
        int size = ascii_surf[a]->h * ascii_surf[a]->w;
        Uint32 *pixels = static_cast<Uint32 *>( ascii_surf[a]->pixels );
        Uint32 color = ( windowsPalette[a].r << 16 ) | ( windowsPalette[a].g << 8 ) | windowsPalette[a].b;
        for( int i = 0; i < size; i++ ) {
            if( pixels[i] == 0xFFFFFF ) {
                pixels[i] = color;
            }
        }
        SDL_UnlockSurface( ascii_surf[a].get() );
    }
    tilewidth = ascii_surf[0]->w / width;

    //convert ascii_surf to SDL_Texture
    for( size_t a = 0; a < std::tuple_size<decltype( ascii )>::value; ++a ) {
        ascii[a] = CreateTextureFromSurface( renderer, ascii_surf[a] );
    }
}

void BitmapFont::draw_ascii_lines( const SDL_Renderer_Ptr &renderer,
                                   const GeometryRenderer_Ptr &geometry,
                                   unsigned char line_id, const point &p, unsigned char color ) const
{
    BitmapFont *t = const_cast<BitmapFont *>( this );
    switch( line_id ) {
        // box bottom/top side (horizontal line)
        case LINE_OXOX_C:
            t->OutputChar( renderer, geometry, 0xcd, p, color );
            break;
        // box left/right side (vertical line)
        case LINE_XOXO_C:
            t->OutputChar( renderer, geometry, 0xba, p, color );
            break;
        // box top left
        case LINE_OXXO_C:
            t->OutputChar( renderer, geometry, 0xc9, p, color );
            break;
        // box top right
        case LINE_OOXX_C:
            t->OutputChar( renderer, geometry, 0xbb, p, color );
            break;
        // box bottom right
        case LINE_XOOX_C:
            t->OutputChar( renderer, geometry, 0xbc, p, color );
            break;
        // box bottom left
        case LINE_XXOO_C:
            t->OutputChar( renderer, geometry, 0xc8, p, color );
            break;
        // box bottom north T (left, right, up)
        case LINE_XXOX_C:
            t->OutputChar( renderer, geometry, 0xca, p, color );
            break;
        // box bottom east T (up, right, down)
        case LINE_XXXO_C:
            t->OutputChar( renderer, geometry, 0xcc, p, color );
            break;
        // box bottom south T (left, right, down)
        case LINE_OXXX_C:
            t->OutputChar( renderer, geometry, 0xcb, p, color );
            break;
        // box X (left down up right)
        case LINE_XXXX_C:
            t->OutputChar( renderer, geometry, 0xce, p, color );
            break;
        // box bottom east T (left, down, up)
        case LINE_XOXX_C:
            t->OutputChar( renderer, geometry, 0xb9, p, color );
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

void BitmapFont::OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                             const std::string &ch, const point &p,
                             unsigned char color, const float opacity )
{
    const int t = UTF8_getch( ch );
    BitmapFont::OutputChar( renderer, geometry, t, p, color, opacity );
}

void BitmapFont::OutputChar( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                             const int t, const point &p,
                             unsigned char color, const float opacity )
{
    if( t <= 256 ) {
        SDL_Rect src;
        src.x = ( t % tilewidth ) * width;
        src.y = ( t / tilewidth ) * height;
        src.w = width;
        src.h = height;
        SDL_Rect rect;
        rect.x = p.x;
        rect.y = p.y;
        rect.w = width;
        rect.h = height;
        if( opacity != 1.0f ) {
            SDL_SetTextureAlphaMod( ascii[color].get(), opacity * 255 );
        }
        RenderCopy( renderer, ascii[color], &src, &rect );
        if( opacity != 1.0f ) {
            SDL_SetTextureAlphaMod( ascii[color].get(), 255 );
        }
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
        draw_ascii_lines( renderer, geometry, uc, p, color );
    }
}

FontFallbackList::FontFallbackList(
    SDL_Renderer_Ptr &renderer, SDL_PixelFormat_Ptr &format,
    const int w, const int h,
    const palette_array &palette,
    const std::vector<std::string> &typefaces,
    const int fontsize, const bool fontblending )
    : Font( w, h, palette )
{
    for( const std::string &typeface : typefaces ) {
        std::unique_ptr<Font> font = Font::load_font( renderer, format, typeface, fontsize, w, h, palette,
                                     fontblending );
        if( !font ) {
            throw std::runtime_error( "Cannot load font " + typeface );
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

void FontFallbackList::OutputChar( const SDL_Renderer_Ptr &renderer,
                                   const GeometryRenderer_Ptr &geometry,
                                   const std::string &ch, const point &p,
                                   unsigned char color, const float opacity )
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
    ( *cached->second )->OutputChar( renderer, geometry, ch, p, color, opacity );
}

#endif // TILES
