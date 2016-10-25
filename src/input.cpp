#include "action.h"
#include "cursesdef.h"
#include "input.h"
#include "debug.h"
#include "json.h"
#include "output.h"
#include "game.h"
#include "path_info.h"
#include "filesystem.h"
#include "translations.h"
#include "catacharset.h"
#include "cata_utility.h"
#include "options.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <errno.h>
#include <ctype.h>
#include <algorithm>

extern bool tile_iso;

static const std::string default_context_id( "default" );

template <class T1, class T2>
struct ContainsPredicate {
    const T1 &container;

    ContainsPredicate( const T1 &container ) : container( container ) { }

    // Operator overload required to leverage std functional iterface.
    bool operator()( T2 c ) {
        return std::find( container.begin(), container.end(), c ) != container.end();
    }
};

static long str_to_long( const std::string &number )
{
    // ensure user's locale doesn't interfere with number format
    std::istringstream buffer( number );
    buffer.imbue( std::locale::classic() );
    long result;
    buffer >> result;
    return result;
}

static std::string long_to_str( long number )
{
    // ensure user's locale doesn't interfere with number format
    std::ostringstream buffer;
    buffer.imbue( std::locale::classic() );
    buffer << number;
    return buffer.str();
}

bool is_mouse_enabled()
{
#if ((defined _WIN32 || defined WINDOWS) && !(defined TILES))
    return false;
#else
    return true;
#endif
}

//helper function for those have problem inputing certain characters.
std::string get_input_string_from_file( std::string fname )
{
    std::string ret;
    read_from_file_optional( fname, [&ret]( std::istream & fin ) {
        getline( fin, ret );
        //remove utf8 bmm
        if( !ret.empty() && ( unsigned char )ret[0] == 0xef ) {
            ret.erase( 0, 3 );
        }
        while( !ret.empty() && ( ret[ret.size() - 1] == '\r' ||  ret[ret.size() - 1] == '\n' ) ) {
            ret.erase( ret.size() - 1, 1 );
        }
    } );
    return ret;
}

input_manager inp_mngr;

void input_manager::init()
{
    std::map<char, action_id> keymap;
    std::string keymap_file_loaded_from;
    std::set<action_id> unbound_keymap;
    load_keyboard_settings( keymap, keymap_file_loaded_from, unbound_keymap );
    init_keycode_mapping();

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

void input_manager::add_keycode_pair( long ch, const std::string &name )
{
    keycode_to_keyname[ch] = name;
    keyname_to_keycode[name] = ch;
}

void input_manager::add_gamepad_keycode_pair( long ch, const std::string &name )
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

long input_manager::get_keycode( const std::string &name ) const
{
    const t_name_to_key_map::const_iterator a = keyname_to_keycode.find( name );
    if( a != keyname_to_keycode.end() ) {
        return a->second;
    }
    // Not found in map, try to parse as long
    if( name.compare( 0, 8, "UNKNOWN_" ) == 0 ) {
        return str_to_long( name.substr( 8 ) );
    }
    return 0;
}

std::string input_manager::get_keyname( long ch, input_event_t inp_type, bool portable ) const
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
        return std::string( "UNKNOWN_" ) + long_to_str( ch );
    }
    return string_format( _( "unknown key %ld" ), ch );
}

const std::vector<input_event> &input_manager::get_input_for_action( const std::string
        &action_descriptor, const std::string context, bool *overwrites_default )
{
    const action_attributes &attributes = get_action_attributes( action_descriptor, context,
                                          overwrites_default );
    return attributes.input_events;
}

