#pragma once
#ifndef CATA_SRC_RECIPE_H
#define CATA_SRC_RECIPE_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <new>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "build_reqs.h"
#include "calendar.h"
#include "optional.h"
#include "requirements.h"
#include "translations.h"
#include "type_id.h"
#include "value_ptr.h"

class Character;
class JsonIn;
class JsonObject;
class item;
template <typename E> struct enum_traits;

enum class recipe_filter_flags : int {
    none = 0,
    no_rotten = 1,
};

enum class recipe_time_flag : int {
    none = 0,
    ignore_proficiencies = 1,
};

template<>
struct enum_traits<recipe_time_flag> {
    static constexpr bool is_flag_enum = true;
};

template<>
struct enum_traits<recipe_filter_flags> {
    static constexpr bool is_flag_enum = true;
};

struct recipe_proficiency {
    proficiency_id id;
    bool required = false;
    float time_multiplier = 0.0f;
    float fail_multiplier = 0.0f;
    float learning_time_mult = 1.0f;
    cata::optional<time_duration> max_experience = cata::nullopt;

    void load( const JsonObject &jo );
    void deserialize( JsonIn &jsin );
};

struct book_recipe_data {
    int skill_req = -1;
    cata::optional<translation> alt_name = cata::nullopt;
    bool hidden = false;

    void load( const JsonObject &jo );
    void deserialize( JsonIn &jsin );
};

class recipe
{
        friend class recipe_dictionary;

    private:
        itype_id result_ = itype_id::NULL_ID();

        int64_t time = 0; // in movement points (100 per turn)

        float exertion = 0.0f;

    public:
        recipe();

        bool is_null() const {
            return result_.is_null();
        }

        explicit operator bool() const {
            return !is_null();
        }

        const itype_id &result() const {
            return result_;
        }

        bool obsolete = false;

        std::string category;
        std::string subcategory;

        translation description;

        int difficulty = 0;

        /** Fetch combined requirement data (inline and via "using" syntax).
         *
         * Use simple_requirements() for player display or when you just want to
         * know the requirements as listed in the json files.  Use
         * deduped_requirements() to calculate actual craftability of a recipe. */
        const requirement_data &simple_requirements() const {
            return requirements_;
        }

        const deduped_requirement_data &deduped_requirements() const {
            return deduped_requirements_;
        }

        const recipe_id &ident() const {
            return ident_;
        }

        bool is_blacklisted() const {
            return requirements_.is_blacklisted();
        }

        // Slower equivalent of is_blacklisted that needs to be used before
        // recipe finalization happens
        bool will_be_blacklisted() const;

        std::function<bool( const item & )> get_component_filter(
            recipe_filter_flags = recipe_filter_flags::none ) const;

        /** Prevent this recipe from ever being added to the player's learned recipes ( used for special NPC crafting ) */
        bool never_learn = false;

        /** If recipe can be used for disassembly fetch the combined requirements */
        requirement_data disassembly_requirements() const {
            if( reversible ) {
                return simple_requirements().disassembly_requirements();
            } else {
                return {};
            }
        }

        /// @returns The name (@ref item::nname) of the resulting item (@ref result).
        std::string result_name() const;

        std::map<itype_id, int> byproducts;

        skill_id skill_used;
        std::map<skill_id, int> required_skills;
        std::vector<recipe_proficiency> proficiencies;

        std::map<skill_id, int> autolearn_requirements; // Skill levels required to autolearn
        std::map<skill_id, int> learn_by_disassembly; // Skill levels required to learn by disassembly
        // Books containing this recipe, and the skill level required
        std::map<itype_id, book_recipe_data> booksets;

        std::set<flag_id> flags_to_delete; // Flags to delete from the resultant item.

        // Create a string list to describe the skill requirements for this recipe
        // Format: skill_name(level/amount), skill_name(level/amount)
        // Character object (if provided) used to color levels

        // These are primarily used by the crafting menu.
        // Format the primary skill string.
        std::string primary_skill_string( const Character *c, bool print_skill_level ) const;

        // Format the other skills string.  This is also used for searching within the crafting
        // menu which includes the primary skill.
        std::string required_skills_string( const Character *, bool include_primary_skill,
                                            bool print_skill_level ) const;
        // Format the proficiencies string.
        std::string required_proficiencies_string( const Character *c ) const;
        std::string used_proficiencies_string( const Character *c ) const;
        std::string missing_proficiencies_string( const Character *c ) const;
        // Proficiencies for search
        std::string recipe_proficiencies_string() const;
        // Required proficiencies
        std::set<proficiency_id> required_proficiencies() const;
        //
        bool character_has_required_proficiencies( const Character &c ) const;
        // Helpful proficiencies
        std::set<proficiency_id> assist_proficiencies() const;
        // The time malus due to proficiencies lacking
        float proficiency_time_maluses( const Character &crafter ) const;
        // The failure malus due to proficiencies lacking
        float proficiency_failure_maluses( const Character &crafter ) const;

