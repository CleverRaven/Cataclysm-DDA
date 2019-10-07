#include "debug_menu.h"

// IWYU pragma: no_include <cxxabi.h>
#include <limits.h>
#include <stdint.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <array>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <unordered_map>

#include "action.h"
#include "avatar.h"
#include "coordinate_conversions.h"
#include "faction.h"
#include "filesystem.h"
#include "game.h"
#include "map_extras.h"
#include "messages.h"
#include "mission.h"
#include "morale_types.h"
#include "npc.h"
#include "npc_class.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "player.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "ui.h"
#include "vitamin.h"
#include "color.h"
#include "debug.h"
#include "enums.h"
#include "faction.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "omdata.h"
#include "optional.h"
#include "pldata.h"
#include "translations.h"
#include "type_id.h"
#include "map.h"
#include "veh_type.h"
#include "weather.h"
#include "recipe_dictionary.h"
#include "martialarts.h"
#include "sounds.h"
#include "trait_group.h"
#include "artifact.h"
#include "vpart_position.h"
#include "rng.h"
#include "signal.h"
#include "magic.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "clzones.h"
#include "compatibility.h"
#include "creature.h"
#include "cursesdef.h"
#include "input.h"
#include "item_group.h"
#include "monster.h"
#include "point.h"
#include "stomach.h"
#include "string_id.h"
#include "units.h"
#include "weather_gen.h"

class vehicle;

#if defined(TILES)
#include "sdl_wrappers.h"
#endif

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
const efftype_id effect_riding( "riding" );
namespace debug_menu
{

enum debug_menu_index {
    DEBUG_WISH,
    DEBUG_SHORT_TELEPORT,
    DEBUG_LONG_TELEPORT,
    DEBUG_REVEAL_MAP,
    DEBUG_SPAWN_NPC,
    DEBUG_SPAWN_MON,
    DEBUG_GAME_STATE,
    DEBUG_KILL_NPCS,
    DEBUG_MUTATE,
    DEBUG_SPAWN_VEHICLE,
    DEBUG_CHANGE_SKILLS,
    DEBUG_LEARN_MA,
    DEBUG_UNLOCK_RECIPES,
    DEBUG_EDIT_PLAYER,
    DEBUG_SPAWN_ARTIFACT,
    DEBUG_SPAWN_CLAIRVOYANCE,
    DEBUG_MAP_EDITOR,
    DEBUG_CHANGE_WEATHER,
    DEBUG_WIND_DIRECTION,
    DEBUG_WIND_SPEED,
    DEBUG_KILL_MONS,
    DEBUG_DISPLAY_HORDES,
    DEBUG_TEST_IT_GROUP,
    DEBUG_DAMAGE_SELF,
    DEBUG_SHOW_SOUND,
    DEBUG_DISPLAY_WEATHER,
    DEBUG_DISPLAY_SCENTS,
    DEBUG_CHANGE_TIME,
    DEBUG_SET_AUTOMOVE,
    DEBUG_SHOW_MUT_CAT,
    DEBUG_OM_EDITOR,
    DEBUG_BENCHMARK,
    DEBUG_OM_TELEPORT,
    DEBUG_TRAIT_GROUP,
    DEBUG_SHOW_MSG,
    DEBUG_CRASH_GAME,
    DEBUG_MAP_EXTRA,
    DEBUG_DISPLAY_NPC_PATH,
    DEBUG_PRINT_FACTION_INFO,
    DEBUG_QUIT_NOSAVE,
    DEBUG_TEST_WEATHER,
    DEBUG_SAVE_SCREENSHOT,
    DEBUG_GAME_REPORT,
    DEBUG_DISPLAY_SCENTS_LOCAL,
    DEBUG_DISPLAY_TEMP,
    DEBUG_DISPLAY_VISIBILITY,
    DEBUG_DISPLAY_RADIATION,
    DEBUG_LEARN_SPELLS,
    DEBUG_LEVEL_SPELLS
};

class mission_debug
{
    private:
        // Doesn't actually "destroy" the mission, just removes assignments
        static void remove_mission( mission &m );
    public:
        static void edit_mission( mission &m );
        static void edit( player &who );
        static void edit_player();
        static void edit_npc( npc &who );
        static std::string describe( const mission &m );
};

static int player_uilist()
{
    std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( DEBUG_MUTATE, true, 'M', _( "Mutate" ) ) },
        { uilist_entry( DEBUG_CHANGE_SKILLS, true, 's', _( "Change all skills" ) ) },
        { uilist_entry( DEBUG_LEARN_MA, true, 'l', _( "Learn all melee styles" ) ) },
        { uilist_entry( DEBUG_UNLOCK_RECIPES, true, 'r', _( "Unlock all recipes" ) ) },
        { uilist_entry( DEBUG_EDIT_PLAYER, true, 'p', _( "Edit player/NPC" ) ) },
        { uilist_entry( DEBUG_DAMAGE_SELF, true, 'd', _( "Damage self" ) ) },
        { uilist_entry( DEBUG_SET_AUTOMOVE, true, 'a', _( "Set automove route" ) ) },
    };
    if( !spell_type::get_all().empty() ) {
        uilist_initializer.emplace_back( uilist_entry( DEBUG_LEARN_SPELLS, true, 'S',
                                         _( "Learn all spells" ) ) );
        uilist_initializer.emplace_back( uilist_entry( DEBUG_LEVEL_SPELLS, true, 'L',
                                         _( "Level a spell" ) ) );
    }

    return uilist( _( "Player..." ), uilist_initializer );
}

static int info_uilist( bool display_all_entries = true )
{
    // always displayed
    std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( DEBUG_SAVE_SCREENSHOT, true, 'H', _( "Take screenshot" ) ) },
        { uilist_entry( DEBUG_GAME_REPORT, true, 'r', _( "Generate game report" ) ) },
    };

    if( display_all_entries ) {
        const std::vector<uilist_entry> debug_only_options = {
            { uilist_entry( DEBUG_GAME_STATE, true, 'g', _( "Check game state" ) ) },
            { uilist_entry( DEBUG_DISPLAY_HORDES, true, 'h', _( "Display hordes" ) ) },
            { uilist_entry( DEBUG_TEST_IT_GROUP, true, 'i', _( "Test item group" ) ) },
            { uilist_entry( DEBUG_SHOW_SOUND, true, 'c', _( "Show sound clustering" ) ) },
            { uilist_entry( DEBUG_DISPLAY_WEATHER, true, 'w', _( "Display weather" ) ) },
            { uilist_entry( DEBUG_DISPLAY_SCENTS, true, 'S', _( "Display overmap scents" ) ) },
            { uilist_entry( DEBUG_DISPLAY_SCENTS_LOCAL, true, 's', _( "Toggle display local scents" ) ) },
            { uilist_entry( DEBUG_DISPLAY_TEMP, true, 'T', _( "Toggle display temperature" ) ) },
            { uilist_entry( DEBUG_DISPLAY_VISIBILITY, true, 'v', _( "Toggle display visibility" ) ) },
            { uilist_entry( DEBUG_DISPLAY_RADIATION, true, 'R', _( "Toggle display radiation" ) ) },
            { uilist_entry( DEBUG_SHOW_MUT_CAT, true, 'm', _( "Show mutation category levels" ) ) },
            { uilist_entry( DEBUG_BENCHMARK, true, 'b', _( "Draw benchmark (X seconds)" ) ) },
            { uilist_entry( DEBUG_TRAIT_GROUP, true, 't', _( "Test trait group" ) ) },
            { uilist_entry( DEBUG_SHOW_MSG, true, 'd', _( "Show debug message" ) ) },
            { uilist_entry( DEBUG_CRASH_GAME, true, 'C', _( "Crash game (test crash handling)" ) ) },
            { uilist_entry( DEBUG_DISPLAY_NPC_PATH, true, 'n', _( "Toggle NPC pathfinding on map" ) ) },
            { uilist_entry( DEBUG_PRINT_FACTION_INFO, true, 'f', _( "Print faction info to console" ) ) },
            { uilist_entry( DEBUG_TEST_WEATHER, true, 'W', _( "Test weather" ) ) },
        };
        uilist_initializer.insert( uilist_initializer.begin(), debug_only_options.begin(),
                                   debug_only_options.end() );
    }

    return uilist( _( "Info..." ), uilist_initializer );
}

