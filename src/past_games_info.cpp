#include "past_games_info.h"

#include <algorithm>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "achievement.h"
#include "cata_utility.h"
#include "debug.h"
#include "event.h"
#include "filesystem.h"
#include "json.h"
#include "memorial_logger.h"
#include "output.h"
#include "path_info.h"
#include "popup.h"
#include "stats_tracker.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui_manager.h"

static void no_op( const achievement *, bool ) {}

class too_old_memorial_file_error : std::runtime_error
{
        using runtime_error::runtime_error;
};

past_game_info::past_game_info( const JsonObject &jo )
{
    int version;
    jo.read( "memorial_version", version );
    if( version == 0 ) {
        jo.read( "log", log_ );
        stats_ = std::make_unique<stats_tracker>();
        jo.read( "stats", *stats_ );
        achievements_ = std::make_unique<achievements_tracker>( *stats_, no_op, no_op, false );
        jo.read( "achievements", *achievements_ );
        jo.read( "scores", scores_ );
    } else {
        throw JsonError( string_format( "unexpected memorial version %d", version ) );
    }

    // Extract the "standard" game info from the game started event
    event_multiset &events = stats_->get_events( event_type::game_start );
    const event_multiset::summaries_type &counts = events.counts();
    if( counts.size() != 1 ) {
        if( counts.empty() ) {
            throw too_old_memorial_file_error( "memorial file lacks game_start event" );
        }
        debugmsg( "Unexpected number of game start events: %d\n", counts.size() );
        return;
    }
    const cata::event::data_type &event_data = counts.begin()->first;
    auto avatar_name_it = event_data.find( "avatar_name" );
    if( avatar_name_it != event_data.end() ) {
        avatar_name_ = avatar_name_it->second.get_string();
    }
}

static past_games_info past_games;

// Using lazy initialization, so no need to do anything in the constructor
past_games_info::past_games_info() = default;

void past_games_info::clear()
{
    *this = past_games_info();
}

const achievement_completion_info *past_games_info::achievement( const achievement_id &ach ) const
{
    auto ach_it = completed_achievements_.find( ach );
    return ach_it == completed_achievements_.end() ? nullptr : &ach_it->second;
}

void past_games_info::ensure_loaded()
{
    if( loaded_ ) {
        return;
    }

    loaded_ = true;

    static_popup popup;
    popup.message( "%s", _( "Please wait while past game data loads…" ) );
    ui_manager::redraw();
    refresh_display();

    const std::string &memorial_dir = PATH_INFO::memorialdir();
    std::vector<std::string> filenames = get_files_from_path( ".json", memorial_dir, true, true );

    // Sort the files by the date & time encoded in the filename
    std::vector<std::pair<std::string, std::string>> sortable_filenames;
    for( const std::string &filename : filenames ) {
        std::vector<std::string> components = string_split( filename, '-' );
        if( components.size() < 7 ) {
            debugmsg( "Unexpected memorial filename %s", filename );
            continue;
        }

        components.erase( components.begin(), components.end() - 6 );
        sortable_filenames.emplace_back( join( components, "-" ), filename );
    }

    // NOLINTNEXTLINE(cata-use-localized-sorting)
    std::sort( sortable_filenames.begin(), sortable_filenames.end() );

    for( const std::pair<std::string, std::string> &filename_pair : sortable_filenames ) {
        const std::string &filename = filename_pair.second;
        std::istringstream iss( read_entire_file( filename ) );
        try {
            JsonIn jsin( iss );
            info_.emplace_back( jsin.get_object() );
        } catch( const JsonError &err ) {
            debugmsg( "Error reading memorial file %s: %s", filename, err.what() );
        } catch( const too_old_memorial_file_error & ) {
            // do nothing
        }
    }

    for( past_game_info &game : info_ ) {
        stats_tracker &stats = game.stats();
        event_multiset &events = stats.get_events( event_type::player_gets_achievement );
        const event_multiset::summaries_type &counts = events.counts();
        for( const std::pair<const cata::event::data_type, event_summary> &p : counts ) {
            const cata::event::data_type &event_data = p.first;
            auto ach_it = event_data.find( "achievement" );
            auto enabled_it = event_data.find( "achievements_enabled" );
            if( ach_it == event_data.end() || enabled_it == event_data.end() ) {
                debugmsg( "Missing field in memorial achievement data" );
                continue;
            }
            if( !enabled_it->second.get<bool>() ) {
                continue;
            }
            achievement_id ach = ach_it->second.get<achievement_id>();
            completed_achievements_[ach].games_completed.push_back( &game );
        }
        inp_mngr.pump_events();
    }
}

const past_games_info &get_past_games()
{
    past_games.ensure_loaded();
    return past_games;
}

void clear_past_games()
{
    past_games.clear();
}
