#include "crafting_gui.h"

#include "crafting.h"
#include "recipe_dictionary.h"
#include "player.h"
#include "itype.h"
#include "input.h"
#include "game.h"
#include "translations.h"
#include "catacharset.h"
#include "cata_utility.h"

#include "debug.h"

#include <algorithm>

enum TAB_MODE {
    NORMAL,
    FILTERED,
    BATCH
};

std::vector<std::string> craft_cat_list;
std::map<std::string, std::vector<std::string> > craft_subcat_list;
std::map<std::string, std::string> normalized_names;

static void draw_recipe_tabs( WINDOW *w, std::string tab, TAB_MODE mode = NORMAL );
static void draw_recipe_subtabs( WINDOW *w, std::string tab, std::string subtab,
                                 TAB_MODE mode = NORMAL );

std::string get_cat_name( std::string prefixed_name )
{
    return prefixed_name.substr( 3, prefixed_name.size() - 3 );
}

void load_recipe_category( JsonObject &jsobj )
{
    JsonArray subcats;
    std::string category = jsobj.get_string( "id" );

    if( category.find( "CC_" ) != 0 ) {
        jsobj.throw_error( "Crafting category id has to be prefixed with 'CC_'" );
    }

    // Don't store noncraft as a category.
    // We're storing the subcategory so we can look it up in load_recipes
    // for the fallback subcategory.
    if( category != "CC_NONCRAFT" ) {
        craft_cat_list.push_back( category );
    }

    std::string cat_name = get_cat_name( category );

    craft_subcat_list[category] = std::vector<std::string>();
    subcats = jsobj.get_array( "recipe_subcategories" );
    while( subcats.has_more() ) {
        std::string subcat_id = subcats.next_string();
        if( subcat_id.find( "CSC_" + cat_name + "_" ) != 0 && subcat_id != "CSC_ALL" &&
            subcat_id != "CSC_NONCRAFT" ) {
            jsobj.throw_error( "Crafting sub-category id has to be prefixed with CSC_<category_name>_" );
        }
        craft_subcat_list[category].push_back( subcat_id );
    }
}

std::string get_subcat_name( const std::string &cat, std::string prefixed_name )
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
        normalized_names[cat] = get_cat_name( _( cat.c_str() ) );

        for( const auto &subcat : craft_subcat_list[cat] ) {
            normalized_names[subcat] =  get_subcat_name( cat, _( subcat.c_str() ) ) ;
        }
    }
}

