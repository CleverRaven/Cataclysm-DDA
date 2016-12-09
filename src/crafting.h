#ifndef CRAFTING_H
#define CRAFTING_H

#include "item.h"         // item
#include "cursesdef.h"    // WINDOW
#include "requirements.h"

#include <string>
#include <vector>
#include <map>
#include <list>

class recipe_dictionary;
class JsonObject;
class Skill;
using skill_id = string_id<Skill>;
class inventory;
class player;
class npc;

enum body_part : int; // From bodypart.h
typedef int nc_color; // From color.h

using itype_id     = std::string; // From itype.h
using requirement_id = string_id<requirement_data>;

struct recipe {
        friend class recipe_dictionary;

    public:
        recipe();

        itype_id result = "null";

        operator bool() const {
            return result != "null";
        }

        std::string category;
        std::string subcategory;

        int time = 0; // in movement points (100 per turn)
        int difficulty = 0;

        /** Fetch combined requirement data (inline and via "using" syntax) */
        const requirement_data& requirements() const {
            return requirements_;
        }

        /** If recipe can be used for disassembly fetch the combined requirements */
        requirement_data disassembly_requirements() const {
            return reversible ? requirements().disassembly_requirements() : requirement_data();
        }

        /** Combined requirements cached when recipe finalized */
        requirement_data requirements_;

        std::map<itype_id,int> byproducts;

        // Does the item spawn contained in container?
        bool contained = false;
        // What does the item spawn contained in? Unset ("null") means default container.
        itype_id container = "null";

        skill_id skill_used;
        std::map<skill_id, int> required_skills;

        /** set learning requirements equal to required skills at finalization? */
        bool autolearn = false;

        std::map<skill_id, int> autolearn_requirements; // Skill levels required to autolearn
        std::map<skill_id, int> learn_by_disassembly; // Skill levels required to learn by disassembly

        /** If set (zero or positive) set charges of output result for items counted by charges */
        int charges = -1;

        // maximum achievable time reduction, as percentage of the original time.
        // if zero then the recipe has no batch crafting time reduction.
        double batch_rscale = 0.0;
        int batch_rsize = 0; // minimum batch size to needed to reach batch_rscale
        int result_mult = 1; // used by certain batch recipes that create more than one stack of the result

        std::map<itype_id,int> booksets;
        std::set<std::string> flags;

        const std::string &ident() const;

        //Create a string list to describe the skill requirements fir this recipe
        // Format: skill_name(amount), skill_name(amount)
        std::string required_skills_string() const;

        // Create an item instance as if the recipe was just finished,
        // Contain charges multiplier
        item create_result() const;
        std::vector<item> create_results( int batch = 1 ) const;

        // Create byproduct instances as if the recipe was just finished
        std::vector<item> create_byproducts( int batch = 1 ) const;

        bool has_byproducts() const;

        bool check_eligible_containers_for_crafting( int batch = 1 ) const;

        int print_items( WINDOW *w, int ypos, int xpos, nc_color col, int batch = 1 ) const;

        int print_time( WINDOW *w, int ypos, int xpos, int width, nc_color col,
                        int batch = 1 ) const;

        int batch_time( int batch = 1 ) const;

        bool has_flag( const std::string &flag_name ) const;

    private:
        std::string ident_;

        /** Abstract recipes can be inherited from but are themselves disposed of at finalization */
        bool abstract = false;

        /** Can recipe be used for disassembly of @ref result via @ref disassembly_requirements */
        bool reversible = false;

        /** External requirements (via "using" syntax) where second field is multiplier */
        std::vector<std::pair<requirement_id, int>> reqs_external;

        /** Requires specified inline with the recipe (and replaced upon inheritance) */
        std::vector<std::pair<requirement_id, int>> reqs_internal;
};

// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo( item *dis_item, player &p );
// same as above but for each item in the list
void remove_ammo( std::list<item> &dis_items, player &p );

// Show the "really disassemble?" query along with a list of possible results.
// Returns false if the player answered no to the query.
bool query_dissamble( const item &dis_item );
const recipe *select_crafting_recipe( int &batch_size );

void batch_recipes( const inventory &crafting_inv,
                    const std::vector<npc *> &helpers,
                    std::vector<const recipe *> &current,
                    std::vector<bool> &available, const recipe *r );

void set_item_spoilage( item &newit, float used_age_tally, int used_age_count );
void set_item_food( item &newit );
void set_item_inventory( item &newit );
void finalize_crafted_item( item &newit, float used_age_tally, int used_age_count );

#endif
