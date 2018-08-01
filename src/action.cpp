#include "action.h"
#include "output.h"
#include "options.h"
#include "path_info.h"
#include "debug.h"
#include "game.h"
#include "map.h"
#include "iexamine.h"
#include "player.h"
#include "options.h"
#include "messages.h"
#include "translations.h"
#include "input.h"
#include "crafting.h"
#include "map_iterator.h"
#include "ui.h"
#include "trap.h"
#include "itype.h"
#include "mapdata.h"
#include "cata_utility.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "optional.h"

#include <istream>
#include <sstream>
#include <iterator>
#include <algorithm>

extern bool tile_iso;

void parse_keymap( std::istream &keymap_txt, std::map<char, action_id> &kmap,
                   std::set<action_id> &unbound_keymap );

void load_keyboard_settings( std::map<char, action_id> &keymap,
                             std::string &keymap_file_loaded_from,
                             std::set<action_id> &unbound_keymap )
{
    const auto parser = [&]( std::istream & fin ) {
        parse_keymap( fin, keymap, unbound_keymap );
    };
    if( read_from_file_optional( FILENAMES["keymap"], parser ) ) {
        keymap_file_loaded_from = FILENAMES["keymap"];
    } else if( read_from_file_optional( FILENAMES["legacy_keymap"], parser ) ) {
        keymap_file_loaded_from = FILENAMES["legacy_keymap"];
    }
}

void parse_keymap( std::istream &keymap_txt, std::map<char, action_id> &kmap,
                   std::set<action_id> &unbound_keymap )
{
    while( !keymap_txt.eof() ) {
        std::string id;
        keymap_txt >> id;
        if( id.empty() ) {
            getline( keymap_txt, id );  // Empty line, chomp it
        } else if( id == "unbind" ) {
            keymap_txt >> id;
            action_id act = look_up_action( id );
            if( act != ACTION_NULL ) {
                unbound_keymap.insert( act );
            }
            break;
        } else if( id[0] != '#' ) {
            action_id act = look_up_action( id );
            if( act == ACTION_NULL )
                debugmsg( "\
Warning! keymap.txt contains an unknown action, \"%s\"\n\
Fix \"%s\" at your next chance!", id.c_str(), FILENAMES["keymap"].c_str() );
            else {
                while( !keymap_txt.eof() ) {
                    char ch;
                    keymap_txt >> std::noskipws >> ch >> std::skipws;
                    if( ch == '\n' ) {
                        break;
                    } else if( ch != ' ' || keymap_txt.peek() == '\n' ) {
                        if( kmap.find( ch ) != kmap.end() ) {
                            debugmsg( "\
Warning!  '%c' assigned twice in the keymap!\n\
%s is being ignored.\n\
Fix \"%s\" at your next chance!", ch, id.c_str(), FILENAMES["keymap"].c_str() );
                        } else {
                            kmap[ ch ] = act;
                        }
                    }
                }
            }
        } else {
            getline( keymap_txt, id ); // Clear the whole line
        }
    }
}

std::vector<char> keys_bound_to( action_id act )
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.keys_bound_to( action_ident( act ) );
}

action_id action_from_key( char ch )
{
    input_context ctxt = get_default_mode_input_context();
    input_event event( ( long ) ch, CATA_INPUT_KEYBOARD );
    const std::string action = ctxt.input_to_action( event );
    return look_up_action( action );
}

