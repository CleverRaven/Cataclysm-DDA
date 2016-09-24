#ifndef CATA_TILES_H
#define CATA_TILES_H

#include <SDL.h>
#include <SDL_ttf.h>

#include "animation.h"
#include "map.h"
#include "weather.h"
#include "tile_id_data.h"
#include "enums.h"
#include "weighted_list.h"

#include <list>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>

class JsonObject;
struct visibility_variables;

extern void set_displaybuffer_rendertarget();

void clear_texture_pool();

/** Structures */
struct tile_type {
    // fg and bg are both a weighted list of lists of sprite IDs
    weighted_int_list<std::vector<int>> fg, bg;
    bool multitile = false;
    bool rotates = false;
    int height_3d = 0;
    point offset = {0, 0};

    std::vector<std::string> available_subtiles;
};

struct tile {
    /** Screen coordinates as tile number */
    int sx, sy;
    /** World coordinates */
    int wx, wy;

    tile() {
        sx = wx = wy = 0;
    }
    tile( int x, int y, int x2, int y2 ) {
        sx = x;
        sy = y;
        wx = x2;
        wy = y2;
    }
};

/* Enums */
enum MULTITILE_TYPE {
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
enum TILE_CATEGORY {
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
struct SDL_Texture_deleter {
    // Operator overload required to leverage unique_ptr API.
    void operator()( SDL_Texture *const ptr );
};
using SDL_Texture_Ptr = std::unique_ptr<SDL_Texture, SDL_Texture_deleter>;

struct SDL_Surface_deleter {
    // Operator overload required to leverage unique_ptr API.
    void operator()( SDL_Surface *const ptr );
};
using SDL_Surface_Ptr = std::unique_ptr<SDL_Surface, SDL_Surface_deleter>;

// Cache of a single tile, used to avoid redrawing what didn't change.
struct tile_drawing_cache {

    tile_drawing_cache() { };

    // Sprite indices drawn on this tile.
    // The same indices in a different order need to be drawn differently!
    std::vector<tile_type *> sprites;
    std::vector<int> rotations;

    bool operator==( const tile_drawing_cache &other ) const {
        if( sprites.size() != other.sprites.size() ) {
            return false;
        } else {
            for( size_t i = 0; i < sprites.size(); ++i ) {
                if( sprites[i] != other.sprites[i] || rotations[i] != other.rotations[i] ) {
                    return false;
                }
            }
        }

        return true;
    }

    bool operator!=( const tile_drawing_cache &other ) const {
        return !( this->operator==( other ) );
    }

    void operator=( const tile_drawing_cache &other ) {
        this->sprites = other.sprites;
        this->rotations = other.rotations;
    }
};

struct pixel {
    int r;
    int g;
    int b;
    int a;

    pixel() : r( 0 ), g( 0 ), b( 0 ), a( 0 ) {
    }

    pixel( int sr, int sg, int sb, int sa ) : r( sr ), g( sg ), b( sb ), a( sa ) {
    }

    pixel( SDL_Color c ) {
        r = c.r;
        g = c.g;
        b = c.b;
        a = c.a;
    }

    SDL_Color getSdlColor() const {
        SDL_Color c;
        c.r = static_cast<Uint8>( r );
        c.g = static_cast<Uint8>( g );
        c.b = static_cast<Uint8>( b );
        c.a = static_cast<Uint8>( a );
        return c;
    }

    bool isBlack() const {
        return ( r == 0 && g == 0 && b == 0 );
    }

    bool operator==( const pixel &other ) const {
        return ( r == other.r && g == other.g && b == other.b && a == other.a );
    }

    bool operator!=( const pixel &other ) const {
        return !operator==( other );
    }
};

// a texture pool to avoid recreating textures every time player changes their view
// at most 142 out of 144 textures can be in use due to regular player movement
//  (moving from submap corner to new corner) with MAPSIZE = 11
// textures are dumped when the player moves more than one submap in one update
//  (teleporting, z-level change) to prevent running out of the remaining pool
struct minimap_shared_texture_pool {
    std::vector<SDL_Texture_Ptr> texture_pool;
    std::set<int> active_index;
    std::vector<int> inactive_index;
    minimap_shared_texture_pool() {
        reinit();
    }

