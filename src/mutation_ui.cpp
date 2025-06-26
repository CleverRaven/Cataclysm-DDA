#include "character.h" // IWYU pragma: associated

#include <algorithm> //std::min
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>

#include "avatar.h"
#include "color.h"
#include "cursesdef.h"
#include "enums.h"
#include "input_context.h"
#include "inventory.h"
#include "magic.h"
#include "mutation.h"
#include "output.h"
#include "popup.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"
enum class mutation_menu_mode {
    activating,
    examining,
    reassigning,
    hiding
};
enum class mutation_tab_mode {
    active,
    passive,
    none
};
// '!' and '=' are uses as default bindings in the menu
static const invlet_wrapper
mutation_chars( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"#&()*+./:;@[\\]^_{|}" );

static void draw_exam_window( const catacurses::window &win, const int border_y )
{
    const int width = getmaxx( win );
    wattron( win, BORDER_COLOR );
    mvwaddch( win, point( 0, border_y ), LINE_XXXO );
    mvwhline( win, point( 1, border_y ), LINE_OXOX, width - 2 );
    mvwaddch( win, point( width - 1, border_y ), LINE_XOXX );
    wattroff( win, BORDER_COLOR );
}

static const auto shortcut_desc = []( std::string_view comment, const std::string &keys )
{
    return string_format( comment, string_format( "[<color_yellow>%s</color>]", keys ) );
};

// needs extensive improvement

static std::optional<trait_id> GetTrait( const std::vector<trait_id> &active,
        const std::vector<trait_id> &passive,
        int cursor, mutation_tab_mode tab_mode )
{
    std::optional<trait_id> mut_id;
    if( tab_mode == mutation_tab_mode::active ) {
        mut_id = active[cursor];
    } else if( tab_mode == mutation_tab_mode::passive ) {
        mut_id = passive[cursor];
    }
    return mut_id;
}

