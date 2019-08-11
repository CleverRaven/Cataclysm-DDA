#pragma once
#ifndef PIXEL_MINIMAP_PROJECTORS_H
#define PIXEL_MINIMAP_PROJECTORS_H

#include "enums.h"
#include "point.h"
#include "sdl_wrappers.h"


class pixel_minimap_projector
{
    public:
        pixel_minimap_projector() = default;
        virtual ~pixel_minimap_projector() = default;

        virtual point get_tile_size() const = 0;
        virtual point get_tiles_size( const point &tiles_count ) const = 0;
        virtual point get_tile_pos( const point &p, const point &tiles_count ) const = 0;

        virtual SDL_Rect get_chunk_rect( const point &p, const point &tiles_count ) const = 0;
};


class pixel_minimap_ortho_projector : public pixel_minimap_projector
{
    public:
        pixel_minimap_ortho_projector( const point &total_tiles_count,
                                       const SDL_Rect &max_screen_rect,
                                       bool square_pixels );

        point get_tile_size() const override;
        point get_tiles_size( const point &tiles_count ) const override;
        point get_tile_pos( const point &p, const point &tiles_count ) const override;

        SDL_Rect get_chunk_rect( const point &p, const point &tiles_count ) const override;

    private:
        point tile_size;
};


class pixel_minimap_iso_projector : public pixel_minimap_projector
{
    public:
        pixel_minimap_iso_projector( const point &total_tiles_count,
                                     const SDL_Rect &max_screen_rect,
                                     bool square_pixels );

        point get_tile_size() const override;
        point get_tiles_size( const point &tiles_count ) const override;
        point get_tile_pos( const point &p, const point &tiles_count ) const override;

        SDL_Rect get_chunk_rect( const point &p, const point &tiles_count ) const override;

    private:
        point total_tiles_count;
        point tile_size;
};

#endif // PIXEL_MINIMAP_PROJECTORS_H
