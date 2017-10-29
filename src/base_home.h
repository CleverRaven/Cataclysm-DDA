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

enum base_area_flag{
    BASE_IN, //Inside of actual base
    BASE_GND, //on base grounds. i.e. between perimiter fence and building
    BASE_WALL, //terrain seperating BASE_IN and BASE_GND
    BASE_EDGE, //outer edge of base area
    NO_NPC,  //NPCs stay out
};

class base_home
{
    //data members
    int base_level;
    int food_ration_lv;
    int ammo_ration_lv;
    int med_ration_lv;
    std::unordered_map<ammotype_id, int> gun_count; //stores ammo_type, #guns_that_use_them*burst size
    std::unordered_map<ammotype_id, int> ammo_count; //stores ammo_types, ammount

    std::list<base_area_flag> baflag[SEEX][SEEY];  //aditional ter and fur flags specific to bases
    std::list<tripoint> communal_storage;//list of locations of communal storage

    void count_guns();
    void define_base_area(const submap &base_map, const tripoint &coreloc);  //populates baflag[][]

    public:
        
        base(submap &base_map, const tripoint &coreloc);
        bool is_in_base(const tripoint &);  //returns true if inside defined base area.
        void set_food_ration(int val);
        void set_ammo_ration(int val);
        void set_med_ration(int val);
        inline int get_level(){
            return base_level;
        }
};

#endif
