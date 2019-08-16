#include "input.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <array>
#include <exception>
#include <locale>
#include <memory>
#include <set>
#include <utility>

#include "action.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "cursesdef.h"
#include "debug.h"
#include "filesystem.h"
#include "game.h"
#include "help.h"
#include "ime.h"
#include "json.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "popup.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "color.h"
#include "point.h"

using std::min; // from <algorithm>
using std::max;

static const std::string default_context_id( "default" );

template <class T1, class T2>
struct ContainsPredicate {
    const T1 &container;

    ContainsPredicate( const T1 &container ) : container( container ) { }

    // Operator overload required to leverage std functional interface.
    bool operator()( T2 c ) {
        return std::find( container.begin(), container.end(), c ) != container.end();
    }
};

static int str_to_int( const std::string &number )
{
    // ensure user's locale doesn't interfere with number format
    std::istringstream buffer( number );
    buffer.imbue( std::locale::classic() );
    int result;
    buffer >> result;
    return result;
}

static std::string int_to_str( int number )
{
    // ensure user's locale doesn't interfere with number format
    std::ostringstream buffer;
    buffer.imbue( std::locale::classic() );
    buffer << number;
    return buffer.str();
}

bool is_mouse_enabled()
{
#if defined(_WIN32) && !defined(TILES)
    return false;
#else
    return true;
#endif
}

//helper function for those have problem inputting certain characters.
std::string get_input_string_from_file( const std::string &fname )
{
    std::string ret;
    read_from_file_optional( fname, [&ret]( std::istream & fin ) {
        getline( fin, ret );
        //remove utf8 bmm
        if( !ret.empty() && static_cast<unsigned char>( ret[0] ) == 0xef ) {
            ret.erase( 0, 3 );
        }
        while( !ret.empty() && ( ret[ret.size() - 1] == '\r' ||  ret[ret.size() - 1] == '\n' ) ) {
            ret.erase( ret.size() - 1, 1 );
        }
    } );
    return ret;
}

int input_event::get_first_input() const
{
    if( sequence.empty() ) {
        return UNKNOWN_UNICODE;
    }

    return sequence[0];
}

input_manager inp_mngr;

void input_manager::init()
{
    std::map<char, action_id> keymap;
    std::string keymap_file_loaded_from;
    std::set<action_id> unbound_keymap;
    load_keyboard_settings( keymap, keymap_file_loaded_from, unbound_keymap );
    init_keycode_mapping();
    reset_timeout();

    try {
        load( FILENAMES["keybindings"], false );
    } catch( const JsonError &err ) {
        throw std::runtime_error( FILENAMES["keybindings"] + ": " + err.what() );
    }
    try {
        load( FILENAMES["keybindings_vehicle"], false );
    } catch( const JsonError &err ) {
        throw std::runtime_error( FILENAMES["keybindings_vehicle"] + ": " + err.what() );
    }
    try {
        load( FILENAMES["user_keybindings"], true );
    } catch( const JsonError &err ) {
        throw std::runtime_error( FILENAMES["user_keybindings"] + ": " + err.what() );
    }

    if( keymap_file_loaded_from.empty() || ( keymap.empty() && unbound_keymap.empty() ) ) {
        // No keymap file was loaded, or the file has no mappings and no unmappings,
        // we can skip the remaining part of the function, especially the save function
        return;
    }
    t_actions &actions = action_contexts["DEFAULTMODE"];
    std::set<action_id> touched;
    for( std::map<char, action_id>::const_iterator a = keymap.begin(); a != keymap.end(); ++a ) {
        const std::string action_id = action_ident( a->second );
        // Put the binding from keymap either into the global context
        // (if an action with that ident already exists there - think movement keys)
        // or otherwise to the DEFAULTMODE context.
        std::string context = "DEFAULTMODE";
        if( action_contexts[default_context_id].count( action_id ) > 0 ) {
            context = default_context_id;
        } else if( touched.count( a->second ) == 0 ) {
            // Note: movement keys are somehow special as the default in keymap
            // does not contain the arrow keys, so we don't clear existing keybindings
            // for them.
            // If the keymap contains a binding for this action, erase all the
            // previously (default!) existing bindings, to only keep the bindings,
            // the user is used to
            action_contexts[action_id].clear();
            touched.insert( a->second );
        }
        add_input_for_action( action_id, context, input_event( a->first, CATA_INPUT_KEYBOARD ) );
    }
    // Unmap actions that are explicitly not mapped
    for( const auto &elem : unbound_keymap ) {
        const std::string action_id = action_ident( elem );
        actions[action_id].input_events.clear();
    }
    // Imported old bindings from old keymap file, save those to the new
    // keybindings.json file.
    try {
        save();
    } catch( std::exception &err ) {
        debugmsg( "Could not write imported keybindings: %s", err.what() );
        return;
    }
    // Finally if we did import a file, and saved it to the new keybindings
    // file, delete the old keymap file to prevent re-importing it.
    if( !keymap_file_loaded_from.empty() ) {
        remove_file( keymap_file_loaded_from );
    }
}

