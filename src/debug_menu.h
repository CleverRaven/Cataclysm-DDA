#pragma once
#ifndef CATA_SRC_DEBUG_MENU_H
#define CATA_SRC_DEBUG_MENU_H

#include <cstddef>
#include <functional>
#include <optional>
#include <iosfwd>
#include <string> // IWYU pragma: keep

#include "cata_variant.h"

struct tripoint;
template <typename E> struct enum_traits;

class Character;

struct requirement_data;

namespace debug_menu
{

enum class debug_menu_index : int {
    WISH,
    SHORT_TELEPORT,
    LONG_TELEPORT,
    REVEAL_MAP,
    SPAWN_NPC,
    SPAWN_OM_NPC,
    SPAWN_MON,
    GAME_STATE,
    KILL_AREA,
    KILL_NPCS,
    MUTATE,
    SPAWN_VEHICLE,
    CHANGE_SKILLS,
    CHANGE_THEORY,
    LEARN_MA,
    UNLOCK_RECIPES,
    UNLOCK_ALL,
    EDIT_PLAYER,
    CONTROL_NPC,
    SPAWN_ARTIFACT,
    SPAWN_CLAIRVOYANCE,
    MAP_EDITOR,
    CHANGE_WEATHER,
    WIND_DIRECTION,
    WIND_SPEED,
    GEN_SOUND,
    KILL_MONS,
    DISPLAY_HORDES,
    TEST_IT_GROUP,
    DAMAGE_SELF,
    BLEED_SELF,
    SHOW_SOUND,
    DISPLAY_WEATHER,
    DISPLAY_SCENTS,
    CHANGE_TIME,
    FORCE_TEMP,
    SET_AUTOMOVE,
    SHOW_MUT_CAT,
    OM_EDITOR,
    BENCHMARK,
    OM_TELEPORT,
    OM_TELEPORT_COORDINATES,
    OM_TELEPORT_CITY,
    TRAIT_GROUP,
    ENABLE_ACHIEVEMENTS,
    SHOW_MSG,
    CRASH_GAME,
    MAP_EXTRA,
    DISPLAY_NPC_PATH,
    DISPLAY_NPC_ATTACK,
    PRINT_FACTION_INFO,
    PRINT_NPC_MAGIC,
    QUIT_NOSAVE,
    TEST_WEATHER,
    SAVE_SCREENSHOT,
    GAME_REPORT,
    GAME_MIN_ARCHIVE,
    DISPLAY_SCENTS_LOCAL,
    DISPLAY_SCENTS_TYPE_LOCAL,
    DISPLAY_TEMP,
    DISPLAY_VEHICLE_AI,
    DISPLAY_VISIBILITY,
    DISPLAY_LIGHTING,
    DISPLAY_TRANSPARENCY,
    DISPLAY_RADIATION,
    HOUR_TIMER,
    CHANGE_SPELLS,
    TEST_MAP_EXTRA_DISTRIBUTION,
    NESTED_MAPGEN,
    VEHICLE_BATTERY_CHARGE,
    VEHICLE_DELETE,
    VEHICLE_EXPORT,
    GENERATE_EFFECT_LIST,
    WRITE_GLOBAL_EOCS,
    WRITE_GLOBAL_VARS,
    EDIT_GLOBAL_VARS,
    ACTIVATE_EOC,
    WRITE_TIMED_EVENTS,
    QUICKLOAD,
    IMPORT_FOLLOWER,
    EXPORT_FOLLOWER,
    EXPORT_SELF,
    QUICK_SETUP,
    TOGGLE_SETUP_MUTATION,
    NORMALIZE_BODY_STAT,
    SIX_MILLION_DOLLAR_SURVIVOR,
    EDIT_FACTION,
    last
};

void wisheffect( Character &p );
void wishitem( Character *you = nullptr );
void wishitem( Character *you, const tripoint & );
void wishmonster( const std::optional<tripoint> &p );
void wishmutate( Character *you );
void wishbionics( Character *you );
void wishskill( Character *you, bool change_theory = false );
void wishproficiency( Character *you );

void debug();

/* Splits a string by @param delimiter and push_back's the elements into _Container */
template<typename Container>
Container string_to_iterable( const std::string_view str, const std::string_view delimiter )
{
    Container res;

    size_t pos = 0;
    size_t start = 0;
    while( ( pos = str.find( delimiter, start ) ) != std::string::npos ) {
        if( pos > start ) {
            res.emplace_back( str.substr( start, pos - start ) );
        }
        start = pos + delimiter.length();
    }
    if( start != str.length() ) {
        res.emplace_back( str.substr( start, str.length() - start ) );
    }

    return res;
}

/* Merges iterable elements into std::string with
 * @param delimiter between them
 * @param f is callable that is called to transform each value
 * */
template<typename Container, typename Mapper>
std::string iterable_to_string( const Container &values, const std::string_view delimiter,
                                const Mapper &f )
{
    std::string res;
    for( auto iter = values.begin(); iter != values.end(); ++iter ) {
        if( iter != values.begin() ) {
            res += delimiter;
        }
        res += f( *iter );
    }
    return res;
}

template<typename Container>
std::string iterable_to_string( const Container &values, const std::string_view delimiter )
{
    return iterable_to_string( values, delimiter, []( const std::string_view f ) {
        return f;
    } );
}

/* Allows the player to specify the current random seed.  Useful if a random behaviour is being tested
 * and you want to replicate
 */
void set_random_seed( const std::string &title );

/* Function used for debugging and testing crafting and other tasks with requirements
 * Creates a list of items (first in pair) and quantities/charges (second in pair) that
 * will meet the provided requirements.  Results formatted for passing to debug_spawn_item_collection.
 * Will try not to use pseudo items, but if no other option is available, will warn with a debugmsg,
 * then include pseudo items in the list.
 */
std::vector<std::pair<itype_id, int>> get_items_for_requirements( const requirement_data &req,
                                   int batch_size, const std::string &requirement_name, const bool silent = false );

/* Will spawn, around the player's feet, the specified items with the specified
 * quantity/charges/ammunition.  If multiple items that have ammunition/battery are desired, they should
 * be listed separately (e.g. light_battery_cell, 2 should spawn a battery with 2 charges, not 2 batteries
 * While light_battery_cell, 1000, will spawn light batteries providing 1000 charges).  If not silent,
 * will provide a message log of the items spawned.
 */
void spawn_item_collection( const std::vector<std::pair<itype_id, int>> &items_to_spawn,
                            bool silent );

} // namespace debug_menu

template<>
struct enum_traits<debug_menu::debug_menu_index> {
    static constexpr debug_menu::debug_menu_index last = debug_menu::debug_menu_index::last;
};

namespace std
{
template<>
struct hash<debug_menu::debug_menu_index> {
    std::size_t operator()( const debug_menu::debug_menu_index v ) const noexcept {
        return hash<int>()( static_cast<int>( v ) );
    }
};
} // namespace std

#endif // CATA_SRC_DEBUG_MENU_H
