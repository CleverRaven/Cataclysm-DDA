#include "player.h"
#include "profession.h"
#include "bionics.h"
#include "mission.h"
#include "game.h"
#include "disease.h"
#include "addiction.h"
#include "keypress.h"
#include "moraledata.h"
#include "inventory.h"
#include "artifact.h"
#include "options.h"
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include "weather.h"
#include "item.h"
#include "material.h"
#include "translations.h"
#include "name.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "disease.h"
#include "get_version.h"
#include "crafting.h"
#include "monstergenerator.h"
#include "help.h" // get_hint
#include "martialarts.h"
#include "output.h"

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

#include <ctime>
#include <algorithm>
#include <numeric>

nc_color encumb_color(int level);
bool activity_is_suspendable(activity_type type);
static void manage_fire_exposure(player& p, int fireStrength = 1);
static void handle_cough(player& p, int intensity = 1, int volume = 12);

std::map<std::string, trait> traits;
std::map<std::string, martialart> ma_styles;
std::vector<std::string> vStartingTraits[2];

std::string morale_data[NUM_MORALE_TYPES];

stats player_stats;

void game::init_morale()
{
    std::string tmp_morale_data[NUM_MORALE_TYPES] = {
    "This is a bug (moraledata.h:moraledata)",
    _("Enjoyed %i"),
    _("Enjoyed a hot meal"),
    _("Music"),
    _("Played Video Game"),
    _("Marloss Bliss"),
    _("Good Feeling"),

    _("Nicotine Craving"),
    _("Caffeine Craving"),
    _("Alcohol Craving"),
    _("Opiate Craving"),
    _("Speed Craving"),
    _("Cocaine Craving"),
    _("Crack Cocaine Craving"),

    _("Disliked %i"),
    _("Ate Human Flesh"),
    _("Ate Meat"),
    _("Ate Vegetables"),
    _("Wet"),
    _("Dried Off"),
    _("Cold"),
    _("Hot"),
    _("Bad Feeling"),
    _("Killed Innocent"),
    _("Killed Friend"),
    _("Guilty about Killing"),
    _("Chimerical Mutation"),
    _("Fey Mutation"),

    _("Moodswing"),
    _("Read %i"),
    _("Heard Disturbing Scream"),

    _("Masochism"),
    _("Hoarder"),
    _("Stylish"),
    _("Optimist"),
    _("Bad Tempered"),
    _("Found kitten <3")
    };
    for(int i=0; i<NUM_MORALE_TYPES; i++){morale_data[i]=tmp_morale_data[i];}
}

player::player() : Character(), name("")
{
 id = 0; // Player is 0. NPCs are different.
 view_offset_x = 0;
 view_offset_y = 0;
 str_cur = 8;
 str_max = 8;
 dex_cur = 8;
 dex_max = 8;
 int_cur = 8;
 int_max = 8;
 per_cur = 8;
 per_max = 8;
 underwater = false;
 dodges_left = 1;
 blocks_left = 1;
 power_level = 0;
 max_power_level = 0;
 hunger = 0;
 thirst = 0;
 fatigue = 0;
 stim = 0;
 pain = 0;
 pkill = 0;
 radiation = 0;
 cash = 0;
 recoil = 0;
 driving_recoil = 0;
 scent = 500;
 health = 0;
 male = true;
 prof = profession::has_initialized() ? profession::generic() : NULL; //workaround for a potential structural limitation, see player::create
 moves = 100;
 movecounter = 0;
 oxygen = 0;
 next_climate_control_check=0;
 last_climate_control_ret=false;
 active_mission = -1;
 in_vehicle = false;
 controlling_vehicle = false;
 grab_point.x = 0;
 grab_point.y = 0;
 grab_type = OBJECT_NONE;
 style_selected = "style_none";
 focus_pool = 100;
 last_item = itype_id("null");
 sight_max = 9999;
 sight_boost = 0;
 sight_boost_cap = 0;
 lastrecipe = NULL;
 next_expected_position.x = -1;
 next_expected_position.y = -1;

 for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
    my_traits.erase(iter->first);
    my_mutations.erase(iter->first);
 }

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
      aSkill != Skill::skills.end(); ++aSkill) {
   skillLevel(*aSkill).level(0);
 }

 for (int i = 0; i < num_bp; i++) {
  temp_cur[i] = BODYTEMP_NORM;
  frostbite_timer[i] = 0;
  temp_conv[i] = BODYTEMP_NORM;
 }
 nv_cached = false;
 volume = 0;

 memorial_log.clear();
 player_stats.reset();

 mDrenchEffect[bp_eyes] = 1;
 mDrenchEffect[bp_mouth] = 1;
 mDrenchEffect[bp_head] = 7;
 mDrenchEffect[bp_legs] = 21;
 mDrenchEffect[bp_feet] = 6;
 mDrenchEffect[bp_arms] = 19;
 mDrenchEffect[bp_hands] = 5;
 mDrenchEffect[bp_torso] = 40;

 recalc_sight_limits();
}

player::player(const player &rhs): Character(rhs), JsonSerializer(), JsonDeserializer()
{
 *this = rhs;
}

player::~player()
{
}

player& player::operator= (const player & rhs)
{
 id = rhs.id;
 posx = rhs.posx;
 posy = rhs.posy;
 view_offset_x = rhs.view_offset_x;
 view_offset_y = rhs.view_offset_y;

 in_vehicle = rhs.in_vehicle;
 controlling_vehicle = rhs.controlling_vehicle;
 grab_point = rhs.grab_point;
 activity = rhs.activity;
 backlog = rhs.backlog;

 active_missions = rhs.active_missions;
 completed_missions = rhs.completed_missions;
 failed_missions = rhs.failed_missions;
 active_mission = rhs.active_mission;

 name = rhs.name;
 male = rhs.male;
 prof = rhs.prof;

 sight_max = rhs.sight_max;
 sight_boost = rhs.sight_boost;
 sight_boost_cap = rhs.sight_boost_cap;

 my_traits = rhs.my_traits;
 my_mutations = rhs.my_mutations;
 mutation_category_level = rhs.mutation_category_level;

 my_bionics = rhs.my_bionics;

 str_cur = rhs.str_cur;
 dex_cur = rhs.dex_cur;
 int_cur = rhs.int_cur;
 per_cur = rhs.per_cur;

 str_max = rhs.str_max;
 dex_max = rhs.dex_max;
 int_max = rhs.int_max;
 per_max = rhs.per_max;

 power_level = rhs.power_level;
 max_power_level = rhs.max_power_level;

 hunger = rhs.hunger;
 thirst = rhs.thirst;
 fatigue = rhs.fatigue;
 health = rhs.health;

 underwater = rhs.underwater;
 oxygen = rhs.oxygen;
 next_climate_control_check=rhs.next_climate_control_check;
 last_climate_control_ret=rhs.last_climate_control_ret;

 recoil = rhs.recoil;
 driving_recoil = rhs.driving_recoil;
 scent = rhs.scent;
 dodges_left = rhs.dodges_left;
 blocks_left = rhs.blocks_left;

 stim = rhs.stim;
 pain = rhs.pain;
 pkill = rhs.pkill;
 radiation = rhs.radiation;

 cash = rhs.cash;
 moves = rhs.moves;
 movecounter = rhs.movecounter;

 for (int i = 0; i < num_hp_parts; i++)
  hp_cur[i] = rhs.hp_cur[i];

 for (int i = 0; i < num_hp_parts; i++)
  hp_max[i] = rhs.hp_max[i];

 for (int i = 0; i < num_bp; i++)
  temp_cur[i] = rhs.temp_cur[i];

 for (int i = 0 ; i < num_bp; i++)
  temp_conv[i] = rhs.temp_conv[i];

 for (int i = 0; i < num_bp; i++)
  frostbite_timer[i] = rhs.frostbite_timer[i];

 morale = rhs.morale;
 focus_pool = rhs.focus_pool;

 _skills = rhs._skills;

 learned_recipes = rhs.learned_recipes;

 inv.clear();
 for (int i = 0; i < rhs.inv.size(); i++)
  inv.add_stack(rhs.inv.const_stack(i));

 volume = rhs.volume;

 lastrecipe = rhs.lastrecipe;
 last_item = rhs.last_item;
 worn = rhs.worn;
 ma_styles = rhs.ma_styles;
 style_selected = rhs.style_selected;
 weapon = rhs.weapon;

 ret_null = rhs.ret_null;

 illness = rhs.illness;
 addictions = rhs.addictions;

 nv_cached = false;

 return (*this);
}

void player::normalize()
{
    Creature::normalize();

 ret_null = item(itypes["null"], 0);
 weapon   = item(itypes["null"], 0);
 style_selected = "style_none";
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = 60 + str_max * 3;
  if (has_trait("TOUGH"))
   hp_max[i] = int(hp_max[i] * 1.2);
  hp_cur[i] = hp_max[i];
 }
 for (int i = 0 ; i < num_bp; i++)
  temp_conv[i] = BODYTEMP_NORM;
}

void player::pick_name() {
    name = Name::generate(male);
}

std::string player::disp_name() {
    if (is_player())
        return "you";
    return name;
}

std::string player::skin_name() {
    return "thin skin";
}

// just a shim for now since actual player death is handled in game::is_game_over
void player::die(Creature* nkiller) {
    killer = nkiller;
}

void player::reset_stats()
{
    // We can dodge again!
    blocks_left = get_num_blocks();
    dodges_left = get_num_dodges();

    suffer();

    // Didn't just pick something up
    last_item = itype_id("null");

    // Bionic buffs
    if (has_active_bionic("bio_hydraulics"))
        mod_str_bonus(20);
    if (has_bionic("bio_eye_enhancer"))
        mod_per_bonus(2);
    if (has_bionic("bio_str_enhancer"))
        mod_str_bonus(2);
    if (has_bionic("bio_int_enhancer"))
        mod_int_bonus(2);
    if (has_bionic("bio_dex_enhancer"))
        mod_dex_bonus(2);
    if (has_active_bionic("bio_metabolics") && power_level < max_power_level &&
            hunger < 100 && (int(g->turn) % 5 == 0)) {
        hunger += 2;
        power_level++;
    }

    // Trait / mutation buffs
    if (has_trait("THICK_SCALES"))
        mod_dex_bonus(-2);
    if (has_trait("CHITIN2") || has_trait("CHITIN3"))
        mod_dex_bonus(-1);
    if (has_trait("COMPOUND_EYES") && !wearing_something_on(bp_eyes))
        mod_per_bonus(1);
    if (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
            has_trait("ARM_TENTACLES_8"))
        mod_dex_bonus(1);

    // Pain
    if (pain > pkill) {
        mod_str_bonus(-int((pain - pkill) / 15));
        mod_dex_bonus(-int((pain - pkill) / 15));
        mod_per_bonus(-int((pain - pkill) / 20));
        mod_int_bonus(-(1 + int((pain - pkill) / 25)));
    }
    // Morale
    if (abs(morale_level()) >= 100) {
        mod_str_bonus(int(morale_level() / 180));
        mod_dex_bonus(int(morale_level() / 200));
        mod_per_bonus(int(morale_level() / 125));
        mod_int_bonus(int(morale_level() / 100));
    }
    // Radiation
    if (radiation > 0) {
        mod_str_bonus(-int(radiation / 80));
        mod_dex_bonus(-int(radiation / 110));
        mod_per_bonus(-int(radiation / 100));
        mod_int_bonus(-int(radiation / 120));
    }
    // Stimulants
    mod_dex_bonus(int(stim / 10));
    mod_per_bonus(int(stim /  7));
    mod_int_bonus(int(stim /  6));
    if (stim >= 30) {
        mod_dex_bonus(-int(abs(stim - 15) /  8));
        mod_per_bonus(-int(abs(stim - 15) / 12));
        mod_int_bonus(-int(abs(stim - 15) / 14));
    }

    // Dodge-related effects
    mod_dodge_bonus(
            mabuff_dodge_bonus()
            - encumb(bp_legs)/2
            - encumb(bp_torso)
        );
    if (has_trait("TAIL_LONG")) {mod_dodge_bonus(2);}
    if (has_trait("TAIL_CATTLE")) {mod_dodge_bonus(1);}
    if (has_trait("TAIL_RAT")) {mod_dodge_bonus(2);}
    if (has_trait("TAIL_THICK")) {mod_dodge_bonus(1);}
    if (has_trait("TAIL_RAPTOR")) {mod_dodge_bonus(3);}
    if (has_trait("TAIL_FLUFFY")) {mod_dodge_bonus(4);}
    if (has_trait("WHISKERS")) {mod_dodge_bonus(1);}
    if (has_trait("WINGS_BAT")) {mod_dodge_bonus(-3);}

    if (str_max >= 16) {mod_dodge_bonus(-1);} // Penalty if we're huge
    else if (str_max <= 5) {mod_dodge_bonus(1);} // Bonus if we're small

    // Hit-related effects
    mod_hit_bonus(
            mabuff_tohit_bonus()
            + weapon.type->m_to_hit
            - encumb(bp_torso)
        );

    // Set our scent towards the norm
    int norm_scent = 500;
    if (has_trait("WEAKSCENT"))
        norm_scent = 300;
    if (has_trait("SMELLY"))
        norm_scent = 800;
    if (has_trait("SMELLY2"))
        norm_scent = 1200;

    // Scent increases fast at first, and slows down as it approaches normal levels.
    // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
    // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
    if (scent < rng(0, norm_scent))
        scent++;

    // Unusually high scent decreases steadily until it reaches normal levels.
    if (scent > norm_scent)
        scent--;

    // Apply static martial arts buffs
    ma_static_effects();

    if (int(g->turn) % 10 == 0) {
        update_mental_focus();
    }
    nv_cached = false;

    recalc_sight_limits();
    recalc_speed_bonus();

    Creature::reset_stats();

}

void player::action_taken()
{
    nv_cached = false;
}

void player::update_morale()
{
    // Decay existing morale entries.
    for (int i = 0; i < morale.size(); i++)
    {
        // Age the morale entry by one turn.
        morale[i].age += 1;

        // If it's past its expiration date, remove it.
        if (morale[i].age >= morale[i].duration)
        {
            morale.erase(morale.begin() + i);
            i--;

            // Future-proofing.
            continue;
        }

        // We don't actually store the effective strength; it gets calculated when we
        // need it.
    }

    // We reapply persistent morale effects after every decay step, to keep them fresh.
    apply_persistent_morale();
}

void player::apply_persistent_morale()
{
    // Hoarders get a morale penalty if they're not carrying a full inventory.
    if (has_trait("HOARDER"))
    {
        int pen = int((volume_capacity()-volume_carried()) / 2);
        if (pen > 70)
        {
            pen = 70;
        }
        if (pen <= 0)
        {
            pen = 0;
        }
        if (has_disease("took_xanax"))
        {
            pen = int(pen / 7);
        }
        else if (has_disease("took_prozac"))
        {
            pen = int(pen / 2);
        }
        add_morale(MORALE_PERM_HOARDER, -pen, -pen, 5, 5, true);
    }

    // The stylish get a morale bonus for each body part covered in an item
    // with the FANCY or SUPER_FANCY tag.
    if (has_trait("STYLISH"))
    {
        int bonus = 0;
        std::string basic_flag = "FANCY";
        std::string bonus_flag = "SUPER_FANCY";

        unsigned char covered = 0; // body parts covered
        for(int i=0; i<worn.size(); i++) {
            if(worn[i].has_flag(basic_flag) || worn[i].has_flag(bonus_flag) ) {
                it_armor* item_type = (it_armor*) worn[i].type;
                covered |= item_type->covers;
            }
            if(worn[i].has_flag(bonus_flag)) {
              bonus+=2;
            }
        }
        if(covered & mfb(bp_torso)) {
            bonus += 6;
        }
        if(covered & mfb(bp_legs)) {
            bonus += 4;
        }
        if(covered & mfb(bp_feet)) {
            bonus += 2;
        }
        if(covered & mfb(bp_hands)) {
            bonus += 2;
        }
        if(covered & mfb(bp_head)) {
            bonus += 3;
        }
        if(covered & mfb(bp_eyes)) {
            bonus += 2;
        }

        if(bonus) {
            add_morale(MORALE_PERM_FANCY, bonus, bonus, 5, 5, true);
        }
    }

    // Masochists get a morale bonus from pain.
    if (has_trait("MASOCHIST"))
    {
        int bonus = pain / 2.5;
        if (bonus > 25)
        {
            bonus = 25;
        }
        if (has_disease("took_prozac"))
        {
            bonus = int(bonus / 3);
        }
        if (bonus != 0)
        {
            add_morale(MORALE_PERM_MASOCHIST, bonus, bonus, 5, 5, true);
        }
    }

    // Optimist gives a base +4 to morale.
    // The +25% boost from optimist also applies here, for a net of +5.
    if (has_trait("OPTIMISTIC"))
    {
        add_morale(MORALE_PERM_OPTIMIST, 4, 4, 5, 5, true);
    }
    
    // And Bad Temper works just the same way.  But in reverse.  ):
    if (has_trait("BADTEMPER"))
    {
        add_morale(MORALE_PERM_BADTEMPER, -4, -4, 5, 5, true);
    }
}

void player::update_mental_focus()
{
    int focus_gain_rate = calc_focus_equilibrium() - focus_pool;

    // handle negative gain rates in a symmetric manner
    int base_change = 1;
    if (focus_gain_rate < 0)
    {
        base_change = -1;
        focus_gain_rate = -focus_gain_rate;
    }

    // for every 100 points, we have a flat gain of 1 focus.
    // for every n points left over, we have an n% chance of 1 focus
    int gain = focus_gain_rate / 100;
    if (rng(1, 100) <= (focus_gain_rate % 100))
    {
        gain++;
    }

    focus_pool += (gain * base_change);
}

// written mostly by FunnyMan3595 in Github issue #613 (DarklingWolf's repo),
// with some small edits/corrections by Soron
int player::calc_focus_equilibrium()
{
    // Factor in pain, since it's harder to rest your mind while your body hurts.
    int eff_morale = morale_level() - pain;
    int focus_gain_rate = 100;

    if (activity.type == ACT_READ)
    {
        it_book* reading;
        if (this->activity.index == -2)
        {
            reading = dynamic_cast<it_book *>(weapon.type);
        }
        else
        {
            reading = dynamic_cast<it_book *>(inv.find_item(activity.position).type);
        }
        // apply a penalty when we're actually learning something
        if (skillLevel(reading->type) < (int)reading->level)
        {
            focus_gain_rate -= 50;
        }
    }

    if (eff_morale < -99)
    {
        // At very low morale, focus goes up at 1% of the normal rate.
        focus_gain_rate = 1;
    }
    else if (eff_morale <= 50)
    {
        // At -99 to +50 morale, each point of morale gives 1% of the normal rate.
        focus_gain_rate += eff_morale;
    }
    else
    {
        /* Above 50 morale, we apply strong diminishing returns.
         * Each block of 50% takes twice as many morale points as the previous one:
         * 150% focus gain at 50 morale (as before)
         * 200% focus gain at 150 morale (100 more morale)
         * 250% focus gain at 350 morale (200 more morale)
         * ...
         * Cap out at 400% focus gain with 3,150+ morale, mostly as a sanity check.
         */

        int block_multiplier = 1;
        int morale_left = eff_morale;
        while (focus_gain_rate < 400)
        {
            if (morale_left > 50 * block_multiplier)
            {
                // We can afford the entire block.  Get it and continue.
                morale_left -= 50 * block_multiplier;
                focus_gain_rate += 50;
                block_multiplier *= 2;
            }
            else
            {
                // We can't afford the entire block.  Each block_multiplier morale
                // points give 1% focus gain, and then we're done.
                focus_gain_rate += morale_left / block_multiplier;
                break;
            }
        }
    }

    // This should be redundant, but just in case...
    if (focus_gain_rate < 1)
    {
        focus_gain_rate = 1;
    }
    else if (focus_gain_rate > 400)
    {
        focus_gain_rate = 400;
    }

    return focus_gain_rate;
}

/* Here lies the intended effects of body temperature

Assumption 1 : a naked person is comfortable at 19C/66.2F (31C/87.8F at rest).
Assumption 2 : a "lightly clothed" person is comfortable at 13C/55.4F (25C/77F at rest).
Assumption 3 : the player is always running, thus generating more heat.
Assumption 4 : frostbite cannot happen above 0C temperature.*
* In the current model, a naked person can get frostbite at 1C. This isn't true, but it's a compromise with using nice whole numbers.

Here is a list of warmth values and the corresponding temperatures in which the player is comfortable, and in which the player is very cold.

Warmth  Temperature (Comfortable)    Temperature (Very cold)    Notes
  0       19C /  66.2F               -11C /  12.2F               * Naked
 10       13C /  55.4F               -17C /   1.4F               * Lightly clothed
 20        7C /  44.6F               -23C /  -9.4F
 30        1C /  33.8F               -29C / -20.2F
 40       -5C /  23.0F               -35C / -31.0F
 50      -11C /  12.2F               -41C / -41.8F
 60      -17C /   1.4F               -47C / -52.6F
 70      -23C /  -9.4F               -53C / -63.4F
 80      -29C / -20.2F               -59C / -74.2F
 90      -35C / -31.0F               -65C / -85.0F
100      -41C / -41.8F               -71C / -95.8F
*/

