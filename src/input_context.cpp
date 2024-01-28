#include "input_context.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <set>
#include <utility>

#include "action.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "game.h"
#include "help.h"
#include "input.h"
#include "map.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "popup.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui_manager.h"

static const std::string default_context_id( "default" );

static constexpr int LEGEND_HEIGHT = 9;
static constexpr int BORDER_SPACE = 2;

template <class T1, class T2>
struct ContainsPredicate {
    const T1 &container;

    explicit ContainsPredicate( const T1 &container ) : container( container ) { }

    // Operator overload required to leverage std functional interface.
    bool operator()( T2 c ) {
        return std::find( container.begin(), container.end(), c ) != container.end();
    }
};

bool input_context::action_uses_input( const std::string &action_id,
                                       const input_event &event ) const
{
    const auto &events = inp_mngr.get_action_attributes( action_id, category ).input_events;
    return std::find( events.begin(), events.end(), event ) != events.end();
}

std::string input_context::get_conflicts(
    const input_event &event, const std::string &ignore_action ) const
{
    return enumerate_as_string( registered_actions.begin(), registered_actions.end(),
    [ this, &event, &ignore_action ]( const std::string & action ) {
        return action != ignore_action && action_uses_input( action, event )
               ? get_action_name( action ) : std::string();
    } );
}

void input_context::clear_conflicting_keybindings( const input_event &event )
{
    // The default context is always included to cover cases where the same
    // keybinding exists for the same action in both the global and local
    // contexts.
    input_manager::t_actions &default_actions = inp_mngr.action_contexts[default_context_id];
    input_manager::t_actions &category_actions = inp_mngr.action_contexts[category];

    for( const auto &registered_action : registered_actions ) {
        input_manager::t_actions::iterator default_action = default_actions.find( registered_action );
        input_manager::t_actions::iterator category_action = category_actions.find( registered_action );
        if( default_action != default_actions.end() ) {
            std::vector<input_event> &events = default_action->second.input_events;
            events.erase( std::remove( events.begin(), events.end(), event ), events.end() );
        }
        if( category_action != category_actions.end() ) {
            std::vector<input_event> &events = category_action->second.input_events;
            events.erase( std::remove( events.begin(), events.end(), event ), events.end() );
        }
    }
}

static const std::string CATA_ERROR = "ERROR";
static const std::string ANY_INPUT = "ANY_INPUT";
static const std::string HELP_KEYBINDINGS = "HELP_KEYBINDINGS";
static const std::string COORDINATE = "COORDINATE";
static const std::string TIMEOUT = "TIMEOUT";

const std::string &input_context::input_to_action( const input_event &inp ) const
{
    for( const std::string &elem : registered_actions ) {
        const std::string &action = elem;
        const std::vector<input_event> &check_inp = inp_mngr.get_input_for_action( action, category );

        // Does this action have our queried input event in its keybindings?
        for( const input_event &check_inp_i : check_inp ) {
            if( check_inp_i == inp ) {
                return action;
            }
        }
    }
    return CATA_ERROR;
}

#if defined(__ANDROID__)
std::list<input_context *> input_context::input_context_stack;

void input_context::register_manual_key( manual_key mk )
{
    // Prevent duplicates
    for( const manual_key &manual_key : registered_manual_keys )
        if( manual_key.key == mk.key ) {
            return;
        }

    registered_manual_keys.push_back( mk );
}

void input_context::register_manual_key( int key, const std::string text )
{
    // Prevent duplicates
    for( const manual_key &manual_key : registered_manual_keys )
        if( manual_key.key == key ) {
            return;
        }

    registered_manual_keys.push_back( manual_key( key, text ) );
}
#endif

void input_context::register_action( const std::string &action_descriptor )
{
    register_action( action_descriptor, translation() );
}

void input_context::register_action( const std::string &action_descriptor,
                                     const translation &name )
{
    if( action_descriptor == "ANY_INPUT" ) {
        registered_any_input = true;
    } else if( action_descriptor == "COORDINATE" ) {
        handling_coordinate_input = true;
    }

    if( std::find( registered_actions.begin(), registered_actions.end(),
                   action_descriptor ) == registered_actions.end() ) {
        registered_actions.push_back( action_descriptor );
    }
    if( !name.empty() ) {
        action_name_overrides[action_descriptor] = name;
    }
}

std::vector<input_event> input_context::keys_bound_to( const std::string &action_descriptor,
        const int maximum_modifier_count,
        const bool restrict_to_printable,
        const bool restrict_to_keyboard ) const
{
    std::vector<input_event> result;
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_descriptor,
            category );
    for( const input_event &events_event : events ) {
        // Ignore non-keyboard input
        if( ( !restrict_to_keyboard || ( events_event.type == input_event_t::keyboard_char
                                         || events_event.type == input_event_t::keyboard_code ) )
            && is_event_type_enabled( events_event.type )
            && events_event.sequence.size() == 1
            && ( maximum_modifier_count < 0
                 || events_event.modifiers.size() <= static_cast<size_t>( maximum_modifier_count ) ) ) {
            if( !restrict_to_printable || ( events_event.sequence.front() < 0xFF &&
                                            events_event.sequence.front() != ' ' &&
                                            isprint( events_event.sequence.front() ) ) ) {
                result.emplace_back( events_event );
            }
        }
    }
    return result;
}

