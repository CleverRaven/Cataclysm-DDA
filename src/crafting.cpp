#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "json.h"
#include "input.h"
#include "game.h"
#include "options.h"
#include "output.h"
#include "crafting.h"
#include "inventory.h"
#include "item_factory.h"
#include "catacharset.h"
#include "messages.h"
#include <queue>
#include <math.h>    //sqrt
#include <algorithm> //std::min

std::vector<craft_cat> craft_cat_list;
std::map<craft_cat, std::vector<craft_subcat> > craft_subcat_list;
recipe_map recipes;
std::map<std::string, quality> qualities;

bool any_marked_available(const std::vector<component> &comps);
static void draw_recipe_tabs(WINDOW *w, craft_cat tab, bool filtered = false);
static void draw_recipe_subtabs(WINDOW *w, craft_cat tab, craft_subcat subtab,
                                bool filtered = false);
static craft_cat first_craft_cat();
static craft_cat next_craft_cat(const craft_cat cat);
static craft_cat prev_craft_cat(const craft_cat cat);
static craft_subcat first_craft_subcat(const craft_cat cat);
static craft_subcat last_craft_subcat(const craft_cat cat);
static craft_subcat next_craft_subcat(const craft_cat cat, const craft_subcat subcat);
static craft_subcat prev_craft_subcat(const craft_cat cat, const craft_subcat subcat);

void load_recipe_category(JsonObject &jsobj)
{
    JsonArray subcats;
    std::string category = jsobj.get_string("id");
    // Don't store noncraft as a category.
    // We're storing the subcategory so we can look it up in load_recipes
    // for the fallback subcategory.
    if( category != "CC_NONCRAFT" ) {
        craft_cat_list.push_back( category );
    }
    craft_subcat_list[category] = std::vector<craft_subcat>();
    subcats = jsobj.get_array("recipe_subcategories");
    while (subcats.has_more()) {
        craft_subcat_list[category].push_back( subcats.next_string() );
    }
}

void reset_recipe_categories()
{
    craft_cat_list.clear();
    craft_subcat_list.clear();
}

void load_obj_list(JsonArray &jsarr, std::vector< std::vector<component> > &objs) {
    while (jsarr.has_more()) {
        std::vector<component> choices;
        JsonArray ja = jsarr.next_array();
        while (ja.has_more()) {
            JsonArray comp = ja.next_array();
            std::string name = comp.get_string(0);
            int quant = comp.get_int(1);
            choices.push_back(component(name, quant));
        }
        if (!choices.empty()) {
            objs.push_back(choices);
        }
    }
}

// Check that the given recipe ident (rec_name) is unique, throw if not,
// Returns the id for the new recipe.
// If the recipe should override an existing one, the function removes the existing
// recipe and returns the id if the removed recipe.
int check_recipe_ident(const std::string &rec_name, JsonObject &jsobj)
{
    const bool override_existing = jsobj.get_bool("override", false);
    int recipe_count = 0;
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter) {
        for (recipe_list::iterator list_iter = map_iter->second.begin();
             list_iter != map_iter->second.end(); ++list_iter) {
            if ((*list_iter)->ident == rec_name) {
                if (!override_existing) {
                    jsobj.throw_error(std::string("Recipe name collision (set a unique value for the id_suffix field to fix): ") + rec_name, "result");
                }
                // overriding an existing recipe: delete it and remove the pointer
                // keep the id,
                const int tmp_id = (*list_iter)->id;
                delete *list_iter;
                map_iter->second.erase(list_iter);
                return tmp_id;
            }
        }
        recipe_count += map_iter->second.size();
    }
    return recipe_count;
}

void load_recipe(JsonObject &jsobj)
{
    JsonArray jsarr;

    // required
    std::string result = jsobj.get_string("result");
    std::string category = jsobj.get_string("category");
    std::string subcategory = "";

    if ( !jsobj.has_string("subcategory") ) {
        subcategory = last_craft_subcat( category );
    } else {
        subcategory = jsobj.get_string("subcategory");
    }

    int difficulty = jsobj.get_int("difficulty");
    int time = jsobj.get_int("time");
    bool autolearn = jsobj.get_bool("autolearn");
    // optional
    bool reversible = jsobj.get_bool("reversible", false);
    std::string skill_used = jsobj.get_string("skill_used", "");
    std::string id_suffix = jsobj.get_string("id_suffix", "");
    int learn_by_disassembly = jsobj.get_int("decomp_learn", -1);
    int result_mult = jsobj.get_int("result_mult", 1);

    std::map<std::string, int> requires_skills;
    jsarr = jsobj.get_array("skills_required");
    if (!jsarr.empty()) {
        // could be a single requirement, or multiple
        try {
            // try to parse as single requirement
            requires_skills[jsarr.get_string(0)] = jsarr.get_int(1);
        } catch (std::string e) {
            // get_string or get_int failed, so assume array of arrays
            while (jsarr.has_more()) {
                JsonArray ja = jsarr.next_array();
                requires_skills[ja.get_string(0)] = ja.get_int(1);
            }
        }
    }

    std::string rec_name = result + id_suffix;
    int id = check_recipe_ident(rec_name, jsobj);

    recipe *rec = new recipe(rec_name, id, result, category, subcategory, skill_used,
                             requires_skills, difficulty, time, reversible,
                             autolearn, learn_by_disassembly, result_mult);

    jsarr = jsobj.get_array("components");
    load_obj_list(jsarr, rec->components);

    jsarr = jsobj.get_array("qualities");
    while(jsarr.has_more()) {
        JsonObject quality_data = jsarr.next_object();
        std::string ident = quality_data.get_string("id");
        int level = quality_data.get_int("level", 1);
        int amount = quality_data.get_int("amount", 1);
        rec->qualities.push_back(quality_requirement(ident, amount, level));
    }

    jsarr = jsobj.get_array("tools");
    load_obj_list(jsarr, rec->tools);

    jsarr = jsobj.get_array("book_learn");
    while (jsarr.has_more()) {
        JsonArray ja = jsarr.next_array();
        std::string book_name = ja.get_string(0);
        int book_level = ja.get_int(1);
        rec->booksets.push_back(std::pair<std::string,int>(book_name, book_level));
    }

    recipes[category].push_back(rec);
}

void reset_recipes()
{
    for (recipe_map::iterator it = recipes.begin(); it != recipes.end(); ++it) {
        for (size_t i = 0; i < it->second.size(); ++i) {
            delete it->second[i];
        }
    }
    recipes.clear();
}

void finalize_recipes()
{
    for (recipe_map::iterator it = recipes.begin(); it != recipes.end(); ++it) {
        for (size_t i = 0; i < it->second.size(); ++i) {
            recipe* r = it->second[i];
            for(size_t j = 0; j < r->booksets.size(); j++) {
                const std::string &book_id = r->booksets[j].first;
                const int skill_level = r->booksets[j].second;
                if (!item_controller->has_template(book_id)) {
                    debugmsg("book %s for recipe %s does not exist", book_id.c_str(), r->ident.c_str());
                    continue;
                }
                it_book *book_def = dynamic_cast<it_book *>(item_controller->find_template(book_id));
                if (book_def == NULL) {
                    debugmsg("book %s for recipe %s is not a book", book_id.c_str(), r->ident.c_str());
                    continue;
                }
                book_def->recipes[r] = skill_level;
            }
            r->booksets.clear();
        }
    }
}

void reset_recipes_qualities()
{
    qualities.clear();
}

void load_quality(JsonObject &jo)
{
    quality qual;
    qual.id = jo.get_string("id");
    qual.name = _(jo.get_string("name").c_str());
    qualities[qual.id] = qual;
}

bool game::crafting_allowed()
{
    if (u.morale_level() < MIN_MORALE_CRAFT) { // See morale.h
        add_msg(m_info, _("Your morale is too low to craft..."));
        return false;
    }
    return true;
}

bool game::crafting_can_see()
{
    if (u.fine_detail_vision_mod() > 4) {//minimum LL_LOW of LL_DARK + (ELFA_NV or atomic_light) (vs 2.5)
        add_msg(m_info, _("You can't see to craft!"));
        return false;

    }

    return true;
}

void game::recraft()
{
    if(u.lastrecipe == NULL) {
        popup(_("Craft something first"));
    } else if (making_would_work(u.lastrecipe)) {
        make_craft(u.lastrecipe);
    }
}

std::string print_missing_objs(const std::vector< std::vector <component> > &objs, bool is_tools) {
    std::ostringstream buffer;
    for(size_t i = 0; i < objs.size(); i++) {
        const std::vector<component> &list = objs[i];
        if (any_marked_available(list)) {
            continue;
        }
        if (!buffer.str().empty()) {
            buffer << _("\nand ");
        }
        for(size_t j = 0; j < list.size(); j++) {
            const component &comp = list[j];
            const itype *itt = item_controller->find_template(comp.type);
            itype *it = item_controller->find_template(comp.type);
            if (j > 0) {
                buffer << _(" or ");
            }
            if (!is_tools) {
                //~ <item-count> <item-name>
                buffer << string_format(_("%d %s"), abs(comp.count), it->nname(abs(comp.count)).c_str());
            } else if (comp.count > 0) {
                //~ <tool-name> (<numer-of-charges> charges)
                buffer << string_format(ngettext("%s (%d charge)", "%s (%d charges)", comp.count), itt->name.c_str(), comp.count);
            } else {
                buffer << it->nname(abs(comp.count));
            }
        }
    }
    return buffer.str();
}

std::string print_missing_objs(const std::vector< quality_requirement > &objs) {
    std::ostringstream buffer;
    for(size_t i = 0; i < objs.size(); i++) {
        const quality_requirement &req = objs[i];
        if (i > 0) {
            buffer << _("\nand ");
        }
        buffer << string_format(ngettext("%d tool with %s of %d or more.",
                                         "%d tools with %s of %d or more.",
                                         req.count),
                req.count, qualities[req.id].name.c_str(), req.level);
    }
    return buffer.str();
}