static int teleport_uilist()
{
    const std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( DEBUG_SHORT_TELEPORT, true, 's', _( "Teleport - short range" ) ) },
        { uilist_entry( DEBUG_LONG_TELEPORT, true, 'l', _( "Teleport - long range" ) ) },
        { uilist_entry( DEBUG_OM_TELEPORT, true, 'o', _( "Teleport - adjacent overmap" ) ) },
    };

    return uilist( _( "Teleport..." ), uilist_initializer );
}

static int spawning_uilist()
{
    const std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( DEBUG_WISH, true, 'w', _( "Spawn an item" ) ) },
        { uilist_entry( DEBUG_SPAWN_NPC, true, 'n', _( "Spawn NPC" ) ) },
        { uilist_entry( DEBUG_SPAWN_MON, true, 'm', _( "Spawn monster" ) ) },
        { uilist_entry( DEBUG_SPAWN_VEHICLE, true, 'v', _( "Spawn a vehicle" ) ) },
        { uilist_entry( DEBUG_SPAWN_ARTIFACT, true, 'a', _( "Spawn artifact" ) ) },
        { uilist_entry( DEBUG_SPAWN_CLAIRVOYANCE, true, 'c', _( "Spawn clairvoyance artifact" ) ) },
    };

    return uilist( _( "Spawning..." ), uilist_initializer );
}

static int map_uilist()
{
    const std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( DEBUG_REVEAL_MAP, true, 'r', _( "Reveal map" ) ) },
        { uilist_entry( DEBUG_KILL_NPCS, true, 'k', _( "Kill NPCs" ) ) },
        { uilist_entry( DEBUG_MAP_EDITOR, true, 'M', _( "Map editor" ) ) },
        { uilist_entry( DEBUG_CHANGE_WEATHER, true, 'w', _( "Change weather" ) ) },
        { uilist_entry( DEBUG_WIND_DIRECTION, true, 'd', _( "Change wind direction" ) ) },
        { uilist_entry( DEBUG_WIND_SPEED, true, 's', _( "Change wind speed" ) ) },
        { uilist_entry( DEBUG_KILL_MONS, true, 'K', _( "Kill all monsters" ) ) },
        { uilist_entry( DEBUG_CHANGE_TIME, true, 't', _( "Change time" ) ) },
        { uilist_entry( DEBUG_OM_EDITOR, true, 'O', _( "Overmap editor" ) ) },
        { uilist_entry( DEBUG_MAP_EXTRA, true, 'm', _( "Spawn map extra" ) ) },
    };

    return uilist( _( "Map..." ), uilist_initializer );
}

/**
 * Create the debug menu UI list.
 * @param display_all_entries: `true` if all entries should be displayed, `false` is some entries should be hidden (for ex. when the debug menu is called from the main menu).
 *   This allows to have some menu elements at the same time in the main menu and in the debug menu.
 * @returns The chosen action.
 */
static int debug_menu_uilist( bool display_all_entries = true )
{
    std::vector<uilist_entry> menu = {
        { uilist_entry( 1, true, 'i', _( "Info..." ) ) },
    };

    if( display_all_entries ) {
        const std::vector<uilist_entry> debug_menu = {
            { uilist_entry( DEBUG_QUIT_NOSAVE, true, 'Q', _( "Quit to main menu" ) )  },
            { uilist_entry( 2, true, 's', _( "Spawning..." ) ) },
            { uilist_entry( 3, true, 'p', _( "Player..." ) ) },
            { uilist_entry( 4, true, 't', _( "Teleport..." ) ) },
            { uilist_entry( 5, true, 'm', _( "Map..." ) ) },
        };

        // insert debug-only menu right after "Info".
        menu.insert( menu.begin() + 1, debug_menu.begin(), debug_menu.end() );
    }

    std::string msg;
    if( display_all_entries ) {
        msg = _( "Debug Functions - Using these will cheat not only the game, but yourself.\nYou won't grow. You won't improve.\nTaking this shortcut will gain you nothing. Your victory will be hollow.\nNothing will be risked and nothing will be gained." );
    } else {
        msg = _( "Debug Functions" );
    }

    while( true ) {
        const int group = uilist( msg, menu );

        int action;

        switch( group ) {
            case DEBUG_QUIT_NOSAVE:
                action = DEBUG_QUIT_NOSAVE;
                break;
            case 1:
                action = info_uilist( display_all_entries );
                break;
            case 2:
                action = spawning_uilist();
                break;
            case 3:
                action = player_uilist();
                break;
            case 4:
                action = teleport_uilist();
                break;
            case 5:
                action = map_uilist();
                break;

            default:
                return group;
        }
        if( action >= 0 ) {
            return action;
        }
    }
}