void player::update_bodytemp()
{
    // NOTE : visit weather.h for some details on the numbers used
    // Converts temperature to Celsius/10(Wito plans on using degrees Kelvin later)
    int Ctemperature = 100*(g->get_temperature() - 32) * 5/9;
    // Temperature norms
    // Ambient normal temperature is lower while asleep
    int ambient_norm = (has_disease("sleep") ? 3100 : 1900);
    // This adjusts the temperature scale to match the bodytemp scale
    int adjusted_temp = (Ctemperature - ambient_norm);
    // This gets incremented in the for loop and used in the morale calculation
    int morale_pen = 0;
    const trap_id trap_at_pos = g->m.tr_at(posx, posy);
    const ter_id ter_at_pos = g->m.ter(posx, posy);
    const furn_id furn_at_pos = g->m.furn(posx, posy);
    // When the player is sleeping, he will use floor items for warmth
    int floor_item_warmth = 0;
    // When the player is sleeping, he will use floor bedding for warmth
    int floor_bedding_warmth = 0;
    if ( has_disease("sleep") ) {
        // Search the floor for items
        std::vector<item>& floor_item = g->m.i_at(posx, posy);
        it_armor* floor_armor = NULL;

        for ( std::vector<item>::iterator afloor_item = floor_item.begin() ;
        afloor_item != floor_item.end() ;
        ++afloor_item) {
            if ( !afloor_item->is_armor() ) {
                continue;
            }
            floor_armor = dynamic_cast<it_armor*>(afloor_item->type);
            // Items that are big enough and covers the torso are used to keep warm.
            // Smaller items don't do as good a job
            if ( floor_armor->volume > 1 &&
            ((floor_armor->covers & mfb(bp_torso)) ||
             (floor_armor->covers & mfb(bp_legs))) ) {
                floor_item_warmth += 60 * floor_armor->warmth * floor_armor->volume / 10;
            }
        }

        // Search the floor for bedding
        int vpart = -1;
        vehicle *veh = g->m.veh_at (posx, posy, vpart);
        if      (furn_at_pos == f_bed)
        {
            floor_bedding_warmth += 1000;
        }
        else if (furn_at_pos == f_makeshift_bed ||
                 furn_at_pos == f_armchair ||
                 furn_at_pos == f_sofa||
                 furn_at_pos == f_hay)
        {
            floor_bedding_warmth += 500;
        }
        else if (trap_at_pos == tr_cot)
        {
            floor_bedding_warmth -= 500;
        }
        else if (trap_at_pos == tr_rollmat)
        {
            floor_bedding_warmth -= 1000;
        }
        else if (trap_at_pos == tr_fur_rollmat)
        {
            floor_bedding_warmth += 0;
        }
        else if (veh && veh->part_with_feature (vpart, "SEAT") >= 0)
        {
            floor_bedding_warmth += 200;
        }
        else if (veh && veh->part_with_feature (vpart, "BED") >= 0)
        {
            floor_bedding_warmth += 300;
        }
        else
        {
            floor_bedding_warmth -= 2000;
        }
    }
    // Current temperature and converging temperature calculations
    for (int i = 0 ; i < num_bp ; i++)
    {
        // Skip eyes
        if (i == bp_eyes) { continue; }
        // Represents the fact that the body generates heat when it is cold. TODO : should this increase hunger?
        float homeostasis_adjustement = (temp_cur[i] > BODYTEMP_NORM ? 30.0 : 60.0);
        int clothing_warmth_adjustement = homeostasis_adjustement * warmth(body_part(i));
        // Convergeant temperature is affected by ambient temperature, clothing warmth, and body wetness.
        temp_conv[i] = BODYTEMP_NORM + adjusted_temp + clothing_warmth_adjustement;
        // HUNGER
        temp_conv[i] -= hunger/6 + 100;
        // FATIGUE
        if (!has_disease("sleep")) { temp_conv[i] -= 1.5*fatigue; }
        // CONVECTION HEAT SOURCES (generates body heat, helps fight frostbite)
        int blister_count = 0; // If the counter is high, your skin starts to burn
        for (int j = -6 ; j <= 6 ; j++) {
            for (int k = -6 ; k <= 6 ; k++) {
                int heat_intensity = 0;

                int ffire = g->m.get_field_strength( point(posx + j, posy + k), fd_fire );
                if(ffire > 0) {
                    heat_intensity = ffire;
                } else if (g->m.tr_at(posx + j, posy + k) == tr_lava ) {
                    heat_intensity = 3;
                }
                if (heat_intensity > 0 && g->u_see(posx + j, posy + k)) {
                    // Ensure fire_dist >= 1 to avoid divide-by-zero errors.
                    int fire_dist = std::max(1, std::max(j, k));
                    if (frostbite_timer[i] > 0) {
                        frostbite_timer[i] -= heat_intensity - fire_dist / 2;
                    }
                    temp_conv[i] +=  300 * heat_intensity * heat_intensity / (fire_dist * fire_dist);
                    blister_count += heat_intensity / (fire_dist * fire_dist);
                }
            }
        }
        // TILES
        // Being on fire affects temp_cur (not temp_conv): this is super dangerous for the player
        if (has_effect("onfire")) {
            temp_cur[i] += 250;
        }
        if ( g->m.get_field_strength( point(posx, posy), fd_fire ) > 2 || trap_at_pos == tr_lava) {
            temp_cur[i] += 250;
        }
        // WEATHER
        if (g->weather == WEATHER_SUNNY && g->is_in_sunlight(posx, posy))
        {
            temp_conv[i] += 1000;
        }
        if (g->weather == WEATHER_CLEAR && g->is_in_sunlight(posx, posy))
        {
            temp_conv[i] += 500;
        }
        // DISEASES
        if (has_disease("flu") && i == bp_head) { temp_conv[i] += 1500; }
        if (has_disease("common_cold")) { temp_conv[i] -= 750; }
        // BIONICS
        // Bionic "Internal Climate Control" says it eases the effects of high and low ambient temps
        const int variation = BODYTEMP_NORM*0.5;
        if (in_climate_control()
            && temp_conv[i] < BODYTEMP_SCORCHING + variation
            && temp_conv[i] > BODYTEMP_FREEZING - variation)
        {
            if      (temp_conv[i] > BODYTEMP_SCORCHING)
            {
                temp_conv[i] = BODYTEMP_VERY_HOT;
            }
            else if (temp_conv[i] > BODYTEMP_VERY_HOT)
            {
                temp_conv[i] = BODYTEMP_HOT;
            }
            else if (temp_conv[i] > BODYTEMP_HOT)
            {
                temp_conv[i] = BODYTEMP_NORM;
            }
            else if (temp_conv[i] < BODYTEMP_FREEZING)
            {
                temp_conv[i] = BODYTEMP_VERY_COLD;
            }
            else if (temp_conv[i] < BODYTEMP_VERY_COLD)
            {
                temp_conv[i] = BODYTEMP_COLD;
            }
            else if (temp_conv[i] < BODYTEMP_COLD)
            {
                temp_conv[i] = BODYTEMP_NORM;
            }
        }
        // Bionic "Thermal Dissipation" says it prevents fire damage up to 2000F. 500 is picked at random...
        if (has_bionic("bio_heatsink") && blister_count < 500)
        {
            blister_count = (has_trait("BARK") ? -100 : 0);
        }
        // BLISTERS : Skin gets blisters from intense heat exposure.
        if (blister_count - 10*get_env_resist(body_part(i)) > 20)
        {
            add_disease("blisters", 1, false, 1, 1, 0, 1, (body_part)i, -1);
        }
        // BLOOD LOSS : Loss of blood results in loss of body heat
        int blood_loss = 0;
        if      (i == bp_legs)
        {
            blood_loss = (100 - 100*(hp_cur[hp_leg_l] + hp_cur[hp_leg_r]) / (hp_max[hp_leg_l] + hp_max[hp_leg_r]));
        }
        else if (i == bp_arms)
        {
            blood_loss = (100 - 100*(hp_cur[hp_arm_l] + hp_cur[hp_arm_r]) / (hp_max[hp_arm_l] + hp_max[hp_arm_r]));
        }
        else if (i == bp_torso)
        {
            blood_loss = (100 - 100* hp_cur[hp_torso] / hp_max[hp_torso]);
        }
        else if (i == bp_head)
        {
            blood_loss = (100 - 100* hp_cur[hp_head] / hp_max[hp_head]);
        }
        temp_conv[i] -= blood_loss*temp_conv[i]/200; // 1% bodyheat lost per 2% hp lost
        // EQUALIZATION
        switch (i)
        {
            case bp_torso :
                temp_equalizer(bp_torso, bp_arms);
                temp_equalizer(bp_torso, bp_legs);
                temp_equalizer(bp_torso, bp_head);
                break;
            case bp_head :
                temp_equalizer(bp_head, bp_torso);
                temp_equalizer(bp_head, bp_mouth);
                break;
            case bp_arms :
                temp_equalizer(bp_arms, bp_torso);
                temp_equalizer(bp_arms, bp_hands);
                break;
            case bp_legs :
                temp_equalizer(bp_legs, bp_torso);
                temp_equalizer(bp_legs, bp_feet);
                break;
            case bp_mouth : temp_equalizer(bp_mouth, bp_head); break;
            case bp_hands : temp_equalizer(bp_hands, bp_arms); break;
            case bp_feet  : temp_equalizer(bp_feet, bp_legs); break;
        }
        // MUTATIONS and TRAITS
        // Bark : lowers blister count to -100; harder to get blisters
        // Lightly furred
        if (has_trait("LIGHTFUR"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 250 : 500);
        }
        // Furry or Lupine Fur
        if (has_trait("FUR") || has_trait("LUPINE_FUR"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 750 : 1500);
        }
        // Feline fur
        if (has_trait("FELINE_FUR"))
        {
            temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 500 : 1000);
        }
        // Disintergration
        if (has_trait("ROT1")) { temp_conv[i] -= 250;}
        else if (has_trait("ROT2")) { temp_conv[i] -= 750;}
        else if (has_trait("ROT3")) { temp_conv[i] -= 1500;}
        // Radioactive
        if (has_trait("RADIOACTIVE1")) { temp_conv[i] += 250; }
        else if (has_trait("RADIOACTIVE2")) { temp_conv[i] += 750; }
        else if (has_trait("RADIOACTIVE3")) { temp_conv[i] += 1500; }
        // Chemical Imbalance
        // Added linse in player::suffer()
        // FINAL CALCULATION : Increments current body temperature towards convergant.
        if ( has_disease("sleep") ) {
            int sleep_bonus = floor_bedding_warmth + floor_item_warmth;
            // Too warm, don't need items on the floor
            if ( temp_conv[i] > BODYTEMP_NORM ) {
                // Do nothing
            }
            // Intelligently use items on the floor; just enough to be comfortable
            else if ( (temp_conv[i] + sleep_bonus) > BODYTEMP_NORM ) {
                temp_conv[i] = BODYTEMP_NORM;
            }
            // Use all items on the floor -- there are not enough to keep comfortable
            else {
                temp_conv[i] += sleep_bonus;
            }
        }
        int temp_before = temp_cur[i];
        int temp_difference = temp_cur[i] - temp_conv[i]; // Negative if the player is warming up.
        // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes,
        // exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
        int rounding_error = 0;
        // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
        if (temp_difference < 0 && temp_difference > -600 )
        {
            rounding_error = 1;
        }
        if (temp_cur[i] != temp_conv[i])
        {
            if      ((ter_at_pos == t_water_sh || ter_at_pos == t_sewage)
                    && (i == bp_feet || i == bp_legs))
            {
                temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
            }
            else if (ter_at_pos == t_water_dp)
            {
                temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
            }
            else if (i == bp_torso || i == bp_head)
            {
                temp_cur[i] = temp_difference*exp(-0.003) + temp_conv[i] + rounding_error;
            }
            else
            {
                temp_cur[i] = temp_difference*exp(-0.002) + temp_conv[i] + rounding_error;
            }
        }
        int temp_after = temp_cur[i];
        // PENALTIES
        if      (temp_cur[i] < BODYTEMP_FREEZING)
        {
            add_disease("cold", 1, false, 3, 3, 0, 1, (body_part)i, -1);
            frostbite_timer[i] += 3;
        }
        else if (temp_cur[i] < BODYTEMP_VERY_COLD)
        {
            add_disease("cold", 1, false, 2, 3, 0, 1, (body_part)i, -1);
            frostbite_timer[i] += 2;
        }
        else if (temp_cur[i] < BODYTEMP_COLD)
        {
            // Frostbite timer does not go down if you are still cold.
            add_disease("cold", 1, false, 1, 3, 0, 1, (body_part)i, -1);
            frostbite_timer[i] += 1;
        }
        else if (temp_cur[i] > BODYTEMP_SCORCHING)
        {
            // If body temp rises over 15000, disease.cpp ("hot_head") acts weird and the player will die
            add_disease("hot",  1, false, 3, 3, 0, 1, (body_part)i, -1);
        }
        else if (temp_cur[i] > BODYTEMP_VERY_HOT)
        {
            add_disease("hot",  1, false, 2, 3, 0, 1, (body_part)i, -1);
        }
        else if (temp_cur[i] > BODYTEMP_HOT)
        {
            add_disease("hot",  1, false, 1, 3, 0, 1, (body_part)i, -1);
        }
        // MORALE : a negative morale_pen means the player is cold
        // Intensity multiplier is negative for cold, positive for hot
        int intensity_mult =
            - disease_intensity("cold", false, (body_part)i) +
            disease_intensity("hot", false, (body_part)i);
        if (has_disease("cold", (body_part)i) ||
            has_disease("hot", (body_part)i))
        {
            switch (i)
            {
                case bp_head :
                case bp_torso :
                case bp_mouth : morale_pen += 2*intensity_mult;
                case bp_arms :
                case bp_legs : morale_pen += 1*intensity_mult;
                case bp_hands:
                case bp_feet : morale_pen += 1*intensity_mult;
            }
        }
        // FROSTBITE (level 1 after 2 hours, level 2 after 4 hours)
        if      (frostbite_timer[i] >   0)
        {
            frostbite_timer[i]--;
        }
        if      (frostbite_timer[i] >= 240 && g->get_temperature() < 32)
        {
            add_disease("frostbite", 1, false, 2, 2, 0, 1, (body_part)i, -1);
            // Warning message for the player
            if (disease_intensity("frostbite", false, (body_part)i) < 2
                &&  (i == bp_mouth || i == bp_hands || i == bp_feet))
            {
                g->add_msg((i == bp_mouth ? _("Your %s hardens from the frostbite!") : _("Your %s harden from the frostbite!")), body_part_name(body_part(i), -1).c_str());
            }
            else if (frostbite_timer[i] >= 120 && g->get_temperature() < 32)
            {
                add_disease("frostbite", 1, false, 1, 2, 0, 1, (body_part)i, -1);
                // Warning message for the player
                if (!has_disease("frostbite", (body_part)i))
                {
                    g->add_msg(_("You lose sensation in your %s."),
                        body_part_name(body_part(i), -1).c_str());
                }
            }
        }
        // Warn the player if condition worsens
        if  (temp_before > BODYTEMP_FREEZING && temp_after < BODYTEMP_FREEZING)
        {
            g->add_msg(_("You feel your %s beginning to go numb from the cold!"),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD)
        {
            g->add_msg(_("You feel your %s getting very cold."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before > BODYTEMP_COLD && temp_after < BODYTEMP_COLD)
        {
            g->add_msg(_("You feel your %s getting chilly."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING)
        {
            g->add_msg(_("You feel your %s getting red hot from the heat!"),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_VERY_HOT && temp_after > BODYTEMP_VERY_HOT)
        {
            g->add_msg(_("You feel your %s getting very hot."),
                body_part_name(body_part(i), -1).c_str());
        }
        else if (temp_before < BODYTEMP_HOT && temp_after > BODYTEMP_HOT)
        {
            g->add_msg(_("You feel your %s getting warm."),
                body_part_name(body_part(i), -1).c_str());
        }
    }
    // Morale penalties, updated at the same rate morale is
    if (morale_pen < 0 && int(g->turn) % 10 == 0)
    {
        add_morale(MORALE_COLD, -2, -abs(morale_pen), 10, 5, true);
    }
    if (morale_pen > 0 && int(g->turn) % 10 == 0)
    {
        add_morale(MORALE_HOT,  -2, -abs(morale_pen), 10, 5, true);
    }
}

void player::temp_equalizer(body_part bp1, body_part bp2)
{
 // Body heat is moved around.
 // Shift in one direction only, will be shifted in the other direction seperately.
 int diff = (temp_cur[bp2] - temp_cur[bp1])*0.0001; // If bp1 is warmer, it will lose heat
 temp_cur[bp1] += diff;
}

void player::recalc_speed_bonus()
{
// Minus some for weight...
 int carry_penalty = 0;
 if (weight_carried() > weight_capacity())
  carry_penalty = 25 * (weight_carried() - weight_capacity()) / (weight_capacity());
 mod_speed_bonus(-carry_penalty);

 if (pain > pkill) {
  int pain_penalty = int((pain - pkill) * .7);
  if (pain_penalty > 60)
   pain_penalty = 60;
  mod_speed_bonus(-pain_penalty);
 }
 if (pkill >= 10) {
  int pkill_penalty = int(pkill * .1);
  if (pkill_penalty > 30)
   pkill_penalty = 30;
  mod_speed_bonus(-pkill_penalty);
 }

 if (abs(morale_level()) >= 100) {
  int morale_bonus = int(morale_level() / 25);
  if (morale_bonus < -10)
   morale_bonus = -10;
  else if (morale_bonus > 10)
   morale_bonus = 10;
  mod_speed_bonus(morale_bonus);
 }

 if (radiation >= 40) {
  int rad_penalty = radiation / 40;
  if (rad_penalty > 20)
   rad_penalty = 20;
  mod_speed_bonus(-rad_penalty);
 }

 if (thirst > 40)
  mod_speed_bonus(-int((thirst - 40) / 10));
 if (hunger > 100)
  mod_speed_bonus(-int((hunger - 100) / 10));

 mod_speed_bonus(stim > 40 ? 40 : stim);

 for (int i = 0; i < illness.size(); i++)
  mod_speed_bonus(disease_speed_boost(illness[i]));

 // add martial arts speed bonus
 mod_speed_bonus(mabuff_speed_bonus());

 if (g != NULL) {
  if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(posx, posy))
   mod_speed_bonus(-(g->light_level() >= 12 ? 5 : 10));
  if (has_trait("COLDBLOOD3") && g->get_temperature() < 60)
   mod_speed_bonus(-int( (65 - g->get_temperature()) / 2));
  else if (has_trait("COLDBLOOD2") && g->get_temperature() < 60)
   mod_speed_bonus(-int( (65 - g->get_temperature()) / 3));
  else if (has_trait("COLDBLOOD") && g->get_temperature() < 60)
   mod_speed_bonus(-int( (65 - g->get_temperature()) / 5));
 }

 if (has_artifact_with(AEP_SPEED_UP))
  mod_speed_bonus(20);
 if (has_artifact_with(AEP_SPEED_DOWN))
  mod_speed_bonus(-20);

 if (has_trait("QUICK")) // multiply by 1.1
  set_speed_bonus(get_speed() * 1.10 - get_speed_base());

 if (get_speed_bonus() < -0.75 * get_speed_base())
  set_speed_bonus(0.75 * get_speed_base());
}

int player::run_cost(int base_cost, bool diag)
{
    float movecost = float(base_cost);
    if (diag)
        movecost *= 0.7071f; // because everything here assumes 100 is base
    bool flatground = movecost < 105;

    if (has_trait("PARKOUR") && movecost > 100 ) {
        movecost *= .5f;
        if (movecost < 100)
            movecost = 100;
    }
    if (has_trait("BADKNEES") && movecost > 100 ) {
        movecost *= 1.25f;
        if (movecost < 100)
            movecost = 100;
    }

    if (hp_cur[hp_leg_l] == 0)
        movecost += 50;
    else if (hp_cur[hp_leg_l] < hp_max[hp_leg_l] * .40)
        movecost += 25;
    if (hp_cur[hp_leg_r] == 0)
        movecost += 50;
    else if (hp_cur[hp_leg_r] < hp_max[hp_leg_r] * .40)
        movecost += 25;

    if (has_trait("FLEET") && flatground)
        movecost *= .85f;
    if (has_trait("FLEET2") && flatground)
        movecost *= .7f;
    if (has_trait("SLOWRUNNER") && flatground)
        movecost *= 1.15f;
    if (has_trait("PADDED_FEET") && !wearing_something_on(bp_feet))
        movecost *= .9f;
    if (has_trait("LIGHT_BONES"))
        movecost *= .9f;
    if (has_trait("HOLLOW_BONES"))
        movecost *= .8f;
    if (has_trait("WINGS_INSECT"))
        movecost -= 15;
    if (has_trait("LEG_TENTACLES"))
        movecost += 20;
    if (has_trait("PONDEROUS1"))
        movecost *= 1.1f;
    if (has_trait("PONDEROUS2"))
        movecost *= 1.2f;
    if (has_trait("PONDEROUS3"))
        movecost *= 1.3f;
    if (is_wearing("swim_fins"))
        movecost *= 1.5f;

    movecost += encumb(bp_mouth) * 5 + encumb(bp_feet) * 5 + encumb(bp_legs) * 3;

    if (!is_wearing_shoes() && !has_trait("PADDED_FEET") && !has_trait("HOOVES")&& !has_trait("TOUGH_FEET")){
        movecost += 15;
    }

    if (diag)
        movecost *= 1.4142;

    return int(movecost);
}

int player::swim_speed()
{
  int ret = 440 + weight_carried() / 60 - 50 * skillLevel("swimming");
 if (has_trait("PAWS"))
  ret -= 15 + str_cur * 4;
 if (is_wearing("swim_fins"))
  ret -= (10 * str_cur) * 1.5;
 if (has_trait("WEBBED"))
  ret -= 60 + str_cur * 5;
 if (has_trait("TAIL_FIN"))
  ret -= 100 + str_cur * 10;
 if (has_trait("SLEEK_SCALES"))
  ret -= 100;
 if (has_trait("LEG_TENTACLES"))
  ret -= 60;
 ret += (50 - skillLevel("swimming") * 2) * encumb(bp_legs);
 ret += (80 - skillLevel("swimming") * 3) * encumb(bp_torso);
 if (skillLevel("swimming") < 10) {
  for (int i = 0; i < worn.size(); i++)
    ret += (worn[i].volume() * (10 - skillLevel("swimming"))) / 2;
 }
 ret -= str_cur * 6 + dex_cur * 4;
 if( worn_with_flag("FLOATATION") ) {
     ret = std::max(ret, 400);
     ret = std::min(ret, 200);
 }
// If (ret > 500), we can not swim; so do not apply the underwater bonus.
 if (underwater && ret < 500)
  ret -= 50;
 if (ret < 30)
  ret = 30;
 return ret;
}

bool player::digging() {
    return false;
}

bool player::is_on_ground()
{
    bool on_ground = false;
    if(has_effect("downed") || hp_cur[hp_leg_l] == 0 || hp_cur[hp_leg_r] == 0 ){
        on_ground = true;
    }
    return  on_ground;
}

bool player::is_underwater() const
{
    return underwater;
}

void player::set_underwater(bool u)
{
    if (underwater != u) {
        underwater = u;
        recalc_sight_limits();
    }
}


nc_color player::color()
{
 if (has_effect("onfire"))
  return c_red;
 if (has_effect("stunned"))
  return c_ltblue;
 if (has_disease("boomered"))
  return c_pink;
 if (underwater)
  return c_blue;
 if (has_active_bionic("bio_cloak") || has_artifact_with(AEP_INVISIBLE) ||
    (is_wearing("optical_cloak") && (has_active_item("UPS_on") || has_active_item("adv_UPS_on"))))
  return c_dkgray;
 return c_white;
}

void player::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;

    char check = dump.peek();
    if ( check == ' ' ) {
        // sigh..
        check = data[1];
    }
    if ( check == '{' ) {
        JsonIn jsin(dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad player json\n%s", jsonerr.c_str() );
        }
        return;
    } else { // old save
        load_legacy(dump);
    }
}

std::string player::save_info()
{
    std::stringstream dump;
    dump << serialize(); // saves contents
    dump << std::endl;
    dump << dump_memorial();
    return dump.str();
}

void player::memorial( std::ofstream &memorial_file )
{
    //Ask the player for their final words
    std::string epitaph = string_input_popup(_("Do you have any last words?"), 256);

    //Size of indents in the memorial file
    const std::string indent = "  ";

    const std::string gender_str = male ? _("male") : _("female");
    const std::string pronoun = male ? _("He") : _("She");

    //Avoid saying "a male unemployed" or similar
    std::stringstream profession_name;
    if(prof == prof->generic()) {
      profession_name << _("an unemployed ") << gender_str;
    } else {
      profession_name << _("a ") << gender_str << " " << prof->name();
    }

    //Figure out the location
    point cur_loc = g->om_location();
    oter_id cur_ter = g->cur_om->ter(cur_loc.x, cur_loc.y, g->levz);
    if (cur_ter == "") {
        if (cur_loc.x >= OMAPX && cur_loc.y >= OMAPY) {
            cur_ter = g->om_diag->ter(cur_loc.x - OMAPX, cur_loc.y - OMAPY, g->levz);
        } else if (cur_loc.x >= OMAPX) {
            cur_ter = g->om_hori->ter(cur_loc.x - OMAPX, cur_loc.y, g->levz);
        } else if (cur_loc.y >= OMAPY) {
            cur_ter = g->om_vert->ter(cur_loc.x, cur_loc.y - OMAPY, g->levz);
        }
    }
    std::string tername = otermap[cur_ter].name;

    //Were they in a town, or out in the wilderness?
    int city_index = g->cur_om->closest_city(cur_loc);
    std::stringstream city_name;
    if(city_index < 0) {
        city_name << _("in the middle of nowhere");
    } else {
        city nearest_city = g->cur_om->cities[city_index];
        //Give slightly different messages based on how far we are from the middle
        int distance_from_city = abs(g->cur_om->dist_from_city(cur_loc));
        if(distance_from_city > nearest_city.s + 4) {
            city_name << _("in the wilderness");
        } else if(distance_from_city >= nearest_city.s) {
            city_name << _("on the outskirts of ") << nearest_city.name;
        } else {
            city_name << _("in ") << nearest_city.name;
        }
    }

    //Header
    std::string version = string_format("%s", getVersionString());
    memorial_file << _("Cataclysm - Dark Days Ahead version ") << version << _(" memorial file") << "\n";
    memorial_file << "\n";
    memorial_file << _("In memory of: ") << name << "\n";
    if(epitaph.length() > 0) { //Don't record empty epitaphs
        memorial_file << "\"" << epitaph << "\"" << "\n\n";
    }
    memorial_file << pronoun << _(" was ") << profession_name.str()
                  << _(" when the apocalypse began.") << "\n";
    memorial_file << pronoun << _(" died on ") << _(season_name[g->turn.get_season()].c_str())
                  << _(" of year ") << (g->turn.years() + 1)
                  << _(", day ") << (g->turn.days() + 1)
                  << _(", at ") << g->turn.print_time() << ".\n";
    memorial_file << pronoun << _(" was killed in a ") << tername << " " << city_name.str() << ".\n";
    memorial_file << "\n";

    //Misc
    memorial_file << _("Cash on hand: ") << "$" << cash << "\n";
    memorial_file << "\n";

    //HP
    memorial_file << _("Final HP:") << "\n";
    memorial_file << indent << _(" Head: ") << hp_cur[hp_head] << "/" << hp_max[hp_head] << "\n";
    memorial_file << indent << _("Torso: ") << hp_cur[hp_torso] << "/" << hp_max[hp_torso] << "\n";
    memorial_file << indent << _("L Arm: ") << hp_cur[hp_arm_l] << "/" << hp_max[hp_arm_l] << "\n";
    memorial_file << indent << _("R Arm: ") << hp_cur[hp_arm_r] << "/" << hp_max[hp_arm_r] << "\n";
    memorial_file << indent << _("L Leg: ") << hp_cur[hp_leg_l] << "/" << hp_max[hp_leg_l] << "\n";
    memorial_file << indent << _("R Leg: ") << hp_cur[hp_leg_r] << "/" << hp_max[hp_leg_r] << "\n";
    memorial_file << "\n";

    //Stats
    memorial_file << _("Final Stats:") << "\n";
    memorial_file << indent << _("Str ") << str_cur << indent << _("Dex ") << dex_cur << indent
                  << _("Int ") << int_cur << indent << _("Per ") << per_cur << "\n";
    memorial_file << _("Base Stats:") << "\n";
    memorial_file << indent << _("Str ") << str_max << indent << _("Dex ") << dex_max << indent
                  << _("Int ") << int_max << indent << _("Per ") << per_max << "\n";
    memorial_file << "\n";

    //Last 20 messages
    memorial_file << _("Final Messages:") << "\n";
    std::vector<game_message> recent_messages = g->recent_messages(20);
    for(int i = 0; i < recent_messages.size(); i++) {
      memorial_file << indent << recent_messages[i].turn.print_time() << " " <<
              recent_messages[i].message;
      if(recent_messages[i].count > 1) {
        memorial_file << " x" << recent_messages[i].count;
      }
      memorial_file << "\n";
    }
    memorial_file << "\n";

    //Kill list
    memorial_file << _("Kills:") << "\n";

    int total_kills = 0;

    const std::map<std::string, mtype*> monids = MonsterGenerator::generator().get_all_mtypes();
    for (std::map<std::string, mtype*>::const_iterator mon = monids.begin(); mon != monids.end(); ++mon){
        if (g->kill_count(mon->first) > 0){
            memorial_file << "  " << (char)mon->second->sym << " - " << mon->second->name << " x" << g->kill_count(mon->first) << "\n";
            total_kills += g->kill_count(mon->first);
        }
    }
    if(total_kills == 0) {
      memorial_file << indent << _("No monsters were killed.") << "\n";
    } else {
      memorial_file << _("Total kills: ") << total_kills << "\n";
    }
    memorial_file << "\n";

    //Skills
    memorial_file << _("Skills:") << "\n";
    for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
      aSkill != Skill::skills.end(); ++aSkill) {
      SkillLevel next_skill_level = skillLevel(*aSkill);
      memorial_file << indent << (*aSkill)->name() << ": "
              << next_skill_level.level() << " (" << next_skill_level.exercise() << "%)\n";
    }
    memorial_file << "\n";

    //Traits
    memorial_file << _("Traits:") << "\n";
    bool had_trait = false;
    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
      if(has_trait(iter->first)) {
        had_trait = true;
        memorial_file << indent << traits[iter->first].name << "\n";
      }
    }
    if(!had_trait) {
      memorial_file << indent << _("(None)") << "\n";
    }
    memorial_file << "\n";

    //Effects (illnesses)
    memorial_file << _("Ongoing Effects:") << "\n";
    bool had_effect = false;
    for(int i = 0; i < illness.size(); i++) {
      disease next_illness = illness[i];
      if(dis_name(next_illness).size() > 0) {
        had_effect = true;
        memorial_file << indent << dis_name(next_illness) << "\n";
      }
    }
    //Various effects not covered by the illness list - from player.cpp
    if(morale_level() >= 100) {
      had_effect = true;
      memorial_file << indent << _("Elated") << "\n";
    }
    if(morale_level() <= -100) {
      had_effect = true;
      memorial_file << indent << _("Depressed") << "\n";
    }
    if(pain - pkill > 0) {
      had_effect = true;
      memorial_file << indent << _("Pain") << " (" << (pain - pkill) << ")";
    }
    if(stim > 0) {
      had_effect = true;
      int dexbonus = int(stim / 10);
      if (abs(stim) >= 30) {
        dexbonus -= int(abs(stim - 15) /  8);
      }
      if(dexbonus < 0) {
        memorial_file << indent << _("Stimulant Overdose") << "\n";
      } else {
        memorial_file << indent << _("Stimulant") << "\n";
      }
    } else if(stim < 0) {
      had_effect = true;
      memorial_file << indent << _("Depressants") << "\n";
    }
    if(!had_effect) {
      memorial_file << indent << _("(None)") << "\n";
    }
    memorial_file << "\n";

    //Bionics
    memorial_file << _("Bionics:") << "\n";
    int total_bionics = 0;
    for(int i = 0; i < my_bionics.size(); i++) {
      bionic_id next_bionic_id = my_bionics[i].id;
      memorial_file << indent << (i+1) << ": " << bionics[next_bionic_id]->name << "\n";
      total_bionics++;
    }
    if(total_bionics == 0) {
      memorial_file << indent << _("No bionics were installed.") << "\n";
    } else {
      memorial_file << _("Total bionics: ") << total_bionics << "\n";
    }
    memorial_file << _("Power: ") << power_level << "/" << max_power_level << "\n";
    memorial_file << "\n";

    //Equipment
    memorial_file << _("Weapon:") << "\n";
    memorial_file << indent << weapon.invlet << " - " << weapon.tname() << "\n";
    memorial_file << "\n";

    memorial_file << _("Equipment:") << "\n";
    for(int i = 0; i < worn.size(); i++) {
      item next_item = worn[i];
      memorial_file << indent << next_item.invlet << " - " << next_item.tname();
      if(next_item.charges > 0) {
        memorial_file << " (" << next_item.charges << ")";
      } else if (next_item.contents.size() == 1
              && next_item.contents[0].charges > 0) {
        memorial_file << " (" << next_item.contents[0].charges << ")";
      }
      memorial_file << "\n";
    }
    memorial_file << "\n";

    //Inventory
    memorial_file << _("Inventory:") << "\n";
    inv.restack(this);
    inv.sort();
    invslice slice = inv.slice();
    for(int i = 0; i < slice.size(); i++) {
      item& next_item = slice[i]->front();
      memorial_file << indent << next_item.invlet << " - " << next_item.tname();
      if(slice[i]->size() > 1) {
        memorial_file << " [" << slice[i]->size() << "]";
      }
      if(next_item.charges > 0) {
        memorial_file << " (" << next_item.charges << ")";
      } else if (next_item.contents.size() == 1
              && next_item.contents[0].charges > 0) {
        memorial_file << " (" << next_item.contents[0].charges << ")";
      }
      memorial_file << "\n";
    }
    memorial_file << "\n";

    //Lifetime stats
    memorial_file << _("Lifetime Stats") << "\n";
    memorial_file << indent << _("Distance Walked: ")
                       << player_stats.squares_walked << _(" Squares") << "\n";
    memorial_file << indent << _("Damage Taken: ")
                       << player_stats.damage_taken << _(" Damage") << "\n";
    memorial_file << indent << _("Damage Healed: ")
                       << player_stats.damage_healed << _(" Damage") << "\n";
    memorial_file << indent << _("Headshots: ")
                       << player_stats.headshots << "\n";
    memorial_file << "\n";

    //History
    memorial_file << _("Game History") << "\n";
    memorial_file << dump_memorial();

}

/**
 * Adds an event to the memorial log, to be written to the memorial file when
 * the character dies. The message should contain only the informational string,
 * as the timestamp and location will be automatically prepended.
 */
void player::add_memorial_log(const char* message, ...)
{

  va_list ap;
  va_start(ap, message);
  char buff[1024];
  vsprintf(buff, message, ap);
  va_end(ap);

  std::stringstream timestamp;
  timestamp << _("Year") << " " << (g->turn.years() + 1) << ", "
            << _(season_name[g->turn.get_season()].c_str()) << " "
            << (g->turn.days() + 1) << ", " << g->turn.print_time();

  oter_id cur_ter = g->cur_om->ter((g->levx + int(MAPSIZE / 2)) / 2, (g->levy + int(MAPSIZE / 2)) / 2, g->levz);
  std::string location = otermap[cur_ter].name;

  std::stringstream log_message;
  log_message << "| " << timestamp.str() << " | " << location.c_str() << " | " << buff;

  memorial_log.push_back(log_message.str());

}

/**
 * Loads the data in a memorial file from the given ifstream. All the memorial
 * entry lines begin with a pipe (|).
 * @param fin The ifstream to read the memorial entries from.
 */
void player::load_memorial_file(std::ifstream &fin)
{
  std::string entry;
  memorial_log.clear();
  while(fin.peek() == '|') {
    getline(fin, entry);
    memorial_log.push_back(entry);
  }
}

/**
 * Concatenates all of the memorial log entries, delimiting them with newlines,
 * and returns the resulting string. Used for saving and for writing out to the
 * memorial file.
 */
std::string player::dump_memorial()
{

  std::stringstream output;

  for(int i = 0; i < memorial_log.size(); i++) {
    output << memorial_log[i] << "\n";
  }

  return output.str();

}

/**
 * Returns a pointer to the stat-tracking struct. Its fields should be edited
 * as necessary to track ongoing counters, which will be added to the memorial
 * file. For single events, rather than cumulative counters, see
 * add_memorial_log.
 * @return A pointer to the stats struct being used to track this player's
 *         lifetime stats.
 */
stats* player::lifetime_stats()
{
    return &player_stats;
}

// copy of stats, for saving
stats player::get_stats() const
{
    return player_stats;
}

inline bool skill_display_sort(const std::pair<Skill *, int> &a, const std::pair<Skill *, int> &b)
{
    int levelA = a.second;
    int levelB = b.second;
    return levelA > levelB || (levelA == levelB && a.first->name() < b.first->name());
}

void player::disp_info()
{
 int line;
 std::vector<std::string> effect_name;
 std::vector<std::string> effect_text;
 for (int i = 0; i < illness.size(); i++) {
  if (dis_name(illness[i]).size() > 0) {
   effect_name.push_back(dis_name(illness[i]));
   effect_text.push_back(dis_description(illness[i]));
  }
 }
    for (std::vector<effect>::iterator it = effects.begin();
        it != effects.end(); ++it) {
        effect_name.push_back(it->disp_name());
        effect_text.push_back(it->get_effect_type()->get_desc());
    }
 if (abs(morale_level()) >= 100) {
  bool pos = (morale_level() > 0);
  effect_name.push_back(pos ? _("Elated") : _("Depressed"));
  std::stringstream morale_text;
  if (abs(morale_level()) >= 200)
   morale_text << _("Dexterity") << (pos ? " +" : " ") <<
                   int(morale_level() / 200) << "   ";
  if (abs(morale_level()) >= 180)
   morale_text << _("Strength") << (pos ? " +" : " ") <<
                  int(morale_level() / 180) << "   ";
  if (abs(morale_level()) >= 125)
   morale_text << _("Perception") << (pos ? " +" : " ") <<
                  int(morale_level() / 125) << "   ";
  morale_text << _("Intelligence") << (pos ? " +" : " ") <<
                 int(morale_level() / 100) << "   ";
  effect_text.push_back(morale_text.str());
 }
 if (pain - pkill > 0) {
  effect_name.push_back(_("Pain"));
  std::stringstream pain_text;
  if (pain - pkill >= 15)
   pain_text << "Strength" << " -" << int((pain - pkill) / 15) << "   " << _("Dexterity") << " -" <<
                int((pain - pkill) / 15) << "   ";
  if (pain - pkill >= 20)
   pain_text << _("Perception") << " -" << int((pain - pkill) / 15) << "   ";
  pain_text << _("Intelligence") << " -" << 1 + int((pain - pkill) / 25);
  effect_text.push_back(pain_text.str());
 }
 if (stim > 0) {
  int dexbonus = int(stim / 10);
  int perbonus = int(stim /  7);
  int intbonus = int(stim /  6);
  if (abs(stim) >= 30) {
   dexbonus -= int(abs(stim - 15) /  8);
   perbonus -= int(abs(stim - 15) / 12);
   intbonus -= int(abs(stim - 15) / 14);
  }

  if (dexbonus < 0)
   effect_name.push_back(_("Stimulant Overdose"));
  else
   effect_name.push_back(_("Stimulant"));
  std::stringstream stim_text;
  stim_text << _("Speed") << " +" << stim << "   " << _("Intelligence") <<
               (intbonus > 0 ? " + " : " ") << intbonus << "   " << _("Perception") <<
               (perbonus > 0 ? " + " : " ") << perbonus << "   " << _("Dexterity")  <<
               (dexbonus > 0 ? " + " : " ") << dexbonus;
  effect_text.push_back(stim_text.str());
 } else if (stim < 0) {
  effect_name.push_back(_("Depressants"));
  std::stringstream stim_text;
  int dexpen = int(stim / 10);
  int perpen = int(stim /  7);
  int intpen = int(stim /  6);
// Since dexpen etc. are always less than 0, no need for + signs
  stim_text << _("Speed") << " " << stim << "   " << _("Intelligence") << " " << intpen <<
               "   " << _("Perception") << " " << perpen << "   " << "Dexterity" << " " << dexpen;
  effect_text.push_back(stim_text.str());
 }

 if ((has_trait("TROGLO") && g->is_in_sunlight(posx, posy) &&
      g->weather == WEATHER_SUNNY) ||
     (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy) &&
      g->weather != WEATHER_SUNNY)) {
  effect_name.push_back(_("In Sunlight"));
  effect_text.push_back(_("The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Perception - 1"));
 } else if (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy)) {
  effect_name.push_back(_("In Sunlight"));
  effect_text.push_back(_("The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Perception - 2"));
 } else if (has_trait("TROGLO3") && g->is_in_sunlight(posx, posy)) {
  effect_name.push_back(_("In Sunlight"));
  effect_text.push_back(_("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Perception - 4"));
 }

 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].sated < 0 &&
      addictions[i].intensity >= MIN_ADDICTION_LEVEL) {
   effect_name.push_back(addiction_name(addictions[i]));
   effect_text.push_back(addiction_text(addictions[i]));
  }
 }

    int maxy = TERRAIN_WINDOW_HEIGHT;

    int infooffsetytop = 11;
    int infooffsetybottom = 15;
    std::vector<std::string> traitslist;

    for (std::set<std::string>::iterator iter = my_mutations.begin(); iter != my_mutations.end(); ++iter) {
        traitslist.push_back(*iter);
    }

    int effect_win_size_y = 1 + effect_name.size();
    int trait_win_size_y = 1 + traitslist.size();
    int skill_win_size_y = 1 + Skill::skill_count();

    if (trait_win_size_y + infooffsetybottom > maxy) {
        trait_win_size_y = maxy - infooffsetybottom;
    }

    if (skill_win_size_y + infooffsetybottom > maxy) {
        skill_win_size_y = maxy - infooffsetybottom;
    }

    WINDOW* w_grid_top    = newwin(infooffsetybottom, FULL_SCREEN_WIDTH+1, VIEW_OFFSET_Y, VIEW_OFFSET_X);
    WINDOW* w_grid_skill  = newwin(skill_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
    WINDOW* w_grid_trait  = newwin(trait_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
    WINDOW* w_grid_effect = newwin(effect_win_size_y+ 1, 28, infooffsetybottom + VIEW_OFFSET_Y, 53 + VIEW_OFFSET_X);

 WINDOW* w_tip     = newwin(1, FULL_SCREEN_WIDTH,  VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
 WINDOW* w_stats   = newwin(9, 26,  1 + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
 WINDOW* w_traits  = newwin(trait_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,  27 + VIEW_OFFSET_X);
 WINDOW* w_encumb  = newwin(9, 26,  1 + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
 WINDOW* w_effects = newwin(effect_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
 WINDOW* w_speed   = newwin(9, 26,  1 + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
 WINDOW* w_skills  = newwin(skill_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
 WINDOW* w_info    = newwin(3, FULL_SCREEN_WIDTH, infooffsetytop + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);

 for (int i = 0; i < FULL_SCREEN_WIDTH+1; i++) {
  //Horizontal line top grid
  mvwputch(w_grid_top, 10, i, BORDER_COLOR, LINE_OXOX);
  mvwputch(w_grid_top, 14, i, BORDER_COLOR, LINE_OXOX);

  //Vertical line top grid
  if (i <= infooffsetybottom) {
   mvwputch(w_grid_top, i, 26, BORDER_COLOR, LINE_XOXO);
   mvwputch(w_grid_top, i, 53, BORDER_COLOR, LINE_XOXO);
   mvwputch(w_grid_top, i, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXO);
  }

  //Horizontal line skills
  if (i <= 26) {
   mvwputch(w_grid_skill, skill_win_size_y, i, BORDER_COLOR, LINE_OXOX);
  }

  //Vertical line skills
  if (i <= skill_win_size_y) {
   mvwputch(w_grid_skill, i, 26, BORDER_COLOR, LINE_XOXO);
  }

  //Horizontal line traits
  if (i <= 26) {
   mvwputch(w_grid_trait, trait_win_size_y, i, BORDER_COLOR, LINE_OXOX);
  }

  //Vertical line traits
  if (i <= trait_win_size_y) {
   mvwputch(w_grid_trait, i, 26, BORDER_COLOR, LINE_XOXO);
  }

  //Horizontal line effects
  if (i <= 27) {
   mvwputch(w_grid_effect, effect_win_size_y, i, BORDER_COLOR, LINE_OXOX);
  }

  //Vertical line effects
  if (i <= effect_win_size_y) {
   mvwputch(w_grid_effect, i, 0, BORDER_COLOR, LINE_XOXO);
   mvwputch(w_grid_effect, i, 27, BORDER_COLOR, LINE_XOXO);
  }
 }

 //Intersections top grid
 mvwputch(w_grid_top, 14, 26, BORDER_COLOR, LINE_OXXX); // T
 mvwputch(w_grid_top, 14, 53, BORDER_COLOR, LINE_OXXX); // T
 mvwputch(w_grid_top, 10, 26, BORDER_COLOR, LINE_XXOX); // _|_
 mvwputch(w_grid_top, 10, 53, BORDER_COLOR, LINE_XXOX); // _|_
 mvwputch(w_grid_top, 10, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX); // -|
 mvwputch(w_grid_top, 14, FULL_SCREEN_WIDTH, BORDER_COLOR, LINE_XOXX); // -|
 wrefresh(w_grid_top);

 mvwputch(w_grid_skill, skill_win_size_y, 26, BORDER_COLOR, LINE_XOOX); // _|

 if (skill_win_size_y > trait_win_size_y)
  mvwputch(w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXXO); // |-
 else if (skill_win_size_y == trait_win_size_y)
  mvwputch(w_grid_skill, trait_win_size_y, 26, BORDER_COLOR, LINE_XXOX); // _|_

 mvwputch(w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOOX); // _|

 if (trait_win_size_y > effect_win_size_y)
  mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXXO); // |-
 else if (trait_win_size_y == effect_win_size_y)
  mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOX); // _|_
 else if (trait_win_size_y < effect_win_size_y) {
  mvwputch(w_grid_trait, trait_win_size_y, 26, BORDER_COLOR, LINE_XOXX); // -|
  mvwputch(w_grid_trait, effect_win_size_y, 26, BORDER_COLOR, LINE_XXOO); // |_
 }

 mvwputch(w_grid_effect, effect_win_size_y, 0, BORDER_COLOR, LINE_XXOO); // |_
 mvwputch(w_grid_effect, effect_win_size_y, 27, BORDER_COLOR, LINE_XOOX); // _|

 wrefresh(w_grid_skill);
 wrefresh(w_grid_effect);
 wrefresh(w_grid_trait);

 //-1 for header
 trait_win_size_y--;
 skill_win_size_y--;
 effect_win_size_y--;

// Print name and header
 if (male) {
    mvwprintw(w_tip, 0, 0, _("%s - Male"), name.c_str());
 } else {
    mvwprintw(w_tip, 0, 0, _("%s - Female"), name.c_str());
 }
 mvwprintz(w_tip, 0, 39, c_ltred, _("| Press TAB to cycle, ESC or q to return."));
 wrefresh(w_tip);

// First!  Default STATS screen.
 const char* title_STATS = _("STATS");
 mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
 mvwprintz(w_stats, 2, 1, c_ltgray, "                     ");
 mvwprintz(w_stats, 2, 1, c_ltgray, _("Strength:"));
 mvwprintz(w_stats, 2, 20, c_ltgray, str_max>9?"(%d)":" (%d)", str_max);
 mvwprintz(w_stats, 3, 1, c_ltgray, "                     ");
 mvwprintz(w_stats, 3, 1, c_ltgray, _("Dexterity:"));
 mvwprintz(w_stats, 3, 20, c_ltgray, dex_max>9?"(%d)":" (%d)", dex_max);
 mvwprintz(w_stats, 4, 1, c_ltgray, "                     ");
 mvwprintz(w_stats, 4, 1, c_ltgray, _("Intelligence:"));
 mvwprintz(w_stats, 4, 20, c_ltgray, int_max>9?"(%d)":" (%d)", int_max);
 mvwprintz(w_stats, 5, 1, c_ltgray, "                     ");
 mvwprintz(w_stats, 5, 1, c_ltgray, _("Perception:"));
 mvwprintz(w_stats, 5, 20, c_ltgray, per_max>9?"(%d)":" (%d)", per_max);

 nc_color status = c_white;

 if (str_cur <= 0)
  status = c_dkgray;
 else if (str_cur < str_max / 2)
  status = c_red;
 else if (str_cur < str_max)
  status = c_ltred;
 else if (str_cur == str_max)
  status = c_white;
 else if (str_cur < str_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  2, (str_cur < 10 ? 17 : 16), status, "%d", str_cur);

 if (dex_cur <= 0)
  status = c_dkgray;
 else if (dex_cur < dex_max / 2)
  status = c_red;
 else if (dex_cur < dex_max)
  status = c_ltred;
 else if (dex_cur == dex_max)
  status = c_white;
 else if (dex_cur < dex_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  3, (dex_cur < 10 ? 17 : 16), status, "%d", dex_cur);

 if (int_cur <= 0)
  status = c_dkgray;
 else if (int_cur < int_max / 2)
  status = c_red;
 else if (int_cur < int_max)
  status = c_ltred;
 else if (int_cur == int_max)
  status = c_white;
 else if (int_cur < int_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  4, (int_cur < 10 ? 17 : 16), status, "%d", int_cur);

 if (per_cur <= 0)
  status = c_dkgray;
 else if (per_cur < per_max / 2)
  status = c_red;
 else if (per_cur < per_max)
  status = c_ltred;
 else if (per_cur == per_max)
  status = c_white;
 else if (per_cur < per_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  5, (per_cur < 10 ? 17 : 16), status, "%d", per_cur);

 wrefresh(w_stats);

// Next, draw encumberment.
 std::string asText[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("Arms"), _("Hands"), _("Legs"), _("Feet")};
 body_part aBodyPart[] = {bp_torso, bp_head, bp_eyes, bp_mouth, bp_arms, bp_hands, bp_legs, bp_feet};
 int iEnc, iArmorEnc, iWarmth;
 double iLayers;

 const char *title_ENCUMB = _("ENCUMBRANCE AND WARMTH");
 mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, c_ltgray, title_ENCUMB);
 for (int i=0; i < 8; i++) {
  iLayers = iArmorEnc = 0;
  iWarmth = warmth(body_part(i));
  iEnc = encumb(aBodyPart[i], iLayers, iArmorEnc);
  mvwprintz(w_encumb, i+1, 1, c_ltgray, "%s:", asText[i].c_str());
  mvwprintz(w_encumb, i+1, 8, c_ltgray, "(%d)", iLayers);
  mvwprintz(w_encumb, i+1, 11, c_ltgray, "%*s%d%s%d=", (iArmorEnc < 0 || iArmorEnc > 9 ? 1 : 2), " ", iArmorEnc, "+", iEnc-iArmorEnc);
  wprintz(w_encumb, encumb_color(iEnc), "%s%d", (iEnc < 0 || iEnc > 9 ? "" : " ") , iEnc);
  // Color the warmth value to let the player know what is sufficient
  nc_color color = c_ltgray;
  if (i == bp_eyes) continue; // Eyes don't count towards warmth
  else if (temp_conv[i] >  BODYTEMP_SCORCHING) color = c_red;
  else if (temp_conv[i] >  BODYTEMP_VERY_HOT)  color = c_ltred;
  else if (temp_conv[i] >  BODYTEMP_HOT)       color = c_yellow;
  else if (temp_conv[i] >  BODYTEMP_COLD)      color = c_green; // More than cold is comfortable
  else if (temp_conv[i] >  BODYTEMP_VERY_COLD) color = c_ltblue;
  else if (temp_conv[i] >  BODYTEMP_FREEZING)  color = c_cyan;
  else if (temp_conv[i] <= BODYTEMP_FREEZING)  color = c_blue;
  wprintz(w_encumb, color, " (%3d)", iWarmth);
 }
 wrefresh(w_encumb);

// Next, draw traits.
 const char *title_TRAITS = _("TRAITS");
 mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
 std::sort(traitslist.begin(), traitslist.end(), trait_display_sort);
 for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
  if (traits[traitslist[i]].points > 0)
   status = c_ltgreen;
  else if (traits[traitslist[i]].points < 0)
   status = c_ltred;
  else
   status = c_yellow;
  mvwprintz(w_traits, i+1, 1, status, traits[traitslist[i]].name.c_str());
 }

 wrefresh(w_traits);

// Next, draw effects.
 const char *title_EFFECTS = _("EFFECTS");
 mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
 for (int i = 0; i < effect_name.size() && i < effect_win_size_y; i++) {
  mvwprintz(w_effects, i+1, 0, c_ltgray, effect_name[i].c_str());
 }
 wrefresh(w_effects);

// Next, draw skills.
 line = 1;
 std::vector<Skill*> skillslist;
 const char *title_SKILLS = _("SKILLS");
 mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);

 std::vector<std::pair<Skill *, int> > sorted;
 int num_skills = Skill::skills.size();
 for (int i = 0; i < num_skills; i++) {
     Skill *s = Skill::skills[i];
     SkillLevel &sl = skillLevel(s);
     sorted.push_back(std::pair<Skill *, int>(s, sl.level() * 100 + sl.exercise()));
 }
 std::sort(sorted.begin(), sorted.end(), skill_display_sort);
 for (std::vector<std::pair<Skill *, int> >::iterator i = sorted.begin(); i != sorted.end(); ++i) {
     skillslist.push_back((*i).first);
 }

 for (std::vector<Skill*>::iterator aSkill = skillslist.begin();
      aSkill != skillslist.end(); ++aSkill)
 {
   SkillLevel level = skillLevel(*aSkill);

   // Default to not training and not rusting
   nc_color text_color = c_blue;
   bool training = level.isTraining();
   bool rusting = level.isRusting(g->turn);

   if(training && rusting)
   {
       text_color = c_ltred;
   }
   else if (training)
   {
       text_color = c_ltblue;
   }
   else if (rusting)
   {
       text_color = c_red;
   }

   if (line < skill_win_size_y + 1)
   {
     mvwprintz(w_skills, line, 1, text_color, "%s", ((*aSkill)->name() + ":").c_str());
     mvwprintz(w_skills, line, 19, text_color, "%-2d(%2d%%)", (int)level,
               (level.exercise() <  0 ? 0 : level.exercise()));
     line++;
   }
 }

 wrefresh(w_skills);

// Finally, draw speed.
 const char *title_SPEED = _("SPEED");
 mvwprintz(w_speed, 0, 13 - utf8_width(title_SPEED)/2, c_ltgray, title_SPEED);
 mvwprintz(w_speed, 1,  1, c_ltgray, _("Base Move Cost:"));
 mvwprintz(w_speed, 2,  1, c_ltgray, _("Current Speed:"));
 int newmoves = get_speed();
 int pen = 0;
 line = 3;
 if (weight_carried() > weight_capacity()) {
  pen = 25 * (weight_carried() - weight_capacity()) / (weight_capacity());
  mvwprintz(w_speed, line, 1, c_red, _("Overburdened        -%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 pen = int(morale_level() / 25);
 if (abs(pen) >= 4) {
  if (pen > 10)
   pen = 10;
  else if (pen < -10)
   pen = -10;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, _("Good mood           +%s%d%%"),
             (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, _("Depressed           -%s%d%%"),
             (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 pen = int((pain - pkill) * .7);
 if (pen > 60)
  pen = 60;
 if (pen >= 1) {
  mvwprintz(w_speed, line, 1, c_red, _("Pain                -%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (pkill >= 10) {
  pen = int(pkill * .1);
  mvwprintz(w_speed, line, 1, c_red, _("Painkillers         -%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (stim != 0) {
  pen = stim;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, _("Stimulants          +%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, _("Depressants         -%s%d%%"),
            (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 if (thirst > 40) {
  pen = int((thirst - 40) / 10);
  mvwprintz(w_speed, line, 1, c_red, _("Thirst              -%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (hunger > 100) {
  pen = int((hunger - 100) / 10);
  mvwprintz(w_speed, line, 1, c_red, _("Hunger              -%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (has_trait("SUNLIGHT_DEPENDENT") && !g->is_in_sunlight(posx, posy)) {
  pen = (g->light_level() >= 12 ? 5 : 10);
  mvwprintz(w_speed, line, 1, c_red, _("Out of Sunlight     -%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if ((has_trait("COLDBLOOD") || has_trait("COLDBLOOD2") ||
      has_trait("COLDBLOOD3")) && g->get_temperature() < 65) {
  if (has_trait("COLDBLOOD3"))
   pen = int( (65 - g->get_temperature()) / 2);
  else if (has_trait("COLDBLOOD2"))
   pen = int( (65 - g->get_temperature()) / 3);
  else
   pen = int( (65 - g->get_temperature()) / 2);
  mvwprintz(w_speed, line, 1, c_red, _("Cold-Blooded        -%s%d%%"),
            (pen < 10 ? " " : ""), pen);
  line++;
 }

    std::map<std::string, int> speed_effects;
    std::string dis_text = "";
    for (int i = 0; i < illness.size(); i++) {
        int move_adjust = disease_speed_boost(illness[i]);
        if (move_adjust != 0) {
            if (dis_combined_name(illness[i]) == "") {
                dis_text = dis_name(illness[i]);
            } else {
                dis_text = dis_combined_name(illness[i]);
            }
            speed_effects[dis_text] += move_adjust;
        }
    }

    for (std::map<std::string, int>::iterator it = speed_effects.begin();
          it != speed_effects.end(); ++it) {
        nc_color col = (it->second > 0 ? c_green : c_red);
        mvwprintz(w_speed, line,  1, col, it->first.c_str());
        mvwprintz(w_speed, line, 21, col, (it->second > 0 ? "+" : "-"));
        mvwprintz(w_speed, line, (abs(it->second) >= 10 ? 22 : 23), col, "%d%%",
                   abs(it->second));
        line++;
    }

 if (has_trait("QUICK")) {
  pen = int(newmoves * .1);
  mvwprintz(w_speed, line, 1, c_green, _("Quick               +%s%d%%"),
            (pen < 10 ? " " : ""), pen);
 }
 int runcost = run_cost(100);
 nc_color col = (runcost <= 100 ? c_green : c_red);
 mvwprintz(w_speed, 1, (runcost  >= 100 ? 21 : (runcost  < 10 ? 23 : 22)), col,
           "%d", runcost);
 col = (newmoves >= 100 ? c_green : c_red);
 mvwprintz(w_speed, 2, (newmoves >= 100 ? 21 : (newmoves < 10 ? 23 : 22)), col,
           "%d", newmoves);
 wrefresh(w_speed);

 refresh();
 int curtab = 1;
 int min, max;
 line = 0;
 bool done = false;
 int half_y = 0;

// Initial printing is DONE.  Now we give the player a chance to scroll around
// and "hover" over different items for more info.
 do {
  werase(w_info);
  switch (curtab) {
  case 1: // Stats tab
   mvwprintz(w_stats, 0, 0, h_ltgray, _("                          "));
   mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, h_ltgray, title_STATS);
   if (line == 0) {
    mvwprintz(w_stats, 2, 1, h_ltgray, _("Strength:"));

// display player current STR effects
    mvwprintz(w_stats, 6, 1, c_magenta, _("Base HP: %d              "),
             hp_max[1]);
    mvwprintz(w_stats, 7, 1, c_magenta, _("Carry weight: %.1f %s     "), convert_weight(weight_capacity(false)),
                      OPTIONS["USE_METRIC_WEIGHTS"] == "kg"?_("kg"):_("lbs"));
    mvwprintz(w_stats, 8, 1, c_magenta, _("Melee damage: %d         "),
             base_damage(false));

    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Strength affects your melee damage, the amount of weight you can carry, your total HP, \
your resistance to many diseases, and the effectiveness of actions which require brute force."));
   } else if (line == 1) {
    mvwprintz(w_stats, 3, 1, h_ltgray, _("Dexterity:"));
 // display player current DEX effects
    mvwprintz(w_stats, 6, 1, c_magenta, _("Melee to-hit bonus: +%d                      "),
             base_to_hit(false));
    mvwprintz(w_stats, 7, 1, c_magenta, "                                            ");
    mvwprintz(w_stats, 7, 1, c_magenta, _("Ranged penalty: -%d"),
             abs(ranged_dex_mod(false)));
    mvwprintz(w_stats, 8, 1, c_magenta, "                                            ");
    if (throw_dex_mod(false) <= 0) {
        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing bonus: +%d"),
                  abs(throw_dex_mod(false)));
    } else {
        mvwprintz(w_stats, 8, 1, c_magenta, _("Throwing penalty: -%d"),
                  abs(throw_dex_mod(false)));
    }
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Dexterity affects your chance to hit in melee combat, helps you steady your \
gun for ranged combat, and enhances many actions that require finesse."));
   } else if (line == 2) {
    mvwprintz(w_stats, 4, 1, h_ltgray, _("Intelligence:"));
 // display player current INT effects
   mvwprintz(w_stats, 6, 1, c_magenta, _("Read times: %d%%           "),
             read_speed(false));
   mvwprintz(w_stats, 7, 1, c_magenta, _("Skill rust: %d%%           "),
             rust_rate(false));
   mvwprintz(w_stats, 8, 1, c_magenta, _("Crafting Bonus: %d          "),
             int_cur);

    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Intelligence is less important in most situations, but it is vital for more complex tasks like \
electronics crafting. It also affects how much skill you can pick up from reading a book."));
   } else if (line == 3) {
    mvwprintz(w_stats, 5, 1, h_ltgray, _("Perception:"));

       mvwprintz(w_stats, 6, 1,  c_magenta, _("Ranged penalty: -%d"),
             abs(ranged_per_mod(false)),"          ");
    mvwprintz(w_stats, 7, 1, c_magenta, _("Trap dection level: %d       "),
             per_cur);
    mvwprintz(w_stats, 8, 1, c_magenta, "                             ");
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Perception is the most important stat for ranged combat. It's also used for \
detecting traps and other things of interest."));
   }
   wrefresh(w_stats);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 4)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 3;
     break;
    case '\t':
     mvwprintz(w_stats, 0, 0, c_ltgray, _("                          "));
     mvwprintz(w_stats, 0, 13 - utf8_width(title_STATS)/2, c_ltgray, title_STATS);
     wrefresh(w_stats);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_stats, 2, 1, c_ltgray, _("Strength:"));
   mvwprintz(w_stats, 3, 1, c_ltgray, _("Dexterity:"));
   mvwprintz(w_stats, 4, 1, c_ltgray, _("Intelligence:"));
   mvwprintz(w_stats, 5, 1, c_ltgray, _("Perception:"));
   wrefresh(w_stats);
   break;
  case 2: // Encumberment tab
   mvwprintz(w_encumb, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, h_ltgray, title_ENCUMB);
   if (line == 0) {
    mvwprintz(w_encumb, 1, 1, h_ltgray, _("Torso"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Melee skill %+d;      Dodge skill %+d;\n\
Swimming costs %+d movement points;\n\
Melee and thrown attacks cost %+d movement points."), -encumb(bp_torso), -encumb(bp_torso),
              encumb(bp_torso) * (80 - skillLevel("swimming") * 3), encumb(bp_torso) * 20);
   } else if (line == 1) {
    mvwprintz(w_encumb, 2, 1, h_ltgray, _("Head"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Head encumbrance has no effect; it simply limits how much you can put on."));
   } else if (line == 2) {
    mvwprintz(w_encumb, 3, 1, h_ltgray, _("Eyes"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Perception %+d when checking traps or firing ranged weapons;\n\
Perception %+.1f when throwing items."), -encumb(bp_eyes),
double(double(-encumb(bp_eyes)) / 2));
   } else if (line == 3) {
    mvwprintz(w_encumb, 4, 1, h_ltgray, _("Mouth"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Running costs %+d movement points."), encumb(bp_mouth) * 5);
   } else if (line == 4)
  {
    mvwprintz(w_encumb, 5, 1, h_ltgray, _("Arms"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Arm encumbrance affects your accuracy with ranged weapons."));
   } else if (line == 5)
   {
    mvwprintz(w_encumb, 6, 1, h_ltgray, _("Hands"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Reloading costs %+d movement points; \
Dexterity %+d when throwing items."), encumb(bp_hands) * 30, -encumb(bp_hands));
   } else if (line == 6) {
    mvwprintz(w_encumb, 7, 1, h_ltgray, _("Legs"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Running costs %+d movement points;  Swimming costs %+d movement points;\n\
Dodge skill %+.1f."), encumb(bp_legs) * 3,
              encumb(bp_legs) *(50 - skillLevel("swimming") * 2),
                     double(double(-encumb(bp_legs)) / 2));
   } else if (line == 7) {
    mvwprintz(w_encumb, 8, 1, h_ltgray, _("Feet"));
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH - 2, c_magenta, _("\
Running costs %+d movement points."), encumb(bp_feet) * 5);
   }
   wrefresh(w_encumb);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 8)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 7;
     break;
    case '\t':
     mvwprintz(w_encumb, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_encumb, 0, 13 - utf8_width(title_ENCUMB)/2, c_ltgray, title_ENCUMB);
     wrefresh(w_encumb);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_encumb, 1, 1, c_ltgray, _("Torso"));
   mvwprintz(w_encumb, 2, 1, c_ltgray, _("Head"));
   mvwprintz(w_encumb, 3, 1, c_ltgray, _("Eyes"));
   mvwprintz(w_encumb, 4, 1, c_ltgray, _("Mouth"));
   mvwprintz(w_encumb, 5, 1, c_ltgray, _("Arms"));
   mvwprintz(w_encumb, 6, 1, c_ltgray, _("Hands"));
   mvwprintz(w_encumb, 7, 1, c_ltgray, _("Legs"));
   mvwprintz(w_encumb, 8, 1, c_ltgray, _("Feet"));
   wrefresh(w_encumb);
   break;
  case 4: // Traits tab
   mvwprintz(w_traits, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, h_ltgray, title_TRAITS);
   if (line <= (trait_win_size_y-1)/2) {
    min = 0;
    max = trait_win_size_y;
    if (traitslist.size() < max)
     max = traitslist.size();
   } else if (line >= traitslist.size() - (trait_win_size_y+1)/2) {
    min = (traitslist.size() < trait_win_size_y ? 0 : traitslist.size() - trait_win_size_y);
    max = traitslist.size();
   } else {
    min = line - (trait_win_size_y-1)/2;
    max = line + (trait_win_size_y+1)/2;
    if (traitslist.size() < max)
     max = traitslist.size();
    if (min < 0)
     min = 0;
   }

   for (int i = min; i < max; i++) {
    mvwprintz(w_traits, 1 + i - min, 1, c_ltgray, "                         ");
    if (i > traits.size())
     status = c_ltblue;
    else if (traits[traitslist[i]].points > 0)
     status = c_ltgreen;
    else if (traits[traitslist[i]].points < 0)
     status = c_ltred;
    else
     status = c_yellow;
    if (i == line)
     mvwprintz(w_traits, 1 + i - min, 1, hilite(status),
               traits[traitslist[i]].name.c_str());
    else
     mvwprintz(w_traits, 1 + i - min, 1, status,
               traits[traitslist[i]].name.c_str());
   }
   if (line >= 0 && line < traitslist.size()) {
     fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, "%s", traits[traitslist[line]].description.c_str());
   }
   wrefresh(w_traits);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < traitslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_traits, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_traits, 0, 13 - utf8_width(title_TRAITS)/2, c_ltgray, title_TRAITS);
     for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
      mvwprintz(w_traits, i + 1, 1, c_black, "                         ");
      if (traits[traitslist[i]].points > 0)
       status = c_ltgreen;
      else if (traits[traitslist[i]].points < 0)
       status = c_ltred;
      else
       status = c_yellow;
      mvwprintz(w_traits, i + 1, 1, status, traits[traitslist[i]].name.c_str());
     }
     wrefresh(w_traits);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 5: // Effects tab
   mvwprintz(w_effects, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, h_ltgray, title_EFFECTS);
   half_y = effect_win_size_y / 2;
   if (line <= half_y) {
    min = 0;
    max = effect_win_size_y;
    if (effect_name.size() < max)
     max = effect_name.size();
   } else if (line >= effect_name.size() - half_y) {
    min = (effect_name.size() < effect_win_size_y ? 0 : effect_name.size() - effect_win_size_y);
    max = effect_name.size();
   } else {
    min = line - half_y;
    max = line - half_y + effect_win_size_y;
    if (effect_name.size() < max)
     max = effect_name.size();
    if (min < 0)
     min = 0;
   }

   for (int i = min; i < max; i++) {
    if (i == line)
     mvwprintz(w_effects, 1 + i - min, 0, h_ltgray, effect_name[i].c_str());
    else
     mvwprintz(w_effects, 1 + i - min, 0, c_ltgray, effect_name[i].c_str());
   }
   if (line >= 0 && line < effect_text.size()) {
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, "%s", effect_text[line].c_str());
   }
   wrefresh(w_effects);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < effect_name.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_effects, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_effects, 0, 13 - utf8_width(title_EFFECTS)/2, c_ltgray, title_EFFECTS);
     for (int i = 0; i < effect_name.size() && i < 7; i++)
      mvwprintz(w_effects, i + 1, 0, c_ltgray, effect_name[i].c_str());
     wrefresh(w_effects);
     line = 0;
     curtab = 1;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 3: // Skills tab
   mvwprintz(w_skills, 0, 0, h_ltgray,  _("                          "));
   mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, h_ltgray, title_SKILLS);
   half_y = skill_win_size_y / 2;
   if (line <= half_y) {
    min = 0;
    max = skill_win_size_y;
    if (skillslist.size() < max)
     max = skillslist.size();
   } else if (line >= skillslist.size() - half_y) {
    min = (skillslist.size() < skill_win_size_y ? 0 : skillslist.size() - skill_win_size_y);
    max = skillslist.size();
   } else {
    min = line - half_y;
    max = line - half_y + skill_win_size_y;
    if (skillslist.size() < max)
     max = skillslist.size();
    if (min < 0)
     min = 0;
   }

   Skill *selectedSkill = NULL;

   for (int i = min; i < max; i++)
   {
    Skill *aSkill = skillslist[i];
    SkillLevel level = skillLevel(aSkill);

    bool isLearning = level.isTraining();
    int exercise = level.exercise();
    bool rusting = level.isRusting(g->turn);

    if (i == line) {
      selectedSkill = aSkill;
     if (exercise >= 100)
      status = isLearning ? h_pink : h_magenta;
     else if (rusting)
      status = isLearning ? h_ltred : h_red;
     else
      status = isLearning ? h_ltblue : h_blue;
    } else {
     if (rusting)
      status = isLearning ? c_ltred : c_red;
     else
      status = isLearning ? c_ltblue : c_blue;
    }
    mvwprintz(w_skills, 1 + i - min, 1, c_ltgray, "                         ");
    mvwprintz(w_skills, 1 + i - min, 1, status, "%s:", aSkill->name().c_str());
    mvwprintz(w_skills, 1 + i - min,19, status, "%-2d(%2d%%)", (int)level, (exercise <  0 ? 0 : exercise));
   }

   //Draw Scrollbar
   draw_scrollbar(w_skills, line, skill_win_size_y, skillslist.size(), 1);

   werase(w_info);
   if (line >= 0 && line < skillslist.size()) {
    fold_and_print(w_info, 0, 1, FULL_SCREEN_WIDTH-2, c_magenta, "%s", selectedSkill->description().c_str());
   }
   wrefresh(w_skills);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < skillslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
      werase(w_skills);
     mvwprintz(w_skills, 0, 0, c_ltgray,  _("                          "));
     mvwprintz(w_skills, 0, 13 - utf8_width(title_SKILLS)/2, c_ltgray, title_SKILLS);
     for (int i = 0; i < skillslist.size() && i < skill_win_size_y; i++) {
      Skill *thisSkill = skillslist[i];
      SkillLevel level = skillLevel(thisSkill);
      bool isLearning = level.isTraining();
      bool rusting = level.isRusting(g->turn);

      if (rusting)
       status = isLearning ? c_ltred : c_red;
      else
       status = isLearning ? c_ltblue : c_blue;

      mvwprintz(w_skills, i + 1,  1, status, "%s:", thisSkill->name().c_str());
      mvwprintz(w_skills, i + 1, 19, status, "%-2d(%2d%%)", (int)level, (level.exercise() <  0 ? 0 : level.exercise()));
     }
     wrefresh(w_skills);
     line = 0;
     curtab++;
     break;
   case ' ':
     skillLevel(selectedSkill).toggleTraining();
     break;
    case 'q':
    case 'Q':
    case KEY_ESCAPE:
     done = true;
   }
  }
 } while (!done);

 werase(w_info);
 werase(w_tip);
 werase(w_stats);
 werase(w_encumb);
 werase(w_traits);
 werase(w_effects);
 werase(w_skills);
 werase(w_speed);
 werase(w_info);
 werase(w_grid_top);
 werase(w_grid_effect);
 werase(w_grid_skill);
 werase(w_grid_trait);

 delwin(w_info);
 delwin(w_tip);
 delwin(w_stats);
 delwin(w_encumb);
 delwin(w_traits);
 delwin(w_effects);
 delwin(w_skills);
 delwin(w_speed);
 delwin(w_grid_top);
 delwin(w_grid_effect);
 delwin(w_grid_skill);
 delwin(w_grid_trait);
 erase();
}

void player::disp_morale()
{
    // Ensure the player's persistent morale effects are up-to-date.
    apply_persistent_morale();

    // Create and draw the window itself.
    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                        (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0,
                        (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0);
    draw_border(w);

    // Figure out how wide the name column needs to be.
    int name_column_width = 18;
    for (int i = 0; i < morale.size(); i++)
    {
        int length = morale[i].name(morale_data).length();
        if ( length > name_column_width)
        {
            name_column_width = length;
        }
    }

    // If it's too wide, truncate.
    if (name_column_width > 72)
    {
        name_column_width = 72;
    }

    // Start printing the number right after the name column.
    // We'll right-justify it later.
    int number_pos = name_column_width + 1;

    // Header
    mvwprintz(w, 1,  1, c_white, _("Morale Modifiers:"));
    mvwprintz(w, 2,  1, c_ltgray, _("Name"));
    mvwprintz(w, 2, name_column_width+2, c_ltgray, _("Value"));

    // Print out the morale entries.
    for (int i = 0; i < morale.size(); i++)
    {
        std::string name = morale[i].name(morale_data);
        int bonus = net_morale(morale[i]);

        // Trim the name if need be.
        if (name.length() > name_column_width)
        {
            name = name.erase(name_column_width-3, std::string::npos) + "...";
        }

        // Print out the name.
        mvwprintz(w, i + 3,  1, (bonus < 0 ? c_red : c_green), name.c_str());

        // Print out the number, right-justified.
        mvwprintz(w, i + 3, number_pos, (bonus < 0 ? c_red : c_green),
                  "% 6d", bonus);
    }

    // Print out the total morale, right-justified.
    int mor = morale_level();
    mvwprintz(w, 20, 1, (mor < 0 ? c_red : c_green), _("Total:"));
    mvwprintz(w, 20, number_pos, (mor < 0 ? c_red : c_green), "% 6d", mor);

    // Print out the focus gain rate, right-justified.
    double gain = (calc_focus_equilibrium() - focus_pool) / 100.0;
    mvwprintz(w, 22, 1, (gain < 0 ? c_red : c_green), _("Focus gain:"));
    mvwprintz(w, 22, number_pos-3, (gain < 0 ? c_red : c_green), _("%6.2f per minute"), gain);

    // Make sure the changes are shown.
    wrefresh(w);

    // Wait for any keystroke.
    getch();

    // Close the window.
    werase(w);
    delwin(w);
}

void player::disp_status(WINDOW *w, WINDOW *w2)
{
    bool sideStyle = use_narrow_sidebar();
    WINDOW *weapwin = sideStyle ? w2 : w;

    // Print current weapon, or attachment if active.
    item* gunmod = weapon.active_gunmod();
    std::string mode = "";
    std::stringstream attachment;
    if (gunmod != NULL)
    {
        attachment << gunmod->type->name.c_str();
        for (int i = 0; i < weapon.contents.size(); i++)
                if (weapon.contents[i].is_gunmod() &&
                        weapon.contents[i].has_flag("MODE_AUX"))
                    attachment << " (" << weapon.contents[i].charges << ")";
        mvwprintz(weapwin, sideStyle ? 1 : 0, 0, c_ltgray, _("%s (Mod)"), attachment.str().c_str());
    }
    else
    {
        if (weapon.mode == "MODE_BURST")
                mvwprintz(weapwin, sideStyle ? 1 : 0, 0, c_ltgray, _("%s (Burst)"), weapname().c_str());
        else
            mvwprintz(weapwin, sideStyle ? 1 : 0, 0, c_ltgray, _("%s"), weapname().c_str());
    }

    if (weapon.is_gun()) {
        int adj_recoil = recoil + driving_recoil;
        if (adj_recoil > 0) {
            nc_color c = c_ltgray;
            if (adj_recoil >= 36)
                c = c_red;
            else if (adj_recoil >= 20)
                c = c_ltred;
            else if (adj_recoil >= 4)
                c = c_yellow;
            int y = sideStyle ? 1 : 0;
            int x = sideStyle ? (getmaxx(weapwin) - 6) : 34;
            mvwprintz(weapwin, y, x, c, _("Recoil"));
        }
    }

    // Print currently used style or weapon mode.
    std::string style = "";
    if (is_armed())
    {
        if (style_selected == "style_none" || !can_melee())
            style = _("Normal");
        else
            style = martialarts[style_selected].name;

        int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
        mvwprintz(weapwin, 1, x, c_red, style.c_str());
    }
    else
    {
        if (style_selected == "style_none")
        {
            style = _("No Style");
        }
        else
        {
            style = martialarts[style_selected].name;
        }
        if (style != "")
        {
            int x = sideStyle ? (getmaxx(weapwin) - 13) : 0;
            mvwprintz(weapwin, 1, x, c_blue, style.c_str());
        }
    }

    wmove(w, sideStyle ? 1 : 2, 0);
    if (hunger > 2800)
        wprintz(w, c_red,    _("Starving!"));
    else if (hunger > 1400)
        wprintz(w, c_ltred,  _("Near starving"));
    else if (hunger > 300)
        wprintz(w, c_ltred,  _("Famished"));
    else if (hunger > 100)
        wprintz(w, c_yellow, _("Very hungry"));
    else if (hunger > 40)
        wprintz(w, c_yellow, _("Hungry"));
    else if (hunger < 0)
        wprintz(w, c_green,  _("Full"));
    else if (hunger < -20)
        wprintz(w, c_green,  _("Sated"));
    else if (hunger < -60)
        wprintz(w, c_green,  _("Engorged"));

 // Find hottest/coldest bodypart
 int min = 0, max = 0;
 for (int i = 0; i < num_bp ; i++ ){
  if      (temp_cur[i] > BODYTEMP_HOT  && temp_cur[i] > temp_cur[max]) max = i;
  else if (temp_cur[i] < BODYTEMP_COLD && temp_cur[i] < temp_cur[min]) min = i;
 }
 // Compare which is most extreme
 int print;
 if (temp_cur[max] - BODYTEMP_NORM > BODYTEMP_NORM + temp_cur[min]) print = max;
 else print = min;
 // Assign zones to temp_cur and temp_conv for comparison
 int cur_zone = 0;
 if      (temp_cur[print] >  BODYTEMP_SCORCHING) cur_zone = 7;
 else if (temp_cur[print] >  BODYTEMP_VERY_HOT)  cur_zone = 6;
 else if (temp_cur[print] >  BODYTEMP_HOT)       cur_zone = 5;
 else if (temp_cur[print] >  BODYTEMP_COLD)      cur_zone = 4;
 else if (temp_cur[print] >  BODYTEMP_VERY_COLD) cur_zone = 3;
 else if (temp_cur[print] >  BODYTEMP_FREEZING)  cur_zone = 2;
 else if (temp_cur[print] <= BODYTEMP_FREEZING)  cur_zone = 1;
 int conv_zone = 0;
 if      (temp_conv[print] >  BODYTEMP_SCORCHING) conv_zone = 7;
 else if (temp_conv[print] >  BODYTEMP_VERY_HOT)  conv_zone = 6;
 else if (temp_conv[print] >  BODYTEMP_HOT)       conv_zone = 5;
 else if (temp_conv[print] >  BODYTEMP_COLD)      conv_zone = 4;
 else if (temp_conv[print] >  BODYTEMP_VERY_COLD) conv_zone = 3;
 else if (temp_conv[print] >  BODYTEMP_FREEZING)  conv_zone = 2;
 else if (temp_conv[print] <= BODYTEMP_FREEZING)  conv_zone = 1;
 // delta will be positive if temp_cur is rising
 int delta = conv_zone - cur_zone;
 // Decide if temp_cur is rising or falling
 const char *temp_message = "Error";
 if      (delta >   2) temp_message = _(" (Rising!!)");
 else if (delta ==  2) temp_message = _(" (Rising!)");
 else if (delta ==  1) temp_message = _(" (Rising)");
 else if (delta ==  0) temp_message = "";
 else if (delta == -1) temp_message = _(" (Falling)");
 else if (delta == -2) temp_message = _(" (Falling!)");
 else if (delta <  -2) temp_message = _(" (Falling!!)");
 // Print the hottest/coldest bodypart, and if it is rising or falling in temperature

    wmove(w, sideStyle ? 6 : 1, sideStyle ? 0 : 9);
    if      (temp_cur[print] >  BODYTEMP_SCORCHING)
        wprintz(w, c_red,   _("Scorching!%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_VERY_HOT)
        wprintz(w, c_ltred, _("Very hot!%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_HOT)
        wprintz(w, c_yellow,_("Warm%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_COLD) // If you're warmer than cold, you are comfortable
        wprintz(w, c_green, _("Comfortable%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_VERY_COLD)
        wprintz(w, c_ltblue,_("Chilly%s"), temp_message);
    else if (temp_cur[print] >  BODYTEMP_FREEZING)
        wprintz(w, c_cyan,  _("Very cold!%s"), temp_message);
    else if (temp_cur[print] <= BODYTEMP_FREEZING)
        wprintz(w, c_blue,  _("Freezing!%s"), temp_message);

    int x = sideStyle ? 37 : 32;
    int y = sideStyle ?  0 :  1;
    if(has_disease("deaf")) {
        mvwprintz(sideStyle ? w2 : w, y, x, c_red, _("Deaf!"), volume);
    } else {
        mvwprintz(sideStyle ? w2 : w, y, x, c_yellow, _("Sound %d"), volume);
    }
    volume = 0;

    wmove(w, 2, sideStyle ? 0 : 15);
    if (thirst > 520)
        wprintz(w, c_ltred,  _("Parched"));
    else if (thirst > 240)
        wprintz(w, c_ltred,  _("Dehydrated"));
    else if (thirst > 80)
        wprintz(w, c_yellow, _("Very thirsty"));
    else if (thirst > 40)
        wprintz(w, c_yellow, _("Thirsty"));
    else if (thirst < 0)
        wprintz(w, c_green,  _("Slaked"));
    else if (thirst < -20)
        wprintz(w, c_green,  _("Hydrated"));
    else if (thirst < -60)
        wprintz(w, c_green,  _("Turgid"));

    wmove(w, sideStyle ? 3 : 2, sideStyle ? 0 : 30);
    if (fatigue > 575)
        wprintz(w, c_red,    _("Exhausted"));
    else if (fatigue > 383)
        wprintz(w, c_ltred,  _("Dead tired"));
    else if (fatigue > 191)
        wprintz(w, c_yellow, _("Tired"));

    wmove(w, sideStyle ? 4 : 2, sideStyle ? 0 : 41);
    wprintz(w, c_white, _("Focus"));
    nc_color col_xp = c_dkgray;
    if (focus_pool >= 100)
        col_xp = c_white;
    else if (focus_pool >  0)
        col_xp = c_ltgray;
    wprintz(w, col_xp, " %d", focus_pool);

    nc_color col_pain = c_yellow;
    if (pain - pkill >= 60)
        col_pain = c_red;
    else if (pain - pkill >= 40)
        col_pain = c_ltred;
    if (pain - pkill > 0)
        mvwprintz(w, sideStyle ? 0 : 3, 0, col_pain, _("Pain %d"), pain - pkill);

    int morale_cur = morale_level ();
    nc_color col_morale = c_white;
    if (morale_cur >= 10)
        col_morale = c_green;
    else if (morale_cur <= -10)
        col_morale = c_red;
    const char *morale_str;
    if      (morale_cur >= 200) morale_str = "8D";
    else if (morale_cur >= 100) morale_str = ":D";
    else if (morale_cur >= 10)  morale_str = ":)";
    else if (morale_cur > -10)  morale_str = ":|";
    else if (morale_cur > -100) morale_str = "):";
    else if (morale_cur > -200) morale_str = "D:";
    else                        morale_str = "D8";
    mvwprintz(w, sideStyle ? 0 : 3, sideStyle ? 11 : 10, col_morale, morale_str);

 vehicle *veh = g->m.veh_at (posx, posy);
 if (in_vehicle && veh) {
  veh->print_fuel_indicator(w, sideStyle ? 2 : 3, sideStyle ? getmaxx(w) - 5 : 49);
  nc_color col_indf1 = c_ltgray;

  float strain = veh->strain();
  nc_color col_vel = strain <= 0? c_ltblue :
                     (strain <= 0.2? c_yellow :
                     (strain <= 0.4? c_ltred : c_red));

  bool has_turrets = false;
  for (int p = 0; p < veh->parts.size(); p++) {
   if (veh->part_flag (p, "TURRET")) {
    has_turrets = true;
    break;
   }
  }

  if (has_turrets) {
   mvwprintz(w, 3, sideStyle ? 16 : 25, col_indf1, "Gun:");
   wprintz(w, veh->turret_mode ? c_ltred : c_ltblue,
              veh->turret_mode ? "auto" : "off ");
  }

  //
  // Draw the speedometer.
  //

  int speedox = sideStyle ? 0 : 33;
  int speedoy = sideStyle ? 5 :  3;

  bool metric = OPTIONS["USE_METRIC_SPEEDS"] == "km/h";
  const char *units = metric ? _("km/h") : _("mph");
  int velx    = metric ?  5 : 4; // strlen(units) + 1
  int cruisex = metric ? 10 : 9; // strlen(units) + 6
  float conv  = metric ? 0.0161f : 0.01f;

  if (0 == sideStyle) {
    if (!veh->cruise_on) speedox += 2;
    if (!metric)         speedox++;
  }

  const char *speedo = veh->cruise_on ? "{%s....>....}" : "{%s....}";
  mvwprintz(w, speedoy, speedox,        col_indf1, speedo, units);
  mvwprintz(w, speedoy, speedox + velx, col_vel,   "%4d", int(veh->velocity * conv));
  if (veh->cruise_on)
    mvwprintz(w, speedoy, speedox + cruisex, c_ltgreen, "%4d", int(veh->cruise_velocity * conv));


  if (veh->velocity != 0) {
   nc_color col_indc = veh->skidding? c_red : c_green;
   int dfm = veh->face.dir() - veh->move.dir();
   wmove(w, sideStyle ? 5 : 3, getmaxx(w) - 3);
   wprintz(w, col_indc, dfm <  0 ? "L" : ".");
   wprintz(w, col_indc, dfm == 0 ? "0" : ".");
   wprintz(w, col_indc, dfm >  0 ? "R" : ".");
  }
 } else {  // Not in vehicle
  nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
           col_per = c_white, col_spd = c_white, col_time = c_white;
  if (str_cur < str_max)
   col_str = c_red;
  if (str_cur > str_max)
   col_str = c_green;
  if (dex_cur < dex_max)
   col_dex = c_red;
  if (dex_cur > dex_max)
   col_dex = c_green;
  if (int_cur < int_max)
   col_int = c_red;
  if (int_cur > int_max)
   col_int = c_green;
  if (per_cur < per_max)
   col_per = c_red;
  if (per_cur > per_max)
   col_per = c_green;
  int spd_cur = get_speed();
  if (spd_cur < 100)
   col_spd = c_red;
  if (spd_cur > 100)
   col_spd = c_green;

    int x  = sideStyle ? 18 : 13;
    int y  = sideStyle ?  0 :  3;
    int dx = sideStyle ?  0 :  7;
    int dy = sideStyle ?  1 :  0;
    mvwprintz(w, y + dy * 0, x + dx * 0, col_str, _("Str %2d"), str_cur);
    mvwprintz(w, y + dy * 1, x + dx * 1, col_dex, _("Dex %2d"), dex_cur);
    mvwprintz(w, y + dy * 2, x + dx * 2, col_int, _("Int %2d"), int_cur);
    mvwprintz(w, y + dy * 3, x + dx * 3, col_per, _("Per %2d"), per_cur);

    int spdx = sideStyle ?  0 : x + dx * 4;
    int spdy = sideStyle ?  5 : y + dy * 4;
    mvwprintz(w, spdy, spdx, col_spd, _("Spd %2d"), spd_cur);
		if (this->weight_carried() > this->weight_capacity() || this->volume_carried() > this->volume_capacity() - 2) {
				col_time = c_red;
		}
    wprintz(w, col_time, "  %d", movecounter);
 }
}

bool player::has_trait(const std::string &flag) const
{
    // Look for active mutations and traits
    return (flag != "") && (my_mutations.find(flag) != my_mutations.end());
}

bool player::has_base_trait(const std::string &flag) const
{
    // Look only at base traits
    return (flag != "") && (my_traits.find(flag) != my_traits.end());
}

bool player::has_conflicting_trait(const std::string &flag) const
{
    if(mutation_data[flag].cancels.size() > 0) {
        std::vector<std::string> cancels = mutation_data[flag].cancels;

        for (int i = 0; i < cancels.size(); i++) {
            if ( has_trait(cancels[i]) )
                return true;
        }
    }

    return false;
}

void toggle_str_set(std::set<std::string> &set, const std::string &str)
{
    std::set<std::string>::iterator i = set.find(str);
    if (i == set.end()) {
        set.insert(str);
    } else {
        set.erase(i);
    }
}

void player::toggle_trait(const std::string &flag)
{
    toggle_str_set(my_traits, flag); //Toggles a base trait on the player
    toggle_str_set(my_mutations, flag); //Toggles corresponding trait in mutations list as well.
    recalc_sight_limits();
}

void player::toggle_mutation(const std::string &flag)
{
    toggle_str_set(my_mutations, flag); //Toggles a mutation on the player
    recalc_sight_limits();
}

void player::set_cat_level_rec(const std::string &sMut)
{
    if (!has_base_trait(sMut)) { //Skip base traits
        for (int i = 0; i < mutation_data[sMut].category.size(); i++) {
            mutation_category_level[mutation_data[sMut].category[i]] += 8;
        }

        for (int i = 0; i < mutation_data[sMut].prereqs.size(); i++) {
            set_cat_level_rec(mutation_data[sMut].prereqs[i]);
        }
    }
}

void player::set_highest_cat_level()
{
    mutation_category_level.clear();

    // Loop through our mutations
    for (std::set<std::string>::iterator iter = my_mutations.begin(); iter != my_mutations.end(); ++iter) {
        set_cat_level_rec(*iter);
    }
}

std::string player::get_highest_category() const // Returns the mutation category with the highest strength
{
    int iLevel = 0;
    std::string sMaxCat = "";

    for (std::map<std::string, int>::const_iterator iter = mutation_category_level.begin(); iter != mutation_category_level.end(); ++iter) {
        if (iter->second > iLevel) {
            sMaxCat = iter->first;
            iLevel = iter->second;
        } else if (iter->second == iLevel) {
            sMaxCat = "";  // no category on ties
        }
    }
    return sMaxCat;
}

std::string player::get_category_dream(const std::string &cat, int strength) const // Returns a randomly selected dream
{
    std::string message;
    std::vector<dream> valid_dreams;
    dream selected_dream;
    for (int i = 0; i < dreams.size(); i++) { //Pull the list of dreams
        if ((dreams[i].category == cat) && (dreams[i].strength == strength)) { //Pick only the ones matching our desired category and strength
            valid_dreams.push_back(dreams[i]); // Put the valid ones into our list
        }
    }

    int index = rng(0, valid_dreams.size() - 1); // Randomly select a dream from the valid list
    selected_dream = valid_dreams[index];
    index = rng(0, selected_dream.messages.size() - 1); // Randomly selected a message from the chosen dream
    message = selected_dream.messages[index];
    return message;
}

bool player::in_climate_control()
{
    bool regulated_area=false;
    // Check
    if(has_active_bionic("bio_climate")) { return true; }
    for (int i = 0; i < worn.size(); i++)
    {
        if ((dynamic_cast<it_armor*>(worn[i].type))->is_power_armor() &&
            (has_active_item("UPS_on") || has_active_item("adv_UPS_on") || has_active_bionic("bio_power_armor_interface") || has_active_bionic("bio_power_armor_interface_mkII")))
        {
            return true;
        }
    }
    if(int(g->turn) >= next_climate_control_check)
    {
        next_climate_control_check=int(g->turn)+20;  // save cpu and similate acclimation.
        int vpart = -1;
        vehicle *veh = g->m.veh_at(posx, posy, vpart);
        if(veh)
        {
            regulated_area=(
                veh->is_inside(vpart) &&    // Already checks for opened doors
                veh->total_power(true) > 0  // Out of gas? No AC for you!
            );  // TODO: (?) Force player to scrounge together an AC unit
        }
        // TODO: AC check for when building power is implemented
        last_climate_control_ret=regulated_area;
        if(!regulated_area) { next_climate_control_check+=40; }  // Takes longer to cool down / warm up with AC, than it does to step outside and feel cruddy.
    }
    else
    {
        return ( last_climate_control_ret ? true : false );
    }
    return regulated_area;
}

bool player::has_bionic(const bionic_id & b) const
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return true;
 }
 return false;
}

bool player::has_active_optcloak() const {
  static const std::string str_UPS_on("UPS_on");
  static const std::string str_adv_UPS_on("adv_UPS_on");
  static const std::string str_optical_cloak("optical_cloak");

  if ((has_active_item(str_UPS_on) || has_active_item(str_adv_UPS_on))
      && is_wearing(str_optical_cloak)) {
    return true;
  } else {
    return false;
  }
}

bool player::has_active_bionic(const bionic_id & b) const
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return (my_bionics[i].powered);
 }
 return false;
}

void player::add_bionic(bionic_id b)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return; // No duplicates!
 }
 char newinv;
 if (my_bionics.size() == 0)
  newinv = 'a';
 else if (my_bionics.size() == 26)
  newinv = 'A';
 else if (my_bionics.size() == 52)
  newinv = '0';
 else if (my_bionics.size() == 62)
  newinv = '"';
 else if (my_bionics.size() == 76)
  newinv = ':';
 else if (my_bionics.size() == 83)
  newinv = '[';
 else if (my_bionics.size() == 89)
  newinv = '{';
 else
  newinv = my_bionics[my_bionics.size() - 1].invlet + 1;
 my_bionics.push_back(bionic(b, newinv));
 recalc_sight_limits();
}

int player::num_bionics() const
{
    return my_bionics.size();
}

bionic& player::bionic_at_index(int i)
{
    return my_bionics[i];
}

bionic* player::bionic_by_invlet(char ch) {
    for (size_t i = 0; i < my_bionics.size(); i++) {
        if (my_bionics[i].invlet == ch) {
            return &my_bionics[i];
        }
    }
    return 0;
}

// Returns true if a bionic was removed.
bool player::remove_random_bionic() {
    const int numb = num_bionics();
    if (numb) {
        int rem = rng(0, num_bionics() - 1);
        my_bionics.erase(my_bionics.begin() + rem);
        recalc_sight_limits();
    }
    return numb;
}

void player::charge_power(int amount)
{
 power_level += amount;
 if (power_level > max_power_level)
  power_level = max_power_level;
 if (power_level < 0)
  power_level = 0;
}


/*
 * Calculate player brightness based on the brightest active item, as
 * per itype tag LIGHT_* and optional CHARGEDIM ( fade starting at 20% charge )
 * item.light.* is -unimplemented- for the moment, as it is a custom override for
 * applying light sources/arcs with specific angle and direction.
 */
float player::active_light()
{
    float lumination = 0;

    int maxlum = 0;
    const invslice & stacks = inv.slice();
    for( int x = 0; x < stacks.size(); ++x ) {
        item &itemit = stacks[x]->front();
        item * stack_iter = &itemit;
        if (stack_iter->active && stack_iter->charges > 0) {
            int lumit = stack_iter->getlight_emit(true);
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    // worn light sources? Unimplemented

    if (!weapon.is_null()) {
        if ( weapon.active  && weapon.charges > 0) {
            int lumit = weapon.getlight_emit(true);
            if ( maxlum < lumit ) {
                maxlum = lumit;
            }
        }
    }

    lumination = (float)maxlum;

    if ( lumination < 60 && has_active_bionic("bio_flashlight") ) {
        lumination = 60;
    } else if ( lumination < 25 && has_artifact_with(AEP_GLOW) ) {
        lumination = 25;
    }

    return lumination;
}

point player::pos()
{
    return point(posx, posy);
}

int player::sight_range(int light_level) const
{
    // Apply the sight boost (night vision).
    if (light_level < sight_boost_cap) {
        light_level = std::min(light_level + sight_boost, sight_boost_cap);
    }

    // Clamp to sight_max.
    return std::min(light_level, sight_max);
}

// This must be called when any of the following change:
// - diseases
// - bionics
// - traits
// - underwater
// - clothes
// With the exception of clothes, all changes to these player attributes must
// occur through a function in this class which calls this function. Clothes are
// typically added/removed with wear() and takeoff(), but direct access to the
// 'wears' vector is still allowed due to refactor exhaustion.
void player::recalc_sight_limits()
{
    sight_max = 9999;
    sight_boost = 0;
    sight_boost_cap = 0;

    // Set sight_max.
    if (has_effect("blind")) {
        sight_max = 0;
    } else if (has_disease("in_pit") ||
            has_disease("boomered") ||
            (underwater && !has_bionic("bio_membrane") &&
                !has_trait("MEMBRANE") && !worn_with_flag("SWIM_GOGGLES"))) {
        sight_max = 1;
    } else if (has_trait("MYOPIC") && !is_wearing("glasses_eye") &&
            !is_wearing("glasses_monocle") && !is_wearing("glasses_bifocal") &&
            !has_disease("contacts")) {
        sight_max = 4;
    }

    // Set sight_boost and sight_boost_cap, based on night vision.
    // (A player will never have more than one night vision trait.)
    sight_boost_cap = 12;
    if (has_nv() || has_trait("NIGHTVISION3") || has_trait("ELFA_FNV")) {
        sight_boost = sight_boost_cap;
	}else if (has_trait("ELFA_NV")) {
        sight_boost = 6;
    } else if (has_trait("NIGHTVISION2") || has_trait("FEL_NV")) {
        sight_boost = 4;
    } else if (has_trait("NIGHTVISION")) {
        sight_boost = 1;
    }
}

int player::unimpaired_range()
{
 int ret = DAYLIGHT_LEVEL;
 if (has_disease("in_pit"))
  ret = 1;
 if (has_effect("blind"))
  ret = 0;
 return ret;
}

int player::overmap_sight_range(int light_level)
{
    int sight = sight_range(light_level);
    if( sight < SEEX ) {
        return 0;
    }
    if( sight <= SEEX * 4) {
        return (sight / (SEEX / 2) );
    }
    if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && !has_trait("EAGLEEYED"))  {
        return 20;
    }
    else if (!(has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && has_trait("EAGLEEYED"))  {
        return 20;
    }
    else if ((has_amount("binoculars", 1) || has_amount("rifle_scope", 1) ||
        -1 != weapon.has_gunmod("rifle_scope") ) && has_trait("EAGLEEYED"))  {
        return 30;
    }
    
    return 10;
}

int player::clairvoyance()
{
 if (has_artifact_with(AEP_CLAIRVOYANCE))
  return 3;
 if (has_artifact_with(AEP_SUPER_CLAIRVOYANCE))
  return 40;
 return 0;
}

bool player::sight_impaired()
{
 return has_disease("boomered") ||
  (underwater && !has_bionic("bio_membrane") && !has_trait("MEMBRANE")
              && !worn_with_flag("SWIM_GOGGLES")) ||
  (has_trait("MYOPIC") && !is_wearing("glasses_eye")
                        && !is_wearing("glasses_monocle")
                        && !is_wearing("glasses_bifocal")
                        && !has_disease("contacts"));
}

bool player::has_two_arms() const
{
 if (has_bionic("bio_blaster") || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10)
  return false;
 return true;
}

bool player::avoid_trap(trap* tr)
{
  int myroll = dice(3, int(dex_cur + skillLevel("dodge") * 1.5));
 int traproll;
 if (per_cur - encumb(bp_eyes) >= tr->visibility)
  traproll = dice(3, tr->avoidance);
 else
  traproll = dice(6, tr->avoidance);
 if (has_trait("LIGHTSTEP"))
  myroll += dice(2, 6);
 if (has_trait("CLUMSY"))
  myroll -= dice(2, 6);
 if (myroll >= traproll)
  return true;
 return false;
}

bool player::has_nv()
{
    static bool nv = false;

    if( !nv_cached ) {
        nv_cached = true;
        nv = ((is_wearing("goggles_nv") && (has_active_item("UPS_on") ||
                                            has_active_item("adv_UPS_on"))) ||
              has_active_bionic("bio_night_vision"));
    }

    return nv;
}

void player::pause()
{
    moves = 0;
    if (recoil > 0) {
        if (str_cur + 2 * skillLevel("gun") >= recoil) {
            recoil = 0;
        } else {
            recoil -= str_cur + 2 * skillLevel("gun");
            recoil = int(recoil / 2);
        }
    }

    //Web Weavers...weave web
    if (has_trait("WEB_WEAVER") && !in_vehicle) {
      g->m.add_field(posx, posy, fd_web, 1); //this adds density to if its not already there.
      g->add_msg("You spin some webbing.");
     }

    // Meditation boost for Toad Style, obsolete
    if (weapon.type->id == "style_toad" && activity.type == ACT_NULL) {
        int arm_amount = 1 + (int_cur - 6) / 3 + (per_cur - 6) / 3;
        int arm_max = (int_cur + per_cur) / 2;
        if (arm_amount > 3) {
            arm_amount = 3;
        }
        if (arm_max > 20) {
            arm_max = 20;
        }
        add_disease("armor_boost", 2, false, arm_amount, arm_max);
    }

    // Train swimming if underwater
    if (underwater) {
        practice(g->turn, "swimming", 1);
        if (g->temperature <= 50) {
            drench(100, mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms)|mfb(bp_head)|
                           mfb(bp_eyes)|mfb(bp_mouth)|mfb(bp_feet)|mfb(bp_hands));
        } else {
            drench(100, mfb(bp_legs)|mfb(bp_torso)|mfb(bp_arms)|mfb(bp_head)|
                           mfb(bp_eyes)|mfb(bp_mouth));
        }
    }

    VehicleList vehs = g->m.get_vehicles();
    vehicle* veh = NULL;
    for (int v = 0; v < vehs.size(); ++v) {
        veh = vehs[v].v;
        if (veh && veh->velocity != 0 && veh->player_in_control(this)) {
            if (one_in(10)) {
                practice(g->turn, "driving", 1);
            }
            break;
        }
    }
}

int player::throw_range(int pos)
{
 item tmp;
 if (pos == -1)
  tmp = weapon;
 else if (pos == INT_MIN)
  return -1;
 else
  tmp = inv.find_item(pos);

 if (tmp.count_by_charges() && tmp.charges > 1)
  tmp.charges = 1;

 if ((tmp.weight() / 113) > int(str_cur * 15))
  return 0;
 // Increases as weight decreases until 150 g, then decreases again
 int ret = (str_cur * 8) / (tmp.weight() >= 150 ? tmp.weight() / 113 : 10 - int(tmp.weight() / 15));
 ret -= int(tmp.volume() / 4);
 if (has_active_bionic("bio_railgun") && (tmp.made_of("iron") || tmp.made_of("steel")))
    ret *= 2;
 if (ret < 1)
  return 1;
// Cap at double our strength + skill
 if (ret > str_cur * 1.5 + skillLevel("throw"))
   return str_cur * 1.5 + skillLevel("throw");
 return ret;
}

int player::ranged_dex_mod(bool real_life)
{
    const int dex = (real_life ? dex_cur : dex_max);

    if (dex >= 12) { return 0; }
    return 12 - dex;
}

int player::ranged_per_mod(bool real_life)
{
 const int per = (real_life ? per_cur : per_max);

 if (per >= 12) { return 0; }
 return 12 - per;
}

int player::throw_dex_mod(bool real_life)
{
 int dex = (real_life ? dex_cur : dex_max);
 if (dex == 8 || dex == 9)
  return 0;
 if (dex >= 10)
  return (real_life ? 0 - rng(0, dex - 9) : 9 - dex);

 int deviation = 0;
 if (dex < 4)
  deviation = 4 * (8 - dex);
 else if (dex < 6)
  deviation = 3 * (8 - dex);
 else
  deviation = 2 * (8 - dex);

 return (real_life ? rng(0, deviation) : deviation);
}

int player::read_speed(bool real_life)
{
 int intel = (real_life ? int_cur : int_max);
 int ret = 1000 - 50 * (intel - 8);
 if (has_trait("FASTREADER"))
  ret *= .8;
 if (has_trait("SLOWREADER"))
  ret *= 1.3;
 if (ret < 100)
  ret = 100;
 return (real_life ? ret : ret / 10);
}

int player::rust_rate(bool real_life)
{
    if (OPTIONS["SKILL_RUST"] == "off") {
        return 0;
    }

    int intel = (real_life ? int_cur : int_max);
    int ret = ((OPTIONS["SKILL_RUST"] == "vanilla" || OPTIONS["SKILL_RUST"] == "capped") ? 500 : 500 - 35 * (intel - 8));

    if (has_trait("FORGETFUL")) {
        ret *= 1.33;
    }
    
    if (has_trait("GOODMEMORY")) {
        ret *= .66;
    }

    if (ret < 0) {
        ret = 0;
    }

    return (real_life ? ret : ret / 10);
}

int player::talk_skill()
{
    int ret = int_cur + per_cur + skillLevel("speech") * 3;
    if (has_trait("UGLY"))
        ret -= 3;
    else if (has_trait("DEFORMED"))
        ret -= 6;
    else if (has_trait("DEFORMED2"))
        ret -= 12;
    else if (has_trait("DEFORMED3"))
        ret -= 18;
    else if (has_trait("PRETTY"))
        ret += 1;
    else if (has_trait("BEAUTIFUL"))
        ret += 2;
    else if (has_trait("BEAUTIFUL2"))
        ret += 4;
    else if (has_trait("BEAUTIFUL3"))
        ret += 6;
    return ret;
}

int player::intimidation()
{
 int ret = str_cur * 2;
 if (weapon.is_gun())
  ret += 10;
 if (weapon.damage_bash() >= 12 || weapon.damage_cut() >= 12)
  ret += 5;
 if (has_trait("DEFORMED2"))
  ret += 3;
 else if (has_trait("DEFORMED3"))
  ret += 6;
 else if (has_trait("PRETTY"))
  ret -= 1;
 else if (has_trait("BEAUTIFUL") || has_trait("BEAUTIFUL2") || has_trait("BEAUTIFUL3"))
  ret -= 4;
 if (stim > 20)
  ret += 2;
 if (has_disease("drunk"))
  ret -= 4;

 return ret;
}

bool player::is_dead_state() {
    return hp_cur[hp_head] <= 0 || hp_cur[hp_head] <= 0;
}

void player::on_gethit(Creature *source, body_part bp_hit, damage_instance &) {
    bool u_see = g->u_see(this);
    if (is_player())
    {
        if (g->u.activity.type == ACT_RELOAD)
        {
            g->add_msg(_("You stop reloading."));
        }
        else if (g->u.activity.type == ACT_READ)
        {
            g->add_msg(_("You stop reading."));
        }
        else if (g->u.activity.type == ACT_CRAFT || g->u.activity.type == ACT_LONGCRAFT)
        {
            g->add_msg(_("You stop crafting."));
            g->u.activity.type = ACT_NULL;
        }
    }
    if (source != NULL) {
        if (has_active_bionic("bio_ods"))
        {
            if (is_player()) {
                g->add_msg(_("Your offensive defense system shocks %s in mid-attack!"),
                            source->disp_name().c_str());
            } else if (u_see) {
                g->add_msg(_("%s's offensive defense system shocks %s in mid-attack!"),
                            disp_name().c_str(),
                            source->disp_name().c_str());
            }
            damage_instance ods_shock_damage;
            ods_shock_damage.add_damage(DT_ELECTRIC, rng(10,40));
            source->deal_damage(this, bp_torso, 3, ods_shock_damage);
        }
        if (encumb(bp_hit) == 0 &&(has_trait("SPINES") || has_trait("QUILLS")))
        {
            int spine = rng(1, (has_trait("QUILLS") ? 20 : 8));
            if (!is_player()) {
                if( u_see ) {
                    g->add_msg(_("%1$s's %2$s puncture %s in mid-attack!"), name.c_str(),
                                (g->u.has_trait("QUILLS") ? _("quills") : _("spines")),
                                source->disp_name().c_str());
                }
            } else {
                g->add_msg(_("Your %s puncture %s in mid-attack!"),
                            (g->u.has_trait("QUILLS") ? _("quills") : _("spines")),
                            source->disp_name().c_str());
            }
            damage_instance spine_damage;
            spine_damage.add_damage(DT_STAB, spine);
            source->deal_damage(this, bp_torso, 3, spine_damage);
        }
    }
}

dealt_damage_instance player::deal_damage(Creature* source, body_part bp,
                                          int side, const damage_instance& d) {

    dealt_damage_instance dealt_dams = Creature::deal_damage(source, bp, side, d);
    int dam = dealt_dams.total_damage();

    if (has_disease("sleep")) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

    if (is_player())
        g->cancel_activity_query(_("You were hurt!"));

    // TODO: Pre or post blit hit tile onto "this"'s location here
    if( g->u_see( this->posx, this->posy ) ) {
        g->draw_hit_player(this);
    }

    // handle snake artifacts
    if (has_artifact_with(AEP_SNAKES) && dam >= 6) {
        int snakes = int(dam / 6);
        std::vector<point> valid;
        for (int x = posx - 1; x <= posx + 1; x++) {
            for (int y = posy - 1; y <= posy + 1; y++) {
                if (g->is_empty(x, y))
                valid.push_back( point(x, y) );
            }
        }
        if (snakes > valid.size())
            snakes = valid.size();
        if (snakes == 1)
            g->add_msg(_("A snake sprouts from your body!"));
        else if (snakes >= 2)
            g->add_msg(_("Some snakes sprout from your body!"));
        monster snake(GetMType("mon_shadow_snake"));
        for (int i = 0; i < snakes; i++) {
            int index = rng(0, valid.size() - 1);
            point sp = valid[index];
            valid.erase(valid.begin() + index);
            snake.spawn(sp.x, sp.y);
            snake.friendly = -1;
            g->add_zombie(snake);
        }
    }

    if( g->u_see( this->xpos(), this->ypos() ) ) {
        g->draw_hit_player(this);
    }

    if (has_trait("ADRENALINE") && !has_disease("adrenaline") &&
        (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
    add_disease("adrenaline", 200);

    int cut_dam = dealt_dams.type_damage(DT_CUT);
    switch (bp) {
    case bp_eyes:
        mod_pain(1);
        if (dam > 5 || cut_dam > 0) {
            int minblind = int((dam + cut_dam) / 10);
            if (minblind < 1)
                minblind = 1;
            int maxblind = int((dam + cut_dam) /  4);
            if (maxblind > 5)
                maxblind = 5;
            add_effect("blind", rng(minblind, maxblind));
        }
    case bp_mouth: // Fall through to head damage
    case bp_head:
        mod_pain(1);
        hp_cur[hp_head] -= dam;
        if (hp_cur[hp_head] < 0)
        {
            lifetime_stats()->damage_taken+=hp_cur[hp_head];
            hp_cur[hp_head] = 0;
        }
        break;
    case bp_torso:
        // getting hit throws off our shooting
        recoil += int(dam / 5);
        break;
    case bp_hands: // Fall through to arms
    case bp_arms:
        // getting hit in the arms throws off our shooting
        if (side == 1 || weapon.is_two_handed(this))
            recoil += int(dam / 3);
        break;
    case bp_feet: // Fall through to legs
    case bp_legs:
        break;
    default:
        debugmsg("Wacky body part hit!");
    }

    return dealt_damage_instance(dealt_dams);
}

void player::apply_damage(Creature* source, body_part bp, int side, int dam) {
    if (is_dead_state()) return; // don't do any more damage if we're already dead
    switch (bp) {
    case bp_eyes: // Fall through to head damage
    case bp_mouth: // Fall through to head damage
    case bp_head:
        hp_cur[hp_head] -= dam;
        if (hp_cur[hp_head] < 0)
        {
            lifetime_stats()->damage_taken+=hp_cur[hp_head];
            hp_cur[hp_head] = 0;
        }
        break;
    case bp_torso:
        hp_cur[hp_torso] -= dam;
        if (hp_cur[hp_torso] < 0)
        {
            lifetime_stats()->damage_taken+=hp_cur[hp_torso];
            hp_cur[hp_torso] = 0;
        }
        break;
    case bp_hands: // Fall through to arms
    case bp_arms:
        if (side == 0) {
            hp_cur[hp_arm_l] -= dam;
            if (hp_cur[hp_arm_l] < 0)
            {
                lifetime_stats()->damage_taken+=hp_cur[hp_arm_l];
                hp_cur[hp_arm_l] = 0;
            }
        }
        if (side == 1) {
            hp_cur[hp_arm_r] -= dam;
            if (hp_cur[hp_arm_r] < 0)
            {
                lifetime_stats()->damage_taken+=hp_cur[hp_arm_r];
                hp_cur[hp_arm_r] = 0;
            }
        }
        break;
    case bp_feet: // Fall through to legs
    case bp_legs:
        if (side == 0) {
            hp_cur[hp_leg_l] -= dam;
            if (hp_cur[hp_leg_l] < 0)
            {
                lifetime_stats()->damage_taken+=hp_cur[hp_leg_l];
                hp_cur[hp_leg_l] = 0;
            }
        }
        if (side == 1) {
            hp_cur[hp_leg_r] -= dam;
            if (hp_cur[hp_leg_r] < 0)
            {
                lifetime_stats()->damage_taken+=hp_cur[hp_leg_r];
                hp_cur[hp_leg_r] = 0;
            }
        }
        break;
    default:
        debugmsg("Wacky body part hurt!");
    }
    lifetime_stats()->damage_taken+=dam;

    if (is_dead_state())
        die(source);
}

void player::mod_pain(int npain) {
    if (has_trait("PAINRESIST") && npain > 1) // if it's 1 it'll just become 0, which is bad
        npain = npain * 4 / rng(4,8);
    Creature::mod_pain(npain);
}

void player::hurt(body_part, int, int dam)
{
    int painadd = 0;
    if (has_disease("sleep") && rng(0, dam) > 2) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

 if (dam <= 0)
  return;

 if (!is_npc())
  g->cancel_activity_query(_("You were hurt!"));

 if (has_trait("PAINRESIST"))
  painadd = dam / 3;
 else
  painadd = dam / 2;
 pain += painadd;


 if (has_trait("ADRENALINE") && !has_disease("adrenaline") &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease("adrenaline", 200);
  lifetime_stats()->damage_taken+=dam;
}

void player::hurt(hp_part hurt, int dam)
{
    int painadd = 0;
    if (has_disease("sleep") && rng(0, dam) > 2) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

    if (dam <= 0)
        return;

    if (!is_npc()) {
        g->cancel_activity_query(_("You were hurt!"));
    }

    if (has_trait("PAINRESIST")) {
        painadd = dam / 3;
    } else {
        painadd = dam / 2;
    }
        pain += painadd;

    hp_cur[hurt] -= dam;
    if (hp_cur[hurt] < 0) {
        lifetime_stats()->damage_taken+=hp_cur[hurt];
        hp_cur[hurt] = 0;
    }
    lifetime_stats()->damage_taken+=dam;
}

void player::heal(body_part healed, int side, int dam)
{
 hp_part healpart;
 switch (healed) {
 case bp_eyes: // Fall through to head damage
 case bp_mouth: // Fall through to head damage
 case bp_head:
  healpart = hp_head;
 break;
 case bp_torso:
  healpart = hp_torso;
 break;
 case bp_hands:
// Shouldn't happen, but fall through to arms
  debugmsg("Heal against hands!");
 case bp_arms:
  if (side == 0)
   healpart = hp_arm_l;
  else
   healpart = hp_arm_r;
 break;
 case bp_feet:
// Shouldn't happen, but fall through to legs
  debugmsg("Heal against feet!");
 case bp_legs:
  if (side == 0)
   healpart = hp_leg_l;
  else
   healpart = hp_leg_r;
 break;
 default:
  debugmsg("Wacky body part healed!");
  healpart = hp_torso;
 }
 hp_cur[healpart] += dam;
 if (hp_cur[healpart] > hp_max[healpart])
 {
  lifetime_stats()->damage_healed-=hp_cur[healpart]-hp_max[healpart];
  hp_cur[healpart] = hp_max[healpart];
 }
 lifetime_stats()->damage_healed+=dam;
}

void player::heal(hp_part healed, int dam)
{
 hp_cur[healed] += dam;
 if (hp_cur[healed] > hp_max[healed])
 {
  lifetime_stats()->damage_healed-=hp_cur[healed]-hp_max[healed];
  hp_cur[healed] = hp_max[healed];
 }
 lifetime_stats()->damage_healed+=dam;
}

void player::healall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  if (hp_cur[i] > 0) {
   hp_cur[i] += dam;
   if (hp_cur[i] > hp_max[i])
   {
    lifetime_stats()->damage_healed-=hp_cur[i]-hp_max[i];
    hp_cur[i] = hp_max[i];
   }
   lifetime_stats()->damage_healed+=dam;
  }
 }
}

void player::hurtall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  int painadd = 0;
  hp_cur[i] -= dam;
   if (hp_cur[i] < 0)
   {
     lifetime_stats()->damage_taken+=hp_cur[i];
     hp_cur[i] = 0;
   }
  if (has_trait("PAINRESIST"))
   painadd = dam / 3;
  else
   painadd = dam / 2;
  pain += painadd;
  lifetime_stats()->damage_taken+=dam;
 }
}

void player::hitall(int dam, int vary)
{
    if (has_disease("sleep")) {
        wake_up(_("You wake up!"));
    } else if (has_disease("lying_down")) {
        rem_disease("lying_down");
    }

 for (int i = 0; i < num_hp_parts; i++) {
  int ddam = vary? dam * rng (100 - vary, 100) / 100 : dam;
  int cut = 0;
  absorb((body_part) i, ddam, cut);
  int painadd = 0;
  hp_cur[i] -= ddam;
   if (hp_cur[i] < 0)
   {
     lifetime_stats()->damage_taken+=hp_cur[i];
     hp_cur[i] = 0;
   }
  if (has_trait("PAINRESIST"))
   painadd = dam / 3 / 4;
  else
   painadd = dam / 2 / 4;
  pain += painadd;
  lifetime_stats()->damage_taken+=dam;
 }
}

void player::knock_back_from(int x, int y)
{
 if (x == posx && y == posy)
  return; // No effect
 point to(posx, posy);
 if (x < posx)
  to.x++;
 if (x > posx)
  to.x--;
 if (y < posy)
  to.y++;
 if (y > posy)
  to.y--;

// First, see if we hit a monster
 int mondex = g->mon_at(to.x, to.y);
 if (mondex != -1) {
  monster *critter = &(g->zombie(mondex));
  hit(this, bp_torso, -1, critter->type->size, 0);
  add_effect("stunned", 1);
  if ((str_max - 6) / 4 > critter->type->size) {
   critter->knock_back_from(posx, posy); // Chain reaction!
   critter->hurt((str_max - 6) / 4);
   critter->add_effect("stunned", 1);
  } else if ((str_max - 6) / 4 == critter->type->size) {
   critter->hurt((str_max - 6) / 4);
   critter->add_effect("stunned", 1);
  }

  g->add_msg_player_or_npc( this, _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                            critter->name().c_str() );

  return;
 }

 int npcdex = g->npc_at(to.x, to.y);
 if (npcdex != -1) {
  npc *p = g->active_npc[npcdex];
  hit(this, bp_torso, -1, 3, 0);
  add_effect("stunned", 1);
  p->hit(this, bp_torso, -1, 3, 0);
  g->add_msg_player_or_npc( this, _("You bounce off %s!"), _("<npcname> bounces off %s!"), p->name.c_str() );
  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.move_cost(to.x, to.y) == 0) { // Wait, it's a wall (or water)

  if (g->m.has_flag("LIQUID", to.x, to.y)) {
   if (!is_npc()) {
    g->plswim(to.x, to.y);
   }
// TODO: NPCs can't swim!
  } else { // It's some kind of wall.
   hurt(bp_torso, -1, 3);
   add_effect("stunned", 2);
   g->add_msg_player_or_npc( this, _("You bounce off a %s!"), _("<npcname> bounces off a %s!"),
                             g->m.tername(to.x, to.y).c_str() );
  }

 } else { // It's no wall
  posx = to.x;
  posy = to.y;
 }
}

void player::bp_convert(hp_part &hpart, body_part bp, int side)
{
    hpart =  num_hp_parts;
    switch(bp) {
        case bp_head:
            hpart = hp_head;
            break;
        case bp_torso:
            hpart = hp_torso;
            break;
        case bp_arms:
            if (side == 0) {
                hpart = hp_arm_l;
            } else {
                hpart = hp_arm_r;
            }
            break;
        case bp_legs:
            if (side == 0) {
                hpart = hp_leg_l;
            } else {
                hpart = hp_leg_r;
            }
            break;
    }
}

void player::hp_convert(hp_part hpart, body_part &bp, int &side)
{
    bp =  num_bp;
    side = -1;
    switch(hpart) {
        case hp_head:
            bp = bp_head;
            break;
        case hp_torso:
            bp = bp_torso;
            break;
        case hp_arm_l:
            bp = bp_arms;
            side = 0;
            break;
        case hp_arm_r:
            bp = bp_arms;
            side = 1;
            break;
        case hp_leg_l:
            bp = bp_legs;
            side = 0;
            break;
        case hp_leg_r:
            bp = bp_legs;
            side = 1;
            break;
    }
}

int player::hp_percentage()
{
 int total_cur = 0, total_max = 0;
// Head and torso HP are weighted 3x and 2x, respectively
 total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
 total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
 for (int i = hp_arm_l; i < num_hp_parts; i++) {
  total_cur += hp_cur[i];
  total_max += hp_max[i];
 }
 return (100 * total_cur) / total_max;
}

void player::recalc_hp()
{
    int new_max_hp[num_hp_parts];
    for (int i = 0; i < num_hp_parts; i++)
    {
        new_max_hp[i] = 60 + str_max * 3;
        if (has_trait("TOUGH"))
        {
            new_max_hp[i] *= 1.2;
        }
        if (has_trait("FRAIL"))
        {
            new_max_hp[i] *= 0.25;
        }
    }
    if (has_trait("GLASSJAW"))
    {
        new_max_hp[hp_head] *= 0.8;
    }
    for (int i = 0; i < num_hp_parts; i++)
    {
        hp_cur[i] *= (float)new_max_hp[i]/(float)hp_max[i];
        hp_max[i] = new_max_hp[i];
    }
}

void player::get_sick()
{
 if (health > 0 && rng(0, health + 10) < health)
  health--;
 if (health < 0 && rng(0, 10 - health) < (0 - health))
  health++;
 if (one_in(12))
  health -= 1;

 if (g->debugmon)
  debugmsg("Health: %d", health);

 if (has_trait("DISIMMUNE"))
  return;

 if (!has_disease("flu") && !has_disease("common_cold") &&
     one_in(900 + 10 * health + (has_trait("DISRESISTANT") ? 300 : 0))) {
  if (one_in(6))
   infect("flu", bp_mouth, 3, rng(40000, 80000));
  else
   infect("common_cold", bp_mouth, 3, rng(20000, 60000));
 }
}

bool player::infect(dis_type type, body_part vector, int strength,
                     int duration, bool permanent, int intensity,
                     int max_intensity, int decay, int additive, bool targeted,
                     int side, bool main_parts_only)
{
    if (strength <= 0) {
        return false;
    }

    if (dice(strength, 3) > dice(get_env_resist(vector), 3)) {
        if (targeted) {
            add_disease(type, duration, permanent, intensity, max_intensity, decay,
                          additive, vector, side, main_parts_only);
        } else {
            add_disease(type, duration, permanent, intensity, max_intensity, decay, additive);
        }
        return true;
    }

    return false;
}

void player::add_disease(dis_type type, int duration, bool permanent,
                         int intensity, int max_intensity, int decay,
                         int additive, body_part part, int side,
                         bool main_parts_only)
{
    if (duration <= 0) {
        return;
    }

    if (part !=  num_bp && hp_cur[part] == 0) {
        return;
    }

    if (main_parts_only) {
        if (part == bp_eyes || part == bp_mouth) {
            part = bp_head;
        } else if (part == bp_hands) {
            part = bp_arms;
        } else if (part == bp_feet) {
            part = bp_legs;
        }
    }

    bool found = false;
    int i = 0;
    while ((i < illness.size()) && !found) {
        if (illness[i].type == type) {
            if ((part == num_bp) ^ (illness[i].bp == num_bp)) {
                debugmsg("Bodypart missmatch when applying disease %s",
                         type.c_str());
                return;
            } else if (illness[i].bp == part &&
                       ((illness[i].side == -1) ^ (side == -1))) {
                debugmsg("Side of body missmatch when applying disease %s",
                         type.c_str());
                return;
            }
            if (illness[i].bp == part && illness[i].side == side) {
                if (additive > 0) {
                    illness[i].duration += duration;
                } else if (additive < 0) {
                    illness[i].duration -= duration;
                    if (illness[i].duration <= 0) {
                        illness[i].duration = 1;
                    }
                }
                illness[i].intensity += intensity;
                if (max_intensity != -1 && illness[i].intensity > max_intensity) {
                    illness[i].intensity = max_intensity;
                }
                if (permanent) {
                    illness[i].permanent = true;
                }
                illness[i].decay = decay;
                found = true;
            }
        }
        i++;
    }
    if (!found) {
        if (!is_npc()) {
            dis_msg(type);
        }
        disease tmp(type, duration, intensity, part, side, permanent, decay);
        illness.push_back(tmp);
    }

    recalc_sight_limits();
}

void player::rem_disease(dis_type type, body_part part, int side)
{
    for (int i = 0; i < illness.size();) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
            illness.erase(illness.begin() + i);
            if(!is_npc()) {
                dis_remove_memorial(type);
            }
        } else {
            i++;
        }
    }

    recalc_sight_limits();
}

bool player::has_disease(dis_type type, body_part part, int side) const
{
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
            return true;
        }
    }
    return false;
}

bool player::pause_disease(dis_type type, body_part part, int side)
{
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
                illness[i].permanent = true;
                return true;
        }
    }
    return false;
}

bool player::unpause_disease(dis_type type, body_part part, int side)
{
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type &&
            ( part == num_bp || illness[i].bp == part ) &&
            ( side == -1 || illness[i].side == side ) ) {
                illness[i].permanent = false;
                return true;
        }
    }
    return false;
}

int player::disease_duration(dis_type type, bool all, body_part part, int side)
{
    int tmp = 0;
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type && (part ==  num_bp || illness[i].bp == part) &&
            (side == -1 || illness[i].side == side)) {
            if (all == false) {
                return illness[i].duration;
            } else {
                tmp += illness[i].duration;
            }
        }
    }
    return tmp;
}

int player::disease_intensity(dis_type type, bool all, body_part part, int side)
{
    int tmp = 0;
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == type && (part ==  num_bp || illness[i].bp == part) &&
            (side == -1 || illness[i].side == side)) {
            if (all == false) {
                return illness[i].intensity;
            } else {
                tmp += illness[i].intensity;
            }
        }
    }
    return tmp;
}

void player::add_addiction(add_type type, int strength)
{
 if (type == ADD_NULL)
  return;
 int timer = 1200;
 if (has_trait("ADDICTIVE")) {
  strength = int(strength * 1.5);
  timer = 800;
 }
 if (has_trait("NONADDICTIVE")) {
  strength = int(strength * .50);
  timer = 1800;
 }
 //Update existing addiction
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
   if (addictions[i].sated < 0) {
    addictions[i].sated = timer;
   } else if (addictions[i].sated < 600) {
    addictions[i].sated += timer; // TODO: Make this variable?
   } else {
    addictions[i].sated += int((3000 - addictions[i].sated) / 2);
   }
   if ((rng(0, strength) > rng(0, addictions[i].intensity * 5) ||
       rng(0, 500) < strength) && addictions[i].intensity < 20) {
    addictions[i].intensity++;
   }
   return;
  }
 }
 //Add a new addiction
 if (rng(0, 100) < strength) {
  add_memorial_log(_("Became addicted to %s."), addiction_type_name(type).c_str());
  addiction tmp(type, 1);
  addictions.push_back(tmp);
 }
}

bool player::has_addiction(add_type type) const
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type &&
      addictions[i].intensity >= MIN_ADDICTION_LEVEL)
   return true;
 }
 return false;
}

void player::rem_addiction(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
   add_memorial_log(_("Overcame addiction to %s."), addiction_type_name(type).c_str());
   addictions.erase(addictions.begin() + i);
   return;
  }
 }
}

int player::addiction_level(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type)
   return addictions[i].intensity;
 }
 return 0;
}

