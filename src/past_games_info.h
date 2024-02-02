#pragma once
#ifndef CATA_SRC_PAST_GAMES_INFO_H
#define CATA_SRC_PAST_GAMES_INFO_H

#include <iosfwd>
#include <memory>
#include <unordered_map>
#include <vector>

#include "achievement.h"
#include "cata_variant.h"
#include "memorial_logger.h"
#include "stats_tracker.h"
#include "type_id.h"

class JsonObject;
class score;

class past_game_info
{
    public:
        explicit past_game_info( const JsonObject &jo );

        stats_tracker &stats() {
            return *stats_;
        }

        const std::string &avatar_name() const {
            return avatar_name_;
        }
    private:
        std::vector<memorial_log_entry> log_;
        std::unique_ptr<stats_tracker> stats_;
        std::unique_ptr<achievements_tracker> achievements_;
        std::unordered_map<string_id<score>, cata_variant> scores_;
        std::string avatar_name_;
};

struct achievement_completion_info {
    std::vector<const past_game_info *> games_completed;
};

// This class is intended to provide information about past games loaded from
// memorial files.  It can be used for example to know what achievements have
// been completed in past games.
class past_games_info
{
    public:
        past_games_info();

        void write_json_achievements( std::ostream &achievement_file ) const;
        void ensure_loaded();
        void clear();
        const achievement_completion_info *achievement( const achievement_id & ) const;
    private:

        bool loaded_ = false;
        std::unordered_map<achievement_id, achievement_completion_info> completed_achievements_;
        std::vector<past_game_info> info_;
        std::vector<achievement_id> ach_ids_;
};

const past_games_info &get_past_games();
void clear_past_games();

#endif // CATA_SRC_PAST_GAMES_INFO_H
