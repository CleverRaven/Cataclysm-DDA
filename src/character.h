#pragma once
#ifndef CHARACTER_H
#define CHARACTER_H

#include <cstddef>
#include <bitset>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <functional>
#include <limits>
#include <list>
#include <set>
#include <string>
#include <utility>

#include "bodypart.h"
#include "calendar.h"
#include "character_id.h"
#include "creature.h"
#include "game_constants.h"
#include "inventory.h"
#include "pimpl.h"
#include "pldata.h"
#include "visitable.h"
#include "color.h"
#include "damage.h"
#include "enums.h"
#include "item.h"
#include "optional.h"
#include "player_activity.h"
#include "stomach.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"
#include "point.h"
#include "magic_enchantment.h"

struct pathfinding_settings;
class item_location;
class SkillLevel;
class SkillLevelMap;

class JsonObject;
class JsonIn;
class JsonOut;
class vehicle;
struct mutation_branch;
class bionic_collection;
class player_morale;
struct points_left;
class faction;
struct construction;

enum vision_modes {
    DEBUG_NIGHTVISION,
    NV_GOGGLES,
    NIGHTVISION_1,
    NIGHTVISION_2,
    NIGHTVISION_3,
    FULL_ELFA_VISION,
    ELFA_VISION,
    CEPH_VISION,
    FELINE_VISION,
    BIRD_EYE,
    URSINE_VISION,
    BOOMERED,
    DARKNESS,
    IR_VISION,
    VISION_CLAIRVOYANCE,
    VISION_CLAIRVOYANCE_PLUS,
    VISION_CLAIRVOYANCE_SUPER,
    NUM_VISION_MODES
};

enum character_movemode : unsigned char {
    CMM_WALK = 0,
    CMM_RUN = 1,
    CMM_CROUCH = 2,
    CMM_COUNT
};

static const std::array< std::string, CMM_COUNT > character_movemode_str = { {
        "walk",
        "run",
        "crouch"
    }
};

enum fatigue_levels {
    TIRED = 191,
    DEAD_TIRED = 383,
    EXHAUSTED = 575,
    MASSIVE_FATIGUE = 1000
};
const std::unordered_map<std::string, fatigue_levels> fatigue_level_strs = { {
        { "TIRED", TIRED },
        { "DEAD_TIRED", DEAD_TIRED },
        { "EXHAUSTED", EXHAUSTED },
        { "MASSIVE_FATIGUE", MASSIVE_FATIGUE }
    }
};

// Sleep deprivation is defined in minutes, and although most calculations scale linearly,
// maluses are bestowed only upon reaching the tiers defined below.
enum sleep_deprivation_levels {
    SLEEP_DEPRIVATION_HARMLESS = 2 * 24 * 60,
    SLEEP_DEPRIVATION_MINOR = 4 * 24 * 60,
    SLEEP_DEPRIVATION_SERIOUS = 7 * 24 * 60,
    SLEEP_DEPRIVATION_MAJOR = 10 * 24 * 60,
    SLEEP_DEPRIVATION_MASSIVE = 14 * 24 * 60
};

struct layer_details {

    std::vector<int> pieces;
    int max = 0;
    int total = 0;

    void reset();
    int layer( int encumbrance );

    bool operator ==( const layer_details &rhs ) const {
        return max == rhs.max &&
               total == rhs.total &&
               pieces == rhs.pieces;
    }
};

struct encumbrance_data {
    int encumbrance = 0;
    int armor_encumbrance = 0;
    int layer_penalty = 0;

    std::array<layer_details, static_cast<size_t>( layer_level::MAX_CLOTHING_LAYER )>
    layer_penalty_details;

    void layer( const layer_level level, const int encumbrance ) {
        layer_penalty += layer_penalty_details[static_cast<size_t>( level )].layer( encumbrance );
    }

    void reset() {
        *this = encumbrance_data();
    }

    bool operator ==( const encumbrance_data &rhs ) const {
        return encumbrance == rhs.encumbrance &&
               armor_encumbrance == rhs.armor_encumbrance &&
               layer_penalty == rhs.layer_penalty &&
               layer_penalty_details == rhs.layer_penalty_details;
    }
};

struct aim_type {
    std::string name;
    std::string action;
    std::string help;
    bool has_threshold;
    int threshold;
};

struct social_modifiers {
    int lie = 0;
    int persuade = 0;
    int intimidate = 0;

    social_modifiers &operator+=( const social_modifiers &other ) {
        this->lie += other.lie;
        this->persuade += other.persuade;
        this->intimidate += other.intimidate;
        return *this;
    }
};
inline social_modifiers operator+( social_modifiers lhs, const social_modifiers &rhs )
{
    lhs += rhs;
    return lhs;
}

class Character : public Creature, public visitable<Character>
{
    public:
        Character( const Character & ) = delete;
        Character &operator=( const Character & ) = delete;
        ~Character() override;

        Character *as_character() override {
            return this;
        }
        const Character *as_character() const override {
            return this;
        }

        character_id getID() const;
        // sets the ID, will *only* succeed when the current id is not valid
        void setID( character_id i );

        field_type_id bloodType() const override;
        field_type_id gibType() const override;
        bool is_warm() const override;
        bool in_species( const species_id &spec ) const override;

        const std::string &symbol() const override;

        enum stat {
            STRENGTH,
            DEXTERITY,
            INTELLIGENCE,
            PERCEPTION,
            DUMMY_STAT
        };

        // Character stats
        // TODO: Make those protected
        int str_max;
        int dex_max;
        int int_max;
        int per_max;

        int str_cur;
        int dex_cur;
        int int_cur;
        int per_cur;

        // The prevalence of getter, setter, and mutator functions here is partially
        // a result of the slow, piece-wise migration of the player class upwards into
        // the character class. As enough logic is moved upwards to fully separate
        // utility upwards out of the player class, as many of these as possible should
        // be eliminated to allow for proper code separation. (Note: Not "all", many").
        /** Getters for stats exclusive to characters */
        virtual int get_str() const;
        virtual int get_dex() const;
        virtual int get_per() const;
        virtual int get_int() const;

