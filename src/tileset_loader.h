#pragma once
#ifndef CATA_SRC_TILESET_LOADER_H
#define CATA_SRC_TILESET_LOADER_H

#include <string>
#include <string_view>
#include <vector>

#include "cata_tiles.h"
#include "point.h"
#include "sdl_wrappers.h"

class JsonObject;
class cata_path;
template <typename T> struct weighted_int_list;

class tileset_cache::loader
{
    private:
        tileset &ts;
        const SDL_Renderer_Ptr &renderer;

        point sprite_offset;
        point sprite_offset_retracted;
        float sprite_pixelscale = 1.0;

        int sprite_width = 0;
        int sprite_height = 0;

        int offset = 0;
        int sprite_id_offset = 0;
        int size = 0;

        int R = 0;
        int G = 0;
        int B = 0;

        int tile_atlas_width = 0;

        void ensure_default_item_highlight();

        void copy_surface_to_texture( const SDL_Surface_Ptr &surf, const point &offset,
                                      std::vector<texture> &target,
                                      const std::vector<SDL_Rect> &opaque_bounds );
        void create_textures_from_tile_atlas( const SDL_Surface_Ptr &tile_atlas, const point &offset );

        void process_variations_after_loading( weighted_int_list<std::vector<int>> &v ) const;

        void add_ascii_subtile( tile_type &curr_tile, const std::string &t_id, int sprite_id,
                                const std::string &s_id );
        void load_ascii_set( const JsonObject &entry );
        // Create a new tile_type, add it to tile_ids (using id). Sets fg/bg
        // properties from the json object. Throws if either is outside [0,size)
        // and not -1.
        tile_type &load_tile( const JsonObject &entry, const std::string &id );

        void load_tile_spritelists( const JsonObject &entry, weighted_int_list<std::vector<int>> &vs,
                                    std::string_view objname ) const;

        void load_ascii( const JsonObject &config );
        // Loads the tileset image and emits texture variants. R, G, B identify
        // the transparent color. pump_events handles window events / refreshes
        // the screen during the load; ensure the tileset is not accessed while
        // pump_events is true.
        void load_tileset( const cata_path &path, bool pump_events );
        // Load tile definitions from json. config must have a "tiles" array
        // with definitions for the previously-loaded tileset image. Sprite
        // indices in tile_type::fg / tile_type::bg are validated against
        // [0,size) and adjusted by offset.
        void load_tilejson_from_file( const JsonObject &config );
        // Helper for load. pump_events same caveat as load_tileset.
        void load_internal( const JsonObject &config, const cata_path &tileset_root,
                            const cata_path &img_path, bool pump_events );

        // Load layering data from json.
        void load_layers( const JsonObject &config );

    public:
        loader( tileset &ts, const SDL_Renderer_Ptr &r ) : ts( ts ), renderer( r ) {
        }
        // tileset_id matches the option string. precheck only loads metadata
        // (tile dimensions). pump_events same caveat as load_tileset. terrain
        // marks this as an overmap/terrain tileset.
        void load( const std::string &tileset_id, bool precheck, bool pump_events = false,
                   bool terrain = false );
};

#endif // CATA_SRC_TILESET_LOADER_H