std::string action_ident( action_id act )
{
    switch( act ) {
        case ACTION_PAUSE:
            return "pause";
        case ACTION_TIMEOUT:
            return "TIMEOUT";
        case ACTION_MOVE_N:
            return "UP";
        case ACTION_MOVE_NE:
            return "RIGHTUP";
        case ACTION_MOVE_E:
            return "RIGHT";
        case ACTION_MOVE_SE:
            return "RIGHTDOWN";
        case ACTION_MOVE_S:
            return "DOWN";
        case ACTION_MOVE_SW:
            return "LEFTDOWN";
        case ACTION_MOVE_W:
            return "LEFT";
        case ACTION_MOVE_NW:
            return "LEFTUP";
        case ACTION_MOVE_DOWN:
            return "LEVEL_DOWN";
        case ACTION_MOVE_UP:
            return "LEVEL_UP";
        case ACTION_CENTER:
            return "center";
        case ACTION_SHIFT_N:
            return "shift_n";
        case ACTION_SHIFT_NE:
            return "shift_ne";
        case ACTION_SHIFT_E:
            return "shift_e";
        case ACTION_SHIFT_SE:
            return "shift_se";
        case ACTION_SHIFT_S:
            return "shift_s";
        case ACTION_SHIFT_SW:
            return "shift_sw";
        case ACTION_SHIFT_W:
            return "shift_w";
        case ACTION_SHIFT_NW:
            return "shift_nw";
        case ACTION_TOGGLE_MOVE:
            return "toggle_move";
        case ACTION_OPEN:
            return "open";
        case ACTION_CLOSE:
            return "close";
        case ACTION_SMASH:
            return "smash";
        case ACTION_EXAMINE:
            return "examine";
        case ACTION_ADVANCEDINV:
            return "advinv";
        case ACTION_PICKUP:
            return "pickup";
        case ACTION_GRAB:
            return "grab";
        case ACTION_BUTCHER:
            return "butcher";
        case ACTION_CHAT:
            return "chat";
        case ACTION_LOOK:
            return "look";
        case ACTION_PEEK:
            return "peek";
        case ACTION_LIST_ITEMS:
            return "listitems";
        case ACTION_ZONES:
            return "zones";
        case ACTION_INVENTORY:
            return "inventory";
        case ACTION_COMPARE:
            return "compare";
        case ACTION_ORGANIZE:
            return "organize";
        case ACTION_USE:
            return "apply";
        case ACTION_USE_WIELDED:
            return "apply_wielded";
        case ACTION_WEAR:
            return "wear";
        case ACTION_TAKE_OFF:
            return "take_off";
        case ACTION_EAT:
            return "eat";
        case ACTION_READ:
            return "read";
        case ACTION_WIELD:
            return "wield";
        case ACTION_PICK_STYLE:
            return "pick_style";
        case ACTION_RELOAD:
            return "reload";
        case ACTION_UNLOAD:
            return "unload";
        case ACTION_MEND:
            return "mend";
        case ACTION_THROW:
            return "throw";
        case ACTION_FIRE:
            return "fire";
        case ACTION_FIRE_BURST:
            return "fire_burst";
        case ACTION_SELECT_FIRE_MODE:
            return "select_fire_mode";
        case ACTION_DROP:
            return "drop";
        case ACTION_DIR_DROP:
            return "drop_adj";
        case ACTION_BIONICS:
            return "bionics";
        case ACTION_MUTATIONS:
            return "mutations";
        case ACTION_SORT_ARMOR:
            return "sort_armor";
        case ACTION_WAIT:
            return "wait";
        case ACTION_CRAFT:
            return "craft";
        case ACTION_RECRAFT:
            return "recraft";
        case ACTION_LONGCRAFT:
            return "long_craft";
        case ACTION_CONSTRUCT:
            return "construct";
        case ACTION_DISASSEMBLE:
            return "disassemble";
        case ACTION_SLEEP:
            return "sleep";
        case ACTION_CONTROL_VEHICLE:
            return "control_vehicle";
        case ACTION_TOGGLE_SAFEMODE:
            return "safemode";
        case ACTION_TOGGLE_AUTOSAFE:
            return "autosafe";
        case ACTION_IGNORE_ENEMY:
            return "ignore_enemy";
        case ACTION_WHITELIST_ENEMY:
            return "whitelist_enemy";
        case ACTION_SAVE:
            return "save";
        case ACTION_QUICKSAVE:
            return "quicksave";
        case ACTION_QUICKLOAD:
            return "quickload";
        case ACTION_QUIT:
            return "quit";
        case ACTION_PL_INFO:
            return "player_data";
        case ACTION_MAP:
            return "map";
        case ACTION_MISSIONS:
            return "missions";
        case ACTION_FACTIONS:
            return "factions";
        case ACTION_KILLS:
            return "kills";
        case ACTION_MORALE:
            return "morale";
        case ACTION_MESSAGES:
            return "messages";
        case ACTION_HELP:
            return "help";
        case ACTION_DEBUG:
            return "debug";
        case ACTION_DISPLAY_SCENT:
            return "debug_scent";
        case ACTION_TOGGLE_DEBUG_MODE:
            return "debug_mode";
        case ACTION_ZOOM_OUT:
            return "zoom_out";
        case ACTION_ZOOM_IN:
            return "zoom_in";
        case ACTION_TOGGLE_SIDEBAR_STYLE:
            return "toggle_sidebar_style";
        case ACTION_TOGGLE_FULLSCREEN:
            return "toggle_fullscreen";
        case ACTION_TOGGLE_PIXEL_MINIMAP:
            return "toggle_pixel_minimap";
        case ACTION_TOGGLE_AUTO_PULP_BUTCHER:
            return "toggle_auto_pulp_butcher";
        case ACTION_ACTIONMENU:
            return "action_menu";
        case ACTION_ITEMACTION:
            return "item_action_menu";
        case ACTION_SELECT:
            return "SELECT";
        case ACTION_SEC_SELECT:
            return "SEC_SELECT";
        case ACTION_AUTOATTACK:
            return "autoattack";
        case ACTION_MAIN_MENU:
            return "main_menu";
        case ACTION_KEYBINDINGS:
            return "open_keybindings";
        case ACTION_OPTIONS:
            return "open_options";
        case ACTION_AUTOPICKUP:
            return "open_autopickup";
        case ACTION_SAFEMODE:
            return "open_safemode";
        case ACTION_COLOR:
            return "open_color";
        case ACTION_WORLD_MODS:
            return "open_world_mods";
        case ACTION_NULL:
            return "null";
        default:
            return "unknown";
    }
}

