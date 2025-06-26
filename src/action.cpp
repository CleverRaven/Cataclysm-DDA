#include "action.h"

#include <algorithm>
#include <climits>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "avatar.h"
#include "cached_options.h" // IWYU pragma: keep
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "input_context.h"
#include "input_enums.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "messages.h"
#include "options.h"
#include "output.h"
#include "path_info.h"
#include "point.h"
#include "popup.h"
#include "ret_val.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "uilist.h"
#include "ui_manager.h"
#include "vehicle.h"
#include "vpart_position.h"

static const itype_id itype_swim_fins( "swim_fins" );

static const quality_id qual_BUTCHER( "BUTCHER" );
static const quality_id qual_CUT_FINE( "CUT_FINE" );

static void parse_keymap( std::istream &keymap_txt, std::map<char, action_id> &kmap,
                          std::set<action_id> &unbound_keymap );

void load_keyboard_settings( std::map<char, action_id> &keymap,
                             std::string &keymap_file_loaded_from,
                             std::set<action_id> &unbound_keymap )
{
    const auto parser = [&]( std::istream & fin ) {
        parse_keymap( fin, keymap, unbound_keymap );
    };
    if( read_from_file_optional( PATH_INFO::keymap(), parser ) ) {
        keymap_file_loaded_from = PATH_INFO::keymap();
    }
}

void parse_keymap( std::istream &keymap_txt, std::map<char, action_id> &kmap,
                   std::set<action_id> &unbound_keymap )
{
    while( !keymap_txt.eof() ) {
        std::string id;
        keymap_txt >> id;
        if( id.empty() || id[0] == '#' ) {
            // Empty line or comment, chomp it
            getline( keymap_txt, id );
        } else if( id == "unbind" ) {
            keymap_txt >> id;
            const action_id act = look_up_action( id );
            if( act != ACTION_NULL ) {
                unbound_keymap.insert( act );
            }
            break;
        } else {
            const action_id act = look_up_action( id );
            if( act == ACTION_NULL ) {
                debugmsg( "Warning!  keymap.txt contains an unknown action, \"%s\"\n"
                          "Fix \"%s\" at your next chance!", id, PATH_INFO::keymap() );
            } else {
                while( !keymap_txt.eof() ) {
                    char ch;
                    keymap_txt >> std::noskipws >> ch >> std::skipws;
                    if( ch == '\n' ) {
                        break;
                    } else if( ch != ' ' || keymap_txt.peek() == '\n' ) {
                        if( kmap.find( ch ) != kmap.end() ) {
                            debugmsg( "Warning!  '%c' assigned twice in the keymap!\n"
                                      "%s is being ignored.\n"
                                      "Fix \"%s\" at your next chance!", ch, id, PATH_INFO::keymap() );
                        } else {
                            kmap[ ch ] = act;
                        }
                    }
                }
            }
        }
    }
}

std::vector<input_event> keys_bound_to( const action_id act,
                                        const int maximum_modifier_count,
                                        const bool restrict_to_printable )
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.keys_bound_to( action_ident( act ), maximum_modifier_count,
                               restrict_to_printable );
}

