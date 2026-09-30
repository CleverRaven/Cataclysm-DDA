#pragma once
#ifndef CATA_SRC_PLAYER_DIFFICULTY_H
#define CATA_SRC_PLAYER_DIFFICULTY_H

#include <map>
#include <string>
#include <utility>

#include "npc.h"

class Character;
class avatar;

// The point after which stats cost double
constexpr int HIGH_STAT = 12;

//Leftover from removing the legacy point pool character creation. TRANSFER is required to be 3 so transfer character templates dont break
enum class pool_type {
    FREEFORM = 0,
    TRANSFER = 3,
};

enum class rating_category {
    Defense,
    Combat,
    Genetics,
    Expertise,
    Social
};

// Helper to determine what band each of our character values are at. They use different percents for each category, so...
enum class OP_level {
    Underpowered,
    Weak,
    Average,
    Strong,
    Powerful,
    Overpowered
};

class player_difficulty
{
    private:
        player_difficulty();
        ~player_difficulty() = default;

        // Calculate individual properties. Each call updates the `ratings` map to record their current OP_level for that category.
        std::pair<OP_level, std::string> get_defense_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_combat_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_genetics_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_expertise_difficulty( const Character &u );
        std::pair<OP_level, std::string> get_social_difficulty( const Character &u );

        // helpers for the above functions
        static double calc_armor_value( const Character &u );
        static double calc_dps_value( const Character &u );
        static int calc_social_value( const Character &u, const npc &compare );

        // npc helpers
        static void reset_npc( Character &dummy );
        static void npc_from_avatar( const avatar &u, npc &dummy );

        npc average;

        std::map<rating_category, OP_level> ratings;

    public:
        const npc &get_average_npc();

        player_difficulty( const player_difficulty & ) = delete;
        player_difficulty &operator= ( const player_difficulty & ) = delete;

        static player_difficulty &getInstance() {
            static player_difficulty instance;
            return instance;
        }

        std::map<rating_category, OP_level> get_ratings() const;

        // Get a description of a specific rating.
        // The optional 'per' argument serves double purpose as a sentinel value. Failing to pass it in will skip debug text.
        // We skip this debug text in one newcharacter.cpp query since at that point we don't know what rating_category and thus what `per` should be used.
        static std::string format_text_for( OP_level overpowered, float per = -9999.9f );

        // Returns a formatted single-line string with names and descriptions all ratings.
        std::string difficulty_to_string( const avatar &u );
};

#endif // CATA_SRC_PLAYER_DIFFICULTY_H
