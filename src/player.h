#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "character.h"
#include "item.h"
#include "monster.h"
#include "trap.h"
#include "morale.h"
#include "inventory.h"
#include "artifact.h"
#include "mutation.h"
#include "crafting.h"
#include "vehicle.h"
#include "martialarts.h"

class monster;
class game;
struct trap;
class mission;
class profession;

struct special_attack
{
 std::string text;
 int bash;
 int cut;
 int stab;

 special_attack() { bash = 0; cut = 0; stab = 0; };
};

//Don't forget to add new memorial counters
//to the save and load functions in savegame_json.cpp
struct stats : public JsonSerializer, public JsonDeserializer
{
    int squares_walked;
    int damage_taken;
    int damage_healed;
    int headshots;

    void reset() {
        squares_walked = 0;
        damage_taken = 0;
        damage_healed = 0;
        headshots = 0;
    }

    stats() {
        reset();
    }

    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const {
        json.start_object();
        json.member("squares_walked", squares_walked);
        json.member("damage_taken", damage_taken);
        json.member("damage_healed", damage_healed);
        json.member("headshots", headshots);
        json.end_object();
    }
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) {
        JsonObject jo = jsin.get_object();
        jo.read("squares_walked", squares_walked);
        jo.read("damage_taken", damage_taken);
        jo.read("damage_healed", damage_healed);
        jo.read("headshots", headshots);
    }
};

class player : public Character, public JsonSerializer, public JsonDeserializer
{
  std::map<Skill*,SkillLevel> _skills;

public:
 player();
 player(const player &rhs);
 virtual ~player();

 player& operator= (const player & rhs);

// newcharacter.cpp
 bool create(character_type type, std::string tempname = "");
 std::set<std::string> get_traits() const;
 std::string random_good_trait();
 std::string random_bad_trait();
 void normalize(); // Starting set up of HP and inventory

 virtual void die(Creature* nkiller);
// </newcharacter.cpp>

    void pick_name(); // Picks a name from NAMES_*
    std::string disp_name(); // what to call 'im
    std::string skin_name(); // what to call 'im

    virtual bool is_player() { return true; }

    void process_effects(); // Process long-term effects

 virtual bool is_npc() { return false; } // Overloaded for NPCs in npc.h
 nc_color color(); // What color to draw us as

 virtual void load_legacy(std::stringstream & dump);  // stringstream loader for old data
 virtual void load_info(std::string data); // deserialize string when loading
 virtual std::string save_info(); // output serialized json string for saving

    // populate variables, inventory items, and misc from json object
    void json_load_common_variables(JsonObject &jsout);
    using JsonDeserializer::deserialize;
    virtual void deserialize(JsonIn &jsin);

    void json_save_common_variables(JsonOut &json) const;
    using JsonSerializer::serialize;
    // by default save all contained info
    void serialize(JsonOut &jsout) const { serialize(jsout, true); }
    virtual void serialize(JsonOut &jsout, bool save_contents) const;

 void memorial( std::ofstream &memorial_file ); // Write out description of player.
 void disp_info(); // '@' key; extended character info
 void disp_morale(); // '%' key; morale info
 void disp_status(WINDOW* w, WINDOW *w2);// On-screen data

 void reset_stats();// Resets movement points, stats, applies effects
 void recalc_speed_bonus(); // Calculate the various speed bonuses we will get from mutations etc
 void action_taken(); // Called after every action, invalidates player caches.
 void update_morale(); // Ticks down morale counters and removes them
 void apply_persistent_morale(); // Ensure persistent morale effects are up-to-date.
 void update_mental_focus();
 int calc_focus_equilibrium();
 void update_bodytemp();  // Maintains body temperature
 int  run_cost(int base_cost, bool diag = false); // Adjust base_cost
 int  swim_speed(); // Our speed when swimming

