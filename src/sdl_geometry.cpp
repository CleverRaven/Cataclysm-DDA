#if defined(TILES)
#include "sdl_geometry.h"

#include <ostream>
#include <stdexcept>

#include "debug.h"
#include "point.h"
#include "sdl_utils.h"

void GeometryRenderer::horizontal_line( const SDL_Renderer_Ptr &renderer, const point &pos, int x2,
                                        int thickness, const SDL_Color &color ) const
{
    SDL_Rect rect { pos.x, pos.y, x2 - pos.x, thickness };
    this->rect( renderer, rect, color );
}

void GeometryRenderer::vertical_line( const SDL_Renderer_Ptr &renderer, const point &pos, int y2,
                                      int thickness, const SDL_Color &color ) const
{
    SDL_Rect rect { pos.x, pos.y, thickness, y2 - pos.y };
    this->rect( renderer, rect, color );
}

void GeometryRenderer::rect( const SDL_Renderer_Ptr &renderer, const point &pos, int width,
                             int height, const SDL_Color &color ) const
{
    SDL_Rect rect { pos.x, pos.y, width, height };
    this->rect( renderer, rect, color );
}

void DefaultGeometryRenderer::rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                                    const SDL_Color &color ) const
{
    SetRenderDrawColor( renderer, color.r, color.g, color.b, color.a );
    RenderFillRect( renderer, &rect );
}

ColorModulatedGeometryRenderer::ColorModulatedGeometryRenderer( const SDL_Renderer_Ptr &renderer )
{
    SDL_Surface_Ptr alt_surf;
    try {
        alt_surf = create_surface_32( 1, 1 );
    } catch( const std::runtime_error & ) {
        DebugLog( D_ERROR, DC_ALL ) << "CreateRGBSurface failed: " << SDL_GetError();
        return;
    }

    FillRect( alt_surf, nullptr, SDL_MapRGB( alt_surf->format, 255, 255, 255 ) );

    tex.reset( SDL_CreateTextureFromSurface( renderer.get(), alt_surf.get() ) );
    alt_surf.reset();

    // Test to make sure color modulation is supported by renderer
    bool tex_enable = !SetTextureColorMod( tex, 0, 0, 0 );
    if( !tex_enable ) {
        tex.reset();
    }
    DebugLog( D_INFO, DC_ALL ) << "ColorModulatedGeometryRenderer constructor() = " <<
                               ( tex_enable ? "SUCCESS" : "FAIL" ) << ". tex_enable = " << tex_enable;
}

void ColorModulatedGeometryRenderer::rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
        const SDL_Color &color ) const
{
    if( tex ) {
        SetTextureColorMod( tex, color.r, color.g, color.b );
        RenderCopy( renderer, tex, nullptr, &rect );
    } else {
        DefaultGeometryRenderer::rect( renderer, rect, color );
    }
}

#endif // TILES
