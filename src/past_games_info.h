#ifndef CATA_SRC_PAST_GAMES_INFO_H
#define CATA_SRC_PAST_GAMES_INFO_H

#include <unordered_set>
#include <vector>

#include "type_id.h"

class past_game_info;

// This class is intended to provide information about past games loaded from
// memorial files.  It can be used for example to know what achievements have
// been completed in past games.
class past_games_info
{
    public:
        past_games_info();

        void ensure_loaded();
        void clear();
        bool was_achievement_completed( const achievement_id & ) const;
    private:

        bool loaded_ = false;
        std::unordered_set<achievement_id> completed_achievements_;
        std::vector<past_game_info> info_;
};

const past_games_info &get_past_games();
void clear_past_games();

#endif // CATA_SRC_PAST_GAMES_INFO_H
