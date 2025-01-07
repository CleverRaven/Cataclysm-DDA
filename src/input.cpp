#include "input.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <locale>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "action.h"
#include "cached_options.h" // IWYU pragma: keep
#include "cata_path.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "debug.h"
#include "filesystem.h"
#include "flexbuffer_json-inl.h"
#include "flexbuffer_json.h"
#include "input_context.h" // IWYU pragma: keep
#include "json.h"
#include "json_error.h"
#include "json_loader.h"
#include "path_info.h"
#include "sdltiles.h" // IWYU pragma: keep
#include "string_formatter.h"
#include "translations.h"

static const std::string default_context_id( "default" );

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

bool is_keycode_mode_supported()
{
#if defined(TILES) && !defined(__ANDROID__) && !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE == 1)
    return keycode_mode;
#else
    return false;
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
        while( !ret.empty() && ( ret.back() == '\r' ||  ret.back() == '\n' ) ) {
            ret.erase( ret.size() - 1, 1 );
        }
    } );
    return ret;
}

input_event::input_event( const std::set<keymod_t> &mod, const int s, const input_event_t t )
    : type( t ), modifiers( mod ), edit_refresh( false )
{
    sequence.emplace_back( s );
#if defined(__ANDROID__)
    shortcut_last_used_action_counter = 0;
#endif
}

int input_event::get_first_input() const
{
    if( sequence.empty() ) {
        return UNKNOWN_UNICODE;
    }

    return sequence[0];
}

bool input_event::operator!=( const input_event &other ) const
{
    return !operator==( other );
}

static const std::vector<std::pair<keymod_t, translation>> keymod_long_desc = {
    { keymod_t::ctrl,  to_translation( "key modifier", "CTRL-" ) },
    { keymod_t::alt,   to_translation( "key modifier", "ALT-" ) },
    { keymod_t::shift, to_translation( "key modifier", "SHIFT-" ) },
};

std::string input_event::long_description() const
{
    std::string rval;
    // test in fixed order to generate consistent description
    for( const auto &v : keymod_long_desc ) {
        if( modifiers.count( v.first ) ) {
            rval += v.second.translated();
        }
    }
    for( const int code : sequence ) {
        rval += inp_mngr.get_keyname( code, type );
    }
    return rval;
}

static const std::vector<std::pair<keymod_t, std::string>> keymod_short_desc = {
    { keymod_t::ctrl,  "^" },
    { keymod_t::alt,   "\u2325" }, // option key
    { keymod_t::shift, "\u21E7" }, // upwards white arrow
};

std::string input_event::short_description() const
{
    std::string rval;
    // test in fixed order to generate consistent description
    for( const auto &v : keymod_short_desc ) {
        if( modifiers.count( v.first ) ) {
            rval += v.second;
        }
    }
    // TODO: add short description for control keys such as return, tab, etc
    for( const int code : sequence ) {
        rval += inp_mngr.get_keyname( code, type );
    }
    return rval;
}