        virtual int get_str_base() const;
        virtual int get_dex_base() const;
        virtual int get_per_base() const;
        virtual int get_int_base() const;

        virtual int get_str_bonus() const;
        virtual int get_dex_bonus() const;
        virtual int get_per_bonus() const;
        virtual int get_int_bonus() const;

        int get_speed() const override;

        // Penalty modifiers applied for ranged attacks due to low stats
        virtual int ranged_dex_mod() const;
        virtual int ranged_per_mod() const;

        /** Setters for stats exclusive to characters */
        virtual void set_str_bonus( int nstr );
        virtual void set_dex_bonus( int ndex );
        virtual void set_per_bonus( int nper );
        virtual void set_int_bonus( int nint );
        virtual void mod_str_bonus( int nstr );
        virtual void mod_dex_bonus( int ndex );
        virtual void mod_per_bonus( int nper );
        virtual void mod_int_bonus( int nint );

        /** Getters for health values exclusive to characters */
        virtual int get_healthy() const;
        virtual int get_healthy_mod() const;

        /** Modifiers for health values exclusive to characters */
        virtual void mod_healthy( int nhealthy );
        virtual void mod_healthy_mod( int nhealthy_mod, int cap );

        /** Setters for health values exclusive to characters */
        virtual void set_healthy( int nhealthy );
        virtual void set_healthy_mod( int nhealthy_mod );

        /** Getter for need values exclusive to characters */
        virtual int get_stored_kcal() const;
        virtual int get_healthy_kcal() const;
        virtual float get_kcal_percent() const;
        virtual int get_hunger() const;
        virtual int get_starvation() const;
        virtual int get_thirst() const;
        virtual std::pair<std::string, nc_color> get_thirst_description() const;
        virtual std::pair<std::string, nc_color> get_hunger_description() const;
        virtual std::pair<std::string, nc_color> get_fatigue_description() const;
        virtual int get_fatigue() const;
        virtual int get_sleep_deprivation() const;

        /** Modifiers for need values exclusive to characters */
        virtual void mod_stored_kcal( int nkcal );
        virtual void mod_stored_nutr( int nnutr );
        virtual void mod_hunger( int nhunger );
        virtual void mod_thirst( int nthirst );
        virtual void mod_fatigue( int nfatigue );
        virtual void mod_sleep_deprivation( int nsleep_deprivation );

        /** Setters for need values exclusive to characters */
        virtual void set_stored_kcal( int kcal );
        virtual void set_hunger( int nhunger );
        virtual void set_thirst( int nthirst );
        virtual void set_fatigue( int nfatigue );
        virtual void set_sleep_deprivation( int nsleep_deprivation );

        void mod_stat( const std::string &stat, float modifier ) override;

        /**Get bonus to max_hp from excess stored fat*/
        int get_fat_to_hp() const;

        /** Returns either "you" or the player's name */
        std::string disp_name( bool possessive = false ) const override;
        /** Returns the name of the player's outer layer, e.g. "armor plates" */
        std::string skin_name() const override;

        /* returns the character's faction */
        virtual faction *get_faction() const {
            return nullptr;
        }
        void set_fac_id( const std::string &my_fac_id );

        /* Adjusts provided sight dispersion to account for player stats */
        int effective_dispersion( int dispersion ) const;

        /* Accessors for aspects of aim speed. */
        std::vector<aim_type> get_aim_types( const item &gun ) const;
        std::pair<int, int> get_fastest_sight( const item &gun, double recoil ) const;
        int get_most_accurate_sight( const item &gun ) const;
        double aim_speed_skill_modifier( const skill_id &gun_skill ) const;
        double aim_speed_dex_modifier() const;
        double aim_speed_encumbrance_modifier() const;
        double aim_cap_from_volume( const item &gun ) const;

        /* Calculate aim improvement per move spent aiming at a given @ref recoil */
        double aim_per_move( const item &gun, double recoil ) const;

        /** Combat getters */
        float get_dodge_base() const override;
        float get_hit_base() const override;

        /** Handles health fluctuations over time */
        virtual void update_health( int external_modifiers = 0 );

        /** Resets the value of all bonus fields to 0. */
        void reset_bonuses() override;
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;
        /** Handles stat and bonus reset. */
        void reset() override;

        /** Picks a random body part, adjusting for mutations, broken body parts etc. */
        body_part get_random_body_part( bool main ) const override;
        /** Returns all body parts this character has, in order they should be displayed. */
        std::vector<body_part> get_all_body_parts( bool only_main = false ) const override;

        /** Recalculates encumbrance cache. */
        void reset_encumbrance();
        /** Returns ENC provided by armor, etc. */
        int encumb( body_part bp ) const;

        /** Returns body weight plus weight of inventory and worn/wielded items */
        units::mass get_weight() const override;
        /** Get encumbrance for all body parts. */
        std::array<encumbrance_data, num_bp> get_encumbrance() const;
        /** Get encumbrance for all body parts as if `new_item` was also worn. */
        std::array<encumbrance_data, num_bp> get_encumbrance( const item &new_item ) const;
        /** Get encumbrance penalty per layer & body part */
        int extraEncumbrance( layer_level level, int bp ) const;

        /** Returns true if the character is wearing power armor */
        bool is_wearing_power_armor( bool *hasHelmet = nullptr ) const;
        /** Returns true if the character is wearing active power */
        bool is_wearing_active_power_armor() const;
        /** Returns true if the player is wearing an active optical cloak */
        bool is_wearing_active_optcloak() const;

        /** Returns true if the player isn't able to see */
        bool is_blind() const;

        bool is_invisible() const;
        /** Checks is_invisible() as well as other factors */
        int visibility( bool check_color = false, int stillness = 0 ) const;

        /** Bitset of all the body parts covered only with items with `flag` (or nothing) */
        body_part_set exclusive_flag_coverage( const std::string &flag ) const;

