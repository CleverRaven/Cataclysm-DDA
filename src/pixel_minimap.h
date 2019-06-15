#pragma once
#ifndef MINIMAP_H
#define MINIMAP_H

#include "enums.h"
#include "sdl_wrappers.h"

#include <cmath>
#include <map>
#include <memory>

enum class pixel_minimap_mode {
    solid,
    squares,
    dots
};

struct pixel_minimap_settings {
    pixel_minimap_mode mode = pixel_minimap_mode::solid;
    int brightness = 100;
    int blink_interval = 0;
    bool square_pixels = true;
};

class pixel_minimap
{
    public:
        pixel_minimap( const SDL_Renderer_Ptr &renderer );
        ~pixel_minimap();

        void set_settings( const pixel_minimap_settings &settings );

        void draw( const SDL_Rect &screen_rect, const tripoint &center );

    private:
        struct submap_cache;

        submap_cache &get_cache_at( const tripoint &abs_sm_pos );

        void set_screen_rect( const SDL_Rect &screen_rect );
        void reset();

        void draw_beacon( const SDL_Rect &rect, const SDL_Color &color );

        void process_cache( const tripoint &center );

        void flush_cache_updates();
        void update_cache_at( const tripoint &pos );
        void prepare_cache_for_updates( const tripoint &center );
        void clear_unused_cache();

        void render( const tripoint &center );
        void render_cache( const tripoint &center );
        void render_critters( const tripoint &center );

        SDL_Rect get_map_chunk_rect( const point &p ) const;
        SDL_Rect get_critter_rect( const point &p ) const;

    private:
        const SDL_Renderer_Ptr &renderer;

        point tile_size;
        point tiles_limit;

        //track the previous viewing area to determine if the minimap cache needs to be cleared
        tripoint cached_center_sm;

        pixel_minimap_settings settings;

        SDL_Rect screen_rect;
        SDL_Rect clip_rect;
        SDL_Texture_Ptr main_tex;

        //the minimap texture pool which is used to reduce new texture allocation spam
        struct shared_texture_pool;
        std::unique_ptr<shared_texture_pool> tex_pool;

        std::map<tripoint, submap_cache> cache;
};

#endif // MINIMAP_H
