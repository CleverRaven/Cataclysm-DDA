#pragma once
#ifndef CATA_SRC_PAST_ACHIEVEMENTS_INFO_H
#define CATA_SRC_PAST_ACHIEVEMENTS_INFO_H

#include <map>

#include "achievement.h"

class past_achievements_info
{
    public:
        past_achievements_info();

        void load();
        void clear();
        bool is_completed( const achievement_id & ) const;
        std::vector<std::string> avatars_completed( const achievement_id & ) const;

    private:
        bool loaded_ = false;
        std::map<achievement_id, std::vector<std::string>> completed_achievements_;
};


const past_achievements_info &get_past_achievements();
void clear_past_achievements();

#endif // CATA_SRC_PAST_ACHIEVEMENTS_INFO_H