        /** Processes effects which may prevent the Character from moving (bear traps, crushed, etc.).
         *  Returns false if movement is stopped. */
        bool move_effects( bool attacking ) override;
        /** Check against the character's current movement mode */
        bool movement_mode_is( character_movemode mode ) const;

        virtual void set_movement_mode( character_movemode mode ) = 0;

        /** Performs any Character-specific modifications to the arguments before passing to Creature::add_effect(). */
        void add_effect( const efftype_id &eff_id, time_duration dur, body_part bp = num_bp,
                         bool permanent = false,
                         int intensity = 0, bool force = false, bool deferred = false ) override;
        /**
         * Handles end-of-turn processing.
         */
        void process_turn() override;

        /** Recalculates HP after a change to max strength */
        void recalc_hp();
        /** Modifies the player's sight values
         *  Must be called when any of the following change:
         *  This must be called when any of the following change:
         * - effects
         * - bionics
         * - traits
         * - underwater
         * - clothes
         */
        void recalc_sight_limits();
        /**
         * Returns the apparent light level at which the player can see.
         * This is adjusted by the light level at the *character's* position
         * to simulate glare, etc, night vision only works if you are in the dark.
         */
        float get_vision_threshold( float light_level ) const;
        /**
         * Flag encumbrance for updating.
        */
        void flag_encumbrance();
        /**
         * Checks worn items for the "RESET_ENCUMBRANCE" flag, which indicates
         * that encumbrance may have changed and require recalculating.
         */
        void check_item_encumbrance_flag();

        // any side effects that might happen when the Character is hit
        void on_hit( Creature *source, body_part /*bp_hit*/,
                     float /*difficulty*/, dealt_projectile_attack const * /*proj*/ ) override;
        // any side effects that might happen when the Character hits a Creature
        void did_hit( Creature &target );

        /**
         * Check for relevant passive, non-clothing that can absorb damage, and reduce by specified
         * damage unit.  Only flat bonuses are checked here.  Multiplicative ones are checked in
         * @ref player::absorb_hit.  The damage amount will never be reduced to less than 0.
         * This is called from @ref player::absorb_hit
         */
        void passive_absorb_hit( body_part bp, damage_unit &du ) const;
        /** Runs through all bionics and armor on a part and reduces damage through their armor_absorb */
        void absorb_hit( body_part bp, damage_instance &dam ) override;
        /**
         * Reduces and mutates du, prints messages about armor taking damage.
         * @return true if the armor was completely destroyed (and the item must be deleted).
         */
        bool armor_absorb( damage_unit &du, item &armor );
        /**
         * Check for passive bionics that provide armor, and returns the armor bonus
         * This is called from player::passive_absorb_hit
         */
        float bionic_armor_bonus( body_part bp, damage_type dt ) const;
        /** Returns the armor bonus against given type from martial arts buffs */
        int mabuff_armor_bonus( damage_type type ) const;
        // --------------- Mutation Stuff ---------------
        // In newcharacter.cpp
        /** Returns the id of a random starting trait that costs >= 0 points */
        trait_id random_good_trait();
        /** Returns the id of a random starting trait that costs < 0 points */
        trait_id random_bad_trait();

        // In mutation.cpp
        /** Returns true if the player has the entered trait */
        bool has_trait( const trait_id &b ) const override;
        /** Returns true if the player has the entered starting trait */
        bool has_base_trait( const trait_id &b ) const;
        /** Returns true if player has a trait with a flag */
        bool has_trait_flag( const std::string &b ) const;
        /** Returns the trait id with the given invlet, or an empty string if no trait has that invlet */
        trait_id trait_by_invlet( int ch ) const;

        /** Toggles a trait on the player and in their mutation list */
        void toggle_trait( const trait_id &flag );
        /** Add or removes a mutation on the player, but does not trigger mutation loss/gain effects. */
        void set_mutation( const trait_id &flag );
        void unset_mutation( const trait_id &flag );

        /** Converts a body_part to an hp_part */
        static hp_part bp_to_hp( body_part bp );
        /** Converts an hp_part to a body_part */
        static body_part hp_to_bp( hp_part hpart );

        bool can_mount( const monster &critter ) const;
        void mount_creature( monster &z );
        bool is_mounted() const;
        void dismount();
        void forced_dismount();

        /** Returns true if the player has two functioning arms */
        bool has_two_arms() const;
        /** Returns the number of functioning arms */
        int get_working_arm_count() const;
        /** Returns the number of functioning legs */
        int get_working_leg_count() const;
        /** Returns true if the limb is disabled(12.5% or less hp)*/
        bool is_limb_disabled( hp_part limb ) const;
        /** Returns true if the limb is hindered(40% or less hp) */
        bool is_limb_hindered( hp_part limb ) const;
        /** Returns true if the limb is broken */
        bool is_limb_broken( hp_part limb ) const;
        /** source of truth of whether a Character can run */
        bool can_run();
        /** Hurts all body parts for dam, no armor reduction */
        void hurtall( int dam, Creature *source, bool disturb = true );
        /** Harms all body parts for dam, with armor reduction. If vary > 0 damage to parts are random within vary % (1-100) */
        int hitall( int dam, int vary, Creature *source );
        /** Handles effects that happen when the player is damaged and aware of the fact. */
        void on_hurt( Creature *source, bool disturb = true );
        /** Heals a body_part for dam */
        void heal( body_part healed, int dam );
        /** Heals an hp_part for dam */
        void heal( hp_part healed, int dam );
        /** Heals all body parts for dam */
        void healall( int dam );
        /**
         * Displays menu with body part hp, optionally with hp estimation after healing.
         * Returns selected part.
         * menu_header - name of item that triggers this menu
         * show_all - show and enable choice of all limbs, not only healable
         * precise - show numerical hp
         * normal_bonus - heal normal limb
         * head_bonus - heal head
         * torso_bonus - heal torso
         * bleed - chance to stop bleeding
         * bite - chance to remove bite
         * infect - chance to remove infection
         * bandage_power - quality of bandage
         * disinfectant_power - quality of disinfectant
         */
        hp_part body_window( const std::string &menu_header,
                             bool show_all, bool precise,
                             int normal_bonus, int head_bonus, int torso_bonus,
                             float bleed, float bite, float infect, float bandage_power, float disinfectant_power ) const;