bool player::siphon(vehicle *veh, ammotype desired_liquid)
{
    int liquid_amount = veh->drain( desired_liquid, veh->fuel_capacity(desired_liquid) );
    item used_item( itypes[default_ammo(desired_liquid)], g->turn );
    used_item.charges = liquid_amount;
    int extra = g->move_liquid( used_item );
    if( extra == -1 ) {
        // Failed somehow, put the liquid back and bail out.
        veh->refill( desired_liquid, used_item.charges );
        return false;
    }
    int siphoned = liquid_amount - extra;
    veh->refill( desired_liquid, extra );
    if( siphoned > 0 ) {
        g->add_msg(_("Siphoned %d units of %s from the %s."),
                   siphoned, used_item.name.c_str(), veh->name.c_str());
        //Don't consume turns if we decided not to siphon
        return true;
    } else {
        return false;
    }
}

static void manage_fire_exposure(player &p, int fireStrength) {
    // TODO: this should be determined by material properties
    p.hurtall(3*fireStrength);
    for (int i = 0; i < p.worn.size(); i++) {
        item tmp = p.worn[i];
        bool burnVeggy = (tmp.made_of("veggy") || tmp.made_of("paper"));
        bool burnFabric = ((tmp.made_of("cotton") || tmp.made_of("wool")) && one_in(10*fireStrength));
        bool burnPlastic = ((tmp.made_of("plastic")) && one_in(50*fireStrength));
        if (burnVeggy || burnFabric || burnPlastic) {
            p.worn.erase(p.worn.begin() + i);
            i--;
        }
    }
}
static void handle_cough(player &p, int intensity, int loudness) {
    if (p.is_player()) {
        g->add_msg(_("You cough heavily."));
        g->sound(p.posx, p.posy, loudness, "");
    } else {
        g->sound(p.posx, p.posy, loudness, _("a hacking cough."));
    }
    p.mod_moves(-80);
    if (rng(1,6) < intensity) {
        p.apply_damage(NULL, bp_torso, -1, 1);
    }
    if (p.has_disease("sleep") && intensity >= 2) {
        p.wake_up(_("You wake up coughing."));
    }
}
void player::process_effects() {
    int psnChance;
    for (std::vector<effect>::iterator it = effects.begin();
            it != effects.end(); ++it) {
        std::string id = it->get_id();
        if (id == "onfire") {
            manage_fire_exposure(*this, 1);
        } else if (id == "poison") {
            psnChance = 150;
            if (has_trait("POISRESIST")) {
                psnChance *= 6;
            } else {
                mod_str_bonus(-2);
                mod_per_bonus(-1);
            }
            if (one_in(psnChance)) {
                g->add_msg_if_player(this,_("You're suddenly wracked with pain!"));
                mod_pain(1);
                hurt(bp_torso, -1, rng(0, 2) * rng(0, 1));
            }
            mod_per_bonus(-1);
            mod_dex_bonus(-1);
        } else if (id == "glare") {
            mod_per_bonus(-1);
            if (one_in(200)) {
                g->add_msg_if_player(this,_("The sunlight's glare makes it hard to see."));
            }
        } else if (id == "smoke") {
            // A hard limit on the duration of the smoke disease.
            if( it->get_duration() >= 600) {
                it->set_duration(600);
            }
            mod_str_bonus(-1);
            mod_dex_bonus(-1);
            it->set_intensity((it->get_duration()+190)/200);
            if (it->get_intensity() >= 10 && one_in(6)) {
                handle_cough(*this, it->get_intensity());
            }
        } else if (id == "teargas") {
            mod_str_bonus(-2);
            mod_dex_bonus(-2);
            mod_per_bonus(-5);
            if (one_in(3)) {
                handle_cough(*this, 4);
            }
        }
    }

    Creature::process_effects();
}

