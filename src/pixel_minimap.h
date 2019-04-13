#pragma once
#ifndef MINIMAP_H
#define MINIMAP_H

#include "enums.h"
#include "sdl_wrappers.h"

#include <cmath>
#include <map>
#include <memory>

extern void set_displaybuffer_rendertarget();

class pixel_minimap
{
    public:
        pixel_minimap( const SDL_Renderer_Ptr &renderer );
        ~pixel_minimap();

        void draw( int destx, int desty, const tripoint &center, int width, int height );
        void reinit();

    private:
        void init( int destx, int desty, int width, int height );

        void draw_rhombus( int destx, int desty, int size, SDL_Color color, int widthLimit,
                           int heightLimit );

        SDL_Texture_Ptr create_cache_texture( int tile_width, int tile_height );
        void process_cache_updates();
        void update_cache( const tripoint &loc, const SDL_Color &color );
        void prepare_cache_for_updates();
        void clear_unused_cache();

    private:
        bool prep;
        bool reinit_flag; //set to true to force a reallocation of minimap details
        //place all submaps on this texture before rendering to screen
        //replaces clipping rectangle usage while SDL still has a flipped y-coordinate bug

        point min_size;
        point max_size;
        point tiles_range;
        point tile_size;
        point tiles_limit;

        //track the previous viewing area to determine if the minimap cache needs to be cleared
        tripoint previous_submap_view;

        int drawn_width;
        int drawn_height;
        int border_width;
        int border_height;

        SDL_Rect clip_rect;
        SDL_Texture_Ptr main_tex;
        const SDL_Renderer_Ptr &renderer;

        //the minimap texture pool which is used to reduce new texture allocation spam
        struct shared_texture_pool;
        std::unique_ptr<shared_texture_pool> tex_pool;

        struct submap_cache;
        std::map<tripoint, submap_cache> cache;
};

#endif // MINIMAP_H