const action_attributes &input_manager::get_action_attributes(
    const std::string &action_id,
    const std::string context,
    bool *overwrites_default )
{

    if( context != default_context_id ) {
        // Check if the action exists in the provided context
        t_action_contexts::const_iterator action_context = action_contexts.find( context );
        if( action_context != action_contexts.end() ) {
            t_actions::const_iterator action = action_context->second.find( action_id );
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
                action->second.input_events.clear();
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

const std::string &input_context::input_to_action( input_event &inp )
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

void input_manager::set_timeout( int delay )
{
    timeout( delay );
    // Use this to determine when curses should return a CATA_INPUT_TIMEOUT event.
    input_timeout = delay;
}

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


std::vector<char> input_context::keys_bound_to( const std::string &action_descriptor ) const
{
    std::vector<char> result;
    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_descriptor,
            category );
    for( const auto &events_event : events ) {
        // Ignore multi-key input and non-keyboard input
        // TODO: fix for unicode.
        if( events_event.type == CATA_INPUT_KEYBOARD && events_event.sequence.size() == 1 &&
            events_event.sequence.front() < 0xFF && isprint( events_event.sequence.front() ) ) {
            result.push_back( ( char )events_event.sequence.front() );
        }
    }
    return result;
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
            if( events_event.type == CATA_INPUT_KEYBOARD && 0 == events_event.modifiers.size() ) {
                requested_keys.erase( std::remove_if( requested_keys.begin(), requested_keys.end(),
                                                      ContainsPredicate<std::vector<long>, char>(
                                                              events_event.sequence ) ),
                                      requested_keys.end() );
            }
        }
    }

    return requested_keys;
}

