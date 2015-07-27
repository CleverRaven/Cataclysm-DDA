#ifndef CATA_TILES_H
#define CATA_TILES_H

// make sure that SDL systems are included: Until testing is available for other systems, keep Windows specific
#if !(defined _WIN32 || defined WINDOWS)
#include <wordexp.h>
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "animation.h"
#include "map.h"
#include "weather.h"
#include "tile_id_data.h"
#include "enums.h"

#include <list>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>

class JsonObject;
struct visibility_variables;

/** Structures */
struct tile_type
{
    std::vector<int> fg, bg;
    bool multitile, rotates;

    std::vector<std::string> available_subtiles;

    tile_type()
    {
        multitile = rotates = false;
        available_subtiles.clear();
    }
};

struct tile
{
    /** Screen coordinates as tile number */
    int sx, sy;
    /** World coordinates */
    int wx, wy;

    tile()
    {
        sx = wx = wy = 0;
    }
    tile(int x, int y, int x2, int y2)
    {
        sx = x;
        sy = y;
        wx = x2;
        wy = y2;
    }
};

/* Enums */
enum MULTITILE_TYPE
{
    center,
    corner,
    edge,
    t_connection,
    end_piece,
    unconnected,
    open_,
    broken,
    num_multitile_types
};
// Make sure to change TILE_CATEGORY_IDS if this changes!
enum TILE_CATEGORY
{
    C_NONE,
    C_VEHICLE_PART,
    C_TERRAIN,
    C_ITEM,
    C_FURNITURE,
    C_TRAP,
    C_FIELD,
    C_LIGHTING,
    C_MONSTER,
    C_BULLET,
    C_HIT_ENTITY,
    C_WEATHER,
};

/** Typedefs */
typedef std::vector<SDL_Texture *> tile_map;
typedef std::unordered_map<std::string, tile_type *> tile_id_map;

typedef tile_map::iterator tile_iterator;
typedef tile_id_map::iterator tile_id_iterator;

// Cache of a single tile, used to avoid redrawing what didn't change.
struct tile_drawing_cache {

    tile_drawing_cache() { };

    // Sprite indices drawn on this tile.
    // The same indices in a different order need to be drawn differently!
    std::vector<tile_type *> sprites;
    std::vector<int> rotations;

    bool operator==(const tile_drawing_cache &other) const {
        if(sprites.size() != other.sprites.size()) {
            return false;
        } else {
            for( size_t i = 0; i < sprites.size(); ++i ) {
                if(sprites[i] != other.sprites[i] || rotations[i] != other.rotations[i]) {
                    return false;
                }
            }
        }

        return true;
    }

    bool operator!=(const tile_drawing_cache &other) const {
        return !(this->operator==(other));
    }

    void operator=(const tile_drawing_cache &other) {
        this->sprites = other.sprites;
        this->rotations = other.rotations;
    }
};

class cata_tiles
{
    public:
        /** Default constructor */
        cata_tiles(SDL_Renderer *render);
        /** Default destructor */
        ~cata_tiles();
    protected:
        void clear();
    public:
        /** Reload tileset, with the given scale. Scale is divided by 16 to allow for scales < 1 without risking
         *  float inaccuracies. */
        void set_draw_scale(int scale);
    protected:
        /** Load tileset, R,G,B, are the color components of the transparent color
         * throws std::string on errors. Returns the number of tiles that have
         * been loaded from this tileset image
         */
        int load_tileset(std::string path, int R, int G, int B);

        /**
         * Load tileset config file (json format).
         * If the tileset uses the old system (one image per tileset) the image
         * path <B>imagepath</B> is used to load the tileset image.
         * Otherwise (the tileset uses the new system) the image pathes
         * are loaded from the json entries.
         * throws std::string on errors.
         */
        void load_tilejson(std::string path, const std::string &imagepath);

        /**
         * throws std::string on errors.
         */
        void load_tilejson_from_file(std::ifstream &f, const std::string &imagepath);

        /**
         * Load tiles from json data.This expects a "tiles" array in
         * <B>config</B>. That array should contain all the tile definition that
         * should be taken from an tileset image.
         * Because the function only loads tile definitions for a single tileset
         * image, only tile inidizes (tile_type::fg tile_type::bg) in the interval
         * [0,size].
         * The <B>offset</B> is automatically added to the tile index.
         * throws std::string on errors.
         */
        void load_tilejson_from_file(JsonObject &config, int offset, int size);

        /**
         * Create a new tile_type, add it to tile_ids (using <B>id</B>).
         * Set the fg and bg properties of it (loaded from the json object).
         * Makes sure each is either -1, or in the interval [0,size).
         * If it's in that interval, adds offset to it, if it's not in the
         * interval (and not -1), throw an std::string error.
         */
        tile_type *load_tile(JsonObject &entry, const std::string &id, int offset, int size);

        void load_ascii_tilejson_from_file(JsonObject &config, int offset, int size);
        void load_ascii_set(JsonObject &entry, int offset, int size);
        void add_ascii_subtile(tile_type *curr_tile, const std::string &t_id, int fg, const std::string &s_id);
    public:
        /** Draw to screen */
        void draw( int destx, int desty, const tripoint &center, int width, int height );
    protected:
        /** How many rows and columns of tiles fit into given dimensions **/
        void get_window_tile_counts(const int width, const int height, int &columns, int &rows) const;

