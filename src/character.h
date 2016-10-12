#ifndef CHARACTER_H
#define CHARACTER_H

#include "visitable.h"
#include "creature.h"
#include "inventory.h"
#include "bionics.h"
#include "skill.h"
#include "map_selector.h"
#include "pathfinding.h"

#include <map>

using skill_id = string_id<Skill>;
enum field_id : int;
class field;
class field_entry;
class vehicle;
struct resistances;

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
    VISION_CLAIRVOYANCE_SUPER,
    NUM_VISION_MODES
};

enum fatigue_levels {
    TIRED = 191,
    DEAD_TIRED = 383,
    EXHAUSTED = 575,
    MASSIVE_FATIGUE = 1000
};

struct encumbrance_data {
    int encumbrance = 0;
    int armor_encumbrance = 0;
    int layer_penalty = 0;
    bool operator ==( const encumbrance_data &rhs ) const
    {
        return encumbrance == rhs.encumbrance &&
               armor_encumbrance == rhs.armor_encumbrance &&
               layer_penalty == rhs.layer_penalty;
    }
};

class Character : public Creature, public visitable<Character>
{
    public:
        ~Character() override { };

        field_id bloodType() const override;
        field_id gibType() const override;
        bool is_warm() const override;
        virtual const std::string &symbol() const override;

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

        /** Setters for stats exclusive to characters */
        virtual void set_str_bonus(int nstr);
        virtual void set_dex_bonus(int ndex);
        virtual void set_per_bonus(int nper);
        virtual void set_int_bonus(int nint);
        virtual void mod_str_bonus(int nstr);
        virtual void mod_dex_bonus(int ndex);
        virtual void mod_per_bonus(int nper);
        virtual void mod_int_bonus(int nint);

        /** Getters for health values exclusive to characters */
        virtual int get_healthy() const;
        virtual int get_healthy_mod() const;

        /** Modifiers for health values exclusive to characters */
        virtual void mod_healthy(int nhealthy);
        virtual void mod_healthy_mod(int nhealthy_mod, int cap);

        /** Setters for health values exclusive to characters */
        virtual void set_healthy(int nhealthy);
        virtual void set_healthy_mod(int nhealthy_mod);

        /** Getter for need values exclusive to characters */
        virtual int get_hunger() const;
        virtual int get_thirst() const;
        virtual int get_fatigue() const;
        virtual int get_stomach_food() const;
        virtual int get_stomach_water() const;

        /** Modifiers for need values exclusive to characters */
        virtual void mod_hunger(int nhunger);
        virtual void mod_thirst(int nthirst);
        virtual void mod_fatigue(int nfatigue);
        virtual void mod_stomach_food(int n_stomach_food);
        virtual void mod_stomach_water(int n_stomach_water);

        /** Setters for need values exclusive to characters */
        virtual void set_hunger(int nhunger);
        virtual void set_thirst(int nthirst);
        virtual void set_fatigue(int nfatigue);
        virtual void set_stomach_food(int n_stomach_food);
        virtual void set_stomach_water(int n_stomach_water);

        void mod_stat( const std::string &stat, float modifier ) override;

        /* Adjusts provided sight dispersion to account for player stats */
        int effective_dispersion( int dispersion ) const;

        /* Calculate aim improvement per move spent aiming at a given @ref recoil */
        double aim_per_move( const item &gun, double recoil ) const;

        /** Combat getters */
        float get_dodge_base() const override;
        float get_hit_base() const override;

        /** Handles health fluctuations over time */
        virtual void update_health(int external_modifiers = 0);

        /** Resets the value of all bonus fields to 0. */
        void reset_bonuses() override;
        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats() override;
        /** Handles stat and bonus reset. */
        void reset() override;

        /** Recalculates encumbrance cache. */
        void reset_encumbrance();
        /** Returns ENC provided by armor, etc. */
        int encumb( body_part bp ) const;

        /** Get encumbrance for all body parts. */
        std::array<encumbrance_data, num_bp> get_encumbrance() const;
        /** Get encumbrance for all body parts as if `new_item` was also worn. */
        std::array<encumbrance_data, num_bp> get_encumbrance( const item &new_item ) const;

        /** Returns true if the character is wearing active power */
        bool is_wearing_active_power_armor() const;

        /** Returns true if the player isn't able to see */
        bool is_blind() const;

        /** Bitset of all the body parts covered only with items with `flag` (or nothing) */
        std::bitset<num_bp> exclusive_flag_coverage( const std::string &flag ) const;