bool input_event::compare_type_mod_code( const input_event &lhs, const input_event &rhs )
{
    return std::tie( lhs.type, lhs.modifiers, lhs.sequence )
           < std::tie( rhs.type, rhs.modifiers, rhs.sequence );
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
        load( PATH_INFO::keybindings(), false );
    } catch( const JsonError &err ) {
        throw std::runtime_error( err.what() );
    }
    try {
        load( PATH_INFO::keybindings_vehicle(), false );
    } catch( const JsonError &err ) {
        throw std::runtime_error( err.what() );
    }
    // save non customized keybindings
    basic_action_contexts = action_contexts;
    try {
        load( PATH_INFO::user_keybindings(), true );
    } catch( const JsonError &err ) {
        throw std::runtime_error( err.what() );
    }

    if( keymap_file_loaded_from.empty() || ( keymap.empty() && unbound_keymap.empty() ) ) {
        // No keymap file was loaded, or the file has no mappings and no unmappings,
        // we can skip the remaining part of the function, especially the save function
        return;
    }
    t_actions &actions = action_contexts["DEFAULTMODE"];
    std::set<action_id> touched;
    for( const std::pair<const char, action_id> &a : keymap ) {
        const std::string action_id = action_ident( a.second );
        // Put the binding from keymap either into the global context
        // (if an action with that ident already exists there - think movement keys)
        // or otherwise to the DEFAULTMODE context.
        std::string context = "DEFAULTMODE";
        if( action_contexts[default_context_id].count( action_id ) > 0 ) {
            context = default_context_id;
        } else if( touched.count( a.second ) == 0 ) {
            // Note: movement keys are somehow special as the default in keymap
            // does not contain the arrow keys, so we don't clear existing keybindings
            // for them.
            // If the keymap contains a binding for this action, erase all the
            // previously (default!) existing bindings, to only keep the bindings,
            // the user is used to
            action_contexts[action_id].clear();
            touched.insert( a.second );
        }
        add_input_for_action( action_id, context, input_event( a.first, input_event_t::keyboard_char ) );
    }
    // Unmap actions that are explicitly not mapped
    for( const action_id &elem : unbound_keymap ) {
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

static constexpr int current_keybinding_version = 2;

void input_manager::load( const cata_path &file_name, bool is_user_preferences )
{
    std::optional<JsonValue> jsin_opt = json_loader::from_path_opt( file_name );

    if( !jsin_opt.has_value() ) {
        // Only throw if this is the first file to load, that file _must_ exist,
        // otherwise the keybindings can not be read at all.
        if( action_contexts.empty() ) {
            throw std::runtime_error( std::string( "Could not read " ) + file_name.generic_u8string() );
        }
        return;
    }

    JsonArray actions_json = *jsin_opt;

    //Crawl through once and create an entry for every definition
    for( JsonObject action : actions_json ) {
        // JSON object representing the action

        int version = current_keybinding_version;
        if( is_user_preferences ) {
            // if there isn't a "version" value it means the object was written before
            // introduction of keybinding version, which is denoted by version 0.
            version = action.get_int( "version", 0 );
        }

        const std::string type = action.get_string( "type", "keybinding" );
        if( type != "keybinding" ) {
            debugmsg( "Only objects of type 'keybinding' (not %s) should appear in the "
                      "keybindings file '%s'", type, file_name.generic_u8string() );
            continue;
        }

        const std::string action_id = action.get_string( "id" );
        const std::string context = action.get_string( "category", default_context_id );
        t_actions &actions = action_contexts[context];
        if( action.has_member( "name" ) ) {
            action.read( "name", actions[action_id].name );
        }

        t_input_event_list events;
        for( const JsonObject keybinding : action.get_array( "bindings" ) ) {
            std::string input_method = keybinding.get_string( "input_method" );
            std::vector<input_event> new_events( 1 );
            if( input_method == "keyboard_any" ) {
                new_events.resize( 2 );
                new_events[0].type = input_event_t::keyboard_char;
                new_events[1].type = input_event_t::keyboard_code;
            } else if( input_method == "keyboard_char" || input_method == "keyboard" ) {
                new_events[0].type = input_event_t::keyboard_char;
            } else if( input_method == "keyboard_code" ) {
                new_events[0].type = input_event_t::keyboard_code;
            } else if( input_method == "gamepad" ) {
                new_events[0].type = input_event_t::gamepad;
            } else if( input_method == "mouse" ) {
                new_events[0].type = input_event_t::mouse;
            } else {
                keybinding.throw_error_at( "input_method", "unknown input_method" );
            }

            if( keybinding.has_member( "mod" ) ) {
                for( const JsonValue val : keybinding.get_array( "mod" ) ) {
                    const std::string str = val;
                    keymod_t mod = keymod_t::ctrl;
                    if( str == "ctrl" ) {
                        mod = keymod_t::ctrl;
                    } else if( str == "alt" ) {
                        mod = keymod_t::alt;
                    } else if( str == "shift" ) {
                        mod = keymod_t::shift;
                    } else {
                        val.throw_error( "unknown modifier name" );
                    }
                    for( input_event &new_event : new_events ) {
                        new_event.modifiers.emplace( mod );
                    }
                }
            }

            if( keybinding.has_array( "key" ) ) {
                for( const std::string line : keybinding.get_array( "key" ) ) {
                    for( input_event &new_event : new_events ) {
                        new_event.sequence.push_back( get_keycode( new_event.type, line ) );
                    }
                }
            } else { // assume string if not array, and throw if not string
                for( input_event &new_event : new_events ) {
                    new_event.sequence.push_back(
                        get_keycode( new_event.type, keybinding.get_string( "key" ) )
                    );
                }
            }

            for( const input_event &evt : new_events ) {
                if( std::find( events.begin(), events.end(), evt ) == events.end() ) {
                    events.emplace_back( evt );
                }
            }

            if( is_user_preferences && version <= 1 ) {
                // Add keypad enter to old keybindings with return key
                for( const input_event &evt : new_events ) {
                    input_event new_evt = evt;
                    bool has_return = false;
                    // As of version 2 the key sequence actually only supports
                    // one key, so we just replace all return with enter
                    if( new_evt.type == input_event_t::keyboard_char ) {
                        for( int &key : new_evt.sequence ) {
                            if( key == '\n' ) {
                                key = KEY_ENTER;
                                has_return = true;
                            }
                        }
                    }
                    if( has_return && std::find( events.begin(), events.end(), new_evt ) == events.end() ) {
                        events.emplace_back( new_evt );
                    }
                }
            }
        }

        // In case this is the second file containing user preferences,
        // this replaces the default bindings with the user's preferences.
        action_attributes &attributes = actions[action_id];
        if( is_user_preferences && version == 0 ) {
            // version 0 means the keybinding was written prior to the division
            // of `input_event_t::keyboard_char` and `input_event_t::keyboard_code`,
            // so we copy any `input_event_t::keyboard_code` event from the default
            // keybindings to be compatible with old user keybinding files.
            for( const input_event &evt : attributes.input_events ) {
                if( evt.type == input_event_t::keyboard_code ) {
                    events.emplace_back( evt );
                }
            }
        }
        attributes.input_events = events;
        if( action.has_member( "is_user_created" ) ) {
            attributes.is_user_created = action.get_bool( "is_user_created" );
        }
    }
}

void input_manager::save()
{
    write_to_file( PATH_INFO::user_keybindings(), [&]( std::ostream & data_file ) {
        JsonOut jsout( data_file, true );

        jsout.start_array();
        for( const auto &a : action_contexts ) {
            const t_actions &actions = a.second;
            for( const auto &action : actions ) {
                const t_input_event_list &events = action.second.input_events;
                jsout.start_object();

                jsout.member( "id", action.first );
                jsout.member( "version", current_keybinding_version );
                jsout.member( "category", a.first );
                bool is_user_created = action.second.is_user_created;
                if( is_user_created ) {
                    jsout.member( "is_user_created", is_user_created );
                }

                // optimization: no writing if empty
                if( !events.empty() ) {
                    jsout.member( "bindings" );
                    jsout.start_array();
                    for( const input_event &event : events ) {
                        jsout.start_object();
                        switch( event.type ) {
                            case input_event_t::keyboard_char:
                                jsout.member( "input_method", "keyboard_char" );
                                break;
                            case input_event_t::keyboard_code:
                                jsout.member( "input_method", "keyboard_code" );
                                break;
                            case input_event_t::gamepad:
                                jsout.member( "input_method", "gamepad" );
                                break;
                            case input_event_t::mouse:
                                jsout.member( "input_method", "mouse" );
                                break;
                            default:
                                throw std::runtime_error( "unknown input_event_t" );
                        }

                        // optimization: no writing if empty
                        if( !event.modifiers.empty() ) {
                            jsout.member( "mod" );
                            jsout.start_array();
                            for( const keymod_t mod : event.modifiers ) {
                                switch( mod ) {
                                    case keymod_t::ctrl:
                                        jsout.write( "ctrl" );
                                        break;
                                    case keymod_t::alt:
                                        jsout.write( "alt" );
                                        break;
                                    case keymod_t::shift:
                                        jsout.write( "shift" );
                                        break;
                                    default:
                                        throw std::runtime_error( "unknown keymod_t" );
                                }
                            }
                            jsout.end_array();
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
                }

                jsout.end_object();
            }
        }
        jsout.end_array();
    }, _( "key bindings configuration" ) );
}

void input_manager::add_keyboard_char_keycode_pair( int ch, const std::string &name )
{
    keyboard_char_keycode_to_keyname[ch] = name;
    keyboard_char_keyname_to_keycode[name] = ch;
}

void input_manager::add_keyboard_code_keycode_pair( const int ch, const std::string &name )
{
    keyboard_code_keycode_to_keyname[ch] = name;
    keyboard_code_keyname_to_keycode[name] = ch;
}

void input_manager::add_gamepad_keycode_pair( int ch, const std::string &name )
{
    gamepad_keycode_to_keyname[ch] = name;
    gamepad_keyname_to_keycode[name] = ch;
}

void input_manager::add_mouse_keycode_pair( const MouseInput mouse_input, const std::string &name )
{
    mouse_keycode_to_keyname[static_cast<int>( mouse_input )] = name;
    mouse_keyname_to_keycode[name] = static_cast<int>( mouse_input );
}

static constexpr int char_key_beg = ' ';
static constexpr int char_key_end = '~';

void input_manager::init_keycode_mapping()
{
    // Between space and tilde, all keys more or less map
    // to themselves(see ASCII table)
    for( char c = char_key_beg; c <= char_key_end; c++ ) {
        std::string name( 1, c );
        add_keyboard_char_keycode_pair( c, name );
        add_keyboard_code_keycode_pair( c, name );
    }

    add_keyboard_char_keycode_pair( '\t',          translate_marker_context( "key name", "TAB" ) );
    add_keyboard_char_keycode_pair( KEY_BTAB,      translate_marker_context( "key name", "BACKTAB" ) );
    add_keyboard_char_keycode_pair( ' ',           translate_marker_context( "key name", "SPACE" ) );
    add_keyboard_char_keycode_pair( KEY_UP,        translate_marker_context( "key name", "UP" ) );
    add_keyboard_char_keycode_pair( KEY_DOWN,      translate_marker_context( "key name", "DOWN" ) );
    add_keyboard_char_keycode_pair( KEY_LEFT,      translate_marker_context( "key name", "LEFT" ) );
    add_keyboard_char_keycode_pair( KEY_RIGHT,     translate_marker_context( "key name", "RIGHT" ) );
    add_keyboard_char_keycode_pair( KEY_NPAGE,     translate_marker_context( "key name", "NPAGE" ) );
    add_keyboard_char_keycode_pair( KEY_PPAGE,     translate_marker_context( "key name", "PPAGE" ) );
    add_keyboard_char_keycode_pair( KEY_ESCAPE,    translate_marker_context( "key name", "ESC" ) );
    add_keyboard_char_keycode_pair( KEY_BACKSPACE,
                                    translate_marker_context( "key name", "BACKSPACE" ) );
    add_keyboard_char_keycode_pair( KEY_DC,        translate_marker_context( "key name", "DELETE" ) );
    add_keyboard_char_keycode_pair( KEY_HOME,      translate_marker_context( "key name", "HOME" ) );
    add_keyboard_char_keycode_pair( KEY_BREAK,     translate_marker_context( "key name", "BREAK" ) );
    add_keyboard_char_keycode_pair( KEY_END,       translate_marker_context( "key name", "END" ) );
    add_keyboard_char_keycode_pair( '\n',          translate_marker_context( "key name", "RETURN" ) );
    add_keyboard_char_keycode_pair( KEY_ENTER,
                                    translate_marker_context( "key name", "KEYPAD_ENTER" ) );

    for( int c = 0; IS_CTRL_CHAR( c ); c++ ) {
        // Some codes fall into this range but have more common names we'd prefer to use.
        if( !IS_NAMED_CTRL_CHAR( c ) ) {
            // These are directly translated in `get_keyname()`
            // NOLINTNEXTLINE(cata-translate-string-literal)
            add_keyboard_char_keycode_pair( c, string_format( "CTRL+%c", c + 64 ) );
        }
    }

    // function keys, as defined by ncurses
    for( int i = F_KEY_NUM_BEG; i <= F_KEY_NUM_END; i++ ) {
        // These are directly translated in `get_keyname()`
        // NOLINTNEXTLINE(cata-translate-string-literal)
        add_keyboard_char_keycode_pair( KEY_F( i ), string_format( "F%d", i ) );
    }

    static const std::vector<std::pair<int, std::string>> keyboard_code_keycode_pair = {
        { keycode::backspace, translate_marker_context( "key name", "BACKSPACE" ) },
        { keycode::tab,       translate_marker_context( "key name", "TAB" ) },
        { keycode::return_,   translate_marker_context( "key name", "RETURN" ) },
        { keycode::escape,    translate_marker_context( "key name", "ESC" ) },
        { keycode::space,     translate_marker_context( "key name", "SPACE" ) },
        { keycode::f1,        translate_marker_context( "key name", "F1" ) },
        { keycode::f2,        translate_marker_context( "key name", "F2" ) },
        { keycode::f3,        translate_marker_context( "key name", "F3" ) },
        { keycode::f4,        translate_marker_context( "key name", "F4" ) },
        { keycode::f5,        translate_marker_context( "key name", "F5" ) },
        { keycode::f6,        translate_marker_context( "key name", "F6" ) },
        { keycode::f7,        translate_marker_context( "key name", "F7" ) },
        { keycode::f8,        translate_marker_context( "key name", "F8" ) },
        { keycode::f9,        translate_marker_context( "key name", "F9" ) },
        { keycode::f10,       translate_marker_context( "key name", "F10" ) },
        { keycode::f11,       translate_marker_context( "key name", "F11" ) },
        { keycode::f12,       translate_marker_context( "key name", "F12" ) },
        { keycode::ppage,     translate_marker_context( "key name", "PPAGE" ) },
        { keycode::home,      translate_marker_context( "key name", "HOME" ) },
        { keycode::end,       translate_marker_context( "key name", "END" ) },
        { keycode::npage,     translate_marker_context( "key name", "NPAGE" ) },
        { keycode::right,     translate_marker_context( "key name", "RIGHT" ) },
        { keycode::left,      translate_marker_context( "key name", "LEFT" ) },
        { keycode::down,      translate_marker_context( "key name", "DOWN" ) },
        { keycode::up,        translate_marker_context( "key name", "UP" ) },
        { keycode::kp_divide, translate_marker_context( "key name", "KEYPAD_DIVIDE" ) },
        { keycode::kp_multiply, translate_marker_context( "key name", "KEYPAD_MULTIPLY" ) },
        { keycode::kp_minus,  translate_marker_context( "key name", "KEYPAD_MINUS" ) },
        { keycode::kp_plus,   translate_marker_context( "key name", "KEYPAD_PLUS" ) },
        { keycode::kp_enter,  translate_marker_context( "key name", "KEYPAD_ENTER" ) },
        { keycode::kp_1,      translate_marker_context( "key name", "KEYPAD_1" ) },
        { keycode::kp_2,      translate_marker_context( "key name", "KEYPAD_2" ) },
        { keycode::kp_3,      translate_marker_context( "key name", "KEYPAD_3" ) },
        { keycode::kp_4,      translate_marker_context( "key name", "KEYPAD_4" ) },
        { keycode::kp_5,      translate_marker_context( "key name", "KEYPAD_5" ) },
        { keycode::kp_6,      translate_marker_context( "key name", "KEYPAD_6" ) },
        { keycode::kp_7,      translate_marker_context( "key name", "KEYPAD_7" ) },
        { keycode::kp_8,      translate_marker_context( "key name", "KEYPAD_8" ) },
        { keycode::kp_9,      translate_marker_context( "key name", "KEYPAD_9" ) },
        { keycode::kp_0,      translate_marker_context( "key name", "KEYPAD_0" ) },
        { keycode::kp_period, translate_marker_context( "key name", "KEYPAD_PERIOD" ) },
        { keycode::f13,       translate_marker_context( "key name", "F13" ) },
        { keycode::f14,       translate_marker_context( "key name", "F14" ) },
        { keycode::f15,       translate_marker_context( "key name", "F15" ) },
        { keycode::f16,       translate_marker_context( "key name", "F16" ) },
        { keycode::f17,       translate_marker_context( "key name", "F17" ) },
        { keycode::f18,       translate_marker_context( "key name", "F18" ) },
        { keycode::f19,       translate_marker_context( "key name", "F19" ) },
        { keycode::f20,       translate_marker_context( "key name", "F20" ) },
        { keycode::f21,       translate_marker_context( "key name", "F21" ) },
        { keycode::f22,       translate_marker_context( "key name", "F22" ) },
        { keycode::f23,       translate_marker_context( "key name", "F23" ) },
        { keycode::f24,       translate_marker_context( "key name", "F24" ) },
    };
    for( const auto &v : keyboard_code_keycode_pair ) {
        add_keyboard_code_keycode_pair( v.first, v.second );
    }

    add_gamepad_keycode_pair( JOY_LEFT,      translate_marker_context( "key name", "JOY_LEFT" ) );
    add_gamepad_keycode_pair( JOY_RIGHT,     translate_marker_context( "key name", "JOY_RIGHT" ) );
    add_gamepad_keycode_pair( JOY_UP,        translate_marker_context( "key name", "JOY_UP" ) );
    add_gamepad_keycode_pair( JOY_DOWN,      translate_marker_context( "key name", "JOY_DOWN" ) );
    add_gamepad_keycode_pair( JOY_LEFTUP,    translate_marker_context( "key name", "JOY_LEFTUP" ) );
    add_gamepad_keycode_pair( JOY_LEFTDOWN,  translate_marker_context( "key name", "JOY_LEFTDOWN" ) );
    add_gamepad_keycode_pair( JOY_RIGHTUP,   translate_marker_context( "key name", "JOY_RIGHTUP" ) );
    add_gamepad_keycode_pair( JOY_RIGHTDOWN, translate_marker_context( "key name", "JOY_RIGHTDOWN" ) );

    add_gamepad_keycode_pair( JOY_0,         translate_marker_context( "key name", "JOY_0" ) );
    add_gamepad_keycode_pair( JOY_1,         translate_marker_context( "key name", "JOY_1" ) );
    add_gamepad_keycode_pair( JOY_2,         translate_marker_context( "key name", "JOY_2" ) );
    add_gamepad_keycode_pair( JOY_3,         translate_marker_context( "key name", "JOY_3" ) );
    add_gamepad_keycode_pair( JOY_4,         translate_marker_context( "key name", "JOY_4" ) );
    add_gamepad_keycode_pair( JOY_5,         translate_marker_context( "key name", "JOY_5" ) );
    add_gamepad_keycode_pair( JOY_6,         translate_marker_context( "key name", "JOY_6" ) );
    add_gamepad_keycode_pair( JOY_7,         translate_marker_context( "key name", "JOY_7" ) );
    add_gamepad_keycode_pair( JOY_8,         translate_marker_context( "key name", "JOY_8" ) );
    add_gamepad_keycode_pair( JOY_9,         translate_marker_context( "key name", "JOY_9" ) );
    add_gamepad_keycode_pair( JOY_10,        translate_marker_context( "key name", "JOY_10" ) );
    add_gamepad_keycode_pair( JOY_11,        translate_marker_context( "key name", "JOY_11" ) );
    add_gamepad_keycode_pair( JOY_12,        translate_marker_context( "key name", "JOY_12" ) );
    add_gamepad_keycode_pair( JOY_13,        translate_marker_context( "key name", "JOY_13" ) );
    add_gamepad_keycode_pair( JOY_14,        translate_marker_context( "key name", "JOY_14" ) );
    add_gamepad_keycode_pair( JOY_15,        translate_marker_context( "key name", "JOY_15" ) );
    add_gamepad_keycode_pair( JOY_16,        translate_marker_context( "key name", "JOY_16" ) );
    add_gamepad_keycode_pair( JOY_17,        translate_marker_context( "key name", "JOY_17" ) );
    add_gamepad_keycode_pair( JOY_18,        translate_marker_context( "key name", "JOY_18" ) );
    add_gamepad_keycode_pair( JOY_19,        translate_marker_context( "key name", "JOY_19" ) );
    add_gamepad_keycode_pair( JOY_20,        translate_marker_context( "key name", "JOY_20" ) );
    add_gamepad_keycode_pair( JOY_21,        translate_marker_context( "key name", "JOY_21" ) );
    add_gamepad_keycode_pair( JOY_22,        translate_marker_context( "key name", "JOY_22" ) );
    add_gamepad_keycode_pair( JOY_23,        translate_marker_context( "key name", "JOY_23" ) );
    add_gamepad_keycode_pair( JOY_24,        translate_marker_context( "key name", "JOY_24" ) );
    add_gamepad_keycode_pair( JOY_25,        translate_marker_context( "key name", "JOY_25" ) );
    add_gamepad_keycode_pair( JOY_26,        translate_marker_context( "key name", "JOY_26" ) );
    add_gamepad_keycode_pair( JOY_27,        translate_marker_context( "key name", "JOY_27" ) );
    add_gamepad_keycode_pair( JOY_28,        translate_marker_context( "key name", "JOY_28" ) );
    add_gamepad_keycode_pair( JOY_29,        translate_marker_context( "key name", "JOY_29" ) );
    add_gamepad_keycode_pair( JOY_30,        translate_marker_context( "key name", "JOY_30" ) );

    add_mouse_keycode_pair( MouseInput::LeftButtonPressed,
                            translate_marker_context( "key name", "MOUSE_LEFT_PRESSED" ) );
    add_mouse_keycode_pair( MouseInput::LeftButtonReleased,
                            translate_marker_context( "key name", "MOUSE_LEFT" ) );
    add_mouse_keycode_pair( MouseInput::RightButtonPressed,
                            translate_marker_context( "key name", "MOUSE_RIGHT_PRESSED" ) );
    add_mouse_keycode_pair( MouseInput::RightButtonReleased,
                            translate_marker_context( "key name", "MOUSE_RIGHT" ) );
    add_mouse_keycode_pair( MouseInput::ScrollWheelUp,
                            translate_marker_context( "key name", "SCROLL_UP" ) );
    add_mouse_keycode_pair( MouseInput::ScrollWheelDown,
                            translate_marker_context( "key name", "SCROLL_DOWN" ) );
    add_mouse_keycode_pair( MouseInput::Move,
                            translate_marker_context( "key name", "MOUSE_MOVE" ) );
    add_mouse_keycode_pair( MouseInput::X1ButtonPressed,
                            translate_marker_context( "key name", "MOUSE_BACK_PRESSED" ) );
    add_mouse_keycode_pair( MouseInput::X1ButtonReleased,
                            translate_marker_context( "key name", "MOUSE_BACK" ) );
    add_mouse_keycode_pair( MouseInput::X2ButtonPressed,
                            translate_marker_context( "key name", "MOUSE_FORWARD_PRESSED" ) );
    add_mouse_keycode_pair( MouseInput::X2ButtonReleased,
                            translate_marker_context( "key name", "MOUSE_FORWARD" ) );

}

int input_manager::get_keycode( const input_event_t inp_type, const std::string &name ) const
{
    const t_name_to_key_map *map = nullptr;
    switch( inp_type ) {
        default:
            break;
        case input_event_t::keyboard_char:
            map = &keyboard_char_keyname_to_keycode;
            break;
        case input_event_t::keyboard_code:
            map = &keyboard_code_keyname_to_keycode;
            break;
        case input_event_t::gamepad:
            map = &gamepad_keyname_to_keycode;
            break;
        case input_event_t::mouse:
            map = &mouse_keyname_to_keycode;
            break;
    }
    if( map ) {
        const auto it = map->find( name );
        if( it != map->end() ) {
            return it->second;
        }
    }
    // Not found in map, try to parse as int
    if( name.compare( 0, 8, "UNKNOWN_" ) == 0 ) {
        return str_to_int( name.substr( 8 ) );
    }
    return 0;
}

std::string input_manager::get_keyname( int ch, input_event_t inp_type, bool portable ) const
{
    const t_key_to_name_map *map = nullptr;
    switch( inp_type ) {
        default:
            break;
        case input_event_t::keyboard_char:
            map = &keyboard_char_keycode_to_keyname;
            break;
        case input_event_t::keyboard_code:
            map = &keyboard_code_keycode_to_keyname;
            break;
        case input_event_t::gamepad:
            map = &gamepad_keycode_to_keyname;
            break;
        case input_event_t::mouse:
            map = &mouse_keycode_to_keyname;
            break;
    }
    if( map ) {
        const auto it = map->find( ch );
        if( it != map->end() ) {
            switch( inp_type ) {
                case input_event_t::keyboard_char:
                    if( IS_F_KEY( ch ) ) {
                        // special case it since F<num> key names are generated using loop
                        // and not marked individually for translation
                        if( portable ) {
                            return it->second;
                        } else {
                            return string_format( pgettext( "function key name", "F%d" ), F_KEY_NUM( ch ) );
                        }
                    } else if( IS_CTRL_CHAR( ch ) && !IS_NAMED_CTRL_CHAR( ch ) ) {
                        if( portable ) {
                            return it->second;
                        } else {
                            return string_format( pgettext( "control key name", "CTRL+%c" ), ch + 64 );
                        }
                    } else if( ch >= char_key_beg && ch <= char_key_end && ch != ' ' ) {
                        // character keys except space need no translation
                        return it->second;
                    }
                    break;
                case input_event_t::keyboard_code:
                    if( ch >= char_key_beg && ch < char_key_end && ch != ' ' ) {
                        // character keys except space need no translation
                        return it->second;
                    }
                    break;
                default:
                    break;
            }
            return portable ? it->second : pgettext( "key name", it->second.c_str() );
        }
    }
    if( portable ) {
        return std::string( "UNKNOWN_" ) + int_to_str( ch );
    } else {
        return string_format( _( "unknown key %ld" ), ch );
    }
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
    bool *overwrites_default,
    bool use_basic_action_contexts )
{
    t_action_contexts &action_contexts_ref = use_basic_action_contexts
            ? basic_action_contexts
            : action_contexts;

    if( context != default_context_id ) {
        // Check if the action exists in the provided context
        const t_action_contexts::const_iterator action_context = action_contexts_ref.find( context );
        if( action_context != action_contexts_ref.end() ) {
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

    t_actions &default_action_context = action_contexts_ref[default_context_id];
    const t_actions::const_iterator default_action = default_action_context.find( action_id );
    if( default_action == default_action_context.end() ) {
        // A new action is created in the event that the requested action is
        // not in the keybindings configuration e.g. the entry is missing.
        default_action_context[action_id].name = get_default_action_name( action_id );
    }

    return default_action_context[action_id];
}

translation input_manager::get_default_action_name( const std::string &action_id ) const
{
    const t_action_contexts::const_iterator default_action_context = action_contexts.find(
                default_context_id );
    if( default_action_context == action_contexts.end() ) {
        return no_translation( action_id );
    }

    const t_actions::const_iterator default_action = default_action_context->second.find( action_id );
    if( default_action != default_action_context->second.end() ) {
        return default_action->second.name;
    } else {
        return no_translation( action_id );
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

bool input_manager::remove_input_for_action(
    const std::string &action_descriptor, const std::string &context )
{
    const t_action_contexts::iterator action_context = action_contexts.find( context );
    if( action_context == action_contexts.end() ) {
        return false;
    }
    t_actions &actions = action_context->second;
    t_actions::iterator action = actions.find( action_descriptor );
    if( action == actions.end() ) {
        return false;
    }
    if( action->second.is_user_created ) {
        // Since this is a user created hotkey, remove it so that the
        // user will fallback to the hotkey in the default context.
        actions.erase( action );
        return true;
    }
    // If a context no longer has any keybindings remaining for an action but
    // there's an attempt to remove bindings anyway, presumably the user wants
    // to fully remove the binding from that context.
    if( action->second.input_events.empty() ) {
        actions.erase( action );
        return true;
    }
    action->second.input_events.clear();
    return false;
}

void input_manager::add_input_for_action(
    const std::string &action_descriptor, const std::string &context, const input_event &event )
{
    t_input_event_list &events = get_or_create_event_list( action_descriptor, context );
    for( input_event &events_a : events ) {
        if( events_a == event ) {
            return;
        }
    }
    events.push_back( event );
}

int input_manager::get_previously_pressed_key() const
{
    return previously_pressed_key;
}

void input_manager::wait_for_any_key()
{
#if defined(__ANDROID__)
    input_context ctxt( "WAIT_FOR_ANY_KEY", keyboard_mode::keycode );
#endif
    while( true ) {
        const input_event evt = inp_mngr.get_input_event();
        switch( evt.type ) {
            case input_event_t::keyboard_char:
                if( !evt.sequence.empty() ) {
                    return;
                }
                break;
            case input_event_t::keyboard_code:
            // errors are accepted as well to avoid an infinite loop
            case input_event_t::error:
                return;
            default:
                break;
        }
    }
}

keyboard_mode input_manager::actual_keyboard_mode( const keyboard_mode preferred_keyboard_mode )
{
    switch( preferred_keyboard_mode ) {
        case keyboard_mode::keycode:
            return is_keycode_mode_supported() ? keyboard_mode::keycode : keyboard_mode::keychar;
        case keyboard_mode::keychar:
            return keyboard_mode::keychar;
    }
    return keyboard_mode::keychar;
}
