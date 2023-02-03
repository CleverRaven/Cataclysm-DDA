#include "crafting_gui.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iterator>
#include <map>
#include <new>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "color.h"
#include "crafting.h"
#include "cursesdef.h"
#include "debug.h"
#include "display.h"
#include "flag.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "localized_comparator.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "popup.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"

static const std::string flag_BLIND_EASY( "BLIND_EASY" );
static const std::string flag_BLIND_HARD( "BLIND_HARD" );

class npc;

class recipe_result_info_cache;

enum TAB_MODE {
    NORMAL,
    FILTERED,
    BATCH
};

enum CRAFTING_SPEED_STATE {
    TOO_DARK_TO_CRAFT,
    TOO_SLOW_TO_CRAFT,
    SLOW_BUT_CRAFTABLE,
    FAST_CRAFTING,
    NORMAL_CRAFTING
};

static const std::map<const CRAFTING_SPEED_STATE, translation> craft_speed_reason_strings = {
    {TOO_DARK_TO_CRAFT, to_translation( "too dark to craft" )},
    {TOO_SLOW_TO_CRAFT, to_translation( "unable to craft" )},
    {SLOW_BUT_CRAFTABLE, to_translation( "crafting is slow %d%%" )},
    {FAST_CRAFTING, to_translation( "crafting is fast %d%%" )},
    {NORMAL_CRAFTING, to_translation( "craftable" )}
};

// TODO: Convert these globals to handling categories via generic_factory?
static std::vector<std::string> craft_cat_list;
static std::map<std::string, std::vector<std::string> > craft_subcat_list;

static bool query_is_yes( const std::string &query );
static int craft_info_width( int window_width );
static void draw_hidden_amount( const catacurses::window &w, int amount, int num_recipe );
static void draw_can_craft_indicator( const catacurses::window &w, const recipe &rec );
static std::map<size_t, inclusive_rectangle<point>> draw_recipe_tabs( const catacurses::window &w,
        const tab_list &tab, TAB_MODE mode,
        bool filtered_unread, std::map<std::string, bool> &unread );
static std::map<size_t, inclusive_rectangle<point>> draw_recipe_subtabs(
            const catacurses::window &w, const std::string &tab,
            size_t subtab,
            const recipe_subset &available_recipes, TAB_MODE mode,
            std::map<std::string, bool> &unread );

static std::string peek_related_recipe( const recipe *current, const recipe_subset &available );
static int related_menu_fill( uilist &rmenu,
                              const std::vector<std::pair<itype_id, std::string>> &related_recipes,
                              const recipe_subset &available );

static std::string get_cat_unprefixed( const std::string &prefixed_name )
{
    return prefixed_name.substr( 3, prefixed_name.size() - 3 );
}

void load_recipe_category( const JsonObject &jsobj )
{
    const std::string category = jsobj.get_string( "id" );
    const bool is_hidden = jsobj.get_bool( "is_hidden", false );

    if( category.find( "CC_" ) != 0 ) {
        jsobj.throw_error( "Crafting category id has to be prefixed with 'CC_'" );
    }

    if( !is_hidden &&
        std::find( craft_cat_list.begin(), craft_cat_list.end(), category ) == craft_cat_list.end() ) {
        craft_cat_list.push_back( category );
    }

    const std::string cat_name = get_cat_unprefixed( category );

    craft_subcat_list[category].clear();
    for( const std::string subcat_id : jsobj.get_array( "recipe_subcategories" ) ) {
        if( subcat_id.find( "CSC_" + cat_name + "_" ) != 0 && subcat_id != "CSC_ALL" ) {
            jsobj.throw_error( "Crafting sub-category id has to be prefixed with CSC_<category_name>_" );
        }
        if( find( craft_subcat_list[category].begin(), craft_subcat_list[category].end(),
                  subcat_id ) == craft_subcat_list[category].end() ) {
            craft_subcat_list[category].push_back( subcat_id );
        }
    }
}

static std::string get_subcat_unprefixed( const std::string &cat, const std::string &prefixed_name )
{
    std::string prefix = "CSC_" + get_cat_unprefixed( cat ) + "_";

    if( prefixed_name.find( prefix ) == 0 ) {
        return prefixed_name.substr( prefix.size(), prefixed_name.size() - prefix.size() );
    }

    return prefixed_name == "CSC_ALL" ? translate_marker( "ALL" ) : translate_marker( "NONCRAFT" );
}

void reset_recipe_categories()
{
    craft_cat_list.clear();
    craft_subcat_list.clear();
}

static bool cannot_gain_skill_or_prof( const Character &player, const recipe &recp )
{
    if( recp.skill_used && player.get_skill_level( recp.skill_used ) <= recp.get_skill_cap() ) {
        return false;
    }
    for( const proficiency_id &prof : recp.used_proficiencies() ) {
        if( !player.has_proficiency( prof ) ) {
            return false;
        }
    }
    return true;
}

namespace
{
struct availability {
        explicit availability( const recipe *r, int batch_size = 1 ) {
            rec = r;
            Character &player = get_player_character();
            const inventory &inv = player.crafting_inventory();
            auto all_items_filter = r->get_component_filter( recipe_filter_flags::none );
            auto no_rotten_filter = r->get_component_filter( recipe_filter_flags::no_rotten );
            auto no_favorite_filter = r->get_component_filter( recipe_filter_flags::no_favorite );
            const deduped_requirement_data &req = r->deduped_requirements();
            has_all_skills = r->skill_used.is_null() ||
                             player.get_skill_level( r->skill_used ) >= r->get_difficulty( player );
            has_proficiencies = r->character_has_required_proficiencies( player );
            if( r->is_nested() ) {
                can_craft = check_can_craft_nested( *r );
            } else {
                can_craft = ( !r->is_practice() || has_all_skills ) && has_proficiencies &&
                            req.can_make_with_inventory( inv, all_items_filter, batch_size, craft_flags::start_only );
            }
            would_use_rotten = !req.can_make_with_inventory( inv, no_rotten_filter, batch_size,
                               craft_flags::start_only );
            would_use_favorite = !req.can_make_with_inventory( inv, no_favorite_filter, batch_size,
                                 craft_flags::start_only );
            would_not_benefit = r->is_practice() && cannot_gain_skill_or_prof( player, *r );
            is_nested_category = r->is_nested();
            const requirement_data &simple_req = r->simple_requirements();
            apparently_craftable = ( !r->is_practice() || has_all_skills ) && has_proficiencies &&
                                   simple_req.can_make_with_inventory( inv, all_items_filter, batch_size, craft_flags::start_only );
            for( const std::pair<const skill_id, int> &e : r->required_skills ) {
                if( player.get_skill_level( e.first ) < e.second ) {
                    has_all_skills = false;
                    break;
                }
            }
        }
        bool can_craft;
        bool would_use_rotten;
        bool would_use_favorite;
        bool would_not_benefit;
        bool apparently_craftable;
        bool has_proficiencies;
        bool has_all_skills;
        bool is_nested_category;
    private:
        const recipe *rec;
        mutable float proficiency_time_maluses = -1.0f;
        mutable float proficiency_failure_maluses = -1.0f;
    public:
        float get_proficiency_time_maluses() const {
            if( proficiency_time_maluses < 0 ) {
                Character &player = get_player_character();
                proficiency_time_maluses = rec->proficiency_time_maluses( player );
            }

            return proficiency_time_maluses;
        }
        float get_proficiency_failure_maluses() const {
            if( proficiency_failure_maluses < 0 ) {
                Character &player = get_player_character();
                proficiency_failure_maluses = rec->proficiency_failure_maluses( player );
            }

            return proficiency_failure_maluses;
        }

        nc_color selected_color() const {
            if( !can_craft && is_nested_category ) {
                return h_blue;
            } else if( !can_craft ) {
                return h_dark_gray;
            } else if( is_nested_category ) {
                return h_light_blue;
            }  else if( would_use_rotten || would_not_benefit ) {
                return has_all_skills ? h_brown : h_red;
            } else if( would_use_favorite ) {
                return has_all_skills ? h_pink : h_red;
            } else {
                return has_all_skills ? h_white : h_yellow;
            }
        }

        nc_color color( bool ignore_missing_skills = false ) const {
            if( !can_craft && is_nested_category ) {
                return c_blue;
            } else if( !can_craft ) {
                return c_dark_gray;
            } else if( is_nested_category ) {
                return c_light_blue;
            } else if( would_use_rotten || would_not_benefit ) {
                return has_all_skills || ignore_missing_skills ? c_brown : c_red;
            } else if( would_use_favorite ) {
                return has_all_skills ? c_pink : c_red;
            } else {
                return has_all_skills || ignore_missing_skills ? c_white : c_yellow;
            }
        }

        static bool check_can_craft_nested( const recipe &r ) {
            // recursively check if you can craft anything in the nest
            bool can_craft = false;
            for( const recipe_id &nested_r : r.nested_category_data ) {
                // if nested recur
                availability av = availability( &nested_r.obj() );
                can_craft |= av.can_craft;

                // early return if we can craft anything
                if( can_craft ) {
                    break;
                }
            }

            return can_craft;
        }
};
} // namespace

