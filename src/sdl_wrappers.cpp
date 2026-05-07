#if defined(TILES) || defined(SDL_SOUND)

#include "sdl_wrappers.h"

#include <algorithm>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>

#include "cata_assert.h"
#include "debug.h"
#include "point.h"

#if defined(USE_SDL3)
#   include <SDL3_image/SDL_image.h>
#elif defined(_MSC_VER) && defined(USE_VCPKG)
#   include <SDL2/SDL_image.h>
#else
#   include <SDL_image.h>
#endif

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

bool printErrorIf( const bool condition, const char *const message )
{
    if( !condition ) {
        return false;
    }
    dbg( D_ERROR ) << message << ": " << SDL_GetError();
    return true;
}

void throwErrorIf( const bool condition, const char *const message )
{
    if( !condition ) {
        return;
    }
    throw std::runtime_error( std::string( message ) + ": " + SDL_GetError() );
}

#if defined(TILES)

#if SDL_MAJOR_VERSION >= 3
// Helper to convert SDL_Rect* to SDL_FRect for SDL3 render functions.
static SDL_FRect to_frect( const SDL_Rect &r )
{
    return { static_cast<float>( r.x ), static_cast<float>( r.y ),
             static_cast<float>( r.w ), static_cast<float>( r.h ) };
}
static SDL_FPoint to_fpoint( const SDL_Point &p )
{
    return { static_cast<float>( p.x ), static_cast<float>( p.y ) };
}
#endif

// Default texture scale quality applied by CreateTexture/CreateTextureFromSurface.
// Replaces the global SDL_HINT_RENDER_SCALE_QUALITY approach with per-texture mode.
// Initialized to "nearest" to match SDL2's documented default for the hint.
static std::string g_default_texture_scale_quality = "nearest";

static SDL_ScaleMode scale_quality_to_mode( const std::string &quality )
{
    if( quality == "0" || quality == "nearest" || quality == "none" ) {
#if SDL_MAJOR_VERSION >= 3
        return SDL_SCALEMODE_NEAREST;
#else
        return SDL_ScaleModeNearest;
#endif
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_SCALEMODE_LINEAR;
#else
    return SDL_ScaleModeLinear;
#endif
}

void RenderCopy( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture,
                 const SDL_Rect *srcrect, const SDL_Rect *dstrect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to render to a null renderer";
        return;
    }
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to render a null texture";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_FRect fsrc, fdst;
    const SDL_FRect *fsrcp = srcrect ? &( fsrc = to_frect( *srcrect ) ) : nullptr;
    const SDL_FRect *fdstp = dstrect ? &( fdst = to_frect( *dstrect ) ) : nullptr;
    printErrorIf( !SDL_RenderTexture( renderer.get(), texture.get(), fsrcp, fdstp ),
                  "SDL_RenderTexture failed" );
#else
    printErrorIf( SDL_RenderCopy( renderer.get(), texture.get(), srcrect, dstrect ) != 0,
                  "SDL_RenderCopy failed" );
#endif
}

SDL_Texture_Ptr CreateTexture( const SDL_Renderer_Ptr &renderer, Uint32 format, int access,
                               int w, int h )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to create texture with a null renderer";
        return SDL_Texture_Ptr();
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_Texture_Ptr result( SDL_CreateTexture( renderer.get(),
                            static_cast<SDL_PixelFormat>( format ),
                            static_cast<SDL_TextureAccess>( access ), w, h ) );
#else
    SDL_Texture_Ptr result( SDL_CreateTexture( renderer.get(), format, access, w, h ) );
#endif
    printErrorIf( !result, "SDL_CreateTexture failed" );
    if( result ) {
        SDL_SetTextureScaleMode( result.get(),
                                 scale_quality_to_mode( g_default_texture_scale_quality ) );
    }
    return result;
}

SDL_Texture_Ptr CreateTextureFromSurface( const SDL_Renderer_Ptr &renderer,
        const SDL_Surface_Ptr &surface )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to create texture with a null renderer";
        return SDL_Texture_Ptr();
    }
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to create texture from a null surface";
        return SDL_Texture_Ptr();
    }
    SDL_Texture_Ptr result( SDL_CreateTextureFromSurface( renderer.get(), surface.get() ) );
    printErrorIf( !result, "SDL_CreateTextureFromSurface failed" );
    if( result ) {
        SDL_SetTextureScaleMode( result.get(),
                                 scale_quality_to_mode( g_default_texture_scale_quality ) );
    }
    return result;
}

void SetRenderDrawColor( const SDL_Renderer_Ptr &renderer, const Uint8 r, const Uint8 g,
                         const Uint8 b, const Uint8 a )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_SetRenderDrawColor( renderer.get(), r, g, b, a ),
                  "SDL_SetRenderDrawColor failed" );
#else
    printErrorIf( SDL_SetRenderDrawColor( renderer.get(), r, g, b, a ) != 0,
                  "SDL_SetRenderDrawColor failed" );
