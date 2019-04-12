#pragma once
#ifndef PIXEL_MINIMAP_DRAWERS_H
#define PIXEL_MINIMAP_DRAWERS_H

#include "enums.h"
#include "point.h"
#include "sdl_wrappers.h"


enum class pixel_minimap_mode {
    solid,
    squares,
    dots
};


class pixel_minimap_drawer
{
    public:
        pixel_minimap_drawer() = default;
        virtual ~pixel_minimap_drawer() = default;

        virtual point get_size_on_screen() const = 0;
        virtual point get_tiles_count() const = 0;

        virtual SDL_Rect get_chunk_rect( const point &p ) const = 0;
        virtual SDL_Rect get_critter_rect( const point &p ) const = 0;

        virtual void clear_chunk_tex( const SDL_Renderer_Ptr &renderer ) = 0;
        virtual void update_chunk_tex( const SDL_Renderer_Ptr &renderer,
                                       const point &p,
                                       const SDL_Color &c ) = 0;
};


class pixel_minimap_ortho_drawer : public pixel_minimap_drawer
{
    public:
        pixel_minimap_ortho_drawer( const point &max_size, bool square_pixels, pixel_minimap_mode mode );
        ~pixel_minimap_ortho_drawer() = default;

        point get_size_on_screen() const override;
        point get_tiles_count() const override;

        SDL_Rect get_chunk_rect( const point &p ) const override;
        SDL_Rect get_critter_rect( const point &p ) const override;

        void clear_chunk_tex( const SDL_Renderer_Ptr &renderer ) override;
        void update_chunk_tex( const SDL_Renderer_Ptr &renderer,
                               const point &p,
                               const SDL_Color &c ) override;

    private:
        point tile_size;
        point tiles_count;
        point pixel_size;
        bool draw_with_dots;
};


#endif // PIXEL_MINIMAP_DRAWERS_H