void input_manager::load( const std::string &file_name, bool is_user_preferences )
{
    std::ifstream data_file( file_name.c_str(), std::ifstream::in | std::ifstream::binary );

    if( !data_file.good() ) {
        // Only throw if this is the first file to load, that file _must_ exist,
        // otherwise the keybindings can not be read at all.
        if( action_contexts.empty() ) {
            throw std::runtime_error( std::string( "Could not read " ) + file_name );
        }
        return;
    }

    JsonIn jsin( data_file );

    //Crawl through once and create an entry for every definition
    jsin.start_array();
    while( !jsin.end_array() ) {
        // JSON object representing the action
        JsonObject action = jsin.get_object();

        const std::string action_id = action.get_string( "id" );
        const std::string context = action.get_string( "category", default_context_id );
        t_actions &actions = action_contexts[context];
        if( !is_user_preferences && action.has_member( "name" ) ) {
            // Action names are not user preferences. Some experimental builds
            // post-0.A had written action names into the user preferences
            // config file. Any names that exist in user preferences will be
            // ignored.
            actions[action_id].name = action.get_string( "name" );
        }

        // Iterate over the bindings JSON array
        JsonArray bindings = action.get_array( "bindings" );
        t_input_event_list events;
        while( bindings.has_more() ) {
            JsonObject keybinding = bindings.next_object();
            std::string input_method = keybinding.get_string( "input_method" );
            input_event new_event;
            if( input_method == "keyboard" ) {
                new_event.type = CATA_INPUT_KEYBOARD;
            } else if( input_method == "gamepad" ) {
                new_event.type = CATA_INPUT_GAMEPAD;
            } else if( input_method == "mouse" ) {
                new_event.type = CATA_INPUT_MOUSE;
            }

            if( keybinding.has_array( "key" ) ) {
                JsonArray keys = keybinding.get_array( "key" );
                while( keys.has_more() ) {
                    new_event.sequence.push_back(
                        get_keycode( keys.next_string() )
                    );
                }
            } else { // assume string if not array, and throw if not string
                new_event.sequence.push_back(
                    get_keycode( keybinding.get_string( "key" ) )
                );
            }

            events.push_back( new_event );
        }

        // An invariant of this class is that user-created, local keybindings
        // with an empty set of input_events do not exist in the
        // action_contexts map. In prior versions of this class, this was not
        // true, so users of experimental builds post-0.A will have empty
        // local keybindings saved in their keybindings.json config.
        //
        // To be backwards compatible with keybindings.json from prior
        // experimental builds, we will detect user-created, local keybindings
        // with empty input_events and disregard them. When keybindings are
        // later saved, these remnants won't be saved.
        if( !is_user_preferences ||
            !events.empty() ||
            context == default_context_id ||
            actions.count( action_id ) > 0 ) {
            // In case this is the second file containing user preferences,
            // this replaces the default bindings with the user's preferences.
            action_attributes &attributes = actions[action_id];
            attributes.input_events = events;
            if( action.has_member( "is_user_created" ) ) {
                attributes.is_user_created = action.get_bool( "is_user_created" );
            }
        }
    }
}

void input_manager::save()
{
    write_to_file( FILENAMES["user_keybindings"], [&]( std::ostream & data_file ) {
        JsonOut jsout( data_file, true );

        jsout.start_array();
        for( t_action_contexts::const_iterator a = action_contexts.begin(); a != action_contexts.end();
             ++a ) {
            const t_actions &actions = a->second;
            for( const auto &action : actions ) {
                const t_input_event_list &events = action.second.input_events;
                jsout.start_object();

                jsout.member( "id", action.first );
                jsout.member( "category", a->first );
                bool is_user_created = action.second.is_user_created;
                if( is_user_created ) {
                    jsout.member( "is_user_created", is_user_created );
                }

                jsout.member( "bindings" );
                jsout.start_array();
                for( const auto &event : events ) {
                    jsout.start_object();
                    switch( event.type ) {
                        case CATA_INPUT_KEYBOARD:
                            jsout.member( "input_method", "keyboard" );
                            break;
                        case CATA_INPUT_GAMEPAD:
                            jsout.member( "input_method", "gamepad" );
                            break;
                        case CATA_INPUT_MOUSE:
                            jsout.member( "input_method", "mouse" );
                            break;
                        default:
                            throw std::runtime_error( "unknown input_event_t" );
                    }
                    jsout.member( "key" );
                    jsout.start_array();
                    for( size_t i = 0; i < event.sequence.size(); i++ ) {
                        jsout.write( get_keyname( event.sequence[i], event.type, true ) );
                    }
                    jsout.end_array();
                    jsout.end_object();
                }
                jsout.end_array();

                jsout.end_object();
            }
        }
        jsout.end_array();
    }, _( "key bindings configuration" ) );
}

void input_manager::add_keycode_pair( int ch, const std::string &name )
{
    keycode_to_keyname[ch] = name;
    keyname_to_keycode[name] = ch;
}

