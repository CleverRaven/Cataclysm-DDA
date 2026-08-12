#if defined(TILES) || defined(SDL_SOUND)

#include "sdl_wrappers.h"

#include <algorithm>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>

#include "cata_assert.h"
#include "cata_shader.h"
#include "debug.h"
#include "point.h"

#include <SDL3_image/SDL_image.h>

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

// Helper to convert SDL_Rect* to SDL_FRect for the render functions.
static SDL_FRect to_frect( const SDL_Rect &r )
{
    return { static_cast<float>( r.x ), static_cast<float>( r.y ),
             static_cast<float>( r.w ), static_cast<float>( r.h ) };
}
static SDL_FPoint to_fpoint( const SDL_Point &p )
{
    return { static_cast<float>( p.x ), static_cast<float>( p.y ) };
}

// Default texture scale quality applied by CreateTexture/CreateTextureFromSurface.
// Per-texture rather than a global hint, so callers can override one texture
// without disturbing the rest.
static std::string g_default_texture_scale_quality = "nearest";

static SDL_ScaleMode scale_quality_to_mode( const std::string &quality )
{
    if( quality == "0" || quality == "nearest" || quality == "none" ) {
        return SDL_SCALEMODE_NEAREST;
    }
    return SDL_SCALEMODE_LINEAR;
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
    SDL_FRect fsrc;
    SDL_FRect fdst;
    const SDL_FRect *fsrcp = srcrect ? &( fsrc = to_frect( *srcrect ) ) : nullptr;
    const SDL_FRect *fdstp = dstrect ? &( fdst = to_frect( *dstrect ) ) : nullptr;
    printErrorIf( !SDL_RenderTexture( renderer.get(), texture.get(), fsrcp, fdstp ),
                  "SDL_RenderTexture failed" );
}

SDL_Texture_Ptr CreateTexture( const SDL_Renderer_Ptr &renderer, Uint32 format, int access,
                               int w, int h )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to create texture with a null renderer";
        return SDL_Texture_Ptr();
    }
    SDL_Texture_Ptr result( SDL_CreateTexture( renderer.get(),
                            static_cast<SDL_PixelFormat>( format ),
                            static_cast<SDL_TextureAccess>( access ), w, h ) );
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

std::shared_ptr<SDL_Texture> make_gated_texture( SDL_Texture *raw, gpu_handle_graveyard::gate g )
{
    return std::shared_ptr<SDL_Texture>( raw,
    [g = std::move( g )]( SDL_Texture * t ) {
        if( t && ( !g || !g->load() ) ) {
            SDL_Texture_deleter{}( t );
        }
    } );
}

gpu_handle_graveyard::gpu_handle_graveyard()
    : gate_( std::make_shared<std::atomic<bool>>( false ) )
{
}

gpu_handle_graveyard::~gpu_handle_graveyard()
{
    // Never SDL_DestroyTexture at teardown -- the renderer may already be gone.
    // Inline the poison-and-clear without abandon()'s gate re-arm: a make_shared
    // in a (noexcept) destructor could terminate on bad_alloc.
    if( gate_ && !handles_.empty() ) {
        gate_->store( true );
    }
    handles_.clear();
}

void gpu_handle_graveyard::add( SDL_Texture *raw )
{
    if( !raw ) {
        return;
    }
    handles_.push_back( make_gated_texture( raw, gate_ ) );
}

void gpu_handle_graveyard::adopt( std::shared_ptr<SDL_Texture> handle )
{
    if( handle ) {
        handles_.push_back( std::move( handle ) );
    }
}

void gpu_handle_graveyard::drain_live_renderer()
{
    // Gate stays clear, so clearing the handles runs each deleter
    // (SDL_DestroyTexture) against the still-live renderer.
    handles_.clear();
    gate_ = std::make_shared<std::atomic<bool>>( false );
}

void gpu_handle_graveyard::abandon()
{
    // Poison the gate only when this graveyard owns handles: an empty one may
    // share its gate with handles committed elsewhere (a successful upload),
    // which must still destroy normally.
    if( gate_ && !handles_.empty() ) {
        gate_->store( true );
    }
    handles_.clear();
    gate_ = std::make_shared<std::atomic<bool>>( false );
}

void SetRenderDrawColor( const SDL_Renderer_Ptr &renderer, const Uint8 r, const Uint8 g,
                         const Uint8 b, const Uint8 a )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( !SDL_SetRenderDrawColor( renderer.get(), r, g, b, a ),
                  "SDL_SetRenderDrawColor failed" );
}