const std::string input_context::get_desc( const std::string &action_descriptor,
        const unsigned int max_limit ) const
{
    if( action_descriptor == "ANY_INPUT" ) {
        return "(*)"; // * for wildcard
    }

    const std::vector<input_event> &events = inp_mngr.get_input_for_action( action_descriptor,
            category );

    if( events.empty() ) {
        return _( "Unbound!" );
    }

    std::vector<input_event> inputs_to_show;
    for( auto &events_i : events ) {
        const input_event &event = events_i;

        // Only display gamepad buttons if a gamepad is available.
        if( gamepad_available() || event.type != CATA_INPUT_GAMEPAD ) {
            inputs_to_show.push_back( event );
        }

        if( max_limit > 0 && inputs_to_show.size() == max_limit ) {
            break;
        }
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

const std::string &input_context::handle_input()
{
    next_action.type = CATA_INPUT_ERROR;
    while( 1 ) {
        next_action = inp_mngr.get_input_event( NULL );
        if( next_action.type == CATA_INPUT_TIMEOUT ) {
            return TIMEOUT;
        }

        const std::string &action = input_to_action( next_action );

        // Special help action
        if( action == "HELP_KEYBINDINGS" ) {
            display_help();
            return HELP_KEYBINDINGS;
        }

        if( next_action.type == CATA_INPUT_MOUSE ) {
            if( !handling_coordinate_input ) {
                continue; // Ignore this mouse input.
            }

            coordinate_input_received = true;
            coordinate_x = next_action.mouse_x;
            coordinate_y = next_action.mouse_y;
        } else {
            coordinate_input_received = false;
        }

        if( action != CATA_ERROR ) {
            return action;
        }

        // If we registered to receive any input, return ANY_INPUT
        // to signify that an unregistered key was pressed.
        if( registered_any_input ) {
            return ANY_INPUT;
        }

        // If it's an invalid key, just keep looping until the user
        // enters something proper.
    }
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
    static const int rotate_direction_vec[] = { 1, 2, 5, 0, 4, 8, 3, 6, 7 };
    dir_num = rotate_direction_vec[dir_num];
    // convert back to -1,0,+1
    dx = ( dir_num % 3 ) - 1;
    dy = ( dir_num / 3 ) - 1;
}

bool input_context::get_direction( int &dx, int &dy, const std::string &action )
{
    if( action == "UP" ) {
        dx = 0;
        dy = -1;
    } else if( action == "DOWN" ) {
        dx = 0;
        dy = 1;
    } else if( action == "LEFT" ) {
        dx = -1;
        dy = 0;
    } else if( action ==  "RIGHT" ) {
        dx = 1;
        dy = 0;
    } else if( action == "LEFTUP" ) {
        dx = -1;
        dy = -1;
    } else if( action == "RIGHTUP" ) {
        dx = 1;
        dy = -1;
    } else if( action == "LEFTDOWN" ) {
        dx = -1;
        dy = 1;
    } else if( action == "RIGHTDOWN" ) {
        dx = 1;
        dy = 1;
    } else {
        dx = -2;
        dy = -2;
        return false;
    }
    if( iso_mode && tile_iso && use_tiles ) {
        rotate_direction_cw( dx, dy );
    }
    return true;
}

void input_context::display_help()
{
    inp_mngr.set_timeout( -1 );
    // Shamelessly stolen from help.cpp
    WINDOW *w_help = newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                             1 + ( int )( ( TERMY > FULL_SCREEN_HEIGHT ) ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ),
                             1 + ( int )( ( TERMX > FULL_SCREEN_WIDTH ) ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ) );

    // has the user changed something?
    bool changed = false;
    // keybindings before the user changed anything.
    input_manager::t_action_contexts old_action_contexts( inp_mngr.action_contexts );
    // current status: adding/removing/showing keybindings
    enum { s_remove, s_add, s_add_global, s_show } status = s_show;
    // copy of registered_actions, but without the ANY_INPUT and COORDINATE, which should not be shown
    std::vector<std::string> org_registered_actions( registered_actions );
    std::vector<std::string>::iterator any_input = std::find( org_registered_actions.begin(),
            org_registered_actions.end(), ANY_INPUT );
    if( any_input != org_registered_actions.end() ) {
        org_registered_actions.erase( any_input );
    }
    std::vector<std::string>::iterator coordinate = std::find( org_registered_actions.begin(),
            org_registered_actions.end(), COORDINATE );
    if( coordinate != org_registered_actions.end() ) {
        org_registered_actions.erase( coordinate );
    }

    // colors of the keybindings
    static const nc_color global_key = c_ltgray;
    static const nc_color local_key = c_ltgreen;
    static const nc_color unbound_key = c_ltred;
    // (vertical) scroll offset
    size_t scroll_offset = 0;
    // height of the area usable for display of keybindings, excludes headers & borders
    const size_t display_height = FULL_SCREEN_HEIGHT - 11 - 2; // -2 for the border
    // width of the legend
    const size_t legwidth = FULL_SCREEN_WIDTH - 4 - 2;
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

    input_context ctxt( "HELP_KEYBINDINGS" );
    ctxt.register_action( "UP", _( "Scroll up" ) );
    ctxt.register_action( "DOWN", _( "Scroll down" ) );
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

    std::string hotkeys = ctxt.get_available_single_char_hotkeys();
    const std::set<long> bound_character_blacklist = { '+', '-', '=', KEY_ESCAPE };
    std::vector<std::string> filtered_registered_actions = org_registered_actions;
    std::string filter_phrase;
    std::string action;
    long raw_input_char = 0;
    int current_search_cursor_pos = -1;

    while( true ) {
        werase( w_help );
        draw_border( w_help );
        draw_scrollbar( w_help, scroll_offset, display_height,
                        filtered_registered_actions.size() - display_height, 10, 0, c_white, true );
        center_print( w_help, 0, c_ltred, _( "Keybindings" ) );
        fold_and_print( w_help, 1, 2, legwidth, c_white, legend.str() );

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
                mvwprintz( w_help, i + 10, 2, c_dkgray, "%c ", invlet );
            } else if( status == s_add || status == s_add_global ) {
                mvwprintz( w_help, i + 10, 2, c_blue, "%c ", invlet );
            } else if( status == s_remove ) {
                mvwprintz( w_help, i + 10, 2, c_blue, "%c ", invlet );
            } else {
                mvwprintz( w_help, i + 10, 2, c_blue, "  " );
            }
            nc_color col;
            if( attributes.input_events.empty() ) {
                col = unbound_key;
            } else if( overwrite_default ) {
                col = local_key;
            } else {
                col = global_key;
            }
            mvwprintz( w_help, i + 10, 4, col, "%s: ", get_action_name( action_id ).c_str() );
            mvwprintz( w_help, i + 10, 52, col, "%s", get_desc( action_id ).c_str() );
        }

        if( status == s_show ) {
            filter_phrase = string_input_win_from_context( w_help, ctxt, filter_phrase, legwidth, 4, 8,
                            legwidth, false, action, raw_input_char, current_search_cursor_pos, "", -1, -1,
                            true, false, false, std::map<long, std::function<void()>>(),
                            bound_character_blacklist );
        } else {
            string_input_win_from_context( w_help, ctxt, filter_phrase, legwidth, 4, 8, legwidth, false,
                                           action, raw_input_char, current_search_cursor_pos, "", -1, -1,
                                           true, false, true );
            action = ctxt.handle_input();
            raw_input_char = ctxt.get_raw_input().get_first_input();
        }


        if( scroll_offset > filtered_registered_actions.size() ) {
            scroll_offset = 0;
        }

        filtered_registered_actions = filter_strings_by_phrase( org_registered_actions, filter_phrase );

        wrefresh( w_help );
        refresh();

        if( filtered_registered_actions.size() == 0 && action != "QUIT" ) {
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
            inp_mngr.get_action_attributes( action_id, category, &is_local );
            const std::string name = get_action_name( action_id );

            if( status == s_remove && ( !get_option<bool>( "QUERY_KEYBIND_REMOVAL" ) ||
                                        query_yn( _( "Clear keys for %s?" ), name.c_str() ) ) ) {

                // If it's global, reset the global actions.
                std::string category_to_access = category;
                if( !is_local ) {
                    category_to_access = default_context_id;
                }

                inp_mngr.remove_input_for_action( action_id, category_to_access );
                changed = true;
            } else if( status == s_add_global && is_local ) {
                // Disallow adding global actions to an action that already has a local defined.
                popup( _( "There are already local keybindings defined for this action, please remove them first." ) );
            } else if( status == s_add || status == s_add_global ) {
                const long newbind = popup_getkey( _( "New key for %s:" ), name.c_str() );
                const input_event new_event( newbind, CATA_INPUT_KEYBOARD );

                if( action_uses_input( action_id, new_event ) ) {
                    popup_getkey( _( "This key is already used for %s." ), name.c_str() );
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
            hotkeys = ctxt.get_available_single_char_hotkeys();
        }
    }

    if( changed && query_yn( _( "Save changes?" ) ) ) {
        try {
            inp_mngr.save();
        } catch( std::exception &err ) {
            popup( _( "saving keybindings failed: %s" ), err.what() );
        }
    } else if( changed ) {
        inp_mngr.action_contexts.swap( old_action_contexts );
    }
    werase( w_help );
    wrefresh( w_help );
    delwin( w_help );
}

input_event input_context::get_raw_input()
{
    return next_action;
}

long input_manager::get_previously_pressed_key() const
{
    return previously_pressed_key;
}

#ifndef TILES
// If we're using curses, we need to provide get_input_event() here.
input_event input_manager::get_input_event( WINDOW * /*win*/ )
{
    previously_pressed_key = 0;
    long key = getch();
    // Our current tiles and Windows code doesn't have ungetch()
#if !(defined TILES || defined _WIN32 || defined WINDOWS)
    if( key != ERR ) {
        long newch;
        // Clear the buffer of characters that match the one we're going to act on.
        timeout( 0 );
        do {
            newch = getch();
        } while( newch != ERR && newch == key );
        timeout( -1 );
        // If we read a different character than the one we're going to act on, re-queue it.
        if( newch != ERR && newch != key ) {
            ungetch( newch );
        }
    }
#endif
    input_event rval;
    if( key == ERR ) {
        if( input_timeout > 0 ) {
            rval.type = CATA_INPUT_TIMEOUT;
        } else {
            rval.type = CATA_INPUT_ERROR;
        }
#if !(defined TILES || defined _WIN32 || defined WINDOWS || defined __CYGWIN__)
        // ncurses mouse handling
    } else if( key == KEY_MOUSE ) {
        MEVENT event;
        if( getmouse( &event ) == OK ) {
            rval.type = CATA_INPUT_MOUSE;
            rval.mouse_x = event.x - VIEW_OFFSET_X;
            rval.mouse_y = event.y - VIEW_OFFSET_Y;
            if( event.bstate & BUTTON1_CLICKED ) {
                rval.add_input( MOUSE_BUTTON_LEFT );
            } else if( event.bstate & BUTTON3_CLICKED ) {
                rval.add_input( MOUSE_BUTTON_RIGHT );
            } else if( event.bstate & REPORT_MOUSE_POSITION ) {
                rval.add_input( MOUSE_MOVE );
                if( input_timeout > 0 ) {
                    // Mouse movement seems to clear ncurses timeout
                    set_timeout( input_timeout );
                }
            } else {
                rval.type = CATA_INPUT_ERROR;
            }
        } else {
            rval.type = CATA_INPUT_ERROR;
        }
#endif
    } else {
        if( key == 127 ) { // == Unicode DELETE
            previously_pressed_key = KEY_BACKSPACE;
            return input_event( KEY_BACKSPACE, CATA_INPUT_KEYBOARD );
        }
        rval.type = CATA_INPUT_KEYBOARD;
        rval.text.append( 1, ( char ) key );
        // Read the UTF-8 sequence (if any)
        if( key < 127 ) {
            // Single byte sequence
        } else if( 194 <= key && key <= 223 ) {
            rval.text.append( 1, ( char ) getch() );
        } else if( 224 <= key && key <= 239 ) {
            rval.text.append( 1, ( char ) getch() );
            rval.text.append( 1, ( char ) getch() );
        } else if( 240 <= key && key <= 244 ) {
            rval.text.append( 1, ( char ) getch() );
            rval.text.append( 1, ( char ) getch() );
            rval.text.append( 1, ( char ) getch() );
        } else {
            // Other control character, etc. - no text at all, return an event
            // without the text property
            previously_pressed_key = key;
            return input_event( key, CATA_INPUT_KEYBOARD );
        }
        // Now we have loaded an UTF-8 sequence (possibly several bytes)
        // but we should only return *one* key, so return the code point of it.
        const char *utf8str = rval.text.c_str();
        int len = rval.text.length();
        const uint32_t cp = UTF8_getch( &utf8str, &len );
        if( cp == UNKNOWN_UNICODE ) {
            // Invalid UTF-8 sequence, this should never happen, what now?
            // Maybe return any error instead?
            previously_pressed_key = key;
            return input_event( key, CATA_INPUT_KEYBOARD );
        }
        previously_pressed_key = cp;
        // for compatibility only add the first byte, not the code point
        // as it would  conflict with the special keys defined by ncurses
        rval.add_input( key );
    }

    return rval;
}

// Also specify that we don't have a gamepad plugged in.
bool gamepad_available()
{
    return false;
}

bool input_context::get_coordinates( WINDOW *capture_win, int &x, int &y )
{
    if( !coordinate_input_received ) {
        return false;
    }
    int view_columns = getmaxx( capture_win );
    int view_rows = getmaxy( capture_win );
    int win_left = getbegx( capture_win ) - VIEW_OFFSET_X;
    int win_right = win_left + view_columns - 1;
    int win_top = getbegy( capture_win ) - VIEW_OFFSET_Y;
    int win_bottom = win_top + view_rows - 1;
    if( coordinate_x < win_left || coordinate_x > win_right || coordinate_y < win_top ||
        coordinate_y > win_bottom ) {
        return false;
    }

    x = g->ter_view_x - ( ( view_columns / 2 ) - coordinate_x );
    y = g->ter_view_y - ( ( view_rows / 2 ) - coordinate_y );

    return true;
}
#endif

#ifndef TILES
void init_interface()
{
#if !(defined TILES || defined _WIN32 || defined WINDOWS || defined __CYGWIN__)
    // ncurses mouse registration
    mousemask( BUTTON1_CLICKED | BUTTON3_CLICKED | REPORT_MOUSE_POSITION, NULL );
#endif
}
#endif

const std::string input_context::get_action_name( const std::string &action_id ) const
{
    // 1) Check action name overrides specific to this input_context
    const input_manager::t_string_string_map::const_iterator action_name_override =
        action_name_overrides.find( action_id );
    if( action_name_override != action_name_overrides.end() ) {
        return action_name_override->second;
    }

    // 2) Check if the hotkey has a name
    const action_attributes &attributes = inp_mngr.get_action_attributes( action_id, category );
    if( !attributes.name.empty() ) {
        return _( attributes.name.c_str() );
    }

    // 3) If the hotkey has no name, the user has created a local hotkey in
    // this context that is masking the global hotkey. Fallback to the global
    // hotkey's name.
    const action_attributes &default_attributes = inp_mngr.get_action_attributes( action_id,
            default_context_id );
    if( !default_attributes.name.empty() ) {
        return _( default_attributes.name.c_str() );
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