void input_manager::add_gamepad_keycode_pair( int ch, const std::string &name )
{
    gamepad_keycode_to_keyname[ch] = name;
    keyname_to_keycode[name] = ch;
}

void input_manager::init_keycode_mapping()
{
    // Between space and tilde, all keys more or less map
    // to themselves(see ASCII table)
    for( char c = ' '; c <= '~'; c++ ) {
        std::string name( 1, c );
        add_keycode_pair( c, name );
    }

    add_keycode_pair( '\t',          "TAB" );
    add_keycode_pair( KEY_BTAB,      "BACKTAB" );
    add_keycode_pair( ' ',           "SPACE" );
    add_keycode_pair( KEY_UP,        "UP" );
    add_keycode_pair( KEY_DOWN,      "DOWN" );
    add_keycode_pair( KEY_LEFT,      "LEFT" );
    add_keycode_pair( KEY_RIGHT,     "RIGHT" );
    add_keycode_pair( KEY_NPAGE,     "NPAGE" );
    add_keycode_pair( KEY_PPAGE,     "PPAGE" );
    add_keycode_pair( KEY_ESCAPE,    "ESC" );
    add_keycode_pair( KEY_BACKSPACE, "BACKSPACE" );
    add_keycode_pair( KEY_HOME,      "HOME" );
    add_keycode_pair( KEY_BREAK,     "BREAK" );
    add_keycode_pair( KEY_END,       "END" );
    add_keycode_pair( '\n',          "RETURN" );

    // function keys, as defined by ncurses
    for( int i = 0; i <= 63; i++ ) {
        add_keycode_pair( KEY_F( i ), string_format( "F%d", i ) );
    }

    add_gamepad_keycode_pair( JOY_LEFT,      "JOY_LEFT" );
    add_gamepad_keycode_pair( JOY_RIGHT,     "JOY_RIGHT" );
    add_gamepad_keycode_pair( JOY_UP,        "JOY_UP" );
    add_gamepad_keycode_pair( JOY_DOWN,      "JOY_DOWN" );
    add_gamepad_keycode_pair( JOY_LEFTUP,    "JOY_LEFTUP" );
    add_gamepad_keycode_pair( JOY_LEFTDOWN,  "JOY_LEFTDOWN" );
    add_gamepad_keycode_pair( JOY_RIGHTUP,   "JOY_RIGHTUP" );
    add_gamepad_keycode_pair( JOY_RIGHTDOWN, "JOY_RIGHTDOWN" );

    add_gamepad_keycode_pair( JOY_0,         "JOY_0" );
    add_gamepad_keycode_pair( JOY_1,         "JOY_1" );
    add_gamepad_keycode_pair( JOY_2,         "JOY_2" );
    add_gamepad_keycode_pair( JOY_3,         "JOY_3" );
    add_gamepad_keycode_pair( JOY_4,         "JOY_4" );
    add_gamepad_keycode_pair( JOY_5,         "JOY_5" );
    add_gamepad_keycode_pair( JOY_6,         "JOY_6" );
    add_gamepad_keycode_pair( JOY_7,         "JOY_7" );

    keyname_to_keycode["MOUSE_LEFT"] = MOUSE_BUTTON_LEFT;
    keyname_to_keycode["MOUSE_RIGHT"] = MOUSE_BUTTON_RIGHT;
    keyname_to_keycode["SCROLL_UP"] = SCROLLWHEEL_UP;
    keyname_to_keycode["SCROLL_DOWN"] = SCROLLWHEEL_DOWN;
    keyname_to_keycode["MOUSE_MOVE"] = MOUSE_MOVE;
}

int input_manager::get_keycode( const std::string &name ) const
{
    const t_name_to_key_map::const_iterator a = keyname_to_keycode.find( name );
    if( a != keyname_to_keycode.end() ) {
        return a->second;
    }
    // Not found in map, try to parse as int
    if( name.compare( 0, 8, "UNKNOWN_" ) == 0 ) {
        return str_to_int( name.substr( 8 ) );
    }
    return 0;
}

std::string input_manager::get_keyname( int ch, input_event_t inp_type, bool portable ) const
{
    if( inp_type == CATA_INPUT_KEYBOARD ) {
        const t_key_to_name_map::const_iterator a = keycode_to_keyname.find( ch );
        if( a != keycode_to_keyname.end() ) {
            return a->second;
        }
    } else if( inp_type == CATA_INPUT_MOUSE ) {
        if( ch == MOUSE_BUTTON_LEFT ) {
            return "MOUSE_LEFT";
        } else if( ch == MOUSE_BUTTON_RIGHT ) {
            return "MOUSE_RIGHT";
        } else if( ch == SCROLLWHEEL_UP ) {
            return "SCROLL_UP";
        } else if( ch == SCROLLWHEEL_DOWN ) {
            return "SCROLL_DOWN";
        } else if( ch == MOUSE_MOVE ) {
            return "MOUSE_MOVE";
        }
    } else if( inp_type == CATA_INPUT_GAMEPAD ) {
        const t_key_to_name_map::const_iterator a = gamepad_keycode_to_keyname.find( ch );
        if( a != gamepad_keycode_to_keyname.end() ) {
            return a->second;
        }
    } else {
        return "UNKNOWN";
    }
    if( portable ) {
        return std::string( "UNKNOWN_" ) + int_to_str( ch );
    }
    return string_format( _( "unknown key %ld" ), ch );
}

