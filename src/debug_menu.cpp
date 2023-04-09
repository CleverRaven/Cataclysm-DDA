#include "debug_menu.h"

#include <cstdint>
// IWYU pragma: no_include <sys/signal.h>
// IWYU pragma: no_include <cxxabi.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <iomanip> // IWYU pragma: keep
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "achievement.h"
#include "action.h"
#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "city.h"
#include "clzones.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "dialogue_chatbin.h"
#include "display.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "effect_source.h"
#include "enum_conversions.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "faction.h"
#include "filesystem.h" // IWYU pragma: keep
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "global_vars.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "localized_comparator.h"
#include "magic.h"
#include "map.h"
#include "map_extras.h"
#include "mapgen.h"
#include "mapgendata.h"
#include "martialarts.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "monster.h"
#include "monstergenerator.h"
#include "morale_types.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "npc_class.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "path_info.h" // IWYU pragma: keep
#include "pimpl.h"
#include "point.h"
#include "popup.h"
#include "recipe_dictionary.h"
#include "relic.h"
#include "rng.h"
#include "sounds.h"
#include "stomach.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "tgz_archiver.h"
#include "trait_group.h"
#include "translations.h"
#include "try_parse_integer.h"
#include "type_id.h"
#include "ui.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vitamin.h"
#include "vpart_position.h"
#include "weather.h"
#include "weather_gen.h"
#include "weather_type.h"
#include "weighted_list.h"
#include "worldfactory.h"

static const achievement_id achievement_achievement_arcade_mode( "achievement_arcade_mode" );

static const bodypart_str_id body_part_no_a_real_part( "no_a_real_part" );

static const efftype_id effect_asthma( "asthma" );
static const efftype_id effect_bleed( "bleed" );

static const faction_id faction_no_faction( "no_faction" );

static const matype_id style_none( "style_none" );

static const mtype_id mon_generator( "mon_generator" );

static const trait_id trait_ASTHMA( "ASTHMA" );
static const trait_id trait_NONE( "NONE" );

static const vproto_id vehicle_prototype_custom( "custom" );

#if defined(TILES)
#include "sdl_wrappers.h"
#endif
#include "timed_event.h"

#define dbg(x) DebugLog((x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

namespace io
{

template<>
std::string enum_to_string<debug_menu::debug_menu_index>( debug_menu::debug_menu_index v )
{
    switch( v ) {
        // *INDENT-OFF*
        case debug_menu::debug_menu_index::WISH: return "WISH";
        case debug_menu::debug_menu_index::SHORT_TELEPORT: return "SHORT_TELEPORT";
        case debug_menu::debug_menu_index::LONG_TELEPORT: return "LONG_TELEPORT";
        case debug_menu::debug_menu_index::REVEAL_MAP: return "REVEAL_MAP";
        case debug_menu::debug_menu_index::SPAWN_NPC: return "SPAWN_NPC";
        case debug_menu::debug_menu_index::SPAWN_OM_NPC: return "SPAWN_OM_NPC";
        case debug_menu::debug_menu_index::SPAWN_MON: return "SPAWN_MON";
        case debug_menu::debug_menu_index::GAME_STATE: return "GAME_STATE";
        case debug_menu::debug_menu_index::KILL_AREA: return "KILL_AREA";
        case debug_menu::debug_menu_index::KILL_NPCS: return "KILL_NPCS";
        case debug_menu::debug_menu_index::MUTATE: return "MUTATE";
        case debug_menu::debug_menu_index::SPAWN_VEHICLE: return "SPAWN_VEHICLE";
        case debug_menu::debug_menu_index::CHANGE_SKILLS: return "CHANGE_SKILLS";
        case debug_menu::debug_menu_index::CHANGE_THEORY: return "CHANGE_THEORY";
        case debug_menu::debug_menu_index::LEARN_MA: return "LEARN_MA";
        case debug_menu::debug_menu_index::UNLOCK_RECIPES: return "UNLOCK_RECIPES";
        case debug_menu::debug_menu_index::EDIT_PLAYER: return "EDIT_PLAYER";
        case debug_menu::debug_menu_index::CONTROL_NPC: return "CONTROL_NPC";
        case debug_menu::debug_menu_index::SPAWN_ARTIFACT: return "SPAWN_ARTIFACT";
        case debug_menu::debug_menu_index::SPAWN_CLAIRVOYANCE: return "SPAWN_CLAIRVOYANCE";
        case debug_menu::debug_menu_index::MAP_EDITOR: return "MAP_EDITOR";
        case debug_menu::debug_menu_index::CHANGE_WEATHER: return "CHANGE_WEATHER";
        case debug_menu::debug_menu_index::WIND_DIRECTION: return "WIND_DIRECTION";
        case debug_menu::debug_menu_index::WIND_SPEED: return "WIND_SPEED";
        case debug_menu::debug_menu_index::GEN_SOUND: return "GEN_SOUND";
        case debug_menu::debug_menu_index::KILL_MONS: return "KILL_MONS";
        case debug_menu::debug_menu_index::DISPLAY_HORDES: return "DISPLAY_HORDES";
        case debug_menu::debug_menu_index::TEST_IT_GROUP: return "TEST_IT_GROUP";
        case debug_menu::debug_menu_index::DAMAGE_SELF: return "DAMAGE_SELF";
        case debug_menu::debug_menu_index::BLEED_SELF: return "BLEED_SELF";
        case debug_menu::debug_menu_index::SHOW_SOUND: return "SHOW_SOUND";
        case debug_menu::debug_menu_index::DISPLAY_WEATHER: return "DISPLAY_WEATHER";
        case debug_menu::debug_menu_index::DISPLAY_SCENTS: return "DISPLAY_SCENTS";
        case debug_menu::debug_menu_index::CHANGE_TIME: return "CHANGE_TIME";
        case debug_menu::debug_menu_index::FORCE_TEMP: return "FORCE_TEMP";
        case debug_menu::debug_menu_index::SET_AUTOMOVE: return "SET_AUTOMOVE";
        case debug_menu::debug_menu_index::SHOW_MUT_CAT: return "SHOW_MUT_CAT";
        case debug_menu::debug_menu_index::OM_EDITOR: return "OM_EDITOR";
        case debug_menu::debug_menu_index::BENCHMARK: return "BENCHMARK";
        case debug_menu::debug_menu_index::OM_TELEPORT: return "OM_TELEPORT";
        case debug_menu::debug_menu_index::OM_TELEPORT_COORDINATES: return "OM_TELEPORT_COORDINATES";
        case debug_menu::debug_menu_index::OM_TELEPORT_CITY: return "OM_TELEPORT_CITY";
        case debug_menu::debug_menu_index::TRAIT_GROUP: return "TRAIT_GROUP";
        case debug_menu::debug_menu_index::ENABLE_ACHIEVEMENTS: return "ENABLE_ACHIEVEMENTS";
        case debug_menu::debug_menu_index::UNLOCK_ALL: return "UNLOCK_ALL";
        case debug_menu::debug_menu_index::SHOW_MSG: return "SHOW_MSG";
        case debug_menu::debug_menu_index::CRASH_GAME: return "CRASH_GAME";
        case debug_menu::debug_menu_index::MAP_EXTRA: return "MAP_EXTRA";
        case debug_menu::debug_menu_index::DISPLAY_NPC_PATH: return "DISPLAY_NPC_PATH";
        case debug_menu::debug_menu_index::DISPLAY_NPC_ATTACK: return "DISPLAY_NPC_ATTACK";
        case debug_menu::debug_menu_index::PRINT_FACTION_INFO: return "PRINT_FACTION_INFO";
        case debug_menu::debug_menu_index::PRINT_NPC_MAGIC: return "PRINT_NPC_MAGIC";
        case debug_menu::debug_menu_index::QUIT_NOSAVE: return "QUIT_NOSAVE";
        case debug_menu::debug_menu_index::QUICKLOAD: return "QUICKLOAD";
        case debug_menu::debug_menu_index::TEST_WEATHER: return "TEST_WEATHER";
        case debug_menu::debug_menu_index::WRITE_GLOBAL_EOCS: return "WRITE_GLOBAL_EOCS";
        case debug_menu::debug_menu_index::WRITE_GLOBAL_VARS: return "WRITE_GLOBAL_VARS";
        case debug_menu::debug_menu_index::EDIT_GLOBAL_VARS: return "SET_GLOBAL_VARS";
        case debug_menu::debug_menu_index::WRITE_TIMED_EVENTS: return "WRITE_TIMED_EVENTS";
        case debug_menu::debug_menu_index::SAVE_SCREENSHOT: return "SAVE_SCREENSHOT";
        case debug_menu::debug_menu_index::GAME_REPORT: return "GAME_REPORT";
        case debug_menu::debug_menu_index::GAME_MIN_ARCHIVE: return "GAME_MIN_ARCHIVE";
        case debug_menu::debug_menu_index::DISPLAY_SCENTS_LOCAL: return "DISPLAY_SCENTS_LOCAL";
        case debug_menu::debug_menu_index::DISPLAY_SCENTS_TYPE_LOCAL: return "DISPLAY_SCENTS_TYPE_LOCAL";
        case debug_menu::debug_menu_index::DISPLAY_TEMP: return "DISPLAY_TEMP";
        case debug_menu::debug_menu_index::DISPLAY_VEHICLE_AI: return "DISPLAY_VEHICLE_AI";
        case debug_menu::debug_menu_index::DISPLAY_VISIBILITY: return "DISPLAY_VISIBILITY";
        case debug_menu::debug_menu_index::DISPLAY_LIGHTING: return "DISPLAY_LIGHTING";
        case debug_menu::debug_menu_index::DISPLAY_TRANSPARENCY: return "DISPLAY_TRANSPARENCY";
        case debug_menu::debug_menu_index::DISPLAY_REACHABILITY_ZONES: return "DISPLAY_REACHABILITY_ZONES";
        case debug_menu::debug_menu_index::DISPLAY_RADIATION: return "DISPLAY_RADIATION";
        case debug_menu::debug_menu_index::HOUR_TIMER: return "HOUR_TIMER";
        case debug_menu::debug_menu_index::CHANGE_SPELLS: return "CHANGE_SPELLS";
        case debug_menu::debug_menu_index::TEST_MAP_EXTRA_DISTRIBUTION: return "TEST_MAP_EXTRA_DISTRIBUTION";
        case debug_menu::debug_menu_index::NESTED_MAPGEN: return "NESTED_MAPGEN";
        case debug_menu::debug_menu_index::EDIT_CAMP_LARDER: return "EDIT_CAMP_LARDER";
        case debug_menu::debug_menu_index::VEHICLE_BATTERY_CHARGE: return "VEHICLE_BATTERY_CHARGE";
        case debug_menu::debug_menu_index::GENERATE_EFFECT_LIST: return "GENERATE_EFFECT_LIST";
        case debug_menu::debug_menu_index::ACTIVATE_EOC: return "ACTIVATE_EOC";
        // *INDENT-ON*
        case debug_menu::debug_menu_index::last:
            break;
    }
    debugmsg( "unknown debug_menu::debug_menu_index %d", static_cast<int>( v ) );
    return "";
}

} // namespace io

namespace
{
struct fake_tripoint { // NOLINT(cata-xy)
    int x = 0, y = 0, z = 0;
};

struct OM_point { // NOLINT(cata-xy)
    int x = 0, y = 0;
};

std::istream &operator>>( std::istream &is, fake_tripoint &pos )
{
    char c = 0;
    is >> pos.x &&is.get( c ) &&c == '.' &&is >> pos.y &&is.get( c ) &&c == '.' &&is >> pos.z;
    return is;
}

std::istream &operator>>( std::istream &is, OM_point &pos )
{
    char c = 0;
    is >> pos.x &&is.get( c ) &&c == '.' &&is >> pos.y;
    return is;
}

template<typename T>
T _from_fs_string( std::string const &s )
{
    std::istringstream is( s );
    is.imbue( std::locale::classic() );
    T result;
    is >> result;
    if( !is ) {
        return T{};
    }
    return result;
}

tripoint _from_map_string( std::string const &s )
{
    fake_tripoint const ft = _from_fs_string<fake_tripoint>( s );
    return { ft.x, ft.y, ft.z };
}

tripoint _from_OM_string( std::string const &s )
{
    OM_point const om = _from_fs_string<OM_point>( s );
    return { om.x, om.y, 0 };
}

using rdi_t = fs::recursive_directory_iterator;
using f_validate_t = std::function<bool( fs::path const &dep, rdi_t &iter )>;

bool _add_dir( tgz_archiver &tgz, fs::path const &root, f_validate_t const &validate = {} )
{
    fs::path const cnc = root.has_root_path() ?  fs::canonical( root ) : root;
    fs::path const parent_cnc = cnc.parent_path();

    for( auto iter = rdi_t( cnc ); iter != rdi_t(); ++iter ) {
        fs::path const dep = iter->path().lexically_relative( parent_cnc );

        if( validate && !validate( dep, iter ) ) {
            continue;
        }

        if( !tgz.add_file( *iter, dep ) ) {
            popup( _( "Failed to create minimized archive" ) );
            return false;
        }
    }

    return true;
}

bool _trim_mapbuffer( fs::path const &dep, rdi_t &iter, tripoint_range<tripoint> const &segs,
                      tripoint_range<tripoint> const &regs )
{
    // discard map memory outside of current region and adjacent regions
    if( dep.parent_path().extension() == ".mm1" &&
        !regs.is_point_inside( tripoint{ _from_map_string( dep.stem().string() ).xy(), 0 } ) ) {
        return false;
    }
    // discard map buffer outside of current and adjacent segments
    if( dep.parent_path().filename() == "maps" &&
        !segs.is_point_inside(
            tripoint{ _from_map_string( dep.filename().string() ).xy(), 0 } ) ) {
        iter.disable_recursion_pending();
        return false;
    }
    return true;
}

bool _trim_overmapbuffer( fs::path const &dep, tripoint_range<tripoint> const &oms )
{
    std::string const fname = dep.filename().generic_u8string();

    std::string::size_type const seenpos = fname.find( ".seen." );
    if( seenpos != std::string::npos ) {
        return oms.is_point_inside( _from_OM_string( fname.substr( seenpos + 6 ) ) );
    }

    return fname.size() < 4 || fname[0] != 'o' || fname[1] != '.' ||
           oms.is_point_inside( _from_OM_string( fname.substr( 2 ) ) );
}

bool _discard_temporary( fs::path const &dep )
{
    return !dep.has_extension() || dep.extension() != ".temp";
}

void write_min_archive()
{
    tripoint_abs_seg const seg = project_to<coords::seg>( get_avatar().get_location() );
    tripoint_range<tripoint> const segs = points_in_radius( tripoint{ seg.xy().raw(), 0 }, 1 );
    point sm = project_to<coords::sm>( get_avatar().get_location() ).raw().xy();
    point const reg = sm_to_mmr_remain( sm.x, sm.y );
    tripoint_range<tripoint> const regs = points_in_radius( tripoint{ reg, 0 }, 1 );
    tripoint_abs_om const om = project_to<coords::om>( get_avatar().get_location() );
    tripoint_range<tripoint> const oms = points_in_radius( tripoint{ om.raw().xy(), 0 }, 1 );

    fs::path const save_root( PATH_INFO::world_base_save_path_path() );
    std::string const ofile = save_root.string() + "-trimmed.tar.gz";

    tgz_archiver tgz( ofile );

    f_validate_t const mb_validate = [&segs, &regs, &oms]( fs::path const & dep, rdi_t & iter ) {
        return _discard_temporary( dep ) && _trim_mapbuffer( dep, iter, segs, regs ) &&
               _trim_overmapbuffer( dep, oms );
    };

    if( _add_dir( tgz, save_root, mb_validate ) &&
        _add_dir( tgz, fs::path( PATH_INFO::config_dir_path() ) ) ) {
        tgz.finalize();
        popup( string_format( _( "Minimized archive saved to %s" ), ofile ) );
    }
}

} // namespace