static std::vector<std::string> recipe_info(
    const recipe &recp,
    const availability &avail,
    Character &guy,
    const std::string &qry_comps,
    const int batch_size,
    const int fold_width,
    const nc_color &color )
{
    std::ostringstream oss;

    oss << string_format( _( "Primary skill: %s\n" ), recp.primary_skill_string( guy ) );

    if( !recp.required_skills.empty() ) {
        oss << string_format( _( "Other skills: %s\n" ), recp.required_skills_string( guy ) );
    }

    const std::string req_profs = recp.required_proficiencies_string( &guy );
    if( !req_profs.empty() ) {
        oss << string_format( _( "Proficiencies Required: %s\n" ), req_profs );
    }
    const std::string used_profs = recp.used_proficiencies_string( &guy );
    if( !used_profs.empty() ) {
        oss << string_format( _( "Proficiencies Used: %s\n" ), used_profs );
    }
    const std::string missing_profs = recp.missing_proficiencies_string( &guy );
    if( !missing_profs.empty() ) {
        oss << string_format( _( "Proficiencies Missing: %s\n" ), missing_profs );
    }

    if( !recp.is_nested() ) {
        const int expected_turns = guy.expected_time_to_craft( recp, batch_size )
                                   / to_moves<int>( 1_turns );
        oss << string_format( _( "Time to complete: <color_cyan>%s</color>\n" ),
                              to_string( time_duration::from_turns( expected_turns ) ) );

    }

    const std::string batch_savings = recp.batch_savings_string();
    if( !batch_savings.empty() ) {
        oss << string_format( _( "Batch time savings: <color_cyan>%s</color>\n" ), batch_savings );
    }

    oss << string_format( _( "Activity level: <color_cyan>%s</color>\n" ),
                          display::activity_level_str( recp.exertion_level() ) );

    const int makes = recp.makes_amount();
    if( makes > 1 ) {
        oss << string_format( _( "Recipe makes: <color_cyan>%d</color>\n" ), makes );
    }

    oss << string_format( _( "Craftable in the dark?  <color_cyan>%s</color>\n" ),
                          recp.has_flag( flag_BLIND_EASY ) ? _( "Easy" ) :
                          recp.has_flag( flag_BLIND_HARD ) ? _( "Hard" ) :
                          _( "Impossible" ) );

    const inventory &crafting_inv = guy.crafting_inventory();
    if( recp.result() ) {
        const int nearby_amount = crafting_inv.count_item( recp.result() );
        std::string nearby_string;
        if( nearby_amount == 0 ) {
            nearby_string = "<color_light_gray>0</color>";
        } else if( nearby_amount > 9000 ) {
            // at some point you get too many to count at a glance and just know you have a lot
            nearby_string = _( "<color_red>It's Over 9000!!!</color>" );
        } else {
            nearby_string = string_format( "<color_yellow>%d</color>", nearby_amount );
        }
        oss << string_format( _( "Nearby: %s\n" ), nearby_string );
    }

    const bool can_craft_this = avail.can_craft;
    if( can_craft_this && avail.would_use_rotten ) {
        oss << _( "<color_red>Will use rotten ingredients</color>\n" );
    }
    if( can_craft_this && avail.would_use_favorite ) {
        oss << _( "<color_red>Will use favorited ingredients</color>\n" );
    }
    const bool too_complex = recp.deduped_requirements().is_too_complex();
    if( can_craft_this && too_complex ) {
        oss << _( "Due to the complex overlapping requirements, this "
                  "recipe <color_yellow>may appear to be craftable "
                  "when it is not</color>.\n" );
    }
    if( !can_craft_this && avail.apparently_craftable && !recp.is_nested() ) {
        oss << _( "<color_red>Cannot be crafted because the same item is needed "
                  "for multiple components</color>\n" );
    }
    const float time_maluses = avail.get_proficiency_time_maluses();
    const float fail_maluses = avail.get_proficiency_failure_maluses();
    if( time_maluses != 1.0 && fail_maluses != 1.0 ) {
        oss << string_format( _( "<color_yellow>This recipe will take %.1fx as long as normal, "
                                 "and be %.1fx more likely to incur failures, because you "
                                 "lack some of the proficiencies used.</color>\n" ), time_maluses, fail_maluses );
    } else if( time_maluses != 1.0 ) {
        oss << string_format( _( "<color_yellow>This recipe will take %.1fx as long as normal, "
                                 "because you lack some of the proficiencies used.</color>\n" ), time_maluses );
    } else if( fail_maluses != 1.0 ) {
        oss << string_format( _( "<color_yellow>This recipe will be %.1fx more likely to incur failures, "
                                 "because you lack some of the proficiencies used.</color>\n" ), fail_maluses );
    }
    if( !can_craft_this && !avail.has_proficiencies ) {
        oss << _( "<color_red>Cannot be crafted because you lack"
                  " the required proficiencies.</color>\n" );
    }

    if( recp.has_byproducts() ) {
        oss << _( "Byproducts:\n" );
        for( const std::pair<const itype_id, int> &bp : recp.get_byproducts() ) {
            const itype *t = item::find_type( bp.first );
            int amount = bp.second * batch_size;
            if( t->count_by_charges() ) {
                oss << string_format( "> %s (%d)\n", t->nname( 1 ), amount );
            } else {
                oss << string_format( "> %d %s\n", amount,
                                      t->nname( static_cast<unsigned int>( amount ) ) );
            }
        }
    }

    std::vector<std::string> result = foldstring( oss.str(), fold_width );

    if( !recp.is_nested() ) {
        const requirement_data &req = recp.simple_requirements();
        const std::vector<std::string> tools = req.get_folded_tools_list(
                fold_width, color, crafting_inv, batch_size );
        const std::vector<std::string> comps = req.get_folded_components_list(
                fold_width, color, crafting_inv, recp.get_component_filter(), batch_size, qry_comps );
        result.insert( result.end(), tools.begin(), tools.end() );
        result.insert( result.end(), comps.begin(), comps.end() );
    }

    oss = std::ostringstream();
    if( !guy.knows_recipe( &recp ) ) {
        oss << _( "Recipe not memorized yet\n" );
        const std::set<itype_id> books_with_recipe = guy.get_books_for_recipe( crafting_inv, &recp );
        if( !books_with_recipe.empty() ) {
            const std::string enumerated_books = enumerate_as_string( books_with_recipe,
            []( const itype_id & type_id ) {
                return colorize( item::nname( type_id ), c_cyan );
            } );
            oss << string_format( _( "Written in: %s\n" ), enumerated_books );
        } else {
            std::vector<const npc *> knowing_helpers;
            for( const npc *helper : guy.get_crafting_helpers() ) {
                if( helper->knows_recipe( &recp ) ) {
                    knowing_helpers.push_back( helper );
                }
            }
            if( !knowing_helpers.empty() ) {
                const std::string enumerated_helpers = enumerate_as_string( knowing_helpers,
                []( const npc * helper ) {
                    return colorize( helper->get_name(), c_cyan );
                } );
                oss << string_format( _( "Known by: %s\n" ), enumerated_helpers );
            }
        }
    }
    std::vector<std::string> tmp = foldstring( oss.str(), fold_width );
    result.insert( result.end(), tmp.begin(), tmp.end() );

    return result;
}

static std::string practice_recipe_description( const recipe &recp,
        const Character &player_character )
{
    std::ostringstream oss;
    oss << recp.description.translated() << "\n\n";
    if( recp.practice_data->min_difficulty != recp.practice_data->max_difficulty ) {
        std::string txt = string_format( _( "Difficulty range: %d to %d" ),
                                         recp.practice_data->min_difficulty, recp.practice_data->max_difficulty );
        oss << txt << "\n";
    }
    if( recp.skill_used ) {
        const int player_skill_level = player_character.get_all_skills().get_skill_level( recp.skill_used );
        if( player_skill_level < recp.practice_data->min_difficulty ) {
            std::string txt = string_format(
                                  _( "You do not possess the minimum <color_cyan>%s</color> skill level required to practice this." ),
                                  recp.skill_used->name() );
            txt = string_format( "<color_red>%s</color>", txt );
            oss << txt << "\n";
        }
        if( recp.practice_data->skill_limit != MAX_SKILL ) {
            std::string txt = string_format(
                                  _( "This practice action will not increase your <color_cyan>%s</color> skill above %d." ),
                                  recp.skill_used->name(), recp.practice_data->skill_limit );
            if( player_skill_level >= recp.practice_data->skill_limit ) {
                txt = string_format( "<color_brown>%s</color>", txt );
            }
            oss << txt << "\n";
        }
    }
    return oss.str();
}

static input_context make_crafting_context( bool highlight_unread_recipes )
{
    input_context ctxt( "CRAFTING" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "SCROLL_RECIPE_INFO_UP" );
    ctxt.register_action( "SCROLL_RECIPE_INFO_DOWN" );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "HOME" );
    ctxt.register_action( "END" );
    ctxt.register_action( "SCROLL_ITEM_INFO_UP" );
    ctxt.register_action( "SCROLL_ITEM_INFO_DOWN" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "TOGGLE_FAVORITE" );
    ctxt.register_action( "HELP_RECIPE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CYCLE_BATCH" );
    ctxt.register_action( "RELATED_RECIPES" );
    ctxt.register_action( "HIDE_SHOW_RECIPE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    if( highlight_unread_recipes ) {
        ctxt.register_action( "TOGGLE_RECIPE_UNREAD" );
        ctxt.register_action( "MARK_ALL_RECIPES_READ" );
        ctxt.register_action( "TOGGLE_UNREAD_RECIPES_FIRST" );
    }
    return ctxt;
}

class recipe_result_info_cache
{
        std::vector<iteminfo> info;
        const recipe *last_recipe = nullptr;
        int last_terminal_width = 0;
        int panel_width;
        int cached_batch_size = 1;

        void get_byproducts_data( const recipe *rec, std::vector<iteminfo> &summary_info,
                                  std::vector<iteminfo> &details_info );
        void get_item_details( item &dummy_item, int quantity_per_batch,
                               std::vector<iteminfo> &details_info, const std::string &classification, bool uses_charges );
        void get_item_header( item &dummy_item, int quantity_per_batch, std::vector<iteminfo> &info,
                              const std::string &classification, bool uses_charges );
        void insert_iteminfo_block_separator( std::vector<iteminfo> &info_vec,
                                              const std::string &title ) const;
    public:
        item_info_data get_result_data( const recipe *rec, int batch_size, int &scroll_pos,
                                        const catacurses::window &window );
};

void recipe_result_info_cache::get_byproducts_data( const recipe *rec,
        std::vector<iteminfo> &summary_info, std::vector<iteminfo> &details_info )
{
    const std::string byproduct_string = _( "Byproduct" );

    for( const std::pair<const itype_id, int> &bp : rec->get_byproducts() ) {
        insert_iteminfo_block_separator( details_info, byproduct_string );
        item dummy_item = item( bp.first );
        bool uses_charges = dummy_item.count_by_charges();
        get_item_header( dummy_item, bp.second, summary_info, _( "With byproduct" ), uses_charges );
        get_item_details( dummy_item, bp.second, details_info, byproduct_string, uses_charges );
    }
}

void recipe_result_info_cache::get_item_details( item &dummy_item,
        const int quantity_per_batch,
        std::vector<iteminfo> &details_info, const std::string &classification, const bool uses_charges )
{
    std::vector<iteminfo> temp_info;
    int total_quantity = quantity_per_batch * cached_batch_size;
    get_item_header( dummy_item, quantity_per_batch, details_info, classification, uses_charges );
    if( uses_charges ) {
        dummy_item.charges *= total_quantity;
        dummy_item.info( true, temp_info );
        dummy_item.charges /= total_quantity;
    } else {
        dummy_item.info( true, temp_info, total_quantity );
    }
    details_info.insert( std::end( details_info ), std::begin( temp_info ), std::end( temp_info ) );
}