 bool has_trait(const std::string &flag) const;
 bool has_base_trait(const std::string &flag) const;
 bool has_conflicting_trait(const std::string &flag) const;
 bool has_opposite_trait(const std::string &flag) const;
 bool has_lower_trait(const std::string &flag) const;
 bool has_higher_trait(const std::string &flag) const;
 bool crossed_threshold(); // Can only cross one Threshold
 bool purifiable(const std::string &flag) const; // Purifier can/not touch this, defaults "true"
 void toggle_trait(const std::string &flag);
 void toggle_mutation(const std::string &flag);
 void set_cat_level_rec(const std::string &sMut);
 void set_highest_cat_level();
 std::string get_highest_category() const;
 std::string get_category_dream(const std::string &cat, int strength) const;

 bool in_climate_control();

 bool has_bionic(const bionic_id & b) const;
 bool has_active_bionic(const bionic_id & b) const;
 bool has_active_optcloak() const;
 void add_bionic(bionic_id b);
 void charge_power(int amount);
 void power_bionics();
 void activate_bionic(int b);
 bool remove_random_bionic();
 int num_bionics() const;
 bionic& bionic_at_index(int i);
 // returns the bionic with the given invlet, or NULL if no
 // bionic has that invlet.
 bionic* bionic_by_invlet(char ch);
 float active_light();

 bool mutation_ok(std::string mutation, bool force_good, bool force_bad);
 void mutate();
 void mutate_category(std::string);
 void mutate_towards(std::string mut);
 void remove_mutation(std::string mut);
 bool has_child_flag(std::string mut);
 void remove_child_flag(std::string mut);

 point pos();
 int  sight_range(int light_level) const;
 void recalc_sight_limits();
 int  unimpaired_range();
 int  overmap_sight_range(int light_level);
 int  clairvoyance(); // Sight through walls &c
 bool sight_impaired(); // vision impaired between sight_range and max_range
 bool has_two_arms() const;
 bool can_wear_boots();
 bool is_armed(); // True if we're wielding something; true for bionics
 bool unarmed_attack(); // False if we're wielding something; true for bionics
 bool avoid_trap(trap *tr);

 bool has_nv();

 void pause(); // '.' command; pauses & reduces recoil

// martialarts.cpp
 void ma_static_effects(); // fires all non-triggered martial arts events
 void ma_onmove_effects(); // fires all move-triggered martial arts events
 void ma_onhit_effects(); // fires all hit-triggered martial arts events
 void ma_onattack_effects(); // fires all attack-triggered martial arts events
 void ma_ondodge_effects(); // fires all dodge-triggered martial arts events
 void ma_onblock_effects(); // fires all block-triggered martial arts events
 void ma_ongethit_effects(); // fires all get hit-triggered martial arts events

 bool has_mabuff(mabuff_id buff_id); // checks if a player has any martial arts buffs attached

 int mabuff_tohit_bonus(); // martial arts to-hit bonus
 int mabuff_dodge_bonus(); // martial arts dodge bonus
 int mabuff_block_bonus(); // martial arts block bonus
 int mabuff_speed_bonus(); // martial arts to-hit bonus
 int mabuff_arm_bash_bonus(); // martial arts bash armor bonus
 int mabuff_arm_cut_bonus(); // martial arts cut armor bonus
 float mabuff_bash_mult(); // martial arts bash damage multiplier
 int mabuff_bash_bonus(); // martial arts bash damage bonus, applied after mult
 float mabuff_cut_mult(); // martial arts bash damage multiplier
 int mabuff_cut_bonus(); // martial arts bash damage bonus, applied after mult
 bool is_throw_immune(); // martial arts throw immunity
 bool is_quiet(); // martial arts quiet melee

 bool can_melee();
 bool digging();      // always false, because players can't dig
 bool is_on_ground(); // all body parts are available to ground level damage sources
 bool is_dead_state(); // check if we should be dead or not

 bool has_miss_recovery_tec(); // technique-based miss recovery, like tec_feint
 bool has_grab_break_tec(); // technique-based miss recovery, like tec_feint
 bool can_leg_block(); // technique-based defensive ability
 bool can_arm_block(); // technique-based defensive ability, like tec_leg_block
 bool can_block(); // can we block at all

// melee.cpp
 bool can_weapon_block(); //gear-based defensive ability
 void melee_attack(Creature &p, bool allow_special = true);
 double get_weapon_dispersion(item* weapon);
 void fire_gun(int targetx, int targety, bool burst);
 int  hit_mon(monster *critter, bool allow_grab = true);
 void hit_player(player &p, bool allow_grab = true);