void player::suffer()
{
    for (int i = 0; i < my_bionics.size(); i++)
    {
        if (my_bionics[i].powered)
        {
            activate_bionic(i);
        }
    }
    if (underwater)
    {
        if (!has_trait("GILLS"))
        {
            oxygen--;
        }
        if (oxygen < 12 && worn_with_flag("REBREATHER") &&
            (has_active_item("UPS_on") || has_active_item("adv_UPS_on")))
            {
                oxygen += 12;
            }
        if (oxygen < 0)
        {
            if (has_bionic("bio_gills") && power_level > 0)
            {
                oxygen += 5;
                power_level--;
            }
            else
            {
                g->add_msg(_("You're drowning!"));
                hurt(bp_torso, -1, rng(1, 4));
            }
        }
    }

    for (int i = 0; i < illness.size(); i++) {
        dis_effect(*this, illness[i]);
    }

    // Diseases may remove themselves as part of applying (MA buffs do) so do a
    // separate loop through the remaining ones for duration, decay, etc..
    for (int i = 0; i < illness.size(); i++) {
        if (!illness[i].permanent) {
            illness[i].duration--;
        }
        if (illness[i].decay > 0 && one_in(illness[i].decay)) {
            illness[i].intensity--;
        }
        if (illness[i].duration <= 0 || illness[i].intensity == 0) {
            dis_end_msg(*this, illness[i]);
            illness.erase(illness.begin() + i);
            i--;
        }
    }

    if (!has_disease("sleep"))
    {
        if (weight_carried() > weight_capacity())
        {
            // Starts at 1 in 25, goes down by 5 for every 50% more carried
            if (one_in(35 - 5 * weight_carried() / (weight_capacity() / 2))){
                g->add_msg_if_player(this, _("Your body strains under the weight!"));
                // 1 more pain for every 800 grams more (5 per extra STR needed)
                if ( (weight_carried() - weight_capacity()) / 800 > pain && pain < 100) {
                    pain += 1;
                }
            }
        }
        if (weight_carried() > 4 * weight_capacity()) {
            if (has_effect("downed")) {
                add_effect("downed", 1);
            } else {
                add_effect("downed", 2);
            }
        }
        int timer = -3600;
        if (has_trait("ADDICTIVE"))
        {
            timer = -4000;
        }
        if (has_trait("NONADDICTIVE"))
        {
            timer = -3200;
        }
        for (int i = 0; i < addictions.size(); i++)
        {
            if (addictions[i].sated <= 0 &&
                addictions[i].intensity >= MIN_ADDICTION_LEVEL)
            {
                addict_effect(addictions[i]);
            }
            addictions[i].sated--;
            if (!one_in(addictions[i].intensity - 2) && addictions[i].sated > 0)
            {
                addictions[i].sated -= 1;
            }
            if (addictions[i].sated < timer - (100 * addictions[i].intensity))
            {
                if (addictions[i].intensity <= 2)
                {
                    addictions.erase(addictions.begin() + i);
                    i--;
                }
                else
                {
                    addictions[i].intensity = int(addictions[i].intensity / 2);
                    addictions[i].intensity--;
                    addictions[i].sated = 0;
                }
            }
        }
        if (has_trait("CHEMIMBALANCE"))
        {
            if (one_in(3600))
            {
                g->add_msg(_("You suddenly feel sharp pain for no reason."));
                pain += 3 * rng(1, 3);
            }
            if (one_in(3600))
            {
                int pkilladd = 5 * rng(-1, 2);
                if (pkilladd > 0)
                {
                    g->add_msg(_("You suddenly feel numb."));
                }
                else if (pkilladd < 0)
                {
                    g->add_msg(_("You suddenly ache."));
                }
                pkill += pkilladd;
            }
            if (one_in(3600))
            {
                g->add_msg(_("You feel dizzy for a moment."));
                moves -= rng(10, 30);
            }
            if (one_in(3600))
            {
                int hungadd = 5 * rng(-1, 3);
                if (hungadd > 0)
                {
                    g->add_msg(_("You suddenly feel hungry."));
                }
                else
                {
                    g->add_msg(_("You suddenly feel a little full."));
                }
                hunger += hungadd;
            }
            if (one_in(3600))
            {
                g->add_msg(_("You suddenly feel thirsty."));
                thirst += 5 * rng(1, 3);
            }
            if (one_in(3600))
            {
                g->add_msg(_("You feel fatigued all of a sudden."));
                fatigue += 10 * rng(2, 4);
            }
            if (one_in(4800))
            {
                if (one_in(3))
                {
                    add_morale(MORALE_FEELING_GOOD, 20, 100);
                }
                else
                {
                    add_morale(MORALE_FEELING_BAD, -20, -100);
                }
            }
            if (one_in(3600))
            {
                if (one_in(3))
                {
                    g->add_msg(_("You suddenly feel very cold."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_VERY_COLD;
                    }
                }
                else
                {
                    g->add_msg(_("You suddenly feel cold."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_COLD;
                    }
                }
            }
            if (one_in(3600))
            {
                if (one_in(3))
                {
                    g->add_msg(_("You suddenly feel very hot."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_VERY_HOT;
                    }
                }
                else
                {
                    g->add_msg(_("You suddenly feel hot."));
                    for (int i = 0 ; i < num_bp ; i++)
                    {
                        temp_cur[i] = BODYTEMP_HOT;
                    }
                }
            }
        }
        if ((has_trait("SCHIZOPHRENIC") || has_artifact_with(AEP_SCHIZO)) &&
            one_in(2400))
        { // Every 4 hours or so
            monster phantasm;
            int i;
            switch(rng(0, 11))
            {
                case 0:
                    add_disease("hallu", 3600);
                    break;
                case 1:
                    add_disease("visuals", rng(15, 60));
                    break;
                case 2:
                    g->add_msg(_("From the south you hear glass breaking."));
                    break;
                case 3:
                    g->add_msg(_("YOU SHOULD QUIT THE GAME IMMEDIATELY."));
                    add_morale(MORALE_FEELING_BAD, -50, -150);
                    break;
                case 4:
                    for (i = 0; i < 10; i++) {
                        g->add_msg("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
                    }
                    break;
                case 5:
                    g->add_msg(_("You suddenly feel so numb..."));
                    pkill += 25;
                    break;
                case 6:
                    g->add_msg(_("You start to shake uncontrollably."));
                    add_disease("shakes", 10 * rng(2, 5));
                    break;
                case 7:
                    for (i = 0; i < 10; i++)
                    {
                        g->spawn_hallucination();
                    }
                    break;
                case 8:
                    g->add_msg(_("It's a good time to lie down and sleep."));
                    add_disease("lying_down", 200);
                    break;
                case 9:
                    g->add_msg(_("You have the sudden urge to SCREAM!"));
                    g->sound(posx, posy, 10 + 2 * str_cur, "AHHHHHHH!");
                    break;
                case 10:
                    g->add_msg(std::string(name + name + name + name + name + name + name +
                        name + name + name + name + name + name + name +
                        name + name + name + name + name + name).c_str());
                    break;
                case 11:
                    body_part bp = random_body_part(true);
                    int side = random_side(bp);
                    add_disease("formication", 600, false, 1, 3, 0, 1, bp, side, true);
                    break;
            }
        }
  if (has_trait("JITTERY") && !has_disease("shakes")) {
   if (stim > 50 && one_in(300 - stim))
    add_disease("shakes", 300 + stim);
   else if (hunger > 80 && one_in(500 - hunger))
    add_disease("shakes", 400);
  }

  if (has_trait("MOODSWINGS") && one_in(3600)) {
   if (rng(1, 20) > 9) // 55% chance
    add_morale(MORALE_MOODSWING, -100, -500);
   else   // 45% chance
    add_morale(MORALE_MOODSWING, 100, 500);
  }

  if (has_trait("VOMITOUS") && one_in(4200))
   vomit();

  if (has_trait("SHOUT1") && one_in(3600))
   g->sound(posx, posy, 10 + 2 * str_cur, _("You shout loudly!"));
  if (has_trait("SHOUT2") && one_in(2400))
   g->sound(posx, posy, 15 + 3 * str_cur, _("You scream loudly!"));
  if (has_trait("SHOUT3") && one_in(1800))
   g->sound(posx, posy, 20 + 4 * str_cur, _("You let out a piercing howl!"));
 } // Done with while-awake-only effects

 if (has_trait("ASTHMA") && one_in(3600 - stim * 50)) {
  bool auto_use = has_charges("inhaler", 1);
  if (underwater) {
   oxygen = int(oxygen / 2);
   auto_use = false;
  }

        if (has_disease("sleep")) {
            wake_up(_("Your asthma wakes you up!"));
            auto_use = false;
        }

  if (auto_use)
   use_charges("inhaler", 1);
  else {
   add_disease("asthma", 50 * rng(1, 4));
   if (!is_npc())
    g->cancel_activity_query(_("You have an asthma attack!"));
  }
 }

 if (has_trait("LEAVES") && g->is_in_sunlight(posx, posy) && one_in(600))
  hunger--;

 if (pain > 0) {
  if (has_trait("PAINREC1") && one_in(600))
   pain--;
  if (has_trait("PAINREC2") && one_in(300))
   pain--;
  if (has_trait("PAINREC3") && one_in(150))
   pain--;
 }

    if (has_trait("ALBINO") && g->is_in_sunlight(posx, posy) && one_in(20)) {
        g->add_msg(_("The sunlight burns your skin!"));
        if (has_disease("sleep")) {
            wake_up(_("You wake up!"));
        }
        hurtall(1);
    }

 if ((has_trait("TROGLO") || has_trait("TROGLO2")) &&
     g->is_in_sunlight(posx, posy) && g->weather == WEATHER_SUNNY) {
  str_cur--;
  dex_cur--;
  int_cur--;
  per_cur--;
 }
 if (has_trait("TROGLO2") && g->is_in_sunlight(posx, posy)) {
  str_cur--;
  dex_cur--;
  int_cur--;
  per_cur--;
 }
 if (has_trait("TROGLO3") && g->is_in_sunlight(posx, posy)) {
  str_cur -= 4;
  dex_cur -= 4;
  int_cur -= 4;
  per_cur -= 4;
 }

 if (has_trait("SORES")) {
  for (int i = bp_head; i < num_bp; i++) {
   if (pain < 5 + 4 * abs(encumb(body_part(i))))
    pain = 5 + 4 * abs(encumb(body_part(i)));
  }
 }

 if (has_trait("SLIMY") && !in_vehicle) {
   g->m.add_field(posx, posy, fd_slime, 1);
 }

 if (has_trait("WEB_SPINNER") && !in_vehicle && one_in(3)) {
   g->m.add_field(posx, posy, fd_web, 1); //this adds density to if its not already there.
 }

 if (has_trait("RADIOGENIC") && int(g->turn) % 50 == 0 && radiation >= 10) {
  radiation -= 10;
  healall(1);
 }

 if (has_trait("RADIOACTIVE1")) {
  if (g->m.radiation(posx, posy) < 10 && one_in(50))
   g->m.radiation(posx, posy)++;
 }
 if (has_trait("RADIOACTIVE2")) {
  if (g->m.radiation(posx, posy) < 20 && one_in(25))
   g->m.radiation(posx, posy)++;
 }
 if (has_trait("RADIOACTIVE3")) {
  if (g->m.radiation(posx, posy) < 30 && one_in(10))
   g->m.radiation(posx, posy)++;
 }

 if (has_trait("UNSTABLE") && one_in(28800)) // Average once per 2 days
  mutate();
 if (has_artifact_with(AEP_MUTAGENIC) && one_in(28800))
  mutate();
 if (has_artifact_with(AEP_FORCE_TELEPORT) && one_in(600))
  g->teleport(this);

 int localRadiation = g->m.radiation(posx, posy);

 if (localRadiation) {
   bool has_helmet = false;

   bool power_armored = is_wearing_power_armor(&has_helmet);

   if ((power_armored && has_helmet) || is_wearing("hazmat_suit")|| is_wearing("anbc_suit")) {
     radiation += 0; // Power armor protects completely from radiation
   } else if (power_armored || is_wearing("cleansuit")|| is_wearing("aep_suit")) {
     radiation += rng(0, localRadiation / 40);
   } else {
     radiation += rng(0, localRadiation / 16);
   }

   // Apply rads to any radiation badges.
   std::vector<item *> possessions = inv_dump();
   for( std::vector<item *>::iterator it = possessions.begin(); it != possessions.end(); ++it ) {
       if( (*it)->type->id == "rad_badge" ) {
           // Actual irridation levels of badges and the player aren't precisely matched.
           // This is intentional.
           int before = (*it)->irridation;
           (*it)->irridation += rng(0, localRadiation / 16);
           if( inv.has_item(*it) ) { continue; }
           for( int i = 0; i < sizeof(rad_dosage_thresholds)/sizeof(rad_dosage_thresholds[0]); i++ ){
               if( before < rad_dosage_thresholds[i] &&
                   (*it)->irridation >= rad_dosage_thresholds[i] ) {
                   g->add_msg_if_player( this, _("Your radiation badge changes from %s to %s!"),
                                         rad_threshold_colors[i - 1].c_str(),
                                         rad_threshold_colors[i].c_str() );
               }
           }
       }
   }
 }

 if( int(g->turn) % 150 == 0 )
 {
     if (radiation < 0) radiation = 0;
     else if (radiation > 2000) radiation = 2000;
     if (OPTIONS["RAD_MUTATION"] && rng(60, 2500) < radiation)
     {
         mutate();
         radiation /= 2;
         radiation -= 5;
     }
     else if (radiation > 100 && rng(1, 1500) < radiation)
     {
         vomit();
         radiation -= 50;
     }
 }

 if( radiation > 150 && !(int(g->turn) % 90) )
 {
     hurtall(radiation / 100);
 }

// Negative bionics effects
 if (has_bionic("bio_dis_shock") && one_in(1200)) {
  g->add_msg(_("You suffer a painful electrical discharge!"));
  pain++;
  moves -= 150;
 }
 if (has_bionic("bio_dis_acid") && one_in(1500)) {
  g->add_msg(_("You suffer a burning acidic discharge!"));
  hurtall(1);
 }
 if (has_bionic("bio_drain") && power_level > 0 && one_in(600)) {
  g->add_msg(_("Your batteries discharge slightly."));
  power_level--;
 }
 if (has_bionic("bio_noise") && one_in(500)) {
  g->add_msg(_("A bionic emits a crackle of noise!"));
  g->sound(posx, posy, 60, "");
 }
 if (has_bionic("bio_power_weakness") && max_power_level > 0 &&
     power_level >= max_power_level * .75)
  str_cur -= 3;

// Artifact effects
 if (has_artifact_with(AEP_ATTENTION))
  add_disease("attention", 3);

 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;

 // check for limb mending every 1000 turns (~1.6 hours)
 if(g->turn.get_turn() % 1000 == 0) {
  mend();
 }
}

void player::mend()
{
 // Wearing splints can slowly mend a broken limb back to 1 hp.
 // 2 weeks is faster than a fracture would heal IRL,
 // but 3 weeks average (a generous estimate) was tedious and no fun.
 for(int i = 0; i < num_hp_parts; i++) {
  int broken = (hp_cur[i] <= 0);
  if(broken) {
   double mending_odds = 200.0; // 2 weeks, on average. (~20160 minutes / 100 minutes)
   double healing_factor = 1.0;
   // Studies have shown that alcohol and tobacco use delay fracture healing time
   if(has_disease("cig") | addiction_level(ADD_CIG)) {
    healing_factor *= 0.5;
   }
   if(has_disease("drunk") | addiction_level(ADD_ALCOHOL)) {
    healing_factor *= 0.5;
   }

   // Bed rest speeds up mending
   if(has_disease("sleep")) {
    healing_factor *= 4.0;
   } else if(fatigue > 383) {
    // but being dead tired does not...
    healing_factor *= 0.75;
   }

   // Being healthy helps.
   if(health > 0) {
    healing_factor *= 2.0;
   }

   // And being well fed...
   if(hunger < 0) {
    healing_factor *= 2.0;
   }

   if(thirst < 0) {
    healing_factor *= 2.0;
   }

   // Mutagenic healing factor!
   if(has_trait("REGEN")) {
    healing_factor *= 16.0;
   } else if (has_trait("FASTHEALER2")) {
    healing_factor *= 4.0;
   } else if (has_trait("FASTHEALER")) {
    healing_factor *= 2.0;
   } else if (has_trait("SLOWHEALER")) {
    healing_factor *= 0.5;
   }

   bool mended = false;
   int side = 0;
   body_part part;
   switch(i) {
    case hp_arm_r:
     side = 1;
     // fall-through
    case hp_arm_l:
     part = bp_arms;
     mended = is_wearing("arm_splint") && x_in_y(healing_factor, mending_odds);
     break;
    case hp_leg_r:
     side = 1;
     // fall-through
    case hp_leg_l:
     part = bp_legs;
     mended = is_wearing("leg_splint") && x_in_y(healing_factor, mending_odds);
     break;
    default:
     // No mending for you!
     break;
   }
   if(mended) {
    hp_cur[i] = 1;
    add_memorial_log(_("Broken %s began to mend."), body_part_name(part, side).c_str());
    g->add_msg(_("Your %s has started to mend!"),
      body_part_name(part, side).c_str());
   }
  }
 }
}

void player::vomit()
{
    add_memorial_log(_("Threw up."));
    g->add_msg(_("You throw up heavily!"));
    int nut_loss = 100 / (1 + exp(.15 * (hunger / 100)));
    int quench_loss = 100 / (1 + exp(.025 * (thirst / 10)));
    hunger += rng(nut_loss / 2, nut_loss);
    thirst += rng(quench_loss / 2, quench_loss);
    moves -= 100;
    for (int i = 0; i < illness.size(); i++) {
        if (illness[i].type == "foodpoison") {
            illness[i].duration -= 300;
            if (illness[i].duration < 0) {
                rem_disease(illness[i].type);
            }
        } else if (illness[i].type == "drunk") {
            illness[i].duration -= rng(1, 5) * 100;
            if (illness[i].duration < 0) {
                rem_disease(illness[i].type);
            }
        }
    }
    rem_disease("pkill1");
    rem_disease("pkill2");
    rem_disease("pkill3");
    rem_disease("sleep");
}

void player::drench(int saturation, int flags)
{
    if (is_waterproof(flags)) {
        return;
    }

    int effected = 0;
    int tot_ignored = 0; //Always ignored
    int tot_neut = 0; //Ignored for good wet bonus
    int tot_good = 0; //Increase good wet bonus

    for (std::map<int, int>::iterator iter = mDrenchEffect.begin(); iter != mDrenchEffect.end(); ++iter) {
        if (mfb(iter->first) & flags) {
            effected += iter->second;
            tot_ignored += mMutDrench[iter->first]["ignored"];
            tot_neut += mMutDrench[iter->first]["neutral"];
            tot_good += mMutDrench[iter->first]["good"];
        }
    }

    if (effected == 0) {
        return;
    }

    bool wants_drench = false;
    // If not protected by mutations then check clothing
    if (tot_good + tot_neut + tot_ignored < effected) {
        wants_drench = is_water_friendly(flags);
    } else {
        wants_drench = true;
    }

    int morale_cap;
    if (wants_drench) {
        morale_cap = g->get_temperature() - std::min(65, 65 + (tot_ignored - tot_good) / 2) * saturation / 100;
    } else {
        morale_cap = -(saturation / 2);
    }

    // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
    if (morale_cap > 0) {
        morale_cap = morale_cap * (effected - tot_ignored + tot_good) / effected;
    } else if (morale_cap < 0) {
        morale_cap = morale_cap * (effected - tot_ignored - tot_neut - tot_good) / effected;
    }

    if (morale_cap == 0) {
        return;
    }

    int morale_effect = morale_cap / 8;
    if (morale_effect == 0) {
        if (morale_cap > 0) {
            morale_effect = 1;
        } else {
            morale_effect = -1;
        }
    }

    int dur = 60;
    int d_start = 30;
    if (morale_cap < 0) {
        if (has_trait("LIGHTFUR") || has_trait("FUR") || has_trait("FELINE_FUR") || has_trait("LUPINE_FUR")) {
            dur /= 5;
            d_start /= 5;
        }
    } else {
        if (has_trait("SLIMY")) {
            dur *= 1.2;
            d_start *= 1.2;
        }
    }

    add_morale(MORALE_WET, morale_effect, morale_cap, dur, d_start);
}

void player::drench_mut_calc()
{
    mMutDrench.clear();
    int ignored, neutral, good;

    for (std::map<int, int>::iterator it = mDrenchEffect.begin(); it != mDrenchEffect.end(); ++it) {
        ignored = 0;
        neutral = 0;
        good = 0;

        for (std::set<std::string>::iterator iter = my_mutations.begin(); iter != my_mutations.end(); ++iter) {
            for (std::map<std::string,mutation_wet>::iterator wp_iter = mutation_data[*iter].protection.begin(); wp_iter != mutation_data[*iter].protection.end(); ++wp_iter) {
                if (body_parts[wp_iter->first] == it->first) {
                    ignored += wp_iter->second.second.x;
                    neutral += wp_iter->second.second.y;
                    good += wp_iter->second.second.z;
                }
            }
        }

        mMutDrench[it->first]["good"] = good;
        mMutDrench[it->first]["neutral"] = neutral;
        mMutDrench[it->first]["ignored"] = ignored;
    }
}

int player::weight_carried()
{
    int ret = 0;
    ret += weapon.weight();
    for (int i = 0; i < worn.size(); i++)
    {
        ret += worn[i].weight();
    }
    ret += inv.weight();
    return ret;
}

int player::volume_carried()
{
    return inv.volume();
}

int player::weight_capacity(bool real_life)
{
 int str = (real_life ? str_cur : str_max);
 int ret = 13000 + str * 4000;
 if (has_trait("BADBACK"))
  ret = int(ret * .65);
 if (has_trait("STRONGBACK"))
  ret = int(ret * 1.35);
 if (has_trait("LIGHT_BONES"))
  ret = int(ret * .80);
 if (has_trait("HOLLOW_BONES"))
  ret = int(ret * .60);
 if (has_artifact_with(AEP_CARRY_MORE))
  ret += 22500;
 return ret;
}

int player::volume_capacity()
{
 int ret = 2; // A small bonus (the overflow)
 it_armor *armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  ret += armor->storage;
 }
 if (has_bionic("bio_storage"))
  ret += 6;
 if (has_trait("SHELL"))
  ret += 16;
 if (has_trait("PACKMULE"))
  ret = int(ret * 1.4);
 if (has_trait("DISORGANIZED"))
  ret = int(ret * 0.6);
 return ret;
}

double player::convert_weight(int weight)
{
    double ret;
    ret = double(weight);
    if (OPTIONS["USE_METRIC_WEIGHTS"] == "kg") {
        ret /= 1000;
    } else {
        ret /= 453.6;
    }
    return ret;
}

bool player::can_pickVolume(int volume)
{
    return (volume_carried() + volume <= volume_capacity());
}
bool player::can_pickWeight(int weight, bool safe)
{
    if (!safe)
    {
        //Player can carry up to four times their maximum weight
        return (weight_carried() + weight <= weight_capacity() * 4);
    }
    else
    {
        return (weight_carried() + weight <= weight_capacity());
    }
}

int player::net_morale(morale_point effect)
{
    double bonus = effect.bonus;

    // If the effect is old enough to have started decaying,
    // reduce it appropriately.
    if (effect.age > effect.decay_start)
    {
        bonus *= logistic_range(effect.decay_start,
                                effect.duration, effect.age);
    }

    // Optimistic characters focus on the good things in life,
    // and downplay the bad things.
    if (has_trait("OPTIMISTIC"))
    {
        if (bonus >= 0)
        {
            bonus *= 1.25;
        }
        else
        {
            bonus *= 0.75;
        }
    }

     // Again, those grouchy Bad-Tempered folks always focus on the negative.
     // They can't handle positive things as well.  They're No Fun.  D:
    if (has_trait("BADTEMPER"))
    {
        if (bonus < 0)
        {
            bonus *= 1.25;
        }
        else
        {
            bonus *= 0.75;
        }
    }
    return bonus;
}

int player::morale_level()
{
    // Add up all of the morale bonuses (and penalties).
    int ret = 0;
    for (int i = 0; i < morale.size(); i++)
    {
        ret += net_morale(morale[i]);
    }

    // Prozac reduces negative morale by 75%.
    if (has_disease("took_prozac") && ret < 0)
    {
        ret = int(ret / 4);
    }

    return ret;
}

void player::add_morale(morale_type type, int bonus, int max_bonus,
                        int duration, int decay_start,
                        bool cap_existing, itype* item_type)
{
    bool placed = false;

    // Search for a matching morale entry.
    for (int i = 0; i < morale.size() && !placed; i++)
    {
        if (morale[i].type == type && morale[i].item_type == item_type)
        {
            // Found a match!
            placed = true;

            // Scale the morale bonus to its current level.
            if (morale[i].age > morale[i].decay_start)
            {
                morale[i].bonus *= logistic_range(morale[i].decay_start,
                                                  morale[i].duration, morale[i].age);
            }

            // If we're capping the existing effect, we can use the new duration
            // and decay start.
            if (cap_existing)
            {
                morale[i].duration = duration;
                morale[i].decay_start = decay_start;
            }
            else
            {
                // Otherwise, we need to figure out whether the existing effect had
                // more remaining duration and decay-resistance than the new one does.
                // Only takes the new duration if new bonus and old are the same sign.
                if (morale[i].duration - morale[i].age <= duration &&
                   ((morale[i].bonus > 0) == (max_bonus > 0)) )
                {
                    morale[i].duration = duration;
                }
                else
                {
                    // This will give a longer duration than above.
                    morale[i].duration -= morale[i].age;
                }

                if (morale[i].decay_start - morale[i].age <= decay_start &&
                   ((morale[i].bonus > 0) == (max_bonus > 0)) )
                {
                    morale[i].decay_start = decay_start;
                }
                else
                {
                    // This will give a later decay start than above.
                    morale[i].decay_start -= morale[i].age;
                }
            }

            // Now that we've finished using it, reset the age to 0.
            morale[i].age = 0;

            // Is the current morale level for this entry below its cap, if any?
            if (abs(morale[i].bonus) < abs(max_bonus) || max_bonus == 0)
            {
                // Add the requested morale boost.
                morale[i].bonus += bonus;

                // If we passed the cap, pull back to it.
                if (abs(morale[i].bonus) > abs(max_bonus) && max_bonus != 0)
                {
                    morale[i].bonus = max_bonus;
                }
            }
            else if (cap_existing)
            {
                // The existing bonus is above the new cap.  Reduce it.
                morale[i].bonus = max_bonus;
            }
        }
    }

    // No matching entry, so add a new one
    if (!placed)
    {
        morale_point tmp(type, item_type, bonus, duration, decay_start, 0);
        morale.push_back(tmp);
    }
}

int player::has_morale( morale_type type ) const
{
    for( int i = 0; i < morale.size(); i++ ) {
        if( morale[i].type == type ) {
            return morale[i].bonus;
        }
    }
    return 0;
}

void player::rem_morale(morale_type type, itype* item_type)
{
 for (int i = 0; i < morale.size(); i++) {
  if (morale[i].type == type && morale[i].item_type == item_type) {
    morale.erase(morale.begin() + i);
    break;
  }
 }
}

item& player::i_add(item it)
{
 itype_id item_type_id = "null";
 if( it.type ) item_type_id = it.type->id;

 last_item = item_type_id;

 if (it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() ||
     it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container())
  inv.unsort();

 if (g != NULL && it.is_artifact() && it.is_tool()) {
  it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(it.type);
  g->add_artifact_messages(art->effects_carried);
 }
 return inv.add_item(it);
}

bool player::has_active_item(const itype_id & id) const
{
    if (weapon.type->id == id && weapon.active)
    {
        return true;
    }
    return inv.has_active_item(id);
}

int player::active_item_charges(itype_id id)
{
    int max = 0;
    if (weapon.type->id == id && weapon.active)
    {
        max = weapon.charges;
    }

    int inv_max = inv.max_active_item_charges(id);
    if (inv_max > max)
    {
        max = inv_max;
    }
    return max;
}

void player::process_active_items()
{
    if (weapon.is_artifact() && weapon.is_tool()) {
        g->process_artifact(&weapon, this, true);
    } else if (weapon.active) {
        if (weapon.has_flag("CHARGE")) {
            if (weapon.charges == 8) { // Maintaining charge takes less power.
                if( use_charges_if_avail("adv_UPS_on", 2) || use_charges_if_avail("UPS_on", 4) ) {
                    weapon.poison++;
                } else {
                    weapon.poison--;
                }
                if ( (weapon.poison >= 3) && (one_in(20)) ) { // 3 turns leeway, then it may discharge.
                    add_memorial_log(_("Accidental discharge of %s."), weapon.tname().c_str());
                    g->add_msg(_("Your %s discharges!"), weapon.tname().c_str());
                    point target(posx + rng(-12, 12), posy + rng(-12, 12));
                    std::vector<point> traj = line_to(posx, posy, target.x, target.y, 0);
                    g->fire(*this, target.x, target.y, traj, false);
                } else {
                    g->add_msg(_("Your %s beeps alarmingly."), weapon.tname().c_str());
                }
            } else { // We're chargin it up!
                if ( use_charges_if_avail("adv_UPS_on", ceil(static_cast<float>(1 + weapon.charges) / 2)) ||
                     use_charges_if_avail("UPS_on", 1 + weapon.charges) ) {
                    weapon.poison++;
                } else {
                    weapon.poison--;
                }

                if (weapon.poison >= weapon.charges) {
                    weapon.charges++;
                    weapon.poison = 0;
                }
            }
            if (weapon.poison < 0) {
                g->add_msg(_("Your %s spins down."), weapon.tname().c_str());
                weapon.charges--;
                weapon.poison = weapon.charges - 1;
            }
            if (weapon.charges <= 0) {
                weapon.active = false;
            }
        }
        else if (!process_single_active_item(&weapon)) {
            weapon = ret_null;
        }
    }

    std::vector<item *> inv_active = inv.active_items();
    for (std::vector<item *>::iterator iter = inv_active.begin(); iter != inv_active.end(); ++iter) {
        item *tmp_it = *iter;
        if (tmp_it->is_artifact() && tmp_it->is_tool()) {
            g->process_artifact(tmp_it, this);
        }
        if (!process_single_active_item(tmp_it)) {
            inv.remove_item(tmp_it);
        }
    }

    // worn items
    for (int i = 0; i < worn.size(); i++) {
        if (worn[i].is_artifact()) {
            g->process_artifact(&(worn[i]), this);
        }
    }

  // Drain UPS if using optical cloak.
  // TODO: Move somewhere else.
  if ((has_active_item("UPS_on") || has_active_item("adv_UPS_on"))
      && is_wearing("optical_cloak")) {
    // Drain UPS.
    if (has_charges("adv_UPS_on", 24)) {
      use_charges("adv_UPS_on", 24);
      if (charges_of("adv_UPS_on") < 120 && one_in(3))
        g->add_msg_if_player(this, _("Your optical cloak flickers for a moment!"));
    } else if (has_charges("UPS_on", 40)) {
      use_charges("UPS_on", 40);
      if (charges_of("UPS_on") < 200 && one_in(3))
        g->add_msg_if_player(this, _("Your optical cloak flickers for a moment!"));
    } else {
      if (has_charges("adv_UPS_on", charges_of("adv_UPS_on"))) {
          // Drain last power.
          use_charges("adv_UPS_on", charges_of("adv_UPS_on"));
      }
      else {
        use_charges("UPS_on", charges_of("UPS_on"));
      }
    }
  }
}

// returns false if the item needs to be removed
bool player::process_single_active_item(item *it)
{
    if (it->active ||
        (it->is_container() && it->contents.size() > 0 && it->contents[0].active))
    {
        if (it->is_food())
        {
            if (it->has_flag("HOT"))
            {
                it->item_counter--;
                if (it->item_counter == 0)
                {
                    it->item_tags.erase("HOT");
                    it->active = false;
                }
            }
        }
        else if (it->is_food_container())
        {
            if (it->contents[0].has_flag("HOT"))
            {
                it->contents[0].item_counter--;
                if (it->contents[0].item_counter == 0)
                {
                    it->contents[0].item_tags.erase("HOT");
                    it->contents[0].active = false;
                }
            }
        }
        else if (it->is_tool())
        {
            it_tool* tmp = dynamic_cast<it_tool*>(it->type);
            tmp->use.call(this, it, true);
            if (tmp->turns_per_charge > 0 && int(g->turn) % tmp->turns_per_charge == 0)
            {
                it->charges--;
            }
            if (it->charges <= 0)
            {
                tmp->use.call(this, it, false);
                if (tmp->revert_to == "null")
                {
                    return false;
                }
                else
                {
                    it->type = itypes[tmp->revert_to];
                    it->active = false;
                }
            }
        }
        else if (it->type->id == "corpse")
        {
            if (it->ready_to_revive())
            {
                add_memorial_log(_("Had a %s revive while carrying it."), it->name.c_str());
                g->add_msg_if_player(this, _("Oh dear god, a corpse you're carrying has started moving!"));
                g->revive_corpse(posx, posy, it);
                return false;
            }
        }
        else
        {
            debugmsg("%s is active, but has no known active function.", it->tname().c_str());
        }
    }
    return true;
}

item player::remove_weapon()
{
 if (weapon.has_flag("CHARGE") && weapon.active) { //unwield a charged charge rifle.
  weapon.charges = 0;
  weapon.active = false;
 }
 item tmp = weapon;
 weapon = ret_null;
// We need to remove any boosts related to our style
 rem_disease("attack_boost");
 rem_disease("dodge_boost");
 rem_disease("damage_boost");
 rem_disease("speed_boost");
 rem_disease("armor_boost");
 rem_disease("viper_combo");
 return tmp;
}

void player::remove_mission_items(int mission_id)
{
 if (mission_id == -1)
  return;
 if (weapon.mission_id == mission_id) {
  remove_weapon();
 } else {
  for (int i = 0; i < weapon.contents.size(); i++) {
   if (weapon.contents[i].mission_id == mission_id)
    remove_weapon();
  }
 }
 inv.remove_mission_items(mission_id);
}

item player::reduce_charges(int position, int quantity) {
    if (position == -1) {
        if (!weapon.count_by_charges())
        {
            debugmsg("Tried to remove %s by charges, but item is not counted by charges",
                    weapon.type->name.c_str());
        }

        if (quantity > weapon.charges)
        {
            debugmsg("Charges: Tried to remove charges that does not exist, \
                      removing maximum available charges instead");
            quantity = weapon.charges;
        }
        weapon.charges -= quantity;
        if (weapon.charges <= 0)
        {
            return remove_weapon();
        }
        return weapon;
    } else if (position < -1) {
        debugmsg("Wearing charged items is not implemented.");
        return ret_null;
    } else {
        return inv.reduce_charges(position, quantity);
    }
}


item player::i_rem(char let)
{
 item tmp;
 if (weapon.invlet == let) {
  if (std::find(martial_arts_itype_ids.begin(), martial_arts_itype_ids.end(), weapon.type->id) != martial_arts_itype_ids.end()){
   return ret_null;
  }
  tmp = weapon;
  weapon = ret_null;
  return tmp;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let) {
   tmp = worn[i];
   worn.erase(worn.begin() + i);
   return tmp;
  }
 }
 if (!inv.item_by_letter(let).is_null())
  return inv.remove_item(let);
 return ret_null;
}

item player::i_rem(int pos)
{
 item tmp;
 if (pos == -1) {
     if (std::find(martial_arts_itype_ids.begin(), martial_arts_itype_ids.end(), weapon.type->id) != martial_arts_itype_ids.end()){
         return ret_null;
     }
     tmp = weapon;
     weapon = ret_null;
     return tmp;
 } else if (pos < -1 && pos > worn_position_to_index(worn.size())) {
     tmp = worn[worn_position_to_index(pos)];
     worn.erase(worn.begin() + worn_position_to_index(pos));
     return tmp;
 }
 return inv.remove_item(pos);
}

item player::i_rem(itype_id type)
{
    item ret;
    if (weapon.type->id == type)
    {
        return remove_weapon();
    }
    return inv.remove_item(type);
}

// Negative positions indicate weapon/clothing, 0 & positive indicate inventory
item& player::i_at(int position)
{
 if (position == -1)
     return weapon;
 if (position < -1) {
     int worn_index = worn_position_to_index(position);
     if (worn_index < worn.size()) {
         return worn[worn_index];
     }
 }
 return inv.find_item(position);
}

item& player::i_at(char let)
{
 if (let == KEY_ESCAPE)
  return ret_null;
 if (weapon.invlet == let)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return worn[i];
 }
 return inv.item_by_letter(let);
}

item& player::i_of_type(itype_id type)
{
 if (weapon.type->id == type)
  return weapon;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == type)
   return worn[i];
 }
 return inv.item_by_type(type);
}

