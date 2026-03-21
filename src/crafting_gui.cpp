#include "crafting_gui.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "crafting.h"
#include "crafting_gui_helpers.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "debug.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "game_inventory.h"
#include "generic_factory.h"
#include "input.h"
#include "input_context.h"
#include "input_enums.h"
#include "input_popup.h"
#include "inventory.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_location.h"
#include "localized_comparator.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "popup.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "string_formatter.h"
#include "translation.h"
#include "translation_cache.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "ui_iteminfo.h"
#include "ui_manager.h"
#include "uistate.h"

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
    {SLOW_BUT_CRAFTABLE, to_translation( "crafting is slowed to %d%%: %s" )},
    {FAST_CRAFTING, to_translation( "crafting is accelerated to %d%%: %s" )},
    {NORMAL_CRAFTING, to_translation( "craftable" )}
};

namespace
{

generic_factory<crafting_category> craft_cat_list( "recipe_category" );

} // namespace

template<>
const crafting_category &string_id<crafting_category>::obj() const
{
    return craft_cat_list.obj( *this );
}

template<>
bool string_id<crafting_category>::is_valid() const
{
    return craft_cat_list.is_valid( *this );
}

static void draw_hidden_amount( const catacurses::window &w, int amount, int num_recipe );
static void draw_can_craft_indicator( const catacurses::window &w, const recipe &rec,
                                      Character &crafter );
static std::map<size_t, inclusive_rectangle<point>> draw_recipe_tabs( const catacurses::window &w,
        const tab_list &tab, TAB_MODE mode,
        bool filtered_unread, std::map<std::string, bool> &unread );
static std::map<size_t, inclusive_rectangle<point>> draw_recipe_subtabs(
            const catacurses::window &w, const std::string &tab,
            size_t subtab,
            const recipe_subset &available_recipes, TAB_MODE mode,
            std::map<std::string, bool> &unread );
/**
 * return index of newly chosen crafter.
 * return < 0 if error happens or nobody is choosen.
 */
static int choose_crafter( const std::vector<Character *> &crafting_group, int crafter_i,
                           const recipe *rec, bool rec_valid );

static std::string peek_related_recipe( const recipe *current, const recipe_subset &available,
                                        Character &crafter );
static int related_menu_fill( uilist &rmenu,
                              const std::vector<std::pair<itype_id, std::string>> &related_recipes,
                              const recipe_subset &available );
static void compare_recipe_with_item( const item &recipe_item, Character &crafter );
static void prioritize_components( const recipe &recipe, Character &crafter );
static void deprioritize_components( const recipe &recipe );

static std::string get_cat_unprefixed( std::string_view prefixed_name )
{
    return std::string( prefixed_name.substr( 3, prefixed_name.size() - 3 ) );
}

void load_recipe_category( const JsonObject &jsobj, const std::string &src )
{
    craft_cat_list.load( jsobj, src );
}

void crafting_category::finalize_all()
{
    craft_cat_list.finalize();
}

void crafting_category::load( const JsonObject &jo, std::string_view )
{
    // Ensure id is correct
    if( id.str().find( "CC_" ) != 0 ) {
        jo.throw_error( "Crafting category id has to be prefixed with 'CC_'" );
    }

    optional( jo, was_loaded, "is_hidden", is_hidden, false );
    optional( jo, was_loaded, "is_practice", is_practice, false );
    optional( jo, was_loaded, "is_building", is_building, false );
    optional( jo, was_loaded, "is_wildcard", is_wildcard, false );
    mandatory( jo, was_loaded, "recipe_subcategories", subcategories,
               auto_flags_reader<> {} );

    // Ensure subcategory ids are correct and remove dupes
    std::string cat_name = get_cat_unprefixed( id.str() );
    std::unordered_set<std::string> known;
    for( auto it = subcategories.begin(); it != subcategories.end(); ) {
        const std::string &subcat_id = *it;
        if( subcat_id.find( "CSC_" + cat_name + "_" ) != 0 && subcat_id != "CSC_ALL" ) {
            jo.throw_error( "Crafting sub-category id has to be prefixed with CSC_<category_name>_" );
        }
        if( known.find( subcat_id ) != known.end() ) {
            it = subcategories.erase( it );
            continue;
        }
        known.emplace( subcat_id );
        ++it;
    }
}

static std::string get_subcat_unprefixed( std::string_view cat,
        const std::string &prefixed_name )
{
    std::string prefix = "CSC_" + get_cat_unprefixed( cat ) + "_";

    if( prefixed_name.find( prefix ) == 0 ) {
        return prefixed_name.substr( prefix.size(), prefixed_name.size() - prefix.size() );
    }

    return prefixed_name == "CSC_ALL" ? translate_marker( "ALL" ) : translate_marker( "NONCRAFT" );
}