const std::vector<input_event> &input_manager::get_input_for_action( const std::string
        &action_descriptor, const std::string &context, bool *overwrites_default )
{
    const action_attributes &attributes = get_action_attributes( action_descriptor, context,
                                          overwrites_default );
    return attributes.input_events;
}

int input_manager::get_first_char_for_action( const std::string &action_descriptor,
        const std::string &context )
{
    std::vector<input_event> input_events = get_input_for_action( action_descriptor, context );
    return input_events.empty() ? 0 : input_events[0].get_first_input();
}

const action_attributes &input_manager::get_action_attributes(
    const std::string &action_id,
    const std::string &context,
    bool *overwrites_default )
{

    if( context != default_context_id ) {
        // Check if the action exists in the provided context
        const t_action_contexts::const_iterator action_context = action_contexts.find( context );
        if( action_context != action_contexts.end() ) {
            const t_actions::const_iterator action = action_context->second.find( action_id );
            if( action != action_context->second.end() ) {
                if( overwrites_default ) {
                    *overwrites_default = true;
                }

                return action->second;
            }
        }
    }

    // If not, we use the default binding.
    if( overwrites_default ) {
        *overwrites_default = false;
    }

    t_actions &default_action_context = action_contexts[default_context_id];
    const t_actions::const_iterator default_action = default_action_context.find( action_id );
    if( default_action == default_action_context.end() ) {
        // A new action is created in the event that the requested action is
        // not in the keybindings configuration e.g. the entry is missing.
        default_action_context[action_id].name = get_default_action_name( action_id );
    }

    return default_action_context[action_id];
}

std::string input_manager::get_default_action_name( const std::string &action_id ) const
{
    const t_action_contexts::const_iterator default_action_context = action_contexts.find(
                default_context_id );
    if( default_action_context == action_contexts.end() ) {
        return action_id;
    }

    const t_actions::const_iterator default_action = default_action_context->second.find( action_id );
    if( default_action != default_action_context->second.end() ) {
        return default_action->second.name;
    } else {
        return action_id;
    }
}

input_manager::t_input_event_list &input_manager::get_or_create_event_list(
    const std::string &action_descriptor, const std::string &context )
{
    // A new context is created in the event that the user creates a local
    // keymapping in a context that doesn't yet exist e.g. a context without
    // any pre-existing keybindings.
    t_actions &actions = action_contexts[context];

    // A new action is created in the event that the user creates a local
    // keymapping that masks a global one.
    if( actions.find( action_descriptor ) == actions.end() ) {
        action_attributes &attributes = actions[action_descriptor];
        attributes.name = get_default_action_name( action_descriptor );
        attributes.is_user_created = true;
    }

    return actions[action_descriptor].input_events;
}

void input_manager::remove_input_for_action(
    const std::string &action_descriptor, const std::string &context )
{
    const t_action_contexts::iterator action_context = action_contexts.find( context );
    if( action_context != action_contexts.end() ) {
        t_actions &actions = action_context->second;
        t_actions::iterator action = actions.find( action_descriptor );
        if( action != actions.end() ) {
            if( action->second.is_user_created ) {
                // Since this is a user created hotkey, remove it so that the
                // user will fallback to the hotkey in the default context.
                actions.erase( action );
            } else {
                // If a context no longer has any keybindings remaining for an action but
                // there's an attempt to remove bindings anyway, presumably the user wants
                // to fully remove the binding from that context.
                if( action->second.input_events.empty() ) {
                    actions.erase( action );
                } else {
                    action->second.input_events.clear();
                }
            }
        }
    }
}

void input_manager::add_input_for_action(
    const std::string &action_descriptor, const std::string &context, const input_event &event )
{
    t_input_event_list &events = get_or_create_event_list( action_descriptor, context );
    for( auto &events_a : events ) {
        if( events_a == event ) {
            return;
        }
    }
    events.push_back( event );
}

bool input_context::action_uses_input( const std::string &action_id,
                                       const input_event &event ) const
{
    const auto &events = inp_mngr.get_action_attributes( action_id, category ).input_events;
    return std::find( events.begin(), events.end(), event ) != events.end();
}

std::string input_context::get_conflicts( const input_event &event ) const
{
    return enumerate_as_string( registered_actions.begin(), registered_actions.end(),
    [ this, &event ]( const std::string & action ) {
        return action_uses_input( action, event ) ? get_action_name( action ) : std::string();
    } );
}

