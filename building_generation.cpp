#include "building_generation.h"
#include "output.h"
#include "item_factory.h"

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
    switch (dir_in)
    {
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
    switch (dir_in)
    {
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
    for (int i = 0; i < SEEX * 2; i++)
    {
        for (int j = 0; j < SEEY * 2; j++)
        {
            m->ter_set(i, j, t_null);
            m->radiation(i, j) = 0;
        }
    }
}

void mapgen_crater(map *m, mapgendata dat)
{
    for(int i = 0; i < 4; i++)
    {
        if(dat.t_nesw[i] != ot_crater)
        {
            dat.set_dir(i, 6);
        }
    }

    for (int i = 0; i < SEEX * 2; i++)
    {
       for (int j = 0; j < SEEY * 2; j++)
       {
           if (rng(0, dat.w_fac) <= i && rng(0, dat.e_fac) <= SEEX * 2 - 1 - i &&
               rng(0, dat.n_fac) <= j && rng(0, dat.s_fac) <= SEEX * 2 - 1 - j   )
           {
               m->ter_set(i, j, t_rubble);
               m->radiation(i, j) = rng(0, 4) * rng(0, 2);
           }
           else
           {
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

    for (int i = 0; i < SEEX * 2; i++)
    {
        for (int j = 0; j < SEEY * 2; j++)
        {
            m->ter_set(i, j, grass_or_dirt());
            if (one_in(bush_factor))
            {
                if (one_in(berry_bush_factor))
                {
                    m->ter_set(i, j, t_shrub_blueberry);
                }
                else
                if (one_in(berry_bush_factor))
                {
                    m->ter_set(i, j, t_shrub_strawberry);
                }
                else
                {
                    m->ter_set(i, j, t_shrub);
                }
            }
            else
            if (one_in(1000)) { m->furn_set(i,j, f_mutpoppy); }
        }
    }
    m->place_items("field", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn);
}

void mapgen_dirtlot(map *m, game *g)
{
    for (int i = 0; i < SEEX * 2; i++)
    {
        for (int j = 0; j < SEEY * 2; j++)
        {
            m->ter_set(i, j, t_dirt);
            if (one_in(120))
            {
                m->ter_set(i, j, t_pit_shallow);
            }
            else if (one_in(50))
            {
                m->ter_set(i,j, t_grass);
            }
        }
    }
    if (one_in(4))
    {
        m->add_vehicle (g, veh_truck, 12, 12, 90, -1, -1);
    }
}

void mapgen_forest_general(map *m, oter_id terrain_type, mapgendata dat, int turn)
{
    switch (terrain_type)
    {
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
    for (int i = 0; i < 4; i++)
    {
        if (dat.t_nesw[i] == ot_forest || dat.t_nesw[i] == ot_forest_water)
        {
            dat.dir(i) += 14;
        }
        else if (dat.t_nesw[i] == ot_forest_thick)
        {
            dat.dir(i) += 18;
        }
    }
    for (int i = 0; i < SEEX * 2; i++)
    {
        for (int j = 0; j < SEEY * 2; j++)
        {
            int forest_chance = 0, num = 0;
            if (j < dat.n_fac)
            {
                forest_chance += dat.n_fac - j;
                num++;
            }
            if (SEEX * 2 - 1 - i < dat.e_fac)
            {
                forest_chance += dat.e_fac - (SEEX * 2 - 1 - i);
                num++;
            }
            if (SEEY * 2 - 1 - j < dat.s_fac)
            {
                forest_chance += dat.s_fac - (SEEY * 2 - 1 - j);
                num++;
            }
            if (i < dat.w_fac)
            {
                forest_chance += dat.w_fac - i;
                num++;
            }
            if (num > 0)
            {
                forest_chance /= num;
            }
            int rn = rng(0, forest_chance);
            if ((forest_chance > 0 && rn > 13) || one_in(100 - forest_chance))
            {
                if (one_in(250))
                {
                    m->ter_set(i, j, t_tree_apple);
                    m->spawn_item(i, j, "apple", turn);
                }
                else
                {
                    m->ter_set(i, j, t_tree);
                }
            }
            else if ((forest_chance > 0 && rn > 10) || one_in(100 - forest_chance))
            {
                m->ter_set(i, j, t_tree_young);
            }
            else if ((forest_chance > 0 && rn >  9) || one_in(100 - forest_chance))
            {
                if (one_in(250))
                {
                    m->ter_set(i, j, t_shrub_blueberry);
                }
                else
                {
                    m->ter_set(i, j, t_underbrush);
                }
            }
            else
            {
                m->ter_set(i, j, t_dirt);
            }
        }
    }
    m->place_items("forest", 60, 0, 0, SEEX * 2 - 1, SEEY * 2 - 1, true, turn);

    if (terrain_type == ot_forest_water)
    {
        // Reset *_fac to handle where to place water
        for (int i = 0; i < 4; i++)
        {
            if (dat.t_nesw[i] == ot_forest_water)
            {
                dat.set_dir(i, 2);
            }
            else if (dat.t_nesw[i] >= ot_river_center && dat.t_nesw[i] <= ot_river_nw)
            {
                dat.set_dir(i, 3);
            }
            else if (dat.t_nesw[i] == ot_forest || dat.t_nesw[i] == ot_forest_thick)
            {
                dat.set_dir(i, 1);
            }
            else
            {
                dat.set_dir(i, 0);
            }
        }
        int x = SEEX / 2 + rng(0, SEEX), y = SEEY / 2 + rng(0, SEEY);
        for (int i = 0; i < 20; i++)
        {
            if (x >= 0 && x < SEEX * 2 && y >= 0 && y < SEEY * 2)
            {
                if (m->ter(x, y) == t_water_sh)
                {
                    m->ter_set(x, y, t_water_dp);
                }
                else if (m->ter(x, y) == t_dirt || m->ter(x, y) == t_underbrush)
                {
                    m->ter_set(x, y, t_water_sh);
                }
            }
            else
            {
                i = 20;
            }
            x += rng(-2, 2);
            y += rng(-2, 2);
            if (x < 0 || x >= SEEX * 2)
            {
                x = SEEX / 2 + rng(0, SEEX);
            }
            if (y < 0 || y >= SEEY * 2)
            {
                y = SEEY / 2 + rng(0, SEEY);
            }
            for (int j = 0; j < dat.n_fac; j++)
            {
                int wx = rng(0, SEEX * 2 -1), wy = rng(0, SEEY - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_underbrush)
                {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            for (int j = 0; j < dat.e_fac; j++)
            {
                int wx = rng(SEEX, SEEX * 2 - 1), wy = rng(0, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_underbrush)
                {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            for (int j = 0; j < dat.s_fac; j++)
            {
                int wx = rng(0, SEEX * 2 - 1), wy = rng(SEEY, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_underbrush)
                {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
            for (int j = 0; j < dat.w_fac; j++)
            {
                int wx = rng(0, SEEX - 1), wy = rng(0, SEEY * 2 - 1);
                if (m->ter(wx, wy) == t_dirt || m->ter(wx, wy) == t_underbrush)
                {
                    m->ter_set(wx, wy, t_water_sh);
                }
            }
        }
        int rn = rng(0, 2) * rng(0, 1) * (rng(0, 1) + rng(0, 1));// Good chance of 0
        for (int i = 0; i < rn; i++)
        {
            x = rng(0, SEEX * 2 - 1);
            y = rng(0, SEEY * 2 - 1);
            m->add_trap(x, y, tr_sinkhole);
            if (m->ter(x, y) != t_water_sh)
            {
                m->ter_set(x, y, t_dirt);
            }
        }
    }

    if (one_in(10000)) {  //1-2 per overmap, very bad day for low level characters
        m->add_spawn(mon_jabberwock, 1, SEEX, SEEY);
    }


    if (one_in(100)) // One in 100 forests has a spider living in it :o
    {
        for (int i = 0; i < SEEX * 2; i++)
        {
            for (int j = 0; j < SEEX * 2; j++)
            {
                if ((m->ter(i, j) == t_dirt || m->ter(i, j) == t_underbrush) && !one_in(3))
                {
                    m->add_field(NULL, i, j, fd_web, rng(1, 3));
                }
            }
        }
        m->add_spawn(mon_spider_web, rng(1, 2), SEEX, SEEY);
    }
}