namespace debug_menu
{

class mission_debug
{
    private:
        // Doesn't actually "destroy" the mission, just removes assignments
        static void remove_mission( mission &m );
    public:
        static void edit_mission( mission &m );
        static void edit( Character &who );
        static void edit_player();
        static void edit_npc( npc &who );
        static std::string describe( const mission &m );
};

static std::string first_word( const std::string &str )
{
    const size_t space_pos = str.find( ' ' );
    if( space_pos == std::string::npos || space_pos == 0 ) {
        return str;
    }
    return str.substr( 0, space_pos );
}

static bool is_debug_character()
{
    static const std::unordered_set<std::string> debug_names = {
        "Debug", "Test", "Sandbox", "Staging", "QA", "UAT"
    };
    return debug_names.count( first_word( get_player_character().name ) ) ||
           debug_names.count( first_word( world_generator->active_world->world_name ) );
}

static int player_uilist()
{
    std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( debug_menu_index::MUTATE, true, 'M', _( "Mutate" ) ) },
        { uilist_entry( debug_menu_index::CHANGE_SKILLS, true, 's', _( "Change all skills" ) ) },
        { uilist_entry( debug_menu_index::CHANGE_THEORY, true, 'T', _( "Change all skills theoretical knowledge" ) ) },
        { uilist_entry( debug_menu_index::LEARN_MA, true, 'l', _( "Learn all melee styles" ) ) },
        { uilist_entry( debug_menu_index::UNLOCK_RECIPES, true, 'r', _( "Unlock all recipes" ) ) },
        { uilist_entry( debug_menu_index::EDIT_PLAYER, true, 'p', _( "Edit player/NPC" ) ) },
        { uilist_entry( debug_menu_index::DAMAGE_SELF, true, 'd', _( "Damage self" ) ) },
        { uilist_entry( debug_menu_index::BLEED_SELF, true, 'b', _( "Bleed self" ) ) },
        { uilist_entry( debug_menu_index::SET_AUTOMOVE, true, 'a', _( "Set auto move route" ) ) },
        { uilist_entry( debug_menu_index::CONTROL_NPC, true, 'x', _( "Control NPC follower" ) ) },
    };
    if( !spell_type::get_all().empty() ) {
        uilist_initializer.emplace_back( uilist_entry( debug_menu_index::CHANGE_SPELLS, true, 'S',
                                         _( "Change spells" ) ) );
    }

    return uilist( _( "Player…" ), uilist_initializer );
}

