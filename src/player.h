#ifndef PLAYER_H
#define PLAYER_H

#include "character.h"
#include "item.h"
#include "monster.h"
#include "trap.h"
#include "morale.h"
#include "inventory.h"
#include "mutation.h"
#include "crafting.h"
#include "vehicle.h"
#include "martialarts.h"
#include "player_activity.h"
#include "messages.h"
#include "clzones.h"
#include "artifact.h"

#include <unordered_set>
#include <bitset>
#include <array>

class monster;
class game;
struct trap;
class mission;
class profession;
nc_color encumb_color(int level);

struct special_attack {
    std::string text;
    int bash;
    int cut;
    int stab;

    special_attack()
    {
        bash = 0;
        cut = 0;
        stab = 0;
    };
};

// The minimum level recoil will reach without aiming.
// Sets the floor for accuracy of a "snap" or "hip" shot.
#define MIN_RECOIL 150

//Don't forget to add new memorial counters
//to the save and load functions in savegame_json.cpp
struct stats : public JsonSerializer, public JsonDeserializer {
    int squares_walked;
    int damage_taken;
    int damage_healed;
    int headshots;

    void reset()
    {
        squares_walked = 0;
        damage_taken = 0;
        damage_healed = 0;
        headshots = 0;
    }

    stats()
    {
        reset();
    }

    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const
    {
        json.start_object();
        json.member("squares_walked", squares_walked);
        json.member("damage_taken", damage_taken);
        json.member("damage_healed", damage_healed);
        json.member("headshots", headshots);
        json.end_object();
    }
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin)
    {
        JsonObject jo = jsin.get_object();
        jo.read("squares_walked", squares_walked);
        jo.read("damage_taken", damage_taken);
        jo.read("damage_healed", damage_healed);
        jo.read("headshots", headshots);
    }
};

// Likelyhood to pick a reason
struct reason_weight {
    const char *reason;
    unsigned int weight;
};

// Class for picking a reason for a melee miss from a weighted list.
struct reason_weight_list {
        reason_weight_list() : total_weight(0) { };
        void add_item(const char *reason, unsigned int weight);
        unsigned int pick_ent();
        const char *pick();
        void clear();

    private:
        unsigned int total_weight;
        std::vector<reason_weight> items;
};

class player : public Character, public JsonSerializer, public JsonDeserializer
{
        std::map<Skill *, SkillLevel> _skills;

    public:
        player();
        player(const player &) = default;
        player(player &&) = default;
        virtual ~player() override;
        player &operator=(const player &) = default;
        player &operator=(player &&) = default;

        // newcharacter.cpp
        int create(character_type type, std::string tempname = "");
        /** Returns the set "my_traits" */
        std::vector<std::string> get_traits() const;
        /** Empties the trait list */
        void empty_traits();
        void add_traits();
        void empty_skills();
        /** Returns the id of a random starting trait that costs >= 0 points */
        std::string random_good_trait();
        /** Returns the id of a random starting trait that costs < 0 points */
        std::string random_bad_trait();
        /** Calls Creature::normalize()
         *  nulls out the player's weapon and normalizes HP and bodytemperature
         */
        void normalize();

        virtual void die(Creature *nkiller);
        // </newcharacter.cpp>

        /** Returns a random name from NAMES_* */
        void pick_name();
        /** Returns either "you" or the player's name */
        std::string disp_name(bool possessive = false) const;
        /** Returns the name of the player's outer layer, e.g. "armor plates" */
        std::string skin_name() const;

        virtual bool is_player() const
        {
            return true;
        }
        
        /** Handles human-specific effect application effects before calling Creature::add_eff_effects(). */
        virtual void add_eff_effects(effect e, bool reduced);
        /** Processes human-specific effects effects before calling Creature::process_effects(). */
        void process_effects();
        /** Handles the still hard-coded effects. */
        void hardcoded_effects(effect &it);
        /** Returns the modifier value used for vomiting effects. */
        double vomit_mod();

        virtual bool is_npc() const
        {
            return false;    // Overloaded for NPCs in npc.h
        }
        /** Returns what color the player should be drawn as */
        virtual nc_color basic_symbol_color() const override;

        /** Stringstream loader for old player data files */
        virtual void load_legacy(std::stringstream &dump);
        /** Deserializes string data when loading files */
        virtual void load_info(std::string data);
        /** Outputs a serialized json string for saving */
        virtual std::string save_info();

        int print_info(WINDOW *w, int vStart, int vLines, int column) const;

        // populate variables, inventory items, and misc from json object
        using JsonDeserializer::deserialize;
        virtual void deserialize(JsonIn &jsin);

        using JsonSerializer::serialize;
        // by default save all contained info
        virtual void serialize(JsonOut &jsout) const override;

        /** Prints out the player's memorial file */
        void memorial( std::ofstream &memorial_file, std::string epitaph );
        /** Handles and displays detailed character info for the '@' screen */
        void disp_info();
        /** Provides the window and detailed morale data */
        void disp_morale();
        /** Print the bars indicating how well the player is currently aiming.**/
        int print_aim_bars( WINDOW *w, int line_number, item *weapon, Creature *target);
        /** Print just the gun mode indicator. **/
        void print_gun_mode( WINDOW *w, nc_color c );
        /** Print just the colored recoil indicator. **/
        void print_recoil( WINDOW *w ) const;

        /** Generates the sidebar and it's data in-game */
        void disp_status(WINDOW *w, WINDOW *w2);

        /** Resets stats, and applies effects in an idempotent manner */
        void reset_stats();
        /** Resets movement points and applies other non-idempotent changes */
        void process_turn();
        /** Calculates the various speed bonuses we will get from mutations, etc. */
        void recalc_speed_bonus();
        /** Called after every action, invalidates player caches */
        void action_taken();
        /** Ticks down morale counters and removes them */
        void update_morale();
        /** Ensures persistent morale effects are up-to-date */
        void apply_persistent_morale();
        /** Uses calc_focus_equilibrium to update the player's current focus */
        void update_mental_focus();
        /** Uses morale and other factors to return the player's focus gain rate */
        int calc_focus_equilibrium();
        /** Maintains body temperature */
        void update_bodytemp();
        /** Define color for displaying the body temperature */
        nc_color bodytemp_color(int bp);
        /** Returns the player's modified base movement cost */
        int  run_cost(int base_cost, bool diag = false);
        /** Returns the player's speed for swimming across water tiles */
        int  swim_speed();
        /** Maintains body wetness and handles the rate at which the player dries */
        void update_body_wetness();