void recipe_result_info_cache::get_item_header( item &dummy_item, const int quantity_per_batch,
        std::vector<iteminfo> &info, const std::string &classification, const bool uses_charges )
{
    int total_quantity = quantity_per_batch * cached_batch_size;
    //Handle multiple charges and multiple discrete items separately
    if( uses_charges ) {
        dummy_item.charges = total_quantity;
        info.emplace_back( "DESCRIPTION",
                           "<bold>" + classification + ": </bold>" + dummy_item.display_name() );
        //Reset charges so that multiple calls to this function don't produce unexpected results
        dummy_item.charges /= total_quantity;
    } else {
        //Add summary line.  Don't need to indicate count if there's only 1
        info.emplace_back( "DESCRIPTION",
                           "<bold>" + classification + ": </bold>" + dummy_item.display_name( total_quantity ) +
                           ( total_quantity == 1 ? "" : string_format( " (%d)", total_quantity ) ) );
    }
    if( dummy_item.has_flag( flag_VARSIZE ) &&
        dummy_item.has_flag( flag_FIT ) ) {
        /* Resulting item can be (poor fit).  Check if it can actually be crafted as poor fit
         * Currently, that means: can it have poorly-fitted components?*/
        std::vector<std::vector<item_comp> > item_component_reqs =
            last_recipe->simple_requirements().get_components();
        bool has_varsize_components = false;
        for( const std::vector<item_comp> &component_options : item_component_reqs ) {
            for( const item_comp &component : component_options ) {
                const itype *type = item::find_type( component.type );
                if( type->has_flag( flag_VARSIZE ) ) {
                    has_varsize_components = true;
                    break;
                }
            }
            if( has_varsize_components ) {
                break;
            }
        }
        if( has_varsize_components ) {
            info.emplace_back( "DESCRIPTION",
                               _( "<bold>Note:</bold> if crafted from poorly-fitting components, the resulting item may also be poorly-fitted." ) );
        }
    }
}

item_info_data recipe_result_info_cache::get_result_data( const recipe *rec, const int batch_size,
        int &scroll_pos, const catacurses::window &window )
{
    /* If the recipe has not changed, return the cached version in info.
       Unfortunately, the separator lines are baked into info at a specific width, so if the terminal width
       has changed, the info needs to be regenerated */
    if( rec == last_recipe && rec != nullptr && TERMX == last_terminal_width &&
        batch_size == cached_batch_size ) {
        item_info_data data( "", "", info, {}, scroll_pos );
        return data;
    }

    cached_batch_size = batch_size;
    last_recipe = rec;
    scroll_pos = 0;
    last_terminal_width = TERMX;
    panel_width = getmaxx( window );

    info.clear(); //New recipe, new info

    /*We need to do some calculations to put together the results summary and very similar calculations to
      put together the details, so, have a separate vector specifically for the details, to be appended later */
    std::vector<iteminfo> details_info;

    //Make a temporary item for the result.  NOTE: If the result would normally be in a container, this is not.
    item dummy_result = item( rec->result(), calendar::turn, item::default_charges_tag{} );
    //Check if recipe result is a clothing item that can be properly fitted
    if( dummy_result.has_flag( flag_VARSIZE ) && !dummy_result.has_flag( flag_FIT ) ) {
        //Check if it can actually fit.  If so, list the fitted info
        item::sizing general_fit = dummy_result.get_sizing( get_player_character() );
        if( general_fit == item::sizing::small_sized_small_char ||
            general_fit == item::sizing::human_sized_human_char ||
            general_fit == item::sizing::big_sized_big_char ||
            general_fit == item::sizing::ignore ) {
            dummy_result.set_flag( flag_FIT );
        }
    }
    bool result_uses_charges = dummy_result.count_by_charges();
    int const makes_amount = rec->makes_amount();
    if( result_uses_charges ) {
        dummy_result.charges = 1;
    }
    dummy_result.set_var( "recipe_exemplar", rec->ident().str() );
    item dummy_container;

    //Several terms are used repeatedly in headers/descriptions, list them here for a single entry/translation point
    const std::string result_string = _( "Result" );
    const std::string recipe_output_string = _( "Recipe Outputs" );
    const std::string recipe_result_string = _( "Recipe Result" );
    const std::string container_string = _( "Container" );
    const std::string in_container_string = _( "In container" );
    const std::string container_info_string = _( "Container Information" );

    //Set up summary at top so people know they can look further to learn about byproducts and such
    //First, see if we need it at all:
    if( rec->container_id() == itype_id::NULL_ID() && !rec->has_byproducts() ) {
        //We don't need a summary for a single item, just give us the details
        insert_iteminfo_block_separator( details_info, recipe_result_string );
        get_item_details( dummy_result, makes_amount, details_info, result_string, result_uses_charges );

    } else { //We do need a summary
        //Top of the header
        insert_iteminfo_block_separator( info, recipe_output_string );
        //If the primary result uses charges and is in a container, need to calculate number of charges
        //If it's in a container, focus on the contents
        if( rec->container_id() != itype_id::NULL_ID() ) {
            dummy_container = item( rec->container_id(), calendar::turn, item::default_charges_tag{} );
            //Put together the summary in info:
            get_item_header( dummy_result, makes_amount, info, recipe_result_string, result_uses_charges );
            get_item_header( dummy_container, 1, info, in_container_string,
                             false ); //Seems reasonable to assume a container won't use charges
            //Put together the details in details_info:
            insert_iteminfo_block_separator( details_info, recipe_result_string );
            get_item_details( dummy_result, makes_amount, details_info, recipe_result_string,
                              result_uses_charges );

            insert_iteminfo_block_separator( details_info, container_info_string );
            get_item_details( dummy_container, 1, details_info, container_string, false );
        } else { //If it's not in a container, just tell us about the item
            //Add a line to the summary:
            get_item_header( dummy_result, makes_amount, info, recipe_result_string, result_uses_charges );
            //Add the details 'header'
            insert_iteminfo_block_separator( details_info, recipe_result_string );
            //Get the item details:
            get_item_details( dummy_result, makes_amount, details_info, recipe_result_string,
                              result_uses_charges );
        }
        if( rec->has_byproducts() ) {
            get_byproducts_data( rec, info, details_info );
        }
        info.emplace_back( "DESCRIPTION", "  " );  //Blank line for formatting
    }
    //Merge summary and details
    info.insert( std::end( info ), std::begin( details_info ), std::end( details_info ) );
    item_info_data data( "", "", info, {}, scroll_pos );
    return data;
}

void recipe_result_info_cache::insert_iteminfo_block_separator( std::vector<iteminfo> &info_vec,
        const std::string &title ) const
{
    info_vec.emplace_back( "DESCRIPTION", "--" );
    info_vec.emplace_back( "DESCRIPTION", std::string( center_text_pos( title, 0,
                           panel_width ), ' ' ) +
                           "<bold>" + title + "</bold>" );
    info_vec.emplace_back( "DESCRIPTION", "--" );
}

static std::pair<std::vector<const recipe *>, bool>
recipes_from_cat( const recipe_subset &available_recipes, const std::string &cat,
                  const std::string &subcat )
{
    if( subcat == "CSC_*_FAVORITE" ) {
        return std::make_pair( available_recipes.favorite(), false );
    } else if( subcat == "CSC_*_RECENT" ) {
        return std::make_pair( available_recipes.recent(), false );
    } else if( subcat == "CSC_*_HIDDEN" ) {
        return std::make_pair( available_recipes.hidden(), true );
    } else {
        return std::make_pair( available_recipes.in_category( cat, subcat != "CSC_ALL" ? subcat : "" ),
                               false );
    }
}

struct recipe_info_cache {
    const recipe *recp = nullptr;
    std::string qry_comps;
    int batch_size;
    int fold_width;
    std::vector<std::string> text;
};

static const std::vector<std::string> &cached_recipe_info( recipe_info_cache &info_cache,
        const recipe &recp, const availability &avail, Character &guy, const std::string &qry_comps,
        const int batch_size, const int fold_width, const nc_color &color )
{
    if( info_cache.recp != &recp ||
        info_cache.qry_comps != qry_comps ||
        info_cache.batch_size != batch_size ||
        info_cache.fold_width != fold_width ) {
        info_cache.recp = &recp;
        info_cache.qry_comps = qry_comps;
        info_cache.batch_size = batch_size;
        info_cache.fold_width = fold_width;
        info_cache.text = recipe_info( recp, avail, guy, qry_comps, batch_size, fold_width, color );
    }
    return info_cache.text;
}

struct item_info_cache {
    const recipe *last_recipe = nullptr;
    item dummy;
};

static recipe_subset filter_recipes( const recipe_subset &available_recipes,
                                     const std::string &qry,
                                     const Character &player_character,
                                     const std::function<void( size_t, size_t )> &progress_callback )
{
    size_t qry_begin = 0;
    size_t qry_end = 0;
    recipe_subset filtered_recipes = available_recipes;
    do {
        // Find next ','
        qry_end = qry.find_first_of( ',', qry_begin );

        auto qry_filter_str = trim( qry.substr( qry_begin, qry_end - qry_begin ) );
        // Process filter
        if( qry_filter_str.size() > 2 && qry_filter_str[1] == ':' ) {
            switch( qry_filter_str[0] ) {
                case 't':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::tool, progress_callback );
                    break;

                case 'c':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::component, progress_callback );
                    break;

                case 's':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::skill, progress_callback );
                    break;

                case 'p':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::primary_skill, progress_callback );
                    break;

                case 'Q':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::quality, progress_callback );
                    break;

                case 'q':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::quality_result, progress_callback );
                    break;

                case 'd':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::description_result, progress_callback );
                    break;

                case 'm': {
                    const recipe_subset &learned = player_character.get_learned_recipes();
                    recipe_subset temp_subset;
                    if( query_is_yes( qry_filter_str ) ) {
                        temp_subset = available_recipes.intersection( learned );
                    } else {
                        temp_subset = available_recipes.difference( learned );
                    }
                    filtered_recipes = filtered_recipes.intersection( temp_subset );
                    break;
                }

                case 'P':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::proficiency, progress_callback );
                    break;

                case 'l':
                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                       recipe_subset::search_type::difficulty, progress_callback );
                    break;

                default:
                    break;
            }
        } else if( qry_filter_str.size() > 1 && qry_filter_str[0] == '-' ) {
            filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 1 ),
                               recipe_subset::search_type::exclude_name, progress_callback );
        } else {
            filtered_recipes = filtered_recipes.reduce( qry_filter_str );
        }

        qry_begin = qry_end + 1;
    } while( qry_end != std::string::npos );
    return filtered_recipes;
}

namespace
{
struct SearchPrefix {
    char key;
    translation example;
    translation description;
};
} // namespace

