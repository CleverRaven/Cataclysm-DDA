#include "achievement.h"

#include <assert.h>
#include <stdlib.h>
#include <set>
#include <tuple>
#include <utility>

#include "color.h"
#include "debug.h"
#include "enums.h"
#include "event.h"
#include "event_statistics.h"
#include "generic_factory.h"
#include "json.h"
#include "stats_tracker.h"
#include "string_formatter.h"

// Some details about how achievements work
// ========================================
//
// Achievements are built on the stats_tracker, which is in turn built on the
// event_bus.  Each of these layers involves subscription / callback style
// interfaces, so the code flow may now be obvious.  Here's a quick outline of
// the execution flow to help clarify how it all fits together.
//
// * Various core game code paths generate events via the event bus.
// * The stats_traecker subscribes to the event bus, and receives these events.
// * Events contribute to event_multisets managed by the stats_tracker.
// * (In the docs, these event_multisets are described as "event streams").
// * (Optionally) event_transformations transform these event_multisets into
//   other event_multisets based on json-defined transformation rules.  These
//   are also managed by stats_tracker.
// * event_statistics monitor these event_multisets and summarize them into
//   single values.  These are also managed by stats_tracker.
// * Each achievement requirement has a corresponding requirement_watcher which
//   is alerted to statistic changes via the stats_tracker's watch interface.
// * Each requirement_watcher notifies its achievement_tracker (of which there
//   is one per achievement) of the requirement's status on each change to a
//   statistic.
// * The achievement_tracker keeps track of which requirements are currently
//   satisfied and which are not.
// * When all the requirements are satisfied, the achievement_tracker tells the
//   achievement_tracker (only one of these exists per game).
// * The achievement_tracker calls the achievement_attained_callback it was
//   given at construction time.  This hooks into the actual game logic (e.g.
//   telling the player they just got an achievement).

namespace
{

generic_factory<achievement> achievement_factory( "achievement" );

} // namespace

