#ifndef CATA_STATS_TRACKER_H
#define CATA_STATS_TRACKER_H

#include "event_bus.h"
#include "hash_utils.h"

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
