#pragma once
#ifndef CATA_SRC_PAST_ACHIEVEMENTS_INFO_H
#define CATA_SRC_PAST_ACHIEVEMENTS_INFO_H

#include "achievement.h"

struct achievements_completed {
    std::set<const achievement_id *> achievements;
};

class past_achievements_info
{
    public:
        past_achievements_info();

        void load();
        void clear();
        bool isCompleted( const achievement_id & ) const;
    private:

        bool loaded_ = false;
        std::set<achievement_id> completed_achievements_;
};


const past_achievements_info &get_past_achievements();
void clear_past_achievements();

#endif // CATA_SRC_PAST_ACHIEVEMENTS_INFO_H