char player::position_to_invlet(int position) {
    return i_at(position).invlet;
}

int player::invlet_to_position(char invlet) {
    if (weapon.invlet == invlet)
     return -1;
    for (int i = 0; i < worn.size(); i++) {
     if (worn[i].invlet == invlet)
      return worn_position_to_index(i);
    }
    return inv.position_by_letter(invlet);
}

int player::get_item_position(item* it) {
    if (&weapon == it) {
        return -1;
    }
    for (int i = 0; i < worn.size(); i++) {
     if (&worn[i] == it)
      return worn_position_to_index(i);
    }
    return inv.position_by_item(it);
}


martialart player::get_combat_style()
{
 martialart tmp;
 bool pickstyle = (!ma_styles.empty());
 if (pickstyle) {
  tmp = martialarts[style_selected];
  return tmp;
 } else {
  return martialarts["style_none"];
 }
}

std::vector<item *> player::inv_dump()
{
 std::vector<item *> ret;
 if (std::find(standard_itype_ids.begin(), standard_itype_ids.end(), weapon.type->id) != standard_itype_ids.end()){
  ret.push_back(&weapon);
 }
 for (int i = 0; i < worn.size(); i++)
  ret.push_back(&worn[i]);
 inv.dump(ret);
 return ret;
}

item player::i_remn(char invlet)
{
 return inv.remove_item(invlet);
}

std::list<item> player::use_amount(itype_id it, int quantity, bool use_container)
{
 std::list<item> ret;
 bool used_weapon_contents = false;
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it) {
   ret.push_back(weapon.contents[i]);
   quantity--;
   weapon.contents.erase(weapon.contents.begin() + i);
   i--;
   used_weapon_contents = true;
  }
 }
 if (use_container && used_weapon_contents)
  remove_weapon();

 if (weapon.type->id == it && weapon.contents.size() == 0) {
  quantity--;
  ret.push_back(remove_weapon());
 }

 std::list<item> tmp = inv.use_amount(it, quantity, use_container);
 ret.splice(ret.end(), tmp);
 return ret;
}

bool player::use_charges_if_avail(itype_id it, int quantity)
{
    if (has_charges(it, quantity))
    {
        use_charges(it, quantity);
        return true;
    }
    return false;
}

