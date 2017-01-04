#ifndef PROFESSION_H
#define PROFESSION_H

#include "string_id.h"
#include "item_group.h"
#include "item.h"

#include <string>
#include <vector>
#include <map>
#include <set>

template<typename T>
class generic_factory;
class profession;
class player;
class JsonArray;
class JsonObject;
class addiction;
enum add_type : int;
    class Skill;
    using skill_id = string_id<Skill>;

class json_item_substitution {
    public:
        void reset();
        void load( JsonObject &jo, const std::string & );
        void check_consistency();

    private:
        struct substitution {
            std::vector<std::string> traits_present; // If the player has all of these traits
            std::vector<std::string> traits_absent; // And they don't have any of these traits
            itype_id former; // Then replace any starting items with this itype
            int count; // with this amount of items with
            itype_id latter; // this itype
        };
        // Note: If former.empty(), then latter is a bonus item
        std::vector<substitution> substitutions;
        bool meets_trait_conditions( const substitution &sub,
                                     const std::vector<std::string> &traits ) const;
    public:
        std::vector<itype_id> get_bonus_items( const std::vector<std::string> &traits ) const;
        std::vector<item> get_substitution( const item &it, const std::vector<std::string> &traits ) const;
};

class profession
{
    public:
        typedef std::pair<skill_id, int> StartingSkill;
        typedef std::vector<StartingSkill> StartingSkillList;
        struct itypedec {
            std::string type_id;
            /** Snippet id, @see snippet_library. */
            std::string snippet_id;
            // compatible with when this was just a std::string
            itypedec( const char *t ) : type_id( t ), snippet_id() {
            }
            itypedec( const std::string &t, const std::string &d ) : type_id( t ), snippet_id( d ) {
            }
        };
        typedef std::vector<itypedec> itypedecvec;
        friend class string_id<profession>;
        friend class generic_factory<profession>;

        static void load_json_item_substitution( JsonObject &jo, const std::string &src );
        static void reset_json_item_substitution();
        static void check_consistency_json_item_substitution();
    private:
        string_id<profession> id;

        static json_item_substitution item_substitution;

        bool was_loaded = false;

        std::string _name_male;
        std::string _name_female;
        std::string _description_male;
        std::string _description_female;
        std::string _gender_req;
        signed int _point_cost;

        // TODO: In professions.json, replace lists of itypes (legacy) with item groups
        itypedecvec legacy_starting_items;
        itypedecvec legacy_starting_items_male;
        itypedecvec legacy_starting_items_female;
        Group_tag _starting_items = "EMPTY_GROUP";
        Group_tag _starting_items_male = "EMPTY_GROUP";
        Group_tag _starting_items_female = "EMPTY_GROUP";

        std::vector<addiction> _starting_addictions;
        std::vector<std::string> _starting_CBMs;
        std::vector<std::string> _starting_traits;
        std::set<std::string> flags; // flags for some special properties of the profession
        StartingSkillList  _starting_skills;

        void check_item_definitions( const itypedecvec &items ) const;

        void load( JsonObject &jo, const std::string &src );

    public:
        //these three aren't meant for external use, but had to be made public regardless
        profession();

        static void load_profession( JsonObject &obj, const std::string &src );

        // these should be the only ways used to get at professions
        static const profession *generic(); // points to the generic, default profession
        // return a random profession, weighted for use w/ random character creation or npcs
        static const profession *weighted_random();
        static const std::vector<profession> &get_all();

        static bool has_initialized();
        // clear profession map, every profession pointer becames invalid!
        static void reset();
        /** calls @ref check_definition for each profession */
        static void check_definitions();
        /** Check that item/CBM/addiction/skill definitions are valid. */
        void check_definition() const;

        const string_id<profession> &ident() const;
        std::string gender_appropriate_name( bool male ) const;
        std::string description( bool male ) const;
        std::string gender_req() const;
        signed int point_cost() const;
        std::list<item> items( bool male, const std::vector<std::string> &traits ) const;
        std::vector<addiction> addictions() const;
        std::vector<std::string> CBMs() const;
        std::vector<std::string> traits() const;
        const StartingSkillList skills() const;

        /**
         * Check if this type of profession has a certain flag set.
         *
         * Current flags: none
         */
        bool has_flag( std::string flag ) const;

        /**
         * Check if the given player can pick this job with the given amount
         * of points.
         *
         * @return true, if player can pick profession. Otherwise - false.
         */
        bool can_pick( player *u, int points ) const;
        bool locked_traits( const std::string &trait ) const;

};

#endif
