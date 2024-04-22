#pragma once
#ifndef CATA_SRC_PROFICIENCY_H
#define CATA_SRC_PROFICIENCY_H

#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <vector>
#include <string>

#include "calendar.h"
#include "color.h"
#include "flat_set.h"
#include "translations.h"
#include "type_id.h"

#include "mod_tracker.h"

class JsonArray;
class JsonObject;
class JsonOut;
struct display_proficiency;
struct learning_proficiency;
template<typename E> struct enum_traits;
template<typename T>
class generic_factory;
class Character;

enum class proficiency_bonus_type : int {
    strength,
    dexterity,
    intelligence,
    perception,
    stamina,
    last
};

template<>
struct enum_traits<proficiency_bonus_type> {
    static constexpr proficiency_bonus_type last = proficiency_bonus_type::last;
};

struct proficiency_bonus {
    proficiency_bonus_type type = proficiency_bonus_type::last;
    float value = 0;

    void deserialize( const JsonObject &jo );
};

struct proficiency_category {
    proficiency_category_id id;
    translation _name;
    translation _description;
    bool was_loaded = false;

    static void load_proficiency_categories( const JsonObject &jo, const std::string &src );
    static void reset();
    void load( const JsonObject &jo, std::string_view src );
    static const std::vector<proficiency_category> &get_all();
};

class proficiency
{
        friend class generic_factory<proficiency>;
        friend struct mod_tracker;

        proficiency_id id;
        proficiency_category_id _category;
        std::vector<std::pair<proficiency_id, mod_id>> src;
        bool was_loaded = false;

        bool _can_learn = false;
        bool _ignore_focus = false;
        bool _teachable = true;

        translation _name;
        translation _description;

        float _default_time_multiplier = 2.0f;
        float _default_skill_penalty = 1.0f;

        float _default_weakpoint_bonus = 0.0f;
        float _default_weakpoint_penalty = 0.0f;

        time_duration _time_to_learn = 9999_hours;
        std::set<proficiency_id> _required;

        std::map<std::string, std::vector<proficiency_bonus>> _bonuses;

    public:
        static void load_proficiencies( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, std::string_view src );

        static const std::vector<proficiency> &get_all();

        bool can_learn() const;
        bool ignore_focus() const;
        bool is_teachable() const;
        proficiency_id prof_id() const;
        proficiency_category_id prof_category() const;
        std::string name() const;
        std::string description() const;

        float default_time_multiplier() const;
        float default_skill_penalty() const;

        float default_weakpoint_bonus() const;
        float default_weakpoint_penalty() const;

        time_duration time_to_learn() const;
        std::set<proficiency_id> required_proficiencies() const;

        std::vector<proficiency_bonus> get_bonuses( const std::string &category ) const;
        std::optional<float> bonus_for( const std::string &category, proficiency_bonus_type type ) const;
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
                       float remainder, const std::optional<time_duration> &max );
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
        time_duration pct_practiced_time( const proficiency_id &query ) const;
        void set_time_practiced( const proficiency_id &practicing, const time_duration &amount );
        time_duration training_time_needed( const proficiency_id &query ) const;
        std::vector<proficiency_id> known_profs() const;
        std::vector<proficiency_id> learning_profs() const;

        float get_proficiency_bonus( const std::string &category,
                                     proficiency_bonus_type prof_bonus ) const;

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jsobj );
};

struct learning_proficiency {
    proficiency_id id;

    // How long we have practiced this proficiency
    time_duration practiced;
    // Rounding errors of seconds, so that proficiencies practiced very briefly don't get truncated
    float remainder = 0.f;

    learning_proficiency() = default;
    learning_proficiency( const proficiency_id &id, const time_duration &practiced ) : id( id ),
        practiced( practiced ) {}

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonObject &jo );
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
        void deserialize( const JsonObject &jo );

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

void show_proficiencies_window( const Character &u,
                                std::optional<proficiency_id> default_selection = std::nullopt );

#endif // CATA_SRC_PROFICIENCY_H
