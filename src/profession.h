#pragma once
#ifndef CATA_SRC_PROFESSION_H
#define CATA_SRC_PROFESSION_H

#include <list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "addiction.h"
#include "mutation.h"
#include "ret_val.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonObject;
class avatar;
class item;
template<typename T>
class generic_factory;

class profession
{
    public:
        using StartingSkill = std::pair<skill_id, int>;
        using StartingSkillList = std::vector<StartingSkill>;
        struct itypedec {
            itype_id type_id;
            /** Snippet id, @see snippet_library. */
            snippet_id snip_id;
            // compatible with when this was just a std::string
            explicit itypedec( const std::string &t ) :
                type_id( t ), snip_id( snippet_id::NULL_ID() ) {
            }
            itypedec( const std::string &t, const snippet_id &d ) : type_id( t ), snip_id( d ) {
            }
        };
        using itypedecvec = std::vector<itypedec>;
        friend class string_id<profession>;
        friend class generic_factory<profession>;
        friend struct mod_tracker;

    private:
        string_id<profession> id;
        bool was_loaded = false;

        translation _name_male;
        translation _name_female;
        translation _description_male;
        translation _description_female;
        signed int _point_cost = 0;
        std::optional<signed int> _starting_cash = std::nullopt;

        // TODO: In professions.json, replace lists of itypes (legacy) with item groups
        itypedecvec legacy_starting_items;
        itypedecvec legacy_starting_items_male;
        itypedecvec legacy_starting_items_female;
        item_group_id _starting_items = item_group_id( "EMPTY_GROUP" );
        item_group_id _starting_items_male = item_group_id( "EMPTY_GROUP" );
        item_group_id _starting_items_female = item_group_id( "EMPTY_GROUP" );
        itype_id no_bonus; // See profession::items and class json_item_substitution in profession.cpp

        // does this profession require a specific achiement to unlock
        std::optional<achievement_id> _requirement;

        std::vector<addiction> _starting_addictions;
        std::vector<bionic_id> _starting_CBMs;
        std::vector<proficiency_id> _starting_proficiencies;
        std::vector<recipe_id> _starting_recipes;
        std::vector<trait_and_var> _starting_traits;
        std::vector<matype_id> _starting_martialarts;
        std::vector<matype_id> _starting_martialarts_choices;
        std::set<trait_id> _forbidden_traits;
        std::vector<mtype_id> _starting_pets;
        trait_group::Trait_group_tag _starting_npc_background;
        std::set<string_id<profession>> _hobby_exclusion;
        bool hobbies_whitelist = true;
        vproto_id _starting_vehicle = vproto_id::NULL_ID();
        // the int is what level the spell starts at
        std::map<spell_id, int> _starting_spells;
        std::vector<effect_on_condition_id> effect_on_conditions;
        std::set<std::string> flags; // flags for some special properties of the profession
        StartingSkillList  _starting_skills;
        std::vector<mission_type_id> _missions; // starting missions for profession

        std::string _subtype;

        void check_item_definitions( const itypedecvec &items ) const;

        void load( const JsonObject &jo, std::string_view src );

    public:
        //these three aren't meant for external use, but had to be made public regardless
        profession();

        static void load_profession( const JsonObject &jo, const std::string &src );
        static void load_item_substitutions( const JsonObject &jo );

        // these should be the only ways used to get at professions
        static const profession *generic(); // points to the generic, default profession
        static const std::vector<profession> &get_all();
        static std::vector<string_id<profession>> get_all_hobbies();

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
        std::optional<signed int> starting_cash() const;
        std::list<item> items( bool male, const std::vector<trait_id> &traits ) const;
        std::vector<addiction> addictions() const;
        vproto_id vehicle() const;
        std::vector<mtype_id> pets() const;
        std::vector<bionic_id> CBMs() const;
        std::vector<proficiency_id> proficiencies() const;
        std::vector<recipe_id> recipes() const;
        std::vector<matype_id> ma_known() const;
        std::vector<matype_id> ma_choices() const;
        bool allows_hobby( const string_id<profession> &hobby )const;
        int ma_choice_amount;
        StartingSkillList skills() const;
        const std::vector<mission_type_id> &missions() const;
        int age_lower = 21;
        int age_upper = 55;

        std::vector<std::pair<string_id<profession>, mod_id>> src;

        std::optional<achievement_id> get_requirement() const;

        std::map<spell_id, int> spells() const;
        void learn_spells( avatar &you ) const;
        std::vector<effect_on_condition_id> get_eocs() const;
        //returns the profession id
        profession_id get_profession_id() const;

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
        ret_val<void> can_afford( const Character &you, int points ) const;

        /**
         * Do you have the necessary achievement state
         */
        ret_val<void> can_pick() const;
        bool is_locked_trait( const trait_id &trait ) const;
        bool is_forbidden_trait( const trait_id &trait ) const;
        std::vector<trait_and_var> get_locked_traits() const;
        std::set<trait_id> get_forbidden_traits() const;
        trait_id pick_background() const;

        bool is_hobby() const;
        bool is_blacklisted() const;
};

struct profession_blacklist {
    std::set<string_id<profession>> professions;
    bool whitelist = false;

    static void load_profession_blacklist( const JsonObject &jo, std::string_view src );
    static void reset();
    void load( const JsonObject &jo, std::string_view );
    void check_consistency() const;
};

#endif // CATA_SRC_PROFESSION_H
