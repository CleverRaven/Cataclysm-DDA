#include "stats_tracker.h"

#include "event_statistics.h"

static bool event_data_matches( const cata::event::data_type &data,
                                const cata::event::data_type &criteria )
{
    for( const auto &criterion : criteria ) {
        auto it = data.find( criterion.first );
        if( it == data.end() || it->second != criterion.second ) {
            return false;
        }
    }
    return true;
}

void event_multiset::set_type( event_type type )
{
    // Used during stats_tracker deserialization to set the type
    assert( type_ == event_type::num_event_types );
    type_ = type;
}

int event_multiset::count() const
{
    int total = 0;
    for( const auto &pair : counts_ ) {
        total += pair.second;
    }
    return total;
}

int event_multiset::count( const cata::event::data_type &criteria ) const
{
    int total = 0;
    for( const auto &pair : counts_ ) {
        if( event_data_matches( pair.first, criteria ) ) {
            total += pair.second;
        }
    }
    return total;
}

int event_multiset::total( const std::string &field ) const
{
    return total( field, {} );
}

int event_multiset::total( const std::string &field, const cata::event::data_type &criteria ) const
{
    int total = 0;
    for( const auto &pair : counts_ ) {
        auto it = pair.first.find( field );
        if( it == pair.first.end() ) {
            continue;
        }
        if( event_data_matches( pair.first, criteria ) ) {
            total += pair.second * it->second.get<cata_variant_type::int_>();
        }
    }
    return total;
}

void event_multiset::add( const cata::event &e )
{
    counts_[e.data()]++;
}

stat_watcher::~stat_watcher() = default;
event_multiset_watcher::~event_multiset_watcher() = default;
stats_tracker_state::~stats_tracker_state() = default;

void event_multiset::add( const counts_type::value_type &e )
{
    counts_[e.first] += e.second;
}

event_multiset &stats_tracker::get_events( event_type type )
{
    return data.emplace( type, event_multiset( type ) ).first->second;
}

event_multiset stats_tracker::get_events(
    const string_id<event_transformation> &transform_id )
{
    return transform_id->value( *this );
}

cata_variant stats_tracker::value_of( const string_id<event_statistic> &stat )
{
    return stat->value( *this );
}

void stats_tracker::add_watcher( event_type type, event_multiset_watcher *watcher )
{
    event_type_watchers[type].push_back( watcher );
}

void stats_tracker::add_watcher( const string_id<event_transformation> &id,
                                 event_multiset_watcher *watcher )
{
    event_transformation_watchers[id].push_back( watcher );
    std::unique_ptr<stats_tracker_state> &state = event_transformation_states[ id ];
    if( !state ) {
        state = id->watch( *this );
    }
}

void stats_tracker::add_watcher( const string_id<event_statistic> &id, stat_watcher *watcher )
{
    stat_watchers[id].push_back( watcher );
    std::unique_ptr<stats_tracker_state> &state = stat_states[ id ];
    if( !state ) {
        state = id->watch( *this );
    }
}

void stats_tracker::transformed_set_changed( const string_id<event_transformation> &id,
        const cata::event &new_element )
{
    auto it = event_transformation_watchers.find( id );
    if( it != event_transformation_watchers.end() ) {
        for( event_multiset_watcher *watcher : it->second ) {
            watcher->event_added( new_element, *this );
        }
    }
}

void stats_tracker::transformed_set_changed( const string_id<event_transformation> &id,
        const event_multiset &new_value )
{
    auto it = event_transformation_watchers.find( id );
    if( it != event_transformation_watchers.end() ) {
        for( event_multiset_watcher *watcher : it->second ) {
            watcher->events_reset( new_value, *this );
        }
    }
}

void stats_tracker::stat_value_changed( const string_id<event_statistic> &id,
                                        const cata_variant &new_value )
{
    auto it = stat_watchers.find( id );
    if( it != stat_watchers.end() ) {
        for( stat_watcher *watcher : it->second ) {
            watcher->new_value( new_value, *this );
        }
    }
}

std::vector<const score *> stats_tracker::valid_scores() const
{
    std::vector<const score *> result;
    for( const score &scr : score::get_all() ) {
        if( initial_scores.count( scr.id ) ) {
            result.push_back( &scr );
        }
    }
    return result;
}

void stats_tracker::clear()
{
    data.clear();
}

void stats_tracker::notify( const cata::event &e )
{
    const event_type type = e.type();
    get_events( type ).add( e );

    auto it = event_type_watchers.find( type );
    if( it != event_type_watchers.end() ) {
        for( event_multiset_watcher *watcher : it->second ) {
            watcher->event_added( e, *this );
        }
    }

    if( e.type() == event_type::game_start ) {
        for( const score &scr : score::get_all() ) {
            initial_scores.insert( scr.id );
        }
    }
}
