#pragma once
#ifndef CHARACTER_MUTATIONS_H
#define CHARACTER_MUTATIONS_H

#include "bodypart.h"
#include "damage.h"
#include "type_id.h"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Creature;
class Character;
class JsonOut;
class JsonIn;

enum body_part;

struct resistances;
struct social_modifiers;

struct special_attack {
    std::string text;
    damage_instance damage;
};

// contains all of the mutation data a character has
class character_mutations
{
    public:
        struct trait_data {
            /** Whether the mutation is activated. */
            bool powered = false;
            /** Key to select the mutation in the UI. */
            char key = ' ';
            /**
             * Time (in turns) until the mutation increase hunger/thirst/fatigue according
             * to its cost (@ref mutation_branch::cost). When those costs have been paid, this
             * is reset to @ref mutation_branch::cooldown.
             */
            int charge = 0;
            void serialize( JsonOut &json ) const;
            void deserialize( JsonIn &jsin );
        };
        // Drench cache
        enum water_tolerance {
            WT_IGNORED = 0,
            WT_NEUTRAL,
            WT_GOOD,
            NUM_WATER_TOLERANCE
        };

        character_mutations() = default;
        character_mutations( const std::unordered_map<trait_id, character_mutations::trait_data>
                             &my_mutations, const std::unordered_set<trait_id> &my_traits ) {
            this->my_mutations = my_mutations;
            this->my_traits = my_traits;
        }
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        void load_cache_data( Character &guy );

        /** Get the idents of all base traits. */
        std::vector<trait_id> get_base_traits() const;
        /** Get the idents of all traits/mutations. */
        std::vector<trait_id> get_mutations( bool include_hidden = true ) const;
        /**
         * Goes over all mutations, gets min and max of a value with given name
         * @return min( 0, lowest ) + max( 0, highest )
         */
        float mutation_value( const std::string &val ) const;

        /**
         * Goes over all mutations, returning the sum of the social modifiers
         */
        social_modifiers get_mutation_social_mods() const;
        /**
         * Returns resistances on a body part provided by mutations
         */
        // TODO: Cache this, it's kinda expensive to compute
        resistances mutation_armor( body_part bp ) const;
        float mutation_armor( body_part bp, damage_type dt ) const;
        float mutation_armor( body_part bp, const damage_unit &du ) const;
        /** Handles things like destruction of armor, etc. */
        void mutation_effect( Character &owner, const trait_id &mut );
        /** Handles what happens when you lose a mutation. */
        void mutation_loss_effect( Character &owner, const trait_id &mut );

        bool has_active_mutation( const trait_id &b ) const;
        /** Picks a random valid mutation and gives it to the Character, possibly removing/changing others along the way */
        void mutate( Character &owner );
        /** Returns true if the player doesn't have the mutation or a conflicting one and it complies with the force typing */
        bool mutation_ok( const trait_id &mutation, bool force_good, bool force_bad ) const;
        /** Picks a random valid mutation in a category and mutate_towards() it */
        void mutate_category( Character &owner, const std::string &mut_cat );
        /** Mutates toward one of the given mutations, upgrading or removing conflicts if necessary */
        bool mutate_towards( Character &owner, std::vector<trait_id> muts, int num_tries = INT_MAX );
        /** Mutates toward the entered mutation, upgrading or removing conflicts if necessary */
        bool mutate_towards( Character &owner, const trait_id &mut );
        /** Removes a mutation, downgrading to the previous level if possible */
        void remove_mutation( Character &owner, const trait_id &mut, bool silent = false );
        /** Returns true if the player has the entered mutation child flag */
        bool has_child_flag( const trait_id &flag ) const;
        /** Removes the mutation's child flag from the player's list */
        void remove_child_flag( Character &owner, const trait_id &flag );
        /** Recalculates mutation_category_level[] values for the player */
        void set_highest_cat_level();
        /** Returns the highest mutation category */
        std::string get_highest_category() const;
        /** Recalculates mutation drench protection for all bodyparts (ignored/good/neutral stats) */
        void drench_mut_calc();
        /** Recursively traverses the mutation's prerequisites and replacements, building up a map */
        void build_mut_dependency_map( const trait_id &mut,
                                       std::unordered_map<trait_id, int> &dependency_map, int distance );

