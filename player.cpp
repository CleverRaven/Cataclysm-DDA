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

#include "name.h"
#include "cursesdef.h"

nc_color encumb_color(int level);
bool activity_is_suspendable(activity_type type);

player::player()
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
 name = "";
 male = true;
 prof = profession::has_initialized() ? profession::generic() : NULL; //workaround for a potential structural limitation, see player::create
 moves = 100;
 oxygen = 0;
 active_mission = -1;
 in_vehicle = false;
 style_selected = "null";
 xp_pool = 0;
 last_item = itype_id("null");
 for (int i = 0; i < num_skill_types; i++) {
  sklevel[i] = 0;
  skexercise[i] = 0;
  sklearn[i] = true;
 }
 for (int i = 0; i < PF_MAX2; i++)
  my_traits[i] = false;
 for (int i = 0; i < PF_MAX2; i++)
  my_mutations[i] = false;

 mutation_category_level[0] = 5; // Weigh us towards no category for a bit
 for (int i = 1; i < NUM_MUTATION_CATEGORIES; i++)
  mutation_category_level[i] = 0;

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin()++;
      aSkill != Skill::skills.end(); ++aSkill) {
   skillLevel(*aSkill).level(0);
 }

 for (int i = 0; i < num_bp; i++) {
  temp_cur[i] = BODYTEMP_NORM;
  frostbite_timer[i] = 0;
  temp_conv[i] = BODYTEMP_NORM;
 }
}

player::player(const player &rhs)
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
 activity = rhs.activity;
 backlog = rhs.backlog;

 active_missions = rhs.active_missions;
 completed_missions = rhs.completed_missions;
 failed_missions = rhs.failed_missions;
 active_mission = rhs.active_mission;

 name = rhs.name;
 male = rhs.male;
 prof = rhs.prof;

 for (int i = 0; i < PF_MAX2; i++)
  my_traits[i] = rhs.my_traits[i];

 for (int i = 0; i < PF_MAX2; i++)
  my_mutations[i] = rhs.my_mutations[i];

 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  mutation_category_level[i] = rhs.mutation_category_level[i];

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

 for (int i = 0; i < num_hp_parts; i++)
  hp_cur[i] = rhs.hp_cur[i];

 for (int i = 0; i < num_hp_parts; i++)
  hp_max[i] = rhs.hp_max[i];

 for (int i = 0; i < num_bp; i++)
  temp_cur[i] = rhs.temp_cur[i];

 for (int i = 0 ; i < num_bp; i++)
  temp_conv[i] = BODYTEMP_NORM;

 for (int i = 0; i < num_bp; i++)
  frostbite_timer[i] = rhs.frostbite_timer[i];

 morale = rhs.morale;
 xp_pool = rhs.xp_pool;

 for (int i = 0; i < num_skill_types; i++) {
  sklevel[i]    = rhs.sklevel[i];
  skexercise[i] = rhs.skexercise[i];
  sktrain[i]    = rhs.sktrain[i];
  sklearn[i] = rhs.sklearn[i];
 }

 _skills = rhs._skills;

 inv.clear();
 for (int i = 0; i < rhs.inv.size(); i++)
  inv.add_stack(rhs.inv.const_stack(i));

 last_item = rhs.last_item;
 worn = rhs.worn;
 styles = rhs.styles;
 style_selected = rhs.style_selected;
 weapon = rhs.weapon;

 ret_null = rhs.ret_null;

 illness = rhs.illness;
 addictions = rhs.addictions;

 return (*this);
}

void player::normalize(game *g)
{
 ret_null = item(g->itypes["null"], 0);
 weapon   = item(g->itypes["null"], 0);
 style_selected = "null";
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = 60 + str_max * 3;
  if (has_trait(PF_TOUGH))
   hp_max[i] = int(hp_max[i] * 1.2);
  hp_cur[i] = hp_max[i];
 }
 for (int i = 0 ; i < num_bp; i++)
  temp_conv[i] = BODYTEMP_NORM;
}

void player::pick_name() {
  name = Name::generate(male);
}

void player::reset(game *g)
{
// Reset our stats to normal levels
// Any persistent buffs/debuffs will take place in disease.h,
// player::suffer(), etc.
 str_cur = str_max;
 dex_cur = dex_max;
 int_cur = int_max;
 per_cur = per_max;
// We can dodge again!
 dodges_left = 1;
 blocks_left = 1;
// Didn't just pick something up
 last_item = itype_id("null");
// Bionic buffs
 if (has_active_bionic("bio_hydraulics"))
  str_cur += 20;
 if (has_bionic("bio_eye_enhancer"))
  per_cur += 2;
if (has_bionic("bio_metabolics") && power_level < max_power_level &&
     hunger < 100 && (int(g->turn) % 5 == 0)) {
  hunger += 2;
  power_level++;
}

// Trait / mutation buffs
 if (has_trait(PF_THICK_SCALES))
  dex_cur -= 2;
 if (has_trait(PF_CHITIN2) || has_trait(PF_CHITIN3))
  dex_cur--;
 if (has_trait(PF_COMPOUND_EYES) && !wearing_something_on(bp_eyes))
  per_cur++;
 if (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
     has_trait(PF_ARM_TENTACLES_8))
  dex_cur++;
// Pain
 if (pain > pkill) {
  str_cur  -=     int((pain - pkill) / 15);
  dex_cur  -=     int((pain - pkill) / 15);
  per_cur  -=     int((pain - pkill) / 20);
  int_cur  -= 1 + int((pain - pkill) / 25);
 }
// Morale
 if (abs(morale_level()) >= 100) {
  str_cur  += int(morale_level() / 180);
  dex_cur  += int(morale_level() / 200);
  per_cur  += int(morale_level() / 125);
  int_cur  += int(morale_level() / 100);
 }
// Radiation
 if (radiation > 0) {
  str_cur  -= int(radiation / 80);
  dex_cur  -= int(radiation / 110);
  per_cur  -= int(radiation / 100);
  int_cur  -= int(radiation / 120);
 }
// Stimulants
 dex_cur += int(stim / 10);
 per_cur += int(stim /  7);
 int_cur += int(stim /  6);
 if (stim >= 30) {
  dex_cur -= int(abs(stim - 15) /  8);
  per_cur -= int(abs(stim - 15) / 12);
  int_cur -= int(abs(stim - 15) / 14);
 }

// Set our scent towards the norm
 int norm_scent = 500;
 if (has_trait(PF_SMELLY))
  norm_scent = 800;
 if (has_trait(PF_SMELLY2))
  norm_scent = 1200;

 // Scent increases fast at first, and slows down as it approaches normal levels.
 // Estimate it will take about norm_scent * 2 turns to go from 0 - norm_scent / 2
 // Without smelly trait this is about 1.5 hrs. Slows down significantly after that.
 if (scent < rng(0, norm_scent))
   scent++;

 // Unusually high scent decreases steadily until it reaches normal levels.
 if (scent > norm_scent)
  scent--;

// Give us our movement points for the turn.
 moves += current_speed(g);

// Floor for our stats.  No stat changes should occur after this!
 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;

 int mor = morale_level();
 int xp_frequency = 10 - int(mor / 20);
 if (xp_frequency < 1)
  xp_frequency = 1;
 if (int(g->turn) % xp_frequency == 0)
  xp_pool++;

 if (xp_pool > 800)
  xp_pool = 800;
}

void player::update_morale()
{
 for (int i = 0; i < morale.size(); i++) {
  if (morale[i].bonus < 0)
   morale[i].bonus++;
  else if (morale[i].bonus > 0)
   morale[i].bonus--;

  if (morale[i].bonus == 0) {
   morale.erase(morale.begin() + i);
   i--;
  }
 }
}

/* Here lies the intended effects of body temperature

Assumption 1 : a naked person is comfortable at 31C/87.8F.
Assumption 2 : a "lightly clothed" person is comfortable at 25C/77F.
Assumption 3 : frostbite cannot happen above 0C temperature.*
* In the current model, a naked person can get frostbite at 1C. This isn't true, but it's a compromise with using nice whole numbers.

Here is a list of warmth values and the corresponding temperatures in which the player is comfortable, and in which the player is very cold.

Warmth  Temperature (Comfortable)    Temperature (Very cold)    Notes
0        31C /  87.8F                 1C /  33.8F               * Naked
10       25C /  77.0F                -5C /  23.0F               * Lightly clothed
20       19C /  66.2F               -11C /  12.2F
30       13C /  55.4F               -17C /   1.4F
40        7C /  44.6F               -23C /  -9.4F
50        1C /  33.8F               -29C / -20.2F
60       -5C /  23.0F               -35C / -31.0F
70      -11C /  12.2F               -41C / -41.8F
80      -17C /   1.4F               -47C / -52.6F
90      -23C /  -9.4F               -53C / -63.4F
100     -29C / -20.2F               -59C / -74.2F

*/

void player::update_bodytemp(game *g) // TODO bionics, diseases and humidity (not in yet) can affect body temp.
{
 // NOTE : visit weather.h for some details on the numbers used
 // Converts temperature to Celsius/10(Wito plans on using degrees Kelvin later)
 int Ctemperature = 100*(g->temperature - 32) * 5/9;
 // Temperature norms
 const int ambient_norm = 3100;
 // This adjusts the temperature scale to match the bodytemp scale
 int adjusted_temp = (Ctemperature - ambient_norm);
 // Creative thinking for clean morale penalties: this gets incremented in the for loop and applied after the loop
 int morale_pen = 0;
 // Fetch the morale value of wetness for bodywetness
 int bodywetness = 0;
 for (int i = 0; bodywetness == 0 && i < morale.size(); i++)
  if( morale[i].type == MORALE_WET ) {
   bodywetness = abs(morale[i].bonus); // Make it positive, less confusing
   break;
  }
 // Current temperature and converging temperature calculations
 for (int i = 0 ; i < num_bp ; i++){
  // CONDITIONS TO SKIP OVER BODY TEMPERATURE CALCULATION
  // Eyes
  if (i == bp_eyes)
   { temp_conv[i] = temp_cur[i] = BODYTEMP_NORM; continue; }
  // Mutations
  if (i == bp_hands && (has_trait(PF_TALONS) || has_trait(PF_WEBBED)))
   {temp_conv[i] = temp_cur[i] = BODYTEMP_NORM; continue;}
  if (i == bp_mouth && has_trait(PF_BEAK))
   {temp_conv[i] = temp_cur[i] = BODYTEMP_NORM; continue;}
  if (i == bp_feet && has_trait(PF_HOOVES))
   {temp_conv[i] = temp_cur[i] = BODYTEMP_NORM; continue;}
  if (i == bp_torso && has_trait(PF_SHELL))
   {temp_conv[i] = temp_cur[i] = BODYTEMP_NORM; continue;}
  if (i == bp_head && has_trait(PF_HORNS_CURLED))
   {temp_conv[i] = temp_cur[i] = BODYTEMP_NORM; continue;}
  // Represents the fact that the body generates heat when it is cold. TODO : should this increase hunger?
  float homeostasis_adjustement = (temp_cur[i] > BODYTEMP_NORM ? 40.0 : 60.0);
  int clothing_warmth_adjustement = homeostasis_adjustement * (float)warmth(body_part(i)) * (1.0 - (float)bodywetness / 100.0);
  // Disease name shorthand
  int blister_pen = dis_type(DI_BLISTERS) + 1 + i, hot_pen  = dis_type(DI_HOT) + 1 + i;
  int cold_pen = dis_type(DI_COLD)+ 1 + i, frost_pen = dis_type(DI_FROSTBITE) + 1 + i;
  // Convergeant temperature is affected by ambient temperature, clothing warmth, and body wetness.
  temp_conv[i] = BODYTEMP_NORM + adjusted_temp + clothing_warmth_adjustement;
  // HUNGER
  temp_conv[i] -= 2*(hunger + 100);
  // FATIGUE
  if (!has_disease(DI_SLEEP)) temp_conv[i] -= 2*fatigue;
  else {
   int vpart = -1;
   vehicle *veh = g->m.veh_at (posx, posy, vpart);
   if      (g->m.ter(posx, posy) == t_bed)                        temp_conv[i] += 1000;
   else if (g->m.ter(posx, posy) == t_makeshift_bed)              temp_conv[i] +=  500;
   else if (g->m.tr_at(posx, posy) == tr_cot)                     temp_conv[i] -=  500;
   else if (g->m.tr_at(posx, posy) == tr_rollmat)                 temp_conv[i] -= 1000;
   else if (veh && veh->part_with_feature (vpart, vpf_seat) >= 0) temp_conv[i] +=  200;
   else if (veh && veh->part_with_feature (vpart, vpf_bed) >= 0)  temp_conv[i] +=  300;
   else	temp_conv[i] -= 2000;
  }
  // CONVECTION HEAT SOURCES (generates body heat, helps fight frostbite)
  int blister_count = 0; // If the counter is high, your skin starts to burn
  for (int j = -6 ; j <= 6 ; j++){
   for (int k = -6 ; k <= 6 ; k++){
    int heat_intensity = 0;
    if(g->m.field_at(posx + j, posy + k).type == fd_fire)
     heat_intensity = g->m.field_at(posx + j, posy + k).density;
    else if (g->m.tr_at(posx + j, posy + k) == tr_lava )
      heat_intensity = 3;
    if (heat_intensity > 0 && g->u_see(posx + j, posy + k)) {
     // Ensure fire_dist >=1 to avoid divide-by-zero errors.
     int fire_dist = std::max(1, std::max(j, k));
     if (frostbite_timer[i] > 0) frostbite_timer[i] -= heat_intensity - fire_dist / 2;
     temp_conv[i] +=  200 * heat_intensity * heat_intensity / (fire_dist * fire_dist);
     blister_count += heat_intensity / (fire_dist * fire_dist);
    }
   }
  }
  // TILES (it is very dangerous to be on fire for long, as it directly increases body temperature, not temp_conv)
  if (has_disease(DI_ONFIRE)) temp_cur[i] += 250;
  if ((g->m.field_at(posx, posy).type == fd_fire && g->m.field_at(posx, posy).density > 2) || g->m.tr_at(posx, posy) == tr_lava) temp_cur[i] += 250;
  // WEATHER
  if (g->weather == WEATHER_SUNNY && g->is_in_sunlight(posx, posy)) temp_conv[i] += 1000;
  if (g->weather == WEATHER_CLEAR && g->is_in_sunlight(posx, posy)) temp_conv[i] += 500;
  // DISEASES
  if (has_disease(DI_FLU) && i == bp_head) temp_conv[i] += 1500;
  if (has_disease(DI_COMMON_COLD)) temp_conv[i] -= 750;
  // BIONICS
  // Bionic "Internal Climate Control" says it eases the effects of high and low ambient temps
  const int variation = BODYTEMP_NORM*0.5;
  if (has_bionic("bio_climate") && temp_conv[i] < BODYTEMP_SCORCHING + variation && temp_conv[i] > BODYTEMP_FREEZING - variation){
   if      (temp_conv[i] > BODYTEMP_SCORCHING) temp_conv[i] = BODYTEMP_VERY_HOT;
   else if (temp_conv[i] > BODYTEMP_VERY_HOT)  temp_conv[i] = BODYTEMP_HOT;
   else if (temp_conv[i] > BODYTEMP_HOT)       temp_conv[i] = BODYTEMP_NORM;
   else if (temp_conv[i] < BODYTEMP_FREEZING)  temp_conv[i] = BODYTEMP_VERY_COLD;
   else if (temp_conv[i] < BODYTEMP_VERY_COLD) temp_conv[i] = BODYTEMP_COLD;
   else if (temp_conv[i] < BODYTEMP_COLD)      temp_conv[i] = BODYTEMP_NORM;
   }
  // Bionic "Thermal Dissapation" says it prevents fire damage up to 2000F. 500 is picked at random...
  if (has_bionic("bio_heatsink") && blister_count < 500)
   blister_count = (has_trait(PF_BARK) ? -100 : 0);
  // BLISTERS : Skin gets blisters from intense heat exposure.
  if (blister_count - 10*resist(body_part(i)) > 20)
   add_disease(dis_type(blister_pen), 1, g);
  // BLOOD LOSS : Loss of blood results in loss of body heat
  int blood_loss = 0;
  if      (i == bp_legs)  blood_loss = (100 - 100*(hp_cur[hp_leg_l] + hp_cur[hp_leg_r]) / (hp_max[hp_leg_l] + hp_max[hp_leg_r]));
  else if (i == bp_arms)  blood_loss = (100 - 100*(hp_cur[hp_arm_l] + hp_cur[hp_arm_r]) / (hp_max[hp_arm_l] + hp_max[hp_arm_r]));
  else if (i == bp_torso) blood_loss = (100 - 100* hp_cur[hp_torso] / hp_max[hp_torso]);
  else if (i == bp_head)  blood_loss = (100 - 100* hp_cur[hp_head] / hp_max[hp_head]);
  temp_conv[i] -= blood_loss*temp_conv[i]/200; // 1% bodyheat lost per 2% hp lost
  // EQUALIZATION
  switch (i){
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
  // MUTATIONS
  // Bark : lowers blister count to -100; harder to get blisters
  // Lightly furred
  if (has_trait(PF_LIGHTFUR)) temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 250 : 500);
  // Furry
  if (has_trait(PF_FUR)) temp_conv[i] += (temp_cur[i] > BODYTEMP_NORM ? 750 : 1500);
  // FINAL CALCULATION : Increments current body temperature towards convergant.
  int temp_before = temp_cur[i];
  int temp_difference = temp_cur[i] - temp_conv[i]; // Negative if the player is warming up.
  // exp(-0.001) : half life of 60 minutes, exp(-0.002) : half life of 30 minutes, exp(-0.003) : half life of 20 minutes, exp(-0.004) : half life of 15 minutes
  int rounding_error = 0;
  if (temp_difference < 0 && temp_difference > -600 ) rounding_error = 1; // If temp_diff is small, the player cannot warm up due to rounding errors. This fixes that.
  if (temp_cur[i] != temp_conv[i]) {
   if      ((g->m.ter(posx, posy) == t_water_sh || g->m.ter(posx, posy) == t_sewage)
              && (i == bp_feet || i == bp_legs)) temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
   else if (g->m.ter(posx, posy) == t_water_dp)  temp_cur[i] = temp_difference*exp(-0.004) + temp_conv[i] + rounding_error;
   else if (i == bp_torso || i == bp_head)       temp_cur[i] = temp_difference*exp(-0.003) + temp_conv[i] + rounding_error;
   else                                          temp_cur[i] = temp_difference*exp(-0.002) + temp_conv[i] + rounding_error;}
  int temp_after = temp_cur[i];
  // PENALTIES
  if      (temp_cur[i] < BODYTEMP_FREEZING)  {add_disease(dis_type(cold_pen), 1, g, 3, 3); frostbite_timer[i] += 3;}
  else if (temp_cur[i] < BODYTEMP_VERY_COLD) {add_disease(dis_type(cold_pen), 1, g, 2, 3); frostbite_timer[i] += 2;}
  else if (temp_cur[i] < BODYTEMP_COLD)      {add_disease(dis_type(cold_pen), 1, g, 1, 3); frostbite_timer[i] += 1;} // Frostbite timer does not go down if you are still cold.
  else if (temp_cur[i] > BODYTEMP_SCORCHING) {add_disease(dis_type(hot_pen),  1, g, 3, 3); } // If body temp rises over 15000, disease.cpp (DI_HOT_HEAD) acts weird and the player will die
  else if (temp_cur[i] > BODYTEMP_VERY_HOT)  {add_disease(dis_type(hot_pen),  1, g, 2, 3); }
  else if (temp_cur[i] > BODYTEMP_HOT)       {add_disease(dis_type(hot_pen),  1, g, 1, 3); }
  // MORALE : a negative morale_pen means the player is cold
  // Intensity multiplier is negative for cold, positive for hot
  int intensity_mult = -disease_intensity(dis_type(cold_pen)) + disease_intensity(dis_type(hot_pen));
  if (has_disease(dis_type(cold_pen)) || has_disease(dis_type(hot_pen))) {
   switch (i) {
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
  if      (frostbite_timer[i] >   0) frostbite_timer[i]--;
  if      (frostbite_timer[i] >= 240 && g->temperature < 32) {
   if      (disease_intensity(dis_type(frost_pen)) < 2 &&  i == bp_mouth)                  g->add_msg("Your %s hardens from the frostbite!", body_part_name(body_part(i), -1).c_str());
   else if (disease_intensity(dis_type(frost_pen)) < 2 && (i == bp_hands || i == bp_feet)) g->add_msg("Your %s harden from the frostbite!",  body_part_name(body_part(i), -1).c_str());
   add_disease(dis_type(frost_pen), 1, g, 2, 2);}
  else if (frostbite_timer[i] >= 120 && g->temperature < 32) {
   if (!has_disease(dis_type(frost_pen))) g->add_msg("You lose sensation in your %s.", body_part_name(body_part(i), -1).c_str());
   add_disease(dis_type(frost_pen), 1, g, 1, 2);}
  // Warn the player if condition worsens
  if      (temp_before > BODYTEMP_FREEZING  && temp_after < BODYTEMP_FREEZING)  g->add_msg("You feel your %s beginning to go numb from the cold!", body_part_name(body_part(i), -1).c_str());
  else if (temp_before > BODYTEMP_VERY_COLD && temp_after < BODYTEMP_VERY_COLD) g->add_msg("You feel your %s getting very cold.", body_part_name(body_part(i), -1).c_str());
  else if (temp_before > BODYTEMP_COLD      && temp_after < BODYTEMP_COLD)      g->add_msg("You feel your %s getting cold.", body_part_name(body_part(i), -1).c_str());
  else if (temp_before < BODYTEMP_SCORCHING && temp_after > BODYTEMP_SCORCHING) g->add_msg("You feel your %s getting red hot from the heat!", body_part_name(body_part(i), -1).c_str());
  else if (temp_before < BODYTEMP_VERY_HOT  && temp_after > BODYTEMP_VERY_HOT)  g->add_msg("You feel your %s getting very hot.", body_part_name(body_part(i), -1).c_str());
  else if (temp_before < BODYTEMP_HOT       && temp_after > BODYTEMP_HOT)       g->add_msg("You feel your %s getting hot.", body_part_name(body_part(i), -1).c_str());
  }
 // Morale penalties, updated at the same rate morale is
 if (morale_pen < 0 && int(g->turn) % 10 == 0) add_morale(MORALE_COLD, -2, -abs(morale_pen));
 if (morale_pen > 0 && int(g->turn) % 10 == 0) add_morale(MORALE_HOT,  -2, -abs(morale_pen));
}

void player::temp_equalizer(body_part bp1, body_part bp2)
{
 // Body heat is moved around.
 // When bp1 gives 15%, bp2 will return 15% of the _new_ difference
 int diff = (temp_conv[bp2] - temp_conv[bp1])*0.15; // If bp1 is warmer, it will lose heat
 temp_conv[bp1] += diff;
}

int player::current_speed(game *g)
{
 int newmoves = 100; // Start with 100 movement points...
// Minus some for weight...
 int carry_penalty = 0;
 if (weight_carried() > int(weight_capacity() * .25))
  carry_penalty = 75 * double((weight_carried() - int(weight_capacity() * .25))/
                              (weight_capacity() * .75));

 newmoves -= carry_penalty;

 if (pain > pkill) {
  int pain_penalty = int((pain - pkill) * .7);
  if (pain_penalty > 60)
   pain_penalty = 60;
  newmoves -= pain_penalty;
 }
 if (pkill >= 10) {
  int pkill_penalty = int(pkill * .1);
  if (pkill_penalty > 30)
   pkill_penalty = 30;
  newmoves -= pkill_penalty;
 }

 if (abs(morale_level()) >= 100) {
  int morale_bonus = int(morale_level() / 25);
  if (morale_bonus < -10)
   morale_bonus = -10;
  else if (morale_bonus > 10)
   morale_bonus = 10;
  newmoves += morale_bonus;
 }

 if (radiation >= 40) {
  int rad_penalty = radiation / 40;
  if (rad_penalty > 20)
   rad_penalty = 20;
  newmoves -= rad_penalty;
 }

 if (thirst > 40)
  newmoves -= int((thirst - 40) / 10);
 if (hunger > 100)
  newmoves -= int((hunger - 100) / 10);

 newmoves += (stim > 40 ? 40 : stim);

 for (int i = 0; i < illness.size(); i++)
  newmoves += disease_speed_boost(illness[i]);

 if (has_trait(PF_QUICK))
  newmoves = int(newmoves * 1.10);

 if (g != NULL) {
  if (has_trait(PF_SUNLIGHT_DEPENDENT) && !g->is_in_sunlight(posx, posy))
   newmoves -= (g->light_level() >= 12 ? 5 : 10);
  if (has_trait(PF_COLDBLOOD3) && g->temperature < 60)
   newmoves -= int( (65 - g->temperature) / 2);
  else if (has_trait(PF_COLDBLOOD2) && g->temperature < 60)
   newmoves -= int( (65 - g->temperature) / 3);
  else if (has_trait(PF_COLDBLOOD) && g->temperature < 60)
   newmoves -= int( (65 - g->temperature) / 5);
 }

 if (has_artifact_with(AEP_SPEED_UP))
  newmoves += 20;
 if (has_artifact_with(AEP_SPEED_DOWN))
  newmoves -= 20;

 if (newmoves < 25)
  newmoves = 25;

 return newmoves;
}

int player::run_cost(int base_cost)
{
 int movecost = base_cost;
 if (has_trait(PF_PARKOUR) && base_cost > 100) {
  movecost *= .5;
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

 if (has_trait(PF_FLEET) && base_cost == 100)
  movecost = int(movecost * .85);
 if (has_trait(PF_FLEET2) && base_cost == 100)
  movecost = int(movecost * .7);
 if (has_trait(PF_PADDED_FEET) && !wearing_something_on(bp_feet))
  movecost = int(movecost * .9);
 if (has_trait(PF_LIGHT_BONES))
  movecost = int(movecost * .9);
 if (has_trait(PF_HOLLOW_BONES))
  movecost = int(movecost * .8);
 if (has_trait(PF_WINGS_INSECT))
  movecost -= 15;
 if (has_trait(PF_LEG_TENTACLES))
  movecost += 20;
 if (has_trait(PF_PONDEROUS1))
  movecost = int(movecost * 1.1);
 if (has_trait(PF_PONDEROUS2))
  movecost = int(movecost * 1.2);
 if (has_trait(PF_PONDEROUS3))
  movecost = int(movecost * 1.3);
 movecost += encumb(bp_feet) * 5 + encumb(bp_legs) * 3;
 if (!wearing_something_on(bp_feet) && !has_trait(PF_PADDED_FEET) &&
     !has_trait(PF_HOOVES))
  movecost += 15;

 return movecost;
}

int player::swim_speed()
{
  int ret = 440 + 2 * weight_carried() - 50 * skillLevel("swimming");
 if (has_trait(PF_WEBBED))
  ret -= 60 + str_cur * 5;
 if (has_trait(PF_TAIL_FIN))
  ret -= 100 + str_cur * 10;
 if (has_trait(PF_SLEEK_SCALES))
  ret -= 100;
 if (has_trait(PF_LEG_TENTACLES))
  ret -= 60;
 ret += (50 - skillLevel("swimming") * 2) * abs(encumb(bp_legs));
 ret += (80 - skillLevel("swimming") * 3) * abs(encumb(bp_torso));
 if (skillLevel("swimming") < 10) {
  for (int i = 0; i < worn.size(); i++)
    ret += (worn[i].volume() * (10 - skillLevel("swimming"))) / 2;
 }
 ret -= str_cur * 6 + dex_cur * 4;
// If (ret > 500), we can not swim; so do not apply the underwater bonus.
 if (underwater && ret < 500)
  ret -= 50;
 if (ret < 30)
  ret = 30;
 return ret;
}

nc_color player::color()
{
 if (has_disease(DI_ONFIRE))
  return c_red;
 if (has_disease(DI_STUNNED))
  return c_ltblue;
 if (has_disease(DI_BOOMERED))
  return c_pink;
 if (underwater)
  return c_blue;
 if (has_active_bionic("bio_cloak") || has_artifact_with(AEP_INVISIBLE))
  return c_dkgray;
 return c_white;
}

void player::load_info(game *g, std::string data)
{
 std::stringstream dump;
 dump << data;
 int inveh;
 itype_id styletmp;
 std::string prof_ident;

 dump >> posx >> posy >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> power_level >>
         max_power_level >> hunger >> thirst >> fatigue >> stim >>
         pain >> pkill >> radiation >> cash >> recoil >> driving_recoil >>
         inveh >> scent >> moves >> underwater >> dodges_left >> blocks_left >>
         oxygen >> active_mission >> xp_pool >> male >> prof_ident >> health >>
         styletmp;

 if (profession::exists(prof_ident)) {
  prof = profession::prof(prof_ident);
 } else {
  prof = profession::generic();
  debugmsg("Tried to use non-existent profession '%s'", prof_ident.c_str());
 }

 activity.load_info(dump);
 backlog.load_info(dump);

 in_vehicle = inveh != 0;
 style_selected = styletmp;

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_traits[i];

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_mutations[i];

 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  dump >> mutation_category_level[i];

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (int i = 0; i < num_bp; i++)
  dump >> temp_cur[i] >> frostbite_timer[i];

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
   dump >> skillLevel(*aSkill);
 }

 int num_recipes;
 std::string rec_name;
 dump >> num_recipes;
 for (int i = 0; i < num_recipes; ++i)
 {
  dump >> rec_name;
  learned_recipes[rec_name] = g->recipe_by_name(rec_name);
 }

 int numstyles;
 itype_id styletype;
 dump >> numstyles;
 for (int i = 0; i < numstyles; i++) {
  dump >> styletype;
  styles.push_back( styletype );
 }

 int numill;
 int typetmp;
 disease illtmp;
 dump >> numill;
 for (int i = 0; i < numill; i++) {
  dump >> typetmp >> illtmp.duration;
  illtmp.type = dis_type(typetmp);
  illness.push_back(illtmp);
 }

 int numadd = 0;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }

 int numbio = 0;
 bionic_id biotype;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> biotype >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = biotype;
  my_bionics.push_back(biotmp);
 }

 int nummor;
 morale_point mortmp;
 dump >> nummor;
 for (int i = 0; i < nummor; i++) {
  int mortype;
  std::string item_id;
  dump >> mortmp.bonus >> mortype >> item_id;
  mortmp.type = morale_type(mortype);
  if (g->itypes.find(item_id) != g->itypes.end())
   mortmp.item_type = NULL;
  else
   mortmp.item_type = g->itypes[item_id];
  morale.push_back(mortmp);
 }

 int nummis = 0;
 int mistmp;
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  active_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  completed_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  failed_missions.push_back(mistmp);
 }
}