        /** Returns true if the player has the entered trait */
        virtual bool has_trait(const std::string &flag) const;
        /** Returns true if the player has the entered starting trait */
        bool has_base_trait(const std::string &flag) const;
        /** Returns true if the player has a conflicting trait to the entered trait
         *  Uses has_opposite_trait(), has_lower_trait(), and has_higher_trait() to determine conflicts.
         */
        bool has_conflicting_trait(const std::string &flag) const;
        /** Returns true if the player has a trait which cancels the entered trait */
        bool has_opposite_trait(const std::string &flag) const;
        /** Returns true if the player has a trait which upgrades into the entered trait */
        bool has_lower_trait(const std::string &flag) const;
        /** Returns true if the player has a trait which is an upgrade of the entered trait */
        bool has_higher_trait(const std::string &flag) const;
        /** Returns true if the player has crossed a mutation threshold
         *  Player can only cross one mutation threshold.
         */
        bool crossed_threshold();
        /** Returns true if the entered trait may be purified away
         *  Defaults to true
         */
        bool purifiable(const std::string &flag) const;
        /** Toggles a trait on the player and in their mutation list */
        void toggle_trait(const std::string &flag);
        /** Toggles a mutation on the player */
        void toggle_mutation(const std::string &flag);
        void toggle_str_set( std::unordered_set< std::string > &set, const std::string &str );
        /** Modifies mutation_category_level[] based on the entered trait */
        void set_cat_level_rec(const std::string &sMut);
        /** Recalculates mutation_category_level[] values for the player */
        void set_highest_cat_level();
        /** Returns the highest mutation category */
        std::string get_highest_category() const;
        /** Returns a dream's description selected randomly from the player's highest mutation category */
        std::string get_category_dream(const std::string &cat, int strength) const;

        /** Returns true if the player is in a climate controlled area or armor */
        bool in_climate_control();

        /** Returns true if the player has the entered bionic id */
        bool has_bionic(const bionic_id &b) const;
        /** Returns true if the player has the entered bionic id and it is powered on */
        bool has_active_bionic(const bionic_id &b) const;
        bool has_active_mutation(const std::string &b) const;
        /** Returns true if the player is wearing an active optical cloak */
        bool has_active_optcloak() const;
        /** Adds a bionic to my_bionics[] */
        void add_bionic(bionic_id b);
        /** Removes a bionic from my_bionics[] */
        void remove_bionic(bionic_id b);
        /** Used by the player to perform surgery to remove bionics and possibly retrieve parts */
        bool uninstall_bionic(bionic_id b_id);
        /** Adds the entered amount to the player's bionic power_level */
        void charge_power(int amount);
        /** Generates and handles the UI for player interaction with installed bionics */
        void power_bionics();
        void power_mutations();
        /** Handles bionic activation effects of the entered bionic, returns if anything activated */
        bool activate_bionic(int b, bool eff_only = false);
        /** Handles bionic deactivation effects of the entered bionic, returns if anything deactivated */
        bool deactivate_bionic(int b, bool eff_only = false);
        /** Handles bionic effects over time of the entered bionic */
        void process_bionic(int b);
        /** Randomly removes a bionic from my_bionics[] */
        bool remove_random_bionic();
        /** Returns the size of my_bionics[] */
        int num_bionics() const;
        /** Returns the bionic at a given index in my_bionics[] */
        bionic &bionic_at_index(int i);
        /** Returns the bionic with the given invlet, or NULL if no bionic has that invlet */
        bionic *bionic_by_invlet(char ch);
        /** Returns player lumination based on the brightest active item they are carrying */
        float active_light();

        /** Returns true if the player doesn't have the mutation or a conflicting one and it complies with the force typing */
        bool mutation_ok(std::string mutation, bool force_good, bool force_bad);
        /** Picks a random valid mutation and gives it to the player, possibly removing/changing others along the way */
        void mutate();
        /** Picks a random valid mutation in a category and mutate_towards() it */
        void mutate_category(std::string);
        /** Mutates toward the entered mutation, upgrading or removing conflicts if necessary */
        void mutate_towards(std::string mut);
        /** Removes a mutation, downgrading to the previous level if possible */
        void remove_mutation(std::string mut);
        /** Returns true if the player has the entered mutation child flag */
        bool has_child_flag(std::string mut);
        /** Removes the mutation's child flag from the player's list */
        void remove_child_flag(std::string mut);

        point pos() const;
        /** Returns the player's sight range */
        int  sight_range(int light_level) const;
        /** Modifies the player's sight values
         *  Must be called when any of the following change:
         *  This must be called when any of the following change:
         * - diseases
         * - bionics
         * - traits
         * - underwater
         * - clothes
         */
        void recalc_sight_limits();
        /** Returns the player maximum vision range factoring in mutations, diseases, and other effects */
        int  unimpaired_range();
        /** Returns true if overmap tile is within player line-of-sight */
        bool overmap_los(int x, int y, int sight_points);
        /** Returns the distance the player can see on the overmap */
        int  overmap_sight_range(int light_level);
        /** Returns the distance the player can see through walls */
        int  clairvoyance() const;
        /** Returns true if the player has some form of impaired sight */
        bool sight_impaired();
        /** Returns true if the player has two functioning arms */
        bool has_two_arms() const;
        /** Returns true if the player is wielding something, including bionic weapons */
        bool is_armed() const;
        /** Calculates melee weapon wear-and-tear through use, returns true */
        bool handle_melee_wear();
        /** True if unarmed or wielding a weapon with the UNARMED_WEAPON flag */
        bool unarmed_attack() const;
        /** Called when a player triggers a trap, returns true if they don't set it off */
        bool avoid_trap(trap *tr, int x, int y);

        /** Returns true if the player has some form of night vision */
        bool has_nv();
        /** Returns true if the player has a pda */
        bool has_pda();