bool player::has_fire(const int quantity)
{
// TODO: Replace this with a "tool produces fire" flag.

    if (g->m.has_nearby_fire(posx, posy)) {
        return true;
    } else if (has_charges("torch_lit", 1)) {
        return true;
    } else if (has_charges("battletorch_lit", quantity)) {
        return true;
    } else if (has_charges("handflare_lit", 1)) {
        return true;
    } else if (has_charges("candle_lit", 1)) {
        return true;
    } else if (has_bionic("bio_tools")) {
        return true;
    } else if (has_bionic("bio_lighter")) {
        return true;
    } else if (has_bionic("bio_laser")) {
        return true;
    } else if (has_charges("ref_lighter", quantity)) {
        return true;
    } else if (has_charges("matches", quantity)) {
        return true;
    } else if (has_charges("lighter", quantity)) {
        return true;
    } else if (has_charges("flamethrower", quantity)) {
        return true;
    } else if (has_charges("flamethrower_simple", quantity)) {
        return true;
    } else if (has_charges("hotplate", quantity)) {
        return true;
    } else if (has_charges("welder", quantity)) {
        return true;
    } else if (has_charges("welder_crude", quantity)) {
        return true;
    } else if (has_charges("shishkebab_on", quantity)) {
        return true;
    } else if (has_charges("firemachete_on", quantity)) {
        return true;
    } else if (has_charges("broadfire_on", quantity)) {
        return true;
    } else if (has_charges("firekatana_on", quantity)) {
        return true;
    } else if (has_charges("zweifire_on", quantity)) {
        return true;
    }
    return false;
}

void player::use_fire(const int quantity)
{
//Ok, so checks for nearby fires first,
//then held lit torch or candle, bio tool/lighter/laser
//tries to use 1 charge of lighters, matches, flame throwers
// (home made, military), hotplate, welder in that order.
// bio_lighter, bio_laser, bio_tools, has_bionic("bio_tools"

    if (g->m.has_nearby_fire(posx, posy)) {
        return;
    } else if (has_charges("torch_lit", 1)) {
        return;
    } else if (has_charges("battletorch_lit", 1)) {
        return;
    } else if (has_charges("handflare_lit", 1)) {
        return;
    } else if (has_charges("candle_lit", 1)) {
        return;
    } else if (has_charges("shishkebab_on", quantity)) {
        return;
    } else if (has_charges("firemachete_on", quantity)) {
        return;
    } else if (has_charges("broadfire_on", quantity)) {
        return;
    } else if (has_charges("firekatana_on", quantity)) {
        return;
    } else if (has_charges("zweifire_on", quantity)) {
        return;
    } else if (has_bionic("bio_tools")) {
        return;
    } else if (has_bionic("bio_lighter")) {
        return;
    } else if (has_bionic("bio_laser")) {
        return;
    } else if (has_charges("ref_lighter", quantity)) {
        use_charges("ref_lighter", quantity);
        return;
    } else if (has_charges("matches", quantity)) {
        use_charges("matches", quantity);
        return;
    } else if (has_charges("lighter", quantity)) {
        use_charges("lighter", quantity);
        return;
    } else if (has_charges("flamethrower", quantity)) {
        use_charges("flamethrower", quantity);
        return;
    } else if (has_charges("flamethrower_simple", quantity)) {
        use_charges("flamethrower_simple", quantity);
        return;
    } else if (has_charges("hotplate", quantity)) {
        use_charges("hotplate", quantity);
        return;
    } else if (has_charges("welder", quantity)) {
        use_charges("welder", quantity);
        return;
    } else if (has_charges("welder_crude", quantity)) {
        use_charges("welder_crude", quantity);
        return;
    } else if (has_charges("shishkebab_off", quantity)) {
        use_charges("shishkebab_off", quantity);
        return;
    } else if (has_charges("firemachete_off", quantity)) {
        use_charges("firemachete_off", quantity);
        return;
    } else if (has_charges("broadfire_off", quantity)) {
        use_charges("broadfire_off", quantity);
        return;
    } else if (has_charges("firekatana_off", quantity)) {
        use_charges("firekatana_off", quantity);
        return;
    } else if (has_charges("zweifire_off", quantity)) {
        use_charges("zweifire_off", quantity);
        return;
    }
}

// does NOT return anything if the item is integrated toolset or fire!
std::list<item> player::use_charges(itype_id it, int quantity)
{
 std::list<item> ret;
 // the first two cases *probably* don't need to be tracked for now...
 if (it == "toolset") {
  power_level -= quantity;
  if (power_level < 0)
   power_level = 0;
  return ret;
 }
 if (it == "fire")
 {
     use_fire(quantity);
     return ret;
 }

// Start by checking weapon contents
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it) {
   if (weapon.contents[i].charges > 0 &&
       weapon.contents[i].charges <= quantity) {
    ret.push_back(weapon.contents[i]);
    quantity -= weapon.contents[i].charges;
    if (weapon.contents[i].destroyed_at_zero_charges()) {
     weapon.contents.erase(weapon.contents.begin() + i);
     i--;
    } else
     weapon.contents[i].charges = 0;
    if (quantity == 0)
     return ret;
   } else {
    item tmp = weapon.contents[i];
    tmp.charges = quantity;
    ret.push_back(tmp);
    weapon.contents[i].charges -= quantity;
    return ret;
   }
  }
 }

 if (weapon.type->id == it || weapon.ammo_type() == it) {
  if (weapon.charges > 0 && weapon.charges <= quantity) {
   ret.push_back(weapon);
   quantity -= weapon.charges;
   if (weapon.destroyed_at_zero_charges())
    remove_weapon();
   else
    weapon.charges = 0;
   if (quantity == 0)
    return ret;
   } else {
    item tmp = weapon;
    tmp.charges = quantity;
    ret.push_back(tmp);
    weapon.charges -= quantity;
    return ret;
   }
  }

 std::list<item> tmp = inv.use_charges(it, quantity);
 ret.splice(ret.end(), tmp);
 return ret;
}

int player::butcher_factor()
{
 int lowest_factor = 999;
 if (has_bionic("bio_tools"))
  lowest_factor=100;
 int inv_factor = inv.butcher_factor();
 if (inv_factor < lowest_factor) {
  lowest_factor = inv_factor;
 }
 if (weapon.has_quality("CUT") && !weapon.has_flag("SPEAR")) {
  int factor = weapon.volume() * 5 - weapon.weight() / 75 -
               weapon.damage_cut();
  if (weapon.damage_cut() <= 20)
   factor *= 2;
  if (factor < lowest_factor)
   lowest_factor = factor;
 }
 return lowest_factor;
}

item* player::pick_usb()
{
 std::vector<std::pair<item*, int> > drives = inv.all_items_by_type("usb_drive");

 if (drives.empty())
  return NULL; // None available!

 std::vector<std::string> selections;
 for (int i = 0; i < drives.size() && i < 9; i++)
  selections.push_back( drives[i].first->tname() );

 int select = menu_vec(false, _("Choose drive:"), selections);

 return drives[ select - 1 ].first;
}

bool player::is_wearing(const itype_id & it) const
{
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it)
   return true;
 }
 return false;
}

bool player::worn_with_flag( std::string flag ) const
{
    for (int i = 0; i < worn.size(); i++) {
        if (worn[i].has_flag( flag )) {
            return true;
        }
    }
    return false;
}

bool player::covered_with_flag(const std::string flag, int parts) const {
  int covered = 0;

  for (std::vector<item>::const_reverse_iterator armorPiece = worn.rbegin(); armorPiece != worn.rend(); ++armorPiece) {
    int cover = ((it_armor *)(armorPiece->type))->covers & parts;

    if (!cover) continue; // For our purposes, this piece covers nothing.
    if (cover & covered) continue; // the body part(s) is already covered.

    bool hasFlag = armorPiece->has_flag(flag);

    if (!hasFlag)
      return false; // The item is the top layer on a relevant body part, and isn't tagged, so we fail.
    else
      covered |= cover; // The item is the top layer on a relevant body part, and is tagged.
  }

  return (covered == parts);
}

bool player::covered_with_flag_exclusively(const std::string flag, int flags) const {
  for (std::vector<item>::const_iterator armorPiece = worn.begin(); armorPiece != worn.end(); ++armorPiece) {
    if ((((it_armor *)(armorPiece->type))->covers & flags) && !armorPiece->has_flag(flag))
      return false;
  }

  return true;
}

bool player::is_water_friendly(int flags) const {
  return covered_with_flag_exclusively("WATER_FRIENDLY", flags);
}

bool player::is_waterproof(int flags) const {
  return covered_with_flag("WATERPROOF", flags);
}

bool player::has_artifact_with(const art_effect_passive effect) const
{
 if (weapon.is_artifact() && weapon.is_tool()) {
  it_artifact_tool *tool = dynamic_cast<it_artifact_tool*>(weapon.type);
  for (int i = 0; i < tool->effects_wielded.size(); i++) {
   if (tool->effects_wielded[i] == effect)
    return true;
  }
  for (int i = 0; i < tool->effects_carried.size(); i++) {
   if (tool->effects_carried[i] == effect)
    return true;
  }
 }
 if (inv.has_artifact_with(effect)) {
  return true;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].is_artifact()) {
   it_artifact_armor *armor = dynamic_cast<it_artifact_armor*>(worn[i].type);
   for (int i = 0; i < armor->effects_worn.size(); i++) {
    if (armor->effects_worn[i] == effect)
     return true;
   }
  }
 }
 return false;
}

bool player::has_amount(itype_id it, int quantity)
{
    if (it == "toolset")
    {
        return has_bionic("bio_tools");
    }
    return (amount_of(it) >= quantity);
}

int player::amount_of(itype_id it) {
    if (it == "toolset" && has_bionic("bio_tools")) {
        return 1;
    }
    if (it == "apparatus") {
        if (has_amount("crackpipe", 1) ||
            has_amount("can_drink", 1) ||
            has_amount("pipe_glass", 1) ||
            has_amount("pipe_tobacco", 1)) {
            return 1;
        }
    }
    int quantity = 0;
    if (weapon.type->id == it) {
        quantity++;
    }

    for (int i = 0; i < weapon.contents.size(); i++)     {
        if (weapon.contents[i].type->id == it) {
            quantity++;
        }
    }
    quantity += inv.amount_of(it);
    return quantity;
}

bool player::has_charges(itype_id it, int quantity)
{
    if (it == "fire" || it == "apparatus")
    {
        return has_fire(quantity);
    }
    return (charges_of(it) >= quantity);
}

int player::charges_of(itype_id it)
{
 if (it == "toolset") {
  if (has_bionic("bio_tools"))
   return power_level;
  else
   return 0;
 }
 int quantity = 0;
 if (weapon.type->id == it || weapon.ammo_type() == it) {
  quantity += weapon.charges;
 }
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it)
   quantity += weapon.contents[i].charges;
 }
 quantity += inv.charges_of(it);
 return quantity;
}

bool player::has_watertight_container()
{
 if (!inv.watertight_container().is_null()) {
  return true;
 }
 if (weapon.is_container() && weapon.contents.empty()) {
   if (weapon.has_flag("WATERTIGHT") && weapon.has_flag("SEALS"))
    return true;
 }

 return false;
}

bool player::has_matching_liquid(itype_id it)
{
    if (inv.has_liquid(it)) {
        return true;
    }
    if (weapon.is_container() && !weapon.contents.empty()) {
        if (weapon.contents[0].type->id == it) { // liquid matches
            it_container* container = dynamic_cast<it_container*>(weapon.type);
            int holding_container_charges;

            if (weapon.contents[0].type->is_food()) {
                it_comest* tmp_comest = dynamic_cast<it_comest*>(weapon.contents[0].type);

                if (tmp_comest->add == ADD_ALCOHOL) // 1 contains = 20 alcohol charges
                    holding_container_charges = container->contains * 20;
                else
                    holding_container_charges = container->contains;
            }
            else if (weapon.contents[0].type->is_ammo())
                holding_container_charges = container->contains * 200;
            else
                holding_container_charges = container->contains;

            if (weapon.contents[0].charges < holding_container_charges)
                return true;
        }
    }
    return false;
}

bool player::has_weapon_or_armor(char let) const
{
 if (weapon.invlet == let)
  return true;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let)
   return true;
 }
 return false;
}

bool player::has_item_with_flag( std::string flag ) const
{
    //check worn items for flag
    if (worn_with_flag( flag ))
    {
        return true;
    }

    //check weapon for flag
    if (weapon.has_flag( flag ))
    {
        return true;
    }

    //check inventory items for flag
    if (inv.has_flag( flag ))
    {
        return true;
    }

    return false;
}

bool player::has_item(char let)
{
 return (has_weapon_or_armor(let) || !inv.item_by_letter(let).is_null());
}

bool player::has_item(int position) {
    return !i_at(position).is_null();
}

bool player::has_item(item *it)
{
 if (it == &weapon)
  return true;
 for (int i = 0; i < worn.size(); i++) {
  if (it == &(worn[i]))
   return true;
 }
 return inv.has_item(it);
}

bool player::has_mission_item(int mission_id)
{
    if (mission_id == -1)
    {
        return false;
    }
    if (weapon.mission_id == mission_id)
    {
        return true;
    }
    for (int i = 0; i < weapon.contents.size(); i++)
    {
        if (weapon.contents[i].mission_id == mission_id)
        return true;
    }
    if (inv.has_mission_item(mission_id))
    {
        return true;
    }
    return false;
}

bool player::i_add_or_drop(item& it, int qty) {
    bool retval = true;
    bool drop = false;
    inv.assign_empty_invlet(it);
    for (int i = 0; i < qty; ++i) {
        if (!drop && (!can_pickWeight(it.weight(), !OPTIONS["DANGEROUS_PICKUPS"])
                      || !can_pickVolume(it.volume()))) {
            drop = true;
        }
        if (drop) {
            retval &= g->m.add_item_or_charges(posx, posy, it);
        } else {
            i_add(it);
        }
    }
    return retval;
}

char player::lookup_item(char let)
{
 if (weapon.invlet == let)
  return -1;

 if (inv.item_by_letter(let).is_null())
  return -2; // -2 is for "item not found"

 return let;
}

hint_rating player::rate_action_eat(item *it)
{
 //TODO more cases, for HINT_IFFY
 if (it->is_food_container() || it->is_food()) {
  return HINT_GOOD;
 }
 return HINT_CANT;
}

bool player::consume(int pos)
{
    item *to_eat = NULL;
    it_comest *comest = NULL;
    int which = -3; // Helps us know how to delete the item which got eaten

    if(pos == INT_MIN) {
        g->add_msg(_("You do not have that item."));
        return false;
    } if (is_underwater()) {
        g->add_msg_if_player(this, _("You can't do that while underwater."));
        return false;
    } else if (pos == -1) {
        // Consume your current weapon
        if (weapon.is_food_container(this)) {
            to_eat = &weapon.contents[0];
            which = -2;
            if (weapon.contents[0].is_food()) {
                comest = dynamic_cast<it_comest*>(weapon.contents[0].type);
            }
        } else if (weapon.is_food(this)) {
            to_eat = &weapon;
            which = -1;
            comest = dynamic_cast<it_comest*>(weapon.type);
        } else {
            g->add_msg_if_player(this,_("You can't eat your %s."), weapon.tname().c_str());
            if(is_npc()) {
                debugmsg("%s tried to eat a %s", name.c_str(), weapon.tname().c_str());
            }
            return false;
        }
    } else {
        // Consume item from inventory
        item& it = inv.find_item(pos);
        if (it.is_food_container(this)) {
            to_eat = &(it.contents[0]);
            which = 1;
            if (it.contents[0].is_food()) {
                comest = dynamic_cast<it_comest*>(it.contents[0].type);
            }
        } else if (it.is_food(this)) {
            to_eat = &it;
            which = 0;
            comest = dynamic_cast<it_comest*>(it.type);
        } else {
            g->add_msg_if_player(this,_("You can't eat your %s."), it.tname().c_str());
            if(is_npc()) {
                debugmsg("%s tried to eat a %s", name.c_str(), it.tname().c_str());
            }
            return false;
        }
    }

    if(to_eat == NULL) {
        debugmsg("Consumed item is lost!");
        return false;
    }

    bool was_consumed = false;
    if (comest != NULL) {
        if (comest->comesttype == "FOOD" || comest->comesttype == "DRINK") {
            was_consumed = eat(to_eat, comest);
            if (!was_consumed) {
                return was_consumed;
            }
        } else if (comest->comesttype == "MED") {
            if (comest->tool != "null") {
                // Check tools
                bool has = has_amount(comest->tool, 1);
                // Tools with charges need to have charges, not just be present.
                if (itypes[comest->tool]->count_by_charges()) {
                    has = has_charges(comest->tool, 1);
                }
                if (!has) {
                    g->add_msg_if_player(this,_("You need a %s to consume that!"),
                                         itypes[comest->tool]->name.c_str());
                    return false;
                }
                use_charges(comest->tool, 1); // Tools like lighters get used
            }
            if (comest->use != &iuse::none) {
                //Check special use
                int was_used = comest->use.call(this, to_eat, false);
                if( was_used == 0 ) {
                    return false;
                }
            }
            consume_effects(to_eat, comest);
            moves -= 250;
            was_consumed = true;
        } else {
            debugmsg("Unknown comestible type of item: %s\n", to_eat->tname().c_str());
        }
    } else {
 // Consume other type of items.
        // For when bionics let you eat fuel
        if (to_eat->is_ammo()) {
            const int factor = 20;
            int max_change = max_power_level - power_level;
            if (max_change == 0) {
                g->add_msg_if_player(this,_("Your internal power storage is fully powered."));
            }
            charge_power(to_eat->charges / factor);
            to_eat->charges -= max_change * factor; //negative charges seem to be okay
            to_eat->charges++; //there's a flat subtraction later
        } else if (!to_eat->type->is_food() && !to_eat->is_food_container(this)) {
            if (to_eat->type->is_book()) {
                it_book* book = dynamic_cast<it_book*>(to_eat->type);
                if (book->type != NULL && !query_yn(_("Really eat %s?"), book->name.c_str())) {
                    return false;
                }
            }
            int charge = (to_eat->volume() + to_eat->weight()) / 225;
            if (to_eat->type->m1 == "leather" || to_eat->type->m2 == "leather") {
                charge /= 4;
            }
            if (to_eat->type->m1 == "wood" || to_eat->type->m2 == "wood") {
                charge /= 2;
            }
            charge_power(charge);
            to_eat->charges = 0;
            g->add_msg_player_or_npc(this, _("You eat your %s."), _("<npcname> eats a %s."),
                                     to_eat->tname().c_str());
        }
        moves -= 250;
        was_consumed = true;
    }

    if (!was_consumed) {
        return false;
    }

    // Actions after consume
    to_eat->charges--;
    if (to_eat->charges <= 0) {
        if (which == -1) {
            weapon = ret_null;
        } else if (which == -2) {
            weapon.contents.erase(weapon.contents.begin());
            g->add_msg_if_player(this,_("You are now wielding an empty %s."), weapon.tname().c_str());
        } else if (which == 0) {
            inv.remove_item(pos);
        } else if (which >= 0) {
            item& it = inv.find_item(pos);
            it.contents.erase(it.contents.begin());
            if (!is_npc()) {
                if (OPTIONS["DROP_EMPTY"] == "no") {
                    g->add_msg(_("%c - an empty %s"), it.invlet, it.tname().c_str());

                } else if (OPTIONS["DROP_EMPTY"] == "watertight") {
                    if (it.is_container()) {
                        if (!(it.has_flag("WATERTIGHT") && it.has_flag("SEALS"))) {
                            g->add_msg(_("You drop the empty %s."), it.tname().c_str());
                            g->m.add_item_or_charges(posx, posy, inv.remove_item(it.invlet));
                        } else {
                            g->add_msg(_("%c - an empty %s"), it.invlet,it.tname().c_str());
                        }
                    }
                } else if (OPTIONS["DROP_EMPTY"] == "all") {
                    g->add_msg(_("You drop the empty %s."), it.tname().c_str());
                    g->m.add_item_or_charges(posx, posy, inv.remove_item(it.invlet));
                }
            }
            if (inv.stack_by_letter(it.invlet).size() > 0) {
                inv.restack(this);
            }
            inv.unsort();
        }
    }
    return true;
}

bool player::eat(item *eaten, it_comest *comest)
{
    int to_eat = 1;
    if (comest == NULL) {
        debugmsg("player::eat(%s); comest is NULL!", eaten->tname().c_str());
        return false;
    }
    if (comest->tool != "null") {
        bool has = has_amount(comest->tool, 1);
        if (itypes[comest->tool]->count_by_charges()) {
            has = has_charges(comest->tool, 1);
        }
        if (!has) {
            g->add_msg_if_player(this,_("You need a %s to consume that!"),
                       itypes[comest->tool]->name.c_str());
            return false;
        }
    }
    if (is_underwater()) {
        g->add_msg_if_player(this, _("You can't do that while underwater."));
        return false;
    }
    bool overeating = (!has_trait("GOURMAND") && hunger < 0 &&
                       comest->nutr >= 5);
    bool hiberfood = (has_trait("HIBERNATE") && (hunger > -60 && thirst > -60 ));    
    bool spoiled = eaten->rotten();

    last_item = itype_id(eaten->type->id);

    if (overeating && !has_trait("HIBERNATE") && !is_npc() &&
        !query_yn(_("You're full.  Force yourself to eat?"))) {
        return false;
    }
    int temp_nutr = comest->nutr;
    int temp_quench = comest->quench;
    if (hiberfood && !is_npc() && (((hunger - temp_nutr) < -60) || ((thirst - temp_quench) < -60))){
       if (!query_yn(_("You're adequately fueled. Prepare for hibernation?"))) {
        return false;
       }
       else
       if(!is_npc()) {add_memorial_log(_("Began preparing for hibernation."));
                      g->add_msg(_("You've begun stockpiling calories and liquid for hibernation. You get the feeling that you should prepare for bed, just in case, but...you're hungry again, and you could eat a whole week's worth of food RIGHT NOW."));
      }
    }

    if (has_trait("CARNIVORE") && eaten->made_of("veggy") && comest->nutr > 0) {
        g->add_msg_if_player(this, _("You can't stand the thought of eating veggies."));
        return false;
    }
    if ((!has_trait("CANNIBAL") && !has_trait("PSYCHOPATH")) && eaten->made_of("hflesh")&& !is_npc() &&
        !query_yn(_("The thought of eating that makes you feel sick. Really do it?"))) {
        return false;
    }
    if ((has_trait("CANNIBAL") && !has_trait("PSYCHOPATH")) && eaten->made_of("hflesh")&& !is_npc() &&
        !query_yn(_("The thought of eating that makes you feel both guilty and excited. Go through with it?"))) {
        return false;
    }

    if (has_trait("VEGETARIAN") && eaten->made_of("flesh") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }
    if (has_trait("MEATARIAN") && eaten->made_of("veggy") && !is_npc() &&
        !query_yn(_("Really eat that %s? Your stomach won't be happy."), eaten->tname().c_str())) {
        return false;
    }

    if (spoiled) {
        if (is_npc()) {
            return false;
        }
        if (!has_trait("SAPROVORE") &&
            !query_yn(_("This %s smells awful!  Eat it?"), eaten->tname().c_str())) {
            return false;
        }
    }

    if (comest->use != &iuse::none) {
        to_eat = comest->use.call(this, eaten, false);
        if( to_eat == 0 ) {
            return false;
        }
    }

    //not working directly in the equation... can't imagine why
    int temp_hunger = hunger - comest->nutr;
    int temp_thirst = thirst - comest->quench;
    int capacity = has_trait("GOURMAND") ? -60 : -20;
    if( has_trait("HIBERNATE") && !is_npc() &&
        // If BOTH hunger and thirst are above the capacity...
        ( hunger > capacity && thirst > capacity ) &&
        // ...and EITHER of them crosses under the capacity...
        ( temp_hunger < capacity || temp_thirst < capacity ) ) {
        // Prompt to make sure player wants to gorge for hibernation...
        if( query_yn(_("Start gorging in preperation for hibernation?")) ) {
            // ...and explain what that means.
            g->add_msg(_("As you force yourself to eat, you have the feeling that you'll just be able to keep eating and then sleep for a long time."));
        } else {
            return false;
        }
    }

    if( has_trait("HIBERNATE") ) {
        capacity = -620;
    }
    if( ( comest->nutr > 0 && temp_hunger < capacity ) ||
        ( comest->quench > 0 && temp_thirst < capacity ) ) {
        if (spoiled){//rotten get random nutrification
            if (!query_yn(_("You can hardly finish it all. Consume it?"))) {
                return false;
            }
        } else {
            if (!query_yn(_("You will not be able to finish it all. Consume it?"))) {
                return false;
            }
        }
    }

    if( spoiled ) {
        g->add_msg(_("Ick, this %s doesn't taste so good..."), eaten->tname().c_str());
        if (!has_trait("SAPROVORE") && (!has_bionic("bio_digestion") || one_in(3))) {
            add_disease("foodpoison", rng(60, (comest->nutr + 1) * 60));
        }
        consume_effects(eaten, comest, spoiled);
    } else {
        consume_effects(eaten, comest);
        if (!(has_trait("GOURMAND") || has_trait("HIBERNATE"))) {
            if ((overeating && rng(-200, 0) > hunger)) {
                vomit();
            }
        }
    }
    // At this point, we've definitely eaten the item, so use up some turns.
    if (has_trait("GOURMAND")) {
        moves -= 150;
    } else {
        moves -= 250;
    }
    // If it's poisonous... poison us.  TODO: More several poison effects
    if (eaten->poison >= rng(2, 4)) {
        add_effect("poison", eaten->poison * 100);
    }
    if (eaten->poison > 0) {
        add_disease("foodpoison", eaten->poison * 300);
    }

    if (comest->comesttype == "DRINK" && !eaten->has_flag("USE_EAT_VERB")) {
        g->add_msg_player_or_npc( this, _("You drink your %s."), _("<npcname> drinks a %s."),
                                  eaten->tname().c_str());
    } else if (comest->comesttype == "FOOD" || eaten->has_flag("USE_EAT_VERB")) {
        g->add_msg_player_or_npc( this, _("You eat your %s."), _("<npcname> eats a %s."),
                                  eaten->tname().c_str());
    }

    if (itypes[comest->tool]->is_tool()) {
        use_charges(comest->tool, 1); // Tools like lighters get used
    }

    if (has_bionic("bio_ethanol") && comest->use == &iuse::alcohol) {
        charge_power(rng(2, 8));
    }
    if (has_bionic("bio_ethanol") && comest->use == &iuse::alcohol_weak) {
        charge_power(rng(1, 4));
    }

    if (eaten->made_of("hflesh")) {
      if (has_trait("CANNIBAL") && has_trait("PSYCHOPATH")) {
          g->add_msg_if_player(this, _("You feast upon the human flesh."));
          add_morale(MORALE_CANNIBAL, 15, 200);
      } else if (has_trait("PSYCHOPATH") && !has_trait("CANNIBAL")) {
          g->add_msg_if_player(this, _("Meh. You've eaten worse."));
      } else if (!has_trait("PSYCHOPATH") && has_trait("CANNIBAL")) {
          g->add_msg_if_player(this, _("You indulge your shameful hunger."));
          add_morale(MORALE_CANNIBAL, 10, 50);
      } else {
          g->add_msg_if_player(this, _("You feel horrible for eating a person."));
          add_morale(MORALE_CANNIBAL, -60, -400, 600, 300);
      }
    }
    if (has_trait("VEGETARIAN") && (eaten->made_of("flesh") || eaten->made_of("hflesh"))) {
        g->add_msg_if_player(this,_("Almost instantly you feel a familiar pain in your stomach."));
        add_morale(MORALE_VEGETARIAN, -75, -400, 300, 240);
    }
    if (has_trait("MEATARIAN") && eaten->made_of("veggy")) {
        g->add_msg_if_player(this,_("Yuck! How can anybody eat this stuff?"));
        add_morale(MORALE_MEATARIAN, -75, -400, 300, 240);
    }
    if ((has_trait("HERBIVORE") || has_trait("RUMINANT")) &&
            eaten->made_of("flesh")) {
        if (!one_in(3)) {
            vomit();
        }
        if (comest->quench >= 2) {
            thirst += int(comest->quench / 2);
        }
        if (comest->nutr >= 2) {
            hunger += int(comest->nutr * .75);
        }
    }
    return true;
}

void player::consume_effects(item *eaten, it_comest *comest, bool rotten)
{
    if (rotten) {
        hunger -= rng(0, comest->nutr);
        thirst -= comest->quench;
        if (!has_trait("SAPROVORE") && !has_bionic("bio_digestion")) {
            health -= 3;
        }
    } else {
        hunger -= comest->nutr;
        thirst -= comest->quench;
        health += comest->healthy;
    }

    if (has_bionic("bio_digestion")) {
        hunger -= rng(0, comest->nutr);
    }

    if (comest->stim != 0) {
        if (abs(stim) < (abs(comest->stim) * 3)) {
            if (comest->stim < 0) {
                stim = std::max(comest->stim * 3, stim + comest->stim);
            } else {
                stim = std::min(comest->stim * 3, stim + comest->stim);
            }
        }
    }
    add_addiction(comest->add, comest->addict);
    if (addiction_craving(comest->add) != MORALE_NULL) {
        rem_morale(addiction_craving(comest->add));
    }
    if (eaten->has_flag("HOT") && eaten->has_flag("EATEN_HOT")) {
        add_morale(MORALE_FOOD_HOT, 5, 10);
    }
    if (has_trait("GOURMAND")) {
        if (comest->fun < -2) {
            add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 4, 60, 30, false, comest);
        } else if (comest->fun > 0) {
            add_morale(MORALE_FOOD_GOOD, comest->fun * 3, comest->fun * 6, 60, 30, false, comest);
        }
        if (has_trait("GOURMAND") && !(has_trait("HIBERNATE"))) {
        if ((comest->nutr > 0 && hunger < -60) || (comest->quench > 0 && thirst < -60)) {
            g->add_msg_if_player(this,_("You can't finish it all!"));
        }
        if (hunger < -60) {
            hunger = -60;
        }
        if (thirst < -60) {
            thirst = -60;
        }
    }
    } if (has_trait("HIBERNATE")) {
         if ((comest->nutr > 0 && hunger < -60) || (comest->quench > 0 && thirst < -60)) { //Tell the player what's going on
            g->add_msg_if_player(this,_("You gorge yourself, preparing to hibernate."));
            if (one_in(2)) {
                (fatigue += (comest->nutr)); //50% chance of the food tiring you
            }
        }
        if ((comest->nutr > 0 && hunger < -200) || (comest->quench > 0 && thirst < -200)) { //Hibernation should cut burn to 60/day
            g->add_msg_if_player(this,_("You feel stocked for a day or two. Got your bed all ready and secured?"));
            if (one_in(2)) {
                (fatigue += (comest->nutr)); //And another 50%, intended cumulative
            }
        }
        if ((comest->nutr > 0 && hunger < -400) || (comest->quench > 0 && thirst < -400)) {
            g->add_msg_if_player(this,_("Mmm.  You can stil fit some more in...but maybe you should get comfortable and sleep."));
             if (!(one_in(3))) {
                (fatigue += (comest->nutr)); //Third check, this one at 66%
            }
        }
        if ((comest->nutr > 0 && hunger < -600) || (comest->quench > 0 && thirst < -600)) {
            g->add_msg_if_player(this,_("That filled a hole!  Time for bed..."));
            fatigue += (comest->nutr); //At this point, you're done.  Schlaf gut.
        }
        if ((comest->nutr > 0 && hunger < -620) || (comest->quench > 0 && thirst < -620)) {
            g->add_msg_if_player(this,_("You can't finish it all!"));
        }
        if (hunger < -620) {
            hunger = -620;
        }
        if (thirst < -620) {
            thirst = -620;
        }
    } else {
        if (comest->fun < 0) {
            add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 6, 60, 30, false, comest);
        } else if (comest->fun > 0) {
            add_morale(MORALE_FOOD_GOOD, comest->fun * 2, comest->fun * 4, 60, 30, false, comest);
        }
        if ((comest->nutr > 0 && hunger < -20) || (comest->quench > 0 && thirst < -20)) {
            g->add_msg_if_player(this,_("You can't finish it all!"));
        }
        if (hunger < -20) {
            hunger = -20;
        }
        if (thirst < -20) {
            thirst = -20;
        }
    }
}

bool player::wield(signed char ch, bool autodrop)
{
 if (weapon.has_flag("NO_UNWIELD")) {
  g->add_msg(_("You cannot unwield your %s!  Withdraw them with 'p'."),
             weapon.tname().c_str());
  return false;
 }
 if (ch == -3) {
  if(weapon.is_null()) {
   return false;
  }
  if (autodrop || volume_carried() + weapon.volume() < volume_capacity()) {
   inv.add_item_keep_invlet(remove_weapon());
   inv.unsort();
   moves -= 20;
   recoil = 0;
   return true;
  } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                      weapon.tname().c_str())) {
   g->m.add_item_or_charges(posx, posy, remove_weapon());
   recoil = 0;
   return true;
  } else
   return false;
 }
 if (ch == 0) {
  g->add_msg(_("You're already wielding that!"));
  return false;
 } else if (ch == -2) {
  g->add_msg(_("You don't have that item."));
  return false;
 }

 item& it = inv.item_by_letter(ch);
 if (it.is_two_handed(this) && !has_two_arms()) {
  g->add_msg(_("You cannot wield a %s with only one arm."),
             it.tname().c_str());
  return false;
 }
 if (!is_armed()) {
  weapon = inv.remove_item((char)ch);
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  moves -= 30;
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (volume_carried() + weapon.volume() - it.volume() <
            volume_capacity()) {
  item tmpweap = remove_weapon();
  weapon = inv.remove_item((char)ch);
  inv.add_item_keep_invlet(tmpweap);
  inv.unsort();
  moves -= 45;
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (query_yn(_("No space in inventory for your %s.  Drop it?"),
                     weapon.tname().c_str())) {
  g->m.add_item_or_charges(posx, posy, remove_weapon());
  weapon = it;
  inv.remove_item(weapon.invlet);
  inv.unsort();
  moves -= 30;
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 }

 return false;

}

void player::pick_style() // Style selection menu
{
 std::vector<std::string> options;
 options.push_back(_("No style"));
 for (int i = 0; i < ma_styles.size(); i++) {
  if(martialarts.find(ma_styles[i]) == martialarts.end()) {
   debugmsg ("Bad hand to hand style: %s",ma_styles[i].c_str());
  } else {
   options.push_back( martialarts[ma_styles[i]].name );
  }
 }
 int selection = menu_vec(false, _("Select a style"), options);
 if (selection >= 2)
  style_selected = ma_styles[selection - 2];
 else
  style_selected = "style_none";
}

hint_rating player::rate_action_wear(item *it)
{
 //TODO flag already-worn items as HINT_IFFY

 if (!it->is_armor()) {
  return HINT_CANT;
 }

 it_armor* armor = dynamic_cast<it_armor*>(it->type);

 // are we trying to put on power armor? If so, make sure we don't have any other gear on.
 if (armor->is_power_armor() && worn.size()) {
  if (armor->covers & mfb(bp_torso)) {
   return HINT_IFFY;
  } else if (armor->covers & mfb(bp_head) && !((it_armor *)worn[0].type)->is_power_armor()) {
   return HINT_IFFY;
  }
 }
 // are we trying to wear something over power armor? We can't have that, unless it's a backpack, or similar.
 if (worn.size() && ((it_armor *)worn[0].type)->is_power_armor() && !(armor->covers & mfb(bp_head))) {
  if (!(armor->covers & mfb(bp_torso) && armor->color == c_green)) {
   return HINT_IFFY;
  }
 }

 // Make sure we're not wearing 2 of the item already
 int count = 0;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it->type->id)
   count++;
 }
 if (count == 2) {
  return HINT_IFFY;
 }
 if (has_trait("WOOLALLERGY") && it->made_of("wool")) {
  return HINT_IFFY; //should this be HINT_CANT? I kinda think not, because HINT_CANT is more for things that can NEVER happen
 }
 if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait("WEBBED")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait("TALONS")) {
  return HINT_IFFY;
 }
 if ( armor->covers & mfb(bp_hands) && (has_trait("ARM_TENTACLES")
        || has_trait("ARM_TENTACLES_4") || has_trait("ARM_TENTACLES_8")) ) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_mouth) && has_trait("BEAK")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_feet) && has_trait("HOOVES")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_feet) && has_trait("LEG_TENTACLES")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && has_trait("HORNS_CURLED")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_torso) && has_trait("SHELL")) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && !it->made_of("wool") &&
     !it->made_of("cotton") && !it->made_of("leather") && !it->made_of("nomex") &&
     (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS"))) {
  return HINT_IFFY;
 }
 // Checks to see if the player is wearing leather/plastic/etc shoes
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) &&
     (it->made_of("leather") || it->made_of("plastic") ||
      it->made_of("steel") || it->made_of("kevlar") ||
      it->made_of("chitin")) && is_wearing_shoes()){
  return HINT_IFFY;
 }

 return HINT_GOOD;
}

bool player::wear(int pos, bool interactive)
{
    item* to_wear = NULL;
    int index = -1;
    if (pos == -1)
    {
        to_wear = &weapon;
        index = -2;
    }
    else
    {
        to_wear = &inv.find_item(pos);
    }

    if (to_wear->is_null())
    {
        if(interactive)
        {
            g->add_msg(_("You don't have that item."));
        }

        return false;
    }

    if (!wear_item(to_wear, interactive))
    {
        return false;
    }

    if (index == -2)
    {
        weapon = ret_null;
    }
    else
    {
        inv.remove_item(to_wear);
    }

    return true;
}

