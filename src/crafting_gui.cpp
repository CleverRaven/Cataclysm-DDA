#include "crafting_gui.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "cata_utility.h"
#include "catacharset.h"
#include "crafting.h"
#include "debug.h"
#include "game.h"
#include "input.h"
#include "itype.h"
#include "json.h"
#include "output.h"
#include "player.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "uistate.h"

enum TAB_MODE {
    NORMAL,
    FILTERED,
    BATCH
};

std::vector<std::string> craft_cat_list;
std::map<std::string, std::vector<std::string> > craft_subcat_list;
std::map<std::string, std::string> normalized_names;

static bool query_is_yes( const std::string &query );
static void draw_hidden_amount( const catacurses::window &w, const int margin_y, int amount );
static void draw_can_craft_indicator( const catacurses::window &w, const int margin_y,
                                      const recipe &rec );
static void draw_recipe_tabs( const catacurses::window &w, const std::string &tab,
                              TAB_MODE mode = NORMAL );
static void draw_recipe_subtabs( const catacurses::window &w, const std::string &tab,
                                 const std::string &subtab,
                                 const recipe_subset &available_recipes, TAB_MODE mode = NORMAL );

std::string peek_related_recipe( const recipe *current, const recipe_subset &available );
int related_menu_fill( uilist &rmenu,
                       const std::vector<std::pair<itype_id, std::string>> &related_recipes,
                       const recipe_subset &available );

std::string get_cat_name( const std::string &prefixed_name )
{
    return prefixed_name.substr( 3, prefixed_name.size() - 3 );
}

void load_recipe_category( JsonObject &jsobj )
{
    const std::string category = jsobj.get_string( "id" );
    const bool is_hidden = jsobj.get_bool( "is_hidden", false );

    if( category.find( "CC_" ) != 0 ) {
        jsobj.throw_error( "Crafting category id has to be prefixed with 'CC_'" );
    }

    if( !is_hidden ) {
        craft_cat_list.push_back( category );

        const std::string cat_name = get_cat_name( category );

        craft_subcat_list[category] = std::vector<std::string>();
        JsonArray subcats = jsobj.get_array( "recipe_subcategories" );
        while( subcats.has_more() ) {
            const std::string subcat_id = subcats.next_string();
            if( subcat_id.find( "CSC_" + cat_name + "_" ) != 0 && subcat_id != "CSC_ALL" ) {
                jsobj.throw_error( "Crafting sub-category id has to be prefixed with CSC_<category_name>_" );
            }
            craft_subcat_list[category].push_back( subcat_id );
        }
    }
}

std::string get_subcat_name( const std::string &cat, const std::string &prefixed_name )
{
    std::string prefix = "CSC_" + get_cat_name( cat ) + "_";

    if( prefixed_name.find( prefix ) == 0 ) {
        return prefixed_name.substr( prefix.size(), prefixed_name.size() - prefix.size() );
    }

    return ( prefixed_name == "CSC_ALL" ? _( "ALL" ) : _( "NONCRAFT" ) );
}

void translate_all()
{
    for( const auto &cat : craft_cat_list ) {
        normalized_names[cat] = _( get_cat_name( cat ).c_str() );

        for( const auto &subcat : craft_subcat_list[cat] ) {
            normalized_names[subcat] =  _( get_subcat_name( cat, subcat ).c_str() ) ;
        }
    }
}

void reset_recipe_categories()
{
    craft_cat_list.clear();
    craft_subcat_list.clear();
}

int print_items( const recipe &r, const catacurses::window &w, int ypos, int xpos, nc_color col,
                 int batch )
{
    if( !r.has_byproducts() ) {
        return 0;
    }

    const int oldy = ypos;

    mvwprintz( w, ypos++, xpos, col, _( "Byproducts:" ) );
    for( const auto &bp : r.byproducts ) {
        const auto t = item::find_type( bp.first );
        int amount = bp.second * batch;
        std::string desc;
        if( t->count_by_charges() ) {
            amount *= t->charges_default();
            desc = string_format( "> %s (%d)", t->nname( 1 ).c_str(), amount );
        } else {
            desc = string_format( "> %d %s", amount,
                                  t->nname( static_cast<unsigned int>( amount ) ).c_str() );
        }
        mvwprintz( w, ypos++, xpos, col, desc.c_str() );
    }

    return ypos - oldy;
}

