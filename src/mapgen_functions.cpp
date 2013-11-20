#include "mapgen.h"
#include "mapgen_functions.h"
#include "output.h"
#include "item_factory.h"
#include "line.h"
#include "mapgenformat.h"
#include "overmap.h"

void line(map *m, ter_id type, int x1, int y1, int x2, int y2);
void line_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void fill_background(map *m, ter_id type);
void fill_background(map *m, ter_id (*f)());
void square(map *m, ter_id type, int x1, int y1, int x2, int y2);
void square(map *m, ter_id (*f)(), int x1, int y1, int x2, int y2);
void square_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void rough_circle(map *m, ter_id type, int x, int y, int rad);
void add_corpse(game *g, map *m, int x, int y);

mapgendata::mapgendata(oter_id north, oter_id east, oter_id south, oter_id west, oter_id northeast,
                       oter_id northwest, oter_id southeast, oter_id southwest)
{
    t_nesw[0] = north;
    t_nesw[1] = east;
    t_nesw[2] = south;
    t_nesw[3] = west;
    t_nesw[4] = northeast;
    t_nesw[5] = southeast;
    t_nesw[6] = northwest;
    t_nesw[7] = southwest;
    n_fac = 0;
    e_fac = 0;
    s_fac = 0;
    w_fac = 0;
    ne_fac = 0;
    se_fac = 0;
    nw_fac = 0;
    sw_fac = 0;
}
//
std::map<std::string, building_gen_pointer> mapgen_cfunction_map;

void init_mapgen_builtin_functions() {
    mapgen_cfunction_map.clear();
    mapgen_cfunction_map["null"]             = &mapgen_null;
    mapgen_cfunction_map["crater"]           = &mapgen_crater;
    mapgen_cfunction_map["field"]            = &mapgen_field;
    mapgen_cfunction_map["field_test"]       = &mapgen_field_w_puddles;
    mapgen_cfunction_map["dirtlot"]          = &mapgen_dirtlot;
    mapgen_cfunction_map["forest"]           = &mapgen_forest_general;
    mapgen_cfunction_map["hive"]             = &mapgen_hive;
    mapgen_cfunction_map["spider_pit"]       = &mapgen_spider_pit;
    mapgen_cfunction_map["fungal_bloom"]     = &mapgen_fungal_bloom;
    mapgen_cfunction_map["road_straight"]    = &mapgen_road_straight;
    mapgen_cfunction_map["road_curved"]      = &mapgen_road_curved;
    mapgen_cfunction_map["road_tee"]         = &mapgen_road_tee;
    mapgen_cfunction_map["road_four_way"]    = &mapgen_road_four_way;
    mapgen_cfunction_map["field"]            = &mapgen_field;
    mapgen_cfunction_map["bridge"]           = &mapgen_bridge;
    mapgen_cfunction_map["highway"]          = &mapgen_highway;
    mapgen_cfunction_map["river_curved_not"] = &mapgen_river_curved_not;
    mapgen_cfunction_map["river_straight"]   = &mapgen_river_straight;
    mapgen_cfunction_map["river_curved"]     = &mapgen_river_curved;
    mapgen_cfunction_map["parking_lot"]      = &mapgen_parking_lot;
    mapgen_cfunction_map["pool"]             = &mapgen_pool;
    mapgen_cfunction_map["park_playground"]             = &mapgen_park_playground;
    mapgen_cfunction_map["park_basketball"]             = &mapgen_park_basketball;
    mapgen_cfunction_map["s_gas"]      = &mapgen_gas_station;
    mapgen_cfunction_map["house_generic_boxy"]      = &mapgen_generic_house_boxy;
    mapgen_cfunction_map["house_generic_big_livingroom"]      = &mapgen_generic_house_big_livingroom;
    mapgen_cfunction_map["house_generic_center_hallway"]      = &mapgen_generic_house_center_hallway;
    mapgen_cfunction_map["church_new_england"]             = &mapgen_church_new_england;
    mapgen_cfunction_map["church_gothic"]             = &mapgen_church_gothic;
}

//
void mapgendata::set_dir(int dir_in, int val)
{
    switch (dir_in) {
    case 0:
        n_fac = val;
        break;
    case 1:
        e_fac = val;
        break;
    case 2:
        s_fac = val;
        break;
    case 3:
        w_fac = val;
        break;
    case 4:
        ne_fac = val;
        break;
    case 5:
        se_fac = val;
        break;
    case 6:
        nw_fac = val;
        break;
    case 7:
        sw_fac = val;
        break;
    default:
        debugmsg("Invalid direction for mapgendata::set_dir. dir_in = %d", dir_in);
        break;
    }
}

void mapgendata::fill(int val)
{
    n_fac = val;
    e_fac = val;
    s_fac = val;
    w_fac = val;
    ne_fac = val;
    nw_fac = val;
    se_fac = val;
    sw_fac = val;
}

int& mapgendata::dir(int dir_in)
{
    switch (dir_in) {
    case 0:
        return n_fac;
        break;
    case 1:
        return e_fac;
        break;
    case 2:
        return s_fac;
        break;
    case 3:
        return w_fac;
        break;
    case 4:
        return ne_fac;
        break;
    case 5:
        return se_fac;
        break;
    case 6:
        return nw_fac;
        break;
    case 7:
        return sw_fac;
        break;
    default:
        debugmsg("Invalid direction for mapgendata::set_dir. dir_in = %d", dir_in);
        //return something just so the compiler doesn't freak out. Not really correct, though.
        return n_fac;
        break;
    }
}

ter_id grass_or_dirt()
{
 if (one_in(4))
  return t_grass;
 return t_dirt;
}

ter_id dirt_or_pile()
{
 if (one_in(4))
  return t_dirtmound;
 return t_dirt;
}

// helper functions below

void mapgen_null(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    debugmsg("Generating null terrain, please report this as a bug");
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            m->ter_set(i, j, t_null);
            m->radiation(i, j) = 0;
        }
    }
}

void mapgen_crater(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for(int i = 0; i < 4; i++) {
        if(dat.t_nesw[i] != "crater") {
            dat.set_dir(i, 6);
        }
    }

    for (int i = 0; i < SEEX * 2; i++) {
       for (int j = 0; j < SEEY * 2; j++) {
           if (rng(0, dat.w_fac) <= i && rng(0, dat.e_fac) <= SEEX * 2 - 1 - i &&
               rng(0, dat.n_fac) <= j && rng(0, dat.s_fac) <= SEEX * 2 - 1 - j ) {
               m->ter_set(i, j, t_rubble);
               m->radiation(i, j) = rng(0, 4) * rng(0, 2);
           } else {
               m->ter_set(i, j, t_dirt);
               m->radiation(i, j) = rng(0, 2) * rng(0, 2) * rng(0, 2);
            }
        }
    }
    m->place_items("wreckage", 83, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, 0);
}

void mapgen_field(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    // There's a chance this field will be thick with strawberry
    // and blueberry bushes.
    int berry_bush_factor = 200;
    int bush_factor = 120;
    if(one_in(120)) {
        berry_bush_factor = 2;
        bush_factor = 40;
    }
    ter_id bush_type;
    if (one_in(2)) {
        bush_type = t_shrub_blueberry;
    } else {
        bush_type = t_shrub_strawberry;
    }

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            m->ter_set(i, j, grass_or_dirt());
            if (one_in(bush_factor)) {
                if (one_in(berry_bush_factor)) {
                    m->ter_set(i, j, bush_type);
                } else {
                    m->ter_set(i, j, t_shrub);
                }
            }
            else {
                if (one_in(1000)) {
                    m->furn_set(i,j, f_mutpoppy);
                }
            }
        }
    }
    m->place_items("field", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn);
}


void mapgen_field_w_puddles(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    mapgen_field(m, terrain_type, dat, turn, density);
    for ( int x = 0; x < 24; x++ ) {
        for ( int y = 0; y < 24; y++ ) {
            if ( one_in(5) ) {
                 m->ter_set(x, y, t_water_sh);
            }
        }
    }
}

void mapgen_dirtlot(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            m->ter_set(i, j, t_dirt);
            if (one_in(120)) {
                m->ter_set(i, j, t_pit_shallow);
            } else if (one_in(50)) {
                m->ter_set(i,j, t_grass);
            }
        }
    }
    if (one_in(4)) {
        m->add_vehicle (g, "flatbed_truck", 12, 12, 90, -1, -1);
    }
}

