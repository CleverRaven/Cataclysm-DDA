#include "stats_tracker.h"

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

int event_tracker::count( const cata::event::data_type &criteria ) const
{
    int total = 0;
    for( const auto &pair : event_counts ) {
        if( event_data_matches( pair.first, criteria ) ) {
            total += pair.second;
        }
    }
    return total;
}

int event_tracker::total( const std::string &field, const cata::event::data_type &criteria ) const
{
    int total = 0;
    for( const auto &pair : event_counts ) {
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

void event_tracker::add( const cata::event &e )
{
    event_counts[e.data()]++;
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

int stats_tracker::total( event_type type, const std::string &field,
                          const cata::event::data_type &criteria ) const
{
    auto it = data.find( type );
    if( it == data.end() ) {
        return 0;
    }
    return it->second.total( field, criteria );
}

void stats_tracker::clear()
{
    data.clear();
}

void stats_tracker::notify( const cata::event &e )
{
    data[e.type()].add( e );
}
