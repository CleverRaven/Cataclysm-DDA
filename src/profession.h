#ifndef PROFESSION_H
#define PROFESSION_H

#include "string_id.h"

#include <string>
#include <vector>
#include <map>
#include <set>

class profession;
class player;
class JsonArray;
class JsonObject;
class addiction;
enum add_type : int;
    class Skill;
    using skill_id = string_id<Skill>;

    typedef std::map<string_id<profession>, profession> profmap;

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
    private:
        string_id<profession> _ident;
        std::string _name_male;
        std::string _name_female;
        std::string _description_male;
        std::string _description_female;
        std::string _gender_req;
        signed int _point_cost;
        itypedecvec _starting_items;
        itypedecvec _starting_items_male;
        itypedecvec _starting_items_female;
        std::vector<addiction> _starting_addictions;
        std::vector<std::string> _starting_CBMs;
        std::vector<std::string> _starting_traits;
        std::set<std::string> flags; // flags for some special properties of the profession
        StartingSkillList  _starting_skills;

        void add_items_from_jsonarray( JsonArray jsarr, std::string gender );
        void add_item( const itypedec &entry, const std::string &gender );
        void add_addiction( add_type, int );
        void add_CBM( std::string CBM );
        void add_trait( std::string trait );
        // Starting skills will boost the players level in those skills by a
        // given amount.
        void add_skill( const skill_id &skill_name, const int level );

        void check_item_definitions( const itypedecvec &items ) const;

        static profmap _all_profs;
    public:
        //these three aren't meant for external use, but had to be made public regardless
        profession();

        static void load_profession( JsonObject &jsobj );

        // these should be the only ways used to get at professions
        static const profession *generic(); // points to the generic, default profession
        // return a random profession, weighted for use w/ random character creation or npcs
        static const profession *weighted_random();
        static profmap::const_iterator begin();
        static profmap::const_iterator end();
        static int count();

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
        itypedecvec items( bool male ) const;
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

};

#endif