std::string player::save_info()
{
 std::stringstream dump;
 dump << posx    << " " << posy    << " " << str_cur << " " << str_max << " " <<
         dex_cur << " " << dex_max << " " << int_cur << " " << int_max << " " <<
         per_cur << " " << per_max << " " << power_level << " " <<
         max_power_level << " " << hunger << " " << thirst << " " << fatigue <<
         " " << stim << " " << pain << " " << pkill << " " << radiation <<
         " " << cash << " " << recoil << " " << driving_recoil << " " <<
         (in_vehicle? 1 : 0) << " " << scent << " " << moves << " " <<
         underwater << " " << dodges_left << " " << blocks_left << " " <<
         oxygen << " " << active_mission << " " << xp_pool << " " << male <<
         " " << prof->ident() << " " << health << " " << style_selected <<
         " " << activity.save_info() << " " << backlog.save_info() << " ";

 for (int i = 0; i < PF_MAX2; i++)
  dump << my_traits[i] << " ";
 for (int i = 0; i < PF_MAX2; i++)
  dump << my_mutations[i] << " ";
 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  dump << mutation_category_level[i] << " ";
 for (int i = 0; i < num_hp_parts; i++)
  dump << hp_cur[i] << " " << hp_max[i] << " ";
 for (int i = 0; i < num_bp; i++)
  dump << temp_cur[i] << " " << frostbite_timer[i] << " ";

 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin(); aSkill != Skill::skills.end(); ++aSkill) {
   SkillLevel level = skillLevel(*aSkill);
   dump << level;
 }

 dump << learned_recipes.size() << " ";
 for (std::map<std::string, recipe*>::iterator iter = learned_recipes.begin();
      iter != learned_recipes.end();
      ++iter)
 {
  dump << iter->first << " ";
 }

 dump << styles.size() << " ";
 for (int i = 0; i < styles.size(); i++)
  dump << styles[i] << " ";

 dump << illness.size() << " ";
 for (int i = 0; i < illness.size();  i++)
  dump << int(illness[i].type) << " " << illness[i].duration << " ";

 dump << addictions.size() << " ";
 for (int i = 0; i < addictions.size(); i++)
  dump << int(addictions[i].type) << " " << addictions[i].intensity << " " <<
          addictions[i].sated << " ";

 dump << my_bionics.size() << " ";
 for (int i = 0; i < my_bionics.size(); i++)
  dump << my_bionics[i].id << " " << my_bionics[i].invlet << " " <<
          my_bionics[i].powered << " " << my_bionics[i].charge << " ";

 dump << morale.size() << " ";
 for (int i = 0; i < morale.size(); i++) {
  dump << morale[i].bonus << " " << morale[i].type << " ";
  if (morale[i].item_type == NULL)
   dump << "0";
  else
   dump << morale[i].item_type->id;
  dump << " ";
 }

 dump << " " << active_missions.size() << " ";
 for (int i = 0; i < active_missions.size(); i++)
  dump << active_missions[i] << " ";

 dump << " " << completed_missions.size() << " ";
 for (int i = 0; i < completed_missions.size(); i++)
  dump << completed_missions[i] << " ";

 dump << " " << failed_missions.size() << " ";
 for (int i = 0; i < failed_missions.size(); i++)
  dump << failed_missions[i] << " ";

 dump << std::endl;

 dump << inv.save_str_no_quant();

 for (int i = 0; i < worn.size(); i++)
  dump << "W " << worn[i].save_info() << std::endl;
 if (!weapon.is_null())
  dump << "w " << weapon.save_info() << std::endl;
 for (int j = 0; j < weapon.contents.size(); j++)
  dump << "c " << weapon.contents[j].save_info() << std::endl;

 return dump.str();
}

void player::disp_info(game *g)
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
 if (abs(morale_level()) >= 100) {
  bool pos = (morale_level() > 0);
  effect_name.push_back(pos ? "Elated" : "Depressed");
  std::stringstream morale_text;
  if (abs(morale_level()) >= 200)
   morale_text << "Dexterity" << (pos ? " +" : " ") <<
                   int(morale_level() / 200) << "   ";
  if (abs(morale_level()) >= 180)
   morale_text << "Strength" << (pos ? " +" : " ") <<
                  int(morale_level() / 180) << "   ";
  if (abs(morale_level()) >= 125)
   morale_text << "Perception" << (pos ? " +" : " ") <<
                  int(morale_level() / 125) << "   ";
  morale_text << "Intelligence" << (pos ? " +" : " ") <<
                 int(morale_level() / 100) << "   ";
  effect_text.push_back(morale_text.str());
 }
 if (pain - pkill > 0) {
  effect_name.push_back("Pain");
  std::stringstream pain_text;
  if (pain - pkill >= 15)
   pain_text << "Strength -" << int((pain - pkill) / 15) << "   Dexterity -" <<
                int((pain - pkill) / 15) << "   ";
  if (pain - pkill >= 20)
   pain_text << "Perception -" << int((pain - pkill) / 15) << "   ";
  pain_text << "Intelligence -" << 1 + int((pain - pkill) / 25);
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
   effect_name.push_back("Stimulant Overdose");
  else
   effect_name.push_back("Stimulant");
  std::stringstream stim_text;
  stim_text << "Speed +" << stim << "   Intelligence " <<
               (intbonus > 0 ? "+ " : "") << intbonus << "   Perception " <<
               (perbonus > 0 ? "+ " : "") << perbonus << "   Dexterity "  <<
               (dexbonus > 0 ? "+ " : "") << dexbonus;
  effect_text.push_back(stim_text.str());
 } else if (stim < 0) {
  effect_name.push_back("Depressants");
  std::stringstream stim_text;
  int dexpen = int(stim / 10);
  int perpen = int(stim /  7);
  int intpen = int(stim /  6);
// Since dexpen etc. are always less than 0, no need for + signs
  stim_text << "Speed " << stim << "   Intelligence " << intpen <<
               "   Perception " << perpen << "   Dexterity " << dexpen;
  effect_text.push_back(stim_text.str());
 }

 if ((has_trait(PF_TROGLO) && g->is_in_sunlight(posx, posy) &&
      g->weather == WEATHER_SUNNY) ||
     (has_trait(PF_TROGLO2) && g->is_in_sunlight(posx, posy) &&
      g->weather != WEATHER_SUNNY)) {
  effect_name.push_back("In Sunlight");
  effect_text.push_back("The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Dexterity - 1");
 } else if (has_trait(PF_TROGLO2) && g->is_in_sunlight(posx, posy)) {
  effect_name.push_back("In Sunlight");
  effect_text.push_back("The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Dexterity - 2");
 } else if (has_trait(PF_TROGLO3) && g->is_in_sunlight(posx, posy)) {
  effect_name.push_back("In Sunlight");
  effect_text.push_back("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Dexterity - 4");
 }

 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].sated < 0 &&
      addictions[i].intensity >= MIN_ADDICTION_LEVEL) {
   effect_name.push_back(addiction_name(addictions[i]));
   effect_text.push_back(addiction_text(addictions[i]));
  }
 }

 int maxy = (VIEWY*2)+1;
 if (maxy < 25)
  maxy = 25;

 int effect_win_size_y = 0;
 int trait_win_size_y = 0;
 int skill_win_size_y = 0;
 int infooffsetytop = 11;
 int infooffsetybottom = 15;
 std::vector<pl_flag> traitslist;

 effect_win_size_y = effect_name.size()+1;

 for(int i = 0; i < PF_MAX2; i++) {
  if(my_traits[i]) {
   traitslist.push_back(pl_flag(i));
  }
 }

 trait_win_size_y = traitslist.size()+1;
 if (trait_win_size_y + infooffsetybottom > maxy ) {
  trait_win_size_y = maxy - infooffsetybottom;
 }

 skill_win_size_y = num_skill_types;
 if (skill_win_size_y + infooffsetybottom > maxy ) {
  skill_win_size_y = maxy - infooffsetybottom;
 }
