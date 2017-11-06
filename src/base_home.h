#pragma once
#ifndef BASE_HOME_H
#define BASE_HOME_h

#include "submap.h"
#include "ammo.h"
#include "string_id.h"

#include <unordered_map>
#include <set>
#include <list>

struct submap;
struct tripoint;
class ammunition_type;

//furniture accepted as a storage place.
//TODO: Move to json once I get around to figure out how cdda's json works
const std::set<furn_id> store_furn {
    "f_locker",
    "f_dresser",
    "f_rack",
    "f_bookcase",
    "f_safe_o"
}

//Need way to dermine if it counts as a bunk.
//Finding the "This seems like a nice place to sleep" code is annoying so,
//lets do the same should be in json hack for now
const std::set<furn_id> sleep_furn {//Furnature that counts as a bunk
    "f_bed",
    "f_makeshift_bed",
    "f_straw_bed"
}
const std::set<trap_id> sleep_trap {//Traps that count as a bunk
    "tr_cot",
    "tr_rollmat",
    "tr_fur_rollmat"
}


/**
 * Flags unique to a base.
 */
enum base_flag
{
    BASE_IN,   /**Inside of actual base*/
    BASE_GND,  /**On base grounds. i.e. between perimeter fence and building*/
    BASE_WALL, /**The outer walls of the base building.  Separates BASE_IN and BASE_GND*/
    BASE_EDGE, /**Outer edge of base area (perimeter wall/fence).  Used when base area extends beyond the building structure.*/
    NO_NPC,    /**NPCs stay out.  Friendly NPCs won't travel through tile with this flag.*/
    COMMUNAL,  /**Designated for communal use.  NPCs take and leave items here.*/
    OWNED_P,   /**Claimed by the player.  Friendly NPCs won't take from or use anything on this tile.*/
    OWNED_N,   /**Claimed by an NPC.  Takeing or useing stuff here will earn their anger.  Not used by other NPCs.*/
};

//furniture flags in json
//BASE_CORE -> Is the very heart and soul of a base.
//BASE_CMD# -> Level provided by control core.
//SYS_LINK -> Acts as a data conduit for the command system.  If adjacent or forms a path to BASE_CORE, allows placement of aux systems.
//AUX_SYS -> Is an auxiliary system. Must be built adjacent to BASE_CORE flag or CMD_SYS flag linked to a BASE_CORE flag.
//CONNECTABLE -> Possible to install network module
//NETWORKED -> Network enabled; able to form connection between command and remote system.

enum class ration_enum : char {
    Not Rationed = 0,
    Lightly Rationed = 1,
    Moderately Rationed = 2,
    Heavily Rationing = 3
}


class base_home
{

    base_home(player &p, const tripoint &coreloc);

  private:
                //########PRIVATE##########//
                //#BASE CORE AND LOCATIONS#//
                //#########################//

    //data members
    int base_level; /**Level of command core. Determins max # of aux sys and, use of certain features.*/
    int owner_id; /**Player's ID*/ 
    tripoint base_omap; /**Base's overmap location*/
    std::set<base_area_flag> bflag[OMAPX][OMPY]; /**Additional map tile overlay for flags specific to a base. NO-Z levelsupport.*/
    
    std::list<&npc_based> freeloaders;          /**List containing references to the NPCs staying at the base.*/

    std::unordered_map<tripoint, int> bunks;          /**Sleeping spots in base. <location, ownerID*/
    std::list<tripoint> storage_open;   /**List containing locations of unclaimed/designated storage furnature.*/
    std::list<tripoint> storage_player;   /**List containing locations of furnature claimed by player*/
    std::list<tripoint> storage_comm;   /**list containing locations of communal storage.*/
    //note: deal with deconstruction, destruction and, death. Unclaiming can be done by overwriting and adding handling there.*/
    //storage is broken up for performance consideratons.

    //functions
    void gen_overlay(const submap &base_map);
    void define_base_area(const submap &base_map, const tripoint &coreloc); //populates baflag[][]
    bool has_base_flag(const tripoint &p, base_flag flag)
    {
        return(bmap[p.x][p.y].bflag.count(flag) );
    }

                //########PRIVATE##########//
                //#ITEMS AND STASHED STUFF#//
                //#########################//    

    //functions
    void count_guns(); //update gun_count

                //########PRIVATE##########//
                //#RULES AND STUFF FOR NPC#//
                //#########################//

    //data members
    int food_ration = 0; /**Severity of food rationing. 0 = no rationing. Max = 3.*/
    int water_ration = 0;/**Severity of water/drink rationing. 0 = no rationing. Max = 3.*/
    int ammo_ration = 0; /**Severity of ammo rationing. 0 = no rationing. Max = 3.*/
    int med_ration = 0;  /**Severity of medicine/first aid rationing. 0 = no rationing. Max = 3.*/

    //functions

  public:
                //##########PUBLIC#########//
                //#BASE CORE AND LOCATIONS#//
                //#########################//

    bool is_in_base(const tripoint &); //Returns true if inside defined base area.
    void change_lv(int new_level);
    void run_design();

    int get_level(){
        return base_level;
    }
    int num_personnel(){
        return freeloaders.length();
    }
    std::list<&npc_based> get_personnel(){
        return freeloaders;
    }
    int get_max_pop(){
        return std::min(bunks.length(), storage_open.length() + storage_player.length() + storage_comm.length() );
    }
    int get_num_bunks(){
        return bunks.length();
    }
    int num_storage_open(){
        return storage_open.length();
    }
    int num_comm_storage(){
        return storage_comm.length();
    }

                //##########PUBLIC#########//
                //#ITEMS AND STORAGE STUFF#//
                //#########################//

                //##########PUBLIC#########//
                //#RULES AND STUFF FOR NPC#//
                //#########################//

    void set_food_ration(int val);
    void set_water_ration(int)
    void set_ammo_ration(int val);
    void set_med_ration(int val);

    int get_food_ration(){
        return food_ration;
    }
    int get_water_ration(){
        return drink_ration;
    }
    int get_ammo_ration(){
        return ammo_ration;
    }
    int get_med_ration(){
        return med_ration;
    }
};

#endif