bool can_action_change_worldstate( const action_id act )
{
    switch( act ) {
        // Shift view
        case ACTION_CENTER:
        case ACTION_SHIFT_N:
        case ACTION_SHIFT_NE:
        case ACTION_SHIFT_E:
        case ACTION_SHIFT_SE:
        case ACTION_SHIFT_S:
        case ACTION_SHIFT_SW:
        case ACTION_SHIFT_W:
        case ACTION_SHIFT_NW:
        // Environment Interaction
        case ACTION_LOOK:
        case ACTION_LIST_ITEMS:
        case ACTION_ZONES:
        // Long-term / special actions
        case ACTION_SAVE:
        case ACTION_QUICKSAVE:
        case ACTION_QUICKLOAD:
        case ACTION_QUIT:
        // Info Screens
        case ACTION_PL_INFO:
        case ACTION_MAP:
        case ACTION_MISSIONS:
        case ACTION_KILLS:
        case ACTION_FACTIONS:
        case ACTION_MORALE:
        case ACTION_MESSAGES:
        case ACTION_HELP:
        case ACTION_MAIN_MENU:
        case ACTION_KEYBINDINGS:
        case ACTION_OPTIONS:
        case ACTION_AUTOPICKUP:
        case ACTION_SAFEMODE:
        case ACTION_COLOR:
        case ACTION_WORLD_MODS:
        // Debug Functions
        case ACTION_TOGGLE_SIDEBAR_STYLE:
        case ACTION_TOGGLE_FULLSCREEN:
        case ACTION_DEBUG:
        case ACTION_DISPLAY_SCENT:
        case ACTION_ZOOM_OUT:
        case ACTION_ZOOM_IN:
        case ACTION_TOGGLE_PIXEL_MINIMAP:
        case ACTION_TIMEOUT:
        case ACTION_TOGGLE_AUTO_PULP_BUTCHER:
            return false;
        default:
            return true;
    }
}

action_id look_up_action( const std::string &ident )
{
    // Temporarily for the interface with the input manager!
    if( ident == "move_nw" ) {
        return ACTION_MOVE_NW;
    } else if( ident == "move_sw" ) {
        return ACTION_MOVE_SW;
    } else if( ident == "move_ne" ) {
        return ACTION_MOVE_NE;
    } else if( ident == "move_se" ) {
        return ACTION_MOVE_SE;
    } else if( ident == "move_n" ) {
        return ACTION_MOVE_N;
    } else if( ident == "move_s" ) {
        return ACTION_MOVE_S;
    } else if( ident == "move_w" ) {
        return ACTION_MOVE_W;
    } else if( ident == "move_e" ) {
        return ACTION_MOVE_E;
    } else if( ident == "move_down" ) {
        return ACTION_MOVE_DOWN;
    } else if( ident == "move_up" ) {
        return ACTION_MOVE_UP;
    }
    // ^^ Temporarily for the interface with the input manager!
    for( int i = 0; i < NUM_ACTIONS; i++ ) {
        if( action_ident( action_id( i ) ) == ident ) {
            return action_id( i );
        }
    }
    return ACTION_NULL;
}

