#pragma once
#ifndef CATA_SRC_SDL_WRAPPERS_H
#define CATA_SRC_SDL_WRAPPERS_H

// IWYU pragma: begin_exports
#if defined(_MSC_VER) && defined(USE_VCPKG)
#   include <SDL2/SDL.h>
#   include <SDL2/SDL_image.h>
#   include <SDL2/SDL_ttf.h>
#else
#   include <SDL.h>
#   include <SDL_image.h>
#   include <SDL_ttf.h>
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

struct SDL_PixelFormat_deleter {
    void operator()( SDL_PixelFormat *const format ) {
        SDL_FreeFormat( format );
    }
};
using SDL_PixelFormat_Ptr = std::unique_ptr<SDL_PixelFormat, SDL_PixelFormat_deleter>;

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
void FillRect( const SDL_Surface_Ptr &surface, const SDL_Rect *rect, Uint32 color );
void SetTextureBlendMode( const SDL_Texture_Ptr &texture, SDL_BlendMode blendMode );
bool SetTextureColorMod( const SDL_Texture_Ptr &texture, Uint32 r, Uint32 g, Uint32 b );
void SetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode blendMode );
void GetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode &blend_mode );
SDL_Surface_Ptr load_image( const char *path );
void SetRenderTarget( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture );
void RenderClear( const SDL_Renderer_Ptr &renderer );
SDL_Surface_Ptr CreateRGBSurface( Uint32 flags, int width, int height, int depth, Uint32 Rmask,
                                  Uint32 Gmask, Uint32 Bmask, Uint32 Amask );
/**@}*/

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