#endif
}

void RenderDrawPoint( const SDL_Renderer_Ptr &renderer, const point &p )
{
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_RenderPoint( renderer.get(), static_cast<float>( p.x ),
                                    static_cast<float>( p.y ) ),
                  "SDL_RenderPoint failed" );
#else
    printErrorIf( SDL_RenderDrawPoint( renderer.get(), p.x, p.y ) != 0, "SDL_RenderDrawPoint failed" );
#endif
}

void RenderFillRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    if( rect ) {
        SDL_FRect fr = to_frect( *rect );
        printErrorIf( !SDL_RenderFillRect( renderer.get(), &fr ), "SDL_RenderFillRect failed" );
    } else {
        printErrorIf( !SDL_RenderFillRect( renderer.get(), nullptr ), "SDL_RenderFillRect failed" );
    }
#else
    printErrorIf( SDL_RenderFillRect( renderer.get(), rect ) != 0, "SDL_RenderFillRect failed" );
#endif
}

int FillRect( const SDL_Surface_Ptr &surface, const SDL_Rect *const rect, Uint32 color )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to use a null surface";
        return -1;
    }
#if SDL_MAJOR_VERSION >= 3
    const bool ok = SDL_FillSurfaceRect( surface.get(), rect, color );
    printErrorIf( !ok, "SDL_FillSurfaceRect failed" );
    return ok ? 0 : -1;
#else
    const int ret = SDL_FillRect( surface.get(), rect, color );
    printErrorIf( ret != 0, "SDL_FillRect failed" );
    return ret;
#endif
}

void SetTextureBlendMode( const SDL_Texture_Ptr &texture, SDL_BlendMode blendMode )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
    }

#if SDL_MAJOR_VERSION >= 3
    throwErrorIf( !SDL_SetTextureBlendMode( texture.get(), blendMode ),
                  "SDL_SetTextureBlendMode failed" );
#else
    throwErrorIf( SDL_SetTextureBlendMode( texture.get(), blendMode ) != 0,
                  "SDL_SetTextureBlendMode failed" );
#endif
}

void SetTextureBlendMode( const std::shared_ptr<SDL_Texture> &texture, SDL_BlendMode blendMode )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    throwErrorIf( !SDL_SetTextureBlendMode( texture.get(), blendMode ),
                  "SDL_SetTextureBlendMode failed" );
#else
    throwErrorIf( SDL_SetTextureBlendMode( texture.get(), blendMode ) != 0,
                  "SDL_SetTextureBlendMode failed" );
#endif
}

bool SetTextureColorMod( const SDL_Texture_Ptr &texture, Uint32 r, Uint32 g, Uint32 b )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return true;
    }
#if SDL_MAJOR_VERSION >= 3
    return printErrorIf( !SDL_SetTextureColorMod( texture.get(), r, g, b ),
                         "SDL_SetTextureColorMod failed" );
#else
    return printErrorIf( SDL_SetTextureColorMod( texture.get(), r, g, b ) != 0,
                         "SDL_SetTextureColorMod failed" );
#endif
}

bool SetTextureColorMod( const std::shared_ptr<SDL_Texture> &texture, Uint32 r, Uint32 g,
                         Uint32 b )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return true;
    }
#if SDL_MAJOR_VERSION >= 3
    return printErrorIf( !SDL_SetTextureColorMod( texture.get(), r, g, b ),
                         "SDL_SetTextureColorMod failed" );
#else
    return printErrorIf( SDL_SetTextureColorMod( texture.get(), r, g, b ) != 0,
                         "SDL_SetTextureColorMod failed" );
#endif
}

void SetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, const SDL_BlendMode blendMode )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_SetRenderDrawBlendMode( renderer.get(), blendMode ),
                  "SDL_SetRenderDrawBlendMode failed" );
#else
    printErrorIf( SDL_SetRenderDrawBlendMode( renderer.get(), blendMode ) != 0,
                  "SDL_SetRenderDrawBlendMode failed" );
#endif
}

void GetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode &blend_mode )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_GetRenderDrawBlendMode( renderer.get(), &blend_mode ),
                  "SDL_GetRenderDrawBlendMode failed" );
#else
    printErrorIf( SDL_GetRenderDrawBlendMode( renderer.get(), &blend_mode ) != 0,
                  "SDL_GetRenderDrawBlendMode failed" );
#endif
}

SDL_Surface_Ptr load_image( const char *const path )
{
    cata_assert( path );
    SDL_Surface_Ptr result( IMG_Load( path ) );
    if( !result ) {
        throw std::runtime_error( "Could not load image \"" + std::string( path ) + "\": " +
#if SDL_MAJOR_VERSION >= 3
                                  SDL_GetError() );
#else
                                  IMG_GetError() );
#endif
    }
    return result;
}