void teleport_short()
{
    const cata::optional<tripoint> where = g->look_around();
    if( !where || *where == g->u.pos() ) {
        return;
    }
    g->place_player( *where );
    const tripoint new_pos( g->u.pos() );
    add_msg( _( "You teleport to point (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void teleport_long()
{
    const tripoint where( ui::omap::choose_point() );
    if( where == overmap::invalid_tripoint ) {
        return;
    }
    g->place_player_overmap( where );
    add_msg( _( "You teleport to submap (%d,%d,%d)." ), where.x, where.y, where.z );
}

void teleport_overmap()
{
    const cata::optional<tripoint> dir_ = choose_direction( _( "Where is the desired overmap?" ) );
    if( !dir_ ) {
        return;
    }

    const tripoint offset( OMAPX * dir_->x, OMAPY * dir_->y, dir_->z );
    const tripoint where( g->u.global_omt_location() + offset );

    g->place_player_overmap( where );

    const tripoint new_pos( omt_to_om_copy( g->u.global_omt_location() ) );
    add_msg( _( "You teleport to overmap (%d,%d,%d)." ), new_pos.x, new_pos.y, new_pos.z );
}

void character_edit_menu()
{
    std::vector< tripoint > locations;
    uilist charmenu;
    int charnum = 0;
    charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, "%s", _( "You" ) );
    locations.emplace_back( g->u.pos() );
    for( const npc &guy : g->all_npcs() ) {
        charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, guy.name );
        locations.emplace_back( guy.pos() );
    }

    pointmenu_cb callback( locations );
    charmenu.callback = &callback;
    charmenu.w_y = 0;
    charmenu.query();
    if( charmenu.ret < 0 || static_cast<size_t>( charmenu.ret ) >= locations.size() ) {
        return;
    }
    const size_t index = charmenu.ret;
    // The NPC is also required for "Add mission", so has to be in this scope
    npc *np = g->critter_at<npc>( locations[index], false );
    player &p = np ? static_cast<player &>( *np ) : static_cast<player &>( g->u );
    uilist nmenu;

    if( np != nullptr ) {
        std::stringstream data;
        data << np->name << " " << ( np->male ? _( "Male" ) : _( "Female" ) ) << std::endl;
        data << np->myclass.obj().get_name() << "; " <<
             npc_attitude_name( np->get_attitude() ) << "; " <<
             ( np->get_faction() ? np->get_faction()->name : _( "no faction" ) ) << "; " <<
             ( np->get_faction() ? np->get_faction()->currency : _( "no currency" ) ) << "; " <<
             "api: " << np->get_faction_ver() << std::endl;
        if( np->has_destination() ) {
            data << string_format( _( "Destination: %d:%d:%d (%s)" ),
                                   np->goal.x, np->goal.y, np->goal.z,
                                   overmap_buffer.ter( np->goal )->get_name() ) << std::endl;
        } else {
            data << _( "No destination." ) << std::endl;
        }
        data << string_format( _( "Trust: %d" ), np->op_of_u.trust ) << " "
             << string_format( _( "Fear: %d" ), np->op_of_u.fear ) << " "
             << string_format( _( "Value: %d" ), np->op_of_u.value ) << " "
             << string_format( _( "Anger: %d" ), np->op_of_u.anger ) << " "
             << string_format( _( "Owed: %d" ), np->op_of_u.owed ) << std::endl;

        data << string_format( _( "Aggression: %d" ),
                               static_cast<int>( np->personality.aggression ) ) << " "
             << string_format( _( "Bravery: %d" ), static_cast<int>( np->personality.bravery ) ) << " "
             << string_format( _( "Collector: %d" ), static_cast<int>( np->personality.collector ) ) << " "
             << string_format( _( "Altruism: %d" ), static_cast<int>( np->personality.altruism ) ) << std::endl;

        data << _( "Needs:" ) << std::endl;
        for( const auto &need : np->needs ) {
            data << need << std::endl;
        }
        data << string_format( _( "Total morale: %d" ), np->get_morale_level() ) << std::endl;

        nmenu.text = data.str();
    } else {
        nmenu.text = _( "Player" );
    }

    enum {
        D_NAME, D_SKILLS, D_STATS, D_ITEMS, D_DELETE_ITEMS, D_ITEM_WORN,
        D_HP, D_STAMINA, D_MORALE, D_PAIN, D_NEEDS, D_HEALTHY, D_STATUS, D_MISSION_ADD, D_MISSION_EDIT,
        D_TELE, D_MUTATE, D_CLASS, D_ATTITUDE, D_OPINION
    };
    nmenu.addentry( D_NAME, true, 'N', "%s", _( "Edit [N]ame" ) );
    nmenu.addentry( D_SKILLS, true, 's', "%s", _( "Edit [s]kills" ) );
    nmenu.addentry( D_STATS, true, 't', "%s", _( "Edit s[t]ats" ) );
    nmenu.addentry( D_ITEMS, true, 'i', "%s", _( "Grant [i]tems" ) );
    nmenu.addentry( D_DELETE_ITEMS, true, 'd', "%s", _( "[d]elete (all) items" ) );
    nmenu.addentry( D_ITEM_WORN, true, 'w', "%s",
                    _( "[w]ear/[w]ield an item from player's inventory" ) );
    nmenu.addentry( D_HP, true, 'h', "%s", _( "Set [h]it points" ) );
    nmenu.addentry( D_STAMINA, true, 'S', "%s", _( "Set [S]tamina" ) );
    nmenu.addentry( D_MORALE, true, 'o', "%s", _( "Set m[o]rale" ) );
    nmenu.addentry( D_PAIN, true, 'p', "%s", _( "Cause [p]ain" ) );
    nmenu.addentry( D_HEALTHY, true, 'a', "%s", _( "Set he[a]lth" ) );
    nmenu.addentry( D_NEEDS, true, 'n', "%s", _( "Set [n]eeds" ) );
    nmenu.addentry( D_MUTATE, true, 'u', "%s", _( "M[u]tate" ) );
    nmenu.addentry( D_STATUS, true, '@', "%s", _( "Status Window [@]" ) );
    nmenu.addentry( D_TELE, true, 'e', "%s", _( "t[e]leport" ) );
    nmenu.addentry( D_MISSION_EDIT, true, 'M', "%s", _( "Edit [M]issions (WARNING: Unstable!)" ) );
    if( p.is_npc() ) {
        nmenu.addentry( D_MISSION_ADD, true, 'm', "%s", _( "Add [m]ission" ) );
        nmenu.addentry( D_CLASS, true, 'c', "%s", _( "Randomize with [c]lass" ) );
        nmenu.addentry( D_ATTITUDE, true, 'A', "%s", _( "Set [A]ttitude" ) );
        nmenu.addentry( D_OPINION, true, 'O', "%s", _( "Set [O]pinion" ) );
    }
    nmenu.query();
    switch( nmenu.ret ) {
        case D_SKILLS:
            wishskill( &p );
            break;
        case D_STATS: {
            uilist smenu;
            smenu.addentry( 0, true, 'S', "%s: %d", _( "Maximum strength" ), p.str_max );
            smenu.addentry( 1, true, 'D', "%s: %d", _( "Maximum dexterity" ), p.dex_max );
            smenu.addentry( 2, true, 'I', "%s: %d", _( "Maximum intelligence" ), p.int_max );
            smenu.addentry( 3, true, 'P', "%s: %d", _( "Maximum perception" ), p.per_max );
            smenu.query();
            int *bp_ptr = nullptr;
            switch( smenu.ret ) {
                case 0:
                    bp_ptr = &p.str_max;
                    break;
                case 1:
                    bp_ptr = &p.dex_max;
                    break;
                case 2:
                    bp_ptr = &p.int_max;
                    break;
                case 3:
                    bp_ptr = &p.per_max;
                    break;
                default:
                    break;
            }

            if( bp_ptr != nullptr ) {
                int value;
                if( query_int( value, _( "Set the stat to? Currently: %d" ), *bp_ptr ) && value >= 0 ) {
                    *bp_ptr = value;
                    p.reset_stats();
                }
            }
        }
        break;
        case D_ITEMS:
            wishitem( &p );
            break;
        case D_DELETE_ITEMS:
            if( !query_yn( _( "Delete all items from the target?" ) ) ) {
                break;
            }
            for( auto &it : p.worn ) {
                it.on_takeoff( p );
            }
            p.worn.clear();
            p.inv.clear();
            p.weapon = item();
            break;
        case D_ITEM_WORN: {
            int item_pos = g->inv_for_all( _( "Make target equip" ) );
            item &to_wear = g->u.i_at( item_pos );
            if( to_wear.is_armor() ) {
                p.on_item_wear( to_wear );
                p.worn.push_back( to_wear );
            } else if( !to_wear.is_null() ) {
                p.weapon = to_wear;
            }
        }
        break;
        case D_HP: {
            uilist smenu;
            smenu.addentry( 0, true, 'q', "%s: %d", _( "Torso" ), p.hp_cur[hp_torso] );
            smenu.addentry( 1, true, 'w', "%s: %d", _( "Head" ), p.hp_cur[hp_head] );
            smenu.addentry( 2, true, 'a', "%s: %d", _( "Left arm" ), p.hp_cur[hp_arm_l] );
            smenu.addentry( 3, true, 's', "%s: %d", _( "Right arm" ), p.hp_cur[hp_arm_r] );
            smenu.addentry( 4, true, 'z', "%s: %d", _( "Left leg" ), p.hp_cur[hp_leg_l] );
            smenu.addentry( 5, true, 'x', "%s: %d", _( "Right leg" ), p.hp_cur[hp_leg_r] );
            smenu.query();
            int *bp_ptr = nullptr;
            switch( smenu.ret ) {
                case 0:
                    bp_ptr = &p.hp_cur[hp_torso];
                    break;
                case 1:
                    bp_ptr = &p.hp_cur[hp_head];
                    break;
                case 2:
                    bp_ptr = &p.hp_cur[hp_arm_l];
                    break;
                case 3:
                    bp_ptr = &p.hp_cur[hp_arm_r];
                    break;
                case 4:
                    bp_ptr = &p.hp_cur[hp_leg_l];
                    break;
                case 5:
                    bp_ptr = &p.hp_cur[hp_leg_r];
                    break;
                default:
                    break;
            }

            if( bp_ptr != nullptr ) {
                int value;
                if( query_int( value, _( "Set the hitpoints to? Currently: %d" ), *bp_ptr ) && value >= 0 ) {
                    *bp_ptr = value;
                    p.reset_stats();
                }
            }
        }
        break;
        case D_STAMINA:
            int value;
            if( query_int( value, _( "Set stamina to? Current: %d. Max: %d." ), p.stamina,
                           p.get_stamina_max() ) ) {
                if( value >= 0 && value <= p.get_stamina_max() ) {
                    p.stamina = value;
                } else {
                    add_msg( m_bad, _( "Target stamina value out of bounds!" ) );
                }
            }
            break;
        case D_MORALE: {
            int current_morale_level = p.get_morale_level();
            int value;
            if( query_int( value, _( "Set the morale to? Currently: %d" ), current_morale_level ) ) {
                int morale_level_delta = value - current_morale_level;
                p.add_morale( MORALE_PERM_DEBUG, morale_level_delta );
                p.apply_persistent_morale();
            }
        }
        break;
        case D_OPINION: {
            uilist smenu;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "trust" ), np->op_of_u.trust );
            smenu.addentry( 1, true, 's', "%s: %d", _( "fear" ), np->op_of_u.fear );
            smenu.addentry( 2, true, 't', "%s: %d", _( "value" ), np->op_of_u.value );
            smenu.addentry( 3, true, 'f', "%s: %d", _( "anger" ), np->op_of_u.anger );
            smenu.addentry( 4, true, 'd', "%s: %d", _( "owed" ), np->op_of_u.owed );

            smenu.query();
            int value;
            switch( smenu.ret ) {
                case 0:
                    if( query_int( value, _( "Set trust to? Currently: %d" ),
                                   np->op_of_u.trust ) ) {
                        np->op_of_u.trust = value;
                    }
                    break;
                case 1:
                    if( query_int( value, _( "Set fear to? Currently: %d" ), np->op_of_u.fear ) ) {
                        np->op_of_u.fear = value;
                    }
                    break;
                case 2:
                    if( query_int( value, _( "Set value to? Currently: %d" ),
                                   np->op_of_u.value ) ) {
                        np->op_of_u.value = value;
                    }
                    break;
                case 3:
                    if( query_int( value, _( "Set anger to? Currently: %d" ),
                                   np->op_of_u.anger ) ) {
                        np->op_of_u.anger = value;
                    }
                    break;
                case 4:
                    if( query_int( value, _( "Set owed to? Currently: %d" ), np->op_of_u.owed ) ) {
                        np->op_of_u.owed = value;
                    }
                    break;
            }
        }
        break;
        case D_NAME: {
            std::string filterstring = p.name;
            string_input_popup popup;
            popup
            .title( _( "Rename:" ) )
            .width( 85 )
            .description( string_format( _( "NPC: \n%s\n" ), p.name ) )
            .edit( filterstring );
            if( popup.confirmed() ) {
                p.name = filterstring;
            }
        }
        break;
        case D_PAIN: {
            int value;
            if( query_int( value, _( "Cause how much pain? pain: %d" ), p.get_pain() ) ) {
                p.mod_pain( value );
            }
        }
        break;
        case D_NEEDS: {
            uilist smenu;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "Hunger" ), p.get_hunger() );
            smenu.addentry( 1, true, 's', "%s: %d", _( "Stored kCal" ), p.get_stored_kcal() );
            smenu.addentry( 2, true, 't', "%s: %d", _( "Thirst" ), p.get_thirst() );
            smenu.addentry( 3, true, 'f', "%s: %d", _( "Fatigue" ), p.get_fatigue() );
            smenu.addentry( 4, true, 'd', "%s: %d", _( "Sleep Deprivation" ), p.get_sleep_deprivation() );
            smenu.addentry( 5, true, 'a', _( "Reset all basic needs" ) );

            const auto &vits = vitamin::all();
            for( const auto &v : vits ) {
                smenu.addentry( -1, true, 0, "%s: %d", v.second.name(), p.vitamin_get( v.first ) );
            }

            smenu.query();
            int value;
            switch( smenu.ret ) {
                case 0:
                    if( query_int( value, _( "Set hunger to? Currently: %d" ), p.get_hunger() ) ) {
                        p.set_hunger( value );
                    }
                    break;

                case 1:
                    if( query_int( value, _( "Set stored kCal to? Currently: %d" ), p.get_stored_kcal() ) ) {
                        p.set_stored_kcal( value );
                    }
                    break;

                case 2:
                    if( query_int( value, _( "Set thirst to? Currently: %d" ), p.get_thirst() ) ) {
                        p.set_thirst( value );
                    }
                    break;

                case 3:
                    if( query_int( value, _( "Set fatigue to? Currently: %d" ), p.get_fatigue() ) ) {
                        p.set_fatigue( value );
                    }
                    break;

                case 4:
                    if( query_int( value, _( "Set sleep deprivation to? Currently: %d" ),
                                   p.get_sleep_deprivation() ) ) {
                        p.set_sleep_deprivation( value );
                    }
                    break;
                case 5:
                    p.initialize_stomach_contents();
                    p.set_hunger( 0 );
                    p.set_thirst( 0 );
                    p.set_fatigue( 0 );
                    p.set_sleep_deprivation( 0 );
                    p.set_stored_kcal( p.get_healthy_kcal() );
                    break;
                default:
                    if( smenu.ret >= 6 && smenu.ret < static_cast<int>( vits.size() + 6 ) ) {
                        auto iter = std::next( vits.begin(), smenu.ret - 6 );
                        if( query_int( value, _( "Set %s to? Currently: %d" ),
                                       iter->second.name(), p.vitamin_get( iter->first ) ) ) {
                            p.vitamin_set( iter->first, value );
                        }
                    }
            }

        }
        break;
        case D_MUTATE:
            wishmutate( &p );
            break;
        case D_HEALTHY: {
            uilist smenu;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "Health" ), p.get_healthy() );
            smenu.addentry( 1, true, 'm', "%s: %d", _( "Health modifier" ), p.get_healthy_mod() );
            smenu.addentry( 2, true, 'r', "%s: %d", _( "Radiation" ), p.radiation );
            smenu.query();
            int value;
            switch( smenu.ret ) {
                case 0:
                    if( query_int( value, _( "Set the value to? Currently: %d" ), p.get_healthy() ) ) {
                        p.set_healthy( value );
                    }
                    break;
                case 1:
                    if( query_int( value, _( "Set the value to? Currently: %d" ), p.get_healthy_mod() ) ) {
                        p.set_healthy_mod( value );
                    }
                    break;
                case 2:
                    if( query_int( value, _( "Set the value to? Currently: %d" ), p.radiation ) ) {
                        p.radiation = value;
                    }
                    break;
                default:
                    break;
            }
        }
        break;
        case D_STATUS:
            p.disp_info();
            break;
        case D_MISSION_ADD: {
            uilist types;
            types.text = _( "Choose mission type" );
            const auto all_missions = mission_type::get_all();
            std::vector<const mission_type *> mts;
            for( size_t i = 0; i < all_missions.size(); i++ ) {
                types.addentry( i, true, -1, all_missions[i].tname() );
                mts.push_back( &all_missions[i] );
            }

            types.query();
            if( types.ret >= 0 && types.ret < static_cast<int>( mts.size() ) ) {
                np->add_new_mission( mission::reserve_new( mts[types.ret]->id, np->getID() ) );
            }
        }
        break;
        case D_MISSION_EDIT:
            mission_debug::edit( p );
            break;
        case D_TELE: {
            if( const cata::optional<tripoint> newpos = g->look_around() ) {
                p.setpos( *newpos );
                if( p.is_player() ) {
                    if( p.is_mounted() ) {
                        p.mounted_creature->setpos( *newpos );
                    }
                    g->update_map( g->u );
                }
            }
        }
        break;
        case D_CLASS: {
            uilist classes;
            classes.text = _( "Choose new class" );
            std::vector<npc_class_id> ids;
            size_t i = 0;
            for( auto &cl : npc_class::get_all() ) {
                ids.push_back( cl.id );
                classes.addentry( i, true, -1, cl.get_name() );
                i++;
            }

            classes.query();
            if( classes.ret < static_cast<int>( ids.size() ) && classes.ret >= 0 ) {
                np->randomize( ids[classes.ret] );
            }
        }
        break;
        case D_ATTITUDE: {
            uilist attitudes_ui;
            attitudes_ui.text = _( "Choose new attitude" );
            std::vector<npc_attitude> attitudes;
            for( int i = NPCATT_NULL; i < NPCATT_END; i++ ) {
                npc_attitude att_id = static_cast<npc_attitude>( i );
                std::string att_name = npc_attitude_name( att_id );
                attitudes.push_back( att_id );
                if( att_name == _( "Unknown attitude" ) ) {
                    continue;
                }

                attitudes_ui.addentry( i, true, -1, att_name );
            }

            attitudes_ui.query();
            if( attitudes_ui.ret < static_cast<int>( attitudes.size() ) && attitudes_ui.ret >= 0 ) {
                np->set_attitude( attitudes[attitudes_ui.ret] );
            }
        }
    }
}