void mapgen_forest_general(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    if (terrain_type == "forest_thick") {
        dat.fill(8);
    } else if (terrain_type == "forest_water") {
        dat.fill(4);
    } else if (terrain_type == "forest") {
        dat.fill(0);
    }
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] == "forest" || dat.t_nesw[i] == "forest_water") {
            dat.dir(i) += 14;
        } else if (dat.t_nesw[i] == "forest_thick") {
            dat.dir(i) += 18;
        }
    }
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            int forest_chance = 0, num = 0;
            if (j < dat.n_fac) {
                forest_chance += dat.n_fac - j;
                num++;
            }
            if (SEEX * 2 - 1 - i < dat.e_fac) {
                forest_chance += dat.e_fac - (SEEX * 2 - 1 - i);
                num++;
            }
            if (SEEY * 2 - 1 - j < dat.s_fac) {
                forest_chance += dat.s_fac - (SEEY * 2 - 1 - j);
                num++;
            }
            if (i < dat.w_fac) {
                forest_chance += dat.w_fac - i;
                num++;
            }
            if (num > 0) {
                forest_chance /= num;
            }
            int rn = rng(0, forest_chance);
            if ((forest_chance > 0 && rn > 13) || one_in(100 - forest_chance)) {
                if (one_in(250)) {
                    m->ter_set(i, j, t_tree_apple);
                    m->spawn_item(i, j, "apple", 1, 0, turn);
                } else {
                    m->ter_set(i, j, t_tree);
                }
            } else if ((forest_chance > 0 && rn > 10) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_tree_young);
            } else if ((forest_chance > 0 && rn >  9) || one_in(100 - forest_chance)) {
                if (one_in(250)) {
                    m->ter_set(i, j, t_shrub_blueberry);
                } else {
                    m->ter_set(i, j, t_underbrush);
                }
            } else {
                m->ter_set(i, j, grass_or_dirt());
            }
        }
    }
    m->place_items("forest", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn);

    if (terrain_type == "forest_water") {
        // Reset *_fac to handle where to place water
        for (int i = 0; i < 8; i++) {
            if (dat.t_nesw[i] == "forest_water") {
                dat.set_dir(i, 2);
            } else if (is_ot_type("river", dat.t_nesw[i])) {
                dat.set_dir(i, 3);
            } else if (dat.t_nesw[i] == "forest" || dat.t_nesw[i] == "forest_thick") {
                dat.set_dir(i, 1);
            } else {
                dat.set_dir(i, 0);
            }
        }
        int x = SEEX / 2 + rng(0, SEEX), y = SEEY / 2 + rng(0, SEEY);
        for (int i = 0; i < 20; i++) {
            if (x >= 0 && x < SEEX * 2 && y >= 0 && y < SEEY * 2) {
                if (m->ter(x, y) == t_water_sh) {
                    m->ter_set(x, y, t_water_dp);
                } else if (m->ter(x, y) == t_dirt || m->ter(x, y) == t_grass ||
                         m->ter(x, y) == t_underbrush) {
                    m->ter_set(x, y, t_water_sh);
                }
            } else {
                i = 20;
            }
            x += rng(-2, 2);
            y += rng(-2, 2);
            if (x < 0 || x >= SEEX * 2) {
                x = SEEX / 2 + rng(0, SEEX);
            }
            if (y < 0 || y >= SEEY * 2) {
                y = SEEY / 2 + rng(0, SEEY);
            }
            int factor = dat.n_fac + (dat.ne_fac / 2) + (dat.nw_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX * 2 -1), wy = rng(0, SEEY - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_grass ||
                    m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            factor = dat.e_fac + (dat.ne_fac / 2) + (dat.se_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(SEEX, SEEX * 2 - 1), wy = rng(0, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_grass ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            factor = dat.s_fac + (dat.se_fac / 2) + (dat.sw_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX * 2 - 1), wy = rng(SEEY, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_grass ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            factor = dat.w_fac + (dat.nw_fac / 2) + (dat.sw_fac / 2);
            for (int j = 0; j < factor; j++) {
                int wx = rng(0, SEEX - 1), wy = rng(0, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_grass ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
        }
        int rn = rng(0, 2) * rng(0, 1) * (rng(0, 1) + rng(0, 1));// Good chance of 0
        for (int i = 0; i < rn; i++) {
            x = rng(0, SEEX * 2 - 1);
            y = rng(0, SEEY * 2 - 1);
            m->add_trap(x, y, tr_sinkhole);
            if (m->ter(x, y) != t_water_sh) {
                m->ter_set(x, y, grass_or_dirt());
            }
        }
    }

    //1-2 per overmap, very bad day for low level characters
    if (one_in(10000)) {
        m->add_spawn("mon_jabberwock", 1, SEEX, SEEY);
    }

    //Very rare easter egg, ~1 per 10 overmaps
    if (one_in(1000000)) {
        m->add_spawn("mon_shia", 1, SEEX, SEEY);
    }


    // One in 100 forests has a spider living in it :o
    if (one_in(100)) {
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEX * 2; j++) {
                if ((m->ter(i, j) == t_dirt || m->ter(i, j) == t_grass ||
                     m->ter(i, j) == t_underbrush) && !one_in(3)) {
                    m->add_field(NULL, i, j, fd_web, rng(1, 3));
                }
            }
        }
        m->add_spawn("mon_spider_web", rng(1, 2), SEEX, SEEY);
    }
}

void mapgen_hive(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    // Start with a basic forest pattern
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            int rn = rng(0, 14);
            if (rn > 13) {
                m->ter_set(i, j, t_tree);
            } else if (rn > 11) {
                m->ter_set(i, j, t_tree_young);
            } else if (rn > 10) {
                m->ter_set(i, j, t_underbrush);
            } else {
                m->ter_set(i, j, t_dirt);
            }
        }
    }

    // j and i loop through appropriate hive-cell center squares
    for (int j = 5; j < SEEY * 2 - 5; j += 6) {
        for (int i = (j == 5 || j == 17 ? 3 : 6); i < SEEX * 2 - 5; i += 6) {
            if (!one_in(8)) {
                // Caps are always there
                m->ter_set(i    , j - 5, t_wax);
                m->ter_set(i    , j + 5, t_wax);
                for (int k = -2; k <= 2; k++) {
                    for (int l = -1; l <= 1; l++) {
                        m->ter_set(i + k, j + l, t_floor_wax);
                    }
                }
                m->add_spawn("mon_bee", 2, i, j);
                m->add_spawn("mon_beekeeper", 1, i, j);
                m->ter_set(i    , j - 3, t_floor_wax);
                m->ter_set(i    , j + 3, t_floor_wax);
                m->ter_set(i - 1, j - 2, t_floor_wax);
                m->ter_set(i    , j - 2, t_floor_wax);
                m->ter_set(i + 1, j - 2, t_floor_wax);
                m->ter_set(i - 1, j + 2, t_floor_wax);
                m->ter_set(i    , j + 2, t_floor_wax);
                m->ter_set(i + 1, j + 2, t_floor_wax);

                // Up to two of these get skipped; an entrance to the cell
                int skip1 = rng(0, 23);
                int skip2 = rng(0, 23);

                m->ter_set(i - 1, j - 4, t_wax);
                m->ter_set(i    , j - 4, t_wax);
                m->ter_set(i + 1, j - 4, t_wax);
                m->ter_set(i - 2, j - 3, t_wax);
                m->ter_set(i - 1, j - 3, t_wax);
                m->ter_set(i + 1, j - 3, t_wax);
                m->ter_set(i + 2, j - 3, t_wax);
                m->ter_set(i - 3, j - 2, t_wax);
                m->ter_set(i - 2, j - 2, t_wax);
                m->ter_set(i + 2, j - 2, t_wax);
                m->ter_set(i + 3, j - 2, t_wax);
                m->ter_set(i - 3, j - 1, t_wax);
                m->ter_set(i - 3, j    , t_wax);
                m->ter_set(i - 3, j - 1, t_wax);
                m->ter_set(i - 3, j + 1, t_wax);
                m->ter_set(i - 3, j    , t_wax);
                m->ter_set(i - 3, j + 1, t_wax);
                m->ter_set(i - 2, j + 3, t_wax);
                m->ter_set(i - 1, j + 3, t_wax);
                m->ter_set(i + 1, j + 3, t_wax);
                m->ter_set(i + 2, j + 3, t_wax);
                m->ter_set(i - 1, j + 4, t_wax);
                m->ter_set(i    , j + 4, t_wax);
                m->ter_set(i + 1, j + 4, t_wax);

                if (skip1 ==  0 || skip2 ==  0)
                    m->ter_set(i - 1, j - 4, t_floor_wax);
                if (skip1 ==  1 || skip2 ==  1)
                    m->ter_set(i    , j - 4, t_floor_wax);
                if (skip1 ==  2 || skip2 ==  2)
                    m->ter_set(i + 1, j - 4, t_floor_wax);
                if (skip1 ==  3 || skip2 ==  3)
                    m->ter_set(i - 2, j - 3, t_floor_wax);
                if (skip1 ==  4 || skip2 ==  4)
                    m->ter_set(i - 1, j - 3, t_floor_wax);
                if (skip1 ==  5 || skip2 ==  5)
                    m->ter_set(i + 1, j - 3, t_floor_wax);
                if (skip1 ==  6 || skip2 ==  6)
                    m->ter_set(i + 2, j - 3, t_floor_wax);
                if (skip1 ==  7 || skip2 ==  7)
                    m->ter_set(i - 3, j - 2, t_floor_wax);
                if (skip1 ==  8 || skip2 ==  8)
                    m->ter_set(i - 2, j - 2, t_floor_wax);
                if (skip1 ==  9 || skip2 ==  9)
                    m->ter_set(i + 2, j - 2, t_floor_wax);
                if (skip1 == 10 || skip2 == 10)
                    m->ter_set(i + 3, j - 2, t_floor_wax);
                if (skip1 == 11 || skip2 == 11)
                    m->ter_set(i - 3, j - 1, t_floor_wax);
                if (skip1 == 12 || skip2 == 12)
                    m->ter_set(i - 3, j    , t_floor_wax);
                if (skip1 == 13 || skip2 == 13)
                    m->ter_set(i - 3, j - 1, t_floor_wax);
                if (skip1 == 14 || skip2 == 14)
                    m->ter_set(i - 3, j + 1, t_floor_wax);
                if (skip1 == 15 || skip2 == 15)
                    m->ter_set(i - 3, j    , t_floor_wax);
                if (skip1 == 16 || skip2 == 16)
                    m->ter_set(i - 3, j + 1, t_floor_wax);
                if (skip1 == 17 || skip2 == 17)
                    m->ter_set(i - 2, j + 3, t_floor_wax);
                if (skip1 == 18 || skip2 == 18)
                    m->ter_set(i - 1, j + 3, t_floor_wax);
                if (skip1 == 19 || skip2 == 19)
                    m->ter_set(i + 1, j + 3, t_floor_wax);
                if (skip1 == 20 || skip2 == 20)
                    m->ter_set(i + 2, j + 3, t_floor_wax);
                if (skip1 == 21 || skip2 == 21)
                    m->ter_set(i - 1, j + 4, t_floor_wax);
                if (skip1 == 22 || skip2 == 22)
                    m->ter_set(i    , j + 4, t_floor_wax);
                if (skip1 == 23 || skip2 == 23)
                    m->ter_set(i + 1, j + 4, t_floor_wax);

                if (dat.t_nesw[0] == "hive" && dat.t_nesw[1] == "hive" &&
                      dat.t_nesw[2] == "hive" && dat.t_nesw[3] == "hive") {
                    m->place_items("hive_center", 90, i - 2, j - 2, i + 2, j + 2, false, turn);
                } else {
                    m->place_items("hive", 80, i - 2, j - 2, i + 2, j + 2, false, turn);
                }
            }
        }
    }
}

void mapgen_spider_pit(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    // First generate a forest
    dat.fill(4);
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] == "forest" || dat.t_nesw[i] == "forest_water") {
            dat.dir(i) += 14;
        } else if (dat.t_nesw[i] == "forest_thick") {
            dat.dir(i) += 18;
        }
    }
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            int forest_chance = 0, num = 0;
            if (j < dat.n_fac) {
                forest_chance += dat.n_fac - j;
                num++;
            }
            if (SEEX * 2 - 1 - i < dat.e_fac) {
                forest_chance += dat.e_fac - (SEEX * 2 - 1 - i);
                num++;
            }
            if (SEEY * 2 - 1 - j < dat.s_fac) {
                forest_chance += dat.s_fac - (SEEX * 2 - 1 - j);
                num++;
            }
            if (i < dat.w_fac) {
                forest_chance += dat.w_fac - i;
                num++;
            }
            if (num > 0) {
                forest_chance /= num;
            }
            int rn = rng(0, forest_chance);
            if ((forest_chance > 0 && rn > 13) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_tree);
            } else if ((forest_chance > 0 && rn > 10) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_tree_young);
            } else if ((forest_chance > 0 && rn >  9) || one_in(100 - forest_chance)) {
                m->ter_set(i, j, t_underbrush);
            } else {
                m->ter_set(i, j, t_dirt);
            }
        }
    }
    m->place_items("forest", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn);
    // Next, place webs and sinkholes
    for (int i = 0; i < 4; i++) {
        int x = rng(3, SEEX * 2 - 4), y = rng(3, SEEY * 2 - 4);
        if (i == 0)
            m->ter_set(x, y, t_slope_down);
        else {
            m->ter_set(x, y, t_dirt);
            m->add_trap(x, y, tr_sinkhole);
        }
        for (int x1 = x - 3; x1 <= x + 3; x1++) {
            for (int y1 = y - 3; y1 <= y + 3; y1++) {
                m->add_field(NULL, x1, y1, fd_web, rng(2, 3));
                if (m->ter(x1, y1) != t_slope_down)
                    m->ter_set(x1, y1, t_dirt);
            }
        }
    }
}