    void reinit() {
        inactive_index.clear();
        texture_pool.resize( ( MAPSIZE + 1 ) * ( MAPSIZE + 1 ) );
        for( int i = 0; i < static_cast<int>( texture_pool.size() ); i++ ) {
            inactive_index.push_back( i );
        }
    }

    //reserves a texture from the inactive group and returns tracking info
    SDL_Texture_Ptr request_tex( int &i ) {
        if( inactive_index.empty() ) {
            //shouldn't be happening, but minimap will just be default color instead of crashing
            return nullptr;
        }
        int index = inactive_index.back();
        inactive_index.pop_back();
        active_index.insert( index );
        i = index;
        return std::move( texture_pool[index] );
    }

    //releases the provided texture back into the inactive pool to be used again
    //called automatically in the submap cache destructor
    void release_tex( int i, SDL_Texture_Ptr ptr ) {
        auto it = active_index.find( i );
        if( it == active_index.end() ) {
            return;
        }
        inactive_index.push_back( i );
        active_index.erase( i );
        texture_pool[i] = std::move( ptr );
    }
};

struct minimap_submap_cache {
    //the color stored for each submap tile
    std::vector< pixel > minimap_colors;
    //checks if the submap has been looked at by the minimap routine
    bool touched;
    //the texture updates are drawn to
    SDL_Texture_Ptr minimap_tex;
    //the submap being handled
    int texture_index;
    //the list of updates to apply to the texture
    //reduces render target switching to once per submap
    std::vector<point> update_list;
    //if the submap has been drawn to screen during the current draw cycle
    bool drawn;
    //flag used to indicate that the texture needs to be cleared before first use
    bool ready;

    //reserve the SEEX * SEEY submap tiles
    minimap_submap_cache();
    //handle the release of the borrowed texture
    ~minimap_submap_cache();
};

using minimap_cache_ptr = std::unique_ptr< minimap_submap_cache >;

class cata_tiles
{
    public:
        /** Default constructor */
        cata_tiles( SDL_Renderer *render );
        /** Default destructor */
        ~cata_tiles();
    protected:
        void clear();
    public:
        /** Reload tileset, with the given scale. Scale is divided by 16 to allow for scales < 1 without risking
         *  float inaccuracies. */
        void set_draw_scale( int scale );
    protected:
        /** Load tileset, R,G,B, are the color components of the transparent color
         * Returns the number of tiles that have been loaded from this tileset image
         * @throw std::exception If the image can not be loaded.
         */
        int load_tileset( std::string path, int R, int G, int B, int sprite_width, int sprite_height );

        /**
         * Load tileset config file (json format).
         * If the tileset uses the old system (one image per tileset) the image
         * path <B>image_path</B> is used to load the tileset image.
         * Otherwise (the tileset uses the new system) the image pathes
         * are loaded from the json entries.
         * @throw std::exception On any error.
         * @param tileset_root Path to tileset root directory.
         * @param json_conf Path to json config inside tileset_root.
         * @param image_path Path to tiles image inside tileset_root.
         */
        void load_tilejson( std::string tileset_root, std::string json_conf,
                            const std::string &image_path );

        /**
         * Try to load json tileset config. If json valid it lookup
         * it parses it and load tileset.
         * @throw std::exception On errors in the tileset definition.
         * @param tileset_dir Path to tileset root directory.
         * @param f File stream to read from.
         * @param image_path
         */
        void load_tilejson_from_file( const std::string &tileset_dir, std::ifstream &f,
                                      const std::string &image_path );

        /**
         * Load tiles from json data.This expects a "tiles" array in
         * <B>config</B>. That array should contain all the tile definition that
         * should be taken from an tileset image.
         * Because the function only loads tile definitions for a single tileset
         * image, only tile inidizes (tile_type::fg tile_type::bg) in the interval
         * [0,size].
         * The <B>offset</B> is automatically added to the tile index.
         * sprite offset dictates where each sprite should render in its tile
         * @throw std::exception On any error.
         */
        void load_tilejson_from_file( JsonObject &config, int offset, int size, int sprite_offset_x = 0,
                                      int sprite_offset_y = 0 );

        /**
         * Create a new tile_type, add it to tile_ids (using <B>id</B>).
         * Set the fg and bg properties of it (loaded from the json object).
         * Makes sure each is either -1, or in the interval [0,size).
         * If it's in that interval, adds offset to it, if it's not in the
         * interval (and not -1), throw an std::string error.
         */
        tile_type &load_tile( JsonObject &entry, const std::string &id, int offset, int size );