static const std::vector<SearchPrefix> prefixes = {
    //~ Example result description search term
    { 'q', to_translation( "metal sawing" ), to_translation( "<color_cyan>quality</color> of resulting item" ) },
    { 'd', to_translation( "reach attack" ), to_translation( "<color_cyan>full description</color> of resulting item (slow)" ) },
    { 'c', to_translation( "plank" ), to_translation( "<color_cyan>component</color> required to craft" ) },
    { 'p', to_translation( "tailoring" ), to_translation( "<color_cyan>primary skill</color> used to craft" ) },
    { 's', to_translation( "food handling" ), to_translation( "<color_cyan>any skill</color> used to craft" ) },
    { 'Q', to_translation( "fine bolt turning" ), to_translation( "<color_cyan>quality</color> required to craft" ) },
    { 't', to_translation( "soldering iron" ), to_translation( "<color_cyan>tool</color> required to craft" ) },
    { 'm', to_translation( "yes" ), to_translation( "recipes which are <color_cyan>memorized</color> or not" ) },
    { 'P', to_translation( "Blacksmithing" ), to_translation( "<color_cyan>proficiency</color> used to craft" ) },
    { 'l', to_translation( "5" ), to_translation( "<color_cyan>difficulty</color> of the recipe as a number or range" ) },
};

static const translation filter_help_start = to_translation(
            "The default is to search result names.  Some single-character prefixes "
            "can be used with a colon <color_red>:</color> to search in other ways.  Additional filters "
            "are separated by commas <color_red>,</color>.\n"
            "Filtering by difficulty can accept range; "
            "<color_yellow>l</color><color_white>:5~10</color> for all recipes from difficulty 5 to 10.\n"
            "\n\n"
            "<color_white>Examples:</color>\n" );

static bool mouse_in_window( cata::optional<point> coord, const catacurses::window &w_ )
{
    if( coord.has_value() ) {
        inclusive_rectangle<point> window_area( point( getbegx( w_ ), getbegy( w_ ) ),
                                                point( getmaxx( w_ ) + getbegx( w_ ), getmaxy( w_ ) + getbegy( w_ ) ) );
        if( window_area.contains( coord.value() ) ) {
            return true;
        }
    }
    return false;
}

static void recursively_expance_recipes( std::vector<const recipe *> &current,
        std::vector<int> &indent, std::map<const recipe *, availability> &availability_cache, int i,
        Character &player_character, bool unread_recipes_first, bool highlight_unread_recipes,
        const recipe_subset &available_recipes, const std::set<recipe_id> &hidden_recipes )
{
    std::vector<const recipe *> tmp;
    for( const recipe_id &nested : current[i]->nested_category_data ) {

        if( available_recipes.contains( &nested.obj() ) &&
            hidden_recipes.find( nested ) == hidden_recipes.end() ) {
            // only do this if we can actually craft the recipe
            tmp.push_back( &nested.obj() );
            indent.insert( indent.begin() + i + 1, indent[i] + 2 );
            if( !availability_cache.count( &nested.obj() ) ) {
                availability_cache.emplace( &nested.obj(), availability( &nested.obj() ) );
            }
        }
    }

    std::stable_sort( tmp.begin(), tmp.end(), [
                       &player_character, &availability_cache, unread_recipes_first,
                       highlight_unread_recipes
    ]( const recipe * const a, const recipe * const b ) {
        if( highlight_unread_recipes && unread_recipes_first ) {
            const bool a_read = uistate.read_recipes.count( a->ident() );
            const bool b_read = uistate.read_recipes.count( b->ident() );
            if( a_read != b_read ) {
                return !a_read;
            }
        }
        const bool can_craft_a = availability_cache.at( a ).can_craft;
        const bool can_craft_b = availability_cache.at( b ).can_craft;
        if( can_craft_a != can_craft_b ) {
            return can_craft_a;
        }
        if( b->difficulty != a->difficulty ) {
            return b->difficulty < a->difficulty;
        }
        const std::string a_name = a->result_name();
        const std::string b_name = b->result_name();
        if( a_name != b_name ) {
            return localized_compare( a_name, b_name );
        }
        return b->time_to_craft( player_character ) <
               a->time_to_craft( player_character );
    } );

    current.insert( current.begin() + i + 1, tmp.begin(), tmp.end() );
}

// take the current and itterate through expanding each recipe
static void expand_recipes( std::vector<const recipe *> &current,
                            std::vector<int> &indent, std::map<const recipe *, availability> &availability_cache,
                            Character &player_character, bool unread_recipes_first, bool highlight_unread_recipes,
                            const recipe_subset &available_recipes, const std::set<recipe_id> &hidden_recipes )
{
    //TODO Make this more effecient
    for( size_t i = 0; i < current.size(); ++i ) {
        if( current[i]->is_nested() &&
            uistate.expanded_recipes.find( current[i]->ident() ) != uistate.expanded_recipes.end() ) {
            // add all the recipes from the nests
            recursively_expance_recipes( current, indent, availability_cache, i, player_character,
                                         unread_recipes_first, highlight_unread_recipes, available_recipes, hidden_recipes );
        }
    }
}

static std::string list_nested( const recipe *rec, const inventory &crafting_inv,
                                const std::vector<npc *> &helpers, int indent = 0 )
{
    std::string description;
    availability avail( rec );
    if( rec->is_nested() ) {
        description += colorize( std::string( indent,
                                              ' ' ) + rec->result_name() + ":\n", avail.color() );
        for( const recipe_id &r : rec->nested_category_data ) {
            description += list_nested( &r.obj(), crafting_inv, helpers, indent + 2 );
        }
    } else if( get_avatar().has_recipe( rec, crafting_inv, helpers ) ) {
        description += colorize( std::string( indent,
                                              ' ' ) + rec->result_name() + "\n", avail.color() );
    }

    return description;
}

static void nested_toggle( recipe_id rec, bool &recalc, bool &keepline )
{
    auto loc = uistate.expanded_recipes.find( rec );
    if( loc != uistate.expanded_recipes.end() ) {
        uistate.expanded_recipes.erase( rec );
    } else {
        uistate.expanded_recipes.insert( rec );
    }
    recalc = true;
    keepline = true;
}

static bool selection_ok( const std::vector<const recipe *> &list, const int current_line,
                          const bool nested_acceptable )
{
    std::string error_message;
    if( list.empty() ) {
        error_message = _( "Nothing selected!" );
    } else if( list[current_line]->is_nested() && !nested_acceptable ) {
        error_message = _( "Select a recipe within this group" );
    } else {
        return true;
    }

    query_popup()
    .message( "%s", error_message )
    .option( "QUIT" )
    .query();
    return false;
}