// (Press X (or Y)|Try) to Z
std::string press_x( action_id act )
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.press_x( action_ident( act ), _( "Press " ), "", _( "Try" ) );
}
std::string press_x( action_id act, const std::string &key_bound, const std::string &key_unbound )
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.press_x( action_ident( act ), key_bound, "", key_unbound );
}
std::string press_x( action_id act, const std::string &key_bound_pre,
                     const std::string &key_bound_suf,
                     const std::string &key_unbound )
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.press_x( action_ident( act ), key_bound_pre, key_bound_suf, key_unbound );
}

action_id get_movement_direction_from_delta( const int dx, const int dy, const int dz )
{
    if( dz == -1 ) {
        return ACTION_MOVE_DOWN;
    } else if( dz == 1 ) {
        return ACTION_MOVE_UP;
    }

    if( dx == 0 && dy == -1 ) {
        return ACTION_MOVE_N;
    } else if( dx == 1 && dy == -1 ) {
        return ACTION_MOVE_NE;
    } else if( dx == 1 && dy == 0 ) {
        return ACTION_MOVE_E;
    } else if( dx == 1 && dy == 1 ) {
        return ACTION_MOVE_SE;
    } else if( dx == 0 && dy == 1 ) {
        return ACTION_MOVE_S;
    } else if( dx == -1 && dy == 1 ) {
        return ACTION_MOVE_SW;
    } else if( dx == -1 && dy == 0 ) {
        return ACTION_MOVE_W;
    } else {
        return ACTION_MOVE_NW;
    }
}

// Get the key for an action, used in the action menu to give each action the
// hotkey it is bound to.
long hotkey_for_action( action_id action )
{
    std::vector<char> keys = keys_bound_to( action );
    return keys.empty() ? -1 : keys[0];
}

bool can_butcher_at( const tripoint &p )
{
    // TODO: unify this with game::butcher
    const int factor = g->u.max_quality( quality_id( "BUTCHER" ) );
    auto items = g->m.i_at( p );
    bool has_item = false;
    bool has_corpse = false;

    const inventory &crafting_inv = g->u.crafting_inventory();
    for( auto &items_it : items ) {
        if( items_it.is_corpse() ) {
            if( factor != INT_MIN ) {
                has_corpse = true;
            }
        } else if( g->u.can_disassemble( items_it, crafting_inv ).success() ) {
            has_item = true;
        }
    }
    return has_corpse || has_item;
}

bool can_move_vertical_at( const tripoint &p, int movez )
{
    // TODO: unify this with game::move_vertical
    if( g->m.has_flag( "SWIMMABLE", p ) && g->m.has_flag( TFLAG_DEEP_WATER, p ) ) {
        if( movez == -1 ) {
            return !g->u.is_underwater() && !g->u.worn_with_flag( "FLOTATION" );
        } else {
            return g->u.swim_speed() < 500 || g->u.is_wearing( "swim_fins" );
        }
    }

    if( movez == -1 ) {
        return g->m.has_flag( "GOES_DOWN", p );
    } else {
        return g->m.has_flag( "GOES_UP", p );
    }
}

bool can_examine_at( const tripoint &p )
{
    if( g->m.veh_at( p ) ) {
        return true;
    }
    if( g->m.has_flag( "CONSOLE", p ) ) {
        return true;
    }
    if( g->m.has_items( p ) ) {
        return true;
    }
    const furn_t &xfurn_t = g->m.furn( p ).obj();
    const ter_t &xter_t = g->m.ter( p ).obj();

    if( g->m.has_furn( p ) && xfurn_t.examine != &iexamine::none ) {
        return true;
    } else if( xter_t.examine != &iexamine::none ) {
        return true;
    }

    const trap &tr = g->m.tr_at( p );
    if( tr.can_see( p, g->u ) ) {
        return true;
    }

    return false;
}