void mapgen_fungal_bloom(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (one_in(rl_dist(i, j, 12, 12) * 4)) {
                m->ter_set(i, j, t_marloss);
            } else if (one_in(10)) {
                if (one_in(3)) {
                    m->ter_set(i, j, t_tree_fungal);
                } else {
                    m->ter_set(i, j, t_tree_fungal_young);
                }
            
            } else if (one_in(5)) {
                m->ter_set(i, j, t_shrub_fungal);
            } else if (one_in(10)) {
                m->ter_set(i, j, t_fungus_mound);
            } else {
                m->ter_set(i, j, t_fungus);
            }
        }
    }
    square(m, t_fungus, SEEX - 2, SEEY - 2, SEEX + 2, SEEY + 2);
    m->add_spawn("mon_fungaloid_queen", 1, 12, 12);
}

void mapgen_road_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    bool sidewalks = false;
    for (int i = 0; i < 8; i++) {
        if (otermap[dat.t_nesw[i]].sidewalk) {
            sidewalks = true;
        }
    }

    int veh_spawn_heading;
    if (terrain_type == "road_ew") {
        veh_spawn_heading = (one_in(2)? 0 : 180);
    } else {
        veh_spawn_heading = (one_in(2)? 270 : 90);
    }

    m->add_road_vehicles(sidewalks, veh_spawn_heading);

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i < 4 || i >= SEEX * 2 - 4) {
                if (sidewalks) {
                    m->ter_set(i, j, t_sidewalk);
                } else {
                    m->ter_set(i, j, grass_or_dirt());
                }
            } else {
                if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0) {
                    m->ter_set(i, j, t_pavement_y);
                } else {
                    m->ter_set(i, j, t_pavement);
                }
            }
        }
    }
    if (terrain_type == "road_ew") {
        m->rotate(1);
    }
    if(sidewalks) {
        m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
    }
    m->place_items("road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}

void mapgen_road_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    bool sidewalks = false;
    for (int i = 0; i < 8; i++) {
        if (otermap[dat.t_nesw[i]].sidewalk) {
            sidewalks = true;
        }
    }

    m->add_road_vehicles(sidewalks, one_in(2) ? 90 : 180);
    if (sidewalks) { //this crossroad has sidewalk => this crossroad is in the city
        for (int i=0; i< SEEX * 2; i++) {
            for (int j=0; j< SEEY*2; j++) {
                m->ter_set(i,j, grass_or_dirt());
            }
        }
        //draw lines diagonally
        line(m, t_floor_blue, 4, 0, SEEX*2, SEEY*2-4);
        line(m, t_pavement, SEEX*2-4, 0, SEEX*2, 4);
        mapf::formatted_set_simple(m, 0, 0,
"\
ssss.......yy.......ssss\n\
ssss.......yy........sss\n\
ssss.......yy.........ss\n\
ssss...................s\n\
ssss.......yy...........\n\
ssss.......yy...........\n\
ssss.......yy...........\n\
ssss.......yy...........\n\
ssss........yy..........\n\
ssss.........yy.........\n\
ssss..........yy........\n\
ssss...........yyyyy.yyy\n\
ssss............yyyy.yyy\n\
ssss...................\n\
,ssss...................\n\
,,ssss..................\n\
,,,ssss.................\n\
,,,,ssss................\n\
,,,,,ssss...............\n\
,,,,,,ssss..............\n\
,,,,,,,sssssssssssssssss\n\
,,,,,,,,ssssssssssssssss\n\
,,,,,,,,,sssssssssssssss\n\
,,,,,,,,,,ssssssssssssss\n",
        mapf::basic_bind(". , y s", t_pavement, t_dirt, t_pavement_y, t_sidewalk),
        mapf::basic_bind(". , y s", f_null, f_null, f_null, f_null, f_null));
    } else { //crossroad (turn) in the wilderness
        for (int i=0; i< SEEX * 2; i++) {
            for (int j=0; j< SEEY*2; j++) {
                m->ter_set(i,j, grass_or_dirt());
            }
        }
        //draw lines diagonally
        line(m, t_floor_blue, 4, 0, SEEX*2, SEEY*2-4);
        line(m, t_pavement, SEEX*2-4, 0, SEEX*2, 4);
        mapf::formatted_set_simple(m, 0, 0,
"\
,,,,.......yy.......,,,,\n\
,,,,.......yy........,,,\n\
,,,,.......yy.........,,\n\
,,,,...................,\n\
,,,,.......yy...........\n\
,,,,.......yy...........\n\
,,,,.......yy...........\n\
,,,,.......yy...........\n\
,,,,........yy..........\n\
,,,,.........yy.........\n\
,,,,..........yy........\n\
,,,,...........yyyyy.yyy\n\
,,,,............yyyy.yyy\n\
,,,,....................\n\
,,,,,...................\n\
,,,,,,..................\n\
,,,,,,,.................\n\
,,,,,,,,................\n\
,,,,,,,,,...............\n\
,,,,,,,,,,..............\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n",
        mapf::basic_bind(". , y", t_pavement, t_dirt, t_pavement_y),
        mapf::basic_bind(". , y", f_null, f_null, f_null, f_null));
    }
    if (terrain_type == "road_es") {
        m->rotate(1);
    }
    if (terrain_type == "road_sw") {
        m->rotate(2);
    }
    if (terrain_type == "road_wn") {
        m->rotate(3); //looks like that the code above paints road_ne
    }
    if(sidewalks) {
        m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
    }
    m->place_items("road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}

void mapgen_road_tee(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    bool sidewalks = false;
    for (int i = 0; i < 8; i++) {
        if (otermap[dat.t_nesw[i]].sidewalk) {
            sidewalks = true;
        }
    }

    m->add_road_vehicles(sidewalks, one_in(2) ? 90 : 180);

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i < 4 || (i >= SEEX * 2 - 4 && (j < 4 || j >= SEEY * 2 - 4))) {
                if (sidewalks) {
                    m->ter_set(i, j, t_sidewalk);
                } else {
                    m->ter_set(i, j, grass_or_dirt());
                }
            } else {
                if (((i == SEEX - 1 || i == SEEX) && j % 4 != 0) ||
                     ((j == SEEY - 1 || j == SEEY) && i % 4 != 0 && i > SEEX)) {
                    m->ter_set(i, j, t_pavement_y);
                } else {
                    m->ter_set(i, j, t_pavement);
                }
            }
        }
    }
    if (terrain_type == "road_esw") {
        m->rotate(1);
    }
    if (terrain_type == "road_nsw") {
        m->rotate(2);
    }
    if (terrain_type == "road_new") {
        m->rotate(3);
    }
    if(sidewalks) {
        m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
    }
    m->place_items("road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}

void mapgen_road_four_way(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    bool plaza = false;
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] == "road_nesw" || dat.t_nesw[i] == "road_nesw_manhole") {
            plaza = true;
        }
    }
    bool sidewalks = false;
    for (int i = 0; i < 8; i++) {
        if (otermap[dat.t_nesw[i]].sidewalk) {
            sidewalks = true;
        }
    }

    // spawn city car wrecks
    if (sidewalks) {
        m->add_road_vehicles(true, one_in(2) ? 90 : 180);
    }

    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (plaza) {
                m->ter_set(i, j, t_sidewalk);
            } else if ((i < 4 || i >= SEEX * 2 - 4) && (j < 4 || j >= SEEY * 2 - 4)) {
                if (sidewalks) {
                    m->ter_set(i, j, t_sidewalk);
                } else {
                    m->ter_set(i, j, grass_or_dirt());
                }
            } else {
                if (((i == SEEX - 1 || i == SEEX) && j % 4 != 0) ||
                      ((j == SEEY - 1 || j == SEEY) && i % 4 != 0)) {
                    m->ter_set(i, j, t_pavement_y);
                } else {
                    m->ter_set(i, j, t_pavement);
                }
            }
        }
    }
    if (plaza) { // Special embellishments for a plaza
        if (one_in(10)) { // Fountain
            for (int i = SEEX - 2; i <= SEEX + 2; i++) {
                m->ter_set(i, i, t_water_sh);
                m->ter_set(i, SEEX * 2 - i, t_water_sh);
            }
        }
        if (one_in(10)) { // Small trees in center
            mapf::formatted_set_terrain(m, SEEX-2, SEEY-2,
"\
 t t\n\
t   t\n\
\n\
t   t\n\
 t t\n\
",
            mapf::basic_bind("t", t_tree_young), mapf::end());
        }
        if (one_in(14)) { // Rows of small trees
            int gap = rng(2, 4);
            int start = rng(0, 4);
            for (int i = 2; i < SEEX * 2 - start; i += gap) {
                m->ter_set(i, start, t_tree_young);
                m->ter_set(SEEX * 2 - 1 - i, start, t_tree_young);
                m->ter_set(start, i, t_tree_young);
                m->ter_set(start, SEEY * 2 - 1 - i, t_tree_young);
            }
        }
        m->place_items("trash", 5, 0, 0, SEEX * 2 -1, SEEX * 2 - 1, true, 0);
    } else {
        m->place_items("road",  5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
    }
    if(sidewalks) {
        m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
    }
    if (terrain_type == "road_nesw_manhole") {
        m->ter_set(rng(6, SEEX * 2 - 6), rng(6, SEEX * 2 - 6), t_manhole_cover);
    }
}