static std::string mission_status_string( mission::mission_status status )
{
    static const std::map<mission::mission_status, std::string> desc{ {
            { mission::mission_status::yet_to_start, translate_marker( "Yet to start" ) },
            { mission::mission_status::in_progress, translate_marker( "In progress" ) },
            { mission::mission_status::success, translate_marker( "Success" ) },
            { mission::mission_status::failure, translate_marker( "Failure" ) }
        }
    };

    const auto &iter = desc.find( status );
    if( iter != desc.end() ) {
        return _( iter->second );
    }

    return _( "Bugged" );
}

std::string mission_debug::describe( const mission &m )
{
    std::stringstream data;
    data << _( "Type:" ) << m.type->id.str();
    data << _( " Status:" ) << mission_status_string( m.status );
    data << _( " ID:" ) << m.uid;
    data << _( " NPC ID:" ) << m.npc_id;
    data << _( " Target:" ) << m.target.x << "," << m.target.y << "," << m.target.z;
    data << _( "Player ID:" ) << m.player_id;

    return data.str();
}

static void add_header( uilist &mmenu, const std::string &str )
{
    if( !mmenu.entries.empty() ) {
        mmenu.addentry( -1, false, -1, "" );
    }
    uilist_entry header( -1, false, -1, str, c_yellow, c_yellow );
    header.force_color = true;
    mmenu.entries.push_back( header );
}