std::string input_context::get_available_single_char_hotkeys( std::string requested_keys )
{
    for( const auto &registered_action : registered_actions ) {

        const std::vector<input_event> &events = inp_mngr.get_input_for_action( registered_action,
                category );
        for( const input_event &events_event : events ) {
            // Only consider keyboard events without modifiers
            if( events_event.type == input_event_t::keyboard_char && events_event.modifiers.empty() ) {
                requested_keys.erase( std::remove_if( requested_keys.begin(), requested_keys.end(),
                                                      ContainsPredicate<std::vector<int>, char>(
                                                              events_event.sequence ) ),
                                      requested_keys.end() );
            }
        }
    }

    return requested_keys;
}

bool input_context::disallow_lower_case_or_non_modified_letters( const input_event &evt )
{
    const int ch = evt.get_first_input();
    switch( evt.type ) {
        case input_event_t::keyboard_char:
            // std::lower from <cctype> is undefined outside unsigned char range
            // and std::lower from <locale> may throw bad_cast for some locales
            return ch < 'a' || ch > 'z';
        case input_event_t::keyboard_code:
            return !( evt.modifiers.empty() && ( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) ) );
        default:
            return true;
    }
}

bool input_context::allow_all_keys( const input_event & )
{
    return true;
}

std::string input_context::get_desc( const std::string &action_descriptor,
                                     const unsigned int max_limit,
                                     const input_context::input_event_filter &evt_filter ) const
{
    if( action_descriptor == "ANY_INPUT" ) {
        return "(*)"; // * for wildcard
    }

    bool is_local = false;
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_descriptor,
            category, &is_local );

    if( events.empty() ) {
        return is_local ? _( "Unbound locally!" ) : _( "Unbound globally!" );
    }

    std::vector<input_event> inputs_to_show;
    for( const input_event &events_i : events ) {
        const input_event &event = events_i;

        if( is_event_type_enabled( event.type ) && evt_filter( event ) ) {
            inputs_to_show.push_back( event );
        }

        if( max_limit > 0 && inputs_to_show.size() == max_limit ) {
            break;
        }
    }

    if( inputs_to_show.empty() ) {
        return pgettext( "keybinding", "None applicable" );
    }

    const std::string separator = inputs_to_show.size() > 2 ? _( ", or " ) : _( " or " );
    std::string rval;
    for( size_t i = 0; i < inputs_to_show.size(); ++i ) {
        rval += inputs_to_show[i].long_description();

        // We're generating a list separated by "," and "or"
        if( i + 2 == inputs_to_show.size() ) {
            rval += separator;
        } else if( i + 1 < inputs_to_show.size() ) {
            rval += ", ";
        }
    }
    return rval;
}

std::string input_context::get_desc(
    const std::string &action_descriptor,
    const std::string &text,
    const input_context::input_event_filter &evt_filter,
    const translation &inline_fmt,
    const translation &separate_fmt ) const
{
    if( action_descriptor == "ANY_INPUT" ) {
        //~ keybinding description for anykey
        return string_format( separate_fmt, pgettext( "keybinding", "any" ), text );
    }

    const auto &events = inp_mngr.get_input_for_action( action_descriptor, category );

    bool na = true;
    for( const input_event &evt : events ) {
        if( is_event_type_enabled( evt.type ) && evt_filter( evt ) ) {
            na = false;
            if( ( evt.type == input_event_t::keyboard_char || evt.type == input_event_t::keyboard_code ) &&
                evt.modifiers.empty() && evt.sequence.size() == 1 ) {
                const int ch = evt.get_first_input();
                if( ch > ' ' && ch <= '~' ) {
                    const std::string key = utf32_to_utf8( ch );
                    const int pos = ci_find_substr( text, key );
                    if( pos >= 0 ) {
                        return string_format( inline_fmt, text.substr( 0, pos ),
                                              key, text.substr( pos + key.size() ) );
                    }
                }
            }
        }
    }

    if( na ) {
        //~ keybinding description for unbound or non-applicable keys
        return string_format( separate_fmt, pgettext( "keybinding", "n/a" ), text );
    } else {
        return string_format( separate_fmt, get_desc( action_descriptor, 1, evt_filter ), text );
    }
}

std::string input_context::get_desc(
    const std::string &action_descriptor,
    const std::string &text,
    const input_event_filter &evt_filter ) const
{

    // Keybinding tips
    static const translation inline_fmt = to_translation(
            //~ %1$s: action description text before key,
            //~ %2$s: key description,
            //~ %3$s: action description text after key.
            "keybinding", "%1$s[<color_light_green>%2$s</color>]%3$s" );
    static const translation separate_fmt = to_translation(
            // \u00A0 is the non-breaking space
            //~ %1$s: key description,
            //~ %2$s: action description.
            "keybinding", "[<color_light_green>%1$s</color>]\u00A0%2$s" );

    return get_desc( action_descriptor, text, evt_filter, inline_fmt,
                     separate_fmt );
}

std::string input_context::describe_key_and_name( const std::string &action_descriptor,
        const input_context::input_event_filter &evt_filter ) const
{
    return get_desc( action_descriptor, get_action_name( action_descriptor ), evt_filter );
}

const std::string &input_context::handle_input()
{
    return handle_input( timeout );
}

