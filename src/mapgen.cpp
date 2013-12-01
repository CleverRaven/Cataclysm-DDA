#include "map.h"
#include "omdata.h"
#include "mapitems.h"
#include "output.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "debug.h"
#include "options.h"
#include "item_factory.h"
#include "mapgen_functions.h"
#include "mapgenformat.h"
#include "overmapbuffer.h"
#include "enums.h"
#include "monstergenerator.h"

#include <algorithm>
#include <cassert>
#include <list>

#ifndef sgn
#define sgn(x) (((x) < 0) ? -1 : 1)
#endif

#define dbg(x) dout((DebugLevel)(x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

#define MON_RADIUS 3

enum room_type {
    room_null,
    room_closet,
    room_lobby,
    room_chemistry,
    room_teleport,
    room_goo,
    room_cloning,
    room_vivisect,
    room_bionics,
    room_dorm,
    room_living,
    room_bathroom,
    room_kitchen,
    room_bedroom,
    room_backyard,
    room_study,
    room_mine_shaft,
    room_mine_office,
    room_mine_storage,
    room_mine_fuel,
    room_mine_housing,
    room_bunker_bots,
    room_bunker_launcher,
    room_bunker_rifles,
    room_bunker_grenades,
    room_bunker_armor,
    room_mansion_courtyard,
    room_mansion_entry,
    room_mansion_bedroom,
    room_mansion_library,
    room_mansion_kitchen,
    room_mansion_dining,
    room_mansion_game,
    room_mansion_pool,
    room_mansion_study,
    room_mansion_bathroom,
    room_mansion_gallery,
    room_split
};

bool connects_to(oter_id there, int dir_from_here);
void house_room(map *m, room_type type, int x1, int y1, int x2, int y2);
void science_room(map *m, int x1, int y1, int x2, int y2, int rotate);
void set_science_room(map *m, int x1, int y1, bool faces_right, int turn);
void silo_rooms(map *m);
void build_mine_room(map *m, room_type type, int x1, int y1, int x2, int y2);
map_extra random_map_extra(map_extras);

room_type pick_mansion_room(int x1, int y1, int x2, int y2);
void build_mansion_room(map *m, room_type type, int x1, int y1, int x2, int y2);
void mansion_room(map *m, int x1, int y1, int x2, int y2); // pick & build

void line(map *m, ter_id type, int x1, int y1, int x2, int y2);
void line_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void fill_background(map *m, ter_id type);
void fill_background(map *m, ter_id (*f)());
void square(map *m, ter_id type, int x1, int y1, int x2, int y2);
void square(map *m, ter_id (*f)(), int x1, int y1, int x2, int y2);
void square_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void rough_circle(map *m, ter_id type, int x, int y, int rad);
void add_corpse(game *g, map *m, int x, int y);

void map::generate(game *g, overmap *om, const int x, const int y, const int z, const int turn)
{
    dbg(D_INFO) << "map::generate( g[" << g << "], om[" << (void *)om << "], x[" << x << "], "
                << "y[" << y << "], turn[" << turn << "] )";

    // First we have to create new submaps and initialize them to 0 all over
    // We create all the submaps, even if we're not a tinymap, so that map
    //  generation which overflows won't cause a crash.  At the bottom of this
    //  function, we save the upper-left 4 submaps, and delete the rest.
    for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
        grid[i] = new submap;
        grid[i]->active_item_count = 0;
        grid[i]->field_count = 0;
        grid[i]->turn_last_touched = turn;
        grid[i]->temperature = 0;
        grid[i]->comp = computer();
        grid[i]->camp = basecamp();
        for (int x = 0; x < SEEX; x++) {
            for (int y = 0; y < SEEY; y++) {
                grid[i]->ter[x][y] = t_null;
                grid[i]->frn[x][y] = f_null;
                grid[i]->trp[x][y] = tr_null;
                grid[i]->rad[x][y] = 0;
                grid[i]->graf[x][y] = graffiti();
            }
        }
    }

    oter_id terrain_type, t_north, t_neast, t_east, t_seast, t_south,
            t_nwest, t_west, t_swest, t_above;
    unsigned zones = 0;
    int overx = x / 2;
    int overy = y / 2;
    if ( x >= OMAPX * 2 || x < 0 || y >= OMAPY * 2 || y < 0) {
        dbg(D_INFO) << "map::generate: In section 1";

        // This happens when we're at the very edge of the overmap, and are generating
        // terrain for the adjacent overmap.
        int sx = 0, sy = 0;
        overx = (x % (OMAPX * 2)) / 2;
        if (x >= OMAPX * 2) {
            sx = 1;
        }
        if (x < 0) {
            sx = -1;
            overx = (OMAPX * 2 + x) / 2;
        }
        overy = (y % (OMAPY * 2)) / 2;
        if (y >= OMAPY * 2) {
            sy = 1;
        }
        if (y < 0) {
            overy = (OMAPY * 2 + y) / 2;
            sy = -1;
        }
        overmap tmp = overmap_buffer.get(g, om->pos().x + sx, om->pos().y + sy);
        terrain_type = tmp.ter(overx, overy, z);
        //zones = tmp.zones(overx, overy);
        t_above = tmp.ter(overx, overy, z + 1);

        if (overy - 1 >= 0) {
            t_north = tmp.ter(overx, overy - 1, z);
        } else {
            t_north = om->ter(overx, OMAPY - 1, z);
        }

        if (overy - 1 >= 0 && overx + 1 < OMAPX) {
            t_neast = tmp.ter(overx + 1, overy - 1, z);
        } else if (overy - 1 >= 0) {
            t_neast = om->ter(0, overy - 1, z);
        } else if (overx + 1 < OMAPX) {
            t_neast = om->ter(overx + 1, OMAPY - 1, z);
        } else {
            t_neast = om->ter(0, OMAPY - 1, z);
        }

        if (overx + 1 < OMAPX) {
            t_east = tmp.ter(overx + 1, overy, z);
        } else {
            t_east = om->ter(0, overy, z);
        }

        if (overy + 1 < OMAPY && overx + 1 < OMAPX) {
            t_seast = tmp.ter(overx + 1, overy + 1, z);
        } else if (overy + 1 < OMAPY) {
            t_seast = om->ter(0, overy + 1, z);
        } else if (overx + 1 < OMAPX) {
            t_seast = om->ter(overx + 1, 0, z);
        } else {
            t_seast = om->ter(0, 0, z);
        }

        if (overy + 1 < OMAPY) {
            t_south = tmp.ter(overx, overy + 1, z);
        } else {
            t_south = om->ter(overx, 0, z);
        }

        if (overy - 1 >= 0 && overx - 1 >= 0) {
            t_nwest = tmp.ter(overx - 1, overy - 1, z);
        } else if (overy - 1 >= 0) {
            t_nwest = om->ter(OMAPX - 1, overy - 1, z);
        } else if (overx - 1 >= 0) {
            t_nwest = om->ter(overx - 1, OMAPY - 1, z);
        } else {
            t_nwest = om->ter(OMAPX - 1, OMAPY - 1, z);
        }

        if (overx - 1 >= 0) {
            t_west = tmp.ter(overx - 1, overy, z);
        } else {
            t_west = om->ter(OMAPX - 1, overy, z);
        }

        if (overy + 1 < OMAPY && overx - 1 >= 0) {
            t_swest = tmp.ter(overx - 1, overy + 1, z);
        } else if (overy + 1 < OMAPY) {
            t_swest = om->ter(OMAPX - 1, overy + 1, z);
        } else if (overx - 1 >= 0) {
            t_swest = om->ter(overx - 1, 0, z);
        } else {
            t_swest = om->ter(OMAPX - 1, 0, z);
        }

    } else {
        dbg(D_INFO) << "map::generate: In section 2";

        t_above = om->ter(overx, overy, z + 1);
        terrain_type = om->ter(overx, overy, z);

        if (overy - 1 >= 0) {
            t_north = om->ter(overx, overy - 1, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x, om->pos().y - 1);
            t_north = tmp.ter(overx, OMAPY - 1, z);
        }

        if (overy - 1 >= 0 && overx + 1 < OMAPX) {
            t_neast = om->ter(overx + 1, overy - 1, z);
        } else if (overy - 1 >= 0) {
            overmap tmp = overmap_buffer.get(g, om->pos().x, om->pos().y - 1);
            t_neast = tmp.ter(0, overy - 1, z);
        } else if (overx + 1 < OMAPX) {
            overmap tmp = overmap_buffer.get(g, om->pos().x + 1, om->pos().y);
            t_neast = tmp.ter(overx + 1, OMAPY - 1, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x + 1, om->pos().y - 1);
            t_neast = tmp.ter(0, OMAPY - 1, z);
        }

        if (overx + 1 < OMAPX) {
            t_east = om->ter(overx + 1, overy, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x + 1, om->pos().y);
            t_east = tmp.ter(0, overy, z);
        }

        if (overy + 1 < OMAPY && overx + 1 < OMAPX) {
            t_seast = om->ter(overx + 1, overy + 1, z);
        } else if (overy + 1 < OMAPY) {
            overmap tmp = overmap_buffer.get(g, om->pos().x, om->pos().y + 1);
            t_seast = tmp.ter(0, overy + 1, z);
        } else if (overx + 1 < OMAPX) {
            overmap tmp = overmap_buffer.get(g, om->pos().x + 1, om->pos().y);
            t_seast = tmp.ter(overx + 1, 0, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x + 1, om->pos().y + 1);
            t_seast = tmp.ter(0, 0, z);
        }

        if (overy + 1 < OMAPY) {
            t_south = om->ter(overx, overy + 1, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x, om->pos().y + 1);
            t_south = tmp.ter(overx, 0, z);
        }

        if (overy - 1 >= 0 && overx - 1 >= 0) {
            t_nwest = om->ter(overx - 1, overy - 1, z);
        } else if (overy - 1 >= 0) {
            overmap tmp = overmap_buffer.get(g, om->pos().x, om->pos().y - 1);
            t_nwest = tmp.ter(OMAPX - 1, overy - 1, z);
        } else if (overx - 1 >= 0) {
            overmap tmp = overmap_buffer.get(g, om->pos().x - 1, om->pos().y);
            t_nwest = tmp.ter(overx - 1, OMAPY - 1, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x - 1, om->pos().y - 1);
            t_nwest = tmp.ter(OMAPX - 1, OMAPY - 1, z);
        }

        if (overx - 1 >= 0) {
            t_west = om->ter(overx - 1, overy, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x - 1, om->pos().y);
            t_west = tmp.ter(OMAPX - 1, overy, z);
        }

        if (overy + 1 < OMAPY && overx - 1 >= 0) {
            t_swest = om->ter(overx - 1, overy + 1, z);
        } else if (overy + 1 < OMAPY) {
            overmap tmp = overmap_buffer.get(g, om->pos().x, om->pos().y + 1);
            t_swest = tmp.ter(OMAPX - 1, overy + 1, z);
        } else if (overx - 1 >= 0) {
            overmap tmp = overmap_buffer.get(g, om->pos().x - 1, om->pos().y);
            t_swest = tmp.ter(overx - 1, 0, z);
        } else {
            overmap tmp = overmap_buffer.get(g, om->pos().x - 1, om->pos().y + 1);
            t_swest = tmp.ter(OMAPX - 1, 0, z);
        }
    }

    // This attempts to scale density of zombies inversely with distance from the nearest city.
    // In other words, make city centers dense and perimiters sparse.
    float density = 0.0;
    for (int i = overx - MON_RADIUS; i <= overx + MON_RADIUS; i++) {
        for (int j = overy - MON_RADIUS; j <= overy + MON_RADIUS; j++) {
            density += otermap[om->ter(i, j, z)].mondensity;
        }
    }
    density = density / 100;

    draw_map(terrain_type, t_north, t_east, t_south, t_west, t_neast, t_seast, t_nwest, t_swest,
             t_above, turn, g, density, z);

    map_extras ex = get_extras(otermap[terrain_type].extras);
    if ( one_in( ex.chance )) {
        add_extra( random_map_extra( ex ), g);
    }

    post_process(g, zones);

    // Okay, we know who are neighbors are.  Let's draw!
    // And finally save used submaps and delete the rest.
    for (int i = 0; i < my_MAPSIZE; i++) {
        for (int j = 0; j < my_MAPSIZE; j++) {
            dbg(D_INFO) << "map::generate: submap (" << i << "," << j << ")";
            dbg(D_INFO) << grid[i + j];

            if (i <= 1 && j <= 1) {
                saven(om, turn, x, y, z, i, j);
            } else {
                delete grid[i + j * my_MAPSIZE];
            }
        }
    }
}

// TODO: clean up variable shadowing in this function
// unfortunately, due to how absurdly long the function is (over 8000 lines!), it'll be hard to
// track down what is and isn't supposed to be carried around between bits of code.
// I suggest that we break the function down into smaller parts

void map::draw_map(const oter_id terrain_type, const oter_id t_north, const oter_id t_east,
                   const oter_id t_south, const oter_id t_west, const oter_id t_neast,
                   const oter_id t_seast, const oter_id t_nwest, const oter_id t_swest,
                   const oter_id t_above, const int turn, game *g, const float density,
                   const int zlevel)
{
    // Big old switch statement with a case for each overmap terrain type.
    // Many of these can be copied from another type, then rotated; for instance,
    //  "house_east" is identical to "house_north", just rotated 90 degrees to
    //  the right.  The rotate(int) function is at the bottom of this file.

    // Below comment is outdated
    // TODO: Update comment
    // The place_items() function takes a mapitems type (see mapitems.h and
    //  mapitemsdef.cpp), an "odds" int giving the chance for a single item to be
    //  placed, four ints (x1, y1, x2, y2) corresponding to the upper left corner
    //  and lower right corner of a square where the items are placed, a boolean
    //  that indicates whether items may spawn on grass & dirt, and finally an
    //  integer that indicates on which turn the items were created.  This final
    //  integer should be 0, unless the items are "fresh-grown" like wild fruit.

    //these variables are used in regular house generation. Placed here by Whales
    int rn = 0;
    int lw = 0;
    int rw = 0;
    int mw = 0;
    int tw = 0;
    int bw = 0;
    int cw = 0;
    int actual_house_height = 0;
    int bw_old = 0;

    int x = 0;
    int y = 0;

    // To distinguish between types of labs
    bool ice_lab = true;

    oter_id t_nesw[] = {t_north, t_east, t_south, t_west, t_neast, t_seast, t_nwest, t_swest};
    int nesw_fac[] = {0, 0, 0, 0, 0, 0, 0, 0};
    int &n_fac = nesw_fac[0], &e_fac = nesw_fac[1], &s_fac = nesw_fac[2], &w_fac = nesw_fac[3];

    mapgendata facing_data(t_north, t_east, t_south, t_west, t_neast, t_seast, t_nwest, t_swest);

    computer *tmpcomp = NULL;
    bool terrain_type_found = true;

    if (terrain_type == "") {
        mapgen_null(this);
    } else if (terrain_type == "crater") {
        mapgen_crater(this, facing_data);
    } else if (terrain_type == "field") {
        mapgen_field(this, turn);
    } else if (terrain_type == "dirtlot") {
        mapgen_dirtlot(this);

    } else if (is_ot_type("forest", terrain_type)) {
        mapgen_forest_general(this, terrain_type, facing_data, turn);

    } else if (terrain_type == "hive") {
        mapgen_hive(this, facing_data, turn);

    } else if (terrain_type == "spider_pit") {
        mapgen_spider_pit(this, facing_data, turn);

    } else if (terrain_type == "fungal_bloom") {
        mapgen_fungal_bloom(this);

    } else if (terrain_type == "road_ns" ||
               terrain_type == "road_ew") {
        mapgen_road_straight(this, terrain_type, facing_data, turn, density);

    } else if (terrain_type == "road_ne" ||
               terrain_type == "road_es" ||
               terrain_type == "road_sw" ||
               terrain_type == "road_wn") {
        mapgen_road_curved(this, terrain_type, facing_data, turn, density);

    } else if (terrain_type == "road_nes" ||
               terrain_type == "road_new" ||
               terrain_type == "road_nsw" ||
               terrain_type == "road_esw") {
        mapgen_road_tee(this, terrain_type, facing_data, turn, density);

    } else if (terrain_type == "road_nesw" ||
               terrain_type == "road_nesw_manhole") {
        mapgen_road_four_way(this, terrain_type, facing_data, turn, density);

    } else if (is_ot_type("bridge", terrain_type)) {
        mapgen_bridge(this, terrain_type, turn);

    } else if (is_ot_type("hiway", terrain_type)) {
        mapgen_highway(this, terrain_type, turn);

    } else if (terrain_type == "river_center") {
        fill_background(this, t_water_dp);

    } else if (terrain_type == "river_c_not_ne" ||
               terrain_type == "river_c_not_se" ||
               terrain_type == "river_c_not_sw" ||
               terrain_type == "river_c_not_nw") {
        mapgen_river_curved_not(this, terrain_type);

    } else if (terrain_type == "river_north" ||
               terrain_type == "river_east" ||
               terrain_type == "river_south" ||
               terrain_type == "river_west") {
        mapgen_river_straight(this, terrain_type);

    } else if (terrain_type == "river_ne" ||
               terrain_type == "river_se" ||
               terrain_type == "river_sw" ||
               terrain_type == "river_nw") {
        mapgen_river_curved(this, terrain_type);

    } else if (is_ot_type("house", terrain_type)) {

        lw = rng(0, 4);  // West external wall
        mw = lw + rng(7, 10);  // Middle wall between bedroom & kitchen/bath
        rw = SEEX * 2 - rng(1, 5); // East external wall
        tw = rng(1, 6);  // North external wall
        bw = SEEX * 2 - rng(2, 5); // South external wall
        cw = tw + rng(4, 7);  // Middle wall between living room & kitchen/bed
        actual_house_height = bw - rng(4,
                                       6); //reserving some space for backyard. Actual south external wall.
        bw_old = bw;

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i > lw && i < rw && j > tw && j < bw) {
                    ter_set(i, j, t_floor);
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
                if (i >= lw && i <= rw && (j == tw || j == bw)) { //placing north and south walls
                    ter_set(i, j, t_wall_h);
                }
                if ((i == lw || i == rw) && j > tw &&
                    j < bw /*actual_house_height*/) { //placing west (lw) and east walls
                    ter_set(i, j, t_wall_v);
                }
            }
        }
        switch(rng(1, 3)) {
        case 1: // Quadrants, essentially
            mw = rng(lw + 5, rw - 5);
            cw = tw + rng(4, 7);
            house_room(this, room_living, mw, tw, rw, cw);
            house_room(this, room_kitchen, lw, tw, mw, cw);
            ter_set(mw, rng(tw + 2, cw - 2), (one_in(3) ? t_door_c : t_floor));
            rn = rng(lw + 1, mw - 2);
            ter_set(rn    , tw, t_window_domestic);
            ter_set(rn + 1, tw, t_window_domestic);
            rn = rng(mw + 1, rw - 2);
            ter_set(rn    , tw, t_window_domestic);
            ter_set(rn + 1, tw, t_window_domestic);
            rn = rng(lw + 3, rw - 3); // Bottom part mw
            if (rn <= lw + 5) {
                // Bedroom on right, bathroom on left
                house_room(this, room_bedroom, rn, cw, rw, bw);

                // Put door between bedroom and living
                ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);

                if (bw - cw >= 10 && rn - lw >= 6) {
                    // All fits, placing bathroom and 2nd bedroom
                    house_room(this, room_bathroom, lw, bw - 5, rn, bw);
                    house_room(this, room_bedroom, lw, cw, rn, bw - 5);

                    // Put door between bathroom and bedroom
                    ter_set(rn, rng(bw - 4, bw - 1), t_door_c);

                    if (one_in(3)) {
                        // Put door between 2nd bedroom and 1st bedroom
                        ter_set(rn, rng(cw + 1, bw - 6), t_door_c);
                    } else {
                        // ...Otherwise, between 2nd bedroom and kitchen
                        ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);
                    }
                } else if (bw - cw > 4) {
                    // Too big for a bathroom, not big enough for 2nd bedroom
                    // Make it a bathroom anyway, but give the excess space back to
                    // the kitchen.
                    house_room(this, room_bathroom, lw, bw - 4, rn, bw);
                    for (int i = lw + 1; i < mw && i < rn; i++) {
                        ter_set(i, cw, t_floor);
                    }

                    // Put door between excess space and bathroom
                    ter_set(rng(lw + 1, rn - 1), bw - 4, t_door_c);

                    // Put door between excess space and bedroom
                    ter_set(rn, rng(cw + 1, bw - 5), t_door_c);
                } else {
                    // Small enough to be a bathroom; make it one.
                    house_room(this, room_bathroom, lw, cw, rn, bw);

                    if (one_in(5)) {
                        // Put door between batroom and kitchen with low chance
                        ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);
                    } else {
                        // ...Otherwise, between bathroom and bedroom
                        ter_set(rn, rng(cw + 1, bw - 1), t_door_c);
                    }
                }
                // Point on bedroom wall, for window
                rn = rng(rn + 2, rw - 2);
            } else {
                // Bedroom on left, bathroom on right
                house_room(this, room_bedroom, lw, cw, rn, bw);

                // Put door between bedroom and kitchen
                ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);

                if (bw - cw >= 10 && rw - rn >= 6) {
                    // All fits, placing bathroom and 2nd bedroom
                    house_room(this, room_bathroom, rn, bw - 5, rw, bw);
                    house_room(this, room_bedroom, rn, cw, rw, bw - 5);

                    // Put door between bathroom and bedroom
                    ter_set(rn, rng(bw - 4, bw - 1), t_door_c);

                    if (one_in(3)) {
                        // Put door between 2nd bedroom and 1st bedroom
                        ter_set(rn, rng(cw + 1, bw - 6), t_door_c);
                    } else {
                        // ...Otherwise, between 2nd bedroom and living
                        ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);
                    }
                } else if (bw - cw > 4) {
                    // Too big for a bathroom, not big enough for 2nd bedroom
                    // Make it a bathroom anyway, but give the excess space back to
                    // the living.
                    house_room(this, room_bathroom, rn, bw - 4, rw, bw);
                    for (int i = rw - 1; i > rn && i > mw; i--) {
                        ter_set(i, cw, t_floor);
                    }

                    // Put door between excess space and bathroom
                    ter_set(rng(rw - 1, rn + 1), bw - 4, t_door_c);

                    // Put door between excess space and bedroom
                    ter_set(rn, rng(cw + 1, bw - 5), t_door_c);
                } else {
                    // Small enough to be a bathroom; make it one.
                    house_room(this, room_bathroom, rn, cw, rw, bw);

                    if (one_in(5)) {
                        // Put door between bathroom and living with low chance
                        ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);
                    } else {
                        // ...Otherwise, between bathroom and bedroom
                        ter_set(rn, rng(cw + 1, bw - 1), t_door_c);
                    }
                }
                // Point on bedroom wall, for window
                rn = rng(lw + 2, rn - 2);
            }
            ter_set(rn    , bw, t_window_domestic);
            ter_set(rn + 1, bw, t_window_domestic);
            if (!one_in(3) && rw < SEEX * 2 - 1) { // Potential side windows
                rn = rng(tw + 2, bw - 6);
                ter_set(rw, rn    , t_window_domestic);
                ter_set(rw, rn + 4, t_window_domestic);
            }
            if (!one_in(3) && lw > 0) { // Potential side windows
                rn = rng(tw + 2, bw - 6);
                ter_set(lw, rn    , t_window_domestic);
                ter_set(lw, rn + 4, t_window_domestic);
            }
            if (one_in(2)) { // Placement of the main door
                ter_set(rng(lw + 2, mw - 1), tw, (one_in(6) ? t_door_c : t_door_locked));
                if (one_in(5)) { // Placement of side door
                    ter_set(rw, rng(tw + 2, cw - 2), (one_in(6) ? t_door_c : t_door_locked));
                }
            } else {
                ter_set(rng(mw + 1, rw - 2), tw, (one_in(6) ? t_door_c : t_door_locked));
                if (one_in(5)) {
                    ter_set(lw, rng(tw + 2, cw - 2), (one_in(6) ? t_door_c : t_door_locked));
                }
            }
            break;

        case 2: // Old-style; simple;
            //Modified by Jovan in 28 Aug 2013
            //Long narrow living room in front, big kitchen and HUGE bedroom
            bw = SEEX * 2 - 2;
            cw = tw + rng(3, 6);
            mw = rng(lw + 7, rw - 4);
            //int actual_house_height=bw-rng(4,6);
            //in some rare cases some rooms (especially kitchen and living room) may get rather small
            if ((tw <= 3) && ( abs((actual_house_height - 3) - cw) >= 3 ) ) {
                //everything is fine
                house_room(this, room_backyard, lw, actual_house_height + 1, rw, bw);
                //door from bedroom to backyard
                ter_set((lw + mw) / 2, actual_house_height, t_door_c);
            } else { //using old layout
                actual_house_height = bw_old;
            }
            // Plop down the rooms
            house_room(this, room_living, lw, tw, rw, cw);
            house_room(this, room_kitchen, mw, cw, rw, actual_house_height - 3);
            house_room(this, room_bedroom, lw, cw, mw, actual_house_height ); //making bedroom smaller
            house_room(this, room_bathroom, mw, actual_house_height - 3, rw, actual_house_height);

            // Space between kitchen & living room:
            rn = rng(mw + 1, rw - 3);
            ter_set(rn    , cw, t_floor);
            ter_set(rn + 1, cw, t_floor);
            // Front windows
            rn = rng(2, 5);
            ter_set(lw + rn    , tw, t_window_domestic);
            ter_set(lw + rn + 1, tw, t_window_domestic);
            ter_set(rw - rn    , tw, t_window_domestic);
            ter_set(rw - rn + 1, tw, t_window_domestic);
            // Front door
            ter_set(rng(lw + 4, rw - 4), tw, (one_in(6) ? t_door_c : t_door_locked));
            if (one_in(3)) { // Kitchen windows
                rn = rng(cw + 1, actual_house_height - 5);
                ter_set(rw, rn    , t_window_domestic);
                ter_set(rw, rn + 1, t_window_domestic);
            }
            if (one_in(3)) { // Bedroom windows
                rn = rng(cw + 1, actual_house_height - 2);
                ter_set(lw, rn    , t_window_domestic);
                ter_set(lw, rn + 1, t_window_domestic);
            }
            // Door to bedroom
            if (one_in(4)) {
                ter_set(rng(lw + 1, mw - 1), cw, t_door_c);
            } else {
                ter_set(mw, rng(cw + 3, actual_house_height - 4), t_door_c);
            }
            // Door to bathrom
            if (one_in(4)) {
                ter_set(mw, actual_house_height - 1, t_door_c);
            } else {
                ter_set(rng(mw + 2, rw - 2), actual_house_height - 3, t_door_c);
            }
            // Back windows
            rn = rng(lw + 1, mw - 2);
            ter_set(rn    , actual_house_height, t_window_domestic);
            ter_set(rn + 1, actual_house_height, t_window_domestic);
            rn = rng(mw + 1, rw - 1);
            ter_set(rn, actual_house_height, t_window_domestic);
            break;

        case 3: // Long center hallway, kitchen, living room and office
            mw = int((lw + rw) / 2);
            cw = bw - rng(5, 7);
            // Hallway doors and windows
            ter_set(mw    , tw, (one_in(6) ? t_door_c : t_door_locked));
            if (one_in(4)) {
                ter_set(mw - 1, tw, t_window_domestic);
                ter_set(mw + 1, tw, t_window_domestic);
            }
            for (int i = tw + 1; i < cw; i++) { // Hallway walls
                ter_set(mw - 2, i, t_wall_v);
                ter_set(mw + 2, i, t_wall_v);
            }
            if (one_in(2)) { // Front rooms are kitchen or living room
                house_room(this, room_living, lw, tw, mw - 2, cw);
                house_room(this, room_kitchen, mw + 2, tw, rw, cw);
            } else {
                house_room(this, room_kitchen, lw, tw, mw - 2, cw);
                house_room(this, room_living, mw + 2, tw, rw, cw);
            }
            // Front windows
            rn = rng(lw + 1, mw - 4);
            ter_set(rn    , tw, t_window_domestic);
            ter_set(rn + 1, tw, t_window_domestic);
            rn = rng(mw + 3, rw - 2);
            ter_set(rn    , tw, t_window_domestic);
            ter_set(rn + 1, tw, t_window_domestic);
            if (one_in(3) && lw > 0) { // Side windows?
                rn = rng(tw + 1, cw - 2);
                ter_set(lw, rn    , t_window_domestic);
                ter_set(lw, rn + 1, t_window_domestic);
            }
            if (one_in(3) && rw < SEEX * 2 - 1) { // Side windows?
                rn = rng(tw + 1, cw - 2);
                ter_set(rw, rn    , t_window_domestic);
                ter_set(rw, rn + 1, t_window_domestic);
            }
            if (one_in(2)) { // Bottom rooms are bedroom or bathroom
                //bathroom to the left (eastern wall), study to the right
                //house_room(this, room_bedroom, lw, cw, rw - 3, bw);
                house_room(this, room_bedroom, mw - 2, cw, rw - 3, bw);
                house_room(this, room_bathroom, rw - 3, cw, rw, bw);
                house_room(this, room_study, lw, cw, mw - 2, bw);
                //===Study Room Furniture==
                ter_set(mw - 2, (bw + cw) / 2, t_door_o);
                furn_set(lw + 1, cw + 1, f_chair);
                furn_set(lw + 1, cw + 2, f_table);
                ter_set(lw + 1, cw + 3, t_console_broken);
                furn_set(lw + 3, bw - 1, f_bookcase);
                place_items("magazines", 30,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
                place_items("novels", 40,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
                place_items("alcohol", 20,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
                place_items("manuals", 30,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
                //=========================
                ter_set(rng(lw + 2, mw - 3), cw, t_door_c);
                if (one_in(4)) {
                    ter_set(rng(rw - 2, rw - 1), cw, t_door_c);
                } else {
                    ter_set(rw - 3, rng(cw + 2, bw - 2), t_door_c);
                }
                rn = rng(mw, rw - 5); //bedroom windows
                ter_set(rn    , bw, t_window_domestic);
                ter_set(rn + 1, bw, t_window_domestic);
                ter_set(rng(lw + 2, mw - 3), bw, t_window_domestic); //study window

                if (one_in(4)) {
                    ter_set(rng(rw - 2, rw - 1), bw, t_window_domestic);
                } else {
                    ter(rw, rng(cw + 1, bw - 1));
                }
            } else { //bathroom to the right
                house_room(this, room_bathroom, lw, cw, lw + 3, bw);
                //house_room(this, room_bedroom, lw + 3, cw, rw, bw);
                house_room(this, room_bedroom, lw + 3, cw, mw + 2, bw);
                house_room(this, room_study, mw + 2, cw, rw, bw);
                //===Study Room Furniture==
                ter_set(mw + 2, (bw + cw) / 2, t_door_c);
                furn_set(rw - 1, cw + 1, f_chair);
                furn_set(rw - 1, cw + 2, f_table);
                ter_set(rw - 1, cw + 3, t_console_broken);
                furn_set(rw - 3, bw - 1, f_bookcase);
                place_items("magazines", 40,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
                place_items("novels", 40,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
                place_items("alcohol", 20,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
                place_items("manuals", 20,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
                //=========================

                if (one_in(4)) {
                    ter_set(rng(lw + 1, lw + 2), cw, t_door_c);
                } else {
                    ter_set(lw + 3, rng(cw + 2, bw - 2), t_door_c);
                }
                rn = rng(lw + 4, mw); //bedroom windows
                ter_set(rn    , bw, t_window_domestic);
                ter_set(rn + 1, bw, t_window_domestic);
                ter_set(rng(mw + 3, rw - 1), bw, t_window_domestic); //study window
                if (one_in(4)) {
                    ter_set(rng(lw + 1, lw + 2), bw, t_window_domestic);
                } else {
                    ter(lw, rng(cw + 1, bw - 1));
                }
            }
            // Doors off the sides of the hallway
            ter_set(mw - 2, rng(tw + 3, cw - 3), t_door_c);
            ter_set(mw + 2, rng(tw + 3, cw - 3), t_door_c);
            ter_set(mw, cw, t_door_c);
            break;
        } // Done with the various house structures

        if (rng(2, 7) < tw) { // Big front yard has a chance for a fence
            for (int i = lw; i <= rw; i++) {
                ter_set(i, 0, t_fence_h);
            }
            for (int i = 1; i < tw; i++) {
                ter_set(lw, i, t_fence_v);
                ter_set(rw, i, t_fence_v);
            }
            int hole = rng(SEEX - 3, SEEX + 2);
            ter_set(hole, 0, t_dirt);
            ter_set(hole + 1, 0, t_dirt);
            if (one_in(tw)) {
                ter_set(hole - 1, 1, t_tree_young);
                ter_set(hole + 2, 1, t_tree_young);
            }
        }

        if (is_ot_type("house_base", terrain_type)) {
            int attempts = 20;
            do {
                rn = rng(lw + 1, rw - 1);
                attempts--;
            } while (ter(rn, actual_house_height - 1) != t_floor && attempts);
            if( ter(rn, actual_house_height - 1) == t_floor && attempts ) {
                ter_set(rn, actual_house_height - 1, t_stairs_down);
            }
        }
        if (one_in(100)) { // Houses have a 1 in 100 chance of wasps!
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (ter(i, j) == t_door_c || ter(i, j) == t_door_locked) {
                        ter_set(i, j, t_door_frame);
                    }
                    if (ter(i, j) == t_window_domestic && !one_in(3)) {
                        ter_set(i, j, t_window_frame);
                    }
                    if ((ter(i, j) == t_wall_h || ter(i, j) == t_wall_v) && one_in(8)) {
                        ter_set(i, j, t_paper);
                    }
                }
            }
            int num_pods = rng(8, 12);
            for (int i = 0; i < num_pods; i++) {
                int podx = rng(1, SEEX * 2 - 2), pody = rng(1, SEEY * 2 - 2);
                int nonx = 0, nony = 0;
                while (nonx == 0 && nony == 0) {
                    nonx = rng(-1, 1);
                    nony = rng(-1, 1);
                }
                for (int x = -1; x <= 1; x++) {
                    for (int y = -1; y <= 1; y++) {
                        if ((x != nonx || y != nony) && (x != 0 || y != 0)) {
                            ter_set(podx + x, pody + y, t_paper);
                        }
                    }
                }
                add_spawn("mon_wasp", 1, podx, pody);
            }
            place_items("rare", 70, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);

        } else if (one_in(150)) { // No wasps; black widows?
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (ter(i, j) == t_floor) {
                        if (one_in(15)) {
                            add_spawn("mon_spider_widow_giant", rng(1, 2), i, j);
                            for (int x = i - 1; x <= i + 1; x++) {
                                for (int y = j - 1; y <= j + 1; y++) {
                                    if (ter(x, y) == t_floor) {
                                        add_field(NULL, x, y, fd_web, rng(2, 3));
                                    }
                                }
                            }
                        } else if (move_cost(i, j) > 0 && one_in(5)) {
                            add_field(NULL, x, y, fd_web, 1);
                        }
                    }
                }
            }
            place_items("rare", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);
        } else { // Just boring old zombies
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
        }

        if (terrain_type == "house_east" ||
            terrain_type == "house_base_east") {
            rotate(1);
        } else if (terrain_type == "house_south" ||
                   terrain_type == "house_base_south") {
            rotate(2);
        } else if (terrain_type == "house_west" ||
                   terrain_type == "house_base_west") {
            rotate(3);
        }

    } else if (terrain_type == "s_lot") {
        mapgen_parking_lot(this, facing_data, turn);

    } else if (terrain_type == "pool") {
        mapgen_pool(this);

    } else if (terrain_type == "park") {
        mapgen_park(this);

    } else if (is_ot_type("s_gas", terrain_type)) {
        mapgen_gas_station(this, terrain_type, density);

    } else if (is_ot_type("s_pharm", terrain_type)) {

        tw = rng(0, 4);
        bw = SEEY * 2 - rng(1, 5);
        mw = bw - rng(3, 4); // Top of the storage room
        lw = rng(0, 4);
        rw = SEEX * 2 - rng(1, 5);
        cw = rng(13, rw - 5); // Left side of the storage room
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j == tw && ((i > lw + 2 && i < lw + 6) || (i > rw - 6 && i < rw - 2))) {
                    ter_set(i, j, t_window);
                } else if ((j == tw && (i == lw + 8 || i == lw + 9)) ||
                           (i == cw && j == mw + 1)) {
                    ter_set(i, j, t_door_c);
                } else if (((j == tw || j == bw) && i >= lw && i <= rw) ||
                           (j == mw && i >= cw && i < rw)) {
                    ter_set(i, j, t_wall_h);
                } else if (((i == lw || i == rw) && j > tw && j < bw) ||
                           (i == cw && j > mw && j < bw)) {
                    ter_set(i, j, t_wall_v);
                } else if (((i == lw + 8 || i == lw + 9 || i == rw - 4 || i == rw - 3) &&
                            j > tw + 3 && j < mw - 2) ||
                           (j == bw - 1 && i > lw + 1 && i < cw - 1)) {
                    set(i, j, t_floor, f_rack);
                } else if ((i == lw + 1 && j > tw + 8 && j < mw - 1) ||
                           (j == mw - 1 && i > cw + 1 && i < rw)) {
                    set(i, j, t_floor, f_glass_fridge);
                } else if ((j == mw     && i > lw + 1 && i < cw) ||
                           (j == tw + 6 && i > lw + 1 && i < lw + 6) ||
                           (i == lw + 5 && j > tw     && j < tw + 7)) {
                    set(i, j, t_floor, f_counter);
                } else if (i > lw && i < rw && j > tw && j < bw) {
                    ter_set(i, j, t_floor);
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
            }
        }

        {
            int num_carts = rng(0, 5);
            for( int i = 0; i < num_carts; i++ ) {
                add_vehicle (g, "shopping_cart", rng(lw, cw), rng(tw, mw), 90);
            }
        }

        if (one_in(3)) {
            place_items("snacks", 74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
        } else if (one_in(4)) {
            place_items("cleaning", 74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
        } else {
            place_items("magazines", 74, lw + 8, tw + 4, lw + 8, mw - 3, false, 0);
        }
        if (one_in(5)) {
            place_items("softdrugs", 84, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
        } else if (one_in(4)) {
            place_items("cleaning", 74, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
        } else {
            place_items("snacks", 74, lw + 9, tw + 4, lw + 9, mw - 3, false, 0);
        }
        if (one_in(5)) {
            place_items("softdrugs", 84, rw - 4, tw + 4, rw - 4, mw - 3, false, 0);
        } else {
            place_items("snacks", 74, rw - 4, tw + 4, rw - 4, mw - 3, false, 0);
        }
        if (one_in(3)) {
            place_items("snacks", 70, rw - 3, tw + 4, rw - 3, mw - 3, false, 0);
        } else {
            place_items("softdrugs", 80, rw - 3, tw + 4, rw - 3, mw - 3, false, 0);
        }
        place_items("fridgesnacks", 74, lw + 1, tw + 9, lw + 1, mw - 2, false, 0);
        place_items("fridgesnacks", 74, cw + 2, mw - 1, rw - 1, mw - 1, false, 0);
        place_items("harddrugs", 88, lw + 2, bw - 1, cw - 2, bw - 1, false, 0);
        place_items("behindcounter", 78, lw + 1, tw + 1, lw + 4, tw + 5, false, 0);
        if (terrain_type == "s_pharm_east") {
            rotate(1);
        }
        if (terrain_type == "s_pharm_south") {
            rotate(2);
        }
        if (terrain_type == "s_pharm_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_PHARM", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("office_cubical", terrain_type)) {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
      sss               \n\
     ^sss^              \n\
 |---|-D|------|--|---| \n\
 |cdx|..|xdccxl|.^|cxc| \n\
 |.h....|.h..h......hd| \n\
 |---|..|------|..|---| \n\
 |xdl|....h..hd|..|xdc| \n\
 |ch....|dxcccx|....h.| \n\
 |---|..|------|..|---| \n\
 |xcc|...^cclc....|cdx| \n\
 |dh................hc| \n\
 |-------......|------| \n\
 |e.....+......|n.xdc.| \n\
 |S.....|----..|h.ch..| \n\
 |-+|-+-|......+.....^| \n\
 |..|..S|..hc..|------| \n\
 |l.|..T|cccc..|......| \n\
 |--|---|......|.htth.| \n\
 |^hhh..+......+.htth.| \n\
 |o.....|......|.htth.| \n\
 |o.dx..|--++--|.htth.| \n\
 |o.h...|$$ss$$|......| \n\
 |-wwww-|  ss  |-wwww-| \n\
           ss           \n",
                                   mapf::basic_bind("x $ ^ . - | # t + = D w T S e o h c d l s n", t_console_broken, t_shrub, t_floor,
                                           t_floor, t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked,
                                           t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,
                                           t_sidewalk, t_null),
                                   mapf::basic_bind("x $ ^ . - | # t + = D w T S e o h c d l s n", f_null,           f_null,
                                           f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table, f_null,   f_null,
                                           f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                           f_locker, f_null, f_safe_l));
        place_items("fridge", 50,  2,  12, 2,  13, false, 0);
        place_items("cleaning", 50,  2,  15, 3,  16, false, 0);
        place_items("office", 80, 11,  7, 13,  7, false, 0);
        place_items("office", 80,  10,  3, 12,  3, false, 0);
        place_items("cubical_office", 60,  2,  3, 3,  3, false, 0);
        place_items("cubical_office", 60,  3,  6, 4,  6, false, 0);
        place_items("cubical_office", 60,  3,  9, 4,  9, false, 0);
        place_items("cubical_office", 60,  21,  3, 21,  4, false, 0);
        place_items("cubical_office", 60,  20,  6, 21,  6, false, 0);
        place_items("cubical_office", 60,  19,  9, 20,  9, false, 0);
        place_items("cubical_office", 60,  18,  17, 19,  20, false, 0);
        place_items("novels", 70,  2,  19, 2,  21, false, 0);
        {
            int num_chairs = rng(0, 6);
            for( int i = 0; i < num_chairs; i++ ) {
                add_vehicle (g, "swivel_chair", rng(6, 16), rng(6, 16), 0, -1, -1, false);
            }
        }
        if (terrain_type == "office_cubical_east") {
            rotate(3);
        }
        if (terrain_type == "office_cubical_north") {
            rotate(2);
        }
        if (terrain_type == "office_cubical_west") {
            rotate(1);
        }


    } else if (terrain_type == "apartments_con_tower_1_entrance") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
   |------|-|-|-|---|...\n\
   |.dBBd.+r|u+..eSc|...\n\
   w..BB..|-|-|....c|...\n\
   w......|STb|....O|...\n\
   |oo....+..b|..ccc|^..\n\
   |--|-+-|-+-|..hhh|...\n\
   RssX.............|...\n\
   Rssw.............D...\n\
   Rssw..A....A..|+-|...\n\
   Rss|...FFF...^|.r|...\n\
   |------|-|-|--|--|...\n\
   |.dBBd.+r|u+..eSc|...\n\
   w..BB..|-|-|....c|...\n\
   w......|STb|....O|...\n\
   |.d....+..b|..ccc|...\n\
   |--|-+-|-+-|....^|...\n\
   RssX...A.........|...\n\
   Rssw.............D...\n\
   Rssw.........o|+-|...\n\
   Rss|..A.FFF..o|.r|...\n\
   |--|--ww---ww-|--|w-G\n\
                      ss\n\
                      ss\n\
                      ss\n",
                                   mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                           t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_glass_c,
                                           t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                           t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window, t_floor,   t_floor,
                                           t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk, t_floor),
                                   mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", f_cupboard,
                                           f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null, f_null,
                                           f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_bed,
                                           f_null,   f_null,                 f_null,         f_null,        f_null,   f_bathtub, f_toilet,
                                           f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,     f_bookcase));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_rack) {
                    place_items("dresser", 30,  i,  j, i,  j, false, 0);
                    place_items("jackets", 60,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_fridge) {
                    place_items("fridge", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_oven) {
                    place_items("oven", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_cupboard) {
                    place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                    place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                    place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                    place_items("pasta", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_bookcase) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                    place_items("novels", 40,  i,  j, i,  j, false, 0);
                    place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                    place_items("manuals", 20,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        } else {
            add_spawn("mon_zombie", rng(1, 8), 15, 10);
        }
        if (t_north == "apartments_con_tower_1" && t_west == "apartments_con_tower_1") {
            rotate(3);
        } else if (t_north == "apartments_con_tower_1" && t_east == "apartments_con_tower_1") {
            rotate(0);
        } else if (t_south == "apartments_con_tower_1" && t_east == "apartments_con_tower_1") {
            rotate(1);
        } else if (t_west == "apartments_con_tower_1" && t_south == "apartments_con_tower_1") {
            rotate(2);
        }


    } else if (terrain_type == "apartments_con_tower_1") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if ((t_south == "apartments_con_tower_1_entrance" && t_east == "apartments_con_tower_1") ||
            (t_north == "apartments_con_tower_1" && t_east == "apartments_con_tower_1_entrance")
            || (t_west == "apartments_con_tower_1" && t_north == "apartments_con_tower_1_entrance") ||
            (t_south == "apartments_con_tower_1" && t_west == "apartments_con_tower_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
   |------|-|-|ww---|   \n\
   |.dBBd.+r|u+..eSc|-w-\n\
   w..BB..|-|-|....c|...\n\
   w.h....|STb|....O|.hc\n\
   |cxc...+..b|..ccc|cxc\n\
   |--|-+-|-+-|.....|-ww\n\
   RssX.............|^..\n\
   Rssw..F..........D...\n\
   Rssw..F.......|+-|...\n\
   Rss|..F...FFF^|.r|...\n\
   |------|-|-|--|--|...\n\
   |.dBBd.+r|u+..eSc|...\n\
   w..BB..|-|-|....c|...\n\
   w......|STb|....O|...\n\
   |od....+..b|..ccc|...\n\
   |--|-+-|-+-|...hh|...\n\
   RssX.............|...\n\
   Rssw.............D...\n\
   Rssw..A.....F.|+-|...\n\
   Rss|.....FFFF^|.r|...\n\
   |------------||--|...\n\
   |############|EEE=...\n\
   |############|EEx=...\n",
                                       mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                               t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_glass_c,
                                               t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                               t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window, t_floor,   t_floor,
                                               t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk, t_floor),
                                       mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", f_cupboard,
                                               f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null, f_null,
                                               f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_bed,
                                               f_null,   f_null,                 f_null,         f_null,        f_null,   f_bathtub, f_toilet,
                                               f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,     f_bookcase));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_rack) {
                        place_items("dresser", 30,  i,  j, i,  j, false, 0);
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_fridge) {
                        place_items("fridge", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_oven) {
                        place_items("oven", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_cupboard) {
                        place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                        place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                        place_items("pasta", 50,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_bookcase) {
                        place_items("magazines", 30,  i,  j, i,  j, false, 0);
                        place_items("novels", 40,  i,  j, i,  j, false, 0);
                        place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                        place_items("manuals", 20,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                add_spawn("mon_zombie", rng(1, 8), 15, 10);
            }
            if (t_west == "apartments_con_tower_1_entrance") {
                rotate(1);
            }
            if (t_north == "apartments_con_tower_1_entrance") {
                rotate(2);
            }
            if (t_east == "apartments_con_tower_1_entrance") {
                rotate(3);
            }
        }

        else if ((t_west == "apartments_con_tower_1_entrance" && t_north == "apartments_con_tower_1") ||
                 (t_north == "apartments_con_tower_1_entrance" && t_east == "apartments_con_tower_1")
                 || (t_west == "apartments_con_tower_1" && t_south == "apartments_con_tower_1_entrance") ||
                 (t_south == "apartments_con_tower_1" && t_east == "apartments_con_tower_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
...|---|-|-|-|------|   \n\
...|cSe..+u|r+.dBBd.|   \n\
...|c....|-|-|..BB..w   \n\
...|O....|bTS|......w   \n\
...|ccc..|b..+....d.|   \n\
...|.....|-+-|-+-|--|   \n\
...|.............+ssR   \n\
...D.............wssR   \n\
...|-+|...htth...wssR   \n\
...|r.|^..htth.oo|ssR   \n\
...|--|--|-|-|------|   \n\
...|cSe..+u|r+.dBBd.|   \n\
..^|c....|-|-|..BB..w   \n\
...|O....|bTS|......w   \n\
...|ccc..|b..+....d.|   \n\
...|.....|-+-|-+-|--|   \n\
...|.............+ssR   \n\
...D...........A.wssR   \n\
...|-+|..A...h...wssR   \n\
...|r.|o....cxc.^|ssR   \n\
G-w|--|-ww---ww--|--|   \n\
ss                      \n\
ss                      \n\
ss                      \n",
                                       mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                               t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_glass_c,
                                               t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                               t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window, t_floor,   t_floor,
                                               t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk, t_floor),
                                       mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", f_cupboard,
                                               f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null, f_null,
                                               f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_bed,
                                               f_null,   f_null,                 f_null,         f_null,        f_null,   f_bathtub, f_toilet,
                                               f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,     f_bookcase));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_rack) {
                        place_items("dresser", 30,  i,  j, i,  j, false, 0);
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_fridge) {
                        place_items("fridge", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_oven) {
                        place_items("oven", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_cupboard) {
                        place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                        place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                        place_items("pasta", 50,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_bookcase) {
                        place_items("magazines", 30,  i,  j, i,  j, false, 0);
                        place_items("novels", 40,  i,  j, i,  j, false, 0);
                        place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                        place_items("manuals", 20,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                add_spawn("mon_zombie", rng(1, 8), 15, 10);
            }
            if (t_north == "apartments_con_tower_1_entrance") {
                rotate(1);
            }
            if (t_east == "apartments_con_tower_1_entrance") {
                rotate(2);
            }
            if (t_south == "apartments_con_tower_1_entrance") {
                rotate(3);
            }
        } else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
   |---ww|-|-|------|   \n\
-w-|cSe..+u|r+.dBBd.|   \n\
..l|c....|-|-|..BB..w   \n\
..l|O....|bTS|......w   \n\
...|ccc..|b..+....d.|   \n\
-D-|.hh..|-+-|-+-|--|   \n\
...|.............XssR   \n\
...D.............wssR   \n\
...|-+|..........wssR   \n\
...|r.|oo.FFFF..^|ssR   \n\
...|--|--|-|-|------|   \n\
...|cSe..+u|r+.dBBd.|   \n\
...|c....|-|-|..BB..w   \n\
...|O....|bTS|......w   \n\
...|ccc..|b..+....d.|   \n\
..^|h.h..|-+-|-+-|--|   \n\
...|.............XssR   \n\
...D......A......wssR   \n\
...|-+|..........wssR   \n\
...|r.|^....FFF.o|ssR   \n\
...|--||------------|   \n\
...=xEE|############|   \n\
...=EEE|############|   \n",
                                       mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                               t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_glass_c,
                                               t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                               t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window, t_floor,   t_floor,
                                               t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk, t_floor),
                                       mapf::basic_bind("u A F E > < R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", f_cupboard,
                                               f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null, f_null,
                                               f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_bed,
                                               f_null,   f_null,                 f_null,         f_null,        f_null,   f_bathtub, f_toilet,
                                               f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,     f_bookcase));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_locker) {
                        place_items("office", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_rack) {
                        place_items("dresser", 30,  i,  j, i,  j, false, 0);
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_fridge) {
                        place_items("fridge", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_oven) {
                        place_items("oven", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_cupboard) {
                        place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                        place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                        place_items("pasta", 50,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_bookcase) {
                        place_items("magazines", 30,  i,  j, i,  j, false, 0);
                        place_items("novels", 40,  i,  j, i,  j, false, 0);
                        place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                        place_items("manuals", 20,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                add_spawn("mon_zombie", rng(1, 8), 15, 10);
            }
            if (t_west == "apartments_con_tower_1" && t_north == "apartments_con_tower_1") {
                rotate(1);
            } else if (t_east == "apartments_con_tower_1" && t_north == "apartments_con_tower_1") {
                rotate(2);
            } else if (t_east == "apartments_con_tower_1" && t_south == "apartments_con_tower_1") {
                rotate(3);
            }
        }


    } else if (terrain_type == "apartments_mod_tower_1_entrance") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
  w.htth..FFFF..eSc|....\n\
  w...............O|....\n\
  |-X|..........ccc|....\n\
  Rss|-+----|o...h.|....\n\
  Rss|...BBd|o....A|....\n\
  Rssw...BB.|^.....|....\n\
  Rssw...h..|--|...D....\n\
  Rss|..cxc.+.r|-+-|....\n\
 ||--|+|----|--|r..|....\n\
 |b....|bTS.+..|---|....\n\
 |b.T.S|b...|..+..u|....\n\
 |-----|-+|-|..|---|....\n\
 |.dBBd...+r|...eSc|....\n\
 w..BB....|-|.....O|....\n\
 |.....h..+.....ccc|....\n\
 |--|.cxc.|........D....\n\
    |-www-|o.tt....|....\n\
    Rsssss|.......F|....\n\
    RsssssX..A..FFF|-w-G\n\
    |aaaaa|----ww--|  ss\n\
                      ss\n\
                      ss\n\
                      ss\n\
                      ss\n",
                                   mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                           t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_h, t_railing_v, t_rock,
                                           t_door_glass_c, t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v,
                                           t_floor, t_floor, t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window,
                                           t_floor,   t_floor,  t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,
                                           t_sidewalk, t_floor),
                                   mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o",
                                           f_cupboard, f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null,
                                           f_null, f_null,         f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_table, f_bed,   f_null,   f_null,                 f_null,         f_null,        f_null,
                                           f_bathtub, f_toilet, f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,
                                           f_bookcase));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_rack) {
                    place_items("dresser", 30,  i,  j, i,  j, false, 0);
                    place_items("jackets", 60,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_fridge) {
                    place_items("fridge", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_oven) {
                    place_items("oven", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_cupboard) {
                    place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                    place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                    place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                    place_items("pasta", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_bookcase) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                    place_items("novels", 40,  i,  j, i,  j, false, 0);
                    place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                    place_items("manuals", 20,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        } else {
            add_spawn("mon_zombie", rng(1, 8), 15, 10);
        }
        if (t_north == "apartments_mod_tower_1" && t_west == "apartments_mod_tower_1") {
            rotate(3);
        } else if (t_north == "apartments_mod_tower_1" && t_east == "apartments_mod_tower_1") {
            rotate(0);
        } else if (t_south == "apartments_mod_tower_1" && t_east == "apartments_mod_tower_1") {
            rotate(1);
        } else if (t_west == "apartments_mod_tower_1" && t_south == "apartments_mod_tower_1") {
            rotate(2);
        }


    } else if (terrain_type == "apartments_mod_tower_1") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if ((t_south == "apartments_mod_tower_1_entrance" && t_east == "apartments_mod_tower_1") ||
            (t_north == "apartments_mod_tower_1" && t_east == "apartments_mod_tower_1_entrance")
            || (t_west == "apartments_mod_tower_1" && t_north == "apartments_mod_tower_1_entrance") ||
            (t_south == "apartments_mod_tower_1" && t_west == "apartments_mod_tower_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
       |----ww----|     \n\
       |dBB....oddw     \n\
    |--|.BB.......w     \n\
    |r.+..........|     \n\
    |--|-----+--|+|--|  \n\
    RsswFFFF...^|.STb|  \n\
    Rssw........+...b|-w\n\
    RssX........|--|-|EE\n\
    Rss|c.htth...oo|r|EE\n\
    Rss|e.htth.....+.|xE\n\
 |-----|c.........A|-|-=\n\
 |..BBd|cOS........|....\n\
 w..BB.|--|+|......D....\n\
 |d....+.r|u|^....t|....\n\
 |r...||--|-|------|....\n\
 w....|STb|u|...e.S|....\n\
 |....+..b|.+.....c|....\n\
 |--|+|-+-|-|.....O|....\n\
 RssX.....A....cccc|....\n\
 Rssw.........h.hh.|....\n\
 Rssw..............D....\n\
 Rss|..ooo.FFFF...^|....\n\
 R|-|--------------|....\n",
                                       mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                               t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_h, t_railing_v, t_rock,
                                               t_door_glass_c, t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v,
                                               t_floor, t_floor, t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window,
                                               t_floor,   t_floor,  t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,
                                               t_sidewalk, t_floor),
                                       mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o",
                                               f_cupboard, f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null,
                                               f_null, f_null,         f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                               f_null,   f_table, f_bed,   f_null,   f_null,                 f_null,         f_null,        f_null,
                                               f_bathtub, f_toilet, f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,
                                               f_bookcase));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_rack) {
                        place_items("dresser", 30,  i,  j, i,  j, false, 0);
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_fridge) {
                        place_items("fridge", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_oven) {
                        place_items("oven", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_cupboard) {
                        place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                        place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                        place_items("pasta", 50,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_bookcase) {
                        place_items("magazines", 30,  i,  j, i,  j, false, 0);
                        place_items("novels", 40,  i,  j, i,  j, false, 0);
                        place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                        place_items("manuals", 20,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                add_spawn("mon_zombie", rng(1, 8), 15, 10);
            }
            if (t_west == "apartments_mod_tower_1_entrance") {
                rotate(1);
            }
            if (t_north == "apartments_mod_tower_1_entrance") {
                rotate(2);
            }
            if (t_east == "apartments_mod_tower_1_entrance") {
                rotate(3);
            }
        }

        else if ((t_west == "apartments_mod_tower_1_entrance" && t_north == "apartments_mod_tower_1") ||
                 (t_north == "apartments_mod_tower_1_entrance" && t_east == "apartments_con_tower_1")
                 || (t_west == "apartments_mod_tower_1" && t_south == "apartments_mod_tower_1_entrance") ||
                 (t_south == "apartments_mod_tower_1" && t_east == "apartments_mod_tower_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
....|cSe.u.htth...oo.w  \n\
....|O.....htth......w  \n\
....|ccc..........|X-|  \n\
....|......|----+-|ssR  \n\
....|.....A|dBB...|ssR  \n\
....|.....^|.BB...wssR  \n\
....D...|--|......wssR  \n\
....|-+-|r.+......|ssR  \n\
....|..r|--|----|+|--|| \n\
....|---|..+.STb|....b| \n\
....|u..+..|...b|S.T.b| \n\
....|---|..|-|+-|-----| \n\
....|cSe...|r+..d.BBd.| \n\
....|O.....|-|....BB..w \n\
....|ccc.....+........| \n\
....D.......A|....o|--| \n\
....|........|-www-|    \n\
....|t.......wsssssR    \n\
G-w-|t...FFF.XsssssR    \n\
ss  |--ww----|aaaaa|    \n\
ss                      \n\
ss                      \n\
ss                      \n\
ss                      \n",
                                       mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                               t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_h, t_railing_v, t_rock,
                                               t_door_glass_c, t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v,
                                               t_floor, t_floor, t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window,
                                               t_floor,   t_floor,  t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,
                                               t_sidewalk, t_floor),
                                       mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o",
                                               f_cupboard, f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null,
                                               f_null, f_null,         f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                               f_null,   f_table, f_bed,   f_null,   f_null,                 f_null,         f_null,        f_null,
                                               f_bathtub, f_toilet, f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,
                                               f_bookcase));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_rack) {
                        place_items("dresser", 30,  i,  j, i,  j, false, 0);
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_fridge) {
                        place_items("fridge", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_oven) {
                        place_items("oven", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_cupboard) {
                        place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                        place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                        place_items("pasta", 50,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_bookcase) {
                        place_items("magazines", 30,  i,  j, i,  j, false, 0);
                        place_items("novels", 40,  i,  j, i,  j, false, 0);
                        place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                        place_items("manuals", 20,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                add_spawn("mon_zombie", rng(1, 8), 15, 10);
            }
            if (t_north == "apartments_mod_tower_1_entrance") {
                rotate(1);
            }
            if (t_east == "apartments_mod_tower_1_entrance") {
                rotate(2);
            }
            if (t_south == "apartments_mod_tower_1_entrance") {
                rotate(3);
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
     |----ww----|       \n\
     wcxc....BBd|       \n\
     w.h.....BB.|--|    \n\
     |..........+.r|    \n\
  |--|+|--+-----|--|    \n\
  |bTS.|...FFFF.wssR    \n\
w-|b...+........wssR    \n\
EE|-|--|........XssR    \n\
EE|r|..........c|ssR    \n\
EE|.+..........e|ssR    \n\
=-|-|..hh......c|-----| \n\
....|..tt....SOc|dBB..| \n\
....D..tt..|+|--|.BB..w \n\
....|..hh.^|u|r.+....d| \n\
....|------|-|--||...r| \n\
....|S.e..^|u|bTS|....w \n\
....|c.....+.|b..+....| \n\
....|O.....|-|-+-|+|--| \n\
....|cccc...oo..A..XssR \n\
....|.h.h..........wssR \n\
....D..............wssR \n\
....|^....A..FFFF..|ssR \n\
....|--------------|-|R \n",
                                       mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o", t_floor,
                                               t_floor,    t_floor, t_elevator, t_stairs_down, t_stairs_up, t_railing_h, t_railing_v, t_rock,
                                               t_door_glass_c, t_floor, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v,
                                               t_floor, t_floor, t_door_c, t_door_locked_interior, t_door_metal_c, t_door_locked, t_window,
                                               t_floor,   t_floor,  t_floor, t_floor,  t_floor, t_floor, t_floor,   t_floor,   t_floor,
                                               t_sidewalk, t_floor),
                                       mapf::basic_bind("u A F E > < a R # G r x % ^ . - | t B + D = X w b T S e O h c d l s o",
                                               f_cupboard, f_armchair, f_sofa,  f_null,     f_null,        f_null,      f_null,      f_null,
                                               f_null, f_null,         f_rack,  f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                               f_null,   f_table, f_bed,   f_null,   f_null,                 f_null,         f_null,        f_null,
                                               f_bathtub, f_toilet, f_sink,  f_fridge, f_oven,  f_chair, f_counter, f_dresser, f_locker, f_null,
                                               f_bookcase));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_locker) {
                        place_items("office", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_rack) {
                        place_items("dresser", 30,  i,  j, i,  j, false, 0);
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_fridge) {
                        place_items("fridge", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_oven) {
                        place_items("oven", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_cupboard) {
                        place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                        place_items("home_hw", 30,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 50,  i,  j, i,  j, false, 0);
                        place_items("pasta", 50,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_bookcase) {
                        place_items("magazines", 30,  i,  j, i,  j, false, 0);
                        place_items("novels", 40,  i,  j, i,  j, false, 0);
                        place_items("alcohol", 30,  i,  j, i,  j, false, 0);
                        place_items("manuals", 20,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                add_spawn("mon_zombie", rng(1, 8), 15, 10);
            }
            if (t_west == "apartments_mod_tower_1" && t_north == "apartments_mod_tower_1") {
                rotate(1);
            } else if (t_east == "apartments_mod_tower_1" && t_north == "apartments_mod_tower_1") {
                rotate(2);
            } else if (t_east == "apartments_mod_tower_1" && t_south == "apartments_mod_tower_1") {
                rotate(3);
            }
        }


    } else if (terrain_type == "office_tower_1_entrance") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
ss%|....+...|...|EEED...\n\
ss%|----|...|...|EEx|...\n\
ss%Vcdc^|...|-+-|---|...\n\
ss%Vch..+...............\n\
ss%V....|...............\n\
ss%|----|-|-+--ccc--|...\n\
ss%|..C..C|.....h..r|-+-\n\
sss=......+..h.....r|...\n\
ss%|r..CC.|.ddd....r|T.S\n\
ss%|------|---------|---\n\
ss%|####################\n\
ss%|#|------||------|###\n\
ss%|#|......||......|###\n\
ss%|||......||......|###\n\
ss%||x......||......||##\n\
ss%|||......||......x|##\n\
ss%|#|......||......||##\n\
ss%|#|......||......|###\n\
ss%|#|XXXXXX||XXXXXX|###\n\
ss%|-|__,,__||__,,__|---\n\
ss%% x_,,,,_  __,,__  %%\n\
ss    __,,__  _,,,,_    \n\
ssssss__,,__ss__,,__ssss\n\
ssssss______ss______ssss\n",
                                   mapf::basic_bind("E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_elevator,
                                           t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,
                                           t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                           t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                           t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_floor,  t_sidewalk),
                                   mapf::basic_bind("E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_null,
                                           f_null,        f_null,      f_null,      f_null, f_null,              f_null,         f_crate_c,
                                           f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,
                                           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                           f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        place_items("office", 75, 4, 2, 6, 2, false, 0);
        place_items("office", 75, 19, 6, 19, 6, false, 0);
        place_items("office", 75, 12, 8, 14, 8, false, 0);
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 12, 3, density);
        } else {
            if (x_in_y(1, 2)) {
                add_spawn("mon_zombie", 2, 15, 7);
            }
            if (x_in_y(1, 2)) {
                add_spawn("mon_zombie", rng(1, 8), 22, 1);
            }
            if (x_in_y(1, 2)) {
                add_spawn("mon_zombie_cop", 1, 22, 4);
            }
        }
        {
            int num_chairs = rng(0, 6);
            for( int i = 0; i < num_chairs; i++ ) {
                add_vehicle (g, "swivel_chair", rng(6, 16), rng(6, 16), 0, -1, -1, false);
            }
        }
        if (t_north == "office_tower_1" && t_west == "office_tower_1") {
            rotate(3);
        } else if (t_north == "office_tower_1" && t_east == "office_tower_1") {
            rotate(0);
        } else if (t_south == "office_tower_1" && t_east == "office_tower_1") {
            rotate(1);
        } else if (t_west == "office_tower_1" && t_south == "office_tower_1") {
            rotate(2);
        }


    } else if (terrain_type == "office_tower_1") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if ((t_south == "office_tower_1_entrance" && t_east == "office_tower_1") ||
            (t_north == "office_tower_1" && t_east == "office_tower_1_entrance") ||
            (t_west == "office_tower_1" && t_north == "office_tower_1_entrance") ||
            (t_south == "office_tower_1" && t_west == "office_tower_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
ss                      \n\
ss%%%%%%%%%%%%%%%%%%%%%%\n\
ss%|-HH-|-HH-|-HH-|HH|--\n\
ss%Vdcxl|dxdl|lddx|..|.S\n\
ss%Vdh..|dh..|..hd|..+..\n\
ss%|-..-|-..-|-..-|..|--\n\
ss%V.................|.T\n\
ss%V.................|..\n\
ss%|-..-|-..-|-..-|..|--\n\
ss%V.h..|..hd|..hd|..|..\n\
ss%Vdxdl|^dxd|.xdd|..G..\n\
ss%|----|----|----|..G..\n\
ss%|llll|..htth......|..\n\
ss%V.................|..\n\
ss%V.ddd..........|+-|..\n\
ss%|..hd|.hh.ceocc|.l|..\n\
ss%|----|---------|--|..\n\
ss%Vcdcl|...............\n\
ss%V.h..+...............\n\
ss%V...^|...|---|---|...\n\
ss%|----|...|.R>|EEE|...\n\
ss%|rrrr|...|.R.|EEED...\n",
                                       mapf::basic_bind("E > R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_elevator,
                                               t_stairs_down, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,   t_pavement_y,
                                               t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken, t_shrub, t_floor,
                                               t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked, t_door_locked_alarm, t_window,
                                               t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("E > R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_null,
                                               f_null,        f_null,      f_null, f_null,              f_null,         f_crate_c, f_null,
                                               f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,           f_null,
                                               f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 2, 8, density);
            } else {
                add_spawn("mon_zombie", rng(0, 5), 15, 7);
                if (x_in_y(1, 1)) {
                    add_spawn("mon_zombie", 2, 5, 20);
                }
            }
            place_items("office", 75, 4, 23, 7, 23, false, 0);
            place_items("office", 75, 4, 19, 7, 19, false, 0);
            place_items("office", 75, 4, 14, 7, 14, false, 0);
            place_items("office", 75, 5, 16, 7, 16, false, 0);
            place_items("fridge", 80, 14, 17, 14, 17, false, 0);
            place_items("cleaning", 75, 19, 17, 20, 17, false, 0);
            place_items("cubical_office", 75, 6, 12, 7, 12, false, 0);
            place_items("cubical_office", 75, 12, 11, 12, 12, false, 0);
            place_items("cubical_office", 75, 16, 11, 17, 12, false, 0);
            place_items("cubical_office", 75, 4, 5, 5, 5, false, 0);
            place_items("cubical_office", 75, 11, 5, 12, 5, false, 0);
            place_items("cubical_office", 75, 14, 5, 16, 5, false, 0);
            {
                int num_chairs = rng(0, 6);
                for( int i = 0; i < num_chairs; i++ ) {
                    add_vehicle (g, "swivel_chair", rng(6, 16), rng(6, 16), 0, -1, -1, false);
                }
            }
            if (t_west == "office_tower_1_entrance") {
                rotate(1);
            }
            if (t_north == "office_tower_1_entrance") {
                rotate(2);
            }
            if (t_east == "office_tower_1_entrance") {
                rotate(3);
            }
        }

        else if ((t_west == "office_tower_1_entrance" && t_north == "office_tower_1") ||
                 (t_north == "office_tower_1_entrance" && t_east == "office_tower_1") ||
                 (t_west == "office_tower_1" && t_south == "office_tower_1_entrance") ||
                 (t_south == "office_tower_1" && t_east == "office_tower_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
...DEEE|...|..|-----|%ss\n\
...|EEE|...|..|^...lV%ss\n\
...|---|-+-|......hdV%ss\n\
...........G..|..dddV%ss\n\
...........G..|-----|%ss\n\
.......|---|..|...ddV%ss\n\
|+-|...|...+......hdV%ss\n\
|.l|...|rr.|.^|l...dV%ss\n\
|--|...|---|--|-----|%ss\n\
|...........c.......V%ss\n\
|.......cxh.c.#####.Vsss\n\
|.......ccccc.......Gsss\n\
|...................Gsss\n\
|...................Vsss\n\
|#..................Gsss\n\
|#..................Gsss\n\
|#..................Vsss\n\
|#............#####.V%ss\n\
|...................|%ss\n\
--HHHHHGGHHGGHHHHH--|%ss\n\
%%%%% ssssssss %%%%%%%ss\n\
      ssssssss        ss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n",
                                       mapf::basic_bind("E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_elevator,
                                               t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,
                                               t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                               t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                               t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                               t_floor,  t_sidewalk),
                                       mapf::basic_bind("E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_null,
                                               f_null,        f_null,      f_null,      f_null, f_null,              f_null,         f_crate_c,
                                               f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,
                                               f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            place_items("office", 75, 19, 1, 19, 3, false, 0);
            place_items("office", 75, 17, 3, 18, 3, false, 0);
            place_items("office", 90, 8, 7, 9, 7, false, 0);
            place_items("cubical_office", 75, 19, 5, 19, 7, false, 0);
            place_items("cleaning", 80, 1, 7, 2, 7, false, 0);
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 14, 10, density);
            } else {
                add_spawn("mon_zombie", rng(0, 15), 14, 10);
                if (x_in_y(1, 2)) {
                    add_spawn("mon_zombie_cop", 2, 10, 10);
                }
            }
            {
                int num_chairs = rng(0, 6);
                for( int i = 0; i < num_chairs; i++ ) {
                    add_vehicle (g, "swivel_chair", rng(6, 16), rng(6, 16), 0, -1, -1, false);
                }
            }
            if (t_north == "office_tower_1_entrance") {
                rotate(1);
            }
            if (t_east == "office_tower_1_entrance") {
                rotate(2);
            }
            if (t_south == "office_tower_1_entrance") {
                rotate(3);
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
                      ss\n\
%%%%%%%%%%%%%%%%%%%%%%ss\n\
--|---|--HHHH-HHHH--|%ss\n\
.T|..l|............^|%ss\n\
..|-+-|...hhhhhhh...V%ss\n\
--|...G...ttttttt...V%ss\n\
.S|...G...ttttttt...V%ss\n\
..+...|...hhhhhhh...V%ss\n\
--|...|.............|%ss\n\
..|...|-------------|%ss\n\
..G....|l.......dxd^|%ss\n\
..G....G...h....dh..V%ss\n\
..|....|............V%ss\n\
..|....|------|llccc|%ss\n\
..|...........|-----|%ss\n\
..|...........|...ddV%ss\n\
..|----|---|......hdV%ss\n\
.......+...|..|l...dV%ss\n\
.......|rrr|..|-----|%ss\n\
...|---|---|..|l.dddV%ss\n\
...|xEE|.R>|......hdV%ss\n\
...DEEE|.R.|..|.....V%ss\n",
                                       mapf::basic_bind("E > R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_elevator,
                                               t_stairs_down, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,   t_pavement_y,
                                               t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken, t_shrub, t_floor,
                                               t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked, t_door_locked_alarm, t_window,
                                               t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("E > R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_null,
                                               f_null,        f_null,      f_null, f_null,              f_null,         f_crate_c, f_null,
                                               f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,           f_null,
                                               f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            spawn_item(18, 15, "record_accounting");
            place_items("cleaning", 75, 3, 5, 5, 5, false, 0);
            place_items("office", 75, 10, 7, 16, 8, false, 0);
            place_items("cubical_office", 75, 15, 15, 19, 15, false, 0);
            place_items("cubical_office", 75, 16, 12, 16, 13, false, 0);
            place_items("cubical_office", 75, 17, 19, 19, 19, false, 0);
            place_items("office", 75, 17, 21, 19, 21, false, 0);
            place_items("office", 75, 16, 11, 17, 12, false, 0);
            place_items("cleaning", 75, 8, 20, 10, 20, false, 0);
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 9, 15, density);
            } else {
                add_spawn("mon_zombie", rng(0, 5), 9, 15);
            }
            {
                int num_chairs = rng(0, 6);
                for( int i = 0; i < num_chairs; i++ ) {
                    add_vehicle (g, "swivel_chair", rng(6, 16), rng(6, 16), 0, -1, -1, false);
                }
            }
            if (t_west == "office_tower_1" && t_north == "office_tower_1") {
                rotate(1);
            } else if (t_east == "office_tower_1" && t_north == "office_tower_1") {
                rotate(2);
            } else if (t_east == "office_tower_1" && t_south == "office_tower_1") {
                rotate(3);
            }
        }


    } else if (terrain_type == "office_tower_b_entrance") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
sss|........|...|EEED___\n\
sss|........|...|EEx|___\n\
sss|........|-+-|---|HHG\n\
sss|....................\n\
sss|....................\n\
sss|....................\n\
sss|....................\n\
sss|....,,......,,......\n\
sss|...,,,,.....,,......\n\
sss|....,,.....,,,,..xS.\n\
sss|....,,......,,...SS.\n\
sss|-|XXXXXX||XXXXXX|---\n\
sss|s|EEEEEE||EEEEEE|sss\n\
sss|||EEEEEE||EEEEEE|sss\n\
sss||xEEEEEE||EEEEEE||ss\n\
sss|||EEEEEE||EEEEEEx|ss\n\
sss|s|EEEEEE||EEEEEE||ss\n\
sss|s|EEEEEE||EEEEEE|sss\n\
sss|s|------||------|sss\n\
sss|--------------------\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n",
                                   mapf::basic_bind("E s > < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S",
                                           t_elevator, t_rock, t_stairs_down, t_stairs_up, t_railing_v, t_floor, t_door_metal_locked,
                                           t_door_glass_c, t_floor,   t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h,
                                           t_console, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor,
                                           t_door_c, t_door_locked, t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,
                                           t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("E s > < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S", f_null,
                                           f_null, f_null,        f_null,      f_null,      f_bench, f_null,              f_null,
                                           f_crate_c, f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,
                                           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                           f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
        } else {
            add_spawn("mon_zombie", rng(0, 5), SEEX * 2 - 1, SEEX * 2 - 1);
        }
        if (t_north == "office_tower_b" && t_west == "office_tower_b") {
            rotate(3);
        } else if (t_north == "office_tower_b" && t_east == "office_tower_b") {
            rotate(0);
        } else if (t_south == "office_tower_b" && t_east == "office_tower_b") {
            rotate(1);
        } else if (t_west == "office_tower_b" && t_south == "office_tower_b") {
            rotate(2);
        }


    } else if (terrain_type == "office_tower_b") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if ((t_south == "office_tower_b_entrance" && t_east == "office_tower_b") ||
            (t_north == "office_tower_b" && t_east == "office_tower_b_entrance") ||
            (t_west == "office_tower_b" && t_north == "office_tower_b_entrance") ||
            (t_south == "office_tower_b" && t_west == "office_tower_b_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
sss|--------------------\n\
sss|,.....,.....,.....,S\n\
sss|,.....,.....,.....,S\n\
sss|,.....,.....,.....,S\n\
sss|,.....,.....,.....,S\n\
sss|,.....,.....,.....,S\n\
sss|,.....,.....,.....,S\n\
sss|....................\n\
sss|....................\n\
sss|....................\n\
sss|....................\n\
sss|....................\n\
sss|....................\n\
sss|...,,...,....,....,S\n\
sss|..,,,,..,....,....,S\n\
sss|...,,...,....,....,S\n\
sss|...,,...,....,....,S\n\
sss|........,....,....,S\n\
sss|........,....,....,S\n\
sss|........|---|---|HHG\n\
sss|........|.R<|EEE|___\n\
sss|........|.R.|EEED___\n",
                                       mapf::basic_bind("E s < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S", t_elevator,
                                               t_rock, t_stairs_up, t_railing_v, t_floor, t_door_metal_locked, t_door_glass_c, t_floor,
                                               t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                               t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                               t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                               t_floor, t_sidewalk),
                                       mapf::basic_bind("E s < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S", f_null,
                                               f_null, f_null,      f_null,      f_bench, f_null,              f_null,         f_crate_c, f_null,
                                               f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,           f_null,
                                               f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
            } else {
                add_spawn("mon_zombie", rng(0, 5), SEEX * 2 - 1, SEEX * 2 - 1);
            }
            if (t_west == "office_tower_b_entrance") {
                rotate(1);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 17, 7, 180);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "motorcycle", 17, 13, 180);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 17, 19, 180);
                }
            } else if (t_north == "office_tower_b_entrance") {
                rotate(2);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 10, 17, 270);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "motorcycle", 4, 18, 270);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 16, 17, 270);
                }
            } else if (t_east == "office_tower_b_entrance") {
                rotate(3);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 6, 4, 0);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "motorcycle", 6, 10, 180);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 6, 16, 0);
                }

            } else {
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 7, 6, 90);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 14, 6, 90);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "motorcycle", 19, 6, 90);
                }
            }
        }

        else if ((t_west == "office_tower_b_entrance" && t_north == "office_tower_b") ||
                 (t_north == "office_tower_b_entrance" && t_east == "office_tower_b") ||
                 (t_west == "office_tower_b" && t_south == "office_tower_b_entrance") ||
                 (t_south == "office_tower_b" && t_east == "office_tower_b_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
___DEEE|...|...,,...|sss\n\
___|EEE|...|..,,,,..|sss\n\
GHH|---|-+-|...,,...|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
|...................|sss\n\
|...................|sss\n\
|,.....,.....,.....,|sss\n\
|,.....,.....,.....,|sss\n\
|,.....,.....,.....,|sss\n\
|,.....,.....,.....,|sss\n\
|,.....,.....,.....,|sss\n\
|,.....,.....,.....,|sss\n\
|-------------------|sss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n",
                                       mapf::basic_bind("E s > < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S",
                                               t_elevator, t_rock, t_stairs_down, t_stairs_up, t_railing_v, t_floor, t_door_metal_locked,
                                               t_door_glass_c, t_floor,   t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h,
                                               t_console, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor,
                                               t_door_c, t_door_locked, t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,
                                               t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("E s > < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S", f_null,
                                               f_null, f_null,        f_null,      f_null,      f_bench, f_null,              f_null,
                                               f_crate_c, f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,
                                               f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
            } else {
                add_spawn("mon_zombie", rng(0, 5), SEEX * 2 - 1, SEEX * 2 - 1);
            }
            if (t_north == "office_tower_b_entrance") {
                rotate(1);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 8, 15, 0);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 7, 10, 180);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "beetle", 7, 3, 0);
                }
            } else if (t_east == "office_tower_b_entrance") {
                rotate(2);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 7, 7, 270);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 13, 8, 90);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "beetle", 20, 7, 90);
                }
            } else if (t_south == "office_tower_b_entrance") {
                rotate(3);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 16, 7, 0);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 15, 13, 180);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "beetle", 15, 20, 180);
                }
            } else {
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 16, 16, 90);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "car", 9, 15, 270);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "beetle", 4, 16, 270);
                }
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
--------------------|sss\n\
S,.....,.....,.....,|sss\n\
S,.....,.....,.....,|sss\n\
S,.....,.....,.....,|sss\n\
S,.....,.....,.....,|sss\n\
S,.....,.....,.....,|sss\n\
S,.....,.....,.....,|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
....................|sss\n\
S,....,....,........|sss\n\
S,....,....,........|sss\n\
S,....,....,........|sss\n\
S,....,....,........|sss\n\
S,....,....,........|sss\n\
S,....,....,........|sss\n\
GHH|---|---|........|sss\n\
___|xEE|.R<|........|sss\n\
___DEEE|.R.|...,,...|sss\n",
                                       mapf::basic_bind("E s < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S", t_elevator,
                                               t_rock, t_stairs_up, t_railing_v, t_floor, t_door_metal_locked, t_door_glass_c, t_floor,
                                               t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                               t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                               t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                               t_floor, t_sidewalk),
                                       mapf::basic_bind("E s < R # X G C , . r V H 6 x % ^ _ - | t + = D w T S e o h c d l S", f_null,
                                               f_null, f_null,      f_null,      f_bench, f_null,              f_null,         f_crate_c, f_null,
                                               f_null,     f_rack,  f_null,         f_null,         f_null,    f_null,           f_null,
                                               f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
            } else {
                add_spawn("mon_zombie", rng(0, 5), SEEX * 2 - 1, SEEX * 2 - 1);
            }
            if (t_west == "office_tower_b" && t_north == "office_tower_b") {
                rotate(1);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "cube_van", 17, 4, 180);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 17, 10, 180);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "car", 17, 17, 180);
                }
            } else if (t_east == "office_tower_b" && t_north == "office_tower_b") {
                rotate(2);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "cube_van", 6, 17, 270);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 12, 17, 270);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "car", 18, 17, 270);
                }
            } else if (t_east == "office_tower_b" && t_south == "office_tower_b") {
                rotate(3);
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "cube_van", 6, 6, 0);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 6, 13, 0);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "car", 5, 19, 180);
                }
            } else {
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "flatbed_truck", 16, 6, 90);
                }
                if (x_in_y(1, 5)) {
                    add_vehicle (g, "cube_van", 10, 6, 90);
                }
                if (x_in_y(1, 3)) {
                    add_vehicle (g, "car", 4, 6, 90);
                }
            }
        }


    } else if (is_ot_type("church", terrain_type)) {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if (one_in(2)) {
            //New England or Country style, single centered steeple low clear windows
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
         ^^^^^^         \n\
     |---|--------|     \n\
    ||dh.|.6ooo.ll||    \n\
    |l...+.........Dsss \n\
  ^^|--+-|------+--|^^s \n\
  ^||..............||^s \n\
   w.......tt.......w s \n\
   |................| s \n\
  ^w................w^s \n\
  ^|.######..######.|^s \n\
  ^w................w^s \n\
  ^|.######..######.|^s \n\
   w................w s \n\
   |.######..######.| s \n\
  ^w................w^s \n\
  ^|.######..######.|^s \n\
  ^|................|^s \n\
   |-w|----..----|w-| s \n\
    ^^|ll|....|ST|^^  s \n\
     ^|.......+..|^   s \n\
      |----++-|--|    s \n\
         O ss O       s \n\
     ^^    ss    ^^   s \n\
     ^^    ss    ^^   s \n",
                                       mapf::basic_bind("O 6 ^ . - | # t + = D w T S e o h c d l s", t_column, t_console, t_shrub, t_floor,
                                               t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                               t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("O 6 ^ . - | # t + = D w T S e o h c d l s", f_null,   f_null,    f_null,  f_null,
                                               f_null,   f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,
                                               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            spawn_item(9, 6, "brazier");
            spawn_item(14, 6, "brazier");
            place_items("church", 40,  5,  5, 8,  16, false, 0);
            place_items("church", 40,  5,  5, 8,  16, false, 0);
            place_items("church", 85,  12,  2, 14,  2, false, 0);
            place_items("office", 60,  6,  2, 8,  3, false, 0);
            place_items("jackets", 85,  7,  18, 8,  18, false, 0);
            tmpcomp = add_computer(11, 2, _("Church Bells 1.2"), 0);
            tmpcomp->add_option(_("Gathering Toll"), COMPACT_TOLL, 0);
            tmpcomp->add_option(_("Wedding Toll"), COMPACT_TOLL, 0);
            tmpcomp->add_option(_("Funeral Toll"), COMPACT_TOLL, 0);
        } else {
            //Gothic Style, unreachable high stained glass windows, stone construction
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
 $$    W        W    $$ \n\
 $$  WWWWGVBBVGWWWW  $$ \n\
     W..h.cccc.h..W     \n\
 WWVWWR..........RWWBWW \n\
WW#.#.R....cc....R.#.#WW\n\
 G#.#.R..........R.#.#V \n\
 V#.#.Rrrrr..rrrrR.#.#B \n\
WW#..................#WW\n\
 WW+WW#####..#####WW+WW \n\
ssss V............B ssss\n\
s   WW#####..#####WW   s\n\
s $ WW............WW $ s\n\
s $  G#####..#####V  $ s\n\
s $  B............G  $ s\n\
s $ WW#####..#####WW $ s\n\
s $ WW............WW $ s\n\
s    V####....####B    s\n\
s WWWW--|--gg-----WWWW s\n\
s WllWTS|.....lll.W..W s\n\
s W..+..+.........+..W s\n\
s W..WWWWWW++WWWWWW6.W s\n\
s W.CWW$$WWssWW$$WW..W s\n\
s WWWWW    ss    WWWWW s\n\
ssssssssssssssssssssssss\n",
                                       mapf::basic_bind("C V G B W R r 6 $ . - | # t + g T S h c l s", t_floor,   t_window_stained_red,
                                               t_window_stained_green, t_window_stained_blue, t_rock, t_railing_v, t_railing_h, t_console, t_shrub,
                                               t_rock_floor, t_wall_h, t_wall_v, t_rock_floor, t_rock_floor, t_door_c, t_door_glass_c,
                                               t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor, t_sidewalk),
                                       mapf::basic_bind("C V G B W R r 6 $ . - | # t + g T S h c l s", f_crate_c, f_null,
                                               f_null,                 f_null,                f_null, f_null,      f_null,      f_null,    f_null,
                                               f_null,       f_null,   f_null,   f_bench,      f_table,      f_null,   f_null,         f_toilet,
                                               f_sink,       f_chair,      f_counter,    f_locker,     f_null));
            spawn_item(8, 4, "brazier");
            spawn_item(15, 4, "brazier");
            place_items("church", 70,  6,  7, 17,  16, false, 0);
            place_items("church", 70,  6,  7, 17,  16, false, 0);
            place_items("church", 60,  6,  7, 17,  16, false, 0);
            place_items("cleaning", 60,  3,  18, 4,  21, false, 0);
            place_items("jackets", 85,  14,  18, 16,  18, false, 0);
            tmpcomp = add_computer(19, 20, _("Church Bells 1.2"), 0);
            tmpcomp->add_option(_("Gathering Toll"), COMPACT_TOLL, 0);
            tmpcomp->add_option(_("Wedding Toll"), COMPACT_TOLL, 0);
            tmpcomp->add_option(_("Funeral Toll"), COMPACT_TOLL, 0);
        }
        if (terrain_type == "church_east") {
            rotate(3);
        }
        if (terrain_type == "church_north") {
            rotate(2);
        }
        if (terrain_type == "church_west") {
            rotate(1);
        }


    } else if (terrain_type == "cathedral_1_entrance") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
ss         ##...........\n\
ss         ##bbbb...bbbb\n\
ss          #...........\n\
ss          wbbbb...bbbb\n\
ss          w...........\n\
ss          wbbbb...bbbb\n\
ss          #...........\n\
ss         ##bbbb...bbbb\n\
ss         ##...........\n\
ss          #bbb....bbbb\n\
ss          w...........\n\
ss          #####GG###HH\n\
ss          w>R.....#...\n\
ss   ########...........\n\
ss   #......#...........\n\
ss   w......#.lllcc.....\n\
ss   #......###ww####++#\n\
ss   w......#      ##sss\n\
ss   #......#       ssss\n\
ss   ###ww###       ssss\n\
ss                  ssss\n\
ss                  ssss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n",
                                   mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_floor,
                                           t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c,
                                           t_floor,   t_pavement_y, t_pavement, t_railing_h, t_wall_glass_v, t_wall_glass_h, t_console,
                                           t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                           t_door_locked, t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_floor,
                                           t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_bench,
                                           f_null,     f_null,        f_null,      f_null,      f_null, f_null,              f_null,
                                           f_crate_c, f_null,       f_null,     f_null,      f_null,         f_null,         f_null,    f_null,
                                           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                           f_null,               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                           f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bench) {
                    place_items("church", 10,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_locker) {
                    place_items("jackets", 60,  i,  j, i,  j, false, 0);
                }
                if (this->ter(i, j) == t_window_stained_red) {
                    if (one_in(3)) {
                        ter_set(i, j, t_window_stained_blue);
                    } else if (one_in(3)) {
                        ter_set(i, j, t_window_stained_green);
                    }
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        } else {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, .20);
        }
        if (t_north == "cathedral_1" && t_west == "cathedral_1") {
            rotate(3);
        } else if (t_north == "cathedral_1" && t_east == "cathedral_1") {
            rotate(0);
        } else if (t_south == "cathedral_1" && t_east == "cathedral_1") {
            rotate(1);
        } else if (t_west == "cathedral_1" && t_south == "cathedral_1") {
            rotate(2);
        }


    } else if (terrain_type == "cathedral_1") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if ((t_south == "cathedral_1_entrance" && t_east == "cathedral_1") || (t_north == "cathedral_1" &&
                t_east == "cathedral_1_entrance") || (t_west == "cathedral_1" &&
                        t_north == "cathedral_1_entrance") ||
            (t_south == "cathedral_1" && t_west == "cathedral_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                  ##    \n\
                 ###wwww\n\
                ##....tt\n\
               ##.......\n\
               w..h.....\n\
             ###.....ttt\n\
             w..........\n\
      ##   ###R.........\n\
    ####www#..rrrrrrrr..\n\
  ###...................\n\
  #...C.................\n\
 ##......bbbbbbb...bbbbb\n\
  w.....................\n\
  w..bbbbbbbbbbb...bbbbb\n\
  w.....................\n\
 ##..bbbbbbbbbbb...bbbbb\n\
  #.....................\n\
  ###...................\n\
    #####+##............\n\
      ##sss##bbbb...bbbb\n\
sssssssssss>#...........\n\
sssssssssssswbbbb...bbbb\n\
ss          w...........\n\
ss          #bbbb...bbbb\n",
                                       mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_floor,
                                               t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c,
                                               t_floor,   t_pavement_y, t_pavement, t_railing_h, t_wall_glass_v, t_wall_glass_h, t_console,
                                               t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                               t_door_locked, t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_floor,
                                               t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_bench,
                                               f_null,     f_null,        f_null,      f_null,      f_null, f_null,              f_null,
                                               f_crate_c, f_null,       f_null,     f_null,      f_null,         f_null,         f_null,    f_null,
                                               f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                               f_locker, f_null));
            spawn_item(20, 2, "brazier");
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_bench) {
                        place_items("church", 10,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_locker) {
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_window_stained_red) {
                        if (one_in(3)) {
                            ter_set(i, j, t_window_stained_blue);
                        } else if (one_in(3)) {
                            ter_set(i, j, t_window_stained_green);
                        }
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, .20);
            }
            if (t_west == "cathedral_1_entrance") {
                rotate(1);
            }
            if (t_north == "cathedral_1_entrance") {
                rotate(2);
            }
            if (t_east == "cathedral_1_entrance") {
                rotate(3);
            }
        }

        else if ((t_west == "cathedral_1_entrance" && t_north == "cathedral_1") ||
                 (t_north == "cathedral_1_entrance" && t_east == "cathedral_1") || (t_west == "cathedral_1" &&
                         t_south == "cathedral_1_entrance") ||
                 (t_south == "cathedral_1" && t_east == "cathedral_1_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
..........##          ss\n\
bbb...bbbb##          ss\n\
..........#           ss\n\
bbb...bbbbw           ss\n\
..........w           ss\n\
bbb...bbbbw           ss\n\
..........#           ss\n\
bbb...bbbb##          ss\n\
..........##          ss\n\
bbb....bbb#           ss\n\
..........w           ss\n\
H###GG#####           ss\n\
..#.....R>w           ss\n\
..........########    ss\n\
..........#......#    ss\n\
...ccccc..#......w    ss\n\
++####ww###......#    ss\n\
ss##      #......w    ss\n\
sss       #......#    ss\n\
sss       ###ww###    ss\n\
sss                   ss\n\
sss                   ss\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n",
                                       mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_floor,
                                               t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c,
                                               t_floor,   t_pavement_y, t_pavement, t_railing_h, t_wall_glass_v, t_wall_glass_h, t_console,
                                               t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                               t_door_locked, t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_floor,
                                               t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_bench,
                                               f_null,     f_null,        f_null,      f_null,      f_null, f_null,              f_null,
                                               f_crate_c, f_null,       f_null,     f_null,      f_null,         f_null,         f_null,    f_null,
                                               f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                               f_locker, f_null));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_bench) {
                        place_items("church", 10,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_locker) {
                        place_items("jackets", 60,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_window_stained_red) {
                        if (one_in(3)) {
                            ter_set(i, j, t_window_stained_blue);
                        } else if (one_in(3)) {
                            ter_set(i, j, t_window_stained_green);
                        }
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, .20);
            }
            if (t_north == "cathedral_1_entrance") {
                rotate(1);
            }
            if (t_east == "cathedral_1_entrance") {
                rotate(2);
            }
            if (t_south == "cathedral_1_entrance") {
                rotate(3);
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
   ##                   \n\
www###                  \n\
t....##                 \n\
......##                \n\
....h..w                \n\
tt.....###              \n\
.........w              \n\
........R###   ##       \n\
.rrrrrrrr..#www####     \n\
..................###   \n\
................C...#   \n\
bbbb...bbbbbbb......##  \n\
....................w   \n\
bbbb...bbbbbbbbbbb..w   \n\
....................w   \n\
bbbb...bbbbbbbbbbb..##  \n\
....................#   \n\
..................###   \n\
...........##+#####     \n\
bbb...bbbb##sss##       \n\
..........#>ssssssssssss\n\
bbb...bbbbwsssssssssssss\n\
..........w           ss\n\
bbb...bbbb#           ss\n",
                                       mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", t_floor,
                                               t_elevator, t_stairs_down, t_stairs_up, t_railing_v, t_rock, t_door_metal_locked, t_door_glass_c,
                                               t_floor,   t_pavement_y, t_pavement, t_railing_h, t_wall_glass_v, t_wall_glass_h, t_console,
                                               t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                               t_door_locked, t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_floor,
                                               t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("b E > < R # X G C , _ r V H 6 x % ^ . - | t + = D w T S e o h c d l s", f_bench,
                                               f_null,     f_null,        f_null,      f_null,      f_null, f_null,              f_null,
                                               f_crate_c, f_null,       f_null,     f_null,      f_null,         f_null,         f_null,    f_null,
                                               f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,        f_null,
                                               f_null,               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                               f_locker, f_null));
            spawn_item(2, 2, "brazier");
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_bench) {
                        place_items("church", 10,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_window_stained_red) {
                        if (one_in(3)) {
                            ter_set(i, j, t_window_stained_blue);
                        } else if (one_in(3)) {
                            ter_set(i, j, t_window_stained_green);
                        }
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, .20);
            }
            if (t_west == "cathedral_1" && t_north == "cathedral_1") {
                rotate(1);
            } else if (t_east == "cathedral_1" && t_north == "cathedral_1") {
                rotate(2);
            } else if (t_east == "cathedral_1" && t_south == "cathedral_1") {
                rotate(3);
            }
        }


    } else if (terrain_type == "cathedral_b_entrance") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
############|+|+|+|..|..\n\
############|T|T|T|..|..\n\
############|-|-|-|--|..\n\
############|......d.|..\n\
############|hdhd..dh|..\n\
############|........|..\n\
############|hdhd....+..\n\
############|........|..\n\
############|hdhd....|..\n\
############|--------|..\n\
############|..^tt^.....\n\
############|...........\n\
############|<..........\n\
############|--D-|-+-ccc\n\
############|l...|......\n\
############|l..r|...ddd\n\
############|l..r|....hd\n\
############|----|------\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n",
                                   mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", t_floor,
                                           t_elevator, t_stairs_down, t_stairs_up, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,
                                           t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                           t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                           t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_column, t_floor,
                                           t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", f_oven,
                                           f_null,     f_null,        f_null,      f_null, f_null,              f_null,
                                           f_makeshift_bed, f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,
                                           f_null,           f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,
                                           f_null,                 f_null,               f_toilet, f_sink,  f_fridge, f_null,   f_chair,
                                           f_counter, f_desk,  f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_desk) {
                    place_items("school", 50,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_rack) {
                    place_items("cleaning", 70,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_rack) {
                    place_items("cleaning", 50,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        } else {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, .20);
        }
        if (t_north == "cathedral_b" && t_west == "cathedral_b") {
            rotate(3);
        } else if (t_north == "cathedral_b" && t_east == "cathedral_b") {
            rotate(0);
        } else if (t_south == "cathedral_b" && t_east == "cathedral_b") {
            rotate(1);
        } else if (t_west == "cathedral_b" && t_south == "cathedral_b") {
            rotate(2);
        }


    } else if (terrain_type == "cathedral_b") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if ((t_south == "cathedral_b_entrance" && t_east == "cathedral_b") || (t_north == "cathedral_b" &&
                t_east == "cathedral_b_entrance") || (t_west == "cathedral_b" &&
                        t_north == "cathedral_b_entrance") ||
            (t_south == "cathedral_b" && t_west == "cathedral_b_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
########################\n\
########################\n\
####################____\n\
###################__o__\n\
###################_____\n\
###################__o__\n\
################________\n\
################________\n\
#######|---|______o___o_\n\
#######|rrr|____________\n\
#######|...D____________\n\
######||-+-|------------\n\
######|c....c.hhh.......\n\
######|S....c.....C..C..\n\
######|c....c...........\n\
######|O....c.....C..C..\n\
######|O....|...........\n\
######|e....+.htth......\n\
######|e...r|.htth......\n\
######|---|-|.htth..ht..\n\
##########|<+........t..\n\
##########|-|--------|..\n\
############|..S.S.S.|..\n\
############|........+..\n",
                                       mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", t_floor,
                                               t_elevator, t_stairs_down, t_stairs_up, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,
                                               t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                               t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                               t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_column, t_floor,
                                               t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", f_oven,
                                               f_null,     f_null,        f_null,      f_null, f_null,              f_null,
                                               f_makeshift_bed, f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,
                                               f_null,           f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,
                                               f_null,                 f_null,               f_toilet, f_sink,  f_fridge, f_null,   f_chair,
                                               f_counter, f_desk,  f_locker, f_null));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_fridge) {
                        place_items("fridge", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_oven) {
                        place_items("oven", 70,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_table) {
                        place_items("fridgesnacks", 40,  i,  j, i,  j, false, 0);
                    } else if (this->furn(i, j) == f_rack) {
                        place_items("cannedfood", 40,  i,  j, i,  j, false, 0);
                    }
                }
            }
            add_spawn("mon_blank", rng(1, 3), 23, 5);
            if (t_west == "cathedral_b_entrance") {
                rotate(1);
            } else if (t_north == "cathedral_b_entrance") {
                rotate(2);
            } else if (t_east == "cathedral_b_entrance") {
                rotate(3);
            }
        }

        else if ((t_west == "cathedral_b_entrance" && t_north == "cathedral_b") ||
                 (t_north == "cathedral_b_entrance" && t_east == "cathedral_b") || (t_west == "cathedral_b" &&
                         t_south == "cathedral_b_entrance") ||
                 (t_south == "cathedral_b" && t_east == "cathedral_b_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
.|..|+|D|+|#############\n\
.|..|T|T|T|#############\n\
.|--|-|-|-|#############\n\
.|........|#############\n\
.|..htth..|#############\n\
.|..htth..|#############\n\
.+..htth..|#############\n\
.|..htth..|#############\n\
.|........|#############\n\
.|--------|#############\n\
...hhh...^|#############\n\
..........|#############\n\
.........<|#############\n\
cc-|-D----|#############\n\
...|^..dxd|#############\n\
...+....hd|#############\n\
.ll|l.....|#############\n\
---|------|#############\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n",
                                       mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", t_floor,
                                               t_elevator, t_stairs_down, t_stairs_up, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,
                                               t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                               t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                               t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_column, t_floor,
                                               t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", f_oven,
                                               f_null,     f_null,        f_null,      f_null, f_null,              f_null,
                                               f_makeshift_bed, f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,
                                               f_null,           f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,
                                               f_null,                 f_null,               f_toilet, f_sink,  f_fridge, f_null,   f_chair,
                                               f_counter, f_desk,  f_locker, f_null));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_desk) {
                        place_items("office", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_locker) {
                        place_items("office", 70,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_table) {
                        place_items("office", 30,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (density > 1) {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
            } else {
                place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, .20);
            }
            if (t_north == "cathedral_b_entrance") {
                rotate(1);
            } else if (t_east == "cathedral_b_entrance") {
                rotate(2);
            } else if (t_south == "cathedral_b_entrance") {
                rotate(3);
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
########################\n\
########################\n\
____####################\n\
t_o__###################\n\
_____###################\n\
__o__###################\n\
________################\n\
________################\n\
_o___o______############\n\
____________############\n\
____________############\n\
----------|---|#########\n\
..........|htt|#########\n\
C..C..C...|.tt|#########\n\
..........D.tt|#########\n\
C..C..C...D.tt|#########\n\
..........|.tt|#########\n\
C..C..C...|hhh|#########\n\
..........|hhh|#########\n\
..........|-|-|#########\n\
....hh....+<|###########\n\
.|--------|-|###########\n\
.|.S.S.S..|#############\n\
.+........|#############\n",
                                       mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", t_floor,
                                               t_elevator, t_stairs_down, t_stairs_up, t_rock, t_door_metal_locked, t_door_glass_c, t_floor,
                                               t_pavement_y, t_pavement, t_floor, t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken,
                                               t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c, t_door_locked,
                                               t_door_locked_interior, t_window_stained_red, t_floor,  t_floor, t_floor,  t_column, t_floor,
                                               t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("O E > < # X G C , _ r V H 6 x ^ . - | t + = D w T S e o h c d l s", f_oven,
                                               f_null,     f_null,        f_null,      f_null, f_null,              f_null,
                                               f_makeshift_bed, f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,
                                               f_null,           f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,
                                               f_null,                 f_null,               f_toilet, f_sink,  f_fridge, f_null,   f_chair,
                                               f_counter, f_desk,  f_locker, f_null));
            spawn_item(0, 3, "small_relic");
            add_spawn("mon_blank", rng(1, 3), 0, 5);
            if (t_west == "cathedral_b" && t_north == "cathedral_b") {
                rotate(1);
            } else if (t_east == "cathedral_b" && t_north == "cathedral_b") {
                rotate(2);
            } else if (t_east == "cathedral_b" && t_south == "cathedral_b") {
                rotate(3);
            }
        }


    } else if (is_ot_type("s_grocery", terrain_type)) {

        fill_background(this, &grass_or_dirt);
        square(this, t_floor, 3, 3, SEEX * 2 - 4, SEEX * 2 - 4);
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j == 2 && ((i > 4 && i < 8) || (i > 15 && i < 19))) {
                    ter_set(i, j, t_window);
                } else if ((j == 2 && (i == 11 || i == 12)) || (i == 6 && j == 20)) {
                    ter_set(i, j, t_door_c);
                } else if (((j == 2 || j == SEEY * 2 - 3) && i > 1 && i < SEEX * 2 - 2) ||
                           (j == 18 && i > 2 && i < 7)) {
                    ter_set(i, j, t_wall_h);
                } else if (((i == 2 || i == SEEX * 2 - 3) && j > 2 && j < SEEY * 2 - 3) ||
                           (i == 6 && j == 19)) {
                    ter_set(i, j, t_wall_v);
                } else if (j > 4 && j < 8) {
                    if (i == 5 || i == 9 || i == 13 || i == 17) {
                        set(i, j, t_floor, f_counter);
                    } else if (i == 8 || i == 12 || i == 16 || i == 20) {
                        set(i, j, t_floor, f_rack);
                    } else if (i > 2 && i < SEEX * 2 - 3) {
                        ter_set(i, j, t_floor);
                    } else {
                        ter_set(i, j, grass_or_dirt());
                    }
                } else if ((j == 7 && (i == 3 || i == 4)) ||
                           ((j == 11 || j == 14) && (i == 18 || i == 19)) ||
                           ((j > 9 && j < 16) && (i == 6 || i == 7 || i == 10 ||
                                                  i == 11 || i == 14 || i == 15 ||
                                                  i == 20))) {
                    set(i, j, t_floor, f_rack);
                } else if ((j == 18 && i > 15 && i < 21) || (j == 19 && i == 16)) {
                    set(i, j, t_floor, f_counter);
                } else if ((i == 3 && j > 9 && j < 16) ||
                           (j == 20 && ((i > 7 && i < 15) || (i > 18 && i < 21)))) {
                    set(i, j, t_floor, f_glass_fridge);
                } else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < SEEY * 2 - 3) {
                    ter_set(i, j, t_floor);
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
            }
        }

        {
            int num_carts = rng(0, 5);
            for( int i = 0; i < num_carts; i++ ) {
                add_vehicle (g, "shopping_cart", rng(3, 21), rng(3, 21), 90);
            }
        }

        place_items("fridgesnacks", 65,  3, 10,  3, 15, false, 0);
        place_items("fridge", 70,  8, 20, 14, 20, false, 0);
        place_items("fridge", 50, 19, 20, 20, 20, false, 0);
        place_items("softdrugs", 55,  6, 10,  6, 15, false, 0);
        place_items("cleaning", 88,  7, 10,  7, 15, false, 0);
        place_items("kitchen", 75, 10, 10, 10, 15, false, 0);
        place_items("snacks", 78, 11, 10, 11, 15, false, 0);
        place_items("cannedfood", 80, 14, 10, 14, 15, false, 0);
        place_items("pasta",  74, 15, 10, 15, 15, false, 0);
        place_items("produce", 60, 20, 10, 20, 15, false, 0);
        place_items("produce", 50, 18, 11, 19, 11, false, 0);
        place_items("produce", 50, 18, 10, 20, 15, false, 0);
        for (int i = 8; i < 21; i += 4) { // Checkout snacks & magazines
            place_items("snacks",    50, i, 5, i, 6, false, 0);
            place_items("magazines", 70, i, 7, i, 7, false, 0);
        }
        if (terrain_type == "s_grocery_east") {
            rotate(1);
        }
        if (terrain_type == "s_grocery_south") {
            rotate(2);
        }
        if (terrain_type == "s_grocery_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_GROCERY", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_hardware", terrain_type)) {

        fill_background(this, &grass_or_dirt);
        square(this, t_floor, 3, 3, SEEX * 2 - 4, SEEX * 2 - 4);
        rn = 0; // No back door
        //  if (!one_in(3))
        //   rn = 1; // Old-style back door
        if (!one_in(6)) {
            rn = 2;    // Paved back area
        }
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j == 3 && ((i > 5 && i < 9) || (i > 14 && i < 18))) {
                    ter_set(i, j, t_window);
                } else if ((j == 3 && i > 1 && i < SEEX * 2 - 2) ||
                           (j == 15 && i > 1 && i < 14) ||
                           (j == SEEY * 2 - 3 && i > 12 && i < SEEX * 2 - 2)) {
                    ter_set(i, j, t_wall_h);
                } else if ((i == 2 && j > 3 && j < 15) ||
                           (i == SEEX * 2 - 3 && j > 3 && j < SEEY * 2 - 3) ||
                           (i == 13 && j > 15 && j < SEEY * 2 - 3)) {
                    ter_set(i, j, t_wall_v);
                } else if ((i > 3 && i < 10 && j == 6) || (i == 9 && j > 3 && j < 7)) {
                    set(i, j, t_floor, f_counter);
                } else if (((i == 3 || i == 6 || i == 7 || i == 10 || i == 11) &&
                            j > 8 && j < 15) ||
                           (i == SEEX * 2 - 4 && j > 3 && j < SEEX * 2 - 4) ||
                           (i > 14 && i < 18 &&
                            (j == 8 || j == 9 || j == 12 || j == 13)) ||
                           (j == SEEY * 2 - 4 && i > 13 && i < SEEX * 2 - 4) ||
                           (i > 15 && i < 18 && j > 15 && j < 18) ||
                           (i == 9 && j == 7)) {
                    set(i, j, t_floor, f_rack);
                } else if ((i > 2 && i < SEEX * 2 - 3 && j > 3 && j < 15) ||
                           (i > 13 && i < SEEX * 2 - 3 && j > 14 && j < SEEY * 2 - 3)) {
                    ter_set(i, j, t_floor);
                } else if (rn == 2 && i > 1 && i < 13 && j > 15 && j < SEEY * 2 - 3) {
                    ter_set(i, j, t_pavement);
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
            }
        }
        ter_set(rng(10, 13), 3, t_door_c);
        if (rn > 0) {
            ter_set(13, rng(16, 19), (one_in(3) ? t_door_c : t_door_locked));
        }
        if (rn == 2) {
            if (one_in(5)) {
                place_gas_pump(rng(4, 10), 16, rng(500, 5000));
            } else {
                ter_set(rng(4, 10), 16, t_recycler);
            }
            if (one_in(3)) { // Place a dumpster
                int startx = rng(2, 11), starty = rng(18, 19);
                if (startx == 11) {
                    starty = 18;
                }
                bool hori = (starty == 18 ? false : true);
                for (int i = startx; i <= startx + (hori ? 3 : 2); i++) {
                    for (int j = starty; j <= starty + (hori ? 2 : 3); j++) {
                        furn_set(i, j, f_dumpster);
                    }
                }
                if (hori) {
                    place_items("trash", 30, startx, starty, startx + 3, starty + 2, false, 0);
                } else {
                    place_items("trash", 30, startx, starty, startx + 2, starty + 3, false, 0);
                }
            }
            place_items("road", 30, 2, 16, 12, SEEY * 2 - 3, false, 0);
        }

        place_items("magazines", 70,  9,  7,  9,  7, false, 0);
        if (one_in(4)) {
            place_items("snacks", 70,  9,  7,  9,  7, false, 0);
        }

        if (!one_in(3)) {
            place_items("hardware", 80,  3,  9,  3, 14, false, 0);
        } else if (!one_in(3)) {
            place_items("tools", 80,  3,  9,  3, 14, false, 0);
        } else {
            place_items("bigtools", 80,  3,  9,  3, 14, false, 0);
        }

        if (!one_in(3)) {
            place_items("hardware", 80,  6,  9,  6, 14, false, 0);
        } else if (!one_in(3)) {
            place_items("tools", 80,  6,  9,  6, 14, false, 0);
        } else {
            place_items("bigtools", 80,  6,  9,  6, 14, false, 0);
        }

        if (!one_in(4)) {
            place_items("tools", 80,  7,  9,  7, 14, false, 0);
        } else if (one_in(4)) {
            place_items("mischw", 80,  7,  9,  7, 14, false, 0);
        } else {
            place_items("hardware", 80,  7,  9,  7, 14, false, 0);
        }
        if (!one_in(4)) {
            place_items("tools", 80, 10,  9, 10, 14, false, 0);
        } else if (one_in(4)) {
            place_items("mischw", 80, 10,  9, 10, 14, false, 0);
        } else {
            place_items("hardware", 80, 10,  9, 10, 14, false, 0);
        }

        if (!one_in(3)) {
            place_items("bigtools", 75, 11,  9, 11, 14, false, 0);
        } else if (one_in(2)) {
            place_items("cleaning", 75, 11,  9, 11, 14, false, 0);
        } else {
            place_items("tools", 75, 11,  9, 11, 14, false, 0);
        }
        if (one_in(2)) {
            place_items("cleaning", 65, 15,  8, 17,  8, false, 0);
        } else {
            place_items("snacks", 65, 15,  8, 17,  8, false, 0);
        }
        if (one_in(4)) {
            place_items("hardware", 74, 15,  9, 17,  9, false, 0);
        } else {
            place_items("cleaning", 74, 15,  9, 17,  9, false, 0);
        }
        if (one_in(4)) {
            place_items("hardware", 74, 15, 12, 17, 12, false, 0);
        } else {
            place_items("cleaning", 74, 15, 12, 17, 12, false, 0);
        }
        place_items("mischw", 90, 20,  4, 20, 19, false, 0);
        {
            int num_carts = rng(1, 3);
            for( int i = 0; i < num_carts; i++ ) {
                add_vehicle (g, "wheelbarrow", rng(4, 19), rng(3, 11), 90, -1, -1, false);
            }
        }
        if (terrain_type == "s_hardware_east") {
            rotate(1);
        }
        if (terrain_type == "s_hardware_south") {
            rotate(2);
        }
        if (terrain_type == "s_hardware_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_electronics", terrain_type)) {

        fill_background(this, &grass_or_dirt);
        square(this, t_floor, 4, 4, SEEX * 2 - 4, SEEY * 2 - 4);
        line(this, t_wall_v, 3, 4, 3, SEEY * 2 - 4);
        line(this, t_wall_v, SEEX * 2 - 3, 4, SEEX * 2 - 3, SEEY * 2 - 4);
        line(this, t_wall_h, 3, 3, SEEX * 2 - 3, 3);
        line(this, t_wall_h, 3, SEEY * 2 - 3, SEEX * 2 - 3, SEEY * 2 - 3);
        ter_set(13, 3, t_door_c);
        line(this, t_window, 10, 3, 11, 3);
        line(this, t_window, 16, 3, 18, 3);
        line(this, t_window, SEEX * 2 - 3, 9,  SEEX * 2 - 3, 11);
        line(this, t_window, SEEX * 2 - 3, 14,  SEEX * 2 - 3, 16);
        line_furn(this, f_counter, 4, SEEY * 2 - 4, SEEX * 2 - 4, SEEY * 2 - 4);
        line_furn(this, f_counter, 4, SEEY * 2 - 5, 4, SEEY * 2 - 9);
        line_furn(this, f_counter, SEEX * 2 - 4, SEEY * 2 - 5, SEEX * 2 - 4, SEEY * 2 - 9);
        line_furn(this, f_counter, SEEX * 2 - 7, 4, SEEX * 2 - 7, 6);
        line_furn(this, f_counter, SEEX * 2 - 7, 7, SEEX * 2 - 5, 7);
        line_furn(this, f_rack, 9, SEEY * 2 - 5, 9, SEEY * 2 - 9);
        line_furn(this, f_rack, SEEX * 2 - 9, SEEY * 2 - 5, SEEX * 2 - 9, SEEY * 2 - 9);
        line_furn(this, f_rack, 4, 4, 4, SEEY * 2 - 10);
        line_furn(this, f_rack, 5, 4, 8, 4);
        place_items("consumer_electronics", 85, 4, SEEY * 2 - 4, SEEX * 2 - 4,
                    SEEY * 2 - 4, false, turn - 50);
        place_items("consumer_electronics", 85, 4, SEEY * 2 - 5, 4, SEEY * 2 - 9,
                    false, turn - 50);
        place_items("consumer_electronics", 85, SEEX * 2 - 4, SEEY * 2 - 5,
                    SEEX * 2 - 4, SEEY * 2 - 9, false, turn - 50);
        place_items("consumer_electronics", 85, 9, SEEY * 2 - 5, 9, SEEY * 2 - 9,
                    false, turn - 50);
        place_items("consumer_electronics", 85, SEEX * 2 - 9, SEEY * 2 - 5,
                    SEEX * 2 - 9, SEEY * 2 - 9, false, turn - 50);
        place_items("consumer_electronics", 85, 4, 4, 4, SEEY * 2 - 10, false,
                    turn - 50);
        place_items("consumer_electronics", 85, 5, 4, 8, 4, false, turn - 50);
        if (terrain_type == "s_electronics_east") {
            rotate(1);
        }
        if (terrain_type == "s_electronics_south") {
            rotate(2);
        }
        if (terrain_type == "s_electronics_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_ELECTRO", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_sports", terrain_type)) {

        lw = rng(0, 3);
        rw = SEEX * 2 - 1 - rng(0, 3);
        tw = rng(3, 10);
        bw = SEEY * 2 - 1 - rng(0, 3);
        cw = bw - rng(3, 5);
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (((j == tw || j == bw) && i >= lw && i <= rw) ||
                    (j == cw && i > lw && i < rw)) {
                    ter_set(i, j, t_wall_h);
                } else if ((i == lw || i == rw) && j > tw && j < bw) {
                    ter_set(i, j, t_wall_v);
                } else if ((j == cw - 1 && i > lw && i < rw - 4) ||
                           (j < cw - 3 && j > tw && (i == lw + 1 || i == rw - 1))) {
                    set(i, j, t_floor, f_rack);
                } else if (j == cw - 3 && i > lw && i < rw - 4) {
                    set(i, j, t_floor, f_counter);
                } else if (j > tw && j < bw && i > lw && i < rw) {
                    ter_set(i, j, t_floor);
                } else if (tw >= 6 && j >= tw - 6 && j < tw && i >= lw && i <= rw) {
                    if ((i - lw) % 4 == 0) {
                        ter_set(i, j, t_pavement_y);
                    } else {
                        ter_set(i, j, t_pavement);
                    }
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
            }
        }
        rn = rng(tw + 2, cw - 6);
        for (int i = lw + 3; i <= rw - 5; i += 4) {
            if (cw - 6 > tw + 1) {
                furn_set(i    , rn + 1, f_rack);
                furn_set(i    , rn    , f_rack);
                furn_set(i + 1, rn + 1, f_rack);
                furn_set(i + 1, rn    , f_rack);
                place_items("camping", 86, i, rn, i + 1, rn + 1, false, 0);
            } else if (cw - 5 > tw + 1) {
                furn_set(i    , cw - 5, f_rack);
                furn_set(i + 1, cw - 5, f_rack);
                place_items("camping", 80, i, cw - 5, i + 1, cw - 5, false, 0);
            }
        }
        ter_set(rw - rng(2, 3), cw, t_door_c);
        rn = rng(2, 4);
        for (int i = lw + 2; i <= lw + 2 + rn; i++) {
            ter_set(i, tw, t_window);
        }
        for (int i = rw - 2; i >= rw - 2 - rn; i--) {
            ter_set(i, tw, t_window);
        }
        ter_set(rng(lw + 3 + rn, rw - 3 - rn), tw, t_door_c);
        if (one_in(4)) {
            ter_set(rng(lw + 2, rw - 2), bw, t_door_locked);
        }
        place_items("allsporting", 90, lw + 1, cw - 1, rw - 5, cw - 1, false, 0);
        place_items("sports", 82, lw + 1, tw + 1, lw + 1, cw - 4, false, 0);
        place_items("sports", 82, rw - 1, tw + 1, rw - 1, cw - 4, false, 0);
        if (!one_in(4)) {
            place_items("allsporting", 92, lw + 1, cw + 1, rw - 1, bw - 1, false, 0);
        }

        if (terrain_type == "s_sports_east") {
            rotate(1);
        }
        if (terrain_type == "s_sports_south") {
            rotate(2);
        }
        if (terrain_type == "s_sports_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_liquor", terrain_type)) {

        fill_background(this, &grass_or_dirt);
        square(this, t_pavement, 3, 13, SEEX * 2 - 4, SEEY * 2 - 1);
        square(this, t_floor, 3, 2, SEEX * 2 - 4, 12);
        mapf::formatted_set_simple(this, 3, 2,
                                   "\
--:------------:--\n\
|#  #####    c   |\n\
|#  ##       c   |\n\
|#  ##  ##   ccc |\n\
|#  ##  ##       |\n\
|#  ##          &|\n\
|       #####   &|\n\
|----   #####   &|\n\
|   |           &|\n\
|   |         &&&|\n\
------------------\n",
                                   mapf::basic_bind("- | :", t_wall_h, t_wall_v, t_window),
                                   mapf::basic_bind("# c &", f_rack, f_counter, f_glass_fridge));
        square_furn(this, f_dumpster, 5, 13, 7, 14);
        square_furn(this, f_dumpster, SEEX * 2 - 6, 15, SEEX * 2 - 5, 17);

        ter_set(rng(13, 15), 2, t_door_c);
        ter_set(rng(4, 6), 9, t_door_c);
        ter_set(rng(9, 16), 12, t_door_c);

        place_items("alcohol", 96,  4,  3,  4,  7, false, 0);
        place_items("alcohol", 96,  7,  3, 11,  3, false, 0);
        place_items("alcohol", 96,  7,  4,  8,  7, false, 0);
        place_items("alcohol", 96, 11,  8, 15,  9, false, 0);
        place_items("snacks", 85, 11,  5, 12,  6, false, 0);
        place_items("fridgesnacks", 90, 19,  7, 19, 10, false, 0);
        place_items("fridgesnacks", 90, 17, 11, 19, 11, false, 0);
        place_items("behindcounter", 80, 17,  3, 19,  4, false, 0);
        place_items("trash",  30,  5, 14,  7, 14, false, 0);
        place_items("trash",  30, 18, 15, 18, 17, false, 0);

        {
            int num_carts = rng(0, 3);
            for( int i = 0; i < num_carts; i++ ) {
                add_vehicle (g, "shopping_cart", rng(4, 19), rng(3, 11), 90);
            }
        }

        if (terrain_type == "s_liquor_east") {
            rotate(1);
        }
        if (terrain_type == "s_liquor_south") {
            rotate(2);
        }
        if (terrain_type == "s_liquor_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_gun", terrain_type)) {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i == 2 || i == SEEX * 2 - 3) && j > 6 && j < SEEY * 2 - 1) {
                    ter_set(i, j, t_wall_v);
                } else if ((i == 8 && j > 6 && j < 13) ||
                           (j == 16 && (i == 5 || i == 8 || i == 11 || i == 14 || i == 17))) {
                    set(i, j, t_floor, f_counter);
                } else if ((j == 6 && ((i > 4 && i < 8) || (i > 15 && i < 19)))) {
                    ter_set(i, j, t_window);
                } else if ((j == 14 && i > 3 && i < 15)) {
                    ter_set(i, j, t_wall_glass_h);
                } else if (j == 16 && i == SEEX * 2 - 4) {
                    ter_set(i, j, t_door_c);
                } else if (((j == 6 || j == SEEY * 2 - 1) && i > 1 && i < SEEX * 2 - 2) ||
                           ((j == 16 || j == 14) && i > 2 && i < SEEX * 2 - 3)) {
                    ter_set(i, j, t_wall_h);
                } else if (((i == 3 || i == SEEX * 2 - 4) && j > 6 && j < 14) ||
                           ((j > 8 && j < 12) && (i == 12 || i == 13 || i == 16)) ||
                           (j == 13 && i > 15 && i < SEEX * 2 - 4)) {
                    set(i, j, t_floor, f_rack);
                } else if (i > 2 && i < SEEX * 2 - 3 && j > 6 && j < SEEY * 2 - 1) {
                    ter_set(i, j, t_floor);
                } else if ((j > 0 && j < 6 &&
                            (i == 2 || i == 6 || i == 10 || i == 17 || i == SEEX * 2 - 3))) {
                    ter_set(i, j, t_pavement_y);
                } else if (j < 6 && i > 1 && i < SEEX * 2 - 2) {
                    ter_set(i, j, t_pavement);
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
            }
        }
        ter_set(rng(11, 14), 6, t_door_c);
        ter_set(rng(5, 14), 14, t_door_c);
        place_items("pistols", 70, 12,  9, 13, 11, false, 0);
        place_items("shotguns", 60, 16,  9, 16, 11, false, 0);
        place_items("rifles", 80, 20,  7, 20, 12, false, 0);
        place_items("smg",  25,  3,  7,  3,  8, false, 0);
        place_items("assault", 18,  3,  9,  3, 10, false, 0);
        place_items("ammo",  93,  3, 11,  3, 13, false, 0);
        place_items("allguns", 12,  5, 16, 17, 16, false, 0);
        place_items("gunxtras", 67, 16, 13, 19, 13, false, 0);
        if (terrain_type == "s_gun_east") {
            rotate(1);
        }
        if (terrain_type == "s_gun_south") {
            rotate(2);
        }
        if (terrain_type == "s_gun_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_clothes", terrain_type)) {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j == 2 && (i == 11 || i == 12)) {
                    ter_set(i, j, t_door_glass_c);
                } else if (j == 2 && i > 3 && i < SEEX * 2 - 4) {
                    ter_set(i, j, t_wall_glass_h);
                } else if (((j == 2 || j == SEEY * 2 - 2) && i > 1 && i < SEEX * 2 - 2) ||
                           (j == 4 && i > 12 && i < SEEX * 2 - 3) ||
                           (j == 17 && i > 2 && i < 12) ||
                           (j == 20 && i > 2 && i < 11)) {
                    ter_set(i, j, t_wall_h);
                } else if (((i == 2 || i == SEEX * 2 - 3) && j > 1 && j < SEEY * 2 - 1) ||
                           (i == 11 && (j == 18 || j == 20 || j == 21)) ||
                           (j == 21 && (i == 5 || i == 8))) {
                    ter_set(i, j, t_wall_v);
                } else if ((i == 16 && j > 4 && j < 9) ||
                           (j == 8 && (i == 17 || i == 18)) ||
                           (j == 18 && i > 2 && i < 11)) {
                    set(i, j, t_floor, f_counter);
                } else if ((i == 3 && j > 4 && j < 13) ||
                           (i == SEEX * 2 - 4 && j > 9 && j < 20) ||
                           ((j == 10 || j == 11) && i > 6 && i < 13) ||
                           ((j == 14 || j == 15) && i > 4 && i < 13) ||
                           ((i == 15 || i == 16) && j > 10 && j < 18) ||
                           (j == SEEY * 2 - 3 && i > 11 && i < 18)) {
                    set(i, j, t_floor, f_rack);
                } else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < SEEY * 2 - 2) {
                    ter_set(i, j, t_floor);
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
            }
        }

        for (int i = 3; i <= 9; i += 3) {
            if (one_in(2)) {
                ter_set(i, SEEY * 2 - 4, t_door_c);
            } else {
                ter_set(i + 1, SEEY * 2 - 4, t_door_c);
            }
        }

        {
            int num_carts = rng(0, 5);
            for( int i = 0; i < num_carts; i++ ) {
                add_vehicle (g, "shopping_cart", rng(3, 16), rng(3, 21), 90);
            }
        }

        place_items("shoes",  70,  7, 10, 12, 10, false, 0);
        place_items("pants",  88,  5, 14, 12, 14, false, 0);
        place_items("shirts", 88,  7, 11, 12, 11, false, 0);
        place_items("jackets", 80,  3,  5,  3, 12, false, 0);
        place_items("winter", 60,  5, 15, 12, 15, false, 0);
        place_items("bags",  70, 15, 11, 15, 17, false, 0);
        place_items("dresser", 50, 12, 21, 17, 21, false, 0);
        place_items("allclothes", 20,  3, 21, 10, 21, false, 0);
        place_items("allclothes", 20,  3, 18, 10, 18, false, 0);
        switch (rng(0, 2)) {
        case 0:
            place_items("pants", 70, 16, 11, 16, 17, false, 0);
            break;
        case 1:
            place_items("shirts", 70, 16, 11, 16, 17, false, 0);
            break;
        case 2:
            place_items("bags", 70, 16, 11, 16, 17, false, 0);
            break;
        }
        switch (rng(0, 2)) {
        case 0:
            place_items("pants", 75, 20, 10, 20, 19, false, 0);
            break;
        case 1:
            place_items("shirts", 75, 20, 10, 20, 19, false, 0);
            break;
        case 2:
            place_items("jackets", 75, 20, 10, 20, 19, false, 0);
            break;
        }

        if (terrain_type == "s_clothes_east") {
            rotate(1);
        }
        if (terrain_type == "s_clothes_south") {
            rotate(2);
        }
        if (terrain_type == "s_clothes_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_library", terrain_type)) {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j == 2) {
                    if (i == 5 || i == 6 || i == 17 || i == 18) {
                        ter_set(i, j, t_window_domestic);
                    } else if (i == 11 || i == 12) {
                        ter_set(i, j, t_door_c);
                    } else if (i > 1 && i < SEEX * 2 - 2) {
                        ter_set(i, j, t_wall_h);
                    } else {
                        ter_set(i, j, grass_or_dirt());
                    }
                } else if (j == 17 && i > 1 && i < SEEX * 2 - 2) {
                    ter_set(i, j, t_wall_h);
                } else if (i == 2) {
                    if (j == 6 || j == 7 || j == 10 || j == 11 || j == 14 || j == 15) {
                        ter_set(i, j, t_window_domestic);
                    } else if (j > 1 && j < 17) {
                        ter_set(i, j, t_wall_v);
                    } else {
                        ter_set(i, j, grass_or_dirt());
                    }
                } else if (i == SEEX * 2 - 3) {
                    if (j == 6 || j == 7) {
                        ter_set(i, j, t_window_domestic);
                    } else if (j > 1 && j < 17) {
                        ter_set(i, j, t_wall_v);
                    } else {
                        ter_set(i, j, grass_or_dirt());
                    }
                } else if (((j == 4 || j == 5) && i > 2 && i < 10) ||
                           ((j == 8 || j == 9 || j == 12 || j == 13 || j == 16) &&
                            i > 2 && i < 16) || (i == 20 && j > 7 && j < 17)) {
                    set(i, j, t_floor, f_bookcase);
                } else if ((i == 14 && j < 6 && j > 2) || (j == 5 && i > 14 && i < 19)) {
                    set(i, j, t_floor, f_counter);
                } else if (i > 2 && i < SEEX * 2 - 3 && j > 2 && j < 17) {
                    ter_set(i, j, t_floor);
                } else {
                    ter_set(i, j, grass_or_dirt());
                }
            }
        }
        if (!one_in(3)) {
            ter_set(18, 17, t_door_c);
        }
        place_items("magazines",  70,  3,  4,  9,  4, false, 0);
        place_items("magazines", 70, 20,  8, 20, 16, false, 0);
        place_items("novels",  96,  3,  5,  9,  5, false, 0);
        place_items("novels", 96,  3,  8, 15,  9, false, 0);
        place_items("manuals", 92,  3, 12, 15, 13, false, 0);
        place_items("textbooks", 88,  3, 16, 15, 16, false, 0);
        if (terrain_type == "s_library_east") {
            rotate(1);
        }
        if (terrain_type == "s_library_south") {
            rotate(2);
        }
        if (terrain_type == "s_library_west") {
            rotate(3);
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (terrain_type == "s_restaurant_north" ||
               terrain_type == "s_restaurant_east" ||
               terrain_type == "s_restaurant_south" ||
               terrain_type == "s_restaurant_west") {

        // Init to grass/dirt
        fill_background(this, &grass_or_dirt);
        ter_id doortype = (one_in(4) ? t_door_c : t_door_glass_c);
        lw = rng(0, 4);
        rw = rng(19, 23);
        tw = rng(0, 4);
        bw = rng(17, 23);
        // Fill in with floor
        square(this, t_floor, lw + 1, tw + 1, rw - 1, bw - 1);
        // Draw the walls
        line(this, t_wall_h, lw, tw, rw, tw);
        line(this, t_wall_h, lw, bw, rw, bw);
        line(this, t_wall_v, lw, tw + 1, lw, bw - 1);
        line(this, t_wall_v, rw, tw + 1, rw, bw - 1);

        // What's the front wall look like?
        switch (rng(1, 3)) {
        case 1: // Door to one side
        case 2:
            // Mirror it?
            if (one_in(2)) {
                ter_set(lw + 2, tw, doortype);
            } else {
                ter_set(rw - 2, tw, doortype);
            }
            break;
        case 3: // Double-door in center
            line(this, doortype, (lw + rw) / 2, tw, 1 + ((lw + rw) / 2), tw);
            break;
        }
        // What type of windows?
        switch (rng(1, 6)) {
        case 1: // None!
            break;
        case 2:
        case 3: // Glass walls everywhere
            for (int i = lw + 1; i <= rw - 1; i++) {
                if (ter(i, tw) == t_wall_h) {
                    ter_set(i, tw, t_wall_glass_h);
                }
            }
            while (!one_in(3)) { // 2 in 3 chance of having some walls too
                rn = rng(1, 3);
                if (ter(lw + rn, tw) == t_wall_glass_h) {
                    ter_set(lw + rn, tw, t_wall_h);
                }
                if (ter(rw - rn, tw) == t_wall_glass_h) {
                    ter_set(rw - rn, tw, t_wall_h);
                }
            }
            break;
        case 4:
        case 5:
        case 6: { // Just some windows
            rn = rng(1, 3);
            int win_width = rng(1, 3);
            for (int i = rn; i <= rn + win_width; i++) {
                if (ter(lw + i, tw) == t_wall_h) {
                    ter_set(lw + i, tw, t_window);
                }
                if (ter(rw - i, tw) == t_wall_h) {
                    ter_set(rw - i, tw, t_window);
                }
            }
        }
        break;
        } // Done building windows
        // Build a kitchen
        mw = rng(bw - 6, bw - 3);
        cw = (one_in(3) ? rw - 3 : rw - 1); // 1 in 3 chance for corridor to back
        line(this, t_wall_h, lw + 1, mw, cw, mw);
        line(this, t_wall_v, cw, mw + 1, cw, bw - 1);
        furn_set(lw + 1, mw + 1, f_fridge);
        furn_set(lw + 2, mw + 1, f_fridge);
        place_items("fridge", 80, lw + 1, mw + 1, lw + 2, mw + 1, false, 0);
        line_furn(this, f_counter, lw + 3, mw + 1, cw - 1, mw + 1);
        place_items("kitchen", 70, lw + 3, mw + 1, cw - 1, mw + 1, false, 0);
        // Place a door to the kitchen
        if (cw != rw - 1 && one_in(2)) { // side door
            ter_set(cw, rng(mw + 2, bw - 1), t_door_c);
        } else { // north-facing door
            rn = rng(lw + 4, cw - 2);
            // Clear the counters around the door
            line(this, t_floor, rn - 1, mw + 1, rn + 1, mw + 1);
            ter_set(rn, mw, t_door_c);
        }
        // Back door?
        if (bw <= 19 || one_in(3)) {
            // If we have a corridor, put it over there
            if (cw == rw - 3) {
                // One in two chance of a double-door
                if (one_in(2)) {
                    line(this, t_door_locked, cw + 1, bw, rw - 1, bw);
                } else {
                    ter_set( rng(cw + 1, rw - 1), bw, t_door_locked);
                }
            } else { // No corridor
                ter_set( rng(lw + 1, rw - 1), bw, t_door_locked);
            }
        }
        // Build a dining area
        int table_spacing = rng(2, 4);
        for (int i = lw + table_spacing + 1; i <= rw - 2 - table_spacing;
             i += table_spacing + 2) {
            for (int j = tw + table_spacing + 1; j <= mw - 1 - table_spacing;
                 j += table_spacing + 2) {
                square_furn(this, f_table, i, j, i + 1, j + 1);
                place_items("dining", 70, i, j, i + 1, j + 1, false, 0);
            }
        }
        // Dumpster out back?
        if (rng(18, 21) > bw) {
            square(this, t_pavement, lw, bw + 1, rw, 24);
            rn = rng(lw + 1, rw - 4);
            square_furn(this, f_dumpster, rn, 22, rn + 2, 23);
            place_items("trash",  40, rn, 22, rn + 3, 23, false, 0);
            place_items("fridge", 50, rn, 22, rn + 3, 23, false, 0);
        }

        if (terrain_type == "s_restaurant_east") {
            rotate(1);
        }
        if (terrain_type == "s_restaurant_south") {
            rotate(2);
        }
        if (terrain_type == "s_restaurant_west") {
            rotate(3);
        }

        place_spawns(g, "GROUP_GROCERY", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("s_restaurant_fast", terrain_type)) {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
   dd                   \n\
  ,,,,,,,,,,,,,,,,,,,,  \n\
 ,_______,____________, \n\
,________,_____________,\n\
,________,_____________,\n\
,________,_____________,\n\
,_____#|----w-|-|#_____,\n\
,_____||erle.x|T||_____,\n\
,_____|O.....S|.S|_____,\n\
,_____|e.ccl.c|+-|_____,\n\
,_____w.......+..|_____,\n\
,_____|ccccxcc|..%_____,\n\
,,,,,,|..........+_____,\n\
,_____|..........%_____,\n\
,_____%.hh....hh.|_____,\n\
,_____%.tt....tt.%_____,\n\
,_____%.tt....tt.%_____,\n\
,_____|.hh....hh.%_____,\n\
,__,__|-555++555-|__,__,\n\
,__,__#ssssssssss#_,,,_,\n\
,__,__#hthsssshth#__,__,\n\
,_,,,_#ssssssssss#__,__,\n\
,__,__#### ss ####__,__,\n\
,_____ssssssssssss_____,\n",
                                   mapf::basic_bind("d 5 % O , _ r 6 x $ ^ . - | # t + = D w T S e h c l s", t_floor,
                                           t_wall_glass_h, t_wall_glass_v, t_floor, t_pavement_y, t_pavement, t_floor, t_console,
                                           t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_shrub, t_floor,
                                           t_door_glass_c, t_door_locked_alarm, t_door_locked, t_window_domestic, t_floor,  t_floor, t_floor,
                                           t_floor, t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("d 5 % O , _ r 6 x $ ^ . - | # t + = D w T S e h c l s", f_dumpster, f_null,
                                           f_null,         f_oven,  f_null,       f_null,     f_rack,  f_null,    f_null,           f_null,
                                           f_indoor_plant, f_null,  f_null,   f_null,   f_null,  f_table, f_null,         f_null,
                                           f_null,        f_null,            f_toilet, f_sink,  f_fridge, f_chair, f_counter, f_locker,
                                           f_null));
        place_items("fast_food", 80,  8,  7, 8,  7, false, 0);
        place_items("fast_food", 70,  7,  9, 7,  9, false, 0);
        place_items("fast_food", 60,  11,  7, 11,  7, false, 0);
        if (terrain_type == "s_restaurant_fast_east") {
            rotate(3);
        }
        if (terrain_type == "s_restaurant_fast_north") {
            rotate(2);
        }
        if (terrain_type == "s_restaurant_fast_west") {
            rotate(1);
        }


    } else if (is_ot_type("s_restaurant_coffee", terrain_type)) {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
|--------=------|--|--| \n\
|ltSrrrrr..eOSll|T.|.T| \n\
|...............|S.|.S| \n\
|cccxccxccc..ccc|-D|D-| \n\
|.....................| \n\
|.....................| \n\
|hh.......hth.hth.hth.| \n\
|tt....|--555-555-555-| \n\
|tt....%ssssssssssssssss\n\
|hh....+ss______________\n\
|......+ss______________\n\
|......%ss______________\n\
|.htth.|ss______________\n\
|-5555-|ss,,,,,,,_______\n\
#ssssss#ss______________\n\
#shtths#ss______________\n\
#sssssssss______________\n\
#shsshssss______________\n\
#stsstssss,,,,,,,_______\n\
#shsshssss______________\n\
#sssssssss______________\n\
#shtthssss______________\n\
#ssssss#ss______________\n\
########ss,,,,,,,_______\n",
                                   mapf::basic_bind("d 5 % O , _ r 6 x $ ^ . - | # t + = D w T S e h c l s", t_floor,
                                           t_wall_glass_h, t_wall_glass_v, t_floor, t_pavement_y, t_pavement, t_floor, t_console,
                                           t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_shrub, t_floor,
                                           t_door_glass_c, t_door_locked_alarm, t_door_locked, t_window_domestic, t_floor,  t_floor, t_floor,
                                           t_floor, t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("d 5 % O , _ r 6 x $ ^ . - | # t + = D w T S e h c l s", f_dumpster, f_null,
                                           f_null,         f_oven,  f_null,       f_null,     f_rack,  f_null,    f_null,           f_null,
                                           f_indoor_plant, f_null,  f_null,   f_null,   f_null,  f_table, f_null,         f_null,
                                           f_null,        f_null,            f_toilet, f_sink,  f_fridge, f_chair, f_counter, f_locker,
                                           f_null));
        place_items("coffee_shop", 85,  4,  1, 8,  1, false, 0);
        place_items("coffee_shop", 85,  11,  1, 11,  1, false, 0);
        place_items("cleaning", 60,  14,  1, 15,  1, false, 0);
        if (terrain_type == "s_restaurant_coffee_east") {
            rotate(3);
        }
        if (terrain_type == "s_restaurant_coffee_north") {
            rotate(2);
        }
        if (terrain_type == "s_restaurant_coffee_west") {
            rotate(1);
        }

    } else if (terrain_type == "shelter") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        square(this, t_floor, 5, 5, SEEX * 2 - 6, SEEY * 2 - 6);
        mapf::formatted_set_simple(this, 4, 4,
                                   "\
|----:-++-:----|\n\
|llll      c  6|\n\
| b b b    c   |\n\
| b b b    c   |\n\
| b b b    c   |\n\
: b b b        :\n\
|              |\n\
+      >>      +\n\
+      >>      +\n\
|              |\n\
: b b b        :\n\
| b b b    c   |\n\
| b b b    c   |\n\
| b b b    c   |\n\
|          c  x|\n\
|----:-++-:----|\n",
                                   mapf::basic_bind("- | + : 6 x >", t_wall_h, t_wall_v, t_door_c, t_window_domestic,  t_console,
                                           t_console_broken, t_stairs_down),
                                   mapf::basic_bind("b c l", f_bench, f_counter, f_locker));
        tmpcomp = add_computer(SEEX + 6, 5, _("Evac shelter computer"), 0);
        tmpcomp->add_option(_("Emergency Message"), COMPACT_EMERG_MESS, 0);
        if(ACTIVE_WORLD_OPTIONS["BLACK_ROAD"]) {
            //place zombies outside
            place_spawns(g, "GROUP_ZOMBIE", ACTIVE_WORLD_OPTIONS["SPAWN_DENSITY"], 0, 0, SEEX * 2 - 1, 3, 0.4f);
            place_spawns(g, "GROUP_ZOMBIE", ACTIVE_WORLD_OPTIONS["SPAWN_DENSITY"], 0, 4, 3, SEEX * 2 - 4, 0.4f);
            place_spawns(g, "GROUP_ZOMBIE", ACTIVE_WORLD_OPTIONS["SPAWN_DENSITY"], SEEX * 2 - 3, 4,
                         SEEX * 2 - 1, SEEX * 2 - 4, 0.4f);
            place_spawns(g, "GROUP_ZOMBIE", ACTIVE_WORLD_OPTIONS["SPAWN_DENSITY"], 0, SEEX * 2 - 3,
                         SEEX * 2 - 1, SEEX * 2 - 1, 0.4f);
        }


    } else if (terrain_type == "shelter_under") {

        // Make the whole area rock, then plop an open area in the center.
        square(this, t_rock, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);
        square(this, t_rock_floor, 6, 6, SEEX * 2 - 8, SEEY * 2 - 8);
        // Create an anteroom with hte stairs and some locked doors.
        ter_set(SEEX - 1, SEEY * 2 - 7, t_door_locked);
        ter_set(SEEX    , SEEY * 2 - 7, t_door_locked);
        ter_set(SEEX - 1, SEEY * 2 - 6, t_rock_floor);
        ter_set(SEEX    , SEEY * 2 - 6, t_rock_floor);
        ter_set(SEEX - 1, SEEY * 2 - 5, t_stairs_up);
        ter_set(SEEX    , SEEY * 2 - 5, t_stairs_up);
        if( one_in(10) ) {
            // Scatter around lots of items and some zombies.
            for( int x = 0; x < 10; ++x ) {
                place_items("shelter", 90, 6, 6, SEEX * 2 - 8, SEEY * 2 - 8, false, 0);
            }
            place_spawns(g, "GROUP_ZOMBIE", 1, 6, 6, SEEX * 2 - 8, SEEX * 2 - 8, 0.2);
        } else {
            // Scatter around some items.
            place_items("shelter", 80, 6, 6, SEEX * 2 - 8, SEEY * 2 - 8, false, 0);
        }


    } else if (terrain_type == "lmoe") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        square(this, t_shrub, 7, 6, 16, 12);
        square(this, t_rock, 10, 9, 13, 12);
        square(this, t_rock_floor, 11, 10, 12, 11);
        line(this, t_stairs_down, 11, 10, 12, 10);
        ter_set(11, 12, t_door_metal_c);
        line(this, t_tree, 9, 8, 14, 8);
        line(this, t_tree, 9, 8, 9, 12);
        line(this, t_tree, 14, 8, 14, 12);
        square(this, t_shrub, 13, 13, 15, 14);
        square(this, t_shrub, 8, 13, 10, 14);
        ter_set(10, 6, t_tree_young);
        ter_set(14, 6, t_tree_young);
        line(this, t_tree_young, 9, 7, 10, 7);
        ter_set(12, 7, t_tree_young);
        ter_set(14, 7, t_tree_young);
        ter_set(8, 9, t_tree_young);
        line(this, t_tree_young, 7, 11, 8, 11);
        line(this, t_tree_young, 15, 10, 15, 11);
        ter_set(16, 12, t_tree_young);
        ter_set(9, 13, t_tree_young);
        ter_set(12, 13, t_tree_young);
        ter_set(16, 12, t_tree_young);
        line(this, t_tree_young, 14, 13, 15, 13);
        ter_set(10, 14, t_tree_young);
        ter_set(13, 14, t_tree_young);


    } else if (terrain_type == "lmoe_under") {

        fill_background(this, t_rock);
        square(this, t_rock_floor, 3, 3, 20, 20);
        line(this, t_stairs_up, 11, 20, 12, 20);
        line(this, t_wall_metal_h, 3, 12, 20, 12);
        line(this, t_wall_metal_v, 10, 12, 10, 20);
        line(this, t_wall_metal_v, 13, 12, 13, 20);
        line(this, t_chainfence_v, 7, 3, 7, 6);
        line(this, t_chainfence_h, 3, 6, 6, 6);
        line(this, t_wall_metal_v, 15, 3, 15, 10);
        line(this, t_wall_metal_h, 15, 9, 20, 9);
        line(this, t_wall_metal_v, 17, 10, 17, 11);
        ter_set(10, 16, t_door_metal_c);
        ter_set(13, 16, t_door_metal_c);
        ter_set(5, 6, t_chaingate_c);
        line(this, t_door_metal_c, 11, 12, 12, 12);
        ter_set(17, 11, t_door_metal_c);
        ter_set(15, 6, t_door_metal_c);
        square(this, t_rubble, 18, 18, 20, 20);
        line(this, t_rubble, 16, 20, 20, 16);
        line(this, t_rubble, 17, 20, 20, 17);
        line(this, t_water_sh, 15, 20, 20, 15);
        //square(this, t_emergency_light_flicker, 11, 13, 12, 19);
        furn_set(17, 16, f_woodstove);
        furn_set(14, 13, f_chair);
        furn_set(14, 18, f_chair);
        square_furn(this, f_crate_c, 18, 13, 20, 14);
        line_furn(this, f_crate_c, 17, 13, 19, 15);
        line_furn(this, f_counter, 3, 13, 3, 20);
        line_furn(this, f_counter, 3, 20, 9, 20);
        line_furn(this, f_bookcase, 5, 13, 8, 13);
        square_furn(this, f_table, 5, 15, 6, 17);
        furn_set(7, 16, f_chair);
        line_furn(this, f_rack, 3, 11, 7, 11);
        line_furn(this, f_rack, 3, 9, 7, 9);
        line_furn(this, f_rack, 3, 3, 6, 3);
        ter_set(10, 7, t_column);
        ter_set(13, 7, t_column);
        line_furn(this, f_bookcase, 16, 3, 16, 5);
        square_furn(this, f_bed, 19, 3, 20, 4);
        furn_set(19, 7, f_chair);
        furn_set(20, 7, f_desk);
        line(this, t_rubble, 15, 10, 16, 10);
        furn_set(19, 10, f_sink);
        place_toilet(20, 11);
        place_items("allguns", 80, 3, 3, 6, 3, false, 0);
        place_items("ammo", 80, 3, 3, 6, 3, false, 0);
        place_items("cannedfood", 90, 3, 9, 7, 9, false, 0);
        place_items("survival_tools", 80, 3, 11, 7, 11, false, 0);
        place_items("bags", 50, 3, 11, 7, 11, false, 0);
        place_items("softdrugs", 50, 3, 11, 7, 11, false, 0);
        place_items("manuals", 60, 5, 13, 8, 13, false, 0);
        place_items("textbooks", 60, 5, 13, 8, 13, false, 0);
        place_items("tools", 90, 5, 15, 6, 17, false, 0);
        place_items("hardware", 70, 3, 13, 3, 20, false, 0);
        place_items("stash_wood", 70, 3, 20, 9, 20, false, 0);
        place_items("shelter", 70, 18, 13, 20, 14, false, 0);
        place_items("novels", 70, 16, 3, 16, 5, false, 0);
        place_items("office", 50, 20, 7, 20, 7, false, 0);


    } else if (terrain_type == "lab" ||
               terrain_type == "lab_stairs" ||
               terrain_type == "lab_core" ||
               terrain_type == "ice_lab" ||
               terrain_type == "ice_lab_stairs" ||
               terrain_type == "ice_lab_core") {

        if (is_ot_type("ice_lab", terrain_type)) {
            ice_lab = true;
        } else {
            ice_lab = false;
        }

        if (ice_lab) {
            int temperature = -20 + 30 * (zlevel);
            set_temperature(x, y, temperature);
        }

        // Check for adjacent sewers; used below
        tw = 0;
        rw = 0;
        bw = 0;
        lw = 0;
        if (is_ot_type("sewer", t_north) && connects_to(t_north, 2)) {
            tw = SEEY * 2;
        }
        if (is_ot_type("sewer", t_east) && connects_to(t_east, 3)) {
            rw = SEEX * 2;
        }
        if (is_ot_type("sewer", t_south) && connects_to(t_south, 0)) {
            bw = SEEY * 2;
        }
        if (is_ot_type("sewer", t_west) && connects_to(t_west, 1)) {
            lw = SEEX * 2;
        }
        if (t_above == "") { // We're on ground level
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (i <= 1 || i >= SEEX * 2 - 2 ||
                        (j > 1 && j < SEEY * 2 - 2 && (i == SEEX - 2 || i == SEEX + 1))) {
                        ter_set(i, j, t_wall_v);
                    } else if (j <= 1 || j >= SEEY * 2 - 2) {
                        ter_set(i, j, t_wall_h);
                    } else {
                        ter_set(i, j, t_floor);
                    }
                }
            }
            ter_set(SEEX - 1, 0, t_dirt);
            ter_set(SEEX - 1, 1, t_door_metal_locked);
            ter_set(SEEX    , 0, t_dirt);
            ter_set(SEEX    , 1, t_door_metal_locked);
            ter_set(SEEX - 2 + rng(0, 1) * 4, 0, t_card_science);
            ter_set(SEEX - 2, SEEY    , t_door_metal_c);
            ter_set(SEEX + 1, SEEY    , t_door_metal_c);
            ter_set(SEEX - 2, SEEY - 1, t_door_metal_c);
            ter_set(SEEX + 1, SEEY - 1, t_door_metal_c);
            ter_set(SEEX - 1, SEEY * 2 - 3, t_stairs_down);
            ter_set(SEEX    , SEEY * 2 - 3, t_stairs_down);
            science_room(this, 2       , 2, SEEX - 3    , SEEY * 2 - 3, 1);
            science_room(this, SEEX + 2, 2, SEEX * 2 - 3, SEEY * 2 - 3, 3);

            add_spawn("mon_turret", 1, SEEX, 5);

            if (is_ot_type("road", t_east)) {
                rotate(1);
            } else if (is_ot_type("road", t_south)) {
                rotate(2);
            } else if (is_ot_type("road", t_west)) {
                rotate(3);
            }
        } else if (tw != 0 || rw != 0 || lw != 0 || bw != 0) { // Sewers!
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    ter_set(i, j, t_rock_floor);
                    if (((i < lw || i > SEEX * 2 - 1 - rw) && j > SEEY - 3 && j < SEEY + 2) ||
                        ((j < tw || j > SEEY * 2 - 1 - bw) && i > SEEX - 3 && i < SEEX + 2)) {
                        ter_set(i, j, t_sewage);
                    }
                    if ((i == 0 && t_east >= "lab" && t_east <= "lab_core") ||
                        (i == 0 && t_east >= "ice_lab" && t_east <= "ice_lab_core") ||
                        i == SEEX * 2 - 1) {
                        if (ter(i, j) == t_sewage) {
                            ter_set(i, j, t_bars);
                        } else if (j == SEEY - 1 || j == SEEY) {
                            ter_set(i, j, t_door_metal_c);
                        } else {
                            ter_set(i, j, t_concrete_v);
                        }
                    } else if ((j == 0 && t_north >= "lab" && t_north <= "lab_core") ||
                               (j == 0 && t_north >= "ice_lab" && t_north <= "ice_lab_core") ||
                               j == SEEY * 2 - 1) {
                        if (ter(i, j) == t_sewage) {
                            ter_set(i, j, t_bars);
                        } else if (i == SEEX - 1 || i == SEEX) {
                            ter_set(i, j, t_door_metal_c);
                        } else {
                            ter_set(i, j, t_concrete_h);
                        }
                    }
                }
            }
        } else { // We're below ground, and no sewers
            // Set up the boudaries of walls (connect to adjacent lab squares)
            // Are we in an ice lab?
            if ( ice_lab ) {
                tw = is_ot_type("ice_lab", t_north) ? 0 : 2;
                rw = is_ot_type("ice_lab", t_east) ? 1 : 2;
                bw = is_ot_type("ice_lab", t_south) ? 1 : 2;
                lw = is_ot_type("ice_lab", t_west) ? 0 : 2;
            } else {
                tw = is_ot_type("lab", t_north) ? 0 : 2;
                rw = is_ot_type("lab", t_east) ? 1 : 2;
                bw = is_ot_type("lab", t_south) ? 1 : 2;
                lw = is_ot_type("lab", t_west) ? 0 : 2;
            }
            int boarders = 0;
            if (tw == 0 ) {
                boarders++;
            }
            if (rw == 1 ) {
                boarders++;
            }
            if (bw == 1 ) {
                boarders++;
            }
            if (lw == 0 ) {
                boarders++;
            }
            //A lab area with only one entrance
            if (boarders == 1) {
                fill_background(this, t_rock_floor);
                if (one_in(2)) { //armory and military barracks
                    mapf::formatted_set_simple(this, 0, 0,
                                               "\
|----------------------|\n\
|r....................r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|r..rr..rr....rr..rr..r|\n\
|......................|\n\
|......................|\n\
|..rrrrrr..........rrr.|\n\
|-----|----DD-|--+|--|-|\n\
|b.ddd|.......gc..|T.|T|\n\
|b..h.+.......g6h.|-+|+|\n\
|l....|.......gc..|....|\n\
|-----|.......|--D|...S|\n\
|b....+...........|...S|\n\
|b...l|...........|-+--|\n\
|-----|................|\n\
|b....+...x............|\n\
|b...l|..|-DD-|+-|+-|+-|\n\
|-----|..|....|.l|.l|.l|\n\
|b....+..|6...|..|..|..|\n\
|b...l|..|....|bb|bb|bb|\n\
|-----|--|-..-|--|--|--|\n",
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", t_rock_floor, t_rock_floor, t_floor,
                                                       t_rock_floor, t_rock_floor, t_centrifuge, t_rock_floor, t_console, t_console_broken,
                                                       t_reinforced_glass_v, t_reinforced_glass_h, t_floor_blue, t_rock_floor, t_concrete_h, t_concrete_v,
                                                       t_door_metal_c, t_door_metal_locked, t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor),
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", f_bed,        f_locker,     f_crate_c,
                                                       f_rack,       f_desk,       f_null,       f_chair,      f_null,    f_null,           f_null,
                                                       f_null,               f_null,       f_null,       f_null,       f_null,       f_null,
                                                       f_null,              f_table,      f_counter,    f_sink,       f_toilet));
                    for (int i = 0; i <= 23; i++) {
                        for (int j = 0; j <= 23; j++) {
                            if (this->furn(i, j) == f_locker) {
                                place_items("mil_surplus", 50,  i,  j, i,  j, false, 0);
                            } else if (this->furn(i, j) == f_desk) {
                                place_items("office", 50,  i,  j, i,  j, false, 0);
                            } else if (this->furn(i, j) == f_rack) {
                                if (one_in(3)) {
                                    place_items("mil_surplus", 30,  i,  j, i,  j, false, 0);
                                } else if (one_in(2)) {
                                    place_items("ammo", 30,  i,  j, i,  j, false, 0);
                                } else if (one_in(3)) {
                                    place_items("military", 30,  i,  j, i,  j, false, 0);
                                } else {
                                    place_items("mil_rifles", 30,  i,  j, i,  j, false, 0);
                                }
                            }
                        }
                    }
                    computer *tmpcomp2 = NULL;
                    tmpcomp2 = add_computer(10, 21, _("Barracks Entrance"), 4);
                    tmpcomp2->add_option(_("UNLOCK ENTRANCE"), COMPACT_UNLOCK, 6);
                    tmpcomp = add_computer(15, 12, _("Magazine Entrance"), 6);
                    tmpcomp->add_option(_("UNLOCK ENTRANCE"), COMPACT_UNLOCK, 7);
                    if (one_in(2)) {
                        add_spawn("mon_zombie_soldier", rng(1, 4), 12, 12);
                    }
                } else { //human containment
                    mapf::formatted_set_simple(this, 0, 0,
                                               "\
|----|-|----|----|-|---|\n\
|b.T.|.|.T.b|b.T.|.|A.A|\n\
|b...D.D...b|b...D.|..A|\n\
|....|.|....|....|.|...|\n\
|....|.|....|....|.|l..|\n\
|-GG-|+|-GG-|-GG-|.|-D-|\n\
|................+.....|\n\
|................|--D--|\n\
|................|...bb|\n\
|................g.....|\n\
|-GGGGGG-|.......g....T|\n\
|..cc6c..g.......|.....|\n\
|..ch.c..|-GGDGG-|-GGG-|\n\
|........g.............|\n\
|^.......|.............|\n\
|-GGG+GG-|.............|\n\
|ddd.....|.............|\n\
|.hd.....+....|-G+GGGG-|\n\
|........|...x|.......c|\n\
|.......r|-DD-|l......S|\n\
|ddd....r|...6|l......c|\n\
|.hd....r|....|........|\n\
|........|....|..cxcC..|\n\
|--------|-..-|--------|\n",
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", t_rock_floor, t_rock_floor, t_floor,
                                                       t_rock_floor, t_rock_floor, t_centrifuge, t_rock_floor, t_console, t_console_broken,
                                                       t_reinforced_glass_v, t_reinforced_glass_h, t_floor_blue, t_rock_floor, t_concrete_h, t_concrete_v,
                                                       t_door_metal_c, t_door_metal_locked, t_rock_floor, t_rock_floor, t_rock_floor, t_rock_floor),
                                               mapf::basic_bind("b l A r d C h 6 x g G , . - | + D t c S T", f_bed,        f_locker,     f_crate_c,
                                                       f_rack,       f_desk,       f_null,       f_chair,      f_null,    f_null,           f_null,
                                                       f_null,               f_null,       f_null,       f_null,       f_null,       f_null,
                                                       f_null,              f_table,      f_counter,    f_sink,       f_toilet));
                    for (int i = 0; i <= 23; i++) {
                        for (int j = 0; j <= 23; j++) {
                            if (this->furn(i, j) == f_locker) {
                                place_items("science", 60,  i,  j, i,  j, false, 0);
                            }
                            if (this->furn(i, j) == f_desk) {
                                place_items("office", 60,  i,  j, i,  j, false, 0);
                            }
                            if (this->furn(i, j) == f_counter) {
                                place_items("office", 40,  i,  j, i,  j, false, 0);
                            }
                            if (this->furn(i, j) == f_rack || this->furn(i, j) == f_crate_c) {
                                place_items("softdrugs", 40,  i,  j, i,  j, false, 0);
                                place_items("harddrugs", 30,  i,  j, i,  j, false, 0);
                            }
                        }
                    }
                    computer *tmpcomp2 = NULL;
                    tmpcomp2 = add_computer(13, 20, _("Prisoner Containment Entrance"), 4);
                    tmpcomp2->add_option(_("UNLOCK ENTRANCE"), COMPACT_UNLOCK, 4);
                    tmpcomp = add_computer(5, 11, _("Containment Control"), 4);
                    tmpcomp->add_option(_("EMERGENCY CONTAINMENT RELEASE"), COMPACT_OPEN, 5);
                    add_trap(19, 19, tr_dissector);
                    item body;
                    body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                    if (one_in(2)) {
                        add_item(1, 1, body);
                    } else {
                        add_spawn("mon_zombie_shrieker", 1, 1, 1);
                    }
                    if (one_in(2)) {
                        add_item(9, 3, body);
                    } else {
                        add_spawn("mon_zombie_brute", 1, 9, 3);
                    }
                    if (one_in(2)) {
                        add_item(14, 4, body);
                    } else {
                        add_spawn("mon_zombie_child", 1, 14, 4);
                    }
                    if (one_in(2)) {
                        add_item(19, 9, body);
                    } else {
                        add_spawn("mon_zombie_grabber", 1, 19, 9);
                    }
                    if (one_in(2)) {
                        add_spawn("mon_zombie_scientist", rng(1, 2), 12, 14);
                    }
                }
                if (bw == 2) {
                    rotate(2);
                }
                if (rw == 2) {
                    rotate(3);
                }
                if (lw == 2) {
                    rotate(1);
                }
                if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                    int sx, sy;
                    do {
                        sx = rng(lw, SEEX * 2 - 1 - rw);
                        sy = rng(tw, SEEY * 2 - 1 - bw);
                    } while (ter(sx, sy) != t_rock_floor);
                    ter_set(sx, sy, t_stairs_up);
                }

                if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                    int sx, sy;
                    do {
                        sx = rng(lw, SEEX * 2 - 1 - rw);
                        sy = rng(tw, SEEY * 2 - 1 - bw);
                    } while (ter(sx, sy) != t_rock_floor);
                    ter_set(sx, sy, t_stairs_down);
                }
            } else switch (rng(1, 4)) { // Pick a random lab layout
                case 1: // Cross shaped
                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if ((i < lw || i > SEEX * 2 - 1 - rw) ||
                                ((j < SEEY - 1 || j > SEEY) && (i == SEEX - 2 || i == SEEX + 1))) {
                                ter_set(i, j, t_concrete_v);
                            } else if ((j < tw || j > SEEY * 2 - 1 - bw) ||
                                       ((i < SEEX - 1 || i > SEEX) && (j == SEEY - 2 || j == SEEY + 1))) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_rock_floor);
                            }
                        }
                    }
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        ter_set(rng(SEEX - 1, SEEX), rng(SEEY - 1, SEEY), t_stairs_up);
                    }
                    // Top left
                    if (one_in(2)) {
                        ter_set(SEEX - 2, int(SEEY / 2), t_door_metal_c);
                        science_room(this, lw, tw, SEEX - 3, SEEY - 3, 1);
                    } else {
                        ter_set(int(SEEX / 2), SEEY - 2, t_door_metal_c);
                        science_room(this, lw, tw, SEEX - 3, SEEY - 3, 2);
                    }
                    // Top right
                    if (one_in(2)) {
                        ter_set(SEEX + 1, int(SEEY / 2), t_door_metal_c);
                        science_room(this, SEEX + 2, tw, SEEX * 2 - 1 - rw, SEEY - 3, 3);
                    } else {
                        ter_set(SEEX + int(SEEX / 2), SEEY - 2, t_door_metal_c);
                        science_room(this, SEEX + 2, tw, SEEX * 2 - 1 - rw, SEEY - 3, 2);
                    }
                    // Bottom left
                    if (one_in(2)) {
                        ter_set(int(SEEX / 2), SEEY + 1, t_door_metal_c);
                        science_room(this, lw, SEEY + 2, SEEX - 3, SEEY * 2 - 1 - bw, 0);
                    } else {
                        ter_set(SEEX - 2, SEEY + int(SEEY / 2), t_door_metal_c);
                        science_room(this, lw, SEEY + 2, SEEX - 3, SEEY * 2 - 1 - bw, 1);
                    }
                    // Bottom right
                    if (one_in(2)) {
                        ter_set(SEEX + int(SEEX / 2), SEEY + 1, t_door_metal_c);
                        science_room(this, SEEX + 2, SEEY + 2, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw, 0);
                    } else {
                        ter_set(SEEX + 1, SEEY + int(SEEY / 2), t_door_metal_c);
                        science_room(this, SEEX + 2, SEEY + 2, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw, 3);
                    }
                    if (rw == 1) {
                        ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
                        ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
                    }
                    if (bw == 1) {
                        ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
                        ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") { // Stairs going down
                        std::vector<point> stair_points;
                        if (tw != 0) {
                            stair_points.push_back(point(SEEX - 1, 2));
                            stair_points.push_back(point(SEEX - 1, 2));
                            stair_points.push_back(point(SEEX    , 2));
                            stair_points.push_back(point(SEEX    , 2));
                        }
                        if (rw != 1) {
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY - 1));
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY - 1));
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY    ));
                            stair_points.push_back(point(SEEX * 2 - 3, SEEY    ));
                        }
                        if (bw != 1) {
                            stair_points.push_back(point(SEEX - 1, SEEY * 2 - 3));
                            stair_points.push_back(point(SEEX - 1, SEEY * 2 - 3));
                            stair_points.push_back(point(SEEX    , SEEY * 2 - 3));
                            stair_points.push_back(point(SEEX    , SEEY * 2 - 3));
                        }
                        if (lw != 0) {
                            stair_points.push_back(point(2, SEEY - 1));
                            stair_points.push_back(point(2, SEEY - 1));
                            stair_points.push_back(point(2, SEEY    ));
                            stair_points.push_back(point(2, SEEY    ));
                        }
                        stair_points.push_back(point(int(SEEX / 2)       , SEEY    ));
                        stair_points.push_back(point(int(SEEX / 2)       , SEEY - 1));
                        stair_points.push_back(point(int(SEEX / 2) + SEEX, SEEY    ));
                        stair_points.push_back(point(int(SEEX / 2) + SEEX, SEEY - 1));
                        stair_points.push_back(point(SEEX    , int(SEEY / 2)       ));
                        stair_points.push_back(point(SEEX + 2, int(SEEY / 2)       ));
                        stair_points.push_back(point(SEEX    , int(SEEY / 2) + SEEY));
                        stair_points.push_back(point(SEEX + 2, int(SEEY / 2) + SEEY));
                        rn = rng(0, stair_points.size() - 1);
                        ter_set(stair_points[rn].x, stair_points[rn].y, t_stairs_down);
                    }

                    break;

                case 2: // tic-tac-toe # layout
                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if (i < lw || i > SEEX * 2 - 1 - rw || i == SEEX - 4 || i == SEEX + 3) {
                                ter_set(i, j, t_concrete_v);
                            } else if (j < lw || j > SEEY * 2 - 1 - bw || j == SEEY - 4 || j == SEEY + 3) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_rock_floor);
                            }
                        }
                    }
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        ter_set(SEEX - 1, SEEY - 1, t_stairs_up);
                        ter_set(SEEX    , SEEY - 1, t_stairs_up);
                        ter_set(SEEX - 1, SEEY    , t_stairs_up);
                        ter_set(SEEX    , SEEY    , t_stairs_up);
                    }
                    ter_set(SEEX - rng(0, 1), SEEY - 4, t_door_metal_c);
                    ter_set(SEEX - rng(0, 1), SEEY + 3, t_door_metal_c);
                    ter_set(SEEX - 4, SEEY + rng(0, 1), t_door_metal_c);
                    ter_set(SEEX + 3, SEEY + rng(0, 1), t_door_metal_c);
                    ter_set(SEEX - 4, int(SEEY / 2), t_door_metal_c);
                    ter_set(SEEX + 3, int(SEEY / 2), t_door_metal_c);
                    ter_set(int(SEEX / 2), SEEY - 4, t_door_metal_c);
                    ter_set(int(SEEX / 2), SEEY + 3, t_door_metal_c);
                    ter_set(SEEX + int(SEEX / 2), SEEY - 4, t_door_metal_c);
                    ter_set(SEEX + int(SEEX / 2), SEEY + 3, t_door_metal_c);
                    ter_set(SEEX - 4, SEEY + int(SEEY / 2), t_door_metal_c);
                    ter_set(SEEX + 3, SEEY + int(SEEY / 2), t_door_metal_c);
                    science_room(this, lw, tw, SEEX - 5, SEEY - 5, rng(1, 2));
                    science_room(this, SEEX - 3, tw, SEEX + 2, SEEY - 5, 2);
                    science_room(this, SEEX + 4, tw, SEEX * 2 - 1 - rw, SEEY - 5, rng(2, 3));
                    science_room(this, lw, SEEY - 3, SEEX - 5, SEEY + 2, 1);
                    science_room(this, SEEX + 4, SEEY - 3, SEEX * 2 - 1 - rw, SEEY + 2, 3);
                    science_room(this, lw, SEEY + 4, SEEX - 5, SEEY * 2 - 1 - bw, rng(0, 1));
                    science_room(this, SEEX - 3, SEEY + 4, SEEX + 2, SEEY * 2 - 1 - bw, 0);
                    science_room(this, SEEX + 4, SEEX + 4, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw, 3 * rng(0, 1));
                    if (rw == 1) {
                        ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
                        ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
                    }
                    if (bw == 1) {
                        ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
                        ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                        ter_set(SEEX - 3 + 5 * rng(0, 1), SEEY - 3 + 5 * rng(0, 1), t_stairs_down);
                    }
                    break;

                case 3: // Big room
                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if (i < lw || i >= SEEX * 2 - 1 - rw) {
                                ter_set(i, j, t_concrete_v);
                            } else if (j < tw || j >= SEEY * 2 - 1 - bw) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_rock_floor);
                            }
                        }
                    }
                    science_room(this, lw, tw, SEEX * 2 - 1 - rw, SEEY * 2 - 1 - bw, rng(0, 3));
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_up);
                    }
                    if (rw == 1) {
                        ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
                        ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
                    }
                    if (bw == 1) {
                        ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
                        ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_down);
                    }
                    break;

                case 4: // alien containment
                    fill_background(this, t_rock_floor);
                    if (one_in(4)) {
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
.....|..|.....|........|\n\
.....|6.|.....|..cxcC..|\n\
.....g..g.....g.......l|\n\
.....g..g.....g.......l|\n\
.....D..g.....|.......S|\n\
-----|..|.....|-GG+-GG-|\n\
.....D..g..............|\n\
.....g..g..............|\n\
.....g..g..............|\n\
.....|..|-GGGG-|.......|\n\
----||+-|,,,,,,|.......|\n\
....+...D,,,,,,g.......+\n\
....|-G-|,,,,,,g.......+\n\
........|,,,,,,|.......|\n\
........|-GGGG-|.......|\n\
.......................|\n\
.........cxc6cc........|\n\
.........ch.h.c........|\n\
.......................|\n\
.ccxcc............ccxcc|\n\
.c.h.c............c.h.c|\n\
.......................|\n\
.......................|\n\
-----------++----------|\n",
                                                   mapf::basic_bind("l A r d C h 6 x g G , . - | + D t c S", t_rock_floor, t_floor,   t_rock_floor,
                                                           t_rock_floor, t_centrifuge, t_rock_floor, t_console, t_console_broken, t_reinforced_glass_v,
                                                           t_reinforced_glass_h, t_floor_blue, t_rock_floor, t_concrete_h, t_concrete_v, t_door_metal_c,
                                                           t_door_metal_locked, t_rock_floor, t_rock_floor, t_rock_floor),
                                                   mapf::basic_bind("l A r d C h 6 x g G , . - | + D t c S", f_locker,     f_crate_c, f_rack,
                                                           f_desk,       f_null,       f_chair,      f_null,    f_null,           f_null,               f_null,
                                                           f_null,       f_null,       f_null,       f_null,       f_null,         f_null ,
                                                           f_table,      f_counter,    f_sink));
                        add_trap(19, 3, tr_dissector);
                        if (one_in(3)) {
                            add_spawn("mon_mi_go", 1, 12, 12);
                        } else {
                            add_spawn("mon_zombie_brute", 1 , 12, 12);
                        }
                        if (one_in(3)) {
                            add_spawn("mon_kreck", 1, 2, 2);
                        }
                        if (one_in(3)) {
                            add_spawn("mon_crawler", 1, 2, 7);
                        }
                        if (one_in(2)) {
                            add_spawn("mon_zombie_scientist", rng(1, 3), 12, 18);
                        }
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_counter) {
                                    place_items("office", 30,  i,  j, i,  j, false, 0);
                                } else if (this->furn(i, j) == f_locker) {
                                    place_items("science", 60,  i,  j, i,  j, false, 0);
                                }
                                item body;
                                body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_item(i, j, body);
                                }
                            }
                        }
                        computer *tmpcomp2 = NULL;
                        tmpcomp2 = add_computer(6, 1, _("Containment Terminal"), 4);
                        tmpcomp2->add_option(_("EMERGENCY CONTAINMENT UNLOCK"), COMPACT_UNLOCK, 4);
                        tmpcomp = add_computer(12, 16, _("Containment Control"), 4);
                        tmpcomp->add_option(_("EMERGENCY CONTAINMENT UNLOCK"), COMPACT_UNLOCK, 4);
                        tmpcomp->add_option(_("EMERGENCY CLEANSE"), COMPACT_DISCONNECT, 7);
                    } else if (one_in(3)) { //operations or utility
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
.....|...........f.....|\n\
.lll.|...........f.rrrr|\n\
.....+...........H.....|\n\
.ll..|...........fAA..r|\n\
-----|FFFF|---|..fAA..r|\n\
.....f....|...|..f....r|\n\
.pSp.f.PP.|.&.|..fAA..A|\n\
.pSp.f.PP.|.x.|..fAA..A|\n\
.....f.PP.|...|..f.....|\n\
.....H....|-+-|..fFFHFF|\n\
FFHFFf........f........|\n\
.....f........f........+\n\
.....fFFFHFFFFf........+\n\
.......................|\n\
.................|-G-G-|\n\
-------|.........|^....|\n\
AA..A..D.........+.....|\n\
AA.....D.........|..ddd|\n\
AAA....D.........g..dh.|\n\
-------|M........g.....|\n\
A.AA...D.........g.....|\n\
A......D.........|dh...|\n\
.A.AA..D.........|dxd.^|\n\
-------|---++----|-----|\n",
                                                   mapf::basic_bind("M D & P S p l H O f F A r d C h 6 x g G , . - | + D t c ^",
                                                           t_gates_control_concrete, t_door_metal_locked, t_radio_tower, t_generator_broken, t_sewage_pump,
                                                           t_sewage_pipe, t_floor,  t_chaingate_c, t_column, t_chainfence_v, t_chainfence_h, t_floor,
                                                           t_floor, t_floor, t_centrifuge, t_null,  t_console, t_console_broken, t_wall_glass_v,
                                                           t_wall_glass_h, t_rock_blue, t_rock_floor, t_concrete_h, t_concrete_v, t_door_metal_c,
                                                           t_door_metal_locked, t_floor,  t_floor,   t_floor),
                                                   mapf::basic_bind("M D & P S p l H O f F A r d C h 6 x g G , . - | + D t c ^", f_null,
                                                           f_null,              f_null,        f_null,             f_null,        f_null,        f_locker,
                                                           f_null,        f_null,   f_null,         f_null,         f_crate_c, f_rack,  f_desk,  f_null,
                                                           f_chair, f_null,    f_null,           f_null,         f_null,         f_null,      f_null,
                                                           f_null,       f_null,       f_null,         f_null,              f_table,  f_counter,
                                                           f_indoor_plant));
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_crate_c) {
                                    if (one_in(2)) {
                                        place_items("robots", 60,  i,  j, i,  j, false, 0);
                                    } else if (one_in(2)) {
                                        place_items("science", 60,  i,  j, i,  j, false, 0);
                                    } else {
                                        place_items("sewage_plant", 30,  i,  j, i,  j, false, 0);
                                    }
                                } else if (this->furn(i, j) == f_locker) {
                                    place_items("cleaning", 60,  i,  j, i,  j, false, 0);
                                } else if (this->furn(i, j) == f_rack) {
                                    place_items("mine_equipment", 30,  i,  j, i,  j, false, 0);
                                }
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_spawn("mon_zombie", 1, i, j);
                                }
                                item body;
                                body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_item(i, j, body);
                                }
                            }
                        }
                    } else if (one_in(2)) { //tribute
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
%%%%%%%%%|....|%%%%%%%%|\n\
%|-|-|%%%|....|%%%|-|--|\n\
%|T|T|---|....|---|T|.T|\n\
%|.|.|EEE+....+EEE|.|..|\n\
%|=|=|EEE+....+EEE|=|=-|\n\
%|...|EEe|....|eEE|....|\n\
%|...|---|....|---|....|\n\
%|...+............+....|\n\
%|ScS|............|cScS|\n\
-|---|............|----|\n\
.......................|\n\
.......................+\n\
.................w.....+\n\
................www....|\n\
--GGG+GG-|....|-GGGGGG-|\n\
ff.......|....|WWWWWWWW|\n\
...htth..g....gWWWWWWWl|\n\
...htth..g....gWWWcWWWl|\n\
.........+....DWWWcWWWW|\n\
.........g....gWWWCWWWW|\n\
...htth..g....gWWWcWWWW|\n\
...htth..g....gWWWWWWhd|\n\
........^|....|rrrWWdxd|\n\
---------|-++-|--------|\n",
                                                   mapf::basic_bind("D l H O f A r d C h 6 x g G , . - | + D t c ^ w W e E % T S =",
                                                           t_door_metal_locked, t_floor,  t_chaingate_c, t_column, t_floor,  t_floor,   t_floor, t_floor,
                                                           t_centrifuge, t_floor, t_console, t_console_broken, t_reinforced_glass_v, t_reinforced_glass_h,
                                                           t_rock_blue, t_rock_floor, t_concrete_h, t_concrete_v, t_door_metal_c, t_door_metal_locked, t_floor,
                                                           t_floor,   t_floor,        t_water_sh, t_water_dp, t_elevator_control_off, t_elevator, t_rock,
                                                           t_floor,  t_floor, t_door_c),
                                                   mapf::basic_bind("D l H O f A r d C h 6 x g G , . - | + D t c ^ w W e E % T S =", f_null,
                                                           f_locker, f_null,        f_null,   f_fridge, f_crate_c, f_rack,  f_desk,  f_null,       f_chair,
                                                           f_null,    f_null,           f_null,               f_null,               f_null,      f_null,
                                                           f_null,       f_null,       f_null,         f_null,              f_table, f_counter, f_indoor_plant,
                                                           f_null,     f_null,     f_null,                 f_null,     f_null, f_toilet, f_sink,  f_null));
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_locker) {
                                    place_items("science", 60,  i,  j, i,  j, false, 0);
                                } else if (this->furn(i, j) == f_fridge) {
                                    place_items("fridge", 50,  i,  j, i,  j, false, 0);
                                }
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_spawn("mon_zombie", 1, i, j);
                                }
                            }
                        }
                        item body;
                        body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                        add_item(17, 15, body);
                        add_item(8, 3, body);
                        add_item(10, 3, body);
                        spawn_item(18, 15, "ax");
                    }

                    else { //analyzer
                        mapf::formatted_set_simple(this, 0, 0,
                                                   "\
.......................|\n\
.......................|\n\
.......................|\n\
.......................|\n\
....|-GGGGGGGGGGG-|....|\n\
....|.............|....|\n\
....g.....&.&.....g....|\n\
....g......,......g....|\n\
....g.....&6&.....g....|\n\
....g.............g....|\n\
....grrr.rrrrr.rrrg....|\n\
....gcxc..cxc..cxcg....+\n\
....gch....h....hcg....+\n\
....|.............|....|\n\
....|-+|..cxc..|+-|....|\n\
....+..g...h...g..+....|\n\
....g..g.......g..g....|\n\
....|..|.......|..|....|\n\
....|-G|GGGGGGG|G-|....|\n\
.......................|\n\
.......................|\n\
.......................|\n\
.......................|\n\
-----------++----------|\n",
                                                   mapf::basic_bind("r d h 6 x g G , . - | + D t c ^ % = &", t_railing_h, t_rock_floor, t_rock_floor,
                                                           t_console, t_console_broken, t_wall_glass_v, t_wall_glass_h, t_floor_blue, t_rock_floor,
                                                           t_concrete_h, t_concrete_v, t_door_metal_c, t_door_metal_locked, t_rock_floor, t_rock_floor,
                                                           t_floor,        t_rock, t_door_c, t_radio_tower),
                                                   mapf::basic_bind("r d h 6 x g G , . - | + D t c ^ % = &", f_null,      f_desk,       f_chair,
                                                           f_null,    f_null,           f_null,         f_null,         f_null,       f_null,       f_null,
                                                           f_null,       f_null,         f_null,              f_table,      f_counter,    f_indoor_plant,
                                                           f_null, f_null,   f_null));
                        for (int i = 0; i <= 23; i++) {
                            for (int j = 0; j <= 23; j++) {
                                if (this->furn(i, j) == f_counter) {
                                    place_items("cubical_office", 30,  i,  j, i,  j, false, 0);
                                }
                                if (one_in(500) && this->ter(i, j) == t_rock_floor) {
                                    add_spawn("mon_zombie", 1, i, j);
                                }
                                item body;
                                body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                                if (one_in(400) && this->ter(i, j) == t_rock_floor) {
                                    add_item(i, j, body);
                                }
                            }
                        }
                        tmpcomp = add_computer(11, 8, _("Mk IV Algorithmic Data Analyzer"), 4);
                        tmpcomp->add_option(_("Run Decryption Algorithm"), COMPACT_DATA_ANAL, 4);
                        tmpcomp->add_option(_("Upload Data to Melchior"), COMPACT_DISCONNECT, 7);
                        tmpcomp->add_option(_("Access Melchior"), COMPACT_DISCONNECT, 12);
                        tmpcomp->add_failure(COMPFAIL_DESTROY_DATA);
                    }

                    for (int i = 0; i < SEEX * 2; i++) {
                        for (int j = 0; j < SEEY * 2; j++) {
                            if (i < lw || i >= SEEX * 2 - rw + 1) {
                                ter_set(i, j, t_concrete_v);
                            } else if (j < (tw - 1) || j >= SEEY * 2 - bw + 1) {
                                ter_set(i, j, t_concrete_h);
                            }
                        }
                    }
                    if (t_above == "lab_stairs" || t_above == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_up);
                    }
                    if (terrain_type == "lab_stairs" || terrain_type == "ice_lab_stairs") {
                        int sx, sy;
                        do {
                            sx = rng(lw, SEEX * 2 - 1 - rw);
                            sy = rng(tw, SEEY * 2 - 1 - bw);
                        } while (ter(sx, sy) != t_rock_floor);
                        ter_set(sx, sy, t_stairs_down);
                    }
                    break;

                }
        }
        // Ants will totally wreck up the place
        tw = 0;
        rw = 0;
        bw = 0;
        lw = 0;
        if (is_ot_type("ants", t_north) && connects_to(t_north, 2)) {
            tw = SEEY;
        }
        if (is_ot_type("ants", t_east) && connects_to(t_east, 3)) {
            rw = SEEX;
        }
        if (is_ot_type("ants", t_south) && connects_to(t_south, 0)) {
            bw = SEEY + 1;
        }
        if (is_ot_type("ants", t_west) && connects_to(t_west, 1)) {
            lw = SEEX + 1;
        }
        if (tw != 0 || rw != 0 || bw != 0 || lw != 0) {
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if ((i < SEEX * 2 - lw && (!one_in(3) || (j > SEEY - 6 && j < SEEY + 5))) ||
                        (i > rw &&          (!one_in(3) || (j > SEEY - 6 && j < SEEY + 5))) ||
                        (j > tw &&          (!one_in(3) || (i > SEEX - 6 && i < SEEX + 5))) ||
                        (j < SEEY * 2 - bw && (!one_in(3) || (i > SEEX - 6 && i < SEEX + 5)))) {
                        if (one_in(5)) {
                            ter_set(i, j, t_rubble);
                        } else {
                            ter_set(i, j, t_rock_floor);
                        }
                    }
                }
            }
        }

        // Slimes pretty much wreck up the place, too, but only underground
        tw = (t_north == "slimepit" ? SEEY     : 0);
        rw = (t_east  == "slimepit" ? SEEX + 1 : 0);
        bw = (t_south == "slimepit" ? SEEY + 1 : 0);
        lw = (t_west  == "slimepit" ? SEEX     : 0);
        if (tw != 0 || rw != 0 || bw != 0 || lw != 0) {
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (((j <= tw || i >= rw) && i >= j && (SEEX * 2 - 1 - i) <= j) ||
                        ((j >= bw || i <= lw) && i <= j && (SEEY * 2 - 1 - j) <= i)   ) {
                        if (one_in(5)) {
                            ter_set(i, j, t_rubble);
                        } else if (!one_in(5)) {
                            ter_set(i, j, t_slime);
                        }
                    }
                }
            }
        }


    } else if (terrain_type == "lab_finale" ||
               terrain_type == "ice_lab_finale") {

        if (is_ot_type("ice_lab", terrain_type)) {
            ice_lab = true;
        } else {
            ice_lab = false;
        }

        if ( ice_lab ) {
            int temperature = -20 + 30 * (g->levz);
            set_temperature(x, y, temperature);

            tw = is_ot_type("ice_lab", t_north) ? 0 : 2;
            rw = is_ot_type("ice_lab", t_east) ? 1 : 2;
            bw = is_ot_type("ice_lab", t_south) ? 1 : 2;
            lw = is_ot_type("ice_lab", t_west) ? 0 : 2;
        } else {
            tw = is_ot_type("lab", t_north) ? 0 : 2;
            rw = is_ot_type("lab", t_east) ? 1 : 2;
            bw = is_ot_type("lab", t_south) ? 1 : 2;
            lw = is_ot_type("lab", t_west) ? 0 : 2;
        }

        // Start by setting up a large, empty room.
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < lw || i > SEEX * 2 - 1 - rw) {
                    ter_set(i, j, t_concrete_v);
                } else if (j < tw || j > SEEY * 2 - 1 - bw) {
                    ter_set(i, j, t_concrete_h);
                } else {
                    ter_set(i, j, t_floor);
                }
            }
        }
        if (rw == 1) {
            ter_set(SEEX * 2 - 1, SEEY - 1, t_door_metal_c);
            ter_set(SEEX * 2 - 1, SEEY    , t_door_metal_c);
        }
        if (bw == 1) {
            ter_set(SEEX - 1, SEEY * 2 - 1, t_door_metal_c);
            ter_set(SEEX    , SEEY * 2 - 1, t_door_metal_c);
        }

        switch (rng(1, 3)) {
        case 1: // Weapons testing
            add_spawn("mon_secubot", 1,            6,            6);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7,            6);
            add_spawn("mon_secubot", 1,            6, SEEY * 2 - 7);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7, SEEY * 2 - 7);
            add_trap(SEEX - 2, SEEY - 2, tr_dissector);
            add_trap(SEEX + 1, SEEY - 2, tr_dissector);
            add_trap(SEEX - 2, SEEY + 1, tr_dissector);
            add_trap(SEEX + 1, SEEY + 1, tr_dissector);
            if (!one_in(3)) {
                rn = dice(4, 3);
                spawn_item(SEEX - 1, SEEY - 1, "laser_pack", rn);
                spawn_item(SEEX + 1, SEEY - 1, "UPS_off", rn);
                spawn_item(SEEX - 1, SEEY    , "v29");
                spawn_item(SEEX + 1, SEEY    , "ftk93");
            } else if (!one_in(3)) {
                rn = dice(3, 6);
                spawn_item(SEEX - 1, SEEY - 1, "mininuke", rn);
                spawn_item(SEEX    , SEEY - 1, "mininuke", rn);
                spawn_item(SEEX - 1, SEEY    , "mininuke", rn);
                spawn_item(SEEX    , SEEY    , "mininuke", rn);
            } else {
                furn_set(SEEX - 2, SEEY - 1, f_rack);
                furn_set(SEEX - 1, SEEY - 1, f_rack);
                furn_set(SEEX    , SEEY - 1, f_rack);
                furn_set(SEEX + 1, SEEY - 1, f_rack);
                furn_set(SEEX - 2, SEEY    , f_rack);
                furn_set(SEEX - 1, SEEY    , f_rack);
                furn_set(SEEX    , SEEY    , f_rack);
                furn_set(SEEX + 1, SEEY    , f_rack);
                place_items("ammo", 96, SEEX - 2, SEEY - 1, SEEX + 1, SEEY - 1, false, 0);
                place_items("allguns", 96, SEEX - 2, SEEY, SEEX + 1, SEEY, false, 0);
            }
            break;

        case 2: { // Netherworld access
            if (!one_in(4)) { // Trapped netherworld monsters
                std::string nethercreatures[10] = {"mon_flying_polyp", "mon_hunting_horror", "mon_mi_go", "mon_yugg", "mon_gelatin",
                                                   "mon_flaming_eye", "mon_kreck", "mon_gracke", "mon_blank", "mon_gozu"
                                                  };
                tw = rng(SEEY + 3, SEEY + 5);
                bw = tw + 4;
                lw = rng(SEEX - 6, SEEX - 2);
                rw = lw + 6;
                for (int i = lw; i <= rw; i++) {
                    for (int j = tw; j <= bw; j++) {
                        if (j == tw || j == bw) {
                            if ((i - lw) % 2 == 0) {
                                ter_set(i, j, t_concrete_h);
                            } else {
                                ter_set(i, j, t_reinforced_glass_h);
                            }
                        } else if ((i - lw) % 2 == 0) {
                            ter_set(i, j, t_concrete_v);
                        } else if (j == tw + 2) {
                            ter_set(i, j, t_concrete_h);
                        } else { // Empty space holds monsters!
                            std::string type = nethercreatures[(rng(0, 9))];
                            add_spawn(type, 1, i, j);
                        }
                    }
                }
            }
            tmpcomp = add_computer(SEEX, 8, _("Sub-prime contact console"), 7);
            tmpcomp->add_option(_("Terminate Specimens"), COMPACT_TERMINATE, 2);
            tmpcomp->add_option(_("Release Specimens"), COMPACT_RELEASE, 3);
            tmpcomp->add_option(_("Toggle Portal"), COMPACT_PORTAL, 8);
            tmpcomp->add_option(_("Activate Resonance Cascade"), COMPACT_CASCADE, 10);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);
            ter_set(SEEX - 2, 4, t_radio_tower);
            ter_set(SEEX + 1, 4, t_radio_tower);
            ter_set(SEEX - 2, 7, t_radio_tower);
            ter_set(SEEX + 1, 7, t_radio_tower);
        }
        break;

        case 3: // Bionics
            add_spawn("mon_secubot", 1,            6,            6);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7,            6);
            add_spawn("mon_secubot", 1,            6, SEEY * 2 - 7);
            add_spawn("mon_secubot", 1, SEEX * 2 - 7, SEEY * 2 - 7);
            add_trap(SEEX - 2, SEEY - 2, tr_dissector);
            add_trap(SEEX + 1, SEEY - 2, tr_dissector);
            add_trap(SEEX - 2, SEEY + 1, tr_dissector);
            add_trap(SEEX + 1, SEEY + 1, tr_dissector);
            square_furn(this, f_counter, SEEX - 1, SEEY - 1, SEEX, SEEY);
            int item_count = 0;
            while (item_count < 5) {
                item_count += place_items("bionics", 75, SEEX - 1, SEEY - 1, SEEX, SEEY, false, 0);
            }
            line(this, t_reinforced_glass_h, SEEX - 2, SEEY - 2, SEEX + 1, SEEY - 2);
            line(this, t_reinforced_glass_h, SEEX - 2, SEEY + 1, SEEX + 1, SEEY + 1);
            line(this, t_reinforced_glass_v, SEEX - 2, SEEY - 1, SEEX - 2, SEEY);
            line(this, t_reinforced_glass_v, SEEX + 1, SEEY - 1, SEEX + 1, SEEY);
            ter_set(SEEX - 3, SEEY - 3, t_console);
            tmpcomp = add_computer(SEEX - 3, SEEY - 3, _("Bionic access"), 3);
            tmpcomp->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp->add_option(_("Open Chambers"), COMPACT_RELEASE, 5);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);
            break;
        }


    } else if (terrain_type == "bunker") {

        if (t_above == "") { // We're on ground level
            fill_background(this, &grass_or_dirt);
            //chainlink fence that surrounds bunker
            line(this, t_chainfence_v, 1, 1, 1, SEEY * 2 - 1);
            line(this, t_chainfence_v, SEEX * 2 - 1, 1, SEEX * 2 - 1, SEEY * 2 - 1);
            line(this, t_chainfence_h, 1, SEEY * 2 - 1, SEEX * 2 - 1, SEEY * 2 - 1);
            line(this, t_chainfence_h, 1, 1, SEEX * 2 - 1, 1);
            line(this, t_chaingate_l, SEEX - 3, SEEY * 2 - 1, SEEX + 2, SEEY * 2 - 1);
            line(this, t_chaingate_l, 1, SEEY * 2 - 2, 1, SEEY * 2 - 7);
            line(this, t_chaingate_l, SEEX * 2 - 1, SEEY * 2 - 2, SEEX * 2 - 1, SEEY * 2 - 7);
            //watch cabin
            //line(this, t_concrete_h, SEEX-6, SEEY*2-4, SEEX-4, SEEY*2-4);
            ter_set(2, 13, t_concrete_h);
            ter_set(4, 13, t_concrete_h);
            ter_set(3, 13, t_door_c);
            ter_set(4, 14, t_concrete_v);
            ter_set(4, 15, t_concrete_v);
            ter_set(1, 13, t_concrete_h);
            ter_set(3, 15, t_window);
            ter_set(2, 15, t_window);
            ter_set(1, 14, t_reinforced_glass_v);
            ter_set(1, 15, t_reinforced_glass_v);
            ter_set(2, 14, t_floor);
            ter_set(3, 14, t_floor);
            furn_set(2, 14, f_table);
            //watch cabin 2
            ter_set(SEEX * 2 - 2, 13, t_concrete_h);
            ter_set(SEEX * 2 - 4, 13, t_concrete_h);
            ter_set(SEEX * 2 - 3, 13, t_door_c);
            ter_set(SEEX * 2 - 4, 14, t_concrete_v);
            ter_set(SEEX * 2 - 4, 15, t_concrete_v);
            ter_set(SEEX * 2 - 1, 13, t_concrete_h);
            ter_set(SEEX * 2 - 3, 15, t_window);
            ter_set(SEEX * 2 - 2, 15, t_window);
            ter_set(SEEX * 2 - 1, 14, t_reinforced_glass_v);
            ter_set(SEEX * 2 - 1, 15, t_reinforced_glass_v);
            ter_set(SEEX * 2 - 2, 14, t_floor);
            ter_set(SEEX * 2 - 3, 14, t_floor);
            furn_set(SEEX * 2 - 2, 14, f_table);
            line(this, t_wall_metal_h,  7,  7, 16,  7);
            line(this, t_wall_metal_h,  8,  8, 15,  8);
            line(this, t_wall_metal_h,  8, 15, 15, 15);
            line(this, t_wall_metal_h,  7, 16, 10, 16);
            line(this, t_wall_metal_h, 14, 16, 16, 16);
            line(this, t_wall_metal_v,  7,  8,  7, 15);
            line(this, t_wall_metal_v, 16,  8, 16, 15);
            line(this, t_wall_metal_v,  8,  9,  8, 14);
            line(this, t_wall_metal_v, 15,  9, 15, 14);
            square(this, t_floor, 9, 10, 14, 14);
            line(this, t_stairs_down,  11,  9, 12,  9);
            line(this, t_door_metal_locked, 11, 15, 12, 15);
            for (int i = 9; i <= 13; i += 2) {
                line(this, t_wall_metal_h,  9, i, 10, i);
                line(this, t_wall_metal_h, 13, i, 14, i);
                add_spawn("mon_turret", 1, 9, i + 1);
                add_spawn("mon_turret", 1, 14, i + 1);
            }
            ter_set(13, 16, t_card_military);
        } else { // Below ground!

            fill_background(this, t_rock);
            square(this, t_floor, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2);
            line(this, t_wall_metal_h,  2,  8,  8,  8);
            line(this, t_wall_metal_h, 15,  8, 21,  8);
            line(this, t_wall_metal_h,  2, 15,  8, 15);
            line(this, t_wall_metal_h, 15, 15, 21, 15);
            for (int j = 2; j <= 16; j += 7) {
                ter_set( 9, j    , t_card_military);
                ter_set(14, j    , t_card_military);
                ter_set( 9, j + 1, t_door_metal_locked);
                ter_set(14, j + 1, t_door_metal_locked);
                line(this, t_reinforced_glass_v,  9, j + 2,  9, j + 4);
                line(this, t_reinforced_glass_v, 14, j + 2, 14, j + 4);
                line(this, t_wall_metal_v,  9, j + 5,  9, j + 6);
                line(this, t_wall_metal_v, 14, j + 5, 14, j + 6);

                // Fill rooms with items!
                for (int i = 2; i <= 15; i += 13) {
                    items_location goods;
                    int size = 0;
                    bool HW = 0;
                    switch (rng(1, 14)) {
                    case  1:
                    case  2:
                        goods = "bots";
                        size = 85;
                        HW = 0;
                        break;
                    case  3:
                    case  4:
                        goods = "launchers";
                        size = 83;
                        HW = 0;
                        break;
                    case  5:
                    case  6:
                        goods = "mil_rifles";
                        size = 87;
                        HW = 0;
                        break;
                    case  7:
                    case  8:
                        goods = "grenades";
                        size = 88;
                        HW = 0;
                        break;
                    case  9:
                    case 10:
                        goods = "mil_armor";
                        size = 85;
                        HW = 0;
                        break;
                    case 11:
                        goods = "mil_hw";
                        size = 82;
                        HW = 1;
                        break;
                    case 12:
                    case 13:
                        goods = "mil_food";
                        size = 90;
                        HW = 0;
                        break;
                    case 14:
                        goods = "bionics_mil";
                        size = 78;
                        HW = 0;
                        break;
                    }
                    place_items(goods, size, i, j, i + 6, j + 5, false, 0);
                      if (HW == 1) {
                        if (one_in(2)) {
                        spawn_item(i + 2, j + 1, "556", 2);
                        }
                        if (one_in(4)) {
                        spawn_item(i + 2, j + 1, "8mm_fmj", 2);
                        }
                        if (one_in(4)) {
                        spawn_item(i + 2, j + 1, "8mm_inc", 2);
                        }
                        if (one_in(5)) {
                        spawn_item(i + 2, j + 1, "20x66_flechette", 2);
                        }
                        if (one_in(5)) {
                        spawn_item(i + 2, j + 1, "40mm_concussive", 2);
                        }
                      }
                }
            }
            line(this, t_wall_metal_h, 1, 1, SEEX * 2 - 2, 1);
            line(this, t_wall_metal_h, 1, SEEY * 2 - 2, SEEX * 2 - 2, SEEY * 2 - 2);
            line(this, t_wall_metal_v, 1, 2, 1, SEEY * 2 - 3);
            line(this, t_wall_metal_v, SEEX * 2 - 2, 2, SEEX * 2 - 2, SEEY * 2 - 3);
            ter_set(SEEX - 1, 21, t_stairs_up);
            ter_set(SEEX,     21, t_stairs_up);
        }


    } else if (terrain_type == "outpost") {

        fill_background(this, &grass_or_dirt);

        square(this, t_dirt, 3, 3, 20, 20);
        line(this, t_chainfence_h,  2,  2, 10,  2);
        line(this, t_chainfence_h, 13,  2, 21,  2);
        line(this, t_chaingate_l,      11,  2, 12,  2);
        line(this, t_chainfence_h,  2, 21, 10, 21);
        line(this, t_chainfence_h, 13, 21, 21, 21);
        line(this, t_chaingate_l,      11, 21, 12, 21);
        line(this, t_chainfence_v,  2,  3,  2, 10);
        line(this, t_chainfence_v, 21,  3, 21, 10);
        line(this, t_chaingate_l,       2, 11,  2, 12);
        line(this, t_chainfence_v,  2, 13,  2, 20);
        line(this, t_chainfence_v, 21, 13, 21, 20);
        line(this, t_chaingate_l,      21, 11, 21, 12);
        // Place some random buildings

        bool okay = true;
        while (okay) {
            int buildx = rng(6, 17), buildy = rng(6, 17);
            int buildwidthmax  = (buildx <= 11 ? buildx - 3 : 20 - buildx),
                buildheightmax = (buildy <= 11 ? buildy - 3 : 20 - buildy);
            int buildwidth = rng(3, buildwidthmax), buildheight = rng(3, buildheightmax);
            if (ter(buildx, buildy) != t_dirt) {
                okay = false;
            } else {
                int bx1 = buildx - buildwidth, bx2 = buildx + buildwidth,
                    by1 = buildy - buildheight, by2 = buildy + buildheight;
                square(this, t_floor, bx1, by1, bx2, by2);
                line(this, t_concrete_h, bx1, by1, bx2, by1);
                line(this, t_concrete_h, bx1, by2, bx2, by2);
                line(this, t_concrete_v, bx1, by1, bx1, by2);
                line(this, t_concrete_v, bx2, by1, bx2, by2);
                switch (rng(1, 3)) {  // What type of building?
                case 1: // Barracks
                    for (int i = by1 + 1; i <= by2 - 1; i += 2) {
                        line_furn(this, f_bed, bx1 + 1, i, bx1 + 2, i);
                        line_furn(this, f_bed, bx2 - 2, i, bx2 - 1, i);
                    }
                    place_items("bedroom", 84, bx1 + 1, by1 + 1, bx2 - 1, by2 - 1, false, 0);
                    break;
                case 2: // Armory
                    line_furn(this, f_counter, bx1 + 1, by1 + 1, bx2 - 1, by1 + 1);
                    line_furn(this, f_counter, bx1 + 1, by2 - 1, bx2 - 1, by2 - 1);
                    line_furn(this, f_counter, bx1 + 1, by1 + 2, bx1 + 1, by2 - 2);
                    line_furn(this, f_counter, bx2 - 1, by1 + 2, bx2 - 1, by2 - 2);
                    place_items("mil_rifles", 40, bx1 + 1, by1 + 1, bx2 - 1, by1 + 1, false, 0);
                    place_items("launchers",  40, bx1 + 1, by2 - 1, bx2 - 1, by2 - 1, false, 0);
                    place_items("grenades",   40, bx1 + 1, by1 + 2, bx1 + 1, by2 - 2, false, 0);
                    place_items("mil_armor",  40, bx2 - 1, by1 + 2, bx2 - 1, by2 - 2, false, 0);
                    break;
                case 3: // Supplies
                    for (int i = by1 + 1; i <= by2 - 1; i += 3) {
                        line_furn(this, f_rack, bx1 + 2, i, bx2 - 2, i);
                        place_items("mil_food", 78, bx1 + 2, i, bx2 - 2, i, false, 0);
                    }
                    break;
                }
                std::vector<direction> doorsides;
                if (bx1 > 3) {
                    doorsides.push_back(WEST);
                }
                if (bx2 < 20) {
                    doorsides.push_back(EAST);
                }
                if (by1 > 3) {
                    doorsides.push_back(NORTH);
                }
                if (by2 < 20) {
                    doorsides.push_back(SOUTH);
                }
                int doorx = 0, doory = 0;
                switch (doorsides[rng(0, doorsides.size() - 1)]) {
                case WEST:
                    doorx = bx1;
                    doory = rng(by1 + 1, by2 - 1);
                    break;
                case EAST:
                    doorx = bx2;
                    doory = rng(by1 + 1, by2 - 1);
                    break;
                case NORTH:
                    doorx = rng(bx1 + 1, bx2 - 1);
                    doory = by1;
                    break;
                case SOUTH:
                    doorx = rng(bx1 + 1, bx2 - 1);
                    doory = by2;
                    break;
                default:
                    break;
                }
                for (int i = doorx - 1; i <= doorx + 1; i++) {
                    for (int j = doory - 1; j <= doory + 1; j++) {
                        i_clear(i, j);
                        if (furn(i, j) == f_bed || furn(i, j) == f_rack || furn(i, j) == f_counter) {
                            set(i, j, t_floor, f_null);
                        }
                    }
                }
                ter_set(doorx, doory, t_door_c);
            }
        }
        // Seal up the entrances if there's walls there
        if (ter(11,  3) != t_dirt) {
            ter_set(11,  2, t_concrete_h);
        }
        if (ter(12,  3) != t_dirt) {
            ter_set(12,  2, t_concrete_h);
        }

        if (ter(11, 20) != t_dirt) {
            ter_set(11,  2, t_concrete_h);
        }
        if (ter(12, 20) != t_dirt) {
            ter_set(12,  2, t_concrete_h);
        }

        if (ter( 3, 11) != t_dirt) {
            ter_set( 2, 11, t_concrete_v);
        }
        if (ter( 3, 12) != t_dirt) {
            ter_set( 2, 12, t_concrete_v);
        }

        if (ter( 3, 11) != t_dirt) {
            ter_set( 2, 11, t_concrete_v);
        }
        if (ter( 3, 12) != t_dirt) {
            ter_set( 2, 12, t_concrete_v);
        }

        // Place turrets by (possible) entrances
        add_spawn("mon_turret", 1,  3, 11);
        add_spawn("mon_turret", 1,  3, 12);
        add_spawn("mon_turret", 1, 20, 11);
        add_spawn("mon_turret", 1, 20, 12);
        add_spawn("mon_turret", 1, 11,  3);
        add_spawn("mon_turret", 1, 12,  3);
        add_spawn("mon_turret", 1, 11, 20);
        add_spawn("mon_turret", 1, 12, 20);

        // Finally, scatter dead bodies / mil zombies
        for (int i = 0; i < 20; i++) {
            int rnx = rng(3, 20), rny = rng(3, 20);
            if (move_cost(rnx, rny) != 0) {
                if (one_in(5)) { // Military zombie
                    add_spawn("mon_zombie_soldier", 1, rnx, rny);
                } else if (one_in(2)) {
                    item body;
                    body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                    add_item(rnx, rny, body);
                    place_items("launchers",  10, rnx, rny, rnx, rny, true, 0);
                    place_items("mil_rifles", 30, rnx, rny, rnx, rny, true, 0);
                    place_items("mil_armor",  70, rnx, rny, rnx, rny, true, 0);
                    place_items("mil_food",   40, rnx, rny, rnx, rny, true, 0);
                    spawn_item(rnx, rny, "id_military");
                } else if (one_in(20)) {
                    rough_circle(this, t_rubble, rnx, rny, rng(3, 6));
                }
            }
        }
        // Oh wait--let's also put radiation in any rubble
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                radiation(i, j) += (one_in(5) ? rng(1, 2) : 0);
                if (ter(i, j) == t_rubble) {
                    radiation(i, j) += rng(1, 3);
                }
            }
        }


    } else if (terrain_type == "silo") {

        if (t_above == "") { // We're on ground level
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (trig_dist(i, j, SEEX, SEEY) <= 6) {
                        ter_set(i, j, t_metal_floor);
                    } else {
                        ter_set(i, j, grass_or_dirt());
                    }
                }
            }
            switch (rng(1, 4)) { // Placement of stairs
            case 1:
                lw = 3;
                mw = 5;
                tw = 3;
                break;
            case 2:
                lw = 3;
                mw = 5;
                tw = SEEY * 2 - 4;
                break;
            case 3:
                lw = SEEX * 2 - 7;
                mw = lw;
                tw = 3;
                break;
            case 4:
                lw = SEEX * 2 - 7;
                mw = lw;
                tw = SEEY * 2 - 4;
                break;
            }
            for (int i = lw; i <= lw + 2; i++) {
                ter_set(i, tw    , t_wall_metal_h);
                ter_set(i, tw + 2, t_wall_metal_h);
            }
            ter_set(lw    , tw + 1, t_wall_metal_v);
            ter_set(lw + 1, tw + 1, t_stairs_down);
            ter_set(lw + 2, tw + 1, t_wall_metal_v);
            ter_set(mw    , tw + 1, t_door_metal_locked);
            ter_set(mw    , tw + 2, t_card_military);

        } else { // We are NOT above ground.
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    if (trig_dist(i, j, SEEX, SEEY) > 7) {
                        ter_set(i, j, t_rock);
                    } else if (trig_dist(i, j, SEEX, SEEY) > 5) {
                        ter_set(i, j, t_metal_floor);
                        if (one_in(30)) {
                            add_field(NULL, i, j, fd_nuke_gas, 2);    // NULL game; no messages
                        }
                    } else if (trig_dist(i, j, SEEX, SEEY) == 5) {
                        ter_set(i, j, t_hole);
                        add_trap(i, j, tr_ledge);
                    } else {
                        ter_set(i, j, t_missile);
                    }
                }
            }
            silo_rooms(this);
        }


    } else if (terrain_type == "silo_finale") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i == 5) {
                    if (j > 4 && j < SEEY) {
                        ter_set(i, j, t_reinforced_glass_v);
                    } else if (j == SEEY * 2 - 4) {
                        ter_set(i, j, t_door_metal_c);
                    } else {
                        ter_set(i, j, t_rock);
                    }
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        ter_set(0, 0, t_stairs_up);
        tmpcomp = add_computer(4, 5, _("Missile Controls"), 8);
        tmpcomp->add_option(_("Launch Missile"), COMPACT_MISS_LAUNCH, 10);
        tmpcomp->add_option(_("Disarm Missile"), COMPACT_MISS_DISARM,  8);
        tmpcomp->add_failure(COMPFAIL_SECUBOTS);
        tmpcomp->add_failure(COMPFAIL_DAMAGE);


    } else if (terrain_type == "temple" ||
               terrain_type == "temple_stairs") {

        if (t_above == "") { // Ground floor
            // TODO: More varieties?
            fill_background(this, t_dirt);
            square(this, t_grate, SEEX - 1, SEEY - 1, SEEX, SEEX);
            ter_set(SEEX + 1, SEEY + 1, t_pedestal_temple);
        } else { // Underground!  Shit's about to get interesting!
            // Start with all rock floor
            square(this, t_rock_floor, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);
            // We always start at the south and go north.
            // We use (g->levx / 2 + g->levz) % 5 to guarantee that rooms don't repeat.
            switch (1 + int(g->levy / 2 + g->levz) % 4) {// TODO: More varieties!

            case 1: // Flame bursts
                square(this, t_rock, 0, 0, SEEX - 1, SEEY * 2 - 1);
                square(this, t_rock, SEEX + 2, 0, SEEX * 2 - 1, SEEY * 2 - 1);
                for (int i = 2; i < SEEY * 2 - 4; i++) {
                    add_field(g, SEEX    , i, fd_fire_vent, rng(1, 3));
                    add_field(g, SEEX + 1, i, fd_fire_vent, rng(1, 3));
                }
                break;

            case 2: // Spreading water
                square(this, t_water_dp, 4, 4, 5, 5);
                add_spawn("mon_sewer_snake", 1, 4, 4);

                square(this, t_water_dp, SEEX * 2 - 5, 4, SEEX * 2 - 4, 6);
                add_spawn("mon_sewer_snake", 1, SEEX * 2 - 5, 4);

                square(this, t_water_dp, 4, SEEY * 2 - 5, 6, SEEY * 2 - 4);

                square(this, t_water_dp, SEEX * 2 - 5, SEEY * 2 - 5, SEEX * 2 - 4,
                       SEEY * 2 - 4);

                square(this, t_rock, 0, SEEY * 2 - 2, SEEX - 1, SEEY * 2 - 1);
                square(this, t_rock, SEEX + 2, SEEY * 2 - 2, SEEX * 2 - 1, SEEY * 2 - 1);
                line(this, t_grate, SEEX, 1, SEEX + 1, 1); // To drain the water
                add_trap(SEEX, SEEY * 2 - 2, tr_temple_flood);
                add_trap(SEEX + 1, SEEY * 2 - 2, tr_temple_flood);
                for (int y = 2; y < SEEY * 2 - 2; y++) {
                    for (int x = 2; x < SEEX * 2 - 2; x++) {
                        if (ter(x, y) == t_rock_floor && one_in(4)) {
                            add_trap(x, y, tr_temple_flood);
                        }
                    }
                }
                break;

            case 3: { // Flipping walls puzzle
                line(this, t_rock, 0, 0, SEEX - 1, 0);
                line(this, t_rock, SEEX + 2, 0, SEEX * 2 - 1, 0);
                line(this, t_rock, SEEX - 1, 1, SEEX - 1, 6);
                line(this, t_bars, SEEX + 2, 1, SEEX + 2, 6);
                ter_set(14, 1, t_switch_rg);
                ter_set(15, 1, t_switch_gb);
                ter_set(16, 1, t_switch_rb);
                ter_set(17, 1, t_switch_even);
                // Start with clear floors--then work backwards to the starting state
                line(this, t_floor_red,   SEEX, 1, SEEX + 1, 1);
                line(this, t_floor_green, SEEX, 2, SEEX + 1, 2);
                line(this, t_floor_blue,  SEEX, 3, SEEX + 1, 3);
                line(this, t_floor_red,   SEEX, 4, SEEX + 1, 4);
                line(this, t_floor_green, SEEX, 5, SEEX + 1, 5);
                line(this, t_floor_blue,  SEEX, 6, SEEX + 1, 6);
                // Now, randomly choose actions
                // Set up an actions vector so that there's not undue repetion
                std::vector<int> actions;
                actions.push_back(1);
                actions.push_back(2);
                actions.push_back(3);
                actions.push_back(4);
                actions.push_back(rng(1, 3));
                while (!actions.empty()) {
                    int index = rng(0, actions.size() - 1);
                    int action = actions[index];
                    actions.erase(actions.begin() + index);
                    for (int y = 1; y < 7; y++) {
                        for (int x = SEEX; x <= SEEX + 1; x++) {
                            switch (action) {
                            case 1: // Toggle RG
                                if (ter(x, y) == t_floor_red) {
                                    ter_set(x, y, t_rock_red);
                                } else if (ter(x, y) == t_rock_red) {
                                    ter_set(x, y, t_floor_red);
                                } else if (ter(x, y) == t_floor_green) {
                                    ter_set(x, y, t_rock_green);
                                } else if (ter(x, y) == t_rock_green) {
                                    ter_set(x, y, t_floor_green);
                                }
                                break;
                            case 2: // Toggle GB
                                if (ter(x, y) == t_floor_blue) {
                                    ter_set(x, y, t_rock_blue);
                                } else if (ter(x, y) == t_rock_blue) {
                                    ter_set(x, y, t_floor_blue);
                                } else if (ter(x, y) == t_floor_green) {
                                    ter_set(x, y, t_rock_green);
                                } else if (ter(x, y) == t_rock_green) {
                                    ter_set(x, y, t_floor_green);
                                }
                                break;
                            case 3: // Toggle RB
                                if (ter(x, y) == t_floor_blue) {
                                    ter_set(x, y, t_rock_blue);
                                } else if (ter(x, y) == t_rock_blue) {
                                    ter_set(x, y, t_floor_blue);
                                } else if (ter(x, y) == t_floor_red) {
                                    ter_set(x, y, t_rock_red);
                                } else if (ter(x, y) == t_rock_red) {
                                    ter_set(x, y, t_floor_red);
                                }
                                break;
                            case 4: // Toggle Even
                                if (y % 2 == 0) {
                                    if (ter(x, y) == t_floor_blue) {
                                        ter_set(x, y, t_rock_blue);
                                    } else if (ter(x, y) == t_rock_blue) {
                                        ter_set(x, y, t_floor_blue);
                                    } else if (ter(x, y) == t_floor_red) {
                                        ter_set(x, y, t_rock_red);
                                    } else if (ter(x, y) == t_rock_red) {
                                        ter_set(x, y, t_floor_red);
                                    } else if (ter(x, y) == t_floor_green) {
                                        ter_set(x, y, t_rock_green);
                                    } else if (ter(x, y) == t_rock_green) {
                                        ter_set(x, y, t_floor_green);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            break;

            case 4: { // Toggling walls maze
                square(this, t_rock,        0,            0, SEEX     - 1,            1);
                square(this, t_rock,        0, SEEY * 2 - 2, SEEX     - 1, SEEY * 2 - 1);
                square(this, t_rock,        0,            2, SEEX     - 4, SEEY * 2 - 3);
                square(this, t_rock, SEEX + 2,            0, SEEX * 2 - 1,            1);
                square(this, t_rock, SEEX + 2, SEEY * 2 - 2, SEEX * 2 - 1, SEEY * 2 - 1);
                square(this, t_rock, SEEX + 5,            2, SEEX * 2 - 1, SEEY * 2 - 3);
                int x = rng(SEEX - 1, SEEX + 2), y = 2;
                std::vector<point> path; // Path, from end to start
                while (x < SEEX - 1 || x > SEEX + 2 || y < SEEY * 2 - 2) {
                    path.push_back( point(x, y) );
                    ter_set(x, y, ter_id( rng(t_floor_red, t_floor_blue) ));
                    if (y == SEEY * 2 - 2) {
                        if (x < SEEX - 1) {
                            x++;
                        } else if (x > SEEX + 2) {
                            x--;
                        }
                    } else {
                        std::vector<point> next;
                        for (int nx = x - 1; nx <= x + 1; nx++ ) {
                            for (int ny = y; ny <= y + 1; ny++) {
                                if (ter(nx, ny) == t_rock_floor) {
                                    next.push_back( point(nx, ny) );
                                }
                            }
                        }
                        int index = rng(0, next.size() - 1);
                        x = next[index].x;
                        y = next[index].y;
                    }
                }
                // Now go backwards through path (start to finish), toggling any tiles that need
                bool toggle_red = false, toggle_green = false, toggle_blue = false;
                for (int i = path.size() - 1; i >= 0; i--) {
                    if (ter(path[i].x, path[i].y) == t_floor_red) {
                        toggle_green = !toggle_green;
                        if (toggle_red) {
                            ter_set(path[i].x, path[i].y, t_rock_red);
                        }
                    } else if (ter(path[i].x, path[i].y) == t_floor_green) {
                        toggle_blue = !toggle_blue;
                        if (toggle_green) {
                            ter_set(path[i].x, path[i].y, t_rock_green);
                        }
                    } else if (ter(path[i].x, path[i].y) == t_floor_blue) {
                        toggle_red = !toggle_red;
                        if (toggle_blue) {
                            ter_set(path[i].x, path[i].y, t_rock_blue);
                        }
                    }
                }
                // Finally, fill in the rest with random tiles, and place toggle traps
                for (int i = SEEX - 3; i <= SEEX + 4; i++) {
                    for (int j = 2; j <= SEEY * 2 - 2; j++) {
                        add_trap(i, j, tr_temple_toggle);
                        if (ter(i, j) == t_rock_floor) {
                            ter_set(i, j, ter_id( rng(t_rock_red, t_floor_blue) ));
                        }
                    }
                }
            }
            break;
            } // Done with room type switch
            // Stairs down if we need them
            if (terrain_type == "temple_stairs") {
                line(this, t_stairs_down, SEEX, 0, SEEX + 1, 0);
            }
            // Stairs at the south if t_above has stairs down.
            if (t_above == "temple_stairs") {
                line(this, t_stairs_up, SEEX, SEEY * 2 - 1, SEEX + 1, SEEY * 2 - 1);
            }

        } // Done with underground-only stuff


    } else if (terrain_type == "temple_finale") {

        fill_background(this, t_rock);
        square(this, t_rock_floor, SEEX - 1, 1, SEEX + 2, 4);
        square(this, t_rock_floor, SEEX, 5, SEEX + 1, SEEY * 2 - 1);
        line(this, t_stairs_up, SEEX, SEEY * 2 - 1, SEEX + 1, SEEY * 2 - 1);
        spawn_artifact(rng(SEEX, SEEX + 1), rng(2, 3), new_artifact(itypes), 0);
        spawn_artifact(rng(SEEX, SEEX + 1), rng(2, 3), new_artifact(itypes), 0);
        return;

    } else if (terrain_type == "sewage_treatment") {

        fill_background(this, t_floor); // Set all to floor
        line(this, t_wall_h,  0,  0, 23,  0); // Top wall
        line(this, t_window,  1,  0,  6,  0); // Its windows
        line(this, t_wall_h,  0, 23, 23, 23); // Bottom wall
        line(this, t_wall_h,  1,  5,  6,  5); // Interior wall (front office)
        line(this, t_wall_h,  1, 14,  6, 14); // Interior wall (equipment)
        line(this, t_wall_h,  1, 20,  7, 20); // Interior wall (stairs)
        line(this, t_wall_h, 14, 15, 22, 15); // Interior wall (tank)
        line(this, t_wall_v,  0,  1,  0, 22); // Left wall
        line(this, t_wall_v, 23,  1, 23, 22); // Right wall
        line(this, t_wall_v,  7,  1,  7,  5); // Interior wall (front office)
        line(this, t_wall_v,  7, 14,  7, 19); // Interior wall (stairs)
        line(this, t_wall_v,  4, 15,  4, 19); // Interior wall (mid-stairs)
        line(this, t_wall_v, 14, 15, 14, 20); // Interior wall (tank)
        line(this, t_wall_glass_v,  7,  6,  7, 13); // Interior glass (equipment)
        line(this, t_wall_glass_h,  8, 20, 13, 20); // Interior glass (flow)
        line_furn(this, f_counter,  1,  3,  3,  3); // Desk (front office);
        line_furn(this, f_counter,  1,  6,  1, 13); // Counter (equipment);
        // Central tanks:
        square(this, t_sewage, 10,  3, 13,  6);
        square(this, t_sewage, 17,  3, 20,  6);
        square(this, t_sewage, 10, 10, 13, 13);
        square(this, t_sewage, 17, 10, 20, 13);
        // Drainage tank
        square(this, t_sewage, 16, 16, 21, 18);
        square(this, t_grate,  18, 16, 19, 17);
        line(this, t_sewage, 17, 19, 20, 19);
        line(this, t_sewage, 18, 20, 19, 20);
        line(this, t_sewage,  2, 21, 19, 21);
        line(this, t_sewage,  2, 22, 19, 22);
        // Pipes and pumps
        line(this, t_sewage_pipe,  1, 15,  1, 19);
        line(this, t_sewage_pump,  1, 21,  1, 22);
        // Stairs down
        ter_set(2, 15, t_stairs_down);
        // Now place doors
        ter_set(rng(2, 5), 0, t_door_c);
        ter_set(rng(3, 5), 5, t_door_c);
        ter_set(5, 14, t_door_c);
        ter_set(7, rng(15, 17), t_door_c);
        ter_set(14, rng(17, 19), t_door_c);
        if (one_in(3)) { // back door
            ter_set(23, rng(19, 22), t_door_locked);
        }
        ter_set(4, 19, t_door_metal_locked);
        ter_set(2, 19, t_console);
        ter_set(6, 19, t_console);
        // Computers to unlock stair room, and items
        tmpcomp = add_computer(2, 19, _("EnviroCom OS v2.03"), 1);
        tmpcomp->add_option(_("Unlock stairs"), COMPACT_OPEN, 0);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);

        tmpcomp = add_computer(6, 19, _("EnviroCom OS v2.03"), 1);
        tmpcomp->add_option(_("Unlock stairs"), COMPACT_OPEN, 0);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        place_items("sewage_plant", 80, 1, 6, 1, 13, false, 0);


    } else if (terrain_type == "sewage_treatment_hub") {
        // Stairs up, center of 3x3 of treatment_below

        fill_background(this, t_rock_floor);
        // Top & left walls; right & bottom are handled by adjacent terrain
        line(this, t_wall_h,  0,  0, 23,  0);
        line(this, t_wall_v,  0,  1,  0, 23);
        // Top-left room
        line(this, t_wall_v,  8,  1,  8,  8);
        line(this, t_wall_h,  1,  9,  9,  9);
        line(this, t_wall_glass_h, rng(1, 3), 9, rng(4, 7), 9);
        ter_set(2, 15, t_stairs_up);
        ter_set(8, 8, t_door_c);
        ter_set(3, 0, t_door_c);

        // Bottom-left room - stairs and equipment
        line(this, t_wall_h,  1, 14,  8, 14);
        line(this, t_wall_glass_h, rng(1, 3), 14, rng(5, 8), 14);
        line(this, t_wall_v,  9, 14,  9, 23);
        line(this, t_wall_glass_v, 9, 16, 9, 19);
        square_furn(this, f_counter, 5, 16, 6, 20);
        place_items("sewage_plant", 80, 5, 16, 6, 20, false, 0);
        ter_set(0, 20, t_door_c);
        ter_set(9, 20, t_door_c);

        // Bottom-right room
        line(this, t_wall_v, 14, 19, 14, 23);
        line(this, t_wall_h, 14, 18, 19, 18);
        line(this, t_wall_h, 21, 14, 23, 14);
        ter_set(14, 18, t_wall_h);
        ter_set(14, 20, t_door_c);
        ter_set(15, 18, t_door_c);
        line(this, t_wall_v, 20, 15, 20, 18);

        // Tanks and their content
        for (int i = 9; i <= 16; i += 7) {
            for (int j = 2; j <= 9; j += 7) {
                square(this, t_rock, i, j, i + 5, j + 5);
                square(this, t_sewage, i + 1, j + 1, i + 4, j + 4);
            }
        }
        square(this, t_rock, 16, 15, 19, 17); // Wall around sewage from above
        square(this, t_rock, 10, 15, 14, 17); // Extra walls for southward flow
        // Flow in from north, east, and west always connects to the corresponding tank
        square(this, t_sewage, 10,  0, 13,  2); // North -> NE tank
        square(this, t_sewage, 21, 10, 23, 13); // East  -> SE tank
        square(this, t_sewage,  0, 10,  9, 13); // West  -> SW tank
        // Flow from south may go to SW tank or SE tank
        square(this, t_sewage, 10, 16, 13, 23);
        if (one_in(2)) { // To SW tank
            square(this, t_sewage, 10, 14, 13, 17);
            // Then, flow from above may be either to flow from south, to SE tank, or both
            switch (rng(1, 5)) {
            case 1:
            case 2: // To flow from south
                square(this, t_sewage, 14, 16, 19, 17);
                line(this, t_bridge, 15, 16, 15, 17);
                if (!one_in(4)) {
                    line(this, t_wall_glass_h, 16, 18, 19, 18);    // Viewing window
                }
                break;
            case 3:
            case 4: // To SE tank
                square(this, t_sewage, 18, 14, 19, 17);
                if (!one_in(4)) {
                    line(this, t_wall_glass_v, 20, 15, 20, 17);    // Viewing window
                }
                break;
            case 5: // Both!
                square(this, t_sewage, 14, 16, 19, 17);
                square(this, t_sewage, 18, 14, 19, 17);
                line(this, t_bridge, 15, 16, 15, 17);
                if (!one_in(4)) {
                    line(this, t_wall_glass_h, 16, 18, 19, 18);    // Viewing window
                }
                if (!one_in(4)) {
                    line(this, t_wall_glass_v, 20, 15, 20, 17);    // Viewing window
                }
                break;
            }
        } else { // To SE tank, via flow from above
            square(this, t_sewage, 14, 16, 19, 17);
            square(this, t_sewage, 18, 14, 19, 17);
            line(this, t_bridge, 15, 16, 15, 17);
            if (!one_in(4)) {
                line(this, t_wall_glass_h, 16, 18, 19, 18);    // Viewing window
            }
            if (!one_in(4)) {
                line(this, t_wall_glass_v, 20, 15, 20, 17);    // Viewing window
            }
        }

        // Next, determine how the tanks interconnect.
        rn = rng(1, 4); // Which of the 4 possible connections is missing?
        if (rn != 1) {
            line(this, t_sewage, 14,  4, 14,  5);
            line(this, t_bridge, 15,  4, 15,  5);
            line(this, t_sewage, 16,  4, 16,  5);
        }
        if (rn != 2) {
            line(this, t_sewage, 18,  7, 19,  7);
            line(this, t_bridge, 18,  8, 19,  8);
            line(this, t_sewage, 18,  9, 19,  9);
        }
        if (rn != 3) {
            line(this, t_sewage, 14, 11, 14, 12);
            line(this, t_bridge, 15, 11, 15, 12);
            line(this, t_sewage, 16, 11, 16, 12);
        }
        if (rn != 4) {
            line(this, t_sewage, 11,  7, 12,  7);
            line(this, t_bridge, 11,  8, 12,  8);
            line(this, t_sewage, 11,  9, 12,  9);
        }
        // Bridge connecting bottom two rooms
        line(this, t_bridge, 10, 20, 13, 20);
        // Possibility of extra equipment shelves
        if (!one_in(3)) {
            line_furn(this, f_rack, 23, 1, 23, 4);
            place_items("sewage_plant", 60, 23, 1, 23, 4, false, 0);
        }


        // Finally, choose what the top-left and bottom-right rooms do.
        if (one_in(2)) { // Upper left is sampling, lower right valuable finds
            // Upper left...
            line(this, t_wall_h, 1, 3, 2, 3);
            line(this, t_wall_h, 1, 5, 2, 5);
            line(this, t_wall_h, 1, 7, 2, 7);
            ter_set(1, 4, t_sewage_pump);
            furn_set(2, 4, f_counter);
            ter_set(1, 6, t_sewage_pump);
            furn_set(2, 6, f_counter);
            ter_set(1, 2, t_console);
            tmpcomp = add_computer(1, 2, _("EnviroCom OS v2.03"), 0);
            tmpcomp->add_option(_("Download Sewer Maps"), COMPACT_MAP_SEWER, 0);
            tmpcomp->add_option(_("Divert sample"), COMPACT_SAMPLE, 3);
            tmpcomp->add_failure(COMPFAIL_PUMP_EXPLODE);
            tmpcomp->add_failure(COMPFAIL_PUMP_LEAK);
            // Lower right...
            line_furn(this, f_counter, 15, 23, 22, 23);
            place_items("sewer", 65, 15, 23, 22, 23, false, 0);
            line_furn(this, f_counter, 23, 15, 23, 19);
            place_items("sewer", 65, 23, 15, 23, 19, false, 0);
        } else { // Upper left is valuable finds, lower right is sampling
            // Upper left...
            line_furn(this, f_counter,     1, 1, 1, 7);
            place_items("sewer", 65, 1, 1, 1, 7, false, 0);
            line_furn(this, f_counter,     7, 1, 7, 7);
            place_items("sewer", 65, 7, 1, 7, 7, false, 0);
            // Lower right...
            line(this, t_wall_v, 17, 22, 17, 23);
            line(this, t_wall_v, 19, 22, 19, 23);
            line(this, t_wall_v, 21, 22, 21, 23);
            ter_set(18, 23, t_sewage_pump);
            furn_set(18, 22, f_counter);
            ter_set(20, 23, t_sewage_pump);
            furn_set(20, 22, f_counter);
            ter_set(16, 23, t_console);
            tmpcomp = add_computer(16, 23, _("EnviroCom OS v2.03"), 0);
            tmpcomp->add_option(_("Download Sewer Maps"), COMPACT_MAP_SEWER, 0);
            tmpcomp->add_option(_("Divert sample"), COMPACT_SAMPLE, 3);
            tmpcomp->add_failure(COMPFAIL_PUMP_EXPLODE);
            tmpcomp->add_failure(COMPFAIL_PUMP_LEAK);
        }


    } else if (terrain_type == "sewage_treatment_under") {

        fill_background(this, t_floor);

        if (t_north == "sewage_treatment_under" || t_north == "sewage_treatment_hub" ||
            (is_ot_type("sewer", t_north) && connects_to(t_north, 2))) {
            if (t_north == "sewage_treatment_under" ||
                t_north == "sewage_treatment_hub") {
                line(this, t_wall_h,  0,  0, 23,  0);
                ter_set(3, 0, t_door_c);
            }
            n_fac = 1;
            square(this, t_sewage, 10, 0, 13, 13);
        }

        if (t_east == "sewage_treatment_under" ||
            t_east == "sewage_treatment_hub" ||
            (is_ot_type("sewer", t_east) && connects_to(t_east, 3))) {
            e_fac = 1;
            square(this, t_sewage, 10, 10, 23, 13);
        }

        if (t_south == "sewage_treatment_under" ||
            t_south == "sewage_treatment_hub" ||
            (is_ot_type("sewer", t_south) && connects_to(t_south, 0))) {
            s_fac = 1;
            square(this, t_sewage, 10, 10, 13, 23);
        }

        if (t_west == "sewage_treatment_under" ||
            t_west == "sewage_treatment_hub" ||
            (is_ot_type("sewer", t_west) && connects_to(t_west, 1))) {
            if (t_west == "sewage_treatment_under" ||
                t_west == "sewage_treatment_hub") {
                line(this, t_wall_v,  0,  1,  0, 23);
                ter_set(0, 20, t_door_c);
            }
            w_fac = 1;
            square(this, t_sewage,  0, 10, 13, 13);
        }


    } else if (terrain_type == "mine_entrance") {

        fill_background(this, &grass_or_dirt);
        int tries = 0;
        bool build_shaft = true;
        do {
            int x1 = rng(1, SEEX * 2 - 10), y1 = rng(1, SEEY * 2 - 10);
            int x2 = x1 + rng(4, 9),        y2 = y1 + rng(4, 9);
            if (build_shaft) {
                build_mine_room(this, room_mine_shaft, x1, y1, x2, y2);
                build_shaft = false;
            } else {
                bool okay = true;
                for (int x = x1 - 1; x <= x2 + 1 && okay; x++) {
                    for (int y = y1 - 1; y <= y2 + 1 && okay; y++) {
                        if (ter(x, y) != t_grass && ter(x, y) != t_dirt) {
                            okay = false;
                        }
                    }
                }
                if (okay) {
                    room_type type = room_type( rng(room_mine_office, room_mine_housing) );
                    build_mine_room(this, type, x1, y1, x2, y2);
                    tries = 0;
                } else {
                    tries++;
                }
            }
        } while (tries < 5);
        int ladderx = rng(0, SEEX * 2 - 1), laddery = rng(0, SEEY * 2 - 1);
        while (ter(ladderx, laddery) != t_dirt && ter(ladderx, laddery) != t_grass) {
            ladderx = rng(0, SEEX * 2 - 1);
            laddery = rng(0, SEEY * 2 - 1);
        }
        ter_set(ladderx, laddery, t_manhole_cover);


    } else if (terrain_type == "mine_shaft") {
        // Not intended to actually be inhabited!

        fill_background(this, t_rock);
        square(this, t_hole, SEEX - 3, SEEY - 3, SEEX + 2, SEEY + 2);
        line(this, t_grate, SEEX - 3, SEEY - 4, SEEX + 2, SEEY - 4);
        ter_set(SEEX - 3, SEEY - 5, t_ladder_up);
        ter_set(SEEX + 2, SEEY - 5, t_ladder_down);
        rotate(rng(0, 3));


    } else if (terrain_type == "mine" ||
               terrain_type == "mine_down") {

        if (is_ot_type("mine", t_north)) {
            n_fac = (one_in(10) ? 0 : -2);
        } else {
            n_fac = 4;
        }
        if (is_ot_type("mine", t_east)) {
            e_fac = (one_in(10) ? 0 : -2);
        } else {
            e_fac = 4;
        }
        if (is_ot_type("mine", t_south)) {
            s_fac = (one_in(10) ? 0 : -2);
        } else {
            s_fac = 4;
        }
        if (is_ot_type("mine", t_west)) {
            w_fac = (one_in(10) ? 0 : -2);
        } else {
            w_fac = 4;
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i >= w_fac + rng(0, 2) && i <= SEEX * 2 - 1 - e_fac - rng(0, 2) &&
                    j >= n_fac + rng(0, 2) && j <= SEEY * 2 - 1 - s_fac - rng(0, 2) &&
                    i + j >= 4 && (SEEX * 2 - i) + (SEEY * 2 - j) >= 6  ) {
                    ter_set(i, j, t_rock_floor);
                } else {
                    ter_set(i, j, t_rock);
                }
            }
        }

        if (t_above == "mine_shaft") { // We need the entrance room
            square(this, t_floor, 10, 10, 15, 15);
            line(this, t_wall_h,  9,  9, 16,  9);
            line(this, t_wall_h,  9, 16, 16, 16);
            line(this, t_wall_v,  9, 10,  9, 15);
            line(this, t_wall_v, 16, 10, 16, 15);
            line(this, t_wall_h, 10, 11, 12, 11);
            ter_set(10, 10, t_elevator_control);
            ter_set(11, 10, t_elevator);
            ter_set(10, 12, t_ladder_up);
            line_furn(this, f_counter, 10, 15, 15, 15);
            place_items("mine_equipment", 86, 10, 15, 15, 15, false, 0);
            if (one_in(2)) {
                ter_set(9, 12, t_door_c);
            } else {
                ter_set(16, 12, t_door_c);
            }

        } else { // Not an entrance; maybe some hazards!
            switch( rng(0, 6) ) {
            case 0:
                break; // Nothing!  Lucky!

            case 1: { // Toxic gas
                int cx = rng(9, 14), cy = rng(9, 14);
                ter_set(cx, cy, t_rock);
                add_field(g, cx, cy, fd_gas_vent, 1);
            }
            break;

            case 2: { // Lava
                int x1 = rng(6, SEEX),                y1 = rng(6, SEEY),
                    x2 = rng(SEEX + 1, SEEX * 2 - 7), y2 = rng(SEEY + 1, SEEY * 2 - 7);
                int num = rng(2, 4);
                for (int i = 0; i < num; i++) {
                    int lx1 = x1 + rng(-1, 1), lx2 = x2 + rng(-1, 1),
                        ly1 = y1 + rng(-1, 1), ly2 = y2 + rng(-1, 1);
                    line(this, t_lava, lx1, ly1, lx2, ly2);
                }
            }
            break;

            case 3: { // Wrecked equipment
                int x = rng(9, 14), y = rng(9, 14);
                for (int i = x - 3; i < x + 3; i++) {
                    for (int j = y - 3; j < y + 3; j++) {
                        if (!one_in(4)) {
                            ter_set(i, j, t_wreckage);
                        }
                    }
                }
                place_items("wreckage", 70, x - 3, y - 3, x + 2, y + 2, false, 0);
            }
            break;

            case 4: { // Dead miners
                int num_bodies = rng(4, 8);
                for (int i = 0; i < num_bodies; i++) {
                    int tries = 0;
                    point body;
                    do {
                        body = point(-1, -1);
                        int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
                        if (move_cost(x, y) == 2) {
                            body = point(x, y);
                        } else {
                            tries++;
                        }
                    } while (body.x == -1 && tries < 10);
                    if (tries < 10) {
                        item miner;
                        miner.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                        add_item(body.x, body.y, miner);
                        place_items("mine_equipment", 60, body.x, body.y, body.x, body.y,
                                    false, 0);
                    }
                }
            }
            break;

            case 5: { // Dark worm!
                int num_worms = rng(1, 5);
                for (int i = 0; i < num_worms; i++) {
                    std::vector<direction> sides;
                    if (n_fac == 6) {
                        sides.push_back(NORTH);
                    }
                    if (e_fac == 6) {
                        sides.push_back(EAST);
                    }
                    if (s_fac == 6) {
                        sides.push_back(SOUTH);
                    }
                    if (w_fac == 6) {
                        sides.push_back(WEST);
                    }
                    if (sides.empty()) {
                        add_spawn("mon_dark_wyrm", 1, SEEX, SEEY);
                        i = num_worms;
                    } else {
                        direction side = sides[rng(0, sides.size() - 1)];
                        point p;
                        switch (side) {
                        case NORTH:
                            p = point(rng(1, SEEX * 2 - 2), rng(1, 5)           );
                            break;
                        case EAST:
                            p = point(SEEX * 2 - rng(2, 6), rng(1, SEEY * 2 - 2));
                            break;
                        case SOUTH:
                            p = point(rng(1, SEEX * 2 - 2), SEEY * 2 - rng(2, 6));
                            break;
                        case WEST:
                            p = point(rng(1, 5)           , rng(1, SEEY * 2 - 2));
                            break;
                        default:
                            break;
                        }
                        ter_set(p.x, p.y, t_rock_floor);
                        add_spawn("mon_dark_wyrm", 1, p.x, p.y);
                    }
                }
            }
            break;

            case 6: { // Spiral
                int orx = rng(SEEX - 4, SEEX), ory = rng(SEEY - 4, SEEY);
                line(this, t_rock, orx    , ory    , orx + 5, ory    );
                line(this, t_rock, orx + 5, ory    , orx + 5, ory + 5);
                line(this, t_rock, orx + 1, ory + 5, orx + 5, ory + 5);
                line(this, t_rock, orx + 1, ory + 2, orx + 1, ory + 4);
                line(this, t_rock, orx + 1, ory + 2, orx + 3, ory + 2);
                ter_set(orx + 3, ory + 3, t_rock);
                item miner;
                miner.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                add_item(orx + 2, ory + 3, miner);
                place_items("mine_equipment", 60, orx + 2, ory + 3, orx + 2, ory + 3,
                            false, 0);
            }
            break;
            }
        }

        if (terrain_type == "mine_down") { // Don't forget to build a slope down!
            std::vector<direction> open;
            if (n_fac == 4) {
                open.push_back(NORTH);
            }
            if (e_fac == 4) {
                open.push_back(EAST);
            }
            if (s_fac == 4) {
                open.push_back(SOUTH);
            }
            if (w_fac == 4) {
                open.push_back(WEST);
            }

            if (open.empty()) { // We'll have to build it in the center
                int tries = 0;
                point p;
                bool okay = true;
                do {
                    p.x = rng(SEEX - 6, SEEX + 1);
                    p.y = rng(SEEY - 6, SEEY + 1);
                    okay = true;
                    for (int i = p.x; i <= p.x + 5 && okay; i++) {
                        for (int j = p.y; j <= p.y + 5 && okay; j++) {
                            if (ter(i, j) != t_rock_floor) {
                                okay = false;
                            }
                        }
                    }
                    if (!okay) {
                        tries++;
                    }
                } while (!okay && tries < 10);
                if (tries == 10) { // Clear the area around the slope down
                    square(this, t_rock_floor, p.x, p.y, p.x + 5, p.y + 5);
                }
                square(this, t_slope_down, p.x + 1, p.y + 1, p.x + 2, p.y + 2);

            } else { // We can build against a wall
                direction side = open[rng(0, open.size() - 1)];
                switch (side) {
                case NORTH:
                    square(this, t_rock_floor, SEEX - 3, 6, SEEX + 2, SEEY);
                    line(this, t_slope_down, SEEX - 2, 6, SEEX + 1, 6);
                    break;
                case EAST:
                    square(this, t_rock_floor, SEEX + 1, SEEY - 3, SEEX * 2 - 7, SEEY + 2);
                    line(this, t_slope_down, SEEX * 2 - 7, SEEY - 2, SEEX * 2 - 7, SEEY + 1);
                    break;
                case SOUTH:
                    square(this, t_rock_floor, SEEX - 3, SEEY + 1, SEEX + 2, SEEY * 2 - 7);
                    line(this, t_slope_down, SEEX - 2, SEEY * 2 - 7, SEEX + 1, SEEY * 2 - 7);
                    break;
                case WEST:
                    square(this, t_rock_floor, 6, SEEY - 3, SEEX, SEEY + 2);
                    line(this, t_slope_down, 6, SEEY - 2, 6, SEEY + 1);
                    break;
                default:
                    break;
                }
            }
        } // Done building a slope down

        if (t_above == "mine_down") {  // Don't forget to build a slope up!
            std::vector<direction> open;
            if (n_fac == 6 && ter(SEEX, 6) != t_slope_down) {
                open.push_back(NORTH);
            }
            if (e_fac == 6 && ter(SEEX * 2 - 7, SEEY) != t_slope_down) {
                open.push_back(EAST);
            }
            if (s_fac == 6 && ter(SEEX, SEEY * 2 - 7) != t_slope_down) {
                open.push_back(SOUTH);
            }
            if (w_fac == 6 && ter(6, SEEY) != t_slope_down) {
                open.push_back(WEST);
            }

            if (open.empty()) { // We'll have to build it in the center
                int tries = 0;
                point p;
                bool okay = true;
                do {
                    p.x = rng(SEEX - 6, SEEX + 1);
                    p.y = rng(SEEY - 6, SEEY + 1);
                    okay = true;
                    for (int i = p.x; i <= p.x + 5 && okay; i++) {
                        for (int j = p.y; j <= p.y + 5 && okay; j++) {
                            if (ter(i, j) != t_rock_floor) {
                                okay = false;
                            }
                        }
                    }
                    if (!okay) {
                        tries++;
                    }
                } while (!okay && tries < 10);
                if (tries == 10) { // Clear the area around the slope down
                    square(this, t_rock_floor, p.x, p.y, p.x + 5, p.y + 5);
                }
                square(this, t_slope_up, p.x + 1, p.y + 1, p.x + 2, p.y + 2);

            } else { // We can build against a wall
                direction side = open[rng(0, open.size() - 1)];
                switch (side) {
                case NORTH:
                    line(this, t_slope_up, SEEX - 2, 6, SEEX + 1, 6);
                    break;
                case EAST:
                    line(this, t_slope_up, SEEX * 2 - 7, SEEY - 2, SEEX * 2 - 7, SEEY + 1);
                    break;
                case SOUTH:
                    line(this, t_slope_up, SEEX - 2, SEEY * 2 - 7, SEEX + 1, SEEY * 2 - 7);
                    break;
                case WEST:
                    line(this, t_slope_up, 6, SEEY - 2, 6, SEEY + 1);
                    break;
                default:
                    break;
                }
            }
        } // Done building a slope up


    } else if (terrain_type == "mine_finale") {

        // Set up the basic chamber
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i > rng(1, 3) && i < SEEX * 2 - rng(2, 4) &&
                    j > rng(1, 3) && j < SEEY * 2 - rng(2, 4)   ) {
                    ter_set(i, j, t_rock_floor);
                } else {
                    ter_set(i, j, t_rock);
                }
            }
        }
        std::vector<direction> face; // Which walls are solid, and can be a facing?
        // Now draw the entrance(s)
        if (t_north == "mine") {
            square(this, t_rock_floor, SEEX, 0, SEEX + 1, 3);
        } else {
            face.push_back(NORTH);
        }

        if (t_east  == "mine") {
            square(this, t_rock_floor, SEEX * 2 - 4, SEEY, SEEX * 2 - 1, SEEY + 1);
        } else {
            face.push_back(EAST);
        }

        if (t_south == "mine") {
            square(this, t_rock_floor, SEEX, SEEY * 2 - 4, SEEX + 1, SEEY * 2 - 1);
        } else {
            face.push_back(SOUTH);
        }

        if (t_west  == "mine") {
            square(this, t_rock_floor, 0, SEEY, 3, SEEY + 1);
        } else {
            face.push_back(WEST);
        }

        // Now, pick and generate a type of finale!
        if (face.empty()) {
            rn = rng(1, 3);    // Amigara fault is not valid
        } else {
            rn = rng(1, 4);
        }

        switch (rn) {
        case 1: { // Wyrms
            int x = rng(SEEX, SEEX + 1), y = rng(SEEY, SEEY + 1);
            ter_set(x, y, t_pedestal_wyrm);
            spawn_item(x, y, "petrified_eye");
        }
        break; // That's it!  game::examine handles the pedestal/wyrm spawns

        case 2: { // The Thing dog
            item miner;
            miner.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
            int num_bodies = rng(4, 8);
            for (int i = 0; i < num_bodies; i++) {
                int x = rng(4, SEEX * 2 - 5), y = rng(4, SEEX * 2 - 5);
                add_item(x, y, miner);
                place_items("mine_equipment", 60, x, y, x, y, false, 0);
            }
            add_spawn("mon_dog_thing", 1, rng(SEEX, SEEX + 1), rng(SEEX, SEEX + 1), true);
            spawn_artifact(rng(SEEX, SEEX + 1), rng(SEEY, SEEY + 1), new_artifact(itypes), 0);
        }
        break;

        case 3: { // Spiral down
            line(this, t_rock,  5,  5,  5, 18);
            line(this, t_rock,  5,  5, 18,  5);
            line(this, t_rock, 18,  5, 18, 18);
            line(this, t_rock,  8, 18, 18, 18);
            line(this, t_rock,  8,  8,  8, 18);
            line(this, t_rock,  8,  8, 15,  8);
            line(this, t_rock, 15,  8, 15, 15);
            line(this, t_rock, 10, 15, 15, 15);
            line(this, t_rock, 10, 10, 10, 15);
            line(this, t_rock, 10, 10, 13, 10);
            line(this, t_rock, 13, 10, 13, 13);
            ter_set(12, 13, t_rock);
            ter_set(12, 12, t_slope_down);
            ter_set(12, 11, t_slope_down);
        }
        break;

        case 4: { // Amigara fault
            direction fault = face[rng(0, face.size() - 1)];
            // Construct the fault on the appropriate face
            switch (fault) {
            case NORTH:
                square(this, t_rock, 0, 0, SEEX * 2 - 1, 4);
                line(this, t_fault, 4, 4, SEEX * 2 - 5, 4);
                break;
            case EAST:
                square(this, t_rock, SEEX * 2 - 5, 0, SEEY * 2 - 1, SEEX * 2 - 1);
                line(this, t_fault, SEEX * 2 - 5, 4, SEEX * 2 - 5, SEEY * 2 - 5);
                break;
            case SOUTH:
                square(this, t_rock, 0, SEEY * 2 - 5, SEEX * 2 - 1, SEEY * 2 - 1);
                line(this, t_fault, 4, SEEY * 2 - 5, SEEX * 2 - 5, SEEY * 2 - 5);
                break;
            case WEST:
                square(this, t_rock, 0, 0, 4, SEEY * 2 - 1);
                line(this, t_fault, 4, 4, 4, SEEY * 2 - 5);
                break;
            default:
                break;
            }

            ter_set(SEEX, SEEY, t_console);
            tmpcomp = add_computer(SEEX, SEEY, _("NEPowerOS"), 0);
            tmpcomp->add_option(_("Read Logs"), COMPACT_AMIGARA_LOG, 0);
            tmpcomp->add_option(_("Initiate Tremors"), COMPACT_AMIGARA_START, 4);
            tmpcomp->add_failure(COMPFAIL_AMIGARA);
        }
        break;
        }


    } else if (terrain_type == "spiral_hub") {

        fill_background(this, t_rock_floor);
        line(this, t_rock, 23,  0, 23, 23);
        line(this, t_rock,  2, 23, 23, 23);
        line(this, t_rock,  2,  4,  2, 23);
        line(this, t_rock,  2,  4, 18,  4);
        line(this, t_rock, 18,  4, 18, 18); // bad
        line(this, t_rock,  6, 18, 18, 18);
        line(this, t_rock,  6,  7,  6, 18);
        line(this, t_rock,  6,  7, 15,  7);
        line(this, t_rock, 15,  7, 15, 15);
        line(this, t_rock,  8, 15, 15, 15);
        line(this, t_rock,  8,  9,  8, 15);
        line(this, t_rock,  8,  9, 13,  9);
        line(this, t_rock, 13,  9, 13, 13);
        line(this, t_rock, 10, 13, 13, 13);
        line(this, t_rock, 10, 11, 10, 13);
        square(this, t_slope_up, 11, 11, 12, 12);
        rotate(rng(0, 3));


    } else if (terrain_type == "spiral") {

        fill_background(this, t_rock_floor);
        const int num_spiral = rng(1, 4);
        std::list<point> offsets;
        const int spiral_width = 8;
        // Divide the room into quadrants, and place a spiral origin
        // at a random offset within each quadrant.
        for( int x = 0; x < 2; ++x ) {
            for( int y = 0; y < 2; ++y ) {
                const int x_jitter = rng(0, SEEX - spiral_width);
                const int y_jitter = rng(0, SEEY - spiral_width);
                offsets.push_back( point((x * SEEX) + x_jitter,
                                         (y * SEEY) + y_jitter) );
            }
        }

        // Randomly place from 1 - 4 of the spirals at the chosen offsets.
        for (int i = 0; i < num_spiral; i++) {
            std::list<point>::iterator chosen_point = offsets.begin();
            std::advance( chosen_point, rng(0, offsets.size() - 1) );
            const int orx = chosen_point->x;
            const int ory = chosen_point->y;
            offsets.erase( chosen_point );

            line(this, t_rock, orx    , ory    , orx + 5, ory    );
            line(this, t_rock, orx + 5, ory    , orx + 5, ory + 5);
            line(this, t_rock, orx + 1, ory + 5, orx + 5, ory + 5);
            line(this, t_rock, orx + 1, ory + 2, orx + 1, ory + 4);
            line(this, t_rock, orx + 1, ory + 2, orx + 3, ory + 2);
            ter_set(orx + 3, ory + 3, t_rock);
            ter_set(orx + 2, ory + 3, t_rock_floor);
            place_items("spiral", 60, orx + 2, ory + 3, orx + 2, ory + 3, false, 0);
        }


    } else if (terrain_type == "radio_tower") {

        fill_background(this, &grass_or_dirt);
        lw = rng(1, SEEX * 2 - 2);
        tw = rng(1, SEEY * 2 - 2);
        for (int i = lw; i < lw + 4; i++) {
            for (int j = tw; j < tw + 4; j++) {
                ter_set(i, j, t_radio_tower);
            }
        }
        rw = -1;
        bw = -1;
        if (lw <= 4) {
            rw = rng(lw + 5, 10);
        } else if (lw >= 16) {
            rw = rng(3, lw - 13);
        }
        if (tw <= 3) {
            bw = rng(tw + 5, 10);
        } else if (tw >= 16) {
            bw = rng(3, tw - 7);
        }
        if (rw != -1 && bw != -1) {
            for (int i = rw; i < rw + 12; i++) {
                for (int j = bw; j < bw + 6; j++) {
                    if (j == bw || j == bw + 5) {
                        ter_set(i, j, t_wall_h);
                    } else if (i == rw || i == rw + 11) {
                        ter_set(i, j, t_wall_v);
                    } else if (j == bw + 1) {
                        set(i, j, t_floor, f_counter);
                    } else {
                        ter_set(i, j, t_floor);
                    }
                }
            }
            cw = rng(rw + 2, rw + 8);
            ter_set(cw, bw + 5, t_window);
            ter_set(cw + 1, bw + 5, t_window);
            ter_set(rng(rw + 2, rw + 8), bw + 5, t_door_c);
            ter_set(rng(rw + 2, rw + 8), bw + 1, t_radio_controls);
            place_items("radio", 60, rw + 1, bw + 2, rw + 10, bw + 4, true, 0);
        } else { // No control room... simple controls near the tower
            ter_set(rng(lw, lw + 3), tw + 4, t_radio_controls);
        }


    } else if (is_ot_type("station_radio", terrain_type)) {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        //Eventually the northern shed will house the main breaker or generator that must be activated prior to transmitting.
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
        FffffffffffffF  \n\
        F____________F  \n\
   |----|______&&&&__F  \n\
   |....=______&&&&__F  \n\
   |x.ll|______&&&&__F  \n\
   |----|______&&&&__F  \n\
        F____________F  \n\
 |--------|__________G  \n\
 |tS|eSc.r|__________F  \n\
 w..+.....=__________F  \n\
 |-----|..|----------|  \n\
 |..doo|..|..dW..h...|  \n\
 w..h..|..D.hxW.c6c..|  \n\
 |a....|..|...+......|  \n\
 |--+--|..|-----WWW--|  \n\
 |.+.................|  \n\
 |l|..............ch.|  \n\
 |-|+--|--+--|....c..|  \n\
 |o....|....o|--==-w-|  \n\
 |o.d..|..d.o|  ss      \n\
 |o.h..|..h..|  ss      \n\
 |-www-|-www-|  ss      \n\
                ss      \n",
                                   mapf::basic_bind(". - | 6 a r + = D W w t S e o h c d x l F f _ & G s", t_floor, t_wall_h, t_wall_v,
                                           t_console, t_floor,    t_floor,    t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_window_alarm, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_console_broken, t_floor,  t_chainfence_v, t_chainfence_h, t_pavement, t_radio_tower,
                                           t_chaingate_l, t_sidewalk),
                                   mapf::basic_bind(". - | 6 a r + = D W w t S e o h c d x l F f _ & G s", f_null,  f_null,   f_null,
                                           f_null,    f_armchair, f_trashcan, f_null,   f_null,              f_null,        f_null,   f_null,
                                           f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_null,           f_locker,
                                           f_null,         f_null,         f_null,     f_null,        f_null,        f_null));
        tmpcomp = add_computer(17, 13, _("Broadcasting Control"), 0);
        tmpcomp->add_option(_("ERROR:  SIGNAL DISCONNECT"), COMPACT_TOWER_UNRESPONSIVE, 0);
        spawn_item(18, 13, "record_weather");
        place_items("novels", 70,  5,  12, 6,  12, false, 0);
        place_items("novels", 70,  2,  21, 2,  19, false, 0);
        place_items("novels", 70,  12,  19, 12,  20, false, 0);
        place_items("fridge", 70,  5,  9, 7,  9, false, 0);
        place_items("fridge", 20,  5,  9, 7,  9, false, 0);
        place_items("fridge", 10,  5,  9, 7,  9, false, 0);
        place_items("cleaning", 70,  2,  16, 2,  17, false, 0);
        place_items("electronics", 80,  6,  5, 7,  5, false, 0);
        if (terrain_type == "station_radio_east") {
            rotate(3);
        }
        if (terrain_type == "station_radio_north") {
            rotate(2);
        }
        if (terrain_type == "station_radio_west") {
            rotate(1);
        }


    } else if (terrain_type == "public_works_entrance") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
 f  ____________________\n\
 f  ____________________\n\
 f  ___________    sssss\n\
 f  ___________    sssss\n\
 f  ___________  |--D---\n\
 f  ___________  |....|.\n\
 f  ___________  |....|.\n\
 f  ___________  |lttl|.\n\
 fFFF_________FFF|----|.\n\
    ___________ss|..h.|.\n\
    ___________xs|....+.\n\
    ___________sswddd.|.\n\
    ___________ssw.hd.|.\n\
    ___________ss|l...|.\n\
    ___________ss|-ww-|-\n\
    ___________sssssssss\n\
    ____________,_____,_\n\
    ____________,_____,_\n\
    ____________,_____,_\n\
    ____________,_____,_\n\
    ____________________\n\
    ____________________\n\
    ____________________\n\
    ____________________\n",
                                   mapf::basic_bind("P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", t_floor,      t_floor,
                                           t_grate, t_pavement_y, t_pavement, t_floor, t_chainfence_v, t_chainfence_h, t_console,
                                           t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                           t_door_locked, t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor,
                                           t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", f_pool_table,
                                           f_crate_c, f_null,  f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,
                                           f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,
                                           f_null,              f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                           f_locker, f_null));
        place_items("bigtools", 80,  18, 7, 21,  7, false, 0);
        place_items("office", 80,  18,  11, 20,  11, false, 0);
        place_items("office", 60,  18,  13, 18,  13, false, 0);
        place_spawns(g, "GROUP_PUBLICWORKERS", 1, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, 0.2);
        if (t_north == "public_works" && t_west == "public_works") {
            rotate(3);
        } else if (t_north == "public_works" && t_east == "public_works") {
            rotate(0);
        } else if (t_south == "public_works" && t_east == "public_works") {
            rotate(1);
        } else if (t_west == "public_works" && t_south == "public_works") {
            rotate(2);
        }


    } else if (terrain_type == "public_works") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        if ((t_south == "public_works_entrance" && t_east == "public_works") ||
            (t_north == "public_works" && t_east == "public_works_entrance") || (t_west == "public_works" &&
                    t_north == "public_works_entrance") ||
            (t_south == "public_works" && t_west == "public_works_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
 |---------------|FFFFFF\n\
 |....rrrrrrrr...|      \n\
 |r..............| _____\n\
 |r..............| _____\n\
 |r..............| _____\n\
 |r..............| _____\n\
 |r..............| _____\n\
 |r..............| _____\n\
 |...............| _____\n\
 |--___________--| _____\n\
 f  ___________    _____\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n\
 f  ____________________\n",
                                       mapf::basic_bind("P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", t_floor,      t_floor,
                                               t_grate, t_pavement_y, t_pavement, t_floor, t_chainfence_v, t_chainfence_h, t_console,
                                               t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                               t_door_locked, t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor,
                                               t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", f_pool_table,
                                               f_crate_c, f_null,  f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,
                                               f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,
                                               f_null,              f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                               f_locker, f_null));
            place_items("hardware", 85,  2, 3, 2,  8, false, 0);
            place_items("hardware", 85,  6,  2, 13,  2, false, 0);
            spawn_item(21, 2, "log", rng(1, 3));
            spawn_item(15, 2, "pipe", rng(1, 10));
            spawn_item(4, 2, "glass_sheet", rng(1, 7));
            spawn_item(16, 5, "2x4", rng(1, 20));
            spawn_item(16, 7, "2x4", rng(1, 20));
            spawn_item(12, 2, "nail");
            spawn_item(13, 2, "nail");
            place_spawns(g, "GROUP_PUBLICWORKERS", 1, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, 0.1);
            if (t_west == "public_works_entrance") {
                rotate(1);
            }
            if (t_north == "public_works_entrance") {
                rotate(2);
            }
            if (t_east == "public_works_entrance") {
                rotate(3);
            }
        }

        else if ((t_west == "public_works_entrance" && t_north == "public_works") ||
                 (t_north == "public_works_entrance" && t_east == "public_works") || (t_west == "public_works" &&
                         t_south == "public_works_entrance") ||
                 (t_south == "public_works" && t_east == "public_works_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
__________           f  \n\
__________           f  \n\
ss                   f  \n\
ss                   f  \n\
+----ww-ww-ww-ww--|GFf  \n\
.^|..htth.........|     \n\
..+..........PPP..w     \n\
..|..........PPP..w     \n\
..|ccecoS........^|     \n\
..|---------------|     \n\
...llllllll|cScScS|     \n\
...........|......|     \n\
...........+......|     \n\
.htth......|..|+|+|     \n\
--ww---|...|.T|T|T|     \n\
sssssss|...|--|-|-|     \n\
____sss|.........l|     \n\
____sss+...c......w     \n\
____sss+...ch...hdw     \n\
____sss|^..c...ddd|     \n\
____sss|-www--www-|     \n\
____sss                 \n\
____sss                 \n\
____sss                 \n",
                                       mapf::basic_bind("P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", t_floor,      t_floor,
                                               t_grate, t_pavement_y, t_pavement, t_floor, t_chainfence_v, t_chainfence_h, t_console,
                                               t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                               t_door_locked, t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor,
                                               t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", f_pool_table,
                                               f_crate_c, f_null,  f_null,       f_null,     f_rack,  f_null,         f_null,         f_null,
                                               f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,   f_null,
                                               f_null,              f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                               f_locker, f_null));
            place_items("fridge", 80,  5, 8, 5,  8, false, 0);
            place_items("pool_table", 80,  13,  6, 15,  7, false, 0);
            place_items("construction_worker", 90,  3, 10, 10,  10, false, 0);
            place_items("office", 80,  15,  19, 17,  19, false, 0);
            place_items("cleaning", 80,  17,  16, 17,  16, false, 0);
            place_spawns(g, "GROUP_PUBLICWORKERS", 1, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, 0.3);
            if (t_north == "public_works_entrance") {
                rotate(1);
            }
            if (t_east == "public_works_entrance") {
                rotate(2);
            }
            if (t_south == "public_works_entrance") {
                rotate(3);
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
FFFFFFFFF|------------| \n\
         |..ll..rrr...| \n\
__________............| \n\
__________...........c| \n\
__________...........c| \n\
__________...........l| \n\
__________...........l| \n\
________ |............| \n\
________ |..O.clc.O...| \n\
________ |............| \n\
__________............| \n\
__________...........c| \n\
__________...........c| \n\
__________...........l| \n\
__________...........l| \n\
________ |......rr....| \n\
________ |---+--------| \n\
________   |..rrrr|  f  \n\
__________s+......w  f  \n\
__________ |r....r|  f  \n\
__________ |r....r|  f  \n\
__________ |--ww--|  f  \n\
__________           f  \n",
                                       mapf::basic_bind("O P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", t_column, t_floor,
                                               t_floor,   t_grate, t_pavement_y, t_pavement, t_floor, t_chainfence_v, t_chainfence_h, t_console,
                                               t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_door_c,
                                               t_door_locked, t_door_locked_alarm, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor,
                                               t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("O P C G , _ r f F 6 x $ ^ . - | t + = D w T S e o h c d l s", f_null,
                                               f_pool_table, f_crate_c, f_null,  f_null,       f_null,     f_rack,  f_null,         f_null,
                                               f_null,    f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_table, f_null,
                                               f_null,        f_null,              f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair,
                                               f_counter, f_desk,  f_locker, f_null));
            place_items("tools", 85,  14, 18, 17,  18, false, 0);
            place_items("tools", 85,  17,  20, 17,  21, false, 0);
            place_items("tools", 85,  12,  20, 12,  21, false, 0);
            place_items("mechanics", 85,  21, 12, 21,  15, false, 0);
            place_items("mechanics", 85,  21,  4, 21,  7, false, 0);
            place_items("mechanics", 85,  14,  9, 16,  9, false, 0);
            place_items("electronics", 80,  16,  2, 18,  2, false, 0);
            place_items("cleaning", 85,  12,  2, 13,  2, false, 0);
            spawn_item(3, 2, "log", rng(1, 3));
            place_spawns(g, "GROUP_PUBLICWORKERS", 1, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, 0.1);
            if (t_west == "public_works" && t_north == "public_works") {
                rotate(1);
                if (x_in_y(2, 3)) {
                    add_vehicle (g, "flatbed_truck", 2, 0, 90);
                }
            } else if (t_east == "public_works" && t_north == "public_works") {
                rotate(2);
                if (x_in_y(2, 3)) {
                    add_vehicle (g, "flatbed_truck", 23, 10, 270);
                }
            } else if (t_east == "public_works" && t_south == "public_works") {
                rotate(3);
                if (x_in_y(2, 3)) {
                    add_vehicle (g, "flatbed_truck", 10, 23, 0);
                }
            } else {
                if (x_in_y(2, 3)) {
                    add_vehicle (g, "flatbed_truck", 0, 10, 90);
                }
            }
        }


    } else if (terrain_type == "school_1") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
 ||-------|--++----++--|\n\
  |....................|\n\
  |....................|\n\
  |..hhhhhhh..hhhhhhh..|\n\
  |....................|\n\
  |..hhhhhhh..hhhhhhh..|\n\
  |....................|\n\
  |..hhhhhhh..hhhhhhh..|\n\
  |....................|\n\
  |..hhhhhhh..hhhhhhh..|\n\
  |....................|\n\
ss+..hhhhhhh..hhhhhhh..+\n\
ss+....................+\n\
ss|..hhhhhhh..hhhhhhh..|\n\
ss|....................|\n\
ss|---..............---|\n\
ss|....................|\n\
ss|..----------------..|\n\
ss+....................|\n\
ss|....................|\n\
ss|--------------------|\n\
ss                      \n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(20, 60), SEEX, SEEY);
        if (t_north == "school_2") {
            rotate(3);
        } else if (t_east == "school_2") {
            rotate(0);
        } else if (t_south == "school_2") {
            rotate(1);
        } else if (t_west == "school_2") {
            rotate(2);
        }


    } else if (terrain_type == "school_2") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
........l|...|.h..x..w  \n\
l.......l|-+-|.h..dh.w  \n\
l.......l|...+....c..w  \n\
l.......l|...|.......|  \n\
l.......l|h..|-------|$ \n\
l.......l|h..|.......|  \n\
l.......l|h..+....d..w  \n\
l.......l|h..|.h..dh.w  \n\
l.......l|...|....d..|  \n\
l.......l|...|------||  \n\
.........|....#####.|$  \n\
.........+..........+sss\n\
.........+..........+sss\n\
.........|ccccccc...|$  \n\
-w-+++-w-|..........w$  \n\
   sss   |dd..tt.dddw$  \n\
  $sss$  |dh..tt..hdw$  \n\
   sss   |d........^|$  \n\
  $sss$  |-www--www-|$  \n\
   sss    $$$$$$$$$$$$  \n\
  $sss$                 \n\
   sss                  \n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(5, 20), SEEX, SEEY);
        add_spawn("mon_zombie", rng(0, 8), SEEX, SEEY);
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_desk) {
                    place_items("office", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_locker) {
                    place_items("school", 60,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "school_5") {
            rotate(0);
        } else if (t_east == "school_5") {
            rotate(1);
        } else if (t_south == "school_5") {
            rotate(2);
        } else if (t_west == "school_5") {
            rotate(3);
        }


    } else if (terrain_type == "school_3") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
ssssssssssssssssssssssss\n\
ssLLLLLLL_______LLLLLLLs\n\
ss_____________________s\n\
ss_____________________s\n\
ss_____________________s\n\
ssLLLLLLL_______LLLLLLLs\n\
ss_____________________s\n\
ss_____________________s\n\
ss_____________________s\n\
ssLLLLLLL_______LLLLLLLs\n\
ss_____________________s\n\
ss_____________________s\n\
ss_____________________s\n\
ssLLLLLLL_______LLLLLLLs\n\
ss_____________________s\n\
ss_____________________s\n\
ss_____________________s\n\
ssLLLLLLL_______LLLLLLLs\n\
ss_____________________s\n\
ss_____________________s\n\
ss_____________________s\n\
ssLLLLLLL_______LLLLLLLs\n\
ss$$   $$_______$$   $$s\n\
sssssssss_______ssssssss\n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(0, 8), SEEX, SEEY);
        if (t_north == "school_2") {
            rotate(1);
            if (x_in_y(1, 7)) {
                add_vehicle (g, "schoolbus", 19, 10, 0);
            }
        } else if (t_east == "school_2") {
            rotate(2);
            if (x_in_y(1, 7)) {
                add_vehicle (g, "schoolbus", 9, 7, 0);
            }
        } else if (t_south == "school_2") {
            rotate(3);
            if (x_in_y(1, 7)) {
                add_vehicle (g, "schoolbus", 12, 18, 180);
            }
        } else if (t_west == "school_2") {
            rotate(0);
            if (x_in_y(1, 7)) {
                add_vehicle (g, "schoolbus", 17, 7, 0);
            }
        }


    } else if (terrain_type == "school_4") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
 w..ddd...+....|.d.d.d.d\n\
 w........|....|........\n\
 w........|....|........\n\
 wd.d.d.d.|....+..ddd...\n\
 |h.h.h.h.|....|..dh...l\n\
 |d.d.d.d.|....|--------\n\
 wh.h.h.h.|....|..ttt..l\n\
 wd.d.d.d.|....+........\n\
 wh.h.h.h.|....|.h.h.h.h\n\
 w........+....|.d.d.d.d\n\
 |l..ttt..|....|.h.h.h.h\n\
 |--------|....|.d.d.d.d\n\
 |l..hd...|....|.h.h.h.h\n\
 w..ddd...+....|.d.d.d.d\n\
 w........|....|........\n\
 w........|....|........\n\
 wd.d.d.d.|....+..ddd...\n\
 |h.h.h.h.|....|..dh...l\n\
 |d.d.d.d.|....|--------\n\
 wh.h.h.h.|.............\n\
 wd.d.d.d.|.............\n\
 wh.h.h.h.|....^ccc^ccc^\n\
 w........+.............\n\
 |l..ttt..|.............\n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(0, 20), SEEX, SEEY);
        add_spawn("mon_zombie", rng(0, 4), SEEX, SEEY);
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_desk) {
                    place_items("school", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_locker) {
                    place_items("school", 60,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "school_5") {
            rotate(3);
        } else if (t_east == "school_5") {
            rotate(0);
        } else if (t_south == "school_5") {
            rotate(1);
        } else if (t_west == "school_5") {
            rotate(2);
        }


    } else if (terrain_type == "school_5") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
|.oooooooo...o.o.o.o.|--\n\
|............o.o.o.o.|.T\n\
|.oooooooo...o.o.o.o.|.S\n\
|............o.o.o.o.|+-\n\
|.oooooooo..............\n\
|.......................\n\
|--------|o....o.o.o.|+-\n\
|ch......|o....o.o.o.|.S\n\
|cxc.....|o....o.o.o.|.T\n\
|xxx..xxx|o..........|--\n\
|hhh..hhh|.........ooooo\n\
|........|.htth.........\n\
|xxx..xxx|.htth.........\n\
|hhh..hhh|........ccc.cc\n\
|........|.htth...c....t\n\
|ww-++-ww|.htth...ch....\n\
|........|........c...hd\n\
|........|........c..ddd\n\
|........|-----++-------\n\
........................\n\
........................\n\
........................\n\
.........|---|-------|--\n\
........l|l.l|^...ccl|$ \n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(0, 15), SEEX, SEEY);
        add_spawn("mon_zombie", rng(0, 4), SEEX, SEEY);
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_desk) {
                    place_items("school", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_locker) {
                    place_items("school", 60,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_bookcase) {
                    place_items("novels", 50,  i,  j, i,  j, false, 0);
                    place_items("manuals", 40,  i,  j, i,  j, false, 0);
                    place_items("textbooks", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "school_2") {
            rotate(2);
        } else if (t_east == "school_2") {
            rotate(3);
        } else if (t_south == "school_2") {
            rotate(0);
        } else if (t_west == "school_2") {
            rotate(1);
        }


    } else if (terrain_type == "school_6") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
|d.d.d.d.|....|.d.d.d.dw\n\
|........|....|.h.h.h.hw\n\
|........|....|.d.d.d.d|\n\
|..ddd...+....|.h.h.h.h|\n\
|l..hd...|....|.d.d.d.dw\n\
|--------|....|........w\n\
|l..hd...|....|........w\n\
|..ddd...+....+..ddd...w\n\
|........|....|..dh...l|\n\
|........|....|--------|\n\
|d.d.d.d.|....|..ttt..l|\n\
|h.h.h.h.|....+........w\n\
|d.d.d.d.|....|.h.h.h.hw\n\
|h.h.h.h.|....|.d.d.d.dw\n\
|d.d.d.d.|....|.h.h.h.hw\n\
|h.h.h.h.|....|.d.d.d.d|\n\
|........+....|.h.h.h.h|\n\
|l..ttt..|....|.d.d.d.dw\n\
|--------|....|........w\n\
..............|........w\n\
..............+..ddd...w\n\
..............|..dh...l|\n\
-----------++-|--------|\n\
ssssssssssssssssssssssss\n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(0, 20), SEEX, SEEY);
        add_spawn("mon_zombie", rng(0, 4), SEEX, SEEY);
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_desk) {
                    place_items("school", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_locker) {
                    place_items("school", 60,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "school_5") {
            rotate(1);
        } else if (t_east == "school_5") {
            rotate(2);
        } else if (t_south == "school_5") {
            rotate(3);
        } else if (t_west == "school_5") {
            rotate(0);
        }


    } else if (terrain_type == "school_7") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
 |--------|    |--wwww--\n\
 |l..ll..l|-++-|ccccccll\n\
 |l..ll..l|....+........\n\
 |l..ll...+....|Scc..ccS\n\
 |l......l|....|........\n\
 |-+------|....|Scc..ccS\n\
 |...hd.oo|....|........\n\
 w..ddd...+....|Scc..ccS\n\
 w........|....|........\n\
 w........|....|........\n\
 wScc..ccS|....+..ddd...\n\
 |........|....|..dh..ll\n\
 |Scc..ccS|....|--------\n\
 w........|....+........\n\
 wScc..ccS|....|....|+|+\n\
 w........|....|SSS.|T|T\n\
 w........+....|----|-|-\n\
 |l.ttt.ll|....|..ttt..l\n\
 |--------|....+........\n\
 |........+....|.h.h.h.h\n\
 |+|+|....|....|.d.d.d.d\n\
 |T|T|.SSS|....|.h.h.h.h\n\
 |-|-|----|....|.d.d.d.d\n\
 |l..hd...|....|.h.h.h.h\n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(0, 20), SEEX, SEEY);
        add_spawn("mon_zombie", rng(0, 4), SEEX, SEEY);
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_desk) {
                    place_items("school", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_locker) {
                    place_items("chem_school", 60,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "school_8") {
            rotate(3);
        } else if (t_east == "school_8") {
            rotate(0);
        } else if (t_south == "school_8") {
            rotate(1);
        } else if (t_west == "school_8") {
            rotate(2);
        }


    } else if (terrain_type == "school_8") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
|                     |-\n\
|ffffffffffGffffffffff|.\n\
w         sss         w.\n\
w      $$ sss $$      w.\n\
w      $$ sss $$      w.\n\
w         sss         |.\n\
|         ssssssssssss+.\n\
w         sss         |.\n\
w      $$ sss $$      w.\n\
w      $$ sss $$      w.\n\
w      $$ sss $$      w.\n\
|     $$  sss  $$     |.\n\
|    $$  sssss  $$    |-\n\
|   $$  sssssss  $$     \n\
|   $$  sssssss  $$     \n\
|   $$  sssOsss  $$     \n\
|   $$  sssssss  $$     \n\
|    $$  sssss  $$      \n\
w     $$       $$       \n\
w                       \n\
w                       \n\
|                       \n\
|-www-wwww--wwww-www----\n\
|....................+.l\n",
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,         f_null,        f_null,           f_null,  f_indoor_plant, f_null,  f_null,
                                           f_null,   f_bench, f_table, f_null,   f_null,              f_null,        f_null,   f_toilet,
                                           f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        add_spawn("mon_zombie_child", rng(0, 4), SEEX, SEEY);
        add_spawn("mon_zombie", rng(0, 1), SEEX, SEEY);
        place_items("cleaning", 80,  22, 23, 23,  23, false, 0);
        spawn_item(12, 15, "american_flag");
        if (t_north == "school_5") {
            rotate(2);
        } else if (t_east == "school_5") {
            rotate(3);
        } else if (t_south == "school_5") {
            rotate(0);
        } else if (t_west == "school_5") {
            rotate(1);
        }


    } else if (terrain_type == "school_9") {

        mapf::formatted_set_simple(this, 0, 0,
                                   "\
wwww--wwww--wwww--wwww-|\n\
........htth.htth.htth.|\n\
.hhhhh..htth.htth.htth.w\n\
.ttttt..htth.htth.htth.w\n\
.ttttt..htth...........w\n\
.hhhhh.................w\n\
..............ccccccc..|\n\
.hhhhh........c........|\n\
.ttttt.....c..c.....eee|\n\
.ttttt...xhc..|--..----|\n\
.hhhhh..cccc..|c......ew\n\
..............|O..tt..cw\n\
|--------|-++-|O..tt..cw\n\
|........+....|O......cw\n\
|+|+|....|....|-|--...e|\n\
|T|T|.SSS|....+l|r+....+\n\
|-|-|----|....|-|------|\n\
|ll.ttt..|....+........|\n\
w........+....|....|+|+|\n\
wh.h.h.h.|....|SSS.|T|T|\n\
wd.d.d.d.|....|----|-|-|\n\
|h.h.h.h.|....|..ttt..l|\n\
|d.d.d.d.|....+........w\n\
|h.h.h.h.|....|.h.h.h.hw\n",
                                   mapf::basic_bind("e r _ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", t_floor,  t_floor,
                                           t_pavement, t_pavement_y, t_column, t_chainfence_h, t_chaingate_c, t_console_broken, t_shrub,
                                           t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_locked_alarm,
                                           t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_floor,  t_sidewalk),
                                   mapf::basic_bind("e r _ L O f G x $ ^ . - | # t + = D w T S e o h c d l s", f_fridge, f_rack,
                                           f_null,     f_null,       f_null,   f_null,         f_null,        f_null,           f_null,
                                           f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table, f_null,   f_null,
                                           f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,
                                           f_locker, f_null));
        add_spawn("mon_zombie_child", rng(0, 20), SEEX, SEEY);
        add_spawn("mon_zombie", rng(3, 10), SEEX, SEEY);
        place_items("cleaning", 80,  15,  15, 15,  15, false, 0);
        place_items("cannedfood", 95,  17,  15, 17,  15, false, 0);
        place_items("fast_food", 95,  18,  11, 19,  12, false, 0);
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_desk) {
                    place_items("school", 50,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_locker) {
                    place_items("school", 60,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_fridge) {
                    place_items("fridge", 90,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "school_8") {
            rotate(1);
        } else if (t_east == "school_8") {
            rotate(2);
        } else if (t_south == "school_8") {
            rotate(3);
        } else if (t_west == "school_8") {
            rotate(0);
        }


    } else if (terrain_type == "prison_1") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |----|------|--\n\
 %  F                   \n\
 %  F                   \n\
 % |--|                 \n\
 % |,,|                 \n\
 % |,,|fffffffffffffffff\n\
 % |--|                 \n\
 %                      \n\
 %%%%%%%%%%%%%%%%%%%%%%%\n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n",
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", t_floor, t_floor,
                                           t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                           t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                           t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed, t_chainfence_h,
                                           t_chainfence_v, t_floor),
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", f_bench, f_exercise, f_null,
                                           f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker, f_null,   f_null,
                                           f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,            f_null,
                                           f_null, f_null,       f_null,       f_null,         f_null,         f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
            }
        }
        if (t_north == "prison_2") {
            rotate(3);
        } else if (t_east == "prison_2") {
            rotate(0);
        } else if (t_south == "prison_2") {
            rotate(1);
        } else if (t_west == "prison_2") {
            rotate(2);
        }


    } else if (terrain_type == "prison_2") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
,T|s________________s|T,\n\
--|s________________s|--\n\
bb|s________________s|bb\n\
,,|s________________s|,,\n\
,T|s________________s|T,\n\
--|s________________s|--\n\
bb|s________________s|bb\n\
,,|ss______________ss|,,\n\
,T| ss____________ss |T,\n\
--|  ss__________ss  |--\n\
      ss________ss      \n\
       ss______ss       \n\
       ss______ss       \n\
       ss______ss       \n\
fffffffffHHHHHHfffffffff\n\
         ______         \n\
         ______         \n\
%%%%%%%%%______%%%%%%%%%\n\
         ______         \n\
    |-+w|FFFFFF         \n\
    w,h,w______         \n\
    wdxdw______         \n\
    |www|______         \n\
         ______         \n",
                                   mapf::basic_bind("H # E g r + = h c l w s _ o d x T b G , B - | % f F S", t_chaingate_l, t_floor,
                                           t_floor,    t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,
                                           t_floor,  t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,
                                           t_floor, t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                           t_chainfence_h, t_chainfence_v, t_floor),
                                   mapf::basic_bind("H # E g r + = h c l w s _ o d x T b G , B - | % f F S", f_null,        f_bench,
                                           f_exercise, f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter,
                                           f_locker, f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,
                                           f_null,            f_null,  f_null, f_null,       f_null,       f_null,         f_null,
                                           f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
            }
        }
        add_spawn("mon_eyebot", 1, rng(5, 18), rng(12, 18));
        if (t_north == "prison_5") {
            rotate(0);
        } else if (t_east == "prison_5") {
            rotate(1);
        } else if (t_south == "prison_5") {
            rotate(2);
        } else if (t_west == "prison_5") {
            rotate(3);
        }


    } else if (terrain_type == "prison_3") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
,,B,,,,,,B,,,T|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
--|------|----|    F  % \n\
                   F  % \n\
                   F  % \n\
                 |--| % \n\
                 |,,| % \n\
fffffffffffffffff|,,| % \n\
                 |--| % \n\
                      % \n\
%%%%%%%%%%%%%%%%%%%%%%% \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n",
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", t_floor, t_floor,
                                           t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                           t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                           t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed, t_chainfence_h,
                                           t_chainfence_v, t_floor),
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", f_bench, f_exercise, f_null,
                                           f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker, f_null,   f_null,
                                           f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,            f_null,
                                           f_null, f_null,       f_null,       f_null,         f_null,         f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
            }
        }
        if (t_north == "prison_2") {
            rotate(1);
        } else if (t_east == "prison_2") {
            rotate(2);
        } else if (t_south == "prison_2") {
            rotate(3);
        } else if (t_west == "prison_2") {
            rotate(0);
        }


    } else if (terrain_type == "prison_4") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
 %  F    |----|,,,,,,|--\n\
 %  F    |bb,,B,,,,,,|,,\n\
 %  F    |,,,,G,,,,,,|,,\n\
 %  F    |T,,,B,,,,,,|,,\n\
 %  F  |-|-|--|BBGGBB|-+\n\
 %  F  |,,dB,,,,,,,,,,,,\n\
 %  F  |,hdB,,,,,,,,,,,,\n\
 %  F  |,,dB,,,,,,,,,,,,\n\
 %  F  |,,,G,,,,,,,,,,,,\n\
 %  F  |-|-|--|BBGGBB|--\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n",
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", t_floor, t_floor,
                                           t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                           t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                           t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed, t_chainfence_h,
                                           t_chainfence_v, t_floor),
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", f_bench, f_exercise, f_null,
                                           f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker, f_null,   f_null,
                                           f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,            f_null,
                                           f_null, f_null,       f_null,       f_null,         f_null,         f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
                if (this->furn(i, j) == f_desk) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                    place_items("office", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "prison_5") {
            rotate(3);
        } else if (t_east == "prison_5") {
            rotate(0);
        } else if (t_south == "prison_5") {
            rotate(1);
        } else if (t_west == "prison_5") {
            rotate(2);
        }


    } else if (terrain_type == "prison_5") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
--|o,,,,|------|,,,,,|--\n\
,<|o,,,,G,,rr,cB,,,,,|<,\n\
,,|--+--|,,,,,cBBGGBB|,,\n\
,,|,,,,,Bc,,h,cB,,,,,|,,\n\
+-|,,,,,BcccxccB,,,,,|-+\n\
,,B,,,,,BBBBBBBB,,,,,B,,\n\
,,G,,,,,,,,,,,,,,,,,,G,,\n\
,,G,,,,,,,,,,,,,,,,,,G,,\n\
,,B,,,,,,,,,,,,,,,,,,B,,\n\
--|--|-|BBBGGBBB|-+--|--\n\
bb|TS|l|,,,,,,,,|,,,,|bb\n\
,,|,,|=|---++--||,,,,|,,\n\
,T|,,=,,,,,,,,#|,,,,,|T,\n\
--|--|-+|,,,,,#|h|h|h|--\n\
bb|,,,,,|#,,,,#|g|g|g|bb\n\
,,|,,,,,|#,,,,#|h|h|h|,,\n\
,T|,ddd,|#,,,,,|,,,,,|T,\n\
--|,,h,,|#,,,,,=,,,,,|--\n\
bb|,,,,,|,,,,,,|,,,,,|bb\n\
,,|-ggg-|gg++gg|-ggg-|,,\n\
,T|   ssssssssssss   |T,\n\
--|  ss__________ss  |--\n\
bb| ss____________ss |bb\n\
,,|ss______________ss|,,\n",
                                   mapf::basic_bind("< # E g r + = h c l w s _ o d x T b G , B - | % f F S", t_stairs_down, t_floor,
                                           t_floor,    t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,
                                           t_floor,  t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,
                                           t_floor, t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                           t_chainfence_h, t_chainfence_v, t_floor),
                                   mapf::basic_bind("< # E g r + = h c l w s _ o d x T b G , B - | % f F S", f_null,        f_bench,
                                           f_exercise, f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter,
                                           f_locker, f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,
                                           f_null,            f_null,  f_null, f_null,       f_null,       f_null,         f_null,
                                           f_null,         f_sink));
        add_spawn("mon_secubot", rng(1, 2), 11, 7);
        add_spawn("mon_zombie_cop", rng(0, 3), rng(12, 18), rng(4, 19));
        place_items("pistols", 30,  11,  1, 12,  1, false, 0);
        place_items("ammo", 50,  11,  1, 12,  1, false, 0);
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
                if (this->furn(i, j) == f_desk) {
                    place_items("magazines", 40,  i,  j, i,  j, false, 0);
                    place_items("office", 40,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "prison_2") {
            rotate(2);
        } else if (t_east == "prison_2") {
            rotate(3);
        } else if (t_south == "prison_2") {
            rotate(0);
        } else if (t_west == "prison_2") {
            rotate(1);
        }


    } else if (terrain_type == "prison_6") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
--|,,,,,,|----|    F  % \n\
,,|,,,,,,B,,bb|    F  % \n\
,,|,,,,,,G,,,,|    F  % \n\
,,|,,,,,,B,,,T|    F  % \n\
+-|BBGGBB|--|-|-|  F  % \n\
,,,,,,,,,,,,Bd,,|  F  % \n\
,,,,,,,,,,,,Bdh,|  F  % \n\
,,,,,,,,,,,,Bd,,|  F  % \n\
,,,,,,,,,,,,G,,,|  F  % \n\
--|BBGGBB|--|-|-|  F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n",
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", t_floor, t_floor,
                                           t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                           t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                           t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed, t_chainfence_h,
                                           t_chainfence_v, t_floor),
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", f_bench, f_exercise, f_null,
                                           f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker, f_null,   f_null,
                                           f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,            f_null,
                                           f_null, f_null,       f_null,       f_null,         f_null,         f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
                if (this->furn(i, j) == f_desk) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                    place_items("office", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "prison_5") {
            rotate(1);
        } else if (t_east == "prison_5") {
            rotate(2);
        } else if (t_south == "prison_5") {
            rotate(3);
        } else if (t_west == "prison_5") {
            rotate(0);
        }


    } else if (terrain_type == "prison_7") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
 %%%%%%%%%%%%%%%%%%%%%%%\n\
 %                      \n\
 % |--|                 \n\
 % |,,|fffffffffffffffff\n\
 % |,,|                 \n\
 % |--|                 \n\
 %  F                   \n\
 %  F    |----|------|--\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |T,,,B,,,,,,B,,\n\
 %  F    |----|,,,,,,|--\n\
 %  F    |bb,,B,,,,,,B,,\n\
 %  F    |,,,,G,,,,,,G,,\n\
 %  F    |T,,,B,,,,,,B,,\n",
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", t_floor, t_floor,
                                           t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                           t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                           t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed, t_chainfence_h,
                                           t_chainfence_v, t_floor),
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", f_bench, f_exercise, f_null,
                                           f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker, f_null,   f_null,
                                           f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,            f_null,
                                           f_null, f_null,       f_null,       f_null,         f_null,         f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
            }
        }
        if (t_north == "prison_8") {
            rotate(3);
        } else if (t_east == "prison_8") {
            rotate(0);
        } else if (t_south == "prison_8") {
            rotate(1);
        } else if (t_west == "prison_8") {
            rotate(2);
        }


    } else if (terrain_type == "prison_8") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
%%%%%%%%%%%%%%%%%%%%%%%%\n\
                        \n\
                        \n\
ffffffffffffffffffffffff\n\
                        \n\
                        \n\
                        \n\
--|                  |--\n\
,T|                  |T,\n\
,,|------------------|,,\n\
bb|ssssssssssssssssss|bb\n\
--|ssssssssssssssssss|--\n\
,T|ssssssssssssssssss|T,\n\
,,|ssEsssssssssssssss|,,\n\
bb|ssssssssssssssssss|bb\n\
--|ssEsssssssssssssss|--\n\
,T|ssssssssssssssssss|T,\n\
,,|sssEssEssEssssssss|,,\n\
bb|ssssssssssssssssss|bb\n\
--|-----|------|-++--|--\n\
,T|oooo<|,bb,,l|,,,,,|T,\n\
,,|,,,,,=,,,,,l|,,,,,|,,\n\
bb|o,,,,|,,,,,l|,,,,,|bb\n",
                                   mapf::basic_bind("< # E g r + = h c l w s _ o d x T b G , B - | % f F S", t_stairs_down, t_floor,
                                           t_floor,    t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,
                                           t_floor,  t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,
                                           t_floor, t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                           t_chainfence_h, t_chainfence_v, t_floor),
                                   mapf::basic_bind("< # E g r + = h c l w s _ o d x T b G , B - | % f F S", f_null,        f_bench,
                                           f_exercise, f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter,
                                           f_locker, f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,
                                           f_null,            f_null,  f_null, f_null,       f_null,       f_null,         f_null,
                                           f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
                if (this->ter(i, j) == t_sidewalk) {
                    if (one_in(200)) {
                        if (!one_in(3)) {
                            add_spawn("mon_zombie", 1, i, j);
                        } else {
                            add_spawn("mon_zombie_brute", 1, i, j);
                        }
                    }
                }
                if (this->furn(i, j) == f_locker) {
                    place_items("softdrugs", 40,  i,  j, i,  j, false, 0);
                    place_items("harddrugs", 40,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_bookcase) {
                    place_items("novels", 70,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (t_north == "prison_5") {
            rotate(2);
        } else if (t_east == "prison_5") {
            rotate(3);
        } else if (t_south == "prison_5") {
            rotate(0);
        } else if (t_west == "prison_5") {
            rotate(1);
        }


    } else if (terrain_type == "prison_9") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
%%%%%%%%%%%%%%%%%%%%%%% \n\
                      % \n\
                 |--| % \n\
fffffffffffffffff|,,| % \n\
                 |,,| % \n\
                 |--| % \n\
                   F  % \n\
--|------|----|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n\
--|,,,,,,|----|    F  % \n\
,,B,,,,,,B,,bb|    F  % \n\
,,G,,,,,,G,,,,|    F  % \n\
,,B,,,,,,B,,,T|    F  % \n",
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", t_floor, t_floor,
                                           t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                           t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                           t_door_bar_locked, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed, t_chainfence_h,
                                           t_chainfence_v, t_floor),
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G , B - | % f F S", f_bench, f_exercise, f_null,
                                           f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker, f_null,   f_null,
                                           f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,            f_null,
                                           f_null, f_null,       f_null,       f_null,         f_null,         f_null,         f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
            }
        }
        if (t_north == "prison_8") {
            rotate(1);
        } else if (t_east == "prison_8") {
            rotate(2);
        } else if (t_south == "prison_8") {
            rotate(3);
        } else if (t_west == "prison_8") {
            rotate(0);
        }


    } else if (terrain_type == "prison_b_entrance") {

        fill_background(this, t_rock);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
,T|------------------|##\n\
--|#####################\n\
,,|---------------------\n\
,,G,,,,,,,,,,,,,,,,,,,,,\n\
,,G,,,,,,,,,,,,,,,,,,,,,\n\
--|---------------------\n\
bb|#####################\n\
,,|#####################\n\
,T|#####################\n\
--|#####################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n",
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", t_rock, t_floor,
                                           t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                           t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                           t_door_bar_locked, t_grass, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                           t_chainfence_h, t_chainfence_v, t_floor),
                                   mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", f_null, f_exercise,
                                           f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker,
                                           f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,
                                           f_null,  f_null,  f_null, f_null,       f_null,       f_null,         f_null,         f_null,
                                           f_sink));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_toilet) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                    if (!one_in(3)) {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    }
                }
            }
        }
        if (t_west != "prison_b") {
            rotate(1);
        } else if (t_north != "prison_b") {
            rotate(2);
        } else if (t_east != "prison_b") {
            rotate(3);
        }


    } else if (terrain_type == "prison_b") {

        fill_background(this, &grass_or_dirt);
        if (t_above == "prison_1") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
#########|bb,,|,,,,,,|,,\n\
#########|----|,,,,,,|--\n\
#########|T,,,|,,,,,,,,,\n\
#########|,,,,G,,,,,,,,,\n\
#########|bb,,|,,,,,,,,,\n\
#########|----|,,,,,,|--\n\
#########|T,,,|,,,,,,|,,\n\
#########|,,,,G,,,,,,G,,\n\
#########|bb,,|,,,,,,|,,\n\
#########|----|------|--\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n",
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", t_rock, t_floor,
                                               t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                               t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                               t_door_bar_locked, t_grass, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                               t_chainfence_h, t_chainfence_v, t_floor),
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", f_null, f_exercise,
                                               f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker,
                                               f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,
                                               f_null,  f_null,  f_null, f_null,       f_null,       f_null,         f_null,         f_null,
                                               f_sink));
            if (t_south == "prison_b_entrance") {
                rotate(1);
            } else if (t_west == "prison_b_entrance") {
                rotate(2);
            } else if (t_north == "prison_b_entrance") {
                rotate(3);
            }
        }

        if (t_above == "prison_3") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
####|,,|################\n\
####|,,|################\n\
----|,,|################\n\
,,,,,,,|################\n\
,,,,,,,|################\n\
-------|################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n",
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", t_rock, t_floor,
                                               t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                               t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                               t_door_bar_locked, t_grass, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                               t_chainfence_h, t_chainfence_v, t_floor),
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", f_null, f_exercise,
                                               f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker,
                                               f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,
                                               f_null,  f_null,  f_null, f_null,       f_null,       f_null,         f_null,         f_null,
                                               f_sink));
            if (t_north == "prison_b_entrance") {
                rotate(1);
            } else if (t_east == "prison_b_entrance") {
                rotate(2);
            } else if (t_south == "prison_b_entrance") {
                rotate(3);
            }
        }

        if (t_above == "prison_4") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
############|,,,+,,|#|--\n\
############|---|,,|#|,,\n\
################|,,|#|,,\n\
################|,,|#|,,\n\
################|,,|#|-G\n\
################|GG|-|,,\n\
################|,,,,,,,\n\
################|,,,,,,,\n\
################|GG|-|,,\n\
################|,,|#|--\n\
################|,,|####\n\
################|,,|####\n\
################|,,|####\n\
################|,,|####\n\
################|,,|####\n\
################|,,|####\n\
################|,,|####\n\
#########|----|-|++|-|--\n\
#########|T,,,|,,,,,,|,,\n\
#########|,,,,G,,,,,,G,,\n\
#########|bb,,|,,,,,,|,,\n\
#########|----|,,,,,,|--\n\
#########|T,,,|,,,,,,|,,\n\
#########|,,,,G,,,,,,G,,\n",
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", t_rock, t_floor,
                                               t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                               t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                               t_door_bar_locked, t_grass, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                               t_chainfence_h, t_chainfence_v, t_floor),
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", f_null, f_exercise,
                                               f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker,
                                               f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,
                                               f_null,  f_null,  f_null, f_null,       f_null,       f_null,         f_null,         f_null,
                                               f_sink));
            if (t_north != "prison_b") {
                rotate(1);
            } else if (t_east != "prison_b") {
                rotate(2);
            } else if (t_south != "prison_b") {
                rotate(3);
            }
        }

        if (t_above == "prison_5") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
--|t,,,,|------|c,,,,|--\n\
,>|h,,,,G,,rr,cB,,,,,|>,\n\
,,|--+--|,,,,,cBccccc|,,\n\
,,|,,,,,Bc,,h,cB,,,,,|,,\n\
G-|,,,,,BcccxccB,,,,,|-G\n\
,,B,,,,,BBBBBBBB,,,,,B,,\n\
,,G,,,,,,,,,,,,,,,,,,G,,\n\
,,G,,,,,,,,,,,,,,,,,,G,,\n\
,,B,,,,,,,,,,,,,,,,,,B,,\n\
--|,,,,,,,,,,,,,,,,,,|-+\n\
##|,htth,,,,,,,,htth,|,,\n\
##|,htth,,,,,,,,htth,|,,\n\
##|,htth,,,,,,,,htth,|h,\n\
##|,htth,O,,,,O,htth,|h,\n\
##|,htth,,,,,,,,htth,|h,\n\
##|,,,,,,,,,,,,,,,,,,|,,\n\
##|,,,,,,,,,,,,,,,,,,|--\n\
--|,htth,,,,,,,,htth,|##\n\
bb|,htth,O,,,,O,htth,|##\n\
,,|,htth,,,,,,,,htth,|##\n\
,T|,htth,,,,,,,,htth,|##\n\
--|,htth,,,,,,,,htth,|##\n\
bb|,,,,,,,,,,,,,,,,,,|##\n\
,,|,,,,,,,,,,,,,,,,,,|##\n",
                                       mapf::basic_bind("t > O # E g r + = h c l w s _ o d x T b G . , B - | % f F S", t_floor,
                                               t_stairs_up, t_column, t_rock, t_floor,    t_reinforced_glass_h, t_floor, t_door_locked_interior,
                                               t_door_c, t_floor, t_floor,   t_floor,  t_window, t_sidewalk, t_pavement, t_floor,    t_floor,
                                               t_console_broken, t_floor,  t_floor, t_door_bar_locked, t_grass, t_floor, t_bars, t_concrete_h,
                                               t_concrete_v, t_fence_barbed, t_chainfence_h, t_chainfence_v, t_floor),
                                       mapf::basic_bind("t > O # E g r + = h c l w s _ o d x T b G . , B - | % f F S", f_table, f_null,
                                               f_null,   f_null, f_exercise, f_null,               f_rack,  f_null,                 f_null,
                                               f_chair, f_counter, f_locker, f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,
                                               f_toilet, f_bed,   f_null,            f_null,  f_null,  f_null, f_null,       f_null,       f_null,
                                               f_null,         f_null,         f_sink));
            add_spawn("mon_zombie_cop", rng(0, 2), 2, 0);
            add_spawn("mon_zombie_cop", rng(0, 2), 2, 23);
            place_items("pistols", 30,  11,  1, 12,  1, false, 0);
            place_items("ammo", 40,  11,  1, 12,  1, false, 0);
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_chair) {
                        if (one_in(4)) {
                            if (!one_in(3)) {
                                add_spawn("mon_zombie", 1, i, j);
                            } else if (one_in(10)) {
                                add_spawn("mon_zombie_cop", 1, i, j);
                            } else {
                                add_spawn("mon_zombie_brute", 1, i, j);
                            }
                        }
                    }
                }
            }
            if (t_west == "prison_b_entrance") {
                rotate(1);
            } else if (t_north == "prison_b_entrance") {
                rotate(2);
            } else if (t_east == "prison_b_entrance") {
                rotate(3);
            }
        }

        if (t_above == "prison_6") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
+-|#####################\n\
,,|#####################\n\
,,|#####################\n\
,,|-------------|#######\n\
G-|rr,DDDD,,DDDD|#######\n\
,,+,,,,,,,,,,,,,|#######\n\
,,|,,,,tt,tt,tt,|#######\n\
,,c,,,,,,,,,,,,,|#######\n\
,,|rr,WWWW,,WWWW|#######\n\
+-|-|---|-------|#######\n\
,,,,+,,r|###############\n\
,,,,|,,r|###############\n\
h,h,g,,,|###############\n\
h,h,g,t,|###############\n\
h,h,g,,,|###############\n\
,,,,|,,l|###############\n\
----|++||###############\n\
####|,,|################\n\
####|,,|################\n\
####|,,|################\n\
####|,,|################\n\
####|,,|################\n\
####|,,|################\n\
####|,,|################\n",
                                       mapf::basic_bind("D W # t g r + = h c l w s _ o d x T b G . , B - | % f F S", t_floor, t_floor,
                                               t_rock, t_floor, t_reinforced_glass_v, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,
                                               t_floor,  t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,
                                               t_floor, t_door_bar_locked, t_grass, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                               t_chainfence_h, t_chainfence_v, t_floor),
                                       mapf::basic_bind("D W # t g r + = h c l w s _ o d x T b G . , B - | % f F S", f_dryer, f_washer,
                                               f_null, f_table, f_null,               f_rack,  f_null,                 f_null,   f_chair,
                                               f_counter, f_locker, f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,
                                               f_toilet, f_bed,   f_null,            f_null,  f_null,  f_null, f_null,       f_null,       f_null,
                                               f_null,         f_null,         f_sink));
            spawn_item(7, 11, "visions_solitude");
            add_spawn("mon_zombie_brute", 1, 6, 13);
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_locker || this->furn(i, j) == f_rack ) {
                        place_items("science", 30,  i,  j, i,  j, false, 0);
                        place_items("cleaning", 30,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (t_south != "prison_b") {
                rotate(1);
            } else if (t_west != "prison_b") {
                rotate(2);
            } else if (t_north != "prison_b") {
                rotate(3);
            }
        }

        else if (t_above == "prison_7") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
############|----------|\n\
############|..........|\n\
############|..........|\n\
############|...|--|...|\n\
############|...|##|...|\n\
############|...|##|...|\n\
############|...|##|...|\n\
############|...|##|...|\n\
############|...|--|...|\n\
############|..........|\n\
############|..........|\n\
############|---|++|---|\n\
############|rrr|,,|####\n",
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", t_rock, t_floor,
                                               t_reinforced_glass_h, t_floor, t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,
                                               t_window, t_sidewalk, t_pavement, t_floor,    t_floor, t_console_broken, t_floor,  t_floor,
                                               t_door_bar_locked, t_grass, t_floor, t_bars, t_concrete_h, t_concrete_v, t_fence_barbed,
                                               t_chainfence_h, t_chainfence_v, t_floor),
                                       mapf::basic_bind("# E g r + = h c l w s _ o d x T b G . , B - | % f F S", f_null, f_exercise,
                                               f_null,               f_rack,  f_null,                 f_null,   f_chair, f_counter, f_locker,
                                               f_null,   f_null,     f_null,     f_bookcase, f_desk,  f_null,           f_toilet, f_bed,   f_null,
                                               f_null,  f_null,  f_null, f_null,       f_null,       f_null,         f_null,         f_null,
                                               f_sink));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_locker || this->furn(i, j) == f_rack ) {
                        place_items("cleaning", 60,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (t_west == "prison_b" && t_south == "prison_b") {
                rotate(1);
            } else if (t_north == "prison_b" && t_west == "prison_b") {
                rotate(2);
            } else if (t_north == "prison_b" && t_east == "prison_b") {
                rotate(3);
            }
        } else if (t_above == "prison_8") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
##|-|-|-|------|########\n\
##|T|T|T|l,,,,l|########\n\
##|=|=|=|l,,,,l|########\n\
##|,,,,,,,,,,,l|########\n\
##|-----|,,,,,l|--------\n\
##|,,,,>|,,,,,l|c,cSScee\n\
##|h,,,,+,,,,,,|o,,,,,,,\n\
##|t,,,,|cScScc|o,,,,,,,\n",
                                       mapf::basic_bind("t e o > O # E g r + = h c l w s _ d x T b G . , B - | % f F S", t_floor, t_floor,
                                               t_floor, t_stairs_up, t_column, t_rock, t_floor,    t_reinforced_glass_h, t_floor,
                                               t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,  t_window, t_sidewalk, t_pavement,
                                               t_floor, t_console_broken, t_floor,  t_floor, t_door_bar_locked, t_grass, t_floor, t_bars,
                                               t_concrete_h, t_concrete_v, t_fence_barbed, t_chainfence_h, t_chainfence_v, t_floor),
                                       mapf::basic_bind("t e o > O # E g r + = h c l w s _ d x T b G . , B - | % f F S", f_table, f_fridge,
                                               f_oven,  f_null,      f_null,   f_null, f_exercise, f_null,               f_rack,  f_null,
                                               f_null,   f_chair, f_counter, f_locker, f_null,   f_null,     f_null,     f_desk,  f_null,
                                               f_toilet, f_bed,   f_null,            f_null,  f_null,  f_null, f_null,       f_null,       f_null,
                                               f_null,         f_null,         f_sink));
            add_spawn("mon_zombie_cop", rng(0, 2), 12, 19);
            if (t_east != "prison_b") {
                rotate(1);
            } else if (t_south != "prison_b") {
                rotate(2);
            } else if (t_west != "prison_b") {
                rotate(3);
            }
        } else if (t_above == "prison_9") {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
##|-------|#############\n\
##|r,,r,,r|#############\n\
##|r,,r,,r|#############\n\
##|r,,r,,r|#############\n\
--|r,,r,,r|#############\n\
ee|,,,,,,r|#############\n\
,,+,,,,,,r|#############\n\
,,|-------|#############\n",
                                       mapf::basic_bind("e # r + = h c l w s _ o d x T b G . , B - | S", t_floor,  t_rock, t_floor,
                                               t_door_locked_interior, t_door_c, t_floor, t_floor,   t_floor,  t_window, t_sidewalk, t_pavement,
                                               t_floor,    t_floor, t_console_broken, t_floor,  t_floor, t_door_metal_locked, t_grass, t_floor,
                                               t_bars, t_concrete_h, t_concrete_v, t_floor),
                                       mapf::basic_bind("e # r + = h c l w s _ o d x T b G . , B - | S", f_fridge, f_null, f_rack,  f_null,
                                               f_null,   f_chair, f_counter, f_locker, f_null,   f_null,     f_null,     f_bookcase, f_desk,
                                               f_null,           f_toilet, f_bed,   f_null,              f_null,  f_null,  f_null, f_null,
                                               f_null,       f_sink));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_rack) {
                        place_items("cannedfood", 40,  i,  j, i,  j, false, 0);
                        place_items("pasta", 40,  i,  j, i,  j, false, 0);
                    }
                }
            }
            if (t_north == "prison_b" && t_west == "prison_b") {
                rotate(1);
            } else if (t_north == "prison_b" && t_east == "prison_b") {
                rotate(2);
            } else if (t_south == "prison_b" && t_east == "prison_b") {
                rotate(3);
            }
        }
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_toilet) {
                    if (one_in(3)) {
                        add_spawn("mon_zombie_brute", rng(0, 1), i, j);
                    } else if (one_in(3)) {
                        add_spawn("mon_zombie_grabber", rng(0, 1), i, j);
                    } else if (one_in(3)) {
                        add_spawn("mon_zombie_electric", rng(0, 1), i, j);
                    } else {
                        add_spawn("mon_zombie", rng(0, 1), i, j);
                    }
                }
                if (this->furn(i, j) == f_bed) {
                    place_items("novels", 30,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_fridge) {
                    place_items("fridge", 60,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_locker) {
                    place_items("cop_weapons", 20,  i,  j, i,  j, false, 0);
                    place_items("cop_torso", 20,  i,  j, i,  j, false, 0);
                    place_items("cop_pants", 20,  i,  j, i,  j, false, 0);
                    place_items("cop_shoes", 20,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_washer || this->furn(i, j) == f_dryer) {
                    if (one_in(4)) {
                        spawn_item(i, j, "blanket", 3);
                    } else if (one_in(3)) {
                        spawn_item(i, j, "jumpsuit", 3);
                    }
                }
            }
        }
        if (t_north == "prison_2") {
            rotate(3);
        } else if (t_east == "prison_2") {
            rotate(0);
        } else if (t_south == "prison_2") {
            rotate(1);
        } else if (t_west == "prison_2") {
            rotate(2);
        }


    } else if (terrain_type == "hotel_tower_1_1") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_______________________\n\
s_______________________\n\
s_______________________\n\
s_______________________\n\
s_______________________\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
ssssssssssssssssssssssss\n",
                                   mapf::basic_bind("_ , C x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                           t_door_c, t_door_locked_alarm, t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,
                                           t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ , C x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table,
                                           f_null,   f_null,              f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase,
                                           f_chair, f_counter, f_dresser, f_locker, f_null));
        place_spawns(g, "GROUP_ZOMBIE", 2, 6, 6, 18, 18, density);
        if (t_north == "hotel_tower_1_2") {
            rotate(3);
        } else if (t_east == "hotel_tower_1_2") {
            rotate(0);
        } else if (t_south == "hotel_tower_1_2") {
            rotate(1);
        } else if (t_west == "hotel_tower_1_2") {
            rotate(2);
        }


    } else if (terrain_type == "hotel_tower_1_2") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
,_____________________,_\n\
,________sssss________,_\n\
,________s   s________,_\n\
,________s   s________,_\n\
s________s   s________ss\n\
s________s   s________ss\n\
,________s   s________,_\n\
,________s   s________,_\n\
,________s   s________,_\n\
,________s   s________,_\n\
,________sssss________,_\n\
,_____________________,_\n\
________________________\n\
________________________\n\
________________________\n\
________________________\n\
________________________\n\
,_____________________,_\n\
,________sssss________,_\n\
,________s   s________,_\n\
,________s   s________,_\n\
,________s   s________,_\n\
,________s   s________,_\n\
s________sssss________ss\n",
                                   mapf::basic_bind("_ , C x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                           t_door_c, t_door_locked_alarm, t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,
                                           t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ , C x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table,
                                           f_null,   f_null,              f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase,
                                           f_chair, f_counter, f_dresser, f_locker, f_null));
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        if (t_north == "hotel_tower_1_5") {
            rotate(0);
        } else if (t_east == "hotel_tower_1_5") {
            rotate(1);
        } else if (t_south == "hotel_tower_1_5") {
            rotate(2);
        } else if (t_west == "hotel_tower_1_5") {
            rotate(3);
        }


    } else if (terrain_type == "hotel_tower_1_3") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
sssssssssssssssssssssss\n\
sssssssssssssssssssssss\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
______________________s\n\
______________________s\n\
______________________s\n\
______________________s\n\
______________________s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
sssssssssssssssssssssss\n",
                                   mapf::basic_bind("_ , C x $ ^ . - | # t + = D w T S e o h c d l s", t_pavement, t_pavement_y,
                                           t_column, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                           t_door_c, t_door_locked_alarm, t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,
                                           t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("_ , C x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_null,
                                           f_null,   f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table,
                                           f_null,   f_null,              f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase,
                                           f_chair, f_counter, f_dresser, f_locker, f_null));
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        if (t_north == "hotel_tower_1_2") {
            rotate(1);
            if (x_in_y(1, 12)) {
                add_vehicle (g, "car", 12, 18, 180);
            }
        } else if (t_east == "hotel_tower_1_2") {
            rotate(2);
            if (x_in_y(1, 12)) {
                add_vehicle (g, "car", 9, 7, 0);
            }
        } else if (t_south == "hotel_tower_1_2") {
            rotate(3);
            if (x_in_y(1, 12)) {
                add_vehicle (g, "car", 12, 18, 180);
            }
        } else if (t_west == "hotel_tower_1_2") {
            rotate(0);
            if (x_in_y(1, 12)) {
                add_vehicle (g, "car", 17, 7, 0);
            }
        }


    } else if (terrain_type == "hotel_tower_1_4") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
s    |c..BB|c..BB|c..BB|\n\
s    |c..BB|c..BB|c..BB|\n\
s    |....d|....d|....d|\n\
s    |-www-|-www-|-www-|\n\
s                       \n\
s          T     T      \n\
s                       \n\
s                       \n\
ssssssssssssssssssssssss\n\
ssssssssssssssssssssssss\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n\
s_______________________\n\
s_______________________\n\
s_______________________\n\
s_______________________\n\
s_______________________\n\
s_______________________\n\
s_____,_____,_____,_____\n\
s_____,_____,_____,_____\n",
                                   mapf::basic_bind("B _ , C x $ ^ . - | # t + = D w T S e o h c d l s", t_floor, t_pavement,
                                           t_pavement_y, t_column, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v,
                                           t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window, t_tree_young,  t_floor,
                                           t_floor,  t_floor,    t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("B _ , C x $ ^ . - | # t + = D w T S e o h c d l s", f_bed,   f_null,     f_null,
                                           f_null,   f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table,
                                           f_null,   f_null,              f_null,        f_null,   f_null,        f_sink,  f_fridge,
                                           f_bookcase, f_chair, f_counter, f_dresser, f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser && x_in_y(1, 2)) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_counter && x_in_y(1, 5)) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        if (t_north == "hotel_tower_1_5") {
            rotate(3);
        } else if (t_east == "hotel_tower_1_5") {
            rotate(0);
        } else if (t_south == "hotel_tower_1_5") {
            rotate(1);
        } else if (t_west == "hotel_tower_1_5") {
            rotate(2);
        }


    } else if (terrain_type == "hotel_tower_1_5") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
c..BB|t.........t|BB..c|\n\
h..BB|h.........h|BB..c|\n\
th..d|...........|d....|\n\
-www||...........||www-|\n\
    |............^|     \n\
 T  V.hh......|-+-|  T  \n\
    V.tt......|r.D|     \n\
    V.tt......c..h|     \n\
ssss|.hh......x...|sssss\n\
ssssV.........c...|sssss\n\
,sssV.........cccc|sss,_\n\
,sssV............^Vsss,_\n\
,sss|^............Vsss,_\n\
,sss|HHHGGHHHGGHHH|sss,_\n\
,sssssssssssssssssssss,_\n\
,sssssCsssssssssCsssss,_\n\
_sssssssssssssssssssss__\n\
________________________\n\
________________________\n\
________________________\n\
________________________\n\
________________________\n\
,_____________________,_\n\
,_____________________,_\n",
                                   mapf::basic_bind("T r V H G D B _ , C x $ ^ . - | # t + = w S e o h c d l s", t_tree_young, t_floor,
                                           t_wall_glass_v, t_wall_glass_h, t_door_glass_c, t_floor, t_floor, t_pavement, t_pavement_y,
                                           t_column, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor,
                                           t_door_c, t_door_locked_alarm, t_window, t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_floor,  t_sidewalk),
                                   mapf::basic_bind("T r V H G D B _ , C x $ ^ . - | # t + = w S e o h c d l s", f_null,       f_rack,
                                           f_null,         f_null,         f_null,         f_desk,  f_bed,   f_null,     f_null,       f_null,
                                           f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table, f_null,
                                           f_null,              f_null,   f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_dresser,
                                           f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser && x_in_y(1, 2)) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_counter && x_in_y(1, 5)) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        } else {
            if (x_in_y(1, 2)) {
                add_spawn("mon_zombie", 2, 15, 7);
            }
            if (x_in_y(1, 2)) {
                add_spawn("mon_zombie", rng(1, 8), 12, 11);
            }
        }
        {
            int num_carts = rng(1, 3);
            for( int i = 0; i < num_carts; i++ ) {
                add_vehicle (g, "luggage_cart", rng(5, 18), rng(2, 12), 90, -1, -1, false);
            }
        }
        if (t_north == "hotel_tower_1_2") {
            rotate(2);
        } else if (t_east == "hotel_tower_1_2") {
            rotate(3);
        } else if (t_south == "hotel_tower_1_2") {
            rotate(0);
        } else if (t_west == "hotel_tower_1_2") {
            rotate(1);
        }


    } else if (terrain_type == "hotel_tower_1_6") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
BB..c|BB..c|BB..c|    s\n\
BB..c|BB..c|BB..c|    s\n\
d....|d....|d....|    s\n\
-www-|-www-|-www-|    s\n\
                      s\n\
     T     T          s\n\
                      s\n\
                      s\n\
sssssssssssssssssssssss\n\
sssssssssssssssssssssss\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n\
______________________s\n\
______________________s\n\
______________________s\n\
______________________s\n\
______________________s\n\
______________________s\n\
____,_____,_____,_____s\n\
____,_____,_____,_____s\n",
                                   mapf::basic_bind("B _ , C x $ ^ . - | # t + = D w T S e o h c d l s", t_floor, t_pavement,
                                           t_pavement_y, t_column, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v,
                                           t_floor, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window, t_tree_young,  t_floor,
                                           t_floor,  t_floor,    t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("B _ , C x $ ^ . - | # t + = D w T S e o h c d l s", f_bed,   f_null,     f_null,
                                           f_null,   f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table,
                                           f_null,   f_null,              f_null,        f_null,   f_null,        f_sink,  f_fridge,
                                           f_bookcase, f_chair, f_counter, f_dresser, f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser && x_in_y(1, 2)) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_counter && x_in_y(1, 5)) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        if (t_north == "hotel_tower_1_5") {
            rotate(1);
        } else if (t_east == "hotel_tower_1_5") {
            rotate(2);
        } else if (t_south == "hotel_tower_1_5") {
            rotate(3);
        } else if (t_west == "hotel_tower_1_5") {
            rotate(0);
        }


    } else if (terrain_type == "hotel_tower_1_7") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
     |-www-|-www-|-www-|\n\
     |....d|th..d|....d|\n\
     |c..BB|h..BB|c..BB|\n\
     |c..BB|c..BB|c..BB|\n\
     |c....|c...d|c....|\n\
     |....h|c..BB|....h|\n\
     |...ht|...BB|...ht|\n\
     |..|--|..|--|..|--|\n\
     |..+.S|..+.S|..+.S|\n\
     |..|.T|..|.T|..|.T|\n\
  |--|..|bb|..|bb|..|bb|\n\
  |..|-D|--|-D|--|-D|--|\n\
ss|..|..................\n\
ss=..+..................\n\
ss|..|..................\n\
s |.<|-D|--|-D|--|-D|--|\n\
s |--|..|bb|..|bb|..|bb|\n\
s    |..|.T|..|.T|..|.T|\n\
s    |..+.S|..+.S|..+.S|\n\
s    |..|--|..|--|..|--|\n\
s    |...ht|...ht|...ht|\n\
s    |....h|....h|....h|\n\
s    |c....|c....|c....|\n",
                                   mapf::basic_bind("D b > < B _ , C x $ ^ . - | # t + = w T S e o h c d l s", t_door_locked_interior,
                                           t_floor,   t_stairs_up, t_stairs_down, t_floor, t_pavement, t_pavement_y, t_column,
                                           t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor, t_door_c,
                                           t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_floor,  t_sidewalk),
                                   mapf::basic_bind("D b > < B _ , C x $ ^ . - | # t + = w T S e o h c d l s", f_null,
                                           f_bathtub, f_null,      f_null,        f_bed,   f_null,     f_null,       f_null,   f_null,
                                           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table, f_null,   f_null,
                                           f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_dresser, f_locker,
                                           f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser && x_in_y(1, 2)) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_counter && x_in_y(1, 5)) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        } else {
            add_spawn("mon_zombie", rng(0, 12), 14, 11);
        }
        if (t_north == "hotel_tower_1_8") {
            rotate(3);
        } else if (t_east == "hotel_tower_1_8") {
            rotate(0);
        } else if (t_south == "hotel_tower_1_8") {
            rotate(1);
        } else if (t_west == "hotel_tower_1_8") {
            rotate(2);
        }


    } else if (terrain_type == "hotel_tower_1_8") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
-www-|-www-|-www-|-www-|\n\
....d|th..d|d....|d....|\n\
c..BB|h..BB|BB..c|BB..c|\n\
c..BB|c..BB|BB..c|BB..c|\n\
c....|c...d|....c|....c|\n\
....h|c..BB|h....|h....|\n\
...ht|...BB|th...|th...|\n\
..|--|..|--|--|..|--|..|\n\
..+.S|..+.S|S.+..|S.+..|\n\
..|.T|..|.T|T.|..|T.|..|\n\
..|bb|..|bb|bb|..|bb|..|\n\
-D|--|-D|--|--|D-|--|D-|\n\
........................\n\
........................\n\
........................\n\
-D|--|---|...|---|--|D-|\n\
..|bb|EEE=...|xEE|bb|..|\n\
..|.T|EEE=...=EEE|T.|..|\n\
..+.S|EEx|...=EEE|S.+..|\n\
..|--|---|...|---|--|..|\n\
...BB|^.........^|th...|\n\
c..BB|h.........h|h....|\n\
c...d|t.........t|....c|\n",
                                   mapf::basic_bind("E b > < B _ , C x $ ^ . - | # t + = D w T S e o h c d l s", t_elevator, t_floor,
                                           t_stairs_up, t_stairs_down, t_floor, t_pavement, t_pavement_y, t_column, t_elevator_control_off,
                                           t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_floor, t_floor, t_door_c, t_door_metal_c,
                                           t_door_locked_interior, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,
                                           t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("E b > < B _ , C x $ ^ . - | # t + = D w T S e o h c d l s", f_null,     f_bathtub,
                                           f_null,      f_null,        f_bed,   f_null,     f_null,       f_null,   f_null,
                                           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table, f_null,   f_null,
                                           f_null,                 f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter,
                                           f_dresser, f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser && x_in_y(1, 2)) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_counter && x_in_y(1, 5)) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 23, 23, 3, density);
        } else {
            add_spawn("mon_zombie", rng(1, 18), 12, 12);
        }
        if (t_north == "hotel_tower_1_5") {
            rotate(2);
        } else if (t_east == "hotel_tower_1_5") {
            rotate(3);
        } else if (t_south == "hotel_tower_1_5") {
            rotate(0);
        } else if (t_west == "hotel_tower_1_5") {
            rotate(1);
        }


    } else if (terrain_type == "hotel_tower_1_9") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
-www-|-www-|-www-|      \n\
d....|d..ht|d....|      \n\
BB..c|BB..h|BB..c|      \n\
BB..c|BB..c|BB..c|      \n\
....c|d...c|....c|      \n\
h....|BB..c|h....|      \n\
th...|BB...|th...|      \n\
--|..|--|..|--|..|      \n\
S.+..|S.+..|S.+..|      \n\
T.|..|T.|..|T.|..|      \n\
bb|..|bb|..|bb|..|--|   \n\
--|D-|--|D-|--|D-|..|   \n\
................^|..|ss \n\
.................+..=ss \n\
.................|..|ss \n\
--|D-|--|D-|--|D-|<.| s \n\
bb|..|bb|..|bb|..|--| s \n\
T.|..|T.|..|T.|..|    s \n\
S.+..|S.+..|S.+..|    s \n\
--|..|--|..|--|..|    s \n\
th...|th...|th...|    s \n\
h....|h....|h....|    s \n\
....c|....c|....c|    s \n",
                                   mapf::basic_bind("D E b > < B _ , C x $ ^ . - | # t + = w T S e o h c d l s",
                                           t_door_locked_interior, t_elevator, t_floor,   t_stairs_up, t_stairs_down, t_floor, t_pavement,
                                           t_pavement_y, t_column, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v,
                                           t_floor, t_floor, t_door_c, t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,
                                           t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("D E b > < B _ , C x $ ^ . - | # t + = w T S e o h c d l s", f_null,
                                           f_null,     f_bathtub, f_null,      f_null,        f_bed,   f_null,     f_null,       f_null,
                                           f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_bench, f_table, f_null,
                                           f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_dresser,
                                           f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_dresser && x_in_y(1, 2)) {
                    place_items("dresser", 70,  i,  j, i,  j, false, 0);
                } else if (this->furn(i, j) == f_counter && x_in_y(1, 5)) {
                    place_items("magazines", 30,  i,  j, i,  j, false, 0);
                }
            }
        }
        if (density > 1) {
            place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, 23, 23, density);
        } else {
            add_spawn("mon_zombie", rng(1, 8), 12, 12);
        }
        if (t_north == "hotel_tower_1_8") {
            rotate(1);
        } else if (t_east == "hotel_tower_1_8") {
            rotate(2);
        } else if (t_south == "hotel_tower_1_8") {
            rotate(3);
        } else if (t_west == "hotel_tower_1_8") {
            rotate(0);
        }


    } else if (terrain_type == "hotel_tower_b_1") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
########################\n\
#####|--------------|---\n\
#####|......E..ee...|^ht\n\
#####|..............|...\n\
#####|...........E..V...\n\
#####|..............V...\n\
#####|.E.........E..V...\n\
#####|..............V...\n\
#####|..............G...\n\
#####|..............V...\n\
#####|..E..E..E..E..V...\n\
##|--|.............^|...\n\
##|.<|-----HHHHHHHH-|...\n\
##|..|..................\n\
##|..+..................\n\
##|..|^..............htt\n\
##|..|-----------------|\n\
##|--|#################|\n\
#######################|\n\
#######################|\n\
#######################|\n\
#######################|\n\
#######################|\n\
#######################|\n",
                                   mapf::basic_bind("E < H V G C x ^ . - | # t + = D w T S e o h c d l s", t_floor,    t_stairs_up,
                                           t_wall_glass_h, t_wall_glass_v, t_door_glass_c, t_column, t_console_broken, t_floor,        t_floor,
                                           t_wall_h, t_wall_v, t_rock, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                           t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("E < H V G C x ^ . - | # t + = D w T S e o h c d l s", f_exercise, f_null,
                                           f_null,         f_null,         f_null,         f_null,   f_null,           f_indoor_plant, f_null,
                                           f_null,   f_null,   f_null, f_table, f_null,   f_null,              f_null,        f_null,
                                           f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_dresser, f_locker, f_null));
        place_items("snacks", 60,  15,  2, 16,  2, false, 0);
        add_spawn("mon_sewer_snake", rng(0, 3), SEEX, SEEY);
        if (t_north == "hotel_tower_b_2") {
            rotate(3);
        } else if (t_east == "hotel_tower_b_2") {
            rotate(0);
        } else if (t_south == "hotel_tower_b_2") {
            rotate(1);
        } else if (t_west == "hotel_tower_b_2") {
            rotate(2);
        }


    } else if (terrain_type == "hotel_tower_b_2") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
########################\n\
------------------------\n\
th..........^rrr^.......\n\
........................\n\
.sssssssssssssssssssssss\n\
.sWWWWWWWWWwwwwwwwwwwwws\n\
.sWWWWWWWWWwwwwwwwwwwwws\n\
.sWWWWWWWWWwwwwwwwwwwwws\n\
.sWWWWWWWWWwwwwwwwwwwwws\n\
.sWWWWWWWWWwwwwwwwwwwwws\n\
.sWWWWWWWWWwwwwwwwwwwwws\n\
.sssssssssssssssssssssss\n\
........................\n\
........................\n\
........................\n\
h......................h\n\
---+-|---|HGH|---|-+---|\n\
T|..c|EEE+...|xEE|c..|T|\n\
.+..S|EEE+...+EEE|S..+.|\n\
-|..c|EEx|^..+EEE|c..|-|\n\
T+..S|---|-+-|---|S..+T|\n\
-|..c|l..........|c..|-|\n\
T+...|l..l...rrr.|...+T|\n\
-----|-----------|-----|\n",
                                   mapf::basic_bind("E r W w H V G C x ^ . - | # t + = D T S e o h c d l s", t_elevator, t_floor,
                                           t_water_dp, t_water_sh, t_wall_glass_h, t_wall_glass_v, t_door_glass_c, t_column,
                                           t_elevator_control_off, t_floor,        t_floor, t_wall_h, t_wall_v, t_rock, t_floor, t_door_c,
                                           t_door_locked_alarm, t_door_locked, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,
                                           t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("E r W w H V G C x ^ . - | # t + = D T S e o h c d l s", f_null,     f_rack,
                                           f_null,     f_null,     f_null,         f_null,         f_null,         f_null,   f_null,
                                           f_indoor_plant, f_null,  f_null,   f_null,   f_null, f_table, f_null,   f_null,              f_null,
                                           f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_dresser, f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_locker) {
                    place_items("cleaning", 60,  i,  j, i,  j, false, 0);
                }
            }
        }
        add_spawn("mon_sewer_snake", rng(0, 10), SEEX, SEEY);
        if (t_north == "hotel_tower_b_1") {
            rotate(1);
        } else if (t_east == "hotel_tower_b_1") {
            rotate(2);
        } else if (t_south == "hotel_tower_b_1") {
            rotate(3);
        } else if (t_west == "hotel_tower_b_1") {
            rotate(0);
        }


    } else if (terrain_type == "hotel_tower_b_3") {

        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
########################\n\
----|------------|######\n\
...^|............|######\n\
....|.$$$$PP$$$$$|######\n\
....|........$...|######\n\
....|........$...|######\n\
....|............|######\n\
....|---+--------|######\n\
....|r....rrDDDDc|######\n\
....|r..........S|######\n\
....|r..........c|######\n\
....|r......WWWWc|--|###\n\
....|---++-------|<.|###\n\
.................|..|###\n\
.................+..|###\n\
tth.............^|..|###\n\
-----------------|..|###\n\
#################|--|###\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n\
########################\n",
                                   mapf::basic_bind("r P $ W D < H V G C x ^ . - | # t + = w T S e o h c d l s", t_floor,
                                           t_sewage_pump, t_sewage_pipe, t_floor,  t_floor, t_stairs_up, t_wall_glass_h, t_wall_glass_v,
                                           t_door_glass_c, t_column, t_console_broken, t_floor,        t_floor, t_wall_h, t_wall_v, t_rock,
                                           t_floor, t_door_c, t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor,
                                           t_floor,   t_floor,   t_floor,  t_sidewalk),
                                   mapf::basic_bind("r P $ W D < H V G C x ^ . - | # t + = w T S e o h c d l s", f_rack,  f_null,
                                           f_null,        f_washer, f_dryer, f_null,      f_null,         f_null,         f_null,
                                           f_null,   f_null,           f_indoor_plant, f_null,  f_null,   f_null,   f_null, f_table, f_null,
                                           f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_dresser,
                                           f_locker, f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_rack) {
                    place_items("home_hw", 80,  i,  j, i,  j, false, 0);
                }
                if (this->furn(i, j) == f_washer || this->furn(i, j) == f_dryer) {
                    if (x_in_y(1, 2)) {
                        spawn_item(i, j, "blanket", 3);
                    } else if (x_in_y(1, 3)) {
                        place_items("dresser", 80,  i,  j, i,  j, false, 0);
                    }
                }
            }
        }
        add_spawn("mon_sewer_snake", rng(0, 3), SEEX, SEEY);
        if (t_north == "hotel_tower_b_2") {
            rotate(1);
        } else if (t_east == "hotel_tower_b_2") {
            rotate(2);
        } else if (t_south == "hotel_tower_b_2") {
            rotate(3);
        } else if (t_west == "hotel_tower_b_2") {
            rotate(0);
        }


    } else if (is_ot_type("office_doctor", terrain_type)) {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
   |---|----|--------|  \n\
   |..l|.T.S|..eccScc|  \n\
   |...+....+........D  \n\
   |--------|.......6|r \n\
   |o.......|..|--X--|r \n\
   |d.hd.h..+..|l...6|  \n\
   |o.......|..|l...l|  \n\
   |--------|..|l...l|  \n\
   |l....ccS|..|lllll|  \n\
   |l..t....+..|-----|  \n\
   |l.......|..|....l|  \n\
   |--|-----|..|.t..l|  \n\
   |T.|d.......+....l|  \n\
   |S.|d.h..|..|Scc..|  \n\
   |-+|-ccc-|++|-----|  \n\
   |.................|  \n\
   w....####....####.w  \n\
   w.................w  \n\
   |....####....####.|  \n\
   |.................|  \n\
   |-++--wwww-wwww---|  \n\
     ss                 \n\
     ss                 \n",
                                   mapf::basic_bind(". - | 6 X # r t + = D w T S e o h c d l s", t_floor, t_wall_h, t_wall_v,
                                           t_console, t_door_metal_locked, t_floor, t_floor,    t_floor, t_door_c, t_door_locked_alarm,
                                           t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,
                                           t_floor,  t_sidewalk),
                                   mapf::basic_bind(". - | 6 X # r t + = D w T S e o h c d l s", f_null,  f_null,   f_null,   f_null,
                                           f_null,              f_bench, f_trashcan, f_table, f_null,   f_null,              f_null,
                                           f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
        tmpcomp = add_computer(20, 4, _("Medical Supply Access"), 2);
        tmpcomp->add_option(_("Lock Door"), COMPACT_LOCK, 2);
        tmpcomp->add_option(_("Unlock Door"), COMPACT_UNLOCK, 2);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);

        tmpcomp = add_computer(20, 6, _("Medical Supply Access"), 2);
        tmpcomp->add_option(_("Unlock Door"), COMPACT_UNLOCK, 2);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);

        if (one_in(2)) {
            spawn_item(7, 6, "record_patient");
        }
        place_items("dissection", 60,  4,  9, 4,  11, false, 0);
        place_items("dissection", 60,  9,  9, 10,  9, false, 0);
        place_items("dissection", 60,  20,  11, 20,  13, false, 0);
        place_items("dissection", 60,  17,  14, 18,  14, false, 0);
        place_items("fridge", 50,  15,  2, 15,  2, false, 0);
        place_items("surgery", 30,  4,  9, 11,  11, false, 0);
        place_items("surgery", 30,  16,  11, 20, 4, false, 0);
        place_items("harddrugs", 60,  16,  6, 16, 9, false, 0);
        place_items("harddrugs", 60,  17,  9, 19, 9, false, 0);
        place_items("softdrugs", 60,  20,  9, 20, 7, false, 0);
        place_items("cleaning", 50,  4,  2, 6,  3, false, 0);

        if (terrain_type == "office_doctor_east") {
            rotate(3);
        }
        if (terrain_type == "office_doctor_north") {
            rotate(2);
        }
        if (terrain_type == "office_doctor_west") {
            rotate(1);
        }


    } else if (terrain_type == "toxic_dump") {

        fill_background(this, t_dirt);
        for (int n = 0; n < 6; n++) {
            int poolx = rng(4, SEEX * 2 - 5), pooly = rng(4, SEEY * 2 - 5);
            for (int i = poolx - 3; i <= poolx + 3; i++) {
                for (int j = pooly - 3; j <= pooly + 3; j++) {
                    if (rng(2, 5) > rl_dist(poolx, pooly, i, j)) {
                        ter_set(i, j, t_sewage);
                        radiation(i, j) += rng(20, 60);
                    }
                }
            }
        }
        int buildx = rng(6, SEEX * 2 - 7), buildy = rng(6, SEEY * 2 - 7);
        square(this, t_floor, buildx - 3, buildy - 3, buildx + 3, buildy + 3);
        line(this, t_wall_h, buildx - 4, buildy - 4, buildx + 4, buildy - 4);
        line(this, t_wall_h, buildx - 4, buildy + 4, buildx + 4, buildy + 4);
        line(this, t_wall_v, buildx - 4, buildy - 4, buildx - 4, buildy + 4);
        line(this, t_wall_v, buildx + 4, buildy - 4, buildx + 4, buildy + 4);
        line_furn(this, f_counter, buildx - 3, buildy - 3, buildx + 3, buildy - 3);
        place_items("toxic_dump_equipment", 80,
                    buildx - 3, buildy - 3, buildx + 3, buildy - 3, false, 0);
        spawn_item(buildx, buildy, "id_military");
        ter_set(buildx, buildy + 4, t_door_locked);

        rotate(rng(0, 3));


    } else if (terrain_type == "haz_sar_entrance") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
 f    |_________%..S| |.\n\
 f    |_________|..r| |.\n\
 f    |_________|..r| |.\n\
 f    |l________=..r| |c\n\
 f    |l________|..S| |w\n\
 f    |l________%..r|sss\n\
 f    |_________%..r|sss\n\
 f    |_________%..r|ss_\n\
 f    |_________|x..|ss_\n\
 f    |-XXXXXXX-|-D-|ss_\n\
 f     s_______ssssssss_\n\
 f     s_______ssssssss_\n\
 f     s________________\n\
 f     s________________\n\
 f     s________________\n\
 f  ssss________________\n\
 f  ssss_______ssssssss_\n\
 fF|-D-|XXXXXXX-      s_\n\
   wxh.D_______f      s_\n\
   wcdcw_______f      ss\n\
   |www|_______fFFFFFFFF\n\
        _______         \n\
        _______         \n\
        _______         \n",
                                   mapf::basic_bind("1 & V C G 5 % Q E , _ r X f F 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                           t_sewage_pipe, t_sewage_pump, t_vat,  t_floor,   t_grate, t_wall_glass_h, t_wall_glass_v, t_sewage,
                                           t_elevator, t_pavement_y, t_pavement, t_floor, t_door_metal_locked, t_chainfence_v, t_chainfence_h,
                                           t_console, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_rock, t_floor,
                                           t_door_c, t_door_metal_c, t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor,
                                           t_floor,   t_floor, t_floor,  t_sidewalk),
                                   mapf::basic_bind("1 & V C G 5 % Q E , _ r X f F 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                           f_null,        f_null,        f_null, f_crate_c, f_null,  f_null,         f_null,         f_null,
                                           f_null,     f_null,       f_null,     f_rack,  f_null,              f_null,         f_null,
                                           f_null,    f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_null, f_table,
                                           f_null,   f_null,         f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair,
                                           f_counter, f_desk,  f_locker, f_null));
        spawn_item(19, 3, "cleansuit");
        place_items("office", 80,  4, 19, 6, 19, false, 0);
        place_items("cleaning", 90,  7,  3, 7,  5, false, 0);
        place_items("toxic_dump_equipment", 85,  19,  1, 19,  3, false, 0);
        place_items("toxic_dump_equipment", 85,  19,  5, 19,  7, false, 0);
        if (x_in_y(1, 2)) {
            add_spawn("mon_hazmatbot", 1, 10, 5);
        }
        //lazy radiation mapping
        for (int x = 0; x <= 23; x++) {
            for (int y = 0; y <= 23; y++) {
                radiation(x, y) += rng(10, 30);
            }
        }
        if (t_north == "haz_sar" && t_west == "haz_sar") {
            rotate(3);
        } else if (t_north == "haz_sar" && t_east == "haz_sar") {
            rotate(0);
        } else if (t_south == "haz_sar" && t_east == "haz_sar") {
            rotate(1);
        } else if (t_west == "haz_sar" && t_south == "haz_sar") {
            rotate(2);
        }


    } else if (terrain_type == "haz_sar") {

        fill_background(this, &grass_or_dirt);
        if ((t_south == "haz_sar_entrance" && t_east == "haz_sar") || (t_north == "haz_sar" &&
                t_east == "haz_sar_entrance") || (t_west == "haz_sar" && t_north == "haz_sar_entrance") ||
            (t_south == "haz_sar" && t_west == "haz_sar_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
 fFFFFFFFFFFFFFFFFFFFFFF\n\
 f                      \n\
 f                      \n\
 f     #################\n\
 f    ##################\n\
 f   ##...llrr..........\n\
 f  ##.._________.......\n\
 f  ##.._________&&&1111\n\
 f  ##..________x&&&....\n\
 f  ##..____________....\n\
 f  ##r.____________....\n\
 f  ##r.____________....\n\
 f  ##r.____________....\n\
 f  ##r.____________..CC\n\
 f  ##..___________...CC\n\
 f  ##..__________....C.\n\
 f  ##.._________.......\n\
 f  ##..________........\n\
 f  ###._______x##.#####\n\
 f  ####XXXXXXX###+#####\n\
 f   ##________x|x.r|   \n\
 f    |_________%..r| |-\n\
 f    |_________%..r| |^\n",
                                       mapf::basic_bind("1 & V C G 5 % Q E , _ r X f F 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                               t_sewage_pipe, t_sewage_pump, t_vat,  t_floor,   t_grate, t_wall_glass_h, t_wall_glass_v, t_sewage,
                                               t_elevator, t_pavement_y, t_pavement, t_floor, t_door_metal_locked, t_chainfence_v, t_chainfence_h,
                                               t_console, t_console_broken, t_shrub, t_floor,        t_floor, t_wall_h, t_wall_v, t_rock, t_floor,
                                               t_door_c, t_door_metal_c, t_door_locked, t_window, t_floor,  t_floor, t_floor,  t_floor,    t_floor,
                                               t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("1 & V C G 5 % Q E , _ r X f F 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                               f_null,        f_null,        f_null, f_crate_c, f_null,  f_null,         f_null,         f_null,
                                               f_null,     f_null,       f_null,     f_rack,  f_null,              f_null,         f_null,
                                               f_null,    f_null,           f_null,  f_indoor_plant, f_null,  f_null,   f_null,   f_null, f_table,
                                               f_null,   f_null,         f_null,        f_null,   f_toilet, f_sink,  f_fridge, f_bookcase, f_chair,
                                               f_counter, f_desk,  f_locker, f_null));
            spawn_item(19, 22, "cleansuit");
            place_items("cleaning", 85,  6,  11, 6,  14, false, 0);
            place_items("tools", 85,  10,  6, 13,  6, false, 0);
            place_items("toxic_dump_equipment", 85,  22,  14, 23,  15, false, 0);
            if (x_in_y(1, 2)) {
                add_spawn("mon_hazmatbot", 1, 22, 12);
            }
            if (x_in_y(1, 2)) {
                add_spawn("mon_hazmatbot", 1, 23, 18);
            }
            //lazy radiation mapping
            for (int x = 0; x <= 23; x++) {
                for (int y = 0; y <= 23; y++) {
                    radiation(x, y) += rng(10, 30);
                }
            }
            if (t_west == "haz_sar_entrance") {
                rotate(1);
                if (x_in_y(1, 4)) {
                    add_vehicle (g, "military_cargo_truck", 10, 11, 0);
                }
            } else if (t_north == "haz_sar_entrance") {
                rotate(2);
                if (x_in_y(1, 4)) {
                    add_vehicle (g, "military_cargo_truck", 12, 10, 90);
                }
            } else if (t_east == "haz_sar_entrance") {
                rotate(3);
                if (x_in_y(1, 4)) {
                    add_vehicle (g, "military_cargo_truck", 13, 12, 180);
                }
            } else if (x_in_y(1, 4)) {
                add_vehicle (g, "military_cargo_truck", 11, 13, 270);
            }

        }

        else if ((t_west == "haz_sar_entrance" && t_north == "haz_sar") || (t_north == "haz_sar_entrance" &&
                 t_east == "haz_sar") || (t_west == "haz_sar" && t_south == "haz_sar_entrance") ||
                 (t_south == "haz_sar" && t_east == "haz_sar_entrance")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
......|-+-|-+|...h..w f \n\
.c....|.............w f \n\
hd....+....ch.....hdw f \n\
cc....|....cdd...ddd| f \n\
ww-www|w+w-www--www-| f \n\
ssssssssssssssssssss  f \n\
ssssssssssssssssssss  f \n\
___,____,____,____ss  f \n\
___,____,____,____ss  f \n\
___,____,____,____ss  f \n\
___,____,____,____ss  f \n\
___,____,____,____ss  f \n\
__________________ss  f \n\
__________________ss  f \n\
__________________ss  f \n\
__________________ss  f \n\
________,_________ss  f \n\
________,_________ss  f \n\
________,_________ss  f \n\
ssssssssssssssssssss  f \n\
FFFFFFFFFFFFFFFFFFFFFFf \n\
                        \n\
                        \n\
                        \n",
                                       mapf::basic_bind("1 & V C G 5 % Q E , _ r X f F V H 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                               t_sewage_pipe, t_sewage_pump, t_vat,  t_floor,   t_grate, t_wall_glass_h, t_wall_glass_v, t_sewage,
                                               t_elevator, t_pavement_y, t_pavement, t_floor, t_door_metal_locked, t_chainfence_v, t_chainfence_h,
                                               t_wall_glass_v, t_wall_glass_h, t_console, t_console_broken, t_shrub, t_floor,        t_floor,
                                               t_wall_h, t_wall_v, t_rock, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                               t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("1 & V C G 5 % Q E , _ r X f F V H 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                               f_null,        f_null,        f_null, f_crate_c, f_null,  f_null,         f_null,         f_null,
                                               f_null,     f_null,       f_null,     f_rack,  f_null,              f_null,         f_null,
                                               f_null,         f_null,         f_null,    f_null,           f_null,  f_indoor_plant, f_null,
                                               f_null,   f_null,   f_null, f_table, f_null,   f_null,              f_null,        f_null,
                                               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            spawn_item(1, 2, "id_military");
            place_items("office", 85,  1,  1, 1,  3, false, 0);
            place_items("office", 85,  11,  3, 13,  3, false, 0);
            place_items("office", 85,  17,  3, 19,  3, false, 0);
            //lazy radiation mapping
            for (int x = 0; x <= 23; x++) {
                for (int y = 0; y <= 23; y++) {
                    radiation(x, y) += rng(10, 30);
                }
            }
            if (t_north == "haz_sar_entrance") {
                rotate(1);
            }
            if (t_east == "haz_sar_entrance") {
                rotate(2);
            }
            if (t_south == "haz_sar_entrance") {
                rotate(3);
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
FFFFFFFFFFFFFFFFFFFFFFf \n\
                      f \n\
                      f \n\
################      f \n\
#################     f \n\
.V.V.V..........##    f \n\
.......|G|.......##   f \n\
11111111111111...##   f \n\
.......|G|.%515%.##   f \n\
...........%QQQ%.##   f \n\
..CC......x%QQQ%.##   f \n\
.CCC.......%QQQ%.##   f \n\
...........%QQQ%.##   f \n\
.....|.R|..%515%.##   f \n\
......EE|....1...##   f \n\
......EE|....&...##   f \n\
.....---|.......##    f \n\
...............##     f \n\
################      f \n\
###############       f \n\
                      f \n\
------|---|--|---www| f \n\
.x6x..|S.T|l.|^.ddd.| f \n",
                                       mapf::basic_bind("R 1 & V C G 5 % Q E , _ r X f F 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                               t_elevator_control_off, t_sewage_pipe, t_sewage_pump, t_vat,  t_floor,   t_grate, t_wall_glass_h,
                                               t_wall_glass_v, t_sewage, t_elevator, t_pavement_y, t_pavement, t_floor, t_door_metal_locked,
                                               t_chainfence_v, t_chainfence_h, t_console, t_console_broken, t_shrub, t_floor,        t_floor,
                                               t_wall_h, t_wall_v, t_rock, t_floor, t_door_c, t_door_locked_alarm, t_door_locked, t_window,
                                               t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor, t_floor,  t_sidewalk),
                                       mapf::basic_bind("R 1 & V C G 5 % Q E , _ r X f F 6 x $ ^ . - | # t + = D w T S e o h c d l s",
                                               f_null,                 f_null,        f_null,        f_null, f_crate_c, f_null,  f_null,
                                               f_null,         f_null,   f_null,     f_null,       f_null,     f_rack,  f_null,
                                               f_null,         f_null,         f_null,    f_null,           f_null,  f_indoor_plant, f_null,
                                               f_null,   f_null,   f_null, f_table, f_null,   f_null,              f_null,        f_null,
                                               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null));
            place_items("office", 85,  16,  23, 18,  23, false, 0);
            place_items("cleaning", 85,  11,  23, 12,  23, false, 0);
            place_items("robots", 90,  2,  11, 3,  11, false, 0);
            if (x_in_y(1, 2)) {
                add_spawn("mon_hazmatbot", 1, 7, 10);
            }
            if (x_in_y(1, 2)) {
                add_spawn("mon_hazmatbot", 1, 11, 16);
            }
            //lazy radiation mapping
            for (int x = 0; x <= 23; x++) {
                for (int y = 0; y <= 23; y++) {
                    radiation(x, y) += rng(10, 30);
                }
            }
            tmpcomp = add_computer(2, 23, _("SRCF Security Terminal"), 0);
            tmpcomp->add_option(_("Security Reminder [1055]"), COMPACT_SR1_MESS, 0);
            tmpcomp->add_option(_("Security Reminder [1056]"), COMPACT_SR2_MESS, 0);
            tmpcomp->add_option(_("Security Reminder [1057]"), COMPACT_SR3_MESS, 0);
            //tmpcomp->add_option(_("Security Reminder [1058]"), COMPACT_SR4_MESS, 0); limited to 9 computer options
            tmpcomp->add_option(_("EPA: Report All Potential Containment Breaches [3873643]"),
                                COMPACT_SRCF_1_MESS, 2);
            tmpcomp->add_option(_("SRCF: Internal Memo, EPA [2918024]"), COMPACT_SRCF_2_MESS, 2);
            tmpcomp->add_option(_("CDC: Internal Memo, Standby [2918115]"), COMPACT_SRCF_3_MESS, 2);
            tmpcomp->add_option(_("USARMY: SEAL SRCF [987167]"), COMPACT_SRCF_SEAL_ORDER, 4);
            tmpcomp->add_option(_("COMMAND: REACTIVATE ELEVATOR"), COMPACT_SRCF_ELEVATOR, 0);
            tmpcomp->add_option(_("COMMAND: SEAL SRCF [4423]"), COMPACT_SRCF_SEAL, 5);
            tmpcomp->add_failure(COMPFAIL_ALARM);
            if (t_west == "haz_sar" && t_north == "haz_sar") {
                rotate(1);
            }
            if (t_east == "haz_sar" && t_north == "haz_sar") {
                rotate(2);
            }
            if (t_east == "haz_sar" && t_south == "haz_sar") {
                rotate(3);
            }
        }


    } else if (terrain_type == "haz_sar_entrance_b1") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
#############...........\n\
#############...........\n\
|---------|#............\n\
|_________|M............\n\
|_________$.............\n\
|_________$.............\n\
|_________$.............\n\
|_________$.............\n\
|_________$.............\n\
|_________|.............\n\
|---------|#............\n\
############............\n\
###########.............\n\
###########M......####..\n\
#########|--$$$$$--|####\n\
####|----|_________|----\n\
####|___________________\n\
####|___________________\n\
####|___________________\n\
####|___________________\n\
####|___________________\n\
####|___________________\n\
####|___________________\n\
####|-------------------\n",
                                   mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", t_door_metal_c, t_door_metal_o,
                                           t_elevator, t_elevator_control_off, t_console, t_reinforced_glass_h, t_reinforced_glass_v, t_floor,
                                           t_floor, t_floor, t_floor, t_gates_control_concrete, t_sewage, t_door_metal_locked, t_concrete_v,
                                           t_concrete_h, t_rock, t_rock_floor, t_metal_floor, t_floor,  t_floor, t_floor),
                                   mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", f_null,         f_null,
                                           f_null,     f_null,                 f_null,    f_null,               f_null,
                                           f_counter, f_chair, f_desk,  f_rack,  f_null,                   f_null,   f_null,
                                           f_null,       f_null,       f_null, f_null,       f_null,        f_locker, f_sink,  f_toilet));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->ter(i, j) == t_rock_floor) {
                    if (one_in(250)) {
                        item body;
                        body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                        add_item(i, j, body);
                        place_items("science",  70, i, j, i, j, true, 0);
                    } else if (one_in(80)) {
                        add_spawn("mon_zombie", 1, i, j);
                    }
                }
                if (this->ter(i, j) != t_metal_floor) {
                    radiation(x, y) += rng(10, 70);
                }
                if (this->ter(i, j) == t_sewage) {
                    if (one_in(2)) {
                        ter_set(i, j, t_dirtfloor);
                    }
                    if (one_in(4)) {
                        ter_set(i, j, t_dirtmound);
                    }
                    if (one_in(2)) {
                        ter_set(i, j, t_wreckage);
                    }
                    place_items("trash", 50,  i,  j, i,  j, false, 0);
                    place_items("sewer", 50,  i,  j, i,  j, false, 0);
                    if (one_in(5)) {
                        if (one_in(10)) {
                            add_spawn("mon_zombie_child", 1, i, j);
                        } else if (one_in(15)) {
                            add_spawn("mon_zombie_fast", 1, i, j);
                        } else {
                            add_spawn("mon_zombie", 1, i, j);
                        }
                    }
                }
            }
        }
        if (t_north == "haz_sar_b1" && t_west == "haz_sar_b1") {
            rotate(3);
        } else if (t_north == "haz_sar_b1" && t_east == "haz_sar_b1") {
            rotate(0);
        } else if (t_south == "haz_sar_b1" && t_east == "haz_sar_b1") {
            rotate(1);
        } else if (t_west == "haz_sar_b1" && t_south == "haz_sar_b1") {
            rotate(2);
        }


    } else if (terrain_type == "haz_sar_b1") {

        fill_background(this, &grass_or_dirt);
        if ((t_south == "haz_sar_entrance_b1" && t_east == "haz_sar_b1") || (t_north == "haz_sar_b1" &&
                t_east == "haz_sar_entrance_b1") || (t_west == "haz_sar_b1" && t_north == "haz_sar_entrance_b1") ||
            (t_south == "haz_sar_b1" && t_west == "haz_sar_entrance_b1")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
####################.M..\n\
####################--$$\n\
####|----------|###.....\n\
####|__________|M.......\n\
####|__________$........\n\
####|__________$........\n\
####|__________$........\n\
####|__________$........\n\
####|__________$........\n\
####|__________|........\n\
####|----------|........\n\
###############.........\n\
##############..........\n\
#############...........\n\
############...........#\n\
|---------|#.........###\n\
|_________|M.........###\n\
|_________$..........|--\n\
|_________$..........|r,\n\
|_________$..........|r,\n\
|_________$..........|r,\n\
|_________$..........|,,\n\
|_________|..........|,,\n\
|---------|#.........|-$\n",
                                       mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", t_door_metal_c, t_door_metal_o,
                                               t_elevator, t_elevator_control_off, t_console, t_reinforced_glass_h, t_reinforced_glass_v, t_floor,
                                               t_floor, t_floor, t_floor, t_gates_control_concrete, t_sewage, t_door_metal_locked, t_concrete_v,
                                               t_concrete_h, t_rock, t_rock_floor, t_metal_floor, t_floor,  t_floor, t_floor),
                                       mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", f_null,         f_null,
                                               f_null,     f_null,                 f_null,    f_null,               f_null,
                                               f_counter, f_chair, f_desk,  f_rack,  f_null,                   f_null,   f_null,
                                               f_null,       f_null,       f_null, f_null,       f_null,        f_locker, f_sink,  f_toilet));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_rack) {
                        place_items("mechanics", 60,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_rock_floor) {
                        if (one_in(250)) {
                            item body;
                            body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                            add_item(i, j, body);
                            place_items("science",  70, i, j, i, j, true, 0);
                        } else if (one_in(80)) {
                            add_spawn("mon_zombie", 1, i, j);
                        }
                    }
                    if (this->ter(i, j) != t_metal_floor) {
                        radiation(x, y) += rng(10, 70);
                    }
                    if (this->ter(i, j) == t_sewage) {
                        if (one_in(2)) {
                            ter_set(i, j, t_dirtfloor);
                        }
                        if (one_in(4)) {
                            ter_set(i, j, t_dirtmound);
                        }
                        if (one_in(2)) {
                            ter_set(i, j, t_wreckage);
                        }
                        place_items("trash", 50,  i,  j, i,  j, false, 0);
                        place_items("sewer", 50,  i,  j, i,  j, false, 0);
                        if (one_in(5)) {
                            if (one_in(10)) {
                                add_spawn("mon_zombie_child", 1, i, j);
                            } else if (one_in(15)) {
                                add_spawn("mon_zombie_fast", 1, i, j);
                            } else {
                                add_spawn("mon_zombie", 1, i, j);
                            }
                        }
                    }
                }
            }
            if (t_west == "haz_sar_entrance_b1") {
                rotate(1);
            } else if (t_north == "haz_sar_entrance_b1") {
                rotate(2);
            } else if (t_east == "haz_sar_entrance_b1") {
                rotate(3);
            }
        }

        else if ((t_west == "haz_sar_entrance_b1" && t_north == "haz_sar_b1") ||
                 (t_north == "haz_sar_entrance_b1" && t_east == "haz_sar_b1") || (t_west == "haz_sar_b1" &&
                         t_south == "haz_sar_entrance_b1") ||
                 (t_south == "haz_sar_b1" && t_east == "haz_sar_entrance_b1")) {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
....M..|,,,,|........###\n\
.......|-HH=|.........##\n\
.....................###\n\
......................##\n\
......................|#\n\
......................$.\n\
......................$.\n\
......................$.\n\
......................$.\n\
......................$.\n\
.....................M|#\n\
....................####\n\
..................######\n\
###....M.........#######\n\
#####|--$$$$$--|########\n\
|----|_________|----|###\n\
|___________________|###\n\
|___________________|###\n\
|___________________|###\n\
|___________________|###\n\
|___________________|###\n\
|___________________|###\n\
|___________________|###\n\
|-------------------|###\n",
                                       mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", t_door_metal_c, t_door_metal_o,
                                               t_elevator, t_elevator_control_off, t_console, t_reinforced_glass_h, t_reinforced_glass_v, t_floor,
                                               t_floor, t_floor, t_floor, t_gates_control_concrete, t_sewage, t_door_metal_locked, t_concrete_v,
                                               t_concrete_h, t_rock, t_rock_floor, t_metal_floor, t_floor,  t_floor, t_floor),
                                       mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", f_null,         f_null,
                                               f_null,     f_null,                 f_null,    f_null,               f_null,
                                               f_counter, f_chair, f_desk,  f_rack,  f_null,                   f_null,   f_null,
                                               f_null,       f_null,       f_null, f_null,       f_null,        f_locker, f_sink,  f_toilet));
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->ter(i, j) == t_rock_floor) {
                        if (one_in(250)) {
                            item body;
                            body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                            add_item(i, j, body);
                            place_items("science",  70, i, j, i, j, true, 0);
                        } else if (one_in(80)) {
                            add_spawn("mon_zombie", 1, i, j);
                        }
                    }
                    if (this->ter(i, j) != t_metal_floor) {
                        radiation(x, y) += rng(10, 70);
                    }
                    if (this->ter(i, j) == t_sewage) {
                        if (one_in(2)) {
                            ter_set(i, j, t_dirtfloor);
                        }
                        if (one_in(4)) {
                            ter_set(i, j, t_dirtmound);
                        }
                        if (one_in(2)) {
                            ter_set(i, j, t_wreckage);
                        }
                        place_items("trash", 50,  i,  j, i,  j, false, 0);
                        place_items("sewer", 50,  i,  j, i,  j, false, 0);
                        if (one_in(5)) {
                            if (one_in(10)) {
                                add_spawn("mon_zombie_child", 1, i, j);
                            } else if (one_in(15)) {
                                add_spawn("mon_zombie_fast", 1, i, j);
                            } else {
                                add_spawn("mon_zombie", 1, i, j);
                            }
                        }
                    }
                }
            }
            if (t_north == "haz_sar_entrance_b1") {
                rotate(1);
            }
            if (t_east == "haz_sar_entrance_b1") {
                rotate(2);
            }
            if (t_south == "haz_sar_entrance_b1") {
                rotate(3);
            }
        }

        else {
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
...#####################\n\
$$$--###################\n\
...M..#|----------|#####\n\
.......|__________|#####\n\
.......$__________|#####\n\
.......$__________|#####\n\
.......$__________|#####\n\
.......$__________|#####\n\
.......$__________|#####\n\
......M|__________|#####\n\
......#|----------|#####\n\
.....###################\n\
....####|---|----|######\n\
###.##|-|,,,|,S,T|######\n\
#|-=-||&|,,,+,,,,|######\n\
#|,,l|EE+,,,|----|-|####\n\
#|,,l|EE+,,,|ddd,,l|####\n\
-|-$-|--|,,,V,h,,,l|####\n\
,,,,,|,,=,,,V,,,,,,|####\n\
,,,,,|rr|,,,V,,,,c,|####\n\
,,,,,|--|,,,|,,,hc,|####\n\
,,,,,+,,,,,,+,,c6c,|####\n\
,,,,M|,,,,,,|r,,,,,|####\n\
$$$$-|-|=HH-|-HHHH-|####\n",
                                       mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", t_door_metal_c, t_door_metal_o,
                                               t_elevator, t_elevator_control_off, t_console, t_reinforced_glass_h, t_reinforced_glass_v, t_floor,
                                               t_floor, t_floor, t_floor, t_gates_control_concrete, t_sewage, t_door_metal_locked, t_concrete_v,
                                               t_concrete_h, t_rock, t_rock_floor, t_metal_floor, t_floor,  t_floor, t_floor),
                                       mapf::basic_bind("= + E & 6 H V c h d r M _ $ | - # . , l S T", f_null,         f_null,
                                               f_null,     f_null,                 f_null,    f_null,               f_null,
                                               f_counter, f_chair, f_desk,  f_rack,  f_null,                   f_null,   f_null,
                                               f_null,       f_null,       f_null, f_null,       f_null,        f_locker, f_sink,  f_toilet));
            spawn_item(3, 16, "sarcophagus_access_code");
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_locker) {
                        place_items("cleaning", 60,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_desk) {
                        place_items("cubical_office", 60,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_rack) {
                        place_items("sewage_plant", 60,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_rock_floor) {
                        if (one_in(250)) {
                            item body;
                            body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
                            add_item(i, j, body);
                            place_items("science",  70, i, j, i, j, true, 0);
                        } else if (one_in(80)) {
                            add_spawn("mon_zombie", 1, i, j);
                        }
                    }
                    if (this->ter(i, j) != t_metal_floor) {
                        radiation(x, y) += rng(10, 70);
                    }
                    if (this->ter(i, j) == t_sewage) {
                        if (one_in(2)) {
                            ter_set(i, j, t_dirtfloor);
                        }
                        if (one_in(4)) {
                            ter_set(i, j, t_dirtmound);
                        }
                        if (one_in(2)) {
                            ter_set(i, j, t_wreckage);
                        }
                        place_items("trash", 50,  i,  j, i,  j, false, 0);
                        place_items("sewer", 50,  i,  j, i,  j, false, 0);
                        if (one_in(5)) {
                            if (one_in(10)) {
                                add_spawn("mon_zombie_child", 1, i, j);
                            } else if (one_in(15)) {
                                add_spawn("mon_zombie_fast", 1, i, j);
                            } else {
                                add_spawn("mon_zombie", 1, i, j);
                            }
                        }
                    }
                }
            }
            tmpcomp = add_computer(16, 21, _("SRCF Security Terminal"), 0);
            tmpcomp->add_option(_("Security Reminder [1055]"), COMPACT_SR1_MESS, 0);
            tmpcomp->add_option(_("Security Reminder [1056]"), COMPACT_SR2_MESS, 0);
            tmpcomp->add_option(_("Security Reminder [1057]"), COMPACT_SR3_MESS, 0);
            //tmpcomp->add_option(_("Security Reminder [1058]"), COMPACT_SR4_MESS, 0); limited to 9 computer options
            tmpcomp->add_option(_("EPA: Report All Potential Containment Breaches [3873643]"),
                                COMPACT_SRCF_1_MESS, 2);
            tmpcomp->add_option(_("SRCF: Internal Memo, EPA [2918024]"), COMPACT_SRCF_2_MESS, 2);
            tmpcomp->add_option(_("CDC: Internal Memo, Standby [2918115]"), COMPACT_SRCF_3_MESS, 2);
            tmpcomp->add_option(_("USARMY: SEAL SRCF [987167]"), COMPACT_SRCF_SEAL_ORDER, 4);
            tmpcomp->add_option(_("COMMAND: REACTIVATE ELEVATOR"), COMPACT_SRCF_ELEVATOR, 0);
            tmpcomp->add_failure(COMPFAIL_ALARM);
            if (t_west == "haz_sar_b1" && t_north == "haz_sar_b1") {
                rotate(1);
            }
            if (t_east == "haz_sar_b1" && t_north == "haz_sar_b1") {
                rotate(2);
            }
            if (t_east == "haz_sar_b1" && t_south == "haz_sar_b1") {
                rotate(3);
            }
        }


    } else if (terrain_type == "cave") {

        if (t_above == "cave") {
            // We're underground!
            for (int i = 0; i < SEEX * 2; i++) {
                for (int j = 0; j < SEEY * 2; j++) {
                    bool floorHere = (rng(0, 6) < i || SEEX * 2 - rng(1, 7) > i ||
                                      rng(0, 6) < j || SEEY * 2 - rng(1, 7) > j );
                    if (floorHere) {
                        ter_set(i, j, t_rock_floor);
                    } else {
                        ter_set(i, j, t_rock);
                    }
                }
            }
            square(this, t_slope_up, SEEX - 1, SEEY - 1, SEEX, SEEY);
            item body;
            switch(rng(1, 10)) {
            case 1:
                // natural refuse
                place_items("monparts", 80, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                break;
            case 2:
                // trash
                place_items("trash", 70, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                break;
            case 3:
                // bat corpses
                for (int i = rng(1, 12); i < 0; i--) {
                    body.make_corpse(itypes["corpse"], GetMType("mon_bat"), g->turn);
                    add_item(rng(1, SEEX * 2 - 1), rng(1, SEEY * 2 - 1), body);
                }
                break;
            case 4:
                // ant food, chance of 80
                place_items("ant_food", 85, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
                break;
            case 5: {
                // hermitage
                int origx = rng(SEEX - 1, SEEX),
                    origy = rng(SEEY - 1, SEEY),
                    hermx = rng(SEEX - 6, SEEX + 5),
                    hermy = rng(SEEX - 6, SEEY + 5);
                std::vector<point> bloodline = line_to(origx, origy, hermx, hermy, 0);
                for (int ii = 0; ii < bloodline.size(); ii++) {
                    add_field(g, bloodline[ii].x, bloodline[ii].y, fd_blood, 2);
                }
                body.make_corpse(itypes["corpse"], GetMType("mon_null"), g->turn);
                add_item(hermx, hermy, body);
                // This seems verbose.  Maybe a function to spawn from a list of item groups?
                place_items("stash_food", 50, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                place_items("survival_tools", 50, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                place_items("survival_armor", 50, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                place_items("weapons", 40, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                place_items("magazines", 40, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                place_items("rare", 30, hermx - 1, hermy - 1, hermx + 1, hermy + 1, true, 0);
                break;
            }
            default:
                // nothing, half the time
                break;
            }
            place_spawns(g, "GROUP_CAVE", 2, 6, 6, 18, 18, 1.0);
        } else { // We're above ground!
            // First, draw a forest
            draw_map(oter_id("forest"), t_north, t_east, t_south, t_west, t_neast, t_seast, t_nwest, t_swest,
                     t_above, turn, g, density, zlevel);
            // Clear the center with some rocks
            square(this, t_rock, SEEX - 6, SEEY - 6, SEEX + 5, SEEY + 5);
            int pathx, pathy;
            if (one_in(2)) {
                pathx = rng(SEEX - 6, SEEX + 5);
                pathy = (one_in(2) ? SEEY - 8 : SEEY + 7);
            } else {
                pathx = (one_in(2) ? SEEX - 8 : SEEX + 7);
                pathy = rng(SEEY - 6, SEEY + 5);
            }
            std::vector<point> pathline = line_to(pathx, pathy, SEEX - 1, SEEY - 1, 0);
            for (int ii = 0; ii < pathline.size(); ii++) {
                square(this, t_dirt, pathline[ii].x, pathline[ii].y,
                       pathline[ii].x + 1, pathline[ii].y + 1);
            }
            while (!one_in(8)) {
                ter_set(rng(SEEX - 6, SEEX + 5), rng(SEEY - 6, SEEY + 5), t_dirt);
            }
            square(this, t_slope_down, SEEX - 1, SEEY - 1, SEEX, SEEY);
        }


    } else if (terrain_type == "cave_rat") {

        fill_background(this, t_rock);

        if (t_above == "cave_rat") { // Finale
            rough_circle(this, t_rock_floor, SEEX, SEEY, 8);
            square(this, t_rock_floor, SEEX - 1, SEEY, SEEX, SEEY * 2 - 2);
            line(this, t_slope_up, SEEX - 1, SEEY * 2 - 3, SEEX, SEEY * 2 - 2);
            for (int i = SEEX - 4; i <= SEEX + 4; i++) {
                for (int j = SEEY - 4; j <= SEEY + 4; j++) {
                    if ((i <= SEEX - 2 || i >= SEEX + 2) && (j <= SEEY - 2 || j >= SEEY + 2)) {
                        add_spawn("mon_sewer_rat", 1, i, j);
                    }
                }
            }
            add_spawn("mon_rat_king", 1, SEEX, SEEY);
            place_items("rare", 75, SEEX - 4, SEEY - 4, SEEX + 4, SEEY + 4, true, 0);
        } else { // Level 1
            int cavex = SEEX, cavey = SEEY * 2 - 3;
            int stairsx = SEEX - 1, stairsy = 1; // Default stairs location--may change
            int centerx = 0;
            do {
                cavex += rng(-1, 1);
                cavey -= rng(0, 1);
                for (int cx = cavex - 1; cx <= cavex + 1; cx++) {
                    for (int cy = cavey - 1; cy <= cavey + 1; cy++) {
                        ter_set(cx, cy, t_rock_floor);
                        if (one_in(10)) {
                            add_field(g, cx, cy, fd_blood, rng(1, 3));
                        }
                        if (one_in(20)) {
                            add_spawn("mon_sewer_rat", 1, cx, cy);
                        }
                    }
                }
                if (cavey == SEEY - 1) {
                    centerx = cavex;
                }
            } while (cavey > 2);
            // Now draw some extra passages!
            do {
                int tox = (one_in(2) ? 2 : SEEX * 2 - 3), toy = rng(2, SEEY * 2 - 3);
                std::vector<point> path = line_to(centerx, SEEY - 1, tox, toy, 0);
                for (int i = 0; i < path.size(); i++) {
                    for (int cx = path[i].x - 1; cx <= path[i].x + 1; cx++) {
                        for (int cy = path[i].y - 1; cy <= path[i].y + 1; cy++) {
                            ter_set(cx, cy, t_rock_floor);
                            if (one_in(10)) {
                                add_field(g, cx, cy, fd_blood, rng(1, 3));
                            }
                            if (one_in(20)) {
                                add_spawn("mon_sewer_rat", 1, cx, cy);
                            }
                        }
                    }
                }
                if (one_in(2)) {
                    stairsx = tox;
                    stairsy = toy;
                }
            } while (one_in(2));
            // Finally, draw the stairs up and down.
            ter_set(SEEX - 1, SEEX * 2 - 2, t_slope_up);
            ter_set(SEEX    , SEEX * 2 - 2, t_slope_up);
            ter_set(stairsx, stairsy, t_slope_down);
        }


    } else if (is_ot_type("sub_station", terrain_type)) {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j < 9 || j > 12 || i < 4 || i > 19) {
                    ter_set(i, j, t_pavement);
                } else if (j < 12 && j > 8 && (i == 4 || i == 19)) {
                    ter_set(i, j, t_wall_v);
                } else if (i > 3 && i < 20 && j == 12) {
                    ter_set(i, j, t_wall_h);
                } else {
                    ter_set(i, j, t_floor);
                }
            }
        }
        ter_set(16, 10, t_stairs_down);
        if (terrain_type == "sub_station_east") {
            rotate(1);
        }
        if (terrain_type == "sub_station_south") {
            rotate(2);
        }
        if (terrain_type == "sub_station_west") {
            rotate(3);
        }


    } else if (is_ot_type("s_garage", terrain_type)) {

        fill_background(this, &grass_or_dirt);
        int yard_wdth = 5;
        square(this, t_floor, 0, yard_wdth, SEEX * 2 - 4, SEEY * 2 - 4);
        line(this, t_wall_v, 0, yard_wdth, 0, SEEY * 2 - 4);
        line(this, t_wall_v, SEEX * 2 - 3, yard_wdth, SEEX * 2 - 3, SEEY * 2 - 4);
        line(this, t_wall_h, 0, SEEY * 2 - 4, SEEX * 2 - 3, SEEY * 2 - 4);
        line(this, t_window, 0, SEEY * 2 - 4, SEEX * 2 - 14, SEEY * 2 - 4);
        line(this, t_wall_h, 0, SEEY * 2 - 4, SEEX * 2 - 20, SEEY * 2 - 4);
        line(this, t_wall_h, 0, yard_wdth, 2, yard_wdth);
        line(this, t_wall_h, 8, yard_wdth, 13, yard_wdth);
        line(this, t_wall_h, 20, yard_wdth, 21, yard_wdth);
        line_furn(this, f_counter, 1, yard_wdth + 1, 1, yard_wdth + 7);
        line(this, t_wall_h, 1, SEEY * 2 - 9, 3, SEEY * 2 - 9);
        line(this, t_wall_v, 3, SEEY * 2 - 8, 3, SEEY * 2 - 5);
        ter_set(3, SEEY * 2 - 7, t_door_frame);
        ter_set(21, SEEY * 2 - 7, t_door_c);
        line_furn(this, f_counter, 4, SEEY * 2 - 5, 15, SEEY * 2 - 5);
        //office
        line(this, t_wall_glass_h, 16, SEEY * 2 - 9 , 20, SEEY * 2 - 9);
        line(this, t_wall_glass_v, 16, SEEY * 2 - 8, 16, SEEY * 2 - 5);
        ter_set(16, SEEY * 2 - 7, t_door_glass_c);
        line_furn(this, f_bench, SEEX * 2 - 6, SEEY * 2 - 8, SEEX * 2 - 4, SEEY * 2 - 8);
        ter_set(SEEX * 2 - 6, SEEY * 2 - 6, t_console_broken);
        furn_set(SEEX * 2 - 5, SEEY * 2 - 6, f_bench);
        line_furn(this, f_locker, SEEX * 2 - 6, SEEY * 2 - 5, SEEX * 2 - 4, SEEY * 2 - 5);
        //gates
        line(this, t_door_metal_locked, 3, yard_wdth, 8, yard_wdth);
        ter_set(2, yard_wdth + 1, t_gates_mech_control);
        ter_set(2, yard_wdth - 1, t_gates_mech_control);
        line(this, t_door_metal_locked, 14, yard_wdth, 19, yard_wdth );
        ter_set(13, yard_wdth + 1, t_gates_mech_control);
        ter_set(13, yard_wdth - 1, t_gates_mech_control);

        //place items
        place_items("mechanics", 90, 1, yard_wdth + 1, 1, yard_wdth + 7, true, 0);
        place_items("mechanics", 90, 4, SEEY * 2 - 5, 15, SEEY * 2 - 5, true, 0);

        // rotate garage

        int vy = 0, vx = 0, theta = 0;

        if (terrain_type == "s_garage_north") {
            vx = 5, vy = yard_wdth + 6;
            theta = 90;
        } else if (terrain_type == "s_garage_east") {
            rotate(1);
            vx = yard_wdth + 8, vy = 4;
            theta = 0;
        } else if (terrain_type == "s_garage_south") {
            rotate(2);
            vx = SEEX * 2 - 6, vy = SEEY * 2 - (yard_wdth + 3);
            theta = 270;
        } else if (terrain_type == "s_garage_west") {
            rotate(3);
            vx = SEEX * 2 - yard_wdth - 9, vy = SEEY * 2 - 5;
            theta = 180;
        }

        // place vehicle, if any
        if (one_in(3)) {
            std::string vt;
            int vehicle_type = rng(1, 8);
            if(vehicle_type <= 3) {
                vt = one_in(2) ? "car" : "car_chassis";
            } else if(vehicle_type <= 5) {
                vt = one_in(2) ? "quad_bike" : "quad_bike_chassis";
            } else if(vehicle_type <= 7) {
                vt = one_in(2) ? "motorcycle" : "motorcycle_chassis";
            } else {
                vt = "welding_cart";
            }
            add_vehicle (g, vt, vx, vy, theta, -1, -1);
        }


    } else if (terrain_type == "cabin") {

        fill_background(this, t_grass);

        //Cabin design 1 Quad
        if (one_in(2)) {
            square(this, t_wall_log, 2, 3, 21, 20);
            square(this, t_floor, 2, 17, 21, 20);//Front porch
            line(this, t_fence_v, 2, 17, 2, 20);
            line(this, t_fence_v, 21, 17, 21, 20);
            line(this, t_fence_h, 2, 20, 21, 20);
            ter_set(2, 17, t_column);
            ter_set(2, 20, t_column);
            ter_set(21, 17, t_column);
            ter_set(21, 20, t_column);
            ter_set(10, 20, t_column);
            ter_set(13, 20, t_column);
            line(this, t_fencegate_c, 11, 20, 12, 20);
            line_furn(this, f_bench, 4, 17, 7, 17);
            square(this, t_rubble, 19, 18, 20, 19);
            ter_set(20, 17, t_rubble);
            ter_set(18, 19, t_rubble); //Porch done
            line(this, t_door_c, 11, 16, 12, 16);//Interior
            square(this, t_floor, 3, 4, 9, 9);
            square(this, t_floor, 3, 11, 9, 15);
            square(this, t_floor, 11, 4, 12, 15);
            square(this, t_floor, 14, 4, 20, 9);
            square(this, t_floor, 14, 11, 20, 15);
            line(this, t_wall_log, 7, 4, 7, 8);
            square(this, t_wall_log, 8, 8, 9, 9);
            line_furn(this, f_rack, 3, 4, 3, 9); //Pantry Racks
            line(this, t_curtains, 2, 6, 2, 7); //Windows start
            line(this, t_curtains, 2, 12, 2, 13);
            line(this, t_window_domestic, 5, 16, 6, 16);
            line(this, t_window_domestic, 17, 16, 18, 16);
            line(this, t_curtains, 21, 12, 21, 13);
            line(this, t_window_empty, 21, 6, 21, 7);
            ter_set(8, 3, t_curtains);//Windows End
            line(this, t_door_c, 11, 3, 12, 3);//Rear Doors
            square(this, t_rubble, 20, 3, 21, 4);
            ter_set(19, 3, t_rubble);
            ter_set(21, 5, t_rubble);
            furn_set(6, 4, f_desk);
            furn_set(6, 5, f_chair);
            furn_set(7, 9, f_locker);
            ter_set(6, 10, t_door_c);
            ter_set(10, 6, t_door_c);
            square_furn(this, f_table, 3, 11, 4, 12);
            line_furn(this, f_bench, 5, 11, 5, 12);
            line_furn(this, f_bench, 3, 13, 4, 13);
            line_furn(this, f_cupboard, 3, 15, 7, 15);
            furn_set(4, 15, f_fridge);
            furn_set(5, 15, f_sink);
            furn_set(6, 15, f_oven);
            ter_set(10, 13, t_door_c);
            ter_set(13, 13, t_door_c);
            furn_set(14, 11, f_armchair);
            line_furn(this, f_sofa, 16, 11, 18, 11);
            square(this, t_rock_floor, 18, 13, 20, 15);
            furn_set(19, 14, f_woodstove);
            ter_set(19, 10, t_door_c);
            line_furn(this, f_bookcase, 14, 9, 17, 9);
            square_furn(this, f_bed, 17, 4, 18, 5);
            furn_set(16, 4, f_dresser);
            furn_set(19, 4, f_dresser);
            ter_set(13, 6, t_door_c);
            place_toilet(9, 4);
            line_furn(this, f_bathtub, 8, 7, 9, 7);
            furn_set(8, 5, f_sink);
            place_items("fridge", 65, 4, 15, 4, 15, false, 0);
            place_items("homeguns", 30, 7, 9, 7, 9, false, 0);
            place_items("home_hw", 60, 7, 9, 7, 9, false, 0);
            place_items("kitchen", 60, 3, 15, 3, 15, false, 0);
            place_items("kitchen", 60, 7, 15, 7, 15, false, 0);
            place_items("dining", 60, 3, 11, 4, 12, false, 0);
            place_items("trash", 60, 0, 0, 23, 23, false, 0);
            place_items("survival_tools", 30, 3, 4, 3, 9, false, 0);
            place_items("cannedfood", 50, 3, 4, 3, 9, false, 0);
            place_items("camping", 50, 4, 4, 6, 9, false, 0);
            place_items("magazines", 60, 14, 9, 17, 9, false, 0);
            place_items("manuals", 30, 14, 9, 17, 9, false, 0);
            place_items("dresser", 50, 16, 4, 16, 4, false, 0);
            place_items("dresser", 50, 19, 4, 19, 4, false, 0);
            place_items("softdrugs", 60, 8, 4, 9, 7, false, 0);
            place_items("livingroom", 50, 14, 12, 17, 15, false, 0);
            add_spawn("mon_zombie", rng(1, 5), 11, 12);

        } else {
            square(this, t_wall_log, 4, 2, 10, 6);
            square(this, t_floor, 5, 3, 9, 5);
            square(this, t_wall_log, 3, 9, 20, 20);
            square(this, t_floor, 4, 10, 19, 19);
            line(this, t_fence_h, 0, 0, 23, 0);
            line(this, t_fence_v, 0, 0, 0, 22);
            line(this, t_fence_v, 23, 0, 23, 22);
            line(this, t_fence_h, 0, 23, 23, 23);
            line(this, t_fencegate_c, 11, 23, 12, 23);
            line_furn(this, f_locker, 5, 3, 9, 3);
            line_furn(this, f_counter, 6, 3, 8, 3);
            ter_set(4, 4, t_window_boarded);
            ter_set(10, 4, t_window_boarded);
            ter_set(7, 6, t_door_c);
            ter_set(9, 9, t_door_c);
            line(this, t_window_domestic, 13, 9, 14, 9);
            square(this, t_rock, 5, 10, 7, 11);
            line(this, t_rock_floor, 5, 12, 7, 12);
            set(6, 11, t_rock_floor, f_woodstove);
            line_furn(this, f_dresser, 16, 10, 19, 10);
            square_furn(this, f_bed, 17, 10, 18, 11);
            line(this, t_window_domestic, 3, 14, 3, 15);
            line_furn(this, f_sofa, 5, 16, 7, 16);
            square_furn(this, f_chair, 10, 14, 13, 15);
            square_furn(this, f_table, 11, 14, 12, 15);
            line(this, t_window_domestic, 20, 14, 20, 15);
            line(this, t_window_domestic, 7, 20, 8, 20);
            line(this, t_window_domestic, 16, 20, 17, 20);
            ter_set(12, 20, t_door_c);
            place_items("livingroom", 60, 4, 13, 8, 18, false, 0);
            place_items("dining", 60, 11, 14, 12, 15, false, 0);
            place_items("camping", 70, 19, 16, 19, 19, false, 0);
            place_items("dresser", 70, 16, 10, 16, 10, false, 0);
            place_items("dresser", 70, 19, 10, 19, 10, false, 0);
            place_items("tools", 70, 5, 3, 9, 3, false, 0);
            add_spawn("mon_zombie", rng(1, 5), 7, 4);
        }


    } else if (terrain_type == "cabin_strange") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
                        \n\
^  FfffffffffffGfffffF  \n\
   F                 F  \n\
   F              ^  F  \n\
  ^F  |-w---|        F  \n\
   F  |cSecu|sssss   F  \n\
  |-w-|O....=sssss|---| \n\
  |H.T|c...u|-w-w-|>..w \n\
  |H..+....u|d....|-|-| \n\
  |..S|.....+.....+.|r| \n\
  |-+-|.....|...bb|r|.| \n\
  |.........|---|-|-|+| \n\
  w...hh....aaaa|.d...| \n\
  |..htth.......|.....w \n\
  w..htth.......D..bb.w \n\
  w...hh.......o|..bb.| \n\
  |o...........A|-----| \n\
  w.............|d.bb.| \n\
  |.............+..bb.w \n\
  |-+|-w-==-w-|-|.....| \n\
  |r.|ssssssss|r+.....| \n\
  |--|ssssssss|-|--w--| \n\
     ssCssssCss         \n\
  ^                 ^   \n",
                                   mapf::basic_bind("% ^ f F G H u a A b C . - | t + = D w T S e o h c d r s O >", t_shrub, t_tree,
                                           t_fence_h, t_fence_v, t_fencegate_c, t_floor,   t_floor,    t_floor, t_floor,    t_floor, t_column,
                                           t_floor, t_wall_h, t_wall_v,  t_floor, t_door_c, t_door_boarded, t_door_locked_interior,
                                           t_window_boarded, t_floor,  t_floor, t_floor,  t_floor,    t_floor, t_floor,   t_floor,   t_floor,
                                           t_sidewalk, t_floor, t_stairs_down),
                                   mapf::basic_bind("% ^ f F G H u a A b C . - | t + = D w T S e o h c d r s O >", f_null,  f_null,
                                           f_null,    f_null,    f_null,        f_bathtub, f_cupboard, f_sofa,  f_armchair, f_bed,   f_null,
                                           f_null,  f_null,   f_null,    f_table, f_null,   f_null,         f_null,                 f_null,
                                           f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_dresser, f_rack,  f_null,     f_oven,
                                           f_null));
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_fridge) {
                    place_items("fridgesnacks",  30, i, j, i, j, true, 0);
                }
                if (this->furn(i, j) == f_cupboard) {
                    place_items("cannedfood",  30, i, j, i, j, true, 0);
                }
                if (this->furn(i, j) == f_rack || this->furn(i, j) == f_dresser) {
                    place_items("dresser",  40, i, j, i, j, true, 0);
                }
                if (this->furn(i, j) == f_bookcase) {
                    place_items("novels",  40, i, j, i, j, true, 0);
                }
                if (this->ter(i, j) == t_floor) {
                    place_items("subway",  10, i, j, i, j, true, 0);
                }
            }
        }
        add_spawn("mon_darkman", 1, 8, 10);


    } else if (terrain_type == "cabin_strange_b") {

        // Init to grass & dirt;
        fill_background(this, &grass_or_dirt);
        mapf::formatted_set_simple(this, 0, 0,
                                   "\
########################\n\
################...h...#\n\
########c.cc####.httth.#\n\
###T..##c....+...ht.th.#\n\
###...G....c####.......#\n\
###bb.##....############\n\
##########D###|---|---|#\n\
##########.###|cdc|<..|#\n\
##.hhh.##...##|.h.|-D-|#\n\
#.......#.C.##|-+-|..h##\n\
#.hh.hh.D...##c......c##\n\
#.......#.C.##ccC..Ccc##\n\
#.hh.hh.#...##cc.....r##\n\
#.......#.C.##ccC..C.r##\n\
#.hh.hh.#...##tt..ch.r##\n\
#.......#.C.##ttCccC..##\n\
#.......#............A##\n\
#...t...#.C..C.cC..C..##\n\
##.....##..h..ccccbbo.##\n\
###+#+##################\n\
##.....#################\n\
##.....#################\n\
##.....#################\n\
########################\n",
                                   mapf::basic_bind("G A b C . - | t + = D o h c d r < # T", t_door_bar_locked, t_dirtfloor,
                                           t_dirtfloor, t_column, t_dirtfloor, t_wall_h, t_wall_v,  t_dirtfloor, t_door_c, t_door_boarded,
                                           t_door_locked_interior, t_dirtfloor, t_dirtfloor, t_floor,   t_dirtfloor, t_dirtfloor, t_stairs_up,
                                           t_rock, t_dirtfloor),
                                   mapf::basic_bind("G A b C . - | t + = D o h c d r < # T", f_null,            f_armchair,     f_bed,
                                           f_null,   f_null,      f_null,   f_null,    f_table,     f_null,   f_null,         f_null,
                                           f_bookcase,  f_chair,     f_crate_o, f_desk,      f_rack,      f_null,      f_null, f_toilet));
        spawn_item(2, 17, "brazier");
        spawn_item(6, 17, "brazier");
        spawn_item(4, 17, "etched_skull");
        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->furn(i, j) == f_crate_c) {
                    place_items("dresser",  20, i, j, i, j, true, 0);
                }
                if (this->furn(i, j) == f_cupboard || this->furn(i, j) == f_rack) {
                    place_items("cannedfood",  30, i, j, i, j, true, 0);
                }
                if (this->furn(i, j) == f_bookcase) {
                    place_items("novels",  40, i, j, i, j, true, 0);
                }
                if (this->ter(i, j) == t_dirtfloor) {
                    place_items("subway",  10, i, j, i, j, true, 0);
                }
            }
        }
        add_spawn("mon_dementia", rng(3, 6), 4, 12);
        add_spawn("mon_dementia", rng(1, 4), 19, 2);
        add_spawn("mon_blood_sacrifice", 1, 4, 21);


    } else {
        terrain_type_found = false;
    }

    // MSVC can't handle a single "if/else if" with this many clauses. Hack to 
    // break the clause in two so MSVC compiles work, until this file is refactored.
    if (!terrain_type_found) {
    if (terrain_type == "farm") {

        if (!one_in(10)) {
            fill_background(this, &grass_or_dirt);
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
          %%%       %%% \n\
###++### |-w---+-----w-|\n\
#DD____# |uSeuu........|\n\
#D_____# |o............|\n\
#_1__1_# |u..........H.|\n\
#______# |u...|---+----|\n\
#______# |-+-||kh......|\n\
#_1__1_# |...|.........w\n\
#______# |b..+......BB.|\n\
#____ll# |.ST|.....dBBd|\n\
###++### |-w-|----w----|\n\
                        \n\
                        \n\
FFFFFFFFFFFFFFFFFFFFFFFF\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n",
                                       mapf::basic_bind(", F . _ H u e S T o b l # % 1 D + - | w k h B d", t_dirt, t_fence_barbed, t_floor,
                                               t_dirtfloor, t_floor,    t_floor,    t_floor,  t_floor, t_floor,  t_floor, t_floor,   t_floor,
                                               t_wall_wood, t_shrub, t_column, t_dirtmound, t_door_c, t_wall_h, t_wall_v, t_window_domestic,
                                               t_floor, t_floor, t_floor, t_floor),
                                       mapf::basic_bind(", F . _ H u e S T o b l # % 1 D + - | w k h B d", f_null, f_null,         f_null,
                                               f_null,      f_armchair, f_cupboard, f_fridge, f_sink,  f_toilet, f_oven,  f_bathtub, f_locker,
                                               f_null,      f_null,  f_null,   f_null,      f_null,   f_null,   f_null,   f_null,
                                               f_desk,  f_chair, f_bed,   f_dresser));
            place_items("fridge", 65, 12, 11, 12, 11, false, 0);
            place_items("kitchen", 70, 10, 11, 14, 3, false, 0);
            place_items("livingroom", 65, 15, 11, 22, 13, false, 0);
            place_items("dresser", 80, 19, 18, 19, 18, false, 0);
            place_items("dresser", 80, 22, 18, 22, 18, false, 0);
            place_items("bedroom", 65, 15, 15, 22, 18, false, 0);
            place_items("softdrugs", 70, 11, 16, 12, 17, false, 0);
            place_items("bigtools", 50, 1, 11, 6, 18, true, 0);
            place_items("homeguns", 20, 1, 11, 6, 18, true, 0);
            if (one_in(2)) {
                add_spawn("mon_zombie", rng(1, 6), 4, 14);
            } else {
                place_spawns(g, "GROUP_DOMESTIC", 2, 10, 15, 12, 17, 1);
            }
        } else {
            fill_background(this, &grass_or_dirt);
            mapf::formatted_set_simple(this, 0, 0,
                                       "\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n\
          %%%       %%% \n\
###++### |-w---+-----w-|\n\
#DD____# |uSeuu..mm....|\n\
#D_____# |o...........m|\n\
#_1__1_# |u...........m|\n\
#______# |u...|---+----|\n\
#______# |-+-||mm....mm|\n\
#_1__1_# |...|.........w\n\
#______# |b..+......BB.|\n\
#____ll# |.ST|mm...dBBd|\n\
###++### |-w-|----w----|\n\
                        \n\
                        \n\
FFFFFFFFFFFFFFFFFFFFFFFF\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n",
                                       mapf::basic_bind("m , F . _ H u e S T o b l # % 1 D + - | w k h B d", t_floor,         t_dirt,
                                               t_fence_barbed, t_floor, t_dirtfloor, t_floor,    t_floor,    t_floor,  t_floor, t_floor,  t_floor,
                                               t_floor,   t_floor,  t_wall_wood, t_shrub, t_column, t_dirtmound, t_door_c, t_wall_h, t_wall_v,
                                               t_window_domestic, t_floor, t_floor, t_floor, t_floor),
                                       mapf::basic_bind("m , F . _ H u e S T o b l # % 1 D + - | w k h B d", f_makeshift_bed, f_null,
                                               f_null,         f_null,  f_null,      f_armchair, f_cupboard, f_fridge, f_sink,  f_toilet, f_oven,
                                               f_bathtub, f_locker, f_null,      f_null,  f_null,   f_null,      f_null,   f_null,   f_null,
                                               f_null,            f_desk,  f_chair, f_bed,   f_dresser));
            place_items("cannedfood", 65, 12, 11, 12, 11, false, 0);
            place_items("bigtools", 50, 1, 11, 6, 18, true, 0);
            place_items("homeguns", 20, 1, 11, 6, 18, true, 0);
            for (int i = 0; i <= 23; i++) {
                for (int j = 0; j <= 23; j++) {
                    if (this->furn(i, j) == f_dresser) {
                        place_items("dresser", 50,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_floor) {
                        place_items("trash", 20,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_cupboard) {
                        place_items("kitchen", 70,  i,  j, i,  j, false, 0);
                        place_items("softdrugs", 40,  i,  j, i,  j, false, 0);
                        place_items("cannedfood", 40,  i,  j, i,  j, false, 0);
                    }
                    if (this->furn(i, j) == f_makeshift_bed || this->furn(i, j) == f_bed) {
                        place_items("livingroom", 20,  i,  j, i,  j, false, 0);
                        place_items("survival_armor", 20,  i,  j, i,  j, false, 0);
                        place_items("camping", 20,  i,  j, i,  j, false, 0);
                        place_items("survival_tools", 20,  i,  j, i,  j, false, 0);
                    }
                    if (this->ter(i, j) == t_grass) {
                        if (one_in(20)) {
                            add_trap(i, j, tr_beartrap);
                        }
                        if (one_in(20)) {
                            add_trap(i, j, tr_tripwire);
                        }
                        if (one_in(15)) {
                            add_trap(i, j, tr_pit);
                        }
                    }
                }
            }
        }


    } else if (terrain_type == "farm_field") {

        //build barn
        if (t_east == "farm") {
            fill_background(this, &grass_or_dirt);
            square(this, t_wall_wood, 3, 3, 20, 20);
            square(this, t_dirtfloor, 4, 4, 19, 19);
            line(this, t_door_metal_locked, 8, 20, 15, 20);
            ter_set(16, 19, t_barndoor);
            ter_set(16, 21, t_barndoor);
            line(this, t_door_metal_locked, 8, 3, 15, 3);
            ter_set(16, 2, t_barndoor);
            ter_set(16, 4, t_barndoor);
            square_furn(this, f_hay, 4, 4, 6, 6);
            line(this, t_fence_h, 4, 8, 6, 8);
            line(this, t_fence_v, 6, 9, 6, 14);
            line(this, t_fence_h, 4, 15, 6, 15);
            line(this, t_fencegate_c, 6, 11, 6, 12);

            line(this, t_fence_h, 17, 8, 19, 8);
            line(this, t_fence_v, 17, 9, 17, 14);
            line(this, t_fence_h, 17, 15, 19, 15);
            line(this, t_fencegate_c, 17, 11, 17, 12);
            line_furn(this, f_locker, 4, 19, 7, 19);
            ter_set(7, 7, t_column);
            ter_set(16, 7, t_column);
            ter_set(7, 16, t_column);
            ter_set(16, 16, t_column);
            ter_set(5, 3, t_window_boarded);
            ter_set(18, 3, t_window_boarded);
            line(this, t_window_boarded, 3, 5, 3, 6);
            line(this, t_window_boarded, 3, 11, 3, 12);
            line(this, t_window_boarded, 3, 17, 3, 18);
            line(this, t_window_boarded, 20, 5, 20, 6);
            line(this, t_window_boarded, 20, 11, 20, 12);
            line(this, t_window_boarded, 20, 17, 20, 18);
            ter_set(5, 20, t_window_boarded);
            ter_set(18, 20, t_window_boarded);

            if(t_south == "farm_field") {
                square(this, t_fence_barbed, 1, 20, 1, 23);
                ter_set(2, 20, t_fence_barbed);
                ter_set(1, 20, t_fence_post);
                square(this, t_fence_barbed, 22, 20, 22, 22);
                ter_set(21, 20, t_fence_barbed);
                ter_set(23, 22, t_fence_barbed);
                ter_set(22, 22, t_fence_post);
                ter_set(22, 20, t_fence_post);
                square(this, t_dirt, 2, 21, 21, 23);
                square(this, t_dirt, 22, 23, 23, 23);
                ter_set(16, 21, t_barndoor);
            }
            place_items("bigtools", 60, 4, 4, 7, 19, true, 0);
            place_items("bigtools", 60, 16, 5, 19, 19, true, 0);
            place_items("mechanics", 40, 8, 4, 15, 19, true, 0);
            place_items("home_hw", 50, 4, 19, 7, 19, true, 0);
            place_items("tools", 50, 4, 19, 7, 19, true, 0);
            if (one_in(3)) {
                add_spawn("mon_zombie", rng(3, 6), 12, 12);
            } else {
                place_spawns(g, "GROUP_DOMESTIC", 2, 0, 0, 15, 15, 1);
            }

        } else {
            fill_background(this, t_grass); // basic lot
            square(this, t_fence_barbed, 1, 1, 22, 22);
            square(this, t_dirt, 2, 2, 21, 21);
            ter_set(1, 1, t_fence_post);
            ter_set(22, 1, t_fence_post);
            ter_set(1, 22, t_fence_post);
            ter_set(22, 22, t_fence_post);

            int xStart = 4;
            int xEnd = 19;
            //acidia, connecting fields
            if(t_east == "farm_field") {
                square(this, t_fence_barbed, 22, 1, 23, 22);
                square(this, t_dirt, 21, 2, 23, 21);
                xEnd = 22;
            }
            if(t_west == "farm_field") {
                square(this, t_fence_barbed, 0, 1, 1, 22);
                square(this, t_dirt, 0, 2, 2, 21);
                xStart = 1;
            }
            if(t_south == "farm_field") {
                square(this, t_fence_barbed, 1, 22, 22, 23);
                square(this, t_dirt, 2, 21, 21, 23);
                line(this, t_dirtmound, xStart, 21, xEnd, 21);
                if(t_east == "farm_field") {
                    square(this, t_dirt, 20, 20, 23, 23);
                }
                if(t_west == "farm_field") {
                    square(this, t_dirt, 0, 20, 3, 23);
                }
            }
            if(t_north == "farm_field" || t_north == "farm") {
                square(this, t_fence_barbed, 1, 0, 22, 1);
                square(this, t_dirt, 2, 0, 21, 2);
                line(this, t_dirtmound, xStart, 1, xEnd, 1);
                if(t_east == "farm_field") {
                    square(this, t_dirt, 20, 0, 23, 3);
                }
                if(t_west == "farm_field") {
                    square(this, t_dirt, 0, 0, 3, 3);
                }
            }
            if(t_west == "farm") {
                square(this, t_fence_barbed, 0, 22, 1, 22);
                square(this, t_dirt, 0, 23, 2, 23);
                ter_set(1, 22, t_fence_post);
            }
            //standard field
            line(this, t_dirtmound, xStart, 3, xEnd, 3); //Crop rows
            line(this, t_dirtmound, xStart, 5, xEnd, 5);
            line(this, t_dirtmound, xStart, 7, xEnd, 7);
            line(this, t_dirtmound, xStart, 9, xEnd, 9);
            line(this, t_dirtmound, xStart, 11, xEnd, 11);
            line(this, t_dirtmound, xStart, 13, xEnd, 13);
            line(this, t_dirtmound, xStart, 15, xEnd, 15);
            line(this, t_dirtmound, xStart, 17, xEnd, 17);
            line(this, t_dirtmound, xStart, 19, xEnd, 19);

            place_items("hydro", 70, xStart, 3, xEnd, 3, true, turn); //Spawn crops
            place_items("hydro", 70, xStart, 5, xEnd, 5, true, turn);
            place_items("hydro", 70, xStart, 7, xEnd, 7, true, turn);
            place_items("hydro", 70, xStart, 9, xEnd, 9, true, turn);
            place_items("hydro", 70, xStart, 11, xEnd, 11, true, turn);
            place_items("hydro", 70, xStart, 13, xEnd, 13, true, turn);
            place_items("hydro", 70, xStart, 15, xEnd, 15, true, turn);
            place_items("hydro", 70, xStart, 17, xEnd, 17, true, turn);
            place_items("hydro", 70, xStart, 19, xEnd, 19, true, turn);
        }


    } else if (is_ot_type("police", terrain_type)) {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((j ==  7 && i != 17 && i != 18) ||
                    (j == 12 && i !=  0 && i != 17 && i != 18 && i != SEEX * 2 - 1) ||
                    (j == 14 && ((i > 0 && i < 6) || i == 9 || i == 13 || i == 17)) ||
                    (j == 15 && i > 17  && i < SEEX * 2 - 1) ||
                    (j == 17 && i >  0  && i < 17) ||
                    (j == 20)) {
                    ter_set(i, j, t_wall_h);
                } else if (((i == 0 || i == SEEX * 2 - 1) && j > 7 && j < 20) ||
                           ((i == 5 || i == 10 || i == 16 || i == 19) && j > 7 && j < 12) ||
                           ((i == 5 || i ==  9 || i == 13) && j > 14 && j < 17) ||
                           (i == 17 && j > 14 && j < 20)) {
                    ter_set(i, j, t_wall_v);
                } else if (j == 14 && i > 5 && i < 17 && i % 2 == 0) {
                    ter_set(i, j, t_bars);
                } else if ((i > 1 && i < 4 && j > 8 && j < 11) ||
                           (j == 17 && i > 17 && i < 21)) {
                    set(i, j, t_floor, f_counter);
                } else if ((i == 20 && j > 7 && j < 12) || (j == 8 && i > 19 && i < 23) ||
                           (j == 15 && i > 0 && i < 5)) {
                    set(i, j, t_floor, f_locker);
                } else if (j < 7) {
                    ter_set(i, j, t_pavement);
                } else if (j > 20) {
                    ter_set(i, j, t_sidewalk);
                } else {
                    ter_set(i, j, t_floor);
                }
            }
        }
        ter_set(17, 7, t_door_locked);
        ter_set(18, 7, t_door_locked);
        ter_set(rng( 1,  4), 12, t_door_c);
        ter_set(rng( 6,  9), 12, t_door_c);
        ter_set(rng(11, 15), 12, t_door_c);
        ter_set(21, 12, t_door_metal_locked);
        tmpcomp = add_computer(22, 13, _("PolCom OS v1.47"), 3);
        tmpcomp->add_option(_("Open Supply Room"), COMPACT_OPEN, 3);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);
        tmpcomp->add_failure(COMPFAIL_MANHACKS);
        ter_set( 7, 14, t_door_c);
        ter_set(11, 14, t_door_c);
        ter_set(15, 14, t_door_c);
        ter_set(rng(20, 22), 15, t_door_c);
        ter_set(2, 17, t_door_metal_locked);
        tmpcomp = add_computer(22, 13, _("PolCom OS v1.47"), 3);
        tmpcomp->add_option(_("Open Evidence Locker"), COMPACT_OPEN, 3);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);
        tmpcomp->add_failure(COMPFAIL_MANHACKS);
        ter_set(17, 18, t_door_c);
        for (int i = 18; i < SEEX * 2 - 1; i++) {
            ter_set(i, 20, t_window);
        }
        if (one_in(3)) {
            for (int j = 16; j < 20; j++) {
                ter_set(SEEX * 2 - 1, j, t_window);
            }
        }
        rn = rng(18, 21);
        if (one_in(4)) {
            ter_set(rn    , 20, t_door_c);
            ter_set(rn + 1, 20, t_door_c);
        } else {
            ter_set(rn    , 20, t_door_locked);
            ter_set(rn + 1, 20, t_door_locked);
        }
        rn = rng(1, 5);
        ter_set(rn, 20, t_window);
        ter_set(rn + 1, 20, t_window);
        rn = rng(10, 14);
        ter_set(rn, 20, t_window);
        ter_set(rn + 1, 20, t_window);
        if (one_in(2)) {
            for (int i = 6; i < 10; i++) {
                furn_set(i, 8, f_counter);
            }
        }
        if (one_in(3)) {
            for (int j = 8; j < 12; j++) {
                furn_set(6, j, f_counter);
            }
        }
        if (one_in(3)) {
            for (int j = 8; j < 12; j++) {
                furn_set(9, j, f_counter);
            }
        }

        place_items("kitchen",      40,  6,  8,  9, 11,    false, 0);
        place_items("cop_weapons",  70, 20,  8, 22,  8,    false, 0);
        place_items("cop_weapons",  70, 20,  8, 20, 11,    false, 0);
        place_items("cop_evidence", 60,  1, 15,  4, 15,    false, 0);

        for (int i = 0; i <= 23; i++) {
            for (int j = 0; j <= 23; j++) {
                if (this->ter(i, j) == t_floor && one_in(80)) {
                    spawn_item(i, j, "badge_deputy");
                }
            }
        }

        if (terrain_type == "police_west") {
            rotate(1);
        }
        if (terrain_type == "police_north") {
            rotate(2);
        }
        if (terrain_type == "police_east") {
            rotate(3);
        }

        place_spawns(g, "GROUP_POLICE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);


    } else if (is_ot_type("bank", terrain_type)) {

        fill_background(this, &grass_or_dirt);

        square(this, t_floor, 1,  1, 22, 22);
        line(this, t_wall_h,  1,  1, 22,  1);
        line(this, t_wall_h,  2,  6, 19,  6);
        line(this, t_wall_h,  2, 13, 18, 13);
        line(this, t_wall_h,  1, 22, 22, 22);
        line(this, t_wall_h,  9,  9, 18,  9);
        line(this, t_wall_v,  1,  2,  1, 21);
        line(this, t_wall_v, 22,  2, 22, 21);
        line(this, t_wall_v, 19,  9, 19, 21);
        line(this, t_wall_v, 13, 14, 13, 16);
        line(this, t_wall_v, 13, 19, 13, 21);
        line(this, t_wall_v,  8,  7,  8, 12);
        line(this, t_wall_metal_h,  3, 14, 11, 14);
        line(this, t_wall_metal_h,  3, 21, 11, 21);
        line(this, t_wall_metal_v,  2, 14,  2, 21);
        line(this, t_wall_metal_v, 12, 14, 12, 16);
        line(this, t_wall_metal_v, 12, 19, 12, 21);
        line_furn(this, f_counter,  2,  4,  14,  4);
        ter_set(13, 17, t_door_metal_locked);
        ter_set(13, 18, t_door_metal_locked);
        tmpcomp = add_computer(14, 16, _("First United Bank"), 3);
        tmpcomp->add_option(_("Open Vault"), COMPACT_OPEN, 3);
        tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
        tmpcomp->add_failure(COMPFAIL_ALARM);
        // Front wall--glass or windows?
        if (!one_in(4)) {
            line(this, t_wall_glass_h_alarm, 2, 1, 21, 1);
            if (one_in(2)) {
                line(this, t_wall_glass_v_alarm, 1, 2, 1, 5);    // Side wall for teller room
            }
        } else {
            if (one_in(4)) {
                line(this, t_wall_glass_v_alarm, 1, 2, 1, 5);    // Side wall for teller room
            }
            rn = rng(3, 7);
            ter_set(rn    , 1, t_window_alarm);
            ter_set(rn + 1, 1, t_window_alarm);
            rn = rng(13, 18);
            ter_set(rn    , 1, t_window_alarm);
            ter_set(rn + 1, 1, t_window_alarm);
        }
        // Doors for offices
        ter_set(8, rng(7, 8), t_door_c);
        ter_set(rng(10, 17), 9, t_door_c);
        ter_set(19, rng(15, 20), t_door_c);
        // Side and back windows
        ter_set(1, rng(7, 12), t_window_alarm);
        ter_set(1, rng(7, 12), t_window_alarm);
        ter_set(rng(14, 18), 22, t_window_alarm);
        if (one_in(2)) {
            ter_set(rng(14, 18), 22, t_window_alarm);
        }
        if (one_in(10)) {
            line(this, t_wall_glass_v, 22, 2, 22, 21);    // Right side is glass wall!
        } else {
            rn = rng(7, 12);
            ter_set(22, rn    , t_window_alarm);
            ter_set(22, rn + 1, t_window_alarm);
            rn = rng(13, 19);
            ter_set(22, rn    , t_window_alarm);
            ter_set(22, rn + 1, t_window_alarm);
        }
        // Finally, place the front doors.
        if (one_in(4)) { // 1 in 4 are unlocked
            ter_set(10, 1, t_door_c);
            ter_set(11, 1, t_door_c);
        } else if (one_in(4)) { // 1 in 4 locked ones are un-alarmed
            ter_set(10, 1, t_door_locked);
            ter_set(11, 1, t_door_locked);
        } else {
            ter_set(10, 1, t_door_locked_alarm);
            ter_set(11, 1, t_door_locked_alarm);
        }

        place_items("office",       60,  2,  7,  7, 12,    false, 0);
        place_items("office",       60,  9, 10, 18, 12,    false, 0);
        place_items("office",       70, 14, 14, 18, 21,    false, 0);
        place_items("vault",        45,  3, 15, 11, 20,    false, 0);

        if (terrain_type == "bank_east") {
            rotate(1);
        }
        if (terrain_type == "bank_south") {
            rotate(2);
        }
        if (terrain_type == "bank_west") {
            rotate(3);
        }


    } else if (is_ot_type("bar", terrain_type)) {

        fill_background(this, t_pavement);
        square(this, t_floor, 2, 2, 21, 15);
        square(this, t_floor, 18, 17, 21, 18);

        mapf::formatted_set_simple(this, 1, 1,
                                   "\
|---------++---------|\n\
|                    |\n\
|  ##   ##   ##   ccc|\n\
|  ##   ##   ##   c &|\n\
|                 c B|\n\
|                 c B|\n\
|                 c B|\n\
|  ##   ##   ##   c B|\n\
|  ##   ##   ##   c  |\n\
|                 cc |\n\
|                    |\n\
|                    |\n\
|  xxxxx    xxxxx    |\n\
|  xxxxx    xxxxx    |\n\
|                    |\n\
|------------------D-|\n\
                D   &|\n\
                |cccc|\n\
                |----|\n",
                                   mapf::basic_bind("- | + D", t_wall_h, t_wall_v, t_door_c, t_door_locked),
                                   mapf::basic_bind("# c x & B", f_table, f_counter, f_pool_table, f_fridge, f_rack));

        // Pool table items
        place_items("pool_table", 50,  4, 13,  8, 14, false, 0);
        place_items("pool_table", 50, 13, 13, 17, 14, false, 0);

        // 1 in 4 chance to have glass walls in front
        if (one_in(4)) {
            mapf::formatted_set_terrain(this, 1, 1, "  === ===    === ===  ", mapf::basic_bind("=",
                                        t_wall_glass_h), mapf::end() );
            mapf::formatted_set_terrain(this, 1, 1, "\n\n=\n=\n=\n\n=\n=\n=\n\n=\n=\n=\n\n",
                                        mapf::basic_bind("=", t_wall_glass_v), mapf::end());
        } else {
            mapf::formatted_set_terrain(this, 1, 1, "  : : :        : : :  ", mapf::basic_bind(":", t_window),
                                        mapf::end() );
            mapf::formatted_set_terrain(this, 1, 1, "\n\n\n\n\n:\n\n\n\n\n:\n\n\n\n", mapf::basic_bind(":",
                                        t_window), mapf::end());
        }

        // Item placement
        place_items("snacks", 30, 19, 3, 19, 10, false, 0); //counter
        place_items("snacks", 50, 18, 18, 21, 18, false, 0);
        place_items("fridgesnacks", 60, 21, 4, 21, 4, false, 0); // fridge
        place_items("fridgesnacks", 60, 21, 17, 21, 17, false, 0);
        place_items("alcohol", 70, 21, 5, 21, 8, false, 0); // rack
        place_items("trash", 15, 2, 17, 16, 19, true, 0);

        if (terrain_type == "bar_east") {
            rotate(1);
        }
        if (terrain_type == "bar_south") {
            rotate(2);
        }
        if (terrain_type == "bar_west") {
            rotate(3);
        }


    } else if (is_ot_type("pawn", terrain_type)) {

        // Init to plain grass/dirt
        fill_background(this, &grass_or_dirt);

        tw = rng(0, 10);
        bw = SEEY * 2 - rng(1, 2) - rng(0, 1) * rng(0, 1);
        lw = rng(0, 4);
        rw = SEEX * 2 - rng(1, 5);
        if (tw >= 6) { // Big enough for its own parking lot
            square(this, t_pavement, 0, 0, SEEX * 2 - 1, tw - 1);
            for (int i = rng(0, 1); i < SEEX * 2; i += 4) {
                line(this, t_pavement_y, i, 1, i, tw - 1);
            }
        }
        // Floor and walls
        square(this, t_floor, lw, tw, rw, bw);
        line(this, t_wall_h, lw, tw, rw, tw);
        line(this, t_wall_h, lw, bw, rw, bw);
        line(this, t_wall_v, lw, tw + 1, lw, bw - 1);
        line(this, t_wall_v, rw, tw + 1, rw, bw - 1);
        // Doors and windows--almost certainly alarmed
        if (one_in(15)) {
            line(this, t_window, lw + 2, tw, lw + 5, tw);
            line(this, t_window, rw - 5, tw, rw - 2, tw);
            line(this, t_door_locked, SEEX, tw, SEEX + 1, tw);
        } else {
            line(this, t_window_alarm, lw + 2, tw, lw + 5, tw);
            line(this, t_window_alarm, rw - 5, tw, rw - 2, tw);
            line(this, t_door_locked_alarm, SEEX, tw, SEEX + 1, tw);
        }
        // Some display racks by the left and right walls
        line_furn(this, f_rack, lw + 1, tw + 1, lw + 1, bw - 1);
        place_items("pawn", 86, lw + 1, tw + 1, lw + 1, bw - 1, false, 0);
        line_furn(this, f_rack, rw - 1, tw + 1, rw - 1, bw - 1);
        place_items("pawn", 86, rw - 1, tw + 1, rw - 1, bw - 1, false, 0);
        // Some display counters
        line_furn(this, f_counter, lw + 4, tw + 2, lw + 4, bw - 3);
        place_items("pawn", 80, lw + 4, tw + 2, lw + 4, bw - 3, false, 0);
        line_furn(this, f_counter, rw - 4, tw + 2, rw - 4, bw - 3);
        place_items("pawn", 80, rw - 4, tw + 2, rw - 4, bw - 3, false, 0);
        // More display counters, if there's room for them
        if (rw - lw >= 18 && one_in(rw - lw - 17)) {
            for (int j = tw + rng(3, 5); j <= bw - 3; j += 3) {
                line_furn(this, f_counter, lw + 6, j, rw - 6, j);
                place_items("pawn", 75, lw + 6, j, rw - 6, j, false, 0);
            }
        }
        // Finally, place an office sometimes
        if (!one_in(5)) {
            if (one_in(2)) { // Office on the left side
                int office_top = bw - rng(3, 5), office_right = lw + rng(4, 7);
                // Clear out any items in that area!  And reset to floor.
                for (int i = lw + 1; i <= office_right; i++) {
                    for (int j = office_top; j <= bw - 1; j++) {
                        i_clear(i, j);
                        ter_set(i, j, t_floor);
                    }
                }
                line(this, t_wall_h, lw + 1, office_top, office_right, office_top);
                line(this, t_wall_v, office_right, office_top + 1, office_right, bw - 1);
                ter_set(office_right, rng(office_top + 1, bw - 1), t_door_locked);
                if (one_in(4)) { // Back door
                    ter_set(rng(lw + 1, office_right - 1), bw, t_door_locked_alarm);
                }
                // Finally, add some stuff in there
                place_items("office", 70, lw + 1, office_top + 1, office_right - 1, bw - 1,
                            false, 0);
                place_items("homeguns", 50, lw + 1, office_top + 1, office_right - 1,
                            bw - 1, false, 0);
                place_items("harddrugs", 20, lw + 1, office_top + 1, office_right - 1,
                            bw - 1, false, 0);
            } else { // Office on the right side
                int office_top = bw - rng(3, 5), office_left = rw - rng(4, 7);
                for (int i = office_left; i <= rw - 1; i++) {
                    for (int j = office_top; j <= bw - 1; j++) {
                        i_clear(i, j);
                        ter_set(i, j, t_floor);
                    }
                }
                line(this, t_wall_h, office_left, office_top, rw - 1, office_top);
                line(this, t_wall_v, office_left, office_top + 1, office_left, bw - 1);
                ter_set(office_left, rng(office_top + 1, bw - 1), t_door_locked);
                if (one_in(4)) { // Back door
                    ter_set(rng(office_left + 1, rw - 1), bw, t_door_locked_alarm);
                }
                place_items("office", 70, office_left + 1, office_top + 1, rw - 1, bw - 1,
                            false, 0);
                place_items("homeguns", 50, office_left + 1, office_top + 1, rw - 1,
                            bw - 1, false, 0);
                place_items("harddrugs", 20, office_left + 1, office_top + 1, rw - 1,
                            bw - 1, false, 0);
            }
        }
        if (terrain_type == "pawn_east") {
            rotate(1);
        }
        if (terrain_type == "pawn_south") {
            rotate(2);
        }
        if (terrain_type == "pawn_west") {
            rotate(3);
        }


    } else if (is_ot_type("mil_surplus", terrain_type)) {

        // Init to plain grass/dirt
        fill_background(this, &grass_or_dirt);
        lw = rng(0, 2);
        rw = SEEX * 2 - rng(1, 3);
        tw = rng(0, 4);
        bw = SEEY * 2 - rng(3, 8);
        square(this, t_floor, lw, tw, rw, bw);
        line(this, t_wall_h, lw, tw, rw, tw);
        line(this, t_wall_h, lw, bw, rw, bw);
        line(this, t_wall_v, lw, tw + 1, lw, bw - 1);
        line(this, t_wall_v, rw, tw + 1, rw, bw - 1);
        rn = rng(4, 7);
        line(this, t_window, lw + 2, tw, lw + rn, tw);
        line(this, t_window, rw - rn, tw, rw - 2, tw);
        line(this, t_door_c, SEEX, tw, SEEX + 1, tw);
        if (one_in(2)) { // counter on left
            line_furn(this, f_counter, lw + 2, tw + 1, lw + 2, tw + rng(3, 4));
        } else { // counter on right
            line_furn(this, f_counter, rw - 2, tw + 1, rw - 2, tw + rng(3, 4));
        }
        for (int i = lw + 1; i <= SEEX; i += 2) {
            line_furn(this, f_rack, i, tw + 5, i, bw - 2);
            items_location loc;
            if (one_in(3)) {
                loc = "mil_armor";
            } else if (one_in(3)) {
                loc = "mil_surplus";
            } else {
                loc = "mil_food_nodrugs";
            }
            place_items(loc, 70, i, tw + 5, i, bw - 2, false, 0);
        }
        for (int i = rw - 1; i >= SEEX + 1; i -= 2) {
            line_furn(this, f_rack, i, tw + 5, i, bw - 2);
            items_location loc;
            if (one_in(3)) {
                loc = "mil_armor";
            } else if (one_in(3)) {
                loc = "mil_surplus";
            } else {
                loc = "mil_food_nodrugs";
            }
            place_items(loc, 70, i, tw + 5, i, bw - 2, false, 0);
        }
        if (terrain_type == "mil_surplus_east") {
            rotate(1);
        }
        if (terrain_type == "mil_surplus_south") {
            rotate(2);
        }
        if (terrain_type == "mil_surplus_west") {
            rotate(3);
        }


    } else if (is_ot_type("furniture", terrain_type)) {

        fill_background(this, t_pavement);

        square(this, t_floor, 2, 2, 21, 15);

        square(this, t_floor, 2, 17, 7, 18);

        mapf::formatted_set_simple(this, 1, 1,
                                   "\
|ggggggggg++ggggggggg|\n\
| C h H      O O & & |\n\
|B      c            |\n\
|B      c            |\n\
|cccccccc   # m  e  B|\n\
|bb           m  e  B|\n\
|d          mm   e  B|\n\
|bb                 B|\n\
|bb     dd  OO  oo   |\n\
|#      dd  OO  oo  B|\n\
|h                  B|\n\
|h      EE  CC  &&  B|\n\
|H      EE  CC  &&  B|\n\
|H                   |\n\
|      BBBBBBBBBBBBBB|\n\
|DD------------------|\n\
|      D              \n\
|BBBB  D              \n\
|------|              \n",
                                   mapf::basic_bind("g - | + D", t_wall_glass_h, t_wall_h, t_wall_v, t_door_c, t_door_locked),
                                   mapf::basic_bind("# c & B C O b H h o d e m E", f_table, f_counter, f_fridge, f_rack, f_cupboard,
                                           f_oven, f_bed, f_armchair, f_chair, f_toilet, f_dresser, f_desk, f_sofa, f_bookcase),
                                   true // empty toilets
                                  );
        place_items("tools", 50, 21, 5, 21, 8, false, 0);
        //Upper Right Shelf
        place_items("hardware", 50, 21, 10, 21, 13, false, 0);
        //Right Shelf
        place_items("hardware", 75, 8, 15, 21, 15, false, 0);
        //Bottom Right Shelf
        place_items("tools", 75, 2, 18, 5, 18, false, 0);
        //Bottom Left Shelf
        place_items("magazines", 75, 2, 3, 2, 4, false, 0);
        //Upper Left Shelf

        if (terrain_type == "furniture_east") {
            rotate(1);
        }

        if (terrain_type == "furniture_south") {
            rotate(2);
        }

        if (terrain_type == "furniture_west") {
            rotate(3);
        }


    } else if (is_ot_type("abstorefront", terrain_type)) {

        fill_background(this, t_pavement);

        square(this, t_floor, 2, 2, 21, 15);
        mapf::formatted_set_simple(this, 1, 1,
                                   "\
|-xxxxxxxxDDxxxxxxxx-|\n\
|                   B|\n\
|B  c        B  B   B|\n\
|B  c        B  B   B|\n\
|B  c  B  B  B  B   B|\n\
|cccc  B  B  B  B   B|\n\
|B     B  B  B  B   B|\n\
|B                  B|\n\
|B  BBBB  BBBBBB BB B|\n\
|                BB B|\n\
|B  BBBB  BBBBBB    B|\n\
|B               --+-|\n\
|B               |B  |\n\
|BBBBBBB  BBBBBB |B  D\n\
|--------------------|\n",
                                   mapf::basic_bind("x - | + D", t_window_boarded, t_wall_h, t_wall_v, t_door_c, t_door_locked),
                                   mapf::basic_bind("B c", f_rack, f_counter));
        if (terrain_type == "abstorefront_east") {
            rotate(1);
        }

        if (terrain_type == "abstorefront_south") {
            rotate(2);
        }

        if (terrain_type == "abstorefront_west") {
            rotate(3);
        }


    } else if (terrain_type == "megastore_entrance") {

        fill_background(this, t_floor);
        // Construct facing north; below, we'll rotate to face road
        line(this, t_wall_glass_h, 0, 0, SEEX * 2 - 1, 0);
        ter_set(SEEX, 0, t_door_glass_c);
        ter_set(SEEX + 1, 0, t_door_glass_c);
        // Long checkout lanes
        for (int x = 2; x <= 18; x += 4) {
            line_furn(this, f_counter, x, 4, x, 14);
            line_furn(this, f_rack, x + 3, 4, x + 3, 14);
            place_items("snacks",    80, x + 3, 4, x + 3, 14, false, 0);
            place_items("magazines", 70, x + 3, 4, x + 3, 14, false, 0);
        }
        for (int i = 0; i < 10; i++) {
            int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
            if (ter(x, y) == t_floor) {
                add_spawn("mon_zombie", 1, x, y);
            }
        }
        // Finally, figure out where the road is; contruct our entrance facing that.
        std::vector<direction> faces_road;
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east)) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south)) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west)) {
            rotate(3);
        }


    } else if (terrain_type == "megastore") {

        square(this, t_floor, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);

        // Randomly pick contents
        switch (rng(1, 5)) {
        case 1: { // Groceries
            bool fridge = false;
            for (int x = rng(2, 3); x < SEEX * 2 - 1; x += 3) {
                for (int y = 2; y <= SEEY; y += SEEY - 2) {
                    if (one_in(3)) {
                        fridge = !fridge;
                    }
                    if (fridge) {
                        line_furn(this, f_glass_fridge, x, y, x, y + SEEY - 4);
                        if (one_in(3)) {
                            place_items("fridgesnacks", 80, x, y, x, y + SEEY - 4, false, 0);
                        } else {
                            place_items("fridge",       70, x, y, x, y + SEEY - 4, false, 0);
                        }
                    } else {
                        line_furn(this, f_rack, x, y, x, y + SEEY - 4);
                        if (one_in(3)) {
                            place_items("cannedfood", 78, x, y, x, y + SEEY - 4, false, 0);
                        } else if (one_in(2)) {
                            place_items("pasta",      82, x, y, x, y + SEEY - 4, false, 0);
                        } else if (one_in(2)) {
                            place_items("produce",    65, x, y, x, y + SEEY - 4, false, 0);
                        } else {
                            place_items("snacks",     72, x, y, x, y + SEEY - 4, false, 0);
                        }
                    }
                }
            }
        }
        break;
        case 2: // Hardware
            for (int x = 2; x <= 22; x += 4) {
                line_furn(this, f_rack, x, 4, x, SEEY * 2 - 5);
                if (one_in(3)) {
                    place_items("tools",    70, x, 4, x, SEEY * 2 - 5, false, 0);
                } else if (one_in(2)) {
                    place_items("bigtools", 70, x, 4, x, SEEY * 2 - 5, false, 0);
                } else if (one_in(3)) {
                    place_items("hardware", 70, x, 4, x, SEEY * 2 - 5, false, 0);
                } else {
                    place_items("mischw",   70, x, 4, x, SEEY * 2 - 5, false, 0);
                }
            }
            break;
        case 3: // Clothing
            for (int x = 2; x < SEEX * 2; x += 6) {
                for (int y = 3; y <= 9; y += 6) {
                    square_furn(this, f_rack, x, y, x + 1, y + 1);
                    if (one_in(2)) {
                        place_items("shirts",  75, x, y, x + 1, y + 1, false, 0);
                    } else if (one_in(2)) {
                        place_items("pants",   72, x, y, x + 1, y + 1, false, 0);
                    } else if (one_in(2)) {
                        place_items("jackets", 65, x, y, x + 1, y + 1, false, 0);
                    } else {
                        place_items("winter",  62, x, y, x + 1, y + 1, false, 0);
                    }
                }
            }
            for (int y = 13; y <= SEEY * 2 - 2; y += 3) {
                line_furn(this, f_rack, 2, y, SEEX * 2 - 3, y);
                if (one_in(3)) {
                    place_items("shirts",     75, 2, y, SEEX * 2 - 3, y, false, 0);
                } else if (one_in(2)) {
                    place_items("shoes",      75, 2, y, SEEX * 2 - 3, y, false, 0);
                } else if (one_in(2)) {
                    place_items("bags",       75, 2, y, SEEX * 2 - 3, y, false, 0);
                } else {
                    place_items("allclothes", 75, 2, y, SEEX * 2 - 3, y, false, 0);
                }
            }
            break;
        case 4: // Cleaning and soft drugs and novels and junk
            for (int x = rng(2, 3); x < SEEX * 2 - 1; x += 3) {
                for (int y = 2; y <= SEEY; y += SEEY - 2) {
                    line_furn(this, f_rack, x, y, x, y + SEEY - 4);
                    if (one_in(3)) {
                        place_items("cleaning",  78, x, y, x, y + SEEY - 4, false, 0);
                    } else if (one_in(2)) {
                        place_items("softdrugs", 72, x, y, x, y + SEEY - 4, false, 0);
                    } else {
                        place_items("novels",    84, x, y, x, y + SEEY - 4, false, 0);
                    }
                }
            }
            break;
        case 5: // Sporting goods
            for (int x = rng(2, 3); x < SEEX * 2 - 1; x += 3) {
                for (int y = 2; y <= SEEY; y += SEEY - 2) {
                    line_furn(this, f_rack, x, y, x, y + SEEY - 4);
                    if (one_in(2)) {
                        place_items("sports",  72, x, y, x, y + SEEY - 4, false, 0);
                    } else if (one_in(10)) {
                        place_items("rifles",  20, x, y, x, y + SEEY - 4, false, 0);
                    } else {
                        place_items("camping", 68, x, y, x, y + SEEY - 4, false, 0);
                    }
                }
            }
            break;
        }

        // Add some spawns
        for (int i = 0; i < 15; i++) {
            int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
            if (ter(x, y) == t_floor) {
                add_spawn("mon_zombie", 1, x, y);
            }
        }
        // Rotate randomly...
        rotate(rng(0, 3));
        // ... then place walls as needed.
        if (t_north != "megastore_entrance" && t_north != "megastore") {
            line(this, t_wall_h, 0, 0, SEEX * 2 - 1, 0);
        }
        if (t_east != "megastore_entrance" && t_east != "megastore") {
            line(this, t_wall_v, SEEX * 2 - 1, 0, SEEX * 2 - 1, SEEY * 2 - 1);
        }
        if (t_south != "megastore_entrance" && t_south != "megastore") {
            line(this, t_wall_h, 0, SEEY * 2 - 1, SEEX * 2 - 1, SEEY * 2 - 1);
        }
        if (t_west != "megastore_entrance" && t_west != "megastore") {
            line(this, t_wall_v, 0, 0, 0, SEEY * 2 - 1);
        }


    } else if (terrain_type == "hospital_entrance") {

        square(this, t_pavement, 0, 0, SEEX * 2 - 1, 5);
        square(this, t_floor, 0, 6, SEEX * 2 - 1, SEEY * 2 - 1);
        // Ambulance parking place
        line(this, t_pavement_y,  5, 1, 22, 1);
        line(this, t_pavement_y,  5, 2,  5, 5);
        line(this, t_pavement_y, 22, 2, 22, 5);
        // Top wall
        line(this, t_wall_h, 0, 6, 6, 6);
        line(this, t_door_glass_c, 3, 6, 4, 6);
        line(this, t_wall_glass_h, 7, 6, 19, 6);
        line(this, t_wall_h, 20, 6, SEEX * 2 - 1, 6);
        // Left wall
        line(this, t_wall_v, 0, 0, 0, SEEY * 2 - 1);
        line(this, t_floor, 0, 11, 0, 12);
        // Waiting area
        line_furn(this, f_bench,  8, 7, 11,  7);
        line_furn(this, f_bench, 13, 7, 17,  7);
        line_furn(this, f_bench, 20, 7, 22,  7);
        line_furn(this, f_bench, 22, 8, 22, 10);
        place_items("magazines", 70, 8, 7, 22, 10, false, 0);
        // Reception and examination rooms
        line_furn(this, f_counter, 8, 13, 9, 13);
        line(this, t_wall_h, 10, 13, SEEX * 2 - 1, 13);
        line(this, t_door_c, 15, 13, 16, 13);
        line(this, t_wall_h,  8, 17, 13, 17);
        line(this, t_wall_h, 18, 17, 22, 17);
        line(this, t_wall_h,  8, 20, 13, 20);
        line(this, t_wall_h, 18, 20, 22, 20);
        line(this, t_wall_v,  7, 13,  7, 22);
        line(this, t_wall_v, 14, 15, 14, 20);
        line(this, t_wall_v, 17, 14, 17, 22);
        line_furn(this, f_bed,  8, 19,  9, 19);
        line_furn(this, f_bed, 21, 19, 22, 19);
        line_furn(this, f_bed, 21, 22, 22, 22);
        line_furn(this, f_rack, 18, 14, 22, 14);
        place_items("harddrugs", 80, 18, 14, 22, 14, false, 0);
        line_furn(this, f_rack, 8, 21, 8, 22);
        place_items("softdrugs", 70, 8, 21, 8, 22, false, 0);
        ter_set(14, rng(18, 19), t_door_c);
        ter_set(17, rng(15, 16), t_door_locked); // Hard drugs room is locked
        ter_set(17, rng(18, 19), t_door_c);
        ter_set(17, rng(21, 22), t_door_c);
        // ER and bottom wall
        line(this, t_wall_h, 0, 16, 6, 16);
        line(this, t_door_c, 3, 16, 4, 16);
        square_furn(this, f_bed, 3, 19, 4, 20);
        line_furn(this, f_counter, 1, 22, 6, 22);
        place_items("surgery", 78, 1, 22, 6, 22, false, 0);
        line(this, t_wall_h, 1, 23, 22, 23);
        line(this, t_floor, 11, 23, 12, 23);
        // Right side wall (needed sometimes!)
        line(this, t_wall_v, 23,  0, 23, 10);
        line(this, t_wall_v, 23, 13, 23, 23);

        /*
        // Generate bodies / zombies
          rn = rng(10, 15);
          for (int i = 0; i < rn; i++) {
           item body;
           body.make_corpse(itypes["corpse"], GetMType("mon_null"), g->turn);
           int zx = rng(0, SEEX * 2 - 1), zy = rng(0, SEEY * 2 - 1);
           if (ter(zx, zy) == t_bed || one_in(3))
            add_item(zx, zy, body);
           else if (move_cost(zx, zy) > 0) {
            mon_id zom = mon_zombie;
            if (one_in(6))
             zom = mon_zombie_spitter;
            else if (!one_in(3))
             zom = mon_boomer;
            add_spawn(zom, 1, zx, zy);
           }
          }
        */
        // Rotate to face the road
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east)) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south)) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west)) {
            rotate(3);
        }


    } else if (terrain_type == "hospital") {

        fill_background(this, t_floor);
        // We always have walls on the left and bottom
        line(this, t_wall_v, 0,  0,  0, 22);
        line(this, t_wall_h, 0, 23, 23, 23);
        // These walls contain doors if they lead to more hospital
        if (t_west == "hospital_entrance" || t_west == "hospital") {
            line(this, t_door_c, 0, 11, 0, 12);
        }
        if (t_south == "hospital_entrance" || t_south == "hospital") {
            line(this, t_door_c, 11, 23, 12, 23);
        }

        if ((t_north == "hospital_entrance" || t_north == "hospital") &&
            (t_east  == "hospital_entrance" || t_east  == "hospital") &&
            (t_south == "hospital_entrance" || t_south == "hospital") &&
            (t_west  == "hospital_entrance" || t_west  == "hospital")   ) {
            // We're in the center; center is always blood lab
            // Large lab
            line(this, t_wall_h,  1,  2, 21,  2);
            line(this, t_wall_h,  1, 10, 21, 10);
            line(this, t_wall_v, 21,  3, 21,  9);
            line_furn(this, f_counter, 2,  3,  2,  9);
            place_items("hospital_lab", 70, 2, 3, 2, 9, false, 0);
            square_furn(this, f_counter,  5,  4,  6,  8);
            place_items("hospital_lab", 74, 5, 4, 6, 8, false, 0);
            square_furn(this, f_counter, 10,  4, 11,  8);
            spawn_item(5, 17, "record_patient");
            place_items("hospital_lab", 74, 10, 4, 11, 8, false, 0);
            square_furn(this, f_counter, 15,  4, 16,  8);
            place_items("hospital_lab", 74, 15, 4, 16, 8, false, 0);
            ter_set(rng(3, 18),  2, t_door_c);
            ter_set(rng(3, 18), 10, t_door_c);
            if (one_in(4)) { // Door on the right side
                ter_set(21, rng(4, 8), t_door_c);
            } else { // Counter on the right side
                line_furn(this, f_counter, 20, 3, 20, 9);
                place_items("hospital_lab", 70, 20, 3, 20, 9, false, 0);
            }
            // Blood testing facility
            line(this, t_wall_h,  1, 13, 10, 13);
            line(this, t_wall_v, 10, 14, 10, 22);
            rn = rng(1, 3);
            if (rn == 1 || rn == 3) {
                ter_set(rng(3, 8), 13, t_door_c);
            }
            if (rn == 2 || rn == 3) {
                ter_set(10, rng(15, 21), t_door_c);
            }
            line_furn(this, f_counter, 2, 14,  2, 22);
            place_items("hospital_lab", 60, 2, 14, 2, 22, false, 0);
            square_furn(this, f_counter, 4, 17, 6, 19);
            ter_set(4, 18, t_centrifuge);
            line(this, t_floor, 5, 18, 6, rng(17, 19)); // Clear path to console
            tmpcomp = add_computer(5, 18, _("Centrifuge"), 0);
            tmpcomp->add_option(_("Analyze blood"), COMPACT_BLOOD_ANAL, 4);
            tmpcomp->add_failure(COMPFAIL_DESTROY_BLOOD);
            // Sample storage
            line(this, t_wall_h, 13, 13, 23, 13);
            line(this, t_wall_v, 13, 14, 13, 23);
            rn = rng(1, 3);
            if (rn == 1 || rn == 3) {
                ter_set(rng(14, 22), 13, t_door_c);
            }
            if (rn == 2 || rn == 3) {
                ter_set(13, rng(14, 21), t_door_c);
            }
            square_furn(this, f_rack, 16, 16, 21, 17);
            place_items("hospital_samples", 68, 16, 16, 21, 17, false, 0);
            square_furn(this, f_rack, 16, 19, 21, 20);
            place_items("hospital_samples", 68, 16, 19, 21, 20, false, 0);
            line_furn(this, f_rack, 14, 22, 23, 22);
            place_items("hospital_samples", 62, 14, 22, 23, 22, false, 0);

        } else { // We're NOT in the center; a random hospital type!

            switch (rng(1, 4)) { // What type?
            case 1: // Dorms
                // Upper left rooms
                line(this, t_wall_h, 1, 5, 9, 5);
                for (int i = 1; i <= 7; i += 3) {
                    line_furn(this, f_bed, i, 1, i, 2);
                    line(this, t_wall_v, i + 2, 0, i + 2, 4);
                    ter_set(rng(i, i + 1), 5, t_door_c);
                }
                // Upper right rooms
                line(this, t_wall_h, 14, 5, 23, 5);
                line(this, t_wall_v, 14, 0, 14, 4);
                line_furn(this, f_bed, 15, 1, 15, 2);
                ter_set(rng(15, 16), 5, t_door_c);
                line(this, t_wall_v, 17, 0, 17, 4);
                line_furn(this, f_bed, 18, 1, 18, 2);
                ter_set(rng(18, 19), 5, t_door_c);
                line(this, t_wall_v, 20, 0, 20, 4);
                line_furn(this, f_bed, 21, 1, 21, 2);
                ter_set(rng(21, 22), 5, t_door_c);
                // Waiting area
                for (int i = 1; i <= 9; i += 4) {
                    line_furn(this, f_bench, i, 7, i, 10);
                }
                line_furn(this, f_table, 3, 8, 3, 9);
                place_items("magazines", 50, 3, 8, 3, 9, false, 0);
                line_furn(this, f_table, 7, 8, 7, 9);
                place_items("magazines", 50, 7, 8, 7, 9, false, 0);
                // Middle right rooms
                line(this, t_wall_v, 14, 7, 14, 10);
                line(this, t_wall_h, 15, 7, 23, 7);
                line(this, t_wall_h, 15, 10, 23, 10);
                line(this, t_wall_v, 19, 8, 19, 9);
                line_furn(this, f_bed, 18, 8, 18, 9);
                line_furn(this, f_bed, 20, 8, 20, 9);
                if (one_in(3)) { // Doors to north
                    ter_set(rng(15, 16), 7, t_door_c);
                    ter_set(rng(21, 22), 7, t_door_c);
                } else { // Doors to south
                    ter_set(rng(15, 16), 10, t_door_c);
                    ter_set(rng(21, 22), 10, t_door_c);
                }
                line(this, t_wall_v, 14, 13, 14, 16);
                line(this, t_wall_h, 15, 13, 23, 13);
                line(this, t_wall_h, 15, 16, 23, 16);
                line(this, t_wall_v, 19, 14, 19, 15);
                line_furn(this, f_bed, 18, 14, 18, 15);
                line_furn(this, f_bed, 20, 14, 20, 15);
                if (one_in(3)) { // Doors to south
                    ter_set(rng(15, 16), 16, t_door_c);
                    ter_set(rng(21, 22), 16, t_door_c);
                } else { // Doors to north
                    ter_set(rng(15, 16), 13, t_door_c);
                    ter_set(rng(21, 22), 13, t_door_c);
                }
                // Lower left rooms
                line(this, t_wall_v, 5, 13, 5, 22);
                line(this, t_wall_h, 1, 13, 4, 13);
                line_furn(this, f_bed, 1, 14, 1, 15);
                line(this, t_wall_h, 1, 17, 4, 17);
                line_furn(this, f_bed, 1, 18, 1, 19);
                line(this, t_wall_h, 1, 20, 4, 20);
                line_furn(this, f_bed, 1, 21, 1, 22);
                ter_set(5, rng(14, 16), t_door_c);
                ter_set(5, rng(18, 19), t_door_c);
                ter_set(5, rng(21, 22), t_door_c);
                line(this, t_wall_h, 7, 13, 10, 13);
                line(this, t_wall_v, 7, 14, 7, 22);
                line(this, t_wall_v, 10, 14, 10, 22);
                line(this, t_wall_h, 8, 18, 9, 18);
                line_furn(this, f_bed, 8, 17, 9, 17);
                line_furn(this, f_bed, 8, 22, 9, 22);
                if (one_in(3)) { // Doors to west
                    ter_set(7, rng(14, 16), t_door_c);
                    ter_set(7, rng(19, 21), t_door_c);
                } else { // Doors to east
                    ter_set(10, rng(14, 16), t_door_c);
                    ter_set(10, rng(19, 21), t_door_c);
                }
                // Lower-right rooms
                line(this, t_wall_h, 14, 18, 23, 18);
                for (int i = 14; i <= 20; i += 3) {
                    line(this, t_wall_v, i, 19, i, 22);
                    line_furn(this, f_bed, i + 1, 21, i + 1, 22);
                    ter_set(rng(i + 1, i + 2), 18, t_door_c);
                }
                break;

            case 2: // Offices and cafeteria
                // Offices to north
                line(this, t_wall_v, 10, 0, 10, 8);
                line(this, t_wall_v, 13, 0, 13, 8);
                line(this, t_wall_h,  1, 5,  9, 5);
                line(this, t_wall_h, 14, 5, 23, 5);
                line(this, t_wall_h,  1, 9, 23, 9);
                line(this, t_door_c, 11, 9, 12, 9);
                line_furn(this, f_table,  3, 3,  7, 3);
                line_furn(this, f_table, 16, 3, 20, 3);
                line_furn(this, f_table,  3, 8,  7, 8);
                line_furn(this, f_table, 16, 8, 20, 8);
                ter_set(10, rng(2, 3), t_door_c);
                ter_set(13, rng(2, 3), t_door_c);
                ter_set(10, rng(6, 7), t_door_c);
                ter_set(13, rng(6, 7), t_door_c);
                place_items("office", 70,  1, 1,  9, 3, false, 0);
                place_items("office", 70, 14, 1, 22, 3, false, 0);
                place_items("office", 70,  1, 5,  9, 8, false, 0);
                place_items("office", 70, 14, 5, 22, 8, false, 0);
                // Cafeteria to south
                line(this, t_wall_h,  1, 14,  8, 14);
                line(this, t_wall_h, 15, 14, 23, 14);
                for (int i = 16; i <= 19; i += 3) {
                    for (int j = 17; j <= 20; j += 3) {
                        square_furn(this, f_table, i, j, i + 1, j + 1);
                        place_items("snacks",  60, i, j, i + 1, j + 1, false, 0);
                        place_items("produce", 65, i, j, i + 1, j + 1, false, 0);
                    }
                }
                for (int i = 3; i <= 6; i += 3) {
                    for (int j = 17; j <= 20; j += 3) {
                        square_furn(this, f_table, i, j, i + 1, j + 1);
                        place_items("snacks",  60, i, j, i + 1, j + 1, false, 0);
                        place_items("produce", 65, i, j, i + 1, j + 1, false, 0);
                    }
                }
                break;

            case 3: // Operating rooms
                // First, walls and doors; divide it into four big operating rooms
                line(this, t_wall_v, 10,  0, 10,  9);
                line(this, t_door_c, 10,  4, 10,  5);
                line(this, t_wall_v, 13,  0, 13,  9);
                line(this, t_door_c, 13,  4, 13,  5);
                line(this, t_wall_v, 10, 14, 10, 22);
                line(this, t_door_c, 10, 18, 10, 19);
                line(this, t_wall_v, 13, 14, 13, 22);
                line(this, t_door_c, 13, 18, 13, 19);
                // Horizontal walls/doors
                line(this, t_wall_h,  1, 10, 10, 10);
                line(this, t_door_c,  5, 10,  6, 10);
                line(this, t_wall_h, 13, 10, 23, 10);
                line(this, t_door_c, 18, 10, 19, 10);
                line(this, t_wall_h,  1, 13, 10, 13);
                line(this, t_door_c,  5, 13,  6, 13);
                line(this, t_wall_h, 13, 13, 23, 13);
                line(this, t_door_c, 18, 13, 19, 13);
                // Next, the contents of each operating room
                line_furn(this, f_counter, 1, 0, 1, 9);
                place_items("surgery", 70, 1, 1, 1, 9, false, 0);
                square_furn(this, f_bed, 5, 4, 6, 5);

                line_furn(this, f_counter, 1, 14, 1, 22);
                place_items("surgery", 70, 1, 14, 1, 22, false, 0);
                square_furn(this, f_bed, 5, 18, 6, 19);

                line_furn(this, f_counter, 14, 6, 14, 9);
                place_items("surgery", 60, 14, 6, 14, 9, false, 0);
                line_furn(this, f_counter, 15, 9, 17, 9);
                place_items("surgery", 60, 15, 9, 17, 9, false, 0);
                square_furn(this, f_bed, 18, 4, 19, 5);

                line_furn(this, f_counter, 14, 14, 14, 17);
                place_items("surgery", 60, 14, 14, 14, 17, false, 0);
                line_furn(this, f_counter, 15, 14, 17, 14);
                place_items("surgery", 60, 15, 14, 17, 14, false, 0);
                square_furn(this, f_bed, 18, 18, 19, 19);
                // computer to begin healing broken bones,
                tmpcomp = add_computer(16, 16, _("Mr. Stem Cell"), 3);
                tmpcomp->add_option(_("Stem Cell Treatment"), COMPACT_STEMCELL_TREATMENT, 3);
                tmpcomp->add_failure(COMPFAIL_ALARM);

                break;

            case 4: // Storage
                // Soft drug storage
                line(this, t_wall_h, 3,  2, 12,  2);
                line(this, t_wall_h, 3, 10, 12, 10);
                line(this, t_wall_v, 3,  3,  3,  9);
                ter_set(3, 6, t_door_c);
                line_furn(this, f_rack,   4,  3, 11,  3);
                place_items("softdrugs", 90, 4, 3, 11, 3, false, 0);
                line_furn(this, f_rack,   4,  9, 11,  9);
                place_items("softdrugs", 90, 4, 9, 11, 9, false, 0);
                line_furn(this, f_rack, 6, 5, 10, 5);
                place_items("softdrugs", 80, 6, 5, 10, 5, false, 0);
                line_furn(this, f_rack, 6, 7, 10, 7);
                place_items("softdrugs", 80, 6, 7, 10, 7, false, 0);
                // Hard drug storage
                line(this, t_wall_v, 13, 0, 13, 19);
                ter_set(13, 6, t_door_locked);
                line_furn(this, f_rack, 14, 0, 14, 4);
                place_items("harddrugs", 78, 14, 1, 14, 4, false, 0);
                line_furn(this, f_rack, 17, 0, 17, 7);
                place_items("harddrugs", 85, 17, 0, 17, 7, false, 0);
                line_furn(this, f_rack, 20, 0, 20, 7);
                place_items("harddrugs", 85, 20, 0, 20, 7, false, 0);
                line(this, t_wall_h, 20, 10, 23, 10);
                line_furn(this, f_rack, 16, 10, 19, 10);
                place_items("harddrugs", 78, 16, 10, 19, 10, false, 0);
                line_furn(this, f_rack, 16, 12, 19, 12);
                place_items("harddrugs", 78, 16, 12, 19, 12, false, 0);
                line(this, t_wall_h, 14, 14, 19, 14);
                ter_set(rng(14, 15), 14, t_door_locked);
                ter_set(rng(16, 18), 14, t_bars);
                line(this, t_wall_v, 20, 11, 20, 19);
                line(this, t_wall_h, 13, 20, 20, 20);
                line(this, t_door_c, 16, 20, 17, 20);
                // Laundry room
                line(this, t_wall_h, 1, 13, 10, 13);
                ter_set(rng(3, 8), 13, t_door_c);
                line(this, t_wall_v, 10, 14, 10, 22);
                ter_set(10, rng(16, 20), t_door_c);
                line_furn(this, f_counter, 1, 14, 1, 22);
                place_items("allclothes", 70, 1, 14, 1, 22, false, 0);
                for (int j = 15; j <= 21; j += 3) {
                    line_furn(this, f_rack, 4, j, 7, j);
                    if (one_in(2)) {
                        place_items("cleaning", 92, 4, j, 7, j, false, 0);
                    } else if (one_in(5)) {
                        place_items("cannedfood", 75, 4, j, 7, j, false, 0);
                    } else {
                        place_items("allclothes", 50, 4, j, 7, j, false, 0);
                    }
                }
                break;
            }


            // We have walls to the north and east if they're not hospitals
            if (t_east != "hospital_entrance" && t_east != "hospital") {
                line(this, t_wall_v, 23, 0, 23, 23);
            }
            if (t_north != "hospital_entrance" && t_north != "hospital") {
                line(this, t_wall_h, 0, 0, 23, 0);
            }
        }
        // Generate bodies / zombies
        rn = rng(15, 20);
        for (int i = 0; i < rn; i++) {
            item body;
            body.make_corpse(itypes["corpse"], GetMType("mon_null"), g->turn);
            int zx = rng(0, SEEX * 2 - 1), zy = rng(0, SEEY * 2 - 1);
            if (move_cost(zx, zy) > 0) {
                if (furn(zx, zy) == f_bed || one_in(3)) {
                    add_item(zx, zy, body);
                } else {
                    std::string zom = "mon_zombie";
                    if (one_in(6)) {
                        zom = "mon_zombie_spitter";
                    } else if (!one_in(3)) {
                        zom = "mon_boomer";
                    }
                    add_spawn(zom, 1, zx, zy);
                }
            } else {
                //This is a wall: try again
                i--;
            }
        }


    } else if (terrain_type == "mansion_entrance") {

        // Left wall
        line(this, t_wall_v,  0,  0,  0, SEEY * 2 - 2);
        line(this, t_door_c,  0, SEEY - 1, 0, SEEY);
        // Front wall
        line(this, t_wall_h,  1, 10,  SEEX * 2 - 1, 10);
        line(this, t_door_locked, SEEX - 1, 10, SEEX, 10);
        int winx1 = rng(2, 4);
        int winx2 = rng(4, 6);
        line(this, t_window, winx1, 10, winx2, 10);
        line(this, t_window, SEEX * 2 - 1 - winx1, 10, SEEX * 2 - 1 - winx2, 10);
        winx1 = rng(7, 10);
        winx2 = rng(10, 11);
        line(this, t_window, winx1, 10, winx2, 10);
        line(this, t_window, SEEX * 2 - 1 - winx1, 10, SEEX * 2 - 1 - winx2, 10);
        line(this, t_door_c, SEEX - 1, 10, SEEX, 10);
        // Bottom wall
        line(this, t_wall_h,  0, SEEY * 2 - 1, SEEX * 2 - 1, SEEY * 2 - 1);
        line(this, t_door_c, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);

        build_mansion_room(this, room_mansion_courtyard, 1, 0, SEEX * 2 - 1, 9);
        square(this, t_floor, 1, 11, SEEX * 2 - 1, SEEY * 2 - 2);
        build_mansion_room(this, room_mansion_entry, 1, 11, SEEX * 2 - 1, SEEY * 2 - 2);
        // Rotate to face the road
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east)) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south)) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west)) {
            rotate(3);
        }
        // add zombies
        if (one_in(3)) {
            add_spawn("mon_zombie", rng(1, 8), 12, 12);
        }


    } else if (terrain_type == "mansion") {

        // Start with floors all over
        square(this, t_floor, 1, 0, SEEX * 2 - 1, SEEY * 2 - 2);
        // We always have a left and bottom wall
        line(this, t_wall_v, 0, 0, 0, SEEY * 2 - 2);
        line(this, t_wall_h, 0, SEEY * 2 - 1, SEEX * 2 - 1, SEEY * 2 - 1);
        // tw and rw are the boundaries of the rooms inside...
        tw = 0;
        rw = SEEX * 2 - 1;
        // ...if we need outside walls, adjust tw & rw and build them
        // We build windows below.
        if (t_north != "mansion_entrance" && t_north != "mansion") {
            tw = 1;
            line(this, t_wall_h, 0, 0, SEEX * 2 - 1, 0);
        }
        if (t_east != "mansion_entrance" && t_east != "mansion") {
            rw = SEEX * 2 - 2;
            line(this, t_wall_v, SEEX * 2 - 1, 0, SEEX * 2 - 1, SEEX * 2 - 1);
        }
        // Now pick a random layout
        switch (rng(1, 10)) {

        case 1: // Just one. big. room.
            mansion_room(this, 1, tw, rw, SEEY * 2 - 2);
            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_door_c, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_door_c, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;

        case 2: // Wide hallway, two rooms.
        case 3:
            if (one_in(2)) { // vertical hallway
                line(this, t_wall_v,  9,  tw,  9, SEEY * 2 - 2);
                line(this, t_wall_v, 14,  tw, 14, SEEY * 2 - 2);
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
                line(this, t_door_c, 0, SEEY - 1, 0, SEEY);
                mansion_room(this, 1, tw, 8, SEEY * 2 - 2);
                mansion_room(this, 15, tw, rw, SEEY * 2 - 2);
                ter_set( 9, rng(tw + 2, SEEX * 2 - 4), t_door_c);
                ter_set(14, rng(tw + 2, SEEX * 2 - 4), t_door_c);
            } else { // horizontal hallway
                line(this, t_wall_h, 1,  9, rw,  9);
                line(this, t_wall_h, 1, 14, rw, 14);
                line(this, t_door_c, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
                line(this, t_floor, 0, SEEY - 1, 0, SEEY);
                mansion_room(this, 1, tw, rw, 8);
                mansion_room(this, 1, 15, rw, SEEY * 2 - 2);
                ter_set(rng(3, rw - 2),  9, t_door_c);
                ter_set(rng(3, rw - 2), 14, t_door_c);
            }
            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_door_c, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;

        case 4: // Four corners rooms
        case 5:
        case 6:
        case 7:
        case 8:
            line(this, t_wall_v, 10, tw, 10,  9);
            line(this, t_wall_v, 13, tw, 13,  9);
            line(this, t_wall_v, 10, 14, 10, SEEY * 2 - 2);
            line(this, t_wall_v, 13, 14, 13, SEEY * 2 - 2);
            line(this, t_wall_h,  1, 10, 10, 10);
            line(this, t_wall_h,  1, 13, 10, 13);
            line(this, t_wall_h, 13, 10, rw, 10);
            line(this, t_wall_h, 13, 13, rw, 13);
            // Doors
            if (one_in(2)) {
                ter_set(10, rng(tw + 1, 8), t_door_c);
            } else {
                ter_set(rng(2, 8), 10, t_door_c);
            }

            if (one_in(2)) {
                ter_set(13, rng(tw + 1, 8), t_door_c);
            } else {
                ter_set(rng(15, rw - 1), 10, t_door_c);
            }

            if (one_in(2)) {
                ter_set(10, rng(15, SEEY * 2 - 3), t_door_c);
            } else {
                ter_set(rng(2, 8), 13, t_door_c);
            }

            if (one_in(2)) {
                ter_set(13, rng(15, SEEY * 2 - 3), t_door_c);
            } else {
                ter_set(rng(15, rw - 1), 13, t_door_c);
            }

            mansion_room(this,  1, tw,  9,  9);
            mansion_room(this, 14, tw, rw,  9);
            mansion_room(this,  1, 14,  9, SEEY * 2 - 2);
            mansion_room(this, 14, 14, rw, SEEY * 2 - 2);
            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_floor, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;

        case 9: // One large room in lower-left
        case 10:
            mw = rng( 4, 10);
            cw = rng(13, 19);
            x = rng(5, 10);
            y = rng(13, 18);
            line(this, t_wall_h,  1, mw, cw, mw);
            ter_set( rng(x + 1, cw - 1), mw, t_door_c);
            line(this, t_wall_v, cw, mw + 1, cw, SEEY * 2 - 2);
            ter_set(cw, rng(y + 2, SEEY * 2 - 3) , t_door_c);
            mansion_room(this, 1, mw + 1, cw - 1, SEEY * 2 - 2);
            // And a couple small rooms in the UL LR corners
            line(this, t_wall_v, x, tw, x, mw - 1);
            mansion_room(this, 1, tw, x - 1, mw - 1);
            if (one_in(2)) {
                ter_set(rng(2, x - 2), mw, t_door_c);
            } else {
                ter_set(x, rng(tw + 2, mw - 2), t_door_c);
            }
            line(this, t_wall_h, cw + 1, y, rw, y);
            mansion_room(this, cw + 1, y + 1, rw, SEEY * 2 - 2);
            if (one_in(2)) {
                ter_set(rng(cw + 2, rw - 1), y, t_door_c);
            } else {
                ter_set(cw, rng(y + 2, SEEY * 2 - 3), t_door_c);
            }

            if (t_west == "mansion_entrance" || t_west == "mansion") {
                line(this, t_floor, 0, SEEY - 1, 0, SEEY);
            }
            if (t_south == "mansion_entrance" || t_south == "mansion") {
                line(this, t_floor, SEEX - 1, SEEY * 2 - 1, SEEX, SEEY * 2 - 1);
            }
            break;
        } // switch (rng(1, 4))

        // Finally, place windows on outside-facing walls if necessary
        if (t_west != "mansion_entrance" && t_west != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEY; i++) {
                if (move_cost(1, i) != 0 && move_cost(1, SEEY * 2 - 1 - i) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(0, i, t_window);
                        ter_set(0, SEEY * 2 - 1 - i, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }
        if (t_south != "mansion_entrance" && t_south != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEX; i++) {
                if (move_cost(i, SEEY * 2 - 2) != 0 &&
                    move_cost(SEEX * 2 - 1 - i, SEEY * 2 - 2) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(i, SEEY * 2 - 1, t_window);
                        ter_set(SEEX * 2 - 1 - i, SEEY * 2 - 1, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }
        if (t_east != "mansion_entrance" && t_east != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEY; i++) {
                if (move_cost(SEEX * 2 - 2, i) != 0 &&
                    move_cost(SEEX * 2 - 2, SEEY * 2 - 1 - i) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(SEEX * 2 - 1, i, t_window);
                        ter_set(SEEX * 2 - 1, SEEY * 2 - 1 - i, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }

        if (t_north != "mansion_entrance" && t_north != "mansion") {
            int consecutive = 0;
            for (int i = 1; i < SEEX; i++) {
                if (move_cost(i, 1) != 0 && move_cost(SEEX * 2 - 1 - i, 1) != 0) {
                    if (consecutive == 3) {
                        consecutive = 0;    // No really long windows
                    } else {
                        consecutive++;
                        ter_set(i, 0, t_window);
                        ter_set(SEEX * 2 - 1 - i, 0, t_window);
                    }
                } else {
                    consecutive = 0;
                }
            }
        }
        // add zombies
        if (one_in(2)) {
            add_spawn("mon_zombie", rng(4, 8), 12, 12);
        }


    } else if (terrain_type == "fema_entrance") {

        fill_background(this, t_dirt);
        // Left wall
        line(this, t_chainfence_v,  0,  0,  0, SEEY * 2 - 2);
        line(this, t_fence_barbed, 1, 4, 9, 12);
        line(this, t_fence_barbed, 1, 5, 8, 12);
        line(this, t_fence_barbed, 23, 4, 15, 12);
        line(this, t_fence_barbed, 23, 5, 16, 12);
        square(this, t_wall_wood, 2, 13, 9, 21);
        square(this, t_floor, 3, 14, 8, 20);
        line(this, t_reinforced_glass_h, 5, 13, 6, 13);
        line(this, t_reinforced_glass_h, 5, 21, 6, 21);
        line(this, t_reinforced_glass_v, 9, 15, 9, 18);
        line(this, t_door_c, 9, 16, 9, 17);
        line_furn(this, f_locker, 3, 16, 3, 18);
        line_furn(this, f_chair, 5, 16, 5, 18);
        line_furn(this, f_desk, 6, 16, 6, 18);
        line_furn(this, f_chair, 7, 16, 7, 18);
        place_items("office", 80, 3, 16, 3, 18, false, 0);
        place_items("office", 80, 6, 16, 6, 18, false, 0);
        add_spawn("mon_zombie_soldier", rng(1, 6), 4, 17);

        // Rotate to face the road
        if (is_ot_type("road", t_east) || is_ot_type("bridge", t_east)) {
            rotate(1);
        }
        if (is_ot_type("road", t_south) || is_ot_type("bridge", t_south)) {
            rotate(2);
        }
        if (is_ot_type("road", t_west) || is_ot_type("bridge", t_west)) {
            rotate(3);
        }


    } else if (terrain_type == "fema") {

        fill_background(this, t_dirt);
        line(this, t_chainfence_v, 0, 0, 0, 23);
        line(this, t_chainfence_h, 0, 23, 23, 23);
        if (t_north != "fema_entrance" && t_north != "fema") {
            line(this, t_chainfence_h, 0, 0, 23, 0);
        }
        if (t_east != "fema_entrance" && t_east != "fema") {
            line(this, t_chainfence_v, 23, 0, 23, 23);
        }
        if (t_south == "fema") {
            line(this, t_dirt, 0, 23, 23, 23);
        }
        if (t_west == "fema") {
            line(this, t_dirt, 0, 0, 0, 23);
        }
        if(t_west == "fema" && t_east == "fema" && t_south != "fema") {
            //lab bottom side
            square(this, t_dirt, 1, 1, 22, 22);
            square(this, t_floor, 4, 4, 19, 19);
            line(this, t_concrete_h, 4, 4, 19, 4);
            line(this, t_concrete_h, 4, 19, 19, 19);
            line(this, t_concrete_v, 4, 5, 4, 18);
            line(this, t_concrete_v, 19, 5, 19, 18);
            line(this, t_door_metal_c, 11, 4, 12, 4);
            line_furn(this, f_glass_fridge, 6, 5, 9, 5);
            line_furn(this, f_glass_fridge, 14, 5, 17, 5);
            square(this, t_grate, 6, 8, 8, 9);
            line_furn(this, f_table, 7, 8, 7, 9);
            square(this, t_grate, 6, 12, 8, 13);
            line_furn(this, f_table, 7, 12, 7, 13);
            square(this, t_grate, 6, 16, 8, 17);
            line_furn(this, f_table, 7, 16, 7, 17);
            line_furn(this, f_counter, 10, 8, 10, 17);
            square_furn(this, f_chair, 14, 8, 17, 10);
            square(this, t_console_broken, 15, 8, 16, 10);
            line_furn(this, f_desk, 15, 11, 16, 11);
            line_furn(this, f_chair, 15, 12, 16, 12);
            line(this, t_reinforced_glass_h, 13, 14, 18, 14);
            line(this, t_reinforced_glass_v, 13, 14, 13, 18);
            ter_set(15, 14, t_door_metal_locked);
            place_items("dissection", 90, 10, 8, 10, 17, false, 0);
            place_items("hospital_lab", 70, 5, 5, 18, 18, false, 0);
            place_items("harddrugs", 50, 6, 5, 9, 5, false, 0);
            place_items("harddrugs", 50, 14, 5, 17, 5, false, 0);
            place_items("hospital_samples", 50, 6, 5, 9, 5, false, 0);
            place_items("hospital_samples", 50, 14, 5, 17, 5, false, 0);
            add_spawn("mon_zombie_scientist", rng(1, 6), 11, 12);
            if (one_in(2)) {
                add_spawn("mon_zombie_brute", 1, 16, 17);
            }
        } else if (t_west == "fema_entrance") {
            square(this, t_dirt, 1, 1, 22, 22); //Supply tent
            line_furn(this, f_canvas_wall, 4, 4, 19, 4);
            line_furn(this, f_canvas_wall, 4, 4, 4, 19);
            line_furn(this, f_canvas_wall, 19, 19, 19, 4);
            line_furn(this, f_canvas_wall, 19, 19, 4, 19);
            square_furn(this, f_fema_groundsheet, 5, 5, 8, 18);
            square_furn(this, f_fema_groundsheet, 10, 5, 13, 5);
            square_furn(this, f_fema_groundsheet, 10, 18, 13, 18);
            square_furn(this, f_fema_groundsheet, 15, 5, 18, 7);
            square_furn(this, f_fema_groundsheet, 15, 16, 18, 18);
            square_furn(this, f_fema_groundsheet, 16, 10, 17, 14);
            square_furn(this, f_fema_groundsheet, 9, 7, 14, 16);
            line_furn(this, f_canvas_door, 11, 4, 12, 4);
            line_furn(this, f_canvas_door, 11, 19, 12, 19);
            square_furn(this, f_crate_c, 5, 6, 7, 7);
            square_furn(this, f_crate_c, 5, 11, 7, 12);
            square_furn(this, f_crate_c, 5, 16, 7, 17);
            line(this, t_chainfence_h, 9, 6, 14, 6);
            line(this, t_chainfence_h, 9, 17, 14, 17);
            ter_set(9, 5, t_chaingate_c);
            ter_set(14, 18, t_chaingate_c);
            ter_set(14, 5, t_chainfence_h);
            ter_set(9, 18, t_chainfence_h);
            furn_set(12, 17, f_counter);
            furn_set(11, 6, f_counter);
            line_furn(this, f_chair, 10, 10, 13, 10);
            square_furn(this, f_desk, 10, 11, 13, 12);
            line_furn(this, f_chair, 10, 13, 13, 13);
            line(this, t_chainfence_h, 15, 8, 18, 8);
            line(this, t_chainfence_h, 15, 15, 18, 15);
            line(this, t_chainfence_v, 15, 9, 15, 14);
            line(this, t_chaingate_c, 15, 11, 15, 12);
            line_furn(this, f_locker, 18, 9, 18, 14);
            place_items("allclothes", 90, 5, 6, 7, 7, false, 0);
            place_items("softdrugs", 90, 5, 11, 7, 12, false, 0);
            place_items("hardware", 90, 5, 16, 7, 17, false, 0);
            place_items("mil_rifles", 90, 18, 9, 18, 14, false, 0);
            place_items("office", 80, 10, 11, 13, 12, false, 0);
            add_spawn("mon_zombie_soldier", rng(1, 6), 12, 14);
        } else {
            switch (rng(1, 5)) {
            case 1:
            case 2:
            case 3:
                square(this, t_dirt, 1, 1, 22, 22);
                square_furn(this, f_canvas_wall, 4, 4, 19, 19); //Lodging
                square_furn(this, f_fema_groundsheet, 5, 5, 18, 18);
                line_furn(this, f_canvas_door, 11, 4, 12, 4);
                line_furn(this, f_canvas_door, 11, 19, 12, 19);
                line_furn(this, f_makeshift_bed, 6, 6, 6, 17);
                line_furn(this, f_makeshift_bed, 8, 6, 8, 17);
                line_furn(this, f_makeshift_bed, 10, 6, 10, 17);
                line_furn(this, f_makeshift_bed, 13, 6, 13, 17);
                line_furn(this, f_makeshift_bed, 15, 6, 15, 17);
                line_furn(this, f_makeshift_bed, 17, 6, 17, 17);
                line_furn(this, f_fema_groundsheet, 6, 8, 17, 8);
                line_furn(this, f_fema_groundsheet, 6, 8, 17, 8);
                square_furn(this, f_fema_groundsheet, 6, 11, 17, 12);
                line_furn(this, f_fema_groundsheet, 6, 15, 17, 15);
                line_furn(this, f_crate_o, 6, 7, 17, 7);
                line_furn(this, f_crate_o, 6, 10, 17, 10);
                line_furn(this, f_crate_o, 6, 14, 17, 14);
                line_furn(this, f_crate_o, 6, 17, 17, 17);
                line_furn(this, f_fema_groundsheet, 7, 5, 7, 18);
                line_furn(this, f_fema_groundsheet, 9, 5, 9, 18);
                square_furn(this, f_fema_groundsheet, 11, 5, 12, 18);
                line_furn(this, f_fema_groundsheet, 14, 5, 14, 18);
                line_furn(this, f_fema_groundsheet, 16, 5, 16, 18);
                place_items("livingroom", 80, 5, 5, 18, 18, false, 0);
                add_spawn("mon_zombie", rng(1, 5), 11, 12);
                break;
            case 4:
                square(this, t_dirt, 1, 1, 22, 22);
                square_furn(this, f_canvas_wall, 4, 4, 19, 19); //Mess hall/tent
                square_furn(this, f_fema_groundsheet, 5, 5, 18, 18);
                line_furn(this, f_canvas_door, 11, 4, 12, 4);
                line_furn(this, f_canvas_door, 11, 19, 12, 19);
                line_furn(this, f_crate_c, 5, 5, 5, 6);
                square_furn(this, f_counter, 6, 6, 10, 8);
                square(this, t_rock_floor, 6, 5, 9, 7);
                furn_set(7, 6, f_woodstove);
                line_furn(this, f_bench, 13, 6, 17, 6);
                line_furn(this, f_table, 13, 7, 17, 7);
                line_furn(this, f_bench, 13, 8, 17, 8);

                line_furn(this, f_bench, 13, 11, 17, 11);
                line_furn(this, f_table, 13, 12, 17, 12);
                line_furn(this, f_bench, 13, 13, 17, 13);

                line_furn(this, f_bench, 13, 15, 17, 15);
                line_furn(this, f_table, 13, 16, 17, 16);
                line_furn(this, f_bench, 13, 17, 17, 17);

                line_furn(this, f_bench, 6, 11, 10, 11);
                line_furn(this, f_table, 6, 12, 10, 12);
                line_furn(this, f_bench, 6, 13, 10, 13);

                line_furn(this, f_bench, 6, 15, 10, 15);
                line_furn(this, f_table, 6, 16, 10, 16);
                line_furn(this, f_bench, 6, 17, 10, 17);

                place_items("mil_food_nodrugs", 80, 5, 5, 5, 6, false, 0);
                place_items("snacks", 80, 5, 5, 18, 18, false, 0);
                place_items("kitchen", 70, 6, 5, 10, 8, false, 0);
                place_items("dining", 80, 13, 7, 17, 7, false, 0);
                place_items("dining", 80, 13, 12, 17, 12, false, 0);
                place_items("dining", 80, 13, 16, 17, 16, false, 0);
                place_items("dining", 80, 6, 12, 10, 12, false, 0);
                place_items("dining", 80, 6, 16, 10, 16, false, 0);
                add_spawn("mon_zombie", rng(1, 5), 11, 12);
                break;
            case 5:
                square(this, t_dirtfloor, 1, 1, 22, 22);
                square(this, t_fence_barbed, 4, 4, 19, 19);
                square(this, t_dirt, 5, 5, 18, 18);
                square(this, t_pit_corpsed, 6, 6, 17, 17);
                add_spawn("mon_zombie", rng(5, 20), 11, 12);
                break;
            }
        }

    } else if (terrain_type == "spider_pit_under") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i >= 3 && i <= SEEX * 2 - 4 && j >= 3 && j <= SEEY * 2 - 4) ||
                    one_in(4)) {
                    ter_set(i, j, t_rock_floor);
                    if (!one_in(3)) {
                        add_field(NULL, x, y, fd_web, rng(1, 3));
                    }
                } else {
                    ter_set(i, j, t_rock);
                }
            }
        }
        ter_set(rng(3, SEEX * 2 - 4), rng(3, SEEY * 2 - 4), t_slope_up);
        place_items("spider", 85, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, 0);
        add_spawn("mon_spider_trapdoor", 1, rng(3, SEEX * 2 - 5), rng(3, SEEY * 2 - 4));


    } else if (terrain_type == "anthill") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < 8 || j < 8 || i > SEEX * 2 - 9 || j > SEEY * 2 - 9) {
                    ter_set(i, j, grass_or_dirt());
                } else if ((i == 11 || i == 12) && (j == 11 || j == 12)) {
                    ter_set(i, j, t_slope_down);
                } else {
                    ter_set(i, j, t_dirtmound);
                }
            }
        }


    } else if (terrain_type == "rock") {

        for (int i = 0; i < 4; i++) {
            if (t_nesw[i] == "cavern" || t_nesw[i] == "slimepit" ||
                t_nesw[i] == "slimepit_down") {
                nesw_fac[i] = 6;
            } else {
                nesw_fac[i] = 0;
            }
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (rng(0, n_fac) > j || rng(0, e_fac) > SEEX * 2 - 1 - i ||
                    rng(0, w_fac) > i || rng(0, s_fac) > SEEY * 2 - 1 - j   ) {
                    ter_set(i, j, t_rock_floor);
                } else {
                    ter_set(i, j, t_rock);
                }
            }
        }


    } else if (terrain_type == "rift") {

        if (t_north != "rift" && t_north != "hellmouth") {
            if (connects_to(t_north, 2)) {
                n_fac = rng(-6, -2);
            } else {
                n_fac = rng(2, 6);
            }
        }
        if (t_east != "rift" && t_east != "hellmouth") {
            if (connects_to(t_east, 3)) {
                e_fac = rng(-6, -2);
            } else {
                e_fac = rng(2, 6);
            }
        }
        if (t_south != "rift" && t_south != "hellmouth") {
            if (connects_to(t_south, 0)) {
                s_fac = rng(-6, -2);
            } else {
                s_fac = rng(2, 6);
            }
        }
        if (t_west != "rift" && t_west != "hellmouth") {
            if (connects_to(t_west, 1)) {
                w_fac = rng(-6, -2);
            } else {
                w_fac = rng(2, 6);
            }
        }
        // Negative *_fac values indicate rock floor connection, otherwise solid rock
        // Of course, if we connect to a rift, *_fac = 0, and thus lava extends all the
        //  way.
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((n_fac < 0 && j < n_fac * -1) || (s_fac < 0 && j >= SEEY * 2 - s_fac) ||
                    (w_fac < 0 && i < w_fac * -1) || (e_fac < 0 && i >= SEEX * 2 - e_fac)  ) {
                    ter_set(i, j, t_rock_floor);
                } else if (j < n_fac || j >= SEEY * 2 - s_fac ||
                           i < w_fac || i >= SEEX * 2 - e_fac   ) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_lava);
                }
            }
        }


    } else if (terrain_type == "hellmouth") {

        for (int i = 0; i < 4; i++) {
            if (t_nesw[i] != "rift" && t_nesw[i] != "hellmouth") {
                nesw_fac[i] = 6;
            }
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j < n_fac || j >= SEEY * 2 - s_fac || i < w_fac || i >= SEEX * 2 - e_fac ||
                    (i >= 6 && i < SEEX * 2 - 6 && j >= 6 && j < SEEY * 2 - 6)) {
                    ter_set(i, j, t_rock_floor);
                } else {
                    ter_set(i, j, t_lava);
                }
                if (i >= SEEX - 1 && i <= SEEX && j >= SEEY - 1 && j <= SEEY) {
                    ter_set(i, j, t_slope_down);
                }
            }
        }
        switch (rng(0, 4)) { // Randomly chosen "altar" design
        case 0:
            for (int i = 7; i <= 16; i += 3) {
                ter_set(i, 6, t_rock);
                ter_set(i, 17, t_rock);
                ter_set(6, i, t_rock);
                ter_set(17, i, t_rock);
                if (i > 7 && i < 16) {
                    ter_set(i, 10, t_rock);
                    ter_set(i, 13, t_rock);
                } else {
                    ter_set(i - 1, 6 , t_rock);
                    ter_set(i - 1, 10, t_rock);
                    ter_set(i - 1, 13, t_rock);
                    ter_set(i - 1, 17, t_rock);
                }
            }
            break;
        case 1:
            for (int i = 6; i < 11; i++) {
                ter_set(i, i, t_lava);
                ter_set(SEEX * 2 - 1 - i, i, t_lava);
                ter_set(i, SEEY * 2 - 1 - i, t_lava);
                ter_set(SEEX * 2 - 1 - i, SEEY * 2 - 1 - i, t_lava);
                if (i < 10) {
                    ter_set(i + 1, i, t_lava);
                    ter_set(SEEX * 2 - i, i, t_lava);
                    ter_set(i + 1, SEEY * 2 - 1 - i, t_lava);
                    ter_set(SEEX * 2 - i, SEEY * 2 - 1 - i, t_lava);

                    ter_set(i, i + 1, t_lava);
                    ter_set(SEEX * 2 - 1 - i, i + 1, t_lava);
                    ter_set(i, SEEY * 2 - i, t_lava);
                    ter_set(SEEX * 2 - 1 - i, SEEY * 2 - i, t_lava);
                }
                if (i < 9) {
                    ter_set(i + 2, i, t_rock);
                    ter_set(SEEX * 2 - i + 1, i, t_rock);
                    ter_set(i + 2, SEEY * 2 - 1 - i, t_rock);
                    ter_set(SEEX * 2 - i + 1, SEEY * 2 - 1 - i, t_rock);

                    ter_set(i, i + 2, t_rock);
                    ter_set(SEEX * 2 - 1 - i, i + 2, t_rock);
                    ter_set(i, SEEY * 2 - i + 1, t_rock);
                    ter_set(SEEX * 2 - 1 - i, SEEY * 2 - i + 1, t_rock);
                }
            }
            break;
        case 2:
            for (int i = 7; i < 17; i++) {
                ter_set(i, 6, t_rock);
                ter_set(6, i, t_rock);
                ter_set(i, 17, t_rock);
                ter_set(17, i, t_rock);
                if (i != 7 && i != 16 && i != 11 && i != 12) {
                    ter_set(i, 8, t_rock);
                    ter_set(8, i, t_rock);
                    ter_set(i, 15, t_rock);
                    ter_set(15, i, t_rock);
                }
                if (i == 11 || i == 12) {
                    ter_set(i, 10, t_rock);
                    ter_set(10, i, t_rock);
                    ter_set(i, 13, t_rock);
                    ter_set(13, i, t_rock);
                }
            }
            break;
        case 3:
            for (int i = 6; i < 11; i++) {
                for (int j = 6; j < 11; j++) {
                    ter_set(i, j, t_lava);
                    ter_set(SEEX * 2 - 1 - i, j, t_lava);
                    ter_set(i, SEEY * 2 - 1 - j, t_lava);
                    ter_set(SEEX * 2 - 1 - i, SEEY * 2 - 1 - j, t_lava);
                }
            }
            break;
        }


    } else if (is_ot_type("slimepit", terrain_type)) {

        for (int i = 0; i < 4; i++) {
            nesw_fac[i] = (t_nesw[i] == "slimepit" || t_nesw[i] == "slimepit_down" ? 1 : 0);
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (!one_in(10) && (j < n_fac * SEEX        || i < w_fac * SEEX ||
                                    j > SEEY * 2 - s_fac * SEEY || i > SEEX * 2 - e_fac * SEEX)) {
                    ter_set(i, j, (!one_in(10) ? t_slime : t_rock_floor));
                } else if (rng(0, SEEX) > abs(i - SEEX) && rng(0, SEEY) > abs(j - SEEY)) {
                    ter_set(i, j, t_slime);
                } else if (t_above == "") {
                    ter_set(i, j, t_dirt);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }

        if (terrain_type == "slimepit_down") {
            ter_set(rng(3, SEEX * 2 - 4), rng(3, SEEY * 2 - 4), t_slope_down);
        }

        if (t_above == "slimepit_down") {
            switch (rng(1, 4)) {
            case 1:
                ter_set(rng(0, 2), rng(0, 2), t_slope_up);
            case 2:
                ter_set(rng(0, 2), SEEY * 2 - rng(1, 3), t_slope_up);
            case 3:
                ter_set(SEEX * 2 - rng(1, 3), rng(0, 2), t_slope_up);
            case 4:
                ter_set(SEEX * 2 - rng(1, 3), SEEY * 2 - rng(1, 3), t_slope_up);
            }
        }

        add_spawn("mon_blob", 8, SEEX, SEEY);
        place_items("sewer", 40, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);


    } else if (terrain_type == "triffid_grove") {

        fill_background(this, t_dirt);
        for (int rad = 5; rad < SEEX - 2; rad += rng(2, 3)) {
            square(this, t_tree, rad, rad, 23 - rad, 23 - rad);
            square(this, t_dirt, rad + 1, rad + 1, 22 - rad, 22 - rad);
            if (one_in(2)) { // Vertical side opening
                int x = (one_in(2) ? rad : 23 - rad), y = rng(rad + 1, 22 - rad);
                ter_set(x, y, t_dirt);
            } else { // Horizontal side opening
                int x = rng(rad + 1, 22 - rad), y = (one_in(2) ? rad : 23 - rad);
                ter_set(x, y, t_dirt);
            }
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, rad + 1, rad + 1);
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, 22 - rad, rad + 1);
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, rad + 1, 22 - rad);
            add_spawn( (one_in(3) ? "mon_biollante" : "mon_triffid"), 1, 22 - rad, 22 - rad);
        }
        square(this, t_slope_down, SEEX - 1, SEEY - 1, SEEX, SEEY);


    } else if (terrain_type == "triffid_roots") {

        fill_background(this, t_root_wall);
        int node = 0;
        int step = 0;
        bool node_built[16];
        bool done = false;
        for (int i = 0; i < 16; i++) {
            node_built[i] = false;
        }
        do {
            node_built[node] = true;
            step++;
            int nodex = 1 + 6 * (node % 4), nodey = 1 + 6 * int(node / 4);
            // Clear a 4x4 dirt square
            square(this, t_dirt, nodex, nodey, nodex + 3, nodey + 3);
            // Spawn a monster in there
            if (step > 2) { // First couple of chambers are safe
                int monrng = rng(1, 25);
                int spawnx = nodex + rng(0, 3), spawny = nodey + rng(0, 3);
                if (monrng <= 5) {
                    add_spawn("mon_triffid", rng(1, 4), spawnx, spawny);
                } else if (monrng <= 13) {
                    add_spawn("mon_creeper_hub", 1, spawnx, spawny);
                } else if (monrng <= 19) {
                    add_spawn("mon_biollante", 1, spawnx, spawny);
                } else if (monrng <= 24) {
                    add_spawn("mon_fungal_fighter", 1, spawnx, spawny);
                } else {
                    for (int webx = nodex; webx <= nodex + 3; webx++) {
                        for (int weby = nodey; weby <= nodey + 3; weby++) {
                            add_field(g, webx, weby, fd_web, rng(1, 3));
                        }
                    }
                    add_spawn("mon_spider_web", 1, spawnx, spawny);
                }
            }
            // TODO: Non-monster hazards?
            // Next, pick a cell to move to
            std::vector<direction> move;
            if (node % 4 > 0 && !node_built[node - 1]) {
                move.push_back(WEST);
            }
            if (node % 4 < 3 && !node_built[node + 1]) {
                move.push_back(EAST);
            }
            if (int(node / 4) > 0 && !node_built[node - 4]) {
                move.push_back(NORTH);
            }
            if (int(node / 4) < 3 && !node_built[node + 4]) {
                move.push_back(SOUTH);
            }

            if (move.empty()) { // Nowhere to go!
                square(this, t_slope_down, nodex + 1, nodey + 1, nodex + 2, nodey + 2);
                done = true;
            } else {
                int index = rng(0, move.size() - 1);
                switch (move[index]) {
                case NORTH:
                    square(this, t_dirt, nodex + 1, nodey - 2, nodex + 2, nodey - 1);
                    node -= 4;
                    break;
                case EAST:
                    square(this, t_dirt, nodex + 4, nodey + 1, nodex + 5, nodey + 2);
                    node++;
                    break;
                case SOUTH:
                    square(this, t_dirt, nodex + 1, nodey + 4, nodex + 2, nodey + 5);
                    node += 4;
                    break;
                case WEST:
                    square(this, t_dirt, nodex - 2, nodey + 1, nodex - 1, nodey + 2);
                    node--;
                    break;
                default:
                    break;
                }
            }
        } while (!done);
        square(this, t_slope_up, 2, 2, 3, 3);
        rotate(rng(0, 3));


    } else if (terrain_type == "triffid_finale") {

        fill_background(this, t_root_wall);
        square(this, t_dirt, 1, 1, 4, 4);
        square(this, t_dirt, 19, 19, 22, 22);
        // Drunken walk until we reach the heart (lower right, [19, 19])
        // Chance increases by 1 each turn, and gives the % chance of forcing a move
        // to the right or down.
        int chance = 0;
        int x = 4, y = 4;
        do {
            ter_set(x, y, t_dirt);

            if (chance >= 10 && one_in(10)) { // Add a spawn
                if (one_in(2)) {
                    add_spawn("mon_biollante", 1, x, y);
                } else if (!one_in(4)) {
                    add_spawn("mon_creeper_hub", 1, x, y);
                } else {
                    add_spawn("mon_triffid", 1, x, y);
                }
            }

            if (rng(0, 99) < chance) { // Force movement down or to the right
                if (x >= 19) {
                    y++;
                } else if (y >= 19) {
                    x++;
                } else {
                    if (one_in(2)) {
                        x++;
                    } else {
                        y++;
                    }
                }
            } else {
                chance++; // Increase chance of forced movement down/right
                // Weigh movement towards directions with lots of existing walls
                int chance_west = 0, chance_east = 0, chance_north = 0, chance_south = 0;
                for (int dist = 1; dist <= 5; dist++) {
                    if (ter(x - dist, y) == t_root_wall) {
                        chance_west++;
                    }
                    if (ter(x + dist, y) == t_root_wall) {
                        chance_east++;
                    }
                    if (ter(x, y - dist) == t_root_wall) {
                        chance_north++;
                    }
                    if (ter(x, y + dist) == t_root_wall) {
                        chance_south++;
                    }
                }
                int roll = rng(0, chance_west + chance_east + chance_north + chance_south);
                if (roll < chance_west && x > 0) {
                    x--;
                } else if (roll < chance_west + chance_east && x < 23) {
                    x++;
                } else if (roll < chance_west + chance_east + chance_north && y > 0) {
                    y--;
                } else if (y < 23) {
                    y++;
                }
            } // Done with drunken walk
        } while (x < 19 || y < 19);
        square(this, t_slope_up, 1, 1, 2, 2);
        add_spawn("mon_triffid_heart", 1, 21, 21);


    } else if (terrain_type == "basement") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i == 0 || j == 0 || i == SEEX * 2 - 1 || j == SEEY * 2 - 1) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        ter_set(SEEX - 1, SEEY * 2 - 2, t_stairs_up);
        ter_set(SEEX    , SEEY * 2 - 2, t_stairs_up);
        line(this, t_rock, SEEX - 2, SEEY * 2 - 4, SEEX - 2, SEEY * 2 - 2);
        line(this, t_rock, SEEX + 1, SEEY * 2 - 4, SEEX + 1, SEEY * 2 - 2);
        line(this, t_door_locked, SEEX - 1, SEEY * 2 - 4, SEEX, SEEY * 2 - 4);
        switch (rng(0, 4)) { // TODO: More types!

        case 0: // Junk!
            place_items("bedroom", 60, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2, false, 0);
            place_items("home_hw", 80, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2, false, 0);
            place_items("homeguns", 10, 1, 1, SEEX * 2 - 2, SEEY * 2 - 2, false, 0);
            break;

        case 1: // Weapons cache
            for (int i = 2; i < SEEX * 2 - 2; i++) {
                furn_set(i, 1, f_rack);
                furn_set(i, 5, f_rack);
                furn_set(i, 9, f_rack);
            }
            place_items("allguns", 80, 2, 1, SEEX * 2 - 3, 1, false, 0);
            place_items("ammo",    94, 2, 5, SEEX * 2 - 3, 5, false, 0);
            place_items("gunxtras", 88, 2, 9, SEEX * 2 - 7, 9, false, 0);
            place_items("weapons", 88, SEEX * 2 - 6, 9, SEEX * 2 - 3, 9, false, 0);
            break;

        case 2: // Survival Bunker
            furn_set(1, 1, f_bed);
            furn_set(1, 2, f_bed);
            furn_set(SEEX * 2 - 2, 1, f_bed);
            furn_set(SEEX * 2 - 2, 2, f_bed);
            for (int i = 1; i < SEEY; i++) {
                furn_set(SEEX - 1, i, f_rack);
                furn_set(SEEX    , i, f_rack);
            }
            place_items("softdrugs",  86, SEEX - 1,  1, SEEX,  2, false, 0);
            place_items("cannedfood",  92, SEEX - 1,  3, SEEX,  6, false, 0);
            place_items("homeguns",  72, SEEX - 1,  7, SEEX,  7, false, 0);
            place_items("survival_tools", 83, SEEX - 1,  8, SEEX, 10, false, 0);
            place_items("manuals",  60, SEEX - 1, 11, SEEX, 11, false, 0);
            break;

        case 3: // Chem lab
            for (int i = 1; i < SEEY + 4; i++) {
                furn_set(1           , i, f_counter);
                furn_set(SEEX * 2 - 2, i, f_counter);
            }
            place_items("chem_home", 90,        1, 1,        1, SEEY + 3, false, 0);
            if (one_in(3)) {
                place_items("chem_home", 90, SEEX * 2 - 2, 1, SEEX * 2 - 2, SEEY + 3, false, 0);
            } else {
                place_items("electronics", 90, SEEX * 2 - 2, 1, SEEX * 2 - 2, SEEY + 3, false, 0);
            }
            break;

        case 4: // Weed grow
            line_furn(this, f_counter, 1, 1, 1, SEEY * 2 - 2);
            line_furn(this, f_counter, SEEX * 2 - 2, 1, SEEX * 2 - 2, SEEY * 2 - 2);
            for (int i = 3; i < SEEX * 2 - 3; i += 5) {
                for (int j = 3; j < 16; j += 5) {
                    square(this, t_dirt, i, j, i + 2, j + 2);
                    int num_weed = rng(0, 4) * rng(0, 1);
                    for (int n = 0; n < num_weed; n++) {
                        int x = rng(i, i + 2), y = rng(j, j + 2);
                        spawn_item(x, y, one_in(5) ? "seed_weed" : "weed");
                    }
                }
            }
            break;
        }
        // Chance of zombies in the basement, only appear north of the anteroom the stairs are in.
        place_spawns(g, "GROUP_ZOMBIE", 2, 1, 1, SEEX * 2 - 1, SEEX * 2 - 5, density);


        // TODO: Maybe subway systems could have broken down trains in them?
    } else if (terrain_type == "subway_station") {

        if (is_ot_type("subway", t_north) && connects_to(t_north, 2)) {
            n_fac = 1;
        }
        if (is_ot_type("subway", t_east) && connects_to(t_east, 3)) {
            e_fac = 1;
        }
        if (is_ot_type("subway", t_south) && connects_to(t_south, 0)) {
            s_fac = 1;
        }
        if (is_ot_type("subway", t_west) && connects_to(t_west, 1)) {
            w_fac = 1;
        }

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i < 4 && (w_fac == 0 || j < 4 || j > SEEY * 2 - 5)) ||
                    (j < 4 && (n_fac == 0 || i < 4 || i > SEEX * 2 - 5)) ||
                    (i > SEEX * 2 - 5 && (e_fac == 0 || j < 4 || j > SEEY * 2 - 5)) ||
                    (j > SEEY * 2 - 5 && (s_fac == 0 || i < 4 || i > SEEX * 2 - 5))) {
                    ter_set(i, j, t_rock_floor);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        ter_set(2,            2           , t_stairs_up);
        ter_set(SEEX * 2 - 3, 2           , t_stairs_up);
        ter_set(2,            SEEY * 2 - 3, t_stairs_up);
        ter_set(SEEX * 2 - 3, SEEY * 2 - 3, t_stairs_up);
        if (ter(2, SEEY) == t_floor) {
            ter_set(2, SEEY, t_stairs_up);
        }
        if (ter(SEEX * 2 - 3, SEEY) == t_floor) {
            ter_set(SEEX * 2 - 3, SEEY, t_stairs_up);
        }
        if (ter(SEEX, 2) == t_floor) {
            ter_set(SEEX, 2, t_stairs_up);
        }
        if (ter(SEEX, SEEY * 2 - 3) == t_floor) {
            ter_set(SEEX, SEEY * 2 - 3, t_stairs_up);
        }


    } else if (terrain_type == "subway_ns" ||
               terrain_type == "subway_ew") {

        if (terrain_type == "subway_ns") {
            w_fac = (t_west  == "cavern" ? 0 : 4);
            e_fac = (t_east  == "cavern" ? SEEX * 2 : SEEX * 2 - 5);
        } else {
            w_fac = (t_north == "cavern" ? 0 : 4);
            e_fac = (t_south == "cavern" ? SEEX * 2 : SEEX * 2 - 5);
        }
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < w_fac || i > e_fac) {
                    ter_set(i, j, t_rock);
                } else if (one_in(90)) {
                    ter_set(i, j, t_rubble);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (is_ot_type("sub_station", t_above)) {
            ter_set(SEEX * 2 - 5, rng(SEEY - 5, SEEY + 4), t_stairs_up);
        }
        place_items("subway", 30, 4, 0, SEEX * 2 - 5, SEEY * 2 - 1, true, 0);
        if (terrain_type == "subway_ew") {
            rotate(1);
        }


    } else if (terrain_type == "subway_ne" ||
               terrain_type == "subway_es" ||
               terrain_type == "subway_sw" ||
               terrain_type == "subway_wn") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i >= SEEX * 2 - 4 && j < 4) || i < 4 || j >= SEEY * 2 - 4) {
                    ter_set(i, j, t_rock);
                } else if (one_in(30)) {
                    ter_set(i, j, t_rubble);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (t_above >= "sub_station_north" && t_above <= "sub_station_west") {
            ter_set(SEEX * 2 - 5, rng(SEEY - 5, SEEY + 4), t_stairs_up);
        }
        place_items("subway", 30, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "subway_es") {
            rotate(1);
        }
        if (terrain_type == "subway_sw") {
            rotate(2);
        }
        if (terrain_type == "subway_wn") {
            rotate(3);
        }


    } else if (terrain_type == "subway_nes" ||
               terrain_type == "subway_new" ||
               terrain_type == "subway_nsw" ||
               terrain_type == "subway_esw") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < 4 || (i >= SEEX * 2 - 4 && (j < 4 || j >= SEEY * 2 - 4))) {
                    ter_set(i, j, t_rock);
                } else if (one_in(30)) {
                    ter_set(i, j, t_rubble);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (t_above >= "sub_station_north" && t_above <= "sub_station_west") {
            ter_set(4, rng(SEEY - 5, SEEY + 4), t_stairs_up);
        }
        place_items("subway", 35, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "subway_esw") {
            rotate(1);
        }
        if (terrain_type == "subway_nsw") {
            rotate(2);
        }
        if (terrain_type == "subway_new") {
            rotate(3);
        }


    } else if (terrain_type == "subway_nesw") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i < 4 || i >= SEEX * 2 - 4) &&
                    (j < 4 || j >= SEEY * 2 - 4)) {
                    ter_set(i, j, t_rock);
                } else if (one_in(30)) {
                    ter_set(i, j, t_rubble);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }

        if (is_ot_type("sub_station", t_above)) {
            ter_set(4 + rng(0, 1) * (SEEX * 2 - 9), 4 + rng(0, 1) * (SEEY * 2 - 9), t_stairs_up);
        }
        place_items("subway", 40, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);


    } else if (terrain_type == "sewer_ns" ||
               terrain_type == "sewer_ew") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < SEEX - 2 || i > SEEX + 1) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_sewage);
                }
            }
        }
        place_items("sewer", 10, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "sewer_ew") {
            rotate(1);
        }


    } else if (terrain_type == "sewer_ne" ||
               terrain_type == "sewer_es" ||
               terrain_type == "sewer_sw" ||
               terrain_type == "sewer_wn") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i > SEEX + 1 && j < SEEY - 2) || i < SEEX - 2 || j > SEEY + 1) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_sewage);
                }
            }
        }
        place_items("sewer", 18, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "sewer_es") {
            rotate(1);
        }
        if (terrain_type == "sewer_sw") {
            rotate(2);
        }
        if (terrain_type == "sewer_wn") {
            rotate(3);
        }


    } else if (terrain_type == "sewer_nes" ||
               terrain_type == "sewer_new" ||
               terrain_type == "sewer_nsw" ||
               terrain_type == "sewer_esw") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < SEEX - 2 || (i > SEEX + 1 && (j < SEEY - 2 || j > SEEY + 1))) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_sewage);
                }
            }
        }
        place_items("sewer", 23, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        if (terrain_type == "sewer_esw") {
            rotate(1);
        }
        if (terrain_type == "sewer_nsw") {
            rotate(2);
        }
        if (terrain_type == "sewer_new") {
            rotate(3);
        }


    } else if (terrain_type == "sewer_nesw") {

        rn = rng(0, 3);
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i < SEEX - 2 || i > SEEX + 1) && (j < SEEY - 2 || j > SEEY + 1)) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_sewage);
                }
                if (rn == 0 && (trig_dist(i, j, SEEX - 1, SEEY - 1) <= 6 ||
                                trig_dist(i, j, SEEX - 1, SEEY    ) <= 6 ||
                                trig_dist(i, j, SEEX,     SEEY - 1) <= 6 ||
                                trig_dist(i, j, SEEX,     SEEY    ) <= 6   )) {
                    ter_set(i, j, t_sewage);
                }
                if (rn == 0 && (i == SEEX - 1 || i == SEEX) && (j == SEEY - 1 || j == SEEY)) {
                    ter_set(i, j, t_grate);
                }
            }
        }
        place_items("sewer", 28, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);


    } else if (terrain_type == "ants_ns" ||
               terrain_type == "ants_ew") {

        x = SEEX;
        fill_background(this, t_rock);
        for (int j = 0; j < SEEY * 2; j++) {
            for (int i = x - 2; i <= x + 3; i++) {
                if (i >= 1 && i < SEEX * 2 - 1) {
                    ter_set(i, j, t_rock_floor);
                }
            }
            x += rng(-1, 1);
            while (abs(SEEX - x) > SEEX * 2 - j - 1) {
                if (x < SEEX) {
                    x++;
                }
                if (x > SEEX) {
                    x--;
                }
            }
        }
        if (terrain_type == "ants_ew") {
            rotate(1);
        }


    } else if (terrain_type == "ants_ne" ||
               terrain_type == "ants_es" ||
               terrain_type == "ants_sw" ||
               terrain_type == "ants_wn") {

        x = SEEX;
        y = 1;
        rn = 0;
        // First, set it all to rock
        fill_background(this, t_rock);

        for (int i = SEEX - 2; i <= SEEX + 3; i++) {
            ter_set(i, 0, t_rock_floor);
            ter_set(i, 1, t_rock_floor);
            ter_set(i, 2, t_rock_floor);
            ter_set(SEEX * 2 - 1, i, t_rock_floor);
            ter_set(SEEX * 2 - 2, i, t_rock_floor);
            ter_set(SEEX * 2 - 3, i, t_rock_floor);
        }
        do {
            for (int i = x - 2; i <= x + 3; i++) {
                for (int j = y - 2; j <= y + 3; j++) {
                    if (i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1) {
                        ter_set(i, j, t_rock_floor);
                    }
                }
            }
            if (rn < SEEX) {
                x += rng(-1, 1);
                y++;
            } else {
                x++;
                if (!one_in(x - SEEX)) {
                    y += rng(-1, 1);
                } else if (y < SEEY) {
                    y++;
                } else if (y > SEEY) {
                    y--;
                }
            }
            rn++;
        } while (x < SEEX * 2 - 1 || y != SEEY);
        for (int i = x - 2; i <= x + 3; i++) {
            for (int j = y - 2; j <= y + 3; j++) {
                if (i > 0 && i < SEEX * 2 - 1 && j > 0 && j < SEEY * 2 - 1) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (terrain_type == "ants_es") {
            rotate(1);
        }
        if (terrain_type == "ants_sw") {
            rotate(2);
        }
        if (terrain_type == "ants_wn") {
            rotate(3);
        }


    } else if (terrain_type == "ants_nes" ||
               terrain_type == "ants_new" ||
               terrain_type == "ants_nsw" ||
               terrain_type == "ants_esw") {

        fill_background(this, t_rock);
        x = SEEX;
        for (int j = 0; j < SEEY * 2; j++) {
            for (int i = x - 2; i <= x + 3; i++) {
                if (i >= 1 && i < SEEX * 2 - 1) {
                    ter_set(i, j, t_rock_floor);
                }
            }
            x += rng(-1, 1);
            while (abs(SEEX - x) > SEEY * 2 - j - 1) {
                if (x < SEEX) {
                    x++;
                }
                if (x > SEEX) {
                    x--;
                }
            }
        }
        y = SEEY;
        for (int i = SEEX; i < SEEX * 2; i++) {
            for (int j = y - 2; j <= y + 3; j++) {
                if (j >= 1 && j < SEEY * 2 - 1) {
                    ter_set(i, j, t_rock_floor);
                }
            }
            y += rng(-1, 1);
            while (abs(SEEY - y) > SEEX * 2 - 1 - i) {
                if (y < SEEY) {
                    y++;
                }
                if (y > SEEY) {
                    y--;
                }
            }
        }
        if (terrain_type == "ants_new") {
            rotate(3);
        }
        if (terrain_type == "ants_nsw") {
            rotate(2);
        }
        if (terrain_type == "ants_esw") {
            rotate(1);
        }


    } else if (terrain_type == "ants_nesw") {

        fill_background(this, t_rock);
        x = SEEX;
        for (int j = 0; j < SEEY * 2; j++) {
            for (int i = x - 2; i <= x + 3; i++) {
                if (i >= 1 && i < SEEX * 2 - 1) {
                    ter_set(i, j, t_rock_floor);
                }
            }
            x += rng(-1, 1);
            while (abs(SEEX - x) > SEEY * 2 - j - 1) {
                if (x < SEEX) {
                    x++;
                }
                if (x > SEEX) {
                    x--;
                }
            }
        }

        y = SEEY;
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = y - 2; j <= y + 3; j++) {
                if (j >= 1 && j < SEEY * 2 - 1) {
                    ter_set(i, j, t_rock_floor);
                }
            }
            y += rng(-1, 1);
            while (abs(SEEY - y) > SEEX * 2 - i - 1) {
                if (y < SEEY) {
                    y++;
                }
                if (y > SEEY) {
                    y--;
                }
            }
        }


    } else if (terrain_type == "ants_food" ||
               terrain_type == "ants_larvae" ||
               terrain_type == "ants_queen") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (i < SEEX - 4 || i > SEEX + 5 || j < SEEY - 4 || j > SEEY + 5) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        rn = rng(10, 20);
        for (int n = 0; n < rn; n++) {
            cw = rng(1, 8);
            do {
                x = rng(1 + cw, SEEX * 2 - 2 - cw);
                y = rng(1 + cw, SEEY * 2 - 2 - cw);
            } while (ter(x, y) == t_rock);
            for (int i = x - cw; i <= x + cw; i++) {
                for (int j = y - cw; j <= y + cw; j++) {
                    if (trig_dist(x, y, i, j) <= cw) {
                        ter_set(i, j, t_rock_floor);
                    }
                }
            }
        }
        if (connects_to(t_north, 2)) {
            for (int i = SEEX - 2; i <= SEEX + 3; i++) {
                for (int j = 0; j <= SEEY; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (connects_to(t_east, 3)) {
            for (int i = SEEX; i <= SEEX * 2 - 1; i++) {
                for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (connects_to(t_south, 0)) {
            for (int i = SEEX - 2; i <= SEEX + 3; i++) {
                for (int j = SEEY; j <= SEEY * 2 - 1; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (connects_to(t_west, 1)) {
            for (int i = 0; i <= SEEX; i++) {
                for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (terrain_type == "ants_food") {
            place_items("ant_food", 92, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        } else {
            place_items("ant_egg",  98, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        }
        if (terrain_type == "ants_queen") {
            add_spawn("mon_ant_queen", 1, SEEX, SEEY);
        } else if (terrain_type == "ants_larvae") {
            add_spawn("mon_ant_larva", 10, SEEX, SEEY);
        }


    } else if (terrain_type == "tutorial") {

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (j == 0 || j == SEEY * 2 - 1) {
                    ter_set(i, j, t_wall_h);
                } else if (i == 0 || i == SEEX * 2 - 1) {
                    ter_set(i, j, t_wall_v);
                } else if (j == SEEY) {
                    if (i % 4 == 2) {
                        ter_set(i, j, t_door_c);
                    } else if (i % 5 == 3) {
                        ter_set(i, j, t_window_domestic);
                    } else {
                        ter_set(i, j, t_wall_h);
                    }
                } else {
                    ter_set(i, j, t_floor);
                }
            }
        }
        furn_set(7, SEEY * 2 - 4, f_rack);
        place_gas_pump(SEEX * 2 - 2, SEEY * 2 - 4, rng(500, 1000));
        if (t_above != "") {
            ter_set(SEEX - 2, SEEY + 2, t_stairs_up);
            ter_set(2, 2, t_water_sh);
            ter_set(2, 3, t_water_sh);
            ter_set(3, 2, t_water_sh);
            ter_set(3, 3, t_water_sh);
        } else {
            spawn_item(           5, SEEY + 1, "helmet_bike");
            spawn_item(           4, SEEY + 1, "backpack");
            spawn_item(           3, SEEY + 1, "pants_cargo");
            spawn_item(           7, SEEY * 2 - 4, "machete");
            spawn_item(           7, SEEY * 2 - 4, "9mm");
            spawn_item(           7, SEEY * 2 - 4, "9mmP");
            spawn_item(           7, SEEY * 2 - 4, "uzi");
            spawn_item(SEEX * 2 - 2, SEEY + 5, "bubblewrap");
            spawn_item(SEEX * 2 - 2, SEEY + 6, "grenade");
            spawn_item(SEEX * 2 - 3, SEEY + 6, "flashlight");
            spawn_item(SEEX * 2 - 2, SEEY + 7, "cig");
            spawn_item(SEEX * 2 - 2, SEEY + 7, "codeine");
            spawn_item(SEEX * 2 - 3, SEEY + 7, "water");
            ter_set(SEEX - 2, SEEY + 2, t_stairs_down);
        }


    } else if (terrain_type == "cavern") {

        for (int i = 0; i < 4; i++) {
            nesw_fac[i] = (t_nesw[i] == "cavern" || t_nesw[i] == "subway_ns" ||
                           t_nesw[i] == "subway_ew" ? 0 : 3);
        }
        e_fac = SEEX * 2 - 1 - e_fac;
        s_fac = SEEY * 2 - 1 - s_fac;

        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((j < n_fac || j > s_fac || i < w_fac || i > e_fac) &&
                    (!one_in(3) || j == 0 || j == SEEY * 2 - 1 || i == 0 || i == SEEX * 2 - 1)) {
                    ter_set(i, j, t_rock);
                } else {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }

        rn = rng(0, 2) * rng(0, 3) + rng(0, 1); // Number of pillars
        for (int n = 0; n < rn; n++) {
            int px = rng(5, SEEX * 2 - 6);
            int py = rng(5, SEEY * 2 - 6);
            for (int i = px - 1; i <= px + 1; i++) {
                for (int j = py - 1; j <= py + 1; j++) {
                    ter_set(i, j, t_rock);
                }
            }
        }

        if (connects_to(t_north, 2)) {
            for (int i = SEEX - 2; i <= SEEX + 3; i++) {
                for (int j = 0; j <= SEEY; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (connects_to(t_east, 3)) {
            for (int i = SEEX; i <= SEEX * 2 - 1; i++) {
                for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (connects_to(t_south, 0)) {
            for (int i = SEEX - 2; i <= SEEX + 3; i++) {
                for (int j = SEEY; j <= SEEY * 2 - 1; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        if (connects_to(t_west, 1)) {
            for (int i = 0; i <= SEEX; i++) {
                for (int j = SEEY - 2; j <= SEEY + 3; j++) {
                    ter_set(i, j, t_rock_floor);
                }
            }
        }
        place_items("cavern", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, 0);
        if (one_in(6)) { // Miner remains
            int x, y;
            do {
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
            } while (move_cost(x, y) == 0);
            if (!one_in(3)) {
                spawn_item(x, y, "jackhammer");
            }
            if (one_in(3)) {
                spawn_item(x, y, "mask_dust");
            }
            if (one_in(2)) {
                spawn_item(x, y, "hat_hard");
            }
            while (!one_in(3)) {
                put_items_from("cannedfood", 3, x, y, 0, 0, 0);
            }
        }


    } else {
        // not one of the hardcoded ones!
        // load from JSON???
        debugmsg("Error: tried to generate map for omtype %s, \"%s\"",
                 terrain_type.c_str(), otermap[terrain_type].name.c_str());
        fill_background(this, t_floor);

    }}
    // THE END OF THE HUGE IF ELIF ELIF ELIF ELI FLIE FLIE FLIE FEL OMFG

    // WTF it is still going?...


    // Now, fix sewers and subways so that they interconnect.

    if (is_ot_type("subway", terrain_type)) {
        if (is_ot_type("sewer", t_north) &&
            !connects_to(terrain_type, 0)) {
            if (connects_to(t_north, 2)) {
                for (int i = SEEX - 2; i < SEEX + 2; i++) {
                    for (int j = 0; j < SEEY; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int j = 0; j < 3; j++) {
                    ter_set(SEEX, j, t_rock_floor);
                    ter_set(SEEX - 1, j, t_rock_floor);
                }
                ter_set(SEEX, 3, t_door_metal_c);
                ter_set(SEEX - 1, 3, t_door_metal_c);
            }
        }
        if (is_ot_type("sewer", t_east) &&
            !connects_to(terrain_type, 1)) {
            if (connects_to(t_east, 3)) {
                for (int i = SEEX; i < SEEX * 2; i++) {
                    for (int j = SEEY - 2; j < SEEY + 2; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int i = SEEX * 2 - 3; i < SEEX * 2; i++) {
                    ter_set(i, SEEY, t_rock_floor);
                    ter_set(i, SEEY - 1, t_rock_floor);
                }
                ter_set(SEEX * 2 - 4, SEEY, t_door_metal_c);
                ter_set(SEEX * 2 - 4, SEEY - 1, t_door_metal_c);
            }
        }
        if (is_ot_type("sewer", t_south) &&
            !connects_to(terrain_type, 2)) {
            if (connects_to(t_south, 0)) {
                for (int i = SEEX - 2; i < SEEX + 2; i++) {
                    for (int j = SEEY; j < SEEY * 2; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int j = SEEY * 2 - 3; j < SEEY * 2; j++) {
                    ter_set(SEEX, j, t_rock_floor);
                    ter_set(SEEX - 1, j, t_rock_floor);
                }
                ter_set(SEEX, SEEY * 2 - 4, t_door_metal_c);
                ter_set(SEEX - 1, SEEY * 2 - 4, t_door_metal_c);
            }
        }
        if (is_ot_type("sewer", t_west) &&
            !connects_to(terrain_type, 3)) {
            if (connects_to(t_west, 1)) {
                for (int i = 0; i < SEEX; i++) {
                    for (int j = SEEY - 2; j < SEEY + 2; j++) {
                        ter_set(i, j, t_sewage);
                    }
                }
            } else {
                for (int i = 0; i < 3; i++) {
                    ter_set(i, SEEY, t_rock_floor);
                    ter_set(i, SEEY - 1, t_rock_floor);
                }
                ter_set(3, SEEY, t_door_metal_c);
                ter_set(3, SEEY - 1, t_door_metal_c);
            }
        }
    } else if (is_ot_type("sewer", terrain_type)) {
        if (t_above == "road_nesw_manhole") {
            ter_set(rng(SEEX - 2, SEEX + 1), rng(SEEY - 2, SEEY + 1), t_ladder_up);
        }
        if (is_ot_type("subway", t_north) &&
            !connects_to(terrain_type, 0)) {
            for (int j = 0; j < SEEY - 3; j++) {
                ter_set(SEEX, j, t_rock_floor);
                ter_set(SEEX - 1, j, t_rock_floor);
            }
            ter_set(SEEX, SEEY - 3, t_door_metal_c);
            ter_set(SEEX - 1, SEEY - 3, t_door_metal_c);
        }
        if (is_ot_type("subway", t_east) &&
            !connects_to(terrain_type, 1)) {
            for (int i = SEEX + 3; i < SEEX * 2; i++) {
                ter_set(i, SEEY, t_rock_floor);
                ter_set(i, SEEY - 1, t_rock_floor);
            }
            ter_set(SEEX + 2, SEEY, t_door_metal_c);
            ter_set(SEEX + 2, SEEY - 1, t_door_metal_c);
        }
        if (is_ot_type("subway", t_south) &&
            !connects_to(terrain_type, 2)) {
            for (int j = SEEY + 3; j < SEEY * 2; j++) {
                ter_set(SEEX, j, t_rock_floor);
                ter_set(SEEX - 1, j, t_rock_floor);
            }
            ter_set(SEEX, SEEY + 2, t_door_metal_c);
            ter_set(SEEX - 1, SEEY + 2, t_door_metal_c);
        }
        if (is_ot_type("subway", t_west) &&
            !connects_to(terrain_type, 3)) {
            for (int i = 0; i < SEEX - 3; i++) {
                ter_set(i, SEEY, t_rock_floor);
                ter_set(i, SEEY - 1, t_rock_floor);
            }
            ter_set(SEEX - 3, SEEY, t_door_metal_c);
            ter_set(SEEX - 3, SEEY - 1, t_door_metal_c);
        }
    } else if (is_ot_type("ants", terrain_type)) {
        if (t_above == "anthill") {
            bool done = false;
            do {
                int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
                if (ter(x, y) == t_rock_floor) {
                    done = true;
                    ter_set(x, y, t_slope_up);
                }
            } while (!done);
        }
    }

}

void map::post_process(game *g, unsigned zones)
{
    std::string junk;
    if (zones & mfb(OMZONE_CITY)) {
        if (!one_in(10)) { // 90% chance of smashing stuff up
            for (int x = 0; x < 24; x++) {
                for (int y = 0; y < 24; y++) {
                    bash(x, y, 20, junk);
                }
            }
        }
        if (one_in(10)) { // 10% chance of corpses
            int num_corpses = rng(1, 8);
            for (int i = 0; i < num_corpses; i++) {
                int x = rng(0, 23), y = rng(0, 23);
                if (move_cost(x, y) > 0) {
                    add_corpse(g, this, x, y);
                }
            }
        }
    } // OMZONE_CITY

    if (zones & mfb(OMZONE_BOMBED)) {
        while (one_in(4)) {
            point center( rng(4, 19), rng(4, 19) );
            int radius = rng(1, 4);
            for (int x = center.x - radius; x <= center.x + radius; x++) {
                for (int y = center.y - radius; y <= center.y + radius; y++) {
                    if (rl_dist(x, y, center.x, center.y) <= rng(1, radius)) {
                        destroy(g, x, y, false);
                    }
                }
            }
        }
    }

}

void map::place_spawns(game *g, std::string group, const int chance,
                       const int x1, const int y1, const int x2, const int y2, const float density)
{
    if (!ACTIVE_WORLD_OPTIONS["STATIC_SPAWN"]) {
        return;
    }

    if ( MonsterGroupManager::isValidMonsterGroup( group ) == false ) {
        real_coords rc( this->getabs() );
        overmap * thisom = &overmap_buffer.get(g, rc.abs_om.x, rc.abs_om.y );
        oter_id oid = thisom->ter( rc.om_pos.x, rc.om_pos.y, world_z );
        debugmsg("place_spawns: invalid mongroup '%s', om_terrain = '%s' (%s)", group.c_str(), oid.t().id.c_str(), oid.t().id_base.c_str() );
        return;
    }

    float multiplier = ACTIVE_WORLD_OPTIONS["SPAWN_DENSITY"];

    if( multiplier == 0.0 ) {
        return;
    }

    if (one_in(chance / multiplier)) {
        int num = density * (float)rng(10, 50) * multiplier;

        for (int i = 0; i < num; i++) {
            int tries = 10;
            int x = 0;
            int y = 0;

            // Pick a spot for the spawn
            do {
                x = rng(x1, x2);
                y = rng(y1, y2);
                tries--;
            } while( move_cost(x, y) == 0 && tries );

            // Pick a monster type
            MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( group, &num );

            add_spawn(spawn_details.name, spawn_details.pack_size, x, y);
        }
    }
}

void map::place_gas_pump(int x, int y, int charges)
{
    item gas(itypes["gasoline"], 0);
    gas.charges = charges;
    add_item(x, y, gas);
    ter_set(x, y, t_gas_pump);
}

void map::place_toilet(int x, int y, int charges)
{
    item water(itypes["water"], 0);
    water.charges = charges;
    add_item(x, y, water);
    furn_set(x, y, f_toilet);
}

int map::place_items(items_location loc, int chance, int x1, int y1,
                     int x2, int y2, bool ongrass, int turn)
{
int lets_spawn = 100 * ACTIVE_WORLD_OPTIONS["ITEM_SPAWNRATE"];
    
    if (chance >= 100 || chance <= 0) {
        debugmsg("map::place_items() called with an invalid chance (%d)", chance);
        return 0;
    }

    Item_tag selected_item;
    bool guns_have_ammo = false;
    selected_item = item_controller->id_from(loc, guns_have_ammo);
    if ( selected_item == "MISSING_ITEM" ) {
        real_coords rc( this->getabs() );
        overmap * thisom = &overmap_buffer.get(g, rc.abs_om.x, rc.abs_om.y );
        oter_id oid = thisom->ter( rc.om_pos.x, rc.om_pos.y, world_z );
        debugmsg("place_items: invalid item group '%s', om_terrain = '%s' (%s)", loc.c_str(), oid.t().id.c_str(), oid.t().id_base.c_str() );
    }
    int px, py;
    int item_num = 0;
    while (rng(0, 99) < chance) {
    
    if (rng(1,100) > lets_spawn) {
    continue;
    } 
    
        selected_item = item_controller->id_from(loc);
        int tries = 0;
        do {
            px = rng(x1, x2);
            py = rng(y1, y2);
            tries++;
            // Only place on valid terrain
        } while (((terlist[ter(px, py)].movecost == 0 &&
                   !(terlist[ter(px, py)].has_flag("PLACE_ITEM"))) ||
                  (!ongrass && !terlist[ter(px, py)].has_flag("FLAT") )) &&
                 tries < 20);
        if (tries < 20) {
            spawn_item(px, py, selected_item, 1, 0, turn);
            item_num++;
            // Guns in item groups with guns_have_ammo flags are generated with their ammo
            if ( guns_have_ammo ) {
                it_gun *maybe_gun = static_cast<it_gun *> (item_controller->find_template(selected_item));
                if ( maybe_gun != NULL ) {
                    spawn_item(px, py, default_ammo(maybe_gun->ammo), 1, 0, turn);
                }
            }
        }
    }
    return item_num;
}

void map::put_items_from(items_location loc, int num, int x, int y, int turn, int quantity,
                         int charges, int damlevel)
{
    for (int i = 0; i < num; i++) {
        Item_tag selected_item = item_controller->id_from(loc);
        spawn_item(x, y, selected_item, quantity, charges, turn, damlevel);
    }
}

void map::add_spawn(std::string type, int count, int x, int y, bool friendly,
                    int faction_id, int mission_id, std::string name)
{
    if (x < 0 || x >= SEEX * my_MAPSIZE || y < 0 || y >= SEEY * my_MAPSIZE) {
        debugmsg("Bad add_spawn(%s, %d, %d, %d)", type.c_str(), count, x, y);
        return;
    }
    int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;
    if(!grid[nonant]) {
        debugmsg("centadodecamonant doesn't exist in grid; within add_spawn(%s, %d, %d, %d)",
                 type.c_str(), count, x, y);
        return;
    }
    if( ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] && !GetMType(type)->in_category("CLASSIC") &&
        !GetMType(type)->in_category("WILDLIFE") ) {
        // Don't spawn non-classic monsters in classic zombie mode.
        return;
    }
    x %= SEEX;
    y %= SEEY;
    spawn_point tmp(type, count, x, y, faction_id, mission_id, friendly, name);
    grid[nonant]->spawns.push_back(tmp);
}

void map::add_spawn(monster *mon)
{
    int spawnx, spawny;
    std::string spawnname = (mon->unique_name == "" ? "NONE" : mon->unique_name);
    if (mon->spawnmapx != -1) {
        spawnx = mon->spawnposx;
        spawny = mon->spawnposy;
    } else {
        spawnx = mon->posx();
        spawny = mon->posy();
    }
    while (spawnx < 0) {
        spawnx += SEEX;
    }
    while (spawny < 0) {
        spawny += SEEY;
    }
    spawnx %= SEEX;
    spawny %= SEEY;
    add_spawn(mon->type->id, 1, spawnx, spawny, (mon->friendly < 0),
              mon->faction_id, mon->mission_id, spawnname);
}

vehicle *map::add_vehicle(game *g, std::string type, const int x, const int y, const int dir,
                          const int veh_fuel, const int veh_status, const bool merge_wrecks)
{
    if(g->vtypes.count(type) == 0) {
        debugmsg("Nonexistant vehicle type: \"%s\"", type.c_str());
        return NULL;
    }
    if (x < 0 || x >= SEEX * my_MAPSIZE || y < 0 || y >= SEEY * my_MAPSIZE) {
        debugmsg("Out of bounds add_vehicle t=%s d=%d x=%d y=%d", type.c_str(), dir, x, y);
        return NULL;
    }
    // debugmsg("add_vehicle t=%d d=%d x=%d y=%d", type, dir, x, y);

    const int smx = x / SEEX;
    const int smy = y / SEEY;
    // debugmsg("n=%d x=%d y=%d MAPSIZE=%d ^2=%d", nonant, x, y, MAPSIZE, MAPSIZE*MAPSIZE);
    vehicle *veh = new vehicle(g, type, veh_fuel, veh_status);
    veh->posx = x % SEEX;
    veh->posy = y % SEEY;
    veh->smx = smx;
    veh->smy = smy;
    veh->place_spawn_items();
    veh->face.init(dir);
    veh->turn_dir = dir;
    veh->precalc_mounts (0, dir);
    // veh->init_veh_fuel = 50;
    // veh->init_veh_status = 0;

    vehicle *placed_vehicle = add_vehicle_to_map(veh, x, y, merge_wrecks);

    if(placed_vehicle != NULL) {
        const int nonant = placed_vehicle->smx + placed_vehicle->smy * my_MAPSIZE;
        grid[nonant]->vehicles.push_back(placed_vehicle);

        vehicle_list.insert(placed_vehicle);
        update_vehicle_cache(placed_vehicle, true);

        //debugmsg ("grid[%d]->vehicles.size=%d veh.parts.size=%d", nonant, grid[nonant]->vehicles.size(),veh.parts.size());
    }
    return placed_vehicle;
}

/**
 * Takes a vehicle already created with new and attempts to place it on the map,
 * checking for collisions. If the vehicle can't be placed, returns NULL,
 * otherwise returns a pointer to the placed vehicle, which may not necessarily
 * be the one passed in (if wreckage is created by fusing cars).
 * @param veh The vehicle to place on the map.
 * @return The vehicle that was finally placed.
 */
vehicle *map::add_vehicle_to_map(vehicle *veh, const int x, const int y, const bool merge_wrecks)
{
    //We only want to check once per square, so loop over all structural parts
    std::vector<int> frame_indices = veh->all_parts_at_location("structure");
    for (std::vector<int>::const_iterator part = frame_indices.begin();
         part != frame_indices.end(); part++) {
        const int px = x + veh->parts[*part].precalc_dx[0];
        const int py = y + veh->parts[*part].precalc_dy[0];

        //Don't spawn anything in water
        if (ter(px, py) == t_water_dp || ter(px, py) == t_water_pool) {
            delete veh;
            return NULL;
        }

        // Don't spawn shopping carts on top of another vehicle or other obstacle.
        if (veh->type == "shopping_cart") {
            if (veh_at(px, py) != NULL || move_cost(px, py) == 0) {
                delete veh;
                return NULL;
            }
        }

        //When hitting a wall, only smash the vehicle once (but walls many times)
        bool veh_smashed = false;
        //For other vehicles, simulate collisions with (non-shopping cart) stuff
        vehicle *other_veh = veh_at(px, py);
        if (other_veh != NULL && other_veh->type != "shopping cart") {
            if( !merge_wrecks ) {
                delete veh;
                return NULL;
            }

            /* There's a vehicle here, so let's fuse them together into wreckage and
             * smash them up. It'll look like a nasty collision has occurred.
             * Trying to do a local->global->local conversion would be a major
             * headache, so instead, let's make another vehicle whose (0, 0) point
             * is the (0, 0) of the existing vehicle, convert the coordinates of both
             * vehicles into global coordinates, find the distance between them and
             * (px, py) and then install them that way.
             * Create a vehicle with type "null" so it starts out empty. */
            vehicle *wreckage = new vehicle(g);
            wreckage->posx = other_veh->posx;
            wreckage->posy = other_veh->posy;
            wreckage->smx = other_veh->smx;
            wreckage->smy = other_veh->smy;

            //Where are we on the global scale?
            const int global_x = wreckage->smx * SEEX + wreckage->posx;
            const int global_y = wreckage->smy * SEEY + wreckage->posy;

            for (int part_index = 0; part_index < veh->parts.size(); part_index++) {

                const int local_x = (veh->smx * SEEX + veh->posx)
                                    + veh->parts[part_index].precalc_dx[0]
                                    - global_x;
                const int local_y = (veh->smy * SEEY + veh->posy)
                                    + veh->parts[part_index].precalc_dy[0]
                                    - global_y;

                wreckage->install_part(local_x, local_y, veh->parts[part_index].id, -1, true);

            }
            for (int part_index = 0; part_index < other_veh->parts.size(); part_index++) {

                const int local_x = (other_veh->smx * SEEX + other_veh->posx)
                                    + other_veh->parts[part_index].precalc_dx[0]
                                    - global_x;
                const int local_y = (other_veh->smy * SEEY + other_veh->posy)
                                    + other_veh->parts[part_index].precalc_dy[0]
                                    - global_y;

                wreckage->install_part(local_x, local_y, other_veh->parts[part_index].id, -1, true);

            }

            wreckage->name = _("Wreckage");
            wreckage->smash();

            //Now get rid of the old vehicles
            destroy_vehicle(other_veh);
            delete veh;

            //Try again with the wreckage
            return add_vehicle_to_map(wreckage, global_x, global_y);

        } else if (move_cost(px, py) == 0) {
            if( !merge_wrecks ) {
                delete veh;
                return NULL;
            }

            //There's a wall or other obstacle here; destroy it
            destroy(g, px, py, false);

            //Then smash up the vehicle
            if(!veh_smashed) {
                veh->smash();
                veh_smashed = true;
            }

        }
    }

    return veh;
}

computer *map::add_computer(int x, int y, std::string name, int security)
{
    ter_set(x, y, t_console); // TODO: Turn this off?
    int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;
    grid[nonant]->comp = computer(name, security);
    return &(grid[nonant]->comp);
}

/**
 * Rotates this map, and all of its contents, by the specified multiple of 90
 * degrees.
 * @param turns How many 90-degree turns to rotate the map.
 */
void map::rotate(int turns)
{

    //Handle anything outside the 1-3 range gracefully; rotate(0) is a no-op.
    turns = turns % 4;
    if(turns == 0) {
        return;
    }

    ter_id rotated [SEEX * 2][SEEY * 2];
    furn_id furnrot [SEEX * 2][SEEY * 2];
    trap_id traprot [SEEX * 2][SEEY * 2];
    std::vector<item> itrot[SEEX * 2][SEEY * 2];
    std::vector<spawn_point> sprot[MAPSIZE * MAPSIZE];
    computer tmpcomp;
    std::vector<vehicle *> tmpveh;

    //Rotate terrain first
    for (int old_x = 0; old_x < SEEX * 2; old_x++) {
        for (int old_y = 0; old_y < SEEY * 2; old_y++) {
            int new_x = old_x;
            int new_y = old_y;
            switch(turns) {
            case 1:
                new_x = old_y;
                new_y = SEEX * 2 - 1 - old_x;
                break;
            case 2:
                new_x = SEEX * 2 - 1 - old_x;
                new_y = SEEY * 2 - 1 - old_y;
                break;
            case 3:
                new_x = SEEY * 2 - 1 - old_y;
                new_y = old_x;
                break;
            }
            rotated[old_x][old_y] = ter(new_x, new_y);
            furnrot[old_x][old_y] = furn(new_x, new_y);
            itrot [old_x][old_y] = i_at(new_x, new_y);
            traprot[old_x][old_y] = tr_at(new_x, new_y);
            i_clear(new_x, new_y);
        }
    }

    //Next, spawn points
    for (int sx = 0; sx < 2; sx++) {
        for (int sy = 0; sy < 2; sy++) {
            int gridfrom = sx + sy * my_MAPSIZE;
            int gridto = gridfrom;;
            switch(turns) {
            case 1:
                gridto = sx * my_MAPSIZE + 1 - sy;
                break;
            case 2:
                gridto = (1 - sy) * my_MAPSIZE + 1 - sx;
                break;
            case 3:
                gridto = (1 - sx) * my_MAPSIZE + sy;
                break;
            }
            for (int spawn = 0; spawn < grid[gridfrom]->spawns.size(); spawn++) {
                spawn_point tmp = grid[gridfrom]->spawns[spawn];
                int new_x = tmp.posx;
                int new_y = tmp.posy;
                switch(turns) {
                case 1:
                    new_x = SEEY - 1 - tmp.posy;
                    new_y = tmp.posx;
                    break;
                case 2:
                    new_x = SEEX - 1 - tmp.posx;
                    new_y = SEEY - 1 - tmp.posy;
                    break;
                case 3:
                    new_x = tmp.posy;
                    new_y = SEEX - 1 - tmp.posx;
                    break;
                }
                tmp.posx = new_x;
                tmp.posy = new_y;
                sprot[gridto].push_back(tmp);
            }
        }
    }

    //Then computers
    switch (turns) {
    case 1:
        tmpcomp = grid[0]->comp;
        grid[0]->comp = grid[my_MAPSIZE]->comp;
        grid[my_MAPSIZE]->comp = grid[my_MAPSIZE + 1]->comp;
        grid[my_MAPSIZE + 1]->comp = grid[1]->comp;
        grid[1]->comp = tmpcomp;
        break;

    case 2:
        tmpcomp = grid[0]->comp;
        grid[0]->comp = grid[my_MAPSIZE + 1]->comp;
        grid[my_MAPSIZE + 1]->comp = tmpcomp;
        tmpcomp = grid[1]->comp;
        grid[1]->comp = grid[my_MAPSIZE]->comp;
        grid[my_MAPSIZE]->comp = tmpcomp;
        break;

    case 3:
        tmpcomp = grid[0]->comp;
        grid[0]->comp = grid[1]->comp;
        grid[1]->comp = grid[my_MAPSIZE + 1]->comp;
        grid[my_MAPSIZE + 1]->comp = grid[my_MAPSIZE]->comp;
        grid[my_MAPSIZE]->comp = tmpcomp;
        break;
    }

    for(std::set<vehicle *>::iterator next_vehicle = vehicle_list.begin();
        next_vehicle != vehicle_list.end(); next_vehicle++) {

        int new_x = (*next_vehicle)->smx;
        int new_y = (*next_vehicle)->smy;
        switch(turns) {
        case 1:
            new_x = SEEY - 1 - (*next_vehicle)->smy;
            new_y = (*next_vehicle)->smx;
            break;
        case 2:
            new_x = SEEX - 1 - (*next_vehicle)->smx;
            new_y = SEEY - 1 - (*next_vehicle)->smy;
            break;
        case 3:
            new_x = (*next_vehicle)->smy;
            new_y = SEEX - 1 - (*next_vehicle)->smx;
            break;
        }
        (*next_vehicle)->smx = new_x;
        (*next_vehicle)->smy = new_y;

    }

    // change vehicles' directions
    for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
        for (int v = 0; v < grid[i]->vehicles.size(); v++) {
            if (turns >= 1 && turns <= 3) {
                grid[i]->vehicles[v]->turn(turns * 90);
            }
        }
    }

    // Set the spawn points
    grid[0]->spawns = sprot[0];
    grid[1]->spawns = sprot[1];
    grid[my_MAPSIZE]->spawns = sprot[my_MAPSIZE];
    grid[my_MAPSIZE + 1]->spawns = sprot[my_MAPSIZE + 1];
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            ter_set(i, j, rotated[i][j]);
            furn_set(i, j, furnrot[i][j]);
            i_at(i, j) = itrot [i][j];
            add_trap(i, j, traprot[i][j]);
            if (turns % 2 == 1) { // Rotate things like walls 90 degrees
                if (ter(i, j) == t_wall_v) {
                    ter_set(i, j, t_wall_h);
                } else if (ter(i, j) == t_wall_h) {
                    ter_set(i, j, t_wall_v);
                } else if (ter(i, j) == t_wall_metal_v) {
                    ter_set(i, j, t_wall_metal_h);
                } else if (ter(i, j) == t_wall_metal_h) {
                    ter_set(i, j, t_wall_metal_v);
                } else if (ter(i, j) == t_railing_v) {
                    ter_set(i, j, t_railing_h);
                } else if (ter(i, j) == t_railing_h) {
                    ter_set(i, j, t_railing_v);
                } else if (ter(i, j) == t_wall_glass_h) {
                    ter_set(i, j, t_wall_glass_v);
                } else if (ter(i, j) == t_wall_glass_v) {
                    ter_set(i, j, t_wall_glass_h);
                } else if (ter(i, j) == t_wall_glass_h_alarm) {
                    ter_set(i, j, t_wall_glass_v_alarm);
                } else if (ter(i, j) == t_wall_glass_v_alarm) {
                    ter_set(i, j, t_wall_glass_h_alarm);
                } else if (ter(i, j) == t_reinforced_glass_h) {
                    ter_set(i, j, t_reinforced_glass_v);
                } else if (ter(i, j) == t_reinforced_glass_v) {
                    ter_set(i, j, t_reinforced_glass_h);
                } else if (ter(i, j) == t_fence_v) {
                    ter_set(i, j, t_fence_h);
                } else if (ter(i, j) == t_fence_h) {
                    ter_set(i, j, t_fence_v);
                } else if (ter(i, j) == t_chainfence_h) {
                    ter_set(i, j, t_chainfence_v);
                } else if (ter(i, j) == t_chainfence_v) {
                    ter_set(i, j, t_chainfence_h);
                } else if (ter(i, j) == t_concrete_h) {
                    ter_set(i, j, t_concrete_v);
                } else if (ter(i, j) == t_concrete_v) {
                    ter_set(i, j, t_concrete_h);
                }
            }
        }
    }
}

// Hideous function, I admit...
bool connects_to(oter_id there, int dir)
{
    switch (dir) {
    case 2:
        if (there == "subway_ns"  || there == "subway_es" || there == "subway_sw" ||
            there == "subway_nes" || there == "subway_nsw" ||
            there == "subway_esw" || there == "subway_nesw" ||
            there == "sewer_ns"   || there == "sewer_es" || there == "sewer_sw" ||
            there == "sewer_nes"  || there == "sewer_nsw" || there == "sewer_esw" ||
            there == "sewer_nesw" || there == "ants_ns" || there == "ants_es" ||
            there == "ants_sw"    || there == "ants_nes" ||  there == "ants_nsw" ||
            there == "ants_esw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    case 3:
        if (there == "subway_ew"  || there == "subway_sw" || there == "subway_wn" ||
            there == "subway_new" || there == "subway_nsw" ||
            there == "subway_esw" || there == "subway_nesw" ||
            there == "sewer_ew"   || there == "sewer_sw" || there == "sewer_wn" ||
            there == "sewer_new"  || there == "sewer_nsw" || there == "sewer_esw" ||
            there == "sewer_nesw" || there == "ants_ew" || there == "ants_sw" ||
            there == "ants_wn"    || there == "ants_new" || there == "ants_nsw" ||
            there == "ants_esw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    case 0:
        if (there == "subway_ns"  || there == "subway_ne" || there == "subway_wn" ||
            there == "subway_nes" || there == "subway_new" ||
            there == "subway_nsw" || there == "subway_nesw" ||
            there == "sewer_ns"   || there == "sewer_ne" ||  there == "sewer_wn" ||
            there == "sewer_nes"  || there == "sewer_new" || there == "sewer_nsw" ||
            there == "sewer_nesw" || there == "ants_ns" || there == "ants_ne" ||
            there == "ants_wn"    || there == "ants_nes" || there == "ants_new" ||
            there == "ants_nsw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    case 1:
        if (there == "subway_ew"  || there == "subway_ne" || there == "subway_es" ||
            there == "subway_nes" || there == "subway_new" ||
            there == "subway_esw" || there == "subway_nesw" ||
            there == "sewer_ew"   || there == "sewer_ne" || there == "sewer_es" ||
            there == "sewer_nes"  || there == "sewer_new" || there == "sewer_esw" ||
            there == "sewer_nesw" || there == "ants_ew" || there == "ants_ne" ||
            there == "ants_es"    || there == "ants_nes" || there == "ants_new" ||
            there == "ants_esw"   || there == "ants_nesw") {
            return true;
        }
        return false;
    default:
        debugmsg("Connects_to with dir of %d", dir);
        return false;
    }
}

void house_room(map *m, room_type type, int x1, int y1, int x2, int y2)
{
    int pos_x1 = 0;
    int pos_y1 = 0;

    if (type == room_backyard) { //processing it separately
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if ((i == x1) || (i == x2)) {
                    m->ter_set(i, j, t_fence_v);
                } else if (j == y2) {
                    m->ter_set(i, j, t_fence_h);
                } else {
                    m->ter_set( i, j, t_grass);
                    if (one_in(35)) {
                        m->ter_set(i, j, t_tree_young);
                    } else if (one_in(35)) {
                        m->ter_set(i, j, t_tree);
                    } else if (one_in(25)) {
                        m->ter_set(i, j, t_dirt);
                    }
                }
            }
        }
        m->ter_set((x1 + x2) / 2, y2, t_fencegate_c);
        m->furn_set(x1 + 2, y1, f_chair);
        m->furn_set(x1 + 2, y1 + 1, f_table);
        return;
    }

    for (int i = x1; i <= x2; i++) {
        for (int j = y1; j <= y2; j++) {
            if (m->ter(i, j) == t_grass || m->ter(i, j) == t_dirt ||
                m->ter(i, j) == t_floor) {
                if (j == y1 || j == y2) {
                    m->ter_set(i, j, t_wall_h);
                    m->ter_set(i, j, t_wall_h);
                } else if (i == x1 || i == x2) {
                    m->ter_set(i, j, t_wall_v);
                    m->ter_set(i, j, t_wall_v);
                } else {
                    m->ter_set(i, j, t_floor);
                }
            }
        }
    }
    for (int i = y1 + 1; i <= y2 - 1; i++) {
        m->ter_set(x1, i, t_wall_v);
        m->ter_set(x2, i, t_wall_v);
    }

    items_location placed = "none";
    int chance = 0, rn;
    switch (type) {
    case room_study:
        placed = "livingroom";
        chance = 40;
        break;
    case room_living:
        placed = "livingroom";
        chance = 83;
        //choose random wall
        switch (rng(1, 4)) { //some bookshelves
        case 1:
            pos_x1 = x1 + 2;
            pos_y1 = y1 + 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 < x2) {
                pos_x1 += 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 2;
            }
            break;
        case 2:
            pos_x1 = x2 - 2;
            pos_y1 = y1 + 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 > x1) {
                pos_x1 -= 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 2;
            }
            break;
        case 3:
            pos_x1 = x1 + 2;
            pos_y1 = y2 - 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 < x2) {
                pos_x1 += 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 += 2;
            }
            break;
        case 4:
            pos_x1 = x2 - 2;
            pos_y1 = y2 - 1;
            m->furn_set(x1 + 2, y2 - 1, f_desk);
            while (pos_x1 > x1) {
                pos_x1 -= 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 1;
                if ((m->ter(pos_x1, pos_y1) == t_wall_h) || (m->ter(pos_x1, pos_y1) == t_wall_v)) {
                    break;
                }
                m->furn_set(pos_x1, pos_y1, f_bookcase);
                pos_x1 -= 2;
            }
            break;
            m->furn_set(rng(x1 + 2, x2 - 2), rng(y1 + 1, y2 - 1), f_armchair);
        }


        break;
    case room_kitchen: {
        placed = "kitchen";
        chance = 75;
        m->place_items("cleaning",  58, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        m->place_items("home_hw",   40, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        int oven_x = -1, oven_y = -1, cupboard_x = -1, cupboard_y = -1;

        switch (rng(1, 4)) { //fridge, sink, oven and some cupboards near them
        case 1:
            m->furn_set(x1 + 2, y1 + 1, f_fridge);
            m->place_items("fridge", 82, x1 + 2, y1 + 1, x1 + 2, y1 + 1, false, 0);
            m->furn_set(x1 + 1, y1 + 1, f_sink);
            if (x1 + 4 < x2) {
                oven_x     = x1 + 3;
                cupboard_x = x1 + 4;
                oven_y = cupboard_y = y1 + 1;
            }

            break;
        case 2:
            m->furn_set(x2 - 2, y1 + 1, f_fridge);
            m->place_items("fridge", 82, x2 - 2, y1 + 1, x2 - 2, y1 + 1, false, 0);
            m->furn_set(x2 - 1, y1 + 1, f_sink);
            if (x2 - 4 > x1) {
                oven_x     = x2 - 3;
                cupboard_x = x2 - 4;
                oven_y = cupboard_y = y1 + 1;
            }
            break;
        case 3:
            m->furn_set(x1 + 2, y2 - 1, f_fridge);
            m->place_items("fridge", 82, x1 + 2, y2 - 1, x1 + 2, y2 - 1, false, 0);
            m->furn_set(x1 + 1, y2 - 1, f_sink);
            if (x1 + 4 < x2) {
                oven_x     = x1 + 3;
                cupboard_x = x1 + 4;
                oven_y = cupboard_y = y2 - 1;
            }
            break;
        case 4:
            m->furn_set(x2 - 2, y2 - 1, f_fridge);
            m->place_items("fridge", 82, x2 - 2, y2 - 1, x2 - 2, y2 - 1, false, 0);
            m->furn_set(x2 - 1, y2 - 1, f_sink);
            if (x2 - 4 > x1) {
                oven_x     = x2 - 3;
                cupboard_x = x2 - 4;
                oven_y = cupboard_y = y2 - 1;
            }
            break;
        }

        // oven and it's contents
        if ( oven_x != -1 && oven_y != -1 ) {
            m->furn_set(oven_x, oven_y, f_oven);
            m->place_items("oven",       70, oven_x, oven_y, oven_x, oven_y, false, 0);
        }

        // cupboard and it's contents
        if ( cupboard_x != -1 && cupboard_y != -1 ) {
            m->furn_set(cupboard_x, cupboard_y, f_cupboard);
            m->place_items("cleaning",   30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
            m->place_items("home_hw",    30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
            m->place_items("cannedfood", 30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
            m->place_items("pasta",      30, cupboard_x, cupboard_y, cupboard_x, cupboard_y, false, 0);
        }

        if (one_in(2)) { //dining table in the kitchen
            square_furn(m, f_table, int((x1 + x2) / 2) - 1, int((y1 + y2) / 2) - 1, int((x1 + x2) / 2),
                        int((y1 + y2) / 2) );
        }
        if (one_in(2)) {
            for (int i = 0; i <= 2; i++) {
                pos_x1 = rng(x1 + 2, x2 - 2);
                pos_y1 = rng(y1 + 1, y2 - 1);
                if (m->ter(pos_x1, pos_y1) == t_floor) {
                    m->furn_set(pos_x1, pos_y1, f_chair);
                }
            }
        }

    }
    break;
    case room_bedroom:
        placed = "bedroom";
        chance = 78;
        if (one_in(14)) {
            m->place_items("homeguns", 58, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        }
        if (one_in(10)) {
            m->place_items("home_hw",  40, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        }
        switch (rng(1, 5)) {
        case 1:
            m->furn_set(x1 + 1, y1 + 2, f_bed);
            m->furn_set(x1 + 1, y1 + 3, f_bed);
            break;
        case 2:
            m->furn_set(x1 + 2, y2 - 1, f_bed);
            m->furn_set(x1 + 3, y2 - 1, f_bed);
            break;
        case 3:
            m->furn_set(x2 - 1, y2 - 3, f_bed);
            m->furn_set(x2 - 1, y2 - 2, f_bed);
            break;
        case 4:
            m->furn_set(x2 - 3, y1 + 1, f_bed);
            m->furn_set(x2 - 2, y1 + 1, f_bed);
            break;
        case 5:
            m->furn_set(int((x1 + x2) / 2)    , y2 - 1, f_bed);
            m->furn_set(int((x1 + x2) / 2) + 1, y2 - 1, f_bed);
            m->furn_set(int((x1 + x2) / 2)    , y2 - 2, f_bed);
            m->furn_set(int((x1 + x2) / 2) + 1, y2 - 2, f_bed);
            break;
        }
        switch (rng(1, 4)) {
        case 1:
            m->furn_set(x1 + 2, y1 + 1, f_dresser);
            m->place_items("dresser", 80, x1 + 2, y1 + 1, x1 + 2, y1 + 1, false, 0);
            break;
        case 2:
            m->furn_set(x2 - 2, y2 - 1, f_dresser);
            m->place_items("dresser", 80, x2 - 2, y2 - 1, x2 - 2, y2 - 1, false, 0);
            break;
        case 3:
            rn = int((x1 + x2) / 2);
            m->furn_set(rn, y1 + 1, f_dresser);
            m->place_items("dresser", 80, rn, y1 + 1, rn, y1 + 1, false, 0);
            break;
        case 4:
            rn = int((y1 + y2) / 2);
            m->furn_set(x1 + 1, rn, f_dresser);
            m->place_items("dresser", 80, x1 + 1, rn, x1 + 1, rn, false, 0);
            break;
        }
        break;
    case room_bathroom:
        m->place_toilet(x2 - 1, y2 - 1);
        m->place_items("harddrugs", 18, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        m->place_items("cleaning",  48, x1 + 1, y1 + 1, x2 - 1, y2 - 2, false, 0);
        placed = "softdrugs";
        chance = 72;
        m->furn_set(x2 - 1, y2 - 2, f_bathtub);
        if (!((m->ter(x2 - 3, y2 - 2) == t_wall_v) || (m->ter(x2 - 3, y2 - 2) == t_wall_h))) {
            m->furn_set(x2 - 3, y2 - 2, f_sink);
        }
        break;
    default:
        break;
    }
    m->place_items(placed, chance, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
}

void science_room(map *m, int x1, int y1, int x2, int y2, int rotate)
{
    int height = y2 - y1;
    int width  = x2 - x1;
    if (rotate % 2 == 1) { // Swap width & height if we're a lateral room
        int tmp = height;
        height  = width;
        width   = tmp;
    }
    for (int i = x1; i <= x2; i++) {
        for (int j = y1; j <= y2; j++) {
            m->ter_set(i, j, t_rock_floor);
        }
    }
    int area = height * width;
    std::vector<room_type> valid_rooms;
    if (height < 5 && width < 5) {
        valid_rooms.push_back(room_closet);
    }
    if (height > 6 && width > 3) {
        valid_rooms.push_back(room_lobby);
    }
    if (height > 4 || width > 4) {
        valid_rooms.push_back(room_chemistry);
    }
    if ((height > 7 || width > 7) && height > 2 && width > 2) {
        valid_rooms.push_back(room_teleport);
    }
    if (height > 4 && width > 4) {
        valid_rooms.push_back(room_goo);
    }
    if (height > 7 && width > 7) {
        valid_rooms.push_back(room_bionics);
    }
    if (height > 7 && width > 7) {
        valid_rooms.push_back(room_cloning);
    }
    if (area >= 9) {
        valid_rooms.push_back(room_vivisect);
    }
    if (height > 5 && width > 4) {
        valid_rooms.push_back(room_dorm);
    }
    if (width > 8) {
        for (int i = 8; i < width; i += rng(1, 2)) {
            valid_rooms.push_back(room_split);
        }
    }

    room_type chosen = valid_rooms[rng(0, valid_rooms.size() - 1)];
    int trapx = rng(x1 + 1, x2 - 1);
    int trapy = rng(y1 + 1, y2 - 1);
    switch (chosen) {
    case room_closet:
        m->place_items("cleaning", 80, x1, y1, x2, y2, false, 0);
        break;
    case room_lobby:
        if (rotate % 2 == 0) { // Vertical
            int desk = y1 + rng(int(height / 2) - int(height / 4), int(height / 2) + 1);
            for (int x = x1 + int(width / 4); x < x2 - int(width / 4); x++) {
                m->furn_set(x, desk, f_counter);
            }
            computer *tmpcomp = m->add_computer(x2 - int(width / 4), desk,
                                                _("Log Console"), 3);
            tmpcomp->add_option(_("View Research Logs"), COMPACT_RESEARCH, 0);
            tmpcomp->add_option(_("Download Map Data"), COMPACT_MAPS, 0);
            tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
            tmpcomp->add_failure(COMPFAIL_ALARM);
            tmpcomp->add_failure(COMPFAIL_DAMAGE);
            m->add_spawn("mon_turret", 1, int((x1 + x2) / 2), desk);
        } else {
            int desk = x1 + rng(int(height / 2) - int(height / 4), int(height / 2) + 1);
            for (int y = y1 + int(width / 4); y < y2 - int(width / 4); y++) {
                m->furn_set(desk, y, f_counter);
            }
            computer *tmpcomp = m->add_computer(desk, y2 - int(width / 4),
                                                _("Log Console"), 3);
            tmpcomp->add_option(_("View Research Logs"), COMPACT_RESEARCH, 0);
            tmpcomp->add_option(_("Download Map Data"), COMPACT_MAPS, 0);
            tmpcomp->add_failure(COMPFAIL_SHUTDOWN);
            tmpcomp->add_failure(COMPFAIL_ALARM);
            tmpcomp->add_failure(COMPFAIL_DAMAGE);
            m->add_spawn("mon_turret", 1, desk, int((y1 + y2) / 2));
        }
        break;
    case room_chemistry:
        if (rotate % 2 == 0) { // Vertical
            for (int x = x1; x <= x2; x++) {
                if (x % 3 == 0) {
                    for (int y = y1 + 1; y <= y2 - 1; y++) {
                        m->furn_set(x, y, f_counter);
                    }
                    m->place_items("chem_lab", 70, x, y1 + 1, x, y2 - 1, false, 0);
                }
            }
        } else {
            for (int y = y1; y <= y2; y++) {
                if (y % 3 == 0) {
                    for (int x = x1 + 1; x <= x2 - 1; x++) {
                        m->furn_set(x, y, f_counter);
                    }
                    m->place_items("chem_lab", 70, x1 + 1, y, x2 - 1, y, false, 0);
                }
            }
        }
        break;
    case room_teleport:
        m->furn_set(int((x1 + x2) / 2)    , int((y1 + y2) / 2)    , f_counter);
        m->furn_set(int((x1 + x2) / 2) + 1, int((y1 + y2) / 2)    , f_counter);
        m->furn_set(int((x1 + x2) / 2)    , int((y1 + y2) / 2) + 1, f_counter);
        m->furn_set(int((x1 + x2) / 2) + 1, int((y1 + y2) / 2) + 1, f_counter);
        m->add_trap(trapx, trapy, tr_telepad);
        m->place_items("teleport", 70, int((x1 + x2) / 2),
                       int((y1 + y2) / 2), int((x1 + x2) / 2) + 1,
                       int((y1 + y2) / 2) + 1, false, 0);
        break;
    case room_goo:
        do {
            m->add_trap(trapx, trapy, tr_goo);
            trapx = rng(x1 + 1, x2 - 1);
            trapy = rng(y1 + 1, y2 - 1);
        } while(!one_in(5));
        if (rotate == 0) {
            m->remove_trap(x1, y2);
            m->furn_set(x1, y2, f_fridge);
            m->place_items("goo", 60, x1, y2, x1, y2, false, 0);
        } else if (rotate == 1) {
            m->remove_trap(x1, y1);
            m->furn_set(x1, y1, f_fridge);
            m->place_items("goo", 60, x1, y1, x1, y1, false, 0);
        } else if (rotate == 2) {
            m->remove_trap(x2, y1);
            m->furn_set(x2, y1, f_fridge);
            m->place_items("goo", 60, x2, y1, x2, y1, false, 0);
        } else {
            m->remove_trap(x2, y2);
            m->furn_set(x2, y2, f_fridge);
            m->place_items("goo", 60, x2, y2, x2, y2, false, 0);
        }
        break;
    case room_cloning:
        for (int x = x1 + 1; x <= x2 - 1; x++) {
            for (int y = y1 + 1; y <= y2 - 1; y++) {
                if (x % 3 == 0 && y % 3 == 0) {
                    m->ter_set(x, y, t_vat);
                    m->place_items("cloning_vat", 20, x, y, x, y, false, 0);
                }
            }
        }
        break;
    case room_vivisect:
        if        (rotate == 0) {
            for (int x = x1; x <= x2; x++) {
                m->furn_set(x, y2 - 1, f_counter);
            }
            m->place_items("dissection", 80, x1, y2 - 1, x2, y2 - 1, false, 0);
        } else if (rotate == 1) {
            for (int y = y1; y <= y2; y++) {
                m->furn_set(x1 + 1, y, f_counter);
            }
            m->place_items("dissection", 80, x1 + 1, y1, x1 + 1, y2, false, 0);
        } else if (rotate == 2) {
            for (int x = x1; x <= x2; x++) {
                m->furn_set(x, y1 + 1, f_counter);
            }
            m->place_items("dissection", 80, x1, y1 + 1, x2, y1 + 1, false, 0);
        } else if (rotate == 3) {
            for (int y = y1; y <= y2; y++) {
                m->furn_set(x2 - 1, y, f_counter);
            }
            m->place_items("dissection", 80, x2 - 1, y1, x2 - 1, y2, false, 0);
        }
        m->add_trap(int((x1 + x2) / 2), int((y1 + y2) / 2), tr_dissector);
        break;

    case room_bionics:
        if (rotate % 2 == 0) {
            int biox = x1 + 2, bioy = int((y1 + y2) / 2);
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
---\n\
|c=\n\
---\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_v),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            biox = x2 - 2;
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
---\n\
=c|\n\
---\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_v),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            int compx = int((x1 + x2) / 2), compy = int((y1 + y2) / 2);
            m->ter_set(compx, compy, t_console);
            computer *tmpcomp = m->add_computer(compx, compy, _("Bionic access"), 2);
            tmpcomp->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp->add_option(_("Open Chambers"), COMPACT_RELEASE, 3);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);
        } else {
            int bioy = y1 + 2, biox = int((x1 + x2) / 2);
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
|-|\n\
|c|\n\
|=|\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_h),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            bioy = y2 - 2;
            mapf::formatted_set_simple(m, biox - 1, bioy - 1,
                                       "\
|=|\n\
|c|\n\
|-|\n",
                                       mapf::basic_bind("- | =", t_wall_h, t_wall_v, t_reinforced_glass_h),
                                       mapf::basic_bind("c", f_counter));
            m->place_items("bionics_common", 70, biox, bioy, biox, bioy, false, 0);

            int compx = int((x1 + x2) / 2), compy = int((y1 + y2) / 2);
            m->ter_set(compx, compy, t_console);
            computer *tmpcomp = m->add_computer(compx, compy, _("Bionic access"), 2);
            tmpcomp->add_option(_("Manifest"), COMPACT_LIST_BIONICS, 0);
            tmpcomp->add_option(_("Open Chambers"), COMPACT_RELEASE, 3);
            tmpcomp->add_failure(COMPFAIL_MANHACKS);
            tmpcomp->add_failure(COMPFAIL_SECUBOTS);
        }
        break;
    case room_dorm:
        if (rotate % 2 == 0) {
            for (int y = y1 + 1; y <= y2 - 1; y += 3) {
                m->furn_set(x1    , y, f_bed);
                m->furn_set(x1 + 1, y, f_bed);
                m->furn_set(x2    , y, f_bed);
                m->furn_set(x2 - 1, y, f_bed);
                m->furn_set(x1, y + 1, f_dresser);
                m->furn_set(x2, y + 1, f_dresser);
                m->place_items("dresser", 70, x1, y + 1, x1, y + 1, false, 0);
                m->place_items("dresser", 70, x2, y + 1, x2, y + 1, false, 0);
            }
        } else if (rotate % 2 == 1) {
            for (int x = x1 + 1; x <= x2 - 1; x += 3) {
                m->furn_set(x, y1    , f_bed);
                m->furn_set(x, y1 + 1, f_bed);
                m->furn_set(x, y2    , f_bed);
                m->furn_set(x, y2 - 1, f_bed);
                m->furn_set(x + 1, y1, f_dresser);
                m->furn_set(x + 1, y2, f_dresser);
                m->place_items("dresser", 70, x + 1, y1, x + 1, y1, false, 0);
                m->place_items("dresser", 70, x + 1, y2, x + 1, y2, false, 0);
            }
        }
        m->place_items("lab_dorm", 84, x1, y1, x2, y2, false, 0);
        break;
    case room_split:
        if (rotate % 2 == 0) {
            int w1 = int((x1 + x2) / 2) - 2, w2 = int((x1 + x2) / 2) + 2;
            for (int y = y1; y <= y2; y++) {
                m->ter_set(w1, y, t_wall_v);
                m->ter_set(w2, y, t_wall_v);
            }
            m->ter_set(w1, int((y1 + y2) / 2), t_door_metal_c);
            m->ter_set(w2, int((y1 + y2) / 2), t_door_metal_c);
            science_room(m, x1, y1, w1 - 1, y2, 1);
            science_room(m, w2 + 1, y1, x2, y2, 3);
        } else {
            int w1 = int((y1 + y2) / 2) - 2, w2 = int((y1 + y2) / 2) + 2;
            for (int x = x1; x <= x2; x++) {
                m->ter_set(x, w1, t_wall_h);
                m->ter_set(x, w2, t_wall_h);
            }
            m->ter_set(int((x1 + x2) / 2), w1, t_door_metal_c);
            m->ter_set(int((x1 + x2) / 2), w2, t_door_metal_c);
            science_room(m, x1, y1, x2, w1 - 1, 2);
            science_room(m, x1, w2 + 1, x2, y2, 0);
        }
        break;
    default:
        break;
    }
}

void set_science_room(map *m, int x1, int y1, bool faces_right, int turn)
{
    // TODO: More types!
    int type = rng(0, 4);
    int x2 = x1 + 7;
    int y2 = y1 + 4;
    switch (type) {
    case 0: // Empty!
        return;
    case 1: // Chemistry.
        // #######.
        // #.......
        // #.......
        // #.......
        // #######.
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if ((i == x1 || j == y1 || j == y2) && i != x1) {
                    m->set(i, j, t_floor, f_counter);
                }
            }
        }
        m->place_items("chem_lab", 85, x1 + 1, y1, x2 - 1, y1, false, 0);
        m->place_items("chem_lab", 85, x1 + 1, y2, x2 - 1, y2, false, 0);
        m->place_items("chem_lab", 85, x1, y1 + 1, x1, y2 - 1, false, 0);
        break;

    case 2: // Hydroponics.
        // #.......
        // #.~~~~~.
        // #.......
        // #.~~~~~.
        // #.......
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if (i == x1) {
                    m->set(i, j, t_floor, f_counter);
                } else if (i > x1 + 1 && i < x2 && (j == y1 + 1 || j == y2 - 1)) {
                    m->ter_set(i, j, t_water_sh);
                }
            }
        }
        m->place_items("chem_lab", 80, x1, y1, x1, y2, false, turn - 50);
        m->place_items("hydro", 92, x1 + 1, y1 + 1, x2 - 1, y1 + 1, false, turn);
        m->place_items("hydro", 92, x1 + 1, y2 - 1, x2 - 1, y2 - 1, false, turn);
        break;

    case 3: // Electronics.
        // #######.
        // #.......
        // #.......
        // #.......
        // #######.
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if ((i == x1 || j == y1 || j == y2) && i != x1) {
                    m->set(i, j, t_floor, f_counter);
                }
            }
        }
        m->place_items("electronics", 85, x1 + 1, y1, x2 - 1, y1, false, turn - 50);
        m->place_items("electronics", 85, x1 + 1, y2, x2 - 1, y2, false, turn - 50);
        m->place_items("electronics", 85, x1, y1 + 1, x1, y2 - 1, false, turn - 50);
        break;

    case 4: // Monster research.
        // .|.####.
        // -|......
        // .|......
        // -|......
        // .|.####.
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                if (i == x1 + 1) {
                    m->ter_set(i, j, t_wall_glass_v);
                } else if (i == x1 && (j == y1 + 1 || j == y2 - 1)) {
                    m->ter_set(i, j, t_wall_glass_h);
                } else if ((j == y1 || j == y2) && i >= x1 + 3 && i <= x2 - 1) {
                    m->set(i, j, t_floor, f_counter);
                }
            }
        }
        // TODO: Place a monster in the sealed areas.
        m->place_items("monparts", 70, x1 + 3, y1, 2 - 1, y1, false, turn - 100);
        m->place_items("monparts", 70, x1 + 3, y2, 2 - 1, y2, false, turn - 100);
        break;
    }

    if (!faces_right) { // Flip it.
        ter_id rotated[SEEX * 2][SEEY * 2];
        std::vector<item> itrot[SEEX * 2][SEEY * 2];
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                rotated[i][j] = m->ter(i, j);
                itrot[i][j] = m->i_at(i, j);
            }
        }
        for (int i = x1; i <= x2; i++) {
            for (int j = y1; j <= y2; j++) {
                m->ter_set(i, j, rotated[x2 - (i - x1)][j]);
                m->i_at(i, j) = itrot[x2 - (i - x1)][j];
            }
        }
    }
}

void silo_rooms(map *m)
{
    std::vector<point> rooms;
    std::vector<point> room_sizes;
    bool okay = true;
    do {
        int x, y, height, width;
        if (one_in(2)) { // True = top/bottom, False = left/right
            x = rng(0, SEEX * 2 - 6);
            y = rng(0, 4);
            if (one_in(2)) {
                y = SEEY * 2 - 2 - y;    // Bottom of the screen, not the top
            }
            width  = rng(2, 5);
            height = 2;
            if (x + width >= SEEX * 2 - 1) {
                width = SEEX * 2 - 2 - x;    // Make sure our room isn't too wide
            }
        } else {
            x = rng(0, 4);
            y = rng(0, SEEY * 2 - 6);
            if (one_in(2)) {
                x = SEEX * 2 - 3 - x;    // Right side of the screen, not the left
            }
            width  = 2;
            height = rng(2, 5);
            if (y + height >= SEEY * 2 - 1) {
                height = SEEY * 2 - 2 - y;    // Make sure our room isn't too tall
            }
        }
        if (!rooms.empty() && // We need at least one room!
            (m->ter(x, y) != t_rock || m->ter(x + width, y + height) != t_rock)) {
            okay = false;
        } else {
            rooms.push_back(point(x, y));
            room_sizes.push_back(point(width, height));
            for (int i = x; i <= x + width; i++) {
                for (int j = y; j <= y + height; j++) {
                    if (m->ter(i, j) == t_rock) {
                        m->ter_set(i, j, t_floor);
                    }
                }
            }
            items_location used1 = "none", used2 = "none";
            switch (rng(1, 14)) { // What type of items go here?
            case  1:
            case  2:
                used1 = "cannedfood";
                used2 = "fridge";
                break;
            case  3:
            case  4:
                used1 = "tools";
                break;
            case  5:
            case  6:
                used1 = "allguns";
                used2 = "ammo";
                break;
            case  7:
                used1 = "allclothes";
                break;
            case  8:
                used1 = "manuals";
                break;
            case  9:
            case 10:
            case 11:
                used1 = "electronics";
                break;
            case 12:
                used1 = "survival_tools";
                break;
            case 13:
            case 14:
                used1 = "radio";
                break;
            }
            if (used1 != "none") {
                m->place_items(used1, 78, x, y, x + width, y + height, false, 0);
            }
            if (used2 != "none") {
                m->place_items(used2, 64, x, y, x + width, y + height, false, 0);
            }
        }
    } while (okay);

    m->ter_set(rooms[0].x, rooms[0].y, t_stairs_up);
    int down_room = rng(0, rooms.size() - 1);
    point dp = rooms[down_room], ds = room_sizes[down_room];
    m->ter_set(dp.x + ds.x, dp.y + ds.y, t_stairs_down);
    rooms.push_back(point(SEEX, SEEY)); // So the center circle gets connected
    room_sizes.push_back(point(5, 5));

    while (rooms.size() > 1) {
        int best_dist = 999, closest = 0;
        for (int i = 1; i < rooms.size(); i++) {
            int dist = trig_dist(rooms[0].x, rooms[0].y, rooms[i].x, rooms[i].y);
            if (dist < best_dist) {
                best_dist = dist;
                closest = i;
            }
        }
        // We chose the closest room; now draw a corridor there
        point origin = rooms[0], origsize = room_sizes[0], dest = rooms[closest];
        int x = origin.x + origsize.x, y = origin.y + origsize.y;
        bool x_first = (abs(origin.x - dest.x) > abs(origin.y - dest.y));
        while (x != dest.x || y != dest.y) {
            if (m->ter(x, y) == t_rock) {
                m->ter_set(x, y, t_floor);
            }
            if ((x_first && x != dest.x) || (!x_first && y == dest.y)) {
                if (dest.x < x) {
                    x--;
                } else {
                    x++;
                }
            } else {
                if (dest.y < y) {
                    y--;
                } else {
                    y++;
                }
            }
        }
        rooms.erase(rooms.begin());
        room_sizes.erase(room_sizes.begin());
    }
}

void build_mine_room(map *m, room_type type, int x1, int y1, int x2, int y2)
{
    direction door_side;
    std::vector<direction> possibilities;
    int midx = int( (x1 + x2) / 2), midy = int( (y1 + y2) / 2);
    if (x2 < SEEX) {
        possibilities.push_back(EAST);
    }
    if (x1 > SEEX + 1) {
        possibilities.push_back(WEST);
    }
    if (y1 > SEEY + 1) {
        possibilities.push_back(NORTH);
    }
    if (y2 < SEEY) {
        possibilities.push_back(SOUTH);
    }

    if (possibilities.empty()) { // We're in the middle of the map!
        if (midx <= SEEX) {
            possibilities.push_back(EAST);
        } else {
            possibilities.push_back(WEST);
        }
        if (midy <= SEEY) {
            possibilities.push_back(SOUTH);
        } else {
            possibilities.push_back(NORTH);
        }
    }

    door_side = possibilities[rng(0, possibilities.size() - 1)];
    point door_point;
    switch (door_side) {
    case NORTH:
        door_point.x = midx;
        door_point.y = y1;
        break;
    case EAST:
        door_point.x = x2;
        door_point.y = midy;
        break;
    case SOUTH:
        door_point.x = midx;
        door_point.y = y2;
        break;
    case WEST:
        door_point.x = x1;
        door_point.y = midy;
        break;
    default:
        break;
    }
    square(m, t_floor, x1, y1, x2, y2);
    line(m, t_wall_h, x1, y1, x2, y1);
    line(m, t_wall_h, x1, y2, x2, y2);
    line(m, t_wall_v, x1, y1 + 1, x1, y2 - 1);
    line(m, t_wall_v, x2, y1 + 1, x2, y2 - 1);
    // Main build switch!
    switch (type) {
    case room_mine_shaft: {
        m->ter_set(x1 + 1, y1 + 1, t_console);
        line(m, t_wall_h, x2 - 2, y1 + 2, x2 - 1, y1 + 2);
        m->ter_set(x2 - 2, y1 + 1, t_elevator);
        m->ter_set(x2 - 1, y1 + 1, t_elevator_control_off);
        computer *tmpcomp = m->add_computer(x1 + 1, y1 + 1, _("NEPowerOS"), 2);
        tmpcomp->add_option(_("Divert power to elevator"), COMPACT_ELEVATOR_ON, 0);
        tmpcomp->add_failure(COMPFAIL_ALARM);
    }
    break;

    case room_mine_office:
        line_furn(m, f_counter, midx, y1 + 2, midx, y2 - 2);
        line(m, t_window, midx - 1, y1, midx + 1, y1);
        line(m, t_window, midx - 1, y2, midx + 1, y2);
        line(m, t_window, x1, midy - 1, x1, midy + 1);
        line(m, t_window, x2, midy - 1, x2, midy + 1);
        m->place_items("office", 80, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        break;

    case room_mine_storage:
        m->place_items("mine_storage", 85, x1 + 2, y1 + 2, x2 - 2, y2 - 2, false, 0);
        break;

    case room_mine_fuel: {
        int spacing = rng(2, 4);
        if (door_side == NORTH || door_side == SOUTH) {
            int y = (door_side == NORTH ? y1 + 2 : y2 - 2);
            for (int x = x1 + 1; x <= x2 - 1; x += spacing) {
                m->place_gas_pump(x, y, rng(10000, 50000));
            }
        } else {
            int x = (door_side == EAST ? x2 - 2 : x1 + 2);
            for (int y = y1 + 1; y <= y2 - 1; y += spacing) {
                m->place_gas_pump(x, y, rng(10000, 50000));
            }
        }
    }
    break;

    case room_mine_housing:
        if (door_side == NORTH || door_side == SOUTH) {
            for (int y = y1 + 2; y <= y2 - 2; y += 2) {
                m->ter_set(x1    , y, t_window);
                m->furn_set(x1 + 1, y, f_bed);
                m->furn_set(x1 + 2, y, f_bed);
                m->ter_set(x2    , y, t_window);
                m->furn_set(x2 - 1, y, f_bed);
                m->furn_set(x2 - 2, y, f_bed);
                m->furn_set(x1 + 1, y + 1, f_dresser);
                m->place_items("dresser", 78, x1 + 1, y + 1, x1 + 1, y + 1, false, 0);
                m->furn_set(x2 - 1, y + 1, f_dresser);
                m->place_items("dresser", 78, x2 - 1, y + 1, x2 - 1, y + 1, false, 0);
            }
        } else {
            for (int x = x1 + 2; x <= x2 - 2; x += 2) {
                m->ter_set(x, y1    , t_window);
                m->furn_set(x, y1 + 1, f_bed);
                m->furn_set(x, y1 + 2, f_bed);
                m->ter_set(x, y2    , t_window);
                m->furn_set(x, y2 - 1, f_bed);
                m->furn_set(x, y2 - 2, f_bed);
                m->furn_set(x + 1, y1 + 1, f_dresser);
                m->place_items("dresser", 78, x + 1, y1 + 1, x + 1, y1 + 1, false, 0);
                m->furn_set(x + 1, y2 - 1, f_dresser);
                m->place_items("dresser", 78, x + 1, y2 - 1, x + 1, y2 - 1, false, 0);
            }
        }
        m->place_items("bedroom", 65, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        break;
    }

    if (type == room_mine_fuel) { // Fuel stations are open on one side
        switch (door_side) {
        case NORTH:
            line(m, t_floor, x1, y1    , x2, y1    );
            break;
        case EAST:
            line(m, t_floor, x2, y1 + 1, x2, y2 - 1);
            break;
        case SOUTH:
            line(m, t_floor, x1, y2    , x2, y2    );
            break;
        case WEST:
            line(m, t_floor, x1, y1 + 1, x1, y2 - 1);
            break;
        default:
            break;
        }
    } else {
        if (type == room_mine_storage) { // Storage has a locked door
            m->ter_set(door_point.x, door_point.y, t_door_locked);
        } else {
            m->ter_set(door_point.x, door_point.y, t_door_c);
        }
    }
}

map_extra random_map_extra(map_extras embellishments)
{
    int pick = 0;
    // Set pick to the total of all the chances for map extras
    for (int i = 0; i < num_map_extras; i++) {
        if (!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] || mfb(i) & classic_extras) {
            pick += embellishments.chances[i];
        }
    }
    // Set pick to a number between 0 and the total
    pick = rng(0, pick - 1);
    int choice = -1;
    while (pick >= 0) {
        choice++;
        if(!ACTIVE_WORLD_OPTIONS["CLASSIC_ZOMBIES"] || mfb(choice) & classic_extras) {
            pick -= embellishments.chances[choice];
        }
    }
    return map_extra(choice);
}

room_type pick_mansion_room(int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2), dy = abs(y1 - y2), area = dx * dy;
    int shortest = (dx < dy ? dx : dy), longest = (dx > dy ? dx : dy);
    std::vector<room_type> valid;
    if (shortest >= 12) {
        valid.push_back(room_mansion_courtyard);
    }
    if (shortest >= 7 && area >= 64 && area <= 100) {
        valid.push_back(room_mansion_bedroom);
    }
    if (shortest >= 9) {
        valid.push_back(room_mansion_library);
    }
    if (shortest >= 6 && longest <= 10) {
        valid.push_back(room_mansion_kitchen);
    }
    if (longest >= 7 && shortest >= 5) {
        valid.push_back(room_mansion_dining);
    }
    if (shortest >= 6 && longest <= 10) {
        valid.push_back(room_mansion_game);
    }
    if (shortest >= 6 && longest <= 10) {
        valid.push_back(room_mansion_study);
    }
    if (shortest >= 10) {
        valid.push_back(room_mansion_pool);
    }
    if (longest <= 6 || shortest <= 4) {
        valid.push_back(room_mansion_bathroom);
    }
    if (longest >= 8 && shortest <= 6) {
        valid.push_back(room_mansion_gallery);
    }

    if (valid.empty()) {
        debugmsg("x: %d - %d, dx: %d\n\
       y: %d - %d, dy: %d", x1, x2, dx,
                 y1, y2, dy);
        return room_null;
    }

    return valid[ rng(0, valid.size() - 1) ];
}

void build_mansion_room(map *m, room_type type, int x1, int y1, int x2, int y2)
{
    int dx = abs(x1 - x2), dy = abs(y1 - y2);
    int cx_low = (x1 + x2) / 2, cx_hi = (x1 + x2 + 1) / 2,
        cy_low = (y1 + y2) / 2, cy_hi = (y1 + y2 + 1) / 2;

    /*
     debugmsg("\
    x: %d - %d, dx: %d cx: %d/%d\n\
    x: %d - %d, dx: %d cx: %d/%d", x1, x2, dx, cx_low, cx_hi,
                                   y1, y2, dy, cy_low, cy_hi);
    */
    bool walled_south = (y2 >= SEEY * 2 - 2);

    switch (type) {

    case room_mansion_courtyard:
        square(m, &grass_or_dirt, x1, y1, x2, y2);
        if (one_in(4)) { // Tree grid
            for (int x = 1; x <= dx / 2; x += 4) {
                for (int y = 1; y <= dx / 2; y += 4) {
                    m->ter_set(x1 + x, y1 + y, t_tree);
                    m->ter_set(x2 - x, y2 - y, t_tree);
                }
            }
        }
        if (one_in(3)) { // shrub-lined
            for (int i = x1; i <= x2; i++) {
                if (m->ter(i, y2 + 1) != t_door_c) {
                    m->ter_set(i, y2, t_shrub);
                }
            }
            if (walled_south && x1 <= SEEX && SEEX <= x2) {
                m->ter_set(SEEX - 1, y2, grass_or_dirt());
                m->ter_set(SEEX,     y2, grass_or_dirt());
            }
        }
        break;

    case room_mansion_entry:
        if (!one_in(3)) { // Columns
            for (int y = y1 + 2; y <= y2; y += 3) {
                m->ter_set(cx_low - 3, y, t_column);
                m->ter_set(cx_low + 3, y, t_column);
            }
        }
        if (one_in(6)) { // Suits of armor
            int start = y1 + rng(2, 4), end = y2 - rng(0, 4), step = rng(3, 6);
            for (int y = start; y <= end; y += step) {
                m->spawn_item(x1 + 1, y, "helmet_plate");
                m->spawn_item(x1 + 1, y, "armor_plate");
                if (one_in(2)) {
                    m->spawn_item(x1 + 1, y, "pike");
                } else if (one_in(3)) {
                    m->spawn_item(x1 + 1, y, "broadsword");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "mace");
                } else if (one_in(6)) {
                    m->spawn_item(x1 + 1, y, "morningstar");
                }

                m->spawn_item(x2 - 1, y, "helmet_plate");
                m->spawn_item(x2 - 1, y, "armor_plate");
                if (one_in(2)) {
                    m->spawn_item(x2 - 1, y, "pike");
                } else if (one_in(3)) {
                    m->spawn_item(x2 - 1, y, "broadsword");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "mace");
                } else if (one_in(6)) {
                    m->spawn_item(x2 - 1, y, "morningstar");
                }
            }
        }
        break;

    case room_mansion_bedroom:
        if (dx > dy || (dx == dy && one_in(2))) { // horizontal
            if (one_in(2)) { // bed on left
                square_furn(m, f_bed, x1 + 1, cy_low - 1, x1 + 3, cy_low + 1);
            } else { // bed on right
                square_furn(m, f_bed, x2 - 3, cy_low - 1, x2 - 1, cy_low + 1);
            }
            m->furn_set(cx_hi - 2, y1, f_bookcase);
            m->furn_set(cx_hi - 1, y1, f_counter);
            m->ter_set(cx_hi    , y1, t_console_broken);
            m->furn_set(cx_hi + 1, y1, f_counter);
            m->furn_set(cx_hi + 2, y1, f_bookcase);
            m->place_items("bedroom", 60, cx_hi - 2, y1, cx_hi + 2, y1, false, 0);

            m->furn_set(cx_hi - 2, y2, f_dresser);
            m->furn_set(cx_hi - 1, y2, f_dresser);
            m->place_items("dresser", 80, cx_hi - 2, y2, cx_hi - 1, y2, false, 0);
            if (one_in(10)) {
                m->place_items("homeguns", 58, cx_hi - 2, y2, cx_hi - 1, y2, false, 0);
            }

            m->furn_set(cx_hi + 1, y2, f_desk);
            m->place_items("office", 50, cx_hi + 1, y2, cx_hi + 1, y2, false, 0);

            m->furn_set(cx_hi + 2, y2, f_chair);

            m->furn_set(x1, y1, f_indoor_plant);
            m->furn_set(x1, y2, f_indoor_plant);

        } else { // vertical
            if (one_in(2)) { // bed at top
                square_furn(m, f_bed, cx_low - 1, y1 + 1, cx_low + 1, y1 + 3);
            } else { // bed at bottom
                square_furn(m, f_bed, cx_low - 1, y2 - 3, cx_low + 1, y2 - 1);
            }
            m->furn_set(x1, cy_hi - 2, f_bookcase);
            m->furn_set(x1, cy_hi - 1, f_counter);
            m->ter_set(x1, cy_hi, t_console_broken);
            m->furn_set(x1, cy_hi + 1, f_counter);
            m->furn_set(x1, cy_hi + 2, f_bookcase);
            m->place_items("bedroom", 80, x1, cy_hi - 2, x1, cy_hi + 2, false, 0);

            m->furn_set(x2, cy_hi - 2, f_dresser);
            m->furn_set(x2, cy_hi - 1, f_dresser);
            m->place_items("dresser", 80, x2, cy_hi - 2, x2, cy_hi - 1, false, 0);
            if (one_in(10)) {
                m->place_items("homeguns", 58, x2, cy_hi - 2, x2, cy_hi - 1, false, 0);
            }

            m->furn_set(x2, cy_hi + 1, f_desk);
            m->place_items("office", 50, x2, cy_hi + 1, x2, cy_hi + 1, false, 0);

            m->furn_set(x2, cy_hi + 2, f_chair);

            m->furn_set(x1, y2, f_indoor_plant);
            m->furn_set(x2, y2, f_indoor_plant);
        }
        break;

    case room_mansion_library:
        if (dx < dy || (dx == dy && one_in(2))) { // vertically-aligned bookshelves
            for (int x = x1 + 1; x <= cx_low - 2; x += 3) {
                for (int y = y1 + 1; y <= y2 - 3; y += 4) {
                    square_furn(m, f_bookcase, x, y, x + 1, y + 2);
                    m->place_items("novels",    85, x, y, x + 1, y + 2, false, 0);
                    m->place_items("manuals",   62, x, y, x + 1, y + 2, false, 0);
                    m->place_items("textbooks", 40, x, y, x + 1, y + 2, false, 0);
                }
            }
            for (int x = x2 - 1; x >= cx_low + 2; x -= 3) {
                for (int y = y1 + 1; y <= y2 - 3; y += 4) {
                    square_furn(m, f_bookcase, x - 1, y, x, y + 2);
                    m->place_items("novels",    85, x - 1, y, x, y + 2, false, 0);
                    m->place_items("manuals",   62, x - 1, y, x, y + 2, false, 0);
                    m->place_items("textbooks", 40, x - 1, y, x, y + 2, false, 0);
                }
            }
        } else { // horizontally-aligned bookshelves
            for (int y = y1 + 1; y <= cy_low - 2; y += 3) {
                for (int x = x1 + 1; x <= x2 - 3; x += 4) {
                    square_furn(m, f_bookcase, x, y, x + 2, y + 1);
                    m->place_items("novels",    85, x, y, x + 2, y + 1, false, 0);
                    m->place_items("manuals",   62, x, y, x + 2, y + 1, false, 0);
                    m->place_items("textbooks", 40, x, y, x + 2, y + 1, false, 0);
                }
            }
            for (int y = y2 - 1; y >= cy_low + 2; y -= 3) {
                for (int x = x1 + 1; x <= x2 - 3; x += 4) {
                    square_furn(m, f_bookcase, x, y - 1, x + 2, y);
                    m->place_items("novels",    85, x, y - 1, x + 2, y, false, 0);
                    m->place_items("manuals",   62, x, y - 1, x + 2, y, false, 0);
                    m->place_items("textbooks", 40, x, y - 1, x + 2, y, false, 0);
                }
            }
        }
        break;

    case room_mansion_kitchen:
        line_furn(m, f_counter, cx_hi - 2, y1 + 1, cx_hi - 2, y2 - 1);
        line_furn(m, f_counter, cx_hi,     y1 + 1, cx_hi,     y2 - 1);
        m->place_items("kitchen",  60, cx_hi - 2, y1 + 1, cx_hi, y2 - 1, false, 0);

        line_furn(m, f_fridge, cx_hi + 2, y1 + 1, cx_hi + 2, cy_hi - 1);
        m->place_items("fridge",  80, cx_hi + 2, y1 + 1, cx_hi + 2, cy_hi - 1, false, 0);

        m->furn_set(cx_hi + 2, cy_hi, f_oven);

        line_furn(m, f_rack, cx_hi + 2, cy_hi + 1, cx_hi + 2, y2 - 1);
        m->place_items("cannedfood",  70, cx_hi + 2, cy_hi + 1, cx_hi + 2, y2 - 1, false, 0);
        m->place_items("pasta",  70, cx_hi + 2, cy_hi + 1, cx_hi + 2, y2 - 1, false, 0);
        break;

    case room_mansion_dining:
        if (dx < dy || (dx == dy && one_in(2))) { // vertically-aligned table
            line_furn(m, f_table, cx_low, y1 + 2, cx_low, y2 - 2);
            line_furn(m, f_bench, cx_low - 1, y1 + 2, cx_low - 1, y2 - 2);
            line_furn(m, f_bench, cx_low + 1, y1 + 2, cx_low + 1, y2 - 2);
            m->place_items("dining", 78, cx_low, y1 + 2, cx_low, y2 - 2, false, 0);
        } else { // horizontally-aligned table
            line_furn(m, f_table, x1 + 2, cy_low, x2 - 2, cy_low);
            line_furn(m, f_bench, x1 + 2, cy_low - 1, x2 - 2, cy_low - 1);
            line_furn(m, f_bench, x1 + 2, cy_low + 1, x2 - 2, cy_low + 1);
            m->place_items("dining", 78, x1 + 2, cy_low, x2 - 2, cy_low, false, 0);
        }
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_game:
        if (dx < dy || one_in(2)) { // vertically-aligned table
            square_furn(m, f_pool_table, cx_low, cy_low - 1, cx_low + 1, cy_low + 1);
            m->place_items("pool_table", 80, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                           false, 0);
        } else { // horizontally-aligned table
            square_furn(m, f_pool_table, cx_low - 1, cy_low, cx_low + 1, cy_low + 1);
            m->place_items("pool_table", 80, cx_low - 1, cy_low, cx_low + 1, cy_low + 1,
                           false, 0);
        }

        if (one_in(2)) {
            line_furn(m, f_sofa, x1 + 1, cy_low - 1, x1 + 1, cy_low + 1);
            m->furn_set(x1 + 1, cy_low - 2, f_table);
            m->place_items("coffee_shop", 70, x1 + 1, cy_low + 2, x1 + 1, cy_low + 2, false, 0);
            m->place_items("magazines", 50, x1 + 1, cy_low + 2, x1 + 1, cy_low + 2, false, 0);
            m->furn_set(x1 + 1, cy_low + 2, f_table);
            m->place_items("coffee_shop", 70, x1 + 1, cy_low - 2, x1 + 1, cy_low - 2, false, 0);
            m->place_items("magazines", 70, x1 + 1, cy_low - 2, x1 + 1, cy_low - 2, false, 0);
        } else {
            line_furn(m, f_sofa, cx_low - 1, y1 + 1, cx_low + 1, y1 + 1);
            m->furn_set(cx_low - 2, y1 + 1, f_table);
            m->place_items("coffee_shop", 70, cx_low - 2, y1 + 1, cx_low - 2, y1 + 1, false, 0);
            m->place_items("magazines", 50, cx_low - 2, y1 + 1, cx_low - 2, y1 + 1, false, 0);
            m->furn_set(cx_low + 2, y1 + 1, f_table);
            m->place_items("coffee_shop", 70, cx_low + 2, y1 + 1, cx_low + 2, y1 + 1, false, 0);
            m->place_items("magazines", 70, cx_low + 2, y1 + 1, cx_low + 2, y1 + 1, false, 0);
        }
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_pool:
        square(m, t_water_pool, x1 + 3, y1 + 3, x2 - 3, y2 - 3);

        m->furn_set(rng(x1 + 1, cx_hi - 2), y1 + 2, f_chair);
        m->furn_set(cx_hi, y1 + 2, f_table);
        m->furn_set(rng(x1 + 1, cx_hi + 2), y1 + 2, f_chair);
        m->place_items("magazines", 60, cx_hi, y1 + 2, cx_hi, y1 + 2, false, 0);

        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_study:
        int study_y;
        if (one_in(2)) {
            study_y = y1;
        } else {
            study_y = y2;
        }
        for (int x = x1 + 1; x <= x2 - 1; x++) {
            if (x % 2 == 0) {
                m->furn_set(x, study_y, f_rack);
                if (one_in(3)) {
                    m->place_items("alcohol", 60, x, study_y, x, study_y, false, 0);
                } else if (one_in(3)) {
                    m->place_items("church", 60, x, study_y, x, study_y, false, 0);
                } else {
                    m->place_items("art", 60, x, study_y, x, study_y, false, 0);
                }
            }
        }

        square_furn(m, f_table, cx_low, cy_low - 1, cx_low + 1, cy_low + 1);
        m->place_items("novels", 50, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                       false, 0);
        m->place_items("magazines", 60, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                       false, 0);
        m->place_items("office", 60, cx_low, cy_low - 1, cx_low + 1, cy_low + 1,
                       false, 0);
        if (one_in(2)) {
            m->furn_set(cx_low - 1, rng(cy_low - 1, cy_low + 1), f_chair);
        } else {
            m->furn_set(cx_low + 2, rng(cy_low - 1, cy_low + 1), f_chair);
        }
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);
        break;

    case room_mansion_bathroom:
        m->place_toilet(rng(x1 + 1, cx_hi - 1), rng(y1 + 1, cy_hi - 1));
        m->furn_set( rng(cx_hi + 1, x2 - 1), rng(y1 + 1, cy_hi - 1) , f_bathtub);
        m->furn_set( rng(x1 + 1, cx_hi - 1), rng(cy_hi + 1, y2 - 1) , f_sink);

        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);

        m->place_items("harddrugs", 20, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        m->place_items("softdrugs", 72, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        m->place_items("cleaning",  48, x1 + 1, y1 + 1, x2 - 1, y2 - 1, false, 0);
        break;

    case room_mansion_gallery:

        m->furn_set(x2 + 2, y2 + 2, f_rack);
        m->place_items("medieval", 40, x2 + 2, y2 + 2, x2 + 2, y2 + 2, false, 0);
        m->furn_set(x2 - 2, y2 + 2, f_rack);
        m->place_items("art", 70, x2 - 2, y2 + 2, x2 - 2, y2 + 2, false, 0);
        m->furn_set(x2 + 2, y2 - 2, f_rack);
        m->place_items("art", 70, x2 + 2, y2 - 2, x2 + 2, y2 - 2, false, 0);
        m->furn_set(x2 - 2, y2 - 2, f_rack);
        m->place_items("alcohol", 80, x2 - 2, y2 - 2, x2 - 2, y2 - 2, false, 0);

        square_furn(m, f_table, cx_low - 1, cy_low - 1, cx_low + 1, cy_low + 1);
        m->furn_set(x1, y1, f_indoor_plant);
        m->furn_set(x2, y1, f_indoor_plant);
        m->furn_set(x1, y2, f_indoor_plant);
        m->furn_set(x2, y2, f_indoor_plant);

        break;
    default:
        break;
    }
}

void mansion_room(map *m, int x1, int y1, int x2, int y2)
{
    room_type type = pick_mansion_room(x1, y1, x2, y2);
    build_mansion_room(m, type, x1, y1, x2, y2);
}

void map::add_extra(map_extra type, game *g)
{
    item body;
    body.make_corpse(itypes["corpse"], GetMType("mon_null"), g->turn);

    switch (type) {

    case mx_null:
        debugmsg("Tried to generate null map extra.");
        break;

    case mx_helicopter: {
        int cx = rng(4, SEEX * 2 - 5), cy = rng(4, SEEY * 2 - 5);
        for (int x = 0; x < SEEX * 2; x++) {
            for (int y = 0; y < SEEY * 2; y++) {
                if (x >= cx - 4 && x <= cx + 4 && y >= cy - 4 && y <= cy + 4) {
                    if (!one_in(5)) {
                        ter_set(x, y, t_wreckage);
                    } else if (has_flag("BASHABLE", x, y)) {
                        std::string junk;
                        bash(x, y, 500, junk); // Smash the fuck out of it
                        bash(x, y, 500, junk); // Smash the fuck out of it some more
                    }
                } else if (one_in(10)) { // 1 in 10 chance of being wreckage anyway
                    ter_set(x, y, t_wreckage);
                }
            }
        }

        spawn_item(rng(5, 18), rng(5, 18), "black_box");
        place_items("helicopter", 90, cx - 4, cy - 4, cx + 4, cy + 4, true, 0);
        place_items("helicopter", 20, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
        items_location extra_items = "helicopter";
        switch (rng(1, 4)) {
        case 1:
            extra_items = "military";
            break;
        case 2:
            extra_items = "science";
            break;
        case 3:
            extra_items = "allguns";
            break;
        case 4:
            extra_items = "bionics";
            break;
        }
        place_spawns(g, "GROUP_MAYBE_MIL", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, 0.1f);//0.1 = 1-5
        place_items(extra_items, 70, cx - 4, cy - 4, cx + 4, cy + 4, true, 0);
    }
    break;

    case mx_military: {
        int num_bodies = dice(2, 6);
        for (int i = 0; i < num_bodies; i++) {
            int x, y, tries = 0;;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(10)) { 
                    add_spawn("mon_zombie_soldier", 1, x, y);
                } else {
                    add_item(x, y, body);
                    spawn_item(x, y, "pants_army");
                    spawn_item(x, y, "boots_combat");
                    place_items("mil_armor_torso", 40, x, y, x, y, true, 0);
                    place_items("mil_armor_helmet", 30, x, y, x, y, true, 0);
                    place_items("military", 86, x, y, x, y, true, 0);
                    if( one_in(8) ) {
                        spawn_item( x, y, "id_military" );
                    }
                    place_items( one_in(2) ? "male_underwear" : "female_underwear",
                                 40, x, y, x, y, true, 0 );
                }
            }
            
        }
        place_spawns(g, "GROUP_MAYBE_MIL", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1,
                     0.1f);//0.1 = 1-5
        place_items("rare", 25, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    }
    break;

    case mx_science: {
        int num_bodies = dice(2, 5);
        for (int i = 0; i < num_bodies; i++) {
            int x, y, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(10)) { 
                    add_spawn("mon_zombie_scientist", 1, x, y);
                } else {
                    add_item(x, y, body);
                    spawn_item(x, y, "coat_lab");
                    if (one_in(2)) {
                        spawn_item(x, y, "id_science");
                    }
                    place_items("science", 84, x, y, x, y, true, 0);
                    place_items("lab_pants", 50, x, y, x, y, true, 0);
                    place_items("lab_shoes", 50, x, y, x, y, true, 0);
                    place_items("lab_torso", 40, x, y, x, y, true, 0);
                    place_items( one_in(2) ? "male_underwear" : "female_underwear",
                                 50, x, y, x, y, true, 0 );
                }
            }
        }
        place_items("rare", 45, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
    }
    break;

    case mx_stash: {
        int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
        if (move_cost(x, y) != 0) {
            ter_set(x, y, t_dirt);
        }

        int size = 0;
        items_location stash;
        switch (rng(1, 6)) { // What kind of stash?
        case 1:
            stash = "stash_food";
            size = 90;
            break;
        case 2:
            stash = "stash_ammo";
            size = 80;
            break;
        case 3:
            stash = "rare";
            size = 70;
            break;
        case 4:
            stash = "stash_wood";
            size = 90;
            break;
        case 5:
            stash = "stash_drugs";
            size = 85;
            break;
        case 6:
            stash = "trash";
            size = 92;
            break;
        }

        if (move_cost(x, y) == 0) {
            ter_set(x, y, t_dirt);
        }
        place_items(stash, size, x, y, x, y, true, 0);

        // Now add traps around that stash
        for (int i = x - 4; i <= x + 4; i++) {
            for (int j = y - 4; j <= y + 4; j++) {
                if (i >= 0 && j >= 0 && i < SEEX * 2 && j < SEEY * 2 && one_in(4)) {
                    trap_id placed = tr_null;
                    switch (rng(1, 7)) {
                    case 1:
                    case 2:
                    case 3:
                        placed = tr_beartrap;
                        break;
                    case 4:
                        placed = tr_caltrops;
                        break;
                    case 5:
                        placed = tr_nailboard;
                        break;
                    case 6:
                        placed = tr_crossbow;
                        break;
                    case 7:
                        placed = tr_shotgun_2;
                        break;
                    }
                    if (placed == tr_beartrap && has_flag("DIGGABLE", i, j)) {
                        if (one_in(8)) {
                            placed = tr_landmine_buried;
                        } else {
                            placed = tr_beartrap_buried;
                        }
                    }
                    add_trap(i, j,  placed);
                }
            }
        }
    }
    break;

    case mx_drugdeal: {
        // Decide on a drug type
        int num_drugs = 0;
        itype_id drugtype;
        switch (rng(1, 10)) {
        case 1: // Weed
            num_drugs = rng(20, 30);
            drugtype = "weed";
            break;
        case 2:
        case 3:
        case 4:
        case 5: // Cocaine
            num_drugs = rng(10, 20);
            drugtype = "coke";
            break;
        case 6:
        case 7:
        case 8: // Meth
            num_drugs = rng(8, 14);
            drugtype = "meth";
            break;
        case 9:
        case 10: // Heroin
            num_drugs = rng(6, 12);
            drugtype = "heroin";
            break;
        }
        int num_bodies_a = dice(3, 3);
        int num_bodies_b = dice(3, 3);
        bool north_south = one_in(2);
        bool a_has_drugs = one_in(2);

        for (int i = 0; i < num_bodies_a; i++) {
            int x, y, x_offset, y_offset, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                if (north_south) {
                    x = rng(0, SEEX * 2 - 1);
                    y = rng(0, SEEY - 4);
                    x_offset = 0;
                    y_offset = -1;
                } else {
                    x = rng(0, SEEX - 4);
                    y = rng(0, SEEY * 2 - 1);
                    x_offset = -1;
                    y_offset = 0;
                }
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(10)) { 
                    add_spawn("mon_zombie_spitter", 1, x, y);
                } else {
                    add_item(x, y, body);
                    int splatter_range = rng(1, 3);
                    for (int j = 0; j <= splatter_range; j++) {
                        add_field(g, x + (j * x_offset), y + (j * y_offset),
                                  fd_blood, 1);
                    }
                    place_items("drugdealer", 75, x, y, x, y, true, 0);
                    spawn_item(x, y, "pants_cargo");
                    place_items("lab_shoes", 50, x, y, x, y, true, 0);
                    place_items("shirts", 50, x, y, x, y, true, 0);
                    place_items("jackets", 30, x, y, x, y, true, 0);
                    place_items( one_in(2) ? "male_underwear" : "female_underwear",
                                 40, x, y, x, y, true, 0 );
                }
                if (a_has_drugs && num_drugs > 0) {
                    int drugs_placed = rng(2, 6);
                    if (drugs_placed > num_drugs) {
                        drugs_placed = num_drugs;
                        num_drugs = 0;
                    }
                    spawn_item(x, y, drugtype, 0, drugs_placed);
                }
            }
        }
        for (int i = 0; i < num_bodies_b; i++) {
            int x, y, x_offset, y_offset, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                if (north_south) {
                    x = rng(0, SEEX * 2 - 1);
                    y = rng(SEEY + 3, SEEY * 2 - 1);
                    x_offset = 0;
                    y_offset = 1;
                } else {
                    x = rng(SEEX + 3, SEEX * 2 - 1);
                    y = rng(0, SEEY * 2 - 1);
                    x_offset = 1;
                    y_offset = 0;
                }
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);

            if (tries < 10) { // We found a valid spot!
                if (one_in(20)) { 
                    add_spawn("mon_zombie_smoker", 1, x, y);
                } else {
                    add_item(x, y, body);
                    int splatter_range = rng(1, 3);
                    for (int j = 0; j <= splatter_range; j++) {
                        add_field( g, x + (j * x_offset), y + (j * y_offset),
                                   fd_blood, 1 );
                    }
                    place_items("drugdealer", 75, x, y, x, y, true, 0);
                    spawn_item(x, y, "pants_cargo");
                    place_items("lab_shoes", 50, x, y, x, y, true, 0);
                    place_items("shirts", 50, x, y, x, y, true, 0);
                    place_items("jackets", 25, x, y, x, y, true, 0);
                    place_items( one_in(2) ? "male_underwear" : "female_underwear",
                                 40, x, y, x, y, true, 0 );
                    if (!a_has_drugs && num_drugs > 0) {
                        int drugs_placed = rng(2, 6);
                        if (drugs_placed > num_drugs) {
                            drugs_placed = num_drugs;
                            num_drugs = 0;
                        }
                        spawn_item(x, y, drugtype, 0, drugs_placed);
                    }
                }
            }
        }
    }
    break;

    case mx_supplydrop: {
        int num_crates = rng(1, 5);
        for (int i = 0; i < num_crates; i++) {
            int x, y, tries = 0;
            do { // Loop until we find a valid spot to dump a body, or we give up
                x = rng(0, SEEX * 2 - 1);
                y = rng(0, SEEY * 2 - 1);
                tries++;
            } while (tries < 10 && move_cost(x, y) == 0);
            furn_set(x, y, f_crate_c);
            switch (rng(1, 10)) {
            case 1:
            case 2:
            case 3:
            case 4:
                place_items("mil_food", 88, x, y, x, y, true, 0);
                break;
            case 5:
            case 6:
            case 7:
                place_items("grenades", 82, x, y, x, y, true, 0);
                break;
            case 8:
            case 9:
                place_items("mil_armor", 75, x, y, x, y, true, 0);
                break;
            case 10:
                place_items("mil_rifles", 80, x, y, x, y, true, 0);
                break;
            }
        }
    }
    break;

    case mx_portal: {
        std::string spawncreatures[5] = {"mon_gelatin", "mon_flaming_eye",
                                         "mon_kreck", "mon_gracke", "mon_blank"};
        int x = rng(1, SEEX * 2 - 2), y = rng(1, SEEY * 2 - 2);
        for (int i = x - 1; i <= x + 1; i++) {
            for (int j = y - 1; j <= y + 1; j++) {
                ter_set(i, j, t_rubble);
            }
        }
        add_trap(x, y, tr_portal);
        int num_monsters = rng(0, 4);
        for (int i = 0; i < num_monsters; i++) {
            std::string type = spawncreatures[( rng(0, 4) )];
            int mx = rng(1, SEEX * 2 - 2), my = rng(1, SEEY * 2 - 2);
            ter_set(mx, my, t_rubble);
            add_spawn(type, 1, mx, my);
        }
    }
    break;

    case mx_minefield: {
        int num_mines = rng(6, 20);
        for (int x = 0; x < SEEX * 2; x++) {
            for (int y = 0; y < SEEY * 2; y++) {
                if (one_in(3)) {
                    ter_set(x, y, t_dirt);
                }
            }
        }
        for (int i = 0; i < num_mines; i++) {
            int x = rng(0, SEEX * 2 - 1), y = rng(0, SEEY * 2 - 1);
            if (!has_flag("DIGGABLE", x, y) || one_in(8)) {
                ter_set(x, y, t_dirtmound);
            }
            add_trap(x, y, tr_landmine_buried);
        }
    }
    break;

    case mx_crater: {
        int size = rng(2, 6);
        int size_squared = size * size;
        int x = rng(size, SEEX * 2 - 1 - size), y = rng(size, SEEY * 2 - 1 - size);
        for (int i = x - size; i <= x + size; i++) {
            for (int j = y - size; j <= y + size; j++) {
                //If we're using circular distances, make circular craters
                //Pythagoras to the rescue, x^2 + y^2 = hypotenuse^2
                if(!trigdist || (((i - x) * (i - x) + (j - y) * (j - y)) <= size_squared)) {
                    destroy(g, i, j, false);
                    radiation(i, j) += rng(20, 40);
                }
            }
        }
    }
    break;

    case mx_fumarole: {
        int x1 = rng(0,    SEEX     - 1), y1 = rng(0,    SEEY     - 1),
            x2 = rng(SEEX, SEEX * 2 - 1), y2 = rng(SEEY, SEEY * 2 - 1);
        std::vector<point> fumarole = line_to(x1, y1, x2, y2, 0);
        for (int i = 0; i < fumarole.size(); i++) {
            ter_set(fumarole[i].x, fumarole[i].y, t_lava);
        }
    }
    break;

    case mx_portal_in: {
        std::string monids[5] = {"mon_gelatin", "mon_flaming_eye", "mon_kreck", "mon_gracke", "mon_blank"};
        int x = rng(5, SEEX * 2 - 6), y = rng(5, SEEY * 2 - 6);
        add_field(g, x, y, fd_fatigue, 3);
        for (int i = x - 5; i <= x + 5; i++) {
            for (int j = y - 5; j <= y + 5; j++) {
                if (rng(1, 9) >= trig_dist(x, y, i, j)) {
                    marlossify(i, j);
                    if (one_in(15)) {
                        monster creature(GetMType(monids[rng(0, 5)]));
                        creature.spawn(i, j);
                        g->add_zombie(creature);
                    }
                }
            }
        }
    }
    break;

    case mx_anomaly: {
        point center( rng(6, SEEX * 2 - 7), rng(6, SEEY * 2 - 7) );
        artifact_natural_property prop =
            artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1));
        create_anomaly(center.x, center.y, prop);
        spawn_artifact(center.x, center.y, new_natural_artifact(itypes, prop), 0);
    }
    break;

    default:
        break;
    }
}

void map::create_anomaly(int cx, int cy, artifact_natural_property prop)
{
    rough_circle(this, t_rubble, cx, cy, 5);
    switch (prop) {
    case ARTPROP_WRIGGLING:
    case ARTPROP_MOVING:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble) {
                    add_field(NULL, i, j, fd_push_items, 1);
                    if (one_in(3)) {
                        spawn_item(i, j, "rock");
                    }
                }
            }
        }
        break;

    case ARTPROP_GLOWING:
    case ARTPROP_GLITTERING:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble && one_in(2)) {
                    add_trap(i, j, tr_glow);
                }
            }
        }
        break;

    case ARTPROP_HUMMING:
    case ARTPROP_RATTLING:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble && one_in(2)) {
                    add_trap(i, j, tr_hum);
                }
            }
        }
        break;

    case ARTPROP_WHISPERING:
    case ARTPROP_ENGRAVED:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble && one_in(3)) {
                    add_trap(i, j, tr_shadow);
                }
            }
        }
        break;

    case ARTPROP_BREATHING:
        for (int i = cx - 1; i <= cx + 1; i++) {
            for (int j = cy - 1; j <= cy + 1; j++)
                if (i == cx && j == cy) {
                    add_spawn("mon_breather_hub", 1, i, j);
                } else {
                    add_spawn("mon_breather", 1, i, j);
                }
        }
        break;

    case ARTPROP_DEAD:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble) {
                    add_trap(i, j, tr_drain);
                }
            }
        }
        break;

    case ARTPROP_ITCHY:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble) {
                    radiation(i, j) = rng(0, 10);
                }
            }
        }
        break;

    case ARTPROP_ELECTRIC:
    case ARTPROP_CRACKLING:
        add_field(NULL, cx, cy, fd_shock_vent, 3);
        break;

    case ARTPROP_SLIMY:
        add_field(NULL, cx, cy, fd_acid_vent, 3);
        break;

    case ARTPROP_WARM:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble) {
                    add_field(NULL, i, j, fd_fire_vent, 1 + (rl_dist(cx, cy, i, j) % 3));
                }
            }
        }
        break;

    case ARTPROP_SCALED:
        for (int i = cx - 5; i <= cx + 5; i++) {
            for (int j = cy - 5; j <= cy + 5; j++) {
                if (ter(i, j) == t_rubble) {
                    add_trap(i, j, tr_snake);
                }
            }
        }
        break;

    case ARTPROP_FRACTAL:
        create_anomaly(cx - 4, cy - 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        create_anomaly(cx + 4, cy - 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        create_anomaly(cx - 4, cy + 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        create_anomaly(cx + 4, cy - 4,
                       artifact_natural_property(rng(ARTPROP_NULL + 1, ARTPROP_MAX - 1)));
        break;
    default:
        break;
    }
}

void line(map *m, ter_id type, int x1, int y1, int x2, int y2)
{
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (int i = 0; i < line.size(); i++) {
        m->ter_set(line[i].x, line[i].y, type);
    }
    m->ter_set(x1, y1, type);
}

void line_furn(map *m, furn_id type, int x1, int y1, int x2, int y2)
{
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (int i = 0; i < line.size(); i++) {
        m->furn_set(line[i].x, line[i].y, type);
    }
    m->furn_set(x1, y1, type);
}

void fill_background(map *m, ter_id type)
{
    square(m, type, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);
}

void fill_background(map *m, ter_id (*f)())
{
    square(m, f, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1);
}

void square(map *m, ter_id type, int x1, int y1, int x2, int y2)
{
    for (int x = x1; x <= x2; x++)
        for (int y = y1; y <= y2; y++) {
            m->ter_set(x, y, type);
        }
}

void square_furn(map *m, furn_id type, int x1, int y1, int x2, int y2)
{
    for (int x = x1; x <= x2; x++)
        for (int y = y1; y <= y2; y++) {
            m->furn_set(x, y, type);
        }
}

void square(map *m, ter_id (*f)(), int x1, int y1, int x2, int y2)
{
    for (int x = x1; x <= x2; x++)
        for (int y = y1; y <= y2; y++) {
            m->ter_set(x, y, f());
        }
}

void rough_circle(map *m, ter_id type, int x, int y, int rad)
{
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (rl_dist(x, y, i, j) + rng(0, 3) <= rad) {
                m->ter_set(i, j, type);
            }
        }
    }
}

void add_corpse(game *g, map *m, int x, int y)
{
    item body;
    body.make_corpse(itypes["corpse"], GetMType("mon_null"), 0);
    m->add_item_or_charges(x, y, body);
    m->put_items_from("shoes",  1, x, y, 0, 0, 0);
    m->put_items_from("pants",  1, x, y, 0, 0, 0);
    m->put_items_from("shirts", 1, x, y, 0, 0, 0);
    if (one_in(6)) {
        m->put_items_from("jackets", 1, x, y, 0, 0, 0);
    }
    if (one_in(15)) {
        m->put_items_from("bags", 1, x, y, 0, 0, 0);
    }
}

/**
 * Adds vehicles to the current submap, selected from a random weighted
 * distribution of possible vehicles. If the road has a pavement, then set the
 * 'city' flag to true to spawn wrecks. If it doesn't (ie, highway or country
 * road,) then set 'city' to false to spawn far fewer vehicles that are out
 * of gas instead of wrecked.
 * @param city Whether or not to spawn city wrecks.
 * @param facing The direction the spawned car should face (multiple of 90).
 */
void map::add_road_vehicles(bool city, int facing)
{
    if (city) {
        int spawn_type = rng(0, 100);
        if(spawn_type <= 33) {
            //Randomly-distributed wrecks
            int maxwrecks = rng(1, 3);
            for (int nv = 0; nv < maxwrecks; nv++) {
                int vx = rng(0, 19);
                int vy = rng(0, 19);
                int car_type = rng(1, 100);
                if (car_type <= 25) {
                    add_vehicle(g, "car", vx, vy, facing, -1, 1);
                } else if (car_type <= 30) {
                    add_vehicle(g, "policecar", vx, vy, facing, -1, 1);
                } else if (car_type <= 40) {
                    add_vehicle(g, "ambulance", vx, vy, facing, -1, 1);
                } else if (car_type <= 45) {
                    add_vehicle(g, "beetle", vx, vy, facing, -1, 1);
                } else if (car_type <= 50) {
                    add_vehicle(g, "scooter", vx, vy, facing, -1, 1);
                } else if (car_type <= 55) {
                    add_vehicle(g, "motorcycle", vx, vy, facing, -1, 1);
                } else if (car_type <= 65) {
                    add_vehicle(g, "hippie_van", vx, vy, facing, -1, 1);
                } else if (car_type <= 70) {
                    add_vehicle(g, "cube_van", vx, vy, facing, -1, 1);
                } else if (car_type <= 80) {
                    add_vehicle(g, "electric_car", vx, vy, facing, -1, 1);
                } else if (car_type <= 90) {
                    add_vehicle(g, "flatbed_truck", vx, vy, facing, -1, 1);
                } else if (car_type <= 95) {
                    add_vehicle(g, "rv", vx, vy, facing, -1, 1);
                } else {
                    add_vehicle(g, "motorcycle_sidecart", vx, vy, facing, -1, 1);
                }
            }
        } else if(spawn_type <= 66) {
            //Parked vehicles
            int veh_x = 0;
            int veh_y = 0;
            if(facing == 0) {
                veh_x = rng(4, 16);
                veh_y = 17;
            } else if(facing == 90) {
                veh_x = 6;
                veh_y = rng(4, 16);
            } else if(facing == 180) {
                veh_x = rng(4, 16);
                veh_y = 6;
            } else if(facing == 270) {
                veh_x = 17;
                veh_y = rng(4, 16);
            }
            int veh_type = rng(0, 100);
            if(veh_type <= 70) {
                add_vehicle(g, "car", veh_x, veh_y, facing, -1, 1);
            } else if(veh_type <= 95) {
                add_vehicle(g, "electric_car", veh_x, veh_y, facing, -1, 1);
            } else {
                add_vehicle(g, "policecar", veh_x, veh_y, facing, -1, 1);
            }
        } else if(spawn_type <= 99) {
            //Totally clear section of road
            return;
        } else {
            //Road-blocking obstacle of some kind.
            int block_type = rng(0, 100);
            if(block_type <= 75) {
                //Jack-knifed semi
                int semi_x = 0;
                int semi_y = 0;
                int trailer_x = 0;
                int trailer_y = 0;
                if(facing == 0) {
                    semi_x = rng(0, 16);
                    semi_y = rng(14, 16);
                    trailer_x = semi_x + 4;
                    trailer_y = semi_y - 10;
                } else if(facing == 90) {
                    semi_x = rng(0, 8);
                    semi_y = rng(4, 15);
                    trailer_x = semi_x + 12;
                    trailer_y = semi_y + 1;
                } else if(facing == 180) {
                    semi_x = rng(4, 16);
                    semi_y = rng(4, 6);
                    trailer_x = semi_x - 4;
                    trailer_y = semi_y + 10;
                } else {
                    semi_x = rng(12, 20);
                    semi_y = rng(5, 16);
                    trailer_x = semi_x - 12;
                    trailer_y = semi_y - 1;
                }
                add_vehicle(g, "semi_truck", semi_x, semi_y, (facing + 135) % 360, -1, 1);
                add_vehicle(g, "truck_trailer", trailer_x, trailer_y, (facing + 90) % 360, -1, 1);
            } else {
                //Huge pileup of random vehicles
                std::string next_vehicle;
                int num_cars = rng(18, 22);
                bool policecars = block_type >= 95; //Policecar pileup, Blues Brothers style
                vehicle *last_added_car = NULL;
                for(int i = 0; i < num_cars; i++) {
                    if(policecars) {
                        next_vehicle = "policecar";
                    } else {
                        //Random car
                        int car_type = rng(0, 100);
                        if(car_type <= 70) {
                            next_vehicle = "car";
                        } else if(car_type <= 90) {
                            next_vehicle = "flatbed_truck";
                        } else if(car_type <= 95) {
                            next_vehicle = "cube_van";
                        } else {
                            next_vehicle = "hippie_van";
                        }
                    }
                    last_added_car = add_vehicle(g, next_vehicle, rng(4, 16), rng(4, 16), rng(0, 3) * 90, -1, 1);
                }

                //Hopefully by the last one we've got a giant pileup, so name it
                if (last_added_car != NULL) {
                    if(policecars) {
                        last_added_car->name = _("policecar pile-up");
                    } else {
                        last_added_car->name = _("pile-up");
                    }
                }
            }
        }
    } else {
        // spawn regular road out of fuel vehicles
        if (one_in(40)) {
            int vx = rng(8, 16);
            int vy = rng(8, 16);
            int car_type = rng(1, 10);
            if (car_type <= 5) {
                add_vehicle(g, "car", vx, vy, facing, 0, -1);
            } else if (car_type <= 8) {
                add_vehicle(g, "flatbed_truck", vx, vy, facing, 0, -1);
            } else if (car_type <= 9) {
                add_vehicle(g, "semi_truck", vx, vy, facing, 0, -1);
            } else {
                add_vehicle(g, "armored_car", vx, vy, facing, 0, -1);
            }
        }
    }
}
