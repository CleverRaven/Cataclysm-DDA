#include "base_home.h"
#include "submap.h"
#include "mapdata.h"
#include "map.h"

#include <unordered_map>
#include <list>
#include <set>
#include <queue>
#include <unique_ptr>

base_home::base(player &p, const tripoint &coreloc){
    g->bases.push_back(std::unique_ptr<base_home> this);
    define_base_area(base_map, coreloc);
    owner_id = p.id;
    //TODO: Remove spawn points from base
}]

/**
 * Populates bmap.bflag based on the rest of bmap.  Finds area of base
 * 
 * CAUTION: WILL RESET THE BASE!!! (mostly)
 * TODO: Populate baflag with flags coresponding to the structure containing the command desk.
 * TODO: Populate storage_open with locations of storage furnature (lockers etc.)
 * TODO: Populate bunks with locations of sleeping areas (beds, cots, etc.)
 */
void base_home::define_base_area(player &p, const tripoint &coreloc) {
    //First we map out the base floor by basically BFS all the "INDOORS" flags.
    //Set up.
    std::queue q;
    std::set<tripoint> pushed; //not sure how big it needs to get before std::unordered_set is better than std::set.
    tripoint cur, temp;
    que.push(coreloc);
    pushed.insert(coreloc);
    //Start search.
    while (!q.empty()){//While there are connected indoor tiles
        cur = q.pop();
        bflag[cur.x][cur.y].push_back(BASE_IN);//mark as in the base
        //Lets populate other things while we're at it.
        if ( store_fern.count(g->m->furn(cur) ) ) storage_open.push_back(cur); //Storage furnature
        if ( sleep_furn.count(g->m->furn(cur) ) ) bunks.push_back(cur, 0); //sleep furnature
        else if ( sleep_trap.count(g->m->trp[(cur) ) ) bunks.push_back(cur, 0);//sleep traps (cots, rollmats)
        //now lets try and put the surrounding tiles in the que.
        static const std::array<point, 8> offsets = {{
            { 0, 1 }, { 0, -1 }, { 1, 0 }, { 1, 1 }, { 1, -1 }, { -1. 0 }, { -1 , 1 }, { -1, -1 }
        }};
        for( const point &off : offsets ) {
            tripoint temp = cur + off
            if bmap.has_flag_ter("INDOORS", temp){
                q.push(temp);
                pushed.push_back(temp);
            }
        }
    }    
    /*
    At this point, bflag should contain a maping of base's floor space so, now we need to fill in the
        interior walls and mark the base walls.

    The base "walls" are defined as:
        Terrain with the "CONNECT_TO_WALL" flag that is between a "BASE_IN" and the outside.
        OR, the first outside tile should their be no wall.
        OR, the last tile within bounds should structure exceed allowed size.

        To prevent issues from clasped roofs or unusual structures,
            gaps of up to 2 tiles non "BASE_IN" are allowed to still be "in base"

        i=indoor ter, .=non-indoor ter, |=wall,     B= in base, W= base wall, O= Outside base,
        iii|... -> BBBWOOO
        ii..ii  -> BBBBBB
        i|.ii   -> BBBBBB
        i|..ii  -> BWOOWB
        ii||ii  -> BBBBBB
        i|.|i   -> BWOWB
    */

    //first the North/South walls
    for (int col = 0; col < OMAPX; col++){

    }
    //then the East/West walls
    for (int row = 0; row < OMAPY; row++){

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
void base_home::run_designer(){
    add_msg("Not implemented yet.\n(╮°-°)╮┳━━┳ ( ╯°□°)╯ ┻━━┻");
}

/**
 * Mark something on/in the base; claim ownership, designate or other signs.
 * 
 * Generic function for making markings and claiming or designating furnature.
 * NPCs will claim ownership of furnature at tripoint.
 * Players will have more options.
 * 
 * @param person If claiming, the person who will become the owner.
 * @param loc Where the mark is being made.
 * @param ismarker Boolean used for prefix generation. true=marker, false=paint
 * @return Returns true if mark is made.
 */  
bool base_home::make_mark(character &person, const tripoint &loc, const bool ismarker = true){
    if (!p.is_npc()){//they are a player
        
    } else {//they are a npc
        if (storage_open.count(loc)) {
            //TODO: updated NPC
            storage_open.remove(loc);
            bflag[loc.x][loc.y].push_back(OWNED_N);
        } else if (bunks.count(loc)) {
            //TODO: updated NPC
            bunks.remove(loc);
            bflag[loc.x][loc.y].push_back(OWNED_N);
    }
    return false;
}