/*
 std::stringstream ssTemp;
 ssTemp  << skill_win_size_y << " - " << trait_win_size_y << " - " << effect_win_size_y;
 debugmsg((ssTemp.str()).c_str());
*/
 WINDOW* w_grid_top    = newwin(infooffsetybottom, 81,  VIEW_OFFSET_Y,  VIEW_OFFSET_X);
 WINDOW* w_grid_skill  = newwin(skill_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
 WINDOW* w_grid_trait  = newwin(trait_win_size_y + 1, 27, infooffsetybottom + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
 WINDOW* w_grid_effect = newwin(effect_win_size_y+ 1, 28, infooffsetybottom + VIEW_OFFSET_Y, 53 + VIEW_OFFSET_X);

 WINDOW* w_tip     = newwin(1, 80,  VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
 WINDOW* w_stats   = newwin(9, 26,  1 + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);
 WINDOW* w_traits  = newwin(trait_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y,  27 + VIEW_OFFSET_X);
 WINDOW* w_encumb  = newwin(9, 26,  1 + VIEW_OFFSET_Y, 27 + VIEW_OFFSET_X);
 WINDOW* w_effects = newwin(effect_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
 WINDOW* w_speed   = newwin(9, 26,  1 + VIEW_OFFSET_Y, 54 + VIEW_OFFSET_X);
 WINDOW* w_skills  = newwin(skill_win_size_y, 26, infooffsetybottom + VIEW_OFFSET_Y, 0 + VIEW_OFFSET_X);
 WINDOW* w_info    = newwin(3, 80, infooffsetytop + VIEW_OFFSET_Y,  0 + VIEW_OFFSET_X);

 for (int i = 0; i < 81; i++) {
  //Horizontal line top grid
  mvwputch(w_grid_top, 10, i, c_ltgray, LINE_OXOX);
  mvwputch(w_grid_top, 14, i, c_ltgray, LINE_OXOX);

  //Vertical line top grid
  if (i <= infooffsetybottom) {
   mvwputch(w_grid_top, i, 26, c_ltgray, LINE_XOXO);
   mvwputch(w_grid_top, i, 53, c_ltgray, LINE_XOXO);
   mvwputch(w_grid_top, i, 80, c_ltgray, LINE_XOXO);
  }

  //Horizontal line skills
  if (i <= 26) {
   mvwputch(w_grid_skill, skill_win_size_y, i, c_ltgray, LINE_OXOX);
  }

  //Vertical line skills
  if (i <= skill_win_size_y) {
   mvwputch(w_grid_skill, i, 26, c_ltgray, LINE_XOXO);
  }

  //Horizontal line traits
  if (i <= 26) {
   mvwputch(w_grid_trait, trait_win_size_y, i, c_ltgray, LINE_OXOX);
  }

  //Vertical line traits
  if (i <= trait_win_size_y) {
   mvwputch(w_grid_trait, i, 26, c_ltgray, LINE_XOXO);
  }

  //Horizontal line effects
  if (i <= 27) {
   mvwputch(w_grid_effect, effect_win_size_y, i, c_ltgray, LINE_OXOX);
  }

  //Vertical line effects
  if (i <= effect_win_size_y) {
   mvwputch(w_grid_effect, i, 0, c_ltgray, LINE_XOXO);
   mvwputch(w_grid_effect, i, 27, c_ltgray, LINE_XOXO);
  }
 }

 //Intersections top grid
 mvwputch(w_grid_top, 14, 26, c_ltgray, LINE_OXXX); // T
 mvwputch(w_grid_top, 14, 53, c_ltgray, LINE_OXXX); // T
 mvwputch(w_grid_top, 10, 26, c_ltgray, LINE_XXOX); // _|_
 mvwputch(w_grid_top, 10, 53, c_ltgray, LINE_XXOX); // _|_
 mvwputch(w_grid_top, 10, 80, c_ltgray, LINE_XOXX); // -|
 mvwputch(w_grid_top, 14, 80, c_ltgray, LINE_XOXX); // -|
 wrefresh(w_grid_top);

 mvwputch(w_grid_skill, skill_win_size_y, 26, c_ltgray, LINE_XOOX); // _|

 if (skill_win_size_y > trait_win_size_y)
  mvwputch(w_grid_skill, trait_win_size_y, 26, c_ltgray, LINE_XXXO); // |-
 else if (skill_win_size_y == trait_win_size_y)
  mvwputch(w_grid_skill, trait_win_size_y, 26, c_ltgray, LINE_XXOX); // _|_

 mvwputch(w_grid_trait, trait_win_size_y, 26, c_ltgray, LINE_XOOX); // _|

 if (trait_win_size_y > effect_win_size_y)
  mvwputch(w_grid_trait, effect_win_size_y, 26, c_ltgray, LINE_XXXO); // |-
 else if (trait_win_size_y == effect_win_size_y)
  mvwputch(w_grid_trait, effect_win_size_y, 26, c_ltgray, LINE_XXOX); // _|_
 else if (trait_win_size_y < effect_win_size_y) {
  mvwputch(w_grid_trait, trait_win_size_y, 26, c_ltgray, LINE_XOXX); // -|
  mvwputch(w_grid_trait, effect_win_size_y, 26, c_ltgray, LINE_XXOO); // |_
 }

 mvwputch(w_grid_effect, effect_win_size_y, 0, c_ltgray, LINE_XXOO); // |_
 mvwputch(w_grid_effect, effect_win_size_y, 27, c_ltgray, LINE_XOOX); // _|

 wrefresh(w_grid_skill);
 wrefresh(w_grid_effect);
 wrefresh(w_grid_trait);

 //-1 for header
 trait_win_size_y--;
 skill_win_size_y--;
 effect_win_size_y--;

// Print name and header
 mvwprintw(w_tip, 0, 0, "%s - %s", name.c_str(), (male ? "Male" : "Female"));
 mvwprintz(w_tip, 0, 39, c_ltred, "| Press TAB to cycle, ESC or q to return.");
 wrefresh(w_tip);

// First!  Default STATS screen.
 mvwprintz(w_stats, 0, 10, c_ltgray, "STATS");
 mvwprintz(w_stats, 2,  2, c_ltgray, "Strength:%s(%d)",
           (str_max < 10 ? "         " : "        "), str_max);
 mvwprintz(w_stats, 3,  2, c_ltgray, "Dexterity:%s(%d)",
           (dex_max < 10 ? "        "  : "       "),  dex_max);
 mvwprintz(w_stats, 4,  2, c_ltgray, "Intelligence:%s(%d)",
           (int_max < 10 ? "     "     : "    "),     int_max);
 mvwprintz(w_stats, 5,  2, c_ltgray, "Perception:%s(%d)",
           (per_max < 10 ? "       "   : "      "),   per_max);

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
 std::string asText[] = {"Torso", "Head", "Eyes", "Mouth", "Arms", "Hands", "Legs", "Feet"};
 body_part aBodyPart[] = {bp_torso, bp_head, bp_eyes, bp_mouth, bp_arms, bp_hands, bp_legs, bp_feet};
 int iEnc, iLayers, iArmorEnc, iWarmth;

 mvwprintz(w_encumb, 0, 1, c_ltgray, "ENCUMBERANCE AND WARMTH");
 for (int i=0; i < 8; i++) {
  iEnc = iLayers = iArmorEnc = iWarmth = 0;
  iEnc = encumb(aBodyPart[i], iLayers, iArmorEnc, iWarmth);
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
  wprintz(w_encumb, color, "%*s(%d)", (iWarmth > 9 ? ((iWarmth > 99) ? 1: 2) : 3), " ", iWarmth);
 }
 wrefresh(w_encumb);

// Next, draw traits.
 mvwprintz(w_traits, 0, 10, c_ltgray, "TRAITS");
 for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
  if (traits[traitslist[i]].points > 0)
   status = c_ltgreen;
  else
   status = c_ltred;
  mvwprintz(w_traits, i+1, 1, status, traits[traitslist[i]].name.c_str());
 }

 wrefresh(w_traits);

// Next, draw effects.
 mvwprintz(w_effects, 0, 8, c_ltgray, "EFFECTS");
 for (int i = 0; i < effect_name.size() && i < effect_win_size_y; i++) {
  mvwprintz(w_effects, i+1, 1, c_ltgray, effect_name[i].c_str());
 }
 wrefresh(w_effects);

// Next, draw skills.
 line = 1;
 std::vector <skill> skillslist;
 mvwprintz(w_skills, 0, 11, c_ltgray, "SKILLS");
 for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin()++;
      aSkill != Skill::skills.end(); ++aSkill)
 {
  int i = (*aSkill)->id();

  SkillLevel level = skillLevel(*aSkill);

  if ( i != 0 && sklevel[i] >= 0) {
   skillslist.push_back(skill(i));
   // Default to not training and not rusting
   nc_color text_color = c_blue;
   bool training = skillLevel(*aSkill).isTraining();
   bool rusting = skillLevel(*aSkill).isRusting(g->turn);

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
     mvwprintz(w_skills, line, 19, text_color, "%-2d(%2d%%%%)", (int)level,
               (level.exercise() <  0 ? 0 : level.exercise()));
     line++;
    }
  }
 }
 wrefresh(w_skills);

// Finally, draw speed.
 mvwprintz(w_speed, 0, 11, c_ltgray, "SPEED");
 mvwprintz(w_speed, 1,  1, c_ltgray, "Base Move Cost:");
 mvwprintz(w_speed, 2,  1, c_ltgray, "Current Speed:");
 int newmoves = current_speed(g);
 int pen = 0;
 line = 3;
 if (weight_carried() > int(weight_capacity() * .25)) {
  pen = 75 * double((weight_carried() - int(weight_capacity() * .25)) /
                    (weight_capacity() * .75));
  mvwprintz(w_speed, line, 1, c_red, "Overburdened        -%s%d%%%%",
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
   mvwprintz(w_speed, line, 1, c_green, "Good mood           +%s%d%%%%",
             (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, "Depressed           -%s%d%%%%",
             (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 pen = int((pain - pkill) * .7);
 if (pen > 60)
  pen = 60;
 if (pen >= 1) {
  mvwprintz(w_speed, line, 1, c_red, "Pain                -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (pkill >= 10) {
  pen = int(pkill * .1);
  mvwprintz(w_speed, line, 1, c_red, "Painkillers         -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (stim != 0) {
  pen = stim;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, "Stimulants          +%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, "Depressants         -%s%d%%%%",
            (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 if (thirst > 40) {
  pen = int((thirst - 40) / 10);
  mvwprintz(w_speed, line, 1, c_red, "Thirst              -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (hunger > 100) {
  pen = int((hunger - 100) / 10);
  mvwprintz(w_speed, line, 1, c_red, "Hunger              -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (has_trait(PF_SUNLIGHT_DEPENDENT) && !g->is_in_sunlight(posx, posy)) {
  pen = (g->light_level() >= 12 ? 5 : 10);
  mvwprintz(w_speed, line, 1, c_red, "Out of Sunlight     -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if ((has_trait(PF_COLDBLOOD) || has_trait(PF_COLDBLOOD2) ||
      has_trait(PF_COLDBLOOD3)) && g->temperature < 65) {
  if (has_trait(PF_COLDBLOOD3))
   pen = int( (65 - g->temperature) / 2);
  else if (has_trait(PF_COLDBLOOD2))
   pen = int( (65 - g->temperature) / 3);
  else
   pen = int( (65 - g->temperature) / 2);
  mvwprintz(w_speed, line, 1, c_red, "Cold-Blooded        -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }

 for (int i = 0; i < illness.size(); i++) {
  int move_adjust = disease_speed_boost(illness[i]);
  if (move_adjust != 0) {
   nc_color col = (move_adjust > 0 ? c_green : c_red);
   mvwprintz(w_speed, line,  1, col, dis_name(illness[i]).c_str());
   mvwprintz(w_speed, line, 21, col, (move_adjust > 0 ? "+" : "-"));
   move_adjust = abs(move_adjust);
   mvwprintz(w_speed, line, (move_adjust >= 10 ? 22 : 23), col, "%d%%%%",
             move_adjust);
  }
 }
 if (has_trait(PF_QUICK)) {
  pen = int(newmoves * .1);
  mvwprintz(w_speed, line, 1, c_green, "Quick               +%s%d%%%%",
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

// Initial printing is DONE.  Now we give the player a chance to scroll around
// and "hover" over different items for more info.
 do {
  werase(w_info);
  switch (curtab) {
  case 1:	// Stats tab
   mvwprintz(w_stats, 0, 0, h_ltgray, "          STATS           ");
   if (line == 0) {
    mvwprintz(w_stats, 2, 2, h_ltgray, "Strength:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Strength affects your melee damage, the amount of weight you can carry, your\n\
total HP, your resistance to many diseases, and the effectiveness of actions\n\
which require brute force.");
   } else if (line == 1) {
    mvwprintz(w_stats, 3, 2, h_ltgray, "Dexterity:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Dexterity affects your chance to hit in melee combat, helps you steady your\n\
gun for ranged combat, and enhances many actions that require finesse.");
   } else if (line == 2) {
    mvwprintz(w_stats, 4, 2, h_ltgray, "Intelligence:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Intelligence is less important in most situations, but it is vital for more\n\
complex tasks like electronics crafting. It also affects how much skill you\n\
can pick up from reading a book.");
   } else if (line == 3) {
    mvwprintz(w_stats, 5, 2, h_ltgray, "Perception:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Perception is the most important stat for ranged combat. It's also used for\n\
detecting traps and other things of interest.");
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
     mvwprintz(w_stats, 0, 0, c_ltgray, "          STATS           ");
     wrefresh(w_stats);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_stats, 2, 2, c_ltgray, "Strength:");
   mvwprintz(w_stats, 3, 2, c_ltgray, "Dexterity:");
   mvwprintz(w_stats, 4, 2, c_ltgray, "Intelligence:");
   mvwprintz(w_stats, 5, 2, c_ltgray, "Perception:");
   wrefresh(w_stats);
   break;
  case 2:	// Encumberment tab
   mvwprintz(w_encumb, 0, 0, h_ltgray, " ENCUMBERANCE AND WARMTH  ");
   if (line == 0) {
    mvwprintz(w_encumb, 1, 1, h_ltgray, "Torso");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Melee skill -%d;      Dodge skill -%d;\n\
Swimming costs +%d movement points;\n\
Melee attacks cost +%d movement points", encumb(bp_torso), encumb(bp_torso),
              encumb(bp_torso) * (80 - skillLevel("swimming") * 3), encumb(bp_torso) * 20);
   } else if (line == 1) {
    mvwprintz(w_encumb, 2, 1, h_ltgray, "Head");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Head encumberance has no effect; it simply limits how much you can put on.");
   } else if (line == 2) {
    mvwprintz(w_encumb, 3, 1, h_ltgray, "Eyes");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Perception %+d when checking traps or firing ranged weapons;\n\
Perception %+.1f when throwing items", -encumb(bp_eyes),
double(double(-encumb(bp_eyes)) / 2));
   } else if (line == 3) {
    mvwprintz(w_encumb, 4, 1, h_ltgray, "Mouth");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs %+d movement points", encumb(bp_mouth) * 5);
   } else if (line == 4)
  {
    mvwprintz(w_encumb, 5, 1, h_ltgray, "Arms");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Arm encumbrance affects your accuracy with ranged weapons.");
   } else if (line == 5)
   {
    mvwprintz(w_encumb, 6, 1, h_ltgray, "Hands");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Reloading costs %+d movement points;\n\
Dexterity %+d when throwing items", encumb(bp_hands) * 30, -encumb(bp_hands));
   } else if (line == 6) {
    mvwprintz(w_encumb, 7, 1, h_ltgray, "Legs");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs %+d movement points;  Swimming costs %+d movement points;\n\
Dodge skill %+.1f", encumb(bp_legs) * 3,
              encumb(bp_legs) *(50 - skillLevel("swimming")),
                     double(double(-encumb(bp_legs)) / 2));
   } else if (line == 7) {
    mvwprintz(w_encumb, 8, 1, h_ltgray, "Feet");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs %+d movement points", encumb(bp_feet) * 5);
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
     mvwprintz(w_encumb, 0, 0, c_ltgray, " ENCUMBERANCE AND WARMTH  ");
     wrefresh(w_encumb);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_encumb, 1, 1, c_ltgray, "Torso");
   mvwprintz(w_encumb, 2, 1, c_ltgray, "Head");
   mvwprintz(w_encumb, 3, 1, c_ltgray, "Eyes");
   mvwprintz(w_encumb, 4, 1, c_ltgray, "Mouth");
   mvwprintz(w_encumb, 5, 1, c_ltgray, "Arms");
   mvwprintz(w_encumb, 6, 1, c_ltgray, "Hands");
   mvwprintz(w_encumb, 7, 1, c_ltgray, "Legs");
   mvwprintz(w_encumb, 8, 1, c_ltgray, "Feet");
   wrefresh(w_encumb);
   break;
  case 4:	// Traits tab
   mvwprintz(w_traits, 0, 0, h_ltgray, "          TRAITS          ");
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
    if (traitslist[i] > PF_MAX2)
     status = c_ltblue;
    else if (traits[traitslist[i]].points > 0)
     status = c_ltgreen;
    else
     status = c_ltred;
    if (i == line)
     mvwprintz(w_traits, 1 + i - min, 1, hilite(status),
               traits[traitslist[i]].name.c_str());
    else
     mvwprintz(w_traits, 1 + i - min, 1, status,
               traits[traitslist[i]].name.c_str());
   }
   if (line >= 0 && line < traitslist.size())
    mvwprintz(w_info, 0, 0, c_magenta, "%s",
              traits[traitslist[line]].description.c_str());
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
     mvwprintz(w_traits, 0, 0, c_ltgray, "          TRAITS          ");
     for (int i = 0; i < traitslist.size() && i < trait_win_size_y; i++) {
      mvwprintz(w_traits, i + 1, 1, c_black, "                         ");
      if (traits[traitslist[i]].points > 0)
       status = c_ltgreen;
      else
       status = c_ltred;
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

  case 5:	// Effects tab
   mvwprintz(w_effects, 0, 0, h_ltgray, "        EFFECTS           ");
   if (line <= (effect_win_size_y-1)/2) {
    min = 0;
    max = effect_win_size_y;
    if (effect_name.size() < max)
     max = effect_name.size();
   } else if (line >= effect_name.size() - (effect_win_size_y+1)/2) {
    min = (effect_name.size() < effect_win_size_y ? 0 : effect_name.size() - effect_win_size_y);
    max = effect_name.size();
   } else {
    min = line - (effect_win_size_y-1)/2;
    max = line + (effect_win_size_y+1)/2;
    if (effect_name.size() < max)
     max = effect_name.size();
    if (min < 0)
     min = 0;
   }

   for (int i = min; i < max; i++) {
    if (i == line)
     mvwprintz(w_effects, 1 + i - min, 1, h_ltgray, effect_name[i].c_str());
    else
     mvwprintz(w_effects, 1 + i - min, 1, c_ltgray, effect_name[i].c_str());
   }
   if (line >= 0 && line < effect_text.size())
    mvwprintz(w_info, 0, 0, c_magenta, effect_text[line].c_str());
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
     mvwprintz(w_effects, 0, 0, c_ltgray, "        EFFECTS           ");
     for (int i = 0; i < effect_name.size() && i < 7; i++)
      mvwprintz(w_effects, i + 1, 1, c_ltgray, effect_name[i].c_str());
     wrefresh(w_effects);
     line = 0;
     curtab = 1;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 3:	// Skills tab
   mvwprintz(w_skills, 0, 0, h_ltgray, "           SKILLS         ");
   if (line <= (skill_win_size_y-1)/2) {
    min = 0;
    max = skill_win_size_y;
    if (skillslist.size() < max)
     max = skillslist.size();
   } else if (line >= skillslist.size() - (skill_win_size_y+1)/2) {
    min = (skillslist.size() < skill_win_size_y ? 0 : skillslist.size() - skill_win_size_y);
    max = skillslist.size();
   } else {
    min = line - (skill_win_size_y-1)/2;
    max = line + (skill_win_size_y+1)/2;
    if (skillslist.size() < max)
     max = skillslist.size();
    if (min < 0)
     min = 0;
   }

   Skill *selectedSkill;

   for (int i = min; i < max; i++) {
     Skill *aSkill = Skill::skill(skillslist[i]);
     SkillLevel level = skillLevel(aSkill);

     bool isLearning = level.isTraining();
     int exercise = level.exercise();

    if (i == line) {
      selectedSkill = aSkill;
     if (exercise >= 100)
      status = isLearning ? h_pink : h_red;
     else
      status = isLearning ? h_ltblue : h_blue;
    } else {
     if (exercise < 0)
      status = isLearning ? c_ltred : c_red;
     else
      status = isLearning ? c_ltblue : c_blue;
    }
    mvwprintz(w_skills, 1 + i - min, 1, c_ltgray, "                         ");
    mvwprintz(w_skills, 1 + i - min, 1, status, "%s:", aSkill->name().c_str());
    mvwprintz(w_skills, 1 + i - min,19, status, "%-2d(%2d%%%%)", (int)level, (exercise <  0 ? 0 : exercise));
   }
   werase(w_info);
   if (line >= 0 && line < skillslist.size())
    mvwprintz(w_info, 0, 0, c_magenta,
              selectedSkill->description().c_str());
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
     mvwprintz(w_skills, 0, 0, c_ltgray, "           SKILLS         ");
     for (int i = 0; i < skillslist.size() && i < skill_win_size_y; i++) {
      Skill *thisSkill = Skill::skill(skillslist[i]);
      SkillLevel level = skillLevel(thisSkill);
      bool isLearning = level.isTraining();

      if (level.exercise() < 0)
       status = isLearning ? c_ltred : c_red;
      else
       status = isLearning ? c_ltblue : c_blue;

      mvwprintz(w_skills, i + 1,  1, status, "%s:", thisSkill->name().c_str());
      mvwprintz(w_skills, i + 1, 19, status, "%d (%2d%%%%)", (int)level, (level.exercise() <  0 ? 0 : level.exercise()));
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

void player::disp_morale(game* g)
{
 WINDOW* w = newwin(25, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX-80)/2 : 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1,  1, c_white, "Morale Modifiers:");
 mvwprintz(w, 2,  1, c_ltgray, "Name");
 mvwprintz(w, 2, 20, c_ltgray, "Value");

 for (int i = 0; i < morale.size(); i++) {
  int b = morale[i].bonus;

  int bpos = 24;
  if (abs(b) >= 10)
   bpos--;
  if (abs(b) >= 100)
   bpos--;
  if (b < 0)
   bpos--;

  mvwprintz(w, i + 3,  1, (b < 0 ? c_red : c_green),
            morale[i].name(morale_data).c_str());
  mvwprintz(w, i + 3, bpos, (b < 0 ? c_red : c_green), "%d", b);
 }

 int mor = morale_level();
 int bpos = 24;
  if (abs(mor) >= 10)
   bpos--;
  if (abs(mor) >= 100)
   bpos--;
  if (mor < 0)
   bpos--;
 mvwprintz(w, 20, 1, (mor < 0 ? c_red : c_green), "Total:");
 mvwprintz(w, 20, bpos, (mor < 0 ? c_red : c_green), "%d", mor);

 wrefresh(w);
 getch();
 werase(w);
 delwin(w);
}

void player::disp_status(WINDOW *w, game *g)
{
 mvwprintz(w, 0, 0, c_ltgray, "Weapon: %s", weapname().c_str());
 if (weapon.is_gun()) {
   int adj_recoil = recoil + driving_recoil;
       if (adj_recoil >= 36)
   mvwprintz(w, 0, 34, c_red,    "Recoil");
  else if (adj_recoil >= 20)
   mvwprintz(w, 0, 34, c_ltred,  "Recoil");
  else if (adj_recoil >= 4)
   mvwprintz(w, 0, 34, c_yellow, "Recoil");
  else if (adj_recoil > 0)
   mvwprintz(w, 0, 34, c_ltgray, "Recoil");
 }

 // Print the current weapon mode
 if (weapon.mode == IF_NULL)
  mvwprintz(w, 1, 0, c_red,    "Normal");
 else if (weapon.mode == IF_MODE_BURST)
  mvwprintz(w, 1, 0, c_red,    "Burst");
 else {
  item* gunmod = weapon.active_gunmod();
  if (gunmod != NULL)
   mvwprintz(w, 1, 0, c_red, gunmod->type->name.c_str());
 }

 if (hunger > 2800)
  mvwprintz(w, 2, 0, c_red,    "Starving!");
 else if (hunger > 1400)
  mvwprintz(w, 2, 0, c_ltred,  "Near starving");
 else if (hunger > 300)
  mvwprintz(w, 2, 0, c_ltred,  "Famished");
 else if (hunger > 100)
  mvwprintz(w, 2, 0, c_yellow, "Very hungry");
 else if (hunger > 40)
  mvwprintz(w, 2, 0, c_yellow, "Hungry");
 else if (hunger < 0)
  mvwprintz(w, 2, 0, c_green,  "Full");

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
 if      (delta >   2) temp_message = " (Rising!!)";
 else if (delta ==  2) temp_message = " (Rising!)";
 else if (delta ==  1) temp_message = " (Rising)";
 else if (delta ==  0) temp_message = "";
 else if (delta == -1) temp_message = " (Falling)";
 else if (delta == -2) temp_message = " (Falling!)";
 else if (delta <  -2) temp_message = " (Falling!!)";
 // Print the hottest/coldest bodypart, and if it is rising or falling in temperature
 if      (temp_cur[print] >  BODYTEMP_SCORCHING)
  mvwprintz(w, 1, 9, c_red,   "Scorching!%s", temp_message);
 else if (temp_cur[print] >  BODYTEMP_VERY_HOT)
  mvwprintz(w, 1, 9, c_ltred, "Very hot!%s", temp_message);
 else if (temp_cur[print] >  BODYTEMP_HOT)
  mvwprintz(w, 1, 9, c_yellow,"Hot%s", temp_message);
 else if (temp_cur[print] >  BODYTEMP_COLD) // If you're warmer than cold, you are comfortable
  mvwprintz(w, 1, 9, c_green, "Comfortable%s", temp_message);
 else if (temp_cur[print] >  BODYTEMP_VERY_COLD)
  mvwprintz(w, 1, 9, c_ltblue,"Cold%s", temp_message);
 else if (temp_cur[print] >  BODYTEMP_FREEZING)
  mvwprintz(w, 1, 9, c_cyan,  "Very cold!%s", temp_message);
 else if (temp_cur[print] <= BODYTEMP_FREEZING)
  mvwprintz(w, 1, 9, c_blue,  "Freezing!%s", temp_message);

      if (thirst > 520)
  mvwprintz(w, 2, 15, c_ltred,  "Parched");
 else if (thirst > 240)
  mvwprintz(w, 2, 15, c_ltred,  "Dehydrated");
 else if (thirst > 80)
  mvwprintz(w, 2, 15, c_yellow, "Very thirsty");
 else if (thirst > 40)
  mvwprintz(w, 2, 15, c_yellow, "Thirsty");
 else if (thirst < 0)
  mvwprintz(w, 2, 15, c_green,  "Slaked");

      if (fatigue > 575)
  mvwprintz(w, 2, 30, c_red,    "Exhausted");
 else if (fatigue > 383)
  mvwprintz(w, 2, 30, c_ltred,  "Dead tired");
 else if (fatigue > 191)
  mvwprintz(w, 2, 30, c_yellow, "Tired");

 mvwprintz(w, 2, 41, c_white, "XP: ");
 nc_color col_xp = c_dkgray;
 if (xp_pool >= 100)
  col_xp = c_white;
 else if (xp_pool >  0)
  col_xp = c_ltgray;
 mvwprintz(w, 2, 45, col_xp, "%d", xp_pool);

 nc_color col_pain = c_yellow;
 if (pain - pkill >= 60)
  col_pain = c_red;
 else if (pain - pkill >= 40)
  col_pain = c_ltred;
 if (pain - pkill > 0)
  mvwprintz(w, 3, 0, col_pain, "Pain: %d", pain - pkill);

 vehicle *veh = g->m.veh_at (posx, posy);
 int dmor = 0;

 int morale_cur = morale_level ();
 nc_color col_morale = c_white;
 if (morale_cur >= 10)
  col_morale = c_green;
 else if (morale_cur <= -10)
  col_morale = c_red;
 if (morale_cur >= 100)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":D");
 else if (morale_cur >= 10)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":)");
 else if (morale_cur > -10)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":|");
 else if (morale_cur > -100)
  mvwprintz(w, 3, 10 + dmor, col_morale, "):");
 else
  mvwprintz(w, 3, 10 + dmor, col_morale, "D:");

 if (in_vehicle && veh) {
  veh->print_fuel_indicator (w, 3, 49);
  nc_color col_indf1 = c_ltgray;

  float strain = veh->strain();
  nc_color col_vel = strain <= 0? c_ltblue :
                     (strain <= 0.2? c_yellow :
                     (strain <= 0.4? c_ltred : c_red));

  bool has_turrets = false;
  for (int p = 0; p < veh->parts.size(); p++) {
   if (veh->part_flag (p, vpf_turret)) {
    has_turrets = true;
    break;
   }
  }

  if (has_turrets) {
   mvwprintz(w, 3, 25, col_indf1, "Gun:");
   mvwprintz(w, 3, 29, veh->turret_mode ? c_ltred : c_ltblue,
                       veh->turret_mode ? "auto" : "off ");
  }

  if (veh->cruise_on) {
   if(OPTIONS[OPT_USE_METRIC_SYS]) {
    mvwprintz(w, 3, 33, col_indf1, "{Km/h....>....}");
    mvwprintz(w, 3, 38, col_vel, "%4d", int(veh->velocity * 0.0161f));
    mvwprintz(w, 3, 43, c_ltgreen, "%4d", int(veh->cruise_velocity * 0.0161f));
   }
   else {
    mvwprintz(w, 3, 34, col_indf1, "{mph....>....}");
    mvwprintz(w, 3, 38, col_vel, "%4d", veh->velocity / 100);
    mvwprintz(w, 3, 43, c_ltgreen, "%4d", veh->cruise_velocity / 100);
   }
  } else {
   if(OPTIONS[OPT_USE_METRIC_SYS]) {
    mvwprintz(w, 3, 33, col_indf1, "  {Km/h....}  ");
    mvwprintz(w, 3, 40, col_vel, "%4d", int(veh->velocity * 0.0161f));
   }
   else {
    mvwprintz(w, 3, 34, col_indf1, "  {mph....}  ");
    mvwprintz(w, 3, 40, col_vel, "%4d", veh->velocity / 100);
   }
  }

  if (veh->velocity != 0) {
   nc_color col_indc = veh->skidding? c_red : c_green;
   int dfm = veh->face.dir() - veh->move.dir();
   mvwprintz(w, 3, 21, col_indc, dfm < 0? "L" : ".");
   mvwprintz(w, 3, 22, col_indc, dfm == 0? "0" : ".");
   mvwprintz(w, 3, 23, col_indc, dfm > 0? "R" : ".");
  }
 } else {  // Not in vehicle
  nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
           col_per = c_white, col_spd = c_white;
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
  int spd_cur = current_speed();
  if (current_speed() < 100)
   col_spd = c_red;
  if (current_speed() > 100)
   col_spd = c_green;

  mvwprintz(w, 3, 13, col_str, "Str %s%d", str_cur >= 10 ? "" : " ", str_cur);
  mvwprintz(w, 3, 20, col_dex, "Dex %s%d", dex_cur >= 10 ? "" : " ", dex_cur);
  mvwprintz(w, 3, 27, col_int, "Int %s%d", int_cur >= 10 ? "" : " ", int_cur);
  mvwprintz(w, 3, 34, col_per, "Per %s%d", per_cur >= 10 ? "" : " ", per_cur);
  mvwprintz(w, 3, 41, col_spd, "Spd %s%d", spd_cur >= 10 ? "" : " ", spd_cur);
 }
}

bool player::has_trait(int flag) const
{
 if (flag == PF_NULL)
  return true;
 return my_traits[flag];
}

bool player::has_mutation(int flag) const
{
 if (flag == PF_NULL)
  return true;
 return my_mutations[flag];
}

void player::toggle_trait(int flag)
{
 my_traits[flag] = !my_traits[flag];
 my_mutations[flag] = !my_mutations[flag];
}

bool player::has_bionic(bionic_id b) const
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return true;
 }
 return false;
}

bool player::has_active_bionic(bionic_id b) const
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
   return;	// No duplicates!
 }
 char newinv;
 if (my_bionics.size() == 0)
  newinv = 'a';
 else
  newinv = my_bionics[my_bionics.size() - 1].invlet + 1;
 my_bionics.push_back(bionic(b, newinv));
}

void player::charge_power(int amount)
{
 power_level += amount;
 if (power_level > max_power_level)
  power_level = max_power_level;
 if (power_level < 0)
  power_level = 0;
}

float player::active_light()
{
 float lumination = 0;

 int flashlight = active_item_charges("flashlight_on");
 int torch = active_item_charges("torch_lit");
 if (flashlight > 0)
  lumination = std::min(100, flashlight * 5); // Will do for now
 else if (torch > 0)
  lumination = std::min(100, torch * 5);
 else if (has_active_bionic("bio_flashlight"))
  lumination = 60;
 else if (has_artifact_with(AEP_GLOW))
  lumination = 25;

 return lumination;
}

int player::sight_range(int light_level)
{
 int ret = light_level;
 if (((is_wearing("goggles_nv") && has_active_item("UPS_on")) ||
     has_active_bionic("bio_night_vision")) &&
     ret < 12)
  ret = 12;
 if (has_trait(PF_NIGHTVISION) && ret < 12)
  ret += 1;
 if (has_trait(PF_NIGHTVISION2) && ret < 12)
  ret += 4;
 if (has_trait(PF_NIGHTVISION3) && ret < 12)
  ret = 12;
 if (underwater && !has_bionic("bio_membrane") && !has_trait(PF_MEMBRANE) &&
     !is_wearing("goggles_swim"))
  ret = 1;
 if (has_disease(DI_BOOMERED))
  ret = 1;
 if (has_disease(DI_IN_PIT))
  ret = 1;
 if (has_disease(DI_BLIND))
  ret = 0;
 if (ret > 4 && has_trait(PF_MYOPIC) && !is_wearing("glasses_eye") &&
     !is_wearing("glasses_monocle"))
  ret = 4;
 return ret;
}

int player::unimpaired_range()
{
 int ret = 12;
 if (has_disease(DI_IN_PIT))
  ret = 1;
 if (has_disease(DI_BLIND))
  ret = 0;
 return ret;
}

int player::overmap_sight_range(int light_level)
{
 int sight = sight_range(light_level);
 if (sight < SEEX)
  return 0;
 if (sight <= SEEX * 4)
  return (sight / (SEEX / 2));
 if (has_amount("binoculars", 1))
  return 20;

 return 10;
}

int player::clairvoyance()
{
 if (has_artifact_with(AEP_CLAIRVOYANCE))
  return 3;
 return 0;
}

bool player::sight_impaired()
{
 return has_disease(DI_BOOMERED) ||
  (underwater && !has_bionic("bio_membrane") && !has_trait(PF_MEMBRANE)
              && !is_wearing("goggles_swim")) ||
  (has_trait(PF_MYOPIC) && !is_wearing("glasses_eye")
                        && !is_wearing("glasses_monocle"));
}

bool player::has_two_arms() const
{
 if (has_bionic("bio_blaster") || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10)
  return false;
 return true;
}

bool player::avoid_trap(trap* tr)
{
  int myroll = dice(3, dex_cur + skillLevel("dodge") * 1.5);
 int traproll;
 if (per_cur - encumb(bp_eyes) >= tr->visibility)
  traproll = dice(3, tr->avoidance);
 else
  traproll = dice(6, tr->avoidance);
 if (has_trait(PF_LIGHTSTEP))
  myroll += dice(2, 6);
 if (myroll >= traproll)
  return true;
 return false;
}

void player::pause(game *g)
{
 moves = 0;
 if (recoil > 0) {
   if (str_cur + 2 * skillLevel("gun") >= recoil)
   recoil = 0;
  else {
    recoil -= str_cur + 2 * skillLevel("gun");
   recoil = int(recoil / 2);
  }
 }

// Meditation boost for Toad Style
 if (weapon.type->id == "style_toad" && activity.type == ACT_NULL) {
  int arm_amount = 1 + (int_cur - 6) / 3 + (per_cur - 6) / 3;
  int arm_max = (int_cur + per_cur) / 2;
  if (arm_amount > 3)
   arm_amount = 3;
  if (arm_max > 20)
   arm_max = 20;
  add_disease(DI_ARMOR_BOOST, 2, g, arm_amount, arm_max);
 }
}

int player::throw_range(char ch)
{
 item tmp;
 if (ch == -1)
  tmp = weapon;
 else if (ch == -2)
  return -1;
 else
  tmp = inv.item_by_letter(ch);

 if (tmp.weight() > int(str_cur * 15))
  return 0;
 int ret = int((str_cur * 8) / (tmp.weight() > 0 ? tmp.weight() : 10));
 ret -= int(tmp.volume() / 10);
 if (ret < 1)
  return 1;
// Cap at double our strength + skill
 if (ret > str_cur * 1.5 + skillLevel("throw"))
   return str_cur * 1.5 + skillLevel("throw");
 return ret;
}

int player::ranged_dex_mod(bool real_life)
{
 int dex = (real_life ? dex_cur : dex_max);
 if (dex == 8)
  return 0;
 if (dex > 8)
  return (real_life ? (0 - rng(0, dex - 8)) : (8 - dex));

 int deviation = 0;
 if (dex < 4)
  deviation = 4 * (8 - dex);
 if (dex < 6)
  deviation = 2 * (8 - dex);
 else
  deviation = 1.5 * (8 - dex);

 return (real_life ? rng(0, deviation) : deviation);
}

int player::ranged_per_mod(bool real_life)
{
 int per = (real_life ? per_cur : per_max);
 if (per == 8)
  return 0;
 int deviation = 0;

 if (per < 4) {
  deviation = 5 * (8 - per);
  if (real_life)
   deviation = rng(0, deviation);
 } else if (per < 6) {
  deviation = 2.5 * (8 - per);
  if (real_life)
   deviation = rng(0, deviation);
 } else if (per < 8) {
  deviation = 2 * (8 - per);
  if (real_life)
   deviation = rng(0, deviation);
 } else {
  deviation = 3 * (0 - (per > 16 ? 8 : per - 8));
  if (real_life && one_in(per))
   deviation = 0 - rng(0, abs(deviation));
 }
 return deviation;
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

int player::comprehension_percent(skill s, bool real_life)
{
 double intel = (double)(real_life ? int_cur : int_max);
 if (intel == 0.)
  intel = 1.;
 double percent = 80.; // double temporarily, since we divide a lot
 int learned = (real_life ? sklevel[s] : 4);
 if (learned > intel / 2)
  percent /= 1 + ((learned - intel / 2) / (intel / 3));
 else if (!real_life && intel > 8)
  percent += 125 - 1000 / intel;

 if (has_trait(PF_FASTLEARNER))
  percent += 50.;
 return (int)(percent);
}

int player::read_speed(bool real_life)
{
 int intel = (real_life ? int_cur : int_max);
 int ret = 1000 - 50 * (intel - 8);
 if (has_trait(PF_FASTREADER))
  ret *= .8;
 if (ret < 100)
  ret = 100;
 return (real_life ? ret : ret / 10);
}

int player::talk_skill()
{
    int ret = int_cur + per_cur + skillLevel("speech") * 3;
    if (has_trait(PF_UGLY))
        ret -= 3;
    else if (has_trait(PF_DEFORMED))
        ret -= 6;
    else if (has_trait(PF_DEFORMED2))
        ret -= 12;
    else if (has_trait(PF_DEFORMED3))
        ret -= 18;
    else if (has_trait(PF_PRETTY))
        ret += 1;
    else if (has_trait(PF_BEAUTIFUL))
        ret += 2;
    else if (has_trait(PF_BEAUTIFUL2))
        ret += 4;
    else if (has_trait(PF_BEAUTIFUL3))
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
 if (has_trait(PF_DEFORMED2))
  ret += 3;
 else if (has_trait(PF_DEFORMED3))
  ret += 6;
 else if (has_trait(PF_PRETTY))
  ret -= 1;
 else if (has_trait(PF_BEAUTIFUL) || has_trait(PF_BEAUTIFUL2) || has_trait(PF_BEAUTIFUL3))
  ret -= 4;
 if (stim > 20)
  ret += 2;
 if (has_disease(DI_DRUNK))
  ret -= 4;

 return ret;
}

void player::hit(game *g, body_part bphurt, int side, int dam, int cut)
{
 int painadd = 0;
 if (has_disease(DI_SLEEP)) {
  g->add_msg("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 absorb(g, bphurt, dam, cut);

 dam += cut;
 if (dam <= 0)
  return;

 hit_animation(this->posx - g->u.posx + VIEWX - g->u.view_offset_x,
               this->posy - g->u.posy + VIEWY - g->u.view_offset_y,
               red_background(this->color()), '@');

 rem_disease(DI_SPEED_BOOST);
 if (dam >= 6)
  rem_disease(DI_ARMOR_BOOST);

 if (!is_npc())
  g->cancel_activity_query("You were hurt!");

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
   g->add_msg("A snake sprouts from your body!");
  else if (snakes >= 2)
   g->add_msg("Some snakes sprout from your body!");
  monster snake(g->mtypes[mon_shadow_snake]);
  for (int i = 0; i < snakes; i++) {
   int index = rng(0, valid.size() - 1);
   point sp = valid[index];
   valid.erase(valid.begin() + index);
   snake.spawn(sp.x, sp.y);
   snake.friendly = -1;
   g->z.push_back(snake);
  }
 }

 if (has_trait(PF_PAINRESIST))
  painadd = (sqrt(double(cut)) + dam + cut) / (rng(4, 6));
 else
  painadd = (sqrt(double(cut)) + dam + cut) / 4;
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:
  pain++;
  if (dam > 5 || cut > 0) {
   int minblind = int((dam + cut) / 10);
   if (minblind < 1)
    minblind = 1;
   int maxblind = int((dam + cut) /  4);
   if (maxblind > 5)
    maxblind = 5;
   add_disease(DI_BLIND, rng(minblind, maxblind), g);
  }

 case bp_mouth: // Fall through to head damage
 case bp_head:
  pain++;
  hp_cur[hp_head] -= dam;
  if (hp_cur[hp_head] < 0)
   hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  recoil += int(dam / 5);
  hp_cur[hp_torso] -= dam;
  if (hp_cur[hp_torso] < 0)
   hp_cur[hp_torso] = 0;
 break;
 case bp_hands: // Fall through to arms
 case bp_arms:
  if (side == 1 || side == 3 || weapon.is_two_handed(this))
   recoil += int(dam / 3);
  if (side == 0 || side == 3) {
   hp_cur[hp_arm_l] -= dam;
   if (hp_cur[hp_arm_l] < 0)
    hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_arm_r] -= dam;
   if (hp_cur[hp_arm_r] < 0)
    hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet: // Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   hp_cur[hp_leg_l] -= dam;
   if (hp_cur[hp_leg_l] < 0)
    hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_leg_r] -= dam;
   if (hp_cur[hp_leg_r] < 0)
    hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hit!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease(DI_ADRENALINE) &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease(DI_ADRENALINE, 200, g);
}

void player::hurt(game *g, body_part bphurt, int side, int dam)
{
 int painadd = 0;
 if (has_disease(DI_SLEEP) && rng(0, dam) > 2) {
  g->add_msg("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 if (dam <= 0)
  return;

 if (!is_npc())
  g->cancel_activity_query("You were hurt!");

 if (has_trait(PF_PAINRESIST))
  painadd = dam / 3;
 else
  painadd = dam / 2;
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
 case bp_head:
  pain++;
  hp_cur[hp_head] -= dam;
  if (hp_cur[hp_head] < 0)
   hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  hp_cur[hp_torso] -= dam;
  if (hp_cur[hp_torso] < 0)
   hp_cur[hp_torso] = 0;
 break;
 case bp_hands:	// Fall through to arms
 case bp_arms:
  if (side == 0 || side == 3) {
   hp_cur[hp_arm_l] -= dam;
   if (hp_cur[hp_arm_l] < 0)
    hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_arm_r] -= dam;
   if (hp_cur[hp_arm_r] < 0)
    hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet:	// Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   hp_cur[hp_leg_l] -= dam;
   if (hp_cur[hp_leg_l] < 0)
    hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_leg_r] -= dam;
   if (hp_cur[hp_leg_r] < 0)
    hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hurt!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease(DI_ADRENALINE) &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease(DI_ADRENALINE, 200, g);
}

void player::heal(body_part healed, int side, int dam)
{
 hp_part healpart;
 switch (healed) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
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
  hp_cur[healpart] = hp_max[healpart];
}

void player::heal(hp_part healed, int dam)
{
 hp_cur[healed] += dam;
 if (hp_cur[healed] > hp_max[healed])
  hp_cur[healed] = hp_max[healed];
}

void player::healall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  if (hp_cur[i] > 0) {
   hp_cur[i] += dam;
   if (hp_cur[i] > hp_max[i])
    hp_cur[i] = hp_max[i];
  }
 }
}

void player::hurtall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  int painadd = 0;
  hp_cur[i] -= dam;
   if (hp_cur[i] < 0)
     hp_cur[i] = 0;
  if (has_trait(PF_PAINRESIST))
   painadd = dam / 3;
  else
   painadd = dam / 2;
  pain += painadd;
 }
}

void player::hitall(game *g, int dam, int vary)
{
 if (has_disease(DI_SLEEP)) {
  g->add_msg("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 for (int i = 0; i < num_hp_parts; i++) {
  int ddam = vary? dam * rng (100 - vary, 100) / 100 : dam;
  int cut = 0;
  absorb(g, (body_part) i, ddam, cut);
  int painadd = 0;
  hp_cur[i] -= ddam;
   if (hp_cur[i] < 0)
     hp_cur[i] = 0;
  if (has_trait(PF_PAINRESIST))
   painadd = dam / 3 / 4;
  else
   painadd = dam / 2 / 4;
  pain += painadd;
 }
}

void player::knock_back_from(game *g, int x, int y)
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

 bool u_see = (!is_npc() || g->u_see(to.x, to.y));

 std::string You = (is_npc() ? name : "You");
 std::string s = (is_npc() ? "s" : "");

// First, see if we hit a monster
 int mondex = g->mon_at(to.x, to.y);
 if (mondex != -1) {
  monster *z = &(g->z[mondex]);
  hit(g, bp_torso, 0, z->type->size, 0);
  add_disease(DI_STUNNED, 1, g);
  if ((str_max - 6) / 4 > z->type->size) {
   z->knock_back_from(g, posx, posy); // Chain reaction!
   z->hurt((str_max - 6) / 4);
   z->add_effect(ME_STUNNED, 1);
  } else if ((str_max - 6) / 4 == z->type->size) {
   z->hurt((str_max - 6) / 4);
   z->add_effect(ME_STUNNED, 1);
  }

  if (u_see)
   g->add_msg("%s bounce%s off a %s!",
              You.c_str(), s.c_str(), z->name().c_str());

  return;
 }

 int npcdex = g->npc_at(to.x, to.y);
 if (npcdex != -1) {
  npc *p = g->active_npc[npcdex];
  hit(g, bp_torso, 0, 3, 0);
  add_disease(DI_STUNNED, 1, g);
  p->hit(g, bp_torso, 0, 3, 0);
  if (u_see)
   g->add_msg("%s bounce%s off %s!", You.c_str(), s.c_str(), p->name.c_str());

  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.move_cost(to.x, to.y) == 0) { // Wait, it's a wall (or water)

  if (g->m.has_flag(liquid, to.x, to.y)) {
   if (!is_npc())
    g->plswim(to.x, to.y);
// TODO: NPCs can't swim!
  } else { // It's some kind of wall.
   hurt(g, bp_torso, 0, 3);
   add_disease(DI_STUNNED, 2, g);
   if (u_see)
    g->add_msg("%s bounce%s off a %s.", name.c_str(), s.c_str(),
                                        g->m.tername(to.x, to.y).c_str());
  }

 } else { // It's no wall
  posx = to.x;
  posy = to.y;
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

void player::get_sick(game *g)
{
 if (health > 0 && rng(0, health + 10) < health)
  health--;
 if (health < 0 && rng(0, 10 - health) < (0 - health))
  health++;
 if (one_in(12))
  health -= 1;

 if (g->debugmon)
  debugmsg("Health: %d", health);

 if (has_trait(PF_DISIMMUNE))
  return;

 if (!has_disease(DI_FLU) && !has_disease(DI_COMMON_COLD) &&
     one_in(900 + 10 * health + (has_trait(PF_DISRESISTANT) ? 300 : 0))) {
  if (one_in(6))
   infect(DI_FLU, bp_mouth, 3, rng(40000, 80000), g);
  else
   infect(DI_COMMON_COLD, bp_mouth, 3, rng(20000, 60000), g);
 }
}

void player::infect(dis_type type, body_part vector, int strength,
                    int duration, game *g)
{
 if (dice(strength, 3) > dice(resist(vector), 3))
  add_disease(type, duration, g);
}

void player::add_disease(dis_type type, int duration, game *g,
                         int intensity, int max_intensity)
{
 if (duration == 0)
  return;
 bool found = false;
 int i = 0;
 while ((i < illness.size()) && !found) {
  if (illness[i].type == type) {
   illness[i].duration += duration;
   illness[i].intensity += intensity;
   if (max_intensity != -1 && illness[i].intensity > max_intensity)
    illness[i].intensity = max_intensity;
   found = true;
  }
  i++;
 }
 if (!found) {
  if (!is_npc())
   dis_msg(g, type);
  disease tmp(type, duration, intensity);
  illness.push_back(tmp);
 }
// activity.type = ACT_NULL;
}

void player::rem_disease(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   illness.erase(illness.begin() + i);
 }
}

bool player::has_disease(dis_type type) const
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return true;
 }
 return false;
}

int player::disease_level(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return illness[i].duration;
 }
 return 0;
}

int player::disease_intensity(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type)
   return illness[i].intensity;
 }
 return 0;
}

void player::add_addiction(add_type type, int strength)
{
 if (type == ADD_NULL)
  return;
 int timer = 1200;
 if (has_trait(PF_ADDICTIVE)) {
  strength = int(strength * 1.5);
  timer = 800;
 }
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
        if (addictions[i].sated <   0)
    addictions[i].sated = timer;
   else if (addictions[i].sated < 600)
    addictions[i].sated += timer;	// TODO: Make this variable?
   else
    addictions[i].sated += int((3000 - addictions[i].sated) / 2);
   if ((rng(0, strength) > rng(0, addictions[i].intensity * 5) ||
       rng(0, 500) < strength) && addictions[i].intensity < 20)
    addictions[i].intensity++;
   return;
  }
 }
 if (rng(0, 100) < strength) {
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

void player::cauterize(game *g) {
 rem_disease(DI_BLEED);
 rem_disease(DI_BITE);
 pain += 15;
 if (!is_npc()) {
  g->add_msg("You cauterize yourself. It hurts like hell!");
 }
}

void player::suffer(game *g)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].powered)
   activate_bionic(i, g);
 }
 if (underwater) {
  if (!has_trait(PF_GILLS))
   oxygen--;
  if (oxygen < 0) {
   if (has_bionic("bio_gills") && power_level > 0) {
    oxygen += 5;
    power_level--;
   } else {
    g->add_msg("You're drowning!");
    hurt(g, bp_torso, 0, rng(1, 4));
   }
  }
 }
 for (int i = 0; i < illness.size(); i++) {
  dis_effect(g, *this, illness[i]);
  illness[i].duration--;
  if (illness[i].duration < MIN_DISEASE_AGE)// Cap permanent disease age
   illness[i].duration = MIN_DISEASE_AGE;
  if (illness[i].duration == 0) {
   illness.erase(illness.begin() + i);
   i--;
  }
 }
 if (!has_disease(DI_SLEEP)) {
  int timer = -3600;
  if (has_trait(PF_ADDICTIVE))
   timer = -4000;
  for (int i = 0; i < addictions.size(); i++) {
   if (addictions[i].sated <= 0 &&
       addictions[i].intensity >= MIN_ADDICTION_LEVEL)
    addict_effect(g, addictions[i]);
   addictions[i].sated--;
   if (!one_in(addictions[i].intensity - 2) && addictions[i].sated > 0)
    addictions[i].sated -= 1;
   if (addictions[i].sated < timer - (100 * addictions[i].intensity)) {
    if (addictions[i].intensity <= 2) {
     addictions.erase(addictions.begin() + i);
     i--;
    } else {
     addictions[i].intensity = int(addictions[i].intensity / 2);
     addictions[i].intensity--;
     addictions[i].sated = 0;
    }
   }
  }
  if (has_trait(PF_CHEMIMBALANCE)) {
   if (one_in(3600)) {
    g->add_msg("You suddenly feel sharp pain for no reason.");
    pain += 3 * rng(1, 3);
   }
   if (one_in(3600)) {
    int pkilladd = 5 * rng(-1, 2);
    if (pkilladd > 0)
     g->add_msg("You suddenly feel numb.");
    else if (pkilladd < 0)
     g->add_msg("You suddenly ache.");
    pkill += pkilladd;
   }
   if (one_in(3600)) {
    g->add_msg("You feel dizzy for a moment.");
    moves -= rng(10, 30);
   }
   if (one_in(3600)) {
    int hungadd = 5 * rng(-1, 3);
    if (hungadd > 0)
     g->add_msg("You suddenly feel hungry.");
    else
     g->add_msg("You suddenly feel a little full.");
    hunger += hungadd;
   }
   if (one_in(3600)) {
    g->add_msg("You suddenly feel thirsty.");
    thirst += 5 * rng(1, 3);
   }
   if (one_in(3600)) {
    g->add_msg("You feel fatigued all of a sudden.");
    fatigue += 10 * rng(2, 4);
   }
   if (one_in(4800)) {
    if (one_in(3))
     add_morale(MORALE_FEELING_GOOD, 20, 100);
    else
     add_morale(MORALE_FEELING_BAD, -20, -100);
   }
  }
  if ((has_trait(PF_SCHIZOPHRENIC) || has_artifact_with(AEP_SCHIZO)) &&
      one_in(2400)) { // Every 4 hours or so
   monster phantasm;
   int i;
   switch(rng(0, 11)) {
    case 0:
     add_disease(DI_HALLU, 3600, g);
     break;
    case 1:
     add_disease(DI_VISUALS, rng(15, 60), g);
     break;
    case 2:
     g->add_msg("From the south you hear glass breaking.");
     break;
    case 3:
     g->add_msg("YOU SHOULD QUIT THE GAME IMMEDIATELY.");
     add_morale(MORALE_FEELING_BAD, -50, -150);
     break;
    case 4:
     for (i = 0; i < 10; i++) {
      g->add_msg("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
     }
     break;
    case 5:
     g->add_msg("You suddenly feel so numb...");
     pkill += 25;
     break;
    case 6:
     g->add_msg("You start to shake uncontrollably.");
     add_disease(DI_SHAKES, 10 * rng(2, 5), g);
     break;
    case 7:
     for (i = 0; i < 10; i++) {
      phantasm = monster(g->mtypes[mon_hallu_zom + rng(0, 3)]);
      phantasm.spawn(posx + rng(-10, 10), posy + rng(-10, 10));
      if (g->mon_at(phantasm.posx, phantasm.posy) == -1)
       g->z.push_back(phantasm);
     }
     break;
    case 8:
     g->add_msg("It's a good time to lie down and sleep.");
     add_disease(DI_LYING_DOWN, 200, g);
     break;
    case 9:
     g->add_msg("You have the sudden urge to SCREAM!");
     g->sound(posx, posy, 10 + 2 * str_cur, "AHHHHHHH!");
     break;
    case 10:
     g->add_msg(std::string(name + name + name + name + name + name + name +
                            name + name + name + name + name + name + name +
                            name + name + name + name + name + name).c_str());
     break;
    case 11:
     add_disease(DI_FORMICATION, 600, g);
     break;
   }
  }

  if (has_trait(PF_JITTERY) && !has_disease(DI_SHAKES)) {
   if (stim > 50 && one_in(300 - stim))
    add_disease(DI_SHAKES, 300 + stim, g);
   else if (hunger > 80 && one_in(500 - hunger))
    add_disease(DI_SHAKES, 400, g);
  }

  if (has_trait(PF_MOODSWINGS) && one_in(3600)) {
   if (rng(1, 20) > 9)	// 55% chance
    add_morale(MORALE_MOODSWING, -100, -500);
   else			// 45% chance
    add_morale(MORALE_MOODSWING, 100, 500);
  }

  if (has_trait(PF_VOMITOUS) && one_in(4200))
   vomit(g);

  if (has_trait(PF_SHOUT1) && one_in(3600))
   g->sound(posx, posy, 10 + 2 * str_cur, "You shout loudly!");
  if (has_trait(PF_SHOUT2) && one_in(2400))
   g->sound(posx, posy, 15 + 3 * str_cur, "You scream loudly!");
  if (has_trait(PF_SHOUT3) && one_in(1800))
   g->sound(posx, posy, 20 + 4 * str_cur, "You let out a piercing howl!");
 }	// Done with while-awake-only effects

 if (has_trait(PF_ASTHMA) && one_in(3600 - stim * 50)) {
  bool auto_use = has_charges("inhaler", 1);
  if (underwater) {
   oxygen = int(oxygen / 2);
   auto_use = false;
  }
  if (has_disease(DI_SLEEP)) {
   rem_disease(DI_SLEEP);
   g->add_msg("Your asthma wakes you up!");
   auto_use = false;
  }
  if (auto_use)
   use_charges("inhaler", 1);
  else {
   add_disease(DI_ASTHMA, 50 * rng(1, 4), g);
   if (!is_npc())
    g->cancel_activity_query("You have an asthma attack!");
  }
 }

 if (has_trait(PF_LEAVES) && g->is_in_sunlight(posx, posy) && one_in(600))
  hunger--;

 if (pain > 0) {
  if (has_trait(PF_PAINREC1) && one_in(600))
   pain--;
  if (has_trait(PF_PAINREC2) && one_in(300))
   pain--;
  if (has_trait(PF_PAINREC3) && one_in(150))
   pain--;
 }

 if (has_trait(PF_ALBINO) && g->is_in_sunlight(posx, posy) && one_in(20)) {
  g->add_msg("The sunlight burns your skin!");
  if (has_disease(DI_SLEEP)) {
   rem_disease(DI_SLEEP);
   g->add_msg("You wake up!");
  }
  hurtall(1);
 }

 if ((has_trait(PF_TROGLO) || has_trait(PF_TROGLO2)) &&
     g->is_in_sunlight(posx, posy) && g->weather == WEATHER_SUNNY) {
  str_cur--;
  dex_cur--;
  int_cur--;
  per_cur--;
 }
 if (has_trait(PF_TROGLO2) && g->is_in_sunlight(posx, posy)) {
  str_cur--;
  dex_cur--;
  int_cur--;
  per_cur--;
 }
 if (has_trait(PF_TROGLO3) && g->is_in_sunlight(posx, posy)) {
  str_cur -= 4;
  dex_cur -= 4;
  int_cur -= 4;
  per_cur -= 4;
 }

 if (has_trait(PF_SORES)) {
  for (int i = bp_head; i < num_bp; i++) {
   if (pain < 5 + 4 * abs(encumb(body_part(i))))
    pain = 5 + 4 * abs(encumb(body_part(i)));
  }
 }

 if (has_trait(PF_SLIMY)) {
  if (g->m.field_at(posx, posy).type == fd_null)
   g->m.add_field(g, posx, posy, fd_slime, 1);
  else if (g->m.field_at(posx, posy).type == fd_slime &&
           g->m.field_at(posx, posy).density < 3)
   g->m.field_at(posx, posy).density++;
 }

 if (has_trait(PF_WEB_WEAVER) && one_in(3)) {
  if (g->m.field_at(posx, posy).type == fd_null ||
      g->m.field_at(posx, posy).type == fd_slime)
   g->m.add_field(g, posx, posy, fd_web, 1);
  else if (g->m.field_at(posx, posy).type == fd_web &&
           g->m.field_at(posx, posy).density < 3)
   g->m.field_at(posx, posy).density++;
 }

 if (has_trait(PF_RADIOGENIC) && int(g->turn) % 50 == 0 && radiation >= 10) {
  radiation -= 10;
  healall(1);
 }

 if (has_trait(PF_RADIOACTIVE1)) {
  if (g->m.radiation(posx, posy) < 10 && one_in(50))
   g->m.radiation(posx, posy)++;
 }
 if (has_trait(PF_RADIOACTIVE2)) {
  if (g->m.radiation(posx, posy) < 20 && one_in(25))
   g->m.radiation(posx, posy)++;
 }
 if (has_trait(PF_RADIOACTIVE3)) {
  if (g->m.radiation(posx, posy) < 30 && one_in(10))
   g->m.radiation(posx, posy)++;
 }

 if (has_trait(PF_UNSTABLE) && one_in(28800))	// Average once per 2 days
  mutate(g);
 if (has_artifact_with(AEP_MUTAGENIC) && one_in(28800))
  mutate(g);
 if (has_artifact_with(AEP_FORCE_TELEPORT) && one_in(600))
  g->teleport(this);

 if (is_wearing("hazmat_suit")) {
  if (radiation < int((100 * g->m.radiation(posx, posy)) / 20))
   radiation += rng(0, g->m.radiation(posx, posy) / 20);
 } else if (radiation < int((100 * g->m.radiation(posx, posy)) / 8))
  radiation += rng(0, g->m.radiation(posx, posy) / 8);

 if (rng(1, 2500) < radiation && (int(g->turn) % 150 == 0 || radiation > 2000)){
  mutate(g);
  if (radiation > 2000)
   radiation = 2000;
  radiation /= 2;
  radiation -= 5;
  if (radiation < 0)
   radiation = 0;
 }

// Negative bionics effects
 if (has_bionic("bio_dis_shock") && one_in(1200)) {
  g->add_msg("You suffer a painful electrical discharge!");
  pain++;
  moves -= 150;
 }
 if (has_bionic("bio_dis_acid") && one_in(1500)) {
  g->add_msg("You suffer a burning acidic discharge!");
  hurtall(1);
 }
 if (has_bionic("bio_drain") && power_level > 0 && one_in(600)) {
  g->add_msg("Your batteries discharge slightly.");
  power_level--;
 }
 if (has_bionic("bio_noise") && one_in(500)) {
  g->add_msg("A bionic emits a crackle of noise!");
  g->sound(posx, posy, 60, "");
 }
 if (has_bionic("bio_power_weakness") && max_power_level > 0 &&
     power_level >= max_power_level * .75)
  str_cur -= 3;

// Artifact effects
 if (has_artifact_with(AEP_ATTENTION))
  add_disease(DI_ATTENTION, 3, g);

 if (dex_cur < 0)
  dex_cur = 0;
 if (str_cur < 0)
  str_cur = 0;
 if (per_cur < 0)
  per_cur = 0;
 if (int_cur < 0)
  int_cur = 0;
}

