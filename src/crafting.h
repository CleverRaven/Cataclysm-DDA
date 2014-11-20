#ifndef CRAFTING_H
#define CRAFTING_H

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

enum TAB_MODE {
    NORMAL,
    FILTERED,
    BATCH
};

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

struct recipe {
    std::string ident;
    int id;
    itype_id result;
    int time; // in movement points (100 per turn)
    int difficulty;
    requirement_data requirements;
    std::vector<byproduct> byproducts;
    craft_cat cat;
    craft_subcat subcat;
    Skill *skill_used;
    std::map<Skill *, int> required_skills;
    bool reversible; // can the item be disassembled?
    bool autolearn; // do we learn it just by leveling skills?
    int learn_by_disassembly; // what level (if any) do we learn it by disassembly?

    // maximum achievable time reduction, as percentage of the original time.
    // if zero then the recipe has no batch crafting time reduction.
    double batch_rscale;
    int batch_rsize; // minimum batch size to needed to reach batch_rscale
    int result_mult; // used by certain batch recipes that create more than one stack of the result
    bool paired;

    // only used during loading json data: books and the skill needed
    // to learn this recipe from.
    std::vector<std::pair<std::string, int> > booksets;

    //Create a string list to describe the skill requirements fir this recipe
    // Format: skill_name(amount), skill_name(amount)
    std::string required_skills_string() const;

    ~recipe();
    recipe();
    recipe(std::string pident, int pid, itype_id pres, craft_cat pcat,
           craft_subcat psubcat, std::string &to_use,
           std::map<std::string, int> &to_require,
           bool preversible, bool pautolearn, int plearn_dis,
           int pmult, bool ppaired, std::vector<byproduct> &bps,
           int time, int difficulty, double batch_rscale,
           int batch_rsize);

    // Create an item instance as if the recipe was just finished,
    // Contain charges multiplier
    item create_result(int handed = NONE) const;
    std::vector<item> create_results(int batch = 1, int handed = NONE) const;

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

typedef std::vector<recipe *> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

extern recipe_map recipes; // The list of valid recipes

extern std::map<itype_id, recipe_list> recipes_by_component; // reverse lookup

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
                  std::vector<bool> &available, craft_cat tab,
                  craft_subcat subtab, std::string filter);
void batch_recipes(const inventory &crafting_inv,
                   std::vector<const recipe *> &current,
                   std::vector<bool> &available, const recipe* r);

const recipe *find_recipe( std::string id );
void check_recipe_definitions();

void set_item_spoilage(item &newit, float used_age_tally, int used_age_count);
void set_item_food(item &newit);
void set_item_inventory(item &newit);
void finalize_crafted_item(item &newit, float used_age_tally, int used_age_count);

#endif
