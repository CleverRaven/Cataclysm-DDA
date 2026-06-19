#pragma once
#ifndef CATA_SRC_PIXEL_MINIMAP_H
#define CATA_SRC_PIXEL_MINIMAP_H

#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "coordinates.h"
#include "map_scale_constants.h"
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

        // Drop every renderer-owned GPU resource (main texture, texture
        // pool) along with the projector and submap cache. All of it
        // rebuilds lazily on the next draw().
        void reset();

        // True if the last draw() rendered any critters with blinking beacons.
        bool has_blinking_beacons() const {
            return has_blinking_beacons_;
        }

    private:
        // Texture pool, defined in the .cpp; forward declared so submap_cache can hold a ref.
        class shared_texture_pool;

        struct submap_cache {
            //the color stored for each submap tile
            std::array<SDL_Color, SEEX *SEEY> minimap_colors = {};
            //checks if the submap has been looked at by the minimap routine
            bool touched = false;
            //the texture updates are drawn to
            SDL_Texture_Ptr chunk_tex;
            //the submap being handled
            size_t texture_index = 0;
            //the list of updates to apply to the texture
            //reduces render target switching to once per submap
            std::vector<point> update_list;
            //flag used to indicate that the texture needs to be cleared before first use
            bool ready = false;
            shared_texture_pool &pool;

            explicit submap_cache( shared_texture_pool &pool );
            ~submap_cache();

            submap_cache( const submap_cache & ) = delete;
            submap_cache( submap_cache && ) = default;

            SDL_Color &color_at( const point &p );
        };

        submap_cache &get_cache_at( const tripoint_abs_sm &abs_sm_pos );

        void set_screen_rect( const SDL_Rect &screen_rect );

        void draw_beacon( const SDL_Rect &rect, const SDL_Color &color );

        void process_cache( const tripoint_bub_ms &center );

        void flush_cache_updates();
        void update_cache_at( const tripoint_bub_sm &pos );
        void prepare_cache_for_updates( const tripoint_bub_ms &center );
        void clear_unused_cache();

        void render( const tripoint_bub_ms &center );
        void render_cache( const tripoint_bub_ms &center );
        void render_critters( const tripoint_bub_ms &center );

        std::unique_ptr<pixel_minimap_projector> create_projector( const SDL_Rect &max_screen_rect ) const;

        const SDL_Renderer_Ptr &renderer;
        const GeometryRenderer_Ptr &geometry;

        pixel_minimap_type type;
        pixel_minimap_settings settings;

        point pixel_size;

        //track the previous viewing area to determine if the minimap cache needs to be cleared
        tripoint_abs_sm cached_center_sm;

        SDL_Rect screen_rect;
        SDL_Rect main_tex_clip_rect;
        SDL_Rect screen_clip_rect;

        SDL_Texture_Ptr main_tex;

        std::unique_ptr<pixel_minimap_projector> projector;

        std::unique_ptr<shared_texture_pool> tex_pool;

        std::unordered_map<tripoint_abs_sm, submap_cache> cache;

        bool has_blinking_beacons_ = false;
};

#endif // CATA_SRC_PIXEL_MINIMAP_H