void mapgen_bridge(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i < 4 || i >= SEEX * 2 - 4) {
                m->ter_set(i, j, t_water_dp);
            } else if (i == 4 || i == SEEX * 2 - 5) {
                m->ter_set(i, j, t_railing_v);
            } else {
                if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0) {
                    m->ter_set(i, j, t_pavement_y);
                } else {
                    m->ter_set(i, j, t_pavement);
                }
            }
        }
    }
    // spawn regular road out of fuel vehicles
    if (one_in(2)) {
        int vx = rng (10, 12);
        int vy = rng (10, 12);
        int rc = rng(1, 10);
        if (rc <= 5) {
            m->add_vehicle (g, "car", vx, vy, one_in(2)? 90 : 180, 0, -1);
        } else if (rc <= 8) {
            m->add_vehicle (g, "flatbed_truck", vx, vy, one_in(2)? 90 : 180, 0, -1);
        } else if (rc <= 9) {
            m->add_vehicle (g, "semi_truck", vx, vy, one_in(2)? 90 : 180, 0, -1);
        } else {
            m->add_vehicle (g, "armored_car", vx, vy, one_in(2)? 90 : 180, 0, -1);
        }
    }

    if (terrain_type == "bridge_ew") {
        m->rotate(1);
    }
    m->place_items("road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}

void mapgen_highway(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i < 3 || i >= SEEX * 2 - 3) {
                m->ter_set(i, j, grass_or_dirt());
            } else if (i == 3 || i == SEEX * 2 - 4) {
                m->ter_set(i, j, t_railing_v);
            } else {
                if ((i == SEEX - 1 || i == SEEX) && j % 4 != 0) {
                    m->ter_set(i, j, t_pavement_y);
                } else {
                    m->ter_set(i, j, t_pavement);
                }
            }
        }
    }
    if (terrain_type == "hiway_ew") {
        m->rotate(1);
    }
    m->place_items("road", 8, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);

    // spawn regular road out of fuel vehicles
    if (one_in(2)) {
        int vx = rng (10, 12);
        int vy = rng (10, 12);
        int rc = rng(1, 10);
        if (rc <= 5) {
            m->add_vehicle (g, "car", vx, vy, one_in(2)? 90 : 180, 0, -1);
        } else if (rc <= 8) {
            m->add_vehicle (g, "flatbed_truck", vx, vy, one_in(2)? 90 : 180, 0, -1);
        } else if (rc <= 9) {
            m->add_vehicle (g, "semi_truck", vx, vy, one_in(2)? 90 : 180, 0, -1);
        } else {
            m->add_vehicle (g, "armored_car", vx, vy, one_in(2)? 90 : 180, 0, -1);
        }
    }
}

