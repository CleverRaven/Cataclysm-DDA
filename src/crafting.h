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

class JsonObject;
class Skill;
using skill_id = string_id<Skill>;
class inventory;
class player;
struct recipe;

enum body_part : int; // From bodypart.h
typedef int nc_color; // From color.h

using itype_id     = std::string; // From itype.h

// Global list of valid recipes
extern std::map<std::string, std::vector<recipe *>> recipes;
// Global reverse lookup
extern std::map<itype_id, std::vector<recipe *>> recipes_by_component;

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

    ~recipe();
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

enum usage {
    use_from_map = 1,
    use_from_player = 2,
    use_from_both = 1 | 2,
    use_from_none = 4,
    cancel = 5
};

template<typename CompType>
struct comp_selection {
    usage use_from = use_from_none;
    CompType comp;
};
using item_selection = comp_selection<item_comp>;
using tool_selection = comp_selection<tool_comp>;

class craft_command {
    public:
        const recipe* rec = nullptr;
        int batch_size = 0;
        bool is_long = false;
        player* crafter; // This is mainly here for maintainability reasons.

        craft_command() {}
        craft_command( const recipe* to_make, int batch_size, bool is_long, player* crafter ) :
            rec( to_make ), batch_size( batch_size ), is_long( is_long ), crafter( crafter ) {}

        void execute();
        std::list<item> consume_components();

        bool has_cached_selections()
        {
            return !item_selections.empty() || !tool_selections.empty();
        }

        bool empty()
        {
            return rec == nullptr;
        }
    private:
        std::vector<item_selection> item_selections;
        std::vector<tool_selection> tool_selections;

        std::list<item_selection> check_item_components_missing( const inventory* map_inv );
        std::list<tool_selection> check_tool_components_missing( const inventory* map_inv );
};

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
                  std::vector<bool> &available, std::string tab,
                  std::string subtab, std::string filter);
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