std::string action_ident( action_id act )
{
    switch( act ) {
        case ACTION_PAUSE:
            return "pause";
        case ACTION_TIMEOUT:
            return "TIMEOUT";
        case ACTION_MOVE_FORTH:
            return "UP";
        case ACTION_MOVE_FORTH_RIGHT:
            return "RIGHTUP";
        case ACTION_MOVE_RIGHT:
            return "RIGHT";
        case ACTION_MOVE_BACK_RIGHT:
            return "RIGHTDOWN";
        case ACTION_MOVE_BACK:
            return "DOWN";
        case ACTION_MOVE_BACK_LEFT:
            return "LEFTDOWN";
        case ACTION_MOVE_LEFT:
            return "LEFT";
        case ACTION_MOVE_FORTH_LEFT:
            return "LEFTUP";
        case ACTION_MOVE_DOWN:
            return "LEVEL_DOWN";
        case ACTION_MOVE_UP:
            return "LEVEL_UP";
        case ACTION_TOGGLE_MAP_MEMORY:
            return "toggle_map_memory";
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
        case ACTION_CYCLE_MOVE:
            return "cycle_move";
        case ACTION_CYCLE_MOVE_REVERSE:
            return "cycle_move_reverse";
        case ACTION_RESET_MOVE:
            return "reset_move";
        case ACTION_TOGGLE_RUN:
            return "toggle_run";
        case ACTION_TOGGLE_CROUCH:
            return "toggle_crouch";
        case ACTION_TOGGLE_PRONE:
            return "toggle_prone";
        case ACTION_OPEN_MOVEMENT:
            return "open_movement";
        case ACTION_OPEN:
            return "open";
        case ACTION_CLOSE:
            return "close";
        case ACTION_SMASH:
            return "smash";
        case ACTION_EXAMINE:
            return "examine";
        case ACTION_EXAMINE_AND_PICKUP:
            return "examine_and_pickup";
        case ACTION_ADVANCEDINV:
            return "advinv";
        case ACTION_PICKUP:
            return "pickup";
        case ACTION_PICKUP_ALL:
            return "pickup_all";
        case ACTION_GRAB:
            return "grab";
        case ACTION_HAUL:
            return "haul";
        case ACTION_HAUL_TOGGLE:
            return "haul_toggle";
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
        case ACTION_LOOT:
            return "loot";
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
        case ACTION_OPEN_CONSUME:
            return "open_consume";
        case ACTION_READ:
            return "read";
        case ACTION_WIELD:
            return "wield";
        case ACTION_PICK_STYLE:
            return "pick_style";
        case ACTION_RELOAD_ITEM:
            return "reload_item";
        case ACTION_RELOAD_WEAPON:
            return "reload_weapon";
        case ACTION_RELOAD_WIELDED:
            return "reload_wielded";
        case ACTION_INSERT_ITEM:
            return "insert";
        case ACTION_UNLOAD:
            return "unload";
        case ACTION_MEND:
            return "mend";
        case ACTION_THROW:
            return "throw";
        case ACTION_THROW_WIELDED:
            return "throw_wielded";
        case ACTION_FIRE:
            return "fire";
        case ACTION_FIRE_BURST:
            return "fire_burst";
        case ACTION_CAST_SPELL:
            return "cast_spell";
        case ACTION_RECAST_SPELL:
            return "recast_spell";
        case ACTION_SELECT_FIRE_MODE:
            return "select_fire_mode";
        case ACTION_SELECT_DEFAULT_AMMO:
            return "select_default_ammo";
        case ACTION_UNLOAD_CONTAINER:
            return "unload_container";
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
        case ACTION_TOGGLE_AUTO_TRAVEL_MODE:
            return "auto_travel_mode";
        case ACTION_TOGGLE_SAFEMODE:
            return "safemode";
        case ACTION_TOGGLE_AUTOSAFE:
            return "autosafe";
        case ACTION_TOGGLE_THIEF_MODE:
            return "toggle_thief_mode";
        case ACTION_TOGGLE_LANGUAGE_TO_EN:
            return "toggle_language_to_en";
        case ACTION_IGNORE_ENEMY:
            return "ignore_enemy";
        case ACTION_WHITELIST_ENEMY:
            return "whitelist_enemy";
        case ACTION_WORKOUT:
            return "workout";
        case ACTION_SAVE:
            return "save";
        case ACTION_QUICKSAVE:
            return "quicksave";
        case ACTION_QUICKLOAD:
            return "quickload";
        case ACTION_SUICIDE:
            return "SUICIDE";
        case ACTION_PL_INFO:
            return "player_data";
        case ACTION_MAP:
            return "map";
        case ACTION_SKY:
            return "sky";
        case ACTION_MISSIONS:
            return "missions";
        case ACTION_FACTIONS:
            return "factions";
        case ACTION_MEDICAL:
            return "medical";
        case ACTION_BODYSTATUS:
            return "bodystatus";
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
        case ACTION_DISPLAY_SCENT_TYPE:
            return "debug_scent_type";
        case ACTION_DISPLAY_TEMPERATURE:
            return "debug_temp";
        case ACTION_DISPLAY_VEHICLE_AI:
            return "debug_vehicle_ai";
        case ACTION_DISPLAY_VISIBILITY:
            return "debug_visibility";
        case ACTION_DISPLAY_TRANSPARENCY:
            return "debug_transparency";
        case ACTION_DISPLAY_LIGHTING:
            return "debug_lighting";
        case ACTION_DISPLAY_RADIATION:
            return "debug_radiation";
        case ACTION_DISPLAY_NPC_ATTACK_POTENTIAL:
            return "debug_npc_attack_potential";
        case ACTION_TOGGLE_HOUR_TIMER:
            return "debug_hour_timer";
        case ACTION_TOGGLE_DEBUG_MODE:
            return "debug_mode";
        case ACTION_ZOOM_OUT:
            return "zoom_out";
        case ACTION_ZOOM_IN:
            return "zoom_in";
        case ACTION_TOGGLE_FULLSCREEN:
            return "toggle_fullscreen";
        case ACTION_TOGGLE_PIXEL_MINIMAP:
            return "toggle_pixel_minimap";
        case ACTION_TOGGLE_PANEL_ADM:
            return "toggle_panel_adm";
        case ACTION_PANEL_MGMT:
            return "panel_mgmt";
        case ACTION_RELOAD_TILESET:
            return "reload_tileset";
        case ACTION_TOGGLE_AUTO_FEATURES:
            return "toggle_auto_features";
        case ACTION_TOGGLE_AUTO_PULP_BUTCHER:
            return "toggle_auto_pulp_butcher";
        case ACTION_TOGGLE_AUTO_MINING:
            return "toggle_auto_mining";
        case ACTION_TOGGLE_AUTO_FORAGING:
            return "toggle_auto_foraging";
        case ACTION_TOGGLE_AUTO_PICKUP:
            return "toggle_auto_pickup";
        case ACTION_TOGGLE_PREVENT_OCCLUSION:
            return "toggle_prevent_occlusion";
        case ACTION_ACTIONMENU:
            return "action_menu";
        case ACTION_ITEMACTION:
            return "item_action_menu";
        case ACTION_SELECT:
            return "SELECT";
        case ACTION_SEC_SELECT:
            return "SEC_SELECT";
        case ACTION_CLICK_AND_DRAG:
            return "CLICK_AND_DRAG";
        case ACTION_AUTOATTACK:
            return "autoattack";
        case ACTION_MAIN_MENU:
            return "main_menu";
        case ACTION_DIARY:
            return "diary";
        case ACTION_KEYBINDINGS:
            return "HELP_KEYBINDINGS";
        case ACTION_OPTIONS:
            return "open_options";
        case ACTION_AUTOPICKUP:
            return "open_autopickup";
        case ACTION_AUTONOTES:
            return "open_autonotes";
        case ACTION_SAFEMODE:
            return "open_safemode";
        case ACTION_COLOR:
            return "open_color";
        case ACTION_WORLD_MODS:
            return "open_world_mods";
        case ACTION_DISTRACTION_MANAGER:
            return "open_distraction_manager";
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
        case ACTION_TOGGLE_MAP_MEMORY:
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
        case ACTION_SUICIDE:
        // Info Screens
        case ACTION_PL_INFO:
        case ACTION_MAP:
        case ACTION_SKY:
        case ACTION_MISSIONS:
        case ACTION_FACTIONS:
        case ACTION_MORALE:
        case ACTION_MEDICAL:
        case ACTION_BODYSTATUS:
        case ACTION_MESSAGES:
        case ACTION_HELP:
        case ACTION_MAIN_MENU:
        case ACTION_KEYBINDINGS:
        case ACTION_OPTIONS:
        case ACTION_AUTOPICKUP:
        case ACTION_AUTONOTES:
        case ACTION_SAFEMODE:
        case ACTION_COLOR:
        case ACTION_WORLD_MODS:
        case ACTION_DISTRACTION_MANAGER:
        // Debug Functions
        case ACTION_TOGGLE_FULLSCREEN:
        case ACTION_DEBUG:
        case ACTION_DISPLAY_SCENT:
        case ACTION_DISPLAY_SCENT_TYPE:
        case ACTION_DISPLAY_TEMPERATURE:
        case ACTION_DISPLAY_VEHICLE_AI:
        case ACTION_DISPLAY_VISIBILITY:
        case ACTION_DISPLAY_LIGHTING:
        case ACTION_DISPLAY_RADIATION:
        case ACTION_DISPLAY_NPC_ATTACK_POTENTIAL:
        case ACTION_DISPLAY_TRANSPARENCY:
        case ACTION_ZOOM_OUT:
        case ACTION_ZOOM_IN:
        case ACTION_TOGGLE_PIXEL_MINIMAP:
        case ACTION_TOGGLE_PANEL_ADM:
        case ACTION_PANEL_MGMT:
        case ACTION_RELOAD_TILESET:
        case ACTION_TIMEOUT:
        case ACTION_TOGGLE_AUTO_FEATURES:
        case ACTION_TOGGLE_AUTO_PULP_BUTCHER:
        case ACTION_TOGGLE_AUTO_MINING:
        case ACTION_TOGGLE_AUTO_FORAGING:
            return false;
        default:
            return true;
    }
}

