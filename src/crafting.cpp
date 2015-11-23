#include "crafting.h"

#include "catacharset.h"
#include "craft_command.h"
#include "debug.h"
#include "game.h"
#include "input.h"
#include "inventory.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "messages.h"
#include "morale.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "recipe_dictionary.h"
#include "rng.h"
#include "translations.h"
#include "ui.h"
#include "vehicle.h"

#include <algorithm> //std::min
#include <fstream>
#include <iostream>
#include <math.h>    //sqrt
#include <queue>
#include <string>
#include <sstream>


enum TAB_MODE {
    NORMAL,
    FILTERED,
    BATCH
};

std::vector<std::string> craft_cat_list;
std::map<std::string, std::vector<std::string> > craft_subcat_list;

static void draw_recipe_tabs(WINDOW *w, std::string tab, TAB_MODE mode = NORMAL);
static void draw_recipe_subtabs(WINDOW *w, std::string tab, std::string subtab,
                                TAB_MODE mode = NORMAL);
static std::string first_craft_cat();
static std::string next_craft_cat(const std::string cat);
static std::string prev_craft_cat(const std::string cat);
static std::string first_craft_subcat(const std::string cat);
static std::string last_craft_subcat(const std::string cat);
static std::string next_craft_subcat(const std::string cat, const std::string subcat);
static std::string prev_craft_subcat(const std::string cat, const std::string subcat);

void remove_from_component_lookup(recipe* r);

recipe::recipe() :
    id(0), result("null"), contained(false),skill_used( NULL_ID ), reversible(false),
    autolearn(false), learn_by_disassembly(-1), result_mult(1)
{
}

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
    craft_subcat_list[category] = std::vector<std::string>();
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

// Check that the given recipe ident (rec_name) is unique, throw if not,
// Returns the id for the new recipe.
// If the recipe should override an existing one, the function removes the existing
// recipe and returns the id if the removed recipe.
int check_recipe_ident(const std::string &rec_name, JsonObject &jsobj)
{
    const bool override_existing = jsobj.get_bool("override", false);

    for( auto list_iter : recipe_dict ) {
        if (list_iter->ident == rec_name) {
            if (!override_existing) {
                jsobj.throw_error(
                    std::string("Recipe name collision (set a unique value for the id_suffix field to fix): ") +
                    rec_name, "result");
            }
            // overriding an existing recipe: delete it and remove the pointer
            // keep the id,
            const int tmp_id = list_iter->id;
            recipe_dict.remove( list_iter );
            delete list_iter;
            return tmp_id;
        }
    }

    return recipe_dict.size();
}

void load_recipe(JsonObject &jsobj)
{
    JsonArray jsarr;

    // required
    std::string result = jsobj.get_string("result");
    std::string category = jsobj.get_string("category");
    bool autolearn = jsobj.get_bool("autolearn");
    int time = jsobj.get_int("time");
    int difficulty = jsobj.get_int( "difficulty" );

    // optional
    bool contained = jsobj.get_bool("contained",false);
    std::string subcategory = jsobj.get_string("subcategory", "");
    bool reversible = jsobj.get_bool("reversible", false);
    skill_id skill_used( jsobj.get_string("skill_used", skill_id::NULL_ID.str() ) );
    std::string id_suffix = jsobj.get_string("id_suffix", "");
    int learn_by_disassembly = jsobj.get_int("decomp_learn", -1);
    double batch_rscale = 0.0;
    int batch_rsize = 0;
    if (jsobj.has_array( "batch_time_factors" )) {
        jsarr = jsobj.get_array( "batch_time_factors" );
        batch_rscale = (double)jsarr.get_int(0) / 100.0;
        batch_rsize = jsarr.get_int(1);
    }
    int result_mult = jsobj.get_int("result_mult", 1);

    std::map<std::string, int> requires_skills;
    jsarr = jsobj.get_array("skills_required");
    if (!jsarr.empty()) {
        // could be a single requirement, or multiple
        if( jsarr.has_array(0) ) {
            while (jsarr.has_more()) {
                JsonArray ja = jsarr.next_array();
                requires_skills[ja.get_string(0)] = ja.get_int(1);
            }
        } else {
            requires_skills[jsarr.get_string(0)] = jsarr.get_int(1);
        }
    }

    std::vector<byproduct> bps;
    // could be a single byproduct - either id or byproduct, or array of ids and byproducts
    if (jsobj.has_string("byproducts")) {
        bps.push_back(byproduct(jsobj.get_string("byproducts")));
    } else if (jsobj.has_object("byproducts")) {
        JsonObject jsbp = jsobj.get_object("byproducts");
        bps.push_back(byproduct(jsbp.get_string("id"), jsbp.get_int("charges_mult", 1),
                                jsbp.get_int("amount", 1)));
    } else if (jsobj.has_array("byproducts")) {
        jsarr = jsobj.get_array("byproducts");
        while (jsarr.has_more()) {
            if (jsarr.has_string(0)) {
                bps.push_back(byproduct(jsarr.next_string()));
            } else if (jsarr.has_object(0)) {
                JsonObject jsbp = jsarr.next_object();
                bps.push_back(byproduct(jsbp.get_string("id"), jsbp.get_int("charges_mult", 1),
                                        jsbp.get_int("amount", 1)));
            }
        }
    }

    std::string rec_name = result + id_suffix;
    int id = check_recipe_ident(rec_name, jsobj); // may delete recipes

    recipe *rec = new recipe();

    rec->ident = rec_name;
    rec->id = id;
    rec->result = result;
    rec->time = time;
    rec->difficulty = difficulty;
    rec->byproducts = bps;
    rec->cat = category;
    rec->contained = contained;
    rec->subcat = subcategory;
    rec->skill_used = skill_used;
    for( const auto &elem : requires_skills ) {
        rec->required_skills[skill_id( elem.first )] = elem.second;
    }
    rec->reversible = reversible;
    rec->autolearn = autolearn;
    rec->learn_by_disassembly = learn_by_disassembly;
    rec->batch_rscale = batch_rscale;
    rec->batch_rsize = batch_rsize;
    rec->result_mult = result_mult;

    rec->requirements.load(jsobj);

    jsarr = jsobj.get_array("book_learn");
    while (jsarr.has_more()) {
        JsonArray ja = jsarr.next_array();
        recipe::bookdata_t bd{ ja.get_string( 0 ), ja.get_int( 1 ), "", false };
        if( ja.size() >= 3 ) {
            bd.recipe_name = ja.get_string( 2 );
            bd.hidden = bd.recipe_name.empty();
        }
        rec->booksets.push_back( bd );
    }

    // Note, a recipe has to be fully instantiated before adding
    recipe_dict.add( rec );
}

void reset_recipes()
{
    recipe_dict.clear();
}

void finalize_recipes()
{
    for( auto r : recipe_dict ) {
        for( auto j = r->booksets.begin(); j != r->booksets.end(); ++j ) {
            const std::string &book_id = j->book_id;
            if( !item::type_is_defined( book_id ) ) {
                debugmsg("book %s for recipe %s does not exist", book_id.c_str(), r->ident.c_str());
                continue;
            }
            itype *t = item::find_type( book_id );
            if( !t->book ) {
                // TODO: we could make up a book slot?
                debugmsg("book %s for recipe %s is not a book", book_id.c_str(), r->ident.c_str());
                continue;
            }
            islot_book::recipe_with_description_t rwd{ r, j->skill_level, "", j->hidden };
            if( j->recipe_name.empty() ) {
                rwd.name = item::nname( r->result );
            } else {
                rwd.name = _( j->recipe_name.c_str() );
            }
            t->book->recipes.insert( rwd );
        }
        r->booksets.clear();
    }
}

bool player::crafting_allowed()
{
    if ( has_moral_to_craft() ) { // See morale.h
        add_msg(m_info, _("Your morale is too low to craft..."));
        return false;
    }

    if ( can_see_to_craft() ) {
        add_msg(m_info, _("You can't see to craft!"));
        return false;
    }

    return true;
}

bool player::can_see_to_craft()
{
    return fine_detail_vision_mod() > 4;
}

bool player::has_moral_to_craft()
{
    return morale_level() < MIN_MORALE_CRAFT;
}

void player::craft()
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size );
    if (rec) {
        if ( crafting_allowed() ) {
            make_craft( rec->ident, batch_size );
        }
    }
}

void player::recraft()
{
    if( lastrecipe.empty() ) {
        popup(_("Craft something first"));
    } else if (making_would_work( lastrecipe, last_batch )) {
        last_craft.execute();
    }
}

void player::long_craft()
{
    int batch_size = 0;
    const recipe *rec = select_crafting_recipe( batch_size );
    if (rec) {
        if( crafting_allowed() ) {
            make_all_craft( rec->ident, batch_size );
        }
    }
}

bool player::making_would_work(const std::string &id_to_make, int batch_size)
{
    if (!crafting_allowed()) {
        return false;
    }

    const recipe *making = recipe_by_name( id_to_make );
    if( making == nullptr ) {
        return false;
    }

    if( !can_make( making, batch_size ) ) {
        std::ostringstream buffer;
        buffer << _("You can no longer make that craft!") << "\n";
        buffer << making->requirements.list_missing();
        popup(buffer.str(), PF_NONE);
        return false;
    }

    return making->check_eligible_containers_for_crafting(batch_size);
}

