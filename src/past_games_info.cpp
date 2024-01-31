#include "past_games_info.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_set>
#include <utility>

#include "achievement.h"
#include "cata_utility.h"
#include "debug.h"
#include "event.h"
#include "filesystem.h"
#include "input.h"
#include "json_loader.h"
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

    // Extract avatar name info from the game_avatar_new event
    // gives the starting character name; "et. al." is appended if there was character switching
    event_multiset &new_avatar_events = stats_->get_events( event_type::game_avatar_new );
    const event_multiset::summaries_type &new_avatar_counts = new_avatar_events.counts();
    if( !new_avatar_counts.empty() ) {
        const cata::event::data_type &new_avatar_event_data = new_avatar_counts.begin()->first;
        auto avatar_name_it = new_avatar_event_data.find( "avatar_name" );
        if( avatar_name_it != new_avatar_event_data.end() ) {
            avatar_name_ = avatar_name_it->second.get_string() +
                           ( new_avatar_counts.size() > 1 ? " et. al." : "" );
        }
    } else {
        // Legacy approach using the game_start event
        event_multiset &start_events = stats_->get_events( event_type::game_start );
        const event_multiset::summaries_type &start_counts = start_events.counts();
        if( start_counts.size() != 1 ) {
            if( start_counts.empty() ) {
                throw too_old_memorial_file_error( "memorial file lacks game_start event" );
            }
            debugmsg( "Unexpected number of game start events: %d\n", start_counts.size() );
            return;
        }
        const cata::event::data_type &start_event_data = start_counts.begin()->first;
        auto avatar_name_it = start_event_data.find( "avatar_name" );
        if( avatar_name_it != start_event_data.end() ) {
            avatar_name_ = avatar_name_it->second.get_string();
        }
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

void past_games_info::write_json_achievements( std::ostream &achievement_file ) const
{
    JsonOut jsout( achievement_file, true, 2 );
    jsout.start_object();
    jsout.member( "achievement_version", 0 );

    std::unordered_set<std::string> ach_id_strings;

    for( achievement_id kv : ach_ids_ ) {
        if( achievement( kv ) ) {
            ach_id_strings.insert( kv.c_str() );
        }
    }

    jsout.member( "achievements", ach_id_strings );

    jsout.end_object();
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

    const cata_path &memorial_dir = PATH_INFO::memorialdir_path();
    assure_dir_exist( memorial_dir );
    std::vector<cata_path> filenames = get_files_from_path( ".json", memorial_dir, true, true );

    // Sort the files by the date & time encoded in the filename
    std::vector<std::pair<std::string, cata_path>> sortable_filenames;
    for( const cata_path &filename : filenames ) {
        std::vector<std::string> components = string_split(
                filename.get_unrelative_path().generic_u8string(), '-' );
        if( components.size() < 7 ) {
            debugmsg( "Unexpected memorial filename %s", filename.generic_u8string() );
            continue;
        }

        components.erase( components.begin(), components.end() - 6 );
        sortable_filenames.emplace_back( string_join( components, "-" ), filename );
    }

    std::sort( sortable_filenames.begin(),
               sortable_filenames.end(), []( const std::pair<std::string, cata_path> &l,
    const std::pair<std::string, cata_path> &r ) {
        // Manually replicating std::pair<T,U>::operator < to avoid implementing operator< on cata_path.
        // NOLINTNEXTLINE(cata-use-localized-sorting)
        if( l.first < r.first ) {
            return true;
        }
        // NOLINTNEXTLINE(cata-use-localized-sorting)
        if( l.first > r.first ) {
            return false;

        }
        return l.second.get_unrelative_path() < r.second.get_unrelative_path();
    } );

    for( const std::pair<std::string, cata_path> &filename_pair : sortable_filenames ) {
        const cata_path &filename = filename_pair.second;
        try {
            JsonValue jsin = json_loader::from_path( filename );
            info_.emplace_back( jsin.get_object() );
        } catch( const JsonError &err ) {
            debugmsg( "Error reading memorial file %s: %s", filename.generic_u8string(), err.what() );
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
            ach_ids_.push_back( ach );
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