void mapgen_river_curved_not(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = SEEX * 2 - 1; i >= 0; i--) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (j < 4 && i >= SEEX * 2 - 4) {
                m->ter_set(i, j, t_water_sh);
            } else {
                m->ter_set(i, j, t_water_dp);
            }
        }
    }
    if (terrain_type == "river_c_not_se") {
        m->rotate(1);
    }
    if (terrain_type == "river_c_not_sw") {
        m->rotate(2);
    }
    if (terrain_type == "river_c_not_nw") {
        m->rotate(3);
    }
}

void mapgen_river_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (j < 4) {
                m->ter_set(i, j, t_water_sh);
            } else {
                m->ter_set(i, j, t_water_dp);
            }
        }
    }
    if (terrain_type == "river_east") {
        m->rotate(1);
    }
    if (terrain_type == "river_south") {
        m->rotate(2);
    }
    if (terrain_type == "river_west") {
        m->rotate(3);
    }
}

void mapgen_river_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = SEEX * 2 - 1; i >= 0; i--) {
        for (int j = 0; j < SEEY * 2; j++) {
            if (i >= SEEX * 2 - 4 || j < 4) {
                m->ter_set(i, j, t_water_sh);
            } else {
                m->ter_set(i, j, t_water_dp);
            }
        }
    }
    if (terrain_type == "river_se") {
        m->rotate(1);
    }
    if (terrain_type == "river_sw") {
        m->rotate(2);
    }
    if (terrain_type == "river_nw") {
        m->rotate(3);
    }
}

void mapgen_parking_lot(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            if ((j == 5 || j == 9 || j == 13 || j == 17 || j == 21) &&
                 ((i > 1 && i < 8) || (i > 14 && i < SEEX * 2 - 2)))
                m->ter_set(i, j, t_pavement_y);
            else if ((j < 2 && i > 7 && i < 17) || (j >= 2 && j < SEEY * 2 - 2 && i > 1 &&
                      i < SEEX * 2 - 2))
                m->ter_set(i, j, t_pavement);
            else
                m->ter_set(i, j, grass_or_dirt());
        }
    }
    int vx = rng (0, 3) * 4 + 5;
    int vy = 4;
    std::string veh_type = "";
    int roll = rng(1, 100);
    if (roll <= 5) { //specials
        int ra = rng(1, 100);
        if (ra <= 3) {
            veh_type = "military_cargo_truck";
        } else if (ra <= 10) {
            veh_type = "bubble_car";
        } else if (ra <= 15) {
            veh_type = "rv";
        } else if (ra <= 20) {
            veh_type = "schoolbus";
        } else {
            veh_type = "quad_bike";
        }
    } else if (roll <= 15) { //commercial
        int rb = rng(1, 100);
        if (rb <= 25) {
            veh_type = "truck_trailer";
        } else if (rb <= 35) {
            veh_type = "semi_truck";
        } else if (rb <= 50) {
            veh_type = "cube_van";
        } else {
            veh_type = "flatbed_truck";
        }
    } else if (roll < 50) { //commons
        int rc = rng(1, 100);
        if (rc <= 4) {
            veh_type = "golf_cart";
        } else if (rc <= 11) {
            veh_type = "scooter";
        } else if (rc <= 21) {
            veh_type = "beetle";
        } else if (rc <= 50) {
            veh_type = "car";
        } else if (rc <= 60) {
            veh_type = "electric_car";
        } else if (rc <= 65) {
            veh_type = "hippie_van";
        } else if (rc <= 73) {
            veh_type = "bicycle";
        } else if (rc <= 75) {
            veh_type = "unicycle";
        } else if (rc <= 90) {
            veh_type = "motorcycle";
        } else {
            veh_type = "motorcycle_sidecart";
        }
    } else {
        veh_type = "shopping_cart";
    }
    m->add_vehicle (g, veh_type, vx, vy, one_in(2)? 90 : 270, -1, -1);

    m->place_items("road", 8, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);
    for (int i = 1; i < 4; i++) {
        if (dat.t_nesw[i].size() > 5 && dat.t_nesw[i].find("road_",0,5) == 0) {
            m->rotate(i);
        }
    }
}

void mapgen_pool(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    fill_background(m, t_grass);
    mapf::formatted_set_simple(m, 0, 0,
"\
........................\n\
........................\n\
..++n++n++n++n++n++n++..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..+wwwwwwwwwwwwwwwwww+..\n\
..++n++n++n++n++n++n++..\n\
........................\n\
........................\n",
    mapf::basic_bind( "+ n . w", t_concrete, t_concrete, t_grass, t_water_dp ),
    mapf::basic_bind( "n", f_dive_block));
    m->add_spawn("mon_zombie_swimmer", rng(1, 6), SEEX, SEEY); // fixme; use density
}

void mapgen_park_playground(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
        fill_background(m, t_grass);
        mapf::formatted_set_simple(m, 0, 0,
"\
                        \n\
                        \n\
                        \n\
                        \n\
             t          \n\
      t         ##      \n\
                ##      \n\
                        \n\
    mmm                 \n\
    mmm    s        t   \n\
   tmmm    s            \n\
           s            \n\
           s            \n\
                        \n\
                        \n\
      -            t    \n\
     t-                 \n\
               t        \n\
         t              \n\
                        \n\
                        \n\
                        \n\
                        \n\
                        \n",
        mapf::basic_bind( "# m s t", t_sandbox, t_monkey_bars, t_slide, t_tree ),
        mapf::basic_bind( "-", f_bench));
        m->rotate(rng(0, 3));

        int vx = one_in(2) ? 1 : 20;
        int vy = one_in(2) ? 1 : 20;
        if(one_in(3)) {
            m->add_vehicle (g, "ice_cream_cart", vx, vy, 0, -1, -1);
        } else if(one_in(2)) {
            m->add_vehicle (g, "food_cart", vx, vy, one_in(2)? 90 : 180, -1, -1);
        }
        m->add_spawn("mon_zombie_child", rng(2, 8), SEEX, SEEY); // fixme; use density
}

void mapgen_park_basketball(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
        fill_background(m, t_pavement);
        mapf::formatted_set_simple(m, 0, 0,
"\
                        \n\
|-+------------------+-|\n\
|     .  . 7 .  .      |\n\
|     .  .   .  .      |\n\
|#    .  .....  .     #|\n\
|#    .         .     #|\n\
|#    .         .     #|\n\
|#    .         .     #|\n\
|#    .         .     #|\n\
|#     .       .      #|\n\
|#      .     .       #|\n\
|......................|\n\
|#      .     .       #|\n\
|#     .       .      #|\n\
|#    .         .     #|\n\
|#    .         .     #|\n\
|#    .         .     #|\n\
|#    .         .     #|\n\
|#    .  .....  .     #|\n\
|     .  .   .  .      |\n\
|     .  . 7 .  .      |\n\
|-+------------------+-|\n\
                        \n\
                        \n",
        mapf::basic_bind(". 7 | - +", t_pavement_y, t_backboard, t_chainfence_v, t_chainfence_h, t_chaingate_l),
        mapf::basic_bind("#", f_bench));
        m->rotate(rng(0, 3));
//    }
    m->add_spawn("mon_zombie_child", rng(2, 8), SEEX, SEEY); // fixme; use density
}