        /**
         * Check if this creature can see the square at (x,y).
         * Includes checks for line-of-sight and light.
         * @param t The t output of map::sees.
         */
        bool sees(int x, int y) const;
        bool sees(int x, int y, int &t) const;
        /**
         * Check if this creature can see the critter.
         * Includes checks for simple critter visibility
         * (digging/submerged) and if this can see the square
         * the creature is on.
         * If this is not the player, it ignores critters that are
         * hallucinations.
         * @param t The t output of map::sees.
         */
        bool sees(const Creature *critter) const;
        bool sees(const Creature *critter, int &t) const;
        /**
         * For fake-players (turrets, mounted turrets) this functions
         * chooses a target. This is for creatures that are friendly towards
         * the player and therefor choose a target that is hostile
         * to the player.
         * @param pos Position of the fake-player
         * @param range The maximal range to look for monsters, anything
         * outside of that range is ignored.
         * @param boo_hoo The number of targets that have been skipped
         * because the player is in the way.
         * @param area The area of effect of the projectile aimed.
         */
        static Creature *auto_find_hostile_target( point pos, int range, int &boo_hoo, int area = 0);
        /**
         * Returns all creatures that this player can see and that are in the given
         * range. This player object itself is never included.
         * The player character (g->u) is checked and might be included (if applicable).
         * @param range The maximal distance (@ref rl_dist), creatures at this distance or less
         * are included.
         */
        std::vector<Creature*> get_visible_creatures( int range ) const;

        Attitude attitude_to( const Creature &other ) const override;

        void pause(); // '.' command; pauses & reduces recoil

        // martialarts.cpp
        /** Fires all non-triggered martial arts events */
        void ma_static_effects();
        /** Fires all move-triggered martial arts events */
        void ma_onmove_effects();
        /** Fires all hit-triggered martial arts events */
        void ma_onhit_effects();
        /** Fires all attack-triggered martial arts events */
        void ma_onattack_effects();
        /** Fires all dodge-triggered martial arts events */
        void ma_ondodge_effects();
        /** Fires all block-triggered martial arts events */
        void ma_onblock_effects();
        /** Fires all get hit-triggered martial arts events */
        void ma_ongethit_effects();

        /** Returns true if the player has any martial arts buffs attached */
        bool has_mabuff(mabuff_id buff_id);
        /** Returns true if the player has access to the entered martial art */
        bool has_martialart(const matype_id &ma_id) const;
        /** Adds the entered martial art to the player's list */
        void add_martialart(const matype_id &ma_id);

        /** Returns the to hit bonus from martial arts buffs */
        int mabuff_tohit_bonus();
        /** Returns the dodge bonus from martial arts buffs */
        int mabuff_dodge_bonus();
        /** Returns the block bonus from martial arts buffs */
        int mabuff_block_bonus();
        /** Returns the speed bonus from martial arts buffs */
        int mabuff_speed_bonus();
        /** Returns the bash armor bonus from martial arts buffs */
        int mabuff_arm_bash_bonus();
        /** Returns the cut armor bonus from martial arts buffs */
        int mabuff_arm_cut_bonus();
        /** Returns the bash damage multiplier from martial arts buffs */
        float mabuff_bash_mult();
        /** Returns the bash damage bonus from martial arts buffs, applied after the multiplier */
        int mabuff_bash_bonus();
        /** Returns the cut damage multiplier from martial arts buffs */
        float mabuff_cut_mult();
        /** Returns the cut damage bonus from martial arts buffs, applied after the multiplier */
        int mabuff_cut_bonus();
        /** Returns true if the player is immune to throws */
        bool is_throw_immune();
        /** Returns true if the player has quiet melee attacks */
        bool is_quiet();
        /** Returns true if the current martial art works with the player's current weapon */
        bool can_melee();
        /** Always returns false, since players can't dig currently */
        bool digging() const;
        /** Returns true if the player is knocked over or has broken legs */
        bool is_on_ground() const;
        /** Returns true if the player should be dead */
        bool is_dead_state() const;

        /** Returns true if the player has technique-based miss recovery */
        bool has_miss_recovery_tec();
        /** Returns true if the player has a grab breaking technique available */
        bool has_grab_break_tec();
        /** Returns true if the player has the leg block technique available */
        bool can_leg_block();
        /** Returns true if the player has the arm block technique available */
        bool can_arm_block();
        /** Returns true if either can_leg_block() or can_arm_block() returns true */
        bool can_limb_block();

        // melee.cpp
        /** Returns true if the player has a weapon with a block technique */
        bool can_weapon_block();
        /** Sets up a melee attack and handles melee attack function calls */
        void melee_attack(Creature &t, bool allow_special, matec_id technique = "");
        /** Returns the player's dispersion modifier based on skill. **/
        int skill_dispersion( item *weapon, bool random ) const;
        /** Returns a weapon's modified dispersion value */
        double get_weapon_dispersion( item *weapon, bool random ) const;
        /** Returns true if a gun misfires, jams, or has other problems, else returns false */
        bool handle_gun_damage( const itype &firing, const std::set<std::string> &curammo_effects );
        /** Handles gun firing effects and functions */
        void fire_gun(int targetx, int targety, bool burst);

        /** Activates any on-dodge effects and checks for dodge counter techniques */
        void dodge_hit(Creature *source, int hit_spread);
        /** Checks for valid block abilities and reduces damage accordingly. Returns true if the player blocks */
        bool block_hit(Creature *source, body_part &bp_hit, damage_instance &dam);
        /** Reduces and mutates du, returns true if armor is damaged */
        bool armor_absorb(damage_unit &du, item &armor);
        /** Runs through all bionics and armor on a part and reduces damage through their armor_absorb */
        void absorb_hit(body_part bp, damage_instance &dam);
        /** Handles return on-hit effects (spines, electric shields, etc.) */
        void on_gethit(Creature *source, body_part bp_hit, damage_instance &dam);