const recipe *select_crafting_recipe( int &batch_size_out, const recipe_id &goto_recipe )
{
    recipe_result_info_cache result_info;
    recipe_info_cache r_info_cache;
    int recipe_info_scroll = 0;
    int item_info_scroll = 0;
    int item_info_scroll_popup = 0;
    const int headHeight = 3;
    const int subHeadHeight = 2;

    bool isWide = false;
    int width = 0;
    int dataLines = 0;
    int dataHalfLines = 0;
    int dataHeight = 0;
    int item_info_width = 0;
    const bool highlight_unread_recipes = get_option<bool>( "HIGHLIGHT_UNREAD_RECIPES" );

    input_context ctxt = make_crafting_context( highlight_unread_recipes );

    catacurses::window w_head_tabs; //For the recipe category tabs on the left
    catacurses::window w_head_info; //For the new/hidden/status information on the right
    catacurses::window w_subhead;
    catacurses::window w_data;
    catacurses::window w_iteminfo;
    inclusive_rectangle<point> mouseover_area_list;
    inclusive_rectangle<point> mouseover_area_recipe;
    std::vector<std::string> keybinding_tips;
    int keybinding_x = 0;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
        isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );

        width = isWide ? ( freeWidth > FULL_SCREEN_WIDTH ? FULL_SCREEN_WIDTH * 2 : TERMX ) :
                FULL_SCREEN_WIDTH;
        const unsigned int header_info_width = craft_info_width( width );
        const int wStart = ( TERMX - width ) / 2;

        // Keybinding tips
        static const translation inline_fmt = to_translation(
                //~ %1$s: action description text before key,
                //~ %2$s: key description,
                //~ %3$s: action description text after key.
                "keybinding", "%1$s[<color_yellow>%2$s</color>]%3$s" );
        static const translation separate_fmt = to_translation(
                //~ %1$s: key description,
                //~ %2$s: action description.
                "keybinding", "[<color_yellow>%1$s</color>]%2$s" );
        std::vector<std::string> act_descs;
        const auto add_action_desc = [&]( const std::string & act, const std::string & txt ) {
            act_descs.emplace_back( ctxt.get_desc( act, txt, input_context::allow_all_keys,
                                                   inline_fmt, separate_fmt ) );
        };
        add_action_desc( "CONFIRM", pgettext( "crafting gui", "Craft" ) );
        add_action_desc( "HELP_RECIPE", pgettext( "crafting gui", "Describe" ) );
        add_action_desc( "FILTER", pgettext( "crafting gui", "Filter" ) );
        add_action_desc( "RESET_FILTER", pgettext( "crafting gui", "Reset filter" ) );
        if( highlight_unread_recipes ) {
            add_action_desc( "TOGGLE_RECIPE_UNREAD", pgettext( "crafting gui", "Read/unread" ) );
            add_action_desc( "MARK_ALL_RECIPES_READ", pgettext( "crafting gui", "Mark all as read" ) );
            add_action_desc( "TOGGLE_UNREAD_RECIPES_FIRST",
                             pgettext( "crafting gui", "Show unread recipes first" ) );
        }
        add_action_desc( "HIDE_SHOW_RECIPE", pgettext( "crafting gui", "Show/hide" ) );
        add_action_desc( "RELATED_RECIPES", pgettext( "crafting gui", "Related" ) );
        add_action_desc( "TOGGLE_FAVORITE", pgettext( "crafting gui", "Favorite" ) );
        add_action_desc( "CYCLE_BATCH", pgettext( "crafting gui", "Batch" ) );
        add_action_desc( "HELP_KEYBINDINGS", pgettext( "crafting gui", "Keybindings" ) );
        keybinding_x = isWide ? 5 : 2;
        keybinding_tips = foldstring( enumerate_as_string( act_descs, enumeration_conjunction::none ),
                                      width - keybinding_x * 2 );

        const int tailHeight = keybinding_tips.size() + 2;
        dataLines = TERMY - ( headHeight + subHeadHeight ) - tailHeight;
        dataHalfLines = dataLines / 2;
        dataHeight = TERMY - ( headHeight + subHeadHeight );

        w_head_tabs = catacurses::newwin( headHeight, ( width - header_info_width ), point( wStart, 0 ) );
        w_head_info = catacurses::newwin( headHeight, header_info_width,
                                          point( wStart + ( width - header_info_width ), 0 ) );
        w_subhead = catacurses::newwin( subHeadHeight, width, point( wStart, 3 ) );
        w_data = catacurses::newwin( dataHeight, width, point( wStart,
                                     headHeight + subHeadHeight ) );

        if( isWide ) {
            item_info_width = width - FULL_SCREEN_WIDTH - 1;
            const int item_info_height = dataHeight - tailHeight;
            const point item_info( wStart + width - item_info_width, headHeight + subHeadHeight );

            w_iteminfo = catacurses::newwin( item_info_height, item_info_width,
                                             item_info );
        } else {
            item_info_width = 0;
            w_iteminfo = {};
        }

        ui.position( point( wStart, 0 ), point( width, TERMY ) );
    } );
    ui.mark_resize();

    bool is_filtered_unread = false;
    std::map<std::string, bool> is_cat_unread;
    std::map<std::string, std::map<std::string, bool>> is_subcat_unread;
    tab_list tab( craft_cat_list );
    tab_list subtab( craft_subcat_list[tab.cur()] );
    std::map<size_t, inclusive_rectangle<point>> translated_tab_map;
    std::map<size_t, inclusive_rectangle<point>> translated_subtab_map;
    std::map<size_t, inclusive_rectangle<point>> list_map;
    std::vector<const recipe *> current;
    // how much to indent any item
    std::vector<int> indent;
    std::vector<availability> available;
    int line = 0;
    bool unread_recipes_first = false;
    bool user_moved_line = false;
    bool recalc = true;
    bool recalc_unread = highlight_unread_recipes;
    bool keepline = false;
    bool done = false;
    bool batch = false;
    bool show_hidden = false;
    size_t num_hidden = 0;
    int num_recipe = 0;
    int batch_line = 0;
    const recipe *chosen = nullptr;
    int last_line = -1;
    bool just_toggled_unread = false;

    Character &player_character = get_player_character();
    const inventory &crafting_inv = player_character.crafting_inventory();
    const std::vector<npc *> helpers = player_character.get_crafting_helpers();
    std::string filterstring;

    const recipe_subset &available_recipes = player_character.get_available_recipes( crafting_inv,
            &helpers );
    std::map<const recipe *, availability> availability_cache;

    const std::string new_recipe_str = pgettext( "crafting gui", "NEW!" );
    const nc_color new_recipe_str_col = c_light_green;
    const int new_recipe_str_width = utf8_width( new_recipe_str );

    if( goto_recipe.is_valid() ) {
        const std::vector<const recipe *> &gotocat = available_recipes.in_category( goto_recipe->category );
        if( !gotocat.empty() ) {
            const auto gotorec = std::find_if( gotocat.begin(),
            gotocat.end(), [&goto_recipe]( const recipe * r ) {
                return r && r->ident() == goto_recipe;
            } );
            if( gotorec != gotocat.end() &&
                std::find( craft_cat_list.begin(), craft_cat_list.end(),
                           goto_recipe->category ) != craft_cat_list.end() ) {
                while( tab.cur() != goto_recipe->category ) {
                    tab.next();
                }
                subtab = tab_list( craft_subcat_list[tab.cur()] );
                chosen = *gotorec;
                show_hidden = true;
                keepline = true;
                current = gotocat;
                line = gotorec - gotocat.begin();
            }
        }
    }

    ui.on_redraw( [&]( ui_adaptor & ui ) {
        if( highlight_unread_recipes && recalc_unread ) {
            if( filterstring.empty() ) {
                for( const std::string &cat : craft_cat_list ) {
                    is_cat_unread[cat] = false;
                    for( const std::string &subcat : craft_subcat_list[cat] ) {
                        is_subcat_unread[cat][subcat] = false;
                        const std::pair<std::vector<const recipe *>, bool> result = recipes_from_cat( available_recipes,
                                cat, subcat );
                        const std::vector<const recipe *> &recipes = result.first;
                        const bool include_hidden = result.second;
                        for( const recipe *const rcp : recipes ) {
                            const recipe_id &rcp_id = rcp->ident();
                            if( !include_hidden && uistate.hidden_recipes.count( rcp_id ) ) {
                                continue;
                            }
                            if( uistate.read_recipes.count( rcp_id ) ) {
                                continue;
                            }
                            is_cat_unread[cat] = true;
                            is_subcat_unread[cat][subcat] = true;
                            break;
                        }
                    }
                }
            } else {
                is_filtered_unread = false;
                for( const recipe *const rcp : current ) {
                    const recipe_id &rcp_id = rcp->ident();
                    if( uistate.hidden_recipes.count( rcp_id ) ) {
                        continue;
                    }
                    if( uistate.read_recipes.count( rcp_id ) ) {
                        continue;
                    }
                    is_filtered_unread = true;
                    break;
                }
            }
            recalc_unread = false;
        }

        const TAB_MODE m = batch ? BATCH : filterstring.empty() ? NORMAL : FILTERED;
        translated_tab_map = draw_recipe_tabs( w_head_tabs, tab, m, is_filtered_unread, is_cat_unread );
        translated_subtab_map = draw_recipe_subtabs( w_subhead, tab.cur(), subtab.cur_index(),
                                available_recipes, m,
                                is_subcat_unread[tab.cur()] );

        //Clear the crafting info panel, since that can change on a per-recipe basis
        werase( w_head_info );

        if( !show_hidden ) {
            draw_hidden_amount( w_head_info, num_hidden, num_recipe );
        }

        // Clear the screen of recipe data, and draw it anew
        werase( w_data );

        for( size_t i = 0; i < keybinding_tips.size(); ++i ) {
            nc_color dummy = c_white;
            print_colored_text( w_data, point( keybinding_x, dataLines + 1 + i ),
                                dummy, c_white, keybinding_tips[i] );
        }

        // Draw borders
        for( int i = 1; i < width - 1; ++i ) { // -
            mvwputch( w_data, point( i, dataHeight - 1 ), BORDER_COLOR, LINE_OXOX );
        }
        for( int i = 0; i < dataHeight - 1; ++i ) { // |
            mvwputch( w_data, point( 0, i ), BORDER_COLOR, LINE_XOXO );
            mvwputch( w_data, point( width - 1, i ), BORDER_COLOR, LINE_XOXO );
        }
        mvwputch( w_data, point( 0, dataHeight - 1 ), BORDER_COLOR, LINE_XXOO ); // |_
        mvwputch( w_data, point( width - 1, dataHeight - 1 ), BORDER_COLOR, LINE_XOOX ); // _|

        const int max_recipe_name_width = 27;
        int recmin = 0;
        int recmax = current.size();
        int istart = 0;
        int iend = 0;
        if( recmax > dataLines ) {
            if( line <= recmin + dataHalfLines ) {
                istart = recmin;
                iend = recmin + dataLines;
            } else if( line >= recmax - dataHalfLines ) {
                istart = recmax - dataLines;
                iend = recmax;
            } else {
                istart = line - dataHalfLines;
                iend = line - dataHalfLines + dataLines;
            }
        } else {
            istart = 0;
            iend = std::min<int>( current.size(), dataHeight + 1 );
        }
        list_map.clear();
        for( int i = istart; i < iend; ++i ) {
            if( i >= static_cast<int>( indent.size() ) || indent[i] < 0 ) {
                indent.assign( current.size(), 0 );
                debugmsg( _( "Indent for line %i not set correctly.  Indents reset to 0." ), i );
            }
            std::string tmp_name = std::string( indent[i],
                                                ' ' ) + current[i]->result_name( /*decorated=*/true );
            if( batch ) {
                tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name );
            }
            const bool rcp_read = !highlight_unread_recipes ||
                                  uistate.read_recipes.count( current[i]->ident() );
            const bool highlight = i == line;
            nc_color col = highlight ? available[i].selected_color() : available[i].color();
            const point print_from( 2, i - istart );
            if( highlight ) {
                ui.set_cursor( w_data, print_from );
            }
            int rcp_name_trim_width = max_recipe_name_width;
            if( !rcp_read ) {
                const point offset( max_recipe_name_width - new_recipe_str_width, 0 );
                mvwprintz( w_data, print_from + offset, new_recipe_str_col, "%s", new_recipe_str );
                rcp_name_trim_width -= new_recipe_str_width + 1;
            }
            mvwprintz( w_data, print_from, col, "%s", trim_by_length( tmp_name, rcp_name_trim_width ) );
            list_map.emplace( i, inclusive_rectangle<point>( print_from, point( 2 + max_recipe_name_width,
                              i - istart ) ) );
        }

        const int batch_size = batch ? line + 1 : 1;
        if( !current.empty() ) {
            const recipe &recp = *current[line];

            draw_can_craft_indicator( w_head_info, recp );

            const availability &avail = available[line];
            // border + padding + name + padding
            const int xpos = 1 + 1 + max_recipe_name_width + 3;
            const int fold_width = FULL_SCREEN_WIDTH - xpos - 2;
            const int w_left = getbegx( w_data );
            mouseover_area_list = inclusive_rectangle<point>( point( 1 + w_left, headHeight + subHeadHeight ),
                                  point( w_left + xpos - 1, headHeight + subHeadHeight + dataLines ) );
            mouseover_area_recipe = inclusive_rectangle<point>( point( xpos + w_left,
                                    headHeight + subHeadHeight ), point( xpos + w_left + fold_width + 1,
                                            headHeight + subHeadHeight + dataLines ) );
            const nc_color color = avail.color( true );
            const std::string qry = trim( filterstring );
            std::string qry_comps;
            if( qry.compare( 0, 2, "c:" ) == 0 ) {
                qry_comps = qry.substr( 2 );
            }

            const std::vector<std::string> &info = cached_recipe_info( r_info_cache,
                                                   recp, avail, player_character, qry_comps, batch_size, fold_width, color );

            const int total_lines = info.size();
            if( recipe_info_scroll < 0 ) {
                recipe_info_scroll = 0;
            } else if( recipe_info_scroll + dataLines > total_lines ) {
                recipe_info_scroll = std::max( 0, total_lines - dataLines );
            }
            for( int i = recipe_info_scroll;
                 i < std::min( recipe_info_scroll + dataLines, total_lines );
                 ++i ) {
                nc_color dummy = color;
                print_colored_text( w_data, point( xpos, i - recipe_info_scroll ),
                                    dummy, color, info[i] );
            }

            if( total_lines > dataLines ) {
                scrollbar().offset_x( xpos + fold_width + 1 ).content_size( total_lines )
                .viewport_pos( recipe_info_scroll ).viewport_size( dataLines )
                .apply( w_data );
            }
        }

        scrollbar()
        .offset_x( 0 )
        .offset_y( 0 )
        .content_size( recmax )
        .viewport_pos( istart )
        .viewport_size( dataLines ).apply( w_data );

        wnoutrefresh( w_data );

        if( isWide && !current.empty() ) {
            const recipe *cur_recipe = current[line];
            werase( w_iteminfo );
            if( cur_recipe->is_practice() ) {
                const std::string desc = practice_recipe_description( *cur_recipe, player_character );
                fold_and_print( w_iteminfo, point_zero, item_info_width, c_light_gray, desc );
                scrollbar().offset_x( item_info_width - 1 ).offset_y( 0 ).content_size( 1 ).viewport_size( getmaxy(
                            w_iteminfo ) ).apply( w_iteminfo );
                wnoutrefresh( w_iteminfo );
            } else if( cur_recipe->is_nested() ) {
                std::string desc = cur_recipe->description.translated() + "\n\n";;
                desc += list_nested( cur_recipe, crafting_inv, helpers );
                fold_and_print( w_iteminfo, point_zero, item_info_width, c_light_gray, desc );
                scrollbar().offset_x( item_info_width - 1 ).offset_y( 0 ).content_size( 1 ).viewport_size( getmaxy(
                            w_iteminfo ) ).apply( w_iteminfo );
                wnoutrefresh( w_iteminfo );
            } else {
                item_info_data data = result_info.get_result_data( cur_recipe, batch_size, item_info_scroll,
                                      w_iteminfo );
                data.without_getch = true;
                data.without_border = true;
                data.scrollbar_left = false;
                data.use_full_win = true;
                data.padding = 0;
                draw_item_info( w_iteminfo, data );
            }
        }
    } );

    do {
        if( recalc ) {
            // When we switch tabs, redraw the header
            recalc = false;
            const recipe *prev_rcp = nullptr;
            if( keepline && line >= 0
                && static_cast<size_t>( line ) < current.size() ) {
                prev_rcp = current[line];
            }

            show_hidden = false;
            available.clear();

            if( batch ) {
                current.clear();
                for( int i = 1; i <= 50; i++ ) {
                    current.push_back( chosen );
                    available.emplace_back( chosen, i );
                }
                indent.assign( current.size(), 0 );
            } else {
                static_popup popup;
                auto last_update = std::chrono::steady_clock::now();
                static constexpr std::chrono::milliseconds update_interval( 500 );

                std::function<void( size_t, size_t )> progress_callback =
                [&]( size_t at, size_t out_of ) {
                    auto now = std::chrono::steady_clock::now();
                    if( now - last_update < update_interval ) {
                        return;
                    }
                    last_update = now;
                    double percent = 100.0 * at / out_of;
                    popup.message( _( "Searching… %3.0f%%\n" ), percent );
                    ui_manager::redraw();
                    refresh_display();
                    inp_mngr.pump_events();
                };

                std::vector<const recipe *> picking;
                if( !filterstring.empty() ) {
                    auto qry = trim( filterstring );
                    recipe_subset filtered_recipes =
                        filter_recipes( available_recipes, qry, player_character, progress_callback );
                    picking.insert( picking.end(), filtered_recipes.begin(), filtered_recipes.end() );
                } else {
                    const std::pair<std::vector<const recipe *>, bool> result = recipes_from_cat( available_recipes,
                            tab.cur(), subtab.cur() );
                    show_hidden = result.second;
                    if( show_hidden ) {
                        current = result.first;
                    } else {
                        picking = result.first;
                    }
                }

                if( !show_hidden ) {
                    current.clear();
                    for( const recipe *i : picking ) {
                        if( uistate.hidden_recipes.find( i->ident() ) == uistate.hidden_recipes.end() ) {
                            current.push_back( i );
                        }
                    }
                    num_hidden = picking.size() - current.size();
                    num_recipe = picking.size();
                }

                available.reserve( current.size() );
                // cache recipe availability on first display
                for( const recipe *e : current ) {
                    if( !availability_cache.count( e ) ) {
                        availability_cache.emplace( e, availability( e ) );
                    }
                }

                if( subtab.cur() != "CSC_*_RECENT" ) {
                    std::stable_sort( current.begin(), current.end(), [
                       &player_character, &availability_cache, unread_recipes_first,
                       highlight_unread_recipes
                    ]( const recipe * const a, const recipe * const b ) {
                        if( highlight_unread_recipes && unread_recipes_first ) {
                            const bool a_read = uistate.read_recipes.count( a->ident() );
                            const bool b_read = uistate.read_recipes.count( b->ident() );
                            if( a_read != b_read ) {
                                return !a_read;
                            }
                        }
                        const bool can_craft_a = availability_cache.at( a ).can_craft;
                        const bool can_craft_b = availability_cache.at( b ).can_craft;
                        if( can_craft_a != can_craft_b ) {
                            return can_craft_a;
                        }
                        if( b->difficulty != a->difficulty ) {
                            return b->difficulty < a->difficulty;
                        }
                        const std::string a_name = a->result_name();
                        const std::string b_name = b->result_name();
                        if( a_name != b_name ) {
                            return localized_compare( a_name, b_name );
                        }
                        return b->time_to_craft( player_character ) <
                               a->time_to_craft( player_character );
                    } );
                }

                // set up indents and append the expanded entries
                // have to do this after we sort the list
                indent.assign( current.size(), 0 );
                expand_recipes( current, indent, availability_cache, player_character, unread_recipes_first,
                                highlight_unread_recipes, available_recipes, uistate.hidden_recipes );

                std::transform( current.begin(), current.end(),
                std::back_inserter( available ), [&]( const recipe * e ) {
                    return availability_cache.at( e );
                } );
            }

            line = 0;
            if( keepline && prev_rcp ) {
                // point to previously selected recipe
                int rcp_idx = 0;
                for( const recipe *const rcp : current ) {
                    if( rcp == prev_rcp ) {
                        line = rcp_idx;
                        break;
                    }
                    ++rcp_idx;
                }
            }
        }
        keepline = false;

        if( highlight_unread_recipes && !current.empty() && user_moved_line ) {
            // only automatically mark as read when moving cursor up or down by
            // one line, which means that the user is likely reading through the
            // list.
            user_moved_line = false;
            uistate.read_recipes.insert( current[line]->ident() );
            if( last_line != -1 ) {
                uistate.read_recipes.insert( current[last_line]->ident() );
                last_line = -1;
            }
            recalc_unread = true;
        }

        const bool previously_toggled_unread = just_toggled_unread;
        just_toggled_unread = false;
        ui_manager::redraw();
        const int scroll_item_info_lines = catacurses::getmaxy( w_iteminfo ) - 4;
        std::string action = ctxt.handle_input();
        const int recmax = static_cast<int>( current.size() );
        const int scroll_rate = recmax > 20 ? 10 : 3;

        cata::optional<point> coord = ctxt.get_coordinates_text( catacurses::stdscr );
        const bool mouse_in_list = coord.has_value() && mouseover_area_list.contains( coord.value() );
        const bool mouse_in_recipe = coord.has_value() && mouseover_area_recipe.contains( coord.value() );

        // Check mouse selection of recipes separately so that selecting an already-selected recipe
        // can go straight to "CONFIRM"
        if( action == "SELECT" ) {
            if( coord.has_value() ) {
                point local_coord = coord.value() - point( getbegx( w_data ), getbegy( w_data ) );
                for( const auto &entry : list_map ) {
                    if( entry.second.contains( local_coord ) ) {
                        if( line == static_cast<int>( entry.first ) ) {
                            action = "CONFIRM";
                        } else {
                            if( !previously_toggled_unread ) {
                                last_line = line;
                            }
                            line = entry.first;
                            user_moved_line = highlight_unread_recipes;
                        }
                    }
                }
            }
        }

        if( action == "SELECT" ) {
            bool handled = false;
            if( coord.has_value() ) {
                point local_coord = coord.value() - point( getbegx( w_head_tabs ), getbegy( w_head_tabs ) );
                for( const auto &entry : translated_tab_map ) {
                    if( entry.second.contains( local_coord ) ) {
                        tab.set_index( entry.first );
                        recalc = true;
                        subtab = tab_list( craft_subcat_list[tab.cur()] );
                        handled = true;
                    }
                }
                local_coord = coord.value() - point( getbegx( w_subhead ), getbegy( w_subhead ) );
                if( !handled && !batch && filterstring.empty() ) {
                    for( const auto &entry : translated_subtab_map ) {
                        if( entry.second.contains( local_coord ) ) {
                            subtab.set_index( entry.first );
                            recalc = true;
                        }
                    }
                }
            }
        } else if( action == "SCROLL_RECIPE_INFO_UP" ) {
            recipe_info_scroll -= dataLines;
        } else if( action == "SCROLL_UP" && mouse_in_recipe ) {
            --recipe_info_scroll;
        } else if( action == "SCROLL_RECIPE_INFO_DOWN" ) {
            recipe_info_scroll += dataLines;
        } else if( action == "SCROLL_DOWN" && mouse_in_recipe ) {
            ++recipe_info_scroll;
        } else if( action == "LEFT" || ( action == "SCROLL_UP" && mouse_in_window( coord, w_subhead ) ) ) {
            if( batch || !filterstring.empty() ) {
                continue;
            }
            std::string start = subtab.cur();
            do {
                subtab.prev();
            } while( subtab.cur() != start && available_recipes.empty_category( tab.cur(),
                     subtab.cur() != "CSC_ALL" ? subtab.cur() : "" ) );
            recalc = true;
        } else if( action == "SCROLL_ITEM_INFO_UP" ) {
            item_info_scroll -= scroll_item_info_lines;
        } else if( action == "SCROLL_UP" && mouse_in_window( coord, w_iteminfo ) ) {
            --item_info_scroll;
        } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
            item_info_scroll += scroll_item_info_lines;
        } else if( action == "SCROLL_DOWN" && mouse_in_window( coord, w_iteminfo ) ) {
            ++item_info_scroll;
        } else if( action == "PREV_TAB" || ( action == "SCROLL_UP" &&
                                             mouse_in_window( coord, w_head_tabs ) ) ) {
            tab.prev();
            // Default ALL
            subtab = tab_list( craft_subcat_list[tab.cur()] );
            recalc = true;
        } else if( action == "RIGHT" || ( action == "SCROLL_DOWN" &&
                                          mouse_in_window( coord, w_subhead ) ) ) {
            if( batch || !filterstring.empty() ) {
                continue;
            }
            std::string start = subtab.cur();
            do {
                subtab.next();
            } while( subtab.cur() != start && available_recipes.empty_category( tab.cur(),
                     subtab.cur() != "CSC_ALL" ? subtab.cur() : "" ) );
            recalc = true;
        } else if( action == "NEXT_TAB" || ( action == "SCROLL_DOWN" &&
                                             mouse_in_window( coord, w_head_tabs ) ) ) {
            tab.next();
            // Default ALL
            subtab = tab_list( craft_subcat_list[tab.cur()] );
            recalc = true;
        } else if( action == "DOWN" ) {
            if( !previously_toggled_unread ) {
                last_line = line;
            }
            line++;
            user_moved_line = highlight_unread_recipes;
        } else if( action == "SCROLL_DOWN" && mouse_in_list ) {
            line = std::min( recmax - 1, line + 1 );
        } else if( action == "UP" ) {
            if( !previously_toggled_unread ) {
                last_line = line;
            }
            line--;
            user_moved_line = highlight_unread_recipes;
        } else if( action == "SCROLL_UP" && mouse_in_list ) {
            line = std::max( 0, line - 1 );
        } else if( action == "PAGE_DOWN" ) {
            if( line == recmax - 1 ) {
                line = 0;
            } else if( line + scroll_rate >= recmax ) {
                line = recmax - 1;
            } else {
                line += +scroll_rate;
            }
        } else if( action == "PAGE_UP" ) {
            if( line == 0 ) {
                line = recmax - 1;
            } else if( line <= scroll_rate ) {
                line = 0;
            } else {
                line += -scroll_rate;
            }
        } else if( action == "HOME" ) {
            line = 0;
            user_moved_line = highlight_unread_recipes;
        } else if( action == "END" ) {
            line = -1;
            user_moved_line = highlight_unread_recipes;
        } else if( action == "CONFIRM" ) {
            if( available.empty() || ( !available[line].can_craft && !current[line]->is_nested() ) ) {
                query_popup()
                .message( "%s", _( "You can't do that!" ) )
                .option( "QUIT" )
                .query();
            } else if( current[line]->is_nested() ) {
                nested_toggle( current[line]->ident(), recalc, keepline );
            } else if( !player_character.check_eligible_containers_for_crafting( *current[line],
                       batch ? line + 1 : 1 ) ) {
                // popup is already inside check
            } else {
                chosen = current[line];
                batch_size_out = batch ? line + 1 : 1;
                done = true;
                uistate.read_recipes.insert( chosen->ident() );
            }
        } else if( action == "HELP_RECIPE" && selection_ok( current, line, false ) ) {
            uistate.read_recipes.insert( current[line]->ident() );
            recalc_unread = highlight_unread_recipes;
            ui.invalidate_ui();

            item_info_data data = result_info.get_result_data( current[line], 1, item_info_scroll_popup,
                                  w_iteminfo );
            data.handle_scrolling = true;
            draw_item_info( []() -> catacurses::window {
                const int width = std::min( TERMX, FULL_SCREEN_WIDTH );
                const int height = std::min( TERMY, FULL_SCREEN_HEIGHT );
                return catacurses::newwin( height, width, point( ( TERMX - width ) / 2, ( TERMY - height ) / 2 ) );
            }, data );
        } else if( action == "FILTER" ) {
            int max_example_length = 0;
            for( const auto &prefix : prefixes ) {
                max_example_length = std::max( max_example_length, utf8_width( prefix.example.translated() ) );
            }
            std::string spaces( max_example_length, ' ' );

            std::string description = filter_help_start.translated();

            {
                std::string example_name = _( "shirt" );
                int padding = max_example_length - utf8_width( example_name );
                description += string_format(
                                   _( "  <color_white>%s</color>%.*s    %s\n" ),
                                   example_name, padding, spaces,
                                   _( "<color_cyan>name</color> of resulting item" ) );

                std::string example_exclude = _( "clean" );
                padding = max_example_length - utf8_width( example_exclude );
                description += string_format(
                                   _( "  <color_yellow>-</color><color_white>%s</color>%.*s   %s\n" ),
                                   example_exclude, padding, spaces,
                                   _( "<color_cyan>names</color> to exclude" ) );
            }

            for( const auto &prefix : prefixes ) {
                int padding = max_example_length - utf8_width( prefix.example.translated() );
                description += string_format(
                                   _( "  <color_yellow>%c</color><color_white>:%s</color>%.*s  %s\n" ),
                                   prefix.key, prefix.example, padding, spaces, prefix.description );
            }

            description +=
                _( "\nUse <color_red>up/down arrow</color> to go through your search history." );

            string_input_popup popup;
            popup
            .title( _( "Search:" ) )
            .width( 85 )
            .description( description )
            .desc_color( c_light_gray )
            .identifier( "craft_recipe_filter" )
            .hist_use_uilist( false )
            .edit( filterstring );

            if( popup.confirmed() ) {
                recalc = true;
                recalc_unread = highlight_unread_recipes;
                if( batch ) {
                    // exit from batch selection
                    batch = false;
                    line = batch_line;
                }
            }
        } else if( action == "QUIT" ) {
            chosen = nullptr;
            done = true;
        } else if( action == "RESET_FILTER" ) {
            filterstring.clear();
            recalc = true;
            recalc_unread = highlight_unread_recipes;
        } else if( action == "CYCLE_BATCH" && selection_ok( current, line, false ) ) {
            batch = !batch;
            if( batch ) {
                batch_line = line;
                chosen = current[batch_line];
                uistate.read_recipes.insert( chosen->ident() );
                recalc_unread = highlight_unread_recipes;
            } else {
                keepline = true;
            }
            recalc = true;
        } else if( action == "TOGGLE_FAVORITE" && selection_ok( current, line, true ) ) {
            keepline = true;
            recalc = filterstring.empty() && subtab.cur() == "CSC_*_FAVORITE";
            if( uistate.favorite_recipes.find( current[line]->ident() ) != uistate.favorite_recipes.end() ) {
                uistate.favorite_recipes.erase( current[line]->ident() );
                if( recalc ) {
                    if( static_cast<size_t>( line ) + 1 < current.size() ) {
                        line++;
                    } else {
                        line--;
                    }
                }
            } else {
                uistate.favorite_recipes.insert( current[line]->ident() );
                uistate.read_recipes.insert( current[line]->ident() );
            }
            recalc_unread = highlight_unread_recipes;
        } else if( action == "HIDE_SHOW_RECIPE" && selection_ok( current, line, true ) ) {
            if( show_hidden ) {
                uistate.hidden_recipes.erase( current[line]->ident() );
            } else {
                uistate.hidden_recipes.insert( current[line]->ident() );
                uistate.read_recipes.insert( current[line]->ident() );
            }

            recalc = true;
            recalc_unread = highlight_unread_recipes;
            keepline = true;
            if( static_cast<size_t>( line ) + 1 < current.size() ) {
                line++;
            } else {
                line--;
            }
        } else if( action == "TOGGLE_RECIPE_UNREAD" && selection_ok( current, line, true ) ) {
            const recipe_id rcp = current[line]->ident();
            if( uistate.read_recipes.count( rcp ) ) {
                uistate.read_recipes.erase( rcp );
            } else {
                uistate.read_recipes.insert( rcp );
            }
            recalc_unread = highlight_unread_recipes;
            just_toggled_unread = true;
        } else if( action == "MARK_ALL_RECIPES_READ" ) {
            bool current_list_has_unread = false;
            for( const recipe *const rcp : current ) {
                if( !uistate.read_recipes.count( rcp->ident() ) ) {
                    current_list_has_unread = true;
                    break;
                }
            }
            std::string query_str;
            if( !current_list_has_unread ) {
                query_str = _( "<color_yellow>/!\\</color> Mark all recipes as read?  "
                               // NOLINTNEXTLINE(cata-text-style): single spaced for symmetry
                               "This cannot be undone. <color_yellow>/!\\</color>" );
            } else if( filterstring.empty() ) {
                query_str = string_format( _( "Mark recipes in this tab as read?  This cannot be undone.  "
                                              "You can mark all recipes by choosing yes and pressing %s again." ),
                                           ctxt.get_desc( "MARK_ALL_RECIPES_READ" ) );
            } else {
                query_str = string_format( _( "Mark filtered recipes as read?  This cannot be undone.  "
                                              "You can mark all recipes by choosing yes and pressing %s again." ),
                                           ctxt.get_desc( "MARK_ALL_RECIPES_READ" ) );
            }
            if( query_yn( query_str ) ) {
                if( current_list_has_unread ) {
                    for( const recipe *const rcp : current ) {
                        uistate.read_recipes.insert( rcp->ident() );
                    }
                } else {
                    for( const recipe *const rcp : available_recipes ) {
                        uistate.read_recipes.insert( rcp->ident() );
                    }
                }
            }
            recalc_unread = highlight_unread_recipes;
        } else if( action == "TOGGLE_UNREAD_RECIPES_FIRST" ) {
            unread_recipes_first = !unread_recipes_first;
            recalc = true;
            keepline = true;
        } else if( action == "RELATED_RECIPES" && selection_ok( current, line, false ) ) {
            uistate.read_recipes.insert( current[line]->ident() );
            recalc_unread = highlight_unread_recipes;
            ui.invalidate_ui();

            std::string recipe_name = peek_related_recipe( current[line], available_recipes );
            if( !recipe_name.empty() ) {
                filterstring = recipe_name;
                recalc = true;
                recalc_unread = highlight_unread_recipes;
            }
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Regenerate keybinding tips
            ui.mark_resize();
        }
        if( line < 0 ) {
            line = current.size() - 1;
        } else if( line >= static_cast<int>( current.size() ) ) {
            line = 0;
        }
    } while( !done );

    return chosen;
}

