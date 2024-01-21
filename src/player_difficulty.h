#pragma once
#ifndef CATA_SRC_PLAYER_DIFFICULTY_H
#define CATA_SRC_PLAYER_DIFFICULTY_H

#include <npc.h>

// The point after which stats cost double
constexpr int HIGH_STAT = 12;

enum class pool_type {
    FREEFORM = 0,
    ONE_POOL,
    MULTI_POOL,
    TRANSFER,
};

class player_difficulty
{
    private:
        player_difficulty();
        ~player_difficulty() = default;

        // calculate individual properties
        std::string get_defense_difficulty( const Character &u ) const;
        std::string get_combat_difficulty( const Character &u ) const;
        std::string get_genetics_difficulty( const Character &u ) const;
        std::string get_expertise_difficulty( const Character &u ) const;
        std::string get_social_difficulty( const Character &u ) const;

        // helpers for the above functions
        static double calc_armor_value( const Character &u );
        static double calc_dps_value( const Character &u );
        static int calc_social_value( const Character &u, const npc &compare );

        // npc helpers
        static void reset_npc( Character &dummy );
        static void npc_from_avatar( const avatar &u, npc &dummy );

        // format the output
        // percent band is the range to consider for values
        // per is the actual percent as a decimal
        // difficulty is true for things going from Very Easy to Very Hard
        // difficutly is false for things going from Very Weak to Very Powerful
        static std::string format_output( float percent_band, float per );

        npc average;

    public:
        const npc &get_average_npc();

        player_difficulty( const player_difficulty & ) = delete;
        player_difficulty &operator= ( const player_difficulty & ) = delete;

        static player_difficulty &getInstance() {
            static player_difficulty instance;
            return instance;
        }

        // call to get the details out
        std::string difficulty_to_string( const avatar &u ) const;
};

#endif // CATA_SRC_PLAYER_DIFFICULTY_H