        /**
        * Returns true if this category of mutation is allowed.
        */
        bool is_category_allowed( const std::vector<std::string> &category ) const;
        bool is_category_allowed( const std::string &category ) const;

        // In mutation.cpp
        /** Returns true if the player has the entered trait */
        bool has_trait( const trait_id &b ) const;
        /** Returns true if the player has the entered starting trait */
        bool has_base_trait( const trait_id &b ) const;
        /** Returns true if player has a trait with a flag */
        bool has_trait_flag( const std::string &b ) const;
        /** Returns the trait id with the given invlet, or an empty string if no trait has that invlet */
        trait_id trait_by_invlet( int ch ) const;

        /** Toggles a trait on the player and in their mutation list */
        void toggle_trait( Character &owner, const trait_id &flag );
        /** Add or removes a mutation on the player, but does not trigger mutation loss/gain effects. */
        void set_mutation( Character &owner, const trait_id &flag );
        void unset_mutation( Character &owner, const trait_id &flag );
        /** Returns true if the player has a conflicting trait to the entered trait
         *  Uses has_opposite_trait(), has_lower_trait(), and has_higher_trait() to determine conflicts.
         */
        bool has_conflicting_trait( const trait_id &flag ) const;
        /** Returns true if the player has a trait which cancels the entered trait */
        bool has_opposite_trait( const trait_id &flag ) const;
        /** Returns true if the player has a trait which upgrades into the entered trait */
        bool has_lower_trait( const trait_id &flag ) const;
        /** Returns true if the player has a trait which is an upgrade of the entered trait */
        bool has_higher_trait( const trait_id &flag ) const;
        /** Returns true if the player has a trait that shares a type with the entered trait */
        bool has_same_type_trait( const trait_id &flag ) const;
        /** Returns true if the player has crossed a mutation threshold
         *  Player can only cross one mutation threshold.
         */
        bool crossed_threshold() const;
        /** Returns true if the entered trait may be purified away
         *  Defaults to true
         */
        bool purifiable( const trait_id &flag ) const;

        void power_mutations();
        /** Returns a vector of valid mutation attacks */
        std::vector<special_attack> mutation_attacks( Creature &t ) const;
        /** Retrieves a stat mod of a mutation. */
        int get_mod( const trait_id &mut, const std::string &arg ) const;
        /** Empties the trait list */
        void empty_traits( Character &guy );
        /** Correction factor of the body temperature due to traits and mutations **/
        int bodytemp_modifier_traits( bool overheated ) const;
        /** Correction factor of the body temperature due to traits and mutations for player lying on the floor **/
        int bodytemp_modifier_traits_floor() const;
        /** Returns an enumeration of visible mutations with colors */
        std::string visible_mutations( int visibility_cap ) const;
        // gets modifiable trait data
        trait_data &get_trait_data( const trait_id &mut );
        trait_data get_trait_data( const trait_id &mut ) const;

        int get_cat_level( const std::string &category ) const;
        std::array<int, NUM_WATER_TOLERANCE> get_mut_drench( body_part bp ) const;
    private:
        /**
         * Traits / mutations of the character. Key is the mutation id (it's also a valid
         * key into @ref mutation_data), the value describes the status of the mutation.
         * If there is not entry for a mutation, the character does not have it. If the map
         * contains the entry, the character has the mutation.
         */
        std::unordered_map<trait_id, trait_data> my_mutations;
        /**
         * Contains mutation ids of the base traits.
         */
        std::unordered_set<trait_id> my_traits;
        /**
         * Pointers to mutation branches in @ref my_mutations.
         */
        std::vector<const mutation_branch *> cached_mutations;

        std::map<std::string, int> mutation_category_level;

        std::array<std::array<int, NUM_WATER_TOLERANCE>, num_bp> mut_drench;
};

#endif
