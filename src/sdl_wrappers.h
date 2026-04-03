#pragma once
#ifndef CATA_SRC_SDL_WRAPPERS_H
#define CATA_SRC_SDL_WRAPPERS_H

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
// IWYU pragma: begin_exports
#if defined(_MSC_VER) && defined(USE_VCPKG)
#   include <SDL2/SDL.h>
#   include <SDL2/SDL_image.h>
#   include <SDL2/SDL_ttf.h>
#   include <SDL2/SDL_mouse.h>
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#   include <SDL.h>
#pragma GCC diagnostic pop
#   include <SDL_image.h>
#   include <SDL_ttf.h>
#   include <SDL_mouse.h>
#endif
// IWYU pragma: end_exports

#include <memory>

struct point;

struct SDL_Renderer_deleter {
    void operator()( SDL_Renderer *const renderer ) {
        SDL_DestroyRenderer( renderer );
    }
};
using SDL_Renderer_Ptr = std::unique_ptr<SDL_Renderer, SDL_Renderer_deleter>;

struct SDL_Window_deleter {
    void operator()( SDL_Window *const window ) {
        SDL_DestroyWindow( window );
    }
};
using SDL_Window_Ptr = std::unique_ptr<SDL_Window, SDL_Window_deleter>;

struct SDL_Texture_deleter {
    void operator()( SDL_Texture *const ptr ) {
        SDL_DestroyTexture( ptr );
    }
};
using SDL_Texture_Ptr = std::unique_ptr<SDL_Texture, SDL_Texture_deleter>;

struct SDL_Surface_deleter {
    void operator()( SDL_Surface *const ptr ) {
        SDL_FreeSurface( ptr );
    }
};
using SDL_Surface_Ptr = std::unique_ptr<SDL_Surface, SDL_Surface_deleter>;

struct TTF_Font_deleter {
    void operator()( TTF_Font *const font ) {
        TTF_CloseFont( font );
    }
};
using TTF_Font_Ptr = std::unique_ptr<TTF_Font, TTF_Font_deleter>;
/**
 * If the @p condition is `true`, an error (including the given @p message
 * and the output of @ref SDL_GetError) is logged to the debug log.
 * @returns \p condition, in other words: return whether an error was logged.
 */
bool printErrorIf( bool condition, const char *message );
/**
 * If the @p condition is `true`, an exception (including the given @p message
 * and the output of @ref SDL_GetError) is thrown.
 */
void throwErrorIf( bool condition, const char *message );
/**
 * Wrappers for SDL functions that does error reporting and that accept our
 * wrapped pointers.
 * Errors are reported via the usual debug log stream (exceptions are noted below).
 *
 * @ref CreateTextureFromSurface returns an empty `SDL_Texture_Ptr` if the function
 * fails (the failure is also logged by the function).
 * @ref load_image throws if the loading fails. Its input must be a valid C-String.
 */
/**@{*/
void RenderCopy( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture,
                 const SDL_Rect *srcrect, const SDL_Rect *dstrect );
SDL_Texture_Ptr CreateTexture( const SDL_Renderer_Ptr &renderer, Uint32 format, int access,
                               int w, int h );
SDL_Texture_Ptr CreateTextureFromSurface( const SDL_Renderer_Ptr &renderer,
        const SDL_Surface_Ptr &surface );
void SetRenderDrawColor( const SDL_Renderer_Ptr &renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a );
void RenderDrawPoint( const SDL_Renderer_Ptr &renderer, const point &p );
void RenderFillRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
int FillRect( const SDL_Surface_Ptr &surface, const SDL_Rect *rect, Uint32 color );
void SetTextureBlendMode( const SDL_Texture_Ptr &texture, SDL_BlendMode blendMode );
bool SetTextureColorMod( const SDL_Texture_Ptr &texture, Uint32 r, Uint32 g, Uint32 b );
void SetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode blendMode );
void GetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode &blend_mode );
SDL_Surface_Ptr load_image( const char *path );
void SetRenderTarget( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture );
void RenderClear( const SDL_Renderer_Ptr &renderer );
SDL_Surface_Ptr CreateRGBSurface( Uint32 flags, int width, int height, int depth, Uint32 Rmask,
                                  Uint32 Gmask, Uint32 Bmask, Uint32 Amask );