bool player::wear_item(item *to_wear, bool interactive)
{
    it_armor* armor = NULL;

    if (to_wear->is_armor())
    {
        armor = dynamic_cast<it_armor*>(to_wear->type);
    }
    else
    {
        g->add_msg(_("Putting on a %s would be tricky."), to_wear->tname().c_str());
        return false;
    }

    // are we trying to put on power armor? If so, make sure we don't have any other gear on.
    if (armor->is_power_armor())
    {
        for (std::vector<item>::iterator it = worn.begin(); it != worn.end(); ++it)
        {
            if ((dynamic_cast<it_armor*>(it->type))->covers & armor->covers)
            {
                if(interactive)
                {
                    g->add_msg(_("You can't wear power armor over other gear!"));
                }
                return false;
            }
        }

        if (!(armor->covers & mfb(bp_torso)))
        {
            bool power_armor = false;

            if (worn.size())
            {
                for (std::vector<item>::iterator it = worn.begin(); it != worn.end(); ++it)
                {
                    if (dynamic_cast<it_armor*>(it->type)->power_armor)
                    {
                        power_armor = true;
                        break;
                    }
                }
            }

            if (!power_armor)
            {
                if(interactive)
                {
                    g->add_msg(_("You can only wear power armor components with power armor!"));
                }
                return false;
            }
        }

        for (int i = 0; i < worn.size(); i++)
        {
            if (((it_armor *)worn[i].type)->is_power_armor() && worn[i].type == armor)
            {
                if(interactive)
                {
                    g->add_msg(_("You cannot wear more than one %s!"), to_wear->tname().c_str());
                }
                return false;
            }
        }
    }
    else
    {
        // Only headgear can be worn with power armor, except other power armor components
        if( armor->covers & ~(mfb(bp_head) | mfb(bp_eyes) | mfb(bp_mouth) ) ) {
            for (int i = 0; i < worn.size(); i++)
            {
                if( ((it_armor *)worn[i].type)->is_power_armor() )
                {
                    if(interactive)
                    {
                        g->add_msg(_("You can't wear %s with power armor!"), to_wear->tname().c_str());
                    }
                    return false;
                }
            }
        }
    }

    if (!to_wear->has_flag("OVERSIZE"))
    {
        // Make sure we're not wearing 2 of the item already
        int count = 0;

        for (int i = 0; i < worn.size(); i++)
        {
            if (worn[i].type->id == to_wear->type->id)
            {
                count++;
            }
        }

        if (count == 2)
        {
            if(interactive)
            {
                g->add_msg(_("You can't wear more than two %ss at once."), to_wear->tname().c_str());
            }
            return false;
        }

        if (has_trait("WOOLALLERGY") && to_wear->made_of("wool"))
        {
            if(interactive)
            {
                g->add_msg(_("You can't wear that, it's made of wool!"));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0 && armor->encumber > 0)
        {
            if(interactive)
            {
                g->add_msg(wearing_something_on(bp_head) ?
                           _("You can't wear another helmet!") : _("You can't wear a helmet!"));
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && has_trait("WEBBED"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put %s over your webbed hands."), armor->name.c_str());
            }
            return false;
        }

        if ( armor->covers & mfb(bp_hands) &&
             (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
              has_trait("ARM_TENTACLES_8")) )
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put %s over your tentacles."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && has_trait("TALONS"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put %s over your talons."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_hands) && has_trait("PAWS"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot get %s to stay on your paws."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && has_trait("BEAK"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot put a %s over your beak."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) &&
            (has_trait("MUZZLE") || has_trait("BEAR_MUZZLE") || has_trait("LONG_MUZZLE")))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot fit the %s over your muzzle."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_mouth) && has_trait("MINOTAUR"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot fit the %s over your snout."), armor->name.c_str());
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait("HOOVES"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear footwear on your hooves."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait("LEG_TENTACLES"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear footwear on your tentacles."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && has_trait("RAP_TALONS"))
        {
            if(interactive)
            {
                g->add_msg(_("Your talons are much too large for footgear."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) && has_trait("HORNS_CURLED"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear headgear over your horns."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_torso) && has_trait("SHELL"))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear anything over your shell."));
            }
            return false;
        }

        if (armor->covers & mfb(bp_head) &&
            !to_wear->made_of("wool") && !to_wear->made_of("cotton") &&
            !to_wear->made_of("nomex") && !to_wear->made_of("leather") &&
            (has_trait("HORNS_POINTED") || has_trait("ANTENNAE") || has_trait("ANTLERS")))
        {
            if(interactive)
            {
                g->add_msg(_("You cannot wear a helmet over your %s."),
                           (has_trait("HORNS_POINTED") ? _("horns") :
                            (has_trait("ANTENNAE") ? _("antennae") : _("antlers"))));
            }
            return false;
        }

        if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) &&
            (to_wear->made_of("leather") || to_wear->made_of("plastic") ||
             to_wear->made_of("steel") || to_wear->made_of("kevlar") ||
             to_wear->made_of("chitin")) && is_wearing_shoes()) {
            // Checks to see if the player is wearing leather/plastic etc shoes
           	if(interactive){
                g->add_msg(_("You're already wearing footwear!"));
            }
            return false;
        }
    }

    // Armor needs invlets to access, give one if not already assigned.
    if (to_wear->invlet == 0) {
        inv.assign_empty_invlet(*to_wear, true);
    }

    last_item = itype_id(to_wear->type->id);
    worn.push_back(*to_wear);

    if(interactive)
    {
        g->add_msg(_("You put on your %s."), to_wear->tname().c_str());
        moves -= 350; // TODO: Make this variable?

        if (to_wear->is_artifact())
        {
            it_artifact_armor *art = dynamic_cast<it_artifact_armor*>(to_wear->type);
            g->add_artifact_messages(art->effects_worn);
        }

        for (body_part i = bp_head; i < num_bp; i = body_part(i + 1))
        {
            if (armor->covers & mfb(i) && encumb(i) >= 4)
            {
                g->add_msg(
                    (i == bp_head || i == bp_torso || i == bp_mouth) ?
                    _("Your %s is very encumbered! %s"):_("Your %s are very encumbered! %s"),
                    body_part_name(body_part(i), -1).c_str(), encumb_text(body_part(i)).c_str());
            }
        }
    }

    recalc_sight_limits();

    return true;
}

hint_rating player::rate_action_takeoff(item *it) {
 if (!it->is_armor()) {
  return HINT_CANT;
 }

 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == it->invlet) { //surely there must be an easier way to do this?
   return HINT_GOOD;
  }
 }

 return HINT_IFFY;
}

bool player::takeoff(int pos, bool autodrop)
{
    bool taken_off = false;
    if (pos == -1) {
        taken_off = wield(-3, autodrop);
    } else {
        int worn_index = worn_position_to_index(pos);
        if (worn_index >=0 && worn_index < worn.size()) {
            item &w = worn[worn_index];

            // Handle power armor.
            if ((reinterpret_cast<it_armor*>(w.type))->is_power_armor() &&
                    ((reinterpret_cast<it_armor*>(w.type))->covers & mfb(bp_torso))) {
                // We're trying to take off power armor, but cannot do that if we have a power armor component on!
                for (int j = worn.size() - 1; j >= 0; j--) {
                    if ((reinterpret_cast<it_armor*>(worn[j].type))->is_power_armor() &&
                            j != worn_index) {
                        if (autodrop) {
                            g->m.add_item_or_charges(posx, posy, worn[j]);
                            g->add_msg(_("You take off your your %s."), worn[j].tname().c_str());
                            worn.erase(worn.begin() + j);
                            // If we are before worn_index, erasing this element shifted its position by 1.
                            if (worn_index > j) {
                                worn_index -= 1;
                                w = worn[worn_index];
                            }
                            taken_off = true;
                        } else {
                            g->add_msg(_("You can't take off power armor while wearing other power armor components."));
                            return false;
                        }
                    }
                }
            }

            if (autodrop || volume_capacity() - (reinterpret_cast<it_armor*>(w.type))->storage > volume_carried() + w.type->volume) {
                inv.add_item_keep_invlet(w);
                g->add_msg(_("You take off your your %s."), w.tname().c_str());
                worn.erase(worn.begin() + worn_index);
                inv.unsort();
                taken_off = true;
            } else if (query_yn(_("No room in inventory for your %s.  Drop it?"),
                    w.tname().c_str())) {
                g->m.add_item_or_charges(posx, posy, w);
                g->add_msg(_("You take off your your %s."), w.tname().c_str());
                worn.erase(worn.begin() + worn_index);
                taken_off = true;
            }
        } else {
            g->add_msg(_("You are not wearing that item."));
        }
    }

    recalc_sight_limits();

    return taken_off;
}

#include <string>

void player::sort_armor()
{
    int32_t win_x = TERMX/2 - FULL_SCREEN_WIDTH/2;
    int32_t win_y = TERMY/2 - FULL_SCREEN_HEIGHT/2;

    int32_t cont_h   = FULL_SCREEN_HEIGHT - 4;
    int32_t left_w   = 25;
    int32_t right_w  = left_w;
    int32_t middle_w = (FULL_SCREEN_WIDTH-4) - left_w - right_w;

    int32_t tabindex = num_bp;
    int32_t tabcount = num_bp + 1;

    int32_t leftListSize;
    int32_t leftListIndex  = 0;
    int32_t leftListOffset = 0;
    int32_t selected       = -1;

    int32_t rightListSize;
    int32_t rightListOffset = 0;

    item tmp_item;
    std::vector<item*> tmp_worn;
    std::string tmp_str;
    it_armor* each_armor = 0;

    std::string  armor_cat[] = {_("Torso"), _("Head"), _("Eyes"), _("Mouth"), _("Arms"),
                                _("Hands"), _("Legs"), _("Feet"), _("All"),};

    // Layout window
    WINDOW *w_sort_armor = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, win_y, win_x);
    draw_border(w_sort_armor);
    // TODO: use BORDER_COLOR for drawing grids
    mvwhline(w_sort_armor, 2, 1, 0, FULL_SCREEN_WIDTH-2);
    mvwvline(w_sort_armor, 3, left_w + 1, 0, FULL_SCREEN_HEIGHT-4);
    mvwvline(w_sort_armor, 3, left_w + middle_w + 2, 0, FULL_SCREEN_HEIGHT-4);
    // intersections
    mvwhline(w_sort_armor, 2, 0, LINE_XXXO, 1);
    mvwhline(w_sort_armor, 2, FULL_SCREEN_WIDTH-1, LINE_XOXX, 1);
    mvwvline(w_sort_armor, 2, left_w+1, LINE_OXXX, 1);
    mvwvline(w_sort_armor, FULL_SCREEN_HEIGHT-1, left_w+1, LINE_XXOX, 1);
    mvwvline(w_sort_armor, 2, left_w + middle_w + 2, LINE_OXXX, 1);
    mvwvline(w_sort_armor, FULL_SCREEN_HEIGHT-1, left_w + middle_w + 2, LINE_XXOX, 1);
    wrefresh(w_sort_armor);

    // Subwindows (between lines)
    WINDOW *w_sort_cat    = newwin(1, FULL_SCREEN_WIDTH-4, win_y+1, win_x+2);
    WINDOW *w_sort_left   = newwin(cont_h, left_w,   win_y + 3, win_x + 1);
    WINDOW *w_sort_middle = newwin(cont_h, middle_w, win_y + 3, win_x + left_w + 2);
    WINDOW *w_sort_right  = newwin(cont_h, right_w,  win_y + 3, win_x + left_w + middle_w + 3);

    nc_color dam_color[] = {c_green, c_ltgreen, c_yellow, c_magenta, c_ltred, c_red};

    for (bool sorting = true; sorting; ){
        werase(w_sort_cat);
        werase(w_sort_left);
        werase(w_sort_middle);
        werase(w_sort_right);

        // top bar
        wprintz(w_sort_cat, c_white, _("Sort Armor"));
        wprintz(w_sort_cat, c_yellow, "  << %s >>", armor_cat[tabindex].c_str());
        tmp_str = _("Press '?' for help");
        mvwprintz(w_sort_cat, 0, FULL_SCREEN_WIDTH - utf8_width(tmp_str.c_str()) - 4,
                  c_white, tmp_str.c_str());

        // Create ptr list of items to display
        tmp_worn.clear();
        if (tabindex == 8) { // All
            for (int i = 0; i < worn.size(); i++){
                tmp_worn.push_back(&worn[i]);
            }
        } else { // bp_*
            for (int i = 0; i < worn.size(); i++){
                each_armor = dynamic_cast<it_armor*>(worn[i].type);
                if (each_armor->covers & mfb(tabindex))
                    tmp_worn.push_back(&worn[i]);
            }
        }
        leftListSize = (tmp_worn.size() < cont_h-2) ? tmp_worn.size() : cont_h-2;

        // Left header
        mvwprintz(w_sort_left, 0, 0, c_ltgray, _("(Innermost)"));
        mvwprintz(w_sort_left, 0, left_w - utf8_width(_("Storage")), c_ltgray, _("Storage"));

        // Left list
        for (int drawindex = 0; drawindex < leftListSize; drawindex++) {
            int itemindex = leftListOffset + drawindex;
            each_armor = dynamic_cast<it_armor*>(tmp_worn[itemindex]->type);

            if (itemindex == leftListIndex)
                mvwprintz(w_sort_left, drawindex + 1, 1, c_yellow, ">>");

            if (itemindex == selected)
                mvwprintz(w_sort_left, drawindex+1, 4, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          each_armor->name.c_str());
            else
                mvwprintz(w_sort_left, drawindex+1, 3, dam_color[int(tmp_worn[itemindex]->damage + 1)],
                          each_armor->name.c_str());

            mvwprintz(w_sort_left, drawindex+1, left_w-3, c_ltgray, "%2d", int(each_armor->storage));
        }

        // Left footer
        mvwprintz(w_sort_left, cont_h-1, 0, c_ltgray, _("(Outermost)"));
        if (leftListSize > tmp_worn.size())
            mvwprintz(w_sort_left, cont_h-1, left_w - utf8_width(_("<more>")), c_ltblue, _("<more>"));
        if (leftListSize == 0)
            mvwprintz(w_sort_left, cont_h-1, left_w - utf8_width(_("<empty>")), c_ltblue, _("<empty>"));

        // Items stats
        if (leftListSize){
            each_armor = dynamic_cast<it_armor*>(tmp_worn[leftListIndex]->type);

            mvwprintz(w_sort_middle, 0, 1, c_white, each_armor->name.c_str());
            mvwprintz(w_sort_middle, 1, 2, c_ltgray, _("Coverage: "));
            mvwprintz(w_sort_middle, 2, 2, c_ltgray, _("Encumbrance: "));
            mvwprintz(w_sort_middle, 3, 2, c_ltgray, _("Bash protection: "));
            mvwprintz(w_sort_middle, 4, 2, c_ltgray, _("Cut protection: "));
            mvwprintz(w_sort_middle, 5, 2, c_ltgray, _("Warmth: "));
            mvwprintz(w_sort_middle, 6, 2, c_ltgray, _("Storage: "));

            mvwprintz(w_sort_middle, 1, middle_w - 4, c_ltgray, "%d", int(each_armor->coverage));
            mvwprintz(w_sort_middle, 2, middle_w - 4, c_ltgray, "%d",
                      (tmp_worn[leftListIndex]->has_flag("FIT")) ? std::max(0, int(each_armor->encumber) - 1) : int(each_armor->encumber)
                     );
            mvwprintz(w_sort_middle, 3, middle_w - 4, c_ltgray, "%d", int(tmp_worn[leftListIndex]->bash_resist()));
            mvwprintz(w_sort_middle, 4, middle_w - 4, c_ltgray, "%d", int(tmp_worn[leftListIndex]->cut_resist()));
            mvwprintz(w_sort_middle, 5, middle_w - 4, c_ltgray, "%d", int(each_armor->warmth));
            mvwprintz(w_sort_middle, 6, middle_w - 4, c_ltgray, "%d", int(each_armor->storage));

            // Item flags
            if (tmp_worn[leftListIndex]->has_flag("FIT"))
                tmp_str = _("It fits you well.\n");
            else if (tmp_worn[leftListIndex]->has_flag("VARSIZE"))
                tmp_str = _("It could be refitted.\n");
            else
                tmp_str = "";

            if (tmp_worn[leftListIndex]->has_flag("SKINTIGHT"))
                tmp_str += _("It lies close to the skin.\n");
            if (tmp_worn[leftListIndex]->has_flag("POCKETS"))
                tmp_str += _("It has pockets.\n");
                if (tmp_worn[leftListIndex]->has_flag("HOOD"))
                tmp_str += _("It has a hood.\n");
            if (tmp_worn[leftListIndex]->has_flag("WATERPROOF"))
                tmp_str += _("It is waterproof.\n");
            if (tmp_worn[leftListIndex]->has_flag("WATER_FRIENDLY"))
                tmp_str += _("It is water friendly.\n");
            if (tmp_worn[leftListIndex]->has_flag("FANCY"))
                tmp_str += _("It looks fancy.\n");
            if (tmp_worn[leftListIndex]->has_flag("SUPER_FANCY"))
                tmp_str += _("It looks really fancy.\n");
            if (tmp_worn[leftListIndex]->has_flag("FLOATATION"))
                tmp_str += _("You will not drown today.\n");
            if (tmp_worn[leftListIndex]->has_flag("OVERSIZE"))
                tmp_str += _("It is very bulky.\n");
            if (tmp_worn[leftListIndex]->has_flag("SWIM_GOGGLES"))
                tmp_str += _("It helps you to see clearly underwater.\n");
                //WATCH
                //ALARMCLOCK

            mvwprintz(w_sort_middle, 8, 0, c_ltblue, tmp_str.c_str());
        } else {
            mvwprintz(w_sort_middle, 0, 1, c_white, _("Nothing to see here!"));
        }

        // Player encumbrance - altered copy of '@' screen
        mvwprintz(w_sort_middle, cont_h - 9, 1, c_white, _("Encumbrance and Warmth"));
        for (int i = 0; i < num_bp; i++) {
            int enc, armorenc;
            double layers;
            layers = armorenc = 0;
            enc = encumb(body_part(i), layers, armorenc);
            if (leftListSize && (each_armor->covers & mfb(i))) {
                mvwprintz(w_sort_middle, cont_h - 8 + i, 2, c_green, "%s:", armor_cat[i].c_str());
            } else {
                mvwprintz(w_sort_middle, cont_h - 8 + i, 2, c_ltgray, "%s:", armor_cat[i].c_str());
            }
            mvwprintz(w_sort_middle, cont_h - 8 + i, middle_w - 16, c_ltgray, "%d+%d = ", armorenc, enc-armorenc);
            wprintz(w_sort_middle, encumb_color(enc), "%d" , enc);

            nc_color color = c_ltgray;
            if (i == bp_eyes) continue; // Eyes don't count towards warmth
            else if (temp_conv[i] >  BODYTEMP_SCORCHING) color = c_red;
            else if (temp_conv[i] >  BODYTEMP_VERY_HOT)  color = c_ltred;
            else if (temp_conv[i] >  BODYTEMP_HOT)       color = c_yellow;
            else if (temp_conv[i] >  BODYTEMP_COLD)      color = c_green;
            else if (temp_conv[i] >  BODYTEMP_VERY_COLD) color = c_ltblue;
            else if (temp_conv[i] >  BODYTEMP_FREEZING)  color = c_cyan;
            else if (temp_conv[i] <= BODYTEMP_FREEZING)  color = c_blue;
            mvwprintz(w_sort_middle, cont_h - 8 + i, middle_w - 6, color, "(%3d)", warmth(body_part(i)));
        }

        // Right header
        mvwprintz(w_sort_right, 0, 0, c_ltgray, _("(innermost)"));
        mvwprintz(w_sort_right, 0, right_w - utf8_width(_("Encumb")), c_ltgray, _("Encumb"));

        // Right list
        rightListSize     = 0;
        for (int cover = 0, pos = 1; cover < num_bp; cover++){
            if (rightListSize >= rightListOffset && pos <= cont_h-2){
                if (cover == tabindex)
                    mvwprintz(w_sort_right, pos, 1, c_yellow, "%s:", armor_cat[cover].c_str());
                else
                    mvwprintz(w_sort_right, pos, 1, c_white, "%s:", armor_cat[cover].c_str());
                pos++;
            }
            rightListSize++;
            for (int i=0; i<worn.size(); i++){
                each_armor = dynamic_cast<it_armor*>(worn[i].type);
                if (each_armor->covers & mfb(cover)){
                    if (rightListSize >= rightListOffset && pos <= cont_h-2){
                        mvwprintz(w_sort_right, pos, 2, dam_color[int(worn[i].damage + 1)],
                                  each_armor->name.c_str());
                        mvwprintz(w_sort_right, pos, right_w - 2, c_ltgray, "%d",
                                  (worn[i].has_flag("FIT")) ? std::max(0, int(each_armor->encumber) - 1)
                                  : int(each_armor->encumber));
                        pos++;
                    }
                    rightListSize++;
                }
            }
        }

        // Right footer
        mvwprintz(w_sort_right, cont_h - 1, 0, c_ltgray, _("(outermost)"));
        if (rightListSize > cont_h-2)
            mvwprintz(w_sort_right, cont_h-1, right_w - utf8_width(_("<more>")), c_ltblue, _("<more>"));

        // F5
        wrefresh(w_sort_cat);
        wrefresh(w_sort_left);
        wrefresh(w_sort_middle);
        wrefresh(w_sort_right);

        switch (input())
        {
        case '8':
        case 'k':
            if (!leftListSize)
                break;
            leftListIndex--;
            if (leftListIndex < 0)
                leftListIndex = tmp_worn.size()-1;

            // Scrolling logic
            leftListOffset = (leftListIndex < leftListOffset) ? leftListIndex : leftListOffset;
            if (!((leftListIndex >= leftListOffset) && (leftListIndex < leftListOffset + leftListSize))){
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = (leftListOffset > 0) ? leftListOffset : 0;
            }

            // move selected item
            if (selected >= 0){
                tmp_item = *tmp_worn[leftListIndex];
                for (int i=0; i < worn.size(); i++)
                    if (&worn[i] == tmp_worn[leftListIndex])
                        worn[i] = *tmp_worn[selected];

                for (int i=0; i < worn.size(); i++)
                    if (&worn[i] == tmp_worn[selected])
                        worn[i] = tmp_item;

                selected = leftListIndex;
            }
            break;

        case '2':
        case 'j':
            if (!leftListSize)
                break;
            leftListIndex = (leftListIndex+1) % tmp_worn.size();

            // Scrolling logic
            if (!((leftListIndex >= leftListOffset) && (leftListIndex < leftListOffset + leftListSize))){
                leftListOffset = leftListIndex - leftListSize + 1;
                leftListOffset = (leftListOffset > 0) ? leftListOffset : 0;
            }

            // move selected item
            if (selected >= 0){
                tmp_item = *tmp_worn[leftListIndex];
                for (int i=0; i < worn.size(); i++)
                    if (&worn[i] == tmp_worn[leftListIndex])
                        worn[i] = *tmp_worn[selected];

                for (int i=0; i < worn.size(); i++)
                    if (&worn[i] == tmp_worn[selected])
                        worn[i] = tmp_item;

                selected = leftListIndex;
            }
            break;

        case '4':
        case 'h':
            tabindex--;
            if (tabindex < 0)
                tabindex = tabcount - 1;
            leftListIndex = leftListOffset = 0;
            selected = -1;
            break;

        case '\t':
        case '6':
        case 'l':
            tabindex = (tabindex+1) % tabcount;
            leftListIndex = leftListOffset = 0;
            selected = -1;
            break;

        case '>':
            rightListOffset++;
            if (rightListOffset + cont_h-2 > rightListSize)
                rightListOffset = rightListSize - cont_h + 2;
            break;

        case '<':
            rightListOffset--;
            if (rightListOffset < 0)
                rightListOffset = 0;
            break;

        case 's':
            if (selected >= 0)
                selected = -1;
            else
                selected = leftListIndex;
            break;

        case '?':{
            popup_getkey(_("\
Use the arrow- or keypad keys to navigate the left list.\n\
Press 's' to select highlighted armor for reordering.\n\
Use PageUp/PageDown to scroll the right list.\n \n\
[Encumbrance and Warmth] explanation:\n\
The first number is the summed encumbrance from all clothing on that bodypart.\n\
The second number is the encumbrance caused by the number of clothing on that bodypart.\n\
The sum of these values is the effective encumbrance value your character has for that bodypart."));
            draw_border(w_sort_armor); // hack to mark whole window for redrawing
            wrefresh(w_sort_armor);
            break;
        }

        case KEY_ESCAPE:
        case 'q':
            sorting = false;
            break;
        }
    }

    delwin(w_sort_cat);
    delwin(w_sort_left);
    delwin(w_sort_middle);
    delwin(w_sort_right);
    delwin(w_sort_armor);
    return;
}

void player::use_wielded() {
  use(-1);
}

hint_rating player::rate_action_reload(item *it) {
 if (it->is_gun()) {
  if (it->has_flag("RELOAD_AND_SHOOT") || it->ammo_type() == "NULL") {
   return HINT_CANT;
  }
  if (it->charges == it->clip_size()) {
   int alternate_magazine = -1;
   for (int i = 0; i < it->contents.size(); i++)
   {
       if ((it->contents[i].is_gunmod() &&
            (it->contents[i].typeId() == "spare_mag" &&
             it->contents[i].charges < (dynamic_cast<it_gun*>(it->type))->clip)) ||
           (it->contents[i].has_flag("MODE_AUX") &&
            it->contents[i].charges < it->contents[i].clip_size()))
       {
           alternate_magazine = i;
       }
   }
   if(alternate_magazine == -1) {
    return HINT_IFFY;
   }
  }
  return HINT_GOOD;
 } else if (it->is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(it->type);
  if (tool->ammo == "NULL") {
   return HINT_CANT;
  }
  return HINT_GOOD;
 }
 return HINT_CANT;
}

hint_rating player::rate_action_unload(item *it) {
 if (!it->is_gun() && !it->is_container() &&
     (!it->is_tool() || it->ammo_type() == "NULL")) {
  return HINT_CANT;
 }
 int spare_mag = -1;
 int has_m203 = -1;
 int has_40mml = -1;
 int has_shotgun = -1;
 int has_shotgun2 = -1;
 int has_shotgun3 = -1;
 int has_auxflamer = -1;
 if (it->is_gun()) {
  spare_mag = it->has_gunmod ("spare_mag");
  has_m203 = it->has_gunmod ("m203");
  has_40mml = it->has_gunmod ("pipe_launcher40mm");
  has_shotgun = it->has_gunmod ("u_shotgun");
  has_shotgun2 = it->has_gunmod ("masterkey");
  has_shotgun3 = it->has_gunmod ("rm121aux");
  has_auxflamer = it->has_gunmod ("aux_flamer");
 }
 if (it->is_container() ||
     (it->charges == 0 &&
      (spare_mag == -1 || it->contents[spare_mag].charges <= 0) &&
      (has_m203 == -1 || it->contents[has_m203].charges <= 0) &&
      (has_40mml == -1 || it->contents[has_40mml].charges <= 0) &&
      (has_shotgun == -1 || it->contents[has_shotgun].charges <= 0) &&
      (has_shotgun2 == -1 || it->contents[has_shotgun2].charges <= 0) &&
      (has_shotgun3 == -1 || it->contents[has_shotgun3].charges <= 0) &&
      (has_auxflamer == -1 || it->contents[has_auxflamer].charges <= 0) )) {
  if (it->contents.size() == 0) {
   return HINT_IFFY;
  }
 }

 return HINT_GOOD;
}

//TODO refactor stuff so we don't need to have this code mirroring game::disassemble
hint_rating player::rate_action_disassemble(item *it) {
 for (recipe_map::iterator cat_iter = recipes.begin(); cat_iter != recipes.end(); ++cat_iter)
    {
        for (recipe_list::iterator list_iter = cat_iter->second.begin();
             list_iter != cat_iter->second.end();
             ++list_iter)
        {
            recipe* cur_recipe = *list_iter;
            if (it->type == itypes[cur_recipe->result] && cur_recipe->reversible)
            // ok, a valid recipe exists for the item, and it is reversible
            // assign the activity
            {
                // check tools are available
                // loop over the tools and see what's required...again
                inventory crafting_inv = g->crafting_inventory(this);
                for (int j = 0; j < cur_recipe->tools.size(); j++)
                {
                    bool have_tool = false;
                    if (cur_recipe->tools[j].size() == 0) // no tools required, may change this
                    {
                        have_tool = true;
                    }
                    else
                    {
                        for (int k = 0; k < cur_recipe->tools[j].size(); k++)
                        {
                            itype_id type = cur_recipe->tools[j][k].type;
                            int req = cur_recipe->tools[j][k].count; // -1 => 1

                            if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
                                (req >  0 && crafting_inv.has_charges(type, req)))
                            {
                                have_tool = true;
                                k = cur_recipe->tools[j].size();
                            }
                            // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
                            if (type == "welder")
                            {
                                if (crafting_inv.has_amount("hacksaw", 1) ||
                                    crafting_inv.has_amount("toolset", 1))
                                {
                                    have_tool = true;
                                }
                                else
                                {
                                    have_tool = false;
                                }
                            }
                        }
                    }
                    if (!have_tool)
                    {
                       return HINT_IFFY;
                    }
                }
                // all tools present
                return HINT_GOOD;
            }
        }
    }
    if(it->is_book())
        return HINT_GOOD;
    // no recipe exists, or the item cannot be disassembled
    return HINT_CANT;
}

hint_rating player::rate_action_use(const item *it) const
{
 if (it->is_tool()) {
  it_tool *tool = dynamic_cast<it_tool*>(it->type);
  if (tool->charges_per_use != 0 && it->charges < tool->charges_per_use) {
   return HINT_IFFY;
  } else {
   return HINT_GOOD;
  }
 } else if (it->is_gunmod()) {
  if (get_skill_level("gun") == 0) {
   return HINT_IFFY;
  } else {
   return HINT_GOOD;
  }
 } else if (it->is_bionic()) {
  return HINT_GOOD;
 } else if (it->is_food() || it->is_food_container() || it->is_book() || it->is_armor()) {
  return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
 } else if (it->is_gun()) {
   if (!it->contents.empty())
    return HINT_GOOD;
   else
    return HINT_IFFY;
 }

 return HINT_CANT;
}

void player::use(int pos)
{
    item* used = &i_at(pos);
    item copy;

    if (used->is_null()) {
        g->add_msg(_("You do not have that item."));
        return;
    }

    last_item = itype_id(used->type->id);

    if (used->is_tool()) {
        it_tool *tool = dynamic_cast<it_tool*>(used->type);
        if (tool->charges_per_use == 0 || used->charges >= tool->charges_per_use) {
            int charges_used = tool->use.call(this, used, false);
            if ( charges_used >= 1 ) {
                if( tool->charges_per_use > 0 ) {
                    used->charges -= std::min(used->charges, charges_used);
                } else {
                    // An item that doesn't normally expend charges is destroyed instead.
                    i_rem(pos);
                }
            }
            // We may have fiddled with the state of the item in the iuse method,
            // so restack to sort things out.
            inv.restack();
        } else {
            g->add_msg(_("Your %s has %d charges but needs %d."), used->tname().c_str(),
                       used->charges, tool->charges_per_use);
        }
    } else if (used->type->use == &iuse::boots) {
        used->type->use.call(this, used, false);
        return;
    } else if (used->is_gunmod()) {
        if (skillLevel("gun") == 0) {
            g->add_msg(_("You need to be at least level 1 in the marksmanship skill before you\
 can modify weapons."));
            return;
        }
        int gunpos = g->inv(_("Select gun to modify:"));
        it_gunmod *mod = static_cast<it_gunmod*>(used->type);
        item* gun = &(i_at(gunpos));
        if (gun->is_null()) {
            g->add_msg(_("You do not have that item."));
            return;
        } else if (!gun->is_gun()) {
            g->add_msg(_("That %s is not a weapon."), gun->tname().c_str());
            return;
        }
        it_gun* guntype = dynamic_cast<it_gun*>(gun->type);
        if (guntype->skill_used == Skill::skill("pistol") && !mod->used_on_pistol) {
            g->add_msg(_("That %s cannot be attached to a handgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("shotgun") && !mod->used_on_shotgun) {
            g->add_msg(_("That %s cannot be attached to a shotgun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("smg") && !mod->used_on_smg) {
            g->add_msg(_("That %s cannot be attached to a submachine gun."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("rifle") && !mod->used_on_rifle) {
            g->add_msg(_("That %s cannot be attached to a rifle."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("archery") && !mod->used_on_bow && guntype->ammo == "arrow") {
            g->add_msg(_("That %s cannot be attached to a bow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("archery") && !mod->used_on_crossbow && guntype->ammo == "bolt") {
            g->add_msg(_("That %s cannot be attached to a crossbow."),
                       used->tname().c_str());
            return;
        } else if (guntype->skill_used == Skill::skill("launchers") && !mod->used_on_launcher) {
            g->add_msg(_("That %s cannot be attached to a launcher."),
                       used->tname().c_str());
            return;
        } else if ( mod->acceptible_ammo_types.size() &&
                    mod->acceptible_ammo_types.count(guntype->ammo) == 0 ) {
                g->add_msg(_("That %s cannot be used on a %s."), used->tname().c_str(),
                       ammo_name(guntype->ammo).c_str());
                return;
        } else if (!guntype->available_mod_locations[mod->location] > 0) {
            g->add_msg(_("Your %s doesn't have enough room for another %s mod.'  To remove the mods, \
activate your weapon."), gun->tname().c_str(), mod->location.c_str());
            return;
        }
        if (mod->id == "spare_mag" && gun->has_flag("RELOAD_ONE")) {
            g->add_msg(_("You can not use a spare magazine in your %s."),
                       gun->tname().c_str());
            return;
        }
        if (mod->location == "magazine" &&
            gun->clip_size() <= 2) {
            g->add_msg(_("You can not extend the ammo capacity of your %s."),
                       gun->tname().c_str());
            return;
        }
        if (mod->id == "waterproof_gunmod" && gun->has_flag("WATERPROOF_GUN")) {
            g->add_msg(_("Your %s is already waterproof."),
                       gun->tname().c_str());
            return;
        }
        for (int i = 0; i < gun->contents.size(); i++) {
            if (gun->contents[i].type->id == used->type->id) {
                g->add_msg(_("Your %s already has a %s."), gun->tname().c_str(),
                           used->tname().c_str());
                return;
            }
            else if (guntype->available_mod_locations[mod->location] == 0) {
                g->add_msg(_("Your %s cannot hold any more %s modifications."), gun->tname().c_str(),
                           mod->location.c_str());
                return;
            } else if ((mod->id == "clip" || mod->id == "clip2") &&
                       (gun->contents[i].type->id == "clip" ||
                        gun->contents[i].type->id == "clip2")) {
                g->add_msg(_("Your %s already has an extended magazine."),
                           gun->tname().c_str());
                return;
            }
        }
        g->add_msg(_("You attach the %s to your %s."), used->tname().c_str(),
                   gun->tname().c_str());
        gun->contents.push_back(i_rem(pos));
        guntype->available_mod_locations[mod->location] -= 1;
        return;

    } else if (used->is_bionic()) {
        it_bionic* tmp = dynamic_cast<it_bionic*>(used->type);
        if (install_bionics(tmp)) {
            i_rem(pos);
        }
        return;
    } else if (used->is_food() || used->is_food_container()) {
        consume(pos);
        return;
    } else if (used->is_book()) {
        read(pos);
        return;
    } else if (used->is_armor()) {
        wear(pos);
        return;
    } else if (used->is_gun()) {
      // Get weapon mod names.
      std::vector<std::string> mods;
      for (int i = 0; i < used->contents.size(); i++) {
        item tmp = used->contents[i];
        mods.push_back(tmp.name);
      }
      if (!used->contents.empty()) {
        // Create menu.
        int choice = -1;

        uimenu kmenu;
        kmenu.selected = 0;
        kmenu.text = _("Remove which modification?");
        for (int i = 0; i < mods.size(); i++) {
          kmenu.addentry( i, true, -1, mods[i] );
        }
        kmenu.addentry( 4, true, 'r', _("Remove all") );
        kmenu.addentry( 5, true, 'q', _("Cancel") );
        kmenu.query();
        choice = kmenu.ret;

        item *weapon = used;
        if (choice < 4) {
          remove_gunmod(weapon, choice);
          g->add_msg(_("You remove your %s from your %s."), weapon->contents[choice].name.c_str(), weapon->name.c_str());
        }
        else if (choice == 4) {
          for (int i = 0; i < weapon->contents.size(); i++) {
            remove_gunmod(weapon, i);
            i--;
          }
          g->add_msg(_("You remove all the modifications from your %s."), weapon->name.c_str());
        }
        else {
          g->add_msg(_("Never mind."));
          return;
        }
        // Removing stuff from a gun takes time.
        moves -= int(used->reload_time(*this) / 2);
        return;
    }
    else
        g->add_msg(_("Your %s doesn't appear to be modded."), used->name.c_str());
      return;
    } else {
        g->add_msg(_("You can't do anything interesting with your %s."),
                   used->tname().c_str());
        return;
    }
}

void player::remove_gunmod(item *weapon, int id) {
    item *gunmod = &weapon->contents[id];
    item newgunmod;
    item ammo;
    if (gunmod != NULL && gunmod->charges > 0) {
      if (gunmod->curammo != NULL) {
        ammo = item(gunmod->curammo, g->turn);
      } else {
        ammo = item(itypes[default_ammo(weapon->ammo_type())], g->turn);
      }
      ammo.charges = gunmod->charges;
      i_add_or_drop(ammo);
    }
    newgunmod = item(itypes[gunmod->type->id], g->turn);
    i_add_or_drop(newgunmod);
    weapon->contents.erase(weapon->contents.begin()+id);
    return;
}

hint_rating player::rate_action_read(item *it)
{
 //note: there's a cryptic note about macguffins in player::read(). Do we have to account for those?
 if (!it->is_book()) {
  return HINT_CANT;
 }

 it_book *book = dynamic_cast<it_book*>(it->type);

 if (g && g->light_level() < 8 && LL_LIT > g->m.light_at(posx, posy)) {
  return HINT_IFFY;
 } else if (morale_level() < MIN_MORALE_READ &&  book->fun <= 0) {
  return HINT_IFFY; //won't read non-fun books when sad
 } else if (book->intel > 0 && has_trait("ILLITERATE")) {
  return HINT_IFFY;
 } else if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading")
            && !is_wearing("glasses_bifocal") && !has_disease("contacts")) {
  return HINT_IFFY;
 }

 return HINT_GOOD;
}

void player::read(int pos)
{
    vehicle *veh = g->m.veh_at (posx, posy);
    if (veh && veh->player_in_control (this))
    {
        g->add_msg(_("It's a bad idea to read while driving!"));
        return;
    }

    // Check if reading is okay
    // check for light level
    if (fine_detail_vision_mod() > 4)//minimum LL_LOW or LL_DARK + (ELFA_NV or atomic_light)
    {
        g->add_msg(_("You can't see to read!"));
        return;
    }

    // check for traits
    if (has_trait("HYPEROPIC") && !is_wearing("glasses_reading")
        && !is_wearing("glasses_bifocal") && !has_disease("contacts"))
    {
        g->add_msg(_("Your eyes won't focus without reading glasses."));
        return;
    }

    // Find the object
    int index = -1;
    item* it = NULL;
    if (pos == -1)
    {
        index = -2;
        it = &weapon;
    }
    else
    {
        it = &inv.find_item(pos);
    }

    if (it == NULL || it->is_null())
    {
        g->add_msg(_("You do not have that item."));
        return;
    }

// Some macguffins can be read, but they aren't treated like books.
    it_macguffin* mac = NULL;
    if (it->is_macguffin())
    {
        mac = dynamic_cast<it_macguffin*>(it->type);
    }
    if (mac != NULL)
    {
        mac->use.call(this, it, false);
        return;
    }

    if (!it->is_book())
    {
        g->add_msg(_("Your %s is not good reading material."),
        it->tname().c_str());
    return;
    }

    it_book* tmp = dynamic_cast<it_book*>(it->type);
    int time; //Declare this here so that we can change the time depending on whats needed
    bool study = false;
    if (tmp->intel > 0 && has_trait("ILLITERATE")) {
        g->add_msg(_("You're illiterate!"));
        return;
    }
    else if (tmp->type == NULL)
    {
        // special guidebook effect: print a misc. hint when read
        if (tmp->id == "guidebook") {
            g->add_msg(get_hint().c_str());
            moves -= 100;
            return;
        }
        // otherwise do nothing as there's no associated skill
    }
    else if (skillLevel(tmp->type) < (int)tmp->req)
    {
        g->add_msg(_("The %s-related jargon flies over your head!"),
         tmp->type->name().c_str());
        if (tmp->recipes.empty())
        {
            return;
        }
        else
        {
            g->add_msg(_("But you might be able to learn a recipe or two."));
        }
    }
    else if (morale_level() < MIN_MORALE_READ &&  tmp->fun <= 0) // See morale.h
    {
        g->add_msg(_("What's the point of reading?  (Your morale is too low!)"));
        return;
    }
    else if (skillLevel(tmp->type) >= (int)tmp->level && !can_study_recipe(tmp) &&
            !query_yn(_(tmp->fun > 0 ? "It would be fun, but your %s skill won't be improved.  Read anyway?"
                        : "Your %s skill won't be improved.  Read anyway?"),
                      tmp->type->name().c_str()))
    {
        return;
    }
    else if (!activity.continuous && !query_yn("Study %s?", tmp->type->name().c_str()))
    {
        study = false;
    }
    else
    {
        //If we just started studying, tell the player how to stop
        if(!activity.continuous) {
            g->add_msg(_("Now studying %s, %s to stop early."),
                       it->tname().c_str(), press_x(ACTION_PAUSE).c_str());
        }
        study = true;
    }

    if (!tmp->recipes.empty() && !(activity.continuous))
    {
        if (can_study_recipe(tmp))
        {
            g->add_msg(_("This book has more recipes for you to learn."));
        }
        else if (studied_all_recipes(tmp))
        {
            g->add_msg(_("You know all the recipes this book has to offer."));
        }
        else
        {
            g->add_msg(_("This book has more recipes, but you don't have the skill to learn them yet."));
        }
    }

 // Base read_speed() is 1000 move points (1 minute per tmp->time)
    time = tmp->time * read_speed() * (fine_detail_vision_mod());
    if (tmp->intel > int_cur)
    {
        g->add_msg(_("This book is too complex for you to easily understand. It will take longer to read."));
        time += (tmp->time * (tmp->intel - int_cur) * 100); // Lower int characters can read, at a speed penalty
    }

    activity = player_activity(ACT_READ, time, index, pos, "");
    activity.continuous = study;
    moves = 0;

    // Reinforce any existing morale bonus/penalty, so it doesn't decay
    // away while you read more.
    int minutes = time / 1000;
    add_morale(MORALE_BOOK, 0, tmp->fun * 15, minutes + 30, minutes, false,
               tmp);
}

bool player::can_study_recipe(it_book* book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first) &&
            (iter->first->skill_used == NULL || skillLevel(iter->first->skill_used) >= iter->second))
        {
            return true;
        }
    }
    return false;
}

bool player::studied_all_recipes(it_book* book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first))
        {
            return false;
        }
    }
    return true;
}

bool player::try_study_recipe(it_book *book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first) &&
            (iter->first->skill_used == NULL || skillLevel(iter->first->skill_used) >= iter->second))
        {
            if (iter->first->skill_used == NULL || rng(0, 4) <= skillLevel(iter->first->skill_used) - iter->second)
            {
                learn_recipe(iter->first);
                g->add_msg(_("Learned a recipe for %s from the %s."),
                           itypes[iter->first->result]->name.c_str(), book->name.c_str());
                return true;
            }
            else
            {
                g->add_msg(_("Failed to learn a recipe from the %s."), book->name.c_str());
                return false;
            }
        }
    }
    return true; // _("false") seems to mean _("attempted and failed")
}