        void load_tile_spritelists( JsonObject &entry, weighted_int_list<std::vector<int>> &vs, int offset,
                                    int size, const std::string &objname );
        void load_ascii_tilejson_from_file( JsonObject &config, int offset, int size,
                                            int sprite_offset_x = 0, int sprite_offset_y = 0 );
        void load_ascii_set( JsonObject &entry, int offset, int size, int sprite_offset_x = 0,
                             int sprite_offset_y = 0 );
        void add_ascii_subtile( tile_type &curr_tile, const std::string &t_id, int fg,
                                const std::string &s_id );
        void process_variations_after_loading( weighted_int_list<std::vector<int>> &v, int offset );
    public:
        /** Draw to screen */
        void draw( int destx, int desty, const tripoint &center, int width, int height );

        /** Minimap functionality */
        void draw_minimap( int destx, int desty, const tripoint &center, int width, int height );
        void draw_rhombus( int destx, int desty, int size, SDL_Color color, int widthLimit,
                           int heightLimit );
    protected:
        /** How many rows and columns of tiles fit into given dimensions **/
        void get_window_tile_counts( const int width, const int height, int &columns, int &rows ) const;

        bool draw_from_id_string( std::string id, tripoint pos, int subtile, int rota, lit_level ll,
                                  bool apply_night_vision_goggles );
        bool draw_from_id_string( std::string id, TILE_CATEGORY category,
                                  const std::string &subcategory, tripoint pos, int subtile, int rota,
                                  lit_level ll, bool apply_night_vision_goggles );
        bool draw_from_id_string( std::string id, tripoint pos, int subtile, int rota, lit_level ll,
                                  bool apply_night_vision_goggles, int &height_3d );
        bool draw_from_id_string( std::string id, TILE_CATEGORY category,
                                  const std::string &subcategory, tripoint pos, int subtile, int rota,
                                  lit_level ll, bool apply_night_vision_goggles, int &height_3d );
        bool draw_sprite_at( const tile_type &tile, const weighted_int_list<std::vector<int>> &svlist,
                             int x, int y, unsigned int loc_rand, int rota_fg, int rota, lit_level ll,
                             bool apply_night_vision_goggles );
        bool draw_sprite_at( const tile_type &tile, const weighted_int_list<std::vector<int>> &svlist,
                             int x, int y, unsigned int loc_rand, int rota_fg, int rota, lit_level ll,
                             bool apply_night_vision_goggles, int &height_3d );
        bool draw_tile_at( const tile_type &tile, int x, int y, unsigned int loc_rand, int rota,
                           lit_level ll, bool apply_night_vision_goggles, int &height_3d );

        /**
         * Redraws all the tiles that have changed since the last frame.
         */
        void clear_buffer();

        /** Surface/Sprite rotation specifics */
        SDL_Surface_Ptr create_tile_surface();

        /* Tile Picking */
        void get_tile_values( const int t, const int *tn, int &subtile, int &rotation );
        void get_connect_values( const tripoint &p, int &subtile, int &rotation , int connect_group );
        void get_terrain_orientation( const tripoint &p, int &rota, int &subtype );
        void get_rotation_and_subtile( const char val, const int num_connects, int &rota, int &subtype );

