#pragma once
#ifndef BASE_HOME
#define BASE_HOME

#include "submap.h"
#include <unordered_map>
#include "ammo.h"
#include "string_id.h"

struct submap;
struct tripoint;
class ammunition_type;

using furn_id = int_id<furn_t>;
using ammotype_id = string_id<ammunition_type>;

/**
 * Flags unique to a base.
 */
enum base_area_flag
{
    BASE_IN,   /**Inside of actual base*/
    BASE_GND,  /**On base grounds. i.e. between perimeter fence and building*/
    BASE_WALL, /**Terrain seperating BASE_IN and BASE_GND*/
    BASE_EDGE, /**outer edge of base area*/
    NO_NPC,    /**NPCs stay out*/
};

//furniture flags in json
//BASE_CORE -> Is the very heart and soul of a base.
//BASE_CMD# -> Level provided by control core.
//CMD_SYS -> Acts as a data conduit for the command system.  If adjacent or forms a path to BASE_CORE, allows placement of aux systems.
//AUX_SYS -> Is an auxiliary system. Must be built adjacent to BASE_CORE flag or CMD_SYS flag linked to a BASE_CORE flag.
//Connectable -> Possible to install network module
//NETWORKED -> Network enabled; able to form connection between command and remote system.

/**
 * Stores capacities of command system and the auxiliary systems present.
 */
struct command_sys
{
    bool network = false;  /**Is the command system wifi enabled? (cmd core must be networked)*/
    bool auto_conn = true; /**Automaticaly connect to remote systems if able*/
    int max_conn = 0;      /**Network capacity (in generic units).*/
    int max_cpu;           /**Maximum processing power (in generic units.*/

    std::unordered_map<furn_id, int> aux_sys;    /**Maps <auxiliary system type, # in command system>*/
    std::unordered_map<furn_id, int> remote_sys; /**Maps <remote system type, # connected to command system*/
};

class base_home
{

    base(submap &base_map, const tripoint &coreloc);

  private:
    //########PRIVATE##########//
    //#BASE CORE AND LOCATIONS#//
    //#########################//

    //data members
    int base_level; /**Level of command core. Determins max # of aux sys and, use of certain features.*/
    command_sys cmd_sys;

    std::list<base_area_flag> baflag[SEEX][SEEY]; /**additional ter and fur flags specific to bases.*/
    std::list<npc_based *> freeloaders;           /**list containing references to the NPCs staying at the base.*/

    std::list<tripoint> bunks;        /**list containing locations of sleeping spots in base.*/
    std::list<tripoint> storage_open; /**list containing locations of unclaimed/designated storage furnature.*/

    //functions
    void define_base_area(const submap &base_map, const tripoint &coreloc); //populates baflag[][]

    //########PRIVATE##########//
    //#ITEMS AND STASHED STUFF#//
    //#########################//

    //data members
    std::unordered_map<ammotype_id, int> gun_count;  /**stores <ammo_type, #guns_that_use_them>.*/
    std::unordered_map<ammotype_id, int> ammo_count; /**stores <ammo_types, amount>.*/

    std::list<tripoint> storage_comm; /**list containing locations of communal storage.*/

    //functions
    void count_guns(); //update gun_count

    //########PRIVATE##########//
    //#RULES AND STUFF FOR NPC#//
    //#########################//

    //data members
    int food_ration_lv; /**Severity of food rationing. 0 = no rationing. Max = 3.*/
    int ammo_ration_lv; /**Severity of ammo rationing. 0 = no rationing. Max = 3.*/
    int med_ration_lv;  /**Severity of medicine/first aid rationing. 0 = no rationing. Max = 3.*/

    //functions

  public:
    //##########PUBLIC#########//
    //#BASE CORE AND LOCATIONS#//
    //#########################//

    bool is_in_base(const tripoint &); //Returns true if inside defined base area.
    void change_lv(int new_level);
    void run_design();

    inline int get_level()
    {
        return base_level;
    }
    inline int get_max_pop()
    {
        return max(bunks.length, storage_open.length) + npcs.length;
    }

    //##########PUBLIC#########//
    //#ITEMS AND STORAGE STUFF#//
    //#########################//

    //##########PUBLIC#########//
    //#RULES AND STUFF FOR NPC#//
    //#########################//

    void set_food_ration(int val);
    void set_ammo_ration(int val);
    void set_med_ration(int val);

    inline int get_food_ration()
    {
        return food_ration_lv;
    }
    inline int get_ammo_ration()
    {
        return ammo_ration_lv;
    }
    inline int get_med_ration()
    {
        return med_ration_lv;
    }
};

#endif
