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
#include "catajson.h"
#include "mapdata.h"
#include "tile_id_data.h"



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
        sx = sx = wx = wy = 0;
    }
    tile(int x, int y, int x2, int y2)
    {
        sx = x;
        sy = y;
        wx = x2;
        wy = y2;
    }
};
struct tile_rotation
{
    int *dx, *dy;

    tile_rotation(int num_pixels)
    {
        dx = new int[num_pixels];
        dy = new int[num_pixels];
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
    num_multitile_types
};

/** Typedefs */
typedef std::map<int, SDL_Rect*> tile_map;
typedef std::map<std::string, tile_type*> tile_id_map;

typedef tile_map::iterator tile_iterator;
typedef tile_id_map::iterator tile_id_iterator;

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
        /** Draw to screen */
        void draw(); /* Deprecated */
        void draw(int destx, int desty, int centerx, int centery, int width, int height);

        bool draw_from_id_string(std::string id, int x, int y, int subtile, int rota);
        bool draw_tile_at(tile_type *tile, int x, int y, int rota);

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
        bool draw_item(int x, int y);
        bool draw_vpart(int x, int y);
        bool draw_entity(int x, int y);

        bool draw_hit(int x, int y);

        /** Overmap Layer : Not used for now, do later*/
        bool draw_omap();

        /** Used to properly initialize everything for display */
        void init(SDL_Surface *screen, std::string json_path, std::string tileset_path);
        /** Lighting */
        void init_light();
        LIGHTING light_at(int x, int y);

        /** Variables */
        SDL_Surface *buffer, *tile_atlas, *display_screen;
        tile_map *tile_values;
        tile_id_map *tile_ids;

        int tile_height, tile_width;
        tile *screen_tiles;
        int num_tiles;

    protected:
    private:
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
        // offset values
        int o_x, o_y;

        tile_rotation *tile_rotations;
};

#endif // CATA_TILES_H