void SetTextureAlphaMod( const SDL_Texture_Ptr &texture, Uint8 alpha );
void RenderCopyEx( const SDL_Renderer_Ptr &renderer, SDL_Texture *texture,
                   const SDL_Rect *srcrect, const SDL_Rect *dstrect,
                   double angle, const SDL_Point *center, SDL_RendererFlip flip );
void RenderSetClipRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect );
void RenderGetClipRect( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect );
bool RenderIsClipEnabled( const SDL_Renderer_Ptr &renderer );
int BlitSurface( const SDL_Surface_Ptr &src, const SDL_Rect *srcrect,
                 const SDL_Surface_Ptr &dst, SDL_Rect *dstrect );
Uint32 MapRGB( const SDL_Surface_Ptr &surface, Uint8 r, Uint8 g, Uint8 b );
Uint32 MapRGBA( const SDL_Surface_Ptr &surface, Uint8 r, Uint8 g, Uint8 b, Uint8 a );
int SetColorKey( const SDL_Surface_Ptr &surface, int flag, Uint32 key );
int SetSurfaceRLE( const SDL_Surface_Ptr &surface, int flag );
int SetSurfaceBlendMode( const SDL_Surface_Ptr &surface, SDL_BlendMode blendMode );
SDL_Surface_Ptr ConvertSurfaceFormat( const SDL_Surface_Ptr &surface, Uint32 pixel_format );
int LockSurface( const SDL_Surface_Ptr &surface );
void UnlockSurface( const SDL_Surface_Ptr &surface );
// Returns the pixel format enum (SDL_PIXELFORMAT_*) for the surface.
// SDL3: surface->format is the enum directly; SDL2: surface->format->format.
Uint32 GetSurfacePixelFormat( const SDL_Surface_Ptr &surface );
TTF_Font_Ptr OpenFontIndex( const char *file, int ptsize, int64_t index );
const char *FontFaceStyleName( const TTF_Font_Ptr &font );
int FontFaces( const TTF_Font_Ptr &font );
int FontHeight( const TTF_Font_Ptr &font );
void SetFontStyle( const TTF_Font_Ptr &font, int style );
SDL_Surface_Ptr RenderUTF8_Solid( const TTF_Font_Ptr &font, const char *text, SDL_Color fg );
SDL_Surface_Ptr RenderUTF8_Blended( const TTF_Font_Ptr &font, const char *text, SDL_Color fg );
// Project-level helper: can this font produce a glyph for the given codepoint?
// In SDL3_ttf there is no direct TTF_GlyphIsProvided equivalent; this will be
// emulated via glyph metrics or a render attempt.
bool CanRenderGlyph( const TTF_Font_Ptr &font, Uint32 ch );
/**@}*/

void StartTextInput();
void StopTextInput();

/**
 * Comparison operators which SDL lacks being a C-ish lib.
 */
/**@{*/

inline bool operator==( const SDL_Color &lhs, const SDL_Color &rhs )
{
    return
        lhs.r == rhs.r &&
        lhs.g == rhs.g &&
        lhs.b == rhs.b &&
        lhs.a == rhs.a;
}

inline bool operator!=( const SDL_Color &lhs, const SDL_Color &rhs )
{
    return !operator==( lhs, rhs );
}

inline bool operator==( const SDL_Rect &lhs, const SDL_Rect &rhs )
{
    return
        lhs.x == rhs.x &&
        lhs.y == rhs.y &&
        lhs.w == rhs.w &&
        lhs.h == rhs.h;
}

inline bool operator!=( const SDL_Rect &lhs, const SDL_Rect &rhs )
{
    return !operator==( lhs, rhs );
}

/**@}*/

#endif // CATA_SRC_SDL_WRAPPERS_H