void mapgen_gas_station(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    int top_w = rng(5, 14);
    int bottom_w = SEEY * 2 - rng(1, 2);
    int middle_w = rng(top_w + 5, bottom_w - 3);
    if (middle_w < bottom_w - 5) {
        middle_w = bottom_w - 5;
    }
    int left_w = rng(0, 3);
    int right_w = SEEX * 2 - rng(1, 4);
    int center_w = rng(left_w + 4, right_w - 5);
    int pump_count = rng(3, 6);
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEX * 2; j++) {
            if (j < top_w && (top_w - j) % 4 == 0 && i > left_w && i < right_w &&
                 (i - (1 + left_w)) % pump_count == 0) {
                m->place_gas_pump(i, j, rng(1000, 10000));
            } else if ((j < 2 && i > 7 && i < 16) || (j < top_w && i > left_w && i < right_w)) {
                m->ter_set(i, j, t_pavement);
            } else if (j == top_w && (i == left_w + 6 || i == left_w + 7 || i == right_w - 7 ||
                      i == right_w - 6)) {
                m->ter_set(i, j, t_window);
            } else if (((j == top_w || j == bottom_w) && i >= left_w && i <= right_w) ||
                      (j == middle_w && (i >= center_w && i < right_w))) {
                m->ter_set(i, j, t_wall_h);
            } else if (((i == left_w || i == right_w) && j > top_w && j < bottom_w) ||
                      (j > middle_w && j < bottom_w && (i == center_w || i == right_w - 2))) {
                m->ter_set(i, j, t_wall_v);
            } else if (i == left_w + 1 && j > top_w && j < bottom_w) {
                m->set(i, j, t_floor, f_glass_fridge);
            } else if (i > left_w + 2 && i < left_w + 12 && i < center_w && i % 2 == 1 &&
                      j > top_w + 1 && j < middle_w - 1) {
                m->set(i, j, t_floor, f_rack);
            } else if ((i == right_w - 5 && j > top_w + 1 && j < top_w + 4) ||
                      (j == top_w + 3 && i > right_w - 5 && i < right_w)) {
                m->set(i, j, t_floor, f_counter);
            } else if (i > left_w && i < right_w && j > top_w && j < bottom_w) {
                m->ter_set(i, j, t_floor);
            } else {
                m->ter_set(i, j, grass_or_dirt());
            }
        }
    }
    m->ter_set(center_w, rng(middle_w + 1, bottom_w - 1), t_door_c);
    m->ter_set(right_w - 1, middle_w, t_door_c);
    m->ter_set(right_w - 1, bottom_w - 1, t_floor);
    m->place_toilet(right_w - 1, bottom_w - 1);
    m->ter_set(rng(10, 13), top_w, t_door_c);
    if (one_in(5)) {
        m->ter_set(rng(left_w + 1, center_w - 1), bottom_w, (one_in(4) ? t_door_c : t_door_locked));
    }
    for (int i = left_w + (left_w % 2 == 0 ? 3 : 4); i < center_w && i < left_w + 12; i += 2) {
        if (!one_in(3)) {
            m->place_items("snacks", 74, i, top_w + 2, i, middle_w - 2, false, 0);
        } else {
            m->place_items("magazines", 74, i, top_w + 2, i, middle_w - 2, false, 0);
        }
    }
    m->place_items("fridgesnacks", 82, left_w + 1, top_w + 1, left_w + 1, bottom_w - 1, false, 0);
    m->place_items("road",  12, 0,      0,  SEEX*2 - 1, top_w - 1, false, 0);
    m->place_items("behindcounter", 70, right_w - 4, top_w + 1, right_w - 1, top_w + 2, false, 0);
    m->place_items("softdrugs", 12, right_w - 1, bottom_w - 2, right_w - 1, bottom_w - 2, false, 0);
    if (terrain_type == "s_gas_east") {
        m->rotate(1);
    }
    if (terrain_type == "s_gas_south") {
        m->rotate(2);
    }
    if (terrain_type == "s_gas_west") {
        m->rotate(3);
    }
    m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
}
////////////////////


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


void mapgen_generic_house_boxy(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    mapgen_generic_house(m, terrain_type, dat, turn, density, 1);
}

void mapgen_generic_house_big_livingroom(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    mapgen_generic_house(m, terrain_type, dat, turn, density, 2);
}

void mapgen_generic_house_center_hallway(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    mapgen_generic_house(m, terrain_type, dat, turn, density, 3);
}