void RenderDrawPoint( const SDL_Renderer_Ptr &renderer, const point &p )
{
    printErrorIf( !SDL_RenderPoint( renderer.get(), static_cast<float>( p.x ),
                                    static_cast<float>( p.y ) ),
                  "SDL_RenderPoint failed" );
}

void RenderFillRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    if( rect ) {
        SDL_FRect fr = to_frect( *rect );
        printErrorIf( !SDL_RenderFillRect( renderer.get(), &fr ), "SDL_RenderFillRect failed" );
    } else {
        printErrorIf( !SDL_RenderFillRect( renderer.get(), nullptr ), "SDL_RenderFillRect failed" );
    }
}

int FillRect( const SDL_Surface_Ptr &surface, const SDL_Rect *const rect, Uint32 color )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to use a null surface";
        return -1;
    }
    const bool ok = SDL_FillSurfaceRect( surface.get(), rect, color );
    printErrorIf( !ok, "SDL_FillSurfaceRect failed" );
    return ok ? 0 : -1;
}

void SetTextureBlendMode( const SDL_Texture_Ptr &texture, SDL_BlendMode blendMode )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
    }

    throwErrorIf( !SDL_SetTextureBlendMode( texture.get(), blendMode ),
                  "SDL_SetTextureBlendMode failed" );
}

void SetTextureBlendMode( const std::shared_ptr<SDL_Texture> &texture, SDL_BlendMode blendMode )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
    throwErrorIf( !SDL_SetTextureBlendMode( texture.get(), blendMode ),
                  "SDL_SetTextureBlendMode failed" );
}

bool SetTextureColorMod( const SDL_Texture_Ptr &texture, Uint32 r, Uint32 g, Uint32 b )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return true;
    }
    return printErrorIf( !SDL_SetTextureColorMod( texture.get(), r, g, b ),
                         "SDL_SetTextureColorMod failed" );
}

bool SetTextureColorMod( const std::shared_ptr<SDL_Texture> &texture, Uint32 r, Uint32 g,
                         Uint32 b )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return true;
    }
    return printErrorIf( !SDL_SetTextureColorMod( texture.get(), r, g, b ),
                         "SDL_SetTextureColorMod failed" );
}

void SetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, const SDL_BlendMode blendMode )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( !SDL_SetRenderDrawBlendMode( renderer.get(), blendMode ),
                  "SDL_SetRenderDrawBlendMode failed" );
}

void GetRenderDrawBlendMode( const SDL_Renderer_Ptr &renderer, SDL_BlendMode &blend_mode )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( !SDL_GetRenderDrawBlendMode( renderer.get(), &blend_mode ),
                  "SDL_GetRenderDrawBlendMode failed" );
}

SDL_Surface_Ptr load_image( const char *const path )
{
    cata_assert( path );
    SDL_Surface_Ptr result( IMG_Load( path ) );
    if( !result ) {
        throw std::runtime_error( "Could not load image \"" + std::string( path ) + "\": " +
                                  SDL_GetError() );
    }
    return result;
}

void scoped_render_target::mark_boundary_lost()
{
    renderer_boundary_signal_recovery_required();
    boundary_intact_ = false;
}

scoped_render_target::scoped_render_target( const SDL_Renderer_Ptr &renderer,
        SDL_Texture *target, cata_shader::variant_pass *vp )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "scoped_render_target: null renderer";
        return;
    }
    // Already latched (or embargoed): renderer may be dangling after a LOST
    // handover, so even SDL_GetRenderTarget is unsafe. Refuse before any SDL call.
    if( renderer_boundary_recovery_pending() ) {
        boundary_intact_ = false;
        return;
    }
    renderer_ = renderer.get();
    vp_ = vp;
    // Flush the pass FIRST so an embargoed pass refuses before any SDL call;
    // only then capture prior_target_ and switch.
    if( vp_ && !vp_->flush() ) {
        mark_boundary_lost();
        renderer_ = nullptr;
        vp_ = nullptr;
        return;
    }
    prior_target_ = SDL_GetRenderTarget( renderer_ );
    if( !SDL_SetRenderTarget( renderer_, target ) ) {
        dbg( D_ERROR ) << "scoped_render_target: SDL_SetRenderTarget failed: " << SDL_GetError();
        mark_boundary_lost();
        renderer_ = nullptr;
        prior_target_ = nullptr;
        vp_ = nullptr;
        return;
    }
    valid_ = true;
}

