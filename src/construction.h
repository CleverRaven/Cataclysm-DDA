#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H

#include "requirements.h"
#include "cursesdef.h" // WINDOW
#include "enums.h" // point

#include <string>
#include <set>
#include <functional>

class JsonObject;
enum nc_color : int;

struct construction
{
    std::string category; //Construction type category
    std::string description; // how the action is displayed to the player
    std::string skill;
    std::string pre_terrain; // beginning terrain for construction
    std::string post_terrain;// final terrain after construction

    std::set<std::string> pre_flags; // flags beginning terrain must have

    requirement_data requirements;

    int id; // arbitrary internal identifier
    int time;
    int difficulty;

    bool (*pre_special)(point); // custom constructability check
    void (*post_special)(point); // custom after-effects

    bool pre_is_furniture; // whether it's furniture or terrain
    bool post_is_furniture; // whether it's furniture or terrain

    int print_time(WINDOW *w, int ypos, int xpos, int width, nc_color col) const;
};

void for_each_construction(std::function<void (construction&)> f);
void remove_construction_if(std::function<bool (construction&)> pred);

void load_construction(JsonObject &jsobj);
void reset_constructions();
void construction_menu();
void complete_construction();
void check_constructions();

#endif