        /** Processes effects which may prevent the Character from moving (bear traps, crushed, etc.).
         *  Returns false if movement is stopped. */
        bool move_effects(bool attacking) override;
        /** Performs any Character-specific modifications to the arguments before passing to Creature::add_effect(). */
        void add_effect( const efftype_id &eff_id, int dur, body_part bp = num_bp, bool permanent = false,
                                 int intensity = 0, bool force = false ) override;
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
        float get_vision_threshold(int light_level) const;
        // --------------- Mutation Stuff ---------------
        // In newcharacter.cpp
        /** Returns the id of a random starting trait that costs >= 0 points */
        std::string random_good_trait();
        /** Returns the id of a random starting trait that costs < 0 points */
        std::string random_bad_trait();

        // In mutation.cpp
        /** Returns true if the player has the entered trait */
        bool has_trait(const std::string &flag) const override;
        /** Returns true if the player has the entered starting trait */
        bool has_base_trait(const std::string &flag) const;
        /** Returns true if player has a trait with a flag */
        bool has_trait_flag( const std::string &flag ) const;
        /** Returns true if player has a bionic with a flag */
        bool has_bionic_flag( const std::string &flag ) const;
        /** Returns the trait id with the given invlet, or an empty string if no trait has that invlet */
        std::string trait_by_invlet( long ch ) const;

        /** Toggles a trait on the player and in their mutation list */
        void toggle_trait(const std::string &flag);
        /** Add or removes a mutation on the player, but does not trigger mutation loss/gain effects. */
        void set_mutation(const std::string &flag);
        void unset_mutation(const std::string &flag);

        /**
         * Displays menu with body part hp, optionally with hp estimation after healing.
         * Returns selected part.
         */
        hp_part body_window( bool precise = false ) const;
        hp_part body_window( const std::string &menu_header,
                             bool show_all, bool precise,
                             int normal_bonus, int head_bonus, int torso_bonus,
                             bool bleed, bool bite, bool infect ) const;

        // Returns color which this limb would have in healing menus
        nc_color limb_color( body_part bp, bool bleed, bool bite, bool infect ) const;

        bool made_of( const material_id &m ) const override;

 private:
        /** Retrieves a stat mod of a mutation. */
        int get_mod(std::string mut, std::string arg) const;
 protected:
        /** Applies stat mods to character. */
        void apply_mods(const std::string &mut, bool add_remove);

        /** Recalculate encumbrance for all body parts. */
        std::array<encumbrance_data, num_bp> calc_encumbrance() const;
        /** Recalculate encumbrance for all body parts as if `new_item` was also worn. */
        std::array<encumbrance_data, num_bp> calc_encumbrance( const item &new_item ) const;

        /** Applies encumbrance from mutations and bionics only */
        void mut_cbm_encumb( std::array<encumbrance_data, num_bp> &vals ) const;
        /** Applies encumbrance from items only */
        void item_encumb( std::array<encumbrance_data, num_bp> &vals, const item &new_item ) const;
 public:
        /** Handles things like destruction of armor, etc. */
        void mutation_effect(std::string mut);
        /** Handles what happens when you lose a mutation. */
        void mutation_loss_effect(std::string mut);

        bool has_active_mutation(const std::string &b) const;

        /**
         * Returns resistances on a body part provided by mutations
         */
        // @todo Cache this, it's kinda expensive to compute
        resistances mutation_armor( body_part bp ) const;
        float mutation_armor( body_part bp, damage_type dt ) const;
        float mutation_armor( body_part bp, const damage_unit &dt ) const;

        // --------------- Bionic Stuff ---------------
        /** Returns true if the player has the entered bionic id */
        bool has_bionic(const std::string &b) const;
        /** Returns true if the player has the entered bionic id and it is powered on */
        bool has_active_bionic(const std::string &b) const;

        // --------------- Generic Item Stuff ---------------

        struct has_mission_item_filter {
            int mission_id;
            bool operator()(const item &it) {
                return it.mission_id == mission_id;
            }
        };

        // -2 position is 0 worn index, -3 position is 1 worn index, etc
        static int worn_position_to_index(int position)
        {
            return -2 - position;
        }

        // checks to see if an item is worn
        bool is_worn(const item &thing) const
        {
            for(const auto &elem : worn) {
                if(&thing == &elem) {
                    return true;
                }
            }
            return false;
        }