bool can_interact_at( action_id action, const tripoint &p )
{
    switch( action ) {
        case ACTION_OPEN:
            return g->m.open_door( p, !g->m.is_outside( g->u.pos() ), true );
            break;
        case ACTION_CLOSE: {
            const optional_vpart_position vp = g->m.veh_at( p );
            return ( vp &&
                     vp->vehicle().next_part_to_close( vp->part_index(),
                             veh_pointer_or_null( g->m.veh_at( g->u.pos() ) ) != &vp->vehicle() ) >= 0 ) ||
                   g->m.close_door( p, !g->m.is_outside( g->u.pos() ), true );
            break;
        }
        case ACTION_BUTCHER:
            return can_butcher_at( p );
        case ACTION_MOVE_UP:
            return can_move_vertical_at( p, 1 );
        case ACTION_MOVE_DOWN:
            return can_move_vertical_at( p, -1 );
            break;
        case ACTION_EXAMINE:
            return can_examine_at( p );
            break;
        default:
            return false;
            break;
    }
}

action_id handle_action_menu()
{
    const input_context ctxt = get_default_mode_input_context();
    std::string catgname;

#define REGISTER_ACTION(name) entries.emplace_back(uimenu_entry(name, true, hotkey_for_action(name), \
        ctxt.get_action_name(action_ident(name))));
