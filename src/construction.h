#ifndef CONSTRUCTION_H
#define CONSTRUCTION_H

#include "json.h"
#include "requirements.h"
#include "skill.h"
#include "enums.h"

#include <vector>
#include <string>

struct construct;

struct construction
{
    int id; // arbitrary internal identifier

    std::string category; //Construction type category

    std::string description; // how the action is displayed to the player
    std::string skill;
    int time;
    int difficulty;
    requirement_data requirements;

    std::string pre_terrain; // beginning terrain for construction
    bool pre_is_furniture; // whether it's furniture or terrain
    std::set<std::string> pre_flags; // flags beginning terrain must have
    bool (construct::*pre_special)(point); // custom constructability check

    void (construct::*post_special)(point); // custom after-effects
    std::string post_terrain;// final terrain after construction
    bool post_is_furniture; // whether it's furniture or terrain

    int print_time(WINDOW *w, int ypos, int xpos, int width, nc_color col) const;
};

extern std::vector<construction*> constructions;

void load_construction(JsonObject &jsobj);
void reset_constructions();
void construction_menu();
void complete_construction();
void check_constructions();

#endif
