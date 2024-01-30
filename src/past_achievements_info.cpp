#include <vector>

#include "past_achievements_info.h"
#include "past_games_info.h"
#include "achievement.h"
#include "json.h"
#include "json_loader.h"

void past_achievements_info::clear()
{
    *this = past_achievements_info();
}

bool past_achievements_info::is_completed( const achievement_id &ach ) const
{
    auto ach_it = completed_achievements_.find( ach );
    return ach_it != completed_achievements_.end();
}

/*
 * This is for when the player copies their memorial folder
 * in and expects those achievements to still work for scenarios/professions
 *
 * It will create a save/achievements/ entry called memorial_achievements.json,
 * this file contains the achievement ids from the players memorials.
 */
bool past_achievements_info::migrate_memorial()
{
    const std::string &oldachievements_file = PATH_INFO::oldachievements();
    if( memorial_loaded_ ) {
        return false;
    }
    memorial_loaded_ = true;

    std::ostringstream oldachievements_file_path;
    oldachievements_file_path << oldachievements_file;

    return write_to_file( oldachievements_file, [&]( std::ostream & fout ) {
        get_past_games().write_json_achievements( fout );
    }, _( "player achievements" ) );
}

void past_achievements_info::load()
{
    if( loaded_ ) {
        return;
    }
    loaded_ = true;
    const cata_path &achievement_dir = PATH_INFO::achievementdir_path();
    assure_dir_exist( achievement_dir );

    migrate_memorial();
    std::vector<cata_path> filenames = get_files_from_path( ".json", achievement_dir, true, true );

    for( const cata_path &filename : filenames ) {
        int version;
        std::vector<achievement_id> achievements;
        JsonValue jsin = json_loader::from_path( filename );
        JsonObject jo = jsin.get_object();
        jo.read( "achievement_version", version );
        if( version != 0 ) {
            continue;
        }
        jo.read( "achievements", achievements );
        completed_achievements_.insert( achievements.cbegin(), achievements.cend() );
    }
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
