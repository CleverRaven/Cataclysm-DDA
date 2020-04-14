#pragma once
#ifndef CATA_SRC_GAMEMODE_TUTORIAL_H
#define CATA_SRC_GAMEMODE_TUTORIAL_H

#include <functional>
#include <iosfwd>
#include <map>

#include "enums.h"
#include "gamemode.h"

template <typename E> struct enum_traits;

enum special_game_id : int;
enum action_id : int;

enum class tut_lesson : int {
    LESSON_INTRO = 0,
    LESSON_MOVE, LESSON_LOOK, LESSON_OPEN, LESSON_CLOSE, LESSON_SMASH,
    LESSON_WINDOW, LESSON_PICKUP, LESSON_EXAMINE, LESSON_INTERACT,

    LESSON_FULL_INV, LESSON_WIELD_NO_SPACE, LESSON_AUTOWIELD, LESSON_ITEM_INTO_INV,
    LESSON_GOT_ARMOR, LESSON_GOT_WEAPON, LESSON_GOT_FOOD, LESSON_GOT_TOOL,
    LESSON_GOT_GUN, LESSON_GOT_AMMO, LESSON_WORE_ARMOR, LESSON_WORE_STORAGE,
    LESSON_WORE_MASK,

    LESSON_WEAPON_INFO, LESSON_HIT_MONSTER, LESSON_PAIN, LESSON_BUTCHER,

    LESSON_TOOK_PAINKILLER, LESSON_TOOK_CIG, LESSON_DRANK_WATER,

    LESSON_ACT_GRENADE, LESSON_ACT_BUBBLEWRAP,

    LESSON_OVERLOADED,

    LESSON_GUN_LOAD, LESSON_GUN_FIRE, LESSON_RECOIL,

    LESSON_STAIRS, LESSON_DARK_NO_FLASH, LESSON_DARK, LESSON_PICKUP_WATER,

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
        special_game_id id() override {
            return SGAME_TUTORIAL;
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
