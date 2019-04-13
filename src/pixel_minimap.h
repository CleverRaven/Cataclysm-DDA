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
        void init_minimap( int destx, int desty, int width, int height );

        void draw_rhombus( int destx, int desty, int size, SDL_Color color, int widthLimit,
                           int heightLimit );

        SDL_Texture_Ptr create_minimap_cache_texture( int tile_width, int tile_height );
        void process_minimap_cache_updates();
        void update_minimap_cache( const tripoint &loc, const SDL_Color &color );
        void prepare_minimap_cache_for_updates();
        void clear_unused_minimap_cache();

    private:
        bool minimap_prep;
        bool minimap_reinit_flag; //set to true to force a reallocation of minimap details
        //place all submaps on this texture before rendering to screen
        //replaces clipping rectangle usage while SDL still has a flipped y-coordinate bug

        point minimap_min;
        point minimap_max;
        point minimap_tiles_range;
        point minimap_tile_size;
        point minimap_tiles_limit;

        //track the previous viewing area to determine if the minimap cache needs to be cleared
        tripoint previous_submap_view;

        int minimap_drawn_width;
        int minimap_drawn_height;
        int minimap_border_width;
        int minimap_border_height;

        SDL_Rect minimap_clip_rect;
        SDL_Texture_Ptr main_minimap_tex;
        const SDL_Renderer_Ptr &renderer;

        //the minimap texture pool which is used to reduce new texture allocation spam
        struct shared_texture_pool;
        std::unique_ptr<shared_texture_pool> tex_pool;

        struct submap_cache;
        std::map<tripoint, submap_cache> minimap_cache;
};

#endif // MINIMAP_H