const std::string &input_context::handle_input( const int timeout )
{
    const int old_timeout = inp_mngr.get_timeout();
    if( timeout >= 0 ) {
        inp_mngr.set_timeout( timeout );
    }
    next_action.type = input_event_t::error;
    const std::string *result = &CATA_ERROR;
    while( true ) {

        next_action = inp_mngr.get_input_event( preferred_keyboard_mode );
        if( next_action.type == input_event_t::timeout ) {
            result = &TIMEOUT;
            break;
        }

        const std::string &action = input_to_action( next_action );

        //Special global key to toggle language to english and back
        if( action == "toggle_language_to_en" ) {
            g->toggle_language_to_en();
            ui_manager::invalidate_all_ui_adaptors();
            ui_manager::redraw_invalidated();
        }

        // Special help action
        if( action == "HELP_KEYBINDINGS" ) {
            inp_mngr.reset_timeout();
            display_menu();
            inp_mngr.set_timeout( timeout );
            result = &HELP_KEYBINDINGS;
            break;
        }

        if( next_action.type == input_event_t::mouse
            && !handling_coordinate_input && action == CATA_ERROR ) {
            continue; // Ignore mouse movement.
        }
        coordinate_input_received = true;
        coordinate = next_action.mouse_pos;

        if( action != CATA_ERROR ) {
            result = &action;
            break;
        }

        // If we registered to receive any input, return ANY_INPUT
        // to signify that an unregistered key was pressed.
        if( registered_any_input ) {
            result = &ANY_INPUT;
            break;
        }

        // If it's an invalid key, just keep looping until the user
        // enters something proper.
    }
    inp_mngr.set_timeout( old_timeout );
    return *result;
}

void input_context::register_directions()
{
    register_cardinal();
    register_action( "LEFTUP" );
    register_action( "LEFTDOWN" );
    register_action( "RIGHTUP" );
    register_action( "RIGHTDOWN" );
}

void input_context::register_updown()
{
    register_action( "UP" );
    register_action( "DOWN" );
}

void input_context::register_leftright()
{
    register_action( "LEFT" );
    register_action( "RIGHT" );
}

void input_context::register_cardinal()
{
    register_updown();
    register_leftright();
}

void input_context::register_navigate_ui_list()
{
    register_action( "UP", to_translation( "Move cursor up" ) );
    register_action( "DOWN", to_translation( "Move cursor down" ) );
    register_action( "SCROLL_UP", to_translation( "Move cursor up" ) );
    register_action( "SCROLL_DOWN", to_translation( "Move cursor down" ) );
    register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    register_action( "HOME", to_translation( "Scroll to top" ) );
    register_action( "END", to_translation( "Scroll to bottom" ) );
}

// rotate a delta direction clockwise
// dx and dy are -1, 0, or +1. Rotate the indicated direction 1/8 turn clockwise.
static void rotate_direction_cw( int &dx, int &dy )
{
    // convert to
    // 0 1 2
    // 3 4 5
    // 6 7 8
    int dir_num = ( dy + 1 ) * 3 + dx + 1;
    // rotate to
    // 1 2 5
    // 0 4 8
    // 3 6 7
    static const std::array<int, 9> rotate_direction_vec = {{ 1, 2, 5, 0, 4, 8, 3, 6, 7 }};
    dir_num = rotate_direction_vec[dir_num];
    // convert back to -1,0,+1
    dx = dir_num % 3 - 1;
    dy = dir_num / 3 - 1;
}

std::optional<tripoint> input_context::get_direction( const std::string &action ) const
{
    static const auto noop = static_cast<tripoint( * )( tripoint )>( []( tripoint p ) {
        return p;
    } );
    static const auto rotate = static_cast<tripoint( * )( tripoint )>( []( tripoint p ) {
        rotate_direction_cw( p.x, p.y );
        return p;
    } );
    const auto transform = iso_mode && g->is_tileset_isometric() ? rotate : noop;

    if( action == "UP" ) {
        return transform( tripoint_north );
    } else if( action == "DOWN" ) {
        return transform( tripoint_south );
    } else if( action == "LEFT" ) {
        return transform( tripoint_west );
    } else if( action == "RIGHT" ) {
        return transform( tripoint_east );
    } else if( action == "LEFTUP" ) {
        return transform( tripoint_north_west );
    } else if( action == "RIGHTUP" ) {
        return transform( tripoint_north_east );
    } else if( action == "LEFTDOWN" ) {
        return transform( tripoint_south_west );
    } else if( action == "RIGHTDOWN" ) {
        return transform( tripoint_south_east );
    } else {
        return std::nullopt;
    }
}

// Custom set of hotkeys that explicitly don't include the hardcoded
// alternative hotkeys, which mustn't be included so that the hardcoded
// hotkeys do not show up beside entries within the window.
static const std::string display_help_hotkeys =
    "abcdefghijkpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:;'\",/<>?!@#$%^&*()_[]\\{}|`~";

namespace
{
enum class fallback_action {
    add_local,
    add_global,
    remove,
    execute,
};
} // namespace
static const std::map<fallback_action, int> fallback_keys = {
    { fallback_action::add_local, '+' },
    { fallback_action::add_global, '=' },
    { fallback_action::remove, '-' },
    { fallback_action::execute, '.' },
};