bool recipe::check_eligible_containers_for_crafting(int batch) const
{
    std::vector<item> conts = g->u.get_eligible_containers_for_crafting();
    std::vector<item> res = create_results(batch);
    std::vector<item> bps = create_byproducts(batch);
    std::vector<item> all;
    all.reserve(res.size() + bps.size());
    all.insert(all.end(), res.begin(), res.end());
    all.insert(all.end(), bps.begin(), bps.end());

    for(const item & prod : all) {
        if( !prod.made_of(LIQUID)) {
            continue;
        }

        // we go trough half-filled containers first, then go through empty containers if we need
        std::sort( conts.begin(), conts.end(), item_compare_by_charges);

        long charges_to_store = prod.charges;
        for( const item & cont : conts) {
            if( charges_to_store <= 0) {
                break;
            }

            if( !cont.is_container_empty()) {
                if( cont.contents[0].type->id == prod.type->id) {
                    charges_to_store -= cont.get_remaining_capacity_for_liquid( cont.contents[0]);
                }
            } else {
                charges_to_store -= cont.get_remaining_capacity_for_liquid( prod);
            }
        }

        // also check if we're currently in a vehicle that has the necessary storage
        if(charges_to_store > 0) {
            vehicle *veh = g->m.veh_at(g->u.pos());
            if (veh != NULL) {
                const itype_id &ftype = prod.type->id;
                int fuel_cap = veh->fuel_capacity(ftype);
                int fuel_amnt = veh->fuel_left(ftype);

                if(fuel_cap >= 0) {
                    int fuel_space_left = fuel_cap - fuel_amnt;
                    charges_to_store -= fuel_space_left;
                }
            }
        }

        if (charges_to_store > 0) {
            popup(_("You don't have anything to store %s in!"), prod.tname().c_str());
            return false;
        }
    }

    return true;
}

bool is_container_eligible_for_crafting(const item &cont)
{
    if (cont.is_watertight_container()) {
        return !cont.is_container_full();
    }

    return false;
}

std::vector<item> player::get_eligible_containers_for_crafting()
{
    std::vector<item> conts;

    if (is_container_eligible_for_crafting(weapon)) {
        conts.push_back(weapon);
    }
    for (item &i : worn) {
        if (is_container_eligible_for_crafting(i)) {
            conts.push_back(i);
        }
    }
    for (size_t i = 0; i < inv.size(); i++) {
        for (item it : inv.const_stack(i)) {
            if (is_container_eligible_for_crafting(it)) {
                conts.push_back(it);
            }
        }
    }

    // get all potential containers within 1 tile including vehicles
    for( const auto &loc : closest_tripoints_first( 1, pos() ) ) {

        if( g->m.accessible_items( pos(), loc, 1 ) ) {
            for( item &it : g->m.i_at( loc ) ) {
                if( is_container_eligible_for_crafting( it ) ) {
                    conts.emplace_back( it );
                }
            }
        }

        int part = -1;
        vehicle *veh = g->m.veh_at( loc, part );
        if( veh && part >= 0 ) {
            part = veh->part_with_feature( part, "CARGO" );
            if( part != -1 ) {
                for( item &it : veh->get_items( part ) ) {
                    if( is_container_eligible_for_crafting( it ) ) {
                        conts.emplace_back( it );
                    }
                }
            }
        }
    }

    return conts;
}

bool player::can_make(const recipe *r, int batch_size)
{
    if (has_trait( "DEBUG_HS" )) {
        return true;
    }

    const inventory &crafting_inv = crafting_inventory();
    return r->can_make_with_inventory( crafting_inv, batch_size );
}

bool recipe::can_make_with_inventory(const inventory &crafting_inv, int batch) const
{
    if (g->u.has_trait( "DEBUG_HS" )) {
        return true;
    }

    if( !g->u.knows_recipe( this ) && -1 == g->u.has_recipe( this, crafting_inv) ) {
        return false;
    }
    return requirements.can_make_with_inventory(crafting_inv, batch);
}

static std::string first_craft_cat()
{
    return craft_cat_list.front();
}