 bool block_hit(body_part &bp_hit, int &side,
    damage_instance &dam);

 bool armor_absorb(damage_unit& du, item& armor);
 void absorb_hit(body_part bp, int side,
    damage_instance &dam);
 void on_gethit(Creature *source, body_part bp_hit, damage_instance &dam);

 int base_damage(bool real_life = true, int stat = -999);
 int base_to_hit(bool real_life = true, int stat = -999);

 int get_hit_base();     // Returns the player's to-hit, modded by clothing etc
 int  hit_roll(); // Our basic hit roll, compared to our target's dodge roll
 bool scored_crit(int target_dodge = 0); // Critical hit?

 int roll_bash_damage(bool crit);
 int roll_cut_damage(bool crit);
 int roll_stab_damage(bool crit);
 int roll_stuck_penalty(bool stabbing);

 std::vector<matec_id> get_all_techniques();

 bool has_technique(matec_id tec);
 matec_id pick_technique(Creature &t,
                             bool crit, bool allowgrab);
 void perform_technique(ma_technique technique, Creature &t,
                       int &bash_dam, int &cut_dam, int &pierce_dam, int &pain);

 void perform_special_attacks(Creature &t);

 std::vector<special_attack> mutation_attacks(Creature &t);
 std::string melee_special_effects(Creature &t, damage_instance& d);

 int get_dodge_base();   // Returns the players's dodge, modded by clothing etc
 int get_dodge();
 int dodge_roll();// For comparison to hit_roll()

 bool uncanny_dodge(bool is_u = true);      // Move us to an adjacent_tile() if available. Display message if player is dodging.
 point adjacent_tile();     // Returns an unoccupied, safe adjacent point. If none exists, returns player position.

// ranged.cpp
 int throw_range(int pos); // Range of throwing item; -1:ERR 0:Can't throw
 int ranged_dex_mod (bool real_life = true);
 int ranged_per_mod (bool real_life = true);
 int throw_dex_mod  (bool real_life = true);

// Mental skills and stats
 int read_speed     (bool real_life = true);
 int rust_rate      (bool real_life = true);
 int talk_skill(); // Skill at convincing NPCs of stuff
 int intimidation(); // Physical intimidation

// Converts bphurt to a hp_part (if side == 0, the left), then does/heals dam
// absorb() reduces dam and cut by your armor (and bionics, traits, etc)
 void absorb(body_part bp,               int &dam, int &cut);
// hurt() doesn't--effects of disease, what have you
 void hurt (body_part bphurt, int side, int  dam);
 void hurt (hp_part hurt, int dam);

 dealt_damage_instance deal_damage(Creature* source, body_part bp,
                                   int side, const damage_instance& d);
 void apply_damage(Creature* source, body_part bp, int side, int amount);

 void mod_pain(int npain);

 void heal(body_part healed, int side, int dam);
 void heal(hp_part healed, int dam);
 void healall(int dam);
 void hurtall(int dam);
 // checks armor. if vary > 0, then damage to parts are random within 'vary' percent (1-100)
 void hitall(int dam, int vary = 0);
 // Sends us flying one tile
 void knock_back_from(int x, int y);

 //Converts bp/side to hp_part and back again
 void bp_convert(hp_part &hpart, body_part bp, int side);
 void hp_convert(hp_part hpart, body_part &bp, int &side);

 int hp_percentage(); // % of HP remaining, overall
 void recalc_hp(); // Change HP after a change to max strength

