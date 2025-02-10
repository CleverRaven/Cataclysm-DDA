#pragma once
#ifndef CATA_SRC_GAMEMODE_TUTORIAL_H
#define CATA_SRC_GAMEMODE_TUTORIAL_H

#include <cstddef>
#include <functional>
#include <map>

#include "enums.h"
#include "gamemode.h"

template <typename E> struct enum_traits;

enum class tut_lesson : int {
    LESSON_INTRO = 0,
    LESSON_MOVE, LESSON_MOVEMENT_MODES, LESSON_LOOK, LESSON_OPEN, LESSON_CLOSE, LESSON_SMASH,
    LESSON_WINDOW, LESSON_PICKUP, LESSON_EXAMINE, LESSON_INTERACT,

    LESSON_GOT_ARMOR, LESSON_GOT_WEAPON, LESSON_GOT_FOOD, LESSON_GOT_TOOL,
    LESSON_GOT_GUN, LESSON_GOT_AMMO, LESSON_WORE_ARMOR, LESSON_WORE_STORAGE,
    LESSON_WORE_MASK,

    LESSON_WEAPON_INFO, LESSON_HIT_MONSTER, LESSON_PAIN, LESSON_BUTCHER,

    LESSON_TOOK_PAINKILLER, LESSON_TOOK_CIG, LESSON_DRANK_WATER,

    LESSON_ACT_GRENADE, LESSON_ACT_BUBBLEWRAP,

    LESSON_GUN_LOAD, LESSON_GUN_FIRE, LESSON_RECOIL,

    LESSON_STAIRS, LESSON_DARK_NO_FLASH, LESSON_DARK, LESSON_PICKUP_WATER,

    LESSON_MONSTER_SIGHTED, LESSON_REACH_ATTACK, LESSON_HOLSTERS_WEAR, LESSON_HOLSTERS_ACTIVATE,
    LESSON_LOCKED_DOOR, LESSON_RESTORE_STAMINA, LESSON_INVENTORY, LESSON_FLASHLIGHT,
    LESSON_REMOTE_USE, LESSON_CRAFTING_FOOD, LESSON_CONSTRUCTION, LESSON_THROWING,
    LESSON_FINALE,

    NUM_LESSONS
};

template<>
struct enum_traits<tut_lesson> {
    static constexpr tut_lesson last = tut_lesson::NUM_LESSONS;
};

namespace std
{

template<>
struct hash<tut_lesson> {
    size_t operator()( const tut_lesson v ) const noexcept {
        return static_cast<size_t>( v );
    }
};

} // namespace std

struct tutorial_game : public special_game {
        special_game_type id() override {
            return special_game_type::TUTORIAL;
        }
        bool init() override;
        void per_turn() override;
        void pre_action( action_id &act ) override;
        void post_action( action_id act ) override;
        void game_over() override { }

    private:
        void add_message( tut_lesson lesson );

        std::map<tut_lesson, bool> tutorials_seen;
};

#endif // CATA_SRC_GAMEMODE_TUTORIAL_H