action_id input_context::display_menu( const bool permit_execute_action )
{
    action_id action_to_execute = ACTION_NULL;

    input_context ctxt( "HELP_KEYBINDINGS", keyboard_mode::keychar );
    // Keybinding menu actions
    ctxt.register_action( "COORDINATE" );
    ctxt.register_action( "MOUSE_MOVE" );
    ctxt.register_action( "SELECT" );
    ctxt.register_action( "SCROLL_UP" );
    ctxt.register_action( "SCROLL_DOWN" );
    ctxt.register_action( "UP", to_translation( "Scroll up" ) );
    ctxt.register_action( "DOWN", to_translation( "Scroll down" ) );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "REMOVE" );
    ctxt.register_action( "ADD_LOCAL" );
    ctxt.register_action( "ADD_GLOBAL" );
    if( permit_execute_action ) {
        ctxt.register_action( "EXECUTE" );
    }
    ctxt.register_action( "QUIT" );
    // String input actions
    ctxt.register_action( "TEXT.LEFT" );
    ctxt.register_action( "TEXT.RIGHT" );
    ctxt.register_action( "TEXT.CLEAR" );
    ctxt.register_action( "TEXT.BACKSPACE" );
    ctxt.register_action( "TEXT.HOME" );
    ctxt.register_action( "TEXT.END" );
    ctxt.register_action( "TEXT.DELETE" );
#if defined( TILES )
    ctxt.register_action( "TEXT.PASTE" );