 void get_sick(); // Process diseases
// infect() gives us a chance to save (mostly from armor)
 bool infect(dis_type type, body_part vector, int strength,
              int duration, bool permanent = false, int intensity = 1,
              int max_intensity = 1, int decay = 0, int additive = 1,
              bool targeted = false, int side = -1,
              bool main_parts_only = false);
// add_disease() does NOT give us a chance to save
// num_bp indicates that specifying a body part is unessecary such as with
// drug effects
// -1 indicates that side of body is irrelevant
// -1 for intensity represents infinitely stacking
 void add_disease(dis_type type, int duration, bool permanent = false,
                   int intensity = 1, int max_intensity = 1, int decay = 0,
                   int additive = 1, body_part part = num_bp, int side = -1,
                   bool main_parts_only = false);
 void rem_disease(dis_type type, body_part part = num_bp, int side = -1);
 bool has_disease(dis_type type, body_part part = num_bp, int side = -1) const;
// Pause/unpause disease duration timers
 bool pause_disease(dis_type type, body_part part = num_bp, int side = -1);
 bool unpause_disease(dis_type type, body_part part = num_bp, int side = -1);
 int  disease_duration(dis_type type, bool all = false, body_part part = num_bp, int side = -1);
 int  disease_intensity(dis_type type, bool all = false, body_part part = num_bp, int side = -1);

 void add_addiction(add_type type, int strength);
 void rem_addiction(add_type type);
 bool has_addiction(add_type type) const;
 int  addiction_level(add_type type);

 bool siphon(vehicle *veh, ammotype desired_liquid);
 void suffer();
 void mend();
 void vomit();

 void drench(int saturation, int flags); // drenches the player in water; saturation is percent
 void drench_mut_calc(); //Recalculate mutation drench protection for all bodyparts (ignored/good/neutral stats)

 char lookup_item(char let);
 bool consume(int pos);
 bool eat(item *eat, it_comest *comest);
 void consume_effects(item *eaten, it_comest *comest, bool rotten = false);
 virtual bool wield(signed char invlet, bool autodrop = false);// Wield item; returns false on fail
 void pick_style(); // Pick a style
 bool wear(int pos, bool interactive = true); // Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion.
 bool wear_item(item *to_wear, bool interactive = true); // Wear item; returns false on fail. If interactive is false, don't alert the player or drain moves on completion.
 bool takeoff(int pos, bool autodrop = false);// Take off item; returns false on fail
 void sort_armor();      // re-order armor layering
 void use(int pos); // Use a tool
 void use_wielded();
 void remove_gunmod(item *weapon, int id);
 bool install_bionics(it_bionic* type); // Install bionics
 void read(int pos); // Read a book
 void try_to_sleep(); // '$' command; adds DIS_LYING_DOWN
 bool can_sleep(); // Checked each turn during DIS_LYING_DOWN
 void fall_asleep(int duration);
 void wake_up(const char * message = NULL);
 std::string is_snuggling();    // Check to see if the player is using floor items to keep warm. If so, return one such item
 float fine_detail_vision_mod(); // Used for things like reading and sewing, checks light level

 // helper functions meant to tell inventory display code what kind of visual feedback to give to the user
 hint_rating rate_action_use(const item *it) const; //rates usability lower for non-tools (books, etc.)
 hint_rating rate_action_wear(item *it);
 hint_rating rate_action_eat(item *it);
 hint_rating rate_action_read(item *it);
 hint_rating rate_action_takeoff(item *it);
 hint_rating rate_action_reload(item *it);
 hint_rating rate_action_unload(item *it);
 hint_rating rate_action_disassemble(item *it);

 int warmth(body_part bp); // Warmth provided by armor &c
 int encumb(body_part bp); // Encumbrance from armor &c
 int encumb(body_part bp, double &layers, int &armorenc);
 int get_armor_bash(body_part bp); // Bashing resistance, from creature
 int get_armor_cut(body_part bp); // Cutting resistance
 int get_armor_bash_base(body_part bp); // Bashing resistance
 int get_armor_cut_base(body_part bp); // Cutting  resistance
 int get_env_resist(body_part bp); // Infection &c resistance
 bool wearing_something_on(body_part bp); // True if wearing something on bp
 bool is_wearing_shoes();// True if wearing something on feet and it is not wool or not cotton
 bool is_wearing_power_armor(bool *hasHelmet = NULL) const;

 int adjust_for_focus(int amount);
 void practice(const calendar& turn, Skill *s, int amount);
 void practice(const calendar& turn, std::string s, int amount);

 void assign_activity(activity_type type, int moves, int index = -1, int pos = INT_MIN, std::string name = "");
 bool has_activity(const activity_type type);
 void cancel_activity();

