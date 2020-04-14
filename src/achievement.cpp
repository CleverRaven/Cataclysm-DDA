#include "achievement.h"

#include "avatar.h"
#include "event_statistics.h"
#include "generic_factory.h"
#include "stats_tracker.h"

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

enum class achievement_comparison {
    greater_equal,
    last,
};

namespace io
{

template<>
std::string enum_to_string<achievement_comparison>( achievement_comparison data )
{
    switch( data ) {
        // *INDENT-OFF*
        case achievement_comparison::greater_equal: return ">=";
        // *INDENT-ON*
        case achievement_comparison::last:
            break;
    }
    debugmsg( "Invalid achievement_comparison" );
    abort();
}

} // namespace io

template<>
struct enum_traits<achievement_comparison> {
    static constexpr achievement_comparison last = achievement_comparison::last;
};

struct achievement_requirement {
    string_id<event_statistic> statistic;
    achievement_comparison comparison;
    int target;

    void deserialize( JsonIn &jin ) {
        const JsonObject &jo = jin.get_object();
        if( !( jo.read( "event_statistic", statistic ) &&
               jo.read( "is", comparison ) &&
               jo.read( "target", target ) ) ) {
            jo.throw_error( "Mandatory field missing for achievement requirement" );
        }
    }

    void check( const string_id<achievement> &id ) const {
        if( !statistic.is_valid() ) {
            debugmsg( "score %s refers to invalid statistic %s", id.str(), statistic.str() );
        }
    }

    bool satisifed_by( const cata_variant &v ) const {
        int value = v.get<int>();
        switch( comparison ) {
            case achievement_comparison::greater_equal:
                return value >= target;
            case achievement_comparison::last:
                break;
        }
        debugmsg( "Invalid achievement_requirement comparison value" );
        abort();
    }
};


void achievement::load_achievement( const JsonObject &jo, const std::string &src )
{
    achievement_factory.load( jo, src );
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
    mandatory( jo, was_loaded, "description", description_ );
    mandatory( jo, was_loaded, "requirements", requirements_ );
}

void achievement::check() const
{
    for( const achievement_requirement &req : requirements_ ) {
        req.check( id );
    }
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

        void new_value( const cata_variant &new_value, stats_tracker & ) override;

        bool is_satisfied( stats_tracker &stats ) {
            return requirement_->satisifed_by( requirement_->statistic->value( stats ) );
        }

        std::string ui_text() const {
            bool is_satisfied = requirement_->satisifed_by( current_value_ );
            nc_color c = is_satisfied ? c_green : c_yellow;
            int current = current_value_.get<int>();
            std::string result = string_format( _( "%s/%s " ), current, requirement_->target );
            result += requirement_->statistic->description();
            return colorize( result, c );
        }
    private:
        cata_variant current_value_;
        achievement_tracker *tracker_;
        const achievement_requirement *requirement_;
};

void requirement_watcher::new_value( const cata_variant &new_value, stats_tracker & )
{
    current_value_ = new_value;
    tracker_->set_requirement( this, requirement_->satisifed_by( new_value ) );
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
        // *INDENT-ON*
        case achievement_completion::last:
            break;
    }
    debugmsg( "Invalid achievement_completion" );
    abort();
}

} // namespace io

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
    if( !sorted_watchers_[is_satisfied].insert( watcher ).second ) {
        // No change
        return;
    }

    // Remove from other; check for completion.
    sorted_watchers_[!is_satisfied].erase( watcher );
    assert( sorted_watchers_[0].size() + sorted_watchers_[1].size() == watchers_.size() );

    if( sorted_watchers_[false].empty() ) {
        tracker_->report_achievement( achievement_, achievement_completion::completed );
    }
}

std::string achievement_tracker::ui_text( const achievement_state *state ) const
{
    auto color_from_completion = []( achievement_completion comp ) {
        switch( comp ) {
            case achievement_completion::pending:
                return c_yellow;
            case achievement_completion::completed:
                return c_light_green;
            case achievement_completion::last:
                break;
        }
        debugmsg( "Invalid achievement_completion" );
        abort();
    };

    achievement_completion comp = state ? state->completion : achievement_completion::pending;
    nc_color c = color_from_completion( comp );
    std::string result = colorize( achievement_->description(), c ) + "\n";
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
    auto it = achievements_status_.find( a->id );
    achievement_completion existing_comp =
        ( it == achievements_status_.end() ) ? achievement_completion::pending
        : it->second.completion;
    if( existing_comp == comp ) {
        return;
    }
    achievement_state new_state{
        comp,
        calendar::turn
    };
    if( it == achievements_status_.end() ) {
        achievements_status_.emplace( a->id, new_state );
    } else {
        it->second = new_state;
    }
    if( comp == achievement_completion::completed ) {
        achievement_attained_callback_( a );
    }
}

achievement_completion achievements_tracker::is_completed( const string_id<achievement> &id ) const
{
    auto it = achievements_status_.find( id );
    if( it == achievements_status_.end() ) {
        return achievement_completion::pending;
    }
    return it->second.completion;
}

std::string achievements_tracker::ui_text_for( const achievement *ach ) const
{
    auto state_it = achievements_status_.find( ach->id );
    const achievement_state *state = nullptr;
    if( state_it != achievements_status_.end() ) {
        state = &state_it->second;
    }
    auto watcher_it = watchers_.find( ach->id );
    if( watcher_it == watchers_.end() ) {
        return colorize( ach->description() + _( "\nInternal error: achievement lacks watcher." ),
                         c_red );
    }
    return watcher_it->second.ui_text( state );
}

void achievements_tracker::clear()
{
    watchers_.clear();
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
        watchers_.emplace(
            std::piecewise_construct, std::forward_as_tuple( a->id ),
            std::forward_as_tuple( *a, *this, *stats_ ) );
    }
}