void mapgen_generic_house(map *m, oter_id terrain_type, mapgendata dat, int turn, float density, int variant)
{
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
                m->ter_set(i, j, t_floor);
            } else {
                m->ter_set(i, j, grass_or_dirt());
            }
            if (i >= lw && i <= rw && (j == tw || j == bw)) { //placing north and south walls
                m->ter_set(i, j, t_wall_h);
            }
            if ((i == lw || i == rw) && j > tw &&
                j < bw /*actual_house_height*/) { //placing west (lw) and east walls
                m->ter_set(i, j, t_wall_v);
            }
        }
    }
    switch(variant) {
    case 1: // Quadrants, essentially
        mw = rng(lw + 5, rw - 5);
        cw = tw + rng(4, 7);
        house_room(m, room_living, mw, tw, rw, cw);
        house_room(m, room_kitchen, lw, tw, mw, cw);
        m->ter_set(mw, rng(tw + 2, cw - 2), (one_in(3) ? t_door_c : t_floor));
        rn = rng(lw + 1, mw - 2);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        rn = rng(mw + 1, rw - 2);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        rn = rng(lw + 3, rw - 3); // Bottom part mw
        if (rn <= lw + 5) {
            // Bedroom on right, bathroom on left
            house_room(m, room_bedroom, rn, cw, rw, bw);

            // Put door between bedroom and living
            m->ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);

            if (bw - cw >= 10 && rn - lw >= 6) {
                // All fits, placing bathroom and 2nd bedroom
                house_room(m, room_bathroom, lw, bw - 5, rn, bw);
                house_room(m, room_bedroom, lw, cw, rn, bw - 5);

                // Put door between bathroom and bedroom
                m->ter_set(rn, rng(bw - 4, bw - 1), t_door_c);

                if (one_in(3)) {
                    // Put door between 2nd bedroom and 1st bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 6), t_door_c);
                } else {
                    // ...Otherwise, between 2nd bedroom and kitchen
                    m->ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);
                }
            } else if (bw - cw > 4) {
                // Too big for a bathroom, not big enough for 2nd bedroom
                // Make it a bathroom anyway, but give the excess space back to
                // the kitchen.
                house_room(m, room_bathroom, lw, bw - 4, rn, bw);
                for (int i = lw + 1; i < mw && i < rn; i++) {
                    m->ter_set(i, cw, t_floor);
                }

                // Put door between excess space and bathroom
                m->ter_set(rng(lw + 1, rn - 1), bw - 4, t_door_c);

                // Put door between excess space and bedroom
                m->ter_set(rn, rng(cw + 1, bw - 5), t_door_c);
            } else {
                // Small enough to be a bathroom; make it one.
                house_room(m, room_bathroom, lw, cw, rn, bw);

                if (one_in(5)) {
                    // Put door between batroom and kitchen with low chance
                    m->ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);
                } else {
                    // ...Otherwise, between bathroom and bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 1), t_door_c);
                }
            }
            // Point on bedroom wall, for window
            rn = rng(rn + 2, rw - 2);
        } else {
            // Bedroom on left, bathroom on right
            house_room(m, room_bedroom, lw, cw, rn, bw);

            // Put door between bedroom and kitchen
            m->ter_set(rng(lw + 1, rn > mw ? mw - 1 : rn - 1), cw, t_door_c);

            if (bw - cw >= 10 && rw - rn >= 6) {
                // All fits, placing bathroom and 2nd bedroom
                house_room(m, room_bathroom, rn, bw - 5, rw, bw);
                house_room(m, room_bedroom, rn, cw, rw, bw - 5);

                // Put door between bathroom and bedroom
                m->ter_set(rn, rng(bw - 4, bw - 1), t_door_c);

                if (one_in(3)) {
                    // Put door between 2nd bedroom and 1st bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 6), t_door_c);
                } else {
                    // ...Otherwise, between 2nd bedroom and living
                    m->ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);
                }
            } else if (bw - cw > 4) {
                // Too big for a bathroom, not big enough for 2nd bedroom
                // Make it a bathroom anyway, but give the excess space back to
                // the living.
                house_room(m, room_bathroom, rn, bw - 4, rw, bw);
                for (int i = rw - 1; i > rn && i > mw; i--) {
                    m->ter_set(i, cw, t_floor);
                }

                // Put door between excess space and bathroom
                m->ter_set(rng(rw - 1, rn + 1), bw - 4, t_door_c);

                // Put door between excess space and bedroom
                m->ter_set(rn, rng(cw + 1, bw - 5), t_door_c);
            } else {
                // Small enough to be a bathroom; make it one.
                house_room(m, room_bathroom, rn, cw, rw, bw);

                if (one_in(5)) {
                    // Put door between bathroom and living with low chance
                    m->ter_set(rng(rw - 1, rn > mw ? rn + 1 : mw + 1), cw, t_door_c);
                } else {
                    // ...Otherwise, between bathroom and bedroom
                    m->ter_set(rn, rng(cw + 1, bw - 1), t_door_c);
                }
            }
            // Point on bedroom wall, for window
            rn = rng(lw + 2, rn - 2);
        }
        m->ter_set(rn    , bw, t_window_domestic);
        m->ter_set(rn + 1, bw, t_window_domestic);
        if (!one_in(3) && rw < SEEX * 2 - 1) { // Potential side windows
            rn = rng(tw + 2, bw - 6);
            m->ter_set(rw, rn    , t_window_domestic);
            m->ter_set(rw, rn + 4, t_window_domestic);
        }
        if (!one_in(3) && lw > 0) { // Potential side windows
            rn = rng(tw + 2, bw - 6);
            m->ter_set(lw, rn    , t_window_domestic);
            m->ter_set(lw, rn + 4, t_window_domestic);
        }
        if (one_in(2)) { // Placement of the main door
            m->ter_set(rng(lw + 2, mw - 1), tw, (one_in(6) ? t_door_c : t_door_locked));
            if (one_in(5)) { // Placement of side door
                m->ter_set(rw, rng(tw + 2, cw - 2), (one_in(6) ? t_door_c : t_door_locked));
            }
        } else {
            m->ter_set(rng(mw + 1, rw - 2), tw, (one_in(6) ? t_door_c : t_door_locked));
            if (one_in(5)) {
                m->ter_set(lw, rng(tw + 2, cw - 2), (one_in(6) ? t_door_c : t_door_locked));
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
            house_room(m, room_backyard, lw, actual_house_height + 1, rw, bw);
            //door from bedroom to backyard
            m->ter_set((lw + mw) / 2, actual_house_height, t_door_c);
        } else { //using old layout
            actual_house_height = bw_old;
        }
        // Plop down the rooms
        house_room(m, room_living, lw, tw, rw, cw);
        house_room(m, room_kitchen, mw, cw, rw, actual_house_height - 3);
        house_room(m, room_bedroom, lw, cw, mw, actual_house_height ); //making bedroom smaller
        house_room(m, room_bathroom, mw, actual_house_height - 3, rw, actual_house_height);

        // Space between kitchen & living room:
        rn = rng(mw + 1, rw - 3);
        m->ter_set(rn    , cw, t_floor);
        m->ter_set(rn + 1, cw, t_floor);
        // Front windows
        rn = rng(2, 5);
        m->ter_set(lw + rn    , tw, t_window_domestic);
        m->ter_set(lw + rn + 1, tw, t_window_domestic);
        m->ter_set(rw - rn    , tw, t_window_domestic);
        m->ter_set(rw - rn + 1, tw, t_window_domestic);
        // Front door
        m->ter_set(rng(lw + 4, rw - 4), tw, (one_in(6) ? t_door_c : t_door_locked));
        if (one_in(3)) { // Kitchen windows
            rn = rng(cw + 1, actual_house_height - 5);
            m->ter_set(rw, rn    , t_window_domestic);
            m->ter_set(rw, rn + 1, t_window_domestic);
        }
        if (one_in(3)) { // Bedroom windows
            rn = rng(cw + 1, actual_house_height - 2);
            m->ter_set(lw, rn    , t_window_domestic);
            m->ter_set(lw, rn + 1, t_window_domestic);
        }
        // Door to bedroom
        if (one_in(4)) {
            m->ter_set(rng(lw + 1, mw - 1), cw, t_door_c);
        } else {
            m->ter_set(mw, rng(cw + 3, actual_house_height - 4), t_door_c);
        }
        // Door to bathrom
        if (one_in(4)) {
            m->ter_set(mw, actual_house_height - 1, t_door_c);
        } else {
            m->ter_set(rng(mw + 2, rw - 2), actual_house_height - 3, t_door_c);
        }
        // Back windows
        rn = rng(lw + 1, mw - 2);
        m->ter_set(rn    , actual_house_height, t_window_domestic);
        m->ter_set(rn + 1, actual_house_height, t_window_domestic);
        rn = rng(mw + 1, rw - 1);
        m->ter_set(rn, actual_house_height, t_window_domestic);
        break;

    case 3: // Long center hallway, kitchen, living room and office
        mw = int((lw + rw) / 2);
        cw = bw - rng(5, 7);
        // Hallway doors and windows
        m->ter_set(mw    , tw, (one_in(6) ? t_door_c : t_door_locked));
        if (one_in(4)) {
            m->ter_set(mw - 1, tw, t_window_domestic);
            m->ter_set(mw + 1, tw, t_window_domestic);
        }
        for (int i = tw + 1; i < cw; i++) { // Hallway walls
            m->ter_set(mw - 2, i, t_wall_v);
            m->ter_set(mw + 2, i, t_wall_v);
        }
        if (one_in(2)) { // Front rooms are kitchen or living room
            house_room(m, room_living, lw, tw, mw - 2, cw);
            house_room(m, room_kitchen, mw + 2, tw, rw, cw);
        } else {
            house_room(m, room_kitchen, lw, tw, mw - 2, cw);
            house_room(m, room_living, mw + 2, tw, rw, cw);
        }
        // Front windows
        rn = rng(lw + 1, mw - 4);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        rn = rng(mw + 3, rw - 2);
        m->ter_set(rn    , tw, t_window_domestic);
        m->ter_set(rn + 1, tw, t_window_domestic);
        if (one_in(3) && lw > 0) { // Side windows?
            rn = rng(tw + 1, cw - 2);
            m->ter_set(lw, rn    , t_window_domestic);
            m->ter_set(lw, rn + 1, t_window_domestic);
        }
        if (one_in(3) && rw < SEEX * 2 - 1) { // Side windows?
            rn = rng(tw + 1, cw - 2);
            m->ter_set(rw, rn    , t_window_domestic);
            m->ter_set(rw, rn + 1, t_window_domestic);
        }
        if (one_in(2)) { // Bottom rooms are bedroom or bathroom
            //bathroom to the left (eastern wall), study to the right
            //house_room(m, room_bedroom, lw, cw, rw - 3, bw);
            house_room(m, room_bedroom, mw - 2, cw, rw - 3, bw);
            house_room(m, room_bathroom, rw - 3, cw, rw, bw);
            house_room(m, room_study, lw, cw, mw - 2, bw);
            //===Study Room Furniture==
            m->ter_set(mw - 2, (bw + cw) / 2, t_door_o);
            m->furn_set(lw + 1, cw + 1, f_chair);
            m->furn_set(lw + 1, cw + 2, f_table);
            m->ter_set(lw + 1, cw + 3, t_console_broken);
            m->furn_set(lw + 3, bw - 1, f_bookcase);
            m->place_items("magazines", 30,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            m->place_items("novels", 40,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            m->place_items("alcohol", 20,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            m->place_items("manuals", 30,  lw + 3,  bw - 1, lw + 3,  bw - 1, false, 0);
            //=========================
            m->ter_set(rng(lw + 2, mw - 3), cw, t_door_c);
            if (one_in(4)) {
                m->ter_set(rng(rw - 2, rw - 1), cw, t_door_c);
            } else {
                m->ter_set(rw - 3, rng(cw + 2, bw - 2), t_door_c);
            }
            rn = rng(mw, rw - 5); //bedroom windows
            m->ter_set(rn    , bw, t_window_domestic);
            m->ter_set(rn + 1, bw, t_window_domestic);
            m->ter_set(rng(lw + 2, mw - 3), bw, t_window_domestic); //study window

            if (one_in(4)) {
                m->ter_set(rng(rw - 2, rw - 1), bw, t_window_domestic);
            } else {
                m->ter(rw, rng(cw + 1, bw - 1));
            }
        } else { //bathroom to the right
            house_room(m, room_bathroom, lw, cw, lw + 3, bw);
            //house_room(m, room_bedroom, lw + 3, cw, rw, bw);
            house_room(m, room_bedroom, lw + 3, cw, mw + 2, bw);
            house_room(m, room_study, mw + 2, cw, rw, bw);
            //===Study Room Furniture==
            m->ter_set(mw + 2, (bw + cw) / 2, t_door_c);
            m->furn_set(rw - 1, cw + 1, f_chair);
            m->furn_set(rw - 1, cw + 2, f_table);
            m->ter_set(rw - 1, cw + 3, t_console_broken);
            m->furn_set(rw - 3, bw - 1, f_bookcase);
            m->place_items("magazines", 40,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            m->place_items("novels", 40,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            m->place_items("alcohol", 20,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            m->place_items("manuals", 20,  rw - 3,  bw - 1, rw - 3,  bw - 1, false, 0);
            //=========================

            if (one_in(4)) {
                m->ter_set(rng(lw + 1, lw + 2), cw, t_door_c);
            } else {
                m->ter_set(lw + 3, rng(cw + 2, bw - 2), t_door_c);
            }
            rn = rng(lw + 4, mw); //bedroom windows
            m->ter_set(rn    , bw, t_window_domestic);
            m->ter_set(rn + 1, bw, t_window_domestic);
            m->ter_set(rng(mw + 3, rw - 1), bw, t_window_domestic); //study window
            if (one_in(4)) {
                m->ter_set(rng(lw + 1, lw + 2), bw, t_window_domestic);
            } else {
                m->ter(lw, rng(cw + 1, bw - 1));
            }
        }
        // Doors off the sides of the hallway
        m->ter_set(mw - 2, rng(tw + 3, cw - 3), t_door_c);
        m->ter_set(mw + 2, rng(tw + 3, cw - 3), t_door_c);
        m->ter_set(mw, cw, t_door_c);
        break;
    } // Done with the various house structures
    //////
    if (rng(2, 7) < tw) { // Big front yard has a chance for a fence
        for (int i = lw; i <= rw; i++) {
            m->ter_set(i, 0, t_fence_h);
        }
        for (int i = 1; i < tw; i++) {
            m->ter_set(lw, i, t_fence_v);
            m->ter_set(rw, i, t_fence_v);
        }
        int hole = rng(SEEX - 3, SEEX + 2);
        m->ter_set(hole, 0, t_dirt);
        m->ter_set(hole + 1, 0, t_dirt);
        if (one_in(tw)) {
            m->ter_set(hole - 1, 1, t_tree_young);
            m->ter_set(hole + 2, 1, t_tree_young);
        }
    }

    if ("house_base" == terrain_type.t().id_base ) {
        int attempts = 20;
        do {
            rn = rng(lw + 1, rw - 1);
            attempts--;
        } while (m->ter(rn, actual_house_height - 1) != t_floor && attempts);
        if( m->ter(rn, actual_house_height - 1) == t_floor && attempts ) {
            m->ter_set(rn, actual_house_height - 1, t_stairs_down);
        }
    }
    if (one_in(100)) { // todo: region data // Houses have a 1 in 100 chance of wasps!
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (m->ter(i, j) == t_door_c || m->ter(i, j) == t_door_locked) {
                    m->ter_set(i, j, t_door_frame);
                }
                if (m->ter(i, j) == t_window_domestic && !one_in(3)) {
                    m->ter_set(i, j, t_window_frame);
                }
                if ((m->ter(i, j) == t_wall_h || m->ter(i, j) == t_wall_v) && one_in(8)) {
                    m->ter_set(i, j, t_paper);
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
                        m->ter_set(podx + x, pody + y, t_paper);
                    }
                }
            }
            m->add_spawn("mon_wasp", 1, podx, pody);
        }
        m->place_items("rare", 70, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);

    } else if (one_in(150)) { // todo; region_data // No wasps; black widows?
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if (m->ter(i, j) == t_floor) {
                    if (one_in(15)) {
                        m->add_spawn("mon_spider_widow_giant", rng(1, 2), i, j);
                        for (int x = i - 1; x <= i + 1; x++) {
                            for (int y = j - 1; y <= j + 1; y++) {
                                if (m->ter(x, y) == t_floor) {
                                    m->add_field(NULL, x, y, fd_web, rng(2, 3));
                                }
                            }
                        }
                    } else if (m->move_cost(i, j) > 0 && one_in(5)) {
                        m->add_field(NULL, x, y, fd_web, 1);
                    }
                }
            }
        }
        m->place_items("rare", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, false, turn);
    } else { // Just boring old zombies
        m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
    }

    int iid_diff = (int)terrain_type - terrain_type.t().loadid_base;
    if ( iid_diff > 0 ) {
        m->rotate(iid_diff);
    }
}

