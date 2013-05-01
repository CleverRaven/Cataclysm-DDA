#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "item.h"
#include "monster.h"
#include "pldata.h"
#include "skill.h"
#include "bionics.h"
#include "trap.h"
#include "morale.h"
#include "inventory.h"
#include "artifact.h"
#include "mutation.h"
#include "crafting.h"
#include <vector>
#include <string>
#include <map>

class monster;
class game;
struct trap;
struct mission;
class profession;

struct special_attack
{
 std::string text;
 int bash;
 int cut;
 int stab;

 special_attack() { bash = 0; cut = 0; stab = 0; };
};

class player {
  std::map<Skill*,SkillLevel> _skills;

public:
 player();
 player(const player &rhs);
 virtual ~player();

 player& operator= (const player & rhs);

// newcharacter.cpp
 bool create(game *g, character_type type, std::string tempname = "");
 int  random_good_trait();
 int  random_bad_trait ();
 void normalize(game *g);	// Starting set up of HP and inventory
// </newcharacter.cpp>

 void pick_name(); // Picks a name from NAMES_*

 virtual bool is_npc() { return false; }	// Overloaded for NPCs in npc.h
 nc_color color();				// What color to draw us as

 virtual void load_info(game *g, std::string data);// Load from file 'name.sav'
 virtual std::string save_info();		// Save to file matching name

 void disp_info(game *g);	// '@' key; extended character info
 void disp_morale(game *g);		// '%' key; morale info
 void disp_status(WINDOW* w, game *g = NULL);// On-screen data

 void reset(game *g = NULL);// Resets movement points, stats, applies effects
 void update_morale();	// Ticks down morale counters and removes them
 void update_bodytemp(game *g);  // Maintains body temperature
 int  current_speed(game *g = NULL); // Number of movement points we get a turn
 int  run_cost(int base_cost); // Adjust base_cost
 int  swim_speed();	// Our speed when swimming

 bool has_trait(int flag) const;
 bool has_mutation(int flag) const;
 void toggle_trait(int flag);

 bool has_bionic(bionic_id b) const;
 bool has_active_bionic(bionic_id b) const;
 void add_bionic(bionic_id b);
 void charge_power(int amount);
 void power_bionics(game *g);
 void activate_bionic(int b, game *g);
 float active_light();

 bool mutation_ok(game *g, pl_flag mutation, bool force_good, bool force_bad);
 void mutate(game *g);
 void mutate_towards(game *g, pl_flag mut);
 void remove_mutation(game *g, pl_flag mut);
 bool has_child_flag(game *g, pl_flag mut);
 void remove_child_flag(game *g, pl_flag mut);

 int  sight_range(int light_level);
 int  unimpaired_range();
 int  overmap_sight_range(int light_level);
 int  clairvoyance(); // Sight through walls &c
 bool sight_impaired(); // vision impaired between sight_range and max_range
 bool has_two_arms() const;
 bool can_wear_boots();
 bool is_armed();	// True if we're wielding something; true for bionics
 bool unarmed_attack(); // False if we're wielding something; true for bionics
 bool avoid_trap(trap *tr);

 void pause(game *g); // '.' command; pauses & reduces recoil

// melee.cpp
 int  hit_mon(game *g, monster *z, bool allow_grab = true);
 void hit_player(game *g, player &p, bool allow_grab = true);

 int base_damage(bool real_life = true, int stat = -999);
 int base_to_hit(bool real_life = true, int stat = -999);

 int  hit_roll(); // Our basic hit roll, compared to our target's dodge roll
 bool scored_crit(int target_dodge = 0); // Critical hit?

 int roll_bash_damage(monster *z, bool crit);
 int roll_cut_damage(monster *z, bool crit);
 int roll_stab_damage(monster *z, bool crit);
 int roll_stuck_penalty(monster *z, bool stabbing);

 technique_id pick_technique(game *g, monster *z, player *p,
                             bool crit, bool allowgrab);
 void perform_technique(technique_id technique, game *g, monster *z, player *p,
                       int &bash_dam, int &cut_dam, int &pierce_dam, int &pain);

 technique_id pick_defensive_technique(game *g, monster *z, player *p);

 void perform_defensive_technique(technique_id technique, game *g, monster *z,
                                  player *p, body_part &bp_hit, int &side,
                                  int &bash_dam, int &cut_dam, int &stab_dam);

 void perform_special_attacks(game *g, monster *z, player *p,
                        int &bash_dam, int &cut_dam, int &pierce_dam);

 std::vector<special_attack> mutation_attacks(monster *z, player *p);
 void melee_special_effects(game *g, monster *z, player *p, bool crit,
                            int &bash_dam, int &cut_dam, int &stab_dam);