        /** Returns the base damage the player deals based on their stats */
        int base_damage(bool real_life = true, int stat = -999);
        /** Returns the base to hit chance the player has based on their stats */
        int base_to_hit(bool real_life = true, int stat = -999);
        /** Returns Creature::get_hit_base() modified by clothing and weapon skill */
        int get_hit_base() const;
        /** Returns the player's basic hit roll that is compared to the target's dodge roll */
        int hit_roll() const;
        /** Returns true if the player scores a critical hit */
        bool scored_crit(int target_dodge = 0);

        /** Returns the player's total bash damage roll */
        int roll_bash_damage(bool crit);
        /** Returns the player's total cut damage roll */
        int roll_cut_damage(bool crit);
        /** Returns the player's total stab damage roll */
        int roll_stab_damage(bool crit);
        /** Returns the number of moves unsticking a weapon will penalize for */
        int roll_stuck_penalty(bool stabbing, ma_technique &tec);
        std::vector<matec_id> get_all_techniques();

        /** Returns true if the player has a weapon or martial arts skill available with the entered technique */
        bool has_technique(matec_id tec);
        /** Returns a random valid technique */
        matec_id pick_technique(Creature &t,
                                bool crit, bool dodge_counter, bool block_counter);
        void perform_technique(ma_technique technique, Creature &t, int &bash_dam, int &cut_dam,
                               int &stab_dam, int &move_cost);
        /** Performs special attacks and their effects (poisonous, stinger, etc.) */
        void perform_special_attacks(Creature &t);

        /** Returns a vector of valid mutation attacks */
        std::vector<special_attack> mutation_attacks(Creature &t);
        /** Handles combat effects, returns a string of any valid combat effect messages */
        std::string melee_special_effects(Creature &t, damage_instance &d, ma_technique &tec);
        /** Returns Creature::get_dodge_base modified by the player's skill level */
        int get_dodge_base() const;   // Returns the players's dodge, modded by clothing etc
        /** Returns Creature::get_dodge() modified by any player effects */
        int get_dodge() const;
        /** Returns the player's dodge_roll to be compared against an agressor's hit_roll() */
        int dodge_roll();
        /** Returns melee skill level, to be used to throttle dodge practice. **/
        int get_melee() const;
        /**
         * Adds a reason for why the player would miss a melee attack.
         *
         * To possibly be messaged to the player when he misses a melee attack.
         * @param reason A message for the player that gives a reason for him missing.
         * @param weight The weight used when choosing what reason to pick when the
         * player misses.
         */
        void add_miss_reason(const char *reason, unsigned int weight);
        /** Clears the list of reasons for why the player would miss a melee attack. */
        void clear_miss_reasons();
        /**
         * Returns an explanation for why the player would miss a melee attack.
         */
        const char *get_miss_reason();

        /** Handles the uncanny dodge bionic and effects, returns true if the player successfully dodges */
        bool uncanny_dodge(bool is_u = true);
        /** ReReturns an unoccupied, safe adjacent point. If none exists, returns player position. */
        point adjacent_tile();

        // ranged.cpp
        /** Returns the throw range of the item at the entered inventory position. -1 = ERR, 0 = Can't throw */
        int throw_range(int pos);
        /** Returns the ranged attack dexterity mod */
        int ranged_dex_mod() const;
        /** Returns the ranged attack perception mod */
        int ranged_per_mod() const;
        /** Returns the throwing attack dexterity mod */
        int throw_dex_mod(bool return_stat_effect = true) const;
        int aim_per_time( item *gun ) const;

        // Mental skills and stats
        /** Returns the player's reading speed */
        int read_speed     (bool real_life = true);
        /** Returns the player's skill rust rate */
        int rust_rate      (bool real_life = true);
        /** Returns a value used when attempting to convince NPC's of something */
        int talk_skill();
        /** Returns a value used when attempting to intimidate NPC's */
        int intimidation();

        /** Converts bphurt to a hp_part, then does/heals dam
         *  absorb() reduces dam and cut by your armor (and bionics, traits, etc)
         */
        void absorb(body_part bp, int &dam, int &cut);
        /** Calls Creature::deal_damage and handles damaged effects (waking up, etc.) */
        dealt_damage_instance deal_damage(Creature *source, body_part bp, const damage_instance &d);
        /** Actually hurt the player, hurts a body_part directly, no armor reduction */
        void apply_damage(Creature *source, body_part bp, int amount);
        /** Modifies a pain value by player traits before passing it to Creature::mod_pain() */
        void mod_pain(int npain);
        
        void cough(bool harmful = false, int volume = 4);
        
        void add_pain_msg(int val, body_part bp);

        /** Heals a body_part for dam */
        void heal(body_part healed, int dam);
        /** Heals an hp_part for dam */
        void heal(hp_part healed, int dam);
        /** Heals all body parts for dam */
        void healall(int dam);
        /** Hurts all body parts for dam, no armor reduction */
        void hurtall(int dam);
        /** Harms all body parts for dam, with armor reduction. If vary > 0 damage to parts are random within vary % (1-100) */
        void hitall(int dam, int vary = 0);
        /** Knocks the player back one square from a tile */
        void knock_back_from(int x, int y);

        /** Converts a body_part to an hp_part */
        static void bp_convert(hp_part &hpart, body_part bp);
        /** Converts an hp_part to a body_part */
        static void hp_convert(hp_part hpart, body_part &bp);

        /** Returns overall % of HP remaining */
        int hp_percentage();
        /** Recalculates HP after a change to max strength */
        void recalc_hp();