        /** Returns nearby items which match the provided predicate */
        std::vector<item_location> nearby( const std::function<bool(const item *, const item *)>& func, int radius = 1 ) const;

        /**
         * Similar to @ref remove_items_with, but considers only worn items and not their
         * content (@ref item::contents is not checked).
         * If the filter function returns true, the item is removed.
         */
        std::list<item> remove_worn_items_with( std::function<bool(item &)> filter );

        item &i_at(int position);  // Returns the item with a given inventory position.
        const item &i_at(int position) const;
        /**
         * Returns the item position (suitable for @ref i_at or similar) of a
         * specific item. Returns INT_MIN if the item is not found.
         * Note that this may lose some information, for example the returned position is the
         * same when the given item points to the container and when it points to the item inside
         * the container. All items that are part of the same stack have the same item position.
         */
        int get_item_position( const item *it ) const;

        item &i_add(item it);

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
        item i_rem(int pos);
        /**
         * Remove a specific item from player possession. The item is compared
         * by pointer. Contents of the item are removed as well.
         * @param it A pointer to the item to be removed. The item *must* exists
         * in the players possession (one can use @ref has_item to check for this).
         * @return A copy of the removed item.
         */
        item i_rem(const item *it);
        void i_rem_keep_contents( int pos );
        /** Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
         *  An optional qty can be provided (and will perform better than separate calls). */
        bool i_add_or_drop(item &it, int qty = 1);

        /** Only use for UI things. Returns all invlets that are currently used in
         * the player inventory, the weapon slot and the worn items. */
        std::set<char> allocated_invlets() const;

        /**
         * Whether the player carries an active item of the given item type.
         */
        bool has_active_item(const itype_id &id) const;
        item remove_weapon();
        void remove_mission_items(int mission_id);

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
        std::vector<item_location> find_ammo( const item& obj, bool empty = true, int radius = 1 ) const;

        /**
         * Counts ammo and UPS charges (lower of) for a given gun on the character.
         */
        long ammo_count_for( const item &gun );

        /** Maximum thrown range with a given item, taking all active effects into account. */
        int throw_range( const item & ) const;

        int weight_carried() const;
        units::volume volume_carried() const;
        int weight_capacity() const override;
        units::volume volume_capacity() const;
        units::volume volume_capacity_reduced_by( units::volume mod ) const;

        bool can_pickVolume( const item &it, bool safe = false ) const;
        bool can_pickWeight( const item &it, bool safe = true ) const;
        /**
         * Checks if character stats and skills meet minimum requirements for the item.
         * Prints an appropriate message if requirements not met.
         * @param context optionally override effective item when checking contextual skills
         */
        bool can_use( const item& it, const item &context = item() ) const;

        void drop_inventory_overflow();

        bool has_artifact_with(const art_effect_passive effect) const;

        // --------------- Clothing Stuff ---------------
        /** Returns true if the player is wearing the item. */
        bool is_wearing(const itype_id &it) const;
        /** Returns true if the player is wearing the item on the given body_part. */
        bool is_wearing_on_bp(const itype_id &it, body_part bp) const;
        /** Returns true if the player is wearing an item with the given flag. */
        bool worn_with_flag( const std::string &flag ) const;

        // --------------- Skill Stuff ---------------
        SkillLevel &get_skill_level( const skill_id &ident );

        /** for serialization */
        SkillLevel const& get_skill_level(const skill_id &ident, const item &context = item() ) const;
        void set_skill_level( const skill_id &ident, int level );
        void boost_skill_level( const skill_id &ident, int delta );

        /** Calculates skill difference
         * @param req Required skills to be compared with.
         * @param context An item to provide context for contextual skills. Can be null.
         * @return Difference in skills. Positive numbers - exceeds; negative - lacks; empty map - no difference.
         */
        std::map<skill_id, int> compare_skill_requirements( const std::map<skill_id, int> &req,
                                                            const item &context = item() ) const;
        /** Checks whether the character's skills meet the required */
        bool meets_skill_requirements( const std::map<skill_id, int> &req,
                                       const item &context = item() ) const;
        /** Checks whether the character's stats meets the stats required by the item */
        bool meets_stat_requirements( const item &it ) const;
        /** Checks whether the character meets overall requirements to be able to use the item */
        bool meets_requirements( const item &it, const item &context = item() ) const;
        /** Returns a string of missed requirements (both stats and skills) */
        std::string enumerate_unmet_requirements( const item &it, const item &context = item() ) const;

        // --------------- Other Stuff ---------------

