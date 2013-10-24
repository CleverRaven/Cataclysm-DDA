#include "building_generation.h"
#include "output.h"
#include "item_factory.h"
#include "line.h"
#include "mapgenformat.h"

void line(map *m, ter_id type, int x1, int y1, int x2, int y2);
void line_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void fill_background(map *m, ter_id type);
void fill_background(map *m, ter_id (*f)());
void square(map *m, ter_id type, int x1, int y1, int x2, int y2);
void square(map *m, ter_id (*f)(), int x1, int y1, int x2, int y2);
void square_furn(map *m, furn_id type, int x1, int y1, int x2, int y2);
void rough_circle(map *m, ter_id type, int x, int y, int rad);
void add_corpse(game *g, map *m, int x, int y);

mapgendata::mapgendata(oter_id north, oter_id east, oter_id south, oter_id west)
{
    t_nesw[0] = north;
    t_nesw[1] = east;
    t_nesw[2] = south;
    t_nesw[3] = west;
    n_fac = 0;
    e_fac = 0;
    s_fac = 0;
    w_fac = 0;
}

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

void mapgen_null(map *m)
{
    debugmsg("Generating terrain for ot_null, please report this as a bug");
    for (int i = 0; i < SEEX * 2; i++) {
        for (int j = 0; j < SEEY * 2; j++) {
            m->ter_set(i, j, t_null);
            m->radiation(i, j) = 0;
        }
    }
}

void mapgen_crater(map *m, mapgendata dat)
{
    for(int i = 0; i < 4; i++) {
        if(dat.t_nesw[i] != ot_crater) {
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

void mapgen_field(map *m, int turn)
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

void mapgen_dirtlot(map *m, game *g)
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

void mapgen_forest_general(map *m, oter_id terrain_type, mapgendata dat, int turn)
{
    switch (terrain_type) {
    case ot_forest_thick:
        dat.fill(8);
        break;
    case ot_forest_water:
        dat.fill(4);
        break;
    case ot_forest:
        dat.fill(0);
        break;
    }
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] == ot_forest || dat.t_nesw[i] == ot_forest_water) {
            dat.dir(i) += 14;
        } else if (dat.t_nesw[i] == ot_forest_thick) {
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
                    m->spawn_item(i, j, "apple", turn);
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

    if (terrain_type == ot_forest_water) {
        // Reset *_fac to handle where to place water
        for (int i = 0; i < 4; i++) {
            if (dat.t_nesw[i] == ot_forest_water) {
                dat.set_dir(i, 2);
            } else if (dat.t_nesw[i] >= ot_river_center && dat.t_nesw[i] <= ot_river_nw) {
                dat.set_dir(i, 3);
            } else if (dat.t_nesw[i] == ot_forest || dat.t_nesw[i] == ot_forest_thick) {
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
            for (int j = 0; j < dat.n_fac; j++) {
                int wx = rng(0, SEEX * 2 -1), wy = rng(0, SEEY - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_grass ||
                    m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            for (int j = 0; j < dat.e_fac; j++) {
                int wx = rng(SEEX, SEEX * 2 - 1), wy = rng(0, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_grass ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            for (int j = 0; j < dat.s_fac; j++) {
                int wx = rng(0, SEEX * 2 - 1), wy = rng(SEEY, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_grass ||
                      m->ter(wx, wy) == t_underbrush) {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            for (int j = 0; j < dat.w_fac; j++) {
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

void mapgen_hive(map *m, mapgendata dat, int turn)
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

                if (dat.t_nesw[0] == ot_hive && dat.t_nesw[1] == ot_hive &&
                      dat.t_nesw[2] == ot_hive && dat.t_nesw[3] == ot_hive) {
                    m->place_items("hive_center", 90, i - 2, j - 2, i + 2, j + 2, false, turn);
                } else {
                    m->place_items("hive", 80, i - 2, j - 2, i + 2, j + 2, false, turn);
                }
            }
        }
    }
}

void mapgen_spider_pit(map *m, mapgendata dat, int turn)
{
    // First generate a forest
    dat.fill(4);
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] == ot_forest || dat.t_nesw[i] == ot_forest_water) {
            dat.dir(i) += 14;
        } else if (dat.t_nesw[i] == ot_forest_thick) {
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

void mapgen_road_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    bool sidewalks = false;
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] >= ot_house_north && dat.t_nesw[i] <= ot_abstorefront_west) {
            sidewalks = true;
        }
    }

    int veh_spawn_heading;
    if (terrain_type == ot_road_ew) {
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
    if (terrain_type == ot_road_ew)
        m->rotate(1);
    if(sidewalks)
        m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
        m->place_items("road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}

void mapgen_road_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density)
{
    bool sidewalks = false;
    for (int i = 0; i < 4; i++) {
        if (dat.t_nesw[i] >= ot_house_north && dat.t_nesw[i] <= ot_abstorefront_west) {
            sidewalks = true;
        }
    }

    m->add_road_vehicles(sidewalks, one_in(2) ? 90 : 180);
    if (sidewalks) { //this crossroad has sidewalk => this crossroad is in the city
        for (int i = 0; i < SEEX * 2; i++) {
            for (int j = 0; j < SEEY * 2; j++) {
                if ((i >= SEEX * 2 - 4 && j < 4) || i < 4 || j >= SEEY * 2 - 4) {
                    m->ter_set(i, j, t_sidewalk);
                } else {
                    if (((i == SEEX - 1 || i == SEEX) && j % 4 != 0 && j < SEEY - 1) ||
                          ((j == SEEY - 1 || j == SEEY) && i % 4 != 0 && i > SEEX)) {
                        m->ter_set(i, j, t_pavement_y);
                    } else {
                        m->ter_set(i, j, t_pavement);
                    }
                }
            }
        }
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
,,,,....................\n\
,,,,,...................\n\
,,,,,,..................\n\
,,,,,,,.................\n\
,,,,,,,,................\n\
,,,,,,,,,...............\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n\
,,,,,,,,,,,,,,,,,,,,,,,,\n",
        mapf::basic_bind(". , y", t_pavement, t_dirt, t_pavement_y),
        mapf::basic_bind(". , y", f_null, f_null, f_null, f_null));
    }
    if (terrain_type == ot_road_es)
        m->rotate(1);
    if (terrain_type == ot_road_sw)
        m->rotate(2);
    if (terrain_type == ot_road_wn)
        m->rotate(3); //looks like that the code above paints road_ne
    if(sidewalks)
        m->place_spawns(g, "GROUP_ZOMBIE", 2, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, density);
        m->place_items("road", 5, 0, 0, SEEX * 2 - 1, SEEX * 2 - 1, false, turn);
}