        /** Handles the chance to be infected by random diseases */
        void get_sick();
        /** Handles health fluctuations over time, redirects into Creature::update_health */
        void update_health(int base_threshold = 0);
        /** Checks against env_resist of the players armor, if they fail then they become infected with the disease */
        bool infect(dis_type type, body_part vector, int strength,
                    int duration, bool permanent = false, int intensity = 1,
                    int max_intensity = 1, int decay = 0, int additive = 1,
                    bool targeted = false, bool main_parts_only = false);
        /** Adds a disease without save chance
         *  body_part = num_bp indicates that the disease is body part independant
         *  intensity = -1 indicates that the disease is infinitely stacking */
        void add_disease(dis_type type, int duration, bool permanent = false,
                         int intensity = 1, int max_intensity = 1, int decay = 0,
                         int additive = 1, body_part part = num_bp,
                         bool main_parts_only = false);
        /** Removes a disease from a player */
        void rem_disease(dis_type type, body_part part = num_bp);
        /** Returns list of rc items in player inventory. **/
        std::list<item *> get_radio_items();
        /** Returns true if the player has the entered disease */
        bool has_disease(dis_type type, body_part part = num_bp) const;
        /** Pauses a disease, making it permanent until unpaused */
        bool pause_disease(dis_type type, body_part part = num_bp);
        /** Unpauses a permanent disease, making it wear off when it's timer expires */
        bool unpause_disease(dis_type type, body_part part = num_bp);
        /** Returns the duration of the entered disease's timer */
        int  disease_duration(dis_type type, bool all = false, body_part part = num_bp) const;
        /** Returns the intensity level of the entered disease */
        int  disease_intensity(dis_type type, bool all = false, body_part part = num_bp) const;

        /** Adds an addiction to the player */
        void add_addiction(add_type type, int strength);
        /** Removes an addition from the player */
        void rem_addiction(add_type type);
        /** Returns true if the player has an addiction of the specified type */
        bool has_addiction(add_type type) const;
        /** Returns the intensity of the specified addiction */
        int  addiction_level(add_type type);

        /** Siphons fuel from the specified vehicle into the player's inventory */
        bool siphon(vehicle *veh, ammotype desired_liquid);
        /** Handles a large number of timers decrementing and other randomized effects */
        void suffer();
        /** Handles the chance for broken limbs to spontaneously heal to 1 HP */
        void mend();
        /** Handles player vomiting effects */
        void vomit();

        /** Drenches the player with water, saturation is the percent gotten wet */
        void drench(int saturation, int flags);
        /** Recalculates mutation drench protection for all bodyparts (ignored/good/neutral stats) */
        void drench_mut_calc();

        /** used for drinking from hands, returns how many charges were consumed */
        int drink_from_hands(item &water);
        /** Used for eating object at pos, returns true if object is successfully eaten */
        bool consume(int pos);
        /** Used for eating entered comestible, returns true if comestible is successfully eaten */
        bool eat(item *eat, it_comest *comest);
        /** Handles the effects of consuming an item */
        void consume_effects(item *eaten, it_comest *comest, bool rotten = false);
        /** Handles rooting effects */
        void rooted_message() const;
        void rooted();
        /** Wields an item, returns false on failed wield */
        virtual bool wield(item *it, bool autodrop = false);
        /** Creates the UI and handles player input for picking martial arts styles */
        void pick_style();
        /** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */
        bool wear(int pos, bool interactive = true);
        /** Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion. */
        bool wear_item(item *to_wear, bool interactive = true);
        /** Takes off an item, returning false on fail, if an item vector
         is given, stores the items in that vector and not in the inventory */
        bool takeoff( item *target, bool autodrop = false, std::vector<item> *items = nullptr );
        bool takeoff( int pos, bool autodrop = false, std::vector<item> *items = nullptr );
        /** Removes the first item in the container's contents and wields it, taking moves based on skill and volume of item being wielded. */
        void wield_contents(item *container, bool force_invlet, std::string skill_used, int volume_factor);
        /** Stores an item inside another item, taking moves based on skill and volume of item being stored. */
        void store(item *container, item *put, std::string skill_used, int volume_factor);
        /** Draws the UI and handles player input for the armor re-ordering window */
        void sort_armor();
        /** Uses a tool */
        void use(int pos);
        /** Uses the current wielded weapon */
        void use_wielded();
        /** Removes selected gunmod from the entered weapon */
        void remove_gunmod(item *weapon, unsigned id);
        /** Attempts to install bionics, returns false if the player cancels prior to installation */
        bool install_bionics(it_bionic *type);
        /** Handles reading effects */
        void read(int pos);
        /** Completes book reading action. **/
        void do_read( item *book );
        /** Note that we've read a book at least once. **/
        bool has_identified( std::string item_id ) const;
        /** Handles sleep attempts by the player, adds "lying_down" */
        void try_to_sleep();
        /** Checked each turn during "lying_down", returns true if the player falls asleep */
        bool can_sleep();
        /** Adds "sleep" to the player */
        void fall_asleep(int duration);
        /** Removes "sleep" and "lying_down" from the player */
        void wake_up();
        /** Checks to see if the player is using floor items to keep warm, and return the name of one such item if so */
        std::string is_snuggling();
        /** Returns a value used for things like reading and sewing based on light level */
        float fine_detail_vision_mod();

        /** Used to determine player feedback on item use for the inventory code */
        hint_rating rate_action_use(const item *it)
        const; //rates usability lower for non-tools (books, etc.)
        hint_rating rate_action_wear(item *it);
        hint_rating rate_action_eat(item *it);
        hint_rating rate_action_read(item *it);
        hint_rating rate_action_takeoff(item *it);
        hint_rating rate_action_reload(item *it);
        hint_rating rate_action_unload( const item &it ) const;
        hint_rating rate_action_disassemble(item *it);

        /** Returns warmth provided by armor, etc. */
        int warmth(body_part bp) const;
        /** Returns ENC provided by armor, etc. */
        int encumb(body_part bp) const;
        /** Returns warmth provided by armor, etc., factoring in layering */
        int encumb(body_part bp, double &layers, int &armorenc) const;
        /** Returns overall bashing resistance for the body_part */
        int get_armor_bash(body_part bp) const;
        /** Returns overall cutting resistance for the body_part */
        int get_armor_cut(body_part bp) const;
        /** Returns bashing resistance from the creature and armor only */
        int get_armor_bash_base(body_part bp) const;
        /** Returns cutting resistance from the creature and armor only */
        int get_armor_cut_base(body_part bp) const;
        /** Returns overall env_resist on a body_part */
        int get_env_resist(body_part bp) const;
        /** Returns true if the player is wearing something on the entered body_part */
        bool wearing_something_on(body_part bp) const;
        /** Returns true if the player is wearing something on their feet that is not SKINTIGHT */
        bool is_wearing_shoes(std::string side = "both") const;
        /** Returns 1 if the player is wearing something on both feet, .5 if on one, and 0 if on neither */
        double footwear_factor() const;
        /** Returns 1 if the player is wearing an item of that count on one foot, 2 if on both, and zero if on neither */
        int shoe_type_count(const itype_id &it) const;
        /** Returns true if the player is wearing power armor */
        bool is_wearing_power_armor(bool *hasHelmet = NULL) const;
        /** Returns wind resistance provided by armor, etc **/
        int get_wind_resistance(body_part bp) const;

