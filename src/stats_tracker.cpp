#include "stats_tracker.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <utility>

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

int event_multiset::minimum( const std::string &field ) const
{
    int minimum = 0;
    for( const auto &pair : counts_ ) {
        auto it = pair.first.find( field );
        if( it == pair.first.end() ) {
            continue;
        }
        const int potential = it->second.get<cata_variant_type::int_>();
        if( potential < minimum ) {
            minimum = potential;
        }
    }
    return minimum;
}

int event_multiset::maximum( const std::string &field ) const
{
    int maximum = 0;
    for( const auto &pair : counts_ ) {
        auto it = pair.first.find( field );
        if( it == pair.first.end() ) {
            continue;
        }
        const int potential = it->second.get<cata_variant_type::int_>();
        if( potential > maximum ) {
            maximum = potential;
        }
    }
    return maximum;
}

void event_multiset::add( const cata::event &e )
{
    counts_[e.data()]++;
}

void event_multiset::add( const counts_type::value_type &e )
{
    counts_[e.first] += e.second;
}

base_watcher::~base_watcher()
{
    if( subscribed_to ) {
        subscribed_to->unwatch( this );
    }
}

void base_watcher::on_subscribe( stats_tracker *s )
{
    if( subscribed_to ) {
        debugmsg( "Subscribing a single base_watcher multiple times is not supported" );
    }
    subscribed_to = s;
}

void base_watcher::on_unsubscribe( stats_tracker *s )
{
    if( subscribed_to != s ) {
        debugmsg( "Unexpected notification of unsubscription from wrong stats_tracker" );
    } else {
        subscribed_to = nullptr;
    }
}

stats_tracker_state::~stats_tracker_state() = default;

stats_tracker::~stats_tracker()
{
    unwatch_all();
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
    watcher->on_subscribe( this );
}

void stats_tracker::add_watcher( const string_id<event_transformation> &id,
                                 event_multiset_watcher *watcher )
{
    event_transformation_watchers[id].push_back( watcher );
    watcher->on_subscribe( this );
    std::unique_ptr<stats_tracker_state> &state = event_transformation_states[ id ];
    if( !state ) {
        state = id->watch( *this );
    }
}

void stats_tracker::add_watcher( const string_id<event_statistic> &id, stat_watcher *watcher )
{
    stat_watchers[id].push_back( watcher );
    watcher->on_subscribe( this );
    std::unique_ptr<stats_tracker_state> &state = stat_states[ id ];
    if( !state ) {
        state = id->watch( *this );
    }
}

void stats_tracker::unwatch( base_watcher *watcher )
{
    // Use a slow O(n) approach for now; if it proves problematic we can build
    // an index, but that seems over-complex.
    auto erase_from = [watcher]( auto & map_of_vectors ) {
        for( auto &p : map_of_vectors ) {
            auto &vector = p.second;
            auto it = std::find( vector.begin(), vector.end(), watcher );
            if( it != vector.end() ) {
                vector.erase( it );
                return true;
            }
        }
        return false;
    };


    if( erase_from( event_type_watchers ) ||
        erase_from( event_transformation_watchers ) ||
        erase_from( stat_watchers ) ) {
        return;
    }
    debugmsg( "unwatch for a watcher not found" );
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
    unwatch_all();
    data.clear();
    event_transformation_states.clear();
    stat_states.clear();
    initial_scores.clear();
}

void stats_tracker::unwatch_all()
{
    auto unsub_all = [&]( auto & map_of_vectors ) {
        for( auto const &p : map_of_vectors ) {
            const auto &vector = p.second;
            for( base_watcher *watcher : vector ) {
                watcher->on_unsubscribe( this );
            }
        }
        map_of_vectors.clear();
    };
    unsub_all( event_type_watchers );
    unsub_all( event_transformation_watchers );
    unsub_all( stat_watchers );
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
        assert( initial_scores.empty() );
        for( const score &scr : score::get_all() ) {
            initial_scores.insert( scr.id );
        }
    }
}