        /** Drawing Layers */
        void draw_single_tile( const tripoint &p, const lit_level ll,
                               const visibility_variables &cache, int &height_3d );
        bool apply_vision_effects( const tripoint &pos, const visibility_type visibility );
        bool draw_terrain( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_terrain_below( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_furniture( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_trap( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_field_or_item( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_vpart( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_vpart_below( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_critter_at( const tripoint &p, lit_level ll, int &height_3d );
        bool draw_entity( const Creature &critter, const tripoint &p, lit_level ll, int &height_3d );
        void draw_entity_with_overlays( const player &pl, const tripoint &p, lit_level ll, int &height_3d );

        bool draw_item_highlight( const tripoint &pos );

    private:
        //surface manipulation
        SDL_Surface_Ptr create_tile_surface( int w, int h );

    public:
        // Animation layers
        bool draw_hit( const tripoint &p );

        void init_explosion( const tripoint &p, int radius );
        void draw_explosion_frame();
        void void_explosion();

        void init_custom_explosion_layer( const std::map<tripoint, explosion_tile> &layer );
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
                             std::string line_end_name, bool target_line );
        void draw_line();
        void void_line();

        void init_draw_weather( weather_printable weather, std::string name );
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
        /**
         * Initialize the current tileset (load tile images, load mapping), using the current
         * tileset as it is set in the options.
         * @throw std::exception On any error.
         */
        void init();
        /**
         * Reinitializes the current tileset, like @ref init, but using the original screen information.
         * @throw std::exception On any error.
         */
        void reinit();

        void reinit_minimap();

        int get_tile_height() const {
            return tile_height;
        }
        int get_tile_width() const {
            return tile_width;
        }
        float get_tile_ratiox() const {
            return tile_ratiox;
        }
        float get_tile_ratioy() const {
            return tile_ratioy;
        }
        void do_tile_loading_report();
    protected:
        void get_tile_information( std::string dir_path, std::string &json_path,
                                   std::string &tileset_path );
        template <typename maptype>
        void tile_loading_report( maptype const &tiletypemap, std::string const &label,
                                  std::string const &prefix = "" );
        template <typename arraytype>
        void tile_loading_report( arraytype const &array, int array_length, std::string const &label,
                                  std::string const &prefix = "" );
        template <typename basetype>
        void tile_loading_report( size_t count, std::string const &label, std::string const &prefix );
        /**
         * Generic tile_loading_report, begin and end are iterators, id_func translates the iterator
         * to an id string (result of id_func must be convertible to string).
         */
        template<typename Iter, typename Func>
        void lr_generic( Iter begin, Iter end, Func id_func, const std::string &label,
                         const std::string &prefix );
        /** Lighting */
        void init_light();

        /** Variables */
        SDL_Renderer *renderer;
        std::vector<SDL_Texture_Ptr> tile_values;
        std::unordered_map<std::string, tile_type> tile_ids;

        int tile_height = 0, tile_width = 0, default_tile_width, default_tile_height;
        // The width and height of the area we can draw in,
        // measured in map coordinates, *not* in pixels.
        int screentile_width, screentile_height;
        float tile_ratiox, tile_ratioy;
        // multiplier for pixel-doubling tilesets
        float tile_pixelscale;

        bool in_animation;

        bool do_draw_explosion;
        bool do_draw_custom_explosion;
        bool do_draw_bullet;
        bool do_draw_hit;
        bool do_draw_line;
        bool do_draw_weather;
        bool do_draw_sct;
        bool do_draw_zones;

        tripoint exp_pos;
        int exp_rad;

        std::map<tripoint, explosion_tile> custom_explosion_layer;

        tripoint bul_pos;
        std::string bul_id;

        tripoint hit_pos;
        std::string hit_entity_id;

        tripoint line_pos;
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

    private:
        void create_default_item_highlight();
        int last_pos_x, last_pos_y;
        std::vector<SDL_Texture_Ptr> shadow_tile_values;
        std::vector<SDL_Texture_Ptr> night_tile_values;
        std::vector<SDL_Texture_Ptr> overexposed_tile_values;
        /**
         * Tracks active night vision goggle status for each draw call.
         * Allows usage of night vision tilesets during sprite rendering.
         */
        bool nv_goggles_activated;

        //pixel minimap cache methods
        SDL_Texture_Ptr create_minimap_cache_texture( int tile_width, int tile_height );
        void process_minimap_cache_updates();
        void update_minimap_cache( const tripoint &loc, pixel &pix );
        void prepare_minimap_cache_for_updates();
        void clear_unused_minimap_cache();

        std::map< tripoint, minimap_cache_ptr> minimap_cache;

        //persistent tiled minimap values
        void init_minimap( int destx, int desty, int width, int height );
        bool minimap_prep;
        point minimap_min;
        point minimap_max;
        point minimap_tiles_range;
        point minimap_tile_size;
        point minimap_tiles_limit;
        int minimap_drawn_width;
        int minimap_drawn_height;
        int minimap_border_width;
        int minimap_border_height;
        SDL_Rect minimap_clip_rect;
        //track the previous viewing area to determine if the minimap cache needs to be cleared
        tripoint previous_submap_view;
        bool minimap_reinit_flag; //set to true to force a reallocation of minimap details
        //place all submaps on this texture before rendering to screen
        //replaces clipping rectangle usage while SDL still has a flipped y-coordinate bug
        SDL_Texture_Ptr main_minimap_tex;
};

#endif