void input_context::clear_conflicting_keybindings( const input_event &event )
{
    // The default context is always included to cover cases where the same
    // keybinding exists for the same action in both the global and local
    // contexts.
    input_manager::t_actions &default_actions = inp_mngr.action_contexts[default_context_id];
    input_manager::t_actions &category_actions = inp_mngr.action_contexts[category];

    for( std::vector<std::string>::const_iterator registered_action = registered_actions.begin();
         registered_action != registered_actions.end();
         ++registered_action ) {
        input_manager::t_actions::iterator default_action = default_actions.find( *registered_action );
        input_manager::t_actions::iterator category_action = category_actions.find( *registered_action );
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

const std::string CATA_ERROR = "ERROR";
const std::string ANY_INPUT = "ANY_INPUT";
const std::string HELP_KEYBINDINGS = "HELP_KEYBINDINGS";
const std::string COORDINATE = "COORDINATE";
const std::string TIMEOUT = "TIMEOUT";

const std::string &input_context::input_to_action( const input_event &inp ) const
{
    for( auto &elem : registered_actions ) {
        const std::string &action = elem;
        const std::vector<input_event> &check_inp = inp_mngr.get_input_for_action( action, category );

        // Does this action have our queried input event in its keybindings?
        for( auto &check_inp_i : check_inp ) {
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
    register_action( action_descriptor, "" );
}

void input_context::register_action( const std::string &action_descriptor, const std::string &name )
{
    if( action_descriptor == "ANY_INPUT" ) {
        registered_any_input = true;
    } else if( action_descriptor == "COORDINATE" ) {
        handling_coordinate_input = true;
    }

    registered_actions.push_back( action_descriptor );
    if( !name.empty() ) {
        action_name_overrides[action_descriptor] = name;
    }
}

std::vector<char> input_context::keys_bound_to( const std::string &action_descriptor,
        const bool restrict_to_printable ) const
{
    std::vector<char> result;
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_descriptor,
            category );
    for( const auto &events_event : events ) {
        // Ignore multi-key input and non-keyboard input
        // TODO: fix for Unicode.
        if( events_event.type == CATA_INPUT_KEYBOARD && events_event.sequence.size() == 1 ) {
            if( !restrict_to_printable || ( events_event.sequence.front() < 0xFF &&
                                            isprint( events_event.sequence.front() ) ) ) {
                result.push_back( static_cast<char>( events_event.sequence.front() ) );
            }
        }
    }
    return result;
}

std::string input_context::key_bound_to( const std::string &action_descriptor, const size_t index,
        const bool restrict_to_printable ) const
{
    const auto bound_keys = keys_bound_to( action_descriptor, restrict_to_printable );
    return bound_keys.size() > index ? std::string( 1, bound_keys[index] ) : "";
}

std::string input_context::get_available_single_char_hotkeys( std::string requested_keys )
{
    for( std::vector<std::string>::const_iterator registered_action = registered_actions.begin();
         registered_action != registered_actions.end();
         ++registered_action ) {

        const std::vector<input_event> &events = inp_mngr.get_input_for_action( *registered_action,
                category );
        for( const auto &events_event : events ) {
            // Only consider keyboard events without modifiers
            if( events_event.type == CATA_INPUT_KEYBOARD && events_event.modifiers.empty() ) {
                requested_keys.erase( std::remove_if( requested_keys.begin(), requested_keys.end(),
                                                      ContainsPredicate<std::vector<int>, char>(
                                                              events_event.sequence ) ),
                                      requested_keys.end() );
            }
        }
    }

    return requested_keys;
}

std::string input_context::get_desc( const std::string &action_descriptor,
                                     const unsigned int max_limit,
                                     const std::function<bool( const input_event & )> evt_filter ) const
{
    if( action_descriptor == "ANY_INPUT" ) {
        return "(*)"; // * for wildcard
    }

    bool is_local = false;
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_descriptor,
            category, &is_local );

    if( events.empty() ) {
        return is_local ? _( "Unbound locally!" ) : _( "Unbound globally!" ) ;
    }

    std::vector<input_event> inputs_to_show;
    for( auto &events_i : events ) {
        const input_event &event = events_i;

        if( evt_filter( event ) &&
            // Only display gamepad buttons if a gamepad is available.
            ( gamepad_available() || event.type != CATA_INPUT_GAMEPAD ) ) {

            inputs_to_show.push_back( event );
        }

        if( max_limit > 0 && inputs_to_show.size() == max_limit ) {
            break;
        }
    }

    if( inputs_to_show.empty() ) {
        return pgettext( "keybinding", "Disabled" );
    }

    std::stringstream rval;
    for( size_t i = 0; i < inputs_to_show.size(); ++i ) {
        for( size_t j = 0; j < inputs_to_show[i].sequence.size(); ++j ) {
            rval << inp_mngr.get_keyname( inputs_to_show[i].sequence[j], inputs_to_show[i].type );
        }

        // We're generating a list separated by "," and "or"
        if( i + 2 == inputs_to_show.size() ) {
            rval << _( " or " );
        } else if( i + 1 < inputs_to_show.size() ) {
            rval << ", ";
        }
    }
    return rval.str();
}

std::string input_context::get_desc( const std::string &action_descriptor,
                                     const std::string &text,
                                     const std::function<bool( const input_event & )> evt_filter ) const
{
    if( action_descriptor == "ANY_INPUT" ) {
        //~ keybinding description for anykey
        return string_format( pgettext( "keybinding", "[any] %s" ), text );
    }

    const auto &events = inp_mngr.get_input_for_action( action_descriptor, category );

    bool na = true;
    for( const auto &evt : events ) {
        if( evt_filter( evt ) &&
            // Only display gamepad buttons if a gamepad is available.
            ( gamepad_available() || evt.type != CATA_INPUT_GAMEPAD ) ) {

            na = false;
            if( evt.type == CATA_INPUT_KEYBOARD && evt.sequence.size() == 1 ) {
                const int ch = evt.get_first_input();
                const std::string key = utf32_to_utf8( ch );
                const auto pos = ci_find_substr( text, key );
                if( ch > ' ' && ch <= '~' && pos >= 0 ) {
                    return text.substr( 0, pos ) + "(" + key + ")" + text.substr( pos + key.size() );
                }
            }
        }
    }

    if( na ) {
        //~ keybinding description for unbound or disabled keys
        return string_format( pgettext( "keybinding", "[n/a] %s" ), text );
    } else {
        //~ keybinding description for bound keys
        return string_format( pgettext( "keybinding", "[%s] %s" ),
                              get_desc( action_descriptor, 1, evt_filter ), text );
    }
}

const std::string &input_context::handle_input()
{
    return handle_input( timeout );
}

const std::string &input_context::handle_input( const int timeout )
{
    const auto old_timeout = inp_mngr.get_timeout();
    if( timeout >= 0 ) {
        inp_mngr.set_timeout( timeout );
    }
    next_action.type = CATA_INPUT_ERROR;
    const std::string *result = &CATA_ERROR;
    while( true ) {
        next_action = inp_mngr.get_input_event();
        if( next_action.type == CATA_INPUT_TIMEOUT ) {
            result = &TIMEOUT;
            break;
        }

        const std::string &action = input_to_action( next_action );

        // Special help action
        if( action == "HELP_KEYBINDINGS" ) {
            inp_mngr.reset_timeout();
            display_menu();
            inp_mngr.set_timeout( timeout );
            result = &HELP_KEYBINDINGS;
            break;
        }

        if( next_action.type == CATA_INPUT_MOUSE ) {
            if( !handling_coordinate_input && action == CATA_ERROR ) {
                continue; // Ignore this mouse input.
            }

            coordinate_input_received = true;
            coordinate = next_action.mouse_pos;
        } else {
            coordinate_input_received = false;
        }

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

// dx and dy are -1, 0, or +1. Rotate the indicated direction 1/8 turn clockwise.
void rotate_direction_cw( int &dx, int &dy )
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

cata::optional<tripoint> input_context::get_direction( const std::string &action ) const
{
    static const auto noop = static_cast<tripoint( * )( tripoint )>( []( tripoint p ) {
        return p;
    } );
    static const auto rotate = static_cast<tripoint( * )( tripoint )>( []( tripoint p ) {
        rotate_direction_cw( p.x, p.y );
        return p;
    } );
    const auto transform = iso_mode && tile_iso && use_tiles ? rotate : noop;

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
        return cata::nullopt;
    }
}

// Custom set of hotkeys that explicitly don't include the hardcoded
// alternative hotkeys, which mustn't be included so that the hardcoded
// hotkeys do not show up beside entries within the window.
const std::string display_help_hotkeys =
    "abcdefghijkpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:;'\",./<>?!@#$%^&*()_[]\\{}|`~";

void input_context::display_menu()
{
    // Shamelessly stolen from help.cpp

    input_context ctxt( "HELP_KEYBINDINGS" );
    ctxt.register_action( "UP", translate_marker( "Scroll up" ) );
    ctxt.register_action( "DOWN", translate_marker( "Scroll down" ) );
    ctxt.register_action( "PAGE_DOWN" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "REMOVE" );
    ctxt.register_action( "ADD_LOCAL" );
    ctxt.register_action( "ADD_GLOBAL" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "ANY_INPUT" );

    if( category != "HELP_KEYBINDINGS" ) {
        // avoiding inception!
        ctxt.register_action( "HELP_KEYBINDINGS" );
    }

    std::string hotkeys = ctxt.get_available_single_char_hotkeys( display_help_hotkeys );

    int maxwidth = max( FULL_SCREEN_WIDTH, TERMX );
    int width = min( 80, maxwidth );
    int maxheight = max( FULL_SCREEN_HEIGHT, TERMY );
    int height = min( maxheight, static_cast<int>( hotkeys.size() ) + LEGEND_HEIGHT + BORDER_SPACE );

    catacurses::window w_help = catacurses::newwin( height - 2, width - 2,
                                point( maxwidth / 2 - width / 2, maxheight / 2 - height / 2 ) );

    // has the user changed something?
    bool changed = false;
    // keybindings before the user changed anything.
    input_manager::t_action_contexts old_action_contexts( inp_mngr.action_contexts );
    // current status: adding/removing/showing keybindings
    enum { s_remove, s_add, s_add_global, s_show } status = s_show;
    // copy of registered_actions, but without the ANY_INPUT and COORDINATE, which should not be shown
    std::vector<std::string> org_registered_actions( registered_actions );
    org_registered_actions.erase( std::remove_if( org_registered_actions.begin(),
                                  org_registered_actions.end(),
    []( const std::string & a ) {
        return a == ANY_INPUT || a == COORDINATE;
    } ), org_registered_actions.end() );

    // colors of the keybindings
    static const nc_color global_key = c_light_gray;
    static const nc_color local_key = c_light_green;
    static const nc_color unbound_key = c_light_red;
    // (vertical) scroll offset
    size_t scroll_offset = 0;
    // height of the area usable for display of keybindings, excludes headers & borders
    const size_t display_height = height - LEGEND_HEIGHT - BORDER_SPACE; // -2 for the border
    // width of the legend
    const size_t legwidth = width - 4 - BORDER_SPACE;
    // keybindings help
    std::ostringstream legend;
    legend << "<color_" << string_from_color( unbound_key ) << ">" << _( "Unbound keys" ) <<
           "</color>\n";
    legend << "<color_" << string_from_color( local_key ) << ">" <<
           _( "Keybinding active only on this screen" ) << "</color>\n";
    legend << "<color_" << string_from_color( global_key ) << ">" << _( "Keybinding active globally" )
           <<
           "</color>\n";
    legend << _( "Press - to remove keybinding\nPress + to add local keybinding\nPress = to add global keybinding\n" );

    std::vector<std::string> filtered_registered_actions = org_registered_actions;
    std::string filter_phrase;
    std::string action;
    int raw_input_char = 0;
    string_input_popup spopup;
    spopup.window( w_help, 4, 8, legwidth )
    .max_length( legwidth )
    .context( ctxt );

    // do not switch IME mode now, but restore previous mode on return
    ime_sentry sentry( ime_sentry::keep );
    while( true ) {
        werase( w_help );
        draw_border( w_help, BORDER_COLOR, _( "Keybindings" ), c_light_red );
        draw_scrollbar( w_help, scroll_offset, display_height,
                        filtered_registered_actions.size(), 10, 0, c_white, true );
        fold_and_print( w_help, point( 2, 1 ), legwidth, c_white, legend.str() );

        for( size_t i = 0; i + scroll_offset < filtered_registered_actions.size() &&
             i < display_height; i++ ) {
            const std::string &action_id = filtered_registered_actions[i + scroll_offset];

            bool overwrite_default;
            const action_attributes &attributes = inp_mngr.get_action_attributes( action_id, category,
                                                  &overwrite_default );

            char invlet;
            if( i < hotkeys.size() ) {
                invlet = hotkeys[i];
            } else {
                invlet = ' ';
            }

            if( status == s_add_global && overwrite_default ) {
                // We're trying to add a global, but this action has a local
                // defined, so gray out the invlet.
                mvwprintz( w_help, point( 2, i + 10 ), c_dark_gray, "%c ", invlet );
            } else if( status == s_add || status == s_add_global ) {
                mvwprintz( w_help, point( 2, i + 10 ), c_light_blue, "%c ", invlet );
            } else if( status == s_remove ) {
                mvwprintz( w_help, point( 2, i + 10 ), c_light_blue, "%c ", invlet );
            } else {
                mvwprintz( w_help, point( 2, i + 10 ), c_blue, "  " );
            }
            nc_color col;
            if( attributes.input_events.empty() ) {
                col = unbound_key;
            } else if( overwrite_default ) {
                col = local_key;
            } else {
                col = global_key;
            }
            mvwprintz( w_help, point( 4, i + 10 ), col, "%s: ", get_action_name( action_id ) );
            mvwprintz( w_help, point( 52, i + 10 ), col, "%s", get_desc( action_id ) );
        }

        // spopup.query_string() will call wrefresh( w_help )
        catacurses::refresh();

        spopup.text( filter_phrase );
        if( status == s_show ) {
            filter_phrase = spopup.query_string( false );
            action = ctxt.input_to_action( ctxt.get_raw_input() );
        } else {
            spopup.query_string( false, true );
            action = ctxt.handle_input();
        }
        raw_input_char = ctxt.get_raw_input().get_first_input();

        filtered_registered_actions = filter_strings_by_phrase( org_registered_actions, filter_phrase );
        if( scroll_offset > filtered_registered_actions.size() ) {
            scroll_offset = 0;
        }

        if( filtered_registered_actions.empty() && action != "QUIT" ) {
            continue;
        }

        // In addition to the modifiable hotkeys, we also check for hardcoded
        // keys, e.g. '+', '-', '=', in order to prevent the user from
        // entering an unrecoverable state.
        if( action == "ADD_LOCAL" || raw_input_char == '+' ) {
            status = s_add;
        } else if( action == "ADD_GLOBAL" || raw_input_char == '=' ) {
            status = s_add_global;
        } else if( action == "REMOVE" || raw_input_char == '-' ) {
            status = s_remove;
        } else if( action == "ANY_INPUT" ) {
            const size_t hotkey_index = hotkeys.find_first_of( raw_input_char );
            if( hotkey_index == std::string::npos ) {
                continue;
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
                                              .message( _( "New key for %s" ), name )
                                              .allow_anykey( true )
                                              .query()
                                              .evt;

                if( action_uses_input( action_id, new_event ) ) {
                    popup_getkey( _( "This key is already used for %s." ), name );
                    status = s_show;
                    continue;
                }

                const std::string conflicts = get_conflicts( new_event );
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
            }
            status = s_show;
        } else if( action == "DOWN" ) {
            if( filtered_registered_actions.size() > display_height &&
                scroll_offset < filtered_registered_actions.size() - display_height ) {
                scroll_offset++;
            }
        } else if( action == "UP" ) {
            if( scroll_offset > 0 ) {
                scroll_offset--;
            }
        } else if( action == "PAGE_DOWN" ) {
            if( scroll_offset + display_height < filtered_registered_actions.size() ) {
                scroll_offset += std::min( display_height, filtered_registered_actions.size() -
                                           display_height - scroll_offset );
            } else if( filtered_registered_actions.size() > display_height ) {
                scroll_offset = 0;
            }
        } else if( action == "PAGE_UP" ) {
            if( scroll_offset >= display_height ) {
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
    werase( w_help );
    wrefresh( w_help );
}

input_event input_context::get_raw_input()
{
    return next_action;
}

int input_manager::get_previously_pressed_key() const
{
    return previously_pressed_key;
}

void input_manager::wait_for_any_key()
{
#if defined(__ANDROID__)
    input_context ctxt( "WAIT_FOR_ANY_KEY" );
#endif
    while( true ) {
        switch( inp_mngr.get_input_event().type ) {
            case CATA_INPUT_KEYBOARD:
            // errors are accepted as well to avoid an infinite loop
            case CATA_INPUT_ERROR:
                return;
            default:
                break;
        }
    }
}

#if !(defined(TILES) || defined(_WIN32))
// Also specify that we don't have a gamepad plugged in.
bool gamepad_available()
{
    return false;
}

cata::optional<tripoint> input_context::get_coordinates( const catacurses::window &capture_win )
{
    if( !coordinate_input_received ) {
        return cata::nullopt;
    }
    const point view_size( getmaxx( capture_win ), getmaxy( capture_win ) );
    const point win_min( getbegx( capture_win ) - VIEW_OFFSET_X,
                         getbegy( capture_win ) - VIEW_OFFSET_Y );
    const rectangle win_bounds( win_min, win_min + view_size );
    if( !win_bounds.contains_half_open( coordinate ) ) {
        return cata::nullopt;
    }

    point view_offset;
    if( capture_win == g->w_terrain ) {
        view_offset = g->ter_view_p.xy();
    }

    const point p = view_offset - ( view_size / 2 - coordinate );
    return tripoint( p, g->get_levz() );
}
#endif

std::string input_context::get_action_name( const std::string &action_id ) const
{
    // 1) Check action name overrides specific to this input_context
    const input_manager::t_string_string_map::const_iterator action_name_override =
        action_name_overrides.find( action_id );
    if( action_name_override != action_name_overrides.end() ) {
        return _( action_name_override->second );
    }

    // 2) Check if the hotkey has a name
    const action_attributes &attributes = inp_mngr.get_action_attributes( action_id, category );
    if( !attributes.name.empty() ) {
        return _( attributes.name );
    }

    // 3) If the hotkey has no name, the user has created a local hotkey in
    // this context that is masking the global hotkey. Fallback to the global
    // hotkey's name.
    const action_attributes &default_attributes = inp_mngr.get_action_attributes( action_id,
            default_context_id );
    if( !default_attributes.name.empty() ) {
        return _( default_attributes.name );
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
std::string input_context::press_x( const std::string &action_id, const std::string &key_bound_pre,
                                    const std::string &key_bound_suf, const std::string &key_unbound ) const
{
    if( action_id == "ANY_INPUT" ) {
        return _( "any key" );
    }
    if( action_id == "COORDINATE" ) {
        return _( "mouse movement" );
    }
    const input_manager::t_input_event_list &events = inp_mngr.get_input_for_action( action_id,
            category );
    if( events.empty() ) {
        return key_unbound;
    }
    std::ostringstream keyed;
    keyed << key_bound_pre;
    for( size_t j = 0; j < events.size(); j++ ) {
        for( size_t k = 0; k < events[j].sequence.size(); ++k ) {
            keyed << inp_mngr.get_keyname( events[j].sequence[k], events[j].type );
        }
        if( j + 1 < events.size() ) {
            keyed << _( " or " );
        }
    }
    keyed << key_bound_suf;
    return keyed.str();
}

void input_context::set_iso( bool mode )
{
    iso_mode = mode;
}

std::vector<std::string> input_context::filter_strings_by_phrase(
    const std::vector<std::string> &strings, const std::string &phrase ) const
{
    std::vector<std::string> filtered_strings;

    for( auto &str : strings ) {
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