/////////////////////////////
void mapgen_church_new_england(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    computer *tmpcomp = NULL;
    fill_background(m, &grass_or_dirt);
    //New England or Country style, single centered steeple low clear windows
    mapf::formatted_set_simple(m, 0, 0,"\
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
               f_toilet, f_sink,  f_fridge, f_bookcase, f_chair, f_counter, f_desk,  f_locker, f_null)
    );
    m->spawn_item(9, 6, "brazier");
    m->spawn_item(14, 6, "brazier");
    m->place_items("church", 40,  5,  5, 8,  16, false, 0);
    m->place_items("church", 40,  5,  5, 8,  16, false, 0);
    m->place_items("church", 85,  12,  2, 14,  2, false, 0);
    m->place_items("office", 60,  6,  2, 8,  3, false, 0);
    m->place_items("jackets", 85,  7,  18, 8,  18, false, 0);
    tmpcomp = m->add_computer(11, 2, _("Church Bells 1.2"), 0);
    tmpcomp->add_option(_("Gathering Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Wedding Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Funeral Toll"), COMPACT_TOLL, 0);
    int iid_diff = (int)terrain_type - terrain_type.t().loadid_base + 2;
    if ( iid_diff != 4 ) {
        m->rotate(iid_diff);
    }
}

void mapgen_church_gothic(map *m, oter_id terrain_type, mapgendata dat, int turn, float density) {
    computer *tmpcomp = NULL;
    fill_background(m, &grass_or_dirt);
    //Gothic Style, unreachable high stained glass windows, stone construction
    mapf::formatted_set_simple(m, 0, 0, "\
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
               f_sink,       f_chair,      f_counter,    f_locker,     f_null)
    );
    m->spawn_item(8, 4, "brazier");
    m->spawn_item(15, 4, "brazier");
    m->place_items("church", 70,  6,  7, 17,  16, false, 0);
    m->place_items("church", 70,  6,  7, 17,  16, false, 0);
    m->place_items("church", 60,  6,  7, 17,  16, false, 0);
    m->place_items("cleaning", 60,  3,  18, 4,  21, false, 0);
    m->place_items("jackets", 85,  14,  18, 16,  18, false, 0);
    tmpcomp = m->add_computer(19, 20, _("Church Bells 1.2"), 0);
    tmpcomp->add_option(_("Gathering Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Wedding Toll"), COMPACT_TOLL, 0);
    tmpcomp->add_option(_("Funeral Toll"), COMPACT_TOLL, 0);
    int iid_diff = (int)terrain_type - terrain_type.t().loadid_base + 2;
    if ( iid_diff != 4 ) {
        m->rotate(iid_diff);
    }
}
