#include "player.h" // IWYU pragma: associated

#include <algorithm> //std::min
#include <sstream>
#include <cstddef>

#include "mutation.h"
#include "game.h"
#include "input.h"
#include "output.h"
#include "string_formatter.h"
#include "translations.h"
#include "string_id.h"
#include "enums.h"

// '!' and '=' are uses as default bindings in the menu
const invlet_wrapper
mutation_chars( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"#&()*+./:;@[\\]^_{|}" );

static void draw_exam_window( const catacurses::window &win, const int border_y )
{
    const int width = getmaxx( win );
    mvwputch( win, point( 0, border_y ), BORDER_COLOR, LINE_XXXO );
    mvwhline( win, point( 1, border_y ), LINE_OXOX, width - 2 );
    mvwputch( win, point( width - 1, border_y ), BORDER_COLOR, LINE_XOXX );
}

const auto shortcut_desc = []( const std::string &comment, const std::string &keys )
{
    return string_format( comment, string_format( "<color_yellow>%s</color>", keys ) );
};

static void show_mutations_titlebar( const catacurses::window &window,
                                     const std::string &menu_mode, const input_context &ctxt )
{
    werase( window );
    std::ostringstream desc;
    if( menu_mode == "reassigning" ) {
        desc << _( "Reassigning." ) << "  " <<
             _( "Select a mutation to reassign or press <color_yellow>SPACE</color> to cancel. " );
    }
    if( menu_mode == "activating" ) {
        desc << "<color_green>" << _( "Activating" ) << "</color>  " <<
             shortcut_desc( _( "%s to examine mutation, " ), ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode == "examining" ) {
        desc << "<color_light_blue>" << _( "Examining" ) << "</color>  " <<
             shortcut_desc( _( "%s to activate mutation, " ), ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode != "reassigning" ) {
        desc << shortcut_desc( _( "%s to reassign invlet, " ), ctxt.get_desc( "REASSIGN" ) );
    }
    desc << shortcut_desc( _( "%s to assign the hotkeys." ), ctxt.get_desc( "HELP_KEYBINDINGS" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    fold_and_print( window, point( 1, 0 ), getmaxx( window ) - 1, c_white, desc.str() );
    wrefresh( window );
}

void player::power_mutations()
{
    if( !is_player() ) {
        // TODO: Implement NPCs activating mutations
        return;
    }

    std::vector<trait_id> passive;
    std::vector<trait_id> active;
    for( auto &mut : my_mutations ) {
        if( !mut.first->activated ) {
            passive.push_back( mut.first );
        } else {
            active.push_back( mut.first );
        }
        // New mutations are initialized with no key at all, so we have to do this here.
        if( mut.second.key == ' ' ) {
            for( const auto &letter : mutation_chars ) {
                if( trait_by_invlet( letter ).is_null() ) {
                    mut.second.key = letter;
                    break;
                }
            }
        }
    }

    // maximal number of rows in both columns
    const int mutations_count = std::max( passive.size(), active.size() );

    int TITLE_HEIGHT = 2;
    int DESCRIPTION_HEIGHT = 5;

    // Main window
    /** Total required height is:
    * top frame line:                                         + 1
    * height of title window:                                 + TITLE_HEIGHT
    * line after the title:                                   + 1
    * line with active/passive mutation captions:               + 1
    * height of the biggest list of active/passive mutations:   + mutations_count
    * line before mutation description:                         + 1
    * height of description window:                           + DESCRIPTION_HEIGHT
    * bottom frame line:                                      + 1
    * TOTAL: TITLE_HEIGHT + mutations_count + DESCRIPTION_HEIGHT + 5
    */
    int HEIGHT = std::min( TERMY, std::max( FULL_SCREEN_HEIGHT,
                                            TITLE_HEIGHT + mutations_count + DESCRIPTION_HEIGHT + 5 ) );
    int WIDTH = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) / 2;
    int START_X = ( TERMX - WIDTH ) / 2;
    int START_Y = ( TERMY - HEIGHT ) / 2;
    catacurses::window wBio = catacurses::newwin( HEIGHT, WIDTH, point( START_X, START_Y ) );

    // Description window @ the bottom of the bionic window
    int DESCRIPTION_START_Y = START_Y + HEIGHT - DESCRIPTION_HEIGHT - 1;
    int DESCRIPTION_LINE_Y = DESCRIPTION_START_Y - START_Y - 1;
    catacurses::window w_description = catacurses::newwin( DESCRIPTION_HEIGHT, WIDTH - 2,
                                       point( START_X + 1, DESCRIPTION_START_Y ) );

    // Title window
    int TITLE_START_Y = START_Y + 1;
    int HEADER_LINE_Y = TITLE_HEIGHT + 1; // + lines with text in titlebar, local
    catacurses::window w_title = catacurses::newwin( TITLE_HEIGHT, WIDTH - 2, point( START_X + 1,
                                 TITLE_START_Y ) );

    int scroll_position = 0;
    int second_column = 32 + ( TERMX - FULL_SCREEN_WIDTH ) /
                        4; // X-coordinate of the list of active mutations

    input_context ctxt( "MUTATIONS" );
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "TOGGLE_EXAMINE" );
    ctxt.register_action( "REASSIGN" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
#if defined(__ANDROID__)
    for( const auto &p : passive ) {
        ctxt.register_manual_key( my_mutations[p].key, p.obj().name() );
    }
    for( const auto &a : active ) {
        ctxt.register_manual_key( my_mutations[a].key, a.obj().name() );
    }
#endif

    bool redraw = true;
    std::string menu_mode = "activating";

    while( true ) {
        // offset for display: mutation with index i is drawn at y=list_start_y+i
        // drawing the mutation starts with mutation[scroll_position]
        const int list_start_y = HEADER_LINE_Y + 2 - scroll_position;
        int max_scroll_position = HEADER_LINE_Y + 2 + mutations_count -
                                  ( menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1 );
        if( redraw ) {
            redraw = false;

            werase( wBio );
            draw_border( wBio, BORDER_COLOR, _( " MUTATIONS " ) );
            // Draw line under title
            mvwhline( wBio, point( 1, HEADER_LINE_Y ), LINE_OXOX, WIDTH - 2 );
            // Draw symbols to connect additional lines to border
            mvwputch( wBio, point( 0, HEADER_LINE_Y ), BORDER_COLOR, LINE_XXXO ); // |-
            mvwputch( wBio, point( WIDTH - 1, HEADER_LINE_Y ), BORDER_COLOR, LINE_XOXX ); // -|

            // Captions
            mvwprintz( wBio, point( 2, HEADER_LINE_Y + 1 ), c_light_blue, _( "Passive:" ) );
            mvwprintz( wBio, point( second_column, HEADER_LINE_Y + 1 ), c_light_blue, _( "Active:" ) );

            if( menu_mode == "examining" ) {
                draw_exam_window( wBio, DESCRIPTION_LINE_Y );
            }
            nc_color type;
            if( passive.empty() ) {
                mvwprintz( wBio, point( 2, list_start_y ), c_light_gray, _( "None" ) );
            } else {
                for( size_t i = scroll_position; i < passive.size(); i++ ) {
                    const auto &md = passive[i].obj();
                    const auto &td = my_mutations[passive[i]];
                    if( list_start_y + static_cast<int>( i ) ==
                        ( menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1 ) ) {
                        break;
                    }
                    type = has_base_trait( passive[i] ) ? c_cyan : c_light_cyan;
                    mvwprintz( wBio, point( 2, list_start_y + i ), type, "%c %s", td.key, md.name() );
                }
            }

            if( active.empty() ) {
                mvwprintz( wBio, point( second_column, list_start_y ), c_light_gray, _( "None" ) );
            } else {
                for( size_t i = scroll_position; i < active.size(); i++ ) {
                    const auto &md = active[i].obj();
                    const auto &td = my_mutations[active[i]];
                    if( list_start_y + static_cast<int>( i ) ==
                        ( menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1 ) ) {
                        break;
                    }
                    if( td.powered ) {
                        type = has_base_trait( active[i] ) ? c_green : c_light_green;
                    } else {
                        type = has_base_trait( active[i] ) ? c_red : c_light_red;
                    }
                    // TODO: track resource(s) used and specify
                    mvwputch( wBio, point( second_column, list_start_y + i ), type, td.key );
                    std::ostringstream mut_desc;
                    mut_desc << md.name();
                    if( md.cost > 0 && md.cooldown > 0 ) {
                        //~ RU means Resource Units
                        mut_desc << string_format( _( " - %d RU / %d turns" ),
                                                   md.cost, md.cooldown );
                    } else if( md.cost > 0 ) {
                        //~ RU means Resource Units
                        mut_desc << string_format( _( " - %d RU" ), md.cost );
                    } else if( md.cooldown > 0 ) {
                        mut_desc << string_format( _( " - %d turns" ), md.cooldown );
                    }
                    if( td.powered ) {
                        mut_desc << _( " - Active" );
                    }
                    mvwprintz( wBio, point( second_column + 2, list_start_y + i ), type,
                               mut_desc.str() );
                }
            }

            // Scrollbar
            if( scroll_position > 0 ) {
                mvwputch( wBio, point( 0, HEADER_LINE_Y + 2 ), c_light_green, '^' );
            }
            if( scroll_position < max_scroll_position && max_scroll_position > 0 ) {
                mvwputch( wBio, point( 0, ( menu_mode == "examining" ? DESCRIPTION_LINE_Y : HEIGHT - 1 ) - 1 ),
                          c_light_green, 'v' );
            }
        }
        wrefresh( wBio );
        show_mutations_titlebar( w_title, menu_mode, ctxt );
        const std::string action = ctxt.handle_input();
        const int ch = ctxt.get_raw_input().get_first_input();
        if( menu_mode == "reassigning" ) {
            menu_mode = "activating";
            const auto mut_id = trait_by_invlet( ch );
            if( mut_id.is_null() ) {
                // Selected an non-existing mutation (or escape, or ...)
                continue;
            }
            redraw = true;
            const int newch = popup_getkey( _( "%s; enter new letter." ),
                                            mutation_branch::get_name( mut_id ) );
            wrefresh( wBio );
            if( newch == ch || newch == ' ' || newch == KEY_ESCAPE ) {
                continue;
            }
            if( !mutation_chars.valid( newch ) ) {
                popup( _( "Invalid mutation letter. Only those characters are valid:\n\n%s" ),
                       mutation_chars.get_allowed_chars() );
                continue;
            }
            const auto other_mut_id = trait_by_invlet( newch );
            if( !other_mut_id.is_null() ) {
                std::swap( my_mutations[mut_id].key, my_mutations[other_mut_id].key );
            } else {
                my_mutations[mut_id].key = newch;
            }
            // TODO: show a message like when reassigning a key to an item?
        } else if( action == "DOWN" ) {
            if( scroll_position < max_scroll_position ) {
                scroll_position++;
                redraw = true;
            }
        } else if( action == "UP" ) {
            if( scroll_position > 0 ) {
                scroll_position--;
                redraw = true;
            }
        } else if( action == "REASSIGN" ) {
            menu_mode = "reassigning";
        } else if( action == "TOGGLE_EXAMINE" ) { // switches between activation and examination
            menu_mode = menu_mode == "activating" ? "examining" : "activating";
            werase( w_description );
            redraw = true;
        } else if( action == "HELP_KEYBINDINGS" ) {
            redraw = true;
        } else {
            const auto mut_id = trait_by_invlet( ch );
            if( mut_id.is_null() ) {
                // entered a key that is not mapped to any mutation,
                // -> leave screen
                break;
            }
            const auto &mut_data = mut_id.obj();
            if( menu_mode == "activating" ) {
                if( mut_data.activated ) {
                    if( my_mutations[mut_id].powered ) {
                        add_msg_if_player( m_neutral, _( "You stop using your %s." ), mut_data.name() );

                        deactivate_mutation( mut_id );
                        // Action done, leave screen
                        break;
                    } else if( ( !mut_data.hunger || get_kcal_percent() >= 0.8f ) &&
                               ( !mut_data.thirst || get_thirst() <= 400 ) &&
                               ( !mut_data.fatigue || get_fatigue() <= 400 ) ) {

                        g->draw();
                        add_msg_if_player( m_neutral, _( "You activate your %s." ), mut_data.name() );
                        activate_mutation( mut_id );
                        // Action done, leave screen
                        break;
                    } else {
                        popup( _( "You don't have enough in you to activate your %s!" ), mut_data.name() );
                        redraw = true;
                        continue;
                    }
                } else {
                    popup( _( "\
You cannot activate %s!  To read a description of \
%s, press '!', then '%c'." ), mut_data.name(), mut_data.name(),
                           my_mutations[mut_id].key );
                    redraw = true;
                }
            }
            if( menu_mode == "examining" ) { // Describing mutations, not activating them!
                draw_exam_window( wBio, DESCRIPTION_LINE_Y );
                // Clear the lines first
                werase( w_description );
                fold_and_print( w_description, point_zero, WIDTH - 2, c_light_blue, mut_data.desc() );
                wrefresh( w_description );
            }
        }
    }
}