        int adjust_for_focus(int amount);
        void practice( Skill *s, int amount, int cap = 99 );
        void practice( std::string s, int amount, int cap = 99 );

        void assign_activity(activity_type type, int moves, int index = -1, int pos = INT_MIN,
                             std::string name = "");
        bool has_activity(const activity_type type) const;
        void cancel_activity();

        int weight_carried() const;
        int volume_carried() const;
        int weight_capacity() const;
        int volume_capacity() const;
        double convert_weight(int weight);
        bool can_eat(const item i);
        bool can_pickVolume(int volume) const;
        bool can_pickWeight(int weight, bool safe = true) const;
        int net_morale(morale_point effect);
        int morale_level(); // Modified by traits, &c
        void add_morale(morale_type type, int bonus, int max_bonus = 0,
                        int duration = 60, int decay_start = 30,
                        bool cap_existing = false, itype *item_type = NULL);
        int has_morale( morale_type type ) const;
        void rem_morale(morale_type type, itype *item_type = NULL);

        std::string weapname(bool charges = true);

        /**
         * Test whether an item in the possession of this player match a
         * certain filter.
         * The items might be inside other items (containers / quiver / etc.),
         * the filter is recursively applied to all item contents.
         * If this returns true, the vector returned by @ref items_with
         * (with the same filter) will be non-empty.
         * @param filter some object that when invoked with the () operator
         * returns true for item that should checked for.
         * @return Returns true when at least one item matches the filter,
         * if no item matches the filter it returns false.
         */
        template<typename T>
        bool has_item_with(T filter) const
        {
            if( inv.has_item_with( filter ) ) {
                return true;
            }
            if( !weapon.is_null() && inventory::has_item_with_recursive( weapon, filter ) ) {
                return true;
            }
            for( auto &w : worn ) {
                if( inventory::has_item_with_recursive( w, filter ) ) {
                    return true;
                }
            }
            return false;
        }
        /**
         * Gather all items that match a certain filter.
         * The returned vector contains pointers to items in the possession
         * of this player (can be weapon, worn items or inventory).
         * The items might be inside other items (containers / quiver / etc.),
         * the filter is recursively applied to all item contents.
         * The items should not be changed directly, the pointers can be used
         * with @ref i_rem, @ref reduce_charges. The pointers are *not* suitable
         * for @ref get_item_position because the returned index can only
         * refer to items directly in the inventory (e.g. -1 means the weapon,
         * there is no index for the content of the weapon).
         * @param filter some object that when invoked with the () operator
         * returns true for item that should be returned.
         */
        template<typename T>
        std::vector<const item *> items_with(T filter) const
        {
            auto result = inv.items_with( filter );
            if( !weapon.is_null() ) {
                inventory::items_with_recursive( result, weapon, filter );
            }
            for( auto &w : worn ) {
                inventory::items_with_recursive( result, w, filter );
            }
            return result;
        }
        /**
         * Removes the items that match the given filter.
         * The returned items are a copy of the removed item.
         * If no item has been removed, an empty list will be returned.
         */
        template<typename T>
        std::list<item> remove_items_with( T filter )
        {
            // player usually interacts with items in the inventory the most (?)
            std::list<item> result = inv.remove_items_with( filter );
            for( auto &article : worn ) {
                if( filter( article ) ) {
                    result.push_back( article );
                } else {
                    result.splice( result.begin(), article.remove_items_with( filter ) );
                }
            }
            worn.erase( std::remove_if( worn.begin(), worn.end(), filter ), worn.end() );
            if( !weapon.is_null() ) {
                if( filter( weapon ) ) {
                    result.push_back( remove_weapon() );
                } else {
                    result.splice( result.begin(), weapon.remove_items_with( filter ) );
                }
            }
            return result;
        }
        /**
         * All items that have the given flag (@ref item::has_flag).
         */
        std::vector<const item *> all_items_with_flag( const std::string flag ) const;

        item &i_add(item it);
        // Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
        // An optional qty can be provided (and will perform better than separate calls).
        bool i_add_or_drop(item &it, int qty = 1);
        /**
         * Whether the player carries an active item of the given item type.
         */
        bool has_active_item(const itype_id &id) const;
        void process_active_items();
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
        item remove_weapon();
        void remove_mission_items(int mission_id);
        /**
         * Remove charges from a specific item (given by its item position).
         * The item must exist and it must be counted by charges.
         * @param position Item position of the item.
         * @param quantity The number of charges to remove, must not be larger than
         * the current charges of the item.
         * @return An item that contains the removed charges, it's effectively a
         * copy of the item with the proper charges.
         */
        item reduce_charges(int position, long quantity);
        /**
         * Remove charges from a specific item (given by a pointer to it).
         * Otherwise identical to @ref reduce_charges(int,long)
         * @param it A pointer to the item, it *must* exist.
         */
        item reduce_charges(item *it, long quantity);
        item &i_at(int position);  // Returns the item with a given inventory position.
        /** Return the item position of the item with given invlet, return INT_MIN if
         * the player does not have such an item with that invlet. Don't use this on npcs.
         * Only use the invlet in the user interface, otherwise always use the item position. */
        int invlet_to_position(char invlet) const;
        /**
         * Returns the item position (suitable for @ref i_at or similar) of a
         * specific item.
         * NOTE: this only works for items outside of containers, in the main inventory,
         * the weapon or worn items, If the item is a pointer to an item inside a
         * container, it wont work.
         */
        int get_item_position(const item *it);
        const martialart &get_combat_style() const; // Returns the combat style object
        std::vector<item *> inv_dump(); // Inventory + weapon + worn (for death, etc)
        void place_corpse(); // put corpse+inventory on map at the place where this is.
        int butcher_factor() const; // Automatically picks our best butchering tool
        item  *pick_usb(); // Pick a usb drive, interactively if it matters
        /** Returns true if the player is wearing the item */
        bool is_wearing(const itype_id &it) const;
        /** Returns true if the player is wearing the item on the given body_part */
        bool is_wearing_on_bp(const itype_id &it, body_part bp) const;
        bool has_artifact_with(const art_effect_passive effect) const;
        bool worn_with_flag( std::string flag ) const;

