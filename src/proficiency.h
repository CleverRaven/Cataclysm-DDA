#pragma once
#ifndef CATA_SRC_PROFICIENCY_H
#define CATA_SRC_PROFICIENCY_H

#include <iosfwd>
#include <set>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "flat_set.h"
#include "optional.h"
#include "translations.h"
#include "type_id.h"

class JsonArray;
class JsonIn;
class JsonObject;
class JsonOut;
struct display_proficiency;
struct learning_proficiency;
template<typename T>
class generic_factory;

class proficiency
{
        friend class generic_factory<proficiency>;

        proficiency_id id;
        bool was_loaded = false;

        bool _can_learn = false;

        translation _name;
        translation _description;

        float _default_time_multiplier = 2.0f;
        float _default_fail_multiplier = 2.0f;

        time_duration _time_to_learn = 9999_hours;
        std::set<proficiency_id> _required;

    public:
        static void load_proficiencies( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, const std::string &src );
        static const std::vector<proficiency> &get_all();

        bool can_learn() const;
        proficiency_id prof_id() const;
        std::string name() const;
        std::string description() const;

        float default_time_multiplier() const;
        float default_fail_multiplier() const;

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
        bool practice( const proficiency_id &practicing, const time_duration &amount,
                       const cata::optional<time_duration> &max );
        void learn( const proficiency_id &learned );
        void remove( const proficiency_id &lost );

        // Ignore requirements, made for debugging
        void direct_learn( const proficiency_id &learned );
        void direct_remove( const proficiency_id &lost );

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
    learning_proficiency( const proficiency_id &id, const time_duration &practiced ) : id( id ),
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
    float practice = 0.0f;

    // How much time we've spent practicing it
    time_duration spent = 0_turns;

    // If we already know it
    bool known = false;
};

// a class for having bonuses from books instead of proficiencies you know
struct book_proficiency_bonus {
        proficiency_id id;
        float time_factor = default_time_factor;
        float fail_factor = default_fail_factor;
        bool include_prereqs = default_include_prereqs;

        bool was_loaded = false;
        void deserialize( JsonIn &jsin );

    private:
        static const float default_time_factor;
        static const float default_fail_factor;
        static const bool default_include_prereqs;
};

// a container class for book_proficiency_bonus to make it easy to calculate and compartmentalize
class book_proficiency_bonuses
{
    private:
        std::vector<book_proficiency_bonus> bonuses;
        // the inner part of the add function for recursion
        void add( const book_proficiency_bonus &bonus, std::set<proficiency_id> &already_included );
    public:
        void add( const book_proficiency_bonus &bonus );
        book_proficiency_bonuses &operator+=( const book_proficiency_bonuses &rhs );
        // adjustment to the crafting failure malus when missing the proficiency, ranging from 0
        // (no mitigation) to 1 (full mitigation)
        float fail_factor( const proficiency_id &id ) const;
        // adjustment to the crafting time malus when missing the proficiency, ranging from 0
        // (no mitigation) to 1 (full mitigation)
        float time_factor( const proficiency_id &id ) const;
};

#endif // CATA_SRC_PROFICIENCY_H