        // Returns color which this limb would have in healing menus
        nc_color limb_color( body_part bp, bool bleed, bool bite, bool infect ) const;

        static const std::vector<material_id> fleshy;
        bool made_of( const material_id &m ) const override;
        bool made_of_any( const std::set<material_id> &ms ) const override;

        // Drench cache
        enum water_tolerance {
            WT_IGNORED = 0,
            WT_NEUTRAL,
            WT_GOOD,
            NUM_WATER_TOLERANCE
        };
        inline int posx() const override {
            return position.x;
        }
        inline int posy() const override {
            return position.y;
        }
        inline int posz() const override {
            return position.z;
        }
        inline void setx( int x ) {
            setpos( tripoint( x, position.y, position.z ) );
        }
        inline void sety( int y ) {
            setpos( tripoint( position.x, y, position.z ) );
        }
        inline void setz( int z ) {
            setpos( tripoint( position.xy(), z ) );
        }
        inline void setpos( const tripoint &p ) override {
            position = p;
        }

        /**
         * Global position, expressed in map square coordinate system
         * (the most detailed coordinate system), used by the @ref map.
         */
        virtual tripoint global_square_location() const;
        /**
        * Returns the location of the player in global submap coordinates.
        */
        tripoint global_sm_location() const;
        /**
        * Returns the location of the player in global overmap terrain coordinates.
        */
        tripoint global_omt_location() const;

    private:
        /** Retrieves a stat mod of a mutation. */
        int get_mod( const trait_id &mut, const std::string &arg ) const;
        /** Applies skill-based boosts to stats **/
        void apply_skill_boost();
    protected:
        /** Applies stat mods to character. */
        void apply_mods( const trait_id &mut, bool add_remove );

        /** Recalculate encumbrance for all body parts. */
        std::array<encumbrance_data, num_bp> calc_encumbrance() const;
        /** Recalculate encumbrance for all body parts as if `new_item` was also worn. */
        std::array<encumbrance_data, num_bp> calc_encumbrance( const item &new_item ) const;

        /** Applies encumbrance from mutations and bionics only */
        void mut_cbm_encumb( std::array<encumbrance_data, num_bp> &vals ) const;

        /** Return the position in the worn list where new_item would be
         * put by default */
        std::list<item>::iterator position_to_wear_new_item( const item &new_item );

        /** Applies encumbrance from items only
         * If new_item is not null, then calculate under the asumption that it
         * is added to existing work items. */
        void item_encumb( std::array<encumbrance_data, num_bp> &vals,
                          const item &new_item ) const;

        std::array<std::array<int, NUM_WATER_TOLERANCE>, num_bp> mut_drench;
    public:
        // recalculates enchantment cache by iterating through all held, worn, and wielded items
        void recalculate_enchantment_cache();
        // gets add and mult value from enchantment cache
        double calculate_by_enchantment( double modify, enchantment::mod value,
                                         bool round_output = false ) const;
        /** Handles things like destruction of armor, etc. */
        void mutation_effect( const trait_id &mut );
        /** Handles what happens when you lose a mutation. */
        void mutation_loss_effect( const trait_id &mut );

        bool has_active_mutation( const trait_id &b ) const;
        /** Picks a random valid mutation and gives it to the Character, possibly removing/changing others along the way */
        void mutate();
        /** Returns true if the player doesn't have the mutation or a conflicting one and it complies with the force typing */
        bool mutation_ok( const trait_id &mutation, bool force_good, bool force_bad ) const;
        /** Picks a random valid mutation in a category and mutate_towards() it */
        void mutate_category( const std::string &mut_cat );
        /** Mutates toward one of the given mutations, upgrading or removing conflicts if necessary */
        bool mutate_towards( std::vector<trait_id> muts, int num_tries = INT_MAX );
        /** Mutates toward the entered mutation, upgrading or removing conflicts if necessary */
        bool mutate_towards( const trait_id &mut );
        /** Removes a mutation, downgrading to the previous level if possible */
        void remove_mutation( const trait_id &mut, bool silent = false );
        /** Returns true if the player has the entered mutation child flag */
        bool has_child_flag( const trait_id &flag ) const;
        /** Removes the mutation's child flag from the player's list */
        void remove_child_flag( const trait_id &flag );
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

        bool is_weak_to_water() const;

        /**Check for mutation disallowing the use of an healing item*/
        bool can_use_heal_item( const item &med ) const;

        bool can_install_cbm_on_bp( const std::vector<body_part> &bps ) const;

        /**
         * Returns resistances on a body part provided by mutations
         */
        // TODO: Cache this, it's kinda expensive to compute
        resistances mutation_armor( body_part bp ) const;
        float mutation_armor( body_part bp, damage_type dt ) const;
        float mutation_armor( body_part bp, const damage_unit &du ) const;

        // --------------- Bionic Stuff ---------------
        std::vector<bionic_id> get_bionics() const;
        /** Returns true if the player has the entered bionic id */
        bool has_bionic( const bionic_id &b ) const;
        /** Returns true if the player has the entered bionic id and it is powered on */
        bool has_active_bionic( const bionic_id &b ) const;
        /**Returns true if the player has any bionic*/
        bool has_any_bionic() const;
        /**Returns true if the character can fuel a bionic with the item*/
        bool can_fuel_bionic_with( const item &it ) const;
        /**Return bionic_id of bionics able to use it as fuel*/
        std::vector<bionic_id> get_bionic_fueled_with( const item &it ) const;
        /**Return bionic_id of fueled bionics*/
        std::vector<bionic_id> get_fueled_bionics() const;
        /**Return bionic_id of bionic of most fuel efficient bionic*/
        bionic_id get_most_efficient_bionic( const std::vector<bionic_id> &bids ) const;
        /**Return list of available fuel for this bionic*/
        std::vector<itype_id> get_fuel_available( const bionic_id &bio ) const;
        /**Return available space to store specified fuel*/
        int get_fuel_capacity( itype_id fuel ) const;
        /**Return total space to store specified fuel*/
        int get_total_fuel_capacity( itype_id fuel ) const;
        /**Updates which bionic contain fuel and which is empty*/
        void update_fuel_storage( const itype_id &fuel );
        /**Get stat bonus from bionic*/
        int get_mod_stat_from_bionic( const Character::stat &Stat ) const;
        // route for overmap-scale travelling
        std::vector<tripoint> omt_path;