void reset_recipe_categories()
{
    craft_cat_list.clear();
    craft_subcat_list.clear();
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

    int lastid = -1;

    WINDOW *w_head = newwin( headHeight, width, 0, wStart );
    WINDOW *w_subhead = newwin( subHeadHeight, width, 3, wStart );
    WINDOW *w_data = newwin( dataHeight, width, headHeight + subHeadHeight, wStart );

    const int iInfoWidth = width - FULL_SCREEN_WIDTH - 3;
    std::string item_info_text;
    list_circularizer<std::string> tab( craft_cat_list );
    list_circularizer<std::string> subtab( craft_subcat_list[tab] );
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
    input_context ctxt( "CRAFTING" );
    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "CYCLE_MODE" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "HELP_RECIPE" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "CYCLE_BATCH" );

    const inventory &crafting_inv = g->u.crafting_inventory();
    std::string filterstring = "";
    do {
        if( redraw ) {
            // When we switch tabs, redraw the header
            redraw = false;
            if( ! keepline ) {
                line = 0;
            } else {
                keepline = false;
            }

            if( display_mode > 2 ) {
                display_mode = 2;
            }

            TAB_MODE m = ( batch ) ? BATCH : ( filterstring == "" ) ? NORMAL : FILTERED;
            draw_recipe_tabs( w_head, tab, m );
            draw_recipe_subtabs( w_subhead, tab, subtab, m );
            current.clear();
            available.clear();
            if( batch ) {
                batch_recipes( crafting_inv, current, available, chosen );
            } else {
                // Set current to all recipes in the current tab; available are possible to make
                pick_recipes( crafting_inv, current, available, tab, subtab, filterstring );
            }
        }

        // Clear the screen of recipe data, and draw it anew
        werase( w_data );

        if( isWide ) {
            mvwprintz( w_data, dataLines + 1, 5, c_white,
                       _( "Press <ENTER> to attempt to craft object." ) );
            wprintz( w_data, c_white, "  " );
            if( filterstring != "" ) {
                wprintz( w_data, c_white, _( "[E]: Describe, [F]ind, [R]eset, [m]ode, %s [?] keybindings" ),
                         ( batch ) ? _( "cancel [b]atch" ) : _( "[b]atch" ) );
            } else {
                wprintz( w_data, c_white, _( "[E]: Describe, [F]ind, [m]ode, %s [?] keybindings" ),
                         ( batch ) ? _( "cancel [b]atch" ) : _( "[b]atch" ) );
            }
        } else {
            if( filterstring != "" ) {
                mvwprintz( w_data, dataLines + 1, 5, c_white,
                           _( "[E]: Describe, [F]ind, [R]eset, [m]ode, [b]atch [?] keybindings" ) );
            } else {
                mvwprintz( w_data, dataLines + 1, 5, c_white,
                           _( "[E]: Describe, [F]ind, [m]ode, [b]atch [?] keybindings" ) );
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
                    std::string tmp_name = item::nname( current[i]->result );
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name.c_str() );
                    }
                    mvwprintz( w_data, i - recmin, 2, c_dkgray, "" ); // Clear the line
                    if( i == line ) {
                        mvwprintz( w_data, i - recmin, 2, ( available[i] ? h_white : h_dkgray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    } else {
                        mvwprintz( w_data, i - recmin, 2, ( available[i] ? c_white : c_dkgray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    }
                }
            } else if( line >= recmax - dataHalfLines ) {
                for( int i = recmax - dataLines; i < recmax; ++i ) {
                    std::string tmp_name = item::nname( current[i]->result );
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name.c_str() );
                    }
                    mvwprintz( w_data, dataLines + i - recmax, 2, c_ltgray, "" ); // Clear the line
                    if( i == line ) {
                        mvwprintz( w_data, dataLines + i - recmax, 2,
                                   ( available[i] ? h_white : h_dkgray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    } else {
                        mvwprintz( w_data, dataLines + i - recmax, 2,
                                   ( available[i] ? c_white : c_dkgray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    }
                }
            } else {
                for( int i = line - dataHalfLines; i < line - dataHalfLines + dataLines; ++i ) {
                    std::string tmp_name = item::nname( current[i]->result );
                    if( batch ) {
                        tmp_name = string_format( _( "%2dx %s" ), i + 1, tmp_name.c_str() );
                    }
                    mvwprintz( w_data, dataHalfLines + i - line, 2, c_ltgray, "" ); // Clear the line
                    if( i == line ) {
                        mvwprintz( w_data, dataHalfLines + i - line, 2,
                                   ( available[i] ? h_white : h_dkgray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    } else {
                        mvwprintz( w_data, dataHalfLines + i - line, 2,
                                   ( available[i] ? c_white : c_dkgray ),
                                   utf8_truncate( tmp_name, 28 ).c_str() );
                    }
                }
            }
        } else {
            for( size_t i = 0; i < current.size() && i < ( size_t )dataHeight + 1; ++i ) {
                std::string tmp_name = item::nname( current[i]->result );
                if( batch ) {
                    tmp_name = string_format( _( "%2dx %s" ), ( int )i + 1, tmp_name.c_str() );
                }
                if( ( int )i == line ) {
                    mvwprintz( w_data, i, 2, ( available[i] ? h_white : h_dkgray ),
                               utf8_truncate( tmp_name, 28 ).c_str() );
                } else {
                    mvwprintz( w_data, i, 2, ( available[i] ? c_white : c_dkgray ),
                               utf8_truncate( tmp_name, 28 ).c_str() );
                }
            }
        }
        if( !current.empty() ) {
            nc_color col = ( available[line] ? c_white : c_ltgray );
            ypos = 0;

            component_print_buffer = current[line]->requirements.get_folded_components_list(
                                         FULL_SCREEN_WIDTH - 30 - 1, col, crafting_inv, ( batch ) ? line + 1 : 1 );
            if( !g->u.knows_recipe( current[line] ) ) {
                component_print_buffer.push_back( _( "Recipe not memorized yet" ) );
            }

            //handle positioning of component list if it needed to be scrolled
            int componentPrintOffset = 0;
            if( display_mode > 2 ) {
                componentPrintOffset = ( display_mode - 2 ) * componentPrintHeight;
            }
            if( component_print_buffer.size() < static_cast<size_t>( componentPrintOffset ) ) {
                componentPrintOffset = 0;
                if( previous_tab != tab.cur() || previous_subtab != subtab.cur() || previous_item_line != line ) {
                    display_mode = 2;
                } else {
                    display_mode = 0;
                }
            }

            //only used to preserve mode position on components when
            //moving to another item and the view is already scrolled
            previous_tab = tab;
            previous_subtab = subtab;
            previous_item_line = line;

            if( display_mode == 0 ) {
                mvwprintz( w_data, ypos++, 30, col, _( "Skills used: %s" ),
                           ( !current[line]->skill_used ? _( "N/A" ) :
                             current[line]->skill_used.obj().name().c_str() ) );

                mvwprintz( w_data, ypos++, 30, col, _( "Required skills: %s" ),
                           ( current[line]->required_skills_string().c_str() ) );
                mvwprintz( w_data, ypos++, 30, col, _( "Difficulty: %d" ), current[line]->difficulty );
                if( !current[line]->skill_used ) {
                    mvwprintz( w_data, ypos++, 30, col, _( "Your skill level: N/A" ) );
                } else {
                    mvwprintz( w_data, ypos++, 30, col, _( "Your skill level: %d" ),
                               // Macs don't seem to like passing this as a class, so force it to int
                               ( int )g->u.skillLevel( current[line]->skill_used ) );
                }
                ypos += current[line]->print_time( w_data, ypos, 30, FULL_SCREEN_WIDTH - 30 - 1, col,
                                                   ( batch ) ? line + 1 : 1 );
                ypos += current[line]->print_items( w_data, ypos, 30, col, ( batch ) ? line + 1 : 1 );
            }
            if( display_mode == 0 || display_mode == 1 ) {
                ypos += current[line]->requirements.print_tools(
                            w_data, ypos, 30, FULL_SCREEN_WIDTH - 30 - 1, col,
                            crafting_inv, ( batch ) ? line + 1 : 1 );
            }

            //color needs to be preserved in case part of the previous page was cut off
            nc_color stored_color = col;
            if( display_mode > 2 ) {
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
                print_colored_text( w_data, ypos++, 30, stored_color, col, component_print_buffer[i] );
            }

            if( ypos >= componentPrintHeight &&
                component_print_buffer.size() > static_cast<size_t>( components_printed ) ) {
                mvwprintz( w_data, ypos++, 30, col, _( "v (more)" ) );
                rotated_color = stored_color;
            }

            if( isWide ) {
                if( lastid != current[line]->id ) {
                    lastid = current[line]->id;
                    tmp = current[line]->create_result();
                    item_info_text = tmp.info( true );
                }
                mvwprintz( w_data, 0, FULL_SCREEN_WIDTH + 1, col, "%s",
                           utf8_truncate( tmp.type_name( 1 ), iInfoWidth ).c_str() );

                fold_and_print( w_data, 1, FULL_SCREEN_WIDTH + 1, iInfoWidth, col, item_info_text );
            }

        }

        draw_scrollbar( w_data, line, dataLines, recmax, 0 );
        wrefresh( w_data );

        const std::string action = ctxt.handle_input();
        if( action == "CYCLE_MODE" ) {
            display_mode = display_mode + 1;
            if( display_mode <= 0 ) {
                display_mode = 0;
            }
        } else if( action == "LEFT" ) {
            subtab.prev();
            redraw = true;
        } else if( action == "PREV_TAB" ) {
            tab.prev();
            subtab( craft_subcat_list[tab] );//default ALL
            redraw = true;
        } else if( action == "RIGHT" ) {
            subtab.next();
            redraw = true;
        } else if( action == "NEXT_TAB" ) {
            tab.next();
            subtab( craft_subcat_list[tab] );//default ALL
            redraw = true;
        } else if( action == "DOWN" ) {
            line++;
        } else if( action == "UP" ) {
            line--;
        } else if( action == "CONFIRM" ) {
            if( available.empty() || !available[line] ) {
                popup( _( "You can't do that!" ) );
            } else if( !current[line]->check_eligible_containers_for_crafting( ( batch ) ? line + 1 : 1 ) ) {
                ; // popup is already inside check
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
            filterstring = string_input_popup( _( "Search:" ), 85, filterstring,
                                               _( "Special prefixes for requirements:\n"
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
                                                ) );
            redraw = true;
        } else if( action == "QUIT" ) {
            chosen = nullptr;
            done = true;
        } else if( action == "RESET_FILTER" ) {
            filterstring = "";
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
        }
        if( line < 0 ) {
            line = current.size() - 1;
        } else if( line >= ( int )current.size() ) {
            line = 0;
        }
    } while( !done );

    werase( w_head );
    werase( w_subhead );
    werase( w_data );
    delwin( w_head );
    delwin( w_subhead );
    delwin( w_data );
    g->refresh_all();

    return chosen;
}

// Anchors top-right
static void draw_can_craft_indicator( WINDOW *w, int window_width, int margin_x, int margin_y )
{
    int x_align = window_width - margin_x;
    // Draw text
    mvwprintz( w, margin_y, x_align - utf8_width( _( "can craft:" ) ), c_ltgray, _( "can craft:" ) );
    if( g->u.can_see_to_craft() ) {
        mvwprintz( w, margin_y + 1, x_align - 1 - utf8_width( _( "too dark" ) ),  i_red  ,
                   _( "too dark" ) );
    } else if( g->u.has_moral_to_craft() ) {
        mvwprintz( w, margin_y + 1, x_align - 1 - utf8_width( _( "too sad" ) ),  i_red  , _( "too sad" ) );
    } else {
        mvwprintz( w, margin_y + 1, x_align - 1 - utf8_width( _( "yes" ) ),  i_green  , _( "yes" ) );
    }
}

static void draw_recipe_tabs( WINDOW *w, std::string tab, TAB_MODE mode )
{
    werase( w );
    int width = getmaxx( w );
    for( int i = 0; i < width; i++ ) {
        mvwputch( w, 2, i, BORDER_COLOR, LINE_OXOX );
    }

    mvwputch( w, 2,  0, BORDER_COLOR, LINE_OXXO ); // |^
    mvwputch( w, 2, width - 1, BORDER_COLOR, LINE_OOXX ); // ^|

    // Draw a "can craft" indicator
    draw_can_craft_indicator( w, width, 1, 0 );

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

static void draw_recipe_subtabs( WINDOW *w, std::string tab, std::string subtab, TAB_MODE mode )
{
    werase( w );
    int width = getmaxx( w );
    for( int i = 0; i < width; i++ ) {
        if( i == 0 ) {
            mvwputch( w, 2, i, BORDER_COLOR, LINE_XXXO );
        } else if( i == width ) {
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
            for( const auto stt : craft_subcat_list[tab] ) {
                draw_subtab( w, pos_x, normalized_names[stt], subtab == stt );
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

int recipe::print_items( WINDOW *w, int ypos, int xpos, nc_color col, int batch ) const
{
    if( !has_byproducts() ) {
        return 0;
    }

    const int oldy = ypos;

    mvwprintz( w, ypos++, xpos, col, _( "Byproducts:" ) );
    for( auto &bp : byproducts ) {
        print_item( w, ypos++, xpos, col, bp, batch );
    }

    return ypos - oldy;
}

void recipe::print_item( WINDOW *w, int ypos, int xpos, nc_color col, const byproduct &bp,
                         int batch ) const
{
    item it( bp.result, calendar::turn, false );
    std::string str = string_format( _( "> %d %s" ), ( it.charges > 0 ) ? bp.amount : bp.amount * batch,
                                     it.tname().c_str() );
    if( it.charges > 0 ) {
        str = string_format( _( "%s (%d)" ), str.c_str(), it.charges * bp.charges_mult * batch );
    }
    mvwprintz( w, ypos, xpos, col, str.c_str() );
}

int recipe::print_time( WINDOW *w, int ypos, int xpos, int width,
                        nc_color col, int batch ) const
{
    const int turns = batch_time( batch ) / 100;
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
extern bool lcmatch( const std::string &str, const std::string &findstr );

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

void pick_recipes( const inventory &crafting_inv,
                   std::vector<const recipe *> &current,
                   std::vector<bool> &available, std::string tab,
                   std::string subtab, std::string filter )
{
    bool search_name = true;
    bool search_tool = false;
    bool search_component = false;
    bool search_skill = false;
    bool search_skill_primary_only = false;
    bool search_qualities = false;
    bool search_result_qualities = false;
    size_t pos = filter.find( ":" );
    if( pos != std::string::npos ) {
        search_name = false;
        std::string searchType = filter.substr( 0, pos );
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
        filter = filter.substr( pos + 1 );
    }
    std::vector<recipe *> available_recipes;

    if( filter == "" ) {
        available_recipes = recipe_dict.in_category( tab );
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
            ( rec->subcat == "" && craft_subcat_list[tab].back() == subtab ) ||
            filter != "" ) {
            if( ( !g->u.knows_recipe( rec ) && -1 == g->u.has_recipe( rec, crafting_inv ) )
                || ( rec->difficulty < 0 ) ) {
                continue;
            }
            if( filter != "" ) {
                if( ( search_name && !lcmatch( item::nname( rec->result ), filter ) )
                    || ( search_tool && !lcmatch_any( rec->requirements.tools, filter ) )
                    || ( search_component && !lcmatch_any( rec->requirements.components, filter ) ) ) {
                    continue;
                }
                bool match_found = false;
                if( search_result_qualities ) {
                    itype *it = item::find_type( rec->result );
                    for( auto &quality : it->qualities ) {
                        if( lcmatch( quality::get_name( quality.first ), filter ) ) {
                            match_found = true;
                            break;
                        }
                    }
                    if( !match_found ) {
                        continue;
                    }
                }
                if( search_qualities ) {
                    for( auto quality_reqs : rec->requirements.qualities ) {
                        for( auto quality : quality_reqs ) {
                            if( lcmatch( quality.to_string(), filter ) ) {
                                match_found = true;
                                break;
                            }
                        }
                        if( match_found ) {
                            break;
                        }
                    }
                    if( !match_found ) {
                        continue;
                    }
                }
                if( search_skill ) {
                    if( !rec->skill_used ) {
                        continue;
                    } else if( !lcmatch( rec->skill_used.obj().name(), filter ) &&
                               !lcmatch( rec->required_skills_string(), filter ) ) {
                        continue;
                    }
                }
                if( search_skill_primary_only ) {
                    if( !rec->skill_used ) {
                        continue;
                    } else if( !lcmatch( rec->skill_used.obj().name(), filter ) ) {
                        continue;
                    }
                }
            }

            filtered_list.push_back( rec );

        }
        max_difficulty = std::max( max_difficulty, rec->difficulty );
    }

    int truecount = 0;
    for( int i = max_difficulty; i != -1; --i ) {
        for( auto rec : filtered_list ) {
            if( rec->difficulty == i ) {
                if( rec->can_make_with_inventory( crafting_inv ) ) {
                    current.insert( current.begin(), rec );
                    available.insert( available.begin(), true );
                    truecount++;
                } else {
                    current.push_back( rec );
                    available.push_back( false );
                }
            }
        }
    }
    // This is so the list of available recipes is also is order of difficulty.
    std::reverse( current.begin(), current.begin() + truecount );
}
