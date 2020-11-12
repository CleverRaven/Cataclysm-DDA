#pragma once
#ifndef CATA_SRC_PROFESSION_H
#define CATA_SRC_PROFESSION_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "pldata.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"

template<typename T>
class generic_factory;

class item;

using itype_id = std::string;
class JsonObject;
class avatar;
class player;

class profession
{
    public:
        using StartingSkill = std::pair<skill_id, int>;
        using StartingSkillList = std::vector<StartingSkill>;
        struct itypedec {
            std::string type_id;
            /** Snippet id, @see snippet_library. */
            snippet_id snip_id;
            // compatible with when this was just a std::string
            itypedec( const std::string &t ) : type_id( t ), snip_id( snippet_id::NULL_ID() ) {
            }
            itypedec( const std::string &t, const snippet_id &d ) : type_id( t ), snip_id( d ) {
            }
        };
        using itypedecvec = std::vector<itypedec>;
        friend class string_id<profession>;
        friend class generic_factory<profession>;

    private:
        string_id<profession> id;
        bool was_loaded = false;

        translation _name_male;
        translation _name_female;
        translation _description_male;
        translation _description_female;
        signed int _point_cost = 0;

        // TODO: In professions.json, replace lists of itypes (legacy) with item groups
        itypedecvec legacy_starting_items;
        itypedecvec legacy_starting_items_male;
        itypedecvec legacy_starting_items_female;
        item_group_id _starting_items = item_group_id( "EMPTY_GROUP" );
        item_group_id _starting_items_male = item_group_id( "EMPTY_GROUP" );
        item_group_id _starting_items_female = item_group_id( "EMPTY_GROUP" );
        itype_id no_bonus; // See profession::items and class json_item_substitution in profession.cpp

        std::vector<addiction> _starting_addictions;
        std::vector<bionic_id> _starting_CBMs;
        std::vector<trait_id> _starting_traits;
        std::set<trait_id> _forbidden_traits;
        std::vector<mtype_id> _starting_pets;
        vproto_id _starting_vehicle = vproto_id::NULL_ID();
        // the int is what level the spell starts at
        std::map<spell_id, int> _starting_spells;
        std::set<std::string> flags; // flags for some special properties of the profession
        StartingSkillList  _starting_skills;

        void check_item_definitions( const itypedecvec &items ) const;

        void load( const JsonObject &jo, const std::string &src );

    public:
        //these three aren't meant for external use, but had to be made public regardless
        profession();

        static void load_profession( const JsonObject &jo, const std::string &src );
        static void load_item_substitutions( const JsonObject &jo );

        // these should be the only ways used to get at professions
        static const profession *generic(); // points to the generic, default profession
        static const std::vector<profession> &get_all();

        static bool has_initialized();
        // clear profession map, every profession pointer becomes invalid!
        static void reset();
        /** calls @ref check_definition for each profession */
        static void check_definitions();
        /** Check that item/CBM/addiction/skill definitions are valid. */
        void check_definition() const;

        const string_id<profession> &ident() const;
        std::string gender_appropriate_name( bool male ) const;
        std::string description( bool male ) const;
        signed int point_cost() const;
        std::list<item> items( bool male, const std::vector<trait_id> &traits ) const;
        std::vector<addiction> addictions() const;
        vproto_id vehicle() const;
        std::vector<mtype_id> pets() const;
        std::vector<bionic_id> CBMs() const;
        StartingSkillList skills() const;

        std::map<spell_id, int> spells() const;
        void learn_spells( avatar &you ) const;

        /**
         * Check if this type of profession has a certain flag set.
         *
         * Current flags: none
         */
        bool has_flag( const std::string &flag ) const;

        /**
         * Check if the given player can pick this job with the given amount
         * of points.
         *
         * @return true, if player can pick profession. Otherwise - false.
         */
        bool can_pick( const player &u, int points ) const;
        bool is_locked_trait( const trait_id &trait ) const;
        bool is_forbidden_trait( const trait_id &trait ) const;
        std::vector<trait_id> get_locked_traits() const;
        std::set<trait_id> get_forbidden_traits() const;
};

#endif // CATA_SRC_PROFESSION_H