        units::energy get_power_level() const;
        units::energy get_max_power_level() const;
        void mod_power_level( units::energy npower );
        void mod_max_power_level( units::energy npower_max );
        void set_power_level( units::energy npower );
        void set_max_power_level( units::energy npower_max );
        bool is_max_power() const;
        bool has_power() const;
        bool has_max_power() const;
        bool enough_power_for( const bionic_id &bid ) const;
        // --------------- Generic Item Stuff ---------------

        struct has_mission_item_filter {
            int mission_id;
            bool operator()( const item &it ) {
                return it.mission_id == mission_id;
            }
        };

        // -2 position is 0 worn index, -3 position is 1 worn index, etc
        static int worn_position_to_index( int position ) {
            return -2 - position;
        }

        // checks to see if an item is worn
        bool is_worn( const item &thing ) const {
            for( const auto &elem : worn ) {
                if( &thing == &elem ) {
                    return true;
                }
            }
            return false;
        }

        /**
         * Calculate (but do not deduct) the number of moves required when handling (e.g. storing, drawing etc.) an item
         * @param it Item to calculate handling cost for
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */
        int item_handling_cost( const item &it, bool penalties = true,
                                int base_cost = INVENTORY_HANDLING_PENALTY ) const;

        /**
         * Calculate (but do not deduct) the number of moves required when storing an item in a container
         * @param it Item to calculate storage cost for
         * @param container Container to store item in
         * @param penalties Whether item volume and temporary effects (e.g. GRABBED, DOWNED) should be considered.
         * @param base_cost Cost due to storage type.
         * @return cost in moves ranging from 0 to MAX_HANDLING_COST
         */
        int item_store_cost( const item &it, const item &container, bool penalties = true,
                             int base_cost = INVENTORY_HANDLING_PENALTY ) const;

        /** Returns nearby items which match the provided predicate */
        std::vector<item_location> nearby( const std::function<bool( const item *, const item * )> &func,
                                           int radius = 1 ) const;

        /**
         * Similar to @ref remove_items_with, but considers only worn items and not their
         * content (@ref item::contents is not checked).
         * If the filter function returns true, the item is removed.
         */
        std::list<item> remove_worn_items_with( std::function<bool( item & )> filter );

        item &i_at( int position ); // Returns the item with a given inventory position.
        const item &i_at( int position ) const;
        /**
         * Returns the item position (suitable for @ref i_at or similar) of a
         * specific item. Returns INT_MIN if the item is not found.
         * Note that this may lose some information, for example the returned position is the
         * same when the given item points to the container and when it points to the item inside
         * the container. All items that are part of the same stack have the same item position.
         */
        int get_item_position( const item *it ) const;

        /**
         * Returns a reference to the item which will be used to make attacks.
         * At the moment it's always @ref weapon or a reference to a null item.
         */
        /*@{*/
        const item &used_weapon() const;
        item &used_weapon();
        /*@}*/

        /**
         * Try to find a container/s on character containing ammo of type it.typeId() and
         * add charges until the container is full.
         * @param unloading Do not try to add to a container when the item was intentionally unloaded.
         * @return Remaining charges which could not be stored in a container.
         */
        int i_add_to_container( const item &it, bool unloading );
        item &i_add( item it, bool should_stack = true );

        /**
         * Try to pour the given liquid into the given container/vehicle. The transferred charges are
         * removed from the liquid item. Check the charges of afterwards to see if anything has
         * been transferred at all.
         * The functions do not consume any move points.
         * @return Whether anything has been moved at all. `false` indicates the transfer is not
         * possible at all. `true` indicates at least some of the liquid has been moved.
         */
        /**@{*/
        bool pour_into( item &container, item &liquid );
        bool pour_into( vehicle &veh, item &liquid );
        /**@}*/

        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param pos The item position of the item to be removed. The item *must*
         * exists, use @ref has_item to check this.
         * @return A copy of the removed item.
         */
        item i_rem( int pos );
        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in the players possession (one can use @ref has_item to check for this).
         * @return A copy of the removed item.
         */
        item i_rem( const item *it );
        void i_rem_keep_contents( int pos );
        /** Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
         *  An optional qty can be provided (and will perform better than separate calls). */
        bool i_add_or_drop( item &it, int qty = 1 );

        /** Only use for UI things. Returns all invlets that are currently used in
         * the player inventory, the weapon slot and the worn items. */
        std::bitset<std::numeric_limits<char>::max()> allocated_invlets() const;

        /**
         * Whether the player carries an active item of the given item type.
         */
        bool has_active_item( const itype_id &id ) const;
        item remove_weapon();
        void remove_mission_items( int mission_id );

        /**
         * Returns the items that are ammo and have the matching ammo type.
         */
        std::vector<const item *> get_ammo( const ammotype &at ) const;

        /**
         * Searches for ammo or magazines that can be used to reload obj
         * @param obj item to be reloaded. By design any currently loaded ammunition or magazine is ignored
         * @param empty whether empty magazines should be considered as possible ammo
         * @param radius adjacent map/vehicle tiles to search. 0 for only player tile, -1 for only inventory
         */
        std::vector<item_location> find_ammo( const item &obj, bool empty = true, int radius = 1 ) const;

        /**
         * Searches for weapons and magazines that can be reloaded.
         */
        std::vector<item_location> find_reloadables();
        /**
         * Counts ammo and UPS charges (lower of) for a given gun on the character.
         */
        int ammo_count_for( const item &gun );