void player::vomit(game *g)
{
 g->add_msg("You throw up heavily!");
 hunger += rng(30, 50);
 thirst += rng(30, 50);
 moves -= 100;
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == DI_FOODPOISON) {
   illness[i].duration -= 300;
   if (illness[i].duration < 0)
    rem_disease(illness[i].type);
  } else if (illness[i].type == DI_DRUNK) {
   illness[i].duration -= rng(1, 5) * 100;
   if (illness[i].duration < 0)
    rem_disease(illness[i].type);
  }
 }
 rem_disease(DI_PKILL1);
 rem_disease(DI_PKILL2);
 rem_disease(DI_PKILL3);
 rem_disease(DI_SLEEP);
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
 int ret = 400 + str * 35;
 if (has_trait(PF_BADBACK))
  ret = int(ret * .65);
 if (has_trait(PF_LIGHT_BONES))
  ret = int(ret * .80);
 if (has_trait(PF_HOLLOW_BONES))
  ret = int(ret * .60);
 if (has_artifact_with(AEP_CARRY_MORE))
  ret += 200;
 return ret;
}

int player::volume_capacity()
{
 int ret = 2;	// A small bonus (the overflow)
 it_armor *armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  ret += armor->storage;
 }
 if (has_bionic("bio_storage"))
  ret += 6;
 if (has_trait(PF_SHELL))
  ret += 16;
 if (has_trait(PF_PACKMULE))
  ret = int(ret * 1.4);
 return ret;
}