#endif
    ctxt.register_action( "TEXT.INPUT_FROM_FILE" );
    ctxt.register_action( "ANY_INPUT" );

    if( category != "HELP_KEYBINDINGS" ) {
        // avoiding inception!
        ctxt.register_action( "HELP_KEYBINDINGS" );
    }

    std::string hotkeys = ctxt.get_available_single_char_hotkeys( display_help_hotkeys );

    ui_adaptor ui;
    int width = 0;
    catacurses::window w_help;
    size_t display_height = 0;
    size_t legwidth = 0;
    string_input_popup spopup;
    // ignore hardcoded keys in string input popup
    for( const std::pair<const fallback_action, int> &v : fallback_keys ) {
        spopup.add_callback( v.second, []() {
            return true;
        } );
    }
    const auto recalc_size = [&]( ui_adaptor & ui ) {
        width = TERMX >= 100 ? 100 : 80;
        w_help = catacurses::newwin( TERMY, width - 2, point( TERMX / 2 - width / 2, 0 ) );
        // height of the area usable for display of keybindings, excludes headers & borders
        display_height = TERMY - LEGEND_HEIGHT;
        const point filter_pos( 4, 6 );
        // width of the legend
        legwidth = width - filter_pos.x * 2 - BORDER_SPACE;
        // +1 for end-of-text cursor
        spopup.window( w_help, filter_pos, filter_pos.x + legwidth + 1 )
        .max_length( legwidth )
        .context( ctxt );
        ui.position_from_window( w_help );
    };
    recalc_size( ui );
    ui.on_screen_resize( recalc_size );

    // has the user changed something?
    bool changed = false;
    // keybindings before the user changed anything.
    input_manager::t_action_contexts old_action_contexts( inp_mngr.action_contexts );
    // current status: adding/removing/executing/showing keybindings
    enum { s_remove, s_add, s_add_global, s_execute, s_show } status = s_show;
    // copy of registered_actions, but without the ANY_INPUT and COORDINATE, which should not be shown
    std::vector<std::string> org_registered_actions( registered_actions );
    org_registered_actions.erase( std::remove_if( org_registered_actions.begin(),
                                  org_registered_actions.end(),
    []( const std::string & a ) {
        return a == ANY_INPUT || a == COORDINATE;
    } ), org_registered_actions.end() );

    // colors of the keybindings
    static const nc_color global_key = c_light_gray;
    static const nc_color h_global_key = h_light_gray;
    static const nc_color local_key = c_light_green;
    static const nc_color h_local_key = h_light_green;
    static const nc_color unbound_key = c_light_red;
    static const nc_color h_unbound_key = h_light_red;

    enum class kb_btn_idx { none, remove, add_local, add_global } highlighted_btn_index =
        kb_btn_idx::none;
    // (vertical) scroll offset
    size_t scroll_offset = 0;
    // keybindings help
    std::string legend;
    legend += colorize( _( "Unbound keys" ), unbound_key ) + "\n";
    legend += colorize( _( "Keybinding active only on this screen" ), local_key ) + "\n";
    legend += colorize( _( "Keybinding active globally" ), global_key ) + "\n";
    legend += colorize( _( "* User created" ), global_key ) + "\n";
    if( permit_execute_action ) {
        legend += string_format(
                      _( "Press %c to execute action\n" ),
                      fallback_keys.at( fallback_action::execute ) );
    }

    std::vector<std::string> filtered_registered_actions = org_registered_actions;
    std::string filter_phrase;
    std::string action;
    int raw_input_char = 0;
    int highlight_row_index = -1;
    const auto redraw = [&]( ui_adaptor & ui ) {
        werase( w_help );
        draw_border( w_help, BORDER_COLOR, _( "Keybindings" ), c_light_red );
        draw_scrollbar( w_help, scroll_offset, display_height,
                        filtered_registered_actions.size(), point( 0, 7 ), c_white, true );
        const int legend_lines = 1 + fold_and_print( w_help, point( 2, 1 ), legwidth, c_white, legend );
        const auto item_color = []( const int index_to_draw, int index_highlighted ) {
            return index_highlighted == index_to_draw ? h_light_gray : c_light_gray;
        };
        right_print( w_help, legend_lines, 2,
                     item_color( static_cast<int>( kb_btn_idx::remove ), int( highlighted_btn_index ) ),
                     string_format( _( "<[<color_yellow>%c</color>] Remove keybinding>" ),
                                    fallback_keys.at( fallback_action::remove ) ) );
        right_print( w_help, legend_lines, 26,
                     item_color( static_cast<int>( kb_btn_idx::add_local ), int( highlighted_btn_index ) ),
                     string_format( _( "<[<color_yellow>%c</color>] Add local keybinding>" ),
                                    fallback_keys.at( fallback_action::add_local ) ) );
        right_print( w_help, legend_lines, 54,
                     item_color( static_cast<int>( kb_btn_idx::add_global ), int( highlighted_btn_index ) ),
                     string_format( _( "<[<color_yellow>%c</color>] Add global keybinding>" ),
                                    fallback_keys.at( fallback_action::add_global ) ) );

        for( size_t i = 0; i + scroll_offset < filtered_registered_actions.size() &&
             i < display_height; i++ ) {
            const std::string &action_id = filtered_registered_actions[i + scroll_offset];

            bool overwrite_default;
            const action_attributes &attributes = inp_mngr.get_action_attributes( action_id, category,
                                                  &overwrite_default );
            bool basic_overwrite_default;
            const action_attributes &basic_attributes = inp_mngr.get_action_attributes( action_id, category,
                    &basic_overwrite_default, true );

            char invlet;
            if( i < hotkeys.size() ) {
                invlet = hotkeys[i];
            } else {
                invlet = ' ';
            }

            if( status == s_add_global && overwrite_default ) {
                // We're trying to add a global, but this action has a local
                // defined, so gray out the invlet.
                mvwprintz( w_help, point( 2, i + 7 ), c_dark_gray, "%c ", invlet );
            } else if( status == s_add || status == s_add_global || status == s_remove ) {
                mvwprintz( w_help, point( 2, i + 7 ), c_light_blue, "%c ", invlet );
            } else if( status == s_execute ) {
                mvwprintz( w_help, point( 2, i + 7 ), c_white, "%c ", invlet );
            } else {
                mvwprintz( w_help, point( 2, i + 7 ), c_blue, "  " );
            }
            nc_color col;
            if( attributes.input_events.empty() ) {
                col = i == size_t( highlight_row_index ) ? h_unbound_key : unbound_key;
            } else if( overwrite_default ) {
                col = i == size_t( highlight_row_index ) ? h_local_key : local_key;
            } else {
                col = i == size_t( highlight_row_index ) ? h_global_key : global_key;
            }
            if( overwrite_default != basic_overwrite_default
                || attributes.input_events != basic_attributes.input_events
              ) {
                mvwprintz( w_help, point( 3, i + 7 ), col, "*" );
            }
            mvwprintz( w_help, point( 4, i + 7 ), col, "%s:", get_action_name( action_id ) );
            mvwprintz( w_help, point( TERMX >= 100 ? 62 : 52, i + 7 ), col, "%s", get_desc( action_id ) );
        }

        // spopup.query_string() will call wnoutrefresh( w_help )
        spopup.text( filter_phrase );
        spopup.query_string( false, true );
        // Record cursor immediately after spopup drawing
        ui.record_term_cursor();
    };
    ui.on_redraw( redraw );

    while( true ) {
        ui_manager::redraw();

        if( status == s_show ) {
            filter_phrase = spopup.query_string( false );
            action = ctxt.input_to_action( ctxt.get_raw_input() );
        } else {
            action = ctxt.handle_input();
        }
        raw_input_char = ctxt.get_raw_input().get_first_input();
        for( const std::pair<const fallback_action, int> &v : fallback_keys ) {
            if( v.second == raw_input_char ) {
                action.clear();
            }
        }

        filtered_registered_actions = filter_strings_by_phrase( org_registered_actions, filter_phrase );
        if( scroll_offset > filtered_registered_actions.size() ) {
            scroll_offset = 0;
        }
        if( action == "MOUSE_MOVE" || action == "SELECT" ) {
            highlighted_btn_index = kb_btn_idx::none;
            highlight_row_index = -1;
            std::optional<point> o_p = ctxt.get_coordinates_text( w_help );
            if( o_p ) {
                point p = o_p.value();
                if( window_contains_point_relative( w_help, p ) ) {
                    if( p.y >= 7 && p.y < TERMY  && status != s_show ) {
                        highlight_row_index = p.y - 7;
                    } else if( p.y == 4 ) {
                        if( p.x >= 17 && p.x <= 43 ) {
                            highlighted_btn_index = kb_btn_idx::add_global;
                        } else if( p.x >= 46 && p.x < 72 ) {
                            highlighted_btn_index = kb_btn_idx::add_local;
                        } else if( p.x >= 73 && p.x < 96 ) {
                            highlighted_btn_index = kb_btn_idx::remove;
                        }
                    }
                }
            }
            if( action == "SELECT" ) {
                switch( highlighted_btn_index ) {
                    case kb_btn_idx::remove:
                        status = s_remove;
                        break;
                    case kb_btn_idx::add_local:
                        status = s_add;
                        break;
                    case kb_btn_idx::add_global:
                        status = s_add_global;
                        break;
                    case kb_btn_idx::none:
                        break;
                }
            }
        }
        // In addition to the modifiable hotkeys, we also check for hardcoded
        // keys, e.g. '+', '-', '=', '.' in order to prevent the user from
        // entering an unrecoverable state.
        if( action == "ADD_LOCAL"
            || raw_input_char == fallback_keys.at( fallback_action::add_local ) ) {
            if( !filtered_registered_actions.empty() ) {
                status = s_add;
            }
        } else if( action == "ADD_GLOBAL"
                   || raw_input_char == fallback_keys.at( fallback_action::add_global ) ) {
            if( !filtered_registered_actions.empty() ) {
                status = s_add_global;
            }
        } else if( action == "REMOVE"
                   || raw_input_char == fallback_keys.at( fallback_action::remove ) ) {
            if( !filtered_registered_actions.empty() ) {
                status = s_remove;
            }
        } else if( ( action == "EXECUTE"
                     || raw_input_char == fallback_keys.at( fallback_action::execute ) )
                   && permit_execute_action ) {
            if( !filtered_registered_actions.empty() ) {
                status = s_execute;
            }
        } else if( action == "DOWN" ) {
            if( !filtered_registered_actions.empty()
                && filtered_registered_actions.size() > display_height
                && scroll_offset < filtered_registered_actions.size() - display_height ) {
                scroll_offset++;
            }
        } else if( action == "UP" ) {
            if( !filtered_registered_actions.empty()
                && scroll_offset > 0 ) {
                scroll_offset--;
            }
        } else if( action == "PAGE_DOWN" || action == "SCROLL_DOWN" ) {
            if( filtered_registered_actions.empty() ) {
                // do nothing
            } else if( scroll_offset + display_height < filtered_registered_actions.size() ) {
                scroll_offset += std::min( display_height, filtered_registered_actions.size() -
                                           display_height - scroll_offset );
            } else if( filtered_registered_actions.size() > display_height ) {
                scroll_offset = 0;
            }
        } else if( action == "PAGE_UP" || action == "SCROLL_UP" ) {
            if( filtered_registered_actions.empty() ) {
                // do nothing
            } else if( scroll_offset >= display_height ) {
                scroll_offset -= display_height;
            } else if( scroll_offset > 0 ) {
                scroll_offset = 0;
            } else if( filtered_registered_actions.size() > display_height ) {
                scroll_offset = filtered_registered_actions.size() - display_height;
            }
        } else if( action == "QUIT" ) {
            if( status != s_show ) {
                status = s_show;
            } else {
                break;
            }
        } else if( action == "HELP_KEYBINDINGS" ) {
            // update available hotkeys in case they've changed
            hotkeys = ctxt.get_available_single_char_hotkeys( display_help_hotkeys );
        } else if( !filtered_registered_actions.empty() && status != s_show ) {
            size_t hotkey_index = hotkeys.find_first_of( raw_input_char );
            if( hotkey_index == std::string::npos ) {
                if( action == "SELECT" && highlight_row_index != -1 ) {
                    hotkey_index = size_t( highlight_row_index );
                } else {
                    continue;
                }
            }
            const size_t action_index = hotkey_index + scroll_offset;
            if( action_index >= filtered_registered_actions.size() ) {
                continue;
            }
            const std::string &action_id = filtered_registered_actions[action_index];

            // Check if this entry is local or global.
            bool is_local = false;
            const action_attributes &actions = inp_mngr.get_action_attributes( action_id, category, &is_local );
            bool is_empty = actions.input_events.empty();
            const std::string name = get_action_name( action_id );

            // We don't want to completely delete a global context entry.
            // Only attempt removal for a local context, or when there's
            // bindings for the default context.
            if( status == s_remove && ( is_local || !is_empty ) ) {
                if( !get_option<bool>( "QUERY_KEYBIND_REMOVAL" ) || query_yn( is_local &&
                        is_empty ? _( "Reset to global bindings for %s?" ) : _( "Clear keys for %s?" ), name ) ) {

                    // If it's global, reset the global actions.
                    std::string category_to_access = category;
                    if( !is_local ) {
                        category_to_access = default_context_id;
                    }

                    inp_mngr.remove_input_for_action( action_id, category_to_access );
                    changed = true;
                }
            } else if( status == s_add_global && is_local ) {
                // Disallow adding global actions to an action that already has a local defined.
                popup( _( "There are already local keybindings defined for this action, please remove them first." ) );
            } else if( status == s_add || status == s_add_global ) {
                const input_event new_event = query_popup()
                                              .preferred_keyboard_mode( preferred_keyboard_mode )
                                              .message( _( "New key for %s" ), name )
                                              .allow_anykey( true )
                                              .query()
                                              .evt;

                if( action_uses_input( action_id, new_event )
                    // Allow adding keys already used globally to local bindings
                    && ( status == s_add_global || is_local ) ) {
                    popup_getkey( _( "This key is already used for %s." ), name );
                    status = s_show;
                    continue;
                }

                const std::string conflicts = get_conflicts( new_event, action_id );
                const bool has_conflicts = !conflicts.empty();
                bool resolve_conflicts = false;

                if( has_conflicts ) {
                    resolve_conflicts = query_yn(
                                            _( "This key conflicts with %s. Remove this key from the conflicting command(s), and continue?" ),
                                            conflicts.c_str() );
                }

                if( !has_conflicts || resolve_conflicts ) {
                    if( resolve_conflicts ) {
                        clear_conflicting_keybindings( new_event );
                    }

                    // We might be adding a local or global action.
                    std::string category_to_access = category;
                    if( status == s_add_global ) {
                        category_to_access = default_context_id;
                    }

                    inp_mngr.add_input_for_action( action_id, category_to_access, new_event );
                    changed = true;
                }
            } else if( status == s_execute && permit_execute_action ) {
                action_to_execute = look_up_action( action_id );
                break;
            }
            status = s_show;
        }
    }

    if( changed && query_yn( _( "Save changes?" ) ) ) {
        try {
            inp_mngr.save();
            get_help().load();
        } catch( std::exception &err ) {
            popup( _( "saving keybindings failed: %s" ), err.what() );
        }
    } else if( changed ) {
        inp_mngr.action_contexts.swap( old_action_contexts );
    }

    return action_to_execute;
}