bool SetRenderTarget( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return false;
    }
    // a null texture is fine for SDL (resets to default target)
#if SDL_MAJOR_VERSION >= 3
    const bool failed = printErrorIf( !SDL_SetRenderTarget( renderer.get(), texture.get() ),
                                      "SDL_SetRenderTarget failed" );
#else
    const bool failed = printErrorIf( SDL_SetRenderTarget( renderer.get(), texture.get() ) != 0,
                                      "SDL_SetRenderTarget failed" );
#endif
    return !failed;
}

void RenderClear( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_RenderClear( renderer.get() ), "SDL_RenderClear failed" );
#else
    printErrorIf( SDL_RenderClear( renderer.get() ) != 0, "SDL_RenderClear failed" );
#endif
}

SDL_Surface_Ptr CreateRGBSurface( const Uint32 flags, const int width, const int height,
                                  const int depth, const Uint32 Rmask, const Uint32 Gmask, const Uint32 Bmask, const Uint32 Amask )
{
#if SDL_MAJOR_VERSION >= 3
    ( void )flags;
    SDL_PixelFormat fmt = SDL_GetPixelFormatForMasks( depth, Rmask, Gmask, Bmask, Amask );
    SDL_Surface_Ptr surface( SDL_CreateSurface( width, height, fmt ) );
#else
    SDL_Surface_Ptr surface( SDL_CreateRGBSurface( flags, width, height, depth, Rmask, Gmask, Bmask,
                             Amask ) );
#endif
    throwErrorIf( !surface, "Failed to create surface" );
    return surface;
}

void SetTextureAlphaMod( const SDL_Texture_Ptr &texture, const Uint8 alpha )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_SetTextureAlphaMod( texture.get(), alpha ),
                  "SDL_SetTextureAlphaMod failed" );
#else
    printErrorIf( SDL_SetTextureAlphaMod( texture.get(), alpha ) != 0,
                  "SDL_SetTextureAlphaMod failed" );
#endif
}

void SetTextureAlphaMod( const std::shared_ptr<SDL_Texture> &texture, const Uint8 alpha )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_SetTextureAlphaMod( texture.get(), alpha ),
                  "SDL_SetTextureAlphaMod failed" );
#else
    printErrorIf( SDL_SetTextureAlphaMod( texture.get(), alpha ) != 0,
                  "SDL_SetTextureAlphaMod failed" );
#endif
}

void RenderCopyEx( const SDL_Renderer_Ptr &renderer, SDL_Texture *const texture,
                   const SDL_Rect *const srcrect, const SDL_Rect *const dstrect,
                   const double angle, const SDL_Point *const center,
                   const CataFlipMode flip )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to render to a null renderer";
        return;
    }
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to render a null texture";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_FRect fsrc, fdst;
    SDL_FPoint fcenter;
    const SDL_FRect *fsrcp = srcrect ? &( fsrc = to_frect( *srcrect ) ) : nullptr;
    const SDL_FRect *fdstp = dstrect ? &( fdst = to_frect( *dstrect ) ) : nullptr;
    const SDL_FPoint *fcenterp = center ? &( fcenter = to_fpoint( *center ) ) : nullptr;
    printErrorIf( !SDL_RenderTextureRotated( renderer.get(), texture, fsrcp, fdstp, angle,
                  fcenterp, flip ),
                  "SDL_RenderTextureRotated failed" );
#else
    printErrorIf( SDL_RenderCopyEx( renderer.get(), texture, srcrect, dstrect, angle, center,
                                    flip ) != 0,
                  "SDL_RenderCopyEx failed" );
#endif
}

void RenderSetClipRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_SetRenderClipRect( renderer.get(), rect ),
                  "SDL_SetRenderClipRect failed" );
#else
    printErrorIf( SDL_RenderSetClipRect( renderer.get(), rect ) != 0,
                  "SDL_RenderSetClipRect failed" );
#endif
}

void RenderGetClipRect( const SDL_Renderer_Ptr &renderer, SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_GetRenderClipRect( renderer.get(), rect );
#else
    SDL_RenderGetClipRect( renderer.get(), rect );
#endif
}

bool RenderIsClipEnabled( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return false;
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_RenderClipEnabled( renderer.get() );
#else
    return SDL_RenderIsClipEnabled( renderer.get() );
#endif
}

int BlitSurface( const SDL_Surface_Ptr &src, const SDL_Rect *const srcrect,
                 const SDL_Surface_Ptr &dst, SDL_Rect *const dstrect )
{
    if( !src ) {
        dbg( D_ERROR ) << "Tried to blit from a null surface";
        return -1;
    }
    if( !dst ) {
        dbg( D_ERROR ) << "Tried to blit to a null surface";
        return -1;
    }
#if SDL_MAJOR_VERSION >= 3
    // SDL3 returns bool (true=success); convert to SDL2 convention (0=success).
    return SDL_BlitSurface( src.get(), srcrect, dst.get(), dstrect ) ? 0 : -1;
#else
    return SDL_BlitSurface( src.get(), srcrect, dst.get(), dstrect );
#endif
}

