#include "base_home.h"
#include "submap.h"
#include "mapdata.h"
#include "map.h"

#include <unordered_map>
#include <list>
#include <set>

base_home::base(submap &base_map,const tripoint &coreloc){
    base::define_base_area(base_map, coreloc);
    base_map.spawns.clear();//remove spawns so nothing apears in base just outside it.
    base_level;
    food_ration_lv = ammo_ration_lv = med_ration_lv; //no rationing to start
}

/**
 * Check if location has a base specific flag.
 */
bool base_home::has_base_flag(const tripoint &p, base_area_flag flag)
{
    return(baflag[p.x][p.y].count(flag))
}

/**
 * Populates data members containing information read from the map.
 * 
 * CAUTION: WILL RESET THE BASE!!! (mostly)
 * TODO: Populate baflag with flags coresponding to the structure containing the command desk.
 * TODO: Populate storage_open with locations of storage furnature (lockers etc.)
 * TODO: Populate bunks with locations of sleeping areas (beds, cots, etc.)
 */
void base_home::define_base_area(const submap &base_map, const tripoint &coreloc)
{
    //for easy reference
    int x = coreloc.x;
    int y = coreloc.y;

    //TODO:  Re-write. Previous implementation wasn't going to work. (immpossible to distinguish between underground and thick rock wall)
    ///soo much work wasted (T_T)

    //TODO: basically implement bfs
}

/**
 * Recount # of guns using each ammo type in base.
 *
 * TODO: Count the guns in communal storage.
 * TODO: Multiply by burst size.
 * TODO: Count NPC and Player Guns.
 */
void base_home::count_guns()
{
    for (int i=0; i<communal_storage.length(); i++){
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

/**
 * Apply change in base level.
 * 
 * Possibly will require more than changing base_level in future.
 */
void base_home::change_lv(int new_level){
    base_level = new_level;
}

/**
 * Run "HomeMakeoverDesignStudio.exe" basically wants to start a simplified map editor so you can plan your base's development.
 * Ideally NPCs could then follow said plan and work while you are away.
 */
void base_home::run_design(){
    add_msg("Not implemented yet.\n(╮°-°)╮┳━━┳ ( ╯°□°)╯ ┻━━┻");
}