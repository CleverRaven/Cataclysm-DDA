#if defined(TILES) || defined(SDL_SOUND)

#include "sdl_wrappers.h"

#include <ostream>
#include <stdexcept>
#include <string>

#include "cata_assert.h"
#include "debug.h"
#include "point.h"

#if defined(_MSC_VER) && defined(USE_VCPKG)
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
    printErrorIf( SDL_RenderCopy( renderer.get(), texture.get(), srcrect, dstrect ) != 0,
                  "SDL_RenderCopy failed" );
}

SDL_Texture_Ptr CreateTexture( const SDL_Renderer_Ptr &renderer, Uint32 format, int access,
                               int w, int h )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to create texture with a null renderer";
        return SDL_Texture_Ptr();
    }
    SDL_Texture_Ptr result( SDL_CreateTexture( renderer.get(), format, access, w, h ) );
    printErrorIf( !result, "SDL_CreateTexture failed" );
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
    return result;
}

void SetRenderDrawColor( const SDL_Renderer_Ptr &renderer, const Uint8 r, const Uint8 g,
                         const Uint8 b, const Uint8 a )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( SDL_SetRenderDrawColor( renderer.get(), r, g, b, a ) != 0,
                  "SDL_SetRenderDrawColor failed" );
}

void RenderDrawPoint( const SDL_Renderer_Ptr &renderer, const point &p )
{
    printErrorIf( SDL_RenderDrawPoint( renderer.get(), p.x, p.y ) != 0, "SDL_RenderDrawPoint failed" );
}

void RenderFillRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( SDL_RenderFillRect( renderer.get(), rect ) != 0, "SDL_RenderFillRect failed" );
}

int FillRect( const SDL_Surface_Ptr &surface, const SDL_Rect *const rect, Uint32 color )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to use a null surface";
        return -1;
    }
    const int ret = SDL_FillRect( surface.get(), rect, color );
    printErrorIf( ret != 0, "SDL_FillRect failed" );
    return ret;
}

void SetTextureBlendMode( const SDL_Texture_Ptr &texture, SDL_BlendMode blendMode )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
    }

    throwErrorIf( SDL_SetTextureBlendMode( texture.get(), blendMode ) != 0,
                  "SDL_SetTextureBlendMode failed" );
}

void SetTextureBlendMode( const std::shared_ptr<SDL_Texture> &texture, SDL_BlendMode blendMode )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
    throwErrorIf( SDL_SetTextureBlendMode( texture.get(), blendMode ) != 0,
                  "SDL_SetTextureBlendMode failed" );
}

bool SetTextureColorMod( const SDL_Texture_Ptr &texture, Uint32 r, Uint32 g, Uint32 b )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return true;
    }
    return printErrorIf( SDL_SetTextureColorMod( texture.get(), r, g, b ) != 0,
                         "SDL_SetTextureColorMod failed" );
}

bool SetTextureColorMod( const std::shared_ptr<SDL_Texture> &texture, Uint32 r, Uint32 g,
                         Uint32 b )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return true;
    }
    return printErrorIf( SDL_SetTextureColorMod( texture.get(), r, g, b ) != 0,
                         "SDL_SetTextureColorMod failed" );
}

void SetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, const SDL_BlendMode blendMode )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( SDL_SetRenderDrawBlendMode( renderer.get(), blendMode ) != 0,
                  "SDL_SetRenderDrawBlendMode failed" );
}

void GetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode &blend_mode )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( SDL_GetRenderDrawBlendMode( renderer.get(), &blend_mode ) != 0,
                  "SDL_GetRenderDrawBlendMode failed" );
}

SDL_Surface_Ptr load_image( const char *const path )
{
    cata_assert( path );
    SDL_Surface_Ptr result( IMG_Load( path ) );
    if( !result ) {
        throw std::runtime_error( "Could not load image \"" + std::string( path ) + "\": " +
                                  IMG_GetError() );
    }
    return result;
}

void SetRenderTarget( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    // a null texture is fine for SDL
    printErrorIf( SDL_SetRenderTarget( renderer.get(), texture.get() ) != 0,
                  "SDL_SetRenderTarget failed" );
}

void RenderClear( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( SDL_RenderClear( renderer.get() ) != 0, "SDL_RenderClear failed" );
}

SDL_Surface_Ptr CreateRGBSurface( const Uint32 flags, const int width, const int height,
                                  const int depth, const Uint32 Rmask, const Uint32 Gmask, const Uint32 Bmask, const Uint32 Amask )
{
    SDL_Surface_Ptr surface( SDL_CreateRGBSurface( flags, width, height, depth, Rmask, Gmask, Bmask,
                             Amask ) );
    throwErrorIf( !surface, "Failed to create surface" );
    return surface;
}

void SetTextureAlphaMod( const SDL_Texture_Ptr &texture, const Uint8 alpha )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
    printErrorIf( SDL_SetTextureAlphaMod( texture.get(), alpha ) != 0,
                  "SDL_SetTextureAlphaMod failed" );
}

void SetTextureAlphaMod( const std::shared_ptr<SDL_Texture> &texture, const Uint8 alpha )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
    printErrorIf( SDL_SetTextureAlphaMod( texture.get(), alpha ) != 0,
                  "SDL_SetTextureAlphaMod failed" );
}

