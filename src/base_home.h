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

class base_home
{
  private:
    ///////////////////////////
    //BASE CORE AND LOCATIONS//
    ///////////////////////////

    //data members
    std::list<base_area_flag> baflag[SEEX][SEEY]; /**additional ter and fur flags specific to bases.*/

    std::list<tripoint> bunks;        /**list containing locations of sleeping spots in base.*/
    std::list<tripoint> storage_open; /**list containing locations of unclaimed/designated storage furnature.*/

    //functions
    void define_base_area(const submap &base_map, const tripoint &coreloc); //populates baflag[][]

    ///////////////////////////
    //ITEMS AND STASHED STUFF//
    ///////////////////////////

    //data members
    std::unordered_map<ammotype_id, int> gun_count;  /**stores <ammo_type, #guns_that_use_them>.*/
    std::unordered_map<ammotype_id, int> ammo_count; /**stores <ammo_types, amount>.*/

    std::list<tripoint> storage_comm; /**list containing locations of communal storage.*/

    //functions
    void count_guns(); //update gun_count

    ///////////////////////////
    //RULES AND STUFF FOR NPC//
    ///////////////////////////

    //data members
    int base_level;     /**Used to determine if base functions are 'unlocked'.*/
    int food_ration_lv; /**Severity of food rationing. 0 = no rationing. Max = 3.*/
    int ammo_ration_lv; /**Severity of ammo rationing. 0 = no rationing. Max = 3.*/
    int med_ration_lv;  /**Severity of medicine/first aid rationing. 0 = no rationing. Max = 3.*/

    //functions

  public:
    ///////////////////////////
    //BASE CORE AND LOCATIONS//
    ///////////////////////////

    base(submap &base_map, const tripoint &coreloc);
    bool is_in_base(const tripoint &); //Returns true if inside defined base area.
    void change_lv(int new_level);
    inline int get_level()
    {
        return base_level;
    }

    ///////////////////////////
    //ITEMS AND STORAGE STUFF//
    ///////////////////////////

    ///////////////////////////
    //RULES AND NPC BEHAVIOR///
    ///////////////////////////

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