/** @relates string_id */
template<>
const achievement &string_id<achievement>::obj() const
{
    return achievement_factory.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<achievement>::is_valid() const
{
    return achievement_factory.is_valid( *this );
}

namespace io
{

template<>
std::string enum_to_string<achievement_comparison>( achievement_comparison data )
{
    switch( data ) {
        // *INDENT-OFF*
        case achievement_comparison::less_equal: return "<=";
        case achievement_comparison::greater_equal: return ">=";
        case achievement_comparison::anything: return "anything";
        // *INDENT-ON*
        case achievement_comparison::last:
            break;
    }
    debugmsg( "Invalid achievement_comparison" );
    abort();
}

template<>
std::string enum_to_string<achievement::time_bound::epoch>( achievement::time_bound::epoch data )
{
    switch( data ) {
        // *INDENT-OFF*
        case achievement::time_bound::epoch::cataclysm: return "cataclysm";
        case achievement::time_bound::epoch::game_start: return "game_start";
        // *INDENT-ON*
        case achievement::time_bound::epoch::last:
            break;
    }
    debugmsg( "Invalid epoch" );
    abort();
}

} // namespace io

static nc_color color_from_completion( achievement_completion comp )
{
    switch( comp ) {
        case achievement_completion::pending:
            return c_yellow;
        case achievement_completion::completed:
            return c_light_green;
        case achievement_completion::failed:
            return c_light_gray;
        case achievement_completion::last:
            break;
    }
    debugmsg( "Invalid achievement_completion" );
    abort();
}

struct achievement_requirement {
    string_id<event_statistic> statistic;
    achievement_comparison comparison;
    int target;
    bool becomes_false;

    void deserialize( JsonIn &jin ) {
        const JsonObject &jo = jin.get_object();
        if( !( jo.read( "event_statistic", statistic ) &&
               jo.read( "is", comparison ) &&
               ( comparison == achievement_comparison::anything ||
                 jo.read( "target", target ) ) ) ) {
            jo.throw_error( "Mandatory field missing for achievement requirement" );
        }
    }

    void finalize() {
        switch( comparison ) {
            case achievement_comparison::less_equal:
                becomes_false = is_increasing( statistic->monotonicity() );
                return;
            case achievement_comparison::greater_equal:
                becomes_false = is_decreasing( statistic->monotonicity() );
                return;
            case achievement_comparison::anything:
                becomes_false = true;
                return;
            case achievement_comparison::last:
                break;
        }
        debugmsg( "Invalid achievement_comparison" );
        abort();
    }

    void check( const string_id<achievement> &id ) const {
        if( !statistic.is_valid() ) {
            debugmsg( "score %s refers to invalid statistic %s", id.str(), statistic.str() );
        }
    }

    bool satisifed_by( const cata_variant &v ) const {
        int value = v.get<int>();
        switch( comparison ) {
            case achievement_comparison::less_equal:
                return value <= target;
            case achievement_comparison::greater_equal:
                return value >= target;
            case achievement_comparison::anything:
                return true;
            case achievement_comparison::last:
                break;
        }
        debugmsg( "Invalid achievement_requirement comparison value" );
        abort();
    }
};

static time_point epoch_to_time_point( achievement::time_bound::epoch e )
{
    switch( e ) {
        case achievement::time_bound::epoch::cataclysm:
            return calendar::start_of_cataclysm;
        case achievement::time_bound::epoch::game_start:
            return calendar::start_of_game;
        case achievement::time_bound::epoch::last:
            break;
    }
    debugmsg( "Invalid epoch" );
    abort();
}

void achievement::time_bound::deserialize( JsonIn &jin )
{
    const JsonObject &jo = jin.get_object();
    if( !( jo.read( "since", epoch_ ) &&
           jo.read( "is", comparison_ ) &&
           jo.read( "target", period_ ) ) ) {
        jo.throw_error( "Mandatory field missing for achievement time_constaint" );
    }
}

void achievement::time_bound::check( const string_id<achievement> &id ) const
{
    if( comparison_ == achievement_comparison::anything ) {
        debugmsg( "Achievement %s has unconstrained \"anything\" time_constraint.  "
                  "Please change it to a consequential comparison.", id.str() );
    }
}

time_point achievement::time_bound::target() const
{
    return epoch_to_time_point( epoch_ ) + period_;
}

achievement_completion achievement::time_bound::completed() const
{
    time_point now = calendar::turn;
    switch( comparison_ ) {
        case achievement_comparison::less_equal:
            if( now <= target() ) {
                return achievement_completion::completed;
            } else {
                return achievement_completion::failed;
            }
        case achievement_comparison::greater_equal:
            if( now >= target() ) {
                return achievement_completion::completed;
            } else {
                return achievement_completion::pending;
            }
        case achievement_comparison::anything:
            return achievement_completion::completed;
        case achievement_comparison::last:
            break;
    }
    debugmsg( "Invalid achievement_comparison" );
    abort();
}

std::string achievement::time_bound::ui_text() const
{
    time_point now = calendar::turn;
    achievement_completion comp = completed();

    nc_color c = color_from_completion( comp );

    auto translate_epoch = []( epoch e ) {
        switch( e ) {
            case epoch::cataclysm:
                return _( "time of cataclysm" );
            case epoch::game_start:
                return _( "start of game" );
            case epoch::last:
                break;
        }
        debugmsg( "Invalid epoch" );
        abort();
    };

    std::string message = [&]() {
        switch( comp ) {
            case achievement_completion::pending:
                return string_format( _( "At least %s from %s (%s remaining)" ),
                                      to_string( period_ ), translate_epoch( epoch_ ),
                                      to_string( target() - now ) );
            case achievement_completion::completed:
                switch( comparison_ ) {
                    case achievement_comparison::less_equal:
                        return string_format( _( "Within %s of %s (%s remaining)" ),
                                              to_string( period_ ), translate_epoch( epoch_ ),
                                              to_string( target() - now ) );
                    case achievement_comparison::greater_equal:
                        return string_format( _( "At least %s from %s (passed)" ),
                                              to_string( period_ ), translate_epoch( epoch_ ) );
                    case achievement_comparison::anything:
                        return std::string();
                    case achievement_comparison::last:
                        break;
                }
                debugmsg( "Invalid achievement_comparison" );
                abort();
            case achievement_completion::failed:
                return string_format( _( "Within %s of %s (passed)" ),
                                      to_string( period_ ), translate_epoch( epoch_ ) );
            case achievement_completion::last:
                break;
        }
        debugmsg( "Invalid achievement_completion" );
        abort();
    }
    ();

    return colorize( message, c );
}

void achievement::load_achievement( const JsonObject &jo, const std::string &src )
{
    achievement_factory.load( jo, src );
}

void achievement::finalize()
{
    for( achievement &a : const_cast<std::vector<achievement>&>( achievement::get_all() ) ) {
        for( achievement_requirement &req : a.requirements_ ) {
            req.finalize();
        }
    }
}

void achievement::check_consistency()
{
    achievement_factory.check();
}

const std::vector<achievement> &achievement::get_all()
{
    return achievement_factory.get_all();
}

void achievement::reset()
{
    achievement_factory.reset();
}

void achievement::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "name", name_ );
    optional( jo, was_loaded, "description", description_ );
    optional( jo, was_loaded, "hidden_by", hidden_by_ );
    optional( jo, was_loaded, "time_constraint", time_constraint_ );
    mandatory( jo, was_loaded, "requirements", requirements_ );
}