Uint32 MapRGB( const SDL_Surface_Ptr &surface, const Uint8 r, const Uint8 g, const Uint8 b )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to map color on a null surface";
        return 0;
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_MapSurfaceRGB( surface.get(), r, g, b );
#else
    return SDL_MapRGB( surface->format, r, g, b );
#endif
}

Uint32 MapRGBA( const SDL_Surface_Ptr &surface, const Uint8 r, const Uint8 g, const Uint8 b,
                const Uint8 a )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to map color on a null surface";
        return 0;
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_MapSurfaceRGBA( surface.get(), r, g, b, a );
#else
    return SDL_MapRGBA( surface->format, r, g, b, a );
#endif
}

void GetRGBA( const Uint32 pixel, const SDL_Surface_Ptr &surface, Uint8 &r, Uint8 &g, Uint8 &b,
              Uint8 &a )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to get RGBA from a null surface";
        r = g = b = a = 0;
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_GetRGBA( pixel, SDL_GetPixelFormatDetails( surface->format ),
                 SDL_GetSurfacePalette( surface.get() ), &r, &g, &b, &a );
#else
    SDL_GetRGBA( pixel, surface->format, &r, &g, &b, &a );
#endif
}

int SetColorKey( const SDL_Surface_Ptr &surface, const int flag, const Uint32 key )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set colorkey on a null surface";
        return -1;
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_SetSurfaceColorKey( surface.get(), flag, key ) ? 0 : -1;
#else
    return SDL_SetColorKey( surface.get(), flag, key );
#endif
}

int SetSurfaceRLE( const SDL_Surface_Ptr &surface, const int flag )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set RLE on a null surface";
        return -1;
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_SetSurfaceRLE( surface.get(), flag ) ? 0 : -1;
#else
    return SDL_SetSurfaceRLE( surface.get(), flag );
#endif
}

int SetSurfaceBlendMode( const SDL_Surface_Ptr &surface, const SDL_BlendMode blendMode )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set blend mode on a null surface";
        return -1;
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_SetSurfaceBlendMode( surface.get(), blendMode ) ? 0 : -1;
#else
    return SDL_SetSurfaceBlendMode( surface.get(), blendMode );
#endif
}

SDL_Surface_Ptr ConvertSurfaceFormat( const SDL_Surface_Ptr &surface, const Uint32 pixel_format )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to convert a null surface";
        return SDL_Surface_Ptr();
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_Surface_Ptr result( SDL_ConvertSurface( surface.get(),
                            static_cast<SDL_PixelFormat>( pixel_format ) ) );
    printErrorIf( !result, "SDL_ConvertSurface failed" );
#else
    SDL_Surface_Ptr result( SDL_ConvertSurfaceFormat( surface.get(), pixel_format, 0 ) );
    printErrorIf( !result, "SDL_ConvertSurfaceFormat failed" );
#endif
    return result;
}

int LockSurface( const SDL_Surface_Ptr &surface )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to lock a null surface";
        return -1;
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_LockSurface( surface.get() ) ? 0 : -1;
#else
    return SDL_LockSurface( surface.get() );
#endif
}

void UnlockSurface( const SDL_Surface_Ptr &surface )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to unlock a null surface";
        return;
    }
    SDL_UnlockSurface( surface.get() );
}

Uint32 GetSurfacePixelFormat( const SDL_Surface_Ptr &surface )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to get pixel format of a null surface";
        return 0;
    }
#if SDL_MAJOR_VERSION >= 3
    // SDL3: surface->format is the enum directly (no intermediate struct).
    return surface->format;
#else
    return surface->format->format;
#endif
}

TTF_Font_Ptr OpenFontIndex( const char *const file, const int ptsize, const int64_t index )
{
#if SDL_MAJOR_VERSION >= 3
    if( index == 0 ) {
        TTF_Font_Ptr result( TTF_OpenFont( file, static_cast<float>( ptsize ) ) );
        return result;
    }
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty( props, TTF_PROP_FONT_CREATE_FILENAME_STRING, file );
    SDL_SetFloatProperty( props, TTF_PROP_FONT_CREATE_SIZE_FLOAT, static_cast<float>( ptsize ) );
    SDL_SetNumberProperty( props, TTF_PROP_FONT_CREATE_FACE_NUMBER, index );
    TTF_Font_Ptr result( TTF_OpenFontWithProperties( props ) );
    SDL_DestroyProperties( props );
    return result;
#else
    // NOLINTNEXTLINE(cata-no-long)
    TTF_Font_Ptr result( TTF_OpenFontIndex( file, ptsize, static_cast<long>( index ) ) );
    // Callers check the result and may call SDL_GetError themselves.
    return result;
#endif
}

const char *FontFaceStyleName( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query style name of a null font";
        return nullptr;
    }
