#include "base_home.h"
#include "submap.h"
#include "mapdata.h"
#include "map.h"

#include <unordered_map>
#include <list>
#include <set>
#include <queue>

base_home::base(player &p, const tripoint &coreloc){
    gen_overlay();
    define_base_area(base_map, coreloc);
    //TODO: Remove spawn points from base
}

/**
 * Populates bmap.  (Add references to all the tiles in a submap)
*/
void gen_overlay(){
    //TODO: Get help.
}

/**
 * Populates bmap.bflag based on the rest of bmap.  Finds area of base
 * 
 * CAUTION: WILL RESET THE BASE!!! (mostly)
 * TODO: Populate baflag with flags coresponding to the structure containing the command desk.
 * TODO: Populate storage_open with locations of storage furnature (lockers etc.)
 * TODO: Populate bunks with locations of sleeping areas (beds, cots, etc.)
 */
void base_home::define_base_area(const tripoint &coreloc) {
    //First we map out the base floor by basically BFS all the "INDOORS" flags.
    //Set up.
    std::queue q;
    std::set<tripoint> pushed; //not sure how big it needs to get before std::unordered_set is better than std::set.
    tripoint cur, temp;
    que.push(coreloc);
    pushed.insert(coreloc);
    //Make life easier for myself;
    auto &x = cur.x;
    auto &y = cur.y;
    auto &z = cur.z;
    auto &ter = bmap.ter;
    auto &frn = bmap.fur;
    auto &bf = bmap.bflag[x][y];
    auto &in = base_flag::BASE_IN;
    auto &wall = base_flag
    //Start search.
    while (!q.empty()){//While there are connected indoor tiles
        cur = q.pop();
        bf.push_back(in);//mark as in the base
        //Lets populate other things while we're at it.
        if ( store_fern.count(frn[x][y]) ) storage_open.push_back(cur); //Storage furnature
        if ( sleep_furn.count(frn[x][y]) ) bunks.push_back(cur, 0); //sleep furnature
        else if ( sleep_trap.count(bmap.trp[x][y]) ) bunks.push_back(cur, 0);//sleep traps (cots, rollmats)
        //now lets try and put the surrounding tiles in the que.
        temp = tripoint(x+1, y, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        }
        temp = tripoint(x-1, y, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        }
        temp = tripoint(x, y+1, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        } 
        temp = tripoint(x, y-1, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        }
        temp = tripoint(x+1, y+1, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        }
        temp = tripoint(x+1, y-1, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        }
        temp = tripoint(x-1 ,y+1, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        }
        temp = tripoint(x-1, y-1, z);
        if (ter[temp.x][temp.y].has_flag("INDOORS") && !pushed.count(temp) ){
            q.push(temp);
            pushed.push(temp);
        }
    }

    //at this point, bmap::bflag should contain a maping of base's floor space so,
    //now we add in base walls and account for walls, small celling holes and the like.
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