 int  dodge(game *g);     // Returns the players's dodge, modded by clothing etc
 int  dodge_roll(game *g);// For comparison to hit_roll()

// ranged.cpp
 int throw_range(char invlet); // Range of throwing item; -1:ERR 0:Can't throw
 int ranged_dex_mod	(bool real_life = true);
 int ranged_per_mod	(bool real_life = true);
 int throw_dex_mod	(bool real_life = true);

// Mental skills and stats
 int comprehension_percent(skill s, bool real_life = true);
 int read_speed		(bool real_life = true);
 int talk_skill(); // Skill at convincing NPCs of stuff
 int intimidation(); // Physical intimidation

// Converts bphurt to a hp_part (if side == 0, the left), then does/heals dam
// hit() processes damage through armor
 void hit   (game *g, body_part bphurt, int side, int  dam, int  cut);
// absorb() reduces dam and cut by your armor (and bionics, traits, etc)
 void absorb(game *g, body_part bp,               int &dam, int &cut);
// hurt() doesn't--effects of disease, what have you
 void hurt  (game *g, body_part bphurt, int side, int  dam);

 void heal(body_part healed, int side, int dam);
 void heal(hp_part healed, int dam);
 void healall(int dam);
 void hurtall(int dam);
 // checks armor. if vary > 0, then damage to parts are random within 'vary' percent (1-100)
 void hitall(game *g, int dam, int vary = 0);
// Sends us flying one tile
 void knock_back_from(game *g, int x, int y);

 int hp_percentage();	// % of HP remaining, overall

 void get_sick(game *g);	// Process diseases
// infect() gives us a chance to save (mostly from armor)
 void infect(dis_type type, body_part vector, int strength, int duration,
             game *g);
// add_disease() does NOT give us a chance to save
 void add_disease(dis_type type, int duration, game *g, int intensity = 0,
                  int max_intensity = -1);
 void rem_disease(dis_type type);
 bool has_disease(dis_type type) const;
 int  disease_level(dis_type type);
 int  disease_intensity(dis_type type);

 void add_addiction(add_type type, int strength);
 void rem_addiction(add_type type);
 bool has_addiction(add_type type) const;
 int  addiction_level(add_type type);

 void cauterize(game *g);
 void suffer(game *g);
 void vomit(game *g);

 char lookup_item(char let);
 bool eat(game *g, char invlet);	// Eat item; returns false on fail
 virtual bool wield(game *g, char invlet);// Wield item; returns false on fail
 void pick_style(game *g); // Pick a style
 bool wear(game *g, char let);	// Wear item; returns false on fail
 bool wear_item(game *g, item *to_wear);
 bool takeoff(game *g, char let);// Take off item; returns false on fail
 void use(game *g, char let);	// Use a tool
 void use_wielded(game *g);
 bool install_bionics(game *g, it_bionic* type);	// Install bionics
 void read(game *g, char let);	// Read a book
 void try_to_sleep(game *g);	// '$' command; adds DIS_LYING_DOWN
 bool can_sleep(game *g);	// Checked each turn during DIS_LYING_DOWN
 bool can_see_fine_detail(game *g); // Used for things like reading and sewing, checks light level

 // helper functions meant to tell inventory display code what kind of visual feedback to give to the user
 hint_rating rate_action_use(item *it); //rates usability lower for non-tools (books, etc.)
 hint_rating rate_action_wear(item *it);
 hint_rating rate_action_eat(item *it);
 hint_rating rate_action_read(item *it, game *g);
 hint_rating rate_action_takeoff(item *it);
 hint_rating rate_action_reload(item *it);
 hint_rating rate_action_unload(item *it);
 hint_rating rate_action_disassemble(item *it, game *g);

 int warmth(body_part bp);	// Warmth provided by armor &c
 int encumb(body_part bp);	// Encumberance from armor &c
 int encumb(body_part bp, int &layers, int &armorenc, int &warmth);
 int armor_bash(body_part bp);	// Bashing resistance
 int armor_cut(body_part bp);	// Cutting  resistance
 int resist(body_part bp);	// Infection &c resistance
 bool wearing_something_on(body_part bp); // True if wearing something on bp

 void practice(const calendar& turn, Skill *s, int amount);
 void practice(const calendar& turn, std::string s, int amount);

 void assign_activity(game* g, activity_type type, int moves, int index = -1, char invlet = 0);
 void cancel_activity();

 int weight_carried();
 int volume_carried();
 int weight_capacity(bool real_life = true);
 int volume_capacity();
 bool can_pickVolume(int volume);
 bool can_pickWeight(int weight);
 int morale_level();	// Modified by traits, &c
 void add_morale(morale_type type, int bonus, int max_bonus = 0,
                 itype* item_type = NULL);