void RenderCopyEx( const SDL_Renderer_Ptr &renderer, SDL_Texture *const texture,
                   const SDL_Rect *const srcrect, const SDL_Rect *const dstrect,
                   const double angle, const SDL_Point *const center,
                   const SDL_RendererFlip flip )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to render to a null renderer";
        return;
    }
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to render a null texture";
        return;
    }
    printErrorIf( SDL_RenderCopyEx( renderer.get(), texture, srcrect, dstrect, angle, center,
                                    flip ) != 0,
                  "SDL_RenderCopyEx failed" );
}

void RenderSetClipRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( SDL_RenderSetClipRect( renderer.get(), rect ) != 0,
                  "SDL_RenderSetClipRect failed" );
}

void RenderGetClipRect( const SDL_Renderer_Ptr &renderer, SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    SDL_RenderGetClipRect( renderer.get(), rect );
}

bool RenderIsClipEnabled( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return false;
    }
    return SDL_RenderIsClipEnabled( renderer.get() ) == SDL_TRUE;
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
    return SDL_BlitSurface( src.get(), srcrect, dst.get(), dstrect );
}

Uint32 MapRGB( const SDL_Surface_Ptr &surface, const Uint8 r, const Uint8 g, const Uint8 b )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to map color on a null surface";
        return 0;
    }
    return SDL_MapRGB( surface->format, r, g, b );
}

Uint32 MapRGBA( const SDL_Surface_Ptr &surface, const Uint8 r, const Uint8 g, const Uint8 b,
                const Uint8 a )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to map color on a null surface";
        return 0;
    }
    return SDL_MapRGBA( surface->format, r, g, b, a );
}

void GetRGBA( const Uint32 pixel, const SDL_Surface_Ptr &surface, Uint8 &r, Uint8 &g, Uint8 &b,
              Uint8 &a )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to get RGBA from a null surface";
        r = g = b = a = 0;
        return;
    }
    SDL_GetRGBA( pixel, surface->format, &r, &g, &b, &a );
}

int SetColorKey( const SDL_Surface_Ptr &surface, const int flag, const Uint32 key )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set colorkey on a null surface";
        return -1;
    }
    return SDL_SetColorKey( surface.get(), flag, key );
}

int SetSurfaceRLE( const SDL_Surface_Ptr &surface, const int flag )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set RLE on a null surface";
        return -1;
    }
    return SDL_SetSurfaceRLE( surface.get(), flag );
}

int SetSurfaceBlendMode( const SDL_Surface_Ptr &surface, const SDL_BlendMode blendMode )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set blend mode on a null surface";
        return -1;
    }
    return SDL_SetSurfaceBlendMode( surface.get(), blendMode );
}

SDL_Surface_Ptr ConvertSurfaceFormat( const SDL_Surface_Ptr &surface, const Uint32 pixel_format )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to convert a null surface";
        return SDL_Surface_Ptr();
    }
    SDL_Surface_Ptr result( SDL_ConvertSurfaceFormat( surface.get(), pixel_format, 0 ) );
    printErrorIf( !result, "SDL_ConvertSurfaceFormat failed" );
    return result;
}

int LockSurface( const SDL_Surface_Ptr &surface )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to lock a null surface";
        return -1;
    }
    return SDL_LockSurface( surface.get() );
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
    // SDL3: surface->format is the enum directly (no intermediate struct).
    return surface->format->format;
}

TTF_Font_Ptr OpenFontIndex( const char *const file, const int ptsize, const int64_t index )
{
    // NOLINTNEXTLINE(cata-no-long)
    TTF_Font_Ptr result( TTF_OpenFontIndex( file, ptsize, static_cast<long>( index ) ) );
    // Callers check the result and may call TTF_GetError themselves.
    return result;
}

const char *FontFaceStyleName( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query style name of a null font";
        return nullptr;
    }
    return TTF_FontFaceStyleName( font.get() );
}

int FontFaces( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query face count of a null font";
        return 0;
    }
    return TTF_FontFaces( font.get() );
}

int FontHeight( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query height of a null font";
        return 0;
    }
    return TTF_FontHeight( font.get() );
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
    return SDL_Surface_Ptr( TTF_RenderUTF8_Solid( font.get(), text, fg ) );
}

SDL_Surface_Ptr RenderUTF8_Blended( const TTF_Font_Ptr &font, const char *const text,
                                    const SDL_Color fg )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to render with a null font";
        return SDL_Surface_Ptr();
    }
    return SDL_Surface_Ptr( TTF_RenderUTF8_Blended( font.get(), text, fg ) );
}

bool CanRenderGlyph( const TTF_Font_Ptr &font, const Uint32 ch )
{
    if( !font ) {
        return false;
    }
    // SDL2: TTF_GlyphIsProvided only takes Uint16, so clamp for now.
    // SDL3_ttf will use Uint32 natively via glyph metrics.
    if( ch > 0xFFFF ) {
        return false;
    }
    return TTF_GlyphIsProvided( font.get(), static_cast<Uint16>( ch ) ) != 0;
}
#endif // defined(TILES)
#endif // defined(TILES) || defined(SDL_SOUND)
