#pragma once
#ifndef RECIPE_H
#define RECIPE_H

#include <map>
#include <set>
#include <vector>

#include "requirements.h"
#include "string_id.h"

class recipe_dictionary;
class Skill;
class item;
using skill_id = string_id<Skill>;
using itype_id = std::string; // From itype.h
using requirement_id = string_id<requirement_data>;
class recipe;
using recipe_id = string_id<recipe>;
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

        bool has_flag( const std::string &flag_name ) const;

        bool is_reversible() const {
            return reversible;
        }

        void load( JsonObject &jo, const std::string &src );
        void finalize();

        /** Returns a non-empty string describing an inconsistency (if any) in the recipe. */
        std::string get_consistency_error() const;

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
};

#endif // RECIPE_H
