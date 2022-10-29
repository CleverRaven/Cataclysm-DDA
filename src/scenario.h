#pragma once
#ifndef CATA_SRC_SCENARIO_H
#define CATA_SRC_SCENARIO_H

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "effect_on_condition.h"
#include "translations.h"
#include "type_id.h"

class JsonObject;
class profession;
template<typename T>
class generic_factory;

class scenario
{
    private:
        friend class string_id<scenario>;
        friend class generic_factory<scenario>;
        friend struct mod_tracker;
        string_id<scenario> id;
        std::vector<std::pair<string_id<scenario>, mod_id>> src;
        bool was_loaded = false;
        translation _name_male;
        translation _name_female;
        translation _description_male;
        translation _description_female;
        translation _start_name;

        bool blacklist = false; // If true, professions is a blacklist.
        bool extra_professions = false; // If true, professions add to default professions.
        std::vector<string_id<profession>> professions; // as specified in JSON, verbatim

        /**
         * @ref permitted_professions populates this vector on the first call, which takes
         * a bit of work. On subsequent calls, this vector is returned.
        */
        mutable std::vector<string_id<profession>> cached_permitted_professions;

        std::set<trait_id> _allowed_traits;
        std::set<trait_id> _forced_traits;
        std::set<trait_id> _forbidden_traits;
        std::vector<start_location_id> _allowed_locs;
        int _point_cost = 0;
        std::set<std::string> flags; // flags for some special properties of the scenario
        map_extra_id _map_extra;
        std::vector<mission_type_id> _missions;
        std::vector<effect_on_condition_id> _eoc;

        // does this scenario require a specific achiement to unlock
        cata::optional<achievement_id> _requirement;

        bool _custom_start_date = false;
        bool _is_random_hour = false;
        int _start_hour = 8;
        bool _is_random_day = false;
        int _start_day = 0;
        season_type _start_season = SPRING;
        bool _is_random_year = false;
        int _start_year = 1;
        time_point _start_of_cataclysm;

        vproto_id _starting_vehicle = vproto_id::NULL_ID();

        std::vector<std::pair<mongroup_id, float>> _surround_groups;

        void load( const JsonObject &jo, const std::string &src );
        bool scenario_traits_conflict_with_profession_traits( const profession &p ) const;

    public:
        //these three aren't meant for external use, but had to be made public regardless
        scenario();
        static void load_scenario( const JsonObject &jo, const std::string &src );

        // these should be the only ways used to get at scenario
        static const scenario *generic(); // points to the generic, default profession
        // return a random scenario, weighted for use w/ random character creation
        static const scenario *weighted_random();
        static const std::vector<scenario> &get_all();

        // clear scenario map, every scenario pointer becomes invalid!
        static void reset();
        /** calls @ref check_definition for each scenario */
        static void check_definitions();
        /** Check that item definitions are valid */
        void check_definition() const;

        const string_id<scenario> &ident() const;
        std::string gender_appropriate_name( bool male ) const;
        std::string description( bool male ) const;
        start_location_id start_location() const;
        start_location_id random_start_location() const;
        std::string start_name() const;
        int start_location_count() const;
        int start_location_targets_count() const;

        cata::optional<achievement_id> get_requirement() const;

        bool custom_start_date() const;
        void rerandomize() const;
        bool is_random_hour() const;
        bool is_random_day() const;
        bool is_random_year() const;
        int start_hour() const;
        // Returns day of the season this scenario starts on
        int start_day() const;
        season_type start_season() const;
        int start_year() const;

        time_point start_of_cataclysm() const;
        time_point start_of_game() const;

        vproto_id vehicle() const;

        const profession *weighted_random_profession() const;
        std::vector<string_id<profession>> permitted_professions() const;

        bool traitquery( const trait_id &trait ) const;
        std::set<trait_id> get_locked_traits() const;
        bool is_locked_trait( const trait_id &trait ) const;
        bool is_forbidden_trait( const trait_id &trait ) const;

        bool allowed_start( const start_location_id &loc ) const;
        signed int point_cost() const;
        bool has_map_extra() const;
        const map_extra_id &get_map_extra() const;

        /**
         * Returns "All", "Limited", or "Almost all" (translated)
         * This is used by newcharacter.cpp
        */
        std::string prof_count_str() const;

        // Is this scenario blacklisted?
        bool scen_is_blacklisted() const;

        /** Such as a seasonal start, fiery start, surrounded start, etc. */
        bool has_flag( const std::string &flag ) const;

        /**
         * Do you have the necessary achievement state
         */
        ret_val<void> can_pick() const;

        /**
         * Do you have the points to afford swapping to this scenario
         */
        ret_val<void> can_afford( const scenario &current_scenario, int points ) const;

        const std::vector<mission_type_id> &missions() const;
        const std::vector<effect_on_condition_id> &eoc() const;
        const std::vector<std::pair<mongroup_id, float>> &surround_groups() const;

};

struct scen_blacklist {
    std::set<string_id<scenario>> scenarios;
    bool whitelist = false;

    static void load_scen_blacklist( const JsonObject &jo, const std::string &src );
    void load( const JsonObject &jo, const std::string & );
    void finalize();
};

void reset_scenarios_blacklist();

const scenario *get_scenario();
void set_scenario( const scenario *new_scenario );

#endif // CATA_SRC_SCENARIO_H
