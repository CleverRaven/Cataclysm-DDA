#include "base_home.h"
#include "submap.h"
#include <unordered_map>
#include "mapdata.h"
#include "map.h"

base_home::base(submap &base_map,const tripoint &coreloc){
    base::define_base_area(base_map, coreloc);
    base_map.spawns.clear();//remove spawns so nothing apears in base just outside it.
}

bool base_home::is_in_base(const tripoint &p)
{
    return(baflag[p.x][p.y])
}

void base_home::define_base_area(const submap &base_map, const tripoint &coreloc)
{
    //find base_wall
    int x = coreloc.x;
    int y = coreloc.y;

    //scan north from core until outside wall/edge is found.  stops at either the wall/window or first outside tile if no wall exists
    while (!base_map.get_ter(x, y)->has_flag("CONNECT_TO_WALL")|| base_map.get_ter(x, y)->has_flag("INDOORS")) {
        while(base_map.get_ter(x, y + 1)->has_flag("CONNECT_TO_WALL") || base_map.get_ter(x, y)->has_flag("INDOORS")) {
            y++;
        }
    }

    //follow walls/structure edge around until you get back to starting point.  uses tripoint's z value as boolean by setting it to impossible value when done
    tripoint p = tripoint(x, y, coreloc.z);
    while (p.z != -99) {
        //Track East. point at n/s wall. if east is wall or border it is also north wall;
        if (base_map.get_ter(p.x + 1, p.y)->has_flag("CONNECT_TO_WALL") || (!base_map.get_ter(p.x + 1, p.y)->has_flag("INDOORS") && (base_map.get_ter(p.x + 1, p.y - 1)->has_flag("INDOORS") || base_map.get_ter(p.x + 1, p.y + 1)->has_flag("INDOORS"))))
            p = base::wall_east(base_map, p);
        //Track South. point at corner of e/w wall.
        else if (base_map.get_ter(p.x , p.y - 1)->has_flag("CONNECT_TO_WALL") || (!base_map.get_ter(p.x , p.y - 1)->has_flag("INDOORS") && (base_map.get_ter(p.x + 1, p.y - 1)->has_flag("INDOORS") || base_map.get_ter(p.x - 1, p.y - 1)->has_flag("INDOORS"))))
            p = base::wall_south(base_map, p);
        //Track West. Point at corner of n/s wall.
        else if (base_map.get_ter(p.x - 1, p.y)->has_flag("CONNECT_TO_WALL") || (!base_map.get_ter(p.x - 1, p.y)->has_flag("INDOORS") && (base_map.get_ter(p.x - 1, p.y - 1)->has_flag("INDOORS") || base_map.get_ter(p.x - 1, p.y + 1)->has_flag("INDOORS"))))
            p = base::wall_west(base_map, p);
        //Track North. point at corner of e/w wall.
        else if (base_map.get_ter(p.x, p.y + 1)->has_flag("CONNECT_TO_WALL") || (!base_map.get_ter(p.x , p.y + 1)->has_flag("INDOORS") && (base_map.get_ter(p.x + 1, p.y + 1)->has_flag("INDOORS") || base_map.get_ter(p.x - 1, p.y + 1)->has_flag("INDOORS"))))
            p = base::wall_north(base_map, p);
        //TODO: handle error
        else{
            p.z = -99;
        }
    }

    //We have the base's outline now lets fill it in
    bool mark=false;
    for each (int y in baflag[0][y])
    {
        for each (int x in baflag[x][0]) {
            if (baflag[x][y].back==BASE_WALL)
                mark = !mark;
            else if (mark)
                baflag[x][y].push_back(BASE_IN);
        }
    }
}


void base_home::count_guns()
{
    for (int i=0; i<communal_storage.length; i++){
        for (int k=0; i<)
    }
}

void base_home::set_food_ration(int val)
{
    food_ration_lv = val;
}

void base_home::set_ammo_ration(int val)
{
    ammo_ration_lv = val;
}

void base_home::set_med_ration(int val)
{
    med_ration_lv = val;
}