action_id look_up_action( const std::string &ident )
{
    // Temporarily for the interface with the input manager!
    if( ident == "move_nw" ) {
        return ACTION_MOVE_FORTH_LEFT;
    } else if( ident == "move_sw" ) {
        return ACTION_MOVE_BACK_LEFT;
    } else if( ident == "move_ne" ) {
        return ACTION_MOVE_FORTH_RIGHT;
    } else if( ident == "move_se" ) {
        return ACTION_MOVE_BACK_RIGHT;
    } else if( ident == "move_n" ) {
        return ACTION_MOVE_FORTH;
    } else if( ident == "move_s" ) {
        return ACTION_MOVE_BACK;
    } else if( ident == "move_w" ) {
        return ACTION_MOVE_LEFT;
    } else if( ident == "move_e" ) {
        return ACTION_MOVE_RIGHT;
    } else if( ident == "move_down" ) {
        return ACTION_MOVE_DOWN;
    } else if( ident == "move_up" ) {
        return ACTION_MOVE_UP;
    }
    // ^^ Temporarily for the interface with the input manager!
    for( int i = 0; i < NUM_ACTIONS; i++ ) {
        if( action_ident( static_cast<action_id>( i ) ) == ident ) {
            return static_cast<action_id>( i );
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
std::optional<std::string> press_x_if_bound( action_id act )
{
    input_context ctxt = get_default_mode_input_context();
    std::string description = action_ident( act );
    if( ctxt.keys_bound_to( description, /*maximum_modifier_count=*/ -1,
                            /*restrict_to_printable=*/false ).empty() ) {
        return std::nullopt;
    }
    return press_x( act );
}

action_id get_movement_action_from_delta( const tripoint_rel_ms &d, const iso_rotate rot )
{
    const bool iso_mode = rot == iso_rotate::yes && g->is_tileset_isometric();
    if( d.xy() == point_rel_ms::north ) {
        return iso_mode ? ACTION_MOVE_FORTH_LEFT : ACTION_MOVE_FORTH;
    } else if( d.xy() == point_rel_ms::north_east ) {
        return iso_mode ? ACTION_MOVE_FORTH : ACTION_MOVE_FORTH_RIGHT;
    } else if( d.xy() == point_rel_ms::east ) {
        return iso_mode ? ACTION_MOVE_FORTH_RIGHT : ACTION_MOVE_RIGHT;
    } else if( d.xy() == point_rel_ms::south_east ) {
        return iso_mode ? ACTION_MOVE_RIGHT : ACTION_MOVE_BACK_RIGHT;
    } else if( d.xy() == point_rel_ms::south ) {
        return iso_mode ? ACTION_MOVE_BACK_RIGHT : ACTION_MOVE_BACK;
    } else if( d.xy() == point_rel_ms::south_west ) {
        return iso_mode ? ACTION_MOVE_BACK : ACTION_MOVE_BACK_LEFT;
    } else if( d.xy() == point_rel_ms::west ) {
        return iso_mode ? ACTION_MOVE_BACK_LEFT : ACTION_MOVE_LEFT;
    } else if( d.xy() == point_rel_ms::north_west ) {
        return iso_mode ? ACTION_MOVE_LEFT : ACTION_MOVE_FORTH_LEFT;
    }

    // Check z location last.
    // If auto-moving over ramps, we need the character to move the xy directions.
    // The z-movement is caused by the ramp automatically.
    if( d.z() == -1 ) {
        return ACTION_MOVE_DOWN;
    } else if( d.z() == 1 ) {
        return ACTION_MOVE_UP;
    }

    return ACTION_NULL;
}

point_rel_ms get_delta_from_movement_action( const action_id act, const iso_rotate rot )
{
    const bool iso_mode = rot == iso_rotate::yes && g->is_tileset_isometric();
    switch( act ) {
        case ACTION_MOVE_FORTH:
            return iso_mode ? point_rel_ms::north_east : point_rel_ms::north;
        case ACTION_MOVE_FORTH_RIGHT:
            return iso_mode ? point_rel_ms::east : point_rel_ms::north_east;
        case ACTION_MOVE_RIGHT:
            return iso_mode ? point_rel_ms::south_east : point_rel_ms::east;
        case ACTION_MOVE_BACK_RIGHT:
            return iso_mode ? point_rel_ms::south : point_rel_ms::south_east;
        case ACTION_MOVE_BACK:
            return iso_mode ? point_rel_ms::south_west : point_rel_ms::south;
        case ACTION_MOVE_BACK_LEFT:
            return iso_mode ? point_rel_ms::west : point_rel_ms::south_west;
        case ACTION_MOVE_LEFT:
            return iso_mode ? point_rel_ms::north_west : point_rel_ms::west;
        case ACTION_MOVE_FORTH_LEFT:
            return iso_mode ? point_rel_ms::north : point_rel_ms::north_west;
        default:
            return point_rel_ms::zero;
    }
}

std::optional<input_event> hotkey_for_action( const action_id action,
        const int maximum_modifier_count, const bool restrict_to_printable )
{
    const std::vector<input_event> keys = keys_bound_to( action,
                                          maximum_modifier_count,
                                          restrict_to_printable );
    if( keys.empty() ) {
        return std::nullopt;
    } else {
        return keys.front();
    }
}

bool can_butcher_at( map &here, const tripoint_bub_ms &p )
{
    map_stack items = here.i_at( p );
    // Early exit when there's definitely nothing to butcher
    if( items.empty() ) {
        return false;
    }
    Character &player_character = get_player_character();
    // TODO: unify this with game::butcher
    const int factor = player_character.max_quality( qual_BUTCHER, PICKUP_RANGE );
    const int factorD = player_character.max_quality( qual_CUT_FINE, PICKUP_RANGE );
    bool has_item = false;
    bool has_corpse = false;

    const inventory &crafting_inv = player_character.crafting_inventory();
    for( item &items_it : items ) {
        if( items_it.is_corpse() ) {
            if( factor != INT_MIN  || factorD != INT_MIN ) {
                has_corpse = true;
            }
        } else if( player_character.can_disassemble( items_it, crafting_inv ).success() ) {
            has_item = true;
        }
    }
    return has_corpse || has_item;
}

bool can_move_vertical_at( const map &here, const tripoint_bub_ms &p, int movez )
{
    if( p.z() + movez < -OVERMAP_DEPTH || p.z() + movez > OVERMAP_HEIGHT ) {
        return false;
    }
    Character &player_character = get_player_character();
    // TODO: unify this with game::move_vertical
    if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, p ) &&
        here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, p ) ) {
        if( movez == -1 ) {
            return !player_character.is_underwater() && !player_character.worn_with_flag( flag_FLOTATION );
        } else {
            return player_character.swim_speed() < 500 ||
                   player_character.is_wearing( itype_swim_fins );
        }
    }

    if( movez == -1 ) {
        return here.has_flag( ter_furn_flag::TFLAG_GOES_DOWN, p );
    } else {
        return here.has_flag( ter_furn_flag::TFLAG_GOES_UP, p );
    }
}