#if SDL_MAJOR_VERSION >= 3
    return TTF_GetFontStyleName( font.get() );
#else
    return TTF_FontFaceStyleName( font.get() );
#endif
}

int FontFaces( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query face count of a null font";
        return 0;
    }
#if SDL_MAJOR_VERSION >= 3
    return TTF_GetNumFontFaces( font.get() );
#else
    return TTF_FontFaces( font.get() );
#endif
}

int FontHeight( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query height of a null font";
        return 0;
    }
#if SDL_MAJOR_VERSION >= 3
    return TTF_GetFontHeight( font.get() );
#else
    return TTF_FontHeight( font.get() );
#endif
}

void SetFontStyle( const TTF_Font_Ptr &font, const int style )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to set style on a null font";
        return;
    }
    TTF_SetFontStyle( font.get(), style );
}

SDL_Surface_Ptr RenderUTF8_Solid( const TTF_Font_Ptr &font, const char *const text,
                                  const SDL_Color fg )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to render with a null font";
        return SDL_Surface_Ptr();
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_Surface_Ptr( TTF_RenderText_Solid( font.get(), text, 0, fg ) );
#else
    return SDL_Surface_Ptr( TTF_RenderUTF8_Solid( font.get(), text, fg ) );
#endif
}

SDL_Surface_Ptr RenderUTF8_Blended( const TTF_Font_Ptr &font, const char *const text,
                                    const SDL_Color fg )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to render with a null font";
        return SDL_Surface_Ptr();
    }
#if SDL_MAJOR_VERSION >= 3
    return SDL_Surface_Ptr( TTF_RenderText_Blended( font.get(), text, 0, fg ) );
#else
    return SDL_Surface_Ptr( TTF_RenderUTF8_Blended( font.get(), text, fg ) );
#endif
}

bool CanRenderGlyph( const TTF_Font_Ptr &font, const Uint32 ch )
{
    if( !font ) {
        return false;
    }
#if SDL_MAJOR_VERSION >= 3
    return TTF_FontHasGlyph( font.get(), ch );
#else
    // SDL2: TTF_GlyphIsProvided only takes Uint16, so clamp for now.
    if( ch > 0xFFFF ) {
        return false;
    }
    return TTF_GlyphIsProvided( font.get(), static_cast<Uint16>( ch ) ) != 0;
#endif
}

int GetNumVideoDisplays()
{
#if SDL_MAJOR_VERSION >= 3
    int count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays( &count );
    SDL_free( displays );
    return count;
#else
    return SDL_GetNumVideoDisplays();
#endif
}

const char *GetDisplayName( const int displayIndex )
{
#if SDL_MAJOR_VERSION >= 3
    int count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays( &count );
    if( !displays || displayIndex < 0 || displayIndex >= count ) {
        SDL_free( displays );
        return "";
    }
    const char *name = SDL_GetDisplayName( displays[displayIndex] );
    SDL_free( displays );
    return name ? name : "";
#else
    return SDL_GetDisplayName( displayIndex );
#endif
}

bool GetDesktopDisplayMode( const int displayIndex, SDL_DisplayMode *mode )
{
    if( !mode ) {
        return false;
    }
#if SDL_MAJOR_VERSION >= 3
    int count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays( &count );
    if( !displays || displayIndex < 0 || displayIndex >= count ) {
        SDL_free( displays );
        return false;
    }
    const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode( displays[displayIndex] );
    SDL_free( displays );
    if( !dm ) {
        return false;
    }
    *mode = *dm;
    return true;
#else
    return SDL_GetDesktopDisplayMode( displayIndex, mode ) == 0;
#endif
}


int GetNumRenderDrivers()
{
    // SDL3: same name, no change needed.
    return SDL_GetNumRenderDrivers();
}

const char *GetRenderDriverName( const int index )
{
#if SDL_MAJOR_VERSION >= 3
    const char *name = SDL_GetRenderDriver( index );
    return name ? name : "";
#else
    static SDL_RendererInfo info;
    if( SDL_GetRenderDriverInfo( index, &info ) != 0 ) {
        return "";
    }
    return info.name;
#endif
}


const char *GetRendererName( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return "";
    }
#if SDL_MAJOR_VERSION >= 3
    const char *name = SDL_GetRendererName( renderer.get() );
    return name ? name : "";
#else
    static SDL_RendererInfo info;
    if( SDL_GetRendererInfo( renderer.get(), &info ) != 0 ) {
        return "";
    }
    return info.name;
#endif
}

bool IsRendererSoftware( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return false;
    }
#if SDL_MAJOR_VERSION >= 3
    const char *name = SDL_GetRendererName( renderer.get() );
    return name && std::string( name ) == "software";
#else
    SDL_RendererInfo info;
    if( SDL_GetRendererInfo( renderer.get(), &info ) != 0 ) {
        return false;
    }
    return ( info.flags & SDL_RENDERER_SOFTWARE ) != 0;