        /** Maximum thrown range with a given item, taking all active effects into account. */
        int throw_range( const item & ) const;
        /** Dispersion of a thrown item, against a given target, taking into account whether or not the throw was blind. */
        int throwing_dispersion( const item &to_throw, Creature *critter = nullptr,
                                 bool is_blind_throw = false ) const;
        /** How much dispersion does one point of target's dodge add when throwing at said target? */
        int throw_dispersion_per_dodge( bool add_encumbrance = true ) const;

        /// Checks for items, tools, and vehicles with the Lifting quality near the character
        /// returning the highest quality in range.
        int best_nearby_lifting_assist() const;

        /// Alternate version if you need to specify a different orign point for nearby vehicle sources of lifting
        /// used for operations on distant objects (e.g. vehicle installation/uninstallation)
        int best_nearby_lifting_assist( const tripoint &world_pos ) const;

        units::mass weight_carried() const;
        units::volume volume_carried() const;

        /// Sometimes we need to calculate hypothetical volume or weight.  This
        /// struct offers two possible tweaks: a collection of items and
        /// coutnts to remove, or an entire replacement inventory.
        struct item_tweaks {
            item_tweaks() = default;
            item_tweaks( const std::map<const item *, int> &w ) :
                without_items( std::cref( w ) )
            {}
            item_tweaks( const inventory &r ) :
                replace_inv( std::cref( r ) )
            {}
            const cata::optional<std::reference_wrapper<const std::map<const item *, int>>> without_items;
            const cata::optional<std::reference_wrapper<const inventory>> replace_inv;
        };

        units::mass weight_carried_with_tweaks( const item_tweaks & ) const;
        units::volume volume_carried_with_tweaks( const item_tweaks & ) const;
        units::mass weight_capacity() const override;
        units::volume volume_capacity() const;
        units::volume volume_capacity_reduced_by(
            const units::volume &mod,
            const std::map<const item *, int> &without_items = {} ) const;

        bool can_pickVolume( const item &it, bool safe = false ) const;
        bool can_pickWeight( const item &it, bool safe = true ) const;
        /**
         * Checks if character stats and skills meet minimum requirements for the item.
         * Prints an appropriate message if requirements not met.
         * @param it Item we are checking
         * @param context optionally override effective item when checking contextual skills
         */
        bool can_use( const item &it, const item &context = item() ) const;
        /**
         * Returns true if the character is wielding something.
         * Note: this item may not actually be used to attack.
         */
        bool is_armed() const;

        /**
         * Removes currently wielded item (if any) and replaces it with the target item.
         * @param target replacement item to wield or null item to remove existing weapon without replacing it
         * @return whether both removal and replacement were successful (they are performed atomically)
         */
        virtual bool wield( item &target ) = 0;

        void drop_invalid_inventory();

        virtual bool has_artifact_with( art_effect_passive effect ) const;

        // --------------- Clothing Stuff ---------------
        /** Returns true if the player is wearing the item. */
        bool is_wearing( const itype_id &it ) const;
        /** Returns true if the player is wearing the item on the given body_part. */
        bool is_wearing_on_bp( const itype_id &it, body_part bp ) const;
        /** Returns true if the player is wearing an item with the given flag. */
        bool worn_with_flag( const std::string &flag, body_part bp = num_bp ) const;


        // drawing related stuff
        /**
         * Returns a list of the IDs of overlays on this character,
         * sorted from "lowest" to "highest".
         *
         * Only required for rendering.
         */
        std::vector<std::string> get_overlay_ids() const;

        // --------------- Skill Stuff ---------------
        int get_skill_level( const skill_id &ident ) const;
        int get_skill_level( const skill_id &ident, const item &context ) const;

        const SkillLevelMap &get_all_skills() const;
        SkillLevel &get_skill_level_object( const skill_id &ident );
        const SkillLevel &get_skill_level_object( const skill_id &ident ) const;

        void set_skill_level( const skill_id &ident, int level );
        void mod_skill_level( const skill_id &ident, int delta );
        /** Checks whether the character's skills meet the required */
        bool meets_skill_requirements( const std::map<skill_id, int> &req,
                                       const item &context = item() ) const;
        /** Checks whether the character's skills meet the required */
        bool meets_skill_requirements( const construction &con ) const;
        /** Checks whether the character's stats meets the stats required by the item */
        bool meets_stat_requirements( const item &it ) const;
        /** Checks whether the character meets overall requirements to be able to use the item */
        bool meets_requirements( const item &it, const item &context = item() ) const;
        /** Returns a string of missed requirements (both stats and skills) */
        std::string enumerate_unmet_requirements( const item &it, const item &context = item() ) const;

        // --------------- Other Stuff ---------------

        /** return the calendar::turn the character expired */
        time_point get_time_died() const {
            return time_died;
        }
        /** set the turn the turn the character died if not already done */
        void set_time_died( const time_point &time ) {
            if( time_died != calendar::before_time_starts ) {
                time_died = time;
            }
        }
        // magic mod
        known_magic magic;

        void make_bleed( body_part bp, time_duration duration, int intensity = 1,
                         bool permanent = false,
                         bool force = false, bool defferred = false );

        /** Calls Creature::normalize()
         *  nulls out the player's weapon
         *  Should only be called through player::normalize(), not on it's own!
         */
        void normalize() override;
        void die( Creature *nkiller ) override;

        std::string get_name() const override;

        std::vector<std::string> get_grammatical_genders() const override;

        /**
         * It is supposed to hide the query_yn to simplify player vs. npc code.
         */
        template<typename ...Args>
        bool query_yn( const char *const msg, Args &&... args ) const {
            return query_yn( string_format( msg, std::forward<Args>( args ) ... ) );
        }
        virtual bool query_yn( const std::string &msg ) const = 0;

        bool is_immune_field( field_type_id fid ) const override;

        /** Returns true if the player has some form of night vision */
        bool has_nv();

