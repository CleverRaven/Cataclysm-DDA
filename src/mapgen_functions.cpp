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

void mapgen_null(map *m)
{
    debugmsg("Generating null terrain, please report this as a bug");
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

void mapgen_dirtlot(map *m)
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
                m->ter_set(i, j, grass_or_dirt());
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

void mapgen_spider_pit(map *m, mapgendata dat, int turn)
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
                m->ter_set(i, j, grass_or_dirt());
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

void mapgen_fungal_bloom(map *m)
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
ssss....................\n\
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
        mapf::basic_bind(". , y s", t_pavement, t_null, t_pavement_y, t_sidewalk),
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
        mapf::basic_bind(". , y", t_pavement, t_null, t_pavement_y),
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

void mapgen_bridge(map *m, oter_id terrain_type, int turn)
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

void mapgen_highway(map *m, oter_id terrain_type, int turn)
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

void mapgen_river_curved_not(map *m, oter_id terrain_type)
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

void mapgen_river_straight(map *m, oter_id terrain_type)
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

void mapgen_river_curved(map *m, oter_id terrain_type)
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

void mapgen_parking_lot(map *m, mapgendata dat, int turn)
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

void mapgen_pool(map *m)
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
    m->add_spawn("mon_zombie_swimmer", rng(1, 6), SEEX, SEEY);
}

void mapgen_park(map *m)
{
    if (one_in(3)) { // Playground
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

    } else { // Basketball court
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
    }
    m->add_spawn("mon_zombie_child", rng(2, 8), SEEX, SEEY);
}

void mapgen_gas_station(map *m, oter_id terrain_type, float density)
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