#endif
}

bool GetRendererMaxTextureSize( const SDL_Renderer_Ptr &renderer, int *max_w, int *max_h )
{
    if( !renderer ) {
        return false;
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_PropertiesID props = SDL_GetRendererProperties( renderer.get() );
    if( !props ) {
        return false;
    }
    if( max_w ) {
        *max_w = static_cast<int>( SDL_GetNumberProperty( props,
                                   SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0 ) );
    }
    if( max_h ) {
        *max_h = static_cast<int>( SDL_GetNumberProperty( props,
                                   SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0 ) );
    }
    return true;
#else
    SDL_RendererInfo info;
    if( SDL_GetRendererInfo( renderer.get(), &info ) != 0 ) {
        return false;
    }
    if( max_w ) {
        *max_w = info.max_texture_width;
    }
    if( max_h ) {
        *max_h = info.max_texture_height;
    }
    return true;
#endif
}


void RenderPresent( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_RenderPresent( renderer.get() ), "SDL_RenderPresent failed" );
#else
    SDL_RenderPresent( renderer.get() );
#endif
}

void RenderDrawRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect )
{
    if( !renderer ) {
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    if( rect ) {
        SDL_FRect fr = to_frect( *rect );
        printErrorIf( !SDL_RenderRect( renderer.get(), &fr ), "SDL_RenderRect failed" );
    } else {
        printErrorIf( !SDL_RenderRect( renderer.get(), nullptr ), "SDL_RenderRect failed" );
    }
#else
    printErrorIf( SDL_RenderDrawRect( renderer.get(), rect ) != 0,
                  "SDL_RenderDrawRect failed" );
#endif
}

void RenderGetViewport( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect )
{
    if( !renderer ) {
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_GetRenderViewport( renderer.get(), rect );
#else
    SDL_RenderGetViewport( renderer.get(), rect );
#endif
}

void RenderSetLogicalSize( const SDL_Renderer_Ptr &renderer, const int w, const int h )
{
    if( !renderer ) {
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_SetRenderLogicalPresentation( renderer.get(), w, h,
                  SDL_LOGICAL_PRESENTATION_LETTERBOX ),
                  "SDL_SetRenderLogicalPresentation failed" );
#else
    printErrorIf( SDL_RenderSetLogicalSize( renderer.get(), w, h ) != 0,
                  "SDL_RenderSetLogicalSize failed" );
#endif
}

void RenderSetScale( const SDL_Renderer_Ptr &renderer, const float scaleX, const float scaleY )
{
    if( !renderer ) {
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_SetRenderScale( renderer.get(), scaleX, scaleY ),
                  "SDL_SetRenderScale failed" );
#else
    printErrorIf( SDL_RenderSetScale( renderer.get(), scaleX, scaleY ) != 0,
                  "SDL_RenderSetScale failed" );
#endif
}

bool RenderReadPixels( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect,
                       const Uint32 format, void *pixels, const int pitch )
{
    if( !renderer ) {
        return false;
    }
#if SDL_MAJOR_VERSION >= 3
    ( void )format;
    SDL_Surface *surf = SDL_RenderReadPixels( renderer.get(), rect );
    if( !surf ) {
        return false;
    }
    const int copy_h = surf->h;
    const int copy_pitch = std::min( pitch, surf->pitch );
    for( int row = 0; row < copy_h; row++ ) {
        std::memcpy( static_cast<Uint8 *>( pixels ) + row * pitch,
                     static_cast<const Uint8 *>( surf->pixels ) + row * surf->pitch,
                     copy_pitch );
    }
    SDL_DestroySurface( surf );
    return true;
#else
    return SDL_RenderReadPixels( renderer.get(), rect, format, pixels, pitch ) == 0;
#endif
}

void GetRendererOutputSize( const SDL_Renderer_Ptr &renderer, int *w, int *h )
{
    if( !renderer ) {
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    printErrorIf( !SDL_GetCurrentRenderOutputSize( renderer.get(), w, h ),
                  "SDL_GetCurrentRenderOutputSize failed" );
#else
    printErrorIf( SDL_GetRendererOutputSize( renderer.get(), w, h ) != 0,
                  "SDL_GetRendererOutputSize failed" );
#endif
}


uint32_t GetTicks()
{
    // SDL3: returns Uint64. Static cast is safe for ~49 days of uptime.
    return static_cast<uint32_t>( SDL_GetTicks() );
}


bool IsCursorVisible()
{
#if SDL_MAJOR_VERSION >= 3
    return SDL_CursorVisible();
#else
    return SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE;
#endif
}

void ShowCursor()
{
#if SDL_MAJOR_VERSION >= 3
    SDL_ShowCursor();
#else
    SDL_ShowCursor( SDL_ENABLE );
#endif
}

void HideCursor()
{
#if SDL_MAJOR_VERSION >= 3
    SDL_HideCursor();
#else
    SDL_ShowCursor( SDL_DISABLE );
#endif
}


std::string GetClipboardText()
{
#if SDL_MAJOR_VERSION >= 3
    const char *clip = SDL_GetClipboardText();
    return clip ? std::string( clip ) : std::string();
#else
    char *clip = SDL_GetClipboardText();
    if( !clip ) {
        return {};
    }
    std::string result( clip );
    SDL_free( clip );
    return result;
#endif
}

bool SetClipboardText( const std::string &text )
{
#if SDL_MAJOR_VERSION >= 3
    return SDL_SetClipboardText( text.c_str() );
#else
    return SDL_SetClipboardText( text.c_str() ) == 0;
#endif
}


Uint32 GetMouseState( int *x, int *y )
{
#if SDL_MAJOR_VERSION >= 3
    float fx = 0, fy = 0;
    Uint32 buttons = SDL_GetMouseState( &fx, &fy );
    if( x ) {
        *x = static_cast<int>( fx );
    }
    if( y ) {
        *y = static_cast<int>( fy );
    }
    return buttons;
#else
    return SDL_GetMouseState( x, y );
#endif
}


bool IsScancodePressed( const SDL_Scancode scancode )
{
#if SDL_MAJOR_VERSION >= 3
    const bool *state = SDL_GetKeyboardState( nullptr );
    if( !state ) {
        return false;
    }
    return state[scancode];
#else
    const Uint8 *state = SDL_GetKeyboardState( nullptr );
    if( !state ) {
        return false;
    }
    return state[scancode] != 0;
#endif
}


void GetWindowSize( SDL_Window *window, int *w, int *h )
{
    if( !window ) {
        return;
    }
    SDL_GetWindowSize( window, w, h );
}

void GetWindowSizeInPixels( SDL_Window *window, int *w, int *h )
{
    if( !window ) {
        return;
    }
#if SDL_MAJOR_VERSION >= 3
    SDL_GetWindowSizeInPixels( window, w, h );
#elif SDL_VERSION_ATLEAST(2,26,0)
    SDL_GetWindowSizeInPixels( window, w, h );
#else
    // Fallback for old SDL2: logical and pixel size are the same.
    SDL_GetWindowSize( window, w, h );
#endif
}


void SetTextureScaleQuality( const SDL_Texture_Ptr &texture, const std::string &quality )
{
    if( !texture ) {
        return;
    }
    SDL_SetTextureScaleMode( texture.get(), scale_quality_to_mode( quality ) );
}

void SetDefaultTextureScaleQuality( const std::string &quality )
{
    g_default_texture_scale_quality = quality;
}


void StartTextInput( SDL_Window *window )
{
#if SDL_MAJOR_VERSION >= 3
    SDL_StartTextInput( window );
#else
    ( void )window;
    SDL_StartTextInput();
#endif
}

void StopTextInput( SDL_Window *window )
{
#if SDL_MAJOR_VERSION >= 3
    SDL_StopTextInput( window );
#else
    ( void )window;
    SDL_StopTextInput();
#endif
}

bool IsTextInputActive( SDL_Window *window )
{
#if SDL_MAJOR_VERSION >= 3
    return SDL_TextInputActive( window );
#else
    ( void )window;
    return SDL_IsTextInputActive();
#endif
}


bool IsWindowEvent( const SDL_Event &ev )
{
#if SDL_MAJOR_VERSION >= 3
    return ev.type >= SDL_EVENT_WINDOW_FIRST && ev.type <= SDL_EVENT_WINDOW_LAST;
#else
    return ev.type == SDL_WINDOWEVENT;
#endif
}

Uint32 GetWindowEventID( const SDL_Event &ev )
{
#if SDL_MAJOR_VERSION >= 3
    // SDL3: window event type IS the top-level event type.
    return ev.type;
#else
    return ev.window.event;
#endif
}

CataKeysym GetKeysym( const SDL_Event &ev )
{
#if SDL_MAJOR_VERSION >= 3
    return { ev.key.key, ev.key.mod, ev.key.scancode };
#else
    return { ev.key.keysym.sym, ev.key.keysym.mod, ev.key.keysym.scancode };
#endif
}


SDL_Window_Ptr CreateGameWindow( const char *title, const int display, const int w, const int h,
                                 const Uint32 flags )
{
#if SDL_MAJOR_VERSION >= 3
    ( void )display;
    // SDL3: SDL_CreateWindow takes (title, w, h, flags) -- no position params.
    SDL_Window_Ptr result( SDL_CreateWindow( title, w, h, flags ) );
    printErrorIf( !result, "SDL_CreateWindow failed" );
    return result;
#else
    SDL_Window_Ptr result( SDL_CreateWindow( title,
                           SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
                           SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
                           w, h, flags ) );
    printErrorIf( !result, "SDL_CreateWindow failed" );
    return result;
#endif
}


bool SetWindowFullscreen( SDL_Window *window, const FullscreenMode mode )
{
    if( !window ) {
        return false;
    }
#if SDL_MAJOR_VERSION >= 3
    switch( mode ) {
        case FullscreenMode::windowed:
            return SDL_SetWindowFullscreen( window, false );
        case FullscreenMode::fullscreen_desktop:
            return SDL_SetWindowFullscreen( window, true );
        case FullscreenMode::fullscreen_exclusive: {
            SDL_DisplayID disp = SDL_GetDisplayForWindow( window );
            const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode( disp );
            if( dm ) {
                SDL_SetWindowFullscreenMode( window, dm );
            }
            return SDL_SetWindowFullscreen( window, true );
        }
    }
    return false;
#else
    Uint32 flags = 0;
    switch( mode ) {
        case FullscreenMode::windowed:
            flags = 0;
            break;
        case FullscreenMode::fullscreen_desktop:
            flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
            break;
        case FullscreenMode::fullscreen_exclusive:
            flags = SDL_WINDOW_FULLSCREEN;
            break;
    }
    return SDL_SetWindowFullscreen( window, flags ) == 0;
#endif
}


void RestoreWindow( SDL_Window *window )
{
    if( window ) {
        SDL_RestoreWindow( window );
#if SDL_MAJOR_VERSION >= 3
        SDL_SyncWindow( window );
#endif
    }
}

void SetWindowSize( SDL_Window *window, const int w, const int h )
{
    if( window ) {
        SDL_SetWindowSize( window, w, h );
#if SDL_MAJOR_VERSION >= 3
        SDL_SyncWindow( window );
#endif
    }
}

void SetWindowMinimumSize( SDL_Window *window, const int w, const int h )
{
    if( window ) {
        SDL_SetWindowMinimumSize( window, w, h );
    }
}

void SetWindowTitle( SDL_Window *window, const char *title )
{
    if( window ) {
        SDL_SetWindowTitle( window, title );
    }
}


SDL_Renderer_Ptr CreateRenderer( const SDL_Window_Ptr &window, const char *driver_name,
                                 const bool software, const bool vsync )
{
    if( !window ) {
        dbg( D_ERROR ) << "Tried to create renderer with a null window";
        return SDL_Renderer_Ptr();
    }

#if SDL_MAJOR_VERSION >= 3
    const char *name = nullptr;
    if( software ) {
        name = "software";
    } else if( driver_name && driver_name[0] != '\0' ) {
        name = driver_name;
    }
    SDL_Renderer_Ptr result( SDL_CreateRenderer( window.get(), name ) );
    if( result && vsync && !software ) {
        SDL_SetRenderVSync( result.get(), 1 );
    }
    printErrorIf( !result, "SDL_CreateRenderer failed" );
    return result;
#else
    int driver_index = -1;
    Uint32 flags = SDL_RENDERER_TARGETTEXTURE;

    if( software ) {
        flags |= SDL_RENDERER_SOFTWARE;
    } else {
        flags |= SDL_RENDERER_ACCELERATED;
        if( vsync ) {
            flags |= SDL_RENDERER_PRESENTVSYNC;
        }
        // Find driver index by name if specified
        if( driver_name && driver_name[0] != '\0' ) {
            const int num = SDL_GetNumRenderDrivers();
            for( int i = 0; i < num; i++ ) {
                SDL_RendererInfo ri;
                if( SDL_GetRenderDriverInfo( i, &ri ) == 0 && ri.name == std::string( driver_name ) ) {
                    driver_index = i;
                    break;
                }
            }
        }
    }

    SDL_Renderer_Ptr result( SDL_CreateRenderer( window.get(), driver_index, flags ) );
    printErrorIf( !result, "SDL_CreateRenderer failed" );
    return result;
#endif
}


void ConvertEventCoordinates( const SDL_Renderer_Ptr &renderer, SDL_Event *event )
{
#if SDL_MAJOR_VERSION >= 3
    if( renderer && event ) {
        SDL_ConvertEventToRenderCoordinates( renderer.get(), event );
    }
#else
    ( void )renderer;
    ( void )event;
    // SDL2: no-op. SDL_RenderSetLogicalSize auto-scales mouse/touch events.
#endif
}


float GetFingerX( const SDL_Event &ev, const int windowWidth )
{
#if SDL_MAJOR_VERSION >= 3
    ( void )windowWidth;
    return ev.tfinger.x;
#else
    return ev.tfinger.x * windowWidth;
#endif
}

float GetFingerY( const SDL_Event &ev, const int windowHeight )
{
#if SDL_MAJOR_VERSION >= 3
    ( void )windowHeight;
    return ev.tfinger.y;
#else
    return ev.tfinger.y * windowHeight;
#endif
}

#endif // defined(TILES)
#endif // defined(TILES) || defined(SDL_SOUND)
