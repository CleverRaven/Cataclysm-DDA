#pragma once
#ifndef CATA_SRC_TILESET_LOADER_H
#define CATA_SRC_TILESET_LOADER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cata_tiles.h"
#include "point.h"
#include "sdl_wrappers.h"

class JsonObject;
class cata_path;
template <typename T> struct weighted_int_list;

class tileset_cache::loader
{
    public:
        // Destination vectors for the six color-filter variants. Lets the
        // texture-creation path target candidate vectors instead of the live
        // tileset.
        struct tile_value_targets {
            std::vector<texture> *normal = nullptr;
            std::vector<texture> *shadow = nullptr;
            std::vector<texture> *night = nullptr;
            std::vector<texture> *overexposed = nullptr;
            std::vector<texture> *memory = nullptr;
            std::vector<texture> *silhouette = nullptr;
        };

    private:
        tileset &ts;
        const SDL_Renderer_Ptr &renderer;
        std::string memory_map_mode;

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

        void copy_surface_to_texture( const SDL_Surface_Ptr &surf, const point &offset,
                                      std::vector<texture> &target,
                                      const std::vector<SDL_Rect> &opaque_bounds );
        void create_textures_from_tile_atlas( const SDL_Surface_Ptr &tile_atlas, const point &offset,
                                              const tile_value_targets &targets );

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
        // Read atlas image header, extend tileset value vectors, append an
        // atlas_replay_descriptor. No GPU work. kr/kg/kb identify the
        // transparent color; all-negative disables color keying.
        void read_image_dimensions( const cata_path &img_path, int kr, int kg, int kb );
        // Parse tile-id -> sprite-index mappings from a tileset JSON object.
        // Indices in fg/bg are validated against [0,size) and shifted by
        // offset.
        void parse_mappings( const JsonObject &config );
        // Walk every atlas in config (tiles-new array, or the single-atlas
        // fallback when no tiles-new is present) running
        // read_image_dimensions + parse_mappings on each. pump_events
        // refreshes the screen mid-walk; do not access the tileset while it
        // is true.
        void parse_atlases( const JsonObject &config, const cata_path &tileset_root,
                            const cata_path &img_path, bool pump_events );

        // Load layering data from json.
        void load_layers( const JsonObject &config );

        // Upload one atlas: split to renderer-sized chunks, apply color
        // keying, emit all six filter variants. Mutates loader sprite-state
        // from the descriptor.
        void upload_one_atlas( const atlas_replay_descriptor &desc,
                               const tile_value_targets &targets,
                               bool pump_events );

    public:
        loader( tileset &ts, const SDL_Renderer_Ptr &r, std::string memory_map_mode )
            : ts( ts ), renderer( r ), memory_map_mode( std::move( memory_map_mode ) ) {
        }
        // tileset_id matches the option string. precheck only loads metadata
        // (tile dimensions). pump_events handles window events / refreshes
        // the screen during the upload pass. terrain marks this as an
        // overmap/terrain tileset.
        void load( const std::string &tileset_id, bool precheck, bool pump_events = false,
                   bool terrain = false );

        // Upload every descriptor and regenerate the default highlight when
        // active. Builds candidate vectors and swaps them into ts on full
        // success; ts is unchanged on failure. Stamps the upload generations
        // onto the bundle so a later cache lookup can spot a stale upload.
        static void upload_atlases( tileset &ts, const SDL_Renderer_Ptr &renderer,
                                    const std::string &memory_map_mode,
                                    const std::vector<atlas_replay_descriptor> &descriptors,
                                    uint64_t renderer_instance_generation,
                                    uint64_t gpu_textures_generation,
                                    bool pump_events );
};

#endif // CATA_SRC_TILESET_LOADER_H
