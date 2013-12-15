#ifndef CATA_TILES_H
#define CATA_TILES_H

// make sure that SDL systems are included: Until testing is available for other systems, keep Windows specific
#if (defined _WIN32 || defined WINDOWS)
//#include <windows.h>
#include "SDL.h"
#include "SDL_ttf.h"
#else
#include <wordexp.h>
#if (defined OSX_SDL_FW)
#include "SDL.h"
#include "SDL_ttf/SDL_ttf.h"
#else
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#endif
#endif

#include "game.h"
#include "options.h"
#include "mapdata.h"
#include "tile_id_data.h"
#include "enums.h"
#include "file_finder.h"



/** Structures */
struct tile_type
{
    int fg, bg;
    bool multitile, rotates;

    std::vector<std::string> available_subtiles;

    tile_type()
    {
        fg = bg = 0;
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
enum LIGHTING
{
    HIDDEN = -1,
    CLEAR = 0,
    LIGHT_NORMAL = 1,
    LIGHT_DARK = 2,
    BOOMER_NORMAL = 3,
    BOOMER_DARK = 4
};
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

/** Typedefs */
typedef std::map<int, SDL_Surface*> tile_map;
typedef std::map<std::string, tile_type*> tile_id_map;

typedef tile_map::iterator tile_iterator;
typedef tile_id_map::iterator tile_id_iterator;

// Cache of a single tile, used to avoid redrawing what didn't change.
struct tile_drawing_cache {

    tile_drawing_cache() { };

    // Sprite indices drawn on this tile.
    // The same indices in a different order need to be drawn differently!
    std::vector<tile_type*> sprites;
    std::vector<int> rotations;

    bool operator==(const tile_drawing_cache& other) const {
        if(sprites.size() != other.sprites.size()) {
            return false;
        } else {
            for(int i=0; i<sprites.size(); i++) {
                if(sprites[i] != other.sprites[i] || rotations[i] != other.rotations[i]) {
                    return false;
                }
            }
        }

        return true;
    }

    bool operator!=(const tile_drawing_cache& other) const {
        return !(this->operator==(other));
    }

    void operator=(const tile_drawing_cache& other) {
        this->sprites = other.sprites;
        this->rotations = other.rotations;
    }
};

class cata_tiles
{
    public:
        /** Default constructor */
        cata_tiles();
        /** Default destructor */
        ~cata_tiles();
        /** Load tileset */
        void load_tileset(std::string path);
        /** Load tileset config file */
        void load_tilejson(std::string path);
        void load_tilejson_from_file(std::ifstream &f);
        /* After loading tileset, create additional cache for rotating each tile assuming it is used for rotations */
        void create_rotation_cache();
        /** Draw to screen */
        void draw(int destx, int desty, int centerx, int centery, int width, int height);

        /** How many rows and columns of tiles fit into given dimensions **/
        void get_window_tile_counts(const int width, const int height, int &columns, int &rows) const;
        int get_tile_width() const;

        bool draw_from_id_string(std::string id, int x, int y, int subtile, int rota, bool is_at_screen_position = false);
        bool draw_tile_at(tile_type *tile, int x, int y, int rota);

        /**
         * Redraws all the tiles that have changed since the last frame.
         */
        void apply_changes();

        /** Surface/Sprite rotation specifics */
        SDL_Surface *rotate_tile(SDL_Surface *src, SDL_Rect *rect, int rota);
        void put_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
        Uint32 get_pixel(SDL_Surface *surface, int x, int y);
        /* Tile Picking */
        void get_tile_values(const int t, const int *tn, int &subtile, int &rotation);

        void get_wall_values(const int x, const int y, const long vertical_wall_symbol, const long horizontal_wall_symbol, int &subtile, int &rotation);
        void get_terrain_orientation(int x, int y, int &rota, int &subtype);

        void get_rotation_and_subtile(const char val, const int num_connects, int &rota, int &subtype);

        /** Drawing Layers */
        bool draw_lighting(int x, int y, LIGHTING l);
        bool draw_terrain(int x, int y);
        bool draw_furniture(int x, int y);
        bool draw_trap(int x, int y);
        bool draw_field_or_item(int x, int y);
        bool draw_vpart(int x, int y);
        bool draw_entity(int x, int y);

        bool draw_item_highlight(int x, int y);

        // Animation layers
        bool draw_hit(int x, int y);

        void init_explosion(int x, int y, int radius);
        void draw_explosion_frame(int destx, int desty, int centerx, int centery, int width, int height);
        void void_explosion();

        void init_draw_bullet(int x, int y, std::string name);
        void draw_bullet_frame(int destx, int desty, int centerx, int centery, int width, int height);
        void void_bullet();

        void init_draw_hit(int x, int y, std::string name);
        void draw_hit_frame(int destx, int desty, int centerx, int centery, int width, int height);
        void void_hit();

        void init_draw_footsteps(std::queue<point> steps);
        void draw_footsteps_frame(int destx, int desty, int centerx, int centery, int width, int height);
        void void_footsteps();

        // pseudo-animated layer, not really though.
        void init_draw_line(int x, int y, std::vector<point> trajectory, std::string line_end_name, bool target_line);
        void draw_line(int destx, int desty, int centerx, int centery, int width, int height);
        void void_line();

        void init_draw_weather(weather_printable weather, std::string name);
        void draw_weather_frame(int destx, int desty, int centerx, int centery, int width, int height);
        void void_weather();

        /** Overmap Layer : Not used for now, do later*/
        bool draw_omap();

        /**
         * Scroll the map widget by (x, y)
         *
         * scroll(1, 1) would move the view one to the right and one down.
         *
         * Useful for keeping part of the previous "frame" and redrawing
         * only what changed.
         */
        void scroll(int x, int y);

        /** Used to properly initialize everything for display */
        void init(SDL_Surface *screen, std::string json_path, std::string tileset_path);
        /* initialize from an outside file */
        void init(SDL_Surface *screen, std::string load_file_path);
        /* Reinitializes the tile context using the original screen information */
        void reinit(std::string load_file_path);
        void get_tile_information(std::string dir_path, std::string &json_path, std::string &tileset_path);
        /** Lighting */
        void init_light();
        LIGHTING light_at(int x, int y);

        /** Variables */
        SDL_Surface *buffer, *tile_atlas, *display_screen;
        tile_map *tile_values;
        tile_id_map *tile_ids;

        std::map<int, std::vector<SDL_Surface*> > rotation_cache;

        int tile_height, tile_width;
        int screentile_width, screentile_height;
        tile *screen_tiles;
        int num_tiles;

        bool in_animation;

        bool do_draw_explosion;
        bool do_draw_bullet;
        bool do_draw_hit;
        bool do_draw_line;
        bool do_draw_weather;
        bool do_draw_footsteps;

        int exp_pos_x, exp_pos_y, exp_rad;

        int bul_pos_x, bul_pos_y;
        std::string bul_id;

        int hit_pos_x, hit_pos_y;
        std::string hit_entity_id;

        int line_pos_x, line_pos_y;
        bool is_target_line;
        std::vector<point> line_trajectory;
        std::string line_endpoint_id;

        weather_printable anim_weather;
        std::string weather_name;

        std::queue<point> footsteps;
        std::map<point, tile_drawing_cache> cache;
        std::map<point, tile_drawing_cache> tiles_to_draw_this_frame;

        // offset values
        int o_x, o_y;

    protected:
    private:
        void create_default_item_highlight();
        int
            sightrange_natural,
            sightrange_light,
            sightrange_lowlight,
            sightrange_max;
        int
            u_clairvoyance,
            g_lightlevel;
        bool
            boomered,
            sight_impaired,
            bionight_bionic_active;
        int last_pos_x, last_pos_y;

};

#endif // CATA_TILES_H
