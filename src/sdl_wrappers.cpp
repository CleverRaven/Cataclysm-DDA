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

// Default texture scale quality applied by CreateTexture/CreateTextureFromSurface.
// Replaces the global SDL_HINT_RENDER_SCALE_QUALITY approach with per-texture mode.
// Initialized to "nearest" to match SDL2's documented default for the hint.
static std::string g_default_texture_scale_quality = "nearest";

static SDL_ScaleMode scale_quality_to_mode( const std::string &quality )
{
    if( quality == "0" || quality == "nearest" || quality == "none" ) {
        return SDL_ScaleModeNearest;
    }
    // "1", "linear", or anything else defaults to linear.
    // SDL3: SDL_SCALEMODE_NEAREST / SDL_SCALEMODE_LINEAR (renamed enums).
    return SDL_ScaleModeLinear;
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

bool SetRenderTarget( const SDL_Renderer_Ptr &renderer, const SDL_Texture_Ptr &texture )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return false;
    }
    // a null texture is fine for SDL (resets to default target)
    const bool failed = printErrorIf( SDL_SetRenderTarget( renderer.get(), texture.get() ) != 0,
                                      "SDL_SetRenderTarget failed" );
    return !failed;
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

int GetNumVideoDisplays()
{
    // SDL3: SDL_GetDisplays(&count) returns SDL_DisplayID* array; return count.
    return SDL_GetNumVideoDisplays();
}

const char *GetDisplayName( const int displayIndex )
{
    // SDL3: SDL_GetDisplays() + SDL_GetDisplayName(displayID). Map index to ID.
    return SDL_GetDisplayName( displayIndex );
}

bool GetDesktopDisplayMode( const int displayIndex, SDL_DisplayMode *mode )
{
    if( !mode ) {
        return false;
    }
    // SDL3: SDL_GetDesktopDisplayMode(displayID) returns const SDL_DisplayMode*.
    // Copy into caller's struct.
    return SDL_GetDesktopDisplayMode( displayIndex, mode ) == 0;
}


int GetNumRenderDrivers()
{
    // SDL3: same name, no change needed.
    return SDL_GetNumRenderDrivers();
}

const char *GetRenderDriverName( const int index )
{
    // SDL3: SDL_GetRenderDriver(index) returns const char* directly.
    // SDL2: need to go through SDL_RendererInfo.
    static SDL_RendererInfo info;
    if( SDL_GetRenderDriverInfo( index, &info ) != 0 ) {
        return "";
    }
    return info.name;
}


const char *GetRendererName( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return "";
    }
    // SDL3: SDL_GetRendererName(renderer) returns const char* directly.
    // SDL2: go through SDL_RendererInfo.
    static SDL_RendererInfo info;
    if( SDL_GetRendererInfo( renderer.get(), &info ) != 0 ) {
        return "";
    }
    return info.name;
}

bool IsRendererSoftware( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return false;
    }
    // SDL3: check renderer name == "software" or property query.
    SDL_RendererInfo info;
    if( SDL_GetRendererInfo( renderer.get(), &info ) != 0 ) {
        return false;
    }
    return ( info.flags & SDL_RENDERER_SOFTWARE ) != 0;
}

bool GetRendererMaxTextureSize( const SDL_Renderer_Ptr &renderer, int *max_w, int *max_h )
{
    if( !renderer ) {
        return false;
    }
    // SDL3: SDL_GetNumberProperty(SDL_GetRendererProperties(r),
    //       SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 0).
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
}


void RenderPresent( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return;
    }
    // SDL3: returns bool (SDL2: void).
    SDL_RenderPresent( renderer.get() );
}

void RenderDrawRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect )
{
    if( !renderer ) {
        return;
    }
    // SDL3: SDL_RenderRect (renamed, uses SDL_FRect internally).
    printErrorIf( SDL_RenderDrawRect( renderer.get(), rect ) != 0,
                  "SDL_RenderDrawRect failed" );
}

void RenderGetViewport( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect )
{
    if( !renderer ) {
        return;
    }
    // SDL3: SDL_GetRenderViewport (renamed).
    SDL_RenderGetViewport( renderer.get(), rect );
}

void RenderSetLogicalSize( const SDL_Renderer_Ptr &renderer, const int w, const int h )
{
    if( !renderer ) {
        return;
    }
    // SDL3: SDL_SetRenderLogicalPresentation(r, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX).
    // Note: SDL3 does NOT auto-scale mouse/touch events for logical size.
    // Callers must use ConvertEventCoordinates after SDL_PollEvent.
    printErrorIf( SDL_RenderSetLogicalSize( renderer.get(), w, h ) != 0,
                  "SDL_RenderSetLogicalSize failed" );
}

void RenderSetScale( const SDL_Renderer_Ptr &renderer, const float scaleX, const float scaleY )
{
    if( !renderer ) {
        return;
    }
    // SDL3: SDL_SetRenderScale (renamed).
    printErrorIf( SDL_RenderSetScale( renderer.get(), scaleX, scaleY ) != 0,
                  "SDL_RenderSetScale failed" );
}

bool RenderReadPixels( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect,
                       const Uint32 format, void *pixels, const int pitch )
{
    if( !renderer ) {
        return false;
    }
    // SDL3: returns SDL_Surface* instead of writing into a buffer.
    // Wrapper would allocate surface, copy data out, free surface.
    return SDL_RenderReadPixels( renderer.get(), rect, format, pixels, pitch ) == 0;
}