        // How active of exercise this recipe is
        float exertion_level() const;

        // This is used by the basecamp bulletin board.
        std::string required_all_skills_string() const;

        // Create a string to describe the time savings of batch-crafting, if any.
        // Format: "N% at >M units" or "none"
        std::string batch_savings_string() const;

        // Create an item instance as if the recipe was just finished,
        // Contain charges multiplier
        item create_result() const;
        std::vector<item> create_results( int batch = 1 ) const;

        // Create byproduct instances as if the recipe was just finished
        std::vector<item> create_byproducts( int batch = 1 ) const;

        bool has_byproducts() const;

        int64_t batch_time( const Character &guy, int batch, float multiplier, size_t assistants ) const;
        time_duration batch_duration( const Character &guy, int batch = 1, float multiplier = 1.0,
                                      size_t assistants = 0 ) const;

        time_duration time_to_craft( const Character &guy,
                                     recipe_time_flag flags = recipe_time_flag::none ) const;
        int64_t time_to_craft_moves( const Character &guy,
                                     recipe_time_flag flags = recipe_time_flag::none ) const;

        bool has_flag( const std::string &flag_name ) const;

        bool is_reversible() const {
            return reversible;
        }

        void load( const JsonObject &jo, const std::string &src );
        void finalize();

        /** Returns a non-empty string describing an inconsistency (if any) in the recipe. */
        std::string get_consistency_error() const;

        bool is_blueprint() const;
        const std::string &get_blueprint() const;
        const translation &blueprint_name() const;
        const std::vector<itype_id> &blueprint_resources() const;
        const std::vector<std::pair<std::string, int>> &blueprint_provides() const;
        const std::vector<std::pair<std::string, int>> &blueprint_requires() const;
        const std::vector<std::pair<std::string, int>> &blueprint_excludes() const;
        /**
         * Calculate blueprint requirements according to changed terrain and furniture
         * tiles, then check the calculated requirements against blueprint requirements
         * specified in JSON.  If there's any inconsistency, it issues a debug message.
         * This is only used in unit tests so as to speed up data loading in gameplay.
         */
        void check_blueprint_requirements();

        bool hot_result() const;

        bool removes_raw() const;

        // Returns the amount or charges recipe will produce.
        int makes_amount() const;

    private:
        void incorporate_build_reqs();
        void add_requirements( const std::vector<std::pair<requirement_id, int>> &reqs );

    private:
        recipe_id ident_ = recipe_id::NULL_ID();

        /** Abstract recipes can be inherited from but are themselves disposed of at finalization */
        bool abstract = false;

        /** set learning requirements equal to required skills at finalization? */
        bool autolearn = false;

        /** Does the item spawn contained in container? */
        bool contained = false;

        /** Does the container spawn sealed? */
        bool sealed = true;

        /** Can recipe be used for disassembly of @ref result via @ref disassembly_requirements */
        bool reversible = false;

        /** What does the item spawn contained in? Unset ("null") means default container. */
        itype_id container = itype_id::NULL_ID();

        /** External requirements (via "using" syntax) where second field is multiplier */
        std::vector<std::pair<requirement_id, int>> reqs_external;

        /** Requires specified inline with the recipe (and replaced upon inheritance) */
        std::vector<std::pair<requirement_id, int>> reqs_internal;

        /** Combined requirements cached when recipe finalized */
        requirement_data requirements_;

        /** Deduped version constructed from the above requirements_ */
        deduped_requirement_data deduped_requirements_;

        std::set<std::string> flags;

        /** If set (zero or positive) set charges of output result for items counted by charges */
        cata::optional<int> charges;

        // maximum achievable time reduction, as percentage of the original time.
        // if zero then the recipe has no batch crafting time reduction.
        double batch_rscale = 0.0;
        int batch_rsize = 0; // minimum batch size to needed to reach batch_rscale
        int result_mult = 1; // used by certain batch recipes that create more than one stack of the result
        std::string blueprint;
        translation bp_name;
        std::vector<itype_id> bp_resources;
        std::vector<std::pair<std::string, int>> bp_provides;
        std::vector<std::pair<std::string, int>> bp_requires;
        std::vector<std::pair<std::string, int>> bp_excludes;

        /** Blueprint requirements either autocalculcated or explicitly
         * specified.  These members relate to resolving those blueprint
         * requirements into the standard recipe requirements. */
        bool bp_autocalc = false;
        bool check_blueprint_needs = false;
        cata::value_ptr<build_reqs> blueprint_reqs;
};

#endif // CATA_SRC_RECIPE_H