void reset_recipe_categories()
{
    craft_cat_list.reset();
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
    ctxt.register_action( "CHOOSE_CRAFTER" );
    ctxt.register_action( "RELATED_RECIPES" );
    ctxt.register_action( "HIDE_SHOW_RECIPE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "COMPARE" );
    ctxt.register_action( "PRIORITIZE_MISSING_COMPONENTS" );
    ctxt.register_action( "DEPRIORITIZE_COMPONENTS" );
    if( highlight_unread_recipes ) {
        ctxt.register_action( "TOGGLE_RECIPE_UNREAD" );
        ctxt.register_action( "MARK_ALL_RECIPES_READ" );
        ctxt.register_action( "TOGGLE_UNREAD_RECIPES_FIRST" );
    }
    return ctxt;
}

class recipe_result_info_cache
{
        Character &crafter;
        std::vector<iteminfo> info;
        const recipe *last_recipe = nullptr;
        int last_panel_width = 0;
        int cached_batch_size = 0;
        int lang_version = 0;
    public:
        explicit recipe_result_info_cache( Character &c ) : crafter( c ) {}
        item_info_data get_result_data( const recipe *rec, int batch_size,
                                        int &scroll_pos, int panel_width ) {
            if( lang_version == detail::get_current_language_version()
                && rec == last_recipe && rec != nullptr
                && panel_width == last_panel_width
                && batch_size == cached_batch_size ) {
                return item_info_data( "", "", info, {}, scroll_pos );
            }
            lang_version = detail::get_current_language_version();
            last_recipe = rec;
            last_panel_width = panel_width;
            cached_batch_size = batch_size;
            scroll_pos = 0;
            info = recipe_result_info( *rec, crafter, batch_size, panel_width );
            return item_info_data( "", "", info, {}, scroll_pos );
        }
};

std::pair<std::vector<const recipe *>, bool> recipes_from_cat( const recipe_subset
        &available_recipes, const crafting_category_id &cat, const std::string &subcat )
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
    character_id guy_id;
    std::string qry_comps;
    int batch_size;
    int fold_width;
    std::vector<std::string> text;
};

static const std::vector<std::string> &cached_recipe_info( recipe_info_cache &info_cache,
        const recipe &recp, const availability &avail, Character &guy, const std::string &qry_comps,
        const int batch_size, const int fold_width, const nc_color &color,
        const std::vector<Character *> &crafting_group )
{
    static int lang_version = detail::get_current_language_version();

    if( info_cache.recp != &recp ||
        info_cache.guy_id != guy.getID() ||
        info_cache.qry_comps != qry_comps ||
        info_cache.batch_size != batch_size ||
        info_cache.fold_width != fold_width ||
        lang_version != detail::get_current_language_version()
      ) {
        info_cache.recp = &recp;
        info_cache.guy_id = guy.getID();
        info_cache.qry_comps = qry_comps;
        info_cache.batch_size = batch_size;
        info_cache.fold_width = fold_width;
        info_cache.text = recipe_info( recp, avail, guy, qry_comps, batch_size, fold_width, color,
                                       crafting_group );
        lang_version = detail::get_current_language_version();
    }
    return info_cache.text;
}

struct item_info_cache {
    const recipe *last_recipe = nullptr;
    item dummy;
};


static bool mouse_in_window( std::optional<point> coord, const catacurses::window &w_ )
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
    if( list.empty() ) {
        popup( _( "Nothing selected!" ) );
    } else if( list[current_line]->is_nested() && !nested_acceptable ) {
        popup( _( "Select a recipe within this group" ) );
    } else {
        return true;
    }
    return false;
}