bool game::making_would_work(recipe *making)
{
    if (!crafting_allowed()) {
        return false;
    }

    if(!crafting_can_see()) {
        return false;
    }

    if(!can_make(making)) {
        std::ostringstream buffer;
        buffer << _("You can no longer make that craft!");
        const std::string missing_tools = print_missing_objs(making->tools, true);
        if (!missing_tools.empty()) {
            buffer << _("\nThese tools are missing:\n") << missing_tools;
        }
        const std::string missing_quali = print_missing_objs(making->qualities);
        if (!missing_quali.empty()) {
            if (missing_tools.empty()) {
                buffer << _("\nThese tools are missing:");
            }
            buffer << "\n" << missing_quali;
        }
        const std::string missing_comps = print_missing_objs(making->components, false);
        if (!missing_comps.empty()) {
            buffer << _("\nThose components are missing:\n") << missing_comps;
        }
        popup(buffer.str(), PF_NONE);
        return false;
    }

    if (!u.has_container_for(making->create_result())) {
        popup(_("You don't have anything to store that liquid in!"));
        return false;
    }

    return true;
}

bool game::can_make(recipe *r)
{
    inventory crafting_inv = crafting_inventory(&u);
    return can_make_with_inventory(r, crafting_inv);
}

bool game::can_make_with_inventory(recipe *r, const inventory &crafting_inv)
{
    bool retval = true;
    if (!u.knows_recipe(r)) {
        return false;
    }
    // under the assumption that all comp and tool's array contains
    // all the required stuffs at the start of the array

    // check all tool_quality requirements
    // this is an alternate method of checking for tools by using the tools qualities instead of the specific tool
    // You can specify the amount of tools with this quality required, but it does not work for consumed charges.
    std::vector<quality_requirement> &qualities = r->qualities;
    std::vector<quality_requirement>::iterator quality_iter = qualities.begin();
    while (quality_iter != qualities.end()) {
        std::string id = quality_iter->id;
        int amount = quality_iter->count;
        int level = quality_iter->level;
        if(crafting_inv.has_items_with_quality(id, level, amount)) {
            quality_iter->available = true;
        } else {
            quality_iter->available = false;
            retval = false;
        }
        ++quality_iter;
    }

    // check all tools
    std::vector<std::vector<component> > &tools = r->tools;
    std::vector<std::vector<component> >::iterator tool_set_it = tools.begin();
    while (tool_set_it != tools.end()) {
        std::vector<component> &set_of_tools = *tool_set_it;
        bool has_tool_in_set = false;
        std::vector<component>::iterator tool_it = set_of_tools.begin();
        while (tool_it != set_of_tools.end()) {
            component &tool = *tool_it;
            itype_id type = tool.type;
            int req = tool.count;
            if ( (req <= 0 && crafting_inv.has_amount(type, 1)) ||
                 (req <= 0 && ((type == ("goggles_welding")) && (u.has_bionic("bio_sunglasses") || u.is_wearing("rm13_armor_on")))) ||
                 (req > 0 && crafting_inv.has_charges(type, req))) {
                has_tool_in_set = true;
                tool.available = 1;
            } else {
                tool.available = -1;
            }
            ++tool_it;
        }
        if (!has_tool_in_set) {
            retval = false;
        }
        ++tool_set_it;
    }
    // check all components
    std::vector<std::vector<component> > &components = r->components;
    std::vector<std::vector<component> >::iterator comp_set_it = components.begin();
    while (comp_set_it != components.end()) {
        std::vector<component> &component_choices = *comp_set_it;
        bool has_comp_in_set = false;
        std::vector<component>::iterator comp_it = component_choices.begin();
        while (comp_it != component_choices.end()) {
            component &comp = *comp_it;
            itype_id type = comp.type;
            int req = comp.count;
            if (item_controller->find_template(type)->count_by_charges() && req > 0) {
                if (crafting_inv.has_charges(type, req)) {
                    has_comp_in_set = true;
                    comp.available = 1;
                } else {
                    comp.available = -1;
                }
            } else if (crafting_inv.has_components(type, abs(req))) {
                has_comp_in_set = true;
                comp.available = 1;
            } else {
                comp.available = -1;
            }
            ++comp_it;
        }
        if (!has_comp_in_set) {
            retval = false;
        }
        ++comp_set_it;
    }
    return check_enough_materials(r, crafting_inv) && retval;
}