input_event input_context::get_raw_input()
{
    return next_action;
}

#if !(defined(TILES) || defined(_WIN32))
// Also specify that we don't have a gamepad plugged in.
bool gamepad_available()
{
    return false;
}

std::optional<tripoint> input_context::get_coordinates( const catacurses::window &capture_win,
        const point &offset, const bool center_cursor ) const
{
    if( !coordinate_input_received ) {
        return std::nullopt;
    }
    const point view_size( getmaxx( capture_win ), getmaxy( capture_win ) );
    const point win_min( getbegx( capture_win ),
                         getbegy( capture_win ) );
    const half_open_rectangle<point> win_bounds( win_min, win_min + view_size );
    if( !win_bounds.contains( coordinate ) ) {
        return std::nullopt;
    }

    point p = coordinate + offset;
    // If no offset is specified, account for the window location
    if( offset == point_zero ) {
        p -= win_min;
    }
    // Some windows (notably the overmap) want 0,0 to be the center of the screen
    if( center_cursor ) {
        p -= view_size / 2;
    }
    return tripoint( p, get_map().get_abs_sub().z() );
}
#endif

std::optional<point> input_context::get_coordinates_text( const catacurses::window
        &capture_win ) const
{
#if !defined( TILES )
    std::optional<tripoint> coord3d = get_coordinates( capture_win );
    if( coord3d.has_value() ) {
        return get_coordinates( capture_win )->xy();
    } else {
        return std::nullopt;
    }
#else
    if( !coordinate_input_received ) {
        return std::nullopt;
    }
    const window_dimensions dim = get_window_dimensions( capture_win );
    const int &fw = dim.scaled_font_size.x;
    const int &fh = dim.scaled_font_size.y;
    const point &win_min = dim.window_pos_pixel;
    const point screen_pos = coordinate - win_min;
    const point selected( divide_round_down( screen_pos.x, fw ),
                          divide_round_down( screen_pos.y, fh ) );
    return selected;
#endif
}