// returns false if the popup was cancelled
static bool filter_crafting_recipes( std::string &filterstring )
{
    int max_example_length = 0;
    for( const SearchPrefix &prefix : prefixes ) {
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

    for( const SearchPrefix &prefix : prefixes ) {
        int padding = max_example_length - utf8_width( prefix.example.translated() );
        description += string_format(
                           _( "  <color_yellow>%c</color><color_white>:%s</color>%.*s  %s\n" ),
                           prefix.key, prefix.example, padding, spaces, prefix.description );
    }

    description +=
        _( "\nUse <color_red>up/down arrow</color> to go through your search history." );

    string_input_popup_imgui popup( 85, filterstring );
    popup.set_label( _( "Search:" ) );
    popup.set_description( description, c_light_gray, /*monofont=*/ true );
    popup.set_identifier( "craft_recipe_filter" );
    popup.use_uilist_history( false );
    filterstring = popup.query();

    return !popup.cancelled();
}

std::pair<Character *, const recipe *> select_crafter_and_crafting_recipe( int &batch_size_out,
        const recipe_id &goto_recipe, Character *crafter, std::string filterstring, bool camp_crafting,
        inventory *inventory_override )
{
    if( crafter == nullptr ) {
        return {nullptr, nullptr};
    }
    recipe_result_info_cache result_info( *crafter );
    recipe_info_cache r_info_cache;
    int line_recipe_info = 0;
    int line_item_info = 0;
    int line_item_info_popup = 0;
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
        const unsigned int header_info_width = std::max( width / 4, width - FULL_SCREEN_WIDTH - 1 );
        const int wStart = ( TERMX - width ) / 2;

        std::vector<std::string> act_descs;
        const auto add_action_desc = [&]( const std::string & act, const std::string & txt ) {
            act_descs.emplace_back(
                ctxt.get_desc( act, txt, input_context::allow_all_keys ) );
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
        add_action_desc( "CHOOSE_CRAFTER", pgettext( "crafting gui", "Choose crafter" ) );
        add_action_desc( "PRIORITIZE_MISSING_COMPONENTS", pgettext( "crafting gui", "Prioritize" ) );
        add_action_desc( "DEPRIORITIZE_COMPONENTS", pgettext( "crafting gui", "Deprioritize" ) );
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
    std::vector<std::string> crafting_categories;
    crafting_categories.reserve( craft_cat_list.size() );
    for( const crafting_category &cat : craft_cat_list.get_all() ) {
        if( cat.is_hidden ) {
            continue;
        }
        crafting_categories.emplace_back( cat.id.str() );
    }
    tab_list tab( crafting_categories );
    tab_list subtab( crafting_category_id( tab.cur() )->subcategories );
    std::map<size_t, inclusive_rectangle<point>> translated_tab_map;
    std::map<size_t, inclusive_rectangle<point>> translated_subtab_map;
    std::map<size_t, inclusive_rectangle<point>> list_map;
    // List of all recipes to show, to choose from
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

    const std::vector<Character *> crafting_group = crafter->get_crafting_group();
    int crafter_i = find( crafting_group.begin(), crafting_group.end(),
                          crafter ) - crafting_group.begin();

    // Get everyone's recipes
    const recipe_subset &available_recipes = crafter->get_group_available_recipes( inventory_override );
    std::map<character_id, std::map<const recipe *, availability>> guy_availability_cache;
    // next line also inserts empty cache for crafter->getID()
    std::map<const recipe *, availability> *availability_cache =
        &guy_availability_cache[crafter->getID()];

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
            if( gotorec != gotocat.end() && ( *gotorec )->category.is_valid() ) {
                while( tab.cur() != goto_recipe->category.str() ) {
                    tab.next();
                }
                subtab = tab_list( crafting_category_id( tab.cur() )->subcategories );
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
                for( const std::string &cat : crafting_categories ) {
                    is_cat_unread[cat] = false;
                    for( const std::string &subcat : crafting_category_id( cat )->subcategories ) {
                        is_subcat_unread[cat][subcat] = false;
                        const std::pair<std::vector<const recipe *>, bool> result = recipes_from_cat( available_recipes,
                                crafting_category_id( cat ), subcat );
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
        wattron( w_data, BORDER_COLOR );
        mvwhline( w_data, point( 1, dataHeight - 1 ), LINE_OXOX, width - 2 );
        mvwvline( w_data, point::zero, LINE_XOXO, dataHeight - 1 );
        mvwvline( w_data, point( width - 1, 0 ), LINE_XOXO, dataHeight - 1 );
        mvwaddch( w_data, point( 0, dataHeight - 1 ), LINE_XXOO ); // |_
        mvwaddch( w_data, point( width - 1, dataHeight - 1 ), LINE_XOOX ); // _|
        wattroff( w_data, BORDER_COLOR );

        const int max_recipe_name_width = 27;
        const int recmax = current.size();
        // pair<int, int>
        const auto& [istart, iend] = subindex_around_cursor( recmax, dataLines, line );
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

            draw_can_craft_indicator( w_head_info, recp, *crafter );

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
                                                   recp, avail, *crafter, qry_comps, batch_size, fold_width, color, crafting_group );

            const int total_lines = info.size();
            line_recipe_info = clamp( line_recipe_info, 0, total_lines - dataLines );
            for( int i = line_recipe_info; i < std::min( line_recipe_info + dataLines, total_lines ); ++i ) {
                nc_color dummy = color;
                print_colored_text( w_data, point( xpos, i - line_recipe_info ), dummy, color, info[i] );
            }

            if( total_lines > dataLines ) {
                scrollbar()
                .offset_x( xpos + fold_width + 1 )
                .content_size( total_lines )
                .viewport_pos( line_recipe_info )
                .viewport_size( dataLines )
                .apply( w_data );
            }
        }

        scrollbar()
        .offset_x( 0 )
        .offset_y( 0 )
        .content_size( recmax )
        .viewport_pos( istart )
        .viewport_size( dataLines )
        .apply( w_data );

        wnoutrefresh( w_data );

        if( isWide && !current.empty() ) {
            const recipe *cur_recipe = current[line];
            werase( w_iteminfo );
            if( cur_recipe->is_practice() ) {
                const std::string desc = practice_recipe_description( *cur_recipe, *crafter );
                fold_and_print( w_iteminfo, point::zero, item_info_width, c_light_gray, desc );
                scrollbar().offset_x( item_info_width - 1 ).offset_y( 0 ).content_size( 1 ).viewport_size( getmaxy(
                            w_iteminfo ) ).apply( w_iteminfo );
                wnoutrefresh( w_iteminfo );
            } else if( cur_recipe->is_nested() ) {
                std::string desc = cur_recipe->get_description( *crafter ) + "\n\n";
                desc += list_nested( *crafter, cur_recipe, available_recipes );
                fold_and_print( w_iteminfo, point::zero, item_info_width, c_light_gray, desc );
                scrollbar().offset_x( item_info_width - 1 ).offset_y( 0 ).content_size( 1 ).viewport_size( getmaxy(
                            w_iteminfo ) ).apply( w_iteminfo );
                wnoutrefresh( w_iteminfo );
            } else {
                item_info_data data = result_info.get_result_data( cur_recipe, batch_size, line_item_info,
                                      getmaxx( w_iteminfo ) );
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
            if( keepline && line >= 0 && static_cast<size_t>( line ) < current.size() ) {
                prev_rcp = current[line];
            }

            show_hidden = false;
            available.clear();

            if( batch ) {
                current.clear();
                for( int i = 1; i <= 50; i++ ) {
                    current.push_back( chosen );
                    available.emplace_back( *crafter, chosen, i, camp_crafting, inventory_override );
                }
                indent.assign( current.size(), 0 );
            } else {
                static_popup popup;
                std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();
                static constexpr std::chrono::milliseconds update_interval( 500 );
                input_context dummy;
                dummy.register_action( "QUIT" );
                std::string cancel_btn = dummy.get_button_text( "QUIT", _( "Cancel" ) );
                std::function<void( size_t, size_t )> progress_callback =
                [&]( size_t at, size_t out_of ) {
                    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
                    if( now - last_update < update_interval ) {
                        return;
                    }
                    last_update = now;
                    double percent = 100.0 * at / out_of;
                    popup.message( _( "Searching… %3.0f%%\n%s\n" ), percent, cancel_btn );
                    ui_manager::redraw();
                    refresh_display();
                    inp_mngr.pump_events();
                };

                std::vector<const recipe *> picking;
                bool skip_hidden = false;
                if( !filterstring.empty() ) {
                    std::string qry = trim( filterstring );
                    recipe_subset filtered_recipes =
                        filter_recipes( available_recipes, qry, *crafter, progress_callback );
                    picking.insert( picking.end(), filtered_recipes.begin(), filtered_recipes.end() );
                } else {
                    const std::pair<std::vector<const recipe *>, bool> result = recipes_from_cat( available_recipes,
                            crafting_category_id( tab.cur() ), subtab.cur() );
                    picking = result.first;
                    skip_hidden = result.second;
                    show_hidden = result.second;
                }

                const bool skip_sort = ( subtab.cur() == "CSC_*_RECENT" );
                num_recipe = picking.size();
                recipe_list_data list_result = build_recipe_list(
                                                   std::move( picking ), skip_hidden, skip_sort,
                                                   *crafter, camp_crafting, inventory_override,
                                                   highlight_unread_recipes, unread_recipes_first,
                                                   *availability_cache, available_recipes );
                current = std::move( list_result.entries );
                indent = std::move( list_result.indent );
                available = std::move( list_result.available );
                num_hidden = list_result.num_hidden;
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

        std::optional<point> coord = ctxt.get_coordinates_text( catacurses::stdscr );
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
                        subtab = tab_list( crafting_category_id( tab.cur() )->subcategories );
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
            line_recipe_info -= dataLines;
        } else if( action == "SCROLL_UP" && mouse_in_recipe ) {
            --line_recipe_info;
        } else if( action == "SCROLL_RECIPE_INFO_DOWN" ) {
            line_recipe_info += dataLines;
        } else if( action == "SCROLL_DOWN" && mouse_in_recipe ) {
            ++line_recipe_info;
        } else if( action == "LEFT" || ( action == "SCROLL_UP" && mouse_in_window( coord, w_subhead ) ) ) {
            if( batch || !filterstring.empty() ) {
                continue;
            }
            std::string start = subtab.cur();
            do {
                subtab.prev();
            } while( subtab.cur() != start &&
                     available_recipes.empty_category( crafting_category_id( tab.cur() ),
                             subtab.cur() != "CSC_ALL" ? subtab.cur() : "" ) );
            recalc = true;
        } else if( action == "SCROLL_ITEM_INFO_UP" ) {
            line_item_info -= scroll_item_info_lines;
        } else if( action == "SCROLL_UP" && mouse_in_window( coord, w_iteminfo ) ) {
            --line_item_info;
        } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
            line_item_info += scroll_item_info_lines;
        } else if( action == "SCROLL_DOWN" && mouse_in_window( coord, w_iteminfo ) ) {
            ++line_item_info;
        } else if( action == "PREV_TAB" || ( action == "SCROLL_UP" &&
                                             mouse_in_window( coord, w_head_tabs ) ) ) {
            tab.prev();
            // Default ALL
            subtab = tab_list( crafting_category_id( tab.cur() )->subcategories );
            recalc = true;
        } else if( action == "RIGHT" || ( action == "SCROLL_DOWN" &&
                                          mouse_in_window( coord, w_subhead ) ) ) {
            if( batch || !filterstring.empty() ) {
                continue;
            }
            std::string start = subtab.cur();
            do {
                subtab.next();
            } while( subtab.cur() != start &&
                     available_recipes.empty_category( crafting_category_id( tab.cur() ),
                             subtab.cur() != "CSC_ALL" ? subtab.cur() : "" ) );
            recalc = true;
        } else if( action == "NEXT_TAB" || ( action == "SCROLL_DOWN" &&
                                             mouse_in_window( coord, w_head_tabs ) ) ) {
            tab.next();
            // Default ALL
            subtab = tab_list( crafting_category_id( tab.cur() )->subcategories );
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
        } else if( action == "PAGE_UP" || action == "PAGE_DOWN" ) {
            line = inc_clamp( line, action == "PAGE_UP" ? -scroll_rate : scroll_rate, recmax );
        } else if( action == "HOME" ) {
            line = 0;
            user_moved_line = highlight_unread_recipes;
        } else if( action == "END" ) {
            line = -1;
            user_moved_line = highlight_unread_recipes;
        } else if( action == "CONFIRM" ) {
            if( available.empty() ) {
                popup( _( "Nothing selected!" ) );
            } else if( current[line]->is_nested() ) {
                nested_toggle( current[line]->ident(), recalc, keepline );
            } else {
                const int bs = batch ? line + 1 : 1;
                craft_confirm_result confirm = can_start_craft(
                                                   *current[line], available[line], *crafter );
                switch( confirm ) {
                    case craft_confirm_result::cannot_craft:
                        popup( _( "Crafter can't craft that!" ) );
                        break;
                    case craft_confirm_result::too_dark:
                        popup( _( "Crafter can't see!" ) );
                        break;
                    case craft_confirm_result::ok:
                        if( available[line].inv_override == nullptr &&
                            !crafter->check_eligible_containers_for_crafting( *current[line], bs ) ) {
                            break; // popup already inside check
                        }
                        chosen = current[line];
                        batch_size_out = bs;
                        done = true;
                        uistate.read_recipes.insert( chosen->ident() );
                        break;
                }
            }
        } else if( action == "HELP_RECIPE" && selection_ok( current, line, false ) ) {
            uistate.read_recipes.insert( current[line]->ident() );
            recalc_unread = highlight_unread_recipes;
            ui.invalidate_ui();

            item_info_data data = result_info.get_result_data( current[line], 1, line_item_info_popup,
                                  getmaxx( w_iteminfo ) );
            data.handle_scrolling = true;
            data.arrow_scrolling = true;
            const int info_width = std::min( TERMX, FULL_SCREEN_WIDTH );
            const int info_height = std::min( TERMY, FULL_SCREEN_HEIGHT );
            iteminfo_window info_window( data, point( ( TERMX - info_width ) / 2, ( TERMY - info_height ) / 2 ),
                                         info_width, info_height );
            info_window.execute();
        } else if( action == "FILTER" ) {
            if( filter_crafting_recipes( filterstring ) ) {
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
        } else if( action == "CHOOSE_CRAFTER" ) {
            // allow for switching crafter when no recipes are shown (e.g. filter)
            bool rec_valid = !current.empty();
            const recipe *rec = rec_valid ? current[line] : nullptr;
            int new_crafter_i = choose_crafter( crafting_group, crafter_i, rec, rec_valid );
            if( new_crafter_i >= 0 && new_crafter_i != crafter_i ) {
                crafter_i = new_crafter_i;
                crafter = crafting_group[crafter_i];
                // next line also inserts empty cache for crafter->getID() if non existant
                availability_cache = &guy_availability_cache[crafter->getID()];
                recalc = true;
                keepline = true;
            }
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
                for( const recipe_id nested_rcp : rcp->nested_category_data ) {
                    uistate.read_recipes.erase( nested_rcp );
                }
                uistate.read_recipes.erase( rcp );
            } else {
                for( const recipe_id nested_rcp : rcp->nested_category_data ) {
                    uistate.read_recipes.insert( nested_rcp );
                }
                uistate.read_recipes.insert( rcp );
            }
            recalc_unread = highlight_unread_recipes;
            just_toggled_unread = true;
        } else if( action == "MARK_ALL_RECIPES_READ" ) {
            bool current_list_has_unread = false;
            for( const recipe *const rcp : current ) {
                for( const recipe_id nested_rcp : rcp->nested_category_data ) {
                    if( !uistate.read_recipes.count( nested_rcp->ident() ) ) {
                        current_list_has_unread = true;
                        break;
                    }
                    if( current_list_has_unread ) {
                        break;
                    }
                }
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
                        for( const recipe_id nested_rcp : rcp->nested_category_data ) {
                            uistate.read_recipes.insert( nested_rcp->ident() );
                        }
                        uistate.read_recipes.insert( rcp->ident() );
                    }
                } else {
                    for( const recipe *const rcp : available_recipes ) {
                        for( const recipe_id nested_rcp : rcp->nested_category_data ) {
                            uistate.read_recipes.insert( nested_rcp->ident() );
                        }
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

            std::string recipe_name = peek_related_recipe( current[line], available_recipes, *crafter );
            if( !recipe_name.empty() ) {
                filterstring = recipe_name;
                recalc = true;
                recalc_unread = highlight_unread_recipes;
            }
        } else if( action == "COMPARE" && selection_ok( current, line, false ) ) {
            const item recipe_result = get_recipe_result_item( *current[line], *crafter );
            compare_recipe_with_item( recipe_result, *crafter );
        } else if( action == "PRIORITIZE_MISSING_COMPONENTS" && selection_ok( current, line, false ) ) {
            uistate.read_recipes.insert( current[line]->ident() );
            recalc_unread = highlight_unread_recipes;
            ui.invalidate_ui();

            prioritize_components( *current[line], *crafter );
        } else if( action == "DEPRIORITIZE_COMPONENTS" ) {
            uistate.read_recipes.insert( current[line]->ident() );
            recalc_unread = highlight_unread_recipes;
            ui.invalidate_ui();

            deprioritize_components( *current[line] );
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

    return { crafter, chosen };
}

int choose_crafter( const std::vector<Character *> &crafting_group, int crafter_i,
                    const recipe *rec, bool rec_valid )
{
    std::vector<std::string> header = { _( "Crafter" ) };
    if( rec_valid ) {
        header.emplace_back( rec->is_practice() ? _( "Can practice" ) : _( "Can craft" ) );
        header.emplace_back( _( "Missing" ) );
    }
    header.emplace_back( _( "Status" ) );
    uimenu choose_char_menu( static_cast<int>( header.size() ) );
    choose_char_menu.set_title( _( "Choose the crafter" ) );
    choose_char_menu.addentry( -1, false, header );

    int i = 0;
    for( Character *chara : crafting_group ) {
        std::vector<std::string> entry = { chara->name_and_maybe_activity() };
        if( rec_valid ) {
            availability avail = availability( *chara, rec );
            std::vector<std::string> reasons;

            bool has_stuff = rec->deduped_requirements().can_make_with_inventory(
                                 chara->crafting_inventory(), rec->get_component_filter( recipe_filter_flags::none ), 1,
                                 craft_flags::start_only );
            if( !has_stuff ) {
                reasons.emplace_back( _( "stuff" ) );
            }
            if( !avail.crafter_has_primary_skill ) {
                reasons.emplace_back( _( "skill" ) );
            }
            if( !avail.has_proficiencies ) {  // this is required proficiency
                reasons.emplace_back( _( "proficiency" ) );
            }
            if( chara->lighting_craft_speed_multiplier( *rec ) <= 0.0f ) {
                reasons.emplace_back( _( "light" ) );
            }
            std::string dummy;
            if( chara->is_npc() && !rec->npc_can_craft( dummy ) ) {
                reasons.emplace_back( _( "is NPC" ) );
            }

            entry.emplace_back(
                // *INDENT-OFF* readable ternary operator
                rec->is_nested()
                    ? colorize( "-", c_yellow )
                    : reasons.empty()
                        ? colorize( _( "yes" ), c_green )
                        : colorize( _( "no" ), c_red ) );
                // *INDENT-ON* readable ternary operator
            entry.emplace_back( colorize( string_join( reasons, ", " ), c_red ) );
        }
        entry.emplace_back( chara->in_sleep_state()
                            ? colorize( _( "asleep" ), c_red )
                            : colorize( _( "awake" ), c_green ) );
        choose_char_menu.addentry( i, !chara->in_sleep_state(), entry );
        ++i;
    }

    choose_char_menu.set_selected( crafter_i + 1 );  // +1 for header entry
    return choose_char_menu.query();
}

std::string peek_related_recipe( const recipe *current, const recipe_subset &available,
                                 Character &crafter )
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
    item tmp( current->result() );
    // use this item
    const itype_id tid = tmp.typeId();
    const std::set<const recipe *> &known_recipes =
        crafter.get_learned_recipes().of_component( tid );
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

        std::vector<const recipe *> current_part = available.recipes_that_produce( p.first );
        if( current_part.empty() ) {
            continue;
        }

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
        if( !different_recipes ) {
            continue;
        }
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

    return np_last;
}

static void compare_recipe_with_item( const item &recipe_item, Character &crafter )
{
    inventory_pick_selector inv_s( crafter );

    inv_s.add_character_items( crafter );
    inv_s.set_title( _( "Compare" ) );
    inv_s.set_hint( _( "Select item to compare with." ) );

    if( inv_s.empty() ) {
        popup( std::string( _( "There are no items to compare." ) ), PF_GET_KEY );
        return;
    }

    do {
        const item_location to_compare = inv_s.execute();
        if( !to_compare ) {
            break;
        }
        game_menus::inv::compare_item_menu menu( recipe_item, *to_compare );
        menu.show();
    } while( true );
}

static void prioritize_components( const recipe &recipe, Character &crafter )
{
    int new_filters_count = 0;
    std::string added_filters;
    const requirement_data &req = recipe.simple_requirements();
    const inventory &crafting_inv = crafter.crafting_inventory();
    for( const std::vector<item_comp> &comp_list : req.get_components() ) {
        for( const item_comp &i_comp : comp_list ) {
            std::string nname = item::nname( i_comp.type, 1 );
            int filter_pos = uistate.list_item_priority.find( nname );
            bool enough_materials = req.check_enough_materials(
                                        i_comp, crafting_inv, recipe.get_component_filter(), 1
                                    );
            if( filter_pos == -1 && !enough_materials ) {
                new_filters_count++;
                added_filters += nname;
                added_filters += ",";
            }
        }
    }

    if( new_filters_count > 0 ) {
        if( !uistate.list_item_priority.empty()
            && !string_ends_with( uistate.list_item_priority, "," ) ) {
            uistate.list_item_priority += ",";
        }
        uistate.list_item_priority += added_filters;
        uistate.list_item_priority_active = true;
        std::vector<std::string> &hist = uistate.gethistory( "list_item_priority" );
        if( hist.empty() || hist[hist.size() - 1] != uistate.list_item_priority ) {
            hist.push_back( uistate.list_item_priority );
        }

        popup( string_format( _( "Added %d components to the priority filter.\nAdded: %s\nNew Filter: %s" ),
                              new_filters_count, added_filters, uistate.list_item_priority ) );
    } else {
        popup( string_format( _( "Did not find anything to add to the priority filter.\n\nFilter: %s" ),
                              uistate.list_item_priority ) );
    }
}

static void deprioritize_components( const recipe &recipe )
{
    int removed_filters_count = 0;
    std::string removed_filters;
    const requirement_data &req = recipe.simple_requirements();
    for( const std::vector<item_comp> &comp_list : req.get_components() ) {
        for( const item_comp &i_comp : comp_list ) {
            std::string nname = item::nname( i_comp.type, 1 );
            nname += ",";
            if( string_starts_with( uistate.list_item_priority, nname ) ) {
                removed_filters_count++;
                removed_filters += nname;
                uistate.list_item_priority.erase( 0, nname.length() );
            } else {
                std::string find_string = "," + nname;
                int filter_pos = uistate.list_item_priority.find( find_string );
                if( filter_pos > -1 ) {
                    removed_filters_count++;
                    removed_filters += nname;
                    uistate.list_item_priority.replace( filter_pos, find_string.length(), "," );
                }
            }
        }
    }

    if( !uistate.list_item_priority.empty() ) {
        uistate.list_item_priority_active = true;

        std::vector<std::string> &hist = uistate.gethistory( "list_item_priority" );
        if( hist.empty() || hist[hist.size() - 1] != uistate.list_item_priority ) {
            hist.push_back( uistate.list_item_priority );
        }
    } else {
        uistate.list_item_priority_active = false;
    }

    if( removed_filters_count > 0 ) {
        popup( string_format(
                   _( "Removed %d components from the priority filter.\nRemoved: %s\nNew Filter: %s" ),
                   removed_filters_count, removed_filters, uistate.list_item_priority ) );
    } else {
        popup( string_format( _( "Did not find anything to remove from the priority filter.\nFilter: %s" ),
                              uistate.list_item_priority ) );
    }
}


static void draw_hidden_amount( const catacurses::window &w, int amount, int num_recipe )
{
    if( amount == 1 ) {
        right_print( w, 1, 1, c_red, string_format( _( "* %d hidden recipe - %d in category *" ), amount,
                     num_recipe ) );
    } else if( amount >= 2 ) {
        right_print( w, 1, 1, c_red, string_format( _( "* %d hidden recipes - %d in category *" ), amount,
                     num_recipe ) );
    } else if( amount == 0 ) {
        right_print( w, 1, 1, c_green, string_format( _( "* No hidden recipe - %d in category *" ),
                     num_recipe ) );
    }
    //Finish border connection with the recipe tabs
    wattron( w, BORDER_COLOR );
    mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
    mvwaddch( w, point( getmaxx( w ) - 1, getmaxy( w ) - 1 ), LINE_OOXX ); // ^|
    wattroff( w, BORDER_COLOR );
    wnoutrefresh( w );
}

// Anchors top-right
static void draw_can_craft_indicator( const catacurses::window &w, const recipe &rec,
                                      Character &crafter )
{
    int limb_modifier = crafter.limb_score_crafting_speed_multiplier( rec ) * 100;
    int mut_multi = crafter.mut_crafting_speed_multiplier( rec ) * 100;

    std::stringstream modifiers_list;
    if( limb_modifier != 100 ) {
        if( limb_modifier < 100 ) {
            modifiers_list << _( "hands encumbrance/wounds" ) << " " << limb_modifier << "%";
        } else {
            modifiers_list << _( "extra manipulators" ) << " " << limb_modifier << "%";
        }
    }
    if( mut_multi != 100 ) {
        if( !modifiers_list.str().empty() ) {
            modifiers_list << ", ";
        }
        modifiers_list << _( "traits" ) << " " << mut_multi << "%";
    }

    if( crafter.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        right_print( w, 0, 1, i_red, craft_speed_reason_strings.at( TOO_DARK_TO_CRAFT ).translated() );
    } else if( crafter.crafting_speed_multiplier( rec ) <= 0.0f ) {
        right_print( w, 0, 1, i_red, craft_speed_reason_strings.at( TOO_SLOW_TO_CRAFT ).translated() );
    } else if( crafter.crafting_speed_multiplier( rec ) < 1.0f ) {
        int morale_modifier = crafter.morale_crafting_speed_multiplier( rec ) * 100;
        int lighting_modifier = crafter.lighting_craft_speed_multiplier( rec ) * 100;
        const int pain_multi = crafter.pain_crafting_speed_multiplier( rec ) * 100;

        if( morale_modifier < 100 ) {
            if( !modifiers_list.str().empty() ) {
                modifiers_list << ", ";
            }
            modifiers_list << _( "morale" ) << " " << morale_modifier << "%";
        }
        if( lighting_modifier < 100 ) {
            if( !modifiers_list.str().empty() ) {
                modifiers_list << ", ";
            }
            modifiers_list << _( "lighting" ) << " " << lighting_modifier << "%";
        }
        if( pain_multi < 100 ) {
            if( !modifiers_list.str().empty() ) {
                modifiers_list << ", ";
            }
            modifiers_list << _( "pain" ) << " " << pain_multi << "%";
        }

        right_print( w, 0, 1, i_yellow,
                     string_format( craft_speed_reason_strings.at( SLOW_BUT_CRAFTABLE ).translated(),
                                    static_cast<int>( crafter.crafting_speed_multiplier( rec ) * 100 ),
                                    modifiers_list.str() ) );
    } else if( crafter.crafting_speed_multiplier( rec ) > 1.0f ) {
        right_print( w, 0, 1, i_green,
                     string_format( craft_speed_reason_strings.at( FAST_CRAFTING ).translated(),
                                    static_cast<int>( crafter.crafting_speed_multiplier( rec ) * 100 ),
                                    modifiers_list.str() ) );
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
            for( const crafting_category &cat : craft_cat_list.get_all() ) {
                if( cat.is_hidden ) {
                    continue;
                }
                if( unread[ cat.id.str() ] ) {
                    translated_cats.emplace_back( _( get_cat_unprefixed(
                                                         cat.id.str() ) ).append( "<color_light_green>⁺</color>" ) );
                } else {
                    translated_cats.emplace_back( _( get_cat_unprefixed( cat.id.str() ) ) );
                }
            }
            std::pair<std::vector<std::string>, size_t> fitted_tabs = fit_tabs_to_width( getmaxx( w ),
                    tab.cur_index(), translated_cats );
            tab_map = draw_tabs( w, fitted_tabs.first, tab.cur_index() - fitted_tabs.second,
                                 fitted_tabs.second );
            break;
        }
        case FILTERED: {
            wattron( w, BORDER_COLOR );
            mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
            mvwaddch( w, point( 0, getmaxy( w ) - 1 ), LINE_OXXO ); // ┌
            wattroff( w, BORDER_COLOR );
            std::string tab_name = _( "Searched" );
            if( filtered_unread ) {
                tab_name += " ";  // space for green "+"
            }
            draw_tab( w, 2, tab_name, true );
            if( filtered_unread ) {
                mvwprintz( w, point( 2 + utf8_width( tab_name ), 1 ), c_light_green, "⁺" );
            }
            break;
        }
        case BATCH:
            wattron( w, BORDER_COLOR );
            mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
            mvwaddch( w, point( 0, getmaxy( w ) - 1 ), LINE_OXXO ); // ┌
            wattroff( w, BORDER_COLOR );
            draw_tab( w, 2, _( "Batch" ), true );
            break;
    }
    //draw_tabs will produce a border ending with ┐ but that's inappropriate here, so clean it up
    mvwputch( w, point( getmaxx( w ) - 1, 2 ), BORDER_COLOR, LINE_OXOX );  // ─
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

    wattron( w, BORDER_COLOR );
    mvwvline( w, point::zero, LINE_XOXO, getmaxy( w ) );  // |
    mvwvline( w, point( width - 1, 0 ), LINE_XOXO, getmaxy( w ) );  // |
    wattroff( w, BORDER_COLOR );

    switch( mode ) {
        case NORMAL: {
            std::vector<std::string> translated_subcats;
            std::vector<bool> empty_subcats;
            std::vector<bool> unread_subcats;
            crafting_category_id current_cat = crafting_category_id( tab );
            size_t subcats_count = current_cat->subcategories.size();
            translated_subcats.reserve( subcats_count );
            empty_subcats.reserve( subcats_count );
            unread_subcats.reserve( subcats_count );
            for( const std::string &subcat : current_cat->subcategories ) {
                translated_subcats.emplace_back( _( get_subcat_unprefixed( tab, subcat ) ) );
                empty_subcats.emplace_back( available_recipes.empty_category(
                                                crafting_category_id( tab ),
                                                subcat != "CSC_ALL" ? subcat : "" ) );
                unread_subcats.emplace_back( unread[subcat] );
            }
            std::pair<std::vector<std::string>, size_t> fitted_subcat_list = fit_tabs_to_width( getmaxx( w ),
                    subtab, translated_subcats );
            size_t offset = fitted_subcat_list.second;
            if( fitted_subcat_list.first.size() + offset > subcats_count ) {
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
            wattron( w, BORDER_COLOR );
            mvwvline( w, point::zero, LINE_XOXO, 3 ); // |
            mvwvline( w, point( width - 1, 0 ), LINE_XOXO, 3 ); // |
            wattroff( w, BORDER_COLOR );
            break;
    }

    wnoutrefresh( w );
    return subtab_map;
}

const std::vector<std::string> *subcategories_for_category( const std::string &category )
{
    crafting_category_id cat( category );
    if( !cat.is_valid() ) {
        return nullptr;
    }
    return &cat->subcategories;
}