void player::try_to_sleep()
{
 int vpart = -1;
 vehicle *veh = g->m.veh_at (posx, posy, vpart);
 const trap_id trap_at_pos = g->m.tr_at(posx, posy);
 const ter_id ter_at_pos = g->m.ter(posx, posy);
 const furn_id furn_at_pos = g->m.furn(posx, posy);
 if (furn_at_pos == f_bed || furn_at_pos == f_makeshift_bed ||
     trap_at_pos == tr_cot || trap_at_pos == tr_rollmat ||
     trap_at_pos == tr_fur_rollmat || furn_at_pos == f_armchair ||
     furn_at_pos == f_sofa || furn_at_pos == f_hay ||
     (veh && veh->part_with_feature (vpart, "SEAT") >= 0) ||
      (veh && veh->part_with_feature (vpart, "BED") >= 0))
  g->add_msg(_("This is a comfortable place to sleep."));
 else if (ter_at_pos != t_floor)
  g->add_msg(
             terlist[ter_at_pos].movecost <= 2 ?
             _("It's a little hard to get to sleep on this %s.") :
             _("It's hard to get to sleep on this %s."),
             _(terlist[ter_at_pos].name.c_str())); // FIXME i18n
 add_disease("lying_down", 300);
}

bool player::can_sleep()
{
 int sleepy = 0;
 if (has_addiction(ADD_SLEEP))
  sleepy -= 3;
 if (has_trait("INSOMNIA"))
  sleepy -= 8;
 if (has_trait("EASYSLEEPER"))
  sleepy += 8;

 int vpart = -1;
 vehicle *veh = g->m.veh_at (posx, posy, vpart);
 const trap_id trap_at_pos = g->m.tr_at(posx, posy);
 const ter_id ter_at_pos = g->m.ter(posx, posy);
 const furn_id furn_at_pos = g->m.furn(posx, posy);
 if ((veh && veh->part_with_feature (vpart, "BED") >= 0) ||
     furn_at_pos == f_makeshift_bed || trap_at_pos == tr_cot ||
     furn_at_pos == f_sofa || furn_at_pos == f_hay)
  sleepy += 4;
 else if ((veh && veh->part_with_feature (vpart, "SEAT") >= 0) ||
      trap_at_pos == tr_rollmat || trap_at_pos == tr_fur_rollmat || furn_at_pos == f_armchair)
  sleepy += 3;
 else if (furn_at_pos == f_bed)
  sleepy += 5;
 else if (ter_at_pos == t_floor)
  sleepy += 1;
 else
  sleepy -= g->m.move_cost(posx, posy);
 if (fatigue < 192)
  sleepy -= int( (192 - fatigue) / 4);
 else
  sleepy += int((fatigue - 192) / 16);
 sleepy += rng(-8, 8);
 sleepy -= 2 * stim;
 if (sleepy > 0)
  return true;
 return false;
}

void player::fall_asleep(int duration)
{
    add_disease("sleep", duration);
}

void player::wake_up(const char * message)
{
    rem_disease("sleep");
    if (message) {
        g->add_msg_if_player(this, message);
    } else {
        g->add_msg_if_player(this, _("You wake up."));
    }
}

std::string player::is_snuggling()
{
    std::vector<item>& floor_item = g->m.i_at(posx, posy);
    it_armor* floor_armor = NULL;
    int ticker = 0;

    // If there are no items on the floor, return nothing
    if ( floor_item.size() == 0 ) {
        return "nothing";
    }

    for ( std::vector<item>::iterator afloor_item = floor_item.begin() ; afloor_item != floor_item.end() ; ++afloor_item) {
        if ( !afloor_item->is_armor() ) {
            continue;
        }
        else if ( afloor_item->volume() > 1 &&
        (dynamic_cast<it_armor*>(afloor_item->type)->covers & mfb(bp_torso) ||
         dynamic_cast<it_armor*>(afloor_item->type)->covers & mfb(bp_legs)) ){
            floor_armor = dynamic_cast<it_armor*>(afloor_item->type);
            ticker++;
        }
    }

    if ( ticker == 0 ) {
        return "nothing";
    }
    else if ( ticker == 1 ) {
        return floor_armor->name.c_str();
    }
    else if ( ticker > 1 ) {
        return "many";
    }

    return "nothing";
}

// Returned values range from 1.0 (unimpeded vision) to 5.0 (totally blind).
// 2.5 is enough light for detail work.
float player::fine_detail_vision_mod()
{
    if (has_effect("blind") || has_disease("boomered"))
    {
        return 5;
    }
    if ( has_nv() )
    {
        return 1.5;
    }
    // flashlight is handled by the light level check below
    if (g->u.has_active_item("lightstrip"))
    {
        return 1;
    }
    if (LL_LIT <= g->m.light_at(posx, posy))
    {
        return 1;
    }

    float vision_ii = 0;
    if (g->m.light_at(posx, posy) == LL_LOW) { vision_ii = 4; }
    else if (g->m.light_at(posx, posy) == LL_DARK) { vision_ii = 5; }

    if (g->u.has_item_with_flag("LIGHT_2")){
        vision_ii -= 2;
    } else if (g->u.has_item_with_flag("LIGHT_1")){
        vision_ii -= 1;
    }

    if (has_trait("NIGHTVISION")) { vision_ii -= .5; }
	else if (has_trait("ELFA_NV")) { vision_ii -= 1; }
    else if (has_trait("NIGHTVISION2") || has_trait("FEL_NV")) { vision_ii -= 2; }
    else if (has_trait("NIGHTVISION3") || has_trait("ELFA_FNV")) { vision_ii -= 3; }

    if (vision_ii < 1) { vision_ii = 1; }
    return vision_ii;
}

int player::warmth(body_part bp)
{
    int bodywetness = 0;
    int ret = 0, warmth = 0;
    it_armor* armor = NULL;

    // Fetch the morale value of wetness for bodywetness
    for (int i = 0; bodywetness == 0 && i < morale.size(); i++)
    {
        if( morale[i].type == MORALE_WET )
        {
            bodywetness = abs(morale[i].bonus); // Make it positive, less confusing
            break;
        }
    }

    // If the player is not wielding anything, check if hands can be put in pockets
    if(bp == bp_hands && !is_armed() && (temp_conv[bp] <=  BODYTEMP_COLD) && worn_with_flag("POCKETS"))
    {
        ret += 10;
    }

    // If the players head is not encumbered, check if hood can be put up
    if(bp == bp_head && encumb(bp_head) < 1 && (temp_conv[bp] <=  BODYTEMP_COLD) && worn_with_flag("HOOD"))
    {
        ret += 10;
    }

    for (int i = 0; i < worn.size(); i++)
    {
        armor = dynamic_cast<it_armor*>(worn[i].type);

        if (armor->covers & mfb(bp))
        {
            warmth = armor->warmth;
            // Wool items do not lose their warmth in the rain
            if (!worn[i].made_of("wool"))
            {
                warmth *= 1.0 - (float)bodywetness / 100.0;
            }
            ret += warmth;
        }
    }
    return ret;
}

int player::encumb(body_part bp) {
 int iArmorEnc = 0;
 double iLayers = 0;
 return encumb(bp, iLayers, iArmorEnc);
}

int player::encumb(body_part bp, double &layers, int &armorenc)
{
    int ret = 0;

    int skintight = 0;

    it_armor* armor;
    for (int i = 0; i < worn.size(); i++)
    {
        if( !worn[i].is_armor() ) {
            debugmsg("%s::encumb hit a non-armor item at worn[%d] (%s)", name.c_str(),
                     i, worn[i].tname().c_str());
        }
        armor = dynamic_cast<it_armor*>(worn[i].type);

        if( armor->covers & mfb(bp) ) {
            layers++;
            if( armor->is_power_armor() &&
                (has_active_item("UPS_on") || has_active_item("adv_UPS_on") ||
                 has_active_bionic("bio_power_armor_interface") ||
                 has_active_bionic("bio_power_armor_interface_mkII")) ) {
                armorenc += armor->encumber - 4;
            } else {
                armorenc += armor->encumber;
                // Fitted clothes will either reduce encumbrance or negate layering.
                if( worn[i].has_flag( "FIT" ) ) {
                    if( armor->encumber > 0 && armorenc > 0 ) {
                        armorenc--;
                    } else if (layers > 0) {
                        layers -= .5;
                    }
                }
                if( worn[i].has_flag( "SKINTIGHT" ) && layers > 0) {
                  // Skintight clothes will negate layering.
                  // But only if we aren't wearing more than two.
                  if (skintight < 2) {
                    skintight++;
                    layers -= .5;
                  }
                }
            }
        }
    }
    if (armorenc < 0) {
      armorenc = 0;
    }
    if (layers < 0) {
      layers = 0;
    }

    ret += armorenc;

    if (layers > 1) {
        ret += (int(layers) - 1) * (bp == bp_torso ? .75 : 1);// Easier to layer on torso
    }
    if (volume_carried() > volume_capacity() - 2 && bp != bp_head) {
        ret += 3;
    }

    // Bionics and mutation
    if( has_bionic("bio_stiff") && bp != bp_head && bp != bp_mouth ) {
        ret += 1;
    }
    if( has_trait("CHITIN3") && bp != bp_eyes && bp != bp_mouth ) {
        ret += 1;
    }
    if( has_trait("SLIT_NOSTRILS") && bp == bp_mouth ) {
        ret += 1;
    }
    if( has_trait("ARM_FEATHERS") && bp == bp_arms ) {
        ret += 2;
    }
    if ( has_trait("PAWS") && bp == bp_hands ) {
        ret += 1;
    }
    if ( has_trait("LARGE") && (bp == bp_arms || bp == bp_torso )) {
        ret += 1;
    }
    if (bp == bp_hands &&
        (has_trait("ARM_TENTACLES") || has_trait("ARM_TENTACLES_4") ||
         has_trait("ARM_TENTACLES_8")) ) {
        ret += 3;
    }
    if ( ret < 0 ) {
      ret = 0;
    }
    return ret;
}

int player::get_armor_bash(body_part bp) {
    return get_armor_bash_base(bp) + armor_bash_bonus;
}

int player::get_armor_cut(body_part bp) {
    return get_armor_cut_base(bp) + armor_cut_bonus;
}

int player::get_armor_bash_base(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += worn[i].bash_resist();
 }
 if (has_bionic("bio_carbon"))
  ret += 2;
 if (bp == bp_head && has_bionic("bio_armor_head"))
  ret += 3;
 else if (bp == bp_arms && has_bionic("bio_armor_arms"))
  ret += 3;
 else if (bp == bp_torso && has_bionic("bio_armor_torso"))
  ret += 3;
 else if (bp == bp_legs && has_bionic("bio_armor_legs"))
  ret += 3;
  else if (bp == bp_eyes && has_bionic("bio_armor_eyes"))
  ret += 3;
 if (has_trait("FUR") || has_trait("LUPINE_FUR"))
  ret++;
 if (bp == bp_head && has_trait("LYNX_FUR"))
  ret++;
 if (has_trait("CHITIN"))
  ret += 2;
 if (has_trait("SHELL") && bp == bp_torso)
  ret += 6;
 ret += rng(0, disease_intensity("armor_boost"));
 return ret;
}

int player::get_armor_cut_base(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += worn[i].cut_resist();
 }
 if (has_bionic("bio_carbon"))
  ret += 4;
 if (bp == bp_head && has_bionic("bio_armor_head"))
  ret += 3;
 else if (bp == bp_arms && has_bionic("bio_armor_arms"))
  ret += 3;
 else if (bp == bp_torso && has_bionic("bio_armor_torso"))
  ret += 3;
 else if (bp == bp_legs && has_bionic("bio_armor_legs"))
  ret += 3;
 else if (bp == bp_eyes && has_bionic("bio_armor_eyes"))
  ret += 3;
 if (has_trait("THICKSKIN"))
  ret++;
 if (has_trait("THINSKIN"))
  ret--;
 if (has_trait("SCALES"))
  ret += 2;
 if (has_trait("THICK_SCALES"))
  ret += 4;
 if (has_trait("SLEEK_SCALES"))
  ret += 1;
 if (has_trait("CHITIN"))
  ret += 2;
 if (has_trait("CHITIN2"))
  ret += 4;
 if (has_trait("CHITIN3"))
  ret += 8;
 if (has_trait("SHELL") && bp == bp_torso)
  ret += 14;
 ret += rng(0, disease_intensity("armor_boost"));
 return ret;
}

void get_armor_on(player* p, body_part bp, std::vector<int>& armor_indices) {
    it_armor* tmp;
    for (int i=0; i<p->worn.size(); i++) {
        tmp = dynamic_cast<it_armor*>(p->worn[i].type);
        if (tmp->covers & mfb(bp)) {
            armor_indices.push_back(i);
        }
    }
}

// mutates du, returns true iff armor was damaged
bool player::armor_absorb(damage_unit& du, item& armor) {
    it_armor* armor_type = dynamic_cast<it_armor*>(armor.type);

    float mitigation = 0; // total amount of damage mitigated
    float effective_resist = resistances(armor).get_effective_resist(du);
    bool armor_damaged = false;

    std::string pre_damage_name = armor.tname();

    if (rng(0,100) < armor_type->coverage) {
        if (armor_type->is_power_armor()) { // TODO: add some check for power armor
        }

        mitigation = std::min(effective_resist, du.amount);
        du.amount -= mitigation; // mitigate the damage first

        // if the post-mitigation amount is greater than the amount
        if ((du.amount > effective_resist && !one_in(du.amount) && one_in(2)) ||
                // or if it isn't, but 1/50 chance
                (du.amount <= effective_resist && !armor.has_flag("STURDY")
                && !armor_type->is_power_armor() && one_in(200))) {
            armor_damaged = true;
            armor.damage++;
            std::string damage_verb = du.type == DT_BASH
                ? armor_type->bash_dmg_verb()
                : armor_type->cut_dmg_verb();
            g->add_msg_if_player(this, _("Your %s is %s!"), pre_damage_name.c_str(),
                                    damage_verb.c_str());
        }
    }
    return armor_damaged;
}
void player::absorb_hit(body_part bp, int, damage_instance &dam) {
    std::vector<int> armor_indices;

    get_armor_on(this,bp,armor_indices);
    for (std::vector<damage_unit>::iterator it = dam.damage_units.begin();
            it != dam.damage_units.end(); ++it) {
        // CBMs absorb damage first before hitting armour
        if (has_active_bionic("bio_ads")) {
            if (it->amount > 0 && power_level > 1) {
                if (it->type == DT_BASH)
                    it->amount -= rng(1, 8);
                else if (it->type == DT_CUT)
                    it->amount -= rng(1, 4);
                else if (it->type == DT_STAB)
                    it->amount -= rng(1, 2);
                power_level--;
            }
            if (it->amount < 0) it->amount = 0;
        }

        // TODO: do this properly, with std::remove_if or something
        int offset = 0;
        for (std::vector<int>::iterator armor_it = armor_indices.begin();
                armor_it != armor_indices.end(); ++armor_it) {

            int index = *armor_it + offset;

            armor_absorb(*it, worn[index]);

            // now check if armour was completely destroyed and display relevant messages
            // TODO: use something less janky than the old code for this check
            if (worn[index].damage >= 5) {
                add_memorial_log(_("Worn %s was completely destroyed."), worn[index].tname().c_str());
                g->add_msg_player_or_npc( this, _("Your %s is completely destroyed!"),
                                            _("<npcname>'s %s is completely destroyed!"),
                                            worn[index].tname().c_str() );
                worn.erase(worn.begin() + index);
                offset--;
            }
        }
        if (it->type == DT_BASH) {
        } else if (it->type == DT_CUT) {
        }
    }
}


void player::absorb(body_part bp, int &dam, int &cut)
{
    it_armor* tmp;
    int arm_bash = 0, arm_cut = 0;
    bool cut_through = true;      // to determine if cutting damage penetrates multiple layers of armour
    int bash_absorb = 0;      // to determine if lower layers of armour get damaged

    // CBMS absorb damage first before hitting armour
    if (has_active_bionic("bio_ads"))
    {
        if (dam > 0 && power_level > 1)
        {
            dam -= rng(1, 8);
            power_level--;
        }
        if (cut > 0 && power_level > 1)
        {
            cut -= rng(0, 4);
            power_level--;
        }
        if (dam < 0)
            dam = 0;
        if (cut < 0)
            cut = 0;
    }

    // determines how much damage is absorbed by armour
    // zero if damage misses a covered part
    int bash_reduction = 0;
    int cut_reduction = 0;

    // See, we do it backwards, iterating inwards
    for (int i = worn.size() - 1; i >= 0; i--)
    {
        tmp = dynamic_cast<it_armor*>(worn[i].type);
        if (tmp->covers & mfb(bp))
        {
            // first determine if damage is at a covered part of the body
            // probability given by coverage
            if (rng(0, 100) < tmp->coverage)
            {
                // hit a covered part of the body, so now determine if armour is damaged
                arm_bash = worn[i].bash_resist();
                arm_cut  = worn[i].cut_resist();
                // also determine how much damage is absorbed by armour
                // factor of 3 to normalise for material hardness values
                bash_reduction = arm_bash / 3;
                cut_reduction = arm_cut / 3;

                // power armour first  - to depreciate eventually
                if (((it_armor *)worn[i].type)->is_power_armor())
                {
                    if (cut > arm_cut * 2 || dam > arm_bash * 2)
                    {
                        g->add_msg_if_player(this,_("Your %s is damaged!"), worn[i].tname().c_str());
                        worn[i].damage++;
                    }
                }
                else // normal armour
                {
                    // determine how much the damage exceeds the armour absorption
                    // bash damage takes into account preceding layers
                    int diff_bash = (dam - arm_bash - bash_absorb < 0) ? -1 : (dam - arm_bash);
                    int diff_cut  = (cut - arm_cut  < 0) ? -1 : (cut - arm_cut);
                    bool armor_damaged = false;
                    std::string pre_damage_name = worn[i].tname();

                    // armour damage occurs only if damage exceeds armour absorption
                    // plus a luck factor, even if damage is below armour absorption (2% chance)
                    if ((dam > arm_bash && !one_in(diff_bash)) ||
                        (!worn[i].has_flag ("STURDY") && diff_bash == -1 && one_in(50)))
                    {
                        armor_damaged = true;
                        worn[i].damage++;
                    }
                    bash_absorb += arm_bash;

                    // cut damage falls through to inner layers only if preceding layer was damaged
                    if (cut_through)
                    {
                        if ((cut > arm_cut && !one_in(diff_cut)) ||
                            (!worn[i].has_flag ("STURDY") && diff_cut == -1 && one_in(50)))
                        {
                            armor_damaged = true;
                            worn[i].damage++;
                        }
                        else // layer of clothing was not damaged, so stop cutting damage from penetrating
                        {
                            cut_through = false;
                        }
                    }

                    // now check if armour was completely destroyed and display relevant messages
                    if (worn[i].damage >= 5)
                    {
                      add_memorial_log(_("Worn %s was completely destroyed."), worn[i].tname().c_str());
                        g->add_msg_player_or_npc( this, _("Your %s is completely destroyed!"),
                                                  _("<npcname>'s %s is completely destroyed!"),
                                                  worn[i].tname().c_str() );
                        worn.erase(worn.begin() + i);
                    } else if (armor_damaged) {
                        std::string damage_verb = diff_bash > diff_cut ? tmp->bash_dmg_verb() :
                                                                         tmp->cut_dmg_verb();
                        g->add_msg_if_player(this, _("Your %s is %s!"), pre_damage_name.c_str(),
                                             damage_verb.c_str());
                    }
                } // end of armour damage code
            }
        }
        // reduce damage accordingly
        dam -= bash_reduction;
        cut -= cut_reduction;
    }
    // now account for CBMs and mutations
    if (has_bionic("bio_carbon"))
    {
        dam -= 2;
        cut -= 4;
    }
    if (bp == bp_head && has_bionic("bio_armor_head"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_arms && has_bionic("bio_armor_arms"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_torso && has_bionic("bio_armor_torso"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_legs && has_bionic("bio_armor_legs"))
    {
        dam -= 3;
        cut -= 3;
    }
    else if (bp == bp_eyes && has_bionic("bio_armor_eyes"))
    {
        dam -= 3;
        cut -= 3;
    }
    if (has_trait("THICKSKIN"))
        cut--;
    if (has_trait("THINSKIN"))
        cut++;
    if (has_trait("SCALES"))
        cut -= 2;
    if (has_trait("THICK_SCALES"))
        cut -= 4;
    if (has_trait("SLEEK_SCALES"))
        cut -= 1;
    if (has_trait("FEATHERS"))
        dam--;
    if (bp == bp_arms && has_trait("ARM_FEATHERS"))
        dam--;
    if (has_trait("FUR") || has_trait("LUPINE_FUR"))
        dam--;
    if (bp == bp_head && has_trait("LYNX_FUR"))
        dam--;
    if (has_trait("CHITIN"))
        cut -= 2;
    if (has_trait("CHITIN2"))
    {
        dam--;
        cut -= 4;
    }
    if (has_trait("CHITIN3"))
    {
        dam -= 2;
        cut -= 8;
    }
    if (has_trait("PLANTSKIN"))
        dam--;
    if (has_trait("BARK"))
        dam -= 2;
    if (bp == bp_feet && has_trait("HOOVES"))
        cut--;
    if (has_trait("LIGHT_BONES"))
        dam *= 1.4;
    if (has_trait("HOLLOW_BONES"))
        dam *= 1.8;

    // apply martial arts armor buffs
    dam -= mabuff_arm_bash_bonus();
    cut -= mabuff_arm_cut_bonus();

    if (dam < 0)
        dam = 0;
    if (cut < 0)
        cut = 0;
}

int player::get_env_resist(body_part bp)
{
    int ret = 0;
    for (int i = 0; i < worn.size(); i++) {
        if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp) ||
             (bp == bp_eyes && // Head protection works on eyes too (e.g. baseball cap)
             (dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_head))) {
            ret += (dynamic_cast<it_armor*>(worn[i].type))->env_resist;
        }
    }

    if (bp == bp_mouth && has_bionic("bio_purifier") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }
    return ret;

    if (bp == bp_eyes && has_bionic("bio_armor_eyes") && ret < 5) {
        ret += 2;
        if (ret > 5) {
            ret = 5;
        }
    }
    return ret;
}

bool player::wearing_something_on(body_part bp)
{
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp))
    return true;
 }
 return false;
}

bool player::is_wearing_shoes() {
    for (int i = 0; i < worn.size(); i++) {
        item *worn_item = &worn[i];
        it_armor *worn_armor = dynamic_cast<it_armor*>(worn_item->type);

        if (worn_armor->covers & mfb(bp_feet) &&
            (worn_item->made_of("leather") || worn_item->made_of("plastic") ||
             worn_item->made_of("steel") || worn_item->made_of("kevlar") ||
             worn_item->made_of("chitin"))) {
            return true;
        }
    }
    return false;
}

bool player::is_wearing_power_armor(bool *hasHelmet) const {
  if (worn.size() && ((it_armor *)worn[0].type)->is_power_armor()) {
    if (hasHelmet) {
      *hasHelmet = false;

      if (worn.size() > 1) {
        for (size_t i = 1; i < worn.size(); i++) {
          it_armor *candidate = dynamic_cast<it_armor*>(worn[i].type);

          if (candidate->is_power_armor() && candidate->covers & mfb(bp_head)) {
            *hasHelmet = true;
            break;
          }
        }
      }
    }

    return true;
  } else {
    return false;
  }
}

int player::adjust_for_focus(int amount)
{
    int effective_focus = focus_pool;
    if (has_trait("FASTLEARNER"))
    {
        effective_focus += 15;
    }
    if (has_trait("SLOWLEARNER"))
    {
        effective_focus -= 15;
    }
    double tmp = amount * (effective_focus / 100.0);
    int ret = int(tmp);
    if (rng(0, 100) < 100 * (tmp - ret))
    {
        ret++;
    }
    return ret;
}

void player::practice (const calendar& turn, Skill *s, int amount)
{
    SkillLevel& level = skillLevel(s);
    // Double amount, but only if level.exercise isn't a amall negative number?
    if (level.exercise() < 0)
    {
        if (amount >= -level.exercise())
        {
            amount -= level.exercise();
        } else {
            amount += amount;
        }
    }

    bool isSavant = has_trait("SAVANT");

    Skill *savantSkill = NULL;
    SkillLevel savantSkillLevel = SkillLevel();

    if (isSavant)
    {
        for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin();
             aSkill != Skill::skills.end(); ++aSkill)
        {
            if (skillLevel(*aSkill) > savantSkillLevel)
            {
                savantSkill = *aSkill;
                savantSkillLevel = skillLevel(*aSkill);
            }
        }
    }

    amount = adjust_for_focus(amount);
    if (isSavant && s != savantSkill)
    {
        amount /= 2;
    }

    if (amount > 0 && level.isTraining())
    {
        skillLevel(s).train(amount);

        int chance_to_drop = focus_pool;
        focus_pool -= chance_to_drop / 100;
        if (rng(1, 100) <= (chance_to_drop % 100))
        {
            focus_pool--;
        }
    }

    skillLevel(s).practice(turn);
}

void player::practice (const calendar& turn, std::string s, int amount)
{
    Skill *aSkill = Skill::skill(s);
    practice(turn, aSkill, amount);
}

bool player::knows_recipe(recipe *rec)
{
    // do we know the recipe by virtue of it being autolearned?
    if (rec->autolearn)
    {
        // Can the skill being trained can handle the difficulty of the task
        bool meets_requirements = false;
        if(rec->skill_used == NULL || skillLevel(rec->skill_used) >= rec->difficulty){
            meets_requirements = true;
            //If there are required skills, insure their requirements are met, or we can't craft
            if(!rec->required_skills.empty()){
                for(std::map<Skill*,int>::iterator iter=rec->required_skills.begin(); iter!=rec->required_skills.end();++iter){
                    if(skillLevel(iter->first) < iter->second){
                        meets_requirements = false;
                    }
                }
            }
        }
        if(meets_requirements){
            return true;
        }
    }

    if (learned_recipes.find(rec->ident) != learned_recipes.end())
    {
        return true;
    }

    return false;
}

void player::learn_recipe(recipe *rec)
{
    learned_recipes[rec->ident] = rec;
}

void player::assign_activity(activity_type type, int moves, int index, int pos, std::string name)
{
    if (backlog.type == type && backlog.index == index && backlog.position == pos &&
        backlog.name == name && query_yn(_("Resume task?"))) {
            activity = backlog;
            backlog = player_activity();
    } else {
        activity = player_activity(type, moves, index, pos, name);
    }
    activity.warned_of_proximity = false;
}

bool player::has_activity(const activity_type type)
{
    if (activity.type == type) {
        return true;
    }

    return false;
}

void player::cancel_activity()
{
 if (activity_is_suspendable(activity.type))
  backlog = activity;
 activity.type = ACT_NULL;
}

std::vector<item*> player::has_ammo(ammotype at)
{
    return inv.all_ammo(at);
}

std::string player::weapname(bool charges)
{
 if (!(weapon.is_tool() &&
       dynamic_cast<it_tool*>(weapon.type)->max_charges <= 0) &&
     weapon.charges >= 0 && charges) {
  std::stringstream dump;
  int spare_mag = weapon.has_gunmod("spare_mag");
  dump << weapon.tname().c_str();
  if (!weapon.has_flag("NO_AMMO")) {
   dump << " (" << weapon.charges;
   if( -1 != spare_mag )
   dump << "+" << weapon.contents[spare_mag].charges;
   for (int i = 0; i < weapon.contents.size(); i++)
   if (weapon.contents[i].is_gunmod() &&
     weapon.contents[i].has_flag("MODE_AUX"))
    dump << "+" << weapon.contents[i].charges;
   dump << ")";
  }
  return dump.str();
 } else if (weapon.is_container()) {
  std::stringstream dump;
  dump << weapon.tname().c_str();
  if(weapon.contents.size() == 1) {
   dump << " (" << weapon.contents[0].charges << ")";
  }
  return dump.str();
 } else if (weapon.is_null()) {
  return _("fists");
 } else
  return weapon.tname();
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level == 0)
  return c_ltgray;
 if (level < 4)
  return c_yellow;
 if (level < 7)
  return c_ltred;
 return c_red;
}

bool activity_is_suspendable(activity_type type)
{
 if (type == ACT_NULL || type == ACT_RELOAD || type == ACT_DISASSEMBLE)
  return false;
 return true;
}

SkillLevel& player::skillLevel(std::string ident) {
  return _skills[Skill::skill(ident)];
}

SkillLevel& player::skillLevel(Skill *_skill) {
  return _skills[_skill];
}

SkillLevel player::get_skill_level(Skill *_skill) const
{
    for (std::map<Skill*,SkillLevel>::const_iterator it = _skills.begin();
            it != _skills.end(); ++it) {
        if (it->first == _skill) {
            return it->second;
        }
    }
    return SkillLevel();
}

SkillLevel player::get_skill_level(const std::string &ident) const
{
    Skill *sk = Skill::skill(ident);
    return get_skill_level(sk);
}

void player::copy_skill_levels(const player *rhs)
{
    _skills = rhs->_skills;
}

void player::set_skill_level(Skill* _skill, int level)
{
    skillLevel(_skill).level(level);
}
void player::set_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level);
}

void player::boost_skill_level(Skill* _skill, int level)
{
    skillLevel(_skill).level(level+skillLevel(_skill));
}
void player::boost_skill_level(std::string ident, int level)
{
    skillLevel(ident).level(level+skillLevel(ident));
}

void player::setID (int i)
{
    this->id = i;
}

int player::getID () const
{
    return this->id;
}

bool player::uncanny_dodge(bool is_u)
{
    if( this->power_level < 3 || !this->has_active_bionic("bio_uncanny_dodge") ) { return false; }
    point adjacent = adjacent_tile();
    power_level -= 3;
    if (adjacent.x != posx || adjacent.y != posy)
    {
        posx = adjacent.x;
        posy = adjacent.y;
        if (is_u)
            g->add_msg(_("Time seems to slow down and you instinctively dodge!"));
        else
            g->add_msg(_("Your target dodges... so fast!"));
        return true;
    }
    if (is_u)
        g->add_msg(_("You try to dodge but there's no room!"));
    return false;
}
// adjacent_tile() returns a safe, unoccupied adjacent tile. If there are no such tiles, returns player position instead.
point player::adjacent_tile()
{
    std::vector<point> ret;
    field_entry *cur = NULL;
    field tmpfld;
    trap_id curtrap;
    int dangerous_fields;
    for (int i=posx-1; i <= posx+1; i++)
    {
        for (int j=posy-1; j <= posy+1; j++)
        {
            if (i == posx && j == posy) continue;       // don't consider player position
            curtrap=g->m.tr_at(i, j);
            if (g->mon_at(i, j) == -1 && g->npc_at(i, j) == -1 && g->m.move_cost(i, j) > 0 && (curtrap == tr_null || g->traps[curtrap]->is_benign()))        // only consider tile if unoccupied, passable and has no traps
            {
                dangerous_fields = 0;
                tmpfld = g->m.field_at(i, j);
                for(std::map<field_id, field_entry*>::iterator field_list_it = tmpfld.getFieldStart(); field_list_it != tmpfld.getFieldEnd(); ++field_list_it)
                {
                    cur = field_list_it->second;
                    if (cur != NULL && cur->is_dangerous())
                        dangerous_fields++;
                }
                if (dangerous_fields == 0)
                {
                    ret.push_back(point(i, j));
                }
            }
        }
    }
    if (ret.size())
        return ret[rng(0, ret.size()-1)];   // return a random valid adjacent tile
    else
        return point(posx, posy);           // or return player position if no valid adjacent tiles
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

// Basic logistic function.
double player::logistic(double t)
{
    return 1 / (1 + exp(-t));
}

// Logistic curve [-6,6], flipped and scaled to
// range from 1 to 0 as pos goes from min to max.
double player::logistic_range(int min, int max, int pos)
{
    const double LOGI_CUTOFF = 4;
    const double LOGI_MIN = logistic(-LOGI_CUTOFF);
    const double LOGI_MAX = logistic(+LOGI_CUTOFF);
    const double LOGI_RANGE = LOGI_MAX - LOGI_MIN;
    // Anything beyond [min,max] gets clamped.
    if (pos < min)
    {
        return 1.0;
    }
    else if (pos > max)
    {
        return 0.0;
    }

    // Normalize the pos to [0,1]
    double range = max - min;
    double unit_pos = (pos - min) / range;

    // Scale and flip it to [+LOGI_CUTOFF,-LOGI_CUTOFF]
    double scaled_pos = LOGI_CUTOFF - 2 * LOGI_CUTOFF * unit_pos;

    // Get the raw logistic value.
    double raw_logistic = logistic(scaled_pos);

    // Scale the output to [0,1]
    return (raw_logistic - LOGI_MIN) / LOGI_RANGE;
}

// Calculates portions favoring x, then y, then z
void player::calculate_portions(int &x, int &y, int &z, int maximum)
{
    z = std::min(z, std::max(maximum - x - y, 0));
    y = std::min(y, std::max(maximum - x , 0));
    x = std::min(x, std::max(maximum, 0));
}

void player::environmental_revert_effect()
{
    illness.clear();
    addictions.clear();
    morale.clear();

    for (int part = 0; part < num_hp_parts; part++) {
        g->u.hp_cur[part] = g->u.hp_max[part];
    }
    hunger = 0;
    thirst = 0;
    fatigue = 0;
    health = 0;
    stim = 0;
    pain = 0;
    pkill = 0;
    radiation = 0;

    recalc_sight_limits();
}

bool player::is_invisible() const {
    static const std::string str_bio_cloak("bio_cloak"); // This function used in monster::plan_moves
    static const std::string str_bio_night("bio_night");
    return (
        has_active_bionic(str_bio_cloak) ||
        has_active_bionic(str_bio_night) ||
        has_active_optcloak() ||
        has_artifact_with(AEP_INVISIBLE)
    );
}

int player::visibility( bool, int ) const { // 0-100 %
    if ( is_invisible() ) {
        return 0;
    }
    // todo:
    // if ( dark_clothing() && light check ...
    return 100;
}

void player::set_destination(const std::vector<point> &route)
{
    auto_move_route = route;
}

void player::clear_destination()
{
    auto_move_route.clear();
    next_expected_position.x = -1;
    next_expected_position.y = -1;
}

bool player::has_destination() const
{
    return auto_move_route.size() > 0;
}

std::vector<point> &player::get_auto_move_route()
{
    return auto_move_route;
}

action_id player::get_next_auto_move_direction()
{
    if (!has_destination()) {
        return ACTION_NULL;
    }

    if (next_expected_position.x != -1) {
        if (posx != next_expected_position.x || posy != next_expected_position.y) {
            // We're off course, possibly stumbling or stuck, cancel auto move
            return ACTION_NULL;
        }
    }

    next_expected_position = auto_move_route.front();
    auto_move_route.erase(auto_move_route.begin());

    int dx = next_expected_position.x - posx;
    int dy = next_expected_position.y - posy;

    if (abs(dx) > 1 || abs(dy) > 1) {
        // Should never happen, but check just in case
        return ACTION_NULL;
    }

    return get_movement_direction_from_delta(dx, dy);
}

void player::shift_destination(int shiftx, int shifty)
{
    if (next_expected_position.x != -1) {
        next_expected_position.x += shiftx;
        next_expected_position.y += shifty;
    }

    for (std::vector<point>::iterator it = auto_move_route.begin(); it != auto_move_route.end(); it++) {
        it->x += shiftx;
        it->y += shifty;
    }
}

bool player::has_weapon() {
    return !unarmed_attack();
}

// --- End ---