static std::string next_craft_cat(const std::string cat)
{
    for (std::vector<std::string>::iterator iter = craft_cat_list.begin();
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

static std::string prev_craft_cat(const std::string cat)
{
    for (std::vector<std::string>::iterator iter = craft_cat_list.begin();
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

static std::string first_craft_subcat(const std::string cat)
{
    return craft_subcat_list[cat].front();
}

static std::string last_craft_subcat(const std::string cat)
{
    return craft_subcat_list[cat].back();
}

static std::string next_craft_subcat(const std::string cat, const std::string subcat)
{
    for (std::vector<std::string>::iterator iter = craft_subcat_list[cat].begin();
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

static std::string prev_craft_subcat(const std::string cat, const std::string subcat)
{
    for (std::vector<std::string>::iterator iter = craft_subcat_list[cat].begin();
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

const recipe *select_crafting_recipe( int &batch_size )
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
    std::string item_info_text;
    std::string tab = first_craft_cat();
    std::string subtab = first_craft_subcat( tab );
    std::vector<const recipe *> current;
    std::vector<bool> available;
    std::vector<std::string> component_print_buffer;
    const int componentPrintHeight = dataHeight - tailHeight - 1;
    //preserves component color printout between mode rotations
    nc_color rotated_color = c_white;
    int previous_item_line = -1;
    std::string previous_tab = "";
    std::string previous_subtab = "";
    item tmp;
    int line = 0, ypos;
    bool redraw = true;
    bool keepline = false;
    bool done = false;
    bool batch = false;
    int batch_line = 0;
    int display_mode = 0;
    const recipe *chosen = NULL;
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
    ctxt.register_action("CYCLE_BATCH");

    const inventory &crafting_inv = g->u.crafting_inventory();
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

            if( display_mode > 2 ){
                display_mode = 2;
            }

            TAB_MODE m = (batch) ? BATCH : (filterstring == "") ? NORMAL : FILTERED;
            draw_recipe_tabs(w_head, tab, m);
            draw_recipe_subtabs(w_subhead, tab, subtab, m);
            current.clear();
            available.clear();
            if (batch) {
                batch_recipes(crafting_inv, current, available, chosen);
            } else {
                // Set current to all recipes in the current tab; available are possible to make
                pick_recipes(crafting_inv, current, available, tab, subtab, filterstring);
            }
        }

        // Clear the screen of recipe data, and draw it anew
        werase(w_data);

        if ( isWide ) {
            mvwprintz(w_data, dataLines + 1, 5, c_white,
                      _("Press <ENTER> to attempt to craft object."));
            wprintz(w_data, c_white, "  ");
            if (filterstring != "") {
                wprintz(w_data, c_white, _("[E]: Describe, [F]ind, [R]eset, [m]ode, %s [?] keybindings"), (batch) ? _("cancel [b]atch") : _("[b]atch"));
            } else {
                wprintz(w_data, c_white, _("[E]: Describe, [F]ind, [m]ode, %s [?] keybindings"), (batch) ? _("cancel [b]atch") : _("[b]atch"));
            }
        } else {
            if (filterstring != "") {
                mvwprintz(w_data, dataLines + 1, 5, c_white,
                          _("[E]: Describe, [F]ind, [R]eset, [m]ode, [b]atch [?] keybindings"));
            } else {
                mvwprintz(w_data, dataLines + 1, 5, c_white,
                          _("[E]: Describe, [F]ind, [m]ode, [b]atch [?] keybindings"));
            }
            mvwprintz(w_data, dataLines + 2, 5, c_white,
                      _("Press <ENTER> to attempt to craft object."));
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
                    std::string tmp_name = item::nname(current[i]->result);
                    if (batch) {
                        tmp_name = string_format(_("%2dx %s"), i + 1, tmp_name.c_str());
                    }
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
                    std::string tmp_name = item::nname(current[i]->result);
                    if (batch) {
                        tmp_name = string_format(_("%2dx %s"), i + 1, tmp_name.c_str());
                    }
                    mvwprintz(w_data, dataLines + i - recmax, 2, c_ltgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, dataLines + i - recmax, 2,
                                  (available[i] ? h_white : h_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    } else {
                        mvwprintz(w_data, dataLines + i - recmax, 2,
                                  (available[i] ? c_white : c_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    }
                }
            } else {
                for (int i = line - dataHalfLines; i < line - dataHalfLines + dataLines; ++i) {
                    std::string tmp_name = item::nname(current[i]->result);
                    if (batch) {
                        tmp_name = string_format(_("%2dx %s"), i + 1, tmp_name.c_str());
                    }
                    mvwprintz(w_data, dataHalfLines + i - line, 2, c_ltgray, ""); // Clear the line
                    if (i == line) {
                        mvwprintz(w_data, dataHalfLines + i - line, 2,
                                  (available[i] ? h_white : h_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    } else {
                        mvwprintz(w_data, dataHalfLines + i - line, 2,
                                  (available[i] ? c_white : c_dkgray),
                                  utf8_truncate(tmp_name, 28).c_str());
                    }
                }
            }
        } else {
            for (size_t i = 0; i < current.size() && i < (size_t)dataHeight + 1; ++i) {
                std::string tmp_name = item::nname(current[i]->result);
                if (batch) {
                    tmp_name = string_format(_("%2dx %s"), (int)i + 1, tmp_name.c_str());
                }
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

            component_print_buffer = current[line]->requirements.get_folded_components_list(
                FULL_SCREEN_WIDTH - 30 - 1, col, crafting_inv, (batch) ? line + 1 : 1 );
            if( !g->u.knows_recipe( current[line] ) ) {
                component_print_buffer.push_back(_("Recipe not memorized yet"));
            }

            //handle positioning of component list if it needed to be scrolled
            int componentPrintOffset = 0;
            if(display_mode > 2){
                componentPrintOffset = (display_mode - 2) * componentPrintHeight;
            }
            if(component_print_buffer.size() < static_cast<size_t>( componentPrintOffset )){
                componentPrintOffset = 0;
                if( previous_tab != tab || previous_subtab != subtab || previous_item_line != line ){
                    display_mode = 2;
                }else{
                    display_mode = 0;
                }
            }

            //only used to preserve mode position on components when
            //moving to another item and the view is already scrolled
            previous_tab = tab;
            previous_subtab = subtab;
            previous_item_line = line;

            if(display_mode == 0) {
                mvwprintz(w_data, ypos++, 30, col, _("Skills used: %s"),
                          (!current[line]->skill_used ? _("N/A") :
                           current[line]->skill_used.obj().name().c_str()));

                mvwprintz(w_data, ypos++, 30, col, _("Required skills: %s"),
                          (current[line]->required_skills_string().c_str()));
                mvwprintz(w_data, ypos++, 30, col, _("Difficulty: %d"), current[line]->difficulty);
                if( !current[line]->skill_used ) {
                    mvwprintz(w_data, ypos++, 30, col, _("Your skill level: N/A"));
                } else {
                    mvwprintz(w_data, ypos++, 30, col, _("Your skill level: %d"),
                              // Macs don't seem to like passing this as a class, so force it to int
                              (int)g->u.skillLevel(current[line]->skill_used));
                }
                ypos += current[line]->print_time( w_data, ypos, 30, FULL_SCREEN_WIDTH - 30 - 1, col,
                                                   (batch) ? line + 1 : 1 );
                ypos += current[line]->print_items(w_data, ypos, 30, col, (batch) ? line + 1 : 1);
            }
            if(display_mode == 0 || display_mode == 1) {
                ypos += current[line]->requirements.print_tools(
                    w_data, ypos, 30, FULL_SCREEN_WIDTH - 30 - 1, col,
                    crafting_inv, (batch) ? line + 1 : 1 );
            }

            //color needs to be preserved in case part of the previous page was cut off
            nc_color stored_color = col;
            if( display_mode > 2 ){
                stored_color = rotated_color;
            } else {
                rotated_color = col;
            }
            int components_printed = 0;
            for( size_t i = static_cast<size_t>( componentPrintOffset );
                 i < component_print_buffer.size(); i++ ) {
                if( ypos >= componentPrintHeight ) {
                    break;
                }

                components_printed++;
                print_colored_text(w_data, ypos++, 30, stored_color, col, component_print_buffer[i]);
            }

            if( ypos >= componentPrintHeight &&
                component_print_buffer.size() > static_cast<size_t>( components_printed ) ) {
                mvwprintz(w_data, ypos++, 30, col, _("v (more)"));
                rotated_color = stored_color;
            }

            if ( isWide ) {
                if ( lastid != current[line]->id ) {
                    lastid = current[line]->id;
                    tmp = current[line]->create_result();
                    item_info_text = tmp.info( true );
                }
                mvwprintz(w_data, 0, FULL_SCREEN_WIDTH + 1, col, "%s",
                          utf8_truncate(tmp.type_name( 1 ), iInfoWidth).c_str());

                fold_and_print( w_data, 1, FULL_SCREEN_WIDTH + 1, iInfoWidth, col, item_info_text );
            }

        }

        draw_scrollbar(w_data, line, dataLines, recmax, 0);
        wrefresh(w_data);

        const std::string action = ctxt.handle_input();
        if (action == "CYCLE_MODE") {
            display_mode = display_mode + 1;
            if(display_mode <= 0) {
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
            } else if (!current[line]->check_eligible_containers_for_crafting((batch) ? line + 1 : 1)) {
                ; // popup is already inside check
            } else {
                chosen = current[line];
                batch_size = (batch) ? line + 1 : 1;
                done = true;
            }
        } else if (action == "HELP_RECIPE") {
            if (current.empty()) {
                popup(_("Nothing selected!"));
                redraw = true;
                continue;
            }
            tmp = current[line]->create_result();

            full_screen_popup("%s\n%s", tmp.type_name( 1 ).c_str(),  tmp.info(true).c_str());
            redraw = true;
            keepline = true;
        } else if (action == "FILTER") {
            filterstring = string_input_popup(_("Search:"), 85, filterstring,
                                              _("Special prefixes for requirements:\n"
                                                "  [t] search tools\n"
                                                "  [c] search components\n"
                                                "  [q] search qualities\n"
                                                "  [s] search skills\n"
                                                "  [S] search skill used only\n"
                                                "Special prefixes for results:\n"
                                                "  [Q] search qualities\n"
                                                "Examples:\n"
                                                "  t:soldering iron\n"
                                                "  c:two by four\n"
                                                "  q:metal sawing\n"
                                                "  s:cooking\n"
                                                "  Q:fine bolt turning"
                                                ));
            redraw = true;
        } else if (action == "QUIT") {
            chosen = nullptr;
            done = true;
        } else if (action == "RESET_FILTER") {
            filterstring = "";
            redraw = true;
        } else if (action == "CYCLE_BATCH") {
            if (current.empty()) {
                popup(_("Nothing selected!"));
                redraw = true;
                continue;
            }
            batch = !batch;
            if (batch) {
                batch_line = line;
                chosen = current[batch_line];
            } else {
                line = batch_line;
                keepline = true;
            }
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
    g->refresh_all();

    return chosen;
}

// Anchors top-right
static void draw_can_craft_indicator(WINDOW *w, int window_width, int margin_x, int margin_y)
{
    int x_align = window_width - margin_x;
    // Draw text
    mvwprintz(w, margin_y, x_align - utf8_width(_("can craft:")), c_ltgray, _("can craft:"));
    if( g->u.can_see_to_craft() ) {
        mvwprintz(w, margin_y + 1, x_align - 1 - utf8_width( _( "too dark" ) ),  i_red  , _( "too dark" ));
    } else if( g->u.has_moral_to_craft() ) {
        mvwprintz(w, margin_y + 1, x_align - 1 - utf8_width( _( "too sad" ) ),  i_red  , _( "too sad" ));
    } else {
        mvwprintz(w, margin_y + 1, x_align - 1 - utf8_width( _( "yes" ) ),  i_green  , _( "yes" ));
    }
}

static void draw_recipe_tabs(WINDOW *w, std::string tab, TAB_MODE mode)
{
    werase(w);
    int width = getmaxx(w);
    for (int i = 0; i < width; i++) {
        mvwputch(w, 2, i, BORDER_COLOR, LINE_OXOX);
    }

    mvwputch(w, 2,  0, BORDER_COLOR, LINE_OXXO); // |^
    mvwputch(w, 2, width - 1, BORDER_COLOR, LINE_OOXX); // ^|

    // Draw a "can craft" indicator
    draw_can_craft_indicator(w, width, 1, 0);

    switch (mode) {
    case NORMAL:
    {
        int pos_x = 2;//draw the tabs on each other
        int tab_step = 3;//step between tabs, two for tabs border
        draw_tab(w,  pos_x, _("WEAPONS"), tab == "CC_WEAPON");
        pos_x += utf8_width(_("WEAPONS")) + tab_step;
        draw_tab(w, pos_x,  _("AMMO"),    tab == "CC_AMMO");
        pos_x += utf8_width(_("AMMO")) + tab_step;
        draw_tab(w, pos_x,  _("FOOD"),    tab == "CC_FOOD");
        pos_x += utf8_width(_("FOOD")) + tab_step;
        draw_tab(w, pos_x,  _("CHEMS"),   tab == "CC_CHEM");
        pos_x += utf8_width(_("CHEMS")) + tab_step;
        draw_tab(w, pos_x,  _("ELECTRICAL"), tab == "CC_ELECTRONIC");
        pos_x += utf8_width(_("ELECTRICAL")) + tab_step;
        draw_tab(w, pos_x,  _("ARMOR"),   tab == "CC_ARMOR");
        pos_x += utf8_width(_("ARMOR")) + tab_step;
        draw_tab(w, pos_x,  _("OTHER"),   tab == "CC_OTHER");
        break;
    }
    case FILTERED:
        draw_tab(w, 2, _("Searched"), true);
        break;
    case BATCH:
        draw_tab(w, 2, _("Batch"), true);
        break;
    }

    wrefresh(w);
}

static void draw_recipe_subtabs(WINDOW *w, std::string tab, std::string subtab, TAB_MODE mode)
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

    switch (mode) {
    case NORMAL:
    {
        int pos_x = 2;//draw the tabs on each other
        int tab_step = 3;//step between tabs, two for tabs border
        draw_subtab(w, pos_x, _("ALL"), subtab == "CSC_ALL");//Add ALL subcategory to all tabs;
        pos_x += utf8_width(_("ALL")) + tab_step;
        if (tab == "CC_WEAPON") {
            draw_subtab(w, pos_x, _("BASHING"), subtab == "CSC_WEAPON_BASHING");
            pos_x += utf8_width(_("BASHING")) + tab_step;
            draw_subtab(w, pos_x, _("CUTTING"), subtab == "CSC_WEAPON_CUTTING");
            pos_x += utf8_width(_("CUTTING")) + tab_step;
            draw_subtab(w, pos_x, _("PIERCING"), subtab == "CSC_WEAPON_PIERCING");
            pos_x += utf8_width(_("PIERCING")) + tab_step;
            draw_subtab(w, pos_x, _("RANGED"), subtab == "CSC_WEAPON_RANGED");
            pos_x += utf8_width(_("RANGED")) + tab_step;
            draw_subtab(w, pos_x, _("EXPLOSIVE"), subtab == "CSC_WEAPON_EXPLOSIVE");
            pos_x += utf8_width(_("EXPLOSIVE")) + tab_step;
            draw_subtab(w, pos_x, _("MODS"), subtab == "CSC_WEAPON_MODS");
            pos_x += utf8_width(_("MODS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), subtab == "CSC_WEAPON_OTHER");
        } else if (tab == "CC_AMMO") {
            draw_subtab(w, pos_x, _("BULLETS"), subtab == "CSC_AMMO_BULLETS");
            pos_x += utf8_width(_("BULLETS")) + tab_step;
            draw_subtab(w, pos_x, _("ARROWS"), subtab == "CSC_AMMO_ARROWS");
            pos_x += utf8_width(_("ARROWS")) + tab_step;
            draw_subtab(w, pos_x, _("COMPONENTS"), subtab == "CSC_AMMO_COMPONENTS");
            pos_x += utf8_width(_("COMPONENTS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), subtab == "CSC_AMMO_OTHER");
        } else if (tab == "CC_FOOD") {
            draw_subtab(w, pos_x, _("DRINKS"), subtab == "CSC_FOOD_DRINKS");
            pos_x += utf8_width(_("DRINKS")) + tab_step;
            draw_subtab(w, pos_x, _("MEAT"), subtab == "CSC_FOOD_MEAT");
            pos_x += utf8_width(_("MEAT")) + tab_step;
            draw_subtab(w, pos_x, _("VEGGI"), subtab == "CSC_FOOD_VEGGI");
            pos_x += utf8_width(_("VEGGI")) + tab_step;
            draw_subtab(w, pos_x, _("SNACK"), subtab == "CSC_FOOD_SNACK");
            pos_x += utf8_width(_("SNACK")) + tab_step;
            draw_subtab(w, pos_x, _("BREAD"), subtab == "CSC_FOOD_BREAD");
            pos_x += utf8_width(_("BREAD")) + tab_step;
            draw_subtab(w, pos_x, _("PASTA"), subtab == "CSC_FOOD_PASTA");
            pos_x += utf8_width(_("PASTA")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), subtab == "CSC_FOOD_OTHER");
        } else if (tab == "CC_CHEM") {
            draw_subtab(w, pos_x, _("DRUGS"), subtab == "CSC_CHEM_DRUGS");
            pos_x += utf8_width(_("DRUGS")) + tab_step;
            draw_subtab(w, pos_x, _("MUTAGEN"), subtab == "CSC_CHEM_MUTAGEN");
            pos_x += utf8_width(_("MUTAGEN")) + tab_step;
            draw_subtab(w, pos_x, _("CHEMICALS"), subtab == "CSC_CHEM_CHEMICALS");
            pos_x += utf8_width(_("CHEMICALS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), subtab == "CSC_CHEM_OTHER");
        } else if (tab == "CC_ELECTRONIC") {
            draw_subtab(w, pos_x, _("CBMS"), subtab == "CSC_ELECTRONIC_CBMS");
            pos_x += utf8_width(_("CBMS")) + tab_step;
            draw_subtab(w, pos_x, _("TOOLS"), subtab == "CSC_ELECTRONIC_TOOLS");
            pos_x += utf8_width(_("TOOLS")) + tab_step;
            draw_subtab(w, pos_x, _("PARTS"), subtab == "CSC_ELECTRONIC_PARTS");
            pos_x += utf8_width(_("PARTS")) + tab_step;
            draw_subtab(w, pos_x, _("LIGHTING"), subtab == "CSC_ELECTRONIC_LIGHTING");
            pos_x += utf8_width(_("LIGHTING")) + tab_step;
            draw_subtab(w, pos_x, _("COMPONENTS"), subtab == "CSC_ELECTRONIC_COMPONENTS");
            pos_x += utf8_width(_("COMPONENTS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), subtab == "CSC_ELECTRONIC_OTHER");
        } else if (tab == "CC_ARMOR") {
            draw_subtab(w, pos_x, _("STORAGE"), subtab == "CSC_ARMOR_STORAGE");
            pos_x += utf8_width(_("STORAGE")) + tab_step;
            draw_subtab(w, pos_x, _("SUIT"), subtab == "CSC_ARMOR_SUIT");
            pos_x += utf8_width(_("SUIT")) + tab_step;
            draw_subtab(w, pos_x, _("HEAD"), subtab == "CSC_ARMOR_HEAD");
            pos_x += utf8_width(_("HEAD")) + tab_step;
            draw_subtab(w, pos_x, _("TORSO"), subtab == "CSC_ARMOR_TORSO");
            pos_x += utf8_width(_("TORSO")) + tab_step;
            draw_subtab(w, pos_x, _("ARMS"), subtab == "CSC_ARMOR_ARMS");
            pos_x += utf8_width(_("ARMS")) + tab_step;
            draw_subtab(w, pos_x, _("HANDS"), subtab == "CSC_ARMOR_HANDS");
            pos_x += utf8_width(_("HANDS")) + tab_step;
            draw_subtab(w, pos_x, _("LEGS"), subtab == "CSC_ARMOR_LEGS");
            pos_x += utf8_width(_("LEGS")) + tab_step;
            draw_subtab(w, pos_x, _("FEET"), subtab == "CSC_ARMOR_FEET");
            pos_x += utf8_width(_("FEET")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), subtab == "CSC_ARMOR_OTHER");
        } else if (tab == "CC_OTHER") {
            draw_subtab(w, pos_x, _("TOOLS"), subtab == "CSC_OTHER_TOOLS");
            pos_x += utf8_width(_("TOOLS")) + tab_step;
            draw_subtab(w, pos_x, _("MEDICAL"), subtab == "CSC_OTHER_MEDICAL");
            pos_x += utf8_width(_("MEDICAL")) + tab_step;
            draw_subtab(w, pos_x, _("CONTAINERS"), subtab == "CSC_OTHER_CONTAINERS");
            pos_x += utf8_width(_("CONTAINERS")) + tab_step;
            draw_subtab(w, pos_x, _("MATERIALS"), subtab == "CSC_OTHER_MATERIALS");
            pos_x += utf8_width(_("MATERIALS")) + tab_step;
            draw_subtab(w, pos_x, _("PARTS"), subtab == "CSC_OTHER_PARTS");
            pos_x += utf8_width(_("PARTS")) + tab_step;
            draw_subtab(w, pos_x, _("TRAPS"), subtab == "CSC_OTHER_TRAPS");
            pos_x += utf8_width(_("TRAPS")) + tab_step;
            draw_subtab(w, pos_x, _("OTHER"), subtab == "CSC_OTHER_OTHER");
        }
        break;
    }
    case FILTERED:
    case BATCH:
        werase(w);
        for (int i = 0; i < 3; i++) {
            mvwputch(w, i,  0, BORDER_COLOR, LINE_XOXO); // |^
            mvwputch(w, i, width - 1, BORDER_COLOR,  LINE_XOXO); // ^|
        }
        break;
    }

    wrefresh(w);
}

int recipe::print_items(WINDOW *w, int ypos, int xpos, nc_color col, int batch) const
{
    if(!has_byproducts()) {
        return 0;
    }

    const int oldy = ypos;

    mvwprintz(w, ypos++, xpos, col, _( "Byproducts:" ));
    for (auto &bp : byproducts) {
        print_item(w, ypos++, xpos, col, bp, batch);
    }

    return ypos - oldy;
}

void recipe::print_item(WINDOW *w, int ypos, int xpos, nc_color col, const byproduct &bp, int batch) const
{
    item it(bp.result, calendar::turn, false);
    std::string str = string_format( _("> %d %s"), (it.charges > 0) ? bp.amount : bp.amount * batch,
                                     it.tname().c_str() );
    if (it.charges > 0) {
        str = string_format(_("%s (%d)"), str.c_str(), it.charges * bp.charges_mult * batch);
    }
    mvwprintz(w, ypos, xpos, col, str.c_str());
}

const inventory& player::crafting_inventory()
{
    if (cached_moves == moves
            && cached_turn == calendar::turn.get_turn()
            && cached_position == pos3()) {
        return cached_crafting_inventory;
    }
    cached_crafting_inventory.form_from_map(pos3(), PICKUP_RANGE, false);
    cached_crafting_inventory += inv;
    cached_crafting_inventory += weapon;
    cached_crafting_inventory += worn;
    if (has_active_bionic("bio_tools")) {
        item tools("toolset", calendar::turn);
        tools.charges = power_level;
        cached_crafting_inventory += tools;
    }
    cached_moves = moves;
    cached_turn = calendar::turn.get_turn();
    cached_position = pos3();
    return cached_crafting_inventory;
}

void player::invalidate_crafting_inventory()
{
    cached_turn = -1;
}

int recipe::print_time(WINDOW *w, int ypos, int xpos, int width,
                       nc_color col, int batch) const
{
    const int turns = batch_time(batch) / 100;
    std::string text;
    if( turns < MINUTES( 1 ) ) {
        const int seconds = std::max( 1, turns * 6 );
        text = string_format( ngettext( "%d second", "%d seconds", seconds ), seconds );
    } else {
        const int minutes = ( turns % HOURS( 1 ) ) / MINUTES( 1 );
        const int hours = turns / HOURS( 1 );
        if( hours == 0 ) {
            text = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
        } else if( minutes == 0 ) {
            text = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
        } else {
            const std::string h = string_format( ngettext( "%d hour", "%d hours", hours ), hours );
            const std::string m = string_format( ngettext( "%d minute", "%d minutes", minutes ), minutes );
            //~ A time duration: first is hours, second is minutes, e.g. "4 hours" "6 minutes"
            text = string_format( _( "%1$s and %2$s" ), h.c_str(), m.c_str() );
        }
    }
    text = string_format( _( "Time to complete: %s" ), text.c_str() );
    return fold_and_print( w, ypos, xpos, width, col, text );
}

// ui.cpp
extern bool lcmatch(const std::string &str, const std::string &findstr);

template<typename T>
bool lcmatch_any(const std::vector< std::vector<T> > &list_of_list, const std::string &filter)
{
    for( auto &list : list_of_list ) {
        for( auto &comp : list ) {
            if( lcmatch( item::nname( comp.type ), filter ) ) {
                return true;
            }
        }
    }
    return false;
}

void batch_recipes(const inventory &crafting_inv,
                   std::vector<const recipe *> &current,
                   std::vector<bool> &available, const recipe *rec)
{
    current.clear();
    available.clear();

    for (int i = 1; i <= 20; i++) {
        current.push_back(rec);
        available.push_back(rec->can_make_with_inventory(crafting_inv, i));
    }
}

int recipe::batch_time(int batch) const
{
    if (batch_rscale == 0.0) {
        return time * batch;
    }

    // NPCs around you should assist in batch production if they have the skills
    int assistants = 0;
    for( auto &elem : g->active_npc ) {
        if (rl_dist( elem->pos(), g->u.pos() ) < PICKUP_RANGE && elem->is_friend()){
            if (elem->skillLevel(skill_used) >= difficulty)
                assistants++;
        }
    }

    double total_time = 0.0;
    double scale = batch_rsize / 6.0; // At batch_rsize, incremental time increase is 99.5% of batch_rscale
    for (int x = 0; x < batch; x++) {
        // scaled logistic function output
        double logf = (2.0/(1.0+exp(-((double)x/scale)))) - 1.0;
        total_time += (double)time * (1.0 - (batch_rscale * logf));
    }

    //Assistants can decrease the time for production but never less than that of one unit
    if (assistants == 1){
        total_time = total_time * .75;
    } else if (assistants >= 2) {
        total_time = total_time * .60;
    }
    if (total_time < time)
        total_time = time;

    return (int)total_time;
}

void pick_recipes(const inventory &crafting_inv,
                  std::vector<const recipe *> &current,
                  std::vector<bool> &available, std::string tab,
                  std::string subtab, std::string filter)
{
    bool search_name = true;
    bool search_tool = false;
    bool search_component = false;
    bool search_skill = false;
    bool search_skill_primary_only = false;
    bool search_qualities = false;
    bool search_result_qualities = false;
    size_t pos = filter.find(":");
    if(pos != std::string::npos) {
        search_name = false;
        std::string searchType = filter.substr(0, pos);
        for( auto &elem : searchType ) {
            if( elem == 'n' ) {
                search_name = true;
            } else if( elem == 't' ) {
                search_tool = true;
            } else if( elem == 'c' ) {
                search_component = true;
            } else if( elem == 's' ) {
                search_skill = true;
            } else if( elem == 'S' ) {
                search_skill_primary_only = true;
            } else if( elem == 'q' ) {
                search_qualities = true;
            } else if( elem == 'Q' ) {
                search_result_qualities = true;
            }
        }
        filter = filter.substr(pos + 1);
    }
    std::vector<recipe *> available_recipes;

    if (filter == "") {
        available_recipes = recipe_dict.in_category(tab);
    } else {
        // lcmatch needs an all lowercase string to match case-insensitive
        std::transform( filter.begin(), filter.end(), filter.begin(), tolower );

        available_recipes.insert( available_recipes.begin(), recipe_dict.begin(), recipe_dict.end() );
    }

    current.clear();
    available.clear();
    std::vector<const recipe *> filtered_list;
    int max_difficulty = 0;

    for( auto rec : available_recipes ) {

        if( subtab == "CSC_ALL" || rec->subcat == subtab ||
            (rec->subcat == "" && last_craft_subcat( tab ) == subtab) ||
            filter != "") {
            if(  (!g->u.knows_recipe(rec) && -1 == g->u.has_recipe(rec, crafting_inv))
              || (rec->difficulty < 0) ) {
                continue;
            }
            if(filter != "") {
                if( (search_name && !lcmatch( item::nname( rec->result ), filter ))
                 || (search_tool && !lcmatch_any( rec->requirements.tools, filter ))
                 || (search_component && !lcmatch_any( rec->requirements.components, filter )) ) {
                    continue;
                }
                bool match_found = false;
                if(search_result_qualities) {
                    itype *it = item::find_type(rec->result);
                    for( auto & quality : it->qualities ) {
                        if(lcmatch(quality::get_name(quality.first), filter)) {
                            match_found = true;
                            break;
                        }
                    }
                    if(!match_found) {
                        continue;
                    }
                }
                if(search_qualities) {
                    for( auto quality_reqs : rec->requirements.qualities ) {
                        for( auto quality : quality_reqs ) {
                            if(lcmatch( quality.to_string(), filter )) {
                                match_found = true;
                                break;
                            }
                        }
                        if(match_found) {
                            break;
                        }
                    }
                    if(!match_found) {
                        continue;
                    }
                }
                if(search_skill) {
                    if( !rec->skill_used) {
                        continue;
                    } else if( !lcmatch( rec->skill_used.obj().name(), filter ) &&
                               !lcmatch( rec->required_skills_string(), filter )) {
                        continue;
                    }
                }
                if(search_skill_primary_only) {
                    if( !rec->skill_used ) {
                        continue;
                    } else if( !lcmatch( rec->skill_used.obj().name(), filter )) {
                        continue;
                    }
                }
            }

            filtered_list.push_back(rec);

        }
        max_difficulty = std::max(max_difficulty, rec->difficulty);
    }

    int truecount = 0;
    for( int i = max_difficulty; i != -1; --i ) {
        for( auto rec : filtered_list ) {
            if (rec->difficulty == i) {
                if (rec->can_make_with_inventory(crafting_inv)) {
                    current.insert(current.begin(), rec);
                    available.insert(available.begin(), true);
                    truecount++;
                } else {
                    current.push_back(rec);
                    available.push_back(false);
                }
            }
        }
    }
    // This is so the list of available recipes is also is order of difficulty.
    std::reverse(current.begin(), current.begin() + truecount);
}

void player::make_craft(const std::string &id_to_make, int batch_size)
{
    make_craft_with_command(id_to_make, batch_size);
}

void player::make_all_craft(const std::string &id_to_make, int batch_size)
{
    make_craft_with_command(id_to_make, batch_size, true);
}

void player::make_craft_with_command( const std::string &id_to_make, int batch_size, bool is_long )
{
    const recipe *recipe_to_make = recipe_by_name( id_to_make );

    if( recipe_to_make == nullptr ) {
        return;
    }

    last_craft = craft_command( recipe_to_make, batch_size, is_long, this );
    last_craft.execute();
}

item recipe::create_result() const {
    item newit(result, calendar::turn, false);
    if (contained == true) {
        newit = newit.in_its_container();
    }
    if (result_mult != 1) {
        newit.charges *= result_mult;
    }
    if (!newit.craft_has_charges()) {
        newit.charges = 0;
    }
    if( newit.has_flag( "VARSIZE" ) ) {
        newit.item_tags.insert( "FIT" );
    }
    return newit;
}

std::vector<item> recipe::create_results(int batch) const
{
    std::vector<item> items;

    if( !item::count_by_charges( result ) ) {
        for (int i = 0; i < batch; i++) {
            item newit = create_result();
            items.push_back(newit);
        }
    } else {
        item newit = create_result();
        newit.charges *= batch;
        items.push_back(newit);
    }

    return items;
}

std::vector<item> recipe::create_byproducts(int batch) const
{
    std::vector<item> bps;
    for(auto &val : byproducts) {
        if( !item::count_by_charges( val.result ) ) {
            for (int i = 0; i < val.amount * batch; i++) {
                item newit(val.result, calendar::turn, false);
                if (!newit.craft_has_charges()) {
                    newit.charges = 0;
                }
                if( newit.has_flag( "VARSIZE" ) ) {
                    newit.item_tags.insert( "FIT" );
                }
                bps.push_back(newit);
            }
        } else {
            for (int i = 0; i < val.amount; i++) {
                item newit(val.result, calendar::turn, false);
                if (val.charges_mult != 1) {
                    newit.charges *= val.charges_mult;
                }
                newit.charges *= batch;
                if( newit.has_flag( "VARSIZE" ) ) {
                    newit.item_tags.insert( "FIT" );
                }
                bps.push_back(newit);
            }
        }
    }
    return bps;
}

bool recipe::has_byproducts() const
{
    return byproducts.size() != 0;
}

// @param offset is the index of the created item in the range [0, batch_size-1],
// it makes sure that the used items are distributed equally among the new items.
void set_components( std::vector<item> &components, const std::list<item> &used,
                     const int batch_size, const size_t offset )
{
    if( batch_size <= 1 ) {
        components.insert( components.begin(), used.begin(), used.end() );
        return;
    }
    // This count does *not* include items counted by charges!
    size_t non_charges_counter = 0;
    for( auto &tmp : used ) {
        if( tmp.count_by_charges() ) {
            components.push_back( tmp );
            // This assumes all (count-by-charges) items of the same type have been merged into one,
            // which has a charges value that can be evenly divided by batch_size.
            components.back().charges = tmp.charges / batch_size;
        } else {
            if( ( non_charges_counter + offset ) % batch_size == 0 ) {
                components.push_back( tmp );
            }
            non_charges_counter++;
        }
    }
}

void player::complete_craft()
{
    const recipe *making = recipe_by_index(activity.index); // Which recipe is it?
    int batch_size = activity.values.front();
    if( making == nullptr ) {
        debugmsg( "no recipe with id %d found", activity.index );
        activity.type = ACT_NULL;
        return;
    }

    // # of dice is 75% primary skill, 25% secondary (unless secondary is null)
    int skill_dice = skillLevel(making->skill_used) * 4;

    // farsightedness can impose a penalty on electronics and tailoring success
    // it's equivalent to a 2-rank electronics penalty, 1-rank tailoring
    if( has_trait("HYPEROPIC") && !is_wearing("glasses_reading") &&
        !is_wearing("glasses_bifocal") && !has_effect("contacts") ) {
        int main_rank_penalty = 0;
        if (making->skill_used == skill_id("electronics")) {
            main_rank_penalty = 2;
        } else if (making->skill_used == skill_id("tailor")) {
            main_rank_penalty = 1;
        }
        skill_dice -= main_rank_penalty * 4;
    }

    // It's tough to craft with paws.  Fortunately it's just a matter of grip and fine-motor,
    // not inability to see what you're doing
    if (has_trait("PAWS") || has_trait("PAWS_LARGE")) {
        int paws_rank_penalty = 0;
        if (has_trait("PAWS_LARGE")) {
            paws_rank_penalty += 1;
        }
        if ( making->skill_used == skill_id("electronics")
          || making->skill_used == skill_id("tailor")
          || making->skill_used == skill_id("mechanics")) {
            paws_rank_penalty += 1;
        }
        skill_dice -= paws_rank_penalty * 4;
    }

    // Sides on dice is 16 plus your current intelligence
    int skill_sides = 16 + int_cur;

    int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
    int diff_sides = 24; // 16 + 8 (default intelligence)

    int skill_roll = dice(skill_dice, skill_sides);
    int diff_roll  = dice(diff_dice,  diff_sides);

    if (making->skill_used) {
        //normalize experience gain to crafting time, giving a bonus for longer crafting
        practice( making->skill_used, (int)( ( making->difficulty * 15 + 10 ) * ( 1 + making->batch_time( batch_size ) / 30000.0 ) ),
                    (int)making->difficulty * 1.25 );

        //NPCs assisting or watching should gain experience...
        for( auto &elem : g->active_npc ) {
            if (rl_dist( elem->pos(), g->u.pos() ) < PICKUP_RANGE && elem->is_friend()){
                //If the NPC can understand what you are doing, they gain more exp
                if (elem->skillLevel(making->skill_used) >= making->difficulty){
                    elem->practice( making->skill_used, (int)( ( making->difficulty * 15 + 10 ) * ( 1 + making->batch_time( batch_size ) / 30000.0 ) * .50), (int)making->difficulty * 1.25 );
                    if (batch_size > 1)
                        add_msg(m_info, _("%s assists with crafting..."), elem->name.c_str());
                    if (batch_size == 1)
                        add_msg(m_info, _("%s could assist you with a batch..."), elem->name.c_str());
                //NPCs around you understand the skill used better
                } else {
                    elem->practice( making->skill_used, (int)( ( making->difficulty * 15 + 10 ) * ( 1 + making->batch_time( batch_size ) / 30000.0 ) * .15), (int)making->difficulty * 1.25 );
                    add_msg(m_info, _("%s watches you craft..."), elem->name.c_str());
                }
            }
        }

    }

    // Messed up badly; waste some components.
    if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
        add_msg(m_bad, _("You fail to make the %s, and waste some materials."),
                item::nname(making->result).c_str());
        if( last_craft.has_cached_selections() ) {
            last_craft.consume_components();
        } else {
            for( const auto &it : making->requirements.components ) {
                consume_items( it, batch_size );
            }

            for( const auto &it : making->requirements.tools ) {
                consume_tools( it, batch_size );
            }
        }
        activity.type = ACT_NULL;
        return;
        // Messed up slightly; no components wasted.
    } else if (diff_roll > skill_roll) {
        add_msg(m_neutral, _("You fail to make the %s, but don't waste any materials."),
                item::nname(making->result).c_str());
        //this method would only have been called from a place that nulls activity.type,
        //so it appears that it's safe to NOT null that variable here.
        //rationale: this allows certain contexts (e.g. ACT_LONGCRAFT) to distinguish major and minor failures
        return;
    }

    // If we're here, the craft was a success!
    // Use up the components and tools
    std::list<item> used;
    if( last_craft.has_cached_selections() ) {
        used = last_craft.consume_components();
    } else {
        for( const auto &it : making->requirements.components ) {
            std::list<item> tmp = consume_items( it, batch_size );
            used.splice(used.end(), tmp);
        }
        for( const auto &it : making->requirements.tools ) {
            consume_tools( it, batch_size );
        }
    }

    // Set up the new item, and assign an inventory letter if available
    std::vector<item> newits = making->create_results(batch_size);
    bool first = true;
    float used_age_tally = 0;
    int used_age_count = 0;
    size_t newit_counter = 0;
    for(item &newit : newits) {
        // messages, learning of recipe, food spoilage calc only once
        if (first) {
            first = false;
            if( knows_recipe(making) ) {
                add_msg(_("You craft %s from memory."), newit.type_name( 1 ).c_str());
            } else {
                add_msg(_("You craft %s using a book as a reference."), newit.type_name( 1 ).c_str());
                // If we made it, but we don't know it,
                // we're making it from a book and have a chance to learn it.
                // Base expected time to learn is 1000*(difficulty^4)/skill/int moves.
                // This means time to learn is greatly decreased with higher skill level,
                // but also keeps going up as difficulty goes up.
                // Worst case is lvl 10, which will typically take
                // 10^4/10 (1,000) minutes, or about 16 hours of crafting it to learn.
                int difficulty = has_recipe( making, crafting_inventory() );
                if( x_in_y( making->time, (1000 * 8 *
                            ( difficulty * difficulty * difficulty * difficulty ) ) /
                            ( std::max( get_skill_level( making->skill_used ).level(), 1 ) * std::max( get_int(), 1 ) ) ) ) {
                    learn_recipe( (recipe *)making );
                    add_msg(m_good, _("You memorized the recipe for %s!"),
                            newit.type_name( 1 ).c_str());
                }
            }

            for( auto &elem : used ) {
                if( elem.goes_bad() ) {
                    used_age_tally += elem.get_relative_rot();
                    ++used_age_count;
                }
            }
        }

        // Don't store components for things made by charges,
        // don't store components for things that can't be uncrafted.
        if( making->reversible && !newit.count_by_charges() ) {
            // Setting this for items counted by charges gives only problems:
            // those items are automatically merged everywhere (map/vehicle/inventory),
            // which would either loose this information or merge it somehow.
            set_components( newit.components, used, batch_size, newit_counter );
            newit_counter++;
        }
        finalize_crafted_item( newit, used_age_tally, used_age_count );
        set_item_inventory(newit);
    }

    if (making->has_byproducts()) {
        std::vector<item> bps = making->create_byproducts(batch_size);
        for(auto &bp : bps) {
            finalize_crafted_item( bp, used_age_tally, used_age_count );
            set_item_inventory(bp);
        }
    }

    inv.restack(this);
}

void set_item_spoilage(item &newit, float used_age_tally, int used_age_count)
{
    newit.set_relative_rot( used_age_tally / used_age_count );
}

void set_item_food(item &newit)
{
    int bday_tmp = newit.bday % 3600; // fuzzy birthday for stacking reasons
    newit.bday = int(newit.bday) + 3600 - bday_tmp;
    if (newit.has_flag("EATEN_HOT")) { // hot foods generated
        newit.item_tags.insert("HOT");
        newit.item_counter = 600;
        newit.active = true;
    }
}

void finalize_crafted_item( item &newit, float used_age_tally, int used_age_count )
{
    if( newit.is_food() ) {
        set_item_food( newit );
    }
    if( used_age_count > 0 && newit.goes_bad() ) {
        set_item_spoilage( newit, used_age_tally, used_age_count );
    }
}

void set_item_inventory(item &newit)
{
    if (newit.made_of(LIQUID)) {
        while(!g->handle_liquid(newit, false, false, nullptr, nullptr, 1)) {
            ;
        }
    } else {
        g->u.inv.assign_empty_invlet( newit );
        // We might not have space for the item
        if (!g->u.can_pickVolume(newit.volume())) { //Accounts for result_mult
            add_msg(_("There's no room in your inventory for the %s, so you drop it."),
                    newit.tname().c_str());
            g->m.add_item_or_charges(g->u.posx(), g->u.posy(), newit);
        } else if (!g->u.can_pickWeight(newit.weight(), !OPTIONS["DANGEROUS_PICKUPS"])) {
            add_msg(_("The %s is too heavy to carry, so you drop it."),
                    newit.tname().c_str());
            g->m.add_item_or_charges(g->u.posx(), g->u.posy(), newit);
        } else {
            newit = g->u.i_add(newit);
            add_msg(m_info, "%c - %s", newit.invlet == 0 ? ' ' : newit.invlet, newit.tname().c_str());
        }
    }
}

/* selection of component if a recipe requirement has multiple options (e.g. 'duct tap' or 'welder') */
comp_selection<item_comp> player::select_item_component(const std::vector<item_comp> &components, int batch, inventory &map_inv, bool can_cancel)
{
    std::vector<item_comp> player_has;
    std::vector<item_comp> map_has;
    std::vector<item_comp> mixed;

    comp_selection<item_comp> selected;

    for( const auto &component : components ) {
        itype_id type = component.type;
        int count = ( component.count > 0 ) ? component.count * batch : abs( component.count );
        bool pl = false, mp = false;

        if (item::count_by_charges(type) && count > 0) {
            if (has_charges(type, count)) {
                player_has.push_back( component );
                pl = true;
            }
            if (map_inv.has_charges(type, count)) {
                map_has.push_back( component );
                mp = true;
            }
            if (!pl && !mp && charges_of(type) + map_inv.charges_of(type) >= count) {
                mixed.push_back( component );
            }
        } else { // Counting by units, not charges

            if (has_amount(type, count)) {
                player_has.push_back( component );
                pl = true;
            }
            if (map_inv.has_components(type, count)) {
                map_has.push_back( component );
                mp = true;
            }
            if (!pl && !mp && amount_of(type) + map_inv.amount_of(type) >= count) {
                mixed.push_back( component );
            }

        }
    }

    /* select 1 component to use */
    if (player_has.size() + map_has.size() + mixed.size() == 1) { // Only 1 choice
        if (player_has.size() == 1) {
            selected.use_from = use_from_player;
            selected.comp = player_has[0];
        } else if (map_has.size() == 1) {
            selected.use_from = use_from_map;
            selected.comp = map_has[0];
        } else {
            selected.use_from = use_from_both;
            selected.comp = mixed[0];
        }
    } else { // Let the player pick which component they want to use
        std::vector<std::string> options; // List for the menu_vec below
        // Populate options with the names of the items
        for( auto &map_ha : map_has ) {
            std::string tmpStr = item::nname( map_ha.type ) + _( " (nearby)" );
            options.push_back(tmpStr);
        }
        for( auto &player_ha : player_has ) {
            options.push_back( item::nname( player_ha.type ) );
        }
        for( auto &elem : mixed ) {
            std::string tmpStr = item::nname( elem.type ) + _( " (on person & nearby)" );
            options.push_back(tmpStr);
        }

        // unlike with tools, it's a bad thing if there aren't any components available
        if (options.empty()) {
            if (!(has_trait("WEB_ROPE"))) {
                debugmsg("Attempted a recipe with no available components!");
            }
            selected.use_from = cancel;
            return selected;
        }

        // Get the selection via a menu popup
        int selection = menu_vec(can_cancel, _("Use which component?"), options) - 1;
        if(selection == UIMENU_INVALID) {
            selected.use_from = cancel;
            return selected;
        }

        size_t uselection = (size_t) selection;
        if (uselection < map_has.size()) {
            selected.use_from = usage::use_from_map;
            selected.comp = map_has[uselection];
        } else if (uselection < map_has.size() + player_has.size()) {
            uselection -= map_has.size();
            selected.use_from = usage::use_from_player;
            selected.comp = player_has[uselection];
        } else {
            uselection -= map_has.size() + player_has.size();
            selected.use_from = usage::use_from_both;
            selected.comp = mixed[uselection];
        }
    }

    return selected;
}

std::list<item> player::consume_items(const comp_selection<item_comp> &is, int batch) {
    std::list<item> ret;

    if (has_trait("DEBUG_HS")) {
        return ret;
    }

    item_comp selected_comp = is.comp;

    const tripoint &loc = pos3();
    const bool by_charges = (item::count_by_charges( selected_comp.type ) && selected_comp.count > 0);
    // Count given to use_amount/use_charges, changed by those functions!
    long real_count = (selected_comp.count > 0) ? selected_comp.count * batch : abs(selected_comp.count);
    const bool in_container = (selected_comp.count < 0);
    // First try to get everything from the map, than (remaining amount) from player
    if (is.use_from & use_from_map) {
        if (by_charges) {
            std::list<item> tmp = g->m.use_charges(loc, PICKUP_RANGE, selected_comp.type, real_count);
            ret.splice(ret.end(), tmp);
        } else {
            std::list<item> tmp = g->m.use_amount(loc, PICKUP_RANGE, selected_comp.type,
                                                  real_count, in_container);
            remove_ammo(tmp, *this);
            ret.splice(ret.end(), tmp);
        }
    }
    if (is.use_from & use_from_player) {
        if (by_charges) {
            std::list<item> tmp = use_charges(selected_comp.type, real_count);
            ret.splice(ret.end(), tmp);
        } else {
            std::list<item> tmp = use_amount(selected_comp.type, real_count, in_container);
            remove_ammo(tmp, *this);
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
    lastconsumed = selected_comp.type;
    return ret;
}

/* This call is in-efficient when doing it for multiple items with the same map inventory.
In that case, consider using select_item_component with 1 pre-created map inventory, and then passing the results
to consume_items */
std::list<item> player::consume_items( const std::vector<item_comp> &components, int batch ) {
    inventory map_inv;
    map_inv.form_from_map(pos3(), PICKUP_RANGE);
    return consume_items( select_item_component( components, batch, map_inv ), batch );
}

comp_selection<tool_comp>
player::select_tool_component( const std::vector<tool_comp> &tools, int batch, inventory &map_inv,
                               const std::string &hotkeys, bool can_cancel ) {

    comp_selection<tool_comp> selected;

    bool found_nocharge = false;
    std::vector<tool_comp> player_has;
    std::vector<tool_comp> map_has;
    // Use charges of any tools that require charges used
    for (auto it = tools.begin(); it != tools.end() && !found_nocharge; ++it) {
        itype_id type = it->type;
        if (it->count > 0) {
            long count = it->count * batch;
            if (has_charges(type, count)) {
                player_has.push_back(*it);
            }
            if (map_inv.has_charges(type, count)) {
                map_has.push_back(*it);
            }
        } else if (has_amount(type, 1) || map_inv.has_tools(type, 1)) {
            found_nocharge = true;
        }
    }
    if (found_nocharge) {
        selected.use_from = use_from_none;
        return selected;    // Default to using a tool that doesn't require charges
    }

    if (player_has.size() + map_has.size() == 1) {
        if (map_has.empty()) {
            selected.use_from = use_from_player;
            selected.comp = player_has[0];
        } else {
            selected.use_from = use_from_map;
            selected.comp = map_has[0];
        }
    } else { // Variety of options, list them and pick one
        // Populate the list
        std::vector<std::string> options;
        for( auto &map_ha : map_has ) {
            std::string tmpStr = item::nname( map_ha.type ) + _( " (nearby)" );
            options.push_back(tmpStr);
        }
        for( auto &player_ha : player_has ) {
            options.push_back( item::nname( player_ha.type ) );
        }

        if (options.empty()) { // This SHOULD only happen if cooking with a fire,
            selected.use_from = use_from_none;
            return selected;    // and the fire goes out.
        }

        // Get selection via a popup menu
        int selection = menu_vec(can_cancel, _("Use which tool?"), options, hotkeys) - 1;
        if(selection == UIMENU_INVALID) {
            selected.use_from = cancel;
            return selected;
        }

        size_t uselection = (size_t) selection;
        if (uselection < map_has.size()) {
            selected.use_from = use_from_map;
            selected.comp = map_has[uselection];
        } else {
            uselection -= map_has.size();
            selected.use_from = use_from_player;
            selected.comp = player_has[uselection];
        }
    }

    return selected;
}

/* we use this if we selected the tool earlier */
void player::consume_tools(const comp_selection<tool_comp> &tool, int batch) {
    if (has_trait("DEBUG_HS")) {
        return;
    }

    if (tool.use_from & use_from_player) {
        use_charges(tool.comp.type, tool.comp.count * batch);
    }
    if (tool.use_from & use_from_map) {
        long quantity = tool.comp.count * batch;
        g->m.use_charges(pos3(), PICKUP_RANGE, tool.comp.type, quantity);
    }

    // else, use_from_none (or cancel), so we don't use up any tools;
}

/* This call is in-efficient when doing it for multiple items with the same map inventory.
In that case, consider using select_tool_component with 1 pre-created map inventory, and then passing the results
to consume_tools */
void player::consume_tools( const std::vector<tool_comp> &tools, int batch, const std::string &hotkeys )
{
    inventory map_inv;
    map_inv.form_from_map(pos3(), PICKUP_RANGE);
    consume_tools( select_tool_component( tools, batch, map_inv, hotkeys ), batch );
}

const recipe *get_disassemble_recipe(const itype_id &type)
{
    for( auto cur_recipe : recipe_dict ) {

        if (type == cur_recipe->result && cur_recipe->reversible) {
            return cur_recipe;
        }
    }
    // no matching disassemble recipe found.
    return NULL;
}

bool player::can_disassemble( const item &dis_item, const inventory &crafting_inv,
                              const bool print_msg ) const
{
    if( dis_item.is_book() ) {
        return true;
    }

    for( auto cur_recipe : recipe_dict ) {
        if( dis_item.type->id == cur_recipe->result && cur_recipe->reversible ) {
            return can_disassemble( dis_item, cur_recipe, crafting_inv, print_msg );
        }
    }

    return false;
}

bool player::can_disassemble( const item &dis_item, const recipe *cur_recipe,
                              const inventory &crafting_inv, bool print_msg ) const
{
    if (dis_item.count_by_charges()) {
        // Create a new item to get the default charges
        const item tmp = cur_recipe->create_result();
        if (dis_item.charges < tmp.charges) {
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
    for( const auto &it : cur_recipe->requirements.tools ) {
        bool have_this_tool = false;
        for( const auto &tool : it ) {
            itype_id type = tool.type;
            int req = tool.count; // -1 => 1

            if ((req <= 0 && crafting_inv.has_tools (type, 1)) ||
                // No welding, no goggles needed.
                (req <= 0 && type == "goggles_welding") ||
                (req <= 0 && type == "crucible" &&
                 cur_recipe->result != "anvil") ||
                // No mold needed for disassembly.
                (req <= 0 && type == "mold_plastic") ||
                (req >  0 && crafting_inv.has_charges(type, req))) {
                have_this_tool = true;
            }

            // If crafting recipe required a welder,
            // disassembly requires a hacksaw or super toolkit.
            if (type == "welder") {
                have_this_tool = crafting_inv.has_items_with_quality( "SAW_M_FINE", 1, 1 );
            }

            if (type == "sewing_kit") {
                have_this_tool = crafting_inv.has_items_with_quality( "CUT", 1, 1 );
            }

            if( have_this_tool ) {
                break;
            }
        }
        if (!have_this_tool) {
            have_all_tools = false;
            if (print_msg) {
                int req = it[0].count;
                if (it[0].type == "welder") {
                    add_msg(m_info, _("You need an item with %s of 1 or more to disassemble this."), quality::get_name( "SAW_M_FINE" ).c_str() );
                } else {
                    if (req <= 0) {
                        add_msg(m_info, _("You need a %s to disassemble this."),
                                item::nname(it[0].type).c_str());
                    } else {
                        add_msg(m_info, ngettext("You need a %s with %d charge to disassemble this.",
                                                 "You need a %s with %d charges to disassemble this.",
                                                 req),
                                item::nname(it[0].type).c_str(), req);
                    }
                }
            }
        }
    }
    // all tools present, so assign the activity
    return have_all_tools;
}

bool query_dissamble(const item &dis_item)
{
    if( OPTIONS["QUERY_DISASSEMBLE"] ) {
        return query_yn( _("Really disassemble the %s?"), dis_item.tname().c_str() );
    }
    return true;
}

void player::disassemble(int dis_pos)
{
    if (dis_pos == INT_MAX) {
        dis_pos = g->inv(_("Disassemble item:"));
    }
    item &dis_item = i_at(dis_pos);
    if (!has_item(dis_pos)) {
        add_msg(m_info, _("You don't have that item!"), dis_pos);
        return;
    }
    disassemble(dis_item, dis_pos, false);
}

void player::disassemble(item &dis_item, int dis_pos, bool ground)
{
    const recipe *cur_recipe = get_disassemble_recipe( dis_item.type->id );

    //no disassembly without proper light
    if (fine_detail_vision_mod() > 4) {
        add_msg(m_info, _("You can't see to craft!"));
        return;
    }

    //checks to see if you're disassembling rotten food, and will stop you if true
    if( (dis_item.is_food() && dis_item.goes_bad()) ||
        (dis_item.is_food_container() && dis_item.contents[0].goes_bad()) ) {
        dis_item.calc_rot( global_square_location() );
        if( dis_item.rotten() ||
            (dis_item.is_food_container() && dis_item.contents[0].rotten())) {
            add_msg(m_info, _("It's rotten, I'm not taking that apart."));
            return;
        }
    }

    if (cur_recipe != NULL) {
        const inventory &crafting_inv = crafting_inventory();
        if (can_disassemble(dis_item, cur_recipe, crafting_inv, true)) {
            if( !query_dissamble( dis_item ) ) {
                return;
            }
            assign_activity(ACT_DISASSEMBLE, cur_recipe->time, cur_recipe->id);
            activity.values.push_back(dis_pos);
            if( ground ) {
                activity.values.push_back(1);
            }
        }
        return; // recipe exists, but no tools, so do not start disassembly
    }
    //if we're trying to disassemble a book or magazine
    if( dis_item.is_book() ) {
        if (OPTIONS["QUERY_DISASSEMBLE"] &&
            !(query_yn(_("Do you want to tear %s into pages?"), dis_item.tname().c_str()))) {
            return;
        } else {
            //twice the volume then multiplied by 10 (a book with volume 3 will give 60 pages)
            int num_pages = (dis_item.volume() * 2) * 10;
            g->m.spawn_item(posx(), posy(), "paper", 0, num_pages);
            i_rem(dis_pos);
        }
        return;
    }

    // no recipe exists, or the item cannot be disassembled
    add_msg(m_info, _("This item cannot be disassembled!"));
}

// Find out which of the alternative components had been used to craft the item.
item_comp find_component( const std::vector<item_comp> &altercomps, const item &dis_item )
{
    for( auto & comp : altercomps ) {
        for( auto & elem : dis_item.components ) {
            if( elem.typeId() == comp.type ) {
                return comp;
            }
        }
    }
    // Default is the one listed first in json.
    return altercomps.front();
}

void player::complete_disassemble()
{
    // which recipe was it?
    const int item_pos = activity.values[0];
    const bool from_ground = activity.values.size() > 1 && activity.values[1] == 1;
    const recipe *dis = recipe_by_index(activity.index); // Which recipe is it?
    if( dis == nullptr ) {
        debugmsg( "no recipe with id %d found", activity.index );
        activity.type = ACT_NULL;
        return;
    }
    item *org_item;
    auto items_on_ground = g->m.i_at(posx(), posy());
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
        org_item = &i_at(item_pos);
    }
    // Make a copy to keep its data (damage/components) even after it
    // has been removed.
    item dis_item = *org_item;

    float component_success_chance = std::min(std::pow(0.8f, dis_item.damage), 1.0);

    int veh_part = -1;
    vehicle *veh = g->m.veh_at( pos(), veh_part );
    if( veh != nullptr ) {
        veh_part = veh->part_with_feature(veh_part, "CARGO");
    }

    add_msg(_("You disassemble the %s into its components."), dis_item.tname().c_str());
    // remove any batteries or ammo first
    remove_ammo( &dis_item, *this );

    if (dis_item.count_by_charges()) {
        // remove the charges that one would get from crafting it
        org_item->charges -= dis->create_result().charges;
    }
    // remove the item, except when it's counted by charges and still has some
    if (!org_item->count_by_charges() || org_item->charges <= 0) {
        if (from_ground) {
            g->m.i_rem( posx(), posy(), item_pos );
        } else {
            i_rem(item_pos);
        }
    }

    // consume tool charges
    for (const auto &it : dis->requirements.tools) {
        consume_tools(it);
    }

    // add the components to the map
    // Player skills should determine how many components are returned

    int skill_dice = 2 + skillLevel(dis->skill_used) * 3;
    skill_dice += skillLevel(dis->skill_used);

    // Sides on dice is 16 plus your current intelligence
    int skill_sides = 16 + int_cur;

    int diff_dice = dis->difficulty;
    int diff_sides = 24; // 16 + 8 (default intelligence)

    // disassembly only nets a bit of practice
    if (dis->skill_used) {
        practice( dis->skill_used, (dis->difficulty) * 2, dis->difficulty );
    }

    for (const auto &altercomps : dis->requirements.components) {
        const item_comp comp = find_component( altercomps, dis_item );
        int compcount = comp.count;
        item newit( comp.type, calendar::turn );
        if( !comp.recoverable || newit.has_flag( "UNRECOVERABLE" ) ) {
            continue;
        }
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
                //~ %1s - material, %2$s - disassembled item
                add_msg(m_bad, _("You fail to recover %1$s from the %2$s."), newit.tname().c_str(),
                        dis_item.tname().c_str());
                continue;
            }
            // Use item from components list, or (if not contained)
            // use newit, the default constructed.
            item act_item = newit;
            for(item::t_item_vector::iterator a = dis_item.components.begin(); a != dis_item.components.end();
                ++a) {
                if (a->type == newit.type) {
                    act_item = *a;
                    dis_item.components.erase(a);
                    break;
                }
            }
            if (act_item.made_of(LIQUID)) {
                while (!g->handle_liquid(act_item, false, false)) {
                    // Try again, maybe use another container.
                }
            } else if (veh != NULL && veh->add_item(veh_part, act_item)) {
                // add_item did put the items in the vehicle, nothing further to be done
            } else {
                g->m.add_item_or_charges(posx(), posy(), act_item);
            }
        }
    }

    if (dis->learn_by_disassembly >= 0 && !knows_recipe(dis)) {
        if( !dis->skill_used || dis->learn_by_disassembly <= skillLevel(dis->skill_used)) {
            if (one_in(4)) {
                learn_recipe((recipe *)dis);
                add_msg(m_good, _("You learned a recipe from disassembling it!"));
            } else {
                add_msg(m_info, _("You might be able to learn a recipe if you disassemble another."));
            }
        } else {
            add_msg(m_info, _("If you had better skills, you might learn a recipe next time."));
        }
    }
}

const recipe *recipe_by_index( int index )
{
    return recipe_dict[index];
}

const recipe *recipe_by_name( const std::string &name)
{
    return recipe_dict[name];
}

void check_recipe_definitions()
{
    for( auto &elem : recipe_dict ) {
        const recipe &r = *elem;
        const std::string display_name = std::string("recipe ") + r.ident;
        r.requirements.check_consistency(display_name);
        if (!item::type_is_defined(r.result)) {
            debugmsg("result %s in recipe %s is not a valid item template", r.result.c_str(), r.ident.c_str());
        }
        if( r.skill_used && !r.skill_used.is_valid() ) {
            debugmsg("recipe %s uses invalid skill %s", r.ident.c_str(), r.skill_used.c_str());
        }
        for( auto &e : r.required_skills ) {
            if( e.first && !e.first.is_valid() ) {
                debugmsg("recipe %s uses invalid required skill %s", r.ident.c_str(), e.first.c_str());
            }
        }
    }
}

void remove_ammo(std::list<item> &dis_items, player &p)
{
    for( auto &dis_item : dis_items ) {
        remove_ammo( &dis_item, p );
    }
}

void remove_ammo(item *dis_item, player &p)
{
    auto &contents = dis_item->contents;
    const bool is_gun = dis_item->is_gun();
    for( size_t i = 0; i < contents.size(); ) {
        // Gun with gunmods, remove gunmods, remove their ammo
        if( is_gun ) {
            // integrated mods stay in the gun, they are part of the item type itself.
            if( contents[i].has_flag( "IRREMOVABLE" ) ) {
                remove_ammo( &contents[i], p );
                i++;
            } else {
                p.remove_gunmod( dis_item, i );
            }
            continue;
        }
        item tmp = contents[i];
        contents.erase( contents.begin() + i );
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
    if( dis_item->is_gun() && dis_item->has_curammo() && dis_item->ammo_type() != "NULL" ) {
        item ammodrop( dis_item->get_curammo_id(), calendar::turn );
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
        if( dis_item->ammo_type() == "plutonium" ) {
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

std::string recipe::required_skills_string() const
{
    std::ostringstream skills_as_stream;
    if(!required_skills.empty()) {
        for( auto iter = required_skills.begin(); iter != required_skills.end(); ) {
            skills_as_stream << iter->first.obj().name() << "(" << iter->second << ")";
            ++iter;
            if(iter != required_skills.end()) {
                skills_as_stream << ", ";
            }
        }
    } else {
        skills_as_stream << _("N/A");
    }
    return skills_as_stream.str();
}