bool can_examine_at( map &here, const tripoint_bub_ms &p, bool with_pickup )
{
    if( here.veh_at( p ) ) {
        return true;
    }
    if( here.has_flag( ter_furn_flag::TFLAG_CONSOLE, p ) ) {
        return true;
    }
    if( with_pickup && !here.has_flag( ter_furn_flag::TFLAG_SEALED, p ) && here.has_items( p ) ) {
        return true;
    }
    const furn_t &xfurn_t = here.furn( p ).obj();
    const ter_t &xter_t = here.ter( p ).obj();

    if( here.has_furn( p ) && xfurn_t.can_examine( p ) ) {
        return true;
    }
    if( here.partial_con_at( p ) != nullptr ) {
        return true;
    }
    if( xter_t.can_examine( p ) ) {
        return true;
    }

    Creature *c = get_creature_tracker().creature_at( p );
    if( c != nullptr && !c->is_avatar() ) {
        return true;
    }

    return here.can_see_trap_at( p, get_player_character() );
}

static bool can_pickup_at( map &here, const tripoint_bub_ms &p )
{
    if( const std::optional<vpart_reference> ovp = here.veh_at( p ).cargo() ) {
        if( !ovp->items().empty() ) {
            return true;
        }
    }
    return !here.has_flag( ter_furn_flag::TFLAG_SEALED, p ) &&
           !here.only_liquid_in_liquidcont( p ) &&
           here.has_items( p );
}

bool can_interact_at( action_id action, map &here, const tripoint_bub_ms &p )
{
    if( here.impassable_field_at( p ) ) {
        return false;
    }
    tripoint_bub_ms player_pos = get_player_character().pos_bub();
    switch( action ) {
        case ACTION_OPEN:
            return here.open_door( get_avatar(), p, !here.is_outside( player_pos ), true );
        case ACTION_CLOSE: {
            const optional_vpart_position vp = here.veh_at( p );
            return ( vp &&
                     vp->vehicle().next_part_to_close( vp->part_index(),
                             veh_pointer_or_null( here.veh_at( player_pos ) ) != &vp->vehicle() ) >= 0 ) ||
                   here.close_door( p, !here.is_outside( player_pos ), true );
        }
        case ACTION_BUTCHER:
            return can_butcher_at( here, p );
        case ACTION_MOVE_UP:
            return can_move_vertical_at( here, p, 1 );
        case ACTION_MOVE_DOWN:
            return can_move_vertical_at( here, p, -1 );
        case ACTION_EXAMINE:
            return can_examine_at( here, p, false );
        case ACTION_EXAMINE_AND_PICKUP:
            return can_examine_at( here, p, true );
        case ACTION_PICKUP:
            return can_pickup_at( here, p );
        default:
            return false;
    }

    return can_interact_at( action, here, p );
}