std::string input_context::get_action_name( const std::string &action_id ) const
{
    // 1) Check action name overrides specific to this input_context
    const auto action_name_override =
        action_name_overrides.find( action_id );
    if( action_name_override != action_name_overrides.end() ) {
        return action_name_override->second.translated();
    }

    // 2) Check if the hotkey has a name
    const action_attributes &attributes = inp_mngr.get_action_attributes( action_id, category );
    if( !attributes.name.empty() ) {
        return attributes.name.translated();
    }

    // 3) If the hotkey has no name, the user has created a local hotkey in
    // this context that is masking the global hotkey. Fallback to the global
    // hotkey's name.
    const action_attributes &default_attributes = inp_mngr.get_action_attributes( action_id,
            default_context_id );
    if( !default_attributes.name.empty() ) {
        return default_attributes.name.translated();
    }

    // 4) Unable to find suitable name. Keybindings configuration likely borked
    return action_id;
}

// (Press X (or Y)|Try) to Z
std::string input_context::press_x( const std::string &action_id ) const
{
    return press_x( action_id, _( "Press " ), "", _( "Try" ) );
}

std::string input_context::press_x( const std::string &action_id, const std::string &key_bound,
                                    const std::string &key_unbound ) const
{
    return press_x( action_id, key_bound, "", key_unbound );
}

// TODO: merge this with input_context::get_desc
std::string input_context::press_x( const std::string &action_id,
                                    const std::string &key_bound_pre,
                                    const std::string &key_bound_suf, const std::string &key_unbound ) const
{
    if( action_id == "ANY_INPUT" ) {
        return _( "any key" );
    }
    if( action_id == "COORDINATE" ) {
        return _( "mouse movement" );
    }
    input_manager::t_input_event_list events = inp_mngr.get_input_for_action( action_id, category );
    events.erase( std::remove_if( events.begin(), events.end(), [this]( const input_event & evt ) {
        return !is_event_type_enabled( evt.type );
    } ), events.end() );
    if( events.empty() ) {
        return key_unbound;
    }
    const std::string separator = _( " or " );
    std::string keyed = key_bound_pre;
    for( size_t j = 0; j < events.size(); j++ ) {
        keyed += events[j].long_description();

        if( j + 1 < events.size() ) {
            keyed += separator;
        }
    }
    keyed += key_bound_suf;
    return keyed;
}

void input_context::set_iso( bool mode )
{
    iso_mode = mode;
}

std::vector<std::string> input_context::filter_strings_by_phrase(
    const std::vector<std::string> &strings, const std::string_view phrase ) const
{
    std::vector<std::string> filtered_strings;

    for( const std::string &str : strings ) {
        if( lcmatch( remove_color_tags( get_action_name( str ) ), phrase ) ) {
            filtered_strings.push_back( str );
        }
    }

    return filtered_strings;
}

void input_context::set_edittext( const std::string &s )
{
    edittext = s;
}

std::string input_context::get_edittext()
{
    return edittext;
}

void input_context::set_timeout( int val )
{
    timeout = val;
}

void input_context::reset_timeout()
{
    timeout = -1;
}