        /**
         * Returns >0 if character is sitting/lying and relatively inactive.
         * 1 represents sleep on comfortable bed, so anything above that should be rare.
         */
        float rest_quality() const;
        /**
         * Average hit points healed per turn.
         */
        float healing_rate( float at_rest_quality ) const;
        /**
         * Average hit points healed per turn from healing effects.
         */
        float healing_rate_medicine( float at_rest_quality, body_part bp ) const;

        /**
         * Goes over all mutations, gets min and max of a value with given name
         * @return min( 0, lowest ) + max( 0, highest )
         */
        float mutation_value( const std::string &val ) const;

        /**
         * Goes over all mutations, returning the sum of the social modifiers
         */
        social_modifiers get_mutation_social_mods() const;

        /** Color's character's tile's background */
        nc_color symbol_color() const override;

        std::string extended_description() const override;

        // In newcharacter.cpp
        void empty_skills();
        /** Returns a random name from NAMES_* */
        void pick_name( bool bUseDefault = false );
        /** Get the idents of all base traits. */
        std::vector<trait_id> get_base_traits() const;
        /** Get the idents of all traits/mutations. */
        std::vector<trait_id> get_mutations( bool include_hidden = true ) const;
        const std::bitset<NUM_VISION_MODES> &get_vision_modes() const {
            return vision_mode_cache;
        }
        /** Empties the trait list */
        void empty_traits();
        /**
         * Adds mandatory scenario and profession traits unless you already have them
         * And if you do already have them, refunds the points for the trait
         */
        void add_traits();
        void add_traits( points_left &points );
        /** Returns true if the player has crossed a mutation threshold
         *  Player can only cross one mutation threshold.
         */
        bool crossed_threshold() const;

        // --------------- Values ---------------
        std::string name;
        bool male;

        std::list<item> worn;
        std::array<int, num_hp_parts> hp_cur, hp_max, damage_bandaged, damage_disinfected;
        bool nv_cached;
        // Means player sit inside vehicle on the tile he is now
        bool in_vehicle;
        bool hauling;

        player_activity activity;
        std::list<player_activity> backlog;
        inventory inv;
        itype_id last_item;
        item weapon;

        pimpl<bionic_collection> my_bionics;

        stomach_contents stomach;
        stomach_contents guts;

        int oxygen;
        int radiation;

        std::shared_ptr<monster> mounted_creature;
        // for loading NPC mounts
        int mounted_creature_id;
        // for vehicle work
        int activity_vehicle_part_index = -1;

        // Hauling items on the ground
        void start_hauling();
        void stop_hauling();
        bool is_hauling() const;

        // Has a weapon, inventory item or worn item with flag
        bool has_item_with_flag( const std::string &flag, bool need_charges = false ) const;
        /**
         * All items that have the given flag (@ref item::has_flag).
         */
        std::vector<const item *> all_items_with_flag( const std::string &flag ) const;

        bool has_fire( int quantity ) const;

        bool has_charges( const itype_id &it, int quantity,
                          const std::function<bool( const item & )> &filter = return_true<item> ) const;

        /** Legacy activity assignment, should not be used where resuming is important. */
        void assign_activity( const activity_id &type, int moves = calendar::INDEFINITELY_LONG,
                              int index = -1, int pos = INT_MIN,
                              const std::string &name = "" );
        /** Assigns activity to player, possibly resuming old activity if it's similar enough. */
        void assign_activity( const player_activity &act, bool allow_resume = true );
        /** Check if player currently has a given activity */
        bool has_activity( const activity_id &type ) const;
        /** Check if player currently has any of the given activities */
        bool has_activity( const std::vector<activity_id> &types ) const;
        void resume_backlog_activity();
        void cancel_activity();

        void initialize_stomach_contents();

        /** Stable base metabolic rate due to traits */
        float metabolic_rate_base() const;
        // gets the max value healthy you can be, related to your weight
        int get_max_healthy() const;
        // gets the string that describes your weight
        std::string get_weight_string() const;
        // gets the description, printed in player_display, related to your current bmi
        std::string get_weight_description() const;
        // calculates the BMI
        float get_bmi() const;
        // returns amount of calories burned in a day given various metabolic factors
        int get_bmr() const;
        // returns the height of the player character in cm
        int height() const;
        // returns bodyweight of the Character
        units::mass bodyweight() const;
        // returns total weight of installed bionics
        units::mass bionics_weight() const;
        // increases the activity level to the next level
        // does not decrease activity level
        void increase_activity_level( float new_level );
        // decreases the activity level to the previous level
        // does not increase activity level
        void decrease_activity_level( float new_level );
        // sets activity level to NO_EXERCISE
        void reset_activity_level();
        // outputs player activity level to a printable string
        std::string activity_level_str() const;

        int get_stim() const;
        void set_stim( int new_stim );
        void mod_stim( int mod );

        int get_stamina() const;
        int get_stamina_max() const;
        void set_stamina( int new_stamina );
        void mod_stamina( int mod );
        void burn_move_stamina( int moves );
        /** Regenerates stamina */
        void update_stamina( int turns );

    protected:
        void on_stat_change( const std::string &, int ) override {}
        void on_damage_of_type( int adjusted_damage, damage_type type, body_part bp ) override;
        virtual void on_mutation_gain( const trait_id & ) {}
        virtual void on_mutation_loss( const trait_id & ) {}
    public:
        virtual void on_item_wear( const item & ) {}
        virtual void on_item_takeoff( const item & ) {}
        virtual void on_worn_item_washed( const item & ) {}

        bool is_wielding( const item &target ) const;

        /** Returns an unoccupied, safe adjacent point. If none exists, returns player position. */
        tripoint adjacent_tile() const;

        /** Removes "sleep" and "lying_down" */
        void wake_up();
        // how loud a character can shout. based on mutations and clothing
        int get_shout_volume() const;
        // shouts a message
        void shout( std::string msg = "", bool order = false );
        /** Handles Character vomiting effects */
        void vomit();
        // adds total healing to the bodypart. this is only a counter.
        void healed_bp( int bp, int amount );

