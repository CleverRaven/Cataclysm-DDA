#ifndef CATA_STATS_TRACKER_H
#define CATA_STATS_TRACKER_H

#include "event_bus.h"
#include "hash_utils.h"

// The stats_tracker is intended to keep a summary of events that have occured.
// For each event_type it stores an event_tracker.
// Within the event_tracker, counts are kept.  The events are partitioned
// according to their data (an event::data_type object, which is a map of keys
// to values).
// The stats_tracker can be queried in various ways to get summary statistics
// about events that have occured.

class event_tracker
{
    public:
        int count( const cata::event::data_type &criteria ) const;
        int total( const std::string &field, const cata::event::data_type &criteria ) const;

        void add( const cata::event & );

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );
    private:
        std::unordered_map<cata::event::data_type, int, cata::range_hash> event_counts;
};

class stats_tracker : public event_subscriber
{
    public:
        // count returns the number of events matching given criteria that have
        // occured.
        // total returns the sum of some integer-valued field across every
        // event satisfying certain criteria.
        // For example, count might return the number of times the avatar has
        // taken damage, while total might return the total damage taken in all
        // those cases.
        // The criteria have two parts:
        // - The event_type
        // - An event::data_type map specifying some values that must be
        //   matched in the events of that type.  You can provide just a subset
        //   of the relevant keys from the event_type in your criteria.
        // The first count overload combines these criteria into a single event
        // object for convenience since that contains the two pieces necessary.
        int count( const cata::event & ) const;
        int count( event_type, const cata::event::data_type &criteria ) const;
        int total( event_type, const std::string &field,
                   const cata::event::data_type &criteria ) const;

        void clear();
        void notify( const cata::event & ) override;

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );
    private:
        std::unordered_map<event_type, event_tracker> data;
};

#endif // CATA_STATS_TRACKER_H
