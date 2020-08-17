#pragma once
#ifndef CATA_SRC_PROFICIENCY_H
#define CATA_SRC_PROFICIENCY_H

#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "flat_set.h"
#include "json.h"
#include "optional.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"

struct display_proficiency;
struct learning_proficiency;
template<typename T>
class generic_factory;

class proficiency
{
        friend class generic_factory<proficiency>;

        proficiency_id id;
        bool was_loaded = false;

        bool _can_learn;

        translation _name;
        translation _description;

        time_duration _time_to_learn = 9999_hours;
        std::set<proficiency_id> _required;

    public:
        static void load_proficiencies( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, const std::string &src );

        bool can_learn() const;
        std::string name() const;
        std::string description() const;
        time_duration time_to_learn() const;
        std::set<proficiency_id> required_proficiencies() const;
};

// The proficiencies you know, and the ones you're learning.
class proficiency_set
{
        // No need for extra info, we just know these proficiencies
        cata::flat_set<proficiency_id> known;

        // The proficiencies we're learning
        std::vector<learning_proficiency> learning;

        learning_proficiency &fetch_learning( const proficiency_id &target );

    public:
        std::vector<display_proficiency> display() const;
        // True if the proficiency is learned;
        bool practice( const proficiency_id &practicing, time_duration amount,
                       cata::optional<time_duration> max );
        void learn( const proficiency_id &learned );
        void remove( const proficiency_id &lost );

        // Do we know this proficiency?
        bool has_learned( const proficiency_id &query ) const;
        bool has_practiced( const proficiency_id &query ) const;
        bool has_prereqs( const proficiency_id &query ) const;

        float pct_practiced( const proficiency_id &query ) const;
        time_duration training_time_needed( const proficiency_id &query ) const;
        std::vector<proficiency_id> known_profs() const;
        std::vector<proficiency_id> learning_profs() const;

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );
        void deserialize_legacy( const JsonArray &jo );
};

struct learning_proficiency {
    proficiency_id id;

    // How long we have practiced this proficiency
    time_duration practiced;

    learning_proficiency() = default;
    learning_proficiency( const proficiency_id &id, const time_duration practiced ) : id( id ),
        practiced( practiced ) {}

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct display_proficiency {
    // To grab the name, description, etc
    proficiency_id id;

    // White if we know it, gray if we don't
    nc_color color;

    // What percentage we are towards knowing it
    float practice;

    // How much time we've spent practicing it
    time_duration spent;

    // If we already know it
    bool known;
};

#endif // CATA_SRC_PROFICIENCY_H
