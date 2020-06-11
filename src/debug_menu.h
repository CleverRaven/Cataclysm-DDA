#pragma once
#ifndef CATA_SRC_DEBUG_MENU_H
#define CATA_SRC_DEBUG_MENU_H

#include <functional>

#include "enum_traits.h"

struct tripoint;

namespace cata
{
template<typename T>
class optional;
} // namespace cata

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
    KILL_MONS,
    DISPLAY_HORDES,
    TEST_IT_GROUP,
    DAMAGE_SELF,
    SHOW_SOUND,
    DISPLAY_WEATHER,
    DISPLAY_SCENTS,
    CHANGE_TIME,
    SET_AUTOMOVE,
    SHOW_MUT_CAT,
    OM_EDITOR,
    BENCHMARK,
    OM_TELEPORT,
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
    DISPLAY_RADIATION,
    LEARN_SPELLS,
    LEVEL_SPELLS,
    TEST_MAP_EXTRA_DISTRIBUTION,
    NESTED_MAPGEN,
    last
};

void teleport_short();
void teleport_long();
void teleport_overmap();

void spawn_nested_mapgen();
void character_edit_menu();
void wishitem( player *p = nullptr );
void wishitem( player *p, const tripoint & );
void wishmonster( const cata::optional<tripoint> &p );
void wishmutate( player *p );
void wishskill( player *p );
void mutation_wish();
void draw_benchmark( int max_difference );

void debug();

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
