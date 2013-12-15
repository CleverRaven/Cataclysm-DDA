#ifndef _CONSTRUCTION_H_
#define _CONSTRUCTION_H_

#include "crafting.h"
#include "json.h"

#include <vector>
#include <string>

struct construct // Construction functions.
{
    // Checks for whether terrain mod can proceed
    bool check_nothing(point) { return true; }
    bool check_empty(point); // tile is empty
    bool check_support(point); // at least two orthogonal supports

    // Special actions to be run post-terrain-mod
    void done_nothing(point) {}
    void done_tree(point);
    void done_trunk_log(point);
    void done_trunk_plank(point);
    void done_vehicle(point);
    void done_deconstruct(point);
};

struct construction
{
    int id; // arbitrary internal identifier

    std::string description; // how the action is displayed to the player
    std::string skill;
    int difficulty; // carpentry skill level required
    int time; // time taken to construct, in minutes
    std::vector<std::vector<component> > tools; // tools required
    std::vector<std::vector<component> > components; // components required

    std::string pre_terrain; // beginning terrain for construction
    bool pre_is_furniture; // whether it's furniture or terrain
    std::set<std::string> pre_flags; // flags beginning terrain must have
    bool (construct::*pre_special)(point); // custom constructability check

    void (construct::*post_special)(point); // custom after-effects
    std::string post_terrain;// final terrain after construction
    bool post_is_furniture; // whether it's furniture or terrain
};

extern std::vector<construction*> constructions;

void load_construction(JsonObject &jsobj);
void construction_menu();
bool player_can_build(player &p, inventory inv, construction *con);
bool player_can_build(player &p, inventory pinv, const std::string &desc);
bool can_construct(construction *con, int x, int y);
bool can_construct(construction *con);
void place_construction(const std::string &desc);
void complete_construction();

#endif // _CONSTRUCTION_H_