action_id handle_action_menu( map &here )
{
    const input_context ctxt = get_default_mode_input_context();
    std::string catgname;

#define REGISTER_ACTION( name ) entries.emplace_back( name, true, hotkey_for_action( name, /*maximum_modifier_count=*/1 ), \
        ctxt.get_action_name( action_ident( name ) ) );
#define REGISTER_CATEGORY( name )  categories_by_int[last_category] = name; \
    catgname = name; \
    catgname += "…"; \
    entries.emplace_back( last_category, true, -1, catgname ); \
    last_category++;

    // Calculate weightings for the various actions to give the player suggestions
    // Weight >= 200: Special action only available right now
    std::map<action_id, int> action_weightings;

    Character &player_character = get_player_character();
    // Check if we're in a potential combat situation, if so, sort a few actions to the top.
    if( !player_character.get_hostile_creatures( MAX_VIEW_DISTANCE ).empty() ) {
        // Only prioritize movement options if we're not driving.
        if( !player_character.controlling_vehicle ) {
            action_weightings[ACTION_CYCLE_MOVE] = 400;
            action_weightings[ACTION_CYCLE_MOVE_REVERSE] = 400;
        }
        const item_location weapon = player_character.get_wielded_item();
        // Only prioritize fire weapon options if we're wielding a ranged weapon.
        if( weapon && ( weapon->is_gun() || weapon->has_flag( flag_REACH_ATTACK ) ) ) {
            action_weightings[ACTION_FIRE] = 350;
        }
    }

    // If we're already running, make it simple to toggle running to off.
    if( player_character.is_running() ) {
        action_weightings[ACTION_TOGGLE_RUN] = 300;
    }
    // If we're already crouching, make it simple to toggle crouching to off.
    if( player_character.is_crouching() ) {
        action_weightings[ACTION_TOGGLE_CROUCH] = 300;
    }
    // If we're already prone, make it simple to toggle prone to off.
    if( player_character.is_prone() ) {
        action_weightings[ACTION_TOGGLE_PRONE] = 300;
    }

    // Check if we're on a vehicle, if so, vehicle controls should be top.
    if( here.veh_at( player_character.pos_bub() ) ) {
        // Make it 300 to prioritize it before examining the vehicle.
        action_weightings[ACTION_CONTROL_VEHICLE] = 300;
    }

    // Check if we can perform one of our actions on nearby terrain. If so,
    // display that action at the top of the list.
    for( const tripoint_bub_ms &pos : here.points_in_radius( player_character.pos_bub(), 1 ) ) {
        if( pos != player_character.pos_bub() ) {
            // Check for actions that work on nearby tiles
            if( can_interact_at( ACTION_OPEN, here, pos ) ) {
                action_weightings[ACTION_OPEN] = 200;
            }
            if( can_interact_at( ACTION_CLOSE, here, pos ) ) {
                action_weightings[ACTION_CLOSE] = 200;
            }
            if( can_interact_at( ACTION_EXAMINE, here, pos ) ) {
                action_weightings[ACTION_EXAMINE] = 200;
            }
        } else {
            // Check for actions that work on own tile only
            if( can_interact_at( ACTION_BUTCHER, here, pos ) ) {
                action_weightings[ACTION_BUTCHER] = 200;
            }
            if( can_interact_at( ACTION_MOVE_UP, here, pos ) ) {
                action_weightings[ACTION_MOVE_UP] = 200;
            }
            if( can_interact_at( ACTION_MOVE_DOWN, here, pos ) ) {
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
        std::vector<uilist_entry> entries;
        uilist_entry *entry;
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
            if( hotkey_for_action( ACTION_QUICKSAVE, /*maximum_modifier_count=*/1 ).has_value() ) {
                REGISTER_ACTION( ACTION_QUICKSAVE );
            }
            REGISTER_ACTION( ACTION_SAVE );
            if( hotkey_for_action( ACTION_QUICKLOAD, /*maximum_modifier_count=*/1 ).has_value() ) {
                REGISTER_ACTION( ACTION_QUICKLOAD );
            }
            if( hotkey_for_action( ACTION_SUICIDE, /*maximum_modifier_count=*/1 ).has_value() ) {
                REGISTER_ACTION( ACTION_SUICIDE );
            }
            REGISTER_ACTION( ACTION_HELP );
            if( ( entry = &entries.back() ) ) {
                // help _is_a menu.
                entry->txt += "…";
            }
            if( hotkey_for_action( ACTION_DEBUG, /*maximum_modifier_count=*/1 ).has_value() ) {
                // register with global key
                REGISTER_CATEGORY( _( "Debug" ) );
                if( ( entry = &entries.back() ) ) {
                    entry->hotkey = hotkey_for_action( ACTION_DEBUG, /*maximum_modifier_count=*/1 );
                }
            }
        } else if( category == _( "Look" ) ) {
            REGISTER_ACTION( ACTION_LOOK );
            REGISTER_ACTION( ACTION_PEEK );
            REGISTER_ACTION( ACTION_LIST_ITEMS );
            REGISTER_ACTION( ACTION_ZONES );
            REGISTER_ACTION( ACTION_MAP );
            REGISTER_ACTION( ACTION_SKY );
        } else if( category == _( "Inventory" ) ) {
            REGISTER_ACTION( ACTION_INVENTORY );
            REGISTER_ACTION( ACTION_ADVANCEDINV );
            REGISTER_ACTION( ACTION_SORT_ARMOR );
            REGISTER_ACTION( ACTION_DIR_DROP );

            // Everything below here can be accessed through
            // the inventory screen, so it's sorted to the
            // end of the list.
            REGISTER_ACTION( ACTION_INSERT_ITEM );
            REGISTER_ACTION( ACTION_UNLOAD_CONTAINER );
            REGISTER_ACTION( ACTION_DROP );
            REGISTER_ACTION( ACTION_COMPARE );
            REGISTER_ACTION( ACTION_ORGANIZE );
            REGISTER_ACTION( ACTION_USE );
            REGISTER_ACTION( ACTION_WEAR );
            REGISTER_ACTION( ACTION_TAKE_OFF );
            REGISTER_ACTION( ACTION_EAT );
            REGISTER_ACTION( ACTION_OPEN_CONSUME );
            REGISTER_ACTION( ACTION_READ );
            REGISTER_ACTION( ACTION_WIELD );
            REGISTER_ACTION( ACTION_UNLOAD );
        } else if( category == _( "Debug" ) ) {
            REGISTER_ACTION( ACTION_DEBUG );
            if( ( entry = &entries.back() ) ) {
                // debug _is_a menu.
                entry->txt += "…";
            }
#if !defined(TILES)
            REGISTER_ACTION( ACTION_TOGGLE_FULLSCREEN );
#endif
#if defined(TILES)
            REGISTER_ACTION( ACTION_TOGGLE_PIXEL_MINIMAP );
            REGISTER_ACTION( ACTION_RELOAD_TILESET );
            REGISTER_ACTION( ACTION_TOGGLE_PREVENT_OCCLUSION );
#endif // TILES
            REGISTER_ACTION( ACTION_TOGGLE_PANEL_ADM );
            REGISTER_ACTION( ACTION_DISPLAY_SCENT );
            REGISTER_ACTION( ACTION_DISPLAY_SCENT_TYPE );
            REGISTER_ACTION( ACTION_DISPLAY_TEMPERATURE );
            REGISTER_ACTION( ACTION_DISPLAY_VEHICLE_AI );
            REGISTER_ACTION( ACTION_DISPLAY_VISIBILITY );
            REGISTER_ACTION( ACTION_DISPLAY_LIGHTING );
            REGISTER_ACTION( ACTION_DISPLAY_TRANSPARENCY );
            REGISTER_ACTION( ACTION_DISPLAY_RADIATION );
            REGISTER_ACTION( ACTION_DISPLAY_NPC_ATTACK_POTENTIAL );
            REGISTER_ACTION( ACTION_TOGGLE_DEBUG_MODE );
        } else if( category == _( "Interact" ) ) {
            REGISTER_ACTION( ACTION_EXAMINE );
            REGISTER_ACTION( ACTION_EXAMINE_AND_PICKUP );
            REGISTER_ACTION( ACTION_SMASH );
            REGISTER_ACTION( ACTION_MOVE_DOWN );
            REGISTER_ACTION( ACTION_MOVE_UP );
            REGISTER_ACTION( ACTION_OPEN );
            REGISTER_ACTION( ACTION_CLOSE );
            REGISTER_ACTION( ACTION_CHAT );
            REGISTER_ACTION( ACTION_PICKUP );
            REGISTER_ACTION( ACTION_PICKUP_ALL );
            REGISTER_ACTION( ACTION_GRAB );
            REGISTER_ACTION( ACTION_HAUL );
            REGISTER_ACTION( ACTION_HAUL_TOGGLE );
            REGISTER_ACTION( ACTION_BUTCHER );
            REGISTER_ACTION( ACTION_LOOT );
        } else if( category == _( "Combat" ) ) {
            REGISTER_ACTION( ACTION_CYCLE_MOVE );
            REGISTER_ACTION( ACTION_RESET_MOVE );
            REGISTER_ACTION( ACTION_TOGGLE_RUN );
            REGISTER_ACTION( ACTION_TOGGLE_CROUCH );
            REGISTER_ACTION( ACTION_TOGGLE_PRONE );
            REGISTER_ACTION( ACTION_OPEN_MOVEMENT );
            REGISTER_ACTION( ACTION_FIRE );
            REGISTER_ACTION( ACTION_RELOAD_ITEM );
            REGISTER_ACTION( ACTION_RELOAD_WEAPON );
            REGISTER_ACTION( ACTION_RELOAD_WIELDED );
            REGISTER_ACTION( ACTION_CAST_SPELL );
            REGISTER_ACTION( ACTION_RECAST_SPELL );
            REGISTER_ACTION( ACTION_SELECT_FIRE_MODE );
            REGISTER_ACTION( ACTION_SELECT_DEFAULT_AMMO );
            REGISTER_ACTION( ACTION_THROW );
            REGISTER_ACTION( ACTION_THROW_WIELDED );
            REGISTER_ACTION( ACTION_FIRE_BURST );
            REGISTER_ACTION( ACTION_PICK_STYLE );
            REGISTER_ACTION( ACTION_TOGGLE_AUTO_TRAVEL_MODE );
            REGISTER_ACTION( ACTION_TOGGLE_SAFEMODE );
            REGISTER_ACTION( ACTION_TOGGLE_AUTOSAFE );
            REGISTER_ACTION( ACTION_IGNORE_ENEMY );
            REGISTER_ACTION( ACTION_TOGGLE_AUTO_FEATURES );
            REGISTER_ACTION( ACTION_TOGGLE_AUTO_PULP_BUTCHER );
            REGISTER_ACTION( ACTION_TOGGLE_AUTO_MINING );
            REGISTER_ACTION( ACTION_TOGGLE_AUTO_FORAGING );
        } else if( category == _( "Craft" ) ) {
            REGISTER_ACTION( ACTION_CRAFT );
            REGISTER_ACTION( ACTION_RECRAFT );
            REGISTER_ACTION( ACTION_LONGCRAFT );
            REGISTER_ACTION( ACTION_CONSTRUCT );
            REGISTER_ACTION( ACTION_DISASSEMBLE );
        } else if( category == _( "Info" ) ) {
            REGISTER_ACTION( ACTION_PL_INFO );
            REGISTER_ACTION( ACTION_MISSIONS );
            REGISTER_ACTION( ACTION_FACTIONS );
            REGISTER_ACTION( ACTION_MORALE );
            REGISTER_ACTION( ACTION_MEDICAL );
            REGISTER_ACTION( ACTION_BODYSTATUS );
            REGISTER_ACTION( ACTION_MESSAGES );
            REGISTER_ACTION( ACTION_DIARY );
        } else if( category == _( "Misc" ) ) {
            REGISTER_ACTION( ACTION_WAIT );
            REGISTER_ACTION( ACTION_SLEEP );
            REGISTER_ACTION( ACTION_WORKOUT );
            REGISTER_ACTION( ACTION_BIONICS );
            REGISTER_ACTION( ACTION_MUTATIONS );
            REGISTER_ACTION( ACTION_CONTROL_VEHICLE );
            REGISTER_ACTION( ACTION_ITEMACTION );
            REGISTER_ACTION( ACTION_TOGGLE_THIEF_MODE );
#if defined(TILES)
            if( use_tiles ) {
                REGISTER_ACTION( ACTION_ZOOM_OUT );
                REGISTER_ACTION( ACTION_ZOOM_IN );
            }
#endif
        }

        if( category != "back" ) {
            std::string msg = _( "Back" );
            msg += "…";
            entries.emplace_back( 2 * NUM_ACTIONS, true,
                                  hotkey_for_action( ACTION_ACTIONMENU, /*maximum_modifier_count=*/1 ), msg );
        }

        std::string title = _( "Actions" );
        if( category != "back" ) {
            catgname = uppercase_first_letter( category );
            title += ": " + catgname;
        }

        uilist smenu;
        smenu.settext( title );
        smenu.entries = entries;
        smenu.query();
        const int selection = smenu.ret;

        if( selection < 0 || selection == NUM_ACTIONS ) {
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
            return static_cast<action_id>( selection );
        }
    }

#undef REGISTER_ACTION
#undef REGISTER_CATEGORY
}

