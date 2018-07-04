#pragma once
#ifndef SCENARIO_H
#define SCENARIO_H

#include "string_id.h"

#include <string>
#include <vector>
#include <map>
#include <set>

class scenario;
class profession;
class player;
class JsonArray;
class JsonObject;
class addiction;
enum add_type : int;
class start_location;
using start_location_id = string_id<start_location>;
template<typename T>
class generic_factory;
struct mutation_branch;
using trait_id = string_id<mutation_branch>;

class scenario
{

    private:
        friend class string_id<scenario>;
        friend class generic_factory<scenario>;
        string_id<scenario> id;
        bool was_loaded = false;
        std::string _name_male;
        std::string _name_female;
        std::string _description_male;
        std::string _description_female;
        std::string _start_name;

        bool blacklist = false; // If true, professions is a blacklist.
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
        int _point_cost;
        std::set<std::string> flags; // flags for some special properties of the scenario
        std::string _map_special;

        void load( JsonObject &jo, const std::string &src );

    public:
        //these three aren't meant for external use, but had to be made public regardless
        scenario();
        static void load_scenario( JsonObject &jo, const std::string &src );

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

        const profession *weighted_random_profession() const;
        std::vector<string_id<profession>> permitted_professions() const;

        bool traitquery( const trait_id &trait ) const;
        std::set<trait_id> get_locked_traits() const;
        bool is_locked_trait( const trait_id &trait ) const;
        bool is_forbidden_trait( const trait_id &trait ) const;

        bool allowed_start( const start_location_id &loc ) const;
        signed int point_cost() const;
        bool has_map_special() const;
        const std::string &get_map_special() const;

        /**
         * Returns "All", "Limited", or "Almost all" (translated)
         * This is used by newcharacter.cpp
        */
        std::string prof_count_str() const;

        /** Such as a seasonal start, fiery start, surrounded start, etc. */
        bool has_flag( const std::string &flag ) const;

        /**
         *
         */
        bool can_pick( const scenario &current_scenario, int points ) const;

};

#endif
