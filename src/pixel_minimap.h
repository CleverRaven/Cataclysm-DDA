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

        submap_cache &get_cache_at( const tripoint &submap_pos );

        void set_screen_rect( const SDL_Rect &screen_rect );
        void reset();

        void draw_rhombus( int destx, int desty, int size, SDL_Color color, int widthLimit,
                           int heightLimit );

        void process_cache_updates();

        void update_cache_at( const tripoint &pos );

        void prepare_cache_for_updates();
        void clear_unused_cache();

        void render_cache_to_screen( const tripoint &center );
        void render_critters_to_screen( const tripoint &center );

    private:
        const SDL_Renderer_Ptr &renderer;

        point tile_size;
        point tiles_limit;

        //track the previous viewing area to determine if the minimap cache needs to be cleared
        tripoint previous_submap_view;

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