bool player::can_pickVolume(int volume)
{
    return (volume_carried() + volume <= volume_capacity());
}
bool player::can_pickWeight(int weight)
{
    return (weight_carried() + weight <= weight_capacity());
}
int player::morale_level()
{
 std::stringstream morale_text;
 int ret = 0;
 for (int i = 0; i < morale.size(); i++)
  ret += morale[i].bonus;

 if (has_trait(PF_HOARDER)) {
  int pen = int((volume_capacity()-volume_carried()) / 2);
  if (pen > 70)
   pen = 70;
  if (pen <= 0)
   pen = 0;
  if (has_disease(DI_TOOK_XANAX))
   pen = int(pen / 7);
  else if (has_disease(DI_TOOK_PROZAC))
   pen = int(pen / 2);
  add_morale(MORALE_PERM_HOARDER, -pen, -pen);
 }

 if (has_trait(PF_MASOCHIST)) {
  int bonus = pain / 2.5;
  if (bonus > 25)
   bonus = 25;
  if (has_disease(DI_TOOK_PROZAC))
   bonus = int(bonus / 3);
  if (bonus != 0)
   add_morale(MORALE_PERM_MASOCHIST, bonus, bonus);
 }
 // Optimist gives a straight +20 to morale.
 if (has_trait(PF_OPTIMISTIC)) {
  add_morale(MORALE_PERM_OPTIMIST, 20, 20);
 }

 if (has_disease(DI_TOOK_PROZAC) && ret < 0)
  ret = int(ret / 4);

 return ret;
}

