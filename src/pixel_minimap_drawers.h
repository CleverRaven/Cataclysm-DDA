#pragma once
#ifndef PIXEL_MINIMAP_DRAWERS_H
#define PIXEL_MINIMAP_DRAWERS_H

#include "enums.h"
#include "point.h"
#include "sdl_wrappers.h"


class pixel_minimap_drawer
{
    public:
        pixel_minimap_drawer() = default;
        virtual ~pixel_minimap_drawer() = default;

        virtual point get_tile_size() const = 0;
        virtual point get_tiles_size( const point &tiles_count ) const = 0;
        virtual point get_tile_pos( const point &p, const point &tiles_count ) const = 0;

        virtual SDL_Rect get_chunk_rect( const point &p, const point &tiles_count ) const = 0;
};


class pixel_minimap_ortho_drawer : public pixel_minimap_drawer
{
    public:
        pixel_minimap_ortho_drawer( const point &tiles_count, const point &max_screen_size,
                                    bool square_pixels );
        ~pixel_minimap_ortho_drawer() = default;

        point get_tile_size() const override;
        point get_tiles_size( const point &tiles_count ) const override;
        point get_tile_pos( const point &p, const point &tiles_count ) const override;

        SDL_Rect get_chunk_rect( const point &p, const point &tiles_count ) const override;

    private:
        point tile_size;
};


class pixel_minimap_iso_drawer : public pixel_minimap_drawer
{
    public:
        pixel_minimap_iso_drawer( const point &tiles_count, const point &max_screen_size,
                                  bool square_pixels );
        ~pixel_minimap_iso_drawer() = default;

        point get_tile_size() const override;
        point get_tiles_size( const point &tiles_count ) const override;
        point get_tile_pos( const point &p, const point &tiles_count ) const override;

        SDL_Rect get_chunk_rect( const point &p, const point &tiles_count ) const override;

    private:
        point tile_size;
};

#endif // PIXEL_MINIMAP_DRAWERS_H