#pragma once
#ifndef CATA_SRC_DEBUG_MENU_H
#define CATA_SRC_DEBUG_MENU_H

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string> // IWYU pragma: keep

struct tripoint;
template <typename E> struct enum_traits;

namespace cata
{
template<typename T>
class optional;
} // namespace cata

class Character;

namespace debug_menu
{

enum class debug_menu_index : int {
    WISH,
    SHORT_TELEPORT,
    LONG_TELEPORT,
    REVEAL_MAP,
    SPAWN_NPC,
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
    DISPLAY_REACHABILITY_ZONES,
    DISPLAY_RADIATION,
    HOUR_TIMER,
    CHANGE_SPELLS,
    TEST_MAP_EXTRA_DISTRIBUTION,
    NESTED_MAPGEN,
    VEHICLE_BATTERY_CHARGE,
    GENERATE_EFFECT_LIST,
    EDIT_CAMP_LARDER,
    WRITE_GLOBAL_EOCS,
    WRITE_GLOBAL_VARS,
    EDIT_GLOBAL_VARS,
    ACTIVATE_EOC,
    WRITE_TIMED_EVENTS,
    QUICKLOAD,
    last
};

void wisheffect( Character &p );
void wishitem( Character *you = nullptr );
void wishitem( Character *you, const tripoint & );
void wishmonster( const cata::optional<tripoint> &p );
void wishmutate( Character *you );
void wishskill( Character *you, bool change_theory = false );
void wishproficiency( Character *you );

void debug();

/* Splits a string by @param delimiter and push_back's the elements into _Container */
template<typename Container>
Container string_to_iterable( const std::string &str, const std::string &delimiter )
{
    Container res;

    size_t pos = 0;
    size_t start = 0;
    while( ( pos = str.find( delimiter, start ) ) != std::string::npos ) {
        if( pos > start ) {
            res.push_back( str.substr( start, pos - start ) );
        }
        start = pos + delimiter.length();
    }
    if( start != str.length() ) {
        res.push_back( str.substr( start, str.length() - start ) );
    }

    return res;
}

/* Merges iterable elements into std::string with
 * @param delimiter between them
 * @param f is callable that is called to transform each value
 * */
template<typename Container, typename Mapper>
std::string iterable_to_string( const Container &values, const std::string &delimiter,
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
std::string iterable_to_string( const Container &values, const std::string &delimiter )
{
    return iterable_to_string( values, delimiter, []( const std::string & f ) {
        return f;
    } );
}

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
