#include "crafting_gui.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
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
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "optional.h"
#include "output.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "uistate.h"

static const std::string flag_BLIND_EASY( "BLIND_EASY" );
static const std::string flag_BLIND_HARD( "BLIND_HARD" );

class inventory;
class npc;

enum TAB_MODE {
    NORMAL,
    FILTERED,
    BATCH
};

// TODO: Convert these globals to handling categories via generic_factory?
static std::vector<std::string> craft_cat_list;
static std::map<std::string, std::vector<std::string> > craft_subcat_list;
static std::map<std::string, std::string> normalized_names;

static bool query_is_yes( const std::string &query );
static void draw_hidden_amount( const catacurses::window &w, int amount, int num_recipe );
static void draw_can_craft_indicator( const catacurses::window &w, const recipe &rec );
static void draw_recipe_tabs( const catacurses::window &w, const std::string &tab,
                              TAB_MODE mode = NORMAL );
static void draw_recipe_subtabs( const catacurses::window &w, const std::string &tab,
                                 const std::string &subtab,
                                 const recipe_subset &available_recipes, TAB_MODE mode = NORMAL );

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

    if( !is_hidden ) {
        craft_cat_list.push_back( category );
    }

    const std::string cat_name = get_cat_unprefixed( category );

    craft_subcat_list[category].clear();
    for( const std::string subcat_id : jsobj.get_array( "recipe_subcategories" ) ) {
        if( subcat_id.find( "CSC_" + cat_name + "_" ) != 0 && subcat_id != "CSC_ALL" ) {
            jsobj.throw_error( "Crafting sub-category id has to be prefixed with CSC_<category_name>_" );
        }
        craft_subcat_list[category].push_back( subcat_id );
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

static void translate_all()
{
    normalized_names.clear();
    for( const auto &cat : craft_cat_list ) {
        normalized_names[cat] = _( get_cat_unprefixed( cat ) );

        for( const auto &subcat : craft_subcat_list[cat] ) {
            normalized_names[subcat] = _( get_subcat_unprefixed( cat, subcat ) );
        }
    }
}

void reset_recipe_categories()
{
    craft_cat_list.clear();
    craft_subcat_list.clear();
}

static int print_items( const recipe &r, const catacurses::window &w, point pos,
                        nc_color col, int batch )
{
    if( !r.has_byproducts() ) {
        return 0;
    }

    const int oldy = pos.y;

    mvwprintz( w, point( pos.x, pos.y++ ), col, _( "Byproducts:" ) );
    for( const auto &bp : r.byproducts ) {
        const itype *t = item::find_type( bp.first );
        int amount = bp.second * batch;
        std::string desc;
        if( t->count_by_charges() ) {
            amount *= t->charges_default();
            desc = string_format( "> %s (%d)", t->nname( 1 ), amount );
        } else {
            desc = string_format( "> %d %s", amount,
                                  t->nname( static_cast<unsigned int>( amount ) ) );
        }
        mvwprintz( w, point( pos.x, pos.y++ ), col, desc );
    }

    return pos.y - oldy;
}

const recipe *select_crafting_recipe( int &batch_size )
{
    struct {
        const recipe *last_recipe = nullptr;
        item dummy;
    } item_info_cache;
    int item_info_scroll = 0;
    int item_info_scroll_popup = 0;

    const auto item_info_data_from_recipe =
    [&]( const recipe * rec, const int count, int &scroll_pos ) {
        if( item_info_cache.last_recipe != rec ) {
            item_info_cache.last_recipe = rec;
            item_info_cache.dummy = rec->create_result();
            item_info_cache.dummy.set_var( "recipe_exemplar", rec->ident().str() );
            item_info_scroll = 0;
            item_info_scroll_popup = 0;
        }
        std::vector<iteminfo> info;
        item_info_cache.dummy.info( true, info, count );
        item_info_data data( item_info_cache.dummy.tname( count ),
                             item_info_cache.dummy.type_name( count ),
                             info, {}, scroll_pos );
        return data;
    };

    // always re-translate the category names in case the language has changed
    translate_all();

    const int headHeight = 3;
    const int subHeadHeight = 2;

    bool isWide = false;
    int width = 0;
    int dataLines = 0;
    int dataHalfLines = 0;
    int dataHeight = 0;
    int componentPrintHeight = 0;
    int item_info_width = 0;
    catacurses::window w_head;
    catacurses::window w_subhead;
    catacurses::window w_data;
    catacurses::window w_iteminfo;
    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
        isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );

        width = isWide ? ( freeWidth > FULL_SCREEN_WIDTH ? FULL_SCREEN_WIDTH * 2 : TERMX ) :
                FULL_SCREEN_WIDTH;
        const int wStart = ( TERMX - width ) / 2;
        const int tailHeight = isWide ? 3 : 4;
        dataLines = TERMY - ( headHeight + subHeadHeight ) - tailHeight;
        dataHalfLines = dataLines / 2;
        dataHeight = TERMY - ( headHeight + subHeadHeight );
        componentPrintHeight = dataHeight - tailHeight - 1;

        w_head = catacurses::newwin( headHeight, width, point( wStart, 0 ) );
        w_subhead = catacurses::newwin( subHeadHeight, width, point( wStart, 3 ) );
        w_data = catacurses::newwin( dataHeight, width, point( wStart,
                                     headHeight + subHeadHeight ) );

        if( isWide ) {
            item_info_width = width - FULL_SCREEN_WIDTH - 2;
            const int item_info_height = dataHeight - 3;
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

    list_circularizer<std::string> tab( craft_cat_list );
    list_circularizer<std::string> subtab( craft_subcat_list[tab.cur()] );
    std::vector<const recipe *> current;
    struct availability {
        availability( const recipe *r, int batch_size = 1 ) {
            Character &player = get_player_character();
            const inventory &inv = player.crafting_inventory();
            auto all_items_filter = r->get_component_filter( recipe_filter_flags::none );
            auto no_rotten_filter = r->get_component_filter( recipe_filter_flags::no_rotten );
            const deduped_requirement_data &req = r->deduped_requirements();
            has_proficiencies = r->character_has_required_proficiencies( player );
            can_craft = req.can_make_with_inventory(
                            inv, all_items_filter, batch_size, craft_flags::start_only ) && has_proficiencies;
            can_craft_non_rotten = req.can_make_with_inventory(
                                       inv, no_rotten_filter, batch_size, craft_flags::start_only );
            const requirement_data &simple_req = r->simple_requirements();
            apparently_craftable = simple_req.can_make_with_inventory(
                                       inv, all_items_filter, batch_size, craft_flags::start_only );
            proficiency_maluses = r->proficiency_maluses( player );
            has_all_skills = r->skill_used.is_null() ||
                             player.get_skill_level( r->skill_used ) >= r->difficulty;
            for( const std::pair<const skill_id, int> &e : r->required_skills ) {
                if( player.get_skill_level( e.first ) < e.second ) {
                    has_all_skills = false;
                    break;
                }
            }
        }
        bool can_craft;
        bool can_craft_non_rotten;
        bool apparently_craftable;
        bool has_proficiencies;
        bool has_all_skills;
        float proficiency_maluses;

        nc_color selected_color() const {
            if( !can_craft ) {
                return h_dark_gray;
            } else if( !can_craft_non_rotten ) {
                return has_all_skills ? h_brown : h_red;
            } else {
                return has_all_skills ? h_white : h_yellow;
            }
        }

        nc_color color( bool ignore_missing_skills = false ) const {
            if( !can_craft ) {
                return c_dark_gray;
            } else if( !can_craft_non_rotten ) {
                return has_all_skills || ignore_missing_skills ? c_brown : c_red;
            } else {
                return has_all_skills || ignore_missing_skills ? c_white : c_yellow;
            }
        }
    };
    std::vector<availability> available;
    //preserves component color printout between mode rotations
    nc_color rotated_color = c_white;
    int previous_item_line = -1;
    std::string previous_tab;
    std::string previous_subtab;
    int line = 0;
    bool recalc = true;
    bool keepline = false;
    bool done = false;
    bool batch = false;
    bool show_hidden = false;
    size_t num_hidden = 0;
    int num_recipe = 0;
    int batch_line = 0;
    int display_mode = 0;
    const recipe *chosen = nullptr;

    input_context ctxt( "CRAFTING" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "CYCLE_MODE" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
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

    Character &player_character = get_player_character();
    const inventory &crafting_inv = player_character.crafting_inventory();
    const std::vector<npc *> helpers = player_character.get_crafting_helpers();
    std::string filterstring;

    const auto &available_recipes = player_character.get_available_recipes( crafting_inv, &helpers );
    std::map<const recipe *, availability> availability_cache;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        const TAB_MODE m = ( batch ) ? BATCH : ( filterstring.empty() ) ? NORMAL : FILTERED;
        draw_recipe_tabs( w_head, tab.cur(), m );
        draw_recipe_subtabs( w_subhead, tab.cur(), subtab.cur(), available_recipes, m );

        if( !show_hidden ) {
            draw_hidden_amount( w_head, num_hidden, num_recipe );
        }

        // Clear the screen of recipe data, and draw it anew
        werase( w_data );

        if( isWide ) {
            if( !filterstring.empty() ) {
                fold_and_print( w_data, point( 5, dataLines + 1 ), 0, c_white,
                                _( "Press [<color_yellow>ENTER</color>] to attempt to craft object.  "
                                   "D[<color_yellow>e</color>]scribe, [<color_yellow>F</color>]ind, "
                                   "[<color_red>R</color>]eset, [<color_yellow>m</color>]ode, "
                                   "[<color_yellow>s</color>]how/hide, Re[<color_yellow>L</color>]ated, "
                                   "[<color_yellow>*</color>]Favorite, %s, [<color_yellow>?</color>]keybindings" ),
                                ( batch ) ? _( "<color_red>cancel</color> "
                                               "[<color_yellow>b</color>]atch" ) : _( "[<color_yellow>b</color>]atch" ) );
            } else {
                fold_and_print( w_data, point( 5, dataLines + 1 ), 0, c_white,
                                _( "Press [<color_yellow>ENTER</color>] to attempt to craft object.  "
                                   "D[<color_yellow>e</color>]scribe, [<color_yellow>F</color>]ind, "
                                   "[<color_yellow>m</color>]ode, [<color_yellow>s</color>]how/hide, "
                                   "Re[<color_yellow>L</color>]ated, [<color_yellow>*</color>]Favorite, "
                                   "%s, [<color_yellow>?</color>]keybindings" ),
                                ( batch ) ? _( "<color_red>cancel</color> "
                                               "[<color_yellow>b</color>]atch" ) : _( "[<color_yellow>b</color>]atch" ) );
            }
        } else {
            if( !filterstring.empty() ) {
                mvwprintz( w_data, point( 2, dataLines + 2 ), c_white,
                           _( "[F]ind, [R]eset, [m]ode, [s]how/hide, Re[L]ated, [*]Fav, [b]atch." ) );
            } else {
                mvwprintz( w_data, point( 2, dataLines + 2 ), c_white,
                           _( "[F]ind, [m]ode, [s]how/hide, Re[L]ated, [*]Fav, [b]atch." ) );
            }
            mvwprintz( w_data, point( 2, dataLines + 1 ), c_white,
                       _( "Press [ENTER] to attempt to craft object.  D[e]scribe, [?]keybindings," ) );
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

        cata::optional<point> cursor_pos;
        int recmin = 0, recmax = current.size();
        if( recmax > dataLines ) {
            if( line <= recmin + dataHalfLines ) {
                for( int i = recmin; i < recmin + dataLines; ++i ) {
                    std::string tmp_name = current[i]->result_name();
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name );
                    }
                    mvwprintz( w_data, point( 2, i - recmin ), c_dark_gray, "" ); // Clear the line
                    const bool highlight = i == line;
                    const nc_color col = highlight ? available[i].selected_color() : available[i].color();
                    const point print_from( 2, i - recmin );
                    if( highlight ) {
                        cursor_pos = print_from;
                    }
                    mvwprintz( w_data, print_from, col, utf8_truncate( tmp_name, 28 ) );
                }
            } else if( line >= recmax - dataHalfLines ) {
                for( int i = recmax - dataLines; i < recmax; ++i ) {
                    std::string tmp_name = current[i]->result_name();
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name );
                    }
                    mvwprintz( w_data, point( 2, dataLines + i - recmax ), c_light_gray, "" ); // Clear the line
                    const bool highlight = i == line;
                    const nc_color col = highlight ? available[i].selected_color() : available[i].color();
                    const point print_from( 2, dataLines + i - recmax );
                    if( highlight ) {
                        cursor_pos = print_from;
                    }
                    mvwprintz( w_data, print_from, col,
                               utf8_truncate( tmp_name, 28 ) );
                }
            } else {
                for( int i = line - dataHalfLines; i < line - dataHalfLines + dataLines; ++i ) {
                    std::string tmp_name = current[i]->result_name();
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name );
                    }
                    mvwprintz( w_data, point( 2, dataHalfLines + i - line ), c_light_gray, "" ); // Clear the line
                    const bool highlight = i == line;
                    const nc_color col = highlight ? available[i].selected_color() : available[i].color();
                    const point print_from( 2, dataHalfLines + i - line );
                    if( highlight ) {
                        cursor_pos = print_from;
                    }
                    mvwprintz( w_data, print_from, col,
                               utf8_truncate( tmp_name, 28 ) );
                }
            }
        } else {
            for( int i = 0; i < static_cast<int>( current.size() ) && i < dataHeight + 1; ++i ) {
                std::string tmp_name = current[i]->result_name();
                if( batch ) {
                    tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name );
                }
                const bool highlight = i == line;
                const nc_color col = highlight ? available[i].selected_color() : available[i].color();
                const point print_from( 2, i );
                if( highlight ) {
                    cursor_pos = print_from;
                }
                mvwprintz( w_data, print_from, col, utf8_truncate( tmp_name, 28 ) );
            }
        }

        const int count = batch ? line + 1 : 1; // batch size
        if( !current.empty() ) {
            int pane = FULL_SCREEN_WIDTH - 30 - 1;
            nc_color col = available[line].color( true );

            const auto &req = current[line]->simple_requirements();

            draw_can_craft_indicator( w_head, *current[line] );
            wnoutrefresh( w_head );

            int ypos = 0;

            auto qry = trim( filterstring );
            std::string qry_comps;
            if( qry.compare( 0, 2, "c:" ) == 0 ) {
                qry_comps = qry.substr( 2 );
            }

            std::vector<std::string> component_print_buffer;
            auto tools = req.get_folded_tools_list( pane, col, crafting_inv, count );
            auto comps = req.get_folded_components_list( pane, col, crafting_inv,
                         current[line]->get_component_filter(), count, qry_comps );
            component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
            component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

            if( !player_character.knows_recipe( current[line] ) ) {
                component_print_buffer.push_back( _( "Recipe not memorized yet" ) );
                auto books_with_recipe = player_character.get_books_for_recipe( crafting_inv, current[line] );
                std::string enumerated_books =
                    enumerate_as_string( books_with_recipe.begin(), books_with_recipe.end(),
                []( const itype_id & type_id ) {
                    return colorize( item::nname( type_id ), c_cyan );
                } );
                const std::string text = string_format( _( "Written in: %s" ), enumerated_books );
                std::vector<std::string> folded_lines = foldstring( text, pane );
                component_print_buffer.insert(
                    component_print_buffer.end(), folded_lines.begin(), folded_lines.end() );
            }

            //handle positioning of component list if it needed to be scrolled
            int componentPrintOffset = 0;
            if( display_mode > 1 ) {
                componentPrintOffset = ( display_mode - 1 ) * componentPrintHeight;
            }
            if( component_print_buffer.size() < static_cast<size_t>( componentPrintOffset ) ) {
                componentPrintOffset = 0;
                if( previous_tab != tab.cur() || previous_subtab != subtab.cur() || previous_item_line != line ) {
                    display_mode = 1;
                } else {
                    display_mode = 0;
                }
            }

            //only used to preserve mode position on components when
            //moving to another item and the view is already scrolled
            previous_tab = tab.cur();
            previous_subtab = subtab.cur();
            previous_item_line = line;
            const int xpos = 30;

            if( display_mode == 0 ) {
                print_colored_text(
                    w_data, point( xpos, ypos++ ), col, col,
                    string_format( _( "Primary skill: %s" ),
                                   current[line]->primary_skill_string( &player_character, false ) ) );

                ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                        _( "Other skills: %s" ),
                                        current[line]->required_skills_string( &player_character, false, false ) );

                ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col, _( "Proficiencies Required: %s" ),
                                        current[line]->required_proficiencies_string( &get_player_character() ) );

                ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col, _( "Proficiencies Used: %s" ),
                                        current[line]->used_proficiencies_string( &get_player_character() ) );

                const int expected_turns = player_character.expected_time_to_craft( *current[line],
                                           count ) / to_moves<int>( 1_turns );
                ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                        _( "Time to complete: <color_cyan>%s</color>" ),
                                        to_string( time_duration::from_turns( expected_turns ) ) );

                ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                        _( "Batch time savings: <color_cyan>%s</color>" ),
                                        current[line]->batch_savings_string() );

                const int makes = current[line]->makes_amount();
                if( makes > 1 ) {
                    ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                            _( "Recipe makes: <color_cyan>%d</color>" ),
                                            makes );
                }

                print_colored_text(
                    w_data, point( xpos, ypos++ ), col, col,
                    string_format( _( "Dark craftable?  <color_cyan>%s</color>" ),
                                   current[line]->has_flag( flag_BLIND_EASY ) ? _( "Easy" ) :
                                   current[line]->has_flag( flag_BLIND_HARD ) ? _( "Hard" ) :
                                   _( "Impossible" ) ) );

                std::string nearby_string;
                const int nearby_amount = crafting_inv.count_item( current[line]->result() );

                if( nearby_amount == 0 ) {
                    nearby_string = "<color_light_gray>0</color>";
                } else if( nearby_amount > 9000 ) {
                    // at some point you get too many to count at a glance and just know you have a lot
                    nearby_string = _( "<color_red>It's Over 9000!!!</color>" );
                } else {
                    nearby_string = string_format( "<color_yellow>%d</color>", nearby_amount );
                }
                ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                        _( "Nearby: %s" ), nearby_string );

                const bool can_craft_this = available[line].can_craft;
                if( can_craft_this && !available[line].can_craft_non_rotten ) {
                    ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                            _( "<color_red>Will use rotten ingredients</color>" ) );
                }
                const bool too_complex = current[line]->deduped_requirements().is_too_complex();
                if( can_craft_this && too_complex ) {
                    ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                            _( "Due to the complex overlapping requirements, this "
                                               "recipe <color_yellow>may appear to be craftable "
                                               "when it is not</color>." ) );
                }
                if( !can_craft_this && available[line].apparently_craftable && available[line].has_proficiencies ) {
                    ypos += fold_and_print(
                                w_data, point( xpos, ypos ), pane, col,
                                _( "<color_red>Cannot be crafted because the same item is needed "
                                   "for multiple components</color>" ) );
                }
                float maluses = available[line].proficiency_maluses;
                if( maluses != 1.0 ) {
                    std::string msg = string_format( _( "<color_yellow>This recipe will take %g%% of the normal time "
                                                        "because you lack some of the proficiencies used." ), maluses * 100 );
                    ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col, msg );
                }
                if( !can_craft_this && !available[line].has_proficiencies ) {
                    ypos += fold_and_print( w_data, point( xpos, ypos ), pane, col,
                                            _( "<color_red>Cannot be crafted because you lack"
                                               " the required proficiencies.</color>" ) );
                }
                ypos += print_items( *current[line], w_data, point( xpos, ypos ), col, batch ? line + 1 : 1 );
            }

            //color needs to be preserved in case part of the previous page was cut off
            nc_color stored_color = col;
            if( display_mode > 1 ) {
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
                print_colored_text( w_data, point( xpos, ypos++ ), stored_color, col, component_print_buffer[i] );
            }

            if( ypos >= componentPrintHeight &&
                component_print_buffer.size() > static_cast<size_t>( components_printed ) ) {
                mvwprintz( w_data, point( xpos, ypos++ ), col,
                           _( "v (%s for more)" ),
                           ctxt.press_x( "CYCLE_MODE" ) );
                rotated_color = stored_color;
            }
        }

        draw_scrollbar( w_data, line, dataLines, recmax, point_zero );
        wnoutrefresh( w_data );

        if( isWide && !current.empty() ) {
            item_info_data data = item_info_data_from_recipe( current[line], count, item_info_scroll );
            data.without_getch = true;
            data.without_border = true;
            data.scrollbar_left = false;
            data.use_full_win = true;
            draw_item_info( w_iteminfo, data );
        }

        if( cursor_pos ) {
            // place the cursor at the selected item name as expected by screen readers
            wmove( w_data, cursor_pos.value() );
            wnoutrefresh( w_data );
        }
    } );

    do {
        if( recalc ) {
            // When we switch tabs, redraw the header
            recalc = false;
            if( !keepline ) {
                line = 0;
            } else {
                keepline = false;
            }

            if( display_mode > 1 ) {
                display_mode = 1;
            }

            show_hidden = false;
            available.clear();

            if( batch ) {
                current.clear();
                for( int i = 1; i <= 20; i++ ) {
                    current.push_back( chosen );
                    available.push_back( availability( chosen, i ) );
                }
            } else {
                std::vector<const recipe *> picking;
                if( !filterstring.empty() ) {
                    auto qry = trim( filterstring );
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
                                                       recipe_subset::search_type::tool );
                                    break;

                                case 'c':
                                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                                       recipe_subset::search_type::component );
                                    break;

                                case 's':
                                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                                       recipe_subset::search_type::skill );
                                    break;

                                case 'p':
                                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                                       recipe_subset::search_type::primary_skill );
                                    break;

                                case 'Q':
                                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                                       recipe_subset::search_type::quality );
                                    break;

                                case 'q':
                                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                                       recipe_subset::search_type::quality_result );
                                    break;

                                case 'd':
                                    filtered_recipes = filtered_recipes.reduce( qry_filter_str.substr( 2 ),
                                                       recipe_subset::search_type::description_result );
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
                                                       recipe_subset::search_type::proficiency );
                                    break;

                                default:
                                    current.clear();
                            }
                        } else {
                            filtered_recipes = filtered_recipes.reduce( qry_filter_str );
                        }

                        qry_begin = qry_end + 1;
                    } while( qry_end != std::string::npos );
                    picking.insert( picking.end(), filtered_recipes.begin(), filtered_recipes.end() );
                } else if( subtab.cur() == "CSC_*_FAVORITE" ) {
                    picking = available_recipes.favorite();
                } else if( subtab.cur() == "CSC_*_RECENT" ) {
                    picking = available_recipes.recent();
                } else if( subtab.cur() == "CSC_*_HIDDEN" ) {
                    current = available_recipes.hidden();
                    show_hidden = true;
                } else {
                    picking = available_recipes.in_category( tab.cur(), subtab.cur() != "CSC_ALL" ? subtab.cur() : "" );
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
                    std::stable_sort( current.begin(), current.end(),
                    []( const recipe * a, const recipe * b ) {
                        return b->difficulty < a->difficulty;
                    } );

                    std::stable_sort( current.begin(), current.end(),
                    [&]( const recipe * a, const recipe * b ) {
                        return availability_cache.at( a ).can_craft &&
                               !availability_cache.at( b ).can_craft;
                    } );
                }

                std::transform( current.begin(), current.end(),
                std::back_inserter( available ), [&]( const recipe * e ) {
                    return availability_cache.at( e );
                } );
            }

            // current/available have been rebuilt, make sure our cursor is still in range
            if( current.empty() ) {
                line = 0;
            } else {
                line = std::min( line, static_cast<int>( current.size() ) - 1 );
            }
        }

        ui_manager::redraw();
        const int scroll_item_info_lines = catacurses::getmaxy( w_iteminfo ) - 4;
        const std::string action = ctxt.handle_input();
        if( action == "CYCLE_MODE" ) {
            display_mode = display_mode + 1;
            if( display_mode <= 0 ) {
                display_mode = 0;
            }
        } else if( action == "LEFT" ) {
            std::string start = subtab.cur();
            do {
                subtab.prev();
            } while( subtab.cur() != start && available_recipes.empty_category( tab.cur(),
                     subtab.cur() != "CSC_ALL" ? subtab.cur() : "" ) );
            recalc = true;
        } else if( action == "SCROLL_UP" ) {
            item_info_scroll -= scroll_item_info_lines;
        } else if( action == "SCROLL_DOWN" ) {
            item_info_scroll += scroll_item_info_lines;
        } else if( action == "PREV_TAB" ) {
            tab.prev();
            // Default ALL
            subtab = list_circularizer<std::string>( craft_subcat_list[tab.cur()] );
            recalc = true;
        } else if( action == "RIGHT" ) {
            std::string start = subtab.cur();
            do {
                subtab.next();
            } while( subtab.cur() != start && available_recipes.empty_category( tab.cur(),
                     subtab.cur() != "CSC_ALL" ? subtab.cur() : "" ) );
            recalc = true;
        } else if( action == "NEXT_TAB" ) {
            tab.next();
            // Default ALL
            subtab = list_circularizer<std::string>( craft_subcat_list[tab.cur()] );
            recalc = true;
        } else if( action == "DOWN" ) {
            line++;
        } else if( action == "UP" ) {
            line--;
        } else if( action == "CONFIRM" ) {
            if( available.empty() || !available[line].can_craft ) {
                popup( _( "You can't do that!  Press [<color_yellow>ESC</color>]!" ) );
            } else if( !player_character.check_eligible_containers_for_crafting( *current[line],
                       ( batch ) ? line + 1 : 1 ) ) {
                // popup is already inside check
            } else {
                chosen = current[line];
                batch_size = ( batch ) ? line + 1 : 1;
                done = true;
            }
        } else if( action == "HELP_RECIPE" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!  Press [<color_yellow>ESC</color>]!" ) );
                recalc = true;
                continue;
            }
            item_info_data data = item_info_data_from_recipe( current[line], 1, item_info_scroll_popup );
            data.handle_scrolling = true;
            draw_item_info( []() -> catacurses::window {
                const int width = std::min( TERMX, FULL_SCREEN_WIDTH );
                const int height = std::min( TERMY, FULL_SCREEN_HEIGHT );
                return catacurses::newwin( height, width, point( ( TERMX - width ) / 2, ( TERMY - height ) / 2 ) );
            }, data );

            recalc = true;
            keepline = true;
        } else if( action == "FILTER" ) {
            struct SearchPrefix {
                char key;
                std::string example;
                std::string description;
            };
            std::vector<SearchPrefix> prefixes = {
                //~ Example result description search term
                { 'q', _( "metal sawing" ), _( "<color_cyan>quality</color> of resulting item" ) },
                { 'd', _( "reach attack" ), _( "<color_cyan>full description</color> of resulting item (slow)" ) },
                { 'c', _( "two by four" ), _( "<color_cyan>component</color> required to craft" ) },
                { 'p', _( "tailoring" ), _( "<color_cyan>primary skill</color> used to craft" ) },
                { 's', _( "cooking" ), _( "<color_cyan>any skill</color> used to craft" ) },
                { 'Q', _( "fine bolt turning" ), _( "<color_cyan>quality</color> required to craft" ) },
                { 't', _( "soldering iron" ), _( "<color_cyan>tool</color> required to craft" ) },
                { 'm', _( "yes" ), _( "recipes which are <color_cyan>memorized</color> or not" ) },
                { 'P', _( "Blacksmithing" ), _( "<color_cyan>proficiency</color> used to craft" ) },
            };
            int max_example_length = 0;
            for( const auto &prefix : prefixes ) {
                max_example_length = std::max( max_example_length, utf8_width( prefix.example ) );
            }
            std::string spaces( max_example_length, ' ' );

            std::string description =
                _( "The default is to search result names.  Some single-character prefixes "
                   "can be used with a colon (:) to search in other ways.  Additional filters "
                   "are separated by commas (,).\n"
                   "\n"
                   "<color_white>Examples:</color>\n" );

            {
                std::string example_name = _( "shirt" );
                int padding = max_example_length - utf8_width( example_name );
                description += string_format(
                                   _( "  <color_white>%s</color>%.*s    %s\n" ),
                                   example_name, padding, spaces,
                                   _( "<color_cyan>name</color> of resulting item" ) );
            }

            for( const auto &prefix : prefixes ) {
                int padding = max_example_length - utf8_width( prefix.example );
                description += string_format(
                                   _( "  <color_yellow>%c</color><color_white>:%s</color>%.*s  %s\n" ),
                                   prefix.key, prefix.example, padding, spaces, prefix.description );
            }

            description +=
                _( "\nYou can use <color_white>arrow keys</color> to go through search history\n\n" );

            string_input_popup()
            .title( _( "Search:" ) )
            .width( 85 )
            .description( description )
            .desc_color( c_light_gray )
            .identifier( "craft_recipe_filter" )
            .hist_use_uilist( false )
            .edit( filterstring );
            recalc = true;
        } else if( action == "QUIT" ) {
            chosen = nullptr;
            done = true;
        } else if( action == "RESET_FILTER" ) {
            filterstring.clear();
            recalc = true;
        } else if( action == "CYCLE_BATCH" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!  Press [<color_yellow>ESC</color>]!" ) );
                recalc = true;
                continue;
            }
            batch = !batch;
            if( batch ) {
                batch_line = line;
                chosen = current[batch_line];
            } else {
                line = batch_line;
                keepline = true;
            }
            recalc = true;
        } else if( action == "TOGGLE_FAVORITE" ) {
            keepline = true;
            recalc = true;
            if( current.empty() ) {
                popup( _( "Nothing selected!  Press [<color_yellow>ESC</color>]!" ) );
                continue;
            }
            if( uistate.favorite_recipes.find( current[line]->ident() ) != uistate.favorite_recipes.end() ) {
                uistate.favorite_recipes.erase( current[line]->ident() );
            } else {
                uistate.favorite_recipes.insert( current[line]->ident() );
            }
        } else if( action == "HIDE_SHOW_RECIPE" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!  Press [<color_yellow>ESC</color>]!" ) );
                recalc = true;
                continue;
            }
            if( show_hidden ) {
                uistate.hidden_recipes.erase( current[line]->ident() );
            } else {
                uistate.hidden_recipes.insert( current[line]->ident() );
            }

            recalc = true;
        } else if( action == "RELATED_RECIPES" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!  Press [<color_yellow>ESC</color>]!" ) );
                recalc = true;
                continue;
            }
            std::string recipe_name = peek_related_recipe( current[line], available_recipes );
            if( recipe_name.empty() ) {
                keepline = true;
            } else {
                filterstring = recipe_name;
            }

            recalc = true;
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
            related_components.push_back( { a.type, item::nname( a.type, 1 ) } );
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
    for( const auto &b : known_recipes ) {
        if( available.contains( b ) ) {
            related_results.push_back( { b->result(), b->result_name() } );
        }
    }
    std::stable_sort( related_results.begin(), related_results.end(), compare_second );

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
        return rel_menu.entries[rel_menu.ret].txt.substr( strlen( "─ " ) );
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

            bool defferent_recipes = false;

            // 1st pass: check if we need to add group
            for( size_t recipe_n = 0; recipe_n < current_part.size(); recipe_n++ ) {
                if( current_part[recipe_n]->result_name() != recipe_name ) {
                    // add group
                    rmenu.addentry( ++np_last, false, -1, recipe_name );
                    defferent_recipes = true;
                    break;
                } else if( recipe_n == current_part.size() - 1 ) {
                    // only one result
                    rmenu.addentry( ++np_last, true, -1, "─ " + recipe_name );
                }
            }

            if( defferent_recipes ) {
                std::string prev_item_name;
                // 2nd pass: add defferent recipes
                for( size_t recipe_n = 0; recipe_n < current_part.size(); recipe_n++ ) {
                    std::string cur_item_name = current_part[recipe_n]->result_name();
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
}

// Anchors top-right
static void draw_can_craft_indicator( const catacurses::window &w, const recipe &rec )
{
    Character &player_character = get_player_character();
    // Draw text
    if( player_character.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        right_print( w, 0, 1, i_red, _( "too dark to craft" ) );
    } else if( player_character.crafting_speed_multiplier( rec ) <= 0.0f ) {
        // Technically not always only too sad, but must be too sad
        right_print( w, 0, 1, i_red, _( "too sad to craft" ) );
    } else if( player_character.crafting_speed_multiplier( rec ) < 1.0f ) {
        right_print( w, 0, 1, i_yellow, string_format( _( "crafting is slow %d%%" ),
                     static_cast<int>( player_character.crafting_speed_multiplier( rec ) * 100 ) ) );
    } else {
        right_print( w, 0, 1, i_green, _( "craftable" ) );
    }
}

static void draw_recipe_tabs( const catacurses::window &w, const std::string &tab, TAB_MODE mode )
{
    werase( w );

    switch( mode ) {
        case NORMAL: {
            draw_tabs( w, normalized_names, craft_cat_list, tab );
            break;
        }
        case FILTERED:
            mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
            mvwputch( w, point( 0, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OXXO ); // |^
            mvwputch( w, point( getmaxx( w ) - 1, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OOXX ); // ^|
            draw_tab( w, 2, _( "Searched" ), true );
            break;
        case BATCH:
            mvwhline( w, point( 0, getmaxy( w ) - 1 ), LINE_OXOX, getmaxx( w ) - 1 );
            mvwputch( w, point( 0, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OXXO ); // |^
            mvwputch( w, point( getmaxx( w ) - 1, getmaxy( w ) - 1 ), BORDER_COLOR, LINE_OOXX ); // ^|
            draw_tab( w, 2, _( "Batch" ), true );
            break;
    }

    wnoutrefresh( w );
}

static void draw_recipe_subtabs( const catacurses::window &w, const std::string &tab,
                                 const std::string &subtab,
                                 const recipe_subset &available_recipes, TAB_MODE mode )
{
    werase( w );
    int width = getmaxx( w );
    for( int i = 0; i < width; i++ ) {
        if( i == 0 ) {
            mvwputch( w, point( i, 2 ), BORDER_COLOR, LINE_XXXO ); // |-
        } else if( i == width ) { // TODO: that is always false!
            mvwputch( w, point( i, 2 ), BORDER_COLOR, LINE_XOXX ); // -|
        } else {
            mvwputch( w, point( i, 2 ), BORDER_COLOR, LINE_OXOX ); // -
        }
    }

    for( int i = 0; i < 3; i++ ) {
        mvwputch( w, point( 0, i ), BORDER_COLOR, LINE_XOXO ); // |
        mvwputch( w, point( width - 1, i ), BORDER_COLOR, LINE_XOXO ); // |
    }

    switch( mode ) {
        case NORMAL: {
            // Draw the tabs on each other
            int pos_x = 2;
            // Step between tabs, two for tabs border
            int tab_step = 3;
            for( const auto &stt : craft_subcat_list[tab] ) {
                bool empty = available_recipes.empty_category( tab, stt != "CSC_ALL" ? stt : "" );
                draw_subtab( w, pos_x, normalized_names[stt], subtab == stt, true, empty );
                pos_x += utf8_width( normalized_names[stt] ) + tab_step;
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
}

template<typename T>
bool lcmatch_any( const std::vector< std::vector<T> > &list_of_list, const std::string &filter )
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

const std::vector<std::string> *subcategories_for_category( const std::string &category )
{
    auto it = craft_subcat_list.find( category );
    if( it != craft_subcat_list.end() ) {
        return &it->second;
    }
    return nullptr;
}
