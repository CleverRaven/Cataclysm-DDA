#pragma once
#ifndef CATA_SRC_WOUND_H
#define CATA_SRC_WOUND_H

#include <optional>
#include <vector>

#include "body_part_set.h"
#include "calendar.h"
#include "damage.h"
#include "string_id.h"
#include "translation.h"
#include "units.h"

class body_part_set;
class damage_instance;
class JsonObject;
class wound_type;

enum class damage_type;

using wound_id = string_id<wound_type>;

// this is the json object, so it needs to all be public
class wound_type
{
    public:
        wound_type() = default;

        void load( const JsonObject &jo, const std::string & );
        static void load_wound( const JsonObject &jo, const std::string &src );

        static const std::vector<wound_type> &get_all();
        static void finalize();
        static std::map<damage_type, std::vector<wound_id>> wound_lookup;

        wound_id id;
        bool was_loaded;

        translation name;
        translation description;

        // this is a representation of how often this wound would be damaged by attacks, and how they overlap
        // 0-100
        int size;
        int pain;
        // this is basically negative armor, effectively giving you worse wounds with the same amount of damage
        int sunder;
        int min_damage = 1;
        int max_damage = INT_MAX;
        damage_type dmg_type;

        body_part_set limbs;
        // the amount of bleeding per second
        units::volume bleed;
        // how long it takes for this wound to heal.
        std::optional<time_duration> heal_time;
        // transforms into this wound once age has reached heal_time
        std::optional<wound_id> heals_into;
        // transforms into this wound once infection reaches 1.0 (100%)
        std::optional<wound_id> infects_into;
};

// this is the mutable object in code that points to the wound_type
class wound
{
    private:
        wound_id id;
        // represents the location of the wound and how they overlap
        // 0-100
        int location;
        // multiplies various wound effects
        double intensity_multiplier;
        // how old the wound is. if older than heal_time, turns this wound into the next one.
        time_duration age;
        // the percentage to full infection this wound is at. ticks up based on how dirty it is and perhaps other factors.
        double infection;
        // the multiplier for normal infection rate
        double contamination = 1.0;
    public:
        wound() = default;
        wound( const damage_unit &damage );
        // if true, this wound is ready to be removed.
        bool process( const time_duration &t, double healing_factor );
        // value that represents the wound progression to healed status. the wound is healed at 1.0
        double wound_progression() const;
        // value that represents infection progression. the wound is infected at 1.0
        double infection_progression() const;

        bool is_infected() const;
        bool overlaps( int location ) const;

        std::optional<wound_id> heals_into() const;
        std::optional<wound_id> infects_into() const;

        // creates a new version of this wound, with inherited values
        // will error if there is no healed version. make sure to check first.
        wound wound_healed() const;
        wound wound_infected() const;

        int sunder() const;
        damage_type damage_type() const;

        std::string name() const;
        std::string description() const;
};

// this is all of the wounds that are attached to a limb.
// essentially a wrapper object for a list of wounds
class limb_wounds
{
    private:
        std::vector<wound> wounds;
    public:
        limb_wounds() = default;
        void add_wound( const damage_instance &damage, int location );
        void add_wound( const wound &wnd );
        // healing factor is a multiple to duration for age so the wound heals faster.
        void process( const time_duration &t, double healing_factor );
        // the effective negative armor at the location / damage type
        int sunder( damage_type dmg_type, int location ) const;
        const std::vector<wound> &get_all_wounds() const;
};

#endif // CATA_SRC_WOUND_H
