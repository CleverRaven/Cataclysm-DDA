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

void event_multiset::add( const counts_type::value_type &e )
{
    counts_[e.first] += e.second;
}

int stats_tracker::count( const cata::event &e ) const
{
    return count( e.type(), e.data() );
}

int stats_tracker::count( event_type type, const cata::event::data_type &criteria ) const
{
    auto it = data.find( type );
    if( it == data.end() ) {
        return 0;
    }
    return it->second.count( criteria );
}

event_multiset &stats_tracker::get_events( event_type type )
{
    return data.emplace( type, event_multiset( type ) ).first->second;
}

event_multiset stats_tracker::get_events(
    const string_id<event_transformation> &transform_id )
{
    return transform_id->initialize( *this );
}

int stats_tracker::total( event_type type, const std::string &field,
                          const cata::event::data_type &criteria ) const
{
    auto it = data.find( type );
    if( it == data.end() ) {
        return 0;
    }
    return it->second.total( field, criteria );
}

cata_variant stats_tracker::value_of( const string_id<event_statistic> &stat )
{
    return stat->value( *this );
}

void stats_tracker::clear()
{
    data.clear();
}

void stats_tracker::notify( const cata::event &e )
{
    get_events( e.type() ).add( e );
}
