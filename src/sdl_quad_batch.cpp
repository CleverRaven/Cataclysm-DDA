#if defined(TILES)

#include "sdl_quad_batch.h"

#include <algorithm>
#include <cstddef>
#include <iterator>

SDL_FColor to_fcolor( const SDL_Color &color )
{
    return SDL_FColor{ color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
}

void quad_batch::append( const float x, const float y, const float w, const float h,
                         const SDL_FColor &color, const float u0, const float v0,
                         const float u1, const float v1 )
{
    const int base = vertex_count();
    vertices_.push_back( SDL_Vertex{ SDL_FPoint{ x, y }, color, SDL_FPoint{ u0, v0 } } );
    vertices_.push_back( SDL_Vertex{ SDL_FPoint{ x + w, y }, color, SDL_FPoint{ u1, v0 } } );
    vertices_.push_back( SDL_Vertex{ SDL_FPoint{ x + w, y + h }, color, SDL_FPoint{ u1, v1 } } );
    vertices_.push_back( SDL_Vertex{ SDL_FPoint{ x, y + h }, color, SDL_FPoint{ u0, v1 } } );
    const int quad_indices[6] = { base, base + 1, base + 2, base, base + 2, base + 3 };
    indices_.insert( indices_.end(), std::begin( quad_indices ), std::end( quad_indices ) );
}

void quad_batch::append_quad( const float x, const float y, const float w, const float h,
                              const SDL_FColor &color )
{
    append( x, y, w, h, color, 0.0f, 0.0f, 0.0f, 0.0f );
}

void quad_batch::append_textured_quad( const SDL_Rect &dst, const SDL_Rect &src,
                                       const int tex_w, const int tex_h, const SDL_FColor &color )
{
    // divide each edge instead of adding a scaled width so that neither edge
    // inherits the other's rounding
    append( static_cast<float>( dst.x ), static_cast<float>( dst.y ),
            static_cast<float>( dst.w ), static_cast<float>( dst.h ), color,
            static_cast<float>( src.x ) / tex_w, static_cast<float>( src.y ) / tex_h,
            static_cast<float>( src.x + src.w ) / tex_w,
            static_cast<float>( src.y + src.h ) / tex_h );
}

void quad_batch::clear()
{
    vertices_.clear();
    indices_.clear();
}

bool quad_batch::empty() const
{
    return indices_.empty();
}

int quad_batch::vertex_count() const
{
    return static_cast<int>( vertices_.size() );
}

int quad_batch::index_count() const
{
    return static_cast<int>( indices_.size() );
}

const SDL_Vertex *quad_batch::vertex_data() const
{
    return vertices_.data();
}

const int *quad_batch::index_data() const
{
    return indices_.data();
}

void quad_batch::reserve_quads( const int quads )
{
    vertices_.reserve( 4 * static_cast<size_t>( quads ) );
    indices_.reserve( 6 * static_cast<size_t>( quads ) );
}

void render_quad_batch( const SDL_Renderer_Ptr &renderer, const quad_batch &batch,
                        SDL_Texture *const texture )
{
    RenderGeometry( renderer, texture, batch.vertex_data(), batch.vertex_count(),
                    batch.index_data(), batch.index_count() );
}

void text_batch::add_rect( const SDL_Rect &rect, const SDL_Color &color )
{
    rects_.append_quad( static_cast<float>( rect.x ), static_cast<float>( rect.y ),
                        static_cast<float>( rect.w ), static_cast<float>( rect.h ),
                        to_fcolor( color ) );
}

void text_batch::add_glyph( SDL_Texture *const texture, const int tex_w, const int tex_h,
                            const SDL_Rect &dst, const SDL_Rect &src, const SDL_FColor &color )
{
    const auto active_end = glyphs_.begin() + static_cast<std::ptrdiff_t>( active_groups_ );
    auto group = std::find_if( glyphs_.begin(), active_end,
    [texture]( const std::pair<SDL_Texture *, quad_batch> &g ) {
        return g.first == texture;
    } );
    if( group == active_end ) {
        if( active_groups_ == glyphs_.size() ) {
            glyphs_.emplace_back( nullptr, quad_batch() );
        }
        group = glyphs_.begin() + static_cast<std::ptrdiff_t>( active_groups_ );
        group->first = texture;
        ++active_groups_;
    }
    group->second.append_textured_quad( dst, src, tex_w, tex_h, color );
}

bool text_batch::empty() const
{
    return rects_.empty() && active_groups_ == 0;
}

void text_batch::flush( const SDL_Renderer_Ptr &renderer )
{
    render_quad_batch( renderer, rects_ );
    for( size_t i = 0; i < active_groups_; ++i ) {
        render_quad_batch( renderer, glyphs_[i].second, glyphs_[i].first );
    }
    rects_.clear();
    // keep buffers for each group, only drop the texture it pointed at (which a
    // renderer recovery may destroy before the next pass)
    for( size_t i = 0; i < active_groups_; ++i ) {
        glyphs_[i].first = nullptr;
        glyphs_[i].second.clear();
    }
    active_groups_ = 0;
}

#endif // TILES
