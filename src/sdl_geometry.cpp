#if defined(TILES)
#include "sdl_geometry.h"

#include "point.h"

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

#endif // TILES