        bool covered_with_flag(const std::string flag, std::bitset<num_bp> parts) const;
        bool covered_with_flag_exclusively(const std::string flag, std::bitset<num_bp> parts) const;
        bool is_water_friendly(std::bitset<num_bp> parts) const;
        bool is_waterproof(std::bitset<num_bp> parts) const;

        // has_amount works ONLY for quantity.
        // has_charges works ONLY for charges.
        std::list<item> use_amount(itype_id it, int quantity, bool use_container = false);
        bool use_charges_if_avail(itype_id it, long quantity);// Uses up charges
        std::list<item> use_charges(itype_id it, long quantity);// Uses up charges
        bool has_amount(const itype_id &it, int quantity) const;
        bool has_charges(const itype_id &it, long quantity) const;
        int  amount_of(const itype_id &it) const;
        long charges_of(const itype_id &it) const;

        int  leak_level( std::string flag ) const; // carried items may leak radiation or chemicals

        // Check for free container space for the whole liquid item
        bool has_container_for(const item &liquid);
        // Has a weapon, inventory item or worn item with flag
        bool has_item_with_flag( std::string flag ) const;
        // Has amount (or more) items with at least the required quality level.
        bool has_items_with_quality( const std::string &quality_id, int level, int amount ) const;
        bool has_item(int position);
        /**
         * Check whether a specific item is in the players possession.
         * The item is compared by pointer.
         * @param it A pointer to the item to be looked for.
         */
        bool has_item(const item *it) const;
        /** Only use for UI things. Returns all invelts that are currently used in
         * the player inventory, the weapon slot and the worn items. */
        std::set<char> allocated_invlets() const;
        bool has_mission_item(int mission_id) const; // Has item with mission_id
        std::vector<item *> has_ammo(ammotype at); // Returns a list of the ammo
        // same as has_ammo, but all items with typeId() != id are removed,
        // returned items all the the same type: id.
        std::vector<item *> has_exact_ammo( const ammotype &at, const itype_id &id );
        /**
         * Check whether the player has a gun that uses the given type of ammo.
         */
        bool has_gun_for_ammo( const ammotype &at ) const;

        bool has_weapon() const;
        // Check if the player can pickup stuff (fails if wielding
        // certain bionic weapons).
        // Print a message if print_msg is true and this isn't a NPC
        bool can_pickup(bool print_msg) const;

        // Checks crafting inventory for books providing the requested recipe.
        // Returns -1 to indicate recipe not found, otherwise difficulty to learn.
        int has_recipe( const recipe *r, const inventory &crafting_inv ) const;
        bool knows_recipe( const recipe *rec ) const;
        void learn_recipe( recipe *rec );

        bool studied_all_recipes(const itype &book) const;

        // crafting.cpp
        bool crafting_allowed(); // is morale high enough to craft?
        bool crafting_can_see(); // can player see well enough to craft?
        bool can_make(const recipe *r, int batch_size = 1); // have components?
        bool making_would_work(const std::string &id_to_make, int batch_size);
        void craft();
        void recraft();
        void long_craft();
        void make_craft(const std::string &id, int batch_size);
        void make_all_craft(const std::string &id, int batch_size);
        void complete_craft();

        // also crafting.cpp
        /**
         * Check if the player can disassemble the item dis_item with the recipe
         * cur_recipe and the inventory crafting_inv.
         * Checks for example tools (and charges), enough input charges
         * (if disassembled item is counted by charges).
         * If print_msg is true show a message about missing tools/charges.
         */
        bool can_disassemble( const item *dis_item, const recipe *cur_recipe,
                              const inventory &crafting_inv, bool print_msg ) const;
        void disassemble(int pos = INT_MAX);
        void complete_disassemble();

        // yet more crafting.cpp
        const inventory &crafting_inventory(); // includes nearby items
        void invalidate_crafting_inventory();
        std::vector<item> get_eligible_containers_for_crafting();
        std::list<item> consume_items(const std::vector<item_comp> &components, int batch = 1);
        void consume_tools(const std::vector<tool_comp> &tools, int batch = 1);

        // Auto move methods
        void set_destination(const std::vector<point> &route);
        void clear_destination();
        bool has_destination() const;
        std::vector<point> &get_auto_move_route();
        action_id get_next_auto_move_direction();
        void shift_destination(int shiftx, int shifty);

        // Library functions
        double logistic(double t);
        double logistic_range(int min, int max, int pos);
        void calculate_portions(int &x, int &y, int &z, int maximum);

        // ---------------VALUES-----------------
        int posx, posy;
        inline int xpos() const
        {
            return posx;
        }
        inline int ypos() const
        {
            return posy;
        }
        int view_offset_x, view_offset_y;
        bool in_vehicle;       // Means player sit inside vehicle on the tile he is now
        bool controlling_vehicle;  // Is currently in control of a vehicle
        // Relative direction of a grab, add to posx, posy to get the coordinates of the grabbed thing.
        point grab_point;
        object_type grab_type;
        player_activity activity;
        std::list<player_activity> backlog;
        // _missions vectors are of mission IDs
        std::vector<int> active_missions;
        std::vector<int> completed_missions;
        std::vector<int> failed_missions;
        int active_mission;
        int volume;

        std::string name;
        bool male;
        profession *prof;

        std::string start_location;

        std::map<std::string, int> mutation_category_level;

        int next_climate_control_check;
        bool last_climate_control_ret;
        int power_level, max_power_level;
        int hunger, thirst, fatigue;
        int stomach_food, stomach_water;
        int oxygen;
        int recoil;
        int driving_recoil;
        int scent;
        int dodges_left, blocks_left;
        int stim, pkill, radiation;
        unsigned long cash;
        int movecounter;
        std::array<int, num_hp_parts> hp_cur, hp_max;
        std::array<int, num_bp> temp_cur, frostbite_timer, temp_conv;
        void temp_equalizer(body_part bp1, body_part bp2); // Equalizes heat between body parts
        bool nv_cached;
        bool pda_cached;