std::string peek_related_recipe( const recipe *current, const recipe_subset &available )
{
    auto compare_second =
        []( const std::pair<itype_id, std::string> &a,
    const std::pair<itype_id, std::string> &b ) {
        return localized_compare( a.second, b.second );
    };

    // current recipe components
    std::vector<std::pair<itype_id, std::string>> related_components;
    const requirement_data &req = current->simple_requirements();
    for( const std::vector<item_comp> &comp_list : req.get_components() ) {
        for( const item_comp &a : comp_list ) {
            related_components.emplace_back( a.type, item::nname( a.type, 1 ) );
        }
    }
    std::sort( related_components.begin(), related_components.end(), compare_second );
    // current recipe result
    std::vector<std::pair<itype_id, std::string>> related_results;
    item tmp = current->create_result();
    // use this item
    const itype_id tid = tmp.typeId();
    const std::set<const recipe *> &known_recipes =
        get_player_character().get_learned_recipes().of_component( tid );
    for( const recipe * const &b : known_recipes ) {
        if( available.contains( b ) ) {
            related_results.emplace_back( b->result(), b->result_name( /*decorated=*/true ) );
        }
    }
    std::stable_sort( related_results.begin(), related_results.end(), compare_second );

    if( related_components.empty() && related_results.empty() ) {
        return "";
    }

    uilist rel_menu;
    int np_last = -1;
    if( !related_components.empty() ) {
        rel_menu.addentry( ++np_last, false, -1, _( "COMPONENTS" ) );
    }
    np_last = related_menu_fill( rel_menu, related_components, available );
    if( !related_results.empty() ) {
        rel_menu.addentry( ++np_last, false, -1, _( "RESULTS" ) );
    }

    related_menu_fill( rel_menu, related_results, available );

    rel_menu.settext( _( "Related recipes:" ) );
    rel_menu.query();
    if( rel_menu.ret != UILIST_CANCEL ) {

        // Grab the recipe name without our bullet point.
        std::string recipe = rel_menu.entries[rel_menu.ret].txt.substr( strlen( "─ " ) );

        // If the string is decorated as a favourite, return it without the star
        if( recipe.rfind( "* ", 0 ) == 0 ) {
            return recipe.substr( strlen( "* " ) );
        }

        return recipe;
    }

    return "";
}