        /** return the calendar::turn the character expired */
        int get_turn_died() const
        {
            return turn_died;
        }
        /** set the turn the turn the character died if not already done */
        void set_turn_died(int turn)
        {
            turn_died = (turn_died != -1) ? turn : turn_died;
        }

        /** Calls Creature::normalize()
         *  nulls out the player's weapon
         *  Should only be called through player::normalize(), not on it's own!
         */
        void normalize() override;
        void die(Creature *nkiller) override;

        std::string get_name() const override;

        /**
         * It is supposed to hide the query_yn to simplify player vs. npc code.
         */
        virtual bool query_yn( const char *mes, ... ) const = 0;

        bool is_immune_field( const field_id fid ) const override;

        /** Returns true if the player has some form of night vision */
        bool has_nv();

        /** Color's character's tile's background */
        nc_color symbol_color() const override;

        // In newcharacter.cpp
        void empty_skills();
        /** Returns a random name from NAMES_* */
        void pick_name(bool bUseDefault = false);
        /** Get the idents of all base traits. */
        std::vector<std::string> get_base_traits() const;
        /** Get the idents of all traits/mutations. */
        std::vector<std::string> get_mutations() const;
        const std::bitset<NUM_VISION_MODES> &get_vision_modes() const
        {
            return vision_mode_cache;
        }
        /** Empties the trait list */
        void empty_traits();
        /** Adds mandatory scenario and profession traits unless you already have them */
        void add_traits();

        // --------------- Values ---------------
        std::string name;
        bool male;

        std::list<item> worn;
        std::array<int, num_hp_parts> hp_cur, hp_max;
        bool nv_cached;

        inventory inv;
        std::map<char, itype_id> assigned_invlet;
        itype_id last_item;
        item weapon;
        item ret_null; // Null item, sometimes returns by weapon() etc

        std::vector<bionic> my_bionics;

    protected:
        void on_stat_change( const std::string &, int ) override {};
        virtual void on_mutation_gain( const std::string & ) {};
        virtual void on_mutation_loss( const std::string & ) {};

    public:
        virtual void on_item_wear( const item & ) {};
        virtual void on_item_takeoff( const item & ) {};

    protected:
        Character();
        Character(const Character &) = default;
        Character(Character &&) = default;
        Character &operator=(const Character &) = default;
        Character &operator=(Character &&) = default;
        struct trait_data : public JsonSerializer, public JsonDeserializer {
            /** Key to select the mutation in the UI. */
            char key = ' ';
            /**
             * Time (in turns) until the mutation increase hunger/thirst/fatigue according
             * to its cost (@ref mutation_data::cost). When those costs have been paid, this
             * is reset to @ref mutation_data::cooldown.
             */
            int charge = 0;
            /** Whether the mutation is activated. */
            bool powered = false;
            // -- serialization stuff, see savegame_json.cpp
            using JsonSerializer::serialize;
            void serialize( JsonOut &json ) const override;
            using JsonDeserializer::deserialize;
            void deserialize( JsonIn &jsin ) override;
        };

        /** Bonuses to stats, calculated each turn */
        int str_bonus;
        int dex_bonus;
        int per_bonus;
        int int_bonus;

        /** How healthy the character is. */
        int healthy;
        int healthy_mod;

        std::array<encumbrance_data, num_bp> encumbrance_cache;

        /**
         * Traits / mutations of the character. Key is the mutation id (it's also a valid
         * key into @ref mutation_data), the value describes the status of the mutation.
         * If there is not entry for a mutation, the character does not have it. If the map
         * contains the entry, the character has the mutation.
         */
        std::unordered_map<std::string, trait_data> my_mutations;
        /**
         * Contains mutation ids of the base traits.
         */
        std::unordered_set<std::string> my_traits;

        void store(JsonOut &jsout) const;
        void load(JsonObject &jsin);

        // --------------- Values ---------------
        std::map<skill_id, SkillLevel> _skills;

        // Cached vision values.
        std::bitset<NUM_VISION_MODES> vision_mode_cache;
        int sight_max;

        // turn the character expired, if -1 it has not been set yet.
        int turn_died = -1;

        /**
         * Cache for pathfinding settings.
         * Most of it isn't changed too often, hence mutable.
         */
        mutable pathfinding_settings path_settings;

    private:
        /** Needs (hunger, thirst, fatigue, etc.) */
        int hunger;
        int thirst;
        int fatigue;

        int stomach_food;
        int stomach_water;
};

#endif