bool input_context::is_event_type_enabled( const input_event_t type ) const
{
    switch( type ) {
        case input_event_t::error:
            return false;
        case input_event_t::timeout:
            return true;
        case input_event_t::keyboard_char:
            return input_manager::actual_keyboard_mode( preferred_keyboard_mode ) == keyboard_mode::keychar;
        case input_event_t::keyboard_code:
            return input_manager::actual_keyboard_mode( preferred_keyboard_mode ) == keyboard_mode::keycode;
        case input_event_t::gamepad:
            return gamepad_available();
        case input_event_t::mouse:
            return true;
    }
    return true;
}

bool input_context::is_registered_action( const std::string &action_name ) const
{
    return std::find( registered_actions.begin(), registered_actions.end(),
                      action_name ) != registered_actions.end();
}

input_event input_context::first_unassigned_hotkey( const hotkey_queue &queue ) const
{
    input_event ret = queue.first( *this );
    while( ret.type != input_event_t::error
           && &input_to_action( ret ) != &CATA_ERROR ) {
        ret = queue.next( ret );
    }
    return ret;
}

input_event input_context::next_unassigned_hotkey( const hotkey_queue &queue,
        const input_event &prev ) const
{
    input_event ret = prev;
    do {
        ret = queue.next( ret );
    } while( ret.type != input_event_t::error
             && &input_to_action( ret ) != &CATA_ERROR );
    return ret;
}

input_event hotkey_queue::first( const input_context &ctxt ) const
{
    if( ctxt.is_event_type_enabled( input_event_t::keyboard_code ) ) {
        if( !codes_keycode.empty() && !modifiers_keycode.empty() ) {
            return input_event( modifiers_keycode[0], codes_keycode[0], input_event_t::keyboard_code );
        } else {
            return input_event();
        }
    } else {
        if( !codes_keychar.empty() ) {
            return input_event( codes_keychar[0], input_event_t::keyboard_char );
        } else {
            return input_event();
        }
    }
}

input_event hotkey_queue::next( const input_event &prev ) const
{
    switch( prev.type ) {
        default:
            return input_event();
        case input_event_t::keyboard_code: {
            if( prev.sequence.size() != 1 ) {
                return input_event();
            }
            const auto code_it = std::find( codes_keycode.begin(), codes_keycode.end(),
                                            prev.get_first_input() );
            const auto mod_it = std::find( modifiers_keycode.begin(), modifiers_keycode.end(), prev.modifiers );
            if( code_it == codes_keycode.end() || mod_it == modifiers_keycode.end() ) {
                return input_event();
            }
            if( std::next( code_it ) != codes_keycode.end() ) {
                return input_event( prev.modifiers, *std::next( code_it ), prev.type );
            } else if( std::next( mod_it ) != modifiers_keycode.end() ) {
                return input_event( *std::next( mod_it ), codes_keycode[0], prev.type );
            } else {
                return input_event();
            }
            break;
        }
        case input_event_t::keyboard_char: {
            if( prev.sequence.size() != 1 ) {
                return input_event();
            }
            const auto code_it = std::find( codes_keychar.begin(), codes_keychar.end(),
                                            prev.get_first_input() );
            if( code_it == codes_keychar.end() ) {
                return input_event();
            }
            if( std::next( code_it ) != codes_keychar.end() ) {
                return input_event( *std::next( code_it ), prev.type );
            } else {
                return input_event();
            }
            break;
        }
    }
}

const hotkey_queue &hotkey_queue::alphabets()
{
    // NOTE: if you want to add any parameters to this function, remove the static
    // modifier of `queue` and return by value instead of by reference.
    // NOTE: Removal of conflicting keys is handled by `input_context::next_unassigned_key`.
    static std::unique_ptr<hotkey_queue> queue;
    if( !queue ) {
        queue = std::make_unique<hotkey_queue>();
        for( int ch = 'a'; ch <= 'z'; ++ch ) {
            queue->codes_keycode.emplace_back( ch );
            queue->codes_keychar.emplace_back( ch );
        }
        for( int ch = 'A'; ch <= 'Z'; ++ch ) {
            queue->codes_keychar.emplace_back( ch );
        }
        queue->modifiers_keycode.emplace_back();
        queue->modifiers_keycode.emplace_back( std::set<keymod_t>( { keymod_t::shift } ) );
    }
    return *queue;
}

const hotkey_queue &hotkey_queue::alpha_digits()
{
    // NOTE: if you want to add any parameters to this function, remove the static
    // modifier of `queue` and return by value instead of by reference.
    // NOTE: Removal of conflicting keys is handled by `input_context::next_unassigned_key`.
    static std::unique_ptr<hotkey_queue> queue;
    if( !queue ) {
        queue = std::make_unique<hotkey_queue>();
        for( int ch = '1'; ch <= '9'; ++ch ) {
            queue->codes_keycode.emplace_back( ch );
            queue->codes_keychar.emplace_back( ch );
        }
        queue->codes_keycode.emplace_back( '0' );
        queue->codes_keychar.emplace_back( '0' );
        for( int ch = 'a'; ch <= 'z'; ++ch ) {
            queue->codes_keycode.emplace_back( ch );
            queue->codes_keychar.emplace_back( ch );
        }
        for( int ch = 'A'; ch <= 'Z'; ++ch ) {
            queue->codes_keychar.emplace_back( ch );
        }
        queue->modifiers_keycode.emplace_back();
        queue->modifiers_keycode.emplace_back( std::set<keymod_t>( { keymod_t::shift } ) );
    }
    return *queue;
}