int related_menu_fill( uilist &rmenu,
                       const std::vector<std::pair<itype_id, std::string>> &related_recipes,
                       const recipe_subset &available )
{
    const std::vector<uilist_entry> &entries = rmenu.entries;
    int np_last = entries.empty() ? -1 : entries.back().retval;

    if( related_recipes.empty() ) {
        return np_last;
    }

    std::string recipe_name_prev;
    for( const std::pair<itype_id, std::string> &p : related_recipes ) {

        // we have different recipes with the same names
        // list only one of them as we show and filter by name only
        std::string recipe_name = p.second;
        if( recipe_name == recipe_name_prev ) {
            continue;
        }
        recipe_name_prev = recipe_name;

        std::vector<const recipe *> current_part = available.search_result( p.first );
        if( !current_part.empty() ) {

            bool different_recipes = false;

            // 1st pass: check if we need to add group
            for( size_t recipe_n = 0; recipe_n < current_part.size(); recipe_n++ ) {
                if( current_part[recipe_n]->result_name( /*decorated=*/true ) != recipe_name ) {
                    // add group
                    rmenu.addentry( ++np_last, false, -1, recipe_name );
                    different_recipes = true;
                    break;
                } else if( recipe_n == current_part.size() - 1 ) {
                    // only one result
                    rmenu.addentry( ++np_last, true, -1, "─ " + recipe_name );
                }
            }

            if( different_recipes ) {
                std::string prev_item_name;
                // 2nd pass: add different recipes
                for( size_t recipe_n = 0; recipe_n < current_part.size(); recipe_n++ ) {
                    std::string cur_item_name = current_part[recipe_n]->result_name( /*decorated=*/true );
                    if( cur_item_name != prev_item_name ) {
                        std::string sym = recipe_n == current_part.size() - 1 ? "└ " : "├ ";
                        rmenu.addentry( ++np_last, true, -1, sym + cur_item_name );
                    }
                    prev_item_name = cur_item_name;
                }
            }
        }
    }

    return np_last;
}

