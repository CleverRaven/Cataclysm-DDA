#ifndef CATA_SRC_STATS_TRACKER_H
#define CATA_SRC_STATS_TRACKER_H

#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cata_variant.h"
#include "event.h"
#include "event_bus.h"
#include "hash_utils.h"
#include "string_id.h"

class JsonIn;
class JsonOut;
class event_statistic;
class event_transformation;
enum class monotonically : int;
class score;
class stats_tracker;

// The stats_tracker is intended to keep a summary of events that have occured.
// For each event_type it stores an event_multiset.
// Within the event_tracker, the events are partitioned according to their data
// (an event::data_type object, which is a map of keys to values).
// Within each partition, an event_summary is stored, which contains the first
// and last times such events were seen, and the number of them seen.
// The stats_tracker can be queried in various ways to get summary statistics
// about events that have occured.

struct event_summary {
    event_summary();
    event_summary( int c, time_point f, time_point l );

    int count;
    time_point first;
    time_point last;

    void add( const cata::event & );
    void add( const event_summary & );

    void serialize( JsonOut & ) const;
    void deserialize( JsonIn & );
};

class event_multiset
{
    public:
        using summaries_type =
            std::unordered_map<cata::event::data_type, event_summary, cata::range_hash>;

        // Default constructor for deserialization deliberately uses invalid
        // type
        event_multiset() : type_( event_type::num_event_types ) {}
        event_multiset( event_type type ) : type_( type ) {}

        void set_type( event_type );

        const summaries_type &counts() const {
            return summaries_;
        }

        // count returns the number of events matching given criteria that have
        // occured.
        // total returns the sum of some integer-valued field across every
        // event satisfying certain criteria.
        // maximum and minimum return the max and min respectively of some
        // integer-valued field across every event satisfying certain criteria.
        // For example, count might return the number of times the avatar has
        // taken damage, total might return the total damage taken in all
        // those cases, and maximum might return the single largest amount
        // of damage taken in a single event across all those cases.
        // The criteria takes the form of an event::data_type map specifying
        // some values that must be matched in the events of that type.  You can
        // provide just a subset of the relevant keys from the event_type in
        // your criteria.
        int count() const;
        int count( const cata::event::data_type &criteria ) const;
        int total( const std::string &field ) const;
        int total( const std::string &field, const cata::event::data_type &criteria ) const;
        int minimum( const std::string &field ) const;
        int maximum( const std::string &field ) const;
        cata::optional<summaries_type::value_type> first() const;
        cata::optional<summaries_type::value_type> last() const;

        void add( const cata::event & );
        void add( const summaries_type::value_type & );

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );
    private:
        event_type type_;
        summaries_type summaries_;
};

class base_watcher
{
    public:
        base_watcher() = default;
        base_watcher( const base_watcher & ) = delete;
        base_watcher &operator=( const base_watcher & ) = delete;
    protected:
        virtual ~base_watcher();
    private:
        friend class stats_tracker;
        void on_subscribe( stats_tracker * );
        void on_unsubscribe( stats_tracker * );
        stats_tracker *subscribed_to = nullptr;
};

class stat_watcher : public base_watcher
{
    public:
        virtual void new_value( const cata_variant &, stats_tracker & ) = 0;
};

class event_multiset_watcher : public base_watcher
{
    public:
        virtual void event_added( const cata::event &, stats_tracker & ) = 0;
        virtual void events_reset( const event_multiset &, stats_tracker & ) = 0;
};

template<typename Watcher>
class watcher_set
{
        static_assert( std::is_base_of<base_watcher, Watcher>::value,
                       "Watcher must be derived from base_watcher" );
    public:
        void insert( Watcher *watcher ) {
            watchers_.insert( watcher );
        }

        bool erase( base_watcher *watcher ) {
            return watchers_.erase( watcher );
        }

        template<typename Class, typename... FnArgs, typename... Args>
        void send_to_all( void ( Class::*mem_fn )( FnArgs... ), Args &&... args ) const {
            static_assert( std::is_base_of<Class, Watcher>::value,
                           "Watcher must be derived from Class" );
            // Sending an event to a watcher can cause it to be erased, so we
            // need to always ensure we have the next iterator prepared in
            // advance.
            auto current = watchers_.begin();

            while( current != watchers_.end() ) {
                auto next = current;
                ++next;
                ( static_cast<Watcher *>( *current )->*mem_fn )( args... );
                current = next;
            }
        }
    private:
        std::set<base_watcher *> watchers_;
};

class stats_tracker_state
{
    public:
        virtual ~stats_tracker_state() = 0;
};

class stats_tracker : public event_subscriber
{
    public:
        ~stats_tracker() override;

        event_multiset &get_events( event_type );
        event_multiset get_events( const string_id<event_transformation> & );

        cata_variant value_of( const string_id<event_statistic> & );

        void add_watcher( event_type, event_multiset_watcher * );
        void add_watcher( const string_id<event_transformation> &, event_multiset_watcher * );
        void add_watcher( const string_id<event_statistic> &, stat_watcher * );

        void unwatch( base_watcher * );

        void transformed_set_changed( const string_id<event_transformation> &,
                                      const cata::event &new_element );
        void transformed_set_changed( const string_id<event_transformation> &,
                                      const event_multiset &new_value );
        void stat_value_changed( const string_id<event_statistic> &,
                                 const cata_variant &new_value );

        // Return all scores which are valid now and existed at game start
        std::vector<const score *> valid_scores() const;

        void clear();
        void notify( const cata::event & ) override;

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );
    private:
        void unwatch_all();

        std::unordered_map<event_type, event_multiset> data;

        std::unordered_map<event_type, watcher_set<event_multiset_watcher>> event_type_watchers;
        std::unordered_map<string_id<event_transformation>, watcher_set<event_multiset_watcher>>
                event_transformation_watchers;
        std::unordered_map<string_id<event_statistic>, watcher_set<stat_watcher>> stat_watchers;
        std::unordered_map<string_id<event_transformation>, std::unique_ptr<stats_tracker_state>>
                event_transformation_states;
        std::unordered_map<string_id<event_statistic>, std::unique_ptr<stats_tracker_state>>
                stat_states;

        std::unordered_set<string_id<score>> initial_scores;
};

#endif // CATA_SRC_STATS_TRACKER_H
