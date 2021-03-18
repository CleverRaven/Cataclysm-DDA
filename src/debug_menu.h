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
class player;

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
    KILL_NPCS,
    MUTATE,
    SPAWN_VEHICLE,
    CHANGE_SKILLS,
    LEARN_MA,
    UNLOCK_RECIPES,
    EDIT_PLAYER,
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
    SET_AUTOMOVE,
    SHOW_MUT_CAT,
    OM_EDITOR,
    BENCHMARK,
    OM_TELEPORT,
    OM_TELEPORT_COORDINATES,
    TRAIT_GROUP,
    ENABLE_ACHIEVEMENTS,
    SHOW_MSG,
    CRASH_GAME,
    MAP_EXTRA,
    DISPLAY_NPC_PATH,
    PRINT_FACTION_INFO,
    PRINT_NPC_MAGIC,
    QUIT_NOSAVE,
    TEST_WEATHER,
    SAVE_SCREENSHOT,
    GAME_REPORT,
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
    last
};

void change_spells( Character &character );

void teleport_short();
void teleport_long();
void teleport_overmap( bool specific_coordinates = false );

void spawn_nested_mapgen();
void character_edit_menu();
void wishitem( player *p = nullptr );
void wishitem( player *p, const tripoint & );
void wishmonster( const cata::optional<tripoint> &p );
void wishmutate( player *p );
void wishskill( player *p );
void wishproficiency( player *p );
void mutation_wish();
void draw_benchmark( int max_difference );

void debug();

/* Splits a string by @param delimiter and push_back's the elements into _Container */
template<typename _Container>
_Container string_to_iterable( const std::string &str, const std::string &delimiter )
{
    _Container res;

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
template<typename _Container, typename Mapper>
std::string iterable_to_string( const _Container &values, const std::string &delimiter,
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

template<typename _Container>
std::string iterable_to_string( const _Container &values, const std::string &delimiter )
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