void achievement::check() const
{
    for( const string_id<achievement> &a : hidden_by_ ) {
        if( !a.is_valid() ) {
            debugmsg( "Achievement %s specifies hidden_by achievement %s, but the latter does not "
                      "exist.", id.str(), a.str() );
        }
    }
    if( time_constraint_ ) {
        time_constraint_->check( id );
    }
    for( const achievement_requirement &req : requirements_ ) {
        req.check( id );
    }
}

static std::string text_for_requirement( const achievement_requirement &req,
        const cata_variant &current_value )
{
    bool is_satisfied = req.satisifed_by( current_value );
    nc_color c = is_satisfied ? c_green : c_yellow;
    int current = current_value.get<int>();
    int target;
    std::string result;
    if( req.comparison == achievement_comparison::anything ) {
        target = 1;
        result = string_format( _( "Triggered by " ) );
    } else {
        target = req.target;
        result = string_format( _( "%s/%s " ), current, target );
    }
    result += req.statistic->description().translated( target );
    return colorize( result, c );
}

class requirement_watcher : stat_watcher
{
    public:
        requirement_watcher( achievement_tracker &tracker, const achievement_requirement &req,
                             stats_tracker &stats ) :
            current_value_( req.statistic->value( stats ) ),
            tracker_( &tracker ),
            requirement_( &req ) {
            stats.add_watcher( req.statistic, this );
        }

        const cata_variant &current_value() const {
            return current_value_;
        }

        const achievement_requirement &requirement() const {
            return *requirement_;
        }

        void new_value( const cata_variant &new_value, stats_tracker & ) override;

        bool is_satisfied( stats_tracker &stats ) {
            return requirement_->satisifed_by( requirement_->statistic->value( stats ) );
        }

        std::string ui_text() const {
            return text_for_requirement( *requirement_, current_value_ );
        }
    private:
        cata_variant current_value_;
        achievement_tracker *tracker_;
        const achievement_requirement *requirement_;
};

void requirement_watcher::new_value( const cata_variant &new_value, stats_tracker & )
{
    if( !tracker_->time_is_expired() ) {
        current_value_ = new_value;
    }
    // set_requirement can result in this being deleted, so it must be the last
    // thing in this function
    tracker_->set_requirement( this, requirement_->satisifed_by( current_value_ ) );
}

namespace io
{
template<>
std::string enum_to_string<achievement_completion>( achievement_completion data )
{
    switch( data ) {
        // *INDENT-OFF*
        case achievement_completion::pending: return "pending";
        case achievement_completion::completed: return "completed";
        case achievement_completion::failed: return "failed";
        // *INDENT-ON*
        case achievement_completion::last:
            break;
    }
    debugmsg( "Invalid achievement_completion" );
    abort();
}

} // namespace io