        // the amount healed per bodypart per day
        std::array<int, num_hp_parts> healed_total;

        std::map<std::string, int> mutation_category_level;

        /** Modifies intensity of painkillers  */
        void mod_painkiller( int npkill );
        /** Sets intensity of painkillers  */
        void set_painkiller( int npkill );
        /** Returns intensity of painkillers  */
        int get_painkiller() const;
        void react_to_felt_pain( int intensity );

        void spores();
        void blossoms();

        /** Handles rooting effects */
        void rooted_message() const;
        void rooted();

        /** Adds "sleep" to the player */
        void fall_asleep();
        void fall_asleep( const time_duration &duration );
        /** Checks to see if the player is using floor items to keep warm, and return the name of one such item if so */
        std::string is_snuggling() const;

        player_activity get_destination_activity() const;
        void set_destination_activity( const player_activity &new_destination_activity );
        void clear_destination_activity();
        /** Set vitamin deficiency/excess disease states dependent upon current vitamin levels */
        void update_vitamins( const vitamin_id &vit );
        /**
         * Check current level of a vitamin
         *
         * Accesses level of a given vitamin.  If the vitamin_id specified does not
         * exist then this function simply returns 0.
         *
         * @param vit ID of vitamin to check level for.
         * @returns current level for specified vitamin
         */
        int vitamin_get( const vitamin_id &vit ) const;
        /**
         * Add or subtract vitamins from player storage pools
         * @param vit ID of vitamin to modify
         * @param qty amount by which to adjust vitamin (negative values are permitted)
         * @param capped if true prevent vitamins which can accumulate in excess from doing so
         * @return adjusted level for the vitamin or zero if vitamin does not exist
         */
        int vitamin_mod( const vitamin_id &vit, int qty, bool capped = true );

        /** Returns true if the player is wearing something on the entered body_part */
        bool wearing_something_on( body_part bp ) const;
        /** Returns 1 if the player is wearing something on both feet, .5 if on one, and 0 if on neither */
        double footwear_factor() const;
        /** Returns true if the player is wearing something on their feet that is not SKINTIGHT */
        bool is_wearing_shoes( const side &which_side = side::BOTH ) const;

        /** Ticks down morale counters and removes them */
        void update_morale();
        /** Ensures persistent morale effects are up-to-date */
        void apply_persistent_morale();
        /** Used to apply morale modifications from food and medication **/
        void modify_morale( item &food, int nutr = 0 );
        int get_morale_level() const; // Modified by traits, &c
        void add_morale( const morale_type &type, int bonus, int max_bonus = 0,
                         const time_duration &duration = 1_hours,
                         const time_duration &decay_start = 30_minutes, bool capped = false,
                         const itype *item_type = nullptr );
        int has_morale( const morale_type &type ) const;
        void rem_morale( const morale_type &type, const itype *item_type = nullptr );
        void clear_morale();
        bool has_morale_to_read() const;
        bool has_morale_to_craft() const;
        /** Checks permanent morale for consistency and recovers it when an inconsistency is found. */
        void check_and_recover_morale();

        /** Handles the enjoyability value for a comestible. First value is enjoyability, second is cap. **/
        std::pair<int, int> fun_for( const item &comest ) const;

    protected:
        Character();
        Character( Character && );
        Character &operator=( Character && );
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

        // The player's position on the local map.
        tripoint position;

        /** Bonuses to stats, calculated each turn */
        int str_bonus;
        int dex_bonus;
        int per_bonus;
        int int_bonus;

        /** How healthy the character is. */
        int healthy;
        int healthy_mod;

        /**height at character creation*/
        int init_height = 175;

        // the player's activity level for metabolism calculations
        float activity_level = NO_EXERCISE;

        std::array<encumbrance_data, num_bp> encumbrance_cache;
        mutable std::map<std::string, double> cached_info;

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

        void store( JsonOut &json ) const;
        void load( JsonObject &data );

        // --------------- Values ---------------
        pimpl<SkillLevelMap> _skills;

        // Cached vision values.
        std::bitset<NUM_VISION_MODES> vision_mode_cache;
        int sight_max;

        // turn the character expired, if calendar::before_time_starts it has not been set yet.
        // TODO: change into an optional<time_point>
        time_point time_died = calendar::before_time_starts;

        /**
         * Cache for pathfinding settings.
         * Most of it isn't changed too often, hence mutable.
         */
        mutable pimpl<pathfinding_settings> path_settings;

        // faction API versions
        // 2 - allies are in your_followers faction; NPCATT_FOLLOW is follower but not an ally
        // 0 - allies may be in your_followers faction; NPCATT_FOLLOW is an ally (legacy)
        int faction_api_version = 2;  // faction API versioning
        faction_id fac_id; // A temp variable used to inform the game which faction to link
        faction *my_fac = nullptr;

        character_movemode move_mode;
        /** Current deficiency/excess quantity for each vitamin */
        std::map<vitamin_id, int> vitamin_levels;

        pimpl<player_morale> morale;

    private:
        // a cache of all active enchantment values.
        // is recalculated every turn in Character::recalculate_enchantment_cache
        enchantment enchantment_cache;
        player_activity destination_activity;
        // A unique ID number, assigned by the game class. Values should never be reused.
        character_id id;

        units::energy power_level;
        units::energy max_power_level;

        /** Needs (hunger, starvation, thirst, fatigue, etc.) */
        int stored_calories;
        int healthy_calories;

        int hunger;
        int thirst;
        int stamina;

        int fatigue;
        int sleep_deprivation;
        bool check_encumbrance;

        int stim;
        int pkill;

    protected:
        /** Amount of time the player has spent in each overmap tile. */
        std::unordered_map<point, time_duration> overmap_time;
};

template<>
struct enum_traits<Character::stat> {
    static constexpr Character::stat last = Character::stat::DUMMY_STAT;
};
/**Get translated name of a stat*/
std::string get_stat_name( Character::stat Stat );
#endif
