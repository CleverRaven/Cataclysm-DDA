#pragma once
#ifndef RECIPE_H
#define RECIPE_H

#include <cstddef>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include <string>
#include <utility>

#include "requirements.h"
#include "type_id.h"

class item;
class JsonObject;
class time_duration;

using itype_id = std::string; // From itype.h
class Character;

class recipe
{
        friend class recipe_dictionary;

    private:
        itype_id result_ = "null";

    public:
        recipe();

        operator bool() const {
            return result_ != "null";
        }

        const itype_id &result() const {
            return result_;
        }

        bool obsolete = false;

        std::string category;
        std::string subcategory;

        std::string description;

        int time = 0; // in movement points (100 per turn)
        int difficulty = 0;

        /** Fetch combined requirement data (inline and via "using" syntax) */
        const requirement_data &requirements() const {
            return requirements_;
        }

        const recipe_id &ident() const {
            return ident_;
        }

        bool is_blacklisted() const {
            return requirements_.is_blacklisted();
        }

        std::function<bool( const item & )> get_component_filter() const;

        /** Prevent this recipe from ever being added to the player's learned recipies ( used for special NPC crafting ) */
        bool never_learn = false;

        /** If recipe can be used for disassembly fetch the combined requirements */
        requirement_data disassembly_requirements() const {
            return reversible ? requirements().disassembly_requirements() : requirement_data();
        }

        /// @returns The name (@ref item::nname) of the resulting item (@ref result).
        std::string result_name() const;

        std::map<itype_id, int> byproducts;

        skill_id skill_used;
        std::map<skill_id, int> required_skills;

        std::map<skill_id, int> autolearn_requirements; // Skill levels required to autolearn
        std::map<skill_id, int> learn_by_disassembly; // Skill levels required to learn by disassembly
        std::map<itype_id, int> booksets; // Books containing this recipe, and the skill level required

        // Create a string list to describe the skill requirements for this recipe
        // Format: skill_name(level/amount), skill_name(level/amount)
        // Character object (if provided) used to color levels
        std::string required_skills_string( const Character *, bool print_skill_level ) const;
        std::string required_skills_string( const Character * ) const;
        std::string required_skills_string() const;

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

        int batch_time( int batch, float multiplier, size_t assistants ) const;
        time_duration batch_duration( int batch = 1, float multiplier = 1.0,
                                      size_t assistants = 0 ) const;

        bool has_flag( const std::string &flag_name ) const;

        bool is_reversible() const {
            return reversible;
        }

        void load( JsonObject &jo, const std::string &src );
        void finalize();

        /** Returns a non-empty string describing an inconsistency (if any) in the recipe. */
        std::string get_consistency_error() const;

        bool is_blueprint() const;
        const std::string &get_blueprint() const;
        const std::string &blueprint_name() const;
        const std::vector<itype_id> &blueprint_resources() const;
        const std::vector<std::pair<std::string, int>> &blueprint_provides() const;
        const std::vector<std::pair<std::string, int>> &blueprint_requires() const;
        const std::vector<std::pair<std::string, int>> &blueprint_excludes() const;
        /** Retrieves a map of changed ter_id/furn_id to the number of tiles changed, then
         *  converts that to requirement_ids and counts.  The requirements later need to be
         *  consolidated and duplicate tools/qualities eliminated.
         */
        void add_bp_autocalc_requirements();

        bool hot_result() const;

    private:
        void add_requirements( const std::vector<std::pair<requirement_id, int>> &reqs );

    private:
        recipe_id ident_ = recipe_id::NULL_ID();

        /** Abstract recipes can be inherited from but are themselves disposed of at finalization */
        bool abstract = false;

        /** set learning requirements equal to required skills at finalization? */
        bool autolearn = false;

        /** Does the item spawn contained in container? */
        bool contained = false;

        /** Can recipe be used for disassembly of @ref result via @ref disassembly_requirements */
        bool reversible = false;

        /** What does the item spawn contained in? Unset ("null") means default container. */
        itype_id container = "null";

        /** External requirements (via "using" syntax) where second field is multiplier */
        std::vector<std::pair<requirement_id, int>> reqs_external;

        /** Requires specified inline with the recipe (and replaced upon inheritance) */
        std::vector<std::pair<requirement_id, int>> reqs_internal;

        /** Combined requirements cached when recipe finalized */
        requirement_data requirements_;

        std::set<std::string> flags;

        /** If set (zero or positive) set charges of output result for items counted by charges */
        int charges = -1;

        // maximum achievable time reduction, as percentage of the original time.
        // if zero then the recipe has no batch crafting time reduction.
        double batch_rscale = 0.0;
        int batch_rsize = 0; // minimum batch size to needed to reach batch_rscale
        int result_mult = 1; // used by certain batch recipes that create more than one stack of the result
        std::string blueprint;
        std::string bp_name;
        std::vector<itype_id> bp_resources;
        std::vector<std::pair<std::string, int>> bp_provides;
        std::vector<std::pair<std::string, int>> bp_requires;
        std::vector<std::pair<std::string, int>> bp_excludes;
        bool bp_autocalc = false;
};

#endif // RECIPE_H
