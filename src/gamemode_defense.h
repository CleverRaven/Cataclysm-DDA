#pragma once
#ifndef CATA_SRC_GAMEMODE_DEFENSE_H
#define CATA_SRC_GAMEMODE_DEFENSE_H

#include <string>
#include <vector>

#include "calendar.h"
#include "cursesdef.h"
#include "enums.h"
#include "gamemode.h"
#include "omdata.h"
#include "point.h"
#include "type_id.h"

enum action_id : int;
using itype_id = std::string;

enum defense_style {
    DEFENSE_CUSTOM = 0,
    DEFENSE_EASY,
    DEFENSE_MEDIUM,
    DEFENSE_HARD,
    DEFENSE_SHAUN,
    DEFENSE_DAWN,
    DEFENSE_SPIDERS,
    DEFENSE_TRIFFIDS,
    DEFENSE_SKYNET,
    DEFENSE_LOVECRAFT,
    NUM_DEFENSE_STYLES
};

enum defense_location {
    DEFLOC_NULL = 0,
    DEFLOC_HOSPITAL,
    DEFLOC_WORKS,
    DEFLOC_MALL,
    DEFLOC_BAR,
    DEFLOC_MANSION,
    NUM_DEFENSE_LOCATIONS
};

enum caravan_category {
    CARAVAN_CART = 0,
    CARAVAN_MELEE,
    CARAVAN_RANGED,
    CARAVAN_AMMUNITION,
    CARAVAN_COMPONENTS,
    CARAVAN_FOOD,
    CARAVAN_CLOTHES,
    CARAVAN_TOOLS,
    NUM_CARAVAN_CATEGORIES
};

struct defense_game : public special_game {
        defense_game();

        special_game_id id() override {
            return SGAME_DEFENSE;
        }
        bool init() override;
        void per_turn() override;
        void pre_action( action_id &act ) override;
        void post_action( action_id act ) override;
        void game_over() override;

    private:
        void init_to_style( defense_style new_style );

        void setup();
        void refresh_setup( const catacurses::window &w, int selection );
        void init_mtypes();
        void init_constructions();
        void init_map();

        void spawn_wave();
        void caravan();
        std::vector<mtype_id> pick_monster_wave();
        void spawn_wave_monster( const mtype_id &type );

        std::string special_wave_message( std::string name );

        int current_wave = 0;

        // What type of game is it?
        defense_style style;
        // Where are we?
        defense_location location;

        // Total "level" of monsters in first wave
        int initial_difficulty = 0;
        // Increased "level" of monsters per wave
        int wave_difficulty = 0;

        // Cooldown / building / healing time
        time_duration time_between_waves = 0_turns;
        // How many waves until we get to trade?
        int waves_between_caravans = 0;

        // How much cash do we start with?
        int initial_cash = 0;
        // How much cash do we get per wave?
        int cash_per_wave = 0;
        // How much does the above increase per wave?
        int cash_increase = 0;

        bool zombies = false;
        bool specials = false;
        bool spiders = false;
        bool triffids = false;
        bool robots = false;
        bool subspace = false;

        // Do we need to fulfill hunger?
        bool hunger = false;
        // Do we need to fulfill thirst?
        bool thirst = false;
        // Do we need to sleep?
        bool sleep = false;

        // Do caravans offer the option of hiring a mercenary?
        bool mercenaries = false;

        // Allow save
        bool allow_save = false;

        // Start defence location position on overmap
        tripoint defloc_pos;

        // Defense location special
        overmap_special_id defloc_special;
};

#endif // CATA_SRC_GAMEMODE_DEFENSE_H