std::string achievement_state::ui_text( const achievement *ach ) const
{
    // First: the achievement name and description
    nc_color c = color_from_completion( completion );
    std::string result = colorize( ach->name(), c ) + "\n";
    if( !ach->description().empty() ) {
        result += "  " + colorize( ach->description(), c ) + "\n";
    }

    if( completion == achievement_completion::completed ) {
        std::string message = string_format(
                                  _( "Completed %s" ), to_string( last_state_change ) );
        result += "  " + colorize( message, c ) + "\n";
    } else {
        // Next: the time constraint
        if( ach->time_constraint() ) {
            result += "  " + ach->time_constraint()->ui_text() + "\n";
        }
    }

    // Next: the requirements
    const std::vector<achievement_requirement> &reqs = ach->requirements();
    // If these two vectors are of different sizes then the definition must
    // have changed since it was complated / failed, so we don't print any
    // requirements info.
    if( final_values.size() == reqs.size() ) {
        for( size_t i = 0; i < final_values.size(); ++i ) {
            result += "  " + text_for_requirement( reqs[i], final_values[i] ) + "\n";
        }
    }

    return result;
}

void achievement_state::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member_as_string( "completion", completion );
    jsout.member( "last_state_change", last_state_change );
    jsout.end_object();
}

void achievement_state::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    jo.read( "completion", completion );
    jo.read( "last_state_change", last_state_change );
}

achievement_tracker::achievement_tracker( const achievement &a, achievements_tracker &tracker,
        stats_tracker &stats ) :
    achievement_( &a ),
    tracker_( &tracker )
{
    for( const achievement_requirement &req : a.requirements() ) {
        watchers_.push_back( std::make_unique<requirement_watcher>( *this, req, stats ) );
    }

    for( const std::unique_ptr<requirement_watcher> &watcher : watchers_ ) {
        bool is_satisfied = watcher->is_satisfied( stats );
        sorted_watchers_[is_satisfied].insert( watcher.get() );
    }
}

void achievement_tracker::set_requirement( requirement_watcher *watcher, bool is_satisfied )
{
    if( sorted_watchers_[is_satisfied].insert( watcher ).second ) {
        // Remove from other; check for completion.
        sorted_watchers_[!is_satisfied].erase( watcher );
        assert( sorted_watchers_[0].size() + sorted_watchers_[1].size() == watchers_.size() );
    }

    achievement_completion time_comp =
        achievement_->time_constraint() ?
        achievement_->time_constraint()->completed() : achievement_completion::completed;

    if( sorted_watchers_[false].empty() && time_comp == achievement_completion::completed ) {
        // report_achievement can result in this being deleted, so it must be
        // the last thing in the function
        tracker_->report_achievement( achievement_, achievement_completion::completed );
        return;
    }

    if( time_comp == achievement_completion::failed ||
        ( !is_satisfied && watcher->requirement().becomes_false ) ) {
        // report_achievement can result in this being deleted, so it must be
        // the last thing in the function
        tracker_->report_achievement( achievement_, achievement_completion::failed );
    }
}

bool achievement_tracker::time_is_expired() const
{
    return achievement_->time_constraint() &&
           achievement_->time_constraint()->completed() == achievement_completion::failed;
}

std::vector<cata_variant> achievement_tracker::current_values() const
{
    std::vector<cata_variant> result;
    result.reserve( watchers_.size() );
    for( const std::unique_ptr<requirement_watcher> &watcher : watchers_ ) {
        result.push_back( watcher->current_value() );
    }
    return result;
}