const recipe *select_crafting_recipe( int &batch_size )
{
    if( normalized_names.empty() ) {
        translate_all();
    }

    const int headHeight = 3;
    const int subHeadHeight = 2;
    const int freeWidth = TERMX - FULL_SCREEN_WIDTH;
    bool isWide = ( TERMX > FULL_SCREEN_WIDTH && freeWidth > 15 );

    const int width = isWide ? ( freeWidth > FULL_SCREEN_WIDTH ? FULL_SCREEN_WIDTH * 2 : TERMX ) :
                      FULL_SCREEN_WIDTH;
    const int wStart = ( TERMX - width ) / 2;
    const int tailHeight = isWide ? 3 : 4;
    const int dataLines = TERMY - ( headHeight + subHeadHeight ) - tailHeight;
    const int dataHalfLines = dataLines / 2;
    const int dataHeight = TERMY - ( headHeight + subHeadHeight );
    const int infoWidth = width - FULL_SCREEN_WIDTH - 1;

    const recipe *last_recipe = nullptr;

    catacurses::window w_head = catacurses::newwin( headHeight, width, 0, wStart );
    catacurses::window w_subhead = catacurses::newwin( subHeadHeight, width, 3, wStart );
    catacurses::window w_data = catacurses::newwin( dataHeight, width, headHeight + subHeadHeight,
                                wStart );

    int item_info_x = infoWidth;
    int item_info_y = dataHeight - 3;
    int item_info_width = wStart + width - infoWidth;
    int item_info_height = headHeight + subHeadHeight;

    if( !isWide ) {
        item_info_x = 1;
        item_info_y = 1;
        item_info_width = 1;
        item_info_height = 1;
    }

    catacurses::window w_iteminfo = catacurses::newwin( item_info_y, item_info_x, item_info_height,
                                    item_info_width );

    list_circularizer<std::string> tab( craft_cat_list );
    list_circularizer<std::string> subtab( craft_subcat_list[tab.cur()] );
    std::vector<const recipe *> current;
    std::vector<bool> available;
    const int componentPrintHeight = dataHeight - tailHeight - 1;
    //preserves component color printout between mode rotations
    nc_color rotated_color = c_white;
    int previous_item_line = -1;
    std::string previous_tab;
    std::string previous_subtab;
    item tmp;
    int line = 0;
    int ypos = 0;
    int scroll_pos = 0;
    bool redraw = true;
    bool keepline = false;
    bool done = false;
    bool batch = false;
    bool show_hidden = false;
    int batch_line = 0;
    int display_mode = 0;
    const recipe *chosen = nullptr;
    std::vector<iteminfo> thisItem;
    std::vector<iteminfo> dummy;

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

    const inventory &crafting_inv = g->u.crafting_inventory();
    const std::vector<npc *> helpers = g->u.get_crafting_helpers();
    std::string filterstring;

    const auto &available_recipes = g->u.get_available_recipes( crafting_inv, &helpers );
    std::map<const recipe *, bool> availability_cache;

    do {
        if( redraw ) {
            // When we switch tabs, redraw the header
            redraw = false;
            if( ! keepline ) {
                line = 0;
            } else {
                keepline = false;
            }

            if( display_mode > 1 ) {
                display_mode = 1;
            }

            TAB_MODE m = ( batch ) ? BATCH : ( filterstring.empty() ) ? NORMAL : FILTERED;
            draw_recipe_tabs( w_head, tab.cur(), m );
            draw_recipe_subtabs( w_subhead, tab.cur(), subtab.cur(), available_recipes, m );

            show_hidden = false;
            available.clear();

            if( batch ) {
                current.clear();
                for( int i = 1; i <= 20; i++ ) {
                    current.push_back( chosen );
                    available.push_back( chosen->requirements().can_make_with_inventory( crafting_inv, i ) );
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
                                    auto &learned = g->u.get_learned_recipes();
                                    recipe_subset temp_subset;
                                    if( query_is_yes( qry_filter_str ) ) {
                                        temp_subset = available_recipes.intersection( learned );
                                    } else {
                                        temp_subset = available_recipes.difference( learned );
                                    }
                                    filtered_recipes = filtered_recipes.intersection( temp_subset );
                                    break;
                                }

                                case 'h': {
                                    filtered_recipes = filtered_recipes.intersection( available_recipes );
                                    if( query_is_yes( qry_filter_str ) ) {
                                        show_hidden = true;
                                    }
                                    break;
                                }

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
                    picking.insert( picking.end(), available_recipes.begin(), available_recipes.end() );
                    show_hidden = true;
                } else {
                    picking = available_recipes.in_category( tab.cur(), subtab.cur() != "CSC_ALL" ? subtab.cur() : "" );
                }

                current.clear();
                for( auto i : picking ) {
                    if( ( uistate.hidden_recipes.find( i->ident() ) != uistate.hidden_recipes.end() ) == show_hidden ) {
                        current.push_back( i );
                    }
                }
                if( !show_hidden ) {
                    draw_hidden_amount( w_head, 0, picking.size() - current.size() );
                }

                available.reserve( current.size() );
                // cache recipe availability on first display
                for( const auto e : current ) {
                    if( !availability_cache.count( e ) ) {
                        availability_cache.emplace( e, e->requirements().can_make_with_inventory( crafting_inv ) );
                    }
                }

                if( subtab.cur() != "CSC_*_RECENT" ) {
                    std::stable_sort( current.begin(), current.end(), []( const recipe * a, const recipe * b ) {
                        return b->difficulty < a->difficulty;
                    } );

                    std::stable_sort( current.begin(), current.end(), [&]( const recipe * a, const recipe * b ) {
                        return availability_cache[a] && !availability_cache[b];
                    } );
                }

                std::transform( current.begin(), current.end(),
                std::back_inserter( available ), [&]( const recipe * e ) {
                    return availability_cache[e];
                } );
            }

            // current/available have been rebuilt, make sure our cursor is still in range
            if( current.empty() ) {
                line = 0;
            } else {
                line = std::min( line, static_cast<int>( current.size() ) - 1 );
            }
        }

        // Clear the screen of recipe data, and draw it anew
        werase( w_data );

        if( isWide ) {
            werase( w_iteminfo );
        }

        if( isWide ) {
            mvwprintz( w_data, dataLines + 1, 5, c_white,
                       _( "Press <ENTER> to attempt to craft object." ) );
            wprintz( w_data, c_white, "  " );
            if( !filterstring.empty() ) {
                wprintz( w_data, c_white,
                         _( "[E]: Describe, [F]ind, [R]eset, [m]ode, [s]how/hide, Re[L]ated, [*]Favorite, %s [?] keybindings" ),
                         ( batch ) ? _( "cancel [b]atch" ) : _( "[b]atch" ) );
            } else {
                wprintz( w_data, c_white,
                         _( "[E]: Describe, [F]ind, [m]ode, [s]how/hide, Re[L]ated, [*]Favorite, %s [?] keybindings" ),
                         ( batch ) ? _( "cancel [b]atch" ) : _( "[b]atch" ) );
            }
        } else {
            if( !filterstring.empty() ) {
                mvwprintz( w_data, dataLines + 1, 5, c_white,
                           _( "[E]: Describe, [F]ind, [R]eset, [m]ode, [s]how/hide, Re[L]ated, [*]Favorite, [b]atch [?] keybindings" ) );
            } else {
                mvwprintz( w_data, dataLines + 1, 5, c_white,
                           _( "[E]: Describe, [F]ind, [m]ode, [s]how/hide, Re[L]ated, [*]Favorite, [b]atch [?] keybindings" ) );
            }
            mvwprintz( w_data, dataLines + 2, 5, c_white,
                       _( "Press <ENTER> to attempt to craft object." ) );
        }
        // Draw borders
        for( int i = 1; i < width - 1; ++i ) { // _
            mvwputch( w_data, dataHeight - 1, i, BORDER_COLOR, LINE_OXOX );
        }
        for( int i = 0; i < dataHeight - 1; ++i ) { // |
            mvwputch( w_data, i, 0, BORDER_COLOR, LINE_XOXO );
            mvwputch( w_data, i, width - 1, BORDER_COLOR, LINE_XOXO );
        }
        mvwputch( w_data, dataHeight - 1,  0, BORDER_COLOR, LINE_XXOO ); // _|
        mvwputch( w_data, dataHeight - 1, width - 1, BORDER_COLOR, LINE_XOOX ); // |_

        int recmin = 0, recmax = current.size();
        if( recmax > dataLines ) {
            if( line <= recmin + dataHalfLines ) {
                for( int i = recmin; i < recmin + dataLines; ++i ) {
                    std::string tmp_name = current[i]->result_name();
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name.c_str() );
                    }
                    mvwprintz( w_data, i - recmin, 2, c_dark_gray, "" ); // Clear the line
                    if( i == line ) {
                        mvwprintz( w_data, i - recmin, 2, ( available[i] ? h_white : h_dark_gray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    } else {
                        mvwprintz( w_data, i - recmin, 2, ( available[i] ? c_white : c_dark_gray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    }
                }
            } else if( line >= recmax - dataHalfLines ) {
                for( int i = recmax - dataLines; i < recmax; ++i ) {
                    std::string tmp_name = current[i]->result_name();
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name.c_str() );
                    }
                    mvwprintz( w_data, dataLines + i - recmax, 2, c_light_gray, "" ); // Clear the line
                    if( i == line ) {
                        mvwprintz( w_data, dataLines + i - recmax, 2,
                                   ( available[i] ? h_white : h_dark_gray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    } else {
                        mvwprintz( w_data, dataLines + i - recmax, 2,
                                   ( available[i] ? c_white : c_dark_gray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    }
                }
            } else {
                for( int i = line - dataHalfLines; i < line - dataHalfLines + dataLines; ++i ) {
                    std::string tmp_name = current[i]->result_name();
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name.c_str() );
                    }
                    mvwprintz( w_data, dataHalfLines + i - line, 2, c_light_gray, "" ); // Clear the line
                    if( i == line ) {
                        mvwprintz( w_data, dataHalfLines + i - line, 2,
                                   ( available[i] ? h_white : h_dark_gray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    } else {
                        mvwprintz( w_data, dataHalfLines + i - line, 2,
                                   ( available[i] ? c_white : c_dark_gray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    }
                }
            }
        } else {
            for( size_t i = 0; i < current.size() && i < static_cast<size_t>( dataHeight ) + 1; ++i ) {
                std::string tmp_name = current[i]->result_name();
                if( batch ) {
                    tmp_name = string_format( _( "%2dx %s" ), static_cast<int>( i ) + 1, tmp_name.c_str() );
                }
                if( static_cast<int>( i ) == line ) {
                    mvwprintz( w_data, i, 2, ( available[i] ? h_white : h_dark_gray ),
                               utf8_truncate( tmp_name, 28 ).c_str() );
                } else {
                    mvwprintz( w_data, i, 2, ( available[i] ? c_white : c_dark_gray ),
                               utf8_truncate( tmp_name, 28 ).c_str() );
                }
            }
        }

        if( !current.empty() ) {
            int pane = FULL_SCREEN_WIDTH - 30 - 1;
            int count = batch ? line + 1 : 1; // batch size
            nc_color col = available[ line ] ? c_white : c_light_gray;

            const auto &req = current[ line ]->requirements();

            draw_can_craft_indicator( w_head, 0, *current[line] );
            wrefresh( w_head );

            ypos = 0;

            auto qry = trim( filterstring );
            std::string qry_comps;
            if( qry.compare( 0, 2, "c:" ) == 0 ) {
                qry_comps = qry.substr( 2 );
            }

            std::vector<std::string> component_print_buffer;
            auto tools = req.get_folded_tools_list( pane, col, crafting_inv, count );
            auto comps = req.get_folded_components_list( pane, col, crafting_inv, count, qry_comps );
            component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
            component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

            if( !g->u.knows_recipe( current[line] ) ) {
                component_print_buffer.push_back( _( "Recipe not memorized yet" ) );
                auto books_with_recipe = g->u.get_books_for_recipe( crafting_inv, current[line] );
                std::string enumerated_books =
                    enumerate_as_string( books_with_recipe.begin(), books_with_recipe.end(),
                []( itype_id type_id ) {
                    return colorize( item::find_type( type_id )->nname( 1 ), c_cyan );
                } );
                const std::string text = string_format( _( "Written in: %s" ), enumerated_books.c_str() );
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
                const int width = getmaxx( w_data ) - xpos - item_info_x;

                auto player_skill = g->u.get_skill_level( current[line]->skill_used );
                std::string difficulty_color =
                    current[ line ]->difficulty > player_skill ? "yellow" : "green";
                std::string primary_skill_level = string_format( "(%s/%s)", player_skill,
                                                  current[ line ]->difficulty );
                print_colored_text(
                    w_data, ypos++, xpos, col, col,
                    string_format( _( "Primary skill: <color_cyan>%s</color> <color_%s>%s</color>" ),
                                   ( !current[line]->skill_used ? _( "N/A" ) :
                                     current[line]->skill_used.obj().name() ),
                                   difficulty_color,
                                   ( !current[line]->skill_used ? "" : primary_skill_level )
                                 ) );

                ypos += fold_and_print( w_data, ypos, xpos, width, col,
                                        _( "Other skills: %s" ),
                                        current[line]->required_skills_string( &g->u ) );

                const int expected_turns = g->u.expected_time_to_craft( *current[line],
                                           count ) / to_moves<int>( 1_turns );
                ypos += fold_and_print( w_data, ypos, xpos, pane, col,
                                        _( "Time to complete: <color_cyan>%s</color>" ),
                                        to_string( time_duration::from_turns( expected_turns ) ) );

                ypos += fold_and_print( w_data, ypos, xpos, pane, col,
                                        _( "Batch time savings: <color_cyan>%s</color>" ),
                                        current[line]->batch_savings_string() );

                print_colored_text(
                    w_data, ypos++, xpos, col, col,
                    string_format( _( "Dark craftable? <color_cyan>%s</color>" ),
                                   current[line]->has_flag( "BLIND_EASY" ) ? _( "Easy" ) :
                                   current[line]->has_flag( "BLIND_HARD" ) ? _( "Hard" ) :
                                   _( "Impossible" ) ) );
                ypos += print_items( *current[line], w_data, ypos, xpos, col, batch ? line + 1 : 1 );
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
                print_colored_text( w_data, ypos++, xpos, stored_color, col, component_print_buffer[i] );
            }

            if( ypos >= componentPrintHeight &&
                component_print_buffer.size() > static_cast<size_t>( components_printed ) ) {
                mvwprintz( w_data, ypos++, xpos, col,
                           _( "v (%s for more)" ),
                           ctxt.press_x( "CYCLE_MODE" ) );
                rotated_color = stored_color;
            }

            if( isWide ) {
                if( last_recipe != current[line] ) {
                    last_recipe = current[line];
                    tmp = current[line]->create_result();
                }
                tmp.info( true, thisItem, count );
                draw_item_info( w_iteminfo, tmp.tname(), tmp.type_name(), thisItem, dummy,
                                scroll_pos, true, true, true, false, true );
            }
        }

        draw_scrollbar( w_data, line, dataLines, recmax, 0 );
        wrefresh( w_data );

        if( isWide ) {
            wrefresh( w_iteminfo );
        }

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
            redraw = true;
        } else if( action == "SCROLL_UP" ) {
            scroll_pos--;
        } else if( action == "SCROLL_DOWN" ) {
            scroll_pos++;
        } else if( action == "PREV_TAB" ) {
            tab.prev();
            subtab = list_circularizer<std::string>( craft_subcat_list[tab.cur()] );//default ALL
            redraw = true;
        } else if( action == "RIGHT" ) {
            std::string start = subtab.cur();
            do {
                subtab.next();
            } while( subtab.cur() != start && available_recipes.empty_category( tab.cur(),
                     subtab.cur() != "CSC_ALL" ? subtab.cur() : "" ) );
            redraw = true;
        } else if( action == "NEXT_TAB" ) {
            tab.next();
            subtab = list_circularizer<std::string>( craft_subcat_list[tab.cur()] );//default ALL
            redraw = true;
        } else if( action == "DOWN" ) {
            line++;
        } else if( action == "UP" ) {
            line--;
        } else if( action == "CONFIRM" ) {
            if( available.empty() || !available[line] ) {
                popup( _( "You can't do that!" ) );
            } else if( !g->u.check_eligible_containers_for_crafting( *current[line],
                       ( batch ) ? line + 1 : 1 ) ) {
                // popup is already inside check
            } else {
                chosen = current[line];
                batch_size = ( batch ) ? line + 1 : 1;
                done = true;
            }
        } else if( action == "HELP_RECIPE" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!" ) );
                redraw = true;
                continue;
            }
            tmp = current[line]->create_result();

            full_screen_popup( "%s\n%s", tmp.type_name( 1 ).c_str(),  tmp.info( true ).c_str() );
            redraw = true;
            keepline = true;
        } else if( action == "FILTER" ) {
            struct SearchPrefix {
                char key;
                std::string example;
                std::string description;
            };
            std::vector<SearchPrefix> prefixes = {
                { 'q', _( "metal sawing" ), _( "<color_cyan>quality</color> of resulting item" ) },
                //~ Example result description search term
                { 'd', _( "reach attack" ), _( "<color_cyan>full description</color> of resulting item (slow)" ) },
                { 'c', _( "two by four" ), _( "<color_cyan>component</color> required to craft" ) },
                { 'p', _( "tailoring" ), _( "<color_cyan>primary skill</color> used to craft" ) },
                { 's', _( "cooking" ), _( "<color_cyan>any skill</color> used to craft" ) },
                { 'Q', _( "fine bolt turning" ), _( "<color_cyan>quality</color> required to craft" ) },
                { 't', _( "soldering iron" ), _( "<color_cyan>tool</color> required to craft" ) },
                { 'h', _( "yes" ), _( "recipes which are <color_cyan>hidden</color> or not" ) },
                { 'm', _( "no" ), _( "recipes which are <color_cyan>memorized</color> or not" ) },
            };
            int max_example_length = 0;
            for( const auto &prefix : prefixes ) {
                max_example_length = std::max( max_example_length, utf8_width( prefix.example ) );
            }
            std::string spaces( max_example_length, ' ' );

            std::string description =
                _( "The default is to search result names.  Some single-character prefixes "
                   "can be used with a colon (:) to search in other ways. Additional filters "
                   "are separated by commas (,).\n"
                   "\n"
                   "<color_white>Examples:</color>\n" );

            {
                std::string example_name = _( "shirt" );
                auto padding = max_example_length - utf8_width( example_name );
                description += string_format(
                                   _( "  <color_white>%s</color>%.*s    %s\n" ),
                                   example_name, padding, spaces,
                                   _( "<color_cyan>name</color> of resulting item" ) );
            }

            for( const auto &prefix : prefixes ) {
                auto padding = max_example_length - utf8_width( prefix.example );
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
            redraw = true;
        } else if( action == "QUIT" ) {
            chosen = nullptr;
            done = true;
        } else if( action == "RESET_FILTER" ) {
            filterstring.clear();
            redraw = true;
        } else if( action == "CYCLE_BATCH" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!" ) );
                redraw = true;
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
            redraw = true;
        } else if( action == "TOGGLE_FAVORITE" ) {
            keepline = true;
            redraw = true;
            if( current.empty() ) {
                popup( _( "Nothing selected!" ) );
                continue;
            }
            if( uistate.favorite_recipes.find( current[line]->ident() ) != uistate.favorite_recipes.end() ) {
                uistate.favorite_recipes.erase( current[line]->ident() );
            } else {
                uistate.favorite_recipes.insert( current[line]->ident() );
            }
        } else if( action == "HIDE_SHOW_RECIPE" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!" ) );
                redraw = true;
                continue;
            }
            if( show_hidden ) {
                uistate.hidden_recipes.erase( current[line]->ident() );
            } else {
                uistate.hidden_recipes.insert( current[line]->ident() );
            }

            redraw = true;
        } else if( action == "RELATED_RECIPES" ) {
            if( current.empty() ) {
                popup( _( "Nothing selected!" ) );
                redraw = true;
                continue;
            }
            std::string recipe_name = peek_related_recipe( current[ line ], available_recipes );
            if( recipe_name.empty() ) {
                keepline = true;
            } else {
                filterstring = recipe_name;
            }

            redraw = true;
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
    // current recipe components
    std::vector<std::pair<itype_id, std::string>> related_components;
    const requirement_data &req = current->requirements();
    for( const std::vector<item_comp> &comp_list : req.get_components() ) {
        for( const item_comp &a : comp_list ) {
            related_components.push_back( { a.type, item::nname( a.type, 1 ) } );
        }
    }
    // current recipe result
    std::vector<std::pair<itype_id, std::string>> related_results;
    item tmp = current->create_result();
    itype_id tid;
    if( tmp.contents.empty() ) { // use this item
        tid = tmp.typeId();
    } else { // use the contained item
        tid = tmp.contents.front().typeId();
    }
    const std::set<const recipe *> &known_recipes = g->u.get_learned_recipes().of_component( tid );
    for( const auto &b : known_recipes ) {
        if( available.contains( b ) ) {
            related_results.push_back( { b->result(), b->result_name() } );
        }
    }
    std::stable_sort( related_results.begin(), related_results.end(),
    []( const std::pair<std::string, std::string> &a, const std::pair<std::string, std::string> &b ) {
        return a.second < b.second;
    } );

    uilist rel_menu;
    int np_last = -1;
    if( !related_components.empty() ) {
        rel_menu.addentry( ++np_last, false, -1, _( "COMPONENTS" ) );
    }
    np_last = related_menu_fill( rel_menu, related_components, available );
    if( !related_results.empty() ) {
        rel_menu.addentry( ++np_last, false, -1, _( "RESULTS" ) );
    }
    np_last = related_menu_fill( rel_menu, related_results, available );

    rel_menu.settext( _( "Related recipes:" ) );
    rel_menu.query();
    if( rel_menu.ret != UILIST_CANCEL ) {
        std::wstring wstr_recipe_name = utf8_to_wstr( rel_menu.entries[ rel_menu.ret ].txt );
        return wstr_to_utf8( wstr_recipe_name.substr( 2 ) ); // 2 = prefix length
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
                if( current_part[ recipe_n ]->result_name() != recipe_name ) {
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
                    std::string cur_item_name = current_part[ recipe_n ]->result_name();
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

static void draw_hidden_amount( const catacurses::window &w, const int margin_y, int amount )
{
    if( amount > 0 ) {
        right_print( w, margin_y, 14, c_light_gray, string_format( _( "%s hidden" ), amount ) );
    }
}
// Anchors top-right
static void draw_can_craft_indicator( const catacurses::window &w, const int margin_y,
                                      const recipe &rec )
{
    // Erase previous text
    // FIXME: replace this hack by proper solution (based on max width of possible content)
    right_print( w, margin_y + 1, 1, c_black, "        " );
    // Draw text
    right_print( w, margin_y, 1, c_light_gray, _( "can craft:" ) );
    if( g->u.lighting_craft_speed_multiplier( rec ) <= 0.0f ) {
        right_print( w, margin_y + 1, 1, i_red, _( "too dark" ) );
    } else if( g->u.crafting_speed_multiplier( rec ) <= 0.0f ) {
        // Technically not always only too sad, but must be too sad
        right_print( w, margin_y + 1, 1, i_red, _( "too sad" ) );
    } else if( g->u.crafting_speed_multiplier( rec ) < 1.0f ) {
        right_print( w, margin_y + 1, 1, i_yellow, string_format( _( "slow %d%%" ),
                     static_cast<int>( g->u.crafting_speed_multiplier( rec ) * 100 ) ) );
    } else {
        right_print( w, margin_y + 1, 1, i_green, _( "yes" ) );
    }
}

static void draw_recipe_tabs( const catacurses::window &w, const std::string &tab, TAB_MODE mode )
{
    werase( w );
    int width = getmaxx( w );
    for( int i = 0; i < width; i++ ) {
        mvwputch( w, 2, i, BORDER_COLOR, LINE_OXOX );
    }

    mvwputch( w, 2,  0, BORDER_COLOR, LINE_OXXO ); // |^
    mvwputch( w, 2, width - 1, BORDER_COLOR, LINE_OOXX ); // ^|

    switch( mode ) {
        case NORMAL: {
            int pos_x = 2;//draw the tabs on each other
            int tab_step = 3;//step between tabs, two for tabs border
            for( const auto &tt : craft_cat_list ) {
                draw_tab( w, pos_x, normalized_names[tt], tab == tt );
                pos_x += utf8_width( normalized_names[tt] ) + tab_step;
            }
            break;
        }
        case FILTERED:
            draw_tab( w, 2, _( "Searched" ), true );
            break;
        case BATCH:
            draw_tab( w, 2, _( "Batch" ), true );
            break;
    }

    wrefresh( w );
}

static void draw_recipe_subtabs( const catacurses::window &w, const std::string &tab,
                                 const std::string &subtab,
                                 const recipe_subset &available_recipes, TAB_MODE mode )
{
    werase( w );
    int width = getmaxx( w );
    for( int i = 0; i < width; i++ ) {
        if( i == 0 ) {
            mvwputch( w, 2, i, BORDER_COLOR, LINE_XXXO );
        } else if( i == width ) { // TODO: that is always false!
            mvwputch( w, 2, i, BORDER_COLOR, LINE_XOXX );
        } else {
            mvwputch( w, 2, i, BORDER_COLOR, LINE_OXOX );
        }
    }

    for( int i = 0; i < 3; i++ ) {
        mvwputch( w, i,  0, BORDER_COLOR, LINE_XOXO ); // |^
        mvwputch( w, i, width - 1, BORDER_COLOR,  LINE_XOXO ); // ^|
    }

    switch( mode ) {
        case NORMAL: {
            int pos_x = 2;//draw the tabs on each other
            int tab_step = 3;//step between tabs, two for tabs border
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
                mvwputch( w, i,  0, BORDER_COLOR, LINE_XOXO ); // |^
                mvwputch( w, i, width - 1, BORDER_COLOR,  LINE_XOXO ); // ^|
            }
            break;
    }

    wrefresh( w );
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
