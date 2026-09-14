#if defined(TILES)
#include "sdl_geometry.h"

#include "point.h"

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

#endif // TILES