void GetRendererOutputSize( const SDL_Renderer_Ptr &renderer, int *w, int *h )
{
    if( !renderer ) {
        return;
    }
    // SDL3: SDL_GetCurrentRenderOutputSize (renamed).
    printErrorIf( SDL_GetRendererOutputSize( renderer.get(), w, h ) != 0,
                  "SDL_GetRendererOutputSize failed" );
}


uint32_t GetTicks()
{
    // SDL3: returns Uint64. Static cast is safe for ~49 days of uptime.
    return static_cast<uint32_t>( SDL_GetTicks() );
}


bool IsCursorVisible()
{
    // SDL3: SDL_CursorVisible() returns bool.
    return SDL_ShowCursor( SDL_QUERY ) == SDL_ENABLE;
}

void ShowCursor()
{
    // SDL3: SDL_ShowCursor() (no param).
    SDL_ShowCursor( SDL_ENABLE );
}

void HideCursor()
{
    // SDL3: SDL_HideCursor().
    SDL_ShowCursor( SDL_DISABLE );
}


std::string GetClipboardText()
{
    char *clip = SDL_GetClipboardText();
    if( !clip ) {
        return {};
    }
    std::string result( clip );
    SDL_free( clip );
    return result;
}

bool SetClipboardText( const std::string &text )
{
    return SDL_SetClipboardText( text.c_str() ) == 0;
}


Uint32 GetMouseState( int *x, int *y )
{
    // SDL3: returns float coordinates. Wrapper truncates to int.
    // Stays in window-space; downstream pipeline handles logical scaling.
    return SDL_GetMouseState( x, y );
}


bool IsScancodePressed( const SDL_Scancode scancode )
{
    const Uint8 *state = SDL_GetKeyboardState( nullptr );
    if( !state ) {
        return false;
    }
    // SDL3: returns const bool*, but the index semantics are the same.
    return state[scancode] != 0;
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
    // SDL3: always available.
#if SDL_VERSION_ATLEAST(2,26,0)
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


void StartTextInput( SDL_Window */*window*/ )
{
    // SDL3: SDL_StartTextInput(window). SDL2: no window param.
    SDL_StartTextInput();
}

void StopTextInput( SDL_Window */*window*/ )
{
    // SDL3: SDL_StopTextInput(window). SDL2: no window param.
    SDL_StopTextInput();
}

bool IsTextInputActive( SDL_Window */*window*/ )
{
    // SDL3: SDL_TextInputActive(window). SDL2: no window param.
    return SDL_IsTextInputActive() == SDL_TRUE;
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
    // SDL3: SDL_CreateWindowWithProperties to handle display placement for maximized
    // windows (SDL_SetWindowPosition has no effect on maximized windows in SDL3).
    SDL_Window_Ptr result( SDL_CreateWindow( title,
                           SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
                           SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
                           w, h, flags ) );
    printErrorIf( !result, "SDL_CreateWindow failed" );
    return result;
}


bool SetWindowFullscreen( SDL_Window *window, const FullscreenMode mode )
{
    if( !window ) {
        return false;
    }
    // SDL3: SDL_SetWindowFullscreen(window, true/false). For exclusive vs desktop,
    // call SDL_SetWindowFullscreenMode() first. All paths call SDL_SyncWindow.
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
}


void RestoreWindow( SDL_Window *window )
{
    if( window ) {
        SDL_RestoreWindow( window );
        // SDL3: would call SDL_SyncWindow here.
    }
}

void SetWindowSize( SDL_Window *window, const int w, const int h )
{
    if( window ) {
        SDL_SetWindowSize( window, w, h );
        // SDL3: would call SDL_SyncWindow here.
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

    int driver_index = -1;
    Uint32 flags = SDL_RENDERER_TARGETTEXTURE;

    if( software ) {
        flags |= SDL_RENDERER_SOFTWARE;
        // SDL3: pass "software" as driver name.
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

    // SDL3: SDL_CreateRenderer(window, name). Flags removed; vsync via SDL_SetRenderVSync.
    SDL_Renderer_Ptr result( SDL_CreateRenderer( window.get(), driver_index, flags ) );
    printErrorIf( !result, "SDL_CreateRenderer failed" );
    return result;
}


void ConvertEventCoordinates( const SDL_Renderer_Ptr &/*renderer*/, SDL_Event */*event*/ )
{
    // SDL2: no-op. SDL_RenderSetLogicalSize auto-scales mouse/touch events.
    // SDL3: SDL_ConvertEventToRenderCoordinates(renderer, event) on all event types.
}


float GetFingerX( const SDL_Event &ev, const int windowWidth )
{
    // SDL2: finger coords are normalized [0,1]; multiply by window width.
    // SDL3 (after ConvertEventCoordinates): already absolute render-logical.
    return ev.tfinger.x * windowWidth;
}

float GetFingerY( const SDL_Event &ev, const int windowHeight )
{
    // SDL2: finger coords are normalized [0,1]; multiply by window height.
    // SDL3 (after ConvertEventCoordinates): already absolute render-logical.
    return ev.tfinger.y * windowHeight;
}

#endif // defined(TILES)
#endif // defined(TILES) || defined(SDL_SOUND)