bool scoped_render_target::restore()
{
    if( restore_attempted_ ) {
        return last_restore_ok_;
    }
    restore_attempted_ = true;
    last_restore_ok_ = false;
    if( !valid_ || !renderer_ ) {
        return false;
    }
    if( renderer_boundary_recovery_pending() ) {
        // Already latched; do not issue another SDL_SetRenderTarget.
        boundary_intact_ = false;
        return false;
    }
    if( vp_ && !vp_->flush() ) {
        // Undefined shader-state bind: do not switch the target.
        dbg( D_ERROR ) << "scoped_render_target::restore: variant_pass flush failed";
        mark_boundary_lost();
        return false;
    }
    if( !SDL_SetRenderTarget( renderer_, prior_target_ ) ) {
        dbg( D_ERROR ) << "scoped_render_target::restore: SDL_SetRenderTarget failed: "
                       << SDL_GetError();
        mark_boundary_lost();
        return false;
    }
    restored_ = true;
    last_restore_ok_ = true;
    return true;
}

scoped_render_target::~scoped_render_target()
{
    if( restored_ || !valid_ || !renderer_ ) {
        return;
    }
    if( restore_attempted_ ) {
        // Explicit restore already failed and logged; the caller chose to
        // abort. Do not retry to avoid masking the original failure.
        return;
    }
    if( renderer_boundary_recovery_pending() ) {
        // Another callsite latched recovery; suppress the implicit restore.
        // The coordinator rebinds on rebuild.
        boundary_intact_ = false;
        return;
    }
    ( void )restore();
}

bind_result permanent_render_target_bind( const SDL_Renderer_Ptr &renderer, SDL_Texture *target,
        cata_shader::variant_pass *vp )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "permanent_render_target_bind: null renderer";
        // No SDL call issued, so no embargo: pre-switch refusal.
        return bind_result::refused_pre_switch;
    }
    if( vp && !vp->flush() ) {
        // Unsafe shader-state bind: do not switch the target. Latch recovery.
        renderer_boundary_signal_recovery_required();
        return bind_result::failed_in_switch;
    }
    const bool failed = printErrorIf( !SDL_SetRenderTarget( renderer.get(), target ),
                                      "SDL_SetRenderTarget failed" );
    if( failed ) {
        // SDL may have mutated target state before failing; latch recovery.
        // Callers with a quarantine path still see the bind_result and
        // sequence their own teardown -- the latch is additive.
        renderer_boundary_signal_recovery_required();
    }
    return failed ? bind_result::failed_in_switch : bind_result::ok;
}

void RenderClear( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( !SDL_RenderClear( renderer.get() ), "SDL_RenderClear failed" );
}

SDL_Surface_Ptr CreateRGBSurface( const Uint32 flags, const int width, const int height,
                                  const int depth, const Uint32 Rmask, const Uint32 Gmask, const Uint32 Bmask, const Uint32 Amask )
{
    ( void )flags;
    SDL_PixelFormat fmt = SDL_GetPixelFormatForMasks( depth, Rmask, Gmask, Bmask, Amask );
    SDL_Surface_Ptr surface( SDL_CreateSurface( width, height, fmt ) );
    throwErrorIf( !surface, "Failed to create surface" );
    return surface;
}

void SetTextureAlphaMod( const SDL_Texture_Ptr &texture, const Uint8 alpha )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
    printErrorIf( !SDL_SetTextureAlphaMod( texture.get(), alpha ),
                  "SDL_SetTextureAlphaMod failed" );
}

void SetTextureAlphaMod( const std::shared_ptr<SDL_Texture> &texture, const Uint8 alpha )
{
    if( !texture ) {
        dbg( D_ERROR ) << "Tried to use a null texture";
        return;
    }
    printErrorIf( !SDL_SetTextureAlphaMod( texture.get(), alpha ),
                  "SDL_SetTextureAlphaMod failed" );
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
    SDL_FRect fsrc;
    SDL_FRect fdst;
    SDL_FPoint fcenter;
    const SDL_FRect *fsrcp = srcrect ? &( fsrc = to_frect( *srcrect ) ) : nullptr;
    const SDL_FRect *fdstp = dstrect ? &( fdst = to_frect( *dstrect ) ) : nullptr;
    const SDL_FPoint *fcenterp = center ? &( fcenter = to_fpoint( *center ) ) : nullptr;
    printErrorIf( !SDL_RenderTextureRotated( renderer.get(), texture, fsrcp, fdstp, angle,
                  fcenterp, flip ),
                  "SDL_RenderTextureRotated failed" );
}

void RenderSetClipRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    printErrorIf( !SDL_SetRenderClipRect( renderer.get(), rect ),
                  "SDL_SetRenderClipRect failed" );
}

void RenderGetClipRect( const SDL_Renderer_Ptr &renderer, SDL_Rect *const rect )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return;
    }
    SDL_GetRenderClipRect( renderer.get(), rect );
}

bool RenderIsClipEnabled( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        dbg( D_ERROR ) << "Tried to use a null renderer";
        return false;
    }
    return SDL_RenderClipEnabled( renderer.get() );
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
    // SDL_BlitSurface returns bool; this wrapper reports 0 for success.
    return SDL_BlitSurface( src.get(), srcrect, dst.get(), dstrect ) ? 0 : -1;
}

Uint32 MapRGB( const SDL_Surface_Ptr &surface, const Uint8 r, const Uint8 g, const Uint8 b )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to map color on a null surface";
        return 0;
    }
    return SDL_MapSurfaceRGB( surface.get(), r, g, b );
}

Uint32 MapRGBA( const SDL_Surface_Ptr &surface, const Uint8 r, const Uint8 g, const Uint8 b,
                const Uint8 a )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to map color on a null surface";
        return 0;
    }
    return SDL_MapSurfaceRGBA( surface.get(), r, g, b, a );
}

void GetRGBA( const Uint32 pixel, const SDL_Surface_Ptr &surface, Uint8 &r, Uint8 &g, Uint8 &b,
              Uint8 &a )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to get RGBA from a null surface";
        r = g = b = a = 0;
        return;
    }
    SDL_GetRGBA( pixel, SDL_GetPixelFormatDetails( surface->format ),
                 SDL_GetSurfacePalette( surface.get() ), &r, &g, &b, &a );
}

int SetColorKey( const SDL_Surface_Ptr &surface, const int flag, const Uint32 key )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set colorkey on a null surface";
        return -1;
    }
    return SDL_SetSurfaceColorKey( surface.get(), flag, key ) ? 0 : -1;
}

int SetSurfaceRLE( const SDL_Surface_Ptr &surface, const int flag )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set RLE on a null surface";
        return -1;
    }
    return SDL_SetSurfaceRLE( surface.get(), flag ) ? 0 : -1;
}

int SetSurfaceBlendMode( const SDL_Surface_Ptr &surface, const SDL_BlendMode blendMode )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to set blend mode on a null surface";
        return -1;
    }
    return SDL_SetSurfaceBlendMode( surface.get(), blendMode ) ? 0 : -1;
}

SDL_Surface_Ptr ConvertSurfaceFormat( const SDL_Surface_Ptr &surface, const Uint32 pixel_format )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to convert a null surface";
        return SDL_Surface_Ptr();
    }
    SDL_Surface_Ptr result( SDL_ConvertSurface( surface.get(),
                            static_cast<SDL_PixelFormat>( pixel_format ) ) );
    printErrorIf( !result, "SDL_ConvertSurface failed" );
    return result;
}

int LockSurface( const SDL_Surface_Ptr &surface )
{
    if( !surface ) {
        dbg( D_ERROR ) << "Tried to lock a null surface";
        return -1;
    }
    return SDL_LockSurface( surface.get() ) ? 0 : -1;
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
    // surface->format is the enum directly (no intermediate struct).
    return surface->format;
}

TTF_Font_Ptr OpenFontIndex( const char *const file, const int ptsize, const int64_t index )
{
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
}

const char *FontFaceStyleName( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query style name of a null font";
        return nullptr;
    }
    return TTF_GetFontStyleName( font.get() );
}

int FontFaces( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query face count of a null font";
        return 0;
    }
    return TTF_GetNumFontFaces( font.get() );
}

int FontHeight( const TTF_Font_Ptr &font )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to query height of a null font";
        return 0;
    }
    return TTF_GetFontHeight( font.get() );
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
    return SDL_Surface_Ptr( TTF_RenderText_Solid( font.get(), text, 0, fg ) );
}

SDL_Surface_Ptr RenderUTF8_Blended( const TTF_Font_Ptr &font, const char *const text,
                                    const SDL_Color fg )
{
    if( !font ) {
        dbg( D_ERROR ) << "Tried to render with a null font";
        return SDL_Surface_Ptr();
    }
    return SDL_Surface_Ptr( TTF_RenderText_Blended( font.get(), text, 0, fg ) );
}