action_id handle_main_menu()
{
    const input_context ctxt = get_default_mode_input_context();
    std::vector<uilist_entry> entries;

    const auto REGISTER_ACTION = [&]( action_id name, std::optional<char> kb = std::nullopt ) {
        std::optional<input_event> hotkey = hotkey_for_action( name, /*maximum_modifier_count=*/1 );
        if( hotkey.has_value() || !kb.has_value() ) {
            entries.emplace_back( name, true, hotkey, ctxt.get_action_name( action_ident( name ) ) );
        } else {
            entries.emplace_back( name, true, kb.value(), ctxt.get_action_name( action_ident( name ) ) );
        }
    };

    REGISTER_ACTION( ACTION_HELP );

    // The hotkey is reserved for the uilist keybindings menu
    entries.emplace_back( ACTION_KEYBINDINGS, true, std::nullopt,
                          ctxt.get_action_name( action_ident( ACTION_KEYBINDINGS ) ) );

    REGISTER_ACTION( ACTION_OPTIONS );
    REGISTER_ACTION( ACTION_TOGGLE_PANEL_ADM );
    REGISTER_ACTION( ACTION_AUTOPICKUP );
    REGISTER_ACTION( ACTION_AUTONOTES );
    REGISTER_ACTION( ACTION_SAFEMODE );
    REGISTER_ACTION( ACTION_DISTRACTION_MANAGER );
    REGISTER_ACTION( ACTION_COLOR );
    REGISTER_ACTION( ACTION_WORLD_MODS );
    REGISTER_ACTION( ACTION_ACTIONMENU );
    REGISTER_ACTION( ACTION_QUICKSAVE );
    REGISTER_ACTION( ACTION_SAVE );
    REGISTER_ACTION( ACTION_DEBUG, 'd' );

    uilist smenu;
    smenu.settext( _( "MAIN MENU" ) );
    smenu.entries = entries;
    smenu.query();
    int selection = smenu.ret;

    if( selection < 0 || selection >= NUM_ACTIONS ) {
        return ACTION_NULL;
    } else {
        return static_cast<action_id>( selection );
    }
}