#define REGISTER_CATEGORY(name)  categories_by_int[last_category] = name; \
    catgname = name; \
    catgname += "..."; \
    entries.emplace_back(uimenu_entry(last_category, true, -1, catgname)); \
    last_category++;

    // Calculate weightings for the various actions to give the player suggestions
    // Weight >= 200: Special action only available right now
    std::map<action_id, int> action_weightings;

    // Check if we're in a potential combat situation, if so, sort a few actions to the top.
    if( !g->u.get_hostile_creatures( 60 ).empty() ) {
        // Only prioritize movement options if we're not driving.
        if( !g->u.controlling_vehicle ) {
            action_weightings[ACTION_TOGGLE_MOVE] = 400;
        }
        // Only prioritize fire weapon options if we're wielding a ranged weapon.
        if( g->u.weapon.is_gun() || g->u.weapon.has_flag( "REACH_ATTACK" ) ) {
            action_weightings[ACTION_FIRE] = 350;
        }
    }

    // If we're already running, make it simple to toggle running to off.
    if( g->u.move_mode != "walk" ) {
        action_weightings[ACTION_TOGGLE_MOVE] = 300;
    }

    // Check if we're on a vehicle, if so, vehicle controls should be top.
    if( g->m.veh_at( g->u.pos() ) ) {
        // Make it 300 to prioritize it before examining the vehicle.
        action_weightings[ACTION_CONTROL_VEHICLE] = 300;
    }

    // Check if we can perform one of our actions on nearby terrain. If so,
    // display that action at the top of the list.
    for( const tripoint &pos : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        if( pos != g->u.pos() ) {
            // Check for actions that work on nearby tiles
            if( can_interact_at( ACTION_OPEN, pos ) ) {
                action_weightings[ACTION_OPEN] = 200;
            }
            if( can_interact_at( ACTION_CLOSE, pos ) ) {
                action_weightings[ACTION_CLOSE] = 200;
            }
            if( can_interact_at( ACTION_EXAMINE, pos ) ) {
                action_weightings[ACTION_EXAMINE] = 200;
            }
        } else {
            // Check for actions that work on own tile only
            if( can_interact_at( ACTION_BUTCHER, pos ) ) {
                action_weightings[ACTION_BUTCHER] = 200;
            }
            if( can_interact_at( ACTION_MOVE_UP, pos ) ) {
                action_weightings[ACTION_MOVE_UP] = 200;
            }
            if( can_interact_at( ACTION_MOVE_DOWN, pos ) ) {
                action_weightings[ACTION_MOVE_DOWN] = 200;
            }
        }
    }

    // sort the map by its weightings
    std::vector<std::pair<action_id, int> > sorted_pairs;
    std::copy( action_weightings.begin(), action_weightings.end(),
               std::back_inserter<std::vector<std::pair<action_id, int> > >( sorted_pairs ) );
    std::reverse( sorted_pairs.begin(), sorted_pairs.end() );


    // Default category is called "back"
    std::string category = "back";

    while( true ) {
        std::vector<uimenu_entry> entries;
        uimenu_entry *entry;
        std::map<int, std::string> categories_by_int;
        int last_category = NUM_ACTIONS + 1;

        if( category == "back" ) {
            std::vector<std::pair<action_id, int> >::iterator it;
            for( it = sorted_pairs.begin(); it != sorted_pairs.end(); ++it ) {
                if( it->second >= 200 ) {
                    REGISTER_ACTION( it->first );
                }
            }

            REGISTER_CATEGORY( _( "Look" ) );
            REGISTER_CATEGORY( _( "Interact" ) );
            REGISTER_CATEGORY( _( "Inventory" ) );
            REGISTER_CATEGORY( _( "Combat" ) );
            REGISTER_CATEGORY( _( "Craft" ) );
            REGISTER_CATEGORY( _( "Info" ) );
            REGISTER_CATEGORY( _( "Misc" ) );
            if( hotkey_for_action( ACTION_QUICKSAVE ) > -1 ) {
                REGISTER_ACTION( ACTION_QUICKSAVE );
            }
            REGISTER_ACTION( ACTION_SAVE );
            if( hotkey_for_action( ACTION_QUICKLOAD ) > -1 ) {
                REGISTER_ACTION( ACTION_QUICKLOAD );
            }
            if( hotkey_for_action( ACTION_QUIT ) > -1 ) {
                REGISTER_ACTION( ACTION_QUIT );
            }
            REGISTER_ACTION( ACTION_HELP );
            if( ( entry = &entries.back() ) ) {
                entry->txt += "...";        // help _is_a menu.
            }
            if( hotkey_for_action( ACTION_DEBUG ) > -1 ) {
                REGISTER_CATEGORY( _( "Debug" ) ); // register with global key
                if( ( entry = &entries.back() ) ) {
                    entry->hotkey = hotkey_for_action( ACTION_DEBUG );
                }
            }
        } else if( category == _( "Look" ) ) {
            REGISTER_ACTION( ACTION_LOOK );
            REGISTER_ACTION( ACTION_PEEK );
            REGISTER_ACTION( ACTION_LIST_ITEMS );
            REGISTER_ACTION( ACTION_ZONES );
            REGISTER_ACTION( ACTION_MAP );
        } else if( category == _( "Inventory" ) ) {
            REGISTER_ACTION( ACTION_INVENTORY );
            REGISTER_ACTION( ACTION_ADVANCEDINV );
            REGISTER_ACTION( ACTION_SORT_ARMOR );
            REGISTER_ACTION( ACTION_DIR_DROP );

            // Everything below here can be accessed through
            // the inventory screen, so it's sorted to the
            // end of the list.
            REGISTER_ACTION( ACTION_DROP );
            REGISTER_ACTION( ACTION_COMPARE );
            REGISTER_ACTION( ACTION_ORGANIZE );
            REGISTER_ACTION( ACTION_USE );
            REGISTER_ACTION( ACTION_WEAR );
            REGISTER_ACTION( ACTION_TAKE_OFF );
            REGISTER_ACTION( ACTION_EAT );
            REGISTER_ACTION( ACTION_READ );
            REGISTER_ACTION( ACTION_WIELD );
            REGISTER_ACTION( ACTION_UNLOAD );
        } else if( category == _( "Debug" ) ) {
            REGISTER_ACTION( ACTION_DEBUG );
            if( ( entry = &entries.back() ) ) {
                entry->txt += "..."; // debug _is_a menu.
            }
            REGISTER_ACTION( ACTION_TOGGLE_SIDEBAR_STYLE );
#ifndef TILES
            REGISTER_ACTION( ACTION_TOGGLE_FULLSCREEN );
#endif
#ifdef TILES
            REGISTER_ACTION( ACTION_TOGGLE_PIXEL_MINIMAP );
#endif // TILES
            REGISTER_ACTION( ACTION_DISPLAY_SCENT );
            REGISTER_ACTION( ACTION_TOGGLE_DEBUG_MODE );
        } else if( category == _( "Interact" ) ) {
            REGISTER_ACTION( ACTION_EXAMINE );
            REGISTER_ACTION( ACTION_SMASH );
            REGISTER_ACTION( ACTION_MOVE_DOWN );
            REGISTER_ACTION( ACTION_MOVE_UP );
            REGISTER_ACTION( ACTION_OPEN );
            REGISTER_ACTION( ACTION_CLOSE );
            REGISTER_ACTION( ACTION_CHAT );
            REGISTER_ACTION( ACTION_PICKUP );
            REGISTER_ACTION( ACTION_GRAB );
            REGISTER_ACTION( ACTION_BUTCHER );
        } else if( category == _( "Combat" ) ) {
            REGISTER_ACTION( ACTION_TOGGLE_MOVE );
            REGISTER_ACTION( ACTION_FIRE );
            REGISTER_ACTION( ACTION_RELOAD );
            REGISTER_ACTION( ACTION_SELECT_FIRE_MODE );
            REGISTER_ACTION( ACTION_THROW );
            REGISTER_ACTION( ACTION_FIRE_BURST );
            REGISTER_ACTION( ACTION_PICK_STYLE );
            REGISTER_ACTION( ACTION_TOGGLE_SAFEMODE );
            REGISTER_ACTION( ACTION_TOGGLE_AUTOSAFE );
            REGISTER_ACTION( ACTION_IGNORE_ENEMY );
            REGISTER_ACTION( ACTION_TOGGLE_AUTO_PULP_BUTCHER );
        } else if( category == _( "Craft" ) ) {
            REGISTER_ACTION( ACTION_CRAFT );
            REGISTER_ACTION( ACTION_RECRAFT );
            REGISTER_ACTION( ACTION_LONGCRAFT );
            REGISTER_ACTION( ACTION_CONSTRUCT );
            REGISTER_ACTION( ACTION_DISASSEMBLE );
        } else if( category == _( "Info" ) ) {
            REGISTER_ACTION( ACTION_PL_INFO );
            REGISTER_ACTION( ACTION_MISSIONS );
            REGISTER_ACTION( ACTION_KILLS );
            REGISTER_ACTION( ACTION_FACTIONS );
            REGISTER_ACTION( ACTION_MORALE );
            REGISTER_ACTION( ACTION_MESSAGES );
        } else if( category == _( "Misc" ) ) {
            REGISTER_ACTION( ACTION_WAIT );
            REGISTER_ACTION( ACTION_SLEEP );
            REGISTER_ACTION( ACTION_BIONICS );
            REGISTER_ACTION( ACTION_MUTATIONS );
            REGISTER_ACTION( ACTION_CONTROL_VEHICLE );
            REGISTER_ACTION( ACTION_ITEMACTION );
#ifdef TILES
            if( use_tiles ) {
                REGISTER_ACTION( ACTION_ZOOM_OUT );
                REGISTER_ACTION( ACTION_ZOOM_IN );
            }
#endif
        }

        std::string title = _( "Back" );
        title += "...";
        if( category == "back" ) {
            title = _( "Cancel" );
        }
        entries.emplace_back( uimenu_entry( 2 * NUM_ACTIONS, true,
                                            hotkey_for_action( ACTION_ACTIONMENU ), title ) );

        title = _( "Actions" );
        if( category != "back" ) {
            catgname = _( category.c_str() );
            capitalize_letter( catgname, 0 );
            title += ": " + catgname;
        }

        int width = 0;
        for( auto &cur_entry : entries ) {
            if( width < ( int )cur_entry.txt.length() ) {
                width = cur_entry.txt.length();
            }
        }
        //border=2, selectors=3, after=3 for balance.
        width += 2 + 3 + 3;
        int ix = ( TERMX > width ) ? ( TERMX - width ) / 2 - 1 : 0;
        int iy = ( TERMY > ( int )entries.size() + 2 ) ? ( TERMY - ( int )entries.size() - 2 ) / 2 - 1 : 0;
        int selection = ( int ) uimenu( true, std::max( ix, 0 ), std::min( width, TERMX - 2 ),
                                        std::max( iy, 0 ), title, entries );

        g->draw();

        if( selection < 0 ) {
            return ACTION_NULL;
        } else if( selection == 2 * NUM_ACTIONS ) {
            if( category != "back" ) {
                category = "back";
            } else {
                return ACTION_NULL;
            }
        } else if( selection > NUM_ACTIONS ) {
            category = categories_by_int[selection];
        } else {
            return ( action_id ) selection;
        }
    }

