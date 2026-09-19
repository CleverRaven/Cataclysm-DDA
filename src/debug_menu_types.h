#pragma once
#ifndef CATA_SRC_DEBUG_MENU_TYPES_H
#define CATA_SRC_DEBUG_MENU_TYPES_H

#include <cstddef>
#include <functional>

template <typename E> struct enum_traits;

namespace debug_menu
{

enum class debug_menu_index : int {
    WISH,
    SPAWN_ITEM_GROUP,
    SHORT_TELEPORT,
    LONG_TELEPORT,
    SPAWN_NPC,
    SPAWN_NPC_FOLLOWER,
    SPAWN_NAMED_NPC,
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
    FORGET_ALL_RECIPES,
    FORGET_ALL_ITEMS,
    UNLOCK_ALL,
    EDIT_PLAYER,
    EDIT_MONSTER,
    CONTROL_NPC,
    SPAWN_ARTIFACT,
    SPAWN_CLAIRVOYANCE,
    SPAWN_HORDE,
    MAP_EDITOR,
    PALETTE_VIEWER,
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
    PRINT_OVERMAPS,
    PRINT_REGION_LAYOUT,
    TRAIT_GROUP,
    ENABLE_ACHIEVEMENTS,
    SHOW_MSG,
    CRASH_GAME,
    TEST_END_SCREEN,
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
    DISPLAY_SNOW_DEPTH,
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
    QUICK_SETUP_FLAG_DIRTY,
    QUICK_SETUP_CLEAR_MAP,
    TOGGLE_SETUP_MUTATION,
    NORMALIZE_BODY_STAT,
    SIX_MILLION_DOLLAR_SURVIVOR,
    EDIT_FACTION,
    WRITE_CITY_LIST,
    TALK_TOPIC,
    IMGUI_DEMO,
    VEHICLE_EFFECTS,
    WISHPROFICIENCY,
    RELOAD_GPU_SHADERS,
    last
};

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

#endif // CATA_SRC_DEBUG_MENU_TYPES_H