static void show_mutations_titlebar( const catacurses::window &window,
                                     const mutation_menu_mode menu_mode, const input_context &ctxt )
{
    werase( window );
    std::string desc;
    if( menu_mode == mutation_menu_mode::reassigning ) {
        desc += std::string( _( "Reassigning." ) ) + "  " +
                _( "Select a mutation to reassign or press [<color_yellow>SPACE</color>] to cancel. " );
    }
    if( menu_mode == mutation_menu_mode::activating ) {
        desc += colorize( _( "Activating" ),
                          c_green ) + "  " + shortcut_desc( _( "%s Examine, " ),
                                  ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode == mutation_menu_mode::examining ) {
        desc += colorize( _( "Examining" ),
                          c_light_blue ) + "  " + shortcut_desc( _( "%s Activate, " ),
                                  ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode == mutation_menu_mode::hiding ) {
        desc += colorize( _( "Hiding" ), c_cyan ) + "  " + shortcut_desc( _( "%s Activate, " ),
                ctxt.get_desc( "TOGGLE_EXAMINE" ) );
    }
    if( menu_mode != mutation_menu_mode::reassigning ) {
        desc += shortcut_desc( _( "%s Reassign, " ), ctxt.get_desc( "REASSIGN" ) );
    }
    desc += shortcut_desc( _( "%s Toggle sprite visibility, " ), ctxt.get_desc( "TOGGLE_SPRITE" ) );
    desc += shortcut_desc( _( "%s Change keybindings." ), ctxt.get_desc( "HELP_KEYBINDINGS" ) );
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    fold_and_print( window, point( 1, 0 ), getmaxx( window ) - 1, c_white, desc );
    wnoutrefresh( window );
}

void avatar::power_mutations()
{
    std::vector<trait_id> passive;
    std::vector<trait_id> active;
    for( std::pair<const trait_id, trait_data> &mut : cached_mutations ) {
        if( mut.second.corrupted > 0 ) {
            continue;
        }
        if( !mut.first->player_display ) {
            continue;
        }
        if( !mut.first->activated ) {
            passive.push_back( mut.first );
        } else {
            active.push_back( mut.first );
        }
        // New mutations are initialized with no key at all, so we have to do this here.
        if( mut.second.key == ' ' ) {
            for( const char &letter : mutation_chars ) {
                if( trait_by_invlet( letter ).is_null() && mut.first->activated ) {
                    mut.second.key = letter;
                    break;
                }
            }
        }
    }

    // maximal number of rows in both columns
    const int mutations_count = std::max( passive.size(), active.size() );
    const int TITLE_HEIGHT = 2;

    const int DESCRIPTION_HEIGHT = 5;
    // + lines with text in titlebar, local
    const int HEADER_LINE_Y = TITLE_HEIGHT + 1;
    const int list_start_y = HEADER_LINE_Y + 2;

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

    int HEIGHT = 0;
    int WIDTH = 0;
    catacurses::window wBio;

    int DESCRIPTION_LINE_Y = 0;
    catacurses::window w_description;

    catacurses::window w_title;

    int second_column = 0;

    int scroll_position = 0;
    int examine_pos = 0;
    int cursor = 0;
    int max_scroll_position = 0;
    int list_height = 0;
    int half_list_view_location = 0;
    mutation_menu_mode menu_mode = mutation_menu_mode::activating;
    mutation_tab_mode tab_mode;
    if( !active.empty() ) {
        tab_mode = mutation_tab_mode::active;
    } else if( !passive.empty() ) {
        tab_mode = mutation_tab_mode::passive;
    } else {
        tab_mode = mutation_tab_mode::none;
    }

    const auto recalc_max_scroll_position = [&]() {
        list_height = ( menu_mode == mutation_menu_mode::examining ?
                        DESCRIPTION_LINE_Y : HEIGHT - 1 ) - list_start_y;
        max_scroll_position = mutations_count - list_height;
        half_list_view_location = list_height / 2;
        if( max_scroll_position < 0 ) {
            scroll_position = 0;
        } else if( scroll_position > max_scroll_position ) {
            scroll_position = max_scroll_position;
        }
    };

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        HEIGHT = std::min( TERMY, std::max( FULL_SCREEN_HEIGHT,
                                            TITLE_HEIGHT + mutations_count + DESCRIPTION_HEIGHT + 5 ) );
        WIDTH = FULL_SCREEN_WIDTH + ( TERMX - FULL_SCREEN_WIDTH ) / 2;
        const point START( ( TERMX - WIDTH ) / 2, ( TERMY - HEIGHT ) / 2 );
        wBio = catacurses::newwin( HEIGHT, WIDTH, START );

        // Description window @ the bottom of the bionic window
        const int DESCRIPTION_START_Y = START.y + HEIGHT - DESCRIPTION_HEIGHT - 1;
        DESCRIPTION_LINE_Y = DESCRIPTION_START_Y - START.y - 1;
        w_description = catacurses::newwin( DESCRIPTION_HEIGHT, WIDTH - 2,
                                            point( START.x + 1, DESCRIPTION_START_Y ) );

        // Title window
        const int TITLE_START_Y = START.y + 1;
        w_title = catacurses::newwin( TITLE_HEIGHT, WIDTH - 2,
                                      point( START.x + 1, TITLE_START_Y ) );

        recalc_max_scroll_position();
        examine_pos = 0;

        // X-coordinate of the list of active mutations
        second_column = 32 + ( TERMX - FULL_SCREEN_WIDTH ) / 4;

        ui.position_from_window( wBio );
    } );
    ui.mark_resize();

    input_context ctxt( "MUTATIONS", keyboard_mode::keychar );
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "TOGGLE_EXAMINE" );
    ctxt.register_action( "TOGGLE_SPRITE" );
    ctxt.register_action( "REASSIGN" );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "SCROLL_TRAIT_INFO_UP" );
    ctxt.register_action( "SCROLL_TRAIT_INFO_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "QUIT" );
#if defined(__ANDROID__)
    for( const auto &p : passive ) {
        ctxt.register_manual_key( cached_mutations[p].key, p.obj().name() );
    }
    for( const auto &a : active ) {
        ctxt.register_manual_key( cached_mutations[a].key, a.obj().name() );
    }
#endif

    std::optional<trait_id> examine_id;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( wBio );
        draw_border( wBio, BORDER_COLOR, _( "Mutations" ) );
        wattron( wBio, BORDER_COLOR );
        // Draw line under title
        mvwhline( wBio, point( 1, HEADER_LINE_Y ), LINE_OXOX, WIDTH - 2 );
        // Draw symbols to connect additional lines to border
        mvwaddch( wBio, point( 0, HEADER_LINE_Y ), LINE_XXXO ); // |-
        mvwaddch( wBio, point( WIDTH - 1, HEADER_LINE_Y ), LINE_XOXX ); // -|
        wattroff( wBio, BORDER_COLOR );

        // Captions
        mvwprintz( wBio, point( 2, HEADER_LINE_Y + 1 ), c_light_blue, _( "Passive:" ) );
        mvwprintz( wBio, point( second_column, HEADER_LINE_Y + 1 ), c_light_blue, _( "Active:" ) );

        if( menu_mode == mutation_menu_mode::examining ) {
            draw_exam_window( wBio, DESCRIPTION_LINE_Y );
        }
        nc_color type;
        if( passive.empty() ) {
            mvwprintz( wBio, point( 2, list_start_y ), c_light_gray, _( "None" ) );
        } else {
            for( int i = scroll_position; static_cast<size_t>( i ) < passive.size(); i++ ) {
                const mutation_branch &md = passive[i].obj();
                const trait_data &td = cached_mutations[passive[i]];
                const bool is_highlighted = cursor == static_cast<int>( i );
                if( i - scroll_position == list_height ) {
                    break;
                }
                if( is_highlighted && tab_mode == mutation_tab_mode::passive ) {
                    type = has_base_trait( passive[i] ) ? c_cyan_yellow : c_light_cyan_yellow;
                } else {
                    type = has_base_trait( passive[i] ) ? c_cyan : c_light_cyan;
                }
                mvwprintz( wBio, point( 2, list_start_y + i - scroll_position ),
                           type, "%c %s", td.key, mutation_name( md.id ) );
                if( !td.show_sprite ) {
                    //~ Hint: Letter to show which mutation is Hidden in the mutation menu
                    wprintz( wBio, c_cyan, _( " H" ) );
                }
            }
        }

        if( active.empty() ) {
            mvwprintz( wBio, point( second_column, list_start_y ), c_light_gray, _( "None" ) );
        } else {
            for( int i = scroll_position; static_cast<size_t>( i ) < active.size(); i++ ) {
                const mutation_branch &md = active[i].obj();
                const trait_data &td = cached_mutations[active[i]];
                const bool is_highlighted = cursor == static_cast<int>( i );
                if( i - scroll_position == list_height ) {
                    break;
                }
                if( td.powered ) {
                    if( is_highlighted && tab_mode == mutation_tab_mode::active ) {
                        type = has_base_trait( active[i] ) ? c_green_yellow : c_light_green_yellow;
                    } else {
                        type = has_base_trait( active[i] ) ? c_green : c_light_green;
                    }
                } else {
                    if( is_highlighted && tab_mode == mutation_tab_mode::active ) {
                        type = has_base_trait( active[i] ) ? c_red_yellow : c_light_red_yellow;
                    } else {
                        type = has_base_trait( active[i] ) ? c_red : c_light_red;
                    }
                }
                // TODO: track resource(s) used and specify
                mvwputch( wBio, point( second_column, list_start_y + i - scroll_position ),
                          type, td.key );
                std::string mut_desc;
                std::string resource_unit;
                int number_of_resource = 0;
                if( md.hunger ) {
                    resource_unit += _( " kcal" );
                    number_of_resource++;
                }
                if( md.thirst ) {
                    if( number_of_resource > 0 ) {
                        //~ Resources consumed by a mutation: "kcal & thirst & sleepiness & mana"
                        resource_unit += _( " &" );
                    }
                    resource_unit += _( " thirst" );
                    number_of_resource++;
                }
                if( md.sleepiness ) {
                    if( number_of_resource > 0 ) {
                        //~ Resources consumed by a mutation: "kcal & thirst & sleepiness & mana"
                        resource_unit += _( " &" );
                    }
                    resource_unit += _( " sleepiness" );
                }
                if( md.mana ) {
                    resource_unit += _( " mana" );
                    number_of_resource++;
                }
                mut_desc += mutation_name( md.id );
                if( md.cost > 0 && md.cooldown > 0_turns ) {
                    mut_desc += string_format( _( " - %d%s / %s" ),
                                               md.cost, resource_unit, to_string_clipped( md.cooldown ) );
                } else if( md.cost > 0 ) {
                    mut_desc += string_format( _( " - %d%s" ), md.cost, resource_unit );
                } else if( md.cooldown > 0_turns ) {
                    mut_desc += string_format( _( " - %s" ), to_string_clipped( md.cooldown ) );
                }
                if( td.powered ) {
                    mut_desc += _( " - Active" );
                }
                mvwprintz( wBio, point( second_column + 2, list_start_y + i - scroll_position ),
                           type, mut_desc );
            }
        }

        draw_scrollbar( wBio, scroll_position, list_height, mutations_count,
                        point( 0, list_start_y ), c_white, true );

        if( menu_mode == mutation_menu_mode::examining && examine_id.has_value() ) {
            werase( w_description );
            std::string description = mutation_desc( examine_id.value() );
            if( !purifiable( examine_id.value() ) ) {
                description +=
                    _( "\n<color_yellow>This trait is an intrinsic part of you now, purifier won't be able to remove it.</color>" );
            }
            std::vector<std::string> desc = foldstring( description, WIDTH - 2 );
            const int winh = catacurses::getmaxy( w_description );
            const bool do_scroll = desc.size() > static_cast<unsigned>( std::abs( winh ) );
            const int fline = do_scroll ? examine_pos % ( desc.size() + 1 - winh ) : 0;
            const int lline = do_scroll ? fline + winh : desc.size();
            for( int i = fline; i < lline; i++ ) {
                trim_and_print( w_description, point( 0, i - fline ), WIDTH - 2, c_light_blue, desc[i] );
            }
            draw_scrollbar( wBio, fline, winh, desc.size(), point( 0, catacurses::getmaxy( wBio ) - winh - 1 ),
                            c_white, true );
        }
        wnoutrefresh( wBio );
        show_mutations_titlebar( w_title, menu_mode, ctxt );
        if( menu_mode == mutation_menu_mode::examining && examine_id.has_value() ) {
            wnoutrefresh( w_description );
        }
    } );

    bool exit = false;
    while( !exit ) {
        recalc_max_scroll_position();
        ui_manager::redraw();
        bool handled = false;
        const std::string action = ctxt.handle_input();
        const input_event evt = ctxt.get_raw_input();
        if( evt.type == input_event_t::keyboard_char && !evt.sequence.empty() ) {
            const int ch = evt.get_first_input();
            if( ch == ' ' ) { //skip if space is pressed (space is used as an empty hotkey)
                continue;
            }
            const trait_id mut_id = trait_by_invlet( ch );
            if( !mut_id.is_null() ) {
                const mutation_branch &mut_data = mut_id.obj();
                switch( menu_mode ) {
                    case mutation_menu_mode::reassigning: {
                        query_popup pop;
                        pop.message( _( "%s; enter new letter." ), mutation_name( mut_id ) )
                        .preferred_keyboard_mode( keyboard_mode::keychar )
                        .context( "POPUP_WAIT" )
                        .allow_cancel( true )
                        .allow_anykey( true );

                        bool pop_exit = false;
                        while( !pop_exit ) {
                            const query_popup::result ret = pop.query();
                            bool pop_handled = false;
                            if( ret.evt.type == input_event_t::keyboard_char && !ret.evt.sequence.empty() ) {
                                const int newch = ret.evt.get_first_input();
                                if( mutation_chars.valid( newch ) ) {
                                    const trait_id other_mut_id = trait_by_invlet( newch );
                                    if( !other_mut_id.is_null() ) {
                                        std::swap( cached_mutations[mut_id].key, cached_mutations[other_mut_id].key );
                                    } else {
                                        cached_mutations[mut_id].key = newch;
                                    }
                                    pop_exit = true;
                                    pop_handled = true;
                                }
                            }
                            if( !pop_handled ) {
                                if( ret.action == "QUIT" ) {
                                    pop_exit = true;
                                } else if( ret.action != "HELP_KEYBINDINGS" &&
                                           ret.evt.type == input_event_t::keyboard_char ) {
                                    popup( _( "Invalid mutation letter.  Only those characters are valid:\n\n%s" ),
                                           mutation_chars.get_allowed_chars() );
                                }
                            }
                        }

                        menu_mode = mutation_menu_mode::activating;
                        examine_id = std::nullopt;
                        // TODO: show a message like when reassigning a key to an item?
                        break;
                    }
                    case mutation_menu_mode::activating: {
                        if( mut_data.activated ) {
                            if( cached_mutations[mut_id].powered ) {
                                add_msg_if_player( m_neutral, _( "You stop using your %s." ), mutation_name( mut_data.id ) );
                                // Reset menu in advance
                                ui.reset();
                                deactivate_mutation( mut_id );
                                // Action done, leave screen
                                exit = true;
                            } else if( ( !mut_data.hunger || get_kcal_percent() >= 0.8f ) &&
                                       ( !mut_data.thirst || get_thirst() <= 400 ) &&
                                       ( !mut_data.sleepiness || get_sleepiness() <= 400 ) &&
                                       ( !mut_data.mana || magic->available_mana() >= mut_data.cost ) ) {
                                add_msg_if_player( m_neutral,
                                                   string_format( mut_data.activation_msg, mutation_name( mut_data.id ) ) );
                                // Reset menu in advance
                                ui.reset();
                                activate_mutation( mut_id );
                                // Action done, leave screen
                                exit = true;
                            } else {
                                popup( _( "You don't have enough in you to activate your %s!" ), mutation_name( mut_data.id ) );
                            }
                        } else {
                            popup( _( "You cannot activate %1$s!  To read a description of "
                                      "%1$s, press '%2$s', then '%3$c'." ),
                                   mutation_name( mut_data.id ), ctxt.get_desc( "TOGGLE_EXAMINE" ), cached_mutations[mut_id].key );
                        }
                        break;
                    }
                    case mutation_menu_mode::examining:
                        // Describing mutations, not activating them!
                        examine_id = mut_id;
                        break;
                    case mutation_menu_mode::hiding:
                        cached_mutations[mut_id].show_sprite = !cached_mutations[mut_id].show_sprite;
                        break;
                }
                handled = true;
            } else if( mutation_chars.valid( ch ) ) {
                handled = true;
            }
        }
        if( !handled ) {

            // Essentially, up-down navigation adapted from the bionics_ui.cpp, with a bunch of extra workarounds to keep functionality

            if( action == "DOWN" ) {

                int lowerlim;

                if( tab_mode == mutation_tab_mode::passive ) {
                    lowerlim = static_cast<int>( passive.size() ) - 1;
                } else if( tab_mode == mutation_tab_mode::active ) {
                    lowerlim = static_cast<int>( active.size() ) - 1;
                } else {
                    continue;
                }

                if( cursor < lowerlim ) {
                    cursor++;
                } else {
                    cursor = 0;
                }
                if( scroll_position < max_scroll_position &&
                    cursor - scroll_position > list_height - half_list_view_location ) {
                    scroll_position++;
                }
                if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                    scroll_position = std::max( cursor - half_list_view_location, 0 );
                }
                examine_pos = 0;

                // Draw the description, shabby workaround
                examine_id = GetTrait( active, passive, cursor, tab_mode );

            } else if( action == "UP" ) {

                int lim;
                if( tab_mode == mutation_tab_mode::passive ) {
                    lim = passive.size() - 1;
                } else if( tab_mode == mutation_tab_mode::active ) {
                    lim = active.size() - 1;
                } else {
                    continue;
                }
                if( cursor > 0 ) {
                    cursor--;
                } else {
                    cursor = lim;
                }
                if( scroll_position > 0 && cursor - scroll_position < half_list_view_location ) {
                    scroll_position--;
                }
                if( scroll_position < max_scroll_position &&
                    cursor - scroll_position > list_height - half_list_view_location ) {
                    scroll_position =
                        std::max( std::min<int>( lim + 1 - list_height,
                                                 cursor - half_list_view_location ), 0 );
                }
                examine_pos = 0;

                examine_id = GetTrait( active, passive, cursor, tab_mode );
            } else if( ( action == "SCROLL_TRAIT_INFO_UP" || action == "SCROLL_TRAIT_INFO_DOWN" ) &&
                       menu_mode == mutation_menu_mode::examining ) {
                examine_pos += action == "SCROLL_TRAIT_INFO_UP" ? -1 : 1;
            } else if( action == "NEXT_TAB" || action == "PREV_TAB" ) {
                if( tab_mode == mutation_tab_mode::active && !passive.empty() ) {
                    tab_mode = mutation_tab_mode::passive;
                } else if( tab_mode == mutation_tab_mode::passive && !active.empty() ) {
                    tab_mode = mutation_tab_mode::active;
                } else {
                    continue;
                }
                scroll_position = 0;
                examine_pos = 0;
                cursor = 0;
                examine_id = GetTrait( active, passive, cursor, tab_mode );
            } else if( action == "CONFIRM" ) {
                trait_id mut_id;
                if( tab_mode == mutation_tab_mode::active ) {
                    mut_id = active[cursor];
                } else if( tab_mode == mutation_tab_mode::passive ) {
                    mut_id = passive[cursor];
                } else {
                    continue;
                }
                if( !mut_id.is_null() ) {
                    const mutation_branch &mut_data = mut_id.obj();
                    switch( menu_mode ) {
                        case mutation_menu_mode::reassigning: {
                            query_popup pop;
                            pop.message( _( "%s; enter new letter." ), mutation_name( mut_id ) )
                            .preferred_keyboard_mode( keyboard_mode::keychar )
                            .context( "POPUP_WAIT" )
                            .allow_cancel( true )
                            .allow_anykey( true );

                            bool pop_exit = false;
                            while( !pop_exit ) {
                                const query_popup::result ret = pop.query();
                                bool pop_handled = false;
                                if( ret.evt.type == input_event_t::keyboard_char && !ret.evt.sequence.empty() ) {
                                    const int newch = ret.evt.get_first_input();
                                    if( mutation_chars.valid( newch ) ) {
                                        const trait_id other_mut_id = trait_by_invlet( newch );
                                        if( !other_mut_id.is_null() ) {
                                            std::swap( cached_mutations[mut_id].key, cached_mutations[other_mut_id].key );
                                        } else {
                                            cached_mutations[mut_id].key = newch;
                                        }
                                        pop_exit = true;
                                        pop_handled = true;
                                    } else if( newch == ' ' ) {
                                        cached_mutations[mut_id].key = newch;
                                        pop_exit = true;
                                        pop_handled = true;
                                    }
                                }
                                if( !pop_handled ) {
                                    if( ret.action == "QUIT" ) {
                                        pop_exit = true;
                                    } else if( ret.action != "HELP_KEYBINDINGS" &&
                                               ret.evt.type == input_event_t::keyboard_char ) {
                                        popup( _( "Invalid mutation letter.  Only those characters are valid:\n\n%s" ),
                                               mutation_chars.get_allowed_chars() );
                                    }
                                }
                            }

                            menu_mode = mutation_menu_mode::activating;
                            examine_id = std::nullopt;
                            // TODO: show a message like when reassigning a key to an item?
                            break;
                        }
                        case mutation_menu_mode::activating: {
                            if( mut_data.activated ) {
                                if( cached_mutations[mut_id].powered ) {
                                    add_msg_if_player( m_neutral, _( "You stop using your %s." ), mutation_name( mut_data.id ) );
                                    // Reset menu in advance
                                    ui.reset();
                                    deactivate_mutation( mut_id );
                                    // Action done, leave screen
                                    exit = true;
                                } else if( ( !mut_data.hunger || get_kcal_percent() >= 0.8f ) &&
                                           ( !mut_data.thirst || get_thirst() <= 400 ) &&
                                           ( !mut_data.sleepiness || get_sleepiness() <= 400 ) &&
                                           ( !mut_data.mana || magic->available_mana() >= mut_data.cost ) ) {
                                    add_msg_if_player( m_neutral,
                                                       string_format( mut_data.activation_msg, mutation_name( mut_data.id ) ) );
                                    // Reset menu in advance
                                    ui.reset();
                                    activate_mutation( mut_id );
                                    // Action done, leave screen
                                    exit = true;
                                } else {
                                    popup( _( "You don't have enough in you to activate your %s!" ), mutation_name( mut_data.id ) );
                                }
                            } else {
                                popup( _( "You cannot activate %1$s!  To read a description of "
                                          "%1$s, press '%2$s'." ),
                                       mutation_name( mut_data.id ), ctxt.get_desc( "TOGGLE_EXAMINE" ) );
                            }
                            break;
                        }
                        case mutation_menu_mode::examining:
                            // Describing mutations, not activating them!
                            examine_id = mut_id;
                            break;
                        case mutation_menu_mode::hiding:
                            cached_mutations[mut_id].show_sprite = !cached_mutations[mut_id].show_sprite;
                            break;
                    }
                }
            } else if( action == "REASSIGN" ) {
                menu_mode = mutation_menu_mode::reassigning;
                examine_id = std::nullopt;
            } else if( action == "TOGGLE_EXAMINE" ) {
                // switches between activation and examination
                menu_mode = menu_mode == mutation_menu_mode::activating ?
                            mutation_menu_mode::examining : mutation_menu_mode::activating;
                if( menu_mode == mutation_menu_mode::examining ) {
                    examine_id = GetTrait( active, passive, cursor, tab_mode );
                } else {
                    examine_id = std::nullopt;
                }
                examine_pos = 0;
            } else if( action == "TOGGLE_SPRITE" ) {
                menu_mode = mutation_menu_mode::hiding;
                examine_id = std::nullopt;
            } else if( action == "QUIT" ) {
                exit = true;
            }
        }
    }
}
