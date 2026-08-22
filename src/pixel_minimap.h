#pragma once
#ifndef CATA_SRC_PIXEL_MINIMAP_H
#define CATA_SRC_PIXEL_MINIMAP_H

#include <memory>

#include "coordinates.h"
#include "pixel_minimap_geometry.h"
#include "point.h"
#include "sdl_wrappers.h"
#include "sdl_geometry.h"

class pixel_minimap_projector;

enum class pixel_minimap_type : int {
    ortho,
    iso
};

enum class pixel_minimap_mode : int {
    solid,
    squares,
    dots
};

struct pixel_minimap_settings {
    pixel_minimap_mode mode = pixel_minimap_mode::solid;
    int brightness = 100;
    int beacon_size = 2;
    int beacon_blink_interval = 0;
    bool square_pixels = true;
    bool scale_to_fit = false;
};

class pixel_minimap
{
    public:
        pixel_minimap( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry );
        ~pixel_minimap();

        void set_type( pixel_minimap_type type );
        void set_settings( const pixel_minimap_settings &settings );

        void draw( const SDL_Rect &screen_rect, const tripoint_bub_ms &center );

        // The projector and screen transform rebuild lazily on the next draw().
        void reset();

        // True if the last draw() rendered any critters with blinking beacons.
        bool has_blinking_beacons() const {
            return has_blinking_beacons_;
        }

    private:
        void set_screen_rect( const SDL_Rect &screen_rect );
        void build_batches( const tripoint_bub_ms &center );
        void present();

        std::unique_ptr<pixel_minimap_projector> create_projector(
            const SDL_Rect &max_screen_rect ) const;

        const SDL_Renderer_Ptr &renderer;
        const GeometryRenderer_Ptr &geometry;

        pixel_minimap_type type;
        pixel_minimap_settings settings;

        point pixel_size;

        SDL_Rect screen_rect;
        minimap_transform tf_;

        std::unique_ptr<pixel_minimap_projector> projector;

        minimap_vertex_batch terrain_batch_;
        minimap_vertex_batch beacon_batch_;

        bool has_blinking_beacons_ = false;
};

#endif // CATA_SRC_PIXEL_MINIMAP_H