        bool draw_from_id_string(std::string id, int x, int y, int subtile, int rota);
        bool draw_from_id_string(std::string id, TILE_CATEGORY category,
                                 const std::string &subcategory, int x, int y, int subtile, int rota);
        bool draw_sprite_at(std::vector<int>& spritelist, int x, int y, int rota);
        bool draw_tile_at(tile_type *tile, int x, int y, int rota);

        /**
         * Redraws all the tiles that have changed since the last frame.
         */
        void clear_buffer();

        /** Surface/Sprite rotation specifics */
        SDL_Surface *create_tile_surface();

        /* Tile Picking */
        void get_tile_values(const int t, const int *tn, int &subtile, int &rotation);
        void get_wall_values( const tripoint &p, int &subtile, int &rotation );
        void get_terrain_orientation( const tripoint &p, int &rota, int &subtype );
        void get_rotation_and_subtile(const char val, const int num_connects, int &rota, int &subtype);

        /** Drawing Layers */
        void draw_single_tile( const tripoint &p, const lit_level ll,
                               const visibility_variables &cache );
        bool apply_vision_effects( int x, int y, const visibility_type visibility);
        bool draw_terrain( const tripoint &p );
        bool draw_furniture( const tripoint &p );
        bool draw_trap( const tripoint &p );
        bool draw_field_or_item( const tripoint &p );
        bool draw_vpart( const tripoint &p );
        bool draw_entity( const Creature &critter, const tripoint &p );
        void draw_entity_with_overlays( const player &pl, const tripoint &p );

        bool draw_item_highlight(int x, int y);

    public:
        // Animation layers
        bool draw_hit( const tripoint &p );

        void init_explosion( const tripoint &p, int radius );
        void draw_explosion_frame();
        void void_explosion();

        void init_custom_explosion_layer( const std::map<point, explosion_tile> &layer );
        void draw_custom_explosion_frame();
        void void_custom_explosion();

        void init_draw_bullet( const tripoint &p, std::string name );
        void draw_bullet_frame();
        void void_bullet();

        void init_draw_hit( const tripoint &p, std::string name );
        void draw_hit_frame();
        void void_hit();

        void draw_footsteps_frame();

        // pseudo-animated layer, not really though.
        void init_draw_line( const tripoint &p, std::vector<tripoint> trajectory,
                             std::string line_end_name, bool target_line);
        void draw_line();
        void void_line();

        void init_draw_weather(weather_printable weather, std::string name);
        void draw_weather_frame();
        void void_weather();

        void init_draw_sct();
        void draw_sct_frame();
        void void_sct();

        void init_draw_zones( const tripoint &start, const tripoint &end, const tripoint &offset );
        void draw_zones_frame();
        void void_zones();

        /** Overmap Layer : Not used for now, do later*/
        bool draw_omap();

    public:
        /* initialize from an outside file, throws std::string on errors. */
        void init(std::string load_file_path);
        /* Reinitializes the tile context using the original screen information, throws std::string on errors  */
        void reinit(std::string load_file_path);
        int get_tile_height() const { return tile_height; }
        int get_tile_width() const { return tile_width; }
        float get_tile_ratiox() const { return tile_ratiox; }
        float get_tile_ratioy() const { return tile_ratioy; }
        void do_tile_loading_report();
        bool tile_iso;
    protected:
        void get_tile_information(std::string dir_path, std::string &json_path, std::string &tileset_path);
        template <typename maptype>
        void tile_loading_report(maptype const & tiletypemap, std::string const & label, std::string const & prefix = "");
        template <typename arraytype>
        void tile_loading_report(arraytype const & array, int array_length, std::string const & label, std::string const & prefix = "");
        template <typename basetype>
        void tile_loading_report(size_t count, std::string const & label, std::string const & prefix);
        /** Lighting */
        void init_light();

        /** Variables */
        SDL_Renderer *renderer;
        tile_map tile_values;
        tile_id_map tile_ids;

        int tile_height, tile_width, default_tile_width, default_tile_height;
        // The width and height of the area we can draw in,
        // measured in map coordinates, *not* in pixels.
        int screentile_width, screentile_height;
        float tile_ratiox, tile_ratioy;

        bool in_animation;

        bool do_draw_explosion;
        bool do_draw_custom_explosion;
        bool do_draw_bullet;
        bool do_draw_hit;
        bool do_draw_line;
        bool do_draw_weather;
        bool do_draw_sct;
        bool do_draw_zones;

        int exp_pos_x, exp_pos_y, exp_rad;

        std::map<point, explosion_tile> custom_explosion_layer;

        int bul_pos_x, bul_pos_y;
        std::string bul_id;

        int hit_pos_x, hit_pos_y;
        std::string hit_entity_id;

        int line_pos_x, line_pos_y;
        bool is_target_line;
        std::vector<tripoint> line_trajectory;
        std::string line_endpoint_id;

        weather_printable anim_weather;
        std::string weather_name;

        tripoint zone_start;
        tripoint zone_end;
        tripoint zone_offset;

        // offset values, in tile coordinates, not pixels
        int o_x, o_y;
        // offset for drawing, in pixels.
        int op_x, op_y;

    protected:
    private:
        void create_default_item_highlight();
        int last_pos_x, last_pos_y;
};

#endif