void mission_debug::edit( player &who )
{
    if( who.is_player() ) {
        edit_player();
    } else if( who.is_npc() ) {
        edit_npc( dynamic_cast<npc &>( who ) );
    }
}

void mission_debug::edit_npc( npc &who )
{
    npc_chatbin &bin = who.chatbin;
    std::vector<mission *> all_missions;

    uilist mmenu;
    mmenu.text = _( "Select mission to edit" );

    add_header( mmenu, _( "Currently assigned missions:" ) );
    for( mission *m : bin.missions_assigned ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Not assigned missions:" ) );
    for( mission *m : bin.missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    mmenu.query();
    if( mmenu.ret < 0 || mmenu.ret >= static_cast<int>( all_missions.size() ) ) {
        return;
    }

    edit_mission( *all_missions[mmenu.ret] );
}

void mission_debug::edit_player()
{
    std::vector<mission *> all_missions;

    uilist mmenu;
    mmenu.text = _( "Select mission to edit" );

    add_header( mmenu, _( "Active missions:" ) );
    for( mission *m : g->u.active_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Completed missions:" ) );
    for( mission *m : g->u.completed_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Failed missions:" ) );
    for( mission *m : g->u.failed_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    mmenu.query();
    if( mmenu.ret < 0 || mmenu.ret >= static_cast<int>( all_missions.size() ) ) {
        return;
    }

    edit_mission( *all_missions[mmenu.ret] );
}

static bool remove_from_vec( std::vector<mission *> &vec, mission *m )
{
    auto iter = std::remove( vec.begin(), vec.end(), m );
    bool ret = iter != vec.end();
    vec.erase( iter, vec.end() );
    return ret;
}

void mission_debug::remove_mission( mission &m )
{
    if( remove_from_vec( g->u.active_missions, &m ) ) {
        add_msg( _( "Removing from active_missions" ) );
    }
    if( remove_from_vec( g->u.completed_missions, &m ) ) {
        add_msg( _( "Removing from completed_missions" ) );
    }
    if( remove_from_vec( g->u.failed_missions, &m ) ) {
        add_msg( _( "Removing from failed_missions" ) );
    }

    if( g->u.active_mission == &m ) {
        g->u.active_mission = nullptr;
        add_msg( _( "Unsetting active mission" ) );
    }

    const auto giver = g->find_npc( m.npc_id );
    if( giver != nullptr ) {
        if( remove_from_vec( giver->chatbin.missions_assigned, &m ) ) {
            add_msg( _( "Removing from %s missions_assigned" ), giver->name );
        }
        if( remove_from_vec( giver->chatbin.missions, &m ) ) {
            add_msg( _( "Removing from %s missions" ), giver->name );
        }
    }
}

void mission_debug::edit_mission( mission &m )
{
    uilist mmenu;
    mmenu.text = describe( m );

    enum {
        M_FAIL, M_SUCCEED, M_REMOVE
    };

    mmenu.addentry( M_FAIL, true, 'f', "%s", _( "Fail mission" ) );
    mmenu.addentry( M_SUCCEED, true, 'c', "%s", _( "Mark as complete" ) );
    mmenu.addentry( M_REMOVE, true, 'r', "%s", _( "Remove mission without proper cleanup" ) );

    mmenu.query();
    switch( mmenu.ret ) {
        case M_FAIL:
            m.fail();
            break;
        case M_SUCCEED:
            m.status = mission::mission_status::success;
            break;
        case M_REMOVE:
            remove_mission( m );
            break;
    }
}

void draw_benchmark( const int max_difference )
{
    // call the draw procedure as many times as possible in max_difference milliseconds
    auto start_tick = std::chrono::steady_clock::now();
    auto end_tick = std::chrono::steady_clock::now();
    int64_t difference = 0;
    int draw_counter = 0;
    while( true ) {
        end_tick = std::chrono::steady_clock::now();
        difference = std::chrono::duration_cast<std::chrono::milliseconds>( end_tick - start_tick ).count();
        if( difference >= max_difference ) {
            break;
        }
        g->draw();
        draw_counter++;
    }

    DebugLog( D_INFO, DC_ALL ) << "Draw benchmark:\n" <<
                               "\n| USE_TILES |  RENDERER | FRAMEBUFFER_ACCEL | USE_COLOR_MODULATED_TEXTURES | FPS |" <<
                               "\n|:---:|:---:|:---:|:---:|:---:|\n| " <<
                               get_option<bool>( "USE_TILES" ) << " | " <<
#if !defined(__ANDROID__)
                               get_option<std::string>( "RENDERER" ) << " | " <<
#else
                               get_option<bool>( "SOFTWARE_RENDERING" ) << " | " <<
#endif
                               get_option<bool>( "FRAMEBUFFER_ACCEL" ) << " | " <<
                               get_option<bool>( "USE_COLOR_MODULATED_TEXTURES" ) << " | " <<
                               static_cast<int>( 1000.0 * draw_counter / static_cast<double>( difference ) ) << " |\n";

    add_msg( m_info, _( "Drew %d times in %.3f seconds. (%.3f fps average)" ), draw_counter,
             difference / 1000.0, 1000.0 * draw_counter / static_cast<double>( difference ) );
}

void debug()
{
    bool debug_menu_has_hotkey = hotkey_for_action( ACTION_DEBUG, false ) != -1;
    int action = debug_menu_uilist( debug_menu_has_hotkey );
    g->refresh_all();
    avatar &u = g->u;
    map &m = g->m;
    switch( action ) {
        case DEBUG_WISH:
            debug_menu::wishitem( &u );
            break;

        case DEBUG_SHORT_TELEPORT:
            debug_menu::teleport_short();
            break;

        case DEBUG_LONG_TELEPORT:
            debug_menu::teleport_long();
            break;

        case DEBUG_REVEAL_MAP: {
            auto &cur_om = g->get_cur_om();
            for( int i = 0; i < OMAPX; i++ ) {
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                        cur_om.seen( { i, j, k } ) = true;
                    }
                }
            }
            add_msg( m_good, _( "Current overmap revealed." ) );
        }
        break;

        case DEBUG_SPAWN_NPC: {
            std::shared_ptr<npc> temp = std::make_shared<npc>();
            temp->normalize();
            temp->randomize();
            temp->spawn_at_precise( { g->get_levx(), g->get_levy() }, u.pos() + point( -4, -4 ) );
            overmap_buffer.insert_npc( temp );
            temp->form_opinion( u );
            temp->mission = NPC_MISSION_NULL;
            temp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, temp->global_omt_location(),
                                   temp->getID() ) );
            std::string new_fac_id = "solo_";
            new_fac_id += temp->name;
            // create a new "lone wolf" faction for this one NPC
            faction *new_solo_fac = g->faction_manager_ptr->add_new_faction( temp->name,
                                    faction_id( new_fac_id ), faction_id( "no_faction" ) );
            temp->set_fac( new_solo_fac ? new_solo_fac->id : faction_id( "no_faction" ) );
            g->load_npcs();
        }
        break;

        case DEBUG_SPAWN_MON:
            debug_menu::wishmonster( cata::nullopt );
            break;

        case DEBUG_GAME_STATE: {
            std::string s = _( "Location %d:%d in %d:%d, %s\n" );
            s += _( "Current turn: %d.\n%s\n" );
            s += ngettext( "%d creature exists.\n", "%d creatures exist.\n", g->num_creatures() );
            popup_top(
                s.c_str(),
                u.posx(), g->u.posy(), g->get_levx(), g->get_levy(),
                overmap_buffer.ter( g->u.global_omt_location() )->get_name(),
                to_turns<int>( calendar::turn - calendar::turn_zero ),
                get_option<bool>( "RANDOM_NPC" ) ? _( "NPCs are going to spawn." ) :
                _( "NPCs are NOT going to spawn." ),
                g->num_creatures() );
            for( const npc &guy : g->all_npcs() ) {
                tripoint t = guy.global_sm_location();
                add_msg( m_info, _( "%s: map ( %d:%d ) pos ( %d:%d )" ), guy.name, t.x,
                         t.y, guy.posx(), guy.posy() );
            }

            add_msg( m_info, _( "(you: %d:%d)" ), u.posx(), u.posy() );
            std::string stom =
                _( "Stomach Contents: %d ml / %d ml kCal: %d, Water: %d ml" );
            add_msg( m_info, stom.c_str(), units::to_milliliter( u.stomach.contains() ),
                     units::to_milliliter( u.stomach.capacity() ), u.stomach.get_calories(),
                     units::to_milliliter( u.stomach.get_water() ), u.get_hunger() );
            stom = _( "Guts Contents: %d ml / %d ml kCal: %d, Water: %d ml\nHunger: %d, Thirst: %d, kCal: %d / %d" );
            add_msg( m_info, stom.c_str(), units::to_milliliter( u.guts.contains() ),
                     units::to_milliliter( u.guts.capacity() ), u.guts.get_calories(),
                     units::to_milliliter( u.guts.get_water() ), u.get_hunger(), u.get_thirst(), u.get_stored_kcal(),
                     u.get_healthy_kcal() );
            add_msg( m_info, _( "Body Mass Index: %.0f\nBasal Metabolic Rate: %i" ), u.get_bmi(), u.get_bmr() );
            add_msg( m_info, _( "Player activity level: %s" ), u.activity_level_str() );
            if( get_option<bool>( "STATS_THROUGH_KILLS" ) ) {
                add_msg( m_info, _( "Kill xp: %d" ), u.kill_xp() );
            }
            g->disp_NPCs();
            break;
        }
        case DEBUG_KILL_NPCS:
            for( npc &guy : g->all_npcs() ) {
                add_msg( _( "%s's head implodes!" ), guy.name );
                guy.hp_cur[bp_head] = 0;
            }
            break;

        case DEBUG_MUTATE:
            debug_menu::wishmutate( &u );
            break;

        case DEBUG_SPAWN_VEHICLE:
            if( m.veh_at( u.pos() ) ) {
                dbg( D_ERROR ) << "game:load: There's already vehicle here";
                debugmsg( "There's already vehicle here" );
            } else {
                std::vector<vproto_id> veh_strings;
                uilist veh_menu;
                veh_menu.text = _( "Choose vehicle to spawn" );
                int menu_ind = 0;
                for( auto &elem : vehicle_prototype::get_all() ) {
                    if( elem != vproto_id( "custom" ) ) {
                        const vehicle_prototype &proto = elem.obj();
                        veh_strings.push_back( elem );
                        //~ Menu entry in vehicle wish menu: 1st string: displayed name, 2nd string: internal name of vehicle
                        veh_menu.addentry( menu_ind, true, MENU_AUTOASSIGN, _( "%1$s (%2$s)" ), _( proto.name ),
                                           elem.c_str() );
                        ++menu_ind;
                    }
                }
                veh_menu.query();
                if( veh_menu.ret >= 0 && veh_menu.ret < static_cast<int>( veh_strings.size() ) ) {
                    //Didn't cancel
                    const vproto_id &selected_opt = veh_strings[veh_menu.ret];
                    tripoint dest = u.pos(); // TODO: Allow picking this when add_vehicle has 3d argument
                    vehicle *veh = m.add_vehicle( selected_opt, dest.xy(), -90, 100, 0 );
                    if( veh != nullptr ) {
                        m.board_vehicle( u.pos(), &u );
                    }
                }
            }
            break;

        case DEBUG_CHANGE_SKILLS: {
            debug_menu::wishskill( &u );
        }
        break;

        case DEBUG_LEARN_MA:
            add_msg( m_info, _( "Martial arts debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( auto &style : all_martialart_types() ) {
                if( style != matype_id( "style_none" ) ) {
                    u.add_martialart( style );
                }
            }
            add_msg( m_good, _( "You now know a lot more than just 10 styles of kung fu." ) );
            break;

        case DEBUG_UNLOCK_RECIPES: {
            add_msg( m_info, _( "Recipe debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( const auto &e : recipe_dict ) {
                u.learn_recipe( &e.second );
            }
            add_msg( m_good, _( "You know how to craft that now." ) );
        }
        break;

        case DEBUG_EDIT_PLAYER:
            debug_menu::character_edit_menu();
            break;

        case DEBUG_SPAWN_ARTIFACT:
            if( const cata::optional<tripoint> center = g->look_around() ) {
                artifact_natural_property prop = static_cast<artifact_natural_property>( rng( ARTPROP_NULL + 1,
                                                 ARTPROP_MAX - 1 ) );
                m.create_anomaly( *center, prop );
                m.spawn_natural_artifact( *center, prop );
            }
            break;

        case DEBUG_SPAWN_CLAIRVOYANCE:
            u.i_add( item( architects_cube(), calendar::turn ) );
            break;

        case DEBUG_MAP_EDITOR:
            g->look_debug();
            break;

        case DEBUG_CHANGE_WEATHER: {
            uilist weather_menu;
            weather_menu.text = _( "Select new weather pattern:" );
            weather_menu.addentry( 0, true, MENU_AUTOASSIGN, g->weather.weather_override == WEATHER_NULL ?
                                   _( "Keep normal weather patterns" ) : _( "Disable weather forcing" ) );
            for( int weather_id = 1; weather_id < NUM_WEATHER_TYPES; weather_id++ ) {
                weather_menu.addentry( weather_id, true, MENU_AUTOASSIGN,
                                       weather::name( static_cast<weather_type>( weather_id ) ) );
            }

            weather_menu.query();

            if( weather_menu.ret >= 0 && weather_menu.ret < NUM_WEATHER_TYPES ) {
                weather_type selected_weather = static_cast<weather_type>( weather_menu.ret );
                g->weather.weather_override = selected_weather;
                g->weather.set_nextweather( calendar::turn );
            }
        }
        break;

        case DEBUG_WIND_DIRECTION: {
            uilist wind_direction_menu;
            wind_direction_menu.text = _( "Select new wind direction:" );
            wind_direction_menu.addentry( 0, true, MENU_AUTOASSIGN, g->weather.wind_direction_override ?
                                          _( "Disable direction forcing" ) : _( "Keep normal wind direction" ) );
            int count = 1;
            for( int angle = 0; angle <= 315; angle += 45 ) {
                wind_direction_menu.addentry( count, true, MENU_AUTOASSIGN, get_wind_arrow( angle ) );
                count += 1;
            }
            wind_direction_menu.query();
            if( wind_direction_menu.ret == 0 ) {
                g->weather.wind_direction_override = cata::nullopt;
            } else if( wind_direction_menu.ret >= 0 && wind_direction_menu.ret < 9 ) {
                g->weather.wind_direction_override = ( wind_direction_menu.ret - 1 ) * 45;
                g->weather.set_nextweather( calendar::turn );
            }
        }
        break;

        case DEBUG_WIND_SPEED: {
            uilist wind_speed_menu;
            wind_speed_menu.text = _( "Select new wind speed:" );
            wind_speed_menu.addentry( 0, true, MENU_AUTOASSIGN, g->weather.wind_direction_override ?
                                      _( "Disable speed forcing" ) : _( "Keep normal wind speed" ) );
            int count = 1;
            for( int speed = 0; speed <= 100; speed += 10 ) {
                std::string speedstring = std::to_string( speed ) + " " + velocity_units( VU_WIND );
                wind_speed_menu.addentry( count, true, MENU_AUTOASSIGN, speedstring );
                count += 1;
            }
            wind_speed_menu.query();
            if( wind_speed_menu.ret == 0 ) {
                g->weather.windspeed_override = cata::nullopt;
            } else if( wind_speed_menu.ret >= 0 && wind_speed_menu.ret < 12 ) {
                int selected_wind_speed = ( wind_speed_menu.ret - 1 ) * 10;
                g->weather.windspeed_override = selected_wind_speed;
                g->weather.set_nextweather( calendar::turn );
            }
        }
        break;

        case DEBUG_KILL_MONS: {
            for( monster &critter : g->all_monsters() ) {
                // Use the normal death functions, useful for testing death
                // and for getting a corpse.
                critter.die( nullptr );
            }
            g->cleanup_dead();
        }
        break;
        case DEBUG_DISPLAY_HORDES:
            ui::omap::display_hordes();
            break;
        case DEBUG_TEST_IT_GROUP: {
            item_group::debug_spawn();
        }
        break;

        // Damage Self
        case DEBUG_DAMAGE_SELF: {
            int dbg_damage;
            if( query_int( dbg_damage, _( "Damage self for how much? hp: %d" ), u.hp_cur[hp_torso] ) ) {
                u.hp_cur[hp_torso] -= dbg_damage;
                u.die( nullptr );
            }
        }
        break;

        case DEBUG_SHOW_SOUND: {
#if defined(TILES)
            const point offset = u.view_offset.xy() + point( POSX - u.posx(), POSY - u.posy() );
            g->draw_ter();
            auto sounds_to_draw = sounds::get_monster_sounds();
            for( const auto &sound : sounds_to_draw.first ) {
                mvwputch( g->w_terrain, offset + sound.xy(), c_yellow, '?' );
            }
            for( const auto &sound : sounds_to_draw.second ) {
                mvwputch( g->w_terrain, offset + sound.xy(), c_red, '?' );
            }
            wrefresh( g->w_terrain );
            g->draw_panels();
            inp_mngr.wait_for_any_key();
#else
            popup( _( "This binary was not compiled with tiles support." ) );
#endif
        }
        break;

        case DEBUG_DISPLAY_WEATHER:
            ui::omap::display_weather();
            break;
        case DEBUG_DISPLAY_SCENTS:
            ui::omap::display_scents();
            break;
        case DEBUG_DISPLAY_SCENTS_LOCAL:
            g->displaying_temperature = false;
            g->displaying_visibility = false;
            g->displaying_radiation = false;
            g->displaying_scent = !g->displaying_scent;
            break;
        case DEBUG_DISPLAY_TEMP:
            g->displaying_scent = false;
            g->displaying_visibility = false;
            g->displaying_radiation = false;
            g->displaying_temperature = !g->displaying_temperature;
            break;
        case DEBUG_DISPLAY_VISIBILITY: {
            g->displaying_scent = false;
            g->displaying_temperature = false;
            g->displaying_radiation = false;
            g->displaying_visibility = !g->displaying_visibility;
            if( g->displaying_visibility ) {
                std::vector< tripoint > locations;
                uilist creature_menu;
                int num_creatures = 0;
                creature_menu.addentry( num_creatures++, true, MENU_AUTOASSIGN, "%s", _( "You" ) );
                locations.emplace_back( g->u.pos() ); // add player first.
                for( const Creature &critter : g->all_creatures() ) {
                    if( critter.is_player() ) {
                        continue;
                    }
                    creature_menu.addentry( num_creatures++, true, MENU_AUTOASSIGN, critter.disp_name() );
                    locations.emplace_back( critter.pos() );
                }

                pointmenu_cb callback( locations );
                creature_menu.callback = &callback;
                creature_menu.w_y = 0;
                creature_menu.query();
                if( creature_menu.ret >= 0 && static_cast<size_t>( creature_menu.ret ) < locations.size() ) {
                    Creature *creature = g->critter_at<Creature>( locations[creature_menu.ret] );
                    g->displaying_visibility_creature = creature;
                }
            } else {
                g->displaying_visibility_creature = nullptr;
            }
        }
        break;
        case DEBUG_DISPLAY_RADIATION: {
            g->displaying_scent = false;
            g->displaying_temperature = false;
            g->displaying_visibility = false;
            g->displaying_radiation = !g->displaying_radiation;
        }
        break;
        case DEBUG_CHANGE_TIME: {
            auto set_turn = [&]( const int initial, const time_duration factor, const char *const msg ) {
                const auto text = string_input_popup()
                                  .title( msg )
                                  .width( 20 )
                                  .text( to_string( initial ) )
                                  .only_digits( true )
                                  .query_string();
                if( text.empty() ) {
                    return;
                }
                const int new_value = std::atoi( text.c_str() );
                const time_duration offset = ( new_value - initial ) * factor;
                // Arbitrary maximal value.
                const time_point max = calendar::turn_zero + time_duration::from_turns(
                                           std::numeric_limits<int>::max() / 2 );
                calendar::turn = std::max( std::min( max, calendar::turn + offset ), calendar::turn_zero );
            };

            uilist smenu;
            static const auto years = []( const time_point & p ) {
                return static_cast<int>( ( p - calendar::turn_zero ) / calendar::year_length() );
            };
            do {
                const int iSel = smenu.ret;
                smenu.reset();
                smenu.addentry( 0, true, 'y', "%s: %d", _( "year" ), years( calendar::turn ) );
                smenu.addentry( 1, !calendar::eternal_season(), 's', "%s: %d",
                                _( "season" ), static_cast<int>( season_of_year( calendar::turn ) ) );
                smenu.addentry( 2, true, 'd', "%s: %d", _( "day" ), day_of_season<int>( calendar::turn ) );
                smenu.addentry( 3, true, 'h', "%s: %d", _( "hour" ), hour_of_day<int>( calendar::turn ) );
                smenu.addentry( 4, true, 'm', "%s: %d", _( "minute" ), minute_of_hour<int>( calendar::turn ) );
                smenu.addentry( 5, true, 't', "%s: %d", _( "turn" ),
                                to_turns<int>( calendar::turn - calendar::turn_zero ) );
                smenu.selected = iSel;
                smenu.query();

                switch( smenu.ret ) {
                    case 0:
                        set_turn( years( calendar::turn ), calendar::year_length(), _( "Set year to?" ) );
                        break;
                    case 1:
                        set_turn( static_cast<int>( season_of_year( calendar::turn ) ), calendar::season_length(),
                                  _( "Set season to? (0 = spring)" ) );
                        break;
                    case 2:
                        set_turn( day_of_season<int>( calendar::turn ), 1_days, _( "Set days to?" ) );
                        break;
                    case 3:
                        set_turn( hour_of_day<int>( calendar::turn ), 1_hours, _( "Set hour to?" ) );
                        break;
                    case 4:
                        set_turn( minute_of_hour<int>( calendar::turn ), 1_minutes, _( "Set minute to?" ) );
                        break;
                    case 5:
                        set_turn( to_turns<int>( calendar::turn - calendar::turn_zero ), 1_turns,
                                  string_format( _( "Set turn to? (One day is %i turns)" ), to_turns<int>( 1_days ) ).c_str() );
                        break;
                    default:
                        break;
                }
            } while( smenu.ret != UILIST_CANCEL );
        }
        break;
        case DEBUG_SET_AUTOMOVE: {
            const cata::optional<tripoint> dest = g->look_around();
            if( !dest || *dest == u.pos() ) {
                break;
            }

            auto rt = m.route( u.pos(), *dest, u.get_pathfinding_settings(), u.get_path_avoid() );
            if( !rt.empty() ) {
                u.set_destination( rt );
            } else {
                popup( "Couldn't find path" );
            }
        }
        break;
        case DEBUG_SHOW_MUT_CAT:
            for( const auto &elem : u.mutation_category_level ) {
                add_msg( "%s: %d", elem.first.c_str(), elem.second );
            }
            break;

        case DEBUG_OM_EDITOR:
            ui::omap::display_editor();
            break;

        case DEBUG_BENCHMARK: {
            const int ms = string_input_popup()
                           .title( _( "Enter benchmark length (in milliseconds):" ) )
                           .width( 20 )
                           .text( "5000" )
                           .query_int();
            debug_menu::draw_benchmark( ms );
        }
        break;

        case DEBUG_OM_TELEPORT:
            debug_menu::teleport_overmap();
            break;
        case DEBUG_TRAIT_GROUP:
            trait_group::debug_spawn();
            break;
        case DEBUG_SHOW_MSG:
            debugmsg( "Test debugmsg" );
            break;
        case DEBUG_CRASH_GAME:
            raise( SIGSEGV );
            break;
        case DEBUG_MAP_EXTRA: {
            std::unordered_map<std::string, map_extra_pointer> FM = MapExtras::all_functions();
            uilist mx_menu;
            std::vector<std::string> mx_str;
            for( auto &extra : FM ) {
                mx_menu.addentry( -1, true, -1, extra.first );
                mx_str.push_back( extra.first );
            }
            mx_menu.query();
            int mx_choice = mx_menu.ret;
            if( mx_choice >= 0 && mx_choice < static_cast<int>( mx_str.size() ) ) {
                const tripoint where_omt( ui::omap::choose_point() );
                if( where_omt != overmap::invalid_tripoint ) {
                    tripoint where_sm = omt_to_sm_copy( where_omt );
                    tinymap mx_map;
                    mx_map.load( where_sm, false );
                    MapExtras::apply_function( mx_str[mx_choice], mx_map, where_sm );
                    g->load_npcs();
                    g->m.invalidate_map_cache( g->get_levz() );
                    g->refresh_all();
                }
            }
            break;
        }
        case DEBUG_DISPLAY_NPC_PATH:
            g->debug_pathfinding = !g->debug_pathfinding;
            break;
        case DEBUG_PRINT_FACTION_INFO: {
            int count = 0;
            for( const auto elem : g->faction_manager_ptr->all() ) {
                std::cout << std::to_string( count ) << " Faction_id key in factions map = " << elem.first.str() <<
                          std::endl;
                std::cout << std::to_string( count ) << " Faction name associated with this id is " <<
                          elem.second.name << std::endl;
                std::cout << std::to_string( count ) << " the id of that faction object is " << elem.second.id.str()
                          << std::endl;
                count++;
            }
            std::cout << "Player faction is " << g->u.get_faction()->id.str() << std::endl;
            break;
        }
        case DEBUG_QUIT_NOSAVE:
            if( query_yn(
                    _( "Quit without saving? This may cause issues such as duplicated or missing items and vehicles!" ) ) ) {
                u.moves = 0;
                g->uquit = QUIT_NOSAVED;
            }
            break;
        case DEBUG_TEST_WEATHER: {
            weather_generator weathergen;
            weathergen.test_weather();
        }
        break;

        case DEBUG_SAVE_SCREENSHOT: {
#if defined(TILES)
            // check that the current '<world>/screenshots' directory exists
            std::stringstream map_directory;
            map_directory << g->get_world_base_save_path() << "/screenshots/";
            assure_dir_exist( map_directory.str() );

            // build file name: <map_dir>/screenshots/[<character_name>]_<date>.png
            // Date format is a somewhat ISO-8601 compliant GMT time date (except for some characters that wouldn't pass on most file systems like ':').
            std::time_t time = std::time( nullptr );
            std::stringstream date_buffer;
            date_buffer << std::put_time( std::gmtime( &time ), "%F_%H-%M-%S_%z" );
            const auto tmp_file_name = string_format( "[%s]_%s.png", g->u.get_name(), date_buffer.str() );

            std::string file_name = ensure_valid_file_name( tmp_file_name );
            auto current_file_path = map_directory.str() + file_name;

            // Take a screenshot of the viewport.
            if( g->take_screenshot( current_file_path ) ) {
                popup( _( "Successfully saved your screenshot to: %s" ), map_directory.str() );
            } else {
                popup( _( "An error occurred while trying to save the screenshot." ) );
            }
#else
            popup( _( "This binary was not compiled with tiles support." ) );
#endif
        }
        break;

        case DEBUG_GAME_REPORT: {
            // generate a game report, useful for bug reporting.
            std::string report = game_info::game_report();
            // write to log
            DebugLog( DL_ALL, DC_ALL ) << " GAME REPORT: \n" << report;
            std::string popup_msg = _( "Report written to debug.log" );
#if defined(TILES)
            // copy to clipboard
            int clipboard_result = SDL_SetClipboardText( report.c_str() );
            printErrorIf( clipboard_result != 0, "Error while copying the game report to the clipboard." );
            if( clipboard_result == 0 ) {
                popup_msg += _( " and to the clipboard." );
            }
#endif
            popup( popup_msg );
        }
        break;
        case DEBUG_LEARN_SPELLS:
            if( spell_type::get_all().empty() ) {
                add_msg( m_bad, _( "There are no spells to learn.  You must install a mod that adds some." ) );
            } else {
                for( const spell_type &learn : spell_type::get_all() ) {
                    g->u.magic.learn_spell( &learn, g->u, true );
                }
                add_msg( m_good,
                         _( "You have become an Archwizardpriest!  What will you do with your newfound power?" ) );
            }
            break;
        case DEBUG_LEVEL_SPELLS: {
            std::vector<spell *> spells = g->u.magic.get_spells();
            if( spells.empty() ) {
                add_msg( m_bad, _( "Try learning some spells first." ) );
                return;
            }
            std::vector<uilist_entry> uiles;
            {
                uilist_entry uile( _( "Spell" ) );
                uile.ctxt = string_format( "%3s %3s", _( "LVL" ), _( "MAX" ) );
                uile.enabled = false;
                uile.force_color = c_light_blue;
                uiles.emplace_back( uile );
            }
            int retval = 0;
            for( spell *sp : spells ) {
                uilist_entry uile( sp->name() );
                uile.ctxt = string_format( "%3d %3d", sp->get_level(), sp->get_max_level() );
                uile.retval = retval++;
                uile.enabled = !sp->is_max_level();
                uiles.emplace_back( uile );
            }
            int action = uilist( _( "Debug level spell:" ), uiles );
            if( action < 0 ) {
                return;
            }
            int desired_level = 0;
            int cur_level = spells[action]->get_level();
            query_int( desired_level, _( "Desired Spell Level: (Current %d)" ), cur_level );
            desired_level = std::min( desired_level, spells[action]->get_max_level() );
            while( cur_level < desired_level ) {
                spells[action]->gain_level();
                cur_level = spells[action]->get_level();
            }
            add_msg( m_good, _( "%s is now level %d!" ), spells[action]->name(), spells[action]->get_level() );
            break;
        }
    }
    catacurses::erase();
    m.invalidate_map_cache( g->get_levz() );
    g->refresh_all();
}

} // namespace debug_menu