 int weight_carried();
 int volume_carried();
 int weight_capacity(bool real_life = true);
 int volume_capacity();
 double convert_weight(int weight);
 bool can_eat(const item i);
 bool can_pickVolume(int volume);
 bool can_pickWeight(int weight, bool safe = true);
 int net_morale(morale_point effect);
 int morale_level(); // Modified by traits, &c
 void add_morale(morale_type type, int bonus, int max_bonus = 0,
                 int duration = 60, int decay_start = 30,
                 bool cap_existing = false, itype* item_type = NULL);
 int has_morale( morale_type type ) const;
 void rem_morale(morale_type type, itype* item_type = NULL);

 std::string weapname(bool charges = true);

 item& i_add(item it);
 // Sets invlet and adds to inventory if possible, drops otherwise, returns true if either succeeded.
 // An optional qty can be provided (and will perform better than separate calls).
 bool i_add_or_drop(item& it, int qty = 1);
 bool has_active_item(const itype_id &id) const;
 int  active_item_charges(itype_id id);
 void process_active_items();
 bool process_single_active_item(item *it); // returns false if it needs to be removed
 item i_rem(char let); // Remove item from inventory; returns ret_null on fail
 item i_rem(signed char ch) { return i_rem((char) ch); }  // prevent signed char->int conversion
 item i_rem(int pos); // Remove item from inventory; returns ret_null on fail
 item i_rem(itype_id type);// Remove first item w/ this type; fail is ret_null
 item i_rem(item *it);// Remove specific item.
 item remove_weapon();
 void remove_mission_items(int mission_id);
 item reduce_charges(int position, int quantity);
 item i_remn(char invlet);// Remove item from inventory; returns ret_null on fail
 item &i_at(char let); // Returns the item with inventory letter let
 item &i_at(int position);  // Returns the item with a given inventory position.
 item &i_of_type(itype_id type); // Returns the first item with this type
 char position_to_invlet(int position);
 int invlet_to_position(char invlet);
 int get_item_position(item* it);  // looks up an item (via pointer comparison)
 martialart get_combat_style(); // Returns the combat style object
 std::vector<item *> inv_dump(); // Inventory + weapon + worn (for death, etc)
 int  butcher_factor(); // Automatically picks our best butchering tool
 item*  pick_usb(); // Pick a usb drive, interactively if it matters
 bool is_wearing(const itype_id & it) const; // Are we wearing a specific itype?
 bool has_artifact_with(const art_effect_passive effect) const;
 bool worn_with_flag( std::string flag ) const;

 bool covered_with_flag( const std::string flag, int parts ) const;
 bool covered_with_flag_exclusively( const std::string flag, int parts = -1 ) const;
 bool is_water_friendly( int flags = -1 ) const;
 bool is_waterproof( int flags ) const;

// has_amount works ONLY for quantity.
// has_charges works ONLY for charges.
 std::list<item> use_amount(itype_id it, int quantity, bool use_container = false);
 bool use_charges_if_avail(itype_id it, int quantity);// Uses up charges
 std::list<item> use_charges(itype_id it, int quantity);// Uses up charges
 bool has_amount(itype_id it, int quantity);
 bool has_charges(itype_id it, int quantity);
 int  amount_of(itype_id it);
 int  charges_of(itype_id it);

 int  leak_level( std::string flag ) const; // carried items may leak radiation or chemicals

 bool has_watertight_container();
 bool has_matching_liquid(itype_id it);
 bool has_weapon_or_armor(char let) const; // Has an item with invlet let
 bool has_item_with_flag( std::string flag ) const; // Has a weapon, inventory item or worn item with flag
 bool has_item(char let);  // Has an item with invlet let
 bool has_item(int position);
 bool has_item(item *it);  // Has a specific item
 bool has_mission_item(int mission_id); // Has item with mission_id
 std::vector<item*> has_ammo(ammotype at);// Returns a list of the ammo

 bool has_weapon();

 bool knows_recipe(recipe *rec);
 void learn_recipe(recipe *rec);

