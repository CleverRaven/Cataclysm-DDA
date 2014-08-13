#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include <string>
#include <vector>
#include <map>
#include <list>
#include "itype.h"
#include "skill.h"
#include "rng.h"
#include "json.h"
#include "requirements.h"

#define MAX_DISPLAYED_RECIPES 18

typedef std::string craft_cat;
typedef std::string craft_subcat;

struct byproduct {
    itype_id result;
    int charges_mult;
    int amount;

    byproduct()
    {
        result = "null";
        charges_mult = 1;
        amount = 1;
    }

    byproduct(itype_id res, int mult = 1, int amnt = 1)
        : result(res), charges_mult(mult), amount(amnt)
    {
    }
};

struct recipe : public requirements {
    std::string ident;
    int id;
    itype_id result;
    std::vector<byproduct> byproducts;
    craft_cat cat;
    craft_subcat subcat;
    Skill *skill_used;
    std::map<Skill *, int> required_skills;
    int difficulty;
    bool reversible; // can the item be disassembled?
    bool autolearn; // do we learn it just by leveling skills?
    int learn_by_disassembly; // what level (if any) do we learn it by disassembly?
    int result_mult; // used by certain batch recipes that create more than one stack of the result
    bool paired;

    // only used during loading json data: books and the skill needed
    // to learn this recipe from.
    std::vector<std::pair<std::string, int> > booksets;

    //Create a string list to describe the skill requirements fir this recipe
    // Format: skill_name(amount), skill_name(amount)
    std::string required_skills_string();

    recipe()
    {
        id = 0;
        result = "null";
        skill_used = NULL;
        difficulty = 0;
        reversible = false;
        autolearn = false;
        learn_by_disassembly = -1;
        result_mult = 1;
        paired = false;
    }

    recipe(std::string pident, int pid, itype_id pres, craft_cat pcat, craft_subcat psubcat,
           std::string &to_use, std::map<std::string, int> &to_require, int pdiff,
           bool preversible, bool pautolearn, int plearn_dis, int pmult, bool ppaired,
           std::vector<byproduct> &bps) :
        ident (pident), id (pid), result (pres), cat(pcat), subcat(psubcat), difficulty (pdiff),
        reversible (preversible), autolearn (pautolearn), learn_by_disassembly (plearn_dis),
        result_mult(pmult), paired(ppaired)
    {
        skill_used = to_use.size() ? Skill::skill(to_use) : NULL;
        if(!to_require.empty()) {
            for(std::map<std::string, int>::iterator iter = to_require.begin(); iter != to_require.end();
                ++iter) {
                required_skills[Skill::skill(iter->first)] = iter->second;
            }
        }
        if(!bps.empty()) {
            for(auto &val : bps) {
                byproducts.push_back(val);
            }
        }
    }

    // Create an item instance as if the recipe was just finished,
    // Contain charges multiplier
    item create_result(int handed = NONE) const;

    // Create byproduct instances as if the recipe was just finished
    std::vector<item> create_byproducts() const;

    bool has_byproducts() const;

    bool can_make_with_inventory(const inventory &crafting_inv) const;

    int print_items(WINDOW *w, int ypos, int xpos, nc_color col);
    void print_item(WINDOW *w, int ypos, int xpos, nc_color col, const byproduct &bp);
};

typedef std::vector<recipe *> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

class item;
// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo(item *dis_item, player &p);
// same as above but for each item in the list
void remove_ammo(std::list<item> &dis_items, player &p);

void load_recipe_category(JsonObject &jsobj);
void reset_recipe_categories();
void load_recipe(JsonObject &jsobj);
void reset_recipes();
recipe *recipe_by_name(std::string name);
void finalize_recipes();
// Show the "really disassemble?" query along with a list of possible results.
// Returns false if the player answered no to the query.
bool query_dissamble(const recipe *cur_recipe, const item &dis_item);

extern recipe_map recipes; // The list of valid recipes

void check_recipe_definitions();

void set_item_spoilage(item &newit, float used_age_tally, int used_age_count);
void set_item_food(item &newit);
void set_item_inventory(game *g, item &newit);

#endif
