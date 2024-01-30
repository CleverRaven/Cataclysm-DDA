#pragma once
#ifndef CATA_SRC_PAST_ACHIEVEMENTS_INFO_H
#define CATA_SRC_PAST_ACHIEVEMENTS_INFO_H

#include <set>

#include "achievement.h"

class past_achievements_info
{
    public:
        past_achievements_info();

        bool migrate_memorial();
        void load();
        void clear();
        bool is_completed( const achievement_id & ) const;
    private:

        bool loaded_ = false;
        bool memorial_loaded_ = false;
        std::set<achievement_id> completed_achievements_;
};


const past_achievements_info &get_past_achievements();
void clear_past_achievements();

#endif // CATA_SRC_PAST_ACHIEVEMENTS_INFO_H