static bool query_is_yes( const std::string &query )
{
    const std::string subquery = query.substr( 2 );

    return subquery == "yes" || subquery == "y" || subquery == "1" ||
           subquery == "true" || subquery == "t" || subquery == "on" ||
           subquery == _( "yes" );
}

static int craft_info_width( const int window_width )
{
    int reason_width = 0;
    //The crafting speed string is necessary.  Find the longest one
    for( const auto &pair : craft_speed_reason_strings ) {
        reason_width = std::max( utf8_width( pair.second.translated(), true ), reason_width );
    }
    reason_width += 2; //Allow for borders
    //Use about a quarter of the screen if there's room to play, otherwise limit to the longest string
    return std::max( window_width / 4, reason_width );
}

static void draw_hidden_amount( const catacurses::window &w, int amount, int num_recipe )
{
    if( amount == 1 ) {
        right_print( w, 1, 1, c_red, string_format( _( "* %s hidden recipe - %s in category *" ), amount,
                     num_recipe ) );
    } else if( amount >= 2 ) {
        right_print( w, 1, 1, c_red, string_format( _( "* %s hidden recipes - %s in category *" ), amount,
                     num_recipe ) );
    } else if( amount == 0 ) {
        right_print( w, 1, 1, c_green, string_format( _( "* No hidden recipe - %s in category *" ),
                     num_recipe ) );
    }
    //Finish border connection with the recipe tabs
    mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
    mvwputch( w, point( getmaxx( w ) - 1, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OOXX ); // ^|
    wnoutrefresh( w );
}

// Anchors top-right
static void draw_can_craft_indicator( const catacurses::window &w, const recipe &rec )
{
    Character &player_character = get_player_character();

    if( player_character.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        right_print( w, 0, 1, i_red, craft_speed_reason_strings.at( TOO_DARK_TO_CRAFT ).translated() );
    } else if( player_character.crafting_speed_multiplier( rec ) <= 0.0f ) {
        right_print( w, 0, 1, i_red, craft_speed_reason_strings.at( TOO_SLOW_TO_CRAFT ).translated() );
    } else if( player_character.crafting_speed_multiplier( rec ) < 1.0f ) {
        right_print( w, 0, 1, i_yellow,
                     string_format( craft_speed_reason_strings.at( SLOW_BUT_CRAFTABLE ).translated(),
                                    static_cast<int>( player_character.crafting_speed_multiplier( rec ) * 100 ) ) );
    } else if( player_character.crafting_speed_multiplier( rec ) > 1.0f ) {
        right_print( w, 0, 1, i_green,
                     string_format( craft_speed_reason_strings.at( FAST_CRAFTING ).translated(),
                                    static_cast<int>( player_character.crafting_speed_multiplier( rec ) * 100 ) ) );
    } else {
        right_print( w, 0, 1, i_green, craft_speed_reason_strings.at( NORMAL_CRAFTING ).translated() );
    }
    wnoutrefresh( w );
}

static std::map<size_t, inclusive_rectangle<point>> draw_recipe_tabs( const catacurses::window &w,
        const tab_list &tab, TAB_MODE mode,
        const bool filtered_unread, std::map<std::string, bool> &unread )
{
    werase( w );
    std::map<size_t, inclusive_rectangle<point>> tab_map;

    switch( mode ) {
        case NORMAL: {
            std::vector<std::string> translated_cats;
            translated_cats.reserve( craft_cat_list.size() );
            for( const std::string &cat : craft_cat_list ) {
                if( unread[ cat ] ) {
                    translated_cats.emplace_back( _( get_cat_unprefixed(
                                                         cat ) ).append( "<color_light_green>⁺</color>" ) );
                } else {
                    translated_cats.emplace_back( _( get_cat_unprefixed( cat ) ) );
                }
            }
            std::pair<std::vector<std::string>, size_t> fitted_tabs = fit_tabs_to_width( getmaxx( w ),
                    tab.cur_index(), translated_cats );
            tab_map = draw_tabs( w, fitted_tabs.first, tab.cur_index() - fitted_tabs.second,
                                 fitted_tabs.second );
            break;
        }
        case FILTERED: {
            mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
            mvwputch( w, point( 0, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OXXO ); // |^
            const std::string tab_name = _( "Searched" );
            draw_tab( w, 2, tab_name, true );
            if( filtered_unread ) {
                mvwprintz( w, point( 3 + utf8_width( tab_name ), 1 ), c_light_green, "⁺" );
            }
            break;
        }
        case BATCH:
            mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
            mvwputch( w, point( 0, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OXXO ); // |^
            draw_tab( w, 2, _( "Batch" ), true );
            break;
    }
    //draw_tabs will produce a border ending with // ^| but that's inappropriate here, so clean it up
    mvwputch( w, point( getmaxx( w ) - 1, 2 ), BORDER_COLOR, LINE_OXOX ); //_
    wnoutrefresh( w );
    return tab_map;
}

static std::map<size_t, inclusive_rectangle<point>> draw_recipe_subtabs(
            const catacurses::window &w, const std::string &tab,
            const size_t subtab,
            const recipe_subset &available_recipes, TAB_MODE mode,
            std::map<std::string, bool> &unread )
{
    werase( w );
    std::map<size_t, inclusive_rectangle<point>> subtab_map;
    int width = getmaxx( w );

    mvwvline( w, point_zero, LINE_XOXO, getmaxy( w ) );  // |
    mvwvline( w, point( width - 1, 0 ), LINE_XOXO, getmaxy( w ) );  // |

    switch( mode ) {
        case NORMAL: {
            std::vector<std::string> translated_subcats;
            std::vector<bool> empty_subcats;
            std::vector<bool> unread_subcats;
            translated_subcats.reserve( craft_subcat_list[tab].size() );
            empty_subcats.reserve( craft_subcat_list[tab].size() );
            unread_subcats.reserve( craft_subcat_list[tab].size() );
            for( const std::string &subcat : craft_subcat_list[tab] ) {
                translated_subcats.emplace_back( _( get_subcat_unprefixed( tab, subcat ) ) );
                empty_subcats.emplace_back( available_recipes.empty_category( tab,
                                            subcat != "CSC_ALL" ? subcat : "" ) );
                unread_subcats.emplace_back( unread[subcat] );
            }
            std::pair<std::vector<std::string>, size_t> fitted_subcat_list = fit_tabs_to_width( getmaxx( w ),
                    subtab, translated_subcats );
            size_t offset = fitted_subcat_list.second;
            if( fitted_subcat_list.first.size() + offset > craft_subcat_list[tab].size() ) {
                break;
            }
            // Draw the tabs on each other
            int pos_x = 2;
            // Step between tabs, two for tabs border
            int tab_step = 3;
            for( size_t i = 0; i < fitted_subcat_list.first.size(); ++i ) {
                if( empty_subcats[i + offset] ) {
                    draw_subtab( w, pos_x, fitted_subcat_list.first[i], subtab == i + offset, true,
                                 empty_subcats[i + offset] );
                } else {
                    subtab_map.emplace( i + offset, draw_subtab( w, pos_x, fitted_subcat_list.first[i],
                                        subtab == i + offset, true, empty_subcats[i + offset] ) );
                }
                pos_x += utf8_width( fitted_subcat_list.first[i] ) + tab_step;
                if( unread_subcats[i + offset] ) {
                    mvwprintz( w, point( pos_x - 2, 0 ), c_light_green, "⁺" );
                }
            }
            break;
        }
        case FILTERED:
        case BATCH:
            werase( w );
            for( int i = 0; i < 3; i++ ) {
                mvwputch( w, point( 0, i ), BORDER_COLOR, LINE_XOXO ); // |
                mvwputch( w, point( width - 1, i ), BORDER_COLOR, LINE_XOXO ); // |
            }
            break;
    }

    wnoutrefresh( w );
    return subtab_map;
}

const std::vector<std::string> *subcategories_for_category( const std::string &category )
{
    auto it = craft_subcat_list.find( category );
    if( it != craft_subcat_list.end() ) {
        return &it->second;
    }
    return nullptr;
}
