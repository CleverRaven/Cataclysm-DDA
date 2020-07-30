#include "past_games_info.h"

#include "achievement.h"
#include "filesystem.h"
#include "json.h"
#include "memorial_logger.h"
#include "output.h"
#include "path_info.h"
#include "popup.h"
#include "stats_tracker.h"
#include "ui_manager.h"

static void no_op( const achievement *, bool ) {}

class past_game_info
{
    public:
        past_game_info( JsonIn &jsin ) {
            JsonObject jo = jsin.get_object();
            int version;
            jo.read( "memorial_version", version );
            if( version == 0 ) {
                jo.read( "log", log_ );
                stats_ = std::make_unique<stats_tracker>();
                jo.read( "stats", *stats_ );
                achievements_ = std::make_unique<achievements_tracker>( *stats_, no_op, no_op );
                jo.read( "achievements", *achievements_ );
                jo.read( "scores", scores_ );
            } else {
                throw JsonError( string_format( "unexpected memorial version %d", version ) );
            }
        }

        stats_tracker &stats() {
            return *stats_;
        }
    private:
        std::vector<memorial_log_entry> log_;
        std::unique_ptr<stats_tracker> stats_;
        std::unique_ptr<achievements_tracker> achievements_;
        std::unordered_map<string_id<score>, cata_variant> scores_;
};

static past_games_info past_games;

// Using lazy initialization, so no need to do anything in the constructor
past_games_info::past_games_info() = default;

void past_games_info::clear()
{
    *this = past_games_info();
}

bool past_games_info::was_achievement_completed( const achievement_id &ach ) const
{
    return completed_achievements_.count( ach );
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

    for( const std::string &filename : filenames ) {
        std::istringstream iss( read_entire_file( filename ) );
        try {
            JsonIn jsin( iss );
            info_.push_back( past_game_info( jsin ) );
        } catch( const JsonError &err ) {
            debugmsg( "Error reading memorial file %s: %s", filename, err.what() );
        }
    }

    for( past_game_info &game : info_ ) {
        stats_tracker &stats = game.stats();
        event_multiset &events = stats.get_events( event_type::player_gets_achievement );
        const event_multiset::summaries_type &counts = events.counts();
        for( const std::pair<cata::event::data_type, event_summary> &p : counts ) {
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
            completed_achievements_.insert( ach_it->second.get<achievement_id>() );
        }
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