bool game::check_enough_materials(recipe *r, const inventory &crafting_inv)
{
    bool retval = true;
    std::vector<std::vector<component> > &components = r->components;
    std::vector<std::vector<component> >::iterator comp_set_it = components.begin();
    while (comp_set_it != components.end()) {
        std::vector<component> &component_choices = *comp_set_it;
        std::vector<component>::iterator comp_it = component_choices.begin();
        bool atleast_one_available = false;
        while (comp_it != component_choices.end()) {
            component &comp = *comp_it;
            if (comp.available == 1) {
                bool have_enough_in_set = true;
                std::vector<std::vector<component> > &tools = r->tools;
                std::vector<std::vector<component> >::iterator tool_set_it = tools.begin();
                while (tool_set_it != tools.end()) {
                    bool have_enough = false;
                    bool found_same_type = false;
                    std::vector<component> &set_of_tools = *tool_set_it;
                    std::vector<component>::iterator tool_it = set_of_tools.begin();
                    while(tool_it != set_of_tools.end()) {
                        component &tool = *tool_it;
                        if (tool.available == 1) {
                            if (comp.type == tool.type) {
                                found_same_type = true;
                                bool count_by_charges = item_controller->find_template(comp.type)->count_by_charges();
                                if (count_by_charges) {
                                    int req = comp.count;
                                    if (tool.count > 0) {
                                        req += tool.count;
                                    } else  {
                                        ++req;
                                    }
                                    if (crafting_inv.has_charges(comp.type, req)) {
                                        have_enough = true;
                                    }
                                } else {
                                    int req = comp.count + 1;
                                    if (crafting_inv.has_components(comp.type, req)) {
                                        have_enough = true;
                                    }
                                }
                            } else {
                                have_enough = true;
                            }
                        }
                        ++tool_it;
                    }
                    if (found_same_type) {
                        have_enough_in_set &= have_enough;
                    }
                    ++tool_set_it;
                }
                if (!have_enough_in_set) {
                    // This component can't be used with any tools
                    // from one of the sets of tools, which means
                    // its availability should be set to 0 (in inventory,
                    // but not enough for both tool and components).
                    comp.available = 0;
                }
            }
            //Flag that at least one of the components in the set is available
            if (comp.available == 1) {
                atleast_one_available = true;
            }
            ++comp_it;
        }

        if (!atleast_one_available) {
            // this set doesn't have any components available,
            // so the recipe can't be crafted
            retval = false;
        }
        ++comp_set_it;
    }

    std::vector<std::vector<component> > &tools = r->tools;
    std::vector<std::vector<component> >::iterator tool_set_it = tools.begin();
    while (tool_set_it != tools.end()) {
        std::vector<component> &set_of_tools = *tool_set_it;
        std::vector<component>::iterator tool_it = set_of_tools.begin();
        bool atleast_one_available = false;
        while (tool_it != set_of_tools.end()) {
            component &tool = *tool_it;
            if (tool.available == 1) {
                bool have_enough_in_set = true;
                std::vector<std::vector<component> > &components = r->components;
                std::vector<std::vector<component> >::iterator comp_set_it = components.begin();
                while (comp_set_it != components.end()) {
                    bool have_enough = false, conflict = false;
                    std::vector<component> &component_choices = *comp_set_it;
                    std::vector<component>::iterator comp_it = component_choices.begin();
                    while(comp_it != component_choices.end()) {
                        component &comp = *comp_it;
                        if (tool.type == comp.type) {
                            if (tool.count > 0) {
                                int req = comp.count + tool.count;
                                if (!crafting_inv.has_charges(comp.type, req)) {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            } else {
                                int req = comp.count + 1;
                                if (!crafting_inv.has_components(comp.type, req)) {
                                    conflict = true;
                                    have_enough = have_enough || false;
                                }
                            }
                        } else if (comp.available == 1) {
                            have_enough = true;
                        }
                        ++comp_it;
                    }
                    if (conflict) {
                        have_enough_in_set = have_enough_in_set && have_enough;
                    }
                    ++comp_set_it;
                }
                if (!have_enough_in_set) {
                    // This component can't be used with any components
                    // from one of the sets of components, which means
                    // its availability should be set to 0 (in inventory,
                    // but not enough for both tool and components).
                    tool.available = 0;
                }
            }
            //Flag that at least one of the tools in the set is available
            if (tool.available == 1) {
                atleast_one_available = true;
            }
            ++tool_it;
        }

        if (!atleast_one_available) {
            // this set doesn't have any tools available,
            // so the recipe can't be crafted
            retval = false;
        }
        ++tool_set_it;
    }

    return retval;
}

void game::craft()
{
    if (!crafting_allowed()) {
        return;
    }

    recipe *rec = select_crafting_recipe();
    if (rec) {
        if(crafting_can_see()) {
            make_craft(rec);
        }
    }
}

void game::long_craft()
{
    if (!crafting_allowed()) {
        return;
    }

    recipe *rec = select_crafting_recipe();
    if (rec) {
        if(crafting_can_see()) {
            make_all_craft(rec);
        }
    }
}

static craft_cat first_craft_cat()
{
    return craft_cat_list.front();
}

static craft_cat next_craft_cat(const craft_cat cat)
{
    for (std::vector<craft_cat>::iterator iter = craft_cat_list.begin();
         iter != craft_cat_list.end(); ++iter) {
        if ((*iter) == cat) {
            if( ++iter == craft_cat_list.end() ) {
                return craft_cat_list.front();
            }
            return *iter;
        }
    }
    return NULL;
}

static craft_cat prev_craft_cat(const craft_cat cat)
{
    for (std::vector<craft_cat>::iterator iter = craft_cat_list.begin();
         iter != craft_cat_list.end(); ++iter) {
        if ((*iter) == cat) {
            if( iter == craft_cat_list.begin() ) {
                return craft_cat_list.back();
            }
            return *(--iter);
        }
    }
    return NULL;
}

static craft_subcat first_craft_subcat(const craft_cat cat)
{
    return craft_subcat_list[cat].front();
}

static craft_subcat last_craft_subcat(const craft_cat cat)
{
    return craft_subcat_list[cat].back();
}

static craft_subcat next_craft_subcat(const craft_cat cat, const craft_subcat subcat)
{
    for (std::vector<craft_subcat>::iterator iter = craft_subcat_list[cat].begin();
         iter != craft_subcat_list[cat].end(); ++iter) {
        if ((*iter) == subcat) {
            if( ++iter == craft_subcat_list[cat].end() ) {
                return craft_subcat_list[cat].front();
            }
            return *iter;
        }
    }
    return NULL;
}

static craft_subcat prev_craft_subcat(const craft_cat cat, const craft_subcat subcat)
{
    for (std::vector<craft_subcat>::iterator iter = craft_subcat_list[cat].begin();
         iter != craft_subcat_list[cat].end(); ++iter) {
        if ((*iter) == subcat) {
            if( iter == craft_subcat_list[cat].begin() ) {
                return craft_subcat_list[cat].back();
            }
            return *(--iter);
        }
    }
    return NULL;
}

// return whether any of the listed components have been flagged as available
bool any_marked_available(const std::vector<component> &comps)
{
    for (std::vector<component>::const_iterator it = comps.begin();
         it != comps.end(); ++it) {
        if (it->available == 1) {
            return true;
        }
    }
    return false;
}

recipe *game::select_crafting_recipe()
{
    const int headHeight = 3;
    const int subHeadHeight = 2;
    const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
    bool isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );

    const int width = isWide ? ( freeWidth > FULL_SCREEN_WIDTH ? FULL_SCREEN_WIDTH * 2 : TERMX ) :
                          FULL_SCREEN_WIDTH;
    const int wStart = ( TERMX - width ) / 2;
    const int tailHeight = isWide ? 3 : 4;
    const int dataLines = TERMY - (headHeight + subHeadHeight) - tailHeight;
    const int dataHalfLines = dataLines / 2;
    const int dataHeight = TERMY - (headHeight + subHeadHeight);

    int lastid = -1;

    WINDOW *w_head = newwin(headHeight, width, 0, wStart);
    WINDOW *w_subhead = newwin(subHeadHeight, width, 3, wStart);
    WINDOW *w_data = newwin(dataHeight, width, headHeight + subHeadHeight, wStart);

    const int iInfoWidth = width - FULL_SCREEN_WIDTH - 3;
    std::vector<std::string> folded;
    craft_cat tab = first_craft_cat();
    craft_subcat subtab = first_craft_subcat( tab );
    std::vector<recipe *> current;
    std::vector<bool> available;
    item tmp;
    int line = 0, xpos, ypos;
    bool redraw = true;
    bool keepline = false;
    bool done = false;
    int display_mode = 0;
    recipe *chosen = NULL;
    input_context ctxt("CRAFTING");
    ctxt.register_cardinal();
    ctxt.register_action("QUIT");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("CYCLE_MODE");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("FILTER");
    ctxt.register_action("RESET_FILTER");
    ctxt.register_action("HELP_RECIPE");
    ctxt.register_action("HELP_KEYBINDINGS");

    inventory crafting_inv = crafting_inventory(&u);
    std::string filterstring = "";
    do {
        if (redraw) {
            // When we switch tabs, redraw the header
            redraw = false;
            if ( ! keepline ) {
                line = 0;
            } else {
                keepline = false;
            }

            draw_recipe_tabs(w_head, tab, (filterstring == "") ? false : true);
            draw_recipe_subtabs(w_subhead, tab, subtab, (filterstring == "") ? false : true);
            current.clear();
            available.clear();
            // Set current to all recipes in the current tab; available are possible to make
            pick_recipes(crafting_inv, current, available, tab, subtab, filterstring);
        }

        // Clear the screen of recipe data, and draw it anew
        werase(w_data);

        if ( isWide ) {
            mvwprintz(w_data, dataLines + 1, 5, c_white, _("Press <ENTER> to attempt to craft object."));
            wprintz(w_data, c_white, "  ");
            if (filterstring != "") {
                wprintz(w_data, c_white, _("[E]: Describe, [F]ind, [R]eset, [m]ode [?] keybindings"));
            } else {
                wprintz(w_data, c_white, _("[E]: Describe, [F]ind, [m]ode [?] keybindings"));
            }
        } else {
            if (filterstring != "") {
                mvwprintz(w_data, dataLines + 1, 5, c_white, _("[E]: Describe, [F]ind, [R]eset, [m]ode [?] keybindings"));
            } else {
                mvwprintz(w_data, dataLines + 1, 5, c_white, _("[E]: Describe, [F]ind, [m]ode [?] keybindings"));
            }
            mvwprintz(w_data, dataLines + 2, 5, c_white, _("Press <ENTER> to attempt to craft object."));
        }
        // Draw borders
        for (int i = 1; i < width - 1; ++i) { // _
            mvwputch(w_data, dataHeight - 1, i, BORDER_COLOR, LINE_OXOX);
        }
        for (int i = 0; i < dataHeight - 1; ++i) { // |
            mvwputch(w_data, i, 0, BORDER_COLOR, LINE_XOXO);
            mvwputch(w_data, i, width - 1, BORDER_COLOR, LINE_XOXO);
        }
        mvwputch(w_data, dataHeight - 1,  0, BORDER_COLOR, LINE_XXOO); // _|
        mvwputch(w_data, dataHeight - 1, width - 1, BORDER_COLOR, LINE_XOOX); // |_

        int recmin = 0, recmax = current.size();
        if (recmax > dataLines) {
            if (line <= recmin + dataHalfLines) {
                for (int i = recmin; i < recmin + dataLines; ++i) {
                    std::string tmp_name = item_controller->find_template(current[i]->result)->nname(1);
                    mvwprintz(w_data, i - recmin, 2, c_dkgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? h_white : h_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    } else {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? c_white : c_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    }
                }
            } else if (line >= recmax - dataHalfLines) {
                for (int i = recmax - dataLines; i < recmax; ++i) {
                    std::string tmp_name = item_controller->find_template(current[i]->result)->nname(1);
                    mvwprintz(w_data, dataLines + i - recmax, 2, c_ltgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, dataLines + i - recmax, 2, (available[i] ? h_white : h_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    } else {
                        mvwprintz(w_data, dataLines + i - recmax, 2, (available[i] ? c_white : c_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    }
                }
            } else {
                for (int i = line - dataHalfLines; i < line - dataHalfLines + dataLines; ++i) {
                    std::string tmp_name = item_controller->find_template(current[i]->result)->nname(1);
                    mvwprintz(w_data, dataHalfLines + i - line, 2, c_ltgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, dataHalfLines + i - line, 2, (available[i] ? h_white : h_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    } else {
                        mvwprintz(w_data, dataHalfLines + i - line, 2, (available[i] ? c_white : c_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    }
                }
            }
        } else {
            for (size_t i = 0; i < current.size() && i < (size_t)dataHeight + 1; ++i) {
                std::string tmp_name = item_controller->find_template(current[i]->result)->nname(1);
                if( (int)i == line ) {
                    mvwprintz(w_data, i, 2, (available[i] ? h_white : h_dkgray),
                              utf8_truncate(tmp_name, 28).c_str());
                } else {
                    mvwprintz(w_data, i, 2, (available[i] ? c_white : c_dkgray),
                              utf8_truncate(tmp_name, 28).c_str());
                }
            }
        }
        if (!current.empty()) {
            nc_color col = (available[line] ? c_white : c_ltgray);
            ypos = 0;
            if(display_mode == 0) {
                mvwprintz(w_data, ypos++, 30, col, _("Skills used: %s"),
                          (current[line]->skill_used == NULL ? _("N/A") :
                           current[line]->skill_used->name().c_str()));

                mvwprintz(w_data, ypos++, 30, col, _("Required skills: %s"),
                          (current[line]->required_skills_string().c_str()));
                mvwprintz(w_data, ypos++, 30, col, _("Difficulty: %d"), current[line]->difficulty);
                if (current[line]->skill_used == NULL) {
                    mvwprintz(w_data, ypos++, 30, col, _("Your skill level: N/A"));
                } else {
                    mvwprintz(w_data, ypos++, 30, col, _("Your skill level: %d"),
                              // Macs don't seem to like passing this as a class, so force it to int
                              (int)u.skillLevel(current[line]->skill_used));
                }
                if (current[line]->time >= 1000) {
                    mvwprintz(w_data, ypos++, 30, col, ngettext("Time to complete: %d minute","Time to complete: %d minutes",int(current[line]->time / 1000)),
                              int(current[line]->time / 1000));
                } else {
                    mvwprintz(w_data, ypos++, 30, col, ngettext("Time to complete: %d turn","Time to complete: %d turns",int(current[line]->time / 100)),
                              int(current[line]->time / 100));
                }
            }
            if(display_mode == 0 || display_mode == 1) {
                mvwprintz(w_data, ypos++, 30, col, _("Tools required:"));
                if (current[line]->tools.empty() && current[line]->qualities.empty()) {
                    mvwputch(w_data, ypos, 30, col, '>');
                    mvwprintz(w_data, ypos, 32, c_green, _("NONE"));
                } else {
                    // Loop to print the required tool qualities
                    for(std::vector<quality_requirement>::const_iterator iter = current[line]->qualities.begin();
                        iter != current[line]->qualities.end(); ++iter) {
                        xpos = 32;
                        mvwputch(w_data, ypos, 30, col, '>');
                        nc_color toolcol = c_red;
                        if(iter->available) {
                            toolcol = c_green;
                        }

                        std::stringstream qualinfo;
                        qualinfo << string_format(ngettext("Requires %d tool with %s quality of %d or more.",
                                                           "Requires %d tools with %s quality of %d or more.",
                                                           iter->count),
                                                  iter->count, qualities[iter->id].name.c_str(),
                                                  iter->level);
                        ypos += fold_and_print(w_data, ypos, xpos, FULL_SCREEN_WIDTH - xpos - 1,
                                               toolcol, qualinfo.str());
                    }
                    ypos--;
                    // Loop to print the required tools
                    for (size_t i = 0; i < current[line]->tools.size(); i++) {
                        ypos++;
                        xpos = 32;
                        mvwputch(w_data, ypos, 30, col, '>');
                        bool has_one = any_marked_available(current[line]->tools[i]);
                        for (size_t j = 0; j < current[line]->tools[i].size(); j++) {
                            itype_id type = current[line]->tools[i][j].type;
                            long charges = current[line]->tools[i][j].count;
                            nc_color toolcol = has_one ? c_dkgray : c_red;

                            if (current[line]->tools[i][j].available == 0) {
                                toolcol = c_brown;
                            } else if (charges < 0 && crafting_inv.has_tools(type, 1)) {
                                toolcol = c_green;
                            } else if (charges > 0 && crafting_inv.has_charges(type, charges)) {
                                toolcol = c_green;
                            } else if ((type == "goggles_welding") && (u.has_bionic("bio_sunglasses") || u.is_wearing("rm13_armor_on"))) {
                                toolcol = c_cyan;
                            }

                            std::stringstream toolinfo;
                            toolinfo << item_controller->find_template(type)->nname(1) << " ";

                            if (charges > 0) {
                                toolinfo << string_format(ngettext("(%d charge) ","(%d charges) ",charges), charges);
                            }

                            std::string toolname = toolinfo.str();
                            if (xpos != 32 && xpos + utf8_width(toolname.c_str()) >= FULL_SCREEN_WIDTH) {
                                xpos = 32;
                                ypos++;
                            }
                            mvwprintz(w_data, ypos, xpos, toolcol, toolname.c_str());
                            xpos += utf8_width(toolname.c_str());
                            if (j < current[line]->tools[i].size() - 1) {
                                if (xpos >= FULL_SCREEN_WIDTH - 3) {
                                    xpos = 32;
                                    ypos++;
                                }
                                mvwprintz(w_data, ypos, xpos, c_white, _("%s "), _("OR"));
                                xpos += utf8_width(_("OR")) + 1;
                            }
                        }
                    }
                }
                ypos++;
            }
            // Loop to print the required components
            mvwprintz(w_data, ypos, 30, col, _("Components required:"));
            for (unsigned i = 0; i < current[line]->components.size(); i++) {
                ypos++;
                mvwputch(w_data, ypos, 30, col, '>');
                xpos = 32;
                bool has_one = any_marked_available(current[line]->components[i]);
                for (unsigned j = 0; j < current[line]->components[i].size(); j++) {
                    int count = current[line]->components[i][j].count;
                    itype_id type = current[line]->components[i][j].type;
                    nc_color compcol = has_one ? c_dkgray : c_red;
                    if (current[line]->components[i][j].available == 0) {
                        compcol = c_brown;
                    } else if (item_controller->find_template(type)->count_by_charges() && count > 0) {
                        if (crafting_inv.has_charges(type, count)) {
                            compcol = c_green;
                        }
                    } else if (crafting_inv.has_components(type, abs(count))) {
                        compcol = c_green;
                    }
                    std::stringstream dump;
                    dump << abs(count) << " " << item_controller->find_template(type)->nname(abs(count)) << " ";
                    std::string compname = dump.str();
                    if (xpos + utf8_width(compname.c_str()) >= FULL_SCREEN_WIDTH) {
                        ypos++;
                        xpos = 32;
                    }
                    mvwprintz(w_data, ypos, xpos, compcol, compname.c_str());
                    xpos += utf8_width(compname.c_str());
                    if (j < current[line]->components[i].size() - 1) {
                        if (xpos >= FULL_SCREEN_WIDTH - 3) {
                            ypos++;
                            xpos = 32;
                        }
                        mvwprintz(w_data, ypos, xpos, c_white, _("%s "), _("OR"));
                        xpos += utf8_width(_("OR")) + 1;
                    }
                }
            }

            if ( isWide ) {
                if ( lastid != current[line]->id ) {
                    lastid = current[line]->id;
                    tmp = current[line]->create_result();
                    folded = foldstring(tmp.info(true), iInfoWidth);
                }
                int maxline = (int)folded.size() > dataHeight ? dataHeight : (int)folded.size();

                mvwprintz(w_data, 0, FULL_SCREEN_WIDTH + 1, col, "%s",
                          utf8_truncate(tmp.type->nname(1), iInfoWidth).c_str());

                for(int i = 1; i < maxline; i++) {
                    mvwprintz(w_data, i, FULL_SCREEN_WIDTH + 1, col, "%s", folded[i].c_str() );
                }

            }

        }

        //Draw Scrollbar
        draw_scrollbar(w_data, line, dataLines, recmax, 0);

        wrefresh(w_data);
        const std::string action = ctxt.handle_input();
        if (action == "CYCLE_MODE") {
            display_mode = display_mode + 1;
            if(display_mode >= 3 || display_mode <= 0) {
                display_mode = 0;
            }
        } else if (action == "LEFT") {
            subtab = prev_craft_subcat( tab, subtab );
            redraw = true;
        } else if (action == "PREV_TAB") {
            tab = prev_craft_cat(tab);
            subtab = first_craft_subcat( tab );//default ALL
            redraw = true;
        } else if (action == "RIGHT") {
            subtab = next_craft_subcat( tab, subtab );
            redraw = true;
        } else if (action == "NEXT_TAB") {
            tab = next_craft_cat(tab);
            subtab = first_craft_subcat( tab );//default ALL
            redraw = true;
        } else if (action == "DOWN") {
            line++;
        } else if (action == "UP") {
            line--;
        } else if (action == "CONFIRM") {
            if (available.empty() || !available[line]) {
                popup(_("You can't do that!"));
            } else if (!u.has_container_for(current[line]->create_result())) {
                popup(_("You don't have anything to store that liquid in!"));
            } else {
                chosen = current[line];
                done = true;
            }
        } else if (action == "HELP_RECIPE") {
            tmp = current[line]->create_result();

            full_screen_popup("%s\n%s", tmp.type->nname(1).c_str(),  tmp.info(true).c_str());
            redraw = true;
            keepline = true;
        } else if (action == "FILTER") {
            filterstring = string_input_popup(_("Search:"), 85, filterstring, _("Search tools or component using prefix t and c. \n(i.e. \"t:hammer\" or \"c:two by four\".)"));
            redraw = true;
        } else if (action == "QUIT") {
            done = true;
        } else if (action == "RESET_FILTER") {
            filterstring = "";
            redraw = true;
        }
        if (line < 0) {
            line = current.size() - 1;
        } else if (line >= (int)current.size()) {
            line = 0;
        }
    } while (!done);

    werase(w_head);
    werase(w_subhead);
    werase(w_data);
    delwin(w_head);
    delwin(w_subhead);
    delwin(w_data);
    refresh_all();

    return chosen;
}

static void draw_recipe_tabs(WINDOW *w, craft_cat tab, bool filtered)
{
    werase(w);
    int width = getmaxx(w);
    for (int i = 0; i < width; i++) {
        mvwputch(w, 2, i, BORDER_COLOR, LINE_OXOX);
    }

    mvwputch(w, 2,  0, BORDER_COLOR, LINE_OXXO); // |^
    mvwputch(w, 2, width - 1, BORDER_COLOR, LINE_OOXX); // ^|
    mvwprintz(w, 0, width - utf8_width(_("Lighting:")), c_ltgray, _("Lighting:"));//Lighting info
    int light = g->u.fine_detail_vision_mod();
    const char *str;
    nc_color color;
    if (light <= 1) {
        str = _("brightly");
        color = c_yellow;
    } else if (light <= 2) {
        str = _("cloudy");
        color = c_white;
    } else if (light <= 3) {
        str = _("shady");
        color = c_ltgray;
    } else if (light <= 4) {
        str = _("dark");
        color = c_dkgray;
    } else {
        str = _("very dark");
        color = c_black_white;
    }
    mvwprintz(w, 1, width - 1 - utf8_width(str), color, str);
    if(!filtered) {
        int pos_x = 2;//draw the tabs on each other
        int tab_step = 3;//step between tabs, two for tabs border
        draw_tab(w,  pos_x, _("WEAPONS"),     (tab == "CC_WEAPON")     ? true : false);
        pos_x += utf8_width(_("WEAPONS")) + tab_step;
        draw_tab(w, pos_x,  _("AMMO"),        (tab == "CC_AMMO")       ? true : false);
        pos_x += utf8_width(_("AMMO")) + tab_step;
        draw_tab(w, pos_x,  _("FOOD"),        (tab == "CC_FOOD")       ? true : false);
        pos_x += utf8_width(_("FOOD")) + tab_step;
        draw_tab(w, pos_x,  _("CHEMS"),       (tab == "CC_CHEM")       ? true : false);
        pos_x += utf8_width(_("CHEMS")) + tab_step;
        draw_tab(w, pos_x,  _("ELECTRICAL"), (tab == "CC_ELECTRONIC") ? true : false);
        pos_x += utf8_width(_("ELECTRICAL")) + tab_step;
        draw_tab(w, pos_x,  _("ARMOR"),       (tab == "CC_ARMOR")      ? true : false);
        pos_x += utf8_width(_("ARMOR")) + tab_step;
        draw_tab(w, pos_x,  _("OTHER"),       (tab == "CC_OTHER")      ? true : false);
    } else {
        draw_tab(w, 2, _("Searched"), true);
    }

    wrefresh(w);
}

static void draw_recipe_subtabs(WINDOW *w, craft_cat tab, craft_subcat subtab, bool filtered)
{
    werase(w);
    int width = getmaxx(w);
    for (int i = 0; i < width; i++) {
        if (i == 0) {
            mvwputch(w, 2, i, BORDER_COLOR, LINE_XXXO);
        } else if (i == width) {
            mvwputch(w, 2, i, BORDER_COLOR, LINE_XOXX);
        } else {
            mvwputch(w, 2, i, BORDER_COLOR, LINE_OXOX);
        }
    }

    for (int i = 0; i < 3; i++) {
        mvwputch(w, i,  0, BORDER_COLOR, LINE_XOXO); // |^
        mvwputch(w, i, width - 1, BORDER_COLOR,  LINE_XOXO); // ^|
    }

    if(!filtered) {
        int pos_x = 2;//draw the tabs on each other
        int tab_step = 3;//step between tabs, two for tabs border
        draw_subtab(w, pos_x, _("ALL"),
                    (subtab == "CSC_ALL") ? true : false);//Add ALL subcategory to all tabs;
        pos_x += utf8_width(_("ALL")) + tab_step;
        if (tab == "CC_WEAPON") {
            draw_subtab(w, pos_x, _("BASHING"), (subtab == "CSC_WEAPON_BASHING") ? true : false);
            pos_x += utf8_width(_("BASHING")) + tab_step;
            draw_subtab(w, pos_x, _("CUTTING"),    (subtab == "CSC_WEAPON_CUTTING")   ? true : false);
            pos_x += utf8_width(_("CUTTING")) + tab_step;
            draw_subtab(w, pos_x, _("PIERCING"),    (subtab == "CSC_WEAPON_PIERCING")   ? true : false);
            pos_x += utf8_width(_("PIERCING")) + tab_step;
            draw_subtab(w, pos_x, _("RANGED"),  (subtab == "CSC_WEAPON_RANGED")  ? true : false);
            pos_x += utf8_width(_("RANGED")) + tab_step;
            draw_subtab(w, pos_x, _("EXPLOSIVE"),  (subtab == "CSC_WEAPON_EXPLOSIVE")  ? true : false);
            pos_x += utf8_width(_("EXPLOSIVE")) + tab_step;
            draw_subtab(w, pos_x, _("MODS"),  (subtab == "CSC_WEAPON_MODS")  ? true : false);
            pos_x += utf8_width(_("MODS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"),   (subtab == "CSC_WEAPON_OTHER")   ? true : false);
        } else if (tab == "CC_AMMO") {
            draw_subtab(w, pos_x, _("BULLETS"), (subtab == "CSC_AMMO_BULLETS") ? true : false);
            pos_x += utf8_width(_("BULLETS")) + tab_step;
            draw_subtab(w, pos_x, _("ARROWS"), (subtab == "CSC_AMMO_ARROWS") ? true : false);
            pos_x += utf8_width(_("ARROWS")) + tab_step;
            draw_subtab(w, pos_x, _("COMPONENTS"), (subtab == "CSC_AMMO_COMPONENTS") ? true : false);
            pos_x += utf8_width(_("COMPONENTS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), (subtab == "CSC_AMMO_OTHER") ? true : false);
        } else if (tab == "CC_FOOD") {
            draw_subtab(w, pos_x, _("DRINKS"), (subtab == "CSC_FOOD_DRINKS") ? true : false);
            pos_x += utf8_width(_("DRINKS")) + tab_step;
            draw_subtab(w, pos_x, _("MEAT"), (subtab == "CSC_FOOD_MEAT") ? true : false);
            pos_x += utf8_width(_("MEAT")) + tab_step;
            draw_subtab(w, pos_x, _("VEGGI"), (subtab == "CSC_FOOD_VEGGI") ? true : false);
            pos_x += utf8_width(_("VEGGI")) + tab_step;
            draw_subtab(w, pos_x, _("SNACK"), (subtab == "CSC_FOOD_SNACK") ? true : false);
            pos_x += utf8_width(_("SNACK")) + tab_step;
            draw_subtab(w, pos_x, _("BREAD"), (subtab == "CSC_FOOD_BREAD") ? true : false);
            pos_x += utf8_width(_("BREAD")) + tab_step;
            draw_subtab(w, pos_x, _("PASTA"), (subtab == "CSC_FOOD_PASTA") ? true : false);
            pos_x += utf8_width(_("PASTA")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), (subtab == "CSC_FOOD_OTHER") ? true : false);
        } else if (tab == "CC_CHEM") {
            draw_subtab(w, pos_x, _("DRUGS"), (subtab == "CSC_CHEM_DRUGS") ? true : false);
            pos_x += utf8_width(_("DRUGS")) + tab_step;
            draw_subtab(w, pos_x, _("MUTAGEN"), (subtab == "CSC_CHEM_MUTAGEN") ? true : false);
            pos_x += utf8_width(_("MUTAGEN")) + tab_step;
            draw_subtab(w, pos_x, _("CHEMICALS"), (subtab == "CSC_CHEM_CHEMICALS") ? true : false);
            pos_x += utf8_width(_("CHEMICALS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), (subtab == "CSC_CHEM_OTHER") ? true : false);
        } else if (tab == "CC_ELECTRONIC") {
            draw_subtab(w, pos_x, _("CBMS"), (subtab == "CSC_ELECTRONIC_CBMS") ? true : false);
            pos_x += utf8_width(_("CBMS")) + tab_step;
            draw_subtab(w, pos_x, _("TOOLS"), (subtab == "CSC_ELECTRONIC_TOOLS") ? true : false);
            pos_x += utf8_width(_("TOOLS")) + tab_step;
            draw_subtab(w, pos_x, _("PARTS"), (subtab == "CSC_ELECTRONIC_PARTS") ? true : false);
            pos_x += utf8_width(_("PARTS")) + tab_step;
            draw_subtab(w, pos_x, _("LIGHTING"), (subtab == "CSC_ELECTRONIC_LIGHTING") ? true : false);
            pos_x += utf8_width(_("LIGHTING")) + tab_step;
            draw_subtab(w, pos_x, _("COMPONENTS"), (subtab == "CSC_ELECTRONIC_COMPONENTS") ? true : false);
            pos_x += utf8_width(_("COMPONENTS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), (subtab == "CSC_ELECTRONIC_OTHER") ? true : false);
        } else if (tab == "CC_ARMOR") {
            draw_subtab(w, pos_x, _("STORAGE"), (subtab == "CSC_ARMOR_STORAGE") ? true : false);
            pos_x += utf8_width(_("STORAGE")) + tab_step;
            draw_subtab(w, pos_x, _("SUIT"), (subtab == "CSC_ARMOR_SUIT") ? true : false);
            pos_x += utf8_width(_("SUIT")) + tab_step;
            draw_subtab(w, pos_x, _("HEAD"), (subtab == "CSC_ARMOR_HEAD") ? true : false);
            pos_x += utf8_width(_("HEAD")) + tab_step;
            draw_subtab(w, pos_x, _("TORSO"), (subtab == "CSC_ARMOR_TORSO") ? true : false);
            pos_x += utf8_width(_("TORSO")) + tab_step;
            draw_subtab(w, pos_x, _("ARMS"), (subtab == "CSC_ARMOR_ARMS") ? true : false);
            pos_x += utf8_width(_("ARMS")) + tab_step;
            draw_subtab(w, pos_x, _("HANDS"), (subtab == "CSC_ARMOR_HANDS") ? true : false);
            pos_x += utf8_width(_("HANDS")) + tab_step;
            draw_subtab(w, pos_x, _("LEGS"), (subtab == "CSC_ARMOR_LEGS") ? true : false);
            pos_x += utf8_width(_("LEGS")) + tab_step;
            draw_subtab(w, pos_x, _("FEET"), (subtab == "CSC_ARMOR_FEET") ? true : false);
            pos_x += utf8_width(_("FEET")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), (subtab == "CSC_ARMOR_OTHER") ? true : false);
        } else if (tab == "CC_OTHER") {
            draw_subtab(w, pos_x, _("TOOLS"), (subtab == "CSC_OTHER_TOOLS") ? true : false);
            pos_x += utf8_width(_("TOOLS")) + tab_step;
            draw_subtab(w, pos_x, _("MEDICAL"), (subtab == "CSC_OTHER_MEDICAL") ? true : false);
            pos_x += utf8_width(_("MEDICAL")) + tab_step;
            draw_subtab(w, pos_x, _("CONTAINERS"), (subtab == "CSC_OTHER_CONTAINERS") ? true : false);
            pos_x += utf8_width(_("CONTAINERS")) + tab_step;
            draw_subtab(w, pos_x, _("MATERIALS"), (subtab == "CSC_OTHER_MATERIALS") ? true : false);
            pos_x += utf8_width(_("MATERIALS")) + tab_step;
            draw_subtab(w, pos_x, _("PARTS"), (subtab == "CSC_OTHER_PARTS") ? true : false);
            pos_x += utf8_width(_("PARTS")) + tab_step;
            draw_subtab(w, pos_x, _("TRAPS"), (subtab == "CSC_OTHER_TRAPS") ? true : false);
            pos_x += utf8_width(_("TRAPS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), (subtab == "CSC_OTHER_OTHER") ? true : false);
        }
    } else {
        werase(w);

        for (int i = 0; i < 3; i++) {
            mvwputch(w, i,  0, BORDER_COLOR, LINE_XOXO); // |^
            mvwputch(w, i, width - 1, BORDER_COLOR,  LINE_XOXO); // ^|
        }
    }

    wrefresh(w);
}

inventory game::crafting_inventory(player *p)
{
    inventory crafting_inv;
    crafting_inv.form_from_map(point(p->posx, p->posy), PICKUP_RANGE, false);
    crafting_inv += p->inv;
    crafting_inv += p->weapon;
    for (std::vector<item>::const_iterator a = p->worn.begin(); a != p->worn.end(); a++) {
        // Add contents of worn items, but not the worn item itself!
        crafting_inv += a->contents;
    }
    if (p->has_bionic("bio_tools")) {
        item tools("toolset", calendar::turn);
        tools.charges = p->power_level;
        crafting_inv += tools;
    }
    return crafting_inv;
}

void game::pick_recipes(const inventory &crafting_inv, std::vector<recipe *> &current,
                        std::vector<bool> &available, craft_cat tab,
                        craft_subcat subtab, std::string filter)
{
    bool search_name = true;
    bool search_tool = false;
    bool search_component = false;
    size_t pos = filter.find(":");
    if(pos != std::string::npos)
    {
        search_name = false;
        std::string searchType = filter.substr(0, pos);
        for( size_t i = 0; i < searchType.size(); ++i )
        {
            if(searchType[i] == 'n')
            {
                search_name = true;
            }
            else if(searchType[i] == 't')
            {
                search_tool = true;
            }
            else if(searchType[i] == 'c')
            {
                search_component = true;
            }
        }
        filter = filter.substr(pos + 1);
    }
    recipe_list available_recipes;

    if (filter == "") {
        available_recipes = recipes[tab];
    } else {

        for (recipe_map::iterator iter = recipes.begin(); iter != recipes.end(); ++iter) {
            available_recipes.insert(available_recipes.begin(),
                                     iter->second.begin(), iter->second.end());
        }
    }

    current.clear();
    available.clear();

    for (recipe_list::iterator iter = available_recipes.begin();
         iter != available_recipes.end(); ++iter) {
        if (subtab == "CSC_ALL" || (*iter)->subcat == subtab || filter != "") {
            if (!u.knows_recipe(*iter)) {
                continue;
            }

            if ((*iter)->difficulty < 0 ) {
                continue;
            }
            if(filter != "")
            {
                if(search_name)
                {
                    if(item_controller->find_template((*iter)->result)->name.find(filter) == std::string::npos)
                    {
                        continue;
                    }
                }
                if(search_tool)
                {
                    bool found = false;
                    for(std::vector<std::vector<component> >::iterator it = (*iter)->tools.begin() ; it != (*iter)->tools.end() ; ++it)
                    {
                        for(std::vector<component>::iterator it2 = (*it).begin() ; it2 != (*it).end() ; ++it2)
                        {
                            if(item_controller->find_template((*it2).type)->name.find(filter) != std::string::npos)
                            {
                                found = true;
                                break;
                            }
                        }
                        if(found)
                        {
                            break;
                        }
                    }
                    if(!found)
                    {
                        continue;
                    }
                }
                if(search_component)
                {
                    bool found = false;
                    for(std::vector<std::vector<component> >::iterator it = (*iter)->components.begin() ; it != (*iter)->components.end() ; ++it)
                    {
                        for(std::vector<component>::iterator it2 = (*it).begin() ; it2 != (*it).end() ; ++it2)
                        {
                            if(item_controller->find_template((*it2).type)->name.find(filter) != std::string::npos)
                            {
                                found = true;
                                break;
                            }
                        }
                        if(found)
                        {
                            break;
                        }
                    }
                    if(!found)
                    {
                        continue;
                    }
                }
            }
            if (can_make_with_inventory(*iter, crafting_inv)) {
                current.insert(current.begin(), *iter);
                available.insert(available.begin(), true);
            } else {
                current.push_back(*iter);
                available.push_back(false);
            }
        }
    }
}

void game::make_craft(recipe *making)
{
    u.assign_activity(ACT_CRAFT, making->time, making->id);
    u.lastrecipe = making;
}


void game::make_all_craft(recipe *making)
{
    u.assign_activity(ACT_LONGCRAFT, making->time, making->id);
    u.lastrecipe = making;
}

item recipe::create_result() const
{
    item newit(result, calendar::turn, false);
    if (result_mult != 1) {
        newit.charges *= result_mult;
    }
    if (!newit.craft_has_charges()) {
        newit.charges = 0;
    }
    return newit;
}

void game::complete_craft()
{
    recipe *making = recipe_by_index(u.activity.index); // Which recipe is it?

    // # of dice is 75% primary skill, 25% secondary (unless secondary is null)
    int skill_dice = u.skillLevel(making->skill_used) * 4;

    // farsightedness can impose a penalty on electronics and tailoring success
    // it's equivalent to a 2-rank electronics penalty, 1-rank tailoring
    if (u.has_trait("HYPEROPIC") && !u.is_wearing("glasses_reading")
        && !u.is_wearing("glasses_bifocal") && !u.has_disease("contacts")) {
        int main_rank_penalty = 0;
        if (making->skill_used == Skill::skill("electronics")) {
            main_rank_penalty = 2;
        } else if (making->skill_used == Skill::skill("tailor")) {
            main_rank_penalty = 1;
        }
        skill_dice -= main_rank_penalty * 4;
    }

    // It's tough to craft with paws.  Fortunately it's just a matter of grip and fine-motor,
    // not inability to see what you're doing
    if (u.has_trait("PAWS") || u.has_trait("PAWS_LARGE")) {
        int paws_rank_penalty = 0;
        if (u.has_trait("PAWS_LARGE")) {
            paws_rank_penalty += 1;
        }
        if (making->skill_used == Skill::skill("electronics")) {
            paws_rank_penalty += 1;
        } else if (making->skill_used == Skill::skill("tailor")) {
            paws_rank_penalty += 1;
        } else if (making->skill_used == Skill::skill("mechanics")) {
            paws_rank_penalty += 1;
        }
        skill_dice -= paws_rank_penalty * 4;
    }

    // Sides on dice is 16 plus your current intelligence
    int skill_sides = 16 + u.int_cur;

    int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
    int diff_sides = 24; // 16 + 8 (default intelligence)

    int skill_roll = dice(skill_dice, skill_sides);
    int diff_roll  = dice(diff_dice,  diff_sides);

    if (making->skill_used) {
        u.practice( making->skill_used, making->difficulty * 5 + 20,
                    (int)making->difficulty * 1.25 );
    }

    // Messed up badly; waste some components.
    if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
        add_msg(m_bad, _("You fail to make the %s, and waste some materials."),
                item_controller->find_template(making->result)->name.c_str());
        for (unsigned i = 0; i < making->components.size(); i++) {
            consume_items(&u, making->components[i]);
        }

        for (unsigned i = 0; i < making->tools.size(); i++) {
            consume_tools(&u, making->tools[i], false);
        }
        u.activity.type = ACT_NULL;
        return;
        // Messed up slightly; no components wasted.
    } else if (diff_roll > skill_roll) {
        add_msg(m_neutral, _("You fail to make the %s, but don't waste any materials."),
                item_controller->find_template(making->result)->name.c_str());
        //this method would only have been called from a place that nulls u.activity.type,
        //so it appears that it's safe to NOT null that variable here.
        //rationale: this allows certain contexts (e.g. ACT_LONGCRAFT) to distinguish major and minor failures
        return;
    }
    // If we're here, the craft was a success!
    // Use up the components and tools
    std::list<item> used;
    for (unsigned i = 0; i < making->components.size(); i++) {
        std::list<item> tmp = consume_items(&u, making->components[i]);
        used.splice(used.end(), tmp);
    }
    for (unsigned i = 0; i < making->tools.size(); i++) {
        consume_tools(&u, making->tools[i], false);
    }

    // Set up the new item, and assign an inventory letter if available
    item newit = making->create_result();
    if (!newit.count_by_charges() && making->reversible) {
        // Setting this for items counted by charges gives only problems:
        // those items are automatically merged everywhere (map/vehicle/inventory),
        // which would either loose this information or merge it somehow.
        newit.components.insert(newit.components.begin(), used.begin(), used.end());
    }

    if (newit.is_armor() && newit.has_flag("VARSIZE")) {
        newit.item_tags.insert("FIT");
    }
    float used_age_tally = 0;
    int used_age_count = 0;
    for (std::list<item>::iterator iter = used.begin(); iter != used.end(); ++iter) {
        if (iter->goes_bad()) {
            iter->rotten();
            used_age_tally += iter->rot /
                              (float)(dynamic_cast<it_comest *>(iter->type)->spoils);
            ++used_age_count;
        }
    }
    if (used_age_count > 0 && newit.goes_bad()) {
        const int average_used_age = int((used_age_tally / used_age_count) * dynamic_cast<it_comest *>
                                         (newit.type)->spoils);
        newit.bday = newit.bday - average_used_age;
    }
    // for food items
    if (newit.is_food()) {
        int bday_tmp = newit.bday % 3600; // fuzzy birthday for stacking reasons
        newit.bday = int(newit.bday) + 3600 - bday_tmp;

        if (newit.has_flag("EATEN_HOT")) { // hot foods generated
            newit.item_tags.insert("HOT");
            newit.active = true;
            newit.item_counter = 600;
        }
    }

    u.inv.assign_empty_invlet(newit);
    if (newit.made_of(LIQUID)) {
        while(!handle_liquid(newit, false, false)) { ; }
    } else {
        // We might not have space for the item
        if (!u.can_pickVolume(newit.volume())) { //Accounts for result_mult
            add_msg(_("There's no room in your inventory for the %s, so you drop it."),
                    newit.tname().c_str());
            m.add_item_or_charges(u.posx, u.posy, newit);
        } else if (!u.can_pickWeight(newit.weight(), !OPTIONS["DANGEROUS_PICKUPS"])) {
            add_msg(_("The %s is too heavy to carry, so you drop it."),
                    newit.tname().c_str());
            m.add_item_or_charges(u.posx, u.posy, newit);
        } else {
            newit = u.i_add(newit);
            add_msg(m_info, "%c - %s", newit.invlet == 0 ? ' ' : newit.invlet, newit.tname().c_str());
        }
    }
    u.inv.restack(&u);
}

std::list<item> game::consume_items(player *p, std::vector<component> components)
{
    std::list<item> ret;
    // For each set of components in the recipe, fill you_have with the list of all
    // matching ingredients the player has.
    std::vector<component> player_has;
    std::vector<component> map_has;
    std::vector<component> mixed;
    enum {
        use_from_map = 1,
        use_from_player = 2,
        use_from_both = 1 | 2
    } use_from;
    component selected_comp("", 0);
    inventory map_inv;
    map_inv.form_from_map(point(p->posx, p->posy), PICKUP_RANGE);

    for (unsigned i = 0; i < components.size(); i++) {
        itype_id type = components[i].type;
        int count = abs(components[i].count);
        bool pl = false, mp = false;

        if (components[i].available != 1) {
            continue;
        }

        if (item_controller->find_template(type)->count_by_charges() && count > 0) {
            if (p->has_charges(type, count)) {
                player_has.push_back(components[i]);
                pl = true;
            }
            if (map_inv.has_charges(type, count)) {
                map_has.push_back(components[i]);
                mp = true;
            }
            if (!pl && !mp && p->charges_of(type) + map_inv.charges_of(type) >= count) {
                mixed.push_back(components[i]);
            }
        } else { // Counting by units, not charges

            if (p->has_amount(type, count)) {
                player_has.push_back(components[i]);
                pl = true;
            }
            if (map_inv.has_components(type, count)) {
                map_has.push_back(components[i]);
                mp = true;
            }
            if (!pl && !mp && p->amount_of(type) + map_inv.amount_of(type) >= count) {
                mixed.push_back(components[i]);
            }

        }
    }

    if (player_has.size() + map_has.size() + mixed.size() == 1) { // Only 1 choice
        if (player_has.size() == 1) {
            use_from = use_from_player;
            selected_comp = player_has[0];
        } else if (map_has.size() == 1) {
            use_from = use_from_map;
            selected_comp = map_has[0];
        } else {
            use_from = use_from_both;
            selected_comp = mixed[0];
        }
    } else { // Let the player pick which component they want to use
        std::vector<std::string> options; // List for the menu_vec below
        // Populate options with the names of the items
        for (unsigned i = 0; i < map_has.size(); i++) {
            std::string tmpStr = item_controller->find_template(map_has[i].type)->name + _(" (nearby)");
            options.push_back(tmpStr);
        }
        for (unsigned i = 0; i < player_has.size(); i++) {
            options.push_back(item_controller->find_template(player_has[i].type)->name);
        }
        for (unsigned i = 0; i < mixed.size(); i++) {
            std::string tmpStr = item_controller->find_template(mixed[i].type)->name +
                                 _(" (on person & nearby)");
            options.push_back(tmpStr);
        }

        // unlike with tools, it's a bad thing if there aren't any components available
        if (options.empty()) {
            if (!(p->has_trait("WEB_ROPE"))) {
            debugmsg("Attempted a recipe with no available components!");
            }
            return ret;
        }

        // Get the selection via a menu popup
        size_t selection = menu_vec(false, _("Use which component?"), options) - 1;
        if (selection < map_has.size()) {
            use_from = use_from_map;
            selected_comp = map_has[selection];
        } else if (selection < map_has.size() + player_has.size()) {
            selection -= map_has.size();
            use_from = use_from_player;
            selected_comp = player_has[selection];
        } else {
            selection -= map_has.size() + player_has.size();
            use_from = use_from_both;
            selected_comp = mixed[selection];
        }
    }

    const point loc(p->posx, p->posy);
    itype *itt = item_controller->find_template(selected_comp.type);
    const bool by_charges = (itt->count_by_charges() && selected_comp.count > 0);
    // Count given to use_amount/use_charges, changed by those functions!
    int real_count = abs(selected_comp.count);
    const bool in_container = (selected_comp.count < 0);
    // First try to get everything from the map, than (remaining amount) from player
    if (use_from & use_from_map) {
        if (by_charges) {
            std::list<item> tmp = m.use_charges(loc, PICKUP_RANGE, selected_comp.type, real_count);
            ret.splice(ret.end(), tmp);
        } else {
            std::list<item> tmp = m.use_amount(loc, PICKUP_RANGE, selected_comp.type, real_count, in_container);
            remove_ammo( tmp, *p );
            ret.splice(ret.end(), tmp);
        }
    }
    if (use_from & use_from_player) {
        if (by_charges) {
            std::list<item> tmp = p->use_charges(selected_comp.type, real_count);
            ret.splice(ret.end(), tmp);
        } else {
            std::list<item> tmp = p->use_amount(selected_comp.type, real_count, in_container);
            remove_ammo( tmp, *p );
            ret.splice(ret.end(), tmp);
        }
    }
    // condense those items into one
    if (by_charges && ret.size() > 1) {
        std::list<item>::iterator b = ret.begin();
        b++;
        while(ret.size() > 1) {
            ret.front().charges += b->charges;
            b = ret.erase(b);
        }
    }
    p->lastconsumed = selected_comp.type;
    return ret;
}

void game::consume_tools(player *p, std::vector<component> tools, bool force_available)
{
    bool found_nocharge = false;
    inventory map_inv;
    map_inv.form_from_map(point(p->posx, p->posy), PICKUP_RANGE);
    std::vector<component> player_has;
    std::vector<component> map_has;
    // Use charges of any tools that require charges used
    for (unsigned i = 0; i < tools.size() && !found_nocharge; i++) {
        if (!force_available && tools[i].available != 1) {
            continue;
        }
        itype_id type = tools[i].type;
        if (tools[i].count > 0) {
            long count = tools[i].count;
            if (p->has_charges(type, count)) {
                player_has.push_back(tools[i]);
            }
            if (map_inv.has_charges(type, count)) {
                map_has.push_back(tools[i]);
            }
        } else if (p->has_amount(type, 1) || map_inv.has_tools(type, 1)) {
            found_nocharge = true;
        }
    }
    if (found_nocharge) {
        return;    // Default to using a tool that doesn't require charges
    }

    if (player_has.size() + map_has.size() == 1) {
        if(map_has.empty()) {
            p->use_charges(player_has[0].type, player_has[0].count);
        } else {
            m.use_charges(p->pos(), PICKUP_RANGE, map_has[0].type, map_has[0].count);
        }
    } else { // Variety of options, list them and pick one
        // Populate the list
        std::vector<std::string> options;
        for (unsigned i = 0; i < map_has.size(); i++) {
            std::string tmpStr = item_controller->find_template(map_has[i].type)->name + _(" (nearby)");
            options.push_back(tmpStr);
        }
        for (unsigned i = 0; i < player_has.size(); i++) {
            options.push_back(item_controller->find_template(player_has[i].type)->name);
        }

        if (options.empty()) { // This SHOULD only happen if cooking with a fire,
            return;    // and the fire goes out.
        }

        // Get selection via a popup menu
        size_t selection = menu_vec(false, _("Use which tool?"), options) - 1;
        if (selection < map_has.size())
            m.use_charges(point(p->posx, p->posy), PICKUP_RANGE,
                          map_has[selection].type, map_has[selection].count);
        else {
            selection -= map_has.size();
            p->use_charges(player_has[selection].type, player_has[selection].count);
        }
    }
}

recipe *game::get_disassemble_recipe(const itype_id &type)
{
    for (recipe_map::iterator cat_iter = recipes.begin(); cat_iter != recipes.end(); ++cat_iter) {
        for (recipe_list::iterator list_iter = cat_iter->second.begin();
             list_iter != cat_iter->second.end(); ++list_iter) {
            recipe *cur_recipe = *list_iter;
            if (type == cur_recipe->result && cur_recipe->reversible) {
                return cur_recipe;
            }
        }
    }
    // no matching disassemble recipe found.
    return NULL;
}

bool game::can_disassemble(item *dis_item, recipe *cur_recipe, inventory &crafting_inv, bool print_msg)
{
    if (dis_item->count_by_charges()) {
        // Create a new item to get the default charges
        const item tmp = cur_recipe->create_result();
        if (dis_item->charges < tmp.charges) {
            if (print_msg) {
                popup(ngettext("You need at least %d charge of that item to disassemble it.",
                               "You need at least %d charges of that item to disassemble it.",
                               tmp.charges),
                      tmp.charges);
            }
            return false;
        }
    }
    // ok, a valid recipe exists for the item, and it is reversible
    // assign the activity
    // check tools are available
    // loop over the tools and see what's required...again
    bool have_all_tools = true;
    for (unsigned j = 0; j < cur_recipe->tools.size(); j++) {
        bool have_this_tool = false;
        for (unsigned k = 0; k < cur_recipe->tools[j].size(); k++) {
            itype_id type = cur_recipe->tools[j][k].type;
            int req = cur_recipe->tools[j][k].count; // -1 => 1

            if ((req <= 0 && crafting_inv.has_tools (type, 1)) ||
                // No welding, no goggles needed.
                (req <= 0 && type == "goggles_welding") ||
                (req <= 0 && type == "crucible" &&
                    cur_recipe->result != "anvil") ||
                // No mold needed for disassembly.
                (req <= 0 && type == "mold_plastic") ||
                (req >  0 && crafting_inv.has_charges(type, req))) {
                have_this_tool = true;
                k = cur_recipe->tools[j].size();
            }
            // If crafting recipe required a welder,
            // disassembly requires a hacksaw or super toolkit.
            if (type == "welder") {
                have_this_tool = (crafting_inv.has_tools("hacksaw", 1) ||
                                    crafting_inv.has_tools("toolset", 1));
            }
        }
        if (!have_this_tool) {
            have_all_tools = false;
            if (print_msg) {
                int req = cur_recipe->tools[j][0].count;
                if (cur_recipe->tools[j][0].type == "welder") {
                    add_msg(m_info, _("You need a hacksaw to disassemble this."));
                } else {
                    if (req <= 0) {
                        add_msg(m_info, _("You need a %s to disassemble this."),
                                item_controller->find_template(cur_recipe->tools[j][0].type)->name.c_str());
                    } else {
                        add_msg(m_info, ngettext("You need a %s with %d charge to disassemble this.",
                                                 "You need a %s with %d charges to disassemble this.",
                                                 req),
                                item_controller->find_template(cur_recipe->tools[j][0].type)->name.c_str(), req);
                    }
                }
            }
        }
    }
    // all tools present, so assign the activity
    return have_all_tools;
}

void game::disassemble(int pos)
{
    if (pos == INT_MAX) {
        pos = inv(_("Disassemble item:"));
    }
    if (!u.has_item(pos)) {
        add_msg(m_info, _("You don't have that item!"), pos);
        return;
    }

    item *dis_item = &u.i_at(pos);
    recipe *cur_recipe = get_disassemble_recipe(dis_item->type->id);
    if (cur_recipe != NULL) {
        inventory crafting_inv = crafting_inventory(&u);
        if (can_disassemble(dis_item, cur_recipe, crafting_inv, true)) {
            if (OPTIONS["QUERY_DISASSEMBLE"] &&
                !(query_yn(_("Really disassemble your %s?"), dis_item->tname().c_str()))) {
                return;
            }
            u.assign_activity(ACT_DISASSEMBLE, cur_recipe->time, cur_recipe->id);
            u.activity.values.push_back(pos);
        }
        return; // recipe exists, but no tools, so do not start disassembly
    }
    //if we're trying to disassemble a book or magazine
    if(dis_item->is_book()) {
        if (OPTIONS["QUERY_DISASSEMBLE"] &&
            !(query_yn(_("Do you want to tear %s into pages?"), dis_item->tname().c_str()))) {
            return;
        } else {
            //twice the volume then multiplied by 10 (a book with volume 3 will give 60 pages)
            int num_pages = (dis_item->volume() * 2) * 10;
            m.spawn_item(u.posx, u.posy, "paper", 0, num_pages);
            u.i_rem(pos);
        }
        return;
    }

    // no recipe exists, or the item cannot be disassembled
    add_msg(m_info, _("This item cannot be disassembled!"));
}

void game::complete_disassemble()
{
    // which recipe was it?
    const int item_pos = u.activity.values[0];
    const bool from_ground = u.activity.values.size() > 1 && u.activity.values[1] == 1;
    recipe *dis = recipe_by_index(u.activity.index); // Which recipe is it?
    item *org_item;
    std::vector<item>& items_on_ground = m.i_at(u.posx, u.posy);
    if (from_ground) {
        if (static_cast<size_t>(item_pos) >= items_on_ground.size()) {
            add_msg(_("The item has vanished."));
            return;
        }
        org_item = &items_on_ground[item_pos];
        if (org_item->type->id != dis->result) {
            add_msg(_("The item might be gone, at least it is not at the expected position anymore."));
            return;
        }
    } else {
        org_item = &u.i_at(item_pos);
    }
    // Make a copy to keep its data (damage/components) even after it
    // has been removed.
    item dis_item = *org_item;

    float component_success_chance = std::min(std::pow(0.8f, dis_item.damage), 1.0);

    int veh_part = -1;
    vehicle *veh = m.veh_at(u.posx, u.posy, veh_part);
    if(veh != 0) {
        veh_part = veh->part_with_feature(veh_part, "CARGO");
    }

    add_msg(_("You disassemble the %s into its components."), dis_item.name.c_str());
    // remove any batteries or ammo first
    remove_ammo( &dis_item, u );

    if (dis_item.count_by_charges()) {
        // remove the charges that one would get from crafting it
        org_item->charges -= dis->create_result().charges;
    }
    // remove the item, except when it's counted by charges and still has some
    if (!org_item->count_by_charges() || org_item->charges <= 0) {
        if (from_ground) {
            items_on_ground.erase(items_on_ground.begin() + item_pos);
        } else {
            u.i_rem(item_pos);
        }
    }

    // consume tool charges
    for (unsigned j = 0; j < dis->tools.size(); j++) {
        consume_tools(&u, dis->tools[j], false);
    }

    // add the components to the map
    // Player skills should determine how many components are returned

    int skill_dice = 2 + u.skillLevel(dis->skill_used) * 3;
    skill_dice += u.skillLevel(dis->skill_used);

    // Sides on dice is 16 plus your current intelligence
    int skill_sides = 16 + u.int_cur;

    int diff_dice = dis->difficulty;
    int diff_sides = 24; // 16 + 8 (default intelligence)

    // disassembly only nets a bit of practice
    if (dis->skill_used) {
        u.practice( dis->skill_used, (dis->difficulty) * 2, dis->difficulty );
    }

    for (unsigned j = 0; j < dis->components.size(); j++) {
        const std::vector<component> &altercomps = dis->components[j];
        int alter_comp_index = 0;
        // If there are several (alternative) components, search the
        // one that was used. If not found, use the first one.
        // Don't check the first in altercomps, it's the default anyway.
        for(size_t k = 1; alter_comp_index == 0 && k < altercomps.size(); k++) {
            for( item::t_item_vector::iterator a = dis_item.components.begin();
                 a != dis_item.components.end(); ++a ) {
                if (a->type->id == altercomps[k].type) {
                    alter_comp_index = k;
                }
            }
        }
        const component &comp = altercomps[alter_comp_index];
        itype *itt = item_controller->find_template(comp.type);
        if (itt->item_tags.count("UNRECOVERABLE") > 0) {
            continue;
        }

        int compcount = comp.count;
        item newit(itt->id, calendar::turn);
        // Compress liquids and counted-by-charges items into one item,
        // they are added together on the map anyway and handle_liquid
        // should only be called once to put it all into a container at once.
        if (newit.count_by_charges() || newit.made_of(LIQUID)) {
            newit.charges = compcount;
            compcount = 1;
        } else if (!newit.craft_has_charges() && newit.charges > 0) {
            // tools that can be unloaded should be created unloaded,
            // tools that can't be unloaded will keep their default charges.
            newit.charges = 0;
        }

        for( ; compcount > 0; compcount--) {
            const bool comp_success = (dice(skill_dice, skill_sides) > dice(diff_dice,  diff_sides));
            if (dis->difficulty != 0 && !comp_success) {
                add_msg(m_bad, _("You fail to recover %s."), newit.tname().c_str());
                continue;
            }
            const bool dmg_success = component_success_chance > rng_float(0, 1);
            if (!dmg_success) {
                // Show reason for failure (damaged item, tname contains the damage adjective)
                add_msg(m_bad, _("You fail to recover %s from the %s."), newit.tname().c_str(), dis_item.tname().c_str());
                continue;
            }
            // Use item from components list, or (if not contained)
            // use newit, the default constructed.
            item act_item = newit;
            for(item::t_item_vector::iterator a = dis_item.components.begin(); a != dis_item.components.end(); ++a) {
                if (a->type == newit.type) {
                    act_item = *a;
                    dis_item.components.erase(a);
                    break;
                }
            }
            if (act_item.made_of(LIQUID)) {
                while (!handle_liquid(act_item, false, false)) {
                    // Try again, maybe use another container.
                }
            } else if (veh != NULL && veh->add_item(veh_part, act_item)) {
                // add_item did put the items in the vehicle, nothing further to be done
            } else {
                m.add_item_or_charges(u.posx, u.posy, act_item);
            }
        }
    }

    if (dis->learn_by_disassembly >= 0 && !u.knows_recipe(dis)) {
        if (dis->skill_used == NULL || dis->learn_by_disassembly <= u.skillLevel(dis->skill_used)) {
            if (one_in(4)) {
                u.learn_recipe(dis);
                add_msg(m_good, _("You learned a recipe from disassembling it!"));
            } else {
                add_msg(m_info, _("You might be able to learn a recipe if you disassemble another."));
            }
        } else {
            add_msg(m_info, _("If you had better skills, you might learn a recipe next time."));
        }
    }
}

recipe *game::recipe_by_index(int index)
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter) {
        for (recipe_list::iterator list_iter = map_iter->second.begin();
             list_iter != map_iter->second.end(); ++list_iter) {
            if ((*list_iter)->id == index) {
                return *list_iter;
            }
        }
    }
    return NULL;
}

recipe *recipe_by_name(std::string name)
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter) {
        for (recipe_list::iterator list_iter = map_iter->second.begin();
             list_iter != map_iter->second.end(); ++list_iter) {
            if ((*list_iter)->ident == name) {
                return *list_iter;
            }
        }
    }
    return NULL;
}

void check_component_list(const std::vector<std::vector<component> > &vec,
                                 const std::string &display_name)
{
    for (std::vector<std::vector<component> >::const_iterator b = vec.begin(); b != vec.end(); b++) {
        for (std::vector<component>::const_iterator c = b->begin(); c != b->end(); c++) {
            if (!item_controller->has_template(c->type)) {
                debugmsg("%s in %s is not a valid item template", c->type.c_str(), display_name.c_str());
            }
        }
    }
}

static void check_qualities(const std::vector<quality_requirement> &vec,
                                 const std::string &rName)
{
    for (std::vector<quality_requirement>::const_iterator b = vec.begin(); b != vec.end(); b++) {
        if (qualities.count(b->id) == 0) {
            debugmsg("Unknown quality %s in recipe %s", b->id.c_str(), rName.c_str());
        }
    }
}

void check_recipe_definitions()
{
    for (recipe_map::iterator map_iter = recipes.begin(); map_iter != recipes.end(); ++map_iter) {
        for (recipe_list::iterator list_iter = map_iter->second.begin();
             list_iter != map_iter->second.end(); ++list_iter) {
            const recipe &r = **list_iter;
            const std::string display_name = std::string("recipe ") + r.ident;
            ::check_component_list(r.tools, display_name);
            ::check_component_list(r.components, display_name);
            ::check_qualities(r.qualities, r.ident);
            if (!item_controller->has_template(r.result)) {
                debugmsg("result %s in recipe %s is not a valid item template", r.result.c_str(), r.ident.c_str());
            }
        }
    }
}

void remove_ammo(std::list<item> &dis_items, player &p) {
    for(auto a = dis_items.begin(); a != dis_items.end(); ++a) {
        remove_ammo( &*a, p );
    }
}

void remove_ammo(item *dis_item, player &p)
{
    if( dis_item->is_gun() ) {
        // Gun with gunmods, remove gunmods, remove their ammo
        while( !dis_item->contents.empty() ) {
            p.remove_gunmod( dis_item, 0 );
        }
    }
    while( !dis_item->contents.empty() ) {
        item tmp = dis_item->contents.front();
        dis_item->contents.erase( dis_item->contents.begin() );
        if( tmp.made_of( LIQUID ) && &p == &g->u ) {
            while( !g->handle_liquid( tmp, false, false ) ) {
                // Allow selecting several containers
            }
        } else {
            p.i_add_or_drop( tmp );
        }
    }
    if( dis_item->has_flag( "NO_UNLOAD" ) ) {
        return;
    }
    if( dis_item->is_gun() && dis_item->curammo != NULL && dis_item->ammo_type() != "NULL" ) {
        item ammodrop( dis_item->curammo->id, calendar::turn );
        ammodrop.charges = dis_item->charges;
        if( ammodrop.made_of( LIQUID ) && &p == &g->u ) {
            while( !g->handle_liquid( ammodrop, false, false ) ) {
                // Allow selecting several containers
            }
        } else {
            p.i_add_or_drop( ammodrop, 1 );
        }
        dis_item->charges = 0;
    }
    if( dis_item->is_tool() && dis_item->charges > 0 && dis_item->ammo_type() != "NULL" ) {
        item ammodrop( default_ammo( dis_item->ammo_type() ), calendar::turn );
        ammodrop.charges = dis_item->charges;
        if( dis_item->typeId() == "adv_UPS_off" || dis_item->typeId() == "adv_UPS_on" ) {
            ammodrop.charges /= 500;
        }
        if( ammodrop.made_of( LIQUID ) && &p == &g->u ) {
            while( !g->handle_liquid( ammodrop, false, false ) ) {
                // Allow selecting several containers
            }
        } else {
            p.i_add_or_drop( ammodrop, 1 );
        }
        dis_item->charges = 0;
    }
}

std::string recipe::required_skills_string() {
    std::ostringstream skills_as_stream;
    if(!required_skills.empty()){
        for(std::map<Skill*,int>::iterator iter=required_skills.begin(); iter!=required_skills.end();){
            skills_as_stream << iter->first->name() << "(" << iter->second << ")";
            ++iter;
            if(iter != required_skills.end()){
                skills_as_stream << ", ";
            }
        }
    }
    else{
        skills_as_stream << "N/A";
    }
    return skills_as_stream.str();
}