void player::add_morale(morale_type type, int bonus, int max_bonus,
                        itype* item_type)
{
 bool placed = false;

 for (int i = 0; i < morale.size() && !placed; i++) {
  if (morale[i].type == type && morale[i].item_type == item_type) {
   placed = true;
   if (abs(morale[i].bonus) < abs(max_bonus) || max_bonus == 0) {
    morale[i].bonus += bonus;
    if (abs(morale[i].bonus) > abs(max_bonus) && max_bonus != 0)
     morale[i].bonus = max_bonus;
   }
  }
 }
 if (!placed) { // Didn't increase an existing point, so add a new one
  morale_point tmp(type, item_type, bonus);
  morale.push_back(tmp);
 }
}

item& player::i_add(item it, game *g)
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

bool player::has_active_item(itype_id id)
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

void player::process_active_items(game *g)
{
 it_tool* tmp;
 iuse use;
 if (weapon.is_artifact() && weapon.is_tool())
  g->process_artifact(&weapon, this, true);
 else if (weapon.active) {
  if (weapon.has_flag(IF_CHARGE)) { // We're chargin it up!
   if (weapon.charges == 8) {
    bool maintain = false;
    if (use_charges_if_avail("UPS_on", 4)) {
     maintain = true;
    } else if (use_charges_if_avail("UPS_off", 4)) {
     maintain = true;
    }
    if (maintain) {
     if (one_in(20)) {
      g->add_msg("Your %s discharges!", weapon.tname().c_str());
      point target(posx + rng(-12, 12), posy + rng(-12, 12));
      std::vector<point> traj = line_to(posx, posy, target.x, target.y, 0);
      g->fire(*this, target.x, target.y, traj, false);
     } else
      g->add_msg("Your %s beeps alarmingly.", weapon.tname().c_str());
    }
   } else {
    if (use_charges_if_avail("UPS_on", 1 + weapon.charges)) {
     weapon.poison++;
    } else if (use_charges_if_avail("UPS_off", 1 + weapon.charges)) {
     weapon.poison++;
    } else {
     g->add_msg("Your %s spins down.", weapon.tname().c_str());
     if (weapon.poison <= 0) {
      weapon.charges--;
      weapon.poison = weapon.charges - 1;
     } else
      weapon.poison--;
     if (weapon.charges == 0)
      weapon.active = false;
    }
    if (weapon.poison >= weapon.charges) {
     weapon.charges++;
     weapon.poison = 0;
    }
   }
   return;
  } // if (weapon.has_flag(IF_CHARGE))
	if (weapon.is_food()) {	// food items
	  if (weapon.has_flag(IF_HOT)) {
			weapon.item_counter--;
			if (weapon.item_counter == 0) {
				weapon.item_flags ^= mfb(IF_HOT);
				weapon.active = false;
			}
		}
		return;
	} else if (weapon.is_food_container()) {	// food items
	  if (weapon.contents[0].has_flag(IF_HOT)) {
			weapon.contents[0].item_counter--;
			if (weapon.contents[0].item_counter == 0) {
				weapon.contents[0].item_flags ^= mfb(IF_HOT);
				weapon.contents[0].active = false;
			}
		}
		return;
	}
  if (!weapon.is_tool()) {
   debugmsg("%s is active, but it is not a tool.", weapon.tname().c_str());
   return;
  }
  tmp = dynamic_cast<it_tool*>(weapon.type);
  (use.*tmp->use)(g, this, &weapon, true);
  if (tmp->turns_per_charge > 0 && int(g->turn) % tmp->turns_per_charge == 0)
   weapon.charges--;
  if (weapon.charges <= 0) {
   (use.*tmp->use)(g, this, &weapon, false);
   if (tmp->revert_to == "null")
    weapon = ret_null;
   else
    weapon.type = g->itypes[tmp->revert_to];
  }
 }

    std::vector<item*> inv_active = inv.active_items();
    for (std::vector<item*>::iterator iter = inv_active.begin(); iter != inv_active.end(); ++iter)
    {
        item *tmp_it = *iter;
        if (tmp_it->is_artifact() && tmp_it->is_tool())
        {
            g->process_artifact(tmp_it, this);
        }
        if (tmp_it->active ||
            (tmp_it->is_container() && tmp_it->contents.size() > 0 && tmp_it->contents[0].active))
        {
            if (tmp_it->is_food())
            {
                if (tmp_it->has_flag(IF_HOT))
                {
                    tmp_it->item_counter--;
                    if (tmp_it->item_counter == 0)
                    {
                        tmp_it->item_flags ^= mfb(IF_HOT);
                        tmp_it->active = false;
                    }
                }
            }
            else if (tmp_it->is_food_container())
            {
                if (tmp_it->contents[0].has_flag(IF_HOT))
                {
                    tmp_it->contents[0].item_counter--;
                    if (tmp_it->contents[0].item_counter == 0)
                    {
                        tmp_it->contents[0].item_flags ^= mfb(IF_HOT);
                        tmp_it->contents[0].active = false;
                    }
                }
            }
            else
            {
                tmp = dynamic_cast<it_tool*>(tmp_it->type);
                (use.*tmp->use)(g, this, tmp_it, true);
                if (tmp->turns_per_charge > 0 && int(g->turn) % tmp->turns_per_charge == 0)
                {
                    tmp_it->charges--;
                }
                if (tmp_it->charges <= 0)
                {
                    (use.*tmp->use)(g, this, tmp_it, false);
                    if (tmp->revert_to == "null")
                    {
                        inv.remove_item(tmp_it);
                    }
                    else
                    {
                        tmp_it->type = g->itypes[tmp->revert_to];
                    }
                }
            }
        }
    }

// worn items
  for (int i = 0; i < worn.size(); i++) {
    if (worn[i].is_artifact())
    g->process_artifact(&(worn[i]), this);
  }
}

item player::remove_weapon()
{
 if (weapon.has_flag(IF_CHARGE) && weapon.active) { //unwield a charged charge rifle.
  weapon.charges = 0;
  weapon.active = false;
 }
 item tmp = weapon;
 weapon = ret_null;
// We need to remove any boosts related to our style
 rem_disease(DI_ATTACK_BOOST);
 rem_disease(DI_DODGE_BOOST);
 rem_disease(DI_DAMAGE_BOOST);
 rem_disease(DI_SPEED_BOOST);
 rem_disease(DI_ARMOR_BOOST);
 rem_disease(DI_VIPER_COMBO);
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
  return inv.remove_item_by_letter(let);
 return ret_null;
}

item player::i_rem(itype_id type)
{
    item ret;
    if (weapon.type->id == type)
    {
        return remove_weapon();
    }
    return inv.remove_item_by_type(type);
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
 return ret_null;
}

std::vector<item> player::inv_dump()
{
 std::vector<item> ret;
 if (std::find(standard_itype_ids.begin(), standard_itype_ids.end(), weapon.type->id) != standard_itype_ids.end()){
  ret.push_back(weapon);
 }
 for (int i = 0; i < worn.size(); i++)
  ret.push_back(worn[i]);
 inv.dump(ret);
 return ret;
}

item player::i_remn(char invlet)
{
 return inv.remove_item_by_letter(invlet);
}

void player::use_amount(itype_id it, int quantity, bool use_container)
{
 bool used_weapon_contents = false;
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[0].type->id == it) {
   quantity--;
   weapon.contents.erase(weapon.contents.begin() + 0);
   i--;
   used_weapon_contents = true;
  }
 }
 if (use_container && used_weapon_contents)
  remove_weapon();

 if (weapon.type->id == it) {
  quantity--;
  remove_weapon();
 }

 inv.use_amount(it, quantity, use_container);
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

    if (has_charges("torch_lit", 1)) {
        return true;
    } else if (has_charges("candle_lit", 1)) {
        return true;
    } else if (has_bionic("bio_tools")) {
        return true;
    } else if (has_bionic("bio_lighter")) {
        return true;
    } else if (has_bionic("bio_laser")) {
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

    if (has_charges("torch_lit", 1)) {
        return;
    } else if (has_charges("candle_lit", 1)) {
        return;
    } else if (has_bionic("bio_tools")) {
        return;
    } else if (has_bionic("bio_lighter")) {
        return;
    } else if (has_bionic("bio_laser")) {
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
        use_charges("welder", quantity);
        return;
    } else if (has_charges("welder", quantity)) {
        use_charges("welder", quantity);
        return;
    }
}

void player::use_charges(itype_id it, int quantity)
{
 if (it == "toolset") {
  power_level -= quantity;
  if (power_level < 0)
   power_level = 0;
  return;
 }
 if (it == "fire")
 {
     return use_fire(quantity);
 }

// Start by checking weapon contents
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it) {
   if (weapon.contents[i].charges > 0 &&
       weapon.contents[i].charges <= quantity) {
    quantity -= weapon.contents[i].charges;
    if (weapon.contents[i].destroyed_at_zero_charges()) {
     weapon.contents.erase(weapon.contents.begin() + i);
     i--;
    } else
     weapon.contents[i].charges = 0;
    if (quantity == 0)
     return;
   } else {
    weapon.contents[i].charges -= quantity;
    return;
   }
  }
 }

 if (weapon.type->id == it) {
  if (weapon.charges > 0 && weapon.charges <= quantity) {
   quantity -= weapon.charges;
   if (weapon.destroyed_at_zero_charges())
    remove_weapon();
   else
    weapon.charges = 0;
   if (quantity == 0)
    return;
   } else {
    weapon.charges -= quantity;
    return;
   }
  }

 inv.use_charges(it, quantity);
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
 if (weapon.damage_cut() >= 10 && !weapon.has_flag(IF_SPEAR)) {
  int factor = weapon.volume() * 5 - weapon.weight() * 1.5 -
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
 std::vector<item*> drives = inv.all_items_by_type("usb_drive");

 if (drives.empty())
  return NULL; // None available!

 std::vector<std::string> selections;
 for (int i = 0; i < drives.size() && i < 9; i++)
  selections.push_back( drives[i]->tname() );

 int select = menu_vec(false, "Choose drive:", selections);

 return drives[ select - 1 ];
}

bool player::is_wearing(itype_id it)
{
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == it)
   return true;
 }
 return false;
}

bool player::has_artifact_with(art_effect_passive effect)
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

int player::amount_of(itype_id it)
{
    if (it == "toolset" && has_bionic("bio_tools"))
    {
        return 1;
    }
    if (it == "apparatus")
    {
        if (has_amount("crackpipe", 1) || has_amount("can_drink", 1) )
        {
            return 1;
        }
    }
    int quantity = 0;
    if (weapon.type->id == it)
    {
        quantity++;
    }

    for (int i = 0; i < weapon.contents.size(); i++)
    {
        if (weapon.contents[i].type->id == it)
        {
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
 if (weapon.type->id == it)
  quantity += weapon.charges;
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
   it_container* cont = dynamic_cast<it_container*>(weapon.type);
   if (cont->flags & mfb(con_wtight) && cont->flags & mfb(con_seals))
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

bool player::has_item(char let)
{
 return (has_weapon_or_armor(let) || !inv.item_by_letter(let).is_null());
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

bool player::eat(game *g, char ch)
{
    it_comest *comest = NULL;
    item *eaten = NULL;
    int which = -3; // Helps us know how to delete the item which got eaten
    if (ch == -2)
    {
        g->add_msg("You do not have that item.");
        return false;
    }
    else if (ch == -1)
    {
        if (weapon.is_food_container(this))
        {
            eaten = &weapon.contents[0];
            which = -2;
            if (weapon.contents[0].is_food())
                comest = dynamic_cast<it_comest*>(weapon.contents[0].type);
        }
        else if (weapon.is_food(this))
        {
            eaten = &weapon;
            which = -1;
            if (weapon.is_food())
                comest = dynamic_cast<it_comest*>(weapon.type);
        }
        else
        {
            if (!is_npc())
                g->add_msg("You can't eat your %s.", weapon.tname(g).c_str());
            else
                debugmsg("%s tried to eat a %s", name.c_str(), weapon.tname(g).c_str());
            return false;
        }
    }
    else
    {
        item& it = inv.item_by_letter(ch);
        if (it.is_food_container(this))
        {
            eaten = &(it.contents[0]);
            which = 1;
            if (it.contents[0].is_food())
                comest = dynamic_cast<it_comest*>(it.contents[0].type);
        }
        else if (it.is_food(this))
        {
            eaten = &it;
            which = 0;
            if (it.is_food())
                comest = dynamic_cast<it_comest*>(it.type);
        }
        else
        {
            if (!is_npc())
                g->add_msg("You can't eat your %s.", it.tname(g).c_str());
            else
                debugmsg("%s tried to eat a %s", name.c_str(), it.tname(g).c_str());
            return false;
        }
    }
    if (eaten == NULL)
        return false;

    if (eaten->is_ammo())   // For when bionics let you eat fuel
    {
        const int factor = 20;
        int max_change = max_power_level - power_level;
        if (max_change == 0 && !is_npc())
        {
            g->add_msg("Your internal power storage is fully powered.");
        }
        charge_power(eaten->charges / factor);
        eaten->charges -= max_change * factor; //negative charges seem to be okay
        eaten->charges++; //there's a flat subtraction later
    }
    else if (!eaten->type->is_food() && !eaten->is_food_container(this))
    {
            // For when bionics let you burn organic materials
        if (eaten->type->is_book()) {
            it_book* book = dynamic_cast<it_book*>(eaten->type);
            if (book->type != NULL && !query_yn("Really eat %s?", book->name.c_str()))
                return false;
        }
        int charge = (eaten->volume() + eaten->weight()) / 2;
        if (eaten->type->m1 == LEATHER || eaten->type->m2 == LEATHER)
            charge /= 4;
        if (eaten->type->m1 == WOOD    || eaten->type->m2 == WOOD)
            charge /= 2;
        charge_power(charge);
    }
    else     // It's real food!  i.e. an it_comest
    {
        // Remember, comest points to the it_comest data
        if (comest == NULL)
        {
            debugmsg("player::eat(%s); comest is NULL!", eaten->tname(g).c_str());
            return false;
        }
        if (comest->tool != "null")
        {
            bool has = has_amount(comest->tool, 1);
   if (g->itypes[comest->tool]->count_by_charges())
    has = has_charges(comest->tool, 1);
   if (!has) {
                if (!is_npc())
                    g->add_msg("You need a %s to consume that!",
                               g->itypes[comest->tool]->name.c_str());
                return false;
            }
        }
        bool overeating = (!has_trait(PF_GOURMAND) && hunger < 0 &&
                           comest->nutr >= 5);
        bool spoiled = eaten->rotten(g);

        last_item = itype_id(eaten->type->id);

        if (overeating && !is_npc() &&
                !query_yn("You're full.  Force yourself to eat?"))
            return false;

        if (has_trait(PF_CARNIVORE) && eaten->made_of(VEGGY) && comest->nutr > 0)
        {
            if (!is_npc())
                g->add_msg("You can only eat meat!");
            else
                g->add_msg("Carnivore %s tried to eat meat!", name.c_str());
            return false;
        }
        if (!has_trait(PF_CANNIBAL) && eaten->made_of(HFLESH)&& !is_npc() &&
                !query_yn("The thought of eating that makes you feel sick. Really do it?"))
            return false;

        if (has_trait(PF_VEGETARIAN) && eaten->made_of(FLESH) && !is_npc() &&
                !query_yn("Really eat that meat? Your stomach won't be happy."))
            return false;

        if (spoiled)
        {
            if (is_npc())
                return false;
            if (!has_trait(PF_SAPROVORE) &&
                    !query_yn("This %s smells awful!  Eat it?", eaten->tname(g).c_str()))
                return false;
            g->add_msg("Ick, this %s doesn't taste so good...",eaten->tname(g).c_str());
            if (!has_trait(PF_SAPROVORE) && (!has_bionic("bio_digestion") || one_in(3)))
                add_disease(DI_FOODPOISON, rng(60, (comest->nutr + 1) * 60), g);
            hunger -= rng(0, comest->nutr);
            thirst -= comest->quench;
            if (!has_trait(PF_SAPROVORE) && !has_bionic("bio_digestion"))
                health -= 3;
        }
        else
        {
            hunger -= comest->nutr;
            thirst -= comest->quench;
            if (has_bionic("bio_digestion"))
                hunger -= rng(0, comest->nutr);
            else if (!has_trait(PF_GOURMAND))
            {
                if ((overeating && rng(-200, 0) > hunger))
                    vomit(g);
            }
            health += comest->healthy;
        }
        // At this point, we've definitely eaten the item, so use up some turns.
        if (has_trait(PF_GOURMAND))
            moves -= 150;
        else
            moves -= 250;
        // If it's poisonous... poison us.  TODO: More several poison effects
        if (eaten->poison >= rng(2, 4))
            add_disease(DI_POISON, eaten->poison * 100, g);
        if (eaten->poison > 0)
            add_disease(DI_FOODPOISON, eaten->poison * 300, g);

        // Descriptive text
        if (!is_npc())
        {
            if (eaten->made_of(LIQUID))
                g->add_msg("You drink your %s.", eaten->tname(g).c_str());
            else if (comest->nutr >= 5)
                g->add_msg("You eat your %s.", eaten->tname(g).c_str());
        }
        else if (g->u_see(posx, posy))
            g->add_msg("%s eats a %s.", name.c_str(), eaten->tname(g).c_str());

        if (g->itypes[comest->tool]->is_tool())
            use_charges(comest->tool, 1); // Tools like lighters get used
        if (comest->stim > 0)
        {
            if (comest->stim < 10 && stim < comest->stim)
            {
                stim += comest->stim;
                if (stim > comest->stim)
                    stim = comest->stim;
            }
            else if (comest->stim >= 10 && stim < comest->stim * 3)
                stim += comest->stim;
        }

        iuse use;
        if (comest->use != &iuse::none)
        {
            (use.*comest->use)(g, this, eaten, false);
        }
        add_addiction(comest->add, comest->addict);
        if (has_bionic("bio_ethanol") && comest->use == &iuse::alcohol)
            charge_power(rng(2, 8));
        if (has_bionic("bio_ethanol") && comest->use == &iuse::alcohol_weak)
            charge_power(rng(1, 4));

        if (eaten->made_of(HFLESH)) {
          if (has_trait(PF_CANNIBAL)) {
              g->add_msg_if_player(this, "You feast upon the human flesh.");
              add_morale(MORALE_CANNIBAL, 15, 100);
          } else {
              g->add_msg_if_player(this, "You feel horrible for eating a person..");
              add_morale(MORALE_CANNIBAL, -60, -400);
          }
        }
        if (has_trait(PF_VEGETARIAN) && (eaten->made_of(FLESH) || eaten->made_of(HFLESH)))
        {
            if (!is_npc())
                g->add_msg("Almost instantly you feel a familiar pain in your stomach");
            add_morale(MORALE_VEGETARIAN, -75, -400);
        }
        if ((has_trait(PF_HERBIVORE) || has_trait(PF_RUMINANT)) &&
                eaten->made_of(FLESH))
        {
            if (!one_in(3))
                vomit(g);
            if (comest->quench >= 2)
                thirst += int(comest->quench / 2);
            if (comest->nutr >= 2)
                hunger += int(comest->nutr * .75);
        }
        if (eaten->has_flag(IF_HOT) && eaten->has_flag(IF_EATEN_HOT))
            add_morale(MORALE_FOOD_HOT, 5, 10);
        if (has_trait(PF_GOURMAND))
        {
            if (comest->fun < -2)
                add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 4, comest);
            else if (comest->fun > 0)
                add_morale(MORALE_FOOD_GOOD, comest->fun * 3, comest->fun * 6, comest);
            if (!is_npc() && (hunger < -60 || thirst < -60))
                g->add_msg("You can't finish it all!");
            if (hunger < -60)
                hunger = -60;
            if (thirst < -60)
                thirst = -60;
        }
        else
        {
            if (comest->fun < 0)
                add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 6, comest);
            else if (comest->fun > 0)
                add_morale(MORALE_FOOD_GOOD, comest->fun * 2, comest->fun * 4, comest);
            if (!is_npc() && (hunger < -20 || thirst < -20))
                g->add_msg("You can't finish it all!");
            if (hunger < -20)
                hunger = -20;
            if (thirst < -20)
                thirst = -20;
        }
    }

    eaten->charges--;
    if (eaten->charges <= 0)
    {
        if (which == -1)
            weapon = ret_null;
        else if (which == -2)
        {
            weapon.contents.erase(weapon.contents.begin());
            if (!is_npc())
                g->add_msg("You are now wielding an empty %s.", weapon.tname(g).c_str());
        }
        else if (which == 0)
            inv.remove_item_by_letter(ch);
        else if (which >= 0)
        {
            item& it = inv.item_by_letter(ch);
            it.contents.erase(it.contents.begin());
            if (!is_npc())
            {
                switch ((int)OPTIONS[OPT_DROP_EMPTY])
                {
                case 0:
                    g->add_msg("%c - an empty %s", it.invlet,
                               it.tname(g).c_str());
                    break;
                case 1:
                    if (it.is_container())
                    {
                        it_container* cont = dynamic_cast<it_container*>(it.type);
                        if (!(cont->flags & mfb(con_wtight) && cont->flags & mfb(con_seals)))
                        {
                            g->add_msg("You drop the empty %s.", it.tname(g).c_str());
                            g->m.add_item(posx, posy, inv.remove_item_by_letter(it.invlet));
                        }
                        else
                            g->add_msg("%c - an empty %s", it.invlet,
                                       it.tname(g).c_str());
                    }
                    else if (it.type->id == "wrapper") // hack because wrappers aren't containers
                    {
                        g->add_msg("You drop the empty %s.", it.tname(g).c_str());
                        g->m.add_item(posx, posy, inv.remove_item_by_letter(it.invlet));
                    }
                    break;
                case 2:
                    g->add_msg("You drop the empty %s.", it.tname(g).c_str());
                    g->m.add_item(posx, posy, inv.remove_item_by_letter(it.invlet));
                    break;
                }
            }
            if (inv.stack_by_letter(it.invlet).size() > 0)
                inv.restack(this);
            inv.unsort();
        }
    }
    return true;
}

bool player::wield(game *g, char ch)
{
 if (weapon.has_flag(IF_NO_UNWIELD)) {
  g->add_msg("You cannot unwield your %s!  Withdraw them with 'p'.",
             weapon.tname().c_str());
  return false;
 }
 if (ch == -3) {
  bool pickstyle = (!styles.empty());
  if (weapon.is_style())
   remove_weapon();
  else if (!is_armed()) {
   if (!pickstyle) {
    g->add_msg("You are already wielding nothing.");
    return false;
   }
  } else if (volume_carried() + weapon.volume() < volume_capacity()) {
   inv.push_back(remove_weapon());
   inv.unsort();
   moves -= 20;
   recoil = 0;
   if (!pickstyle)
    return true;
  } else if (query_yn("No space in inventory for your %s.  Drop it?",
                      weapon.tname(g).c_str())) {
   g->m.add_item(posx, posy, remove_weapon());
   recoil = 0;
   if (!pickstyle)
    return true;
  } else
   return false;

  if (pickstyle) {
   weapon = item( g->itypes[style_selected], 0 );
   weapon.invlet = ':';
   return true;
  }
 }
 if (ch == 0) {
  g->add_msg("You're already wielding that!");
  return false;
 } else if (ch == -2) {
  g->add_msg("You don't have that item.");
  return false;
 }

 item& it = inv.item_by_letter(ch);
 if (it.is_two_handed(this) && !has_two_arms()) {
  g->add_msg("You cannot wield a %s with only one arm.",
             it.tname(g).c_str());
  return false;
 }
 if (!is_armed()) {
  weapon = inv.remove_item_by_letter(ch);
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
  weapon = inv.remove_item_by_letter(ch);
  inv.push_back(tmpweap);
  inv.unsort();
  moves -= 45;
  if (weapon.is_artifact() && weapon.is_tool()) {
   it_artifact_tool *art = dynamic_cast<it_artifact_tool*>(weapon.type);
   g->add_artifact_messages(art->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (query_yn("No space in inventory for your %s.  Drop it?",
                     weapon.tname(g).c_str())) {
  g->m.add_item(posx, posy, remove_weapon());
  weapon = it;
  inv.remove_item_by_letter(weapon.invlet);
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

void player::pick_style(game *g) // Style selection menu
{
 std::vector<std::string> options;
 options.push_back("No style");
 for (int i = 0; i < styles.size(); i++)
  options.push_back( g->itypes[styles[i]]->name );
 int selection = menu_vec(false, "Select a style", options);
 if (selection >= 2)
  style_selected = styles[selection - 2];
 else
  style_selected = "null";
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
 if (has_trait(PF_WOOLALLERGY) && it->made_of(WOOL)) {
  return HINT_IFFY; //should this be HINT_CANT? I kinda think not, because HINT_CANT is more for things that can NEVER happen
 }
 if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_WEBBED)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_TALONS)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_mouth) && has_trait(PF_BEAK)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_feet) && has_trait(PF_HOOVES)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && has_trait(PF_HORNS_CURLED)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_torso) && has_trait(PF_SHELL)) {
  return HINT_IFFY;
 }
 if (armor->covers & mfb(bp_head) && !it->made_of(WOOL) &&
     !it->made_of(COTTON) && !it->made_of(LEATHER) &&
     (has_trait(PF_HORNS_POINTED) || has_trait(PF_ANTENNAE) ||
      has_trait(PF_ANTLERS))) {
  return HINT_IFFY;
 }
 // Checks to see if the player is wearing not cotton or not wool, ie leather/plastic shoes
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) && !(it->made_of(WOOL) || it->made_of(COTTON))) {
  for (int i = 0; i < worn.size(); i++) {
   item *worn_item = &worn[i];
   it_armor *worn_armor = dynamic_cast<it_armor*>(worn_item->type);
   if( worn_armor->covers & mfb(bp_feet) && !(worn_item->made_of(WOOL) || worn_item->made_of(COTTON))) {
    return HINT_IFFY;
   }
  }
 }
 return HINT_GOOD;
}

