#ifndef CRAFTING_H
#define CRAFTING_H

#include "item.h"         // item
#include "requirements.h" // requirement_data
#include "cursesdef.h"    // WINDOW
#include "string_id.h"

#include <string>
#include <vector>
#include <map>
#include <list>

class recipe_dictionary;
class JsonObject;
class Skill;
using skill_id = string_id<Skill>;
class inventory;
class player;
struct recipe;

enum body_part : int; // From bodypart.h
typedef int nc_color; // From color.h

using itype_id     = std::string; // From itype.h

struct byproduct {
    itype_id result;
    int charges_mult;
    int amount;

    byproduct() : byproduct("null") {}

    byproduct(itype_id res, int mult = 1, int amnt = 1)
        : result(res), charges_mult(mult), amount(amnt)
    {
    }
};

struct recipe {
    std::string ident;
    int id;
    itype_id result;
    int time; // in movement points (100 per turn)
    int difficulty;
    requirement_data requirements;
    std::vector<byproduct> byproducts;
    std::string cat;
    bool contained; // Does the item spawn contained?
    std::string subcat;
    skill_id skill_used;
    std::map<skill_id, int> required_skills;
    bool reversible; // can the item be disassembled?
    bool autolearn; // do we learn it just by leveling skills?
    int learn_by_disassembly; // what level (if any) do we learn it by disassembly?

    // maximum achievable time reduction, as percentage of the original time.
    // if zero then the recipe has no batch crafting time reduction.
    double batch_rscale;
    int batch_rsize; // minimum batch size to needed to reach batch_rscale
    int result_mult; // used by certain batch recipes that create more than one stack of the result

    // only used during loading json data: book_id is the id of an book item, other stuff is copied
    // into @ref islot_book::recipes.
    struct bookdata_t {
        std::string book_id;
        int skill_level;
        std::string recipe_name;
        bool hidden;
    };
    std::vector<bookdata_t> booksets;

    //Create a string list to describe the skill requirements fir this recipe
    // Format: skill_name(amount), skill_name(amount)
    std::string required_skills_string() const;

    recipe();

    // Create an item instance as if the recipe was just finished,
    // Contain charges multiplier
    item create_result() const;
    std::vector<item> create_results(int batch = 1) const;

    // Create byproduct instances as if the recipe was just finished
    std::vector<item> create_byproducts(int batch = 1) const;

    bool has_byproducts() const;

    bool can_make_with_inventory(const inventory &crafting_inv, int batch = 1) const;
    bool check_eligible_containers_for_crafting(int batch = 1) const;

    int print_items(WINDOW *w, int ypos, int xpos, nc_color col, int batch = 1) const;
    void print_item(WINDOW *w, int ypos, int xpos, nc_color col,
                    const byproduct &bp, int batch = 1) const;
    int print_time(WINDOW *w, int ypos, int xpos, int width, nc_color col,
                   int batch = 1) const;

    int batch_time(int batch = 1) const;

};

// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo(item *dis_item, player &p);
// same as above but for each item in the list
void remove_ammo(std::list<item> &dis_items, player &p);

void load_recipe(JsonObject &jsobj);
void reset_recipes();
const recipe *recipe_by_index(int index);
const recipe *recipe_by_name(const std::string &name);
const recipe *get_disassemble_recipe(const itype_id &type);
void finalize_recipes();
// Show the "really disassemble?" query along with a list of possible results.
// Returns false if the player answered no to the query.
bool query_dissamble(const item &dis_item);
const recipe *select_crafting_recipe(int &batch_size);
void pick_recipes(const inventory &crafting_inv,
                  std::vector<const recipe *> &current,
                  std::vector<bool> &available, std::string tab,
                  std::string subtab, std::string filter);
void batch_recipes(const inventory &crafting_inv,
                   std::vector<const recipe *> &current,
                   std::vector<bool> &available, const recipe* r);

void check_recipe_definitions();

void set_item_spoilage(item &newit, float used_age_tally, int used_age_count);
void set_item_food(item &newit);
void set_item_inventory(item &newit);
void finalize_crafted_item(item &newit, float used_age_tally, int used_age_count);

#endif
