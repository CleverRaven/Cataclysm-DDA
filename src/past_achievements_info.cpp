#include <utility>
#include <vector>

#include "filesystem.h"
#include "flexbuffer_json.h"
#include "json_loader.h"
#include "past_achievements_info.h"
#include "past_games_info.h"
#include "path_info.h"
#include "string_formatter.h"

class cata_path;

void past_achievements_info::clear()
{
    *this = past_achievements_info();
}

bool past_achievements_info::is_completed( const achievement_id &ach ) const
{
    return !avatars_completed( ach ).empty();
}

std::vector<std::string> past_achievements_info::avatars_completed(
    const achievement_id &ach ) const
{
    std::vector<std::string> completed;
    const auto it = completed_achievements_.find( ach );
    if( it != completed_achievements_.end() ) {
        completed = it->second;
    }
    // Legacy data from memorials
    const achievement_completion_info *legacy = get_past_games().legacy_achievement( ach );
    if( legacy ) {
        for( const past_game_info *const info : legacy->games_completed ) {
            if( info ) {
                completed.emplace_back( info->avatar_name() );
            }
        }
    }
    return completed;
}

void past_achievements_info::load()
{
    if( loaded_ ) {
        return;
    }
    loaded_ = true;
    const cata_path &achievement_dir = PATH_INFO::achievementdir_path();
    assure_dir_exist( achievement_dir );

    std::vector<cata_path> filenames = get_files_from_path( ".json", achievement_dir, true, true );

    for( const cata_path &filename : filenames ) {
        int version;
        std::vector<achievement_id> achievements;
        std::string avatar_name;
        JsonValue jsin = json_loader::from_path( filename );
        JsonObject jo = jsin.get_object();
        jo.read( "achievement_version", version );
        if( version == 0 ) {
            // version 0 has no `avatar_name` member, skip them
            continue;
        } else if( version != 1 ) {
            throw JsonError( string_format( "unexpected past achievements version %d", version ) );
        }
        jo.read( "achievements", achievements );
        jo.read( "avatar_name", avatar_name );
        for( const achievement_id &id : achievements ) {
            completed_achievements_[id].emplace_back( avatar_name );
        }
    }

    // Ensure memorial data are also loaded for legacy dead character achievements
    get_past_games();
}

static past_achievements_info past_achievements;
past_achievements_info::past_achievements_info() = default;

const past_achievements_info &get_past_achievements()
{
    past_achievements.load();
    return past_achievements;
}

void clear_past_achievements()
{
    past_achievements.clear();
}