bool player::wear(game *g, char let)
{
 item* to_wear = NULL;
 int index = -1;
 if (weapon.invlet == let) {
  to_wear = &weapon;
  index = -2;
 } else {
  to_wear = &inv.item_by_letter(let);
 }

 if (to_wear == NULL) {
  g->add_msg("You don't have item '%c'.", let);
  return false;
 }

 if (!wear_item(g, to_wear))
  return false;

 if (index == -2)
  weapon = ret_null;
 else
  inv.remove_item(to_wear);

 return true;
}

bool player::wear_item(game *g, item *to_wear)
{
 it_armor* armor = NULL;
 if (to_wear->is_armor())
  armor = dynamic_cast<it_armor*>(to_wear->type);
 else {
  g->add_msg("Putting on a %s would be tricky.", to_wear->tname(g).c_str());
  return false;
 }

 // are we trying to put on power armor? If so, make sure we don't have any other gear on.
 if (armor->is_power_armor()) {
   if (worn.size() && armor->covers & mfb(bp_torso)) {
     g->add_msg("You can't wear power armor over other gear!");
     return false;
   } else if (!(armor->covers & mfb(bp_torso)) && (!worn.size() || !((it_armor *)worn[0].type)->is_power_armor())) {
     g->add_msg("You can only wear power armor components with power armor!");
     return false;
   }

   for (int i = 0; i < worn.size(); i++) {
     if (((it_armor *)worn[i].type)->is_power_armor() && worn[i].type == armor) {
       g->add_msg("You cannot wear more than one %s!", to_wear->tname().c_str());
       return false;
     }
   }
 } else {
   // Only helmets can be worn with power armor, except other power armor components
   if (worn.size() && ((it_armor *)worn[0].type)->is_power_armor() && !(armor->covers & (mfb(bp_head) | mfb(bp_eyes)))) {
     g->add_msg("You can't wear %s with power armor!", to_wear->tname().c_str());
     return false;
   }
 }

// Make sure we're not wearing 2 of the item already
 int count = 0;
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].type->id == to_wear->type->id)
   count++;
 }
 if (count == 2) {
  g->add_msg("You can't wear more than two %s at once.",
             to_wear->tname().c_str());
  return false;
 }
 if (has_trait(PF_WOOLALLERGY) && to_wear->made_of(WOOL)) {
  g->add_msg("You can't wear that, it's made of wool!");
  return false;
 }
 if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0) {
  g->add_msg("You can't wear a%s helmet!",
             wearing_something_on(bp_head) ? "nother" : "");
  return false;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_WEBBED)) {
  g->add_msg("You cannot put %s over your webbed hands.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_TALONS)) {
  g->add_msg("You cannot put %s over your talons.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_mouth) && has_trait(PF_BEAK)) {
  g->add_msg("You cannot put a %s over your beak.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_feet) && has_trait(PF_HOOVES)) {
  g->add_msg("You cannot wear footwear on your hooves.");
  return false;
 }
 if (armor->covers & mfb(bp_head) && has_trait(PF_HORNS_CURLED)) {
  g->add_msg("You cannot wear headgear over your horns.");
  return false;
 }
 if (armor->covers & mfb(bp_torso) && has_trait(PF_SHELL)) {
  g->add_msg("You cannot wear anything over your shell.");
  return false;
 }
 if (armor->covers & mfb(bp_head) && !to_wear->made_of(WOOL) &&
     !to_wear->made_of(COTTON) && !to_wear->made_of(LEATHER) &&
     (has_trait(PF_HORNS_POINTED) || has_trait(PF_ANTENNAE) ||
      has_trait(PF_ANTLERS))) {
  g->add_msg("You cannot wear a helmet over your %s.",
             (has_trait(PF_HORNS_POINTED) ? "horns" :
              (has_trait(PF_ANTENNAE) ? "antennae" : "antlers")));
  return false;
 }
 // Checks to see if the player is wearing not cotton or not wool, ie leather/plastic shoes
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet) && !(to_wear->made_of(WOOL) || to_wear->made_of(COTTON))) {
 for (int i = 0; i < worn.size(); i++) {
  item *worn_item = &worn[i];
  it_armor *worn_armor = dynamic_cast<it_armor*>(worn_item->type);
  if( worn_armor->covers & mfb(bp_feet) && !(worn_item->made_of(WOOL) || worn_item->made_of(COTTON))) {
   g->add_msg("You're already wearing footwear!");
   return false;
  }
 }
}
 g->add_msg("You put on your %s.", to_wear->tname(g).c_str());
 if (to_wear->is_artifact()) {
  it_artifact_armor *art = dynamic_cast<it_artifact_armor*>(to_wear->type);
  g->add_artifact_messages(art->effects_worn);
 }
 moves -= 350; // TODO: Make this variable?
 last_item = itype_id(to_wear->type->id);
 worn.push_back(*to_wear);
 for (body_part i = bp_head; i < num_bp; i = body_part(i + 1)) {
  if (armor->covers & mfb(i) && encumb(i) >= 4)
   g->add_msg("Your %s %s very encumbered! %s",
              body_part_name(body_part(i), 2).c_str(),
              (i == bp_head || i == bp_torso ? "is" : "are"),
              encumb_text(body_part(i)).c_str());
 }
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

bool player::takeoff(game *g, char let)
{
 if (weapon.invlet == let) {
  return wield(g, -3);
 } else {
  for (int i = 0; i < worn.size(); i++) {
   if (worn[i].invlet == let) {
     if (i == 0 && (dynamic_cast<it_armor*>(worn[i].type))->is_power_armor()) {
       // We're trying to take off power armor, but cannot do that if we have a power armor component on!
       for (int i = 1; i < worn.size(); i++) {
         if ((dynamic_cast<it_armor*>(worn[i].type))->is_power_armor()) {
           g->add_msg("You can't take off power armor while wearing other power armor components.");
           return false;
         }
       }
     }
    if (volume_capacity() - (dynamic_cast<it_armor*>(worn[i].type))->storage >
        volume_carried() + worn[i].type->volume) {
     inv.push_back(worn[i]);
     worn.erase(worn.begin() + i);
     inv.unsort();
     return true;
    } else if (query_yn("No room in inventory for your %s.  Drop it?",
                        worn[i].tname(g).c_str())) {
     g->m.add_item(posx, posy, worn[i]);
     worn.erase(worn.begin() + i);
     return true;
    } else
     return false;
   }
  }
  g->add_msg("You are not wearing that item.");
  return false;
 }
}

void player::use_wielded(game *g) {
  use(g, weapon.invlet);
}

hint_rating player::rate_action_reload(item *it) {
 if (it->is_gun()) {
  if (it->has_flag(IF_RELOAD_AND_SHOOT) || it->ammo_type() == AT_NULL) {
   return HINT_CANT;
  }
  if (it->charges == it->clip_size()) {
   int alternate_magazine = -1;
   for (int i = 0; i < it->contents.size(); i++)
   {
       if ((it->contents[i].is_gunmod() &&
            (it->contents[i].typeId() == "spare_mag" &&
             it->contents[i].charges < (dynamic_cast<it_gun*>(it->type))->clip)) ||
           (it->contents[i].has_flag(IF_MODE_AUX) &&
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
  if (tool->ammo == AT_NULL) {
   return HINT_CANT;
  }
  return HINT_GOOD;
 }
 return HINT_CANT;
}

hint_rating player::rate_action_unload(item *it) {
 if (!it->is_gun() && !it->is_container() &&
     (!it->is_tool() || it->ammo_type() == AT_NULL)) {
  return HINT_CANT;
 }
 int spare_mag = -1;
 int has_m203 = -1;
 int has_shotgun = -1;
 if (it->is_gun()) {
  spare_mag = it->has_gunmod ("spare_mag");
  has_m203 = it->has_gunmod ("m203");
  has_shotgun = it->has_gunmod ("u_shotgun");
 }
 if (it->is_container() ||
     (it->charges == 0 &&
      (spare_mag == -1 || it->contents[spare_mag].charges <= 0) &&
      (has_m203 == -1 || it->contents[has_m203].charges <= 0) &&
      (has_shotgun == -1 || it->contents[has_shotgun].charges <= 0))) {
  if (it->contents.size() == 0) {
   return HINT_IFFY;
  }
 }

 return HINT_GOOD;
}

//TODO refactor stuff so we don't need to have this code mirroring game::disassemble
hint_rating player::rate_action_disassemble(item *it, game *g) {
 for (recipe_map::iterator cat_iter = g->recipes.begin(); cat_iter != g->recipes.end(); ++cat_iter)
    {
        for (recipe_list::iterator list_iter = cat_iter->second.begin();
             list_iter != cat_iter->second.end();
             ++list_iter)
        {
            recipe* cur_recipe = *list_iter;
            if (it->type == g->itypes[cur_recipe->result] && cur_recipe->reversible)
            // ok, a valid recipe exists for the item, and it is reversible
            // assign the activity
            {
                // check tools are available
                // loop over the tools and see what's required...again
                inventory crafting_inv = g->crafting_inventory();
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
                            int req = cur_recipe->tools[j][k].count;	// -1 => 1

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

                        if (!have_tool)
                        {
                           return HINT_IFFY;
                        }
                    }
                }
                // all tools present
                return HINT_GOOD;
            }
        }
    }
    // no recipe exists, or the item cannot be disassembled
    return HINT_CANT;
}

hint_rating player::rate_action_use(item *it)
{
 if (it->is_tool()) {
  it_tool *tool = dynamic_cast<it_tool*>(it->type);
  if (tool->charges_per_use != 0 && it->charges < tool->charges_per_use) {
   return HINT_IFFY;
  } else {
   return HINT_GOOD;
  }
 } else if (it->is_gunmod()) {
  if (skillLevel("gun") == 0) {
   return HINT_IFFY;
  } else {
   return HINT_GOOD;
  }
 } else if (it->is_bionic()) {
  return HINT_GOOD;
 } else if (it->is_food() || it->is_food_container() || it->is_book() || it->is_armor()) {
  return HINT_IFFY; //the rating is subjective, could be argued as HINT_CANT or HINT_GOOD as well
 }

 return HINT_CANT;
}

void player::use(game *g, char let)
{
 item* used = &i_at(let);
 item copy;
 bool replace_item = false;
 if (!inv.item_by_letter(let).is_null()) {
  copy = inv.remove_item_by_letter(let);
  copy.invlet = let;
  used = &copy;
  replace_item = true;
 }

 if (used->is_null()) {
  g->add_msg("You do not have that item.");
  return;
 }

 last_item = itype_id(used->type->id);

 if (used->is_tool()) {

  it_tool *tool = dynamic_cast<it_tool*>(used->type);
  if (tool->charges_per_use == 0 || used->charges >= tool->charges_per_use) {
   iuse use;
   (use.*tool->use)(g, this, used, false);
   used->charges -= tool->charges_per_use;
  } else
   g->add_msg("Your %s has %d charges but needs %d.", used->tname(g).c_str(),
              used->charges, tool->charges_per_use);

  if (tool->use == &iuse::dogfood) replace_item = false;

  if (replace_item && used->invlet != 0)
   inv.add_item_keep_invlet(copy);
  else if (used->invlet == 0 && used == &weapon)
   remove_weapon();
  return;

 } else if (used->is_gunmod()) {

   if (skillLevel("gun") == 0) {
   g->add_msg("You need to be at least level 1 in the firearms skill before you\
 can modify guns.");
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  char gunlet = g->inv("Select gun to modify:");
  it_gunmod *mod = static_cast<it_gunmod*>(used->type);
  item* gun = &(i_at(gunlet));
  if (gun->is_null()) {
   g->add_msg("You do not have that item.");
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (!gun->is_gun()) {
   g->add_msg("That %s is not a gun.", gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  it_gun* guntype = dynamic_cast<it_gun*>(gun->type);
  if (guntype->skill_used == Skill::skill("archery") || guntype->skill_used == Skill::skill("launcher")) {
   g->add_msg("You cannot mod your %s.", gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  if (guntype->skill_used == Skill::skill("pistol") && !mod->used_on_pistol) {
   g->add_msg("That %s cannot be attached to a handgun.",
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (guntype->skill_used == Skill::skill("shotgun") && !mod->used_on_shotgun) {
   g->add_msg("That %s cannot be attached to a shotgun.",
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (guntype->skill_used == Skill::skill("smg") && !mod->used_on_smg) {
   g->add_msg("That %s cannot be attached to a submachine gun.",
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (guntype->skill_used == Skill::skill("rifle") && !mod->used_on_rifle) {
   g->add_msg("That %s cannot be attached to a rifle.",
              used->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (mod->acceptible_ammo_types != 0 &&
             !(mfb(guntype->ammo) & mod->acceptible_ammo_types)) {
   g->add_msg("That %s cannot be used on a %s gun.", used->tname(g).c_str(),
              ammo_name(guntype->ammo).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  } else if (gun->contents.size() >= 4) {
   g->add_msg("Your %s already has 4 mods installed!  To remove the mods,\
press 'U' while wielding the unloaded gun.", gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  if ((mod->id == "clip" || mod->id == "clip2" || mod->id == "spare_mag") &&
      gun->clip_size() <= 2) {
   g->add_msg("You can not extend the ammo capacity of your %s.",
              gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  if (mod->id == "spare_mag" && gun->has_flag(IF_RELOAD_ONE)) {
   g->add_msg("You can not use a spare magazine with your %s.",
              gun->tname(g).c_str());
   if (replace_item)
    inv.add_item(copy);
   return;
  }
  for (int i = 0; i < gun->contents.size(); i++) {
   if (gun->contents[i].type->id == used->type->id) {
    g->add_msg("Your %s already has a %s.", gun->tname(g).c_str(),
               used->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   } else if (!(mod->item_flags & mfb(IF_MODE_AUX)) && mod->newtype != AT_NULL &&
	      !gun->contents[i].has_flag(IF_MODE_AUX) &&
	      (dynamic_cast<it_gunmod*>(gun->contents[i].type))->newtype != AT_NULL) {
    g->add_msg("Your %s's caliber has already been modified.",
               gun->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   } else if ((mod->id == "barrel_big" || mod->id == "barrel_small") &&
              (gun->contents[i].type->id == "barrel_big" ||
               gun->contents[i].type->id == "barrel_small")) {
    g->add_msg("Your %s already has a barrel replacement.",
               gun->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   } else if ((mod->id == "clip" || mod->id == "clip2") &&
              (gun->contents[i].type->id == "clip" ||
               gun->contents[i].type->id == "clip2")) {
    g->add_msg("Your %s already has its magazine size extended.",
               gun->tname(g).c_str());
    if (replace_item)
     inv.add_item(copy);
    return;
   }
  }
  g->add_msg("You attach the %s to your %s.", used->tname(g).c_str(),
             gun->tname(g).c_str());
  if (replace_item)
   gun->contents.push_back(copy);
  else
   gun->contents.push_back(i_rem(let));
  return;

 } else if (used->is_bionic()) {

  it_bionic* tmp = dynamic_cast<it_bionic*>(used->type);
  if (install_bionics(g, tmp)) {
   if (!replace_item)
    i_rem(let);
  } else if (replace_item)
   inv.add_item(copy);
  return;

 } else if (used->is_food() || used->is_food_container()) {
  if (replace_item)
   inv.add_item(copy);
  eat(g, lookup_item(let));
  return;
 } else if (used->is_book()) {
  if (replace_item)
   inv.add_item(copy);
  read(g, let);
  return;
 } else if (used->is_armor()) {
  if (replace_item)
   inv.add_item(copy);
  wear(g, let);
  return;
 } else
  g->add_msg("You can't do anything interesting with your %s.",
             used->tname(g).c_str());

 if (replace_item)
  inv.add_item(copy);
}

hint_rating player::rate_action_read(item *it, game *g)
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
 } else if (book->intel > 0 && has_trait(PF_ILLITERATE)) {
  return HINT_IFFY;
 } else if (has_trait(PF_HYPEROPIC) && !is_wearing("glasses_reading")) {
  return HINT_IFFY;
 }

 return HINT_GOOD;
}

void player::read(game *g, char ch)
{
 vehicle *veh = g->m.veh_at (posx, posy);
 if (veh && veh->player_in_control (this)) {
  g->add_msg("It's bad idea to read while driving.");
  return;
 }
// Check if reading is okay
 if (!can_see_fine_detail(g)) {
  g->add_msg("It's too dark to read!");
  return;
 }

 if (has_trait(PF_HYPEROPIC) && !is_wearing("glasses_reading")) {
  g->add_msg("Your eyes won't focus without reading glasses.");
  return;
 }

// Find the object
 int index = -1;
 item* it = NULL;
 if (weapon.invlet == ch) {
  index = -2;
  it = &weapon;
 } else {
  it = &inv.item_by_letter(ch);
 }

 if (it == NULL || it->is_null()) {
  g->add_msg("You do not have that item.");
  return;
 }

// Some macguffins can be read, but they aren't treated like books.
 it_macguffin* mac = NULL;
 if (it->is_macguffin()) {
  mac = dynamic_cast<it_macguffin*>(it->type);
 }
 if (mac != NULL) {
  iuse use;
  (use.*mac->use)(g, this, it, false);
  return;
 }

 if (!it->is_book()) {
  g->add_msg("Your %s is not good reading material.",
           it->tname(g).c_str());
  return;
 }
 it_book* tmp = dynamic_cast<it_book*>(it->type);
int time; //Declare this here so that we can change the time depending on whats needed
 if (tmp->intel > 0 && has_trait(PF_ILLITERATE)) {
  g->add_msg("You're illiterate!");
  return;
 }
 else if (skillLevel(tmp->type) < (int)tmp->req) {
  g->add_msg("The %s-related jargon flies over your head!",
             tmp->type->name().c_str());
  return;
 } else if (morale_level() < MIN_MORALE_READ &&  tmp->fun <= 0) {	// See morale.h
  g->add_msg("What's the point of reading?  (Your morale is too low!)");
  return;
 } else if (skillLevel(tmp->type) >= (int)tmp->level && tmp->fun <= 0 && !can_study_recipe(tmp) &&
            !query_yn("Your %s skill won't be improved.  Read anyway?",
                      tmp->type->name().c_str())) {
  return;
 }

 if (tmp->recipes.size() > 0)
 {
  if (can_study_recipe(tmp)) {
   g->add_msg("This book has more recipes for you to learn.");
  } else if (studied_all_recipes(tmp)) {
   g->add_msg("You know all the recipes this book has to offer.");
  } else {
   g->add_msg("This book has more recipes, but you don't have the skill to learn them yet.");
  }
 }

 if (tmp->intel > int_cur) {
  g->add_msg("This book is too complex for you to easily understand. It will take longer to read.");
  time = tmp->time * (read_speed() + ((tmp->intel - int_cur) * 100)); // Lower int characters can read, at a speed penalty
  activity = player_activity(ACT_READ, time, index, ch);
  moves = 0;
  return;
 }

// Base read_speed() is 1000 move points (1 minute per tmp->time)
 time = tmp->time * read_speed();
 activity = player_activity(ACT_READ, time, index, ch);
 moves = 0;
}

bool player::can_study_recipe(it_book* book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first) &&
            (iter->first->sk_primary == NULL || skillLevel(iter->first->sk_primary) >= iter->second))
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

void player::try_study_recipe(game *g, it_book *book)
{
    for (std::map<recipe*, int>::iterator iter = book->recipes.begin(); iter != book->recipes.end(); ++iter)
    {
        if (!knows_recipe(iter->first) &&
            (iter->first->sk_primary == NULL || skillLevel(iter->first->sk_primary) >= iter->second))
        {
            if (iter->first->sk_primary == NULL || rng(0, 4) <= skillLevel(iter->first->sk_primary) - iter->second)
            {
                learn_recipe(iter->first);
                g->add_msg("Learned a recipe for %s from the %s.",
                           g->itypes[iter->first->result]->name.c_str(), book->name.c_str());
            }
            else
            {
                g->add_msg("Failed to learn a recipe from the %s.", book->name.c_str());
            }
            // we only try to learn one recipe at a time
            return;
        }
    }
}

void player::try_to_sleep(game *g)
{
 int vpart = -1;
 vehicle *veh = g->m.veh_at (posx, posy, vpart);
 if (g->m.ter(posx, posy) == t_bed || g->m.ter(posx, posy) == t_makeshift_bed ||
     g->m.tr_at(posx, posy) == tr_cot || g->m.tr_at(posx, posy) == tr_rollmat ||
     (veh && veh->part_with_feature (vpart, vpf_seat) >= 0) ||
      (veh && veh->part_with_feature (vpart, vpf_bed) >= 0))
  g->add_msg("This is a comfortable place to sleep.");
 else if (g->m.ter(posx, posy) != t_floor)
  g->add_msg("It's %shard to get to sleep on this %s.",
             terlist[g->m.ter(posx, posy)].movecost <= 2 ? "a little " : "",
             terlist[g->m.ter(posx, posy)].name.c_str());
 add_disease(DI_LYING_DOWN, 300, g);
}

bool player::can_sleep(game *g)
{
 int sleepy = 0;
 if (has_addiction(ADD_SLEEP))
  sleepy -= 3;
 if (has_trait(PF_INSOMNIA))
  sleepy -= 8;

 int vpart = -1;
 vehicle *veh = g->m.veh_at (posx, posy, vpart);
 if ((veh && veh->part_with_feature (vpart, vpf_seat) >= 0) ||
     g->m.ter(posx, posy) == t_makeshift_bed || g->m.tr_at(posx, posy) == tr_cot)
  sleepy += 4;
 else if (g->m.tr_at(posx, posy) == tr_rollmat)
  sleepy += 3;
 else if (g->m.ter(posx, posy) == t_bed)
  sleepy += 5;
 else if (g->m.ter(posx, posy) == t_floor)
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

bool player::can_see_fine_detail(game *g)
{
    // flashlight is *hopefully* handled by the light level check below
    if (g->u.has_active_item("glowstick_lit") || g->u.has_active_item("lightstrip"))
    {
        return true;
    }
    if (LL_LIT > g->m.light_at(posx, posy))
    {
        return false;
    }
    return true;
}

int player::warmth(body_part bp)
{
 int ret = 0;
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp))
   ret += (dynamic_cast<it_armor*>(worn[i].type))->warmth;
 }
 return ret;
}

int player::encumb(body_part bp) {
 int iLayers = 0, iArmorEnc = 0, iWarmth = 0;
 return encumb(bp, iLayers, iArmorEnc, iWarmth);
}

int player::encumb(body_part bp, int &layers, int &armorenc, int &warmth)
{
    int ret = 0;
    it_armor* armor;
    for (int i = 0; i < worn.size(); i++)
    {
        if (!worn[i].is_armor())
            debugmsg("%s::encumb hit a non-armor item at worn[%d] (%s)", name.c_str(),
            i, worn[i].tname().c_str());
        armor = dynamic_cast<it_armor*>(worn[i].type);

        if (armor->covers & mfb(bp))
        {
            if (armor->is_power_armor() && (has_active_item("UPS_on") || has_active_bionic("bio_power_armor_interface")))
            {
                armorenc += armor->encumber - 4;
                warmth   += armor->warmth - 40;
            }
            else
            {
                armorenc += armor->encumber;
                warmth += armor->warmth;
                if (worn[i].has_flag(IF_FIT))
                {
                    armorenc--;
                }
            }
            if (armor->encumber >= 0 || bp != bp_torso)
            {
                layers++;
            }
        }
    }

    ret += armorenc;

    // Following items undo their layering. Once. Bodypart has to be taken into account, hence the switch.
    switch (bp)
    {
        case bp_feet  : if (is_wearing("socks") || is_wearing("socks_wool")) layers--; break;
        case bp_legs  : if (is_wearing("long_underpants")) layers--; break;
        case bp_hands : if (is_wearing("gloves_liner")) layers--; break;
        case bp_torso : if (is_wearing("under_armor")) layers--; break;
    }
    if (layers > 1)
    {
        ret += (layers - 1) * (bp == bp_torso ? .5 : 2);// Easier to layer on torso
    }
    if (volume_carried() > volume_capacity() - 2 && bp != bp_head)
    {
        ret += 3;
    }

    // Fix for negative hand encumbrance
    if ((bp == bp_hands) && (ret < 0))
     ret =0;

    // Bionics and mutation
    if (has_bionic("bio_stiff") && bp != bp_head && bp != bp_mouth)
    {
        ret += 1;
    }
    if (has_trait(PF_CHITIN3) && bp != bp_eyes && bp != bp_mouth)
    {
        ret += 1;
    }
    if (has_trait(PF_SLIT_NOSTRILS) && bp == bp_mouth)
    {
        ret += 1;
    }
    if (bp == bp_hands &&
     (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
     has_trait(PF_ARM_TENTACLES_8)))
    {
        ret += 3;
    }
    return ret;
}

int player::armor_bash(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += armor->dmg_resist;
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
 if (has_trait(PF_FUR))
  ret++;
 if (has_trait(PF_CHITIN))
  ret += 2;
 if (has_trait(PF_SHELL) && bp == bp_torso)
  ret += 6;
 ret += rng(0, disease_intensity(DI_ARMOR_BOOST));
 return ret;
}

int player::armor_cut(body_part bp)
{
 int ret = 0;
 it_armor* armor;
 for (int i = 0; i < worn.size(); i++) {
  armor = dynamic_cast<it_armor*>(worn[i].type);
  if (armor->covers & mfb(bp))
   ret += armor->cut_resist;
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
 if (has_trait(PF_THICKSKIN))
  ret++;
 if (has_trait(PF_SCALES))
  ret += 2;
 if (has_trait(PF_THICK_SCALES))
  ret += 4;
 if (has_trait(PF_SLEEK_SCALES))
  ret += 1;
 if (has_trait(PF_CHITIN))
  ret += 2;
 if (has_trait(PF_CHITIN2))
  ret += 4;
 if (has_trait(PF_CHITIN3))
  ret += 8;
 if (has_trait(PF_SHELL) && bp == bp_torso)
  ret += 14;
 ret += rng(0, disease_intensity(DI_ARMOR_BOOST));
 return ret;
}

void player::absorb(game *g, body_part bp, int &dam, int &cut)
{
 it_armor* tmp;
 int arm_bash = 0, arm_cut = 0;
 if (has_active_bionic("bio_ads")) {
  if (dam > 0 && power_level > 1) {
   dam -= rng(1, 8);
   power_level--;
  }
  if (cut > 0 && power_level > 1) {
   cut -= rng(0, 4);
   power_level--;
  }
  if (dam < 0)
   dam = 0;
  if (cut < 0)
   cut = 0;
 }
// See, we do it backwards, which assumes the player put on their jacket after
//  their T shirt, for example.  TODO: don't assume! ASS out of U & ME, etc.
 for (int i = worn.size() - 1; i >= 0; i--) {
  tmp = dynamic_cast<it_armor*>(worn[i].type);
  if ((tmp->covers & mfb(bp)) && tmp->storage <= 24) {
   arm_bash = tmp->dmg_resist;
   arm_cut  = tmp->cut_resist;
   switch (worn[i].damage) {
   case 1:
    arm_bash *= .8;
    arm_cut  *= .9;
    break;
   case 2:
    arm_bash *= .7;
    arm_cut  *= .7;
    break;
   case 3:
    arm_bash *= .5;
    arm_cut  *= .4;
    break;
   case 4:
    arm_bash *= .2;
    arm_cut  *= .1;
    break;
   }
   if (((it_armor *)worn[i].type)->is_power_armor()) {
     // Power armor can only be damaged by EXTREME damage
     if (cut > arm_cut * 2 || dam > arm_bash * 2) {
       if (!is_npc())
         g->add_msg("Your %s is damaged!", worn[i].tname(g).c_str());

       worn[i].damage++;
     }
   } else {
// Wool, leather, and cotton clothing may be damaged by CUTTING damage
   if ((worn[i].made_of(WOOL)   || worn[i].made_of(LEATHER) ||
        worn[i].made_of(COTTON) || worn[i].made_of(GLASS)   ||
        worn[i].made_of(WOOD)   || worn[i].made_of(KEVLAR)) &&
       rng(0, tmp->cut_resist * 2) < cut && !one_in(cut))
   {
    if (!is_npc())
    {
     g->add_msg("Your %s is cut!", worn[i].tname(g).c_str());
    }
    worn[i].damage++;
   }
// Kevlar, plastic, iron, steel, and silver may be damaged by BASHING damage
   if ((worn[i].made_of(PLASTIC) || worn[i].made_of(IRON)   ||
        worn[i].made_of(STEEL)   || worn[i].made_of(SILVER) ||
        worn[i].made_of(STONE))  &&
       rng(0, tmp->dmg_resist * 2) < dam && !one_in(dam))
   {
    if (!is_npc())
    {
     g->add_msg("Your %s is dented!", worn[i].tname(g).c_str());
    }
    worn[i].damage++;
   }
   }
   if (worn[i].damage >= 5) {
    if (!is_npc())
     g->add_msg("Your %s is completely destroyed!", worn[i].tname(g).c_str());
    else if (g->u_see(posx, posy))
     g->add_msg("%s's %s is destroyed!", name.c_str(),
                worn[i].tname(g).c_str());
    worn.erase(worn.begin() + i);
   }
  }
  dam -= arm_bash;
  cut -= arm_cut;
 }
 if (has_bionic("bio_carbon")) {
  dam -= 2;
  cut -= 4;
 }
 if (bp == bp_head && has_bionic("bio_armor_head")) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_arms && has_bionic("bio_armor_arms")) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_torso && has_bionic("bio_armor_torso")) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_legs && has_bionic("bio_armor_legs")) {
  dam -= 3;
  cut -= 3;
 }
 if (has_trait(PF_THICKSKIN))
  cut--;
 if (has_trait(PF_SCALES))
  cut -= 2;
 if (has_trait(PF_THICK_SCALES))
  cut -= 4;
 if (has_trait(PF_SLEEK_SCALES))
  cut -= 1;
 if (has_trait(PF_FEATHERS))
  dam--;
 if (has_trait(PF_FUR))
  dam--;
 if (has_trait(PF_CHITIN))
  cut -= 2;
 if (has_trait(PF_CHITIN2)) {
  dam--;
  cut -= 4;
 }
 if (has_trait(PF_CHITIN3)) {
  dam -= 2;
  cut -= 8;
 }
 if (has_trait(PF_PLANTSKIN))
  dam--;
 if (has_trait(PF_BARK))
  dam -= 2;
 if (bp == bp_feet && has_trait(PF_HOOVES))
  cut--;
 if (has_trait(PF_LIGHT_BONES))
  dam *= 1.4;
 if (has_trait(PF_HOLLOW_BONES))
  dam *= 1.8;
 if (dam < 0)
  dam = 0;
 if (cut < 0)
  cut = 0;
}

int player::resist(body_part bp)
{
 int ret = 0;
 for (int i = 0; i < worn.size(); i++) {
  if ((dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp) ||
      (bp == bp_eyes && // Head protection works on eyes too (e.g. baseball cap)
           (dynamic_cast<it_armor*>(worn[i].type))->covers & mfb(bp_head)))
   ret += (dynamic_cast<it_armor*>(worn[i].type))->env_resist;
 }
 if (bp == bp_mouth && has_bionic("bio_purifier") && ret < 5) {
  ret += 2;
  if (ret == 6)
   ret = 5;
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

    bool isSavant = has_trait(PF_SAVANT);

    Skill *savantSkill = NULL;
    SkillLevel savantSkillLevel = SkillLevel();

    if (isSavant)
    {
        for (std::vector<Skill*>::iterator aSkill = Skill::skills.begin()++;
             aSkill != Skill::skills.end(); ++aSkill)
        {
            if (skillLevel(*aSkill) > savantSkillLevel)
            {
                savantSkill = *aSkill;
                savantSkillLevel = skillLevel(*aSkill);
            }
        }
    }

    int newLevel;

    while (level.isTraining() && amount > 0 && xp_pool >= (1 + level))
    {
        amount -= level + 1;
        if ((!isSavant || s == savantSkill || one_in(2)) &&
            rng(0, 100) < level.comprehension(int_cur, has_trait(PF_FASTLEARNER)))
        {
            xp_pool -= (1 + level);
            skillLevel(s).train(newLevel);
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
        if( (rec->sk_primary == NULL ||
             skillLevel(rec->sk_primary) >= rec->difficulty) &&
            (rec->sk_secondary == NULL || skillLevel(rec->sk_secondary) > 0) )
        {
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

void player::assign_activity(game* g, activity_type type, int moves, int index, char invlet)
{
 if (backlog.type == type && backlog.index == index && backlog.invlet == invlet &&
     query_yn("Resume task?")) {
  activity = backlog;
  backlog = player_activity();
 } else
  activity = player_activity(type, moves, index, invlet);
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
  dump << weapon.tname().c_str() << " (" << weapon.charges;
  if( -1 != spare_mag )
   dump << "+" << weapon.contents[spare_mag].charges;
  for (int i = 0; i < weapon.contents.size(); i++)
   if (weapon.contents[i].is_gunmod() &&
       weapon.contents[i].has_flag(IF_MODE_AUX))
    dump << "+" << weapon.contents[i].charges;
  dump << ")";
  return dump.str();
 } else if (weapon.is_null())
  return "fists";

 else if (weapon.is_style()) { // Styles get bonus-bars!
  std::stringstream dump;
  dump << weapon.tname();

  if(weapon.typeId() == "style_capoeira"){
   if (has_disease(DI_DODGE_BOOST))
    dump << " +Dodge";
   if (has_disease(DI_ATTACK_BOOST))
    dump << " +Attack";
  } else if(weapon.typeId() == "style_ninjutsu"){
  } else if(weapon.typeId() == "style_leopard"){
   if (has_disease(DI_ATTACK_BOOST))
    dump << " +Attack";
  } else if(weapon.typeId() == "style_crane"){
   if (has_disease(DI_DODGE_BOOST))
    dump << " +Dodge";
  } else if(weapon.typeId() == "style_dragon"){
   if (has_disease(DI_DAMAGE_BOOST))
    dump << " +Damage";
  } else if(weapon.typeId() == "style_tiger"){
   dump << " [";
   int intensity = disease_intensity(DI_DAMAGE_BOOST);
   for (int i = 1; i <= 5; i++) {
    if (intensity >= i * 2)
     dump << "*";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_centipede"){
   dump << " [";
   int intensity = disease_intensity(DI_SPEED_BOOST);
   for (int i = 1; i <= 8; i++) {
    if (intensity >= i * 4)
     dump << "*";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_venom_snake"){
   dump << " [";
   int intensity = disease_intensity(DI_VIPER_COMBO);
   for (int i = 1; i <= 2; i++) {
    if (intensity >= i)
     dump << "C";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_lizard"){
   dump << " [";
   int intensity = disease_intensity(DI_ATTACK_BOOST);
   for (int i = 1; i <= 4; i++) {
    if (intensity >= i)
     dump << "*";
    else
     dump << ".";
   }
   dump << "]";
  } else if(weapon.typeId() == "style_toad"){
   dump << " [";
   int intensity = disease_intensity(DI_ARMOR_BOOST);
   for (int i = 1; i <= 5; i++) {
    if (intensity >= 5 + i)
     dump << "!";
    else if (intensity >= i)
     dump << "*";
    else
     dump << ".";
    }
    dump << "]";
  }

  return dump.str();
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

SkillLevel& player::skillLevel(size_t id) {
  return _skills[Skill::skill(id)];
}

SkillLevel& player::skillLevel(Skill *_skill) {
  return _skills[_skill];
}

void player::setID (int i)
{
    this->id = i;
}

int player::getID ()
{
    return this->id;
}