 bool can_study_recipe(it_book *book);
 bool studied_all_recipes(it_book *book);
 bool try_study_recipe(it_book *book);

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
 inline int xpos() { return posx; }
 inline int ypos() { return posy; }
 int view_offset_x, view_offset_y;
 bool in_vehicle;       // Means player sit inside vehicle on the tile he is now
 bool controlling_vehicle;  // Is currently in control of a vehicle
 // Relative direction of a grab, add to posx, posy to get the coordinates of the grabbed thing.
 point grab_point;
 object_type grab_type;
 player_activity activity;
 player_activity backlog;
// _missions vectors are of mission IDs
 std::vector<int> active_missions;
 std::vector<int> completed_missions;
 std::vector<int> failed_missions;
 int active_mission;
 int volume;

 std::string name;
 bool male;
 profession* prof;

 std::map<std::string, int> mutation_category_level;

 int next_climate_control_check;
 bool last_climate_control_ret;
 int power_level, max_power_level;
 int hunger, thirst, fatigue, health;
 int oxygen;
 unsigned int recoil;
 unsigned int driving_recoil;
 unsigned int scent;
 int dodges_left, blocks_left;
 int stim, pkill, radiation;
 int cash;
 int movecounter;
 int hp_cur[num_hp_parts], hp_max[num_hp_parts];
 signed int temp_cur[num_bp], frostbite_timer[num_bp], temp_conv[num_bp];
 void temp_equalizer(body_part bp1, body_part bp2); // Equalizes heat between body parts
 bool nv_cached;

 // Drench cache
 std::map<int, std::map<std::string, int> > mMutDrench;
 std::map<int, int> mDrenchEffect;

 std::vector<morale_point> morale;

 int focus_pool;

 SkillLevel& skillLevel(Skill* _skill);
 SkillLevel& skillLevel(std::string ident);

    // for serialization
    SkillLevel get_skill_level(Skill* _skill) const;
    SkillLevel get_skill_level(const std::string &ident) const;

 void set_skill_level(Skill* _skill, int level);
 void set_skill_level(std::string ident, int level);

 void boost_skill_level(Skill* _skill, int level);
 void boost_skill_level(std::string ident, int level);

 void copy_skill_levels(const player *rhs);

 std::map<std::string, recipe*> learned_recipes;

 inventory inv;
 itype_id last_item;
 std::vector<item> worn;
 std::vector<matype_id> ma_styles;
 matype_id style_selected;

 item weapon;
 item ret_null; // Null item, sometimes returns by weapon() etc

 std::vector <addiction> addictions;

 recipe* lastrecipe;
 itype_id lastconsumed;        //used in crafting.cpp and construction.cpp

 //Dumps all memorial events into a single newline-delimited string
 std::string dump_memorial();
 //Log an event, to be later written to the memorial file
 void add_memorial_log(const char* message, ...);
 //Loads the memorial log from a file
 void load_memorial_file(std::ifstream &fin);
 //Notable events, to be printed in memorial
 std::vector <std::string> memorial_log;

    //Record of player stats, for posterity only
    stats* lifetime_stats();
    stats get_stats() const; // for serialization

 int getID () const;

 bool is_underwater() const;
 void set_underwater(bool);

 void environmental_revert_effect();

 bool is_invisible() const;
 int visibility( bool check_color = false, int stillness = 0 ) const; // just checks is_invisible for the moment
 // -2 position is 0 worn index, -3 position is 1 worn index, etc
 static int worn_position_to_index(int position) {
     return -2 - position;
 }

 m_size get_size();

protected:
    std::set<std::string> my_traits;
    std::set<std::string> my_mutations;
    std::vector<bionic> my_bionics;
    std::vector<disease> illness;
    bool underwater;

    int sight_max;
    int sight_boost;
    int sight_boost_cap;

    void setID (int i);

private:
    bool has_fire(const int quantity);
    void use_fire(const int quantity);

    std::vector<point> auto_move_route;
    // Used to make sure auto move is canceled if we stumble off course
    point next_expected_position;

    int id; // A unique ID number, assigned by the game class private so it cannot be overwritten and cause save game corruptions.
    //NPCs also use this ID value. Values should never be reused.
};

#endif