#undef REGISTER_ACTION
#undef REGISTER_CATEGORY
}

action_id handle_main_menu()
{
    const input_context ctxt = get_default_mode_input_context();
    std::vector<uimenu_entry> entries;

    auto REGISTER_ACTION = [&]( action_id name ) {
        entries.emplace_back( uimenu_entry( name, true, hotkey_for_action( name ),
                                            ctxt.get_action_name( action_ident( name ) )
                                          )
                            );
    };

    REGISTER_ACTION( ACTION_HELP );
    REGISTER_ACTION( ACTION_KEYBINDINGS );
    REGISTER_ACTION( ACTION_OPTIONS );
    REGISTER_ACTION( ACTION_AUTOPICKUP );
    REGISTER_ACTION( ACTION_SAFEMODE );
    REGISTER_ACTION( ACTION_COLOR );
    REGISTER_ACTION( ACTION_WORLD_MODS );
    REGISTER_ACTION( ACTION_ACTIONMENU );
    REGISTER_ACTION( ACTION_QUICKSAVE );
    REGISTER_ACTION( ACTION_SAVE );

    int width = 0;
    for( auto &entry : entries ) {
        if( width < ( int )entry.txt.length() ) {
            width = entry.txt.length();
        }
    }
    //border=2, selectors=3, after=3 for balance.
    width += 2 + 3 + 3;
    int ix = ( TERMX > width ) ? ( TERMX - width ) / 2 - 1 : 0;
    int iy = ( TERMY > ( int )entries.size() + 2 ) ? ( TERMY - ( int )entries.size() - 2 ) / 2 - 1 : 0;
    int selection = ( int ) uimenu( true, std::max( ix, 0 ), std::min( width, TERMX - 2 ),
                                    std::max( iy, 0 ), _( "MAIN MENU" ), entries );

    g->draw();

    if( selection < 0 || selection > NUM_ACTIONS ) {
        return ACTION_NULL;
    } else {
        return ( action_id ) selection;
    }
}