bool CanRenderGlyph( const TTF_Font_Ptr &font, const Uint32 ch )
{
    if( !font ) {
        return false;
    }
    return TTF_FontHasGlyph( font.get(), ch );
}

int GetNumVideoDisplays()
{
    int count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays( &count );
    SDL_free( displays );
    return count;
}

const char *GetDisplayName( const int displayIndex )
{
    int count = 0;
    SDL_DisplayID *displays = SDL_GetDisplays( &count );
    if( !displays || displayIndex < 0 || displayIndex >= count ) {
        SDL_free( displays );
        return "";
    }
    const char *name = SDL_GetDisplayName( displays[displayIndex] );
    SDL_free( displays );
    return name ? name : "";
}

bool GetDesktopDisplayMode( const int displayIndex, SDL_DisplayMode *mode )
{
    if( !mode ) {
        return false;
    }
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
}


int GetNumRenderDrivers()
{
    return SDL_GetNumRenderDrivers();
}

const char *GetRenderDriverName( const int index )
{
    const char *name = SDL_GetRenderDriver( index );
    return name ? name : "";
}


const char *GetRendererName( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return "";
    }
    const char *name = SDL_GetRendererName( renderer.get() );
    return name ? name : "";
}

bool IsRendererSoftware( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return false;
    }
    const char *name = SDL_GetRendererName( renderer.get() );
    return name && std::string( name ) == "software";
}

bool GetRendererMaxTextureSize( const SDL_Renderer_Ptr &renderer, int *max_w, int *max_h )
{
    if( !renderer ) {
        return false;
    }
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
}


void RenderPresent( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return;
    }
    printErrorIf( !SDL_RenderPresent( renderer.get() ), "SDL_RenderPresent failed" );
}

void RenderDrawRect( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect )
{
    if( !renderer ) {
        return;
    }
    if( rect ) {
        SDL_FRect fr = to_frect( *rect );
        printErrorIf( !SDL_RenderRect( renderer.get(), &fr ), "SDL_RenderRect failed" );
    } else {
        printErrorIf( !SDL_RenderRect( renderer.get(), nullptr ), "SDL_RenderRect failed" );
    }
}

void RenderGetViewport( const SDL_Renderer_Ptr &renderer, SDL_Rect *rect )
{
    if( !renderer ) {
        return;
    }
    SDL_GetRenderViewport( renderer.get(), rect );
}

void RenderSetLogicalSize( const SDL_Renderer_Ptr &renderer, const int w, const int h )
{
    if( !renderer ) {
        return;
    }
    printErrorIf( !SDL_SetRenderLogicalPresentation( renderer.get(), w, h,
                  SDL_LOGICAL_PRESENTATION_LETTERBOX ),
                  "SDL_SetRenderLogicalPresentation failed" );
}

void RenderSetScale( const SDL_Renderer_Ptr &renderer, const float scaleX, const float scaleY )
{
    if( !renderer ) {
        return;
    }
    printErrorIf( !SDL_SetRenderScale( renderer.get(), scaleX, scaleY ),
                  "SDL_SetRenderScale failed" );
}

bool RenderReadPixels( const SDL_Renderer_Ptr &renderer, const SDL_Rect *rect,
                       const Uint32 format, void *pixels, const int pitch )
{
    if( !renderer ) {
        return false;
    }
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
}

void GetRendererOutputSize( const SDL_Renderer_Ptr &renderer, int *w, int *h )
{
    if( !renderer ) {
        return;
    }
    printErrorIf( !SDL_GetCurrentRenderOutputSize( renderer.get(), w, h ),
                  "SDL_GetCurrentRenderOutputSize failed" );
}

SDL_Texture *GetRenderTarget( const SDL_Renderer_Ptr &renderer )
{
    if( !renderer ) {
        return nullptr;
    }
    return SDL_GetRenderTarget( renderer.get() );
}


uint32_t GetTicks()
{
    // SDL_GetTicks returns Uint64. The cast is safe for ~49 days of uptime.
    return static_cast<uint32_t>( SDL_GetTicks() );
}


bool IsCursorVisible()
{
    return SDL_CursorVisible();
}

void ShowCursor()
{
    SDL_ShowCursor();
}

void HideCursor()
{
    SDL_HideCursor();
}