std::optional<tripoint_rel_ms> choose_direction( const std::string &message,
        const bool allow_vertical, const bool allow_mouse, const int timeout,
        const std::function<std::pair<bool, std::optional<tripoint_rel_ms>>(
            const input_context &ctxt, const std::string &action )> &action_cb )
{
    input_context ctxt( "DEFAULTMODE", keyboard_mode::keycode );
    if( timeout >= 0 ) {
        ctxt.set_timeout( timeout );
    }
    ctxt.set_iso( true );
    ctxt.register_directions();
    ctxt.register_action( "pause" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    if( allow_vertical ) {
        ctxt.register_action( "LEVEL_UP" );
        ctxt.register_action( "LEVEL_DOWN" );
    }
    if( allow_mouse ) {
        ctxt.register_action( "COORDINATE" );
        ctxt.register_action( "MOUSE_MOVE" );
        ctxt.register_action( "SELECT" );
    }

    static_popup popup;
    popup.message( allow_mouse
                   //~ %s: "Close where?" "Pry where?" etc.
                   ? _( "%s (Direction button or mouse)" )
                   //~ %s: "Close where?" "Pry where?" etc.
                   : _( "%s (Direction button)" ), message ).on_top( true );

    temp_hide_advanced_inv();
    std::string action;
    bool done = false;
    do {
        ui_manager::redraw();
        action = ctxt.handle_input();
        if( std::optional<tripoint_rel_ms> vec = ctxt.get_direction_rel_ms( action ) ) {
            FacingDirection &facing = get_player_character().facing;
            // Make player's sprite face left/right if interacting with something to the left or right
            if( vec->x() > 0 ) {
                facing = FacingDirection::RIGHT;
            } else if( vec->x() < 0 ) {
                facing = FacingDirection::LEFT;
            }
            return vec;
        } else if( action == "pause" ) {
            return tripoint_rel_ms::zero;
        } else if( action == "LEVEL_UP" ) {
            return tripoint_rel_ms::above;
        } else if( action == "LEVEL_DOWN" ) {
            return tripoint_rel_ms::below;
        } else if( action == "QUIT" ) {
            done = true;
        }
        if( !done && action_cb ) {
            const std::pair<bool, std::optional<tripoint_rel_ms>> ret = action_cb( ctxt, action );
            done = ret.first;
            if( done && ret.second.has_value()
                && ret.second->x() <= 1 && ret.second->x() >= -1
                && ret.second->y() <= 1 && ret.second->y() >= -1 && ( allow_vertical
                        ? ret.second->z() <= 1 && ret.second->z() >= -1 : ret.second->z() == 0 ) ) {
                return ret.second;
            }
        }
    } while( !done );

    add_msg( _( "Never mind." ) );
    return std::nullopt;
}

std::optional<tripoint_bub_ms> choose_adjacent( const std::string &message,
        const bool allow_vertical )
{
    return choose_adjacent( get_player_character().pos_bub(), message, allow_vertical );
}

std::optional<tripoint_bub_ms> choose_adjacent( const tripoint_bub_ms &pos,
        const std::string &message, bool allow_vertical, int timeout,
        const std::function<std::pair<bool, std::optional<tripoint_bub_ms>>(
            const input_context &ctxt, const std::string &action )> &action_cb )
{
    const std::optional<tripoint_rel_ms> dir = choose_direction(
                message, allow_vertical, /*allow_mouse=*/true, timeout,
    [&]( const input_context & ctxt, const std::string & action ) {
        if( action == "SELECT" ) {
            const std::optional<tripoint_bub_ms> mouse_pos = ctxt.get_coordinates(
                        g->w_terrain, g->ter_view_p.raw().xy(), true );
            if( mouse_pos ) {
                const tripoint_rel_ms vec = *mouse_pos - pos;
                if( vec.x() >= -1 && vec.x() <= 1
                    && vec.y() >= -1 && vec.y() <= 1
                    && ( allow_vertical ? vec.z() >= -1 && vec.y() <= 1 : vec.z() == 0 ) ) {
                    return std::pair<bool, std::optional<tripoint_rel_ms>>( true, vec );
                }
            }
        }
        if( action_cb ) {
            const std::pair<bool, std::optional<tripoint_bub_ms>> ret = action_cb( ctxt, action );
            if( ret.second.has_value() ) {
                return std::pair<bool, std::optional<tripoint_rel_ms>>( ret.first, *ret.second - pos );
            } else {
                return std::pair<bool, std::optional<tripoint_rel_ms>>( ret.first, std::nullopt );
            }
        }
        return std::pair<bool, std::optional<tripoint_rel_ms>>( false, std::nullopt );
    } );
    if( dir ) {
        return pos + *dir;
    } else {
        return std::nullopt;
    }
}

std::optional<tripoint_bub_ms> choose_adjacent_highlight( map &here, const std::string &message,
        const std::string &failure_message, const action_id action,
        const bool allow_vertical, const bool allow_autoselect )
{
    const std::function<bool( const tripoint_bub_ms & )> f = [&action,
    &here]( const tripoint_bub_ms & p ) {
        return can_interact_at( action, here, p );
    };
    return choose_adjacent_highlight( here, message, failure_message, f, allow_vertical,
                                      allow_autoselect );
}

std::optional<tripoint_bub_ms> choose_adjacent_highlight( map &here, const std::string &message,
        const std::string &failure_message, const std::function<bool( const tripoint_bub_ms & )> &allowed,
        const bool allow_vertical, const bool allow_autoselect )
{
    return choose_adjacent_highlight( here, get_avatar().pos_bub( here ), message, failure_message,
                                      allowed,
                                      allow_vertical, allow_autoselect );
}

std::optional<tripoint_bub_ms> choose_adjacent_highlight( map &here, const tripoint_bub_ms &pos,
        const std::string &message,
        const std::string &failure_message, const std::function<bool( const tripoint_bub_ms & )> &allowed,
        bool allow_vertical, bool allow_autoselect )
{
    std::vector<tripoint_bub_ms> valid;
    if( allowed ) {
        for( const tripoint_bub_ms &pos : here.points_in_radius( pos, 1 ) ) {
            if( allowed( pos ) ) {
                valid.emplace_back( pos );
            }
        }
    }

    const bool auto_select = allow_autoselect && get_option<bool>( "AUTOSELECT_SINGLE_VALID_TARGET" );
    if( valid.empty() && auto_select ) {
        add_msg( failure_message );
        return std::nullopt;
    } else if( valid.size() == 1 && auto_select ) {
        return valid.back();
    }

    shared_ptr_fast<game::draw_callback_t> hilite_cb;
    if( !valid.empty() ) {
        hilite_cb = make_shared_fast<game::draw_callback_t>( [&]() {
            for( const tripoint_bub_ms &pos : valid ) {
                here.drawsq( g->w_terrain, pos, drawsq_params().highlight( true ) );
            }
        } );
        g->add_draw_callback( hilite_cb );
    }

    const std::optional<tripoint_bub_ms> chosen = choose_adjacent( pos, message, allow_vertical );
    if( std::find( valid.begin(), valid.end(), chosen ) != valid.end() ) {
        return chosen;
    }

    return std::nullopt;
}