bool choose_direction( const std::string &message, int &x, int &y )
{
    tripoint temp( x, y, g->u.posz() );
    bool ret = choose_direction( message, temp );
    x = temp.x;
    y = temp.y;
    return ret;
}

bool choose_direction( const std::string &message, tripoint &offset, bool allow_vertical )
{
    input_context ctxt( "DEFAULTMODE" );
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "pause" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" ); // why not?
    if( allow_vertical ) {
        ctxt.register_action( "LEVEL_UP" );
        ctxt.register_action( "LEVEL_DOWN" );
    }

    //~ appended to "Close where?" "Pry where?" etc.
    const std::string query_text = message + _( " (Direction button)" );
    popup( query_text, PF_NO_WAIT_ON_TOP );

    const std::string action = ctxt.handle_input();
    if( ctxt.get_direction( offset.x, offset.y, action ) ) {
        offset.z = 0;
        return true;
    } else if( action == "pause" ) {
        offset = tripoint( 0, 0, 0 );
        return true;
    } else if( action == "LEVEL_UP" ) {
        offset = tripoint( 0, 0, 1 );
        return true;
    } else if( action == "LEVEL_DOWN" ) {
        offset = tripoint( 0, 0, -1 );
        return true;
    }

    add_msg( _( "Invalid direction." ) );
    return false;
}

bool choose_adjacent( const std::string &message, int &x, int &y )
{
    tripoint temp( x, y, g->u.posz() );
    bool ret = choose_adjacent( message, temp );
    x = temp.x;
    y = temp.y;
    return ret;
}

bool choose_adjacent( const std::string &message, tripoint &p, bool allow_vertical )
{
    if( !choose_direction( message, p, allow_vertical ) ) {
        return false;
    }
    p += g->u.pos();
    return true;
}

bool choose_adjacent_highlight( const std::string &message, int &x, int &y,
                                action_id action_to_highlight )
{
    tripoint temp( x, y, g->u.posz() );
    bool ret = choose_adjacent_highlight( message, temp, action_to_highlight );
    x = temp.x;
    y = temp.y;
    return ret;
}

bool choose_adjacent_highlight( const std::string &message, tripoint &p,
                                action_id action_to_highlight )
{
    // Highlight nearby terrain according to the highlight function
    bool highlighted = false;
    for( const tripoint &pos : g->m.points_in_radius( g->u.pos(), 1 ) ) {
        if( can_interact_at( action_to_highlight, pos ) ) {
            highlighted = true;
            g->m.drawsq( g->w_terrain, g->u, pos,
                         true, true, g->u.pos() + g->u.view_offset );
        }
    }
    if( highlighted ) {
        wrefresh( g->w_terrain );
    }

    return choose_adjacent( message, p );
}
