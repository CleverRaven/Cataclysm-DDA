#pragma once
#ifndef CATA_EVENT_STATISTICS_H
#define CATA_EVENT_STATISTICS_H

#include <map>
#include <memory>
#include <vector>

#include "clone_ptr.h"
#include "string_id.h"

class cata_variant;
namespace cata
{
class event;
} // namespace cata
class event_multiset;
enum class event_type : int;
class JsonObject;
class stats_tracker;

// A transformation from one multiset of events to another
class event_transformation
{
    public:
        event_multiset initialize( stats_tracker & ) const;

        void load( JsonObject &, const std::string & );
        void check() const;
        static void load_transformation( JsonObject &, const std::string & );
        static void check_consistency();
        static void reset();

        string_id<event_transformation> id;
        bool was_loaded = false;

        class impl;
    private:
        cata::clone_ptr<impl> impl_;
};

// A value computed from events somehow
class event_statistic
{
    public:
        cata_variant value( stats_tracker & ) const;

        void load( JsonObject &, const std::string & );
        void check() const;
        static void load_statistic( JsonObject &, const std::string & );
        static void check_consistency();
        static void reset();

        string_id<event_statistic> id;
        bool was_loaded = false;

        class impl;
    private:
        cata::clone_ptr<impl> impl_;
};

class score
{
    public:
        score() = default;
        // Returns translated description including value
        std::string description( stats_tracker & ) const;
        cata_variant value( stats_tracker & ) const;

        void load( JsonObject &, const std::string & );
        void check() const;
        static void load_score( JsonObject &, const std::string & );
        static void check_consistency();
        static const std::vector<score> &get_all();
        static void reset();

        string_id<score> id;
        bool was_loaded = false;
    private:
        std::string description_;
        string_id<event_statistic> stat_;
};

#endif // CATA_EVENT_STATISTICS_H