std::string GetClipboardText()
{
    const char *clip = SDL_GetClipboardText();
    return clip ? std::string( clip ) : std::string();
}

bool SetClipboardText( const std::string &text )
{
    return SDL_SetClipboardText( text.c_str() );
}


Uint32 GetMouseState( int *x, int *y )
{
    float fx = 0;
    float fy = 0;
    Uint32 buttons = SDL_GetMouseState( &fx, &fy );
    if( x ) {
        *x = static_cast<int>( fx );
    }
    if( y ) {
        *y = static_cast<int>( fy );
    }
    return buttons;
}


bool IsScancodePressed( const SDL_Scancode scancode )
{
    const bool *state = SDL_GetKeyboardState( nullptr );
    if( !state ) {
        return false;
    }
    return state[scancode];
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
    SDL_GetWindowSizeInPixels( window, w, h );
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
    SDL_StartTextInput( window );
}

void StopTextInput( SDL_Window *window )
{
    SDL_StopTextInput( window );
}

bool IsTextInputActive( SDL_Window *window )
{
    return SDL_TextInputActive( window );
}


bool IsWindowEvent( const SDL_Event &ev )
{
    return ev.type >= SDL_EVENT_WINDOW_FIRST && ev.type <= SDL_EVENT_WINDOW_LAST;
}

Uint32 GetWindowEventID( const SDL_Event &ev )
{
    // The window event type IS the top-level event type.
    return ev.type;
}

CataKeysym GetKeysym( const SDL_Event &ev )
{
    return { ev.key.key, ev.key.mod, ev.key.scancode };
}


SDL_Window_Ptr CreateGameWindow( const char *title, const int display, const int w, const int h,
                                 const Uint32 flags )
{
    ( void )display;
    // SDL_CreateWindow takes (title, w, h, flags) -- no position params.
    SDL_Window_Ptr result( SDL_CreateWindow( title, w, h, flags ) );
    printErrorIf( !result, "SDL_CreateWindow failed" );
    return result;
}


bool SetWindowFullscreen( SDL_Window *window, const FullscreenMode mode )
{
    if( !window ) {
        return false;
    }
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
}


void RestoreWindow( SDL_Window *window )
{
    if( window ) {
        SDL_RestoreWindow( window );
        SDL_SyncWindow( window );
    }
}

void SetWindowSize( SDL_Window *window, const int w, const int h )
{
    if( window ) {
        SDL_SetWindowSize( window, w, h );
        SDL_SyncWindow( window );
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

    const char *name = nullptr;
    if( software ) {
        name = "software";
    } else if( driver_name && driver_name[0] != '\0' ) {
        name = driver_name;
    }
    // Use the properties-based create path so the GPU renderer learns we can
    // supply SPIR-V/DXIL/MSL shaders to SDL_GPURenderState; without these
    // SDL.renderer.create.gpu.shaders_* booleans, custom GPURenderState may
    // not wire its texture pipeline correctly even though SDL_GetGPURendererDevice
    // returns a non-null device.
    SDL_PropertiesID props = SDL_CreateProperties();
    if( props == 0 ) {
        printErrorIf( true, "SDL_CreateProperties failed" );
        return SDL_Renderer_Ptr();
    }
    SDL_SetPointerProperty( props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window.get() );
    if( name ) {
        SDL_SetStringProperty( props, SDL_PROP_RENDERER_CREATE_NAME_STRING, name );
    }
    SDL_SetNumberProperty( props, SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER,
                           ( vsync && !software ) ? 1 : 0 );
    SDL_SetBooleanProperty( props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, true );
    SDL_SetBooleanProperty( props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, true );
    SDL_SetBooleanProperty( props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_MSL_BOOLEAN, true );
    SDL_Renderer_Ptr result( SDL_CreateRendererWithProperties( props ) );
    SDL_DestroyProperties( props );
    printErrorIf( !result, "SDL_CreateRendererWithProperties failed" );
    return result;
}


float GetFingerX( const SDL_Event &ev, const int windowWidth )
{
    // tfinger.x arrives in normalized [0..1] space.
    // Multiply by the logical window width to land in window pixel units
    // for shortcut hit-tests and joystick math.
    return ev.tfinger.x * windowWidth;
}

float GetFingerY( const SDL_Event &ev, const int windowHeight )
{
    return ev.tfinger.y * windowHeight;
}

#endif // defined(TILES)
#endif // defined(TILES) || defined(SDL_SOUND)