static int info_uilist( bool display_all_entries = true )
{
    // always displayed
    std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( debug_menu_index::SAVE_SCREENSHOT, true, 'H', _( "Take screenshot" ) ) },
        { uilist_entry( debug_menu_index::GAME_REPORT, true, 'r', _( "Generate game report" ) ) },
        { uilist_entry( debug_menu_index::GAME_MIN_ARCHIVE, true, '!', _( "Generate minimized save archive" ) ) },
    };

    if( display_all_entries ) {
        const std::vector<uilist_entry> debug_only_options = {
            { uilist_entry( debug_menu_index::GAME_STATE, true, 'g', _( "Check game state" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_HORDES, true, 'h', _( "Display hordes" ) ) },
            { uilist_entry( debug_menu_index::TEST_IT_GROUP, true, 'i', _( "Test item group" ) ) },
            { uilist_entry( debug_menu_index::SHOW_SOUND, true, 'c', _( "Show sound clustering" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_WEATHER, true, 'w', _( "Display weather" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_SCENTS, true, 'S', _( "Display overmap scents" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_SCENTS_LOCAL, true, 's', _( "Toggle display local scents" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_SCENTS_TYPE_LOCAL, true, 'y', _( "Toggle display local scents type" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_TEMP, true, 'T', _( "Toggle display temperature" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_VEHICLE_AI, true, 'V', _( "Toggle display vehicle autopilot overlay" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_VISIBILITY, true, 'v', _( "Toggle display visibility" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_LIGHTING, true, 'l', _( "Toggle display lighting" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_TRANSPARENCY, true, 'p', _( "Toggle display transparency" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_REACHABILITY_ZONES, true, 'z', _( "Toggle display reachability zones" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_RADIATION, true, 'R', _( "Toggle display radiation" ) ) },
            { uilist_entry( debug_menu_index::SHOW_MUT_CAT, true, 'm', _( "Show mutation category levels" ) ) },
            { uilist_entry( debug_menu_index::BENCHMARK, true, 'b', _( "Draw benchmark (X seconds)" ) ) },
            { uilist_entry( debug_menu_index::HOUR_TIMER, true, 'E', _( "Toggle hour timer" ) ) },
            { uilist_entry( debug_menu_index::TRAIT_GROUP, true, 't', _( "Test trait group" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_NPC_PATH, true, 'n', _( "Toggle NPC pathfinding on map" ) ) },
            { uilist_entry( debug_menu_index::DISPLAY_NPC_ATTACK, true, 'A', _( "Toggle NPC attack potential values on map" ) ) },
            { uilist_entry( debug_menu_index::PRINT_FACTION_INFO, true, 'f', _( "Print faction info to console" ) ) },
            { uilist_entry( debug_menu_index::PRINT_NPC_MAGIC, true, 'M', _( "Print NPC magic info to console" ) ) },
            { uilist_entry( debug_menu_index::TEST_WEATHER, true, 'W', _( "Test weather" ) ) },
            { uilist_entry( debug_menu_index::WRITE_GLOBAL_EOCS, true, 'C', _( "Write global effect_on_condition(s) to eocs.output" ) ) },
            { uilist_entry( debug_menu_index::WRITE_GLOBAL_VARS, true, 'G', _( "Write global var(s) to var_list.output" ) ) },
            { uilist_entry( debug_menu_index::WRITE_TIMED_EVENTS, true, 'E', _( "Write Timed (E)vents to timed_event_list.output" ) ) },
            { uilist_entry( debug_menu_index::EDIT_GLOBAL_VARS, true, 's', _( "Edit global var(s)" ) ) },
            { uilist_entry( debug_menu_index::TEST_MAP_EXTRA_DISTRIBUTION, true, 'e', _( "Test map extra list" ) ) },
            { uilist_entry( debug_menu_index::GENERATE_EFFECT_LIST, true, 'L', _( "Generate effect list" ) ) },
        };
        uilist_initializer.insert( uilist_initializer.begin(), debug_only_options.begin(),
                                   debug_only_options.end() );
    }

    return uilist( _( "Info…" ), uilist_initializer );
}

static int game_uilist()
{
    std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( debug_menu_index::ENABLE_ACHIEVEMENTS, true, 'a', _( "Enable achievements" ) ) },
        { uilist_entry( debug_menu_index::UNLOCK_ALL, true, 'u', _( "Unlock all progression" ) ) },
        { uilist_entry( debug_menu_index::SHOW_MSG, true, 'd', _( "Show debug message" ) ) },
        { uilist_entry( debug_menu_index::CRASH_GAME, true, 'C', _( "Crash game (test crash handling)" ) ) },
        { uilist_entry( debug_menu_index::ACTIVATE_EOC, true, 'E', _( "Activate EOC" ) ) },
        { uilist_entry( debug_menu_index::QUIT_NOSAVE, true, 'Q', _( "Quit to main menu" ) )  },
        { uilist_entry( debug_menu_index::QUICKLOAD, true, 'q', _( "Quickload" ) )  },
    };

    return uilist( _( "Game…" ), uilist_initializer );
}

static int vehicle_uilist()
{
    std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( debug_menu_index::VEHICLE_BATTERY_CHARGE, true, 'b', _( "Change battery charge" ) ) },
    };

    return uilist( _( "Vehicle…" ), uilist_initializer );
}

static int teleport_uilist()
{
    std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( debug_menu_index::SHORT_TELEPORT, true, 's', _( "Teleport - short range" ) ) },
        { uilist_entry( debug_menu_index::LONG_TELEPORT, true, 'l', _( "Teleport - long range" ) ) },
        { uilist_entry( debug_menu_index::OM_TELEPORT, true, 'o', _( "Teleport - adjacent overmap" ) ) },
        { uilist_entry( debug_menu_index::OM_TELEPORT_COORDINATES, true, 'p', _( "Teleport - specific overmap coordinates" ) ) },
    };

    const bool teleport_city_enabled = get_option<bool>( "SELECT_STARTING_CITY" );
    uilist_initializer.emplace_back( uilist_entry( debug_menu_index::OM_TELEPORT_CITY,
                                     teleport_city_enabled, 'c', _( "Teleport - specific city" ) ) );

    return uilist( _( "Teleport…" ), uilist_initializer );
}

static int spawning_uilist()
{
    const std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( debug_menu_index::WISH, true, 'w', _( "Spawn an item" ) ) },
        { uilist_entry( debug_menu_index::SPAWN_NPC, true, 'n', _( "Spawn NPC" ) ) },
        { uilist_entry( debug_menu_index::SPAWN_OM_NPC, true, 'N', _( "Spawn random NPC on overmap" ) ) },
        { uilist_entry( debug_menu_index::SPAWN_MON, true, 'm', _( "Spawn monster" ) ) },
        { uilist_entry( debug_menu_index::SPAWN_VEHICLE, true, 'v', _( "Spawn a vehicle" ) ) },
        { uilist_entry( debug_menu_index::SPAWN_ARTIFACT, true, 'a', _( "Spawn artifact" ) ) },
        { uilist_entry( debug_menu_index::SPAWN_CLAIRVOYANCE, true, 'c', _( "Spawn clairvoyance artifact" ) ) },
    };

    return uilist( _( "Spawning…" ), uilist_initializer );
}

static int map_uilist()
{
    const std::vector<uilist_entry> uilist_initializer = {
        { uilist_entry( debug_menu_index::REVEAL_MAP, true, 'r', _( "Reveal map" ) ) },
        { uilist_entry( debug_menu_index::KILL_AREA, true, 'a', _( "Kill in Area" ) ) },
        { uilist_entry( debug_menu_index::KILL_NPCS, true, 'k', _( "Kill NPCs" ) ) },
        { uilist_entry( debug_menu_index::MAP_EDITOR, true, 'M', _( "Map editor" ) ) },
        { uilist_entry( debug_menu_index::CHANGE_WEATHER, true, 'w', _( "Change weather" ) ) },
        { uilist_entry( debug_menu_index::WIND_DIRECTION, true, 'd', _( "Change wind direction" ) ) },
        { uilist_entry( debug_menu_index::WIND_SPEED, true, 's', _( "Change wind speed" ) ) },
        { uilist_entry( debug_menu_index::GEN_SOUND, true, 'S', _( "Generate sound" ) ) },
        { uilist_entry( debug_menu_index::KILL_MONS, true, 'K', _( "Kill all monsters" ) ) },
        { uilist_entry( debug_menu_index::CHANGE_TIME, true, 't', _( "Change time" ) ) },
        { uilist_entry( debug_menu_index::FORCE_TEMP, true, 'T', _( "Force temperature" ) ) },
        { uilist_entry( debug_menu_index::OM_EDITOR, true, 'O', _( "Overmap editor" ) ) },
        { uilist_entry( debug_menu_index::MAP_EXTRA, true, 'm', _( "Spawn map extra" ) ) },
        { uilist_entry( debug_menu_index::NESTED_MAPGEN, true, 'n', _( "Spawn nested mapgen" ) ) },
        { uilist_entry( debug_menu_index::EDIT_CAMP_LARDER, true, 'l', _( "Edit the faction camp larder" ) ) },
    };

    return uilist( _( "Map…" ), uilist_initializer );
}

/**
 * Create the debug menu UI list.
 * @param display_all_entries: `true` if all entries should be displayed, `false` is some entries should be hidden (for ex. when the debug menu is called from the main menu).
 *   This allows to have some menu elements at the same time in the main menu and in the debug menu.
 * @returns The chosen action.
 */
static std::optional<debug_menu_index> debug_menu_uilist( bool display_all_entries = true )
{
    std::vector<uilist_entry> menu = {
        { uilist_entry( 1, true, 'i', _( "Info…" ) ) },
    };

    if( display_all_entries ) {
        const std::vector<uilist_entry> debug_menu = {
            { uilist_entry( 6, true, 'g', _( "Game…" ) ) },
            { uilist_entry( 2, true, 's', _( "Spawning…" ) ) },
            { uilist_entry( 3, true, 'p', _( "Player…" ) ) },
            { uilist_entry( 7, true, 'v', _( "Vehicle…" ) ) },
            { uilist_entry( 4, true, 't', _( "Teleport…" ) ) },
            { uilist_entry( 5, true, 'm', _( "Map…" ) ) },
        };

        // insert debug-only menu right after "Info".
        menu.insert( menu.begin() + 1, debug_menu.begin(), debug_menu.end() );
    }

    std::string msg;
    if( display_all_entries && !is_debug_character() ) {
        msg = _( "Debug Functions - Using these will cheat not only the game, but yourself.\nYou won't grow.  You won't improve.\nTaking this shortcut will gain you nothing.  Your victory will be hollow.\nNothing will be risked and nothing will be gained." );
    } else {
        msg = _( "Debug Functions" );
    }

    while( true ) {
        const int group = uilist( msg, menu );

        int action;

        switch( group ) {
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
            case 6:
                action = game_uilist();
                break;
            case 7:
                action = vehicle_uilist();
                break;

            default:
                return std::nullopt;
        }
        if( action >= 0 ) {
            return static_cast<debug_menu_index>( action );
        } else {
            return std::nullopt;
        }
    }
}

static void spell_description(
    std::tuple<spell_type, int, std::string> &spl_data, int width, Character &chrc )
{
    std::ostringstream description;

    const int spl_level = std::get<1>( spl_data );
    spell spl( std::get<0>( spl_data ).id );
    spl.set_level( npc(), spl_level );

    nc_color gray = c_light_gray;
    nc_color yellow = c_yellow;
    nc_color light_green = c_light_green;

    // # spell_id
    description << colorize( string_format( "# %s", spl.id().str() ), c_cyan ) << '\n';

    // Name: spell name
    description << string_format( _( "Name: %1$s" ), colorize( spl.name(), c_white ) ) << '\n';

    // Class: Spell Class
    description << string_format( _( "Class: %1$s" ), colorize( spl.spell_class() == trait_NONE ?
                                  _( "Classless" ) : chrc.mutation_name( spl.spell_class() ),
                                  yellow ) ) << "\n";

    // Spell description
    description << spl.description() << '\n';

    // Spell Casting flags
    description << spell_desc::enumerate_spell_data( spl, chrc ) << '\n';

    // Spell Level: 0 / 0 (MAX)
    description << string_format(
                    //~ %1$s - spell current level, %2$s - spell max level, %3$s - is max level
                    _( "Spell Level: %1$s / %2$d %3$s" ),
                    spl_level == -1 ? _( "Unlearned" ) : std::to_string( spl_level ),
                    spl.get_max_level( chrc ),
                    spl_level == spl.get_max_level( chrc ) ? _( "(MAX)" ) : "" ) << '\n';

    // Difficulty: 0 ( 0.0 % Failure Chance)
    description << string_format(
                    //~ %1$d - difficulty, %2$s - failure chance
                    _( "Difficulty: %1$d (%2$s)" ),
                    spl.get_difficulty( chrc ), spl.colorized_fail_percent( chrc ) ) << '\n';

    const std::string impeded = _( "(impeded)" );

    // Casting Cost: 0 (impeded) ( 0 current )
    description << string_format(
                    //~ %1$s - energy cost, %2$s - is casting impeded, %3$s - current character energy
                    _( "Casting Cost: %1$s %2$s (%3$s current) " ),
                    spl.energy_cost_string( chrc ),
                    spell_desc::energy_cost_encumbered( spl, chrc ) ?  impeded : "",
                    spl.energy_cur_string( chrc ) ) << '\n';
    dialogue d( get_talker_for( chrc ), nullptr );
    // Casting Time: 0 (impeded)
    description << string_format(
                    //~ %1$s - cast time, %2$s - is casting impeded, %3$s - casting base time
                    _( "Casting Time: %1$s %2$s (%3$s base time) " ),
                    to_string( time_duration::from_moves( spl.casting_time( chrc ) ) ),
                    spell_desc::casting_time_encumbered( spl, chrc ) ? impeded : "",
                    to_string( time_duration::from_moves( std::get<0>( spl_data ).base_casting_time.evaluate(
                                   d ) ) ) ) << '\n';

    std::string targets;
    if( spl.is_valid_target( spell_target::none ) ) {
        targets = _( "self" );
    } else {
        targets = spl.enumerate_targets();
    }
    description << string_format( _( "Valid Targets: %1$s" ), targets ) << '\n';

    std::string target_ids = spl.list_targeted_monster_names();
    if( !target_ids.empty() ) {
        description << string_format( _( "Only affects the monsters: %1$s" ), target_ids ) << '\n';
    }

    const int damage = spl.damage( chrc );
    const std::string spl_eff = spl.effect();
    std::string damage_string;
    std::string range_string;
    std::string aoe_string;
    // if it's any type of attack spell, the stats are normal.
    if( spl_eff == "attack" ) {
        if( damage > 0 ) {
            std::string dot_string;
            if( spl.damage_dot( chrc ) ) {
                //~ amount of damage per second, abbreviated
                dot_string = string_format( _( ", %1$d/sec" ), spl.damage_dot( chrc ) );
            }
            damage_string = string_format( _( "Damage: %1$s %2$s%3$s" ), spl.damage_string( chrc ),
                                           spl.damage_type_string(), dot_string );
            damage_string = colorize( damage_string, spl.damage_type_color() );
        } else if( damage < 0 ) {
            damage_string = string_format( _( "Healing: %1$s" ), colorize( spl.damage_string( chrc ),
                                           light_green ) );
        }

        if( spl.aoe( chrc ) > 0 ) {
            std::string aoe_string_temp = _( "Spell Radius" );
            std::string degree_string;
            if( spl.shape() == spell_shape::cone ) {
                aoe_string_temp = _( "Cone Arc" );
                degree_string = _( "degrees" );
            } else if( spl.shape() == spell_shape::line ) {
                aoe_string_temp = _( "Line Width" );
            }
            aoe_string = string_format( _( "%1$s: %2$d %3$s" ), aoe_string_temp, spl.aoe( chrc ),
                                        degree_string );
        }

    } else if( spl_eff == "teleport_random" ) {
        if( spl.aoe( chrc ) > 0 ) {
            aoe_string = string_format( _( "Variance: %1$d" ), spl.aoe( chrc ) );
        }

    } else if( spl_eff == "spawn_item" ) {
        if( spl.has_flag( spell_flag::SPAWN_GROUP ) ) {
            // todo: more user-friendly presentation
            damage_string = string_format( _( "Spawn item group %1$s %2$d times" ), spl.effect_data(),
                                           spl.damage( chrc ) );
        } else {
            damage_string = string_format( _( "Spawn %1$d %2$s" ), spl.damage( chrc ),
                                           item::nname( itype_id( spl.effect_data() ), spl.damage( chrc ) ) );
        }
    } else if( spl_eff == "summon" ) {
        std::string monster_name = "FIXME";
        if( spl.has_flag( spell_flag::SPAWN_GROUP ) ) {
            // TODO: Get a more user-friendly group name
            if( MonsterGroupManager::isValidMonsterGroup( mongroup_id( spl.effect_data() ) ) ) {
                monster_name = string_format( _( "from %1$s" ), spl.effect_data() );
            } else {
                debugmsg( "Unknown monster group: %s", spl.effect_data() );
            }
        } else {
            monster_name = monster( mtype_id( spl.effect_data() ) ).get_name();
        }
        damage_string = string_format( _( "Summon: %1$d %2$s" ), spl.damage( chrc ), monster_name );
        aoe_string = string_format( _( "Spell Radius: %1$d" ), spl.aoe( chrc ) );

    } else if( spl_eff == "targeted_polymorph" ) {
        std::string monster_name = spl.effect_data();
        if( spl.has_flag( spell_flag::POLYMORPH_GROUP ) ) {
            // TODO: Get a more user-friendly group name
            if( MonsterGroupManager::isValidMonsterGroup( mongroup_id( spl.effect_data() ) ) ) {
                monster_name = _( "random creature" );
            } else {
                debugmsg( "Unknown monster group: %s", spl.effect_data() );
            }
        } else if( monster_name.empty() ) {
            monster_name = _( "random creature" );
        } else {
            monster_name = mtype_id( spl.effect_data() )->nname();
        }
        damage_string = string_format( _( "Targets under: %1$dhp become a %2$s" ), spl.damage( chrc ),
                                       monster_name );

    } else if( spl_eff == "ter_transform" ) {
        aoe_string = string_format( "Spell Radius: %1$s", spl.aoe_string( chrc ) );

    } else if( spl_eff == "banishment" ) {
        damage_string = string_format( _( "Damage: %1$s %2$s" ), spl.damage_string( chrc ),
                                       spl.damage_type_string() );
        if( spl.aoe( chrc ) > 0 ) {
            aoe_string = string_format( _( "Spell Radius: %1$d" ), spl.aoe( chrc ) );
        }
    }

    // Range / AOE in two columns
    description << string_format( _( "Range: %1$s" ),
                                  spl.range( chrc ) <= 0 ? _( "self" ) : std::to_string( spl.range( chrc ) ) ) << '\n';

    description << aoe_string << '\n';

    // One line for damage / healing / spawn / summon effect
    description << damage_string << '\n';

    // todo: damage over time here, when it gets implemented

    // Show duration for spells that endure
    if( spl.duration( chrc ) > 0 || spl.has_flag( spell_flag::PERMANENT ) ) {
        description << string_format( _( "Duration: %1$s" ), spl.duration_string( chrc ) ) << '\n';
    }

    // helper function for printing tool and item component requirement lists
    const auto print_vec_string = [&]( const std::vector<std::string> &vec ) {
        for( const std::string &line_str : vec ) {
            description << line_str << '\n';
        }
    };

    if( spl.has_components() ) {
        if( !spl.components().get_components().empty() ) {
            print_vec_string( spl.components().get_folded_components_list( width - 2, gray,
                              chrc.crafting_inventory(), return_true<item> ) );
        }
        if( !( spl.components().get_tools().empty() && spl.components().get_qualities().empty() ) ) {
            print_vec_string( spl.components().get_folded_tools_list( width - 2, gray,
                              chrc.crafting_inventory() ) );
        }
    }

    std::get<2>( spl_data ) = description.str();
}

static void change_spells( Character &character )
{
    if( spell_type::get_all().empty() ) {
        add_msg( m_info, _( "There are no spells to change." ) );
        return;
    }

    static character_id last_char_id = character.getID();

    using spell_tuple = std::tuple<spell_type, int, std::string>;
    const size_t spells_all_size = spell_type::get_all().size();
    // all spells with cached string list
    // the string is rebuilt every time it's empty or its level changed
    static std::vector<spell_tuple> spells_all( spells_all_size );
    // maps which spells will show on the list
    std::vector<spell_tuple *> spells_relative( spells_all_size );

    // number of spells changed, current map is invalid
    bool rebuild_string_cache = false;
    if( spells_all.size() != spells_all_size || last_char_id != character.getID() ) {
        rebuild_string_cache = true;
        last_char_id = character.getID();
        spells_all.clear();
    }

    int spname_len = 0;
    for( size_t i = 0; i < spells_all_size; ++i ) {
        if( rebuild_string_cache ) {
            spells_all.emplace_back( spell_type{}, -1, std::string{} );
            std::get<2>( spells_all[i] ).clear();
        }

        if( std::get<0>( spells_all[i] ).id != spell_type::get_all()[i].id ) {
            std::get<0>( spells_all[i] ) = spell_type::get_all()[i];
            std::get<1>( spells_all[i] ) = -1;
            std::get<2>( spells_all[i] ).clear();
        }

        spells_relative[i] = &spells_all[i];

        // get max spell name length
        spname_len = std::max( spname_len, utf8_width( std::get<0>( spells_all[i] ).name.translated() ) );
    }
    spname_len += 2;

    // fill the levels for spells the character knowns
    for( const spell *sp : character.magic->get_spells() ) {
        auto iterator = std::find_if( spells_all.begin(),
        spells_all.end(), [&sp]( spell_tuple & spt ) -> bool {
            return std::get<0>( spt ).id == sp->id();
        } );
        std::get<1>( spells_all[iterator - spells_all.begin()] ) = sp->get_level();
        std::get<2>( spells_all[iterator - spells_all.begin()] ).clear();
    }

    auto set_spell = [&character]( spell_type & splt, int spell_level ) {
        if( spell_level == -1 ) {
            character.magic->get_spellbook().erase( splt.id );
            return;
        } else if( !character.magic->knows_spell( splt.id ) ) {
            spell spl( splt.id );
            character.magic->get_spellbook().emplace( splt.id, spl );
        }

        character.magic->get_spell( splt.id ).set_exp( spell::exp_for_level( spell_level ) );
    };

    ui_adaptor spellsui;
    border_helper borders;

    struct win_info {
        catacurses::window window;
        border_helper::border_info *border = nullptr;
        int width;
        point start;
    };

    struct win_info w_name;
    w_name.border = &borders.add_border();
    w_name.width = spname_len + 1;
    w_name.start = point_zero;

    struct win_info w_level;
    w_level.border = &borders.add_border();
    w_level.width = 11;
    w_level.start = {w_name.width, 0};

    struct win_info w_descborder;
    w_descborder.border = &borders.add_border();

    // desc is inside descborder with a padding of 2 characters
    struct win_info w_desc;

    scrollbar scrllbr;
    scrllbr.offset_x( 0 ).offset_y( 1 ).border_color( c_magenta );

    spellsui.on_screen_resize( [&]( ui_adaptor & ui ) {

        w_descborder.start = {w_level.start.x + w_level.width, 0};
        w_descborder.width = TERMX - w_descborder.start.x;

        w_desc.width = w_descborder.width - 4;
        w_desc.start = {w_descborder.start.x + 2, 1};

        w_name.window = catacurses::newwin( TERMY, w_name.width, w_name.start );
        w_level.window = catacurses::newwin( TERMY, w_level.width, w_level.start );
        w_descborder.window = catacurses::newwin( TERMY, w_descborder.width, w_descborder.start );
        w_desc.window = catacurses::newwin( TERMY - 2, w_desc.width, w_desc.start );

        w_name.border->set( w_name.start, { w_name.width, TERMY } );
        w_level.border->set( w_level.start, { w_level.width, TERMY } );
        w_descborder.border->set( w_descborder.start, { w_descborder.width, TERMY } );

        scrllbr.viewport_size( TERMY - 2 );
        ui.position( point_zero, { TERMX, TERMY } );
    } );
    spellsui.mark_resize();

    input_context ctxt( "DEBUG_SPELLS" );
    ctxt.register_action( "UNLEARN_SPELL" ); // Quickly unlearn a spell
    ctxt.register_action( "TOGGLE_ALL_SPELL" ); // Cycle level on all spells in spells_relative
    ctxt.register_action( "SHOW_ONLY_LEARNED" ); // Removes all unlearned spells in spells_relative
    ctxt.register_navigate_ui_list();
    ctxt.register_leftright(); // left and right change spell level
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" ); // set a spell to a level
    ctxt.register_action( "FILTER" );
    ctxt.register_action( "RESET_FILTER" );
    ctxt.register_action( "HELP_KEYBINDINGS" );

    int spells_start = 0;
    int spell_selected = 0;
    std::string filterstring;
    spellsui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_name.window );
        werase( w_level.window );
        werase( w_descborder.window );
        werase( w_desc.window );

        borders.draw_border( w_name.window, c_magenta );
        borders.draw_border( w_level.window, c_magenta );
        borders.draw_border( w_descborder.window, c_magenta );

        center_print( w_name.window, 0, c_magenta, _( "<<color_white>Spell Name</color>>" ) );
        center_print( w_level.window, 0, c_magenta, _( "<<color_white>Level</color>>" ) );
        center_print( w_descborder.window, 0, c_magenta, _( "<<color_white>Description</color>>" ) );

        nc_color magenta = c_magenta;
        const std::string help_keybindings = string_format(
                _( "<<color_white>[<color_yellow>%1$s</color>] Keybindings</color>>" ),
                ctxt.get_desc( "HELP_KEYBINDINGS" ) );
        print_colored_text( w_descborder.window,
                            point( w_descborder.width - help_keybindings.length() + 42, TERMY - 1 ),
                            magenta, magenta, help_keybindings );

        std::string help_filter;
        if( filterstring.empty() ) {
            help_filter = string_format( _( "<<color_white>[<color_yellow>%1$s</color>] Filter</color>>" ),
                                         ctxt.get_desc( "FILTER" ) );
        } else {
            help_filter = string_format( "<<color_white>%s</color>>", filterstring );
        }

        print_colored_text( w_name.window, point( 1, TERMY - 1 ),
                            magenta, magenta, help_filter );

        const int relative_size = spells_relative.size();
        scrllbr.content_size( relative_size );
        scrllbr.viewport_pos( spell_selected );
        scrllbr.apply( w_name.window );

        calcStartPos( spells_start, spell_selected, TERMY - 2, relative_size );

        int line_number = 1;
        dialogue d( get_talker_for( get_player_character() ), nullptr );
        for( int i = spells_start; i < relative_size; ++i ) {
            if( line_number == TERMY - 1 ) {
                break;
            }

            const spell_type &splt = std::get<0>( *spells_relative[i] );
            const int &spell_level = std::get<1>( *spells_relative[i] );

            nc_color spell_color = spell_level > -1 ? c_green : c_light_gray;
            spell_color = i == spell_selected ? hilite( spell_color ) : spell_color;

            mvwprintz( w_name.window, point( 2, line_number ),
                       spell_color, splt.name.translated() );
            mvwprintz( w_level.window, point( 1, line_number++ ), spell_color,
                       _( "%1$3d /%2$3d" ), spell_level, static_cast<int>( splt.max_level.evaluate( d ) ) );
        }

        nc_color gray = c_light_gray;
        print_colored_text( w_desc.window, point_zero, gray, gray,
                            std::get<2>( *spells_relative[spell_selected] ) );

        wnoutrefresh( w_name.window );
        wnoutrefresh( w_level.window );
        wnoutrefresh( w_descborder.window );
        wnoutrefresh( w_desc.window );
    } );

    auto update_description = [&]( bool force ) -> void {
        if( force || std::get<2>( *spells_relative[spell_selected] ).empty() )
        {
            spell_description( *spells_relative[spell_selected], w_desc.width, character );
        }
    };

    // keep the same spell selected
    auto spell_middle_or_id = [&]( const spell_id & spellid ) -> void {
        if( spellid.is_empty() )
        {
            spell_selected = 0;
            return;
        }

        // in case we don't find anything, keep selection in the middle of screen
        const size_t spells_relative_size = spells_relative.size();
        spell_selected = std::min( ( TERMY - 2 ) / 2, static_cast<int>( spells_relative_size ) / 2 );
        for( size_t i = 0; i < spells_relative_size; ++i )
        {
            if( std::get<0>( *spells_relative[i] ).id == spellid ) {
                spell_selected = i;
                break;
            }
        }
    };

    // reset spells_relative vector
    auto reset_spells_relative = [&]() -> void {
        for( spell_tuple &spt : spells_all )
        {
            spells_relative.emplace_back( &spt );
        }
    };

    auto filter_spells = [&]( ) -> void {
        const spell_id &spellid = std::get<0>( *spells_relative[spell_selected] ).id;
        spells_relative.clear();
        if( filterstring.empty() )
        {
            reset_spells_relative();
        } else
        {
            for( spell_tuple &spt : spells_all ) {
                const spell_type &spl = std::get<0>( spt );
                if( lcmatch( spl.name.translated(), filterstring ) ||
                    lcmatch( spl.id.str(), filterstring ) ) {
                    spells_relative.emplace_back( &spt );
                }
            }

            // no spell found, reset relative list
            if( spells_relative.empty() ) {
                reset_spells_relative();
                popup( _( "Nothing found." ) );
            }
        }

        spell_middle_or_id( spellid );
    };

    auto toggle_all_spells = [&]( int level, Character & character ) {
        dialogue d( get_talker_for( character ), nullptr );
        // -2 sets it to max level
        for( spell_tuple *spt : spells_relative ) {
            std::get<1>( *spt ) = level > -2 ? level : std::get<0>( *spt ).max_level.evaluate( d );
            set_spell( std::get<0>( *spt ), std::get<1>( *spt ) );
            std::get<2>( *spt ).clear();
        }
    };

    static spell_id last_selected_spellid;
    spell_middle_or_id( last_selected_spellid );

    // 0 -> turn off all spells
    // 1 -> set all spells to level 0
    // 2 -> set all spells to their max level
    int toggle_spells_state = 1;

    bool showing_only_learned = false;

    bool force_update_description = false;
    dialogue d( get_talker_for( character ), nullptr );
    while( true ) {
        update_description( force_update_description );
        force_update_description = false;

        ui_manager::redraw();
        const std::string action = ctxt.handle_input();

        if( action == "QUIT" ) {
            last_selected_spellid = std::get<0>( *spells_relative[spell_selected] ).id;
            break;

        } else if( action == "FILTER" ) {
            string_input_popup()
            .title( _( "Filter:" ) )
            .width( 16 )
            .description( _( "Filter by spell name or id" ) )
            .edit( filterstring );

            showing_only_learned = false;
            filter_spells( );

        } else if( action == "RESET_FILTER" ) {
            showing_only_learned = false;
            filterstring.clear();
            filter_spells();

        } else if( navigate_ui_list( action, spell_selected, 10, spells_relative.size(), true ) ) {
        } else if( action == "LEFT" ) {
            int &spell_level = std::get<1>( *spells_relative[spell_selected] );
            spell_level = std::max( -1, spell_level - 1 );
            set_spell( std::get<0>( *spells_relative[spell_selected] ), spell_level );
            force_update_description = true;

        } else if( action == "RIGHT" ) {
            int &spell_level = std::get<1>( *spells_relative[spell_selected] );
            spell_level = std::min( spell_level + 1,
                                    static_cast<int>( std::get<0>( *spells_relative[spell_selected] ).max_level.evaluate( d ) ) );
            set_spell( std::get<0>( *spells_relative[spell_selected] ), spell_level );
            force_update_description = true;

        } else if( action == "CONFIRM" ) {
            int &spell_level = std::get<1>( *spells_relative[spell_selected] );
            query_int( spell_level, _( "Set spell level to?  Currently: %1$d" ), spell_level );
            spell_level = clamp( spell_level, -1,
                                 static_cast<int>( std::get<0>( *spells_relative[spell_selected] ).max_level.evaluate( d ) ) );
            set_spell( std::get<0>( *spells_relative[spell_selected] ), spell_level );
            force_update_description = true;

        } else if( action == "UNLEARN_SPELL" ) {
            int &spell_level = std::get<1>( *spells_relative[spell_selected] );
            spell_level = -1;
            set_spell( std::get<0>( *spells_relative[spell_selected] ), spell_level );
            force_update_description = true;

        } else if( action == "TOGGLE_ALL_SPELL" ) {
            if( toggle_spells_state == 0 ) {
                toggle_spells_state = 1;
                toggle_all_spells( -1, character ); // unlearn all spells
            } else if( toggle_spells_state == 1 ) {
                toggle_spells_state = 2;
                toggle_all_spells( 0, character ); // sets all spells to the minimum level
            } else  {
                toggle_spells_state = 0;
                toggle_all_spells( -2, character ); // max level
            }

        } else if( action == "SHOW_ONLY_LEARNED" ) {
            showing_only_learned = !showing_only_learned;

            const spell_id &spellid = std::get<0>( *spells_relative[spell_selected] ).id;
            spells_relative.clear();
            if( showing_only_learned ) {
                for( spell_tuple &spt : spells_all ) {
                    if( std::get<1>( spt ) > -1 ) {
                        spells_relative.emplace_back( &spt );
                    }
                }

                if( spells_relative.empty() ) {
                    popup( _( "Nothing found." ) );
                    showing_only_learned = false;
                }
            }

            if( !showing_only_learned ) {
                reset_spells_relative();
            }

            spell_middle_or_id( spellid );
        }
    }
}

static void spawn_artifact()
{
    map &here = get_map();
    uilist relic_menu;
    std::vector<relic_procgen_id> relic_list;
    for( const relic_procgen_data &elem : relic_procgen_data::get_all() ) {
        relic_list.emplace_back( elem.id );
    }
    relic_menu.text = _( "Choose artifact data:" );
    std::sort( relic_list.begin(), relic_list.end(), localized_compare );
    int menu_ind = 0;
    for( auto &elem : relic_list ) {
        relic_menu.addentry( menu_ind, true, MENU_AUTOASSIGN, elem.c_str() );
        ++menu_ind;
    }
    relic_menu.query();
    int artifact_max_attributes;
    int artifact_power_level;
    bool artifact_is_resonant = false;
    int artifact_max_negative_value;
    if( relic_menu.ret >= 0 && relic_menu.ret < static_cast<int>( relic_list.size() ) ) {
        if( query_int( artifact_max_attributes, _( "Enter max attributes:" ) )
            && query_int( artifact_power_level, _( "Enter power level:" ) )
            && query_int( artifact_max_negative_value, _( "Enter negative power limit:" ) ) ) {
            if( const std::optional<tripoint> center = g->look_around() ) {
                if( query_yn( _( "Is the artifact resonant?" ) ) ) {
                    artifact_is_resonant = true;
                }
                here.spawn_artifact( *center, relic_list[relic_menu.ret], artifact_max_attributes,
                                     artifact_power_level, artifact_max_negative_value, artifact_is_resonant );
            }
        }
    }
}

static void teleport_short()
{
    const std::optional<tripoint> where = g->look_around();
    const Character &player_character = get_player_character();
    if( !where || *where == player_character.pos() ) {
        return;
    }
    g->place_player( *where );
    const tripoint new_pos( player_character.pos() );
    add_msg( _( "You teleport to point %s." ), new_pos.to_string() );
}

static void teleport_long()
{
    const tripoint_abs_omt where( ui::omap::choose_point( true ) );
    if( where == overmap::invalid_tripoint ) {
        return;
    }
    g->place_player_overmap( where );
    add_msg( _( "You teleport to submap %s." ), where.to_string() );
}

static void teleport_overmap( bool specific_coordinates = false )
{
    Character &player_character = get_player_character();
    tripoint_abs_omt where;
    if( specific_coordinates ) {
        const std::string text = string_input_popup()
                                 .title( _( "Teleport where?" ) )
                                 .width( 20 )
                                 .query_string();
        if( text.empty() ) {
            return;
        }
        const std::vector<std::string> coord_strings = string_split( text, ',' );
        if( coord_strings.size() < 2 || coord_strings.size() > 3 ) {
            popup( _( "Error interpreting teleport target: "
                      "expected two or three comma-separated values; got %zu" ),
                   coord_strings.size() );
            return;
        }
        std::vector<int> coord_ints;
        for( const std::string &coord_string : coord_strings ) {
            ret_val<int> parsed_coord = try_parse_integer<int>( coord_string, true );
            if( !parsed_coord.success() ) {
                popup( _( "Error interpreting teleport target: %s" ), parsed_coord.str() );
                return;
            }
            coord_ints.push_back( parsed_coord.value() );
        }
        cata_assert( coord_ints.size() >= 2 );
        tripoint coord;
        coord.x = coord_ints[0];
        coord.y = coord_ints[1];
        coord.z = coord_ints.size() >= 3 ? coord_ints[2] : 0;
        where = tripoint_abs_omt( OMAPX * coord.x, OMAPY * coord.y, coord.z );
    } else {
        const std::optional<tripoint> dir_ = choose_direction( _( "Where is the desired overmap?" ) );
        if( !dir_ ) {
            return;
        }
        const tripoint offset = tripoint( OMAPX * dir_->x, OMAPY * dir_->y, dir_->z );
        where = player_character.global_omt_location() + offset;
    }
    g->place_player_overmap( where );

    const tripoint_abs_om new_pos =
        project_to<coords::om>( player_character.global_omt_location() );
    add_msg( _( "You teleport to overmap %s." ), new_pos.to_string() );
}

static void teleport_city()
{
    std::vector<city> cities( city::get_all() );
    const auto cities_cmp_population = []( const city & a, const city & b ) {
        return std::tie( a.population, a.name ) > std::tie( b.population, b.name );
    };
    std::sort( cities.begin(), cities.end(), cities_cmp_population );
    uilist cities_menu;
    ui::omap::setup_cities_menu( cities_menu, cities );
    std::optional<city> c = ui::omap::select_city( cities_menu, cities, false );
    if( c.has_value() ) {
        const tripoint_abs_omt where = tripoint_abs_omt(
                                           project_to<coords::omt>( c->pos_om ) + c->pos.raw(), 0 );
        g->place_player_overmap( where );
    }
}

static void spawn_nested_mapgen()
{
    uilist nest_menu;
    std::vector<nested_mapgen_id> nest_ids;
    for( auto &nested : nested_mapgens ) {
        nest_menu.addentry( -1, true, -1, nested.first.str() );
        nest_ids.push_back( nested.first );
    }
    nest_menu.query();
    const int nest_choice = nest_menu.ret;
    if( nest_choice >= 0 && nest_choice < static_cast<int>( nest_ids.size() ) ) {
        const std::optional<tripoint> where = g->look_around();
        if( !where ) {
            return;
        }

        map &here = get_map();
        const tripoint_abs_ms abs_ms( here.getabs( *where ) );
        const tripoint_abs_omt abs_omt = project_to<coords::omt>( abs_ms );
        const tripoint_abs_sm abs_sub = project_to<coords::sm>( abs_ms );

        map target_map;
        target_map.load( abs_sub, true );
        const tripoint local_ms = target_map.getlocal( abs_ms );
        mapgendata md( abs_omt, target_map, 0.0f, calendar::turn, nullptr );
        const auto &ptr = nested_mapgens[nest_ids[nest_choice]].funcs().pick();
        if( ptr == nullptr ) {
            return;
        }
        ( *ptr )->nest( md, local_ms.xy(), "debug menu" );
        target_map.save();
        g->load_npcs();
        here.invalidate_map_cache( here.get_abs_sub().z() );
    }
}

static void control_npc_menu()
{
    get_avatar().control_npc_menu( true );
}

static void character_edit_stats_menu( Character &you )
{
    uilist smenu;
    smenu.addentry( 0, true, 'S', "%s: %d", _( "Maximum strength" ), you.str_max );
    smenu.addentry( 1, true, 'D', "%s: %d", _( "Maximum dexterity" ), you.dex_max );
    smenu.addentry( 2, true, 'I', "%s: %d", _( "Maximum intelligence" ), you.int_max );
    smenu.addentry( 3, true, 'P', "%s: %d", _( "Maximum perception" ), you.per_max );
    smenu.query();
    int *bp_ptr = nullptr;
    switch( smenu.ret ) {
        case 0:
            bp_ptr = &you.str_max;
            break;
        case 1:
            bp_ptr = &you.dex_max;
            break;
        case 2:
            bp_ptr = &you.int_max;
            break;
        case 3:
            bp_ptr = &you.per_max;
            break;
        default:
            break;
    }

    if( bp_ptr != nullptr ) {
        int value;
        if( query_int( value, _( "Set the stat to?  Currently: %d" ), *bp_ptr ) && value >= 0 ) {
            *bp_ptr = value;
            you.reset_stats();
        }
    }
}

static void character_edit_needs_menu( Character &you )
{
    std::pair<std::string, nc_color> hunger_pair = display::hunger_text_color( you );
    std::pair<std::string, nc_color> thirst_pair = display::thirst_text_color( you );
    std::pair<std::string, nc_color> fatigue_pair = display::fatigue_text_color( you );
    std::pair<std::string, nc_color> weariness_pair = display::weariness_text_color( you );

    std::stringstream data;
    data << string_format( _( "Hunger: %d  %s" ), you.get_hunger(), colorize( hunger_pair.first,
                           hunger_pair.second ) ) << std::endl;
    data << string_format( _( "Thirst: %d  %s" ), you.get_thirst(), colorize( thirst_pair.first,
                           thirst_pair.second ) ) << std::endl;
    data << string_format( _( "Fatigue: %d  %s" ), you.get_fatigue(), colorize( fatigue_pair.first,
                           fatigue_pair.second ) ) << std::endl;
    data << string_format( _( "Weariness: %d  %s" ), you.weariness(), colorize( weariness_pair.first,
                           weariness_pair.second ) ) << std::endl;
    data << std::endl;
    data << _( "Stored kCal: " ) << you.get_stored_kcal() << std::endl;
    data << _( "Total kCal: " ) << you.get_stored_kcal() + you.stomach.get_calories() +
         you.guts.get_calories() << std::endl;
    data << std::endl;
    data << _( "Stomach contents" ) << std::endl;
    data << _( "  Total volume: " ) << vol_to_string( you.stomach.contains() ) << std::endl;
    data << _( "  Water volume: " ) << vol_to_string( you.stomach.get_water() ) << std::endl;
    data << string_format( _( "  kCal: %d" ), you.stomach.get_calories() ) << std::endl;
    data << std::endl;
    data << _( "Gut contents" ) << std::endl;
    data << _( "  Total volume: " ) << vol_to_string( you.guts.contains() ) << std::endl;
    data << _( "  Water volume: " ) << vol_to_string( you.guts.get_water() ) << std::endl;
    data << string_format( _( "  kCal: %d" ), you.guts.get_calories() ) << std::endl;

    uilist smenu;
    smenu.text = data.str();
    smenu.addentry( 0, true, 'h', "%s: %d", _( "Hunger" ), you.get_hunger() );
    smenu.addentry( 1, true, 's', "%s: %d", _( "Stored kCal" ), you.get_stored_kcal() );
    smenu.addentry( 2, true, 'S', "%s: %d", _( "Stomach kCal" ), you.stomach.get_calories() );
    smenu.addentry( 3, true, 'G', "%s: %d", _( "Gut kCal" ), you.guts.get_calories() );
    smenu.addentry( 4, true, 't', "%s: %d", _( "Thirst" ), you.get_thirst() );
    smenu.addentry( 5, true, 'f', "%s: %d", _( "Fatigue" ), you.get_fatigue() );
    smenu.addentry( 6, true, 'd', "%s: %d", _( "Sleep Deprivation" ), you.get_sleep_deprivation() );
    smenu.addentry( 7, true, 'w', "%s: %d", _( "Weariness" ), you.weariness() );
    smenu.addentry( 8, true, 'a', _( "Reset all basic needs" ) );
    smenu.addentry( 9, true, 'e', _( "Empty stomach and guts" ) );

    const auto &vits = vitamin::all();
    for( const auto &v : vits ) {
        smenu.addentry( -1, true, 0, _( "%s: daily %d, overall %d" ), v.second.name(),
                        you.get_daily_vitamin( v.first ), you.vitamin_get( v.first ) );
    }

    smenu.query();
    int value;
    switch( smenu.ret ) {
        case 0:
            if( query_int( value, _( "Set hunger to?  Currently: %d" ), you.get_hunger() ) ) {
                you.set_hunger( value );
            }
            break;

        case 1:
            if( query_int( value, _( "Set stored kCal to?  Currently: %d" ), you.get_stored_kcal() ) ) {
                you.set_stored_kcal( value );
            }
            break;

        case 2:
            if( query_int( value, _( "Set stomach kCal to?  Currently: %d" ), you.stomach.get_calories() ) ) {
                you.stomach.mod_calories( value - you.stomach.get_calories() );
            }
            break;

        case 3:
            if( query_int( value, _( "Set gut kCal to?  Currently: %d" ), you.guts.get_calories() ) ) {
                you.guts.mod_calories( value - you.guts.get_calories() );
            }
            break;

        case 4:
            if( query_int( value, _( "Set thirst to?  Currently: %d" ), you.get_thirst() ) ) {
                you.set_thirst( value );
            }
            break;

        case 5:
            if( query_int( value, _( "Set fatigue to?  Currently: %d" ), you.get_fatigue() ) ) {
                you.set_fatigue( value );
            }
            break;

        case 6:
            if( query_int( value, _( "Set sleep deprivation to?  Currently: %d" ),
                           you.get_sleep_deprivation() ) ) {
                you.set_sleep_deprivation( value );
            }
            break;

        case 7:
            if( query_yn( _( "Reset weariness?  Currently: %d" ),
                          you.weariness() ) ) {
                you.activity_history.weary_clear();
            }
            break;
        case 8:
            you.initialize_stomach_contents();
            you.set_hunger( 0 );
            you.set_thirst( 0 );
            you.set_fatigue( 0 );
            you.set_sleep_deprivation( 0 );
            you.set_stored_kcal( you.get_healthy_kcal() );
            you.activity_history.weary_clear();
            break;
        case 9:
            you.stomach.empty();
            you.guts.empty();
            break;
        default:
            if( smenu.ret >= 10 && smenu.ret < static_cast<int>( vits.size() + 10 ) ) {
                auto iter = std::next( vits.begin(), smenu.ret - 10 );
                if( query_int( value, _( "Set %s to?  Currently: %d" ),
                               iter->second.name(), you.vitamin_get( iter->first ) ) ) {
                    you.vitamin_set( iter->first, value );
                }
            }
    }
}

static void character_edit_hp_menu( Character &you )
{
    const int torso_hp = you.get_part_hp_cur( bodypart_id( "torso" ) );
    const int head_hp = you.get_part_hp_cur( bodypart_id( "head" ) );
    const int arm_l_hp = you.get_part_hp_cur( bodypart_id( "arm_l" ) );
    const int arm_r_hp = you.get_part_hp_cur( bodypart_id( "arm_r" ) );
    const int leg_l_hp = you.get_part_hp_cur( bodypart_id( "leg_l" ) );
    const int leg_r_hp = you.get_part_hp_cur( bodypart_id( "leg_r" ) );
    uilist smenu;
    smenu.addentry( 0, true, 'q', "%s: %d", _( "Torso" ), torso_hp );
    smenu.addentry( 1, true, 'w', "%s: %d", _( "Head" ), head_hp );
    smenu.addentry( 2, true, 'a', "%s: %d", _( "Left arm" ), arm_l_hp );
    smenu.addentry( 3, true, 's', "%s: %d", _( "Right arm" ), arm_r_hp );
    smenu.addentry( 4, true, 'z', "%s: %d", _( "Left leg" ), leg_l_hp );
    smenu.addentry( 5, true, 'x', "%s: %d", _( "Right leg" ), leg_r_hp );
    smenu.addentry( 6, true, 'e', "%s: %d", _( "All" ), you.get_lowest_hp() );
    smenu.query();
    bodypart_str_id bp = body_part_no_a_real_part;
    int bp_ptr = -1;
    bool all_select = false;

    switch( smenu.ret ) {
        case 0:
            bp = body_part_torso;
            bp_ptr = torso_hp;
            break;
        case 1:
            bp = body_part_head;
            bp_ptr = head_hp;
            break;
        case 2:
            bp = body_part_arm_l;
            bp_ptr = arm_l_hp;
            break;
        case 3:
            bp = body_part_arm_r;
            bp_ptr = arm_r_hp;
            break;
        case 4:
            bp = body_part_leg_l;
            bp_ptr = leg_l_hp;
            break;
        case 5:
            bp = body_part_leg_r;
            bp_ptr = leg_r_hp;
            break;
        case 6:
            all_select = true;
            break;
        default:
            break;
    }

    if( bp.is_valid() ) {
        int value;
        if( query_int( value, _( "Set the hitpoints to?  Currently: %d" ), bp_ptr ) && value >= 0 ) {
            you.set_part_hp_cur( bp.id(), value );
            you.reset_stats();
        }
    } else if( all_select ) {
        int value;
        if( query_int( value, _( "Set the hitpoints to?  Currently: %d" ), you.get_lowest_hp() ) &&
            value >= 0 ) {
            for( bodypart_id part_id : you.get_all_body_parts( get_body_part_flags::only_main ) ) {
                you.set_part_hp_cur( part_id, value );
            }
            you.reset_stats();
        }
    }
}

static void character_edit_opinion_menu( npc *np )
{
    uilist smenu;
    smenu.addentry( 0, true, 'h', "%s: %d", _( "trust" ), np->op_of_u.trust );
    smenu.addentry( 1, true, 's', "%s: %d", _( "fear" ), np->op_of_u.fear );
    smenu.addentry( 2, true, 't', "%s: %d", _( "value" ), np->op_of_u.value );
    smenu.addentry( 3, true, 'f', "%s: %d", _( "anger" ), np->op_of_u.anger );
    smenu.addentry( 4, true, 'd', "%s: %d", _( "owed" ), np->op_of_u.owed );
    smenu.addentry( 5, true, 'd', "%s: %d", _( "sold" ), np->op_of_u.sold );

    smenu.query();
    int value;
    switch( smenu.ret ) {
        case 0:
            if( query_int( value, _( "Set trust to?  Currently: %d" ),
                           np->op_of_u.trust ) ) {
                np->op_of_u.trust = value;
            }
            break;
        case 1:
            if( query_int( value, _( "Set fear to?  Currently: %d" ), np->op_of_u.fear ) ) {
                np->op_of_u.fear = value;
            }
            break;
        case 2:
            if( query_int( value, _( "Set value to?  Currently: %d" ),
                           np->op_of_u.value ) ) {
                np->op_of_u.value = value;
            }
            break;
        case 3:
            if( query_int( value, _( "Set anger to?  Currently: %d" ),
                           np->op_of_u.anger ) ) {
                np->op_of_u.anger = value;
            }
            break;
        case 4:
            if( query_int( value, _( "Set owed to?  Currently: %d" ), np->op_of_u.owed ) ) {
                np->op_of_u.owed = value;
            }
            break;
        case 5:
            if( query_int( value, _( "Set sold to?  Currently: %d" ), np->op_of_u.sold ) ) {
                np->op_of_u.sold = value;
            }
            break;
    }
}

static void character_edit_desc_menu( Character &you )
{
    uilist smenu;
    std::string current_bloodt = io::enum_to_string( you.my_blood_type ) + ( you.blood_rh_factor ? "+" :
                                 "-" );
    smenu.text = _( "Select a value and press enter to change it." );
    if( you.is_avatar() ) {
        smenu.addentry( 0, true, 's', "%s: %s", _( "Current save file name" ), get_avatar().get_save_id() );
    }
    smenu.addentry( 1, true, 'n', "%s: %s", _( "Current pre-Cataclysm name" ), you.name );
    smenu.addentry( 2, true, 'a', "%s: %d", _( "Current age" ), you.base_age() );
    smenu.addentry( 3, true, 'h', "%s: %d", _( "Current height in cm" ), you.base_height() );
    smenu.addentry( 4, true, 'b', "%s: %s", _( "Current blood type" ), current_bloodt );
    smenu.query();
    switch( smenu.ret ) {
        case 0: {
            std::string filterstring = get_avatar().get_save_id();
            string_input_popup popup;
            popup
            .title( _( "Rename save file (WARNING: this will duplicate the save):" ) )
            .width( 85 )
            .edit( filterstring );
            if( popup.confirmed() ) {
                get_avatar().set_save_id( filterstring );
            }
        }
        break;
        case 1: {
            std::string filterstring = you.name;
            string_input_popup popup;
            popup
            .title( _( "Rename character:" ) )
            .width( 85 )
            .edit( filterstring );
            if( popup.confirmed() ) {
                you.name = filterstring;
            }
        }
        break;
        case 2: {
            string_input_popup popup;
            popup.title( _( "Enter age in years.  Minimum 16, maximum 55" ) )
            .text( string_format( "%d", you.base_age() ) )
            .only_digits( true );
            const int result = popup.query_int();
            if( result != 0 ) {
                you.set_base_age( clamp( result, 16, 55 ) );
            }
        }
        break;
        case 3: {
            string_input_popup popup;
            popup.title( string_format( _( "Enter height in centimeters.  Minimum %d, maximum %d" ),
                                        Character::min_height(),
                                        Character::max_height() ) )
            .text( string_format( "%d", you.base_height() ) )
            .only_digits( true );
            const int result = popup.query_int();
            if( result != 0 ) {
                you.set_base_height( clamp( result, Character::min_height(), Character::max_height() ) );
            }
        }
        break;
        case 4: {
            uilist btype;
            btype.text = _( "Select blood type" );
            btype.addentry( static_cast<int>( blood_type::blood_O ), true, '1', "O" );
            btype.addentry( static_cast<int>( blood_type::blood_A ), true, '2', "A" );
            btype.addentry( static_cast<int>( blood_type::blood_B ), true, '3', "B" );
            btype.addentry( static_cast<int>( blood_type::blood_AB ), true, '4', "AB" );
            btype.query();
            if( btype.ret < 0 ) {
                break;
            }
            uilist bfac;
            bfac.text = _( "Select Rh factor" );
            bfac.addentry( 0, true, '-', _( "negative" ) );
            bfac.addentry( 1, true, '+', _( "positive" ) );
            bfac.query();
            if( bfac.ret < 0 ) {
                break;
            }
            you.my_blood_type = static_cast<blood_type>( btype.ret );
            you.blood_rh_factor = static_cast<bool>( bfac.ret );
            break;
        }
    }
}

static void character_edit_menu()
{
    std::vector< tripoint > locations;
    uilist charmenu;
    int charnum = 0;
    avatar &player_character = get_avatar();
    charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, "%s", _( "You" ) );
    locations.emplace_back( player_character.pos() );
    for( const npc &guy : g->all_npcs() ) {
        charmenu.addentry( charnum++, true, MENU_AUTOASSIGN, guy.get_name() );
        locations.emplace_back( guy.pos() );
    }

    pointmenu_cb callback( locations );
    charmenu.callback = &callback;
    charmenu.w_y_setup = 0;
    charmenu.query();
    if( charmenu.ret < 0 || static_cast<size_t>( charmenu.ret ) >= locations.size() ) {
        return;
    }
    const size_t index = charmenu.ret;
    // The NPC is also required for "Add mission", so has to be in this scope
    npc *np = get_creature_tracker().creature_at<npc>( locations[index], false );
    Character &you = np ? *np->as_character() : *player_character.as_character();
    uilist nmenu;

    if( np != nullptr ) {
        std::stringstream data;
        data << np->get_name() << " - " << ( np->male ? _( "Male" ) : _( "Female" ) ) << " " <<
             np->myclass->get_name() << std::endl;
        if( !np->get_unique_id().empty() ) {
            data << string_format( _( "Unique Id: %s" ), np->get_unique_id() ) << std::endl;
        }
        data << string_format( _( "Faction: %s (api v%d)" ), np->get_faction()->id.str(),
                               np->get_faction_ver() ) << "; "
             << string_format( _( "Attitude: %s" ), npc_attitude_name( np->get_attitude() ) ) << std::endl;
        if( np->has_destination() ) {
            data << string_format(
                     _( "Destination: %s %s" ), np->goal.to_string(),
                     overmap_buffer.ter( np->goal )->get_name() ) << std::endl;
        }
        data << string_format( _( "Trust: %d" ), np->op_of_u.trust ) << " "
             << string_format( _( "Fear: %d" ), np->op_of_u.fear ) << " "
             << string_format( _( "Value: %d" ), np->op_of_u.value ) << " "
             << string_format( _( "Anger: %d" ), np->op_of_u.anger ) << std::endl;
        data << string_format( _( "Owed: %d" ), np->op_of_u.owed ) << " "
             << string_format( _( "Sold: %d" ), np->op_of_u.sold ) << std::endl;

        data << string_format( _( "Aggression: %d" ),
                               static_cast<int>( np->personality.aggression ) ) << " "
             << string_format( _( "Bravery: %d" ), static_cast<int>( np->personality.bravery ) ) << " "
             << string_format( _( "Collector: %d" ), static_cast<int>( np->personality.collector ) ) << " "
             << string_format( _( "Altruism: %d" ), static_cast<int>( np->personality.altruism ) ) << std::endl;

        data << _( "Needs:" );
        for( const npc_need &need : np->needs ) {
            data << " " << npc::get_need_str_id( need );
        }
        data << std::endl;
        data << string_format( _( "Total morale: %d" ), np->get_morale_level() ) << std::endl;

        nmenu.text = data.str();
    } else {
        nmenu.text = _( "Player" );
    }

    enum {
        D_DESC, D_SKILLS, D_THEORY, D_PROF, D_STATS, D_SPELLS, D_ITEMS, D_DELETE_ITEMS, D_DROP_ITEMS, D_ITEM_WORN,
        D_HP, D_STAMINA, D_MORALE, D_PAIN, D_NEEDS, D_HEALTHY, D_STATUS, D_MISSION_ADD, D_MISSION_EDIT,
        D_TELE, D_MUTATE, D_CLASS, D_ATTITUDE, D_OPINION, D_ADD_EFFECT, D_ASTHMA, D_PRINT_VARS,
        D_WRITE_EOCS, D_KILL_XP, D_CHECK_TEMP, D_EDIT_VARS
    };
    nmenu.addentry( D_DESC, true, 'D', "%s",
                    _( "Edit description - name, age, height or blood type" ) );
    nmenu.addentry( D_SKILLS, true, 's', "%s", _( "Edit skills" ) );
    nmenu.addentry( D_THEORY, true, 'T', "%s", _( "Edit theoretical skill knowledge" ) );
    nmenu.addentry( D_PROF, true, 'P', "%s", _( "Edit proficiencies" ) );
    nmenu.addentry( D_STATS, true, 't', "%s", _( "Edit stats" ) );
    nmenu.addentry( D_SPELLS, true, 'l', "%s", _( "Edit spells" ) );
    nmenu.addentry( D_ITEMS, true, 'i', "%s", _( "Grant items" ) );
    nmenu.addentry( D_DELETE_ITEMS, true, 'd', "%s", _( "Delete (all) items" ) );
    nmenu.addentry( D_DROP_ITEMS, true, 'D', "%s", _( "Drop items" ) );
    nmenu.addentry( D_ITEM_WORN, true, 'w', "%s", _( "Wear/wield an item from player's inventory" ) );
    nmenu.addentry( D_HP, true, 'h', "%s", _( "Set hit points" ) );
    nmenu.addentry( D_STAMINA, true, 'S', "%s", _( "Set stamina" ) );
    nmenu.addentry( D_MORALE, true, 'o', "%s", _( "Set morale" ) );
    nmenu.addentry( D_PAIN, true, 'p', "%s", _( "Cause pain" ) );
    nmenu.addentry( D_HEALTHY, true, 'a', "%s", _( "Set health" ) );
    nmenu.addentry( D_NEEDS, true, 'n', "%s", _( "Set needs" ) );
    if( get_option<bool>( "STATS_THROUGH_KILLS" ) ) {
        nmenu.addentry( D_KILL_XP, true, 'X', "%s", _( "Set kill XP" ) );
    }
    nmenu.addentry( D_MUTATE, true, 'u', "%s", _( "Mutate" ) );
    nmenu.addentry( D_STATUS, true,
                    hotkey_for_action( ACTION_PL_INFO, /*maximum_modifier_count=*/1 ),
                    "%s", _( "Status window" ) );
    nmenu.addentry( D_TELE, true, 'e', "%s", _( "Teleport" ) );
    nmenu.addentry( D_ADD_EFFECT, true, 'E', "%s", _( "Add an effect" ) );
    nmenu.addentry( D_CHECK_TEMP, true, 'U', "%s", _( "Print temperature" ) );
    nmenu.addentry( D_ASTHMA, true, 'k', "%s", _( "Cause asthma attack" ) );
    nmenu.addentry( D_MISSION_EDIT, true, 'M', "%s", _( "Edit missions (WARNING: Unstable!)" ) );
    nmenu.addentry( D_PRINT_VARS, true, 'V', "%s", _( "Print vars to file" ) );
    nmenu.addentry( D_WRITE_EOCS, true, 'W', "%s",
                    _( "Write effect_on_condition(s) to eocs.output" ) );
    nmenu.addentry( D_EDIT_VARS, true, 'v', "%s", _( "Edit vars" ) );

    if( you.is_npc() ) {
        nmenu.addentry( D_MISSION_ADD, true, 'm', "%s", _( "Add mission" ) );
        nmenu.addentry( D_CLASS, true, 'c', "%s", _( "Randomize with class" ) );
        nmenu.addentry( D_ATTITUDE, true, 'A', "%s", _( "Set attitude" ) );
        nmenu.addentry( D_OPINION, true, 'O', "%s", _( "Set opinion" ) );
    }
    nmenu.query();
    switch( nmenu.ret ) {
        case D_SKILLS:
            wishskill( &you );
            break;
        case D_THEORY:
            wishskill( &you, true );
            break;
        case D_STATS:
            character_edit_stats_menu( you );
            break;
        case D_SPELLS:
            change_spells( you );
            break;
        case D_PROF:
            wishproficiency( &you );
            break;
        case D_ITEMS:
            wishitem( &you );
            break;
        case D_DELETE_ITEMS:
            if( !query_yn( _( "Delete all items from the target?" ) ) ) {
                break;
            }
            you.worn.on_takeoff( you );
            you.worn.clear();
            you.inv->clear();
            you.remove_weapon();
            break;
        case D_DROP_ITEMS:
            you.drop( game_menus::inv::multidrop( you ), you.pos() );
            break;
        case D_ITEM_WORN: {
            item_location loc = game_menus::inv::titled_menu( player_character, _( "Make target equip" ) );
            if( !loc ) {
                break;
            }
            item &to_wear = *loc;
            if( to_wear.is_armor() ) {
                you.on_item_wear( to_wear );
                you.worn.wear_item( you, to_wear, false, false );
            } else if( !to_wear.is_null() ) {
                you.set_wielded_item( to_wear );
                get_event_bus().send<event_type::character_wields_item>( you.getID(),
                        you.get_wielded_item()->typeId() );
            }
        }
        break;
        case D_HP:
            character_edit_hp_menu( you );
            break;
        case D_STAMINA:
            int value;
            if( query_int( value, _( "Set stamina to?  Current: %d. Max: %d." ), you.get_stamina(),
                           you.get_stamina_max() ) ) {
                if( value >= 0 && value <= you.get_stamina_max() ) {
                    you.set_stamina( value );
                } else {
                    add_msg( m_bad, _( "Target stamina value out of bounds!" ) );
                }
            }
            break;
        case D_MORALE: {
            int value;
            if( query_int( value, _( "Set the morale to?  Currently: %d" ), you.get_morale_level() ) ) {
                you.rem_morale( MORALE_PERM_DEBUG );
                int morale_level_delta = value - you.get_morale_level();
                you.add_morale( MORALE_PERM_DEBUG, morale_level_delta );
                you.apply_persistent_morale();
            }
        }
        break;
        case D_KILL_XP: {
            int value;
            if( query_int( value, _( "Set kill XP to?  Currently: %d" ), you.kill_xp ) ) {
                you.kill_xp = value;
            }
        }
        break;
        case D_OPINION:
            character_edit_opinion_menu( np );
            break;
        case D_DESC:
            character_edit_desc_menu( you );
            break;
        case D_PAIN: {
            int value;
            if( query_int( value, _( "Cause how much pain?  pain: %d" ), you.get_pain() ) ) {
                you.mod_pain( value );
            }
        }
        break;
        case D_NEEDS:
            character_edit_needs_menu( you );
            break;
        case D_MUTATE: {
            uilist smenu;
            smenu.addentry( 0, true, 'm', _( "Mutate" ) );
            smenu.addentry( 1, true, 'c', _( "Mutate category" ) );
            smenu.addentry( 2, true, 'r', _( "Reset mutations" ) );
            smenu.query();
            switch( smenu.ret ) {
                case 0:
                    wishmutate( &you );
                    break;
                case 1: {
                    uilist ssmenu;
                    std::vector<std::pair<std::string, mutation_category_id>> mutation_categories_list;
                    mutation_categories_list.reserve( mutations_category.size() );
                    for( const std::pair<const mutation_category_id, std::vector<trait_id> > &mut_cat :
                         mutations_category ) {
                        mutation_categories_list.emplace_back( mut_cat.first.c_str(), mut_cat.first );
                    }
                    ssmenu.text = _( "Choose mutation category:" );
                    std::sort( mutation_categories_list.begin(), mutation_categories_list.end(), localized_compare );
                    int menu_ind = 0;
                    for( const std::pair<std::string, mutation_category_id> &mut_cat : mutation_categories_list ) {
                        ssmenu.addentry( menu_ind, true, MENU_AUTOASSIGN, mut_cat.first );
                        ++menu_ind;
                    }
                    ssmenu.query();
                    if( ssmenu.ret >= 0 && ssmenu.ret < static_cast<int>( mutation_categories_list.size() ) ) {
                        you.give_all_mutations( mutation_category_trait::get_category(
                                                    mutation_categories_list[ssmenu.ret].second ), query_yn( _( "Include post-threshold traits?" ) ) );
                    }
                    break;
                }
                case 2:
                    if( query_yn( _( "Remove all mutations?" ) ) ) {
                        you.unset_all_mutations();
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case D_HEALTHY: {
            uilist smenu;
            smenu.addentry( 0, true, 'h', "%s: %d", _( "Health" ), you.get_lifestyle() );
            smenu.addentry( 1, true, 'm', "%s: %d", _( "Health modifier" ), you.get_daily_health() );
            smenu.addentry( 2, true, 'r', "%s: %d", _( "Radiation" ), you.get_rad() );
            smenu.query();
            int value;
            switch( smenu.ret ) {
                case 0:
                    if( query_int( value, _( "Set the value to?  Currently: %d" ), you.get_lifestyle() ) ) {
                        you.set_lifestyle( value );
                    }
                    break;
                case 1:
                    if( query_int( value, _( "Set the value to?  Currently: %d" ), you.get_daily_health() ) ) {
                        you.set_daily_health( value );
                    }
                    break;
                case 2:
                    if( query_int( value, _( "Set the value to?  Currently: %d" ), you.get_rad() ) ) {
                        you.set_rad( value );
                    }
                    break;
                default:
                    break;
            }
        }
        break;
        case D_STATUS:
            you.disp_info( true );
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
            mission_debug::edit( you );
            break;
        case D_TELE: {
            if( const std::optional<tripoint> newpos = g->look_around() ) {
                you.setpos( *newpos );
                if( you.is_avatar() ) {
                    if( you.is_mounted() ) {
                        you.mounted_creature->setpos( *newpos );
                    }
                    g->update_map( player_character );
                }
            }
        }
        break;
        case D_CLASS: {
            uilist classes;
            classes.text = _( "Choose new class" );
            std::vector<npc_class_id> ids;
            size_t i = 0;
            for( const npc_class &cl : npc_class::get_all() ) {
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
        break;
        case D_ADD_EFFECT: {
            wisheffect( you );
            break;
        }
        case D_CHECK_TEMP: {
            for( const bodypart_id &bp : you.get_all_body_parts() ) {
                add_msg( string_format( "%s: temperature: %d, temperature conv: %d, wetness: %d", bp->name,
                                        you.get_part_temp_cur( bp ), you.get_part_temp_conv( bp ), you.get_part_wetness( bp ) ) );
            }
            break;
        }
        case D_ASTHMA: {
            you.set_mutation( trait_ASTHMA );
            you.add_effect( effect_asthma, 10_minutes );
            break;
        }
        case D_PRINT_VARS: {
            write_to_file( "var_list.output", [&you]( std::ostream & testfile ) {
                testfile << "Character Name: " + you.get_name() << std::endl;
                testfile << "|;key;value;" << std::endl;

                for( const auto &value : you.get_values() ) {
                    testfile << "|;" << value.first << ";" << value.second << ";" << std::endl;
                }

            }, "var_list" );

            popup( _( "Var list written to var_list.output" ) );
            break;
        }
        case D_WRITE_EOCS: {
            effect_on_conditions::write_eocs_to_file( you );
            popup( _( "effect_on_condition list written to eocs.output" ) );
            break;
        }
        case D_EDIT_VARS: {
            std::string key;
            std::string value;
            string_input_popup popup_key;
            string_input_popup popup_val;
            popup_key
            .title( _( "Key" ) )
            .width( 85 )
            .edit( key );
            popup_val
            .title( _( "Value" ) )
            .width( 85 )
            .edit( value );
            you.set_value( "npctalk_var_" + key, value );
            break;
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
    data << _( " Target:" ) << m.target.to_string();
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

void mission_debug::edit( Character &who )
{
    if( who.is_avatar() ) {
        edit_player();
    } else if( who.is_npc() ) {
        edit_npc( dynamic_cast<npc &>( who ) );
    }
}

void mission_debug::edit_npc( npc &who )
{
    dialogue_chatbin &bin = who.chatbin;
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

    avatar &player_character = get_avatar();
    add_header( mmenu, _( "Active missions:" ) );
    for( mission *m : player_character.active_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Completed missions:" ) );
    for( mission *m : player_character.completed_missions ) {
        mmenu.addentry( all_missions.size(), true, MENU_AUTOASSIGN, "%s", m->type->id.c_str() );
        all_missions.emplace_back( m );
    }

    add_header( mmenu, _( "Failed missions:" ) );
    for( mission *m : player_character.failed_missions ) {
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
    avatar &player_character = get_avatar();
    if( remove_from_vec( player_character.active_missions, &m ) ) {
        add_msg( _( "Removing from active_missions" ) );
    }
    if( remove_from_vec( player_character.completed_missions, &m ) ) {
        add_msg( _( "Removing from completed_missions" ) );
    }
    if( remove_from_vec( player_character.failed_missions, &m ) ) {
        add_msg( _( "Removing from failed_missions" ) );
    }

    if( player_character.active_mission == &m ) {
        player_character.active_mission = nullptr;
        add_msg( _( "Unsetting active mission" ) );
    }

    npc *giver = g->find_npc( m.npc_id );
    if( giver != nullptr ) {
        if( remove_from_vec( giver->chatbin.missions_assigned, &m ) ) {
            add_msg( _( "Removing from %s missions_assigned" ), giver->get_name() );
        }
        if( remove_from_vec( giver->chatbin.missions, &m ) ) {
            add_msg( _( "Removing from %s missions" ), giver->get_name() );
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

static void draw_benchmark( const int max_difference )
{
    // call the draw procedure as many times as possible in max_difference milliseconds
    std::chrono::steady_clock::time_point start_tick = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point end_tick = std::chrono::steady_clock::now();
    int64_t difference = 0;
    int draw_counter = 0;

    static_popup popup;
    popup.on_top( true ).message( "%s", _( "Benchmark in progress…" ) );

    while( true ) {
        end_tick = std::chrono::steady_clock::now();
        difference = std::chrono::duration_cast<std::chrono::milliseconds>( end_tick - start_tick ).count();
        if( difference >= max_difference ) {
            break;
        }
        g->invalidate_main_ui_adaptor();
        inp_mngr.pump_events();
        ui_manager::redraw_invalidated();
        refresh_display();
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

    add_msg( m_info, _( "Drew %d times in %.3f seconds.  (%.3f fps average)" ), draw_counter,
             difference / 1000.0, 1000.0 * draw_counter / static_cast<double>( difference ) );
}

static void debug_menu_game_state()
{
    avatar &player_character = get_avatar();
    map &here = get_map();
    tripoint_abs_sm abs_sub = here.get_abs_sub();
    std::string mfus;
    std::vector<std::pair<m_flag, int>> sorted;
    sorted.reserve( m_flag::MF_MAX );
    for( int f = 0; f < m_flag::MF_MAX; f++ ) {
        sorted.emplace_back( static_cast<m_flag>( f ),
                             MonsterGenerator::generator().m_flag_usage_stats[f] );
    }
    std::sort( sorted.begin(), sorted.end(), []( std::pair<m_flag, int> a, std::pair<m_flag, int> b ) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    } );
    popup( player_character.total_daily_calories_string() );
    for( auto &m_flag_stat : sorted ) {
        mfus += string_format( "%s;%d\n", io::enum_to_string<m_flag>( m_flag_stat.first ),
                               m_flag_stat.second );
    }
    DebugLog( D_INFO, DC_ALL ) << "Monster flag usage statistics:\nFLAG;COUNT\n" << mfus;
    std::fill( MonsterGenerator::generator().m_flag_usage_stats.begin(),
               MonsterGenerator::generator().m_flag_usage_stats.end(), 0 );
    popup_top( "Monster flag usage statistics were dumped to debug.log and cleared." );

    std::string s = _( "Location %d:%d in %d:%d, %s\n" );
    s += _( "Current turn: %d.\n" );
    s += n_gettext( "%d creature exists.\n", "%d creatures exist.\n", g->num_creatures() );

    std::unordered_map<std::string, int> creature_counts;
    for( Creature &critter : g->all_creatures() ) {
        std::string this_name = critter.get_name();
        creature_counts[this_name]++;
    }

    if( !creature_counts.empty() ) {
        std::vector<std::pair<std::string, int>> creature_names_sorted;
        creature_names_sorted.reserve( creature_counts.size() );
        for( const std::pair<const std::string, int> &it : creature_counts ) {
            creature_names_sorted.emplace_back( it );
        }

        std::stable_sort( creature_names_sorted.begin(), creature_names_sorted.end(), []( auto a, auto b ) {
            return a.second > b.second;
        } );

        s += _( "\nSpecific creature type list:\n" );
        for( const std::pair<std::string, int> &crit_name : creature_names_sorted ) {
            s += string_format( "%i %s\n", crit_name.second, crit_name.first );
        }
    }

    popup_top(
        s.c_str(),
        player_character.posx(), player_character.posy(), abs_sub.x(), abs_sub.y(),
        overmap_buffer.ter( player_character.global_omt_location() )->get_name(),
        to_turns<int>( calendar::turn - calendar::turn_zero ),
        g->num_creatures() );
    for( const npc &guy : g->all_npcs() ) {
        tripoint_abs_sm t = guy.global_sm_location();
        add_msg( m_info, _( "%s: map ( %d:%d ) pos ( %d:%d )" ), guy.get_name(), t.x(),
                 t.y(), guy.posx(), guy.posy() );
    }

    add_msg( m_info, _( "(you: %d:%d)" ), player_character.posx(), player_character.posy() );
    std::string stom =
        _( "Stomach Contents: %d ml / %d ml kCal: %d, Water: %d ml" );
    add_msg( m_info, stom.c_str(), units::to_milliliter( player_character.stomach.contains() ),
             units::to_milliliter( player_character.stomach.capacity( player_character ) ),
             player_character.stomach.get_calories(),
             units::to_milliliter( player_character.stomach.get_water() ), player_character.get_hunger() );
    stom = _( "Guts Contents: %d ml / %d ml kCal: %d, Water: %d ml\nHunger: %d, Thirst: %d, kCal: %d / %d" );
    add_msg( m_info, stom.c_str(), units::to_milliliter( player_character.guts.contains() ),
             units::to_milliliter( player_character.guts.capacity( player_character ) ),
             player_character.guts.get_calories(), units::to_milliliter( player_character.guts.get_water() ),
             player_character.get_hunger(), player_character.get_thirst(), player_character.get_stored_kcal(),
             player_character.get_healthy_kcal() );
    add_msg( m_info, _( "Body Mass Index: %.0f\nBasal Metabolic Rate: %i" ), player_character.get_bmi(),
             player_character.get_bmr() );
    add_msg( m_info, _( "Player activity level: %s" ), player_character.activity_level_str() );
    g->invalidate_main_ui_adaptor();
    g->disp_NPCs();
}

static void debug_menu_spawn_vehicle()
{
    avatar &player_character = get_avatar();
    map &here = get_map();
    if( here.veh_at( player_character.pos() ) ) {
        dbg( D_ERROR ) << "game:load: There's already vehicle here";
        debugmsg( "There's already vehicle here" );
    } else {
        // Vector of name, id so that we can sort by name
        std::vector<std::pair<std::string, vproto_id>> veh_strings;
        for( auto &elem : vehicle_prototype::get_all() ) {
            if( elem == vehicle_prototype_custom ) {
                continue;
            }
            veh_strings.emplace_back( elem->name.translated(), elem );
        }
        std::sort( veh_strings.begin(), veh_strings.end(), localized_compare );
        uilist veh_menu;
        veh_menu.text = _( "Choose vehicle to spawn" );
        int menu_ind = 0;
        for( auto &elem : veh_strings ) {
            //~ Menu entry in vehicle wish menu: 1st string: displayed name, 2nd string: internal name of vehicle
            veh_menu.addentry( menu_ind, true, MENU_AUTOASSIGN, _( "%1$s (%2$s)" ),
                               elem.first, elem.second.c_str() );
            ++menu_ind;
        }
        veh_menu.query();
        if( veh_menu.ret >= 0 && veh_menu.ret < static_cast<int>( veh_strings.size() ) ) {
            // Didn't cancel
            const vproto_id &selected_opt = veh_strings[veh_menu.ret].second;
            tripoint dest = player_character.pos();
            uilist veh_cond_menu;
            veh_cond_menu.text = _( "Vehicle condition" );
            veh_cond_menu.addentry( 0, true, MENU_AUTOASSIGN, _( "Light damage" ) );
            veh_cond_menu.addentry( 1, true, MENU_AUTOASSIGN, _( "Undamaged" ) );
            veh_cond_menu.addentry( 2, true, MENU_AUTOASSIGN, _( "Disabled (tires or engine)" ) );
            veh_cond_menu.query();

            if( veh_cond_menu.ret >= 0 && veh_cond_menu.ret < 3 ) {
                // TODO: Allow picking this when add_vehicle has 3d argument
                vehicle *veh = here.add_vehicle(
                                   selected_opt, dest, -90_degrees, 100, veh_cond_menu.ret - 1 );
                if( veh != nullptr ) {
                    here.board_vehicle( dest, &player_character );
                }
            }
        }
    }
}

static void debug_menu_change_time()
{
    auto set_turn = [&]( const int initial, const time_duration & factor, const char *const msg ) {
        string_input_popup pop;
        const int new_value = pop
                              .title( msg )
                              .width( 20 )
                              .text( std::to_string( initial ) )
                              .only_digits( true )
                              .query_int();
        if( pop.canceled() ) {
            return;
        }
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
                          _( "Set season to?  (0 = spring)" ) );
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
                          string_format( _( "Set turn to?  (One day is %i turns)" ), to_turns<int>( 1_days ) ).c_str() );
                break;
            default:
                break;
        }
    } while( smenu.ret != UILIST_CANCEL );
}

static void debug_menu_force_temperature()
{
    uilist tempmenu;
    std::optional<units::temperature> &forced_temp = get_weather().forced_temperature;

    tempmenu.addentry( 0, forced_temp.has_value(), MENU_AUTOASSIGN, _( "Reset" ) );
    tempmenu.addentry( 1, true, MENU_AUTOASSIGN, _( "Set" ) );
    tempmenu.query();
    if( tempmenu.ret == 0 ) {
        forced_temp.reset();
    } else {
        string_input_popup pop;

        auto ask = [&pop]( const std::string & unit, std::optional<float> current ) {
            int ret = pop.title( string_format( _( "Set temperature to?  [%s]" ), unit ) )
                      .width( 20 )
                      .text( current ? std::to_string( *current ) : "" )
                      .only_digits( true )
                      .query_int();

            return pop.canceled() ? current : std::optional<float>( static_cast<float>( ret ) );
        };

        std::optional<float> current;
        std::string option = get_option<std::string>( "USE_CELSIUS" );

        if( option == "celsius" ) {
            if( forced_temp ) {
                current = units::to_celsius( *forced_temp );
            }
            std::optional<float> ret = ask( "C", current );
            if( ret ) {
                forced_temp = units::from_celsius( *ret );
            }
        } else if( option == "kelvin" ) {
            if( forced_temp ) {
                current = units::to_kelvin( *forced_temp );
            }
            std::optional<float> ret = ask( "K", current );
            if( ret ) {
                forced_temp = units::from_kelvin( *ret );
            }
        } else {
            if( forced_temp ) {
                current = units::to_fahrenheit( *forced_temp );
            }
            std::optional<float> ret = ask( "F", current );
            if( ret ) {
                forced_temp = units::from_fahrenheit( *ret );
            }
        }
    }
}

void debug()
{
    bool debug_menu_has_hotkey = hotkey_for_action( ACTION_DEBUG,
                                 /*maximum_modifier_count=*/ -1, false ).has_value();
    std::optional<debug_menu_index> action = debug_menu_uilist( debug_menu_has_hotkey );

    // For the "cheaty" options, disable achievements when used
    achievements_tracker &achievements = get_achievements();
    static const std::unordered_set<debug_menu_index> non_cheaty_options = {
        debug_menu_index::SAVE_SCREENSHOT,
        debug_menu_index::GAME_REPORT,
        debug_menu_index::GAME_MIN_ARCHIVE,
        debug_menu_index::ENABLE_ACHIEVEMENTS,
        debug_menu_index::UNLOCK_ALL,
        debug_menu_index::BENCHMARK,
        debug_menu_index::SHOW_MSG,
    };
    const bool should_disable_achievements = action && !is_debug_character() &&
            !non_cheaty_options.count( *action );
    if( should_disable_achievements && achievements.is_enabled() ) {
        static const std::string query(
            translate_marker(
                "Using this will disable achievements.  Proceed?"
                "\nThey can be reenabled in the 'game' section of the menu." ) );
        if( query_yn( _( query ) ) ) {
            achievements.set_enabled( false );
        } else {
            action = std::nullopt;
        }
    }

    if( !action ) {
        return;
    }

    get_event_bus().send<event_type::uses_debug_menu>( *action );

    avatar &player_character = get_avatar();
    map &here = get_map();
    switch( *action ) {
        case debug_menu_index::WISH:
            debug_menu::wishitem( &player_character );
            break;

        case debug_menu_index::SHORT_TELEPORT:
            debug_menu::teleport_short();
            break;

        case debug_menu_index::LONG_TELEPORT:
            debug_menu::teleport_long();
            break;

        case debug_menu_index::REVEAL_MAP: {
            overmap &cur_om = g->get_cur_om();
            for( int i = 0; i < OMAPX; i++ ) {
                for( int j = 0; j < OMAPY; j++ ) {
                    for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                        cur_om.set_seen( { i, j, k }, true );
                    }
                }
            }
            add_msg( m_good, _( "Current overmap revealed." ) );
        }
        break;

        case debug_menu_index::SPAWN_NPC: {
            shared_ptr_fast<npc> temp = make_shared_fast<npc>();
            temp->normalize();
            temp->randomize();
            temp->spawn_at_precise( player_character.get_location() + point( -4, -4 ) );
            overmap_buffer.insert_npc( temp );
            temp->form_opinion( player_character );
            temp->mission = NPC_MISSION_NULL;
            temp->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, temp->global_omt_location(),
                                   temp->getID() ) );
            std::string new_fac_id = "solo_";
            new_fac_id += temp->name;
            // create a new "lone wolf" faction for this one NPC
            faction *new_solo_fac = g->faction_manager_ptr->add_new_faction( temp->name,
                                    faction_id( new_fac_id ), faction_no_faction );
            temp->set_fac( new_solo_fac ? new_solo_fac->id : faction_no_faction );
            g->load_npcs();
        }
        break;

        case debug_menu_index::SPAWN_OM_NPC: {
            int num_of_npcs = 1;
            if( query_int( num_of_npcs, _( "How many npcs to try spawning?" ), num_of_npcs ) ) {
                for( int i = 0; i < num_of_npcs; i++ ) {
                    g->perhaps_add_random_npc( true );
                }
            }
        }
        break;

        case debug_menu_index::SPAWN_MON:
            debug_menu::wishmonster( std::nullopt );
            break;

        case debug_menu_index::GAME_STATE:
            debug_menu_game_state();
            break;

        case debug_menu_index::KILL_AREA: {
            static_popup popup;
            popup.on_top( true );
            popup.message( "%s", _( "Select first point." ) );

            tripoint initial_pos = player_character.pos();
            const look_around_result first = g->look_around( false, initial_pos, initial_pos,
                                             false, true, false );

            if( !first.position ) {
                break;
            }

            popup.message( "%s", _( "Select second point." ) );
            const look_around_result second = g->look_around( false, initial_pos, *first.position,
                                              true, true, false );

            if( !second.position ) {
                break;
            }

            const tripoint_range<tripoint> points = get_map().points_in_rectangle(
                    first.position.value(), second.position.value() );

            std::vector<Creature *> creatures = g->get_creatures_if(
            [&points]( const Creature & critter ) -> bool {
                return !critter.is_avatar() && points.is_point_inside( critter.pos() );
            } );

            for( Creature *critter : creatures ) {
                critter->die( nullptr );
            }

            g->cleanup_dead();
        }
        break;

        case debug_menu_index::KILL_NPCS:
            for( npc &guy : g->all_npcs() ) {
                add_msg( _( "%s's head implodes!" ), guy.get_name() );
                guy.set_part_hp_cur( bodypart_id( "head" ), 0 );
            }
            break;

        case debug_menu_index::MUTATE:
            debug_menu::wishmutate( &player_character );
            break;

        case debug_menu_index::SPAWN_VEHICLE:
            debug_menu_spawn_vehicle();
            break;

        case debug_menu_index::CHANGE_SKILLS: {
            debug_menu::wishskill( &player_character );
        }
        break;

        case debug_menu_index::CHANGE_THEORY: {
            debug_menu::wishskill( &player_character, true );
        }
        break;

        case debug_menu_index::LEARN_MA:
            add_msg( m_info, _( "Martial arts debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( auto &style : all_martialart_types() ) {
                if( style != style_none ) {
                    player_character.martial_arts_data->add_martialart( style );
                }
            }
            add_msg( m_good, _( "You now know a lot more than just 10 styles of kung fu." ) );
            break;

        case debug_menu_index::UNLOCK_RECIPES: {
            add_msg( m_info, _( "Recipe debug." ) );
            add_msg( _( "Your eyes blink rapidly as knowledge floods your brain." ) );
            for( const auto &e : recipe_dict ) {
                player_character.learn_recipe( &e.second );
            }
            add_msg( m_good, _( "You know how to craft that now." ) );
        }
        break;

        case debug_menu_index::EDIT_PLAYER:
            character_edit_menu();
            break;

        case debug_menu_index::CONTROL_NPC:
            control_npc_menu();
            break;

        case debug_menu_index::SPAWN_ARTIFACT:
            spawn_artifact();
            break;

        case debug_menu_index::SPAWN_CLAIRVOYANCE:
            player_character.i_add( item( "architect_cube", calendar::turn ) );
            break;

        case debug_menu_index::MAP_EDITOR:
            g->look_debug();
            break;

        case debug_menu_index::CHANGE_WEATHER: {
            uilist weather_menu;
            weather_manager &weather = get_weather();
            weather_generator wgen = weather.get_cur_weather_gen();
            weather_menu.text = _( "Select new weather pattern:" );
            weather_menu.addentry( 0, true, MENU_AUTOASSIGN, weather.weather_override == WEATHER_NULL ?
                                   _( "Keep normal weather patterns" ) : _( "Disable weather forcing" ) );
            for( size_t i = 0; i < wgen.sorted_weather.size(); i++ ) {
                weather_menu.addentry( i, true, MENU_AUTOASSIGN,
                                       wgen.sorted_weather[i]->name.translated() );
            }

            weather_menu.query();

            if( weather_menu.ret >= 0 &&
                static_cast<size_t>( weather_menu.ret ) < wgen.sorted_weather.size() ) {
                const weather_type_id selected_weather = wgen.sorted_weather[weather_menu.ret]->id;
                if( weather.weather_id->debug_leave_eoc.has_value() ) {
                    dialogue d( get_talker_for( get_avatar() ), nullptr );
                    effect_on_condition_id( weather.weather_id->debug_leave_eoc.value() )->activate( d );
                }
                if( selected_weather->debug_cause_eoc.has_value() ) {
                    dialogue d( get_talker_for( get_avatar() ), nullptr );
                    effect_on_condition_id( selected_weather->debug_cause_eoc.value() )->activate( d );
                }
                weather.weather_override = selected_weather;
                weather.set_nextweather( calendar::turn );
            }
        }
        break;

        case debug_menu_index::WIND_DIRECTION: {
            uilist wind_direction_menu;
            weather_manager &weather = get_weather();
            wind_direction_menu.text = _( "Select new wind direction:" );
            wind_direction_menu.addentry( 0, true, MENU_AUTOASSIGN, weather.wind_direction_override ?
                                          _( "Disable direction forcing" ) : _( "Keep normal wind direction" ) );
            int count = 1;
            for( int angle = 0; angle <= 315; angle += 45 ) {
                wind_direction_menu.addentry( count, true, MENU_AUTOASSIGN, get_wind_arrow( angle ) );
                count += 1;
            }
            wind_direction_menu.query();
            if( wind_direction_menu.ret == 0 ) {
                weather.wind_direction_override = std::nullopt;
                weather.set_nextweather( calendar::turn );
            } else if( wind_direction_menu.ret >= 0 && wind_direction_menu.ret < 9 ) {
                weather.wind_direction_override = ( wind_direction_menu.ret - 1 ) * 45;
                weather.set_nextweather( calendar::turn );
            }
        }
        break;

        case debug_menu_index::WIND_SPEED: {
            uilist wind_speed_menu;
            weather_manager &weather = get_weather();
            wind_speed_menu.text = _( "Select new wind speed:" );
            wind_speed_menu.addentry( 0, true, MENU_AUTOASSIGN, weather.wind_direction_override ?
                                      _( "Disable speed forcing" ) : _( "Keep normal wind speed" ) );
            int count = 1;
            for( int speed = 0; speed <= 100; speed += 10 ) {
                std::string speedstring = std::to_string( speed ) + " " + velocity_units( VU_WIND );
                wind_speed_menu.addentry( count, true, MENU_AUTOASSIGN, speedstring );
                count += 1;
            }
            wind_speed_menu.query();
            if( wind_speed_menu.ret == 0 ) {
                weather.windspeed_override = std::nullopt;
                weather.set_nextweather( calendar::turn );
            } else if( wind_speed_menu.ret >= 0 && wind_speed_menu.ret < 12 ) {
                int selected_wind_speed = ( wind_speed_menu.ret - 1 ) * 10;
                weather.windspeed_override = selected_wind_speed;
                weather.set_nextweather( calendar::turn );
            }
        }
        break;

        case debug_menu_index::GEN_SOUND: {
            const std::optional<tripoint> where = g->look_around();
            if( !where ) {
                return;
            }

            int volume;
            if( !query_int( volume, _( "Volume of sound: " ) ) ) {
                return;
            }

            if( volume < 0 ) {
                return;
            }

            sounds::sound( *where, volume, sounds::sound_t::order, string_format( _( "DEBUG SOUND ( %d )" ),
                           volume ) );
        }
        break;

        case debug_menu_index::KILL_MONS: {
            for( monster &critter : g->all_monsters() ) {
                // Use the normal death functions, useful for testing death
                // and for getting a corpse.
                if( critter.type->id != mon_generator ) {
                    critter.die( nullptr );
                }
            }
            g->cleanup_dead();
        }
        break;
        case debug_menu_index::DISPLAY_HORDES:
            ui::omap::display_hordes();
            break;
        case debug_menu_index::TEST_IT_GROUP: {
            item_group::debug_spawn();
        }
        break;

        // Damage Self
        case debug_menu_index::DAMAGE_SELF: {
            const std::vector<bodypart_id> parts = player_character.get_all_body_parts(
                    get_body_part_flags::only_main );
            uilist smenu;
            int i = 0;
            for( const bodypart_id &part : parts ) {
                smenu.addentry( i, true, ' ', "%s: %d",
                                part->name.translated(), player_character.get_part_hp_cur( part ) );
                i++;
            }
            smenu.query();
            bodypart_id part;
            int dbg_damage;
            if( smenu.ret >= 0 && static_cast<std::size_t>( smenu.ret ) <= parts.size() ) {
                part = parts[smenu.ret];
            }
            if( query_int( dbg_damage, _( "Damage self for how much?  hp: %s" ), part.id().c_str() ) ) {
                player_character.apply_damage( nullptr, part, dbg_damage );
                if( player_character.is_dead_state() ) {
                    player_character.die( nullptr );
                }
            }
        }
        break;

        // Add bleeding
        case debug_menu_index::BLEED_SELF: {
            uilist smenu;
            smenu.addentry( 0, true, 'q', _( "Torso" ) );
            smenu.addentry( 1, true, 'w', _( "Head" ) );
            smenu.addentry( 2, true, 'a', _( "Left arm" ) );
            smenu.addentry( 3, true, 's', _( "Right arm" ) );
            smenu.addentry( 4, true, 'z', _( "Left leg" ) );
            smenu.addentry( 5, true, 'x', _( "Right leg" ) );
            smenu.query();
            bodypart_id part;
            int intensity = 0;
            switch( smenu.ret ) {
                case 0:
                    part = bodypart_id( "torso" );
                    break;
                case 1:
                    part = bodypart_id( "head" );
                    break;
                case 2:
                    part = bodypart_id( "arm_l" );
                    break;
                case 3:
                    part = bodypart_id( "arm_r" );
                    break;
                case 4:
                    part = bodypart_id( "leg_l" );
                    break;
                case 5:
                    part = bodypart_id( "leg_r" );
                    break;
                default:
                    break;
            }
            if( query_int( intensity, _( "Add bleeding duration in minutes, equal to intensity:" ) ) ) {
                player_character.add_effect( effect_bleed,  1_minutes * intensity, part );
            }
        }
        break;

        case debug_menu_index::SHOW_SOUND: {
#if defined(TILES)
            const auto &sounds_to_draw = sounds::get_monster_sounds();

            shared_ptr_fast<game::draw_callback_t> sound_cb = make_shared_fast<game::draw_callback_t>( [&]() {
                const point offset {
                    player_character.view_offset.xy() + point( POSX - player_character.posx(), POSY - player_character.posy() )
                };
                for( const tripoint &sound : sounds_to_draw.first ) {
                    mvwputch( g->w_terrain, offset + sound.xy(), c_yellow, '?' );
                }
                for( const tripoint &sound : sounds_to_draw.second ) {
                    mvwputch( g->w_terrain, offset + sound.xy(), c_red, '?' );
                }
            } );
            g->add_draw_callback( sound_cb );

            ui_manager::redraw();
            inp_mngr.wait_for_any_key();
#else
            popup( _( "This binary was not compiled with tiles support." ) );
#endif
        }
        break;

        case debug_menu_index::DISPLAY_WEATHER:
            ui::omap::display_weather();
            break;
        case debug_menu_index::DISPLAY_SCENTS:
            ui::omap::display_scents();
            break;
        case debug_menu_index::DISPLAY_SCENTS_LOCAL:
            g->display_toggle_overlay( ACTION_DISPLAY_SCENT );
            break;
        case debug_menu_index::DISPLAY_SCENTS_TYPE_LOCAL:
            g->display_toggle_overlay( ACTION_DISPLAY_SCENT_TYPE );
            break;
        case debug_menu_index::DISPLAY_NPC_ATTACK:
            g->display_toggle_overlay( ACTION_DISPLAY_NPC_ATTACK_POTENTIAL );
            break;
        case debug_menu_index::DISPLAY_TEMP:
            g->display_toggle_overlay( ACTION_DISPLAY_TEMPERATURE );
            break;
        case debug_menu_index::DISPLAY_VEHICLE_AI:
            g->display_toggle_overlay( ACTION_DISPLAY_VEHICLE_AI );
            break;
        case debug_menu_index::DISPLAY_VISIBILITY:
            g->display_toggle_overlay( ACTION_DISPLAY_VISIBILITY );
            break;
        case debug_menu_index::DISPLAY_LIGHTING:
            g->display_toggle_overlay( ACTION_DISPLAY_LIGHTING );
            break;
        case debug_menu_index::DISPLAY_RADIATION:
            g->display_toggle_overlay( ACTION_DISPLAY_RADIATION );
            break;
        case debug_menu_index::DISPLAY_TRANSPARENCY:
            g->display_toggle_overlay( ACTION_DISPLAY_TRANSPARENCY );
            break;
        case debug_menu_index::DISPLAY_REACHABILITY_ZONES:
            g->display_reachability_zones();
            break;
        case debug_menu_index::HOUR_TIMER:
            g->toggle_debug_hour_timer();
            break;
        case debug_menu_index::CHANGE_TIME:
            debug_menu_change_time();
            break;
        case debug_menu_index::FORCE_TEMP:
            debug_menu_force_temperature();
            break;
        case debug_menu_index::SET_AUTOMOVE: {
            const std::optional<tripoint> dest = g->look_around();
            if( !dest || *dest == player_character.pos() ) {
                break;
            }

            // TODO: fix point types
            auto rt = here.route( player_character.pos_bub(), tripoint_bub_ms( *dest ),
                                  player_character.get_pathfinding_settings(),
                                  player_character.get_path_avoid() );
            if( !rt.empty() ) {
                player_character.set_destination( rt );
            } else {
                popup( "Couldn't find path" );
            }
        }
        break;
        case debug_menu_index::SHOW_MUT_CAT:
            for( const auto &elem : player_character.mutation_category_level ) {
                add_msg( "%s: %d", elem.first.c_str(), elem.second );
            }
            break;

        case debug_menu_index::OM_EDITOR:
            ui::omap::display_editor();
            break;

        case debug_menu_index::BENCHMARK: {
            const int ms = string_input_popup()
                           .title( _( "Enter benchmark length (in milliseconds):" ) )
                           .width( 20 )
                           .text( "5000" )
                           .query_int();
            debug_menu::draw_benchmark( ms );
        }
        break;

        case debug_menu_index::OM_TELEPORT:
            debug_menu::teleport_overmap();
            break;
        case debug_menu_index::OM_TELEPORT_COORDINATES:
            debug_menu::teleport_overmap( true );
            break;
        case debug_menu_index::OM_TELEPORT_CITY:
            debug_menu::teleport_city();
            break;
        case debug_menu_index::TRAIT_GROUP:
            trait_group::debug_spawn();
            break;
        case debug_menu_index::ENABLE_ACHIEVEMENTS:
            if( achievements.is_enabled() ) {
                popup( _( "Achievements are already enabled" ) );
            } else {
                achievements.set_enabled( true );
                popup( _( "Achievements enabled" ) );
            }
            break;
        case debug_menu_index::UNLOCK_ALL:
            if( query_yn(
                    _( "Activating this will add the Arcade Mode achievement unlocking all starting scenarios and professions for all worlds.  The character who performs this action will need to die for it to be recorded.  Achievements are tracked from the memorial folder if you need to get rid of this.  Activating this will spoil factions and situations you may otherwise stumble upon naturally while playing.  Some scenarios are frustrating for the uninitiated, and some professions skip portions of the game's content.  If new to the game progression would otherwise help you be introduced to mechanics at a reasonable pace." ) ) ) {
                get_achievements().report_achievement( &achievement_achievement_arcade_mode.obj(),
                                                       achievement_completion::completed );
            }
            break;
        case debug_menu_index::SHOW_MSG:
            debugmsg( "Test debugmsg" );
            break;
        case debug_menu_index::CRASH_GAME:
            raise( SIGSEGV );
            break;
        case debug_menu_index::ACTIVATE_EOC: {
            const std::vector<effect_on_condition> &eocs = effect_on_conditions::get_all();
            uilist eoc_menu;
            for( const effect_on_condition &eoc : eocs ) {
                eoc_menu.addentry( -1, true, -1, eoc.id.str() );
            }
            eoc_menu.query();

            if( eoc_menu.ret >= 0 && eoc_menu.ret < static_cast<int>( eocs.size() ) ) {
                dialogue newDialog( get_talker_for( get_avatar() ), nullptr );
                eocs[eoc_menu.ret].activate( newDialog );
            }
        }
        break;
        case debug_menu_index::MAP_EXTRA: {
            const std::vector<map_extra_id> &mx_str = MapExtras::get_all_function_names();
            uilist mx_menu;
            for( const map_extra_id &extra : mx_str ) {
                mx_menu.addentry( -1, true, -1, extra.str() );
            }
            mx_menu.query();
            int mx_choice = mx_menu.ret;
            if( mx_choice >= 0 && mx_choice < static_cast<int>( mx_str.size() ) ) {
                const tripoint_abs_omt where_omt( ui::omap::choose_point( true ) );
                if( where_omt != overmap::invalid_tripoint ) {
                    tripoint_abs_sm where_sm = project_to<coords::sm>( where_omt );
                    tinymap mx_map;
                    mx_map.load( where_sm, false );
                    MapExtras::apply_function( mx_str[mx_choice], mx_map, where_sm );
                    g->load_npcs();
                    here.invalidate_map_cache( here.get_abs_sub().z() );
                }
            }
            break;
        }
        case debug_menu_index::NESTED_MAPGEN:
            debug_menu::spawn_nested_mapgen();
            break;
        case debug_menu_index::DISPLAY_NPC_PATH:
            g->debug_pathfinding = !g->debug_pathfinding;
            break;
        case debug_menu_index::PRINT_FACTION_INFO: {
            int count = 0;
            for( const auto &elem : g->faction_manager_ptr->all() ) {
                std::cout << std::to_string( count ) << " Faction_id key in factions map = " << elem.first.str() <<
                          std::endl;
                std::cout << std::to_string( count ) << " Faction name associated with this id is " <<
                          elem.second.name << std::endl;
                std::cout << std::to_string( count ) << " the id of that faction object is " << elem.second.id.str()
                          << std::endl;
                count++;
            }
            std::cout << "Player faction is " << player_character.get_faction()->id.str() << std::endl;
            break;
        }
        case debug_menu_index::PRINT_NPC_MAGIC: {
            for( npc &guy : g->all_npcs() ) {
                const std::vector<spell_id> spells = guy.magic->spells();
                if( spells.empty() ) {
                    std::cout << guy.disp_name() << " does not know any spells." << std::endl;
                    continue;
                }
                std::cout << guy.disp_name() << "knows : ";
                int counter = 1;
                for( const spell_id &sp : spells ) {
                    std::cout << sp->name.translated() << " ";
                    if( counter < static_cast<int>( spells.size() ) ) {
                        std::cout << "and ";
                    } else {
                        std::cout << "." << std::endl;
                    }
                    counter++;
                }
            }
            break;
        }
        case debug_menu_index::QUIT_NOSAVE:
            if( query_yn(
                    _( "Quit without saving?  This may cause issues such as duplicated or missing items and vehicles!" ) ) ) {
                player_character.moves = 0;
                g->uquit = QUIT_NOSAVED;
            }
            break;
        case debug_menu_index::QUICKLOAD:
            if( query_yn(
                    _( "Quickload without saving?  This may cause issues such as duplicated or missing items and vehicles!" ) ) ) {
                g->quickload();
            }
            break;
        case debug_menu_index::TEST_WEATHER: {
            get_weather().get_cur_weather_gen().test_weather( g->get_seed() );
        }
        break;
        case debug_menu_index::WRITE_GLOBAL_EOCS: {
            effect_on_conditions::write_global_eocs_to_file();
            popup( _( "effect_on_condition list written to eocs.output" ) );
        }
        break;
        case debug_menu_index::WRITE_GLOBAL_VARS: {
            write_to_file( "var_list.output", [&]( std::ostream & testfile ) {
                testfile << "Global" << std::endl;
                testfile << "|;key;value;" << std::endl;
                global_variables &globvars = get_globals();
                auto globals = globvars.get_global_values();
                for( const auto &value : globals ) {
                    testfile << "|;" << value.first << ";" << value.second << ";" << std::endl;
                }

            }, "var_list" );

            popup( _( "Var list written to var_list.output" ) );
        }
        break;
        case debug_menu_index::WRITE_TIMED_EVENTS: {
            write_to_file( "timed_event_list.output", [&]( std::ostream & testfile ) {
                testfile << "|;when;type;key;string_id;strength;map_point;faction_id;" << std::endl;
                for( const timed_event &te : get_timed_events().get_all() ) {
                    testfile << "|;" << to_string( te.when ) << ";" << static_cast<int>( te.type ) << ";" << te.key <<
                             ";" << te.string_id << ";" << te.strength << ";" << te.map_point << ";" << te.faction_id << ";" <<
                             std::endl;
                }

            }, "timed_event_list" );

            popup( _( "Var list written to timed_event_list.output" ) );
        }
        break;
        case debug_menu_index::EDIT_GLOBAL_VARS: {
            std::string key;
            std::string value;
            string_input_popup popup_key;
            string_input_popup popup_val;
            popup_key
            .title( _( "Key" ) )
            .width( 85 )
            .edit( key );
            popup_val
            .title( _( "Value" ) )
            .width( 85 )
            .edit( value );
            global_variables &globvars = get_globals();
            globvars.set_global_value( "npctalk_var_" + key, value );
        }
        break;

        case debug_menu_index::SAVE_SCREENSHOT:
            g->queue_screenshot = true;
            break;

        case debug_menu_index::GAME_REPORT: {
            // generate a game report, useful for bug reporting.
            std::string report = game_info::game_report();
            // write to log
            DebugLog( DL_ALL, DC_ALL ) << " GAME REPORT:\n" << report;
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
        case debug_menu_index::GAME_MIN_ARCHIVE: {
            g->quicksave();

            static_popup popup;
            popup.message( "%s", _( "Writing archive, this may take a while." ) );
            ui_manager::redraw();
            refresh_display();

            write_min_archive();
            break;
        }
        case debug_menu_index::CHANGE_SPELLS:
            change_spells( player_character );
            break;
        case debug_menu_index::TEST_MAP_EXTRA_DISTRIBUTION:
            MapExtras::debug_spawn_test();
            break;

        case debug_menu_index::GENERATE_EFFECT_LIST:
            write_to_file( "effect_list.output", [&]( std::ostream & testfile ) {
                testfile << "|;id;duration;intensity;perm;bp" << std::endl;

                for( const effect &eff :  get_player_character().get_effects() ) {
                    testfile << "|;" << eff.get_id().str() << ";" << to_string( eff.get_duration() ) << ";" <<
                             eff.get_intensity() << ";" << ( eff.is_permanent() ? "true" : "false" ) << ";" <<
                             eff.get_bp().id().str()
                             << std::endl;
                }

            }, "effect_list" );

            popup( _( "Effect list written to effect_list.output" ) );
            break;

        case debug_menu_index::VEHICLE_BATTERY_CHARGE: {

            optional_vpart_position v_part_pos = here.veh_at( player_character.pos() );
            if( !v_part_pos ) {
                add_msg( m_bad, _( "There's no vehicle there." ) );
                break;
            }

            int amount = 0;
            string_input_popup popup;
            popup
            .title( _( "By how much?  (in kJ, negative to discharge)" ) )
            .width( 30 )
            .edit( amount );
            if( !popup.canceled() ) {
                vehicle &veh = v_part_pos->vehicle();
                if( amount >= 0 ) {
                    veh.charge_battery( amount, false );
                } else {
                    veh.discharge_battery( -amount, false );
                }
            }
            break;
        }

        case debug_menu_index::EDIT_CAMP_LARDER: {
            faction *your_faction = get_player_character().get_faction();
            int larder;
            if( query_int( larder, _( "Set camp larder kCals to?  Currently: %d" ),
                           your_faction->food_supply ) ) {
                your_faction->food_supply = larder;
            }
            break;
        }

        case debug_menu_index::last:
            return;
    }
    here.invalidate_map_cache( here.get_abs_sub().z() );
}

} // namespace debug_menu