        // Drench cache
        std::map<int, std::map<std::string, int> > mMutDrench;
        std::map<int, int> mDrenchEffect;
        std::array<int, num_bp> body_wetness;

        std::vector<morale_point> morale;

        int focus_pool;

        SkillLevel &skillLevel(Skill *_skill);
        SkillLevel &skillLevel(std::string ident);

        // for serialization
        SkillLevel get_skill_level(Skill *_skill) const;
        SkillLevel get_skill_level(const std::string &ident) const;

        void set_skill_level(Skill *_skill, int level);
        void set_skill_level(std::string ident, int level);

        void boost_skill_level(Skill *_skill, int level);
        void boost_skill_level(std::string ident, int level);

        void copy_skill_levels(const player *rhs);

        std::map<std::string, recipe *> learned_recipes;

        inventory inv;
        itype_id last_item;
        std::vector<item> worn;
        std::map<char, itype_id> assigned_invlet;

        std::vector<matype_id> ma_styles;
        matype_id style_selected;
        bool keep_hands_free;

        item weapon;
        item ret_null; // Null item, sometimes returns by weapon() etc

        std::vector <addiction> addictions;

        std::string lastrecipe;
        int last_batch;
        itype_id lastconsumed;        //used in crafting.cpp and construction.cpp

        //Dumps all memorial events into a single newline-delimited string
        std::string dump_memorial();
        //Log an event, to be later written to the memorial file
        void add_memorial_log(const char *male_msg, const char *female_msg, ...);
        //Loads the memorial log from a file
        void load_memorial_file(std::ifstream &fin);
        //Notable events, to be printed in memorial
        std::vector <std::string> memorial_log;

        //Record of player stats, for posterity only
        stats *lifetime_stats();
        stats get_stats() const; // for serialization
        void mod_stat( std::string stat, int modifier );

        int getID () const;
        // sets the ID, will *only* succeed when the current id is 0 (=not initialized)
        void setID (int i);

        bool is_underwater() const;
        void set_underwater(bool);
        bool is_hallucination() const;

        void environmental_revert_effect();

        bool is_invisible() const;
        bool is_deaf() const;
        int visibility( bool check_color = false,
                        int stillness = 0 ) const; // just checks is_invisible for the moment
        // -2 position is 0 worn index, -3 position is 1 worn index, etc
        static int worn_position_to_index(int position)
        {
            return -2 - position;
        }

        m_size get_size() const;
        int get_hp( hp_part bp ) const;
        int get_hp_max( hp_part bp ) const;

        field_id playerBloodType() const;

        //message related stuff
        virtual void add_msg_if_player(const char *msg, ...) const;
        virtual void add_msg_if_player(game_message_type type, const char *msg, ...) const;
        virtual void add_msg_player_or_npc(const char *player_str, const char *npc_str, ...) const;
        virtual void add_msg_player_or_npc(game_message_type type, const char *player_str,
                                           const char *npc_str, ...) const;

        typedef std::map<tripoint, std::string> trap_map;
        bool knows_trap(int x, int y) const;
        void add_known_trap(int x, int y, const std::string &t);
        /** Search surrounding squares for traps (and maybe other things in the future). */
        void search_surroundings();

        // zone related stuff
        // (zones as in the zones designated by players)
        clZones Zones;

        bool save_zones();
        void load_zones();

        // drawing related stuff
        /**
         * Returns a list of the IDs of overlays on this character,
         * sorted from "lowest" to "highest".
         *
         * Only required for rendering.
         */
        std::vector<std::string> get_overlay_ids() const;

        void spores();
        void blossoms();

        // return the calendar::turn the player expired
        int get_turn_died() const
        {
            return turn_died;
        }
        // set the turn the turn the player died if not already done
        void set_turn_died(int turn)
        {
            turn_died = (turn_died != -1) ? turn : turn_died;
        }

    protected:
        std::unordered_set<std::string> my_traits;
        std::unordered_set<std::string> my_mutations;
        std::map<std::string, char> trait_keys;
        std::vector<bionic> my_bionics;
        std::list<disease> illness;
        bool underwater;
        trap_map known_traps;

        int sight_max;
        int sight_boost;
        int sight_boost_cap;

        void store(JsonOut &jsout) const;
        void load(JsonObject &jsin);

    private:
        // Items the player has identified.
        std::unordered_set<std::string> items_identified;
        /** Check if an area-of-effect technique has valid targets */
        bool valid_aoe_technique( Creature &t, ma_technique &technique );
        bool valid_aoe_technique( Creature &t, ma_technique &technique,
                                  std::vector<int> &mon_targets, std::vector<int> &npc_targets );
        /**
         * Check whether the other creature is in range and can be seen by this creature.
         * @param range The maximal distance (@ref rl_dist), creatures at this distance or less
         * are included.
         */
        bool is_visible_in_range( const Creature &critter, int range ) const;

        // Trigger and disable mutations that can be so toggled.
        void activate_mutation( std::string mutation );
        void deactivate_mutation( std::string mut );
        bool has_fire(const int quantity) const;
        void use_fire(const int quantity);
        /**
         * Has the item enough charges to invoke its use function?
         * Also checks if UPS from this player is used instead of item charges.
         */
        bool has_enough_charges(const item &it, bool show_msg) const;

        bool can_study_recipe(const itype &book);
        bool try_study_recipe(const itype &book);

        std::vector<point> auto_move_route;
        // Used to make sure auto move is canceled if we stumble off course
        point next_expected_position;

        inventory cached_crafting_inventory;
        int cached_moves;
        int cached_turn;
        point cached_position;

        struct reason_weight_list melee_miss_reasons;

        int id; // A unique ID number, assigned by the game class private so it cannot be overwritten and cause save game corruptions.
        //NPCs also use this ID value. Values should never be reused.

        // turn the player expired, if -1 it has not been set yet.
        int turn_died = -1;
};

#endif