 std::string weapname(bool charges = true);

 item& i_add(item it, game *g = NULL);
 bool has_active_item(itype_id id);
 int  active_item_charges(itype_id id);
 void process_active_items(game *g);
 item i_rem(char let);	// Remove item from inventory; returns ret_null on fail
 item i_rem(itype_id type);// Remove first item w/ this type; fail is ret_null
 item remove_weapon();
 void remove_mission_items(int mission_id);
 item i_remn(char invlet);// Remove item from inventory; returns ret_null on fail
 item &i_at(char let);	// Returns the item with inventory letter let
 item &i_of_type(itype_id type); // Returns the first item with this type
 std::vector<item> inv_dump(); // Inventory + weapon + worn (for death, etc)
 int  butcher_factor();	// Automatically picks our best butchering tool
 item*  pick_usb(); // Pick a usb drive, interactively if it matters
 bool is_wearing(itype_id it);	// Are we wearing a specific itype?
 bool has_artifact_with(art_effect_passive effect);

// has_amount works ONLY for quantity.
// has_charges works ONLY for charges.
 void use_amount(itype_id it, int quantity, bool use_container = false);
 bool use_charges_if_avail(itype_id it, int quantity);// Uses up charges
 void use_charges(itype_id it, int quantity);// Uses up charges
 bool has_amount(itype_id it, int quantity);
 bool has_charges(itype_id it, int quantity);
 int  amount_of(itype_id it);
 int  charges_of(itype_id it);

 bool has_watertight_container();
 bool has_matching_liquid(itype_id it);
 bool has_weapon_or_armor(char let) const;	// Has an item with invlet let
 bool has_item(char let);		// Has an item with invlet let
 bool has_item(item *it);		// Has a specific item
 bool has_mission_item(int mission_id);	// Has item with mission_id
 std::vector<item*> has_ammo(ammotype at);// Returns a list of the ammo

 bool knows_recipe(recipe *rec);
 void learn_recipe(recipe *rec);

 bool can_study_recipe(it_book *book);
 bool studied_all_recipes(it_book *book);
 void try_study_recipe(game *g, it_book *book);

// ---------------VALUES-----------------
 int posx, posy;
 int view_offset_x, view_offset_y;
 bool in_vehicle;       // Means player sit inside vehicle on the tile he is now
 player_activity activity;
 player_activity backlog;
// _missions vectors are of mission IDs
 std::vector<int> active_missions;
 std::vector<int> completed_missions;
 std::vector<int> failed_missions;
 int active_mission;

 std::string name;
 bool male;
 profession* prof;
 bool my_traits[PF_MAX2];
 bool my_mutations[PF_MAX2];
 int mutation_category_level[NUM_MUTATION_CATEGORIES];
 std::vector<bionic> my_bionics;
// Current--i.e. modified by disease, pain, etc.
 int str_cur, dex_cur, int_cur, per_cur;
// Maximum--i.e. unmodified by disease
 int str_max, dex_max, int_max, per_max;
 int power_level, max_power_level;
 int hunger, thirst, fatigue, health;
 bool underwater;
 int oxygen;
 unsigned int recoil;
 unsigned int driving_recoil;
 unsigned int scent;
 int dodges_left, blocks_left;
 int stim, pain, pkill, radiation;
 int cash;
 int moves;
 int hp_cur[num_hp_parts], hp_max[num_hp_parts];
 signed int temp_cur[num_bp], frostbite_timer[num_bp], temp_conv[num_bp];
 void temp_equalizer(body_part bp1, body_part bp2); // Equalizes heat between body parts

 std::vector<morale_point> morale;

 int xp_pool;
 int sklevel[num_skill_types];
 int skexercise[num_skill_types];
 int sktrain[num_skill_types];
 bool sklearn[num_skill_types];

 SkillLevel& skillLevel(Skill* _skill);
 SkillLevel& skillLevel(std::string ident);
 SkillLevel& skillLevel(size_t id);

 std::map<std::string, recipe*> learned_recipes;

 inventory inv;
 itype_id last_item;
 std::vector <item> worn;
 std::vector<itype_id> styles;
 itype_id style_selected;
 item weapon;
 item ret_null;	// Null item, sometimes returns by weapon() etc

 std::vector <disease> illness;
 std::vector <addiction> addictions;

 recipe* lastrecipe;
 int getID ();
protected:
    void setID (int i);
private:
 bool has_fire(const int quantity);
 void use_fire(const int quantity);

    int id;	// A unique ID number, assigned by the game class private so it cannot be overwritten and cause save game corruptions.
    //NPCs also use this ID value. Values should never be reused.
};

#endif