std::string achievement_tracker::ui_text() const
{
    // Determine overall achievement status
    if( time_is_expired() ) {
        return achievement_state{
            achievement_completion::failed,
            achievement_->time_constraint()->target(),
            current_values()
        }. ui_text( achievement_ );
    }

    // First: the achievement name and description
    nc_color c = color_from_completion( achievement_completion::pending );
    std::string result = colorize( achievement_->name(), c ) + "\n";
    if( !achievement_->description().empty() ) {
        result += "  " + colorize( achievement_->description(), c ) + "\n";
    }

    // Next: the time constraint
    if( achievement_->time_constraint() ) {
        result += "  " + achievement_->time_constraint()->ui_text() + "\n";
    }

    // Next: the requirements
    for( const std::unique_ptr<requirement_watcher> &watcher : watchers_ ) {
        result += "  " + watcher->ui_text() + "\n";
    }

    return result;
}

achievements_tracker::achievements_tracker(
    stats_tracker &stats,
    const std::function<void( const achievement * )> &achievement_attained_callback ) :
    stats_( &stats ),
    achievement_attained_callback_( achievement_attained_callback )
{}

achievements_tracker::~achievements_tracker() = default;

std::vector<const achievement *> achievements_tracker::valid_achievements() const
{
    std::vector<const achievement *> result;
    for( const achievement &ach : achievement::get_all() ) {
        if( initial_achievements_.count( ach.id ) ) {
            result.push_back( &ach );
        }
    }
    return result;
}

void achievements_tracker::report_achievement( const achievement *a, achievement_completion comp )
{
    assert( comp != achievement_completion::pending );
    assert( !achievements_status_.count( a->id ) );

    auto tracker_it = trackers_.find( a->id );
    achievements_status_.emplace(
        a->id,
    achievement_state{
        comp,
        calendar::turn,
        tracker_it->second.current_values()
    }
    );
    if( comp == achievement_completion::completed ) {
        achievement_attained_callback_( a );
    }
    trackers_.erase( tracker_it );
}

achievement_completion achievements_tracker::is_completed( const string_id<achievement> &id ) const
{
    auto it = achievements_status_.find( id );
    if( it == achievements_status_.end() ) {
        // It might still have failed; check for time expiry
        auto tracker_it = trackers_.find( id );
        if( tracker_it != trackers_.end() && tracker_it->second.time_is_expired() ) {
            return achievement_completion::failed;
        }
        return achievement_completion::pending;
    }
    return it->second.completion;
}

bool achievements_tracker::is_hidden( const achievement *ach ) const
{
    if( is_completed( ach->id ) == achievement_completion::completed ) {
        return false;
    }

    for( const string_id<achievement> &hidden_by : ach->hidden_by() ) {
        if( is_completed( hidden_by ) != achievement_completion::completed ) {
            return true;
        }
    }
    return false;
}

std::string achievements_tracker::ui_text_for( const achievement *ach ) const
{
    auto state_it = achievements_status_.find( ach->id );
    if( state_it != achievements_status_.end() ) {
        return state_it->second.ui_text( ach );
    }
    auto tracker_it = trackers_.find( ach->id );
    if( tracker_it == trackers_.end() ) {
        return colorize( ach->description() + _( "\nInternal error: achievement lacks watcher." ),
                         c_red );
    }
    return tracker_it->second.ui_text();
}

void achievements_tracker::clear()
{
    trackers_.clear();
    initial_achievements_.clear();
    achievements_status_.clear();
}

void achievements_tracker::notify( const cata::event &e )
{
    if( e.type() == event_type::game_start ) {
        assert( initial_achievements_.empty() );
        for( const achievement &ach : achievement::get_all() ) {
            initial_achievements_.insert( ach.id );
        }
        init_watchers();
    }
}

void achievements_tracker::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "initial_achievements", initial_achievements_ );
    jsout.member( "achievements_status", achievements_status_ );
    jsout.end_object();
}

void achievements_tracker::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();
    jo.read( "initial_achievements", initial_achievements_ );
    jo.read( "achievements_status", achievements_status_ );

    init_watchers();
}

void achievements_tracker::init_watchers()
{
    for( const achievement *a : valid_achievements() ) {
        if( achievements_status_.count( a->id ) ) {
            continue;
        }
        trackers_.emplace(
            std::piecewise_construct, std::forward_as_tuple( a->id ),
            std::forward_as_tuple( *a, *this, *stats_ ) );
    }
}
