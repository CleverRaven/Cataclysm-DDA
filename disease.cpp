#include "rng.h"
#include "game.h"
#include "bodypart.h"
#include "disease.h"
#include "weather.h"
#include <sstream>

void dis_msg(game *g, dis_type type)
{
 switch (type) {
 case DI_GLARE:
  g->add_msg("The sunlight's glare makes it hard to see.");
  break;
 case DI_WET:
  g->add_msg("You're getting soaked!");
  break;
 case DI_COMMON_COLD:
  g->add_msg("You feel a cold coming on...");
  break;
 case DI_FLU:
  g->add_msg("You feel a flu coming on...");
  break;
 case DI_ONFIRE:
  g->add_msg("You're on fire!");
  break;
 case DI_SMOKE:
  g->add_msg("You inhale a lungful of thick smoke.");
  break;
 case DI_TEARGAS:
  g->add_msg("You inhale a lungful of tear gas.");
  break;
 case DI_BOOMERED:
  g->add_msg("You're covered in bile!");
  break;
 case DI_SAP:
  g->add_msg("You're coated in sap!");
  break;
 case DI_SPORES:
  g->add_msg("You're covered in tiny spores!");
  break;
 case DI_SLIMED:
  g->add_msg("You're covered in thick goo!");
  break;
 case DI_LYING_DOWN:
  g->add_msg("You lie down to go to sleep...");
  break;
 case DI_FORMICATION:
  g->add_msg("There's bugs crawling under your skin!");
  break;
 case DI_WEBBED:
  g->add_msg("You're covered in webs!");
  break;
 case DI_DRUNK:
 case DI_HIGH:
  g->add_msg("You feel lightheaded.");
  break;
 case DI_ADRENALINE:
  g->add_msg("You feel a surge of adrenaline!");
  g->u.moves += 800;
  break;
 case DI_ASTHMA:
  g->add_msg("You can't breathe... asthma attack!");
  break;
 case DI_DEAF:
  g->add_msg("You're deafened!");
  break;
 case DI_BLIND:
  g->add_msg("You're blinded!");
  break;
 case DI_STUNNED:
  g->add_msg("You're stunned!");
  break;
 case DI_DOWNED:
  g->add_msg("You're knocked to the floor!");
  break;
 case DI_AMIGARA:
  g->add_msg("You can't look away from the fautline...");
  break;
 default:
  break;
 }
}

void dis_effect(game *g, player &p, disease &dis)
{
 int bonus;
 int junk;
 switch (dis.type) {
 case DI_GLARE:
  p.per_cur -= 1;
  break;

 case DI_WET:
  p.add_morale(MORALE_WET, -1, -50);
  break;

 case DI_COLD_HEAD:
  switch (dis.intensity) {
   case 3 : p.int_cur -= 3; if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("Your thoughts are unclear.");
   case 2 : p.int_cur--;
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Brr.\"");
  }
  break;

 case DI_COLD_TORSO:
  switch (dis.intensity) {
   case 3 : p.dex_cur -= 2; if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("Your torso is burning up. You should remove some layers.");
   // Speed -20
   case 2 :	p.dex_cur -= 1;
   // Speed -5
   case 1 : p.dex_cur -= 1; if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Brr.\"");
   // Speed -2
  }
  break;

 case DI_COLD_ARMS:
  switch (dis.intensity) {
   case 3 : p.dex_cur -= 2;
   case 2 :	p.dex_cur--;
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("Your arms are shivering.");
  }
  break;

 case DI_COLD_HANDS:
  switch (dis.intensity) {
   case 3 :	p.dex_cur -= 2;
   case 2 :	p.dex_cur -= 2;
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Brr.\"");
  }
  break;

 case DI_COLD_LEGS:
  switch (dis.intensity) {
   case 3 :	p.str_cur--; if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("Your legs are seizing from the incredible cold.");
   // Speed -20
   case 2 :	p.str_cur--;
   // Speed -5
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("Your legs feel sluggish from the cold.");
   // Speed -2
  }
  break;

 case DI_COLD_FEET:
  switch (dis.intensity) {
   case 3 : p.str_cur--;
   case 2 : p.str_cur--;
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Brr.\"");
  }
  break;

 case DI_FROSTBITE_HANDS:
  switch (dis.intensity) {
   case 2 : p.dex_cur -= 3;
   case 1 :
    if (p.temp_cur[bp_mouth] > BODYTEMP_COLD && p.pain < 40) p.pain++;
    if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("Your hands feel numb.");
  }
 break;

 case DI_FROSTBITE_FEET:
  switch (dis.intensity) {
   case 2 : // -4 speed
   case 1 :
    if (p.temp_cur[bp_mouth] > BODYTEMP_COLD && p.pain < 40) p.pain++;
	if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("Your feet feel numb.");
  }
 break;

 case DI_FROSTBITE_MOUTH:
  switch (dis.intensity) {
   case 2 : p.per_cur -= 3;
   case 1 :
    if (p.temp_cur[bp_mouth] > BODYTEMP_COLD && p.pain < 40) p.pain++;
	if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("Your face feels numb.");
  }
 break;

 case DI_BLISTERS_HANDS:
  p.dex_cur--;
  if (p.pain < 35) p.pain++;
  if (one_in(2)) p.hp_max[hp_arm_r]--;
  else p.hp_max[hp_arm_l]--;
  break;

 case DI_BLISTERS_FEET:
  p.str_cur--;
  if (p.pain < 35) p.pain++;
  if (one_in(2)) p.hp_max[hp_leg_r]--;
  else p.hp_max[hp_leg_l]--;
  break;

 case DI_BLISTERS_MOUTH:
  p.per_cur--;
  p.hp_max[hp_head]--;
  if (p.pain < 35) p.pain++;
  break;

 case DI_HOT_HEAD:
  switch (dis.intensity) {
   case 3 :
    p.thirst--;
	if (p.pain < 50) p.pain++;
    if (!p.has_disease(DI_SLEEP) && one_in(400)) g->add_msg("Your head is pounding from the heat.");
    // Speed -20
   case 2 :
    p.thirst--;
    if (one_in(15000 - p.temp_cur[bp_head])) p.vomit(g); // Hallucinations handled in game.cpp
	if (p.pain < 20) p.pain++;
    if (!p.has_disease(DI_SLEEP) && one_in(400)) g->add_msg("The heat is making you see things.");
    // Speed -5
   case 1 : 
    if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Phew, it's hot warm.\"");
    // Speed -2
  }
  break;

 case DI_HOT_TORSO:
  switch (dis.intensity) {
   case 3 :
    p.thirst--;
    p.str_cur--;
    if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("You are sweating profusely.");
    // Speed -20
   case 2 :
    p.thirst--;
	p.str_cur--;
    // Speed -5
   case 1 : 
    if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Phew, it's hot warm.\"");
    // Speed -2
  }
  break;

 case DI_HOT_ARMS:
  switch (dis.intensity) {
   case 3 : p.thirst--;
    if (p.pain < 50) p.pain++;
   case 2 : p.thirst--;
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Phew, it's hot warm.\"");
  }
  break;

 case DI_HOT_HANDS:
  switch (dis.intensity) {
   case 3 : p.dex_cur--;
   case 2 : p.dex_cur--;
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Phew, it's hot warm.\"");
  }
  break;

 case DI_HOT_LEGS:
  switch (dis.intensity) {
   case 3 : p.thirst--;
    if (p.pain < 50) p.pain++;
    if (one_in(200)) g->add_msg("Your legs are cramping up.");
   case 2 : p.thirst--;
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Phew, it's hot warm.\"");
  }
  break;

 case DI_HOT_FEET:
  switch (dis.intensity) {
   case 3 : if (p.pain < 50) p.pain++;
   case 2 : 
    if (p.pain < 30) p.pain++;
    if (!p.has_disease(DI_SLEEP) && one_in(200)) g->add_msg("Your feet are swelling in the heat.");
   case 1 : if (!p.has_disease(DI_SLEEP) && one_in(600)) g->add_msg("\"Phew, it's hot warm.\"");
  }
  break;

 case DI_COMMON_COLD:
  if (int(g->turn) % 300 == 0)
   p.thirst++;
  if (p.has_disease(DI_TOOK_FLUMED)) {
   p.str_cur--;
   p.int_cur--;
  } else {
   p.str_cur -= 3;
   p.dex_cur--;
   p.int_cur -= 2;
   p.per_cur--;
  }
  if (one_in(300)) {
   p.moves -= 80;
   if (!p.is_npc()) {
    g->add_msg("You cough noisily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "loud coughing");
  }
  break;

 case DI_FLU:
  if (int(g->turn) % 300 == 0)
   p.thirst++;
  if (p.has_disease(DI_TOOK_FLUMED)) {
   p.str_cur -= 2;
   p.int_cur--;
  } else {
   p.str_cur -= 4;
   p.dex_cur -= 2;
   p.int_cur -= 2;
   p.per_cur--;
  }
  if (one_in(300)) {
   p.moves -= 80;
   if (!p.is_npc()) {
    g->add_msg("You cough noisily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "loud coughing");
  }
  if (one_in(3600) || (p.has_trait(PF_WEAKSTOMACH) && one_in(3000)) ||
      (p.has_trait(PF_NAUSEA) && one_in(2400))) {
   if (!p.has_disease(DI_TOOK_FLUMED) || one_in(2))
    p.vomit(g);
  }
  break;

 case DI_SMOKE:
  p.str_cur--;
  p.dex_cur--;
  if (one_in(5)) {
   if (!p.is_npc()) {
    g->add_msg("You cough heavily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "a hacking cough.");
   p.moves -= 80;
   p.hurt(g, bp_torso, 0, 1 - (rng(0, 1) * rng(0, 1)));
  }
  break;

 case DI_TEARGAS:
  p.str_cur -= 2;
  p.dex_cur -= 2;
  p.int_cur -= 1;
  p.per_cur -= 4;
  if (one_in(3)) {
   if (!p.is_npc()) {
    g->add_msg("You cough heavily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "a hacking cough");
   p.moves -= 100;
   p.hurt(g, bp_torso, 0, rng(0, 3) * rng(0, 1));
  }
  break;

 case DI_ONFIRE:
  p.hurtall(3);
  for (int i = 0; i < p.worn.size(); i++) {
   if (p.worn[i].made_of(VEGGY) || p.worn[i].made_of(PAPER)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   } else if ((p.worn[i].made_of(COTTON) || p.worn[i].made_of(WOOL)) &&
              one_in(10)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   } else if (p.worn[i].made_of(PLASTIC) && one_in(50)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   }
  }
  break;

 case DI_BOOMERED:
  p.per_cur -= 5;
  break;

 case DI_SAP:
  p.dex_cur -= 3;
  break;

 case DI_SPORES:
  if (one_in(30))
   p.add_disease(DI_FUNGUS, -1, g);
  break;

 case DI_FUNGUS:
  bonus = 0;
  if (p.has_trait(PF_POISRESIST))
   bonus = 100;
  p.moves -= 10;
  p.str_cur -= 1;
  p.dex_cur -= 1;
  if (dis.duration > -600) {	// First hour symptoms
   if (one_in(160 + bonus)) {
    if (!p.is_npc()) {
     g->add_msg("You cough heavily.");
     g->sound(p.posx, p.posy, 12, "");
    } else
     g->sound(p.posx, p.posy, 12, "a hacking cough");
    p.pain++;
   }
   if (one_in(100 + bonus)) {
    if (!p.is_npc())
     g->add_msg("You feel nauseous.");
   }
   if (one_in(100 + bonus)) {
    if (!p.is_npc())
     g->add_msg("You smell and taste mushrooms.");
   }
  } else if (dis.duration > -3600) {	// One to six hours
   if (one_in(600 + bonus * 3)) {
    if (!p.is_npc())
     g->add_msg("You spasm suddenly!");
    p.moves -= 100;
    p.hurt(g, bp_torso, 0, 5);
   }
   if ((p.has_trait(PF_WEAKSTOMACH) && one_in(1600 + bonus *  8)) ||
       (p.has_trait(PF_NAUSEA) && one_in(800 + bonus * 6)) ||
       one_in(2000 + bonus * 10)) {
    if (!p.is_npc())
     g->add_msg("You vomit a thick, gray goop.");
    else if (g->u_see(p.posx, p.posy, junk))
     g->add_msg("%s vomits a thick, gray goop.", p.name.c_str());
    p.moves = -200;
    p.hunger += 50;
    p.thirst += 68;
   }
  } else {	// Full symptoms
   if (one_in(1000 + bonus * 8)) {
    if (!p.is_npc())
     g->add_msg("You double over, spewing live spores from your mouth!");
    else if (g->u_see(p.posx, p.posy, junk))
     g->add_msg("%s coughs up a stream of live spores!", p.name.c_str());
    p.moves = -500;
    int sporex, sporey;
    monster spore(g->mtypes[mon_spore]);
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++) {
      sporex = p.posx + i;
      sporey = p.posy + j;
      if (g->m.move_cost(sporex, sporey) > 0 && one_in(5)) {
       if (g->mon_at(sporex, sporey) >= 0) {	// Spores hit a monster
        if (g->u_see(sporex, sporey, junk))
         g->add_msg("The %s is covered in tiny spores!",
                    g->z[g->mon_at(sporex, sporey)].name().c_str());
        if (!g->z[g->mon_at(sporex, sporey)].make_fungus(g))
         g->kill_mon(g->mon_at(sporex, sporey));
       } else {
        spore.spawn(sporex, sporey);
        g->z.push_back(spore);
       }
      }
     }
    }
   } else if (one_in(6000 + bonus * 20)) {
    if (!p.is_npc())
     g->add_msg("Fungus stalks burst through your hands!");
    else if (g->u_see(p.posx, p.posy, junk))
     g->add_msg("Fungus stalks burst through %s's hands!", p.name.c_str());
    p.hurt(g, bp_arms, 0, 60);
    p.hurt(g, bp_arms, 1, 60);
   }
  }
  break;

 case DI_SLIMED:
  p.dex_cur -= 2;
  break;

 case DI_LYING_DOWN:
  p.moves = 0;
  if (p.can_sleep(g)) {
   dis.duration = 1;
   if (!p.is_npc())
    g->add_msg("You fall asleep.");
   p.add_disease(DI_SLEEP, 6000, g);
  }
  if (dis.duration == 1 && !p.has_disease(DI_SLEEP))
   if (!p.is_npc())
    g->add_msg("You try to sleep, but can't...");
  break;

 case DI_SLEEP:
  p.moves = 0;
  if (int(g->turn) % 25 == 0) {
   if (p.fatigue > 0)
    p.fatigue -= 1 + rng(0, 1) * rng(0, 1);
   if (p.has_trait(PF_FASTHEALER))
    p.healall(rng(0, 1));
   else
    p.healall(rng(0, 1) * rng(0, 1) * rng(0, 1));
   if (p.fatigue <= 0 && p.fatigue > -20) {
    p.fatigue = -25;
    g->add_msg("Fully rested.");
    dis.duration = dice(3, 100);
   }
  }
  if (int(g->turn) % 100 == 0 && !p.has_bionic(bio_recycler)) {
// Hunger and thirst advance more slowly while we sleep.
   p.hunger--;
   p.thirst--;
  }
  if (rng(5, 80) + rng(0, 120) + rng(0, abs(p.fatigue)) +
      rng(0, abs(p.fatigue * 5)) < g->light_level() &&
      (p.fatigue < 10 || one_in(p.fatigue / 2))) {
   g->add_msg("The light wakes you up.");
   dis.duration = 1;
  }
  // Cold may wake you up.
  // Player gets desperate for sleep
  if (p.temp_cur[bp_torso] < BODYTEMP_VERY_COLD - p.fatigue/2 ||
  (one_in(p.temp_cur[bp_torso]) && p.temp_cur[bp_torso] < BODYTEMP_NORM - p.fatigue/2)){
   g->add_msg("The cold wakes you up.");
   dis.duration = 1;
  }

  break;

 case DI_PKILL1:
  if (dis.duration <= 70 && dis.duration % 7 == 0 && p.pkill < 15)
   p.pkill++;
  break;

 case DI_PKILL2:
  if (dis.duration % 7 == 0 &&
      (one_in(p.addiction_level(ADD_PKILLER)) ||
       one_in(p.addiction_level(ADD_PKILLER))   ))
   p.pkill += 2;
  break;

 case DI_PKILL3:
  if (dis.duration % 2 == 0 &&
      (one_in(p.addiction_level(ADD_PKILLER)) ||
       one_in(p.addiction_level(ADD_PKILLER))   ))
   p.pkill++;
  break;

 case DI_PKILL_L:
  if (dis.duration % 20 == 0 && p.pkill < 40 &&
      (one_in(p.addiction_level(ADD_PKILLER)) ||
       one_in(p.addiction_level(ADD_PKILLER))   ))
   p.pkill++;
  break;

 case DI_IODINE:
  if (p.radiation > 0 && one_in(16))
   p.radiation--;
  break;

 case DI_TOOK_XANAX:
  if (dis.duration % 25 == 0 && (p.stim > 0 || one_in(2)))
   p.stim--;
  break;

 case DI_DRUNK:
// We get 600 turns, or one hour, of DI_DRUNK for each drink we have (on avg)
// So, the duration of DI_DRUNK is a good indicator of how much alcohol is in
//  our system.
  p.per_cur -= int(dis.duration / 1000);
  p.dex_cur -= int(dis.duration / 1000);
  p.int_cur -= int(dis.duration /  700);
  p.str_cur -= int(dis.duration / 1500);
  if (dis.duration <= 600)
   p.str_cur += 1;
  if (dis.duration > 2000 + 100 * dice(2, 100) &&
      (p.has_trait(PF_WEAKSTOMACH) || p.has_trait(PF_NAUSEA) || one_in(20)))
   p.vomit(g);
  if (!p.has_disease(DI_SLEEP) && dis.duration >= 4500 &&
      one_in(500 - int(dis.duration / 80))) {
   if (!p.is_npc())
    g->add_msg("You pass out.");
   p.add_disease(DI_SLEEP, dis.duration / 2, g);
  }
  break;

 case DI_CIG:
  if (dis.duration >= 600) {	// Smoked too much
   p.str_cur--;
   p.dex_cur--;
   if (dis.duration >= 1200 && (one_in(50) ||
                                (p.has_trait(PF_WEAKSTOMACH) && one_in(30)) ||
                                (p.has_trait(PF_NAUSEA) && one_in(20))))
    p.vomit(g);
  } else {
   p.dex_cur++;
   p.int_cur++;
   p.per_cur++;
  }
  break;

 case DI_HIGH:
  p.int_cur--;
  p.per_cur--;
  break;

 case DI_POISON:
  if ((!p.has_trait(PF_POISRESIST) && one_in(150)) ||
      ( p.has_trait(PF_POISRESIST) && one_in(900))   ) {
   if (!p.is_npc())
    g->add_msg("You're suddenly wracked with pain!");
   p.pain++;
   p.hurt(g, bp_torso, 0, rng(0, 2) * rng(0, 1));
  }
  p.per_cur--;
  p.dex_cur--;
  if (!p.has_trait(PF_POISRESIST))
   p.str_cur -= 2;
  break;

 case DI_BLEED:
  if (!p.is_npc() && one_in(2)) {
   g->add_msg("You lose some blood.");
   p.pain++;
   p.hurt(g, bp_torso, 0, 1);
   p.per_cur--;
   p.str_cur --;
   g->m.add_field(g, p.posx, p.posy, fd_blood, 1);
  }
  break;

 case DI_BADPOISON:
  if ((!p.has_trait(PF_POISRESIST) && one_in(100)) ||
      ( p.has_trait(PF_POISRESIST) && one_in(500))   ) {
   if (!p.is_npc())
    g->add_msg("You're suddenly wracked with pain!");
   p.pain += 2;
   p.hurt(g, bp_torso, 0, rng(0, 2));
  }
  p.per_cur -= 2;
  p.dex_cur -= 2;
  if (!p.has_trait(PF_POISRESIST))
   p.str_cur -= 3;
  else
   p.str_cur--;
  break;

 case DI_FOODPOISON:
  bonus = 0;
  if (p.has_trait(PF_POISRESIST))
   bonus = 600;
  if (one_in(300 + bonus)) {
   if (!p.is_npc())
    g->add_msg("You're suddenly wracked with pain and nausea!");
   p.hurt(g, bp_torso, 0, 1);
  }
  if ((p.has_trait(PF_WEAKSTOMACH) && one_in(300 + bonus)) ||
      (p.has_trait(PF_NAUSEA) && one_in(50 + bonus)) ||
      one_in(600 + bonus))
   p.vomit(g);
  p.str_cur -= 3;
  p.dex_cur--;
  p.per_cur--;
  if (p.has_trait(PF_POISRESIST))
   p.str_cur += 2;
  break;

 case DI_SHAKES:
  p.dex_cur -= 4;
  p.str_cur--;
  break;

 case DI_DERMATIK: {
  int formication_chance = 600;
  if (dis.duration > -2400 && dis.duration < 0)
   formication_chance = 2400 + dis.duration;
  if (one_in(formication_chance))
   p.add_disease(DI_FORMICATION, 1200, g);

  if (dis.duration < -2400 && one_in(2400))
   p.vomit(g);

  if (dis.duration < -14400) { // Spawn some larvae!
// Choose how many insects; more for large characters
   int num_insects = 1;
   while (num_insects < 6 && rng(0, 10) < p.str_max)
    num_insects++;
// Figure out where they may be placed
   std::vector<point> valid_spawns;
   for (int x = p.posx - 1; x <= p.posy + 1; x++) {
    for (int y = p.posy - 1; y <= p.posy + 1; y++) {
     if (g->is_empty(x, y))
      valid_spawns.push_back(point(x, y));
    }
   }
   if (valid_spawns.size() >= 1) {
    int t;
    p.rem_disease(DI_DERMATIK); // No more infection!  yay.
    if (!p.is_npc())
     g->add_msg("Insects erupt from your skin!");
    else if (g->u_see(p.posx, p.posy, t))
     g->add_msg("Insects erupt from %s's skin!", p.name.c_str());
    p.moves -= 600;
    monster grub(g->mtypes[mon_dermatik_larva]);
    while (valid_spawns.size() > 0 && num_insects > 0) {
     num_insects--;
// Hurt the player
     body_part burst = bp_torso;
     if (one_in(3))
      burst = bp_arms;
     else if (one_in(3))
      burst = bp_legs;
     p.hurt(g, burst, rng(0, 1), rng(4, 8));
// Spawn a larva
     int sel = rng(0, valid_spawns.size() - 1);
     grub.spawn(valid_spawns[sel].x, valid_spawns[sel].y);
     valid_spawns.erase(valid_spawns.begin() + sel);
// Sometimes it's a friendly larva!
     if (one_in(3))
      grub.friendly = -1;
     else
      grub.friendly =  0;
     g->z.push_back(grub);
    }
   }
  }
 } break;

 case DI_WEBBED:
  p.str_cur -= 2;
  p.dex_cur -= 4;
  break;

 case DI_RAT:
  p.int_cur -= int(dis.duration / 20);
  p.str_cur -= int(dis.duration / 50);
  p.per_cur -= int(dis.duration / 25);
  if (rng(30, 100) < rng(0, dis.duration) && one_in(3))
   p.vomit(g);
  if (rng(0, 100) < rng(0, dis.duration))
   p.mutation_category_level[MUTCAT_RAT]++;
  if (rng(50, 500) < rng(0, dis.duration))
   p.mutate(g);
  break;

 case DI_FORMICATION:
  p.int_cur -= 2;
  p.str_cur -= 1;
  if (one_in(10 + 40 * p.int_cur)) {
   int t;
   if (!p.is_npc()) {
    g->add_msg("You start scratching yourself all over!");
    g->cancel_activity();
   } else if (g->u_see(p.posx, p.posy, t))
    g->add_msg("%s starts scratching %s all over!", p.name.c_str(),
               (p.male ? "himself" : "herself"));
   p.moves -= 150;
   p.hurt(g, bp_torso, 0, 1);
  }
  break;

 case DI_HALLU:
// This assumes that we were given DI_HALLU with a 3600 (6-hour) lifespan
  if (dis.duration > 3000) {	// First hour symptoms
   if (one_in(300)) {
    if (!p.is_npc())
     g->add_msg("You feel a little strange.");
   }
  } else if (dis.duration > 2400) {	// Coming up
   if (one_in(100) || (p.has_trait(PF_WEAKSTOMACH) && one_in(100))) {
    if (!p.is_npc())
     g->add_msg("You feel nauseous.");
    p.hunger -= 5;
   }
   if (!p.is_npc()) {
    if (one_in(200))
     g->add_msg("Huh?  What was that?");
    else if (one_in(200))
     g->add_msg("Oh god, what's happening?");
    else if (one_in(200))
     g->add_msg("Of course... it's all fractals!");
   }
  } else if (dis.duration == 2400)	// Visuals start
   p.add_disease(DI_VISUALS, 2400, g);
  else {	// Full symptoms
   p.per_cur -= 2;
   p.int_cur -= 1;
   p.dex_cur -= 2;
   p.str_cur -= 1;
   if (one_in(50)) {	// Generate phantasm
    monster phantasm(g->mtypes[mon_hallu_zom + rng(0, 3)]);
    phantasm.spawn(p.posx + rng(-10, 10), p.posy + rng(-10, 10));
    g->z.push_back(phantasm);
   }
  }
  break;

 case DI_ADRENALINE:
  if (dis.duration > 150) {	// 5 minutes positive effects
   p.str_cur += 5;
   p.dex_cur += 3;
   p.int_cur -= 8;
   p.per_cur += 1;
  } else if (dis.duration == 150) {	// 15 minutes come-down
   if (!p.is_npc())
    g->add_msg("Your adrenaline rush wears off.  You feel AWFUL!");
   p.moves -= 300;
  } else {
   p.str_cur -= 2;
   p.dex_cur -= 1;
   p.int_cur -= 1;
   p.per_cur -= 1;
  }
  break;

 case DI_ASTHMA:
  if (dis.duration > 1200) {
   if (!p.is_npc())
    g->add_msg("Your asthma overcomes you.  You stop breathing and die...");
   p.hurtall(500);
  }
  p.str_cur -= 2;
  p.dex_cur -= 3;
  break;

 case DI_GRACK:
   p.str_cur += 500;
   p.dex_cur += 500;
   p.int_cur += 500;
   p.per_cur += 500;
  break;

 case DI_METH:
  if (dis.duration > 600) {
   p.str_cur += 2;
   p.dex_cur += 2;
   p.int_cur += 3;
   p.per_cur += 3;
  } else {
   p.str_cur -= 3;
   p.dex_cur -= 2;
   p.int_cur -= 1;
  }
  break;

 case DI_ATTACK_BOOST:
 case DI_DAMAGE_BOOST:
 case DI_DODGE_BOOST:
 case DI_ARMOR_BOOST:
 case DI_SPEED_BOOST:
  if (dis.intensity > 1)
   dis.intensity--;
  break;

 case DI_TELEGLOW:
// Default we get around 300 duration points per teleport (possibly more
// depending on the source).
// TODO: Include a chance to teleport to the nether realm.
  if (dis.duration > 6000) {	// 20 teles (no decay; in practice at least 21)
   if (one_in(1000 - ((dis.duration - 6000) / 10))) {
    if (!p.is_npc())
     g->add_msg("Glowing lights surround you, and you teleport.");
    g->teleport();
    if (one_in(10))
     p.rem_disease(DI_TELEGLOW);
   }
   if (one_in(1200 - ((dis.duration - 6000) / 5)) && one_in(20)) {
    if (!p.is_npc())
     g->add_msg("You pass out.");
    p.add_disease(DI_SLEEP, 1200, g);
    if (one_in(6))
     p.rem_disease(DI_TELEGLOW);
   }
  }
  if (dis.duration > 3600) { // 12 teles
   if (one_in(4000 - int(.25 * (dis.duration - 3600)))) {
    int range = g->moncats[mcat_nether].size();
    mon_id type = (g->moncats[mcat_nether])[rng(0, range - 1)];
    monster beast(g->mtypes[type]);
    int x, y, tries = 0;
    do {
     x = p.posx + rng(-4, 4);
     y = p.posy + rng(-4, 4);
     tries++;
    } while (((x == p.posx && y == p.posy) || g->mon_at(x, y) != -1) &&
             tries < 10);
    if (tries < 10) {
     if (g->m.move_cost(x, y) == 0)
      g->m.ter(x, y) = t_rubble;
     beast.spawn(x, y);
     g->z.push_back(beast);
     if (g->u_see(x, y, junk)) {
      g->cancel_activity_query("A monster appears nearby!");
      g->add_msg("A portal opens nearby, and a monster crawls through!");
     }
     if (one_in(2))
      p.rem_disease(DI_TELEGLOW);
    }
   }
   if (one_in(3500 - int(.25 * (dis.duration - 3600)))) {
    if (!p.is_npc())
     g->add_msg("You shudder suddenly.");
    p.mutate(g);
    if (one_in(4))
     p.rem_disease(DI_TELEGLOW);
   }
  }
  if (dis.duration > 2400) {	// 8 teleports
   if (one_in(10000 - dis.duration))
    p.add_disease(DI_SHAKES, rng(40, 80), g);
   if (one_in(12000 - dis.duration)) {
    if (!p.is_npc())
     g->add_msg("Your vision is filled with bright lights...");
    p.add_disease(DI_BLIND, rng(10, 20), g);
    if (one_in(8))
     p.rem_disease(DI_TELEGLOW);
   }
   if (one_in(5000) && !p.has_disease(DI_HALLU)) {
    p.add_disease(DI_HALLU, 3600, g);
    if (one_in(5))
     p.rem_disease(DI_TELEGLOW);
   }
  }
  if (one_in(4000)) {
   if (!p.is_npc())
    g->add_msg("You're suddenly covered in ectoplasm.");
   p.add_disease(DI_BOOMERED, 100, g);
   if (one_in(4))
    p.rem_disease(DI_TELEGLOW);
  }
  if (one_in(10000)) {
   p.add_disease(DI_FUNGUS, -1, g);
   p.rem_disease(DI_TELEGLOW);
  }
  break;

 case DI_ATTENTION:
  if (one_in( 100000 / dis.duration ) && one_in( 100000 / dis.duration ) &&
      one_in(250)) {
   int range = g->moncats[mcat_nether].size();
   mon_id type = (g->moncats[mcat_nether])[rng(0, range - 1)];
   monster beast(g->mtypes[type]);
   int x, y, tries = 0, junk;
   do {
    x = p.posx + rng(-4, 4);
    y = p.posy + rng(-4, 4);
    tries++;
   } while (((x == p.posx && y == p.posy) || g->mon_at(x, y) != -1) &&
            tries < 10);
   if (tries < 10) {
    if (g->m.move_cost(x, y) == 0)
     g->m.ter(x, y) = t_rubble;
    beast.spawn(x, y);
    g->z.push_back(beast);
    if (g->u_see(x, y, junk)) {
     g->cancel_activity_query("A monster appears nearby!");
     g->add_msg("A portal opens nearby, and a monster crawls through!");
    }
    dis.duration /= 4;
   }
  }
  break;

 case DI_EVIL: {
  bool lesser = false; // Worn or wielded; diminished effects
  if (p.weapon.is_artifact() && p.weapon.is_tool()) {
   it_artifact_tool *tool = dynamic_cast<it_artifact_tool*>(p.weapon.type);
   for (int i = 0; i < tool->effects_carried.size(); i++) {
    if (tool->effects_carried[i] == AEP_EVIL)
     lesser = true;
   }
   for (int i = 0; i < tool->effects_wielded.size(); i++) {
    if (tool->effects_wielded[i] == AEP_EVIL)
     lesser = true;
   }
  }
  for (int i = 0; !lesser && i < p.worn.size(); i++) {
   if (p.worn[i].is_artifact()) {
    it_artifact_armor *armor = dynamic_cast<it_artifact_armor*>(p.worn[i].type);
    for (int i = 0; i < armor->effects_worn.size(); i++) {
     if (armor->effects_worn[i] == AEP_EVIL)
      lesser = true;
    }
   }
  }

  if (lesser) { // Only minor effects, some even good!
   p.str_cur += (dis.duration > 4500 ? 10 : int(dis.duration / 450));
   if (dis.duration < 600)
    p.dex_cur++;
   else
    p.dex_cur -= (dis.duration > 3600 ? 10 : int((dis.duration - 600) / 300));
   p.int_cur -= (dis.duration > 3000 ? 10 : int((dis.duration - 500) / 250));
   p.per_cur -= (dis.duration > 4800 ? 10 : int((dis.duration - 800) / 400));
  } else { // Major effects, all bad.
   p.str_cur -= (dis.duration > 5000 ? 10 : int(dis.duration / 500));
   p.dex_cur -= (dis.duration > 6000 ? 10 : int(dis.duration / 600));
   p.int_cur -= (dis.duration > 4500 ? 10 : int(dis.duration / 450));
   p.per_cur -= (dis.duration > 4000 ? 10 : int(dis.duration / 400));
  }
 } break;
 }
}

int disease_speed_boost(disease dis)
{
 switch (dis.type) {
 case DI_COLD_TORSO:
  switch (dis.intensity) {
   case 1 : return  -2;
   case 2 : return  -5;
   case 3 : return -20;}
 case DI_COLD_LEGS:
  switch (dis.intensity) {
   case 1 : return  -2;
   case 2 : return  -5;
   case 3 : return -20;}
 case DI_FROSTBITE_FEET:
  switch (dis.intensity) {
   case 2 : return -4;}
 case DI_HOT_HEAD:
  switch (dis.intensity) {
   case 1 : return  -2;
   case 2 : return  -5;
   case 3 : return -20;}
 case DI_HOT_TORSO:
  switch (dis.intensity) {
   case 1 : return  -2;
   case 2 : return  -5;
   case 3 : return -20;}
 case DI_INFECTION:	return -80;
 case DI_SAP:		return -25;
 case DI_SPORES:	return -15;
 case DI_SLIMED:	return -25;
 case DI_BADPOISON:	return -10;
 case DI_FOODPOISON:	return -20;
 case DI_WEBBED:	return -25;
 case DI_ADRENALINE:	return (dis.duration > 150 ? 40 : -10);
 case DI_ASTHMA:	return 0 - int(dis.duration / 5);
 case DI_GRACK:         return +20000;
 case DI_METH:		return (dis.duration > 600 ? 50 : -40);
 default:		return 0;
 }
}

std::string dis_name(disease dis)
{
 switch (dis.type) {
 case DI_NULL:		return "";
 case DI_GLARE:		return "Glare";
 case DI_COLD_HEAD:		
  switch (dis.intensity) {
   case 1: return "Cold head";
   case 2: return "Cold head!";
   case 3: return "Freezing head!!";}	
 case DI_COLD_TORSO:		
  switch (dis.intensity) {
   case 1: return "Cold torso";
   case 2: return "Cold torso!";
   case 3: return "Freezing torso!!";}	
 case DI_COLD_ARMS:		
  switch (dis.intensity) {
   case 1: return "Cold arms";
   case 2: return "Cold arms!";
   case 3: return "Freezing arms!!";}	
 case DI_COLD_HANDS:		
  switch (dis.intensity) {
   case 1: return "Cold hands";
   case 2: return "Cold hands!";
   case 3: return "Freezing hands!!";}	
 case DI_COLD_LEGS:		
  switch (dis.intensity) {
   case 1: return "Cold legs";
   case 2: return "Cold legs!";
   case 3: return "Freezing legs!!";}	
 case DI_COLD_FEET:		
  switch (dis.intensity) {
   case 1: return "Cold feet";
   case 2: return "Cold feet!";
   case 3: return "Freezing feet!!";}
 case DI_FROSTBITE_HANDS:
  switch (dis.intensity) {
   case 1: return "Frostnip - hands";
   case 2: return "Frostbite - hands";}
 case DI_FROSTBITE_FEET:
  switch (dis.intensity) {
   case 1: return "Frostnip - feet";
   case 2: return "Frostbite - feet";}
 case DI_FROSTBITE_MOUTH:
  switch (dis.intensity) {
   case 1: return "Frostnip - face";
   case 2: return "Frostbite - face";}
 case DI_HOT_HEAD:		
  switch (dis.intensity) {
   case 1: return "Hot head";
   case 2: return "Hot head!";
   case 3: return "Scorching head!!";}	
 case DI_HOT_TORSO:		
  switch (dis.intensity) {
   case 1: return "Hot torso";
   case 2: return "Hot torso!";
   case 3: return "Scorching torso!!";}	
 case DI_HOT_ARMS:		
  switch (dis.intensity) {
   case 1: return "Hot arms";
   case 2: return "Hot arms!";
   case 3: return "Scorching arms!!";}	
 case DI_HOT_HANDS:		
  switch (dis.intensity) {
   case 1: return "Hot hands";
   case 2: return "Hot hands!";
   case 3: return "Scorching hands!!";}	
 case DI_HOT_LEGS:		
  switch (dis.intensity) {
   case 1: return "Hot legs";
   case 2: return "Hot legs!";
   case 3: return "Scorching legs!!";}	
 case DI_HOT_FEET:		
  switch (dis.intensity) {
   case 1: return "Hot feet";
   case 2: return "Hot feet!";
   case 3: return "Scorching feet!!";}	
 case DI_BLISTERS_MOUTH: return "Blisters - face";
 case DI_BLISTERS_TORSO: return "Blisters - torso";
 case DI_BLISTERS_ARMS: return "Blisters - arms";
 case DI_BLISTERS_HANDS: return "Blisters - hands";
 case DI_BLISTERS_LEGS: return "Blisters - legs";
 case DI_BLISTERS_FEET: return "Blisters - feet";
 case DI_COMMON_COLD:	return "Common Cold";
 case DI_FLU:		return "Influenza";
 case DI_SMOKE:		return "Smoke";
 case DI_TEARGAS:	return "Tear gas";
 case DI_ONFIRE:	return "On Fire";
 case DI_BOOMERED:	return "Boomered";
 case DI_SAP:		return "Sap-coated";
 case DI_SPORES:	return "Spores";
 case DI_SLIMED:	return "Slimed";
 case DI_DEAF:		return "Deaf";
 case DI_BLIND:		return "Blind";
 case DI_STUNNED:	return "Stunned";
 case DI_DOWNED:	return "Downed";
 case DI_POISON:	return "Poisoned";
 case DI_BLEED:	        return "Bleeding";
 case DI_BADPOISON:	return "Badly Poisoned";
 case DI_FOODPOISON:	return "Food Poisoning";
 case DI_SHAKES:	return "Shakes";
 case DI_FORMICATION:	return "Bugs Under Skin";
 case DI_WEBBED:	return "Webbed";
 case DI_RAT:		return "Ratting";
 case DI_DRUNK:
  if (dis.duration > 2200) return "Wasted";
  if (dis.duration > 1400) return "Trashed";
  if (dis.duration > 800)  return "Drunk";
                           return "Tipsy";

 case DI_CIG:		return "Cigarette";
 case DI_HIGH:		return "High";
 case DI_VISUALS:	return "Hallucinating";

 case DI_ADRENALINE:
  if (dis.duration > 150) return "Adrenaline Rush";
                          return "Adrenaline Comedown";

 case DI_ASTHMA:
  if (dis.duration > 800) return "Heavy Asthma";
                          return "Asthma";

 case DI_GRACK:         return "UNLEASH THE GRACKEN!!!!";

 case DI_METH:
  if (dis.duration > 600) return "High on Meth";
                          return "Meth Comedown";

 case DI_IN_PIT:	return "Stuck in Pit";

 case DI_ATTACK_BOOST:  return "Hit Bonus";
 case DI_DAMAGE_BOOST:  return "Damage Bonus";
 case DI_DODGE_BOOST:   return "Dodge Bonus";
 case DI_ARMOR_BOOST:   return "Armor Bonus";
 case DI_SPEED_BOOST:   return "Attack Speed Bonus";
 case DI_VIPER_COMBO:
  switch (dis.intensity) {
   case 1: return "Snakebite Unlocked!";
   case 2: return "Viper Strike Unlocked!";
   default: return "VIPER BUG!!!!";
  }
  break;

 default:		return "";
 }
}

std::string dis_description(disease dis)
{
 int strpen, dexpen, intpen, perpen;
 std::stringstream stream;
 switch (dis.type) {

 case DI_NULL:
  return "None";

 case DI_GLARE:
  return "Perception - 1";

 case DI_COLD_HEAD:		
  switch (dis.intensity) {
   case 1: return "Your head is exposed to the cold.";
   case 2: return "Your head is very exposed to the cold. It is hard to concentrate.";
   case 3: return "Your head is dangerously cold. Getting undressed sounds like a good idea.";}

 case DI_COLD_EYES:		
  switch (dis.intensity) {
   case 1: return "Your eyes are exposed to the cold.";
   case 2: return "Your eyes are very exposed to the cold.";
   case 3: return "Your eyes are dangerously cold.";}

 case DI_COLD_MOUTH:		
  switch (dis.intensity) {
   case 1: return "Your mouth is exposed to the cold.";
   case 2: return "Your mouth is very exposed to the cold.";
   case 3: return "Your mouth is dangerously cold.";}

 case DI_COLD_TORSO:		
  switch (dis.intensity) {
   case 1: return "Your torso is exposed to the cold.";
   case 2: return "Your torso is very exposed to the cold. Your actions are incoordinated.";
   case 3: return "Your torso is dangerously cold. Your actions are incredibly incoordinated.";}

 case DI_COLD_ARMS:		
  switch (dis.intensity) {
   case 1: return "Your arms are exposed to the cold.";
   case 2: return "Your arms are very exposed to the cold. Your arms are shivering.";
   case 3: return "Your arms is dangerously cold. Your arms are shivering uncontrollably";}

 case DI_COLD_HANDS:		
  switch (dis.intensity) {
   case 1: return "Your hands are exposed to the cold.";
   case 2: return "Your hands are very exposed to the cold. Your hands are shivering.";
   case 3: return "Your hands is dangerously cold. Your hands are shivering uncontrollably";}

 case DI_COLD_LEGS:		
  switch (dis.intensity) {
   case 1: return "Your legs are exposed to the cold.";
   case 2: return "Your legs are very exposed to the cold. Your strength is sapped.";
   case 3: return "Your legs is dangerously cold. Your strength is sapped.";}

 case DI_COLD_FEET:		
  switch (dis.intensity) {
   case 1: return "Your feet are exposed to the cold.";
   case 2: return "Your feet are very exposed to the cold. Your strength is sapped.";
   case 3: return "Your feet is dangerously cold. Your strength is sapped.";}

 case DI_FROSTBITE_HANDS:
  switch (dis.intensity) {
   case 1: return "\
   Your hands are frostnipped from the prolonged exposure to the cold and have gone numb. When the blood begins to flow, it will be painful.";
   case 2: return "\
   Your hands are frostbitten from the prolonged exposure to the cold. The tissues in your hands are frozen.";}
 case DI_FROSTBITE_FEET:
  switch (dis.intensity) {
   case 1: return "\
   Your feet are frostnipped from the prolonged exposure to the cold and have gone numb. When the blood begins to flow, it will be painful.";
   case 2: return "\
   Your feet are frostbitten from the prolonged exposure to the cold. The tissues in your feet are frozen.";}
 case DI_FROSTBITE_MOUTH:
  switch (dis.intensity) {
   case 1: return "\
   Your face is frostnipped from the prolonged exposure to the cold and has gone numb. When the blood begins to flow, it will be painful.";
   case 2: return "\
   Your face is frostbitten from the prolonged exposure to the cold. The tissues in your face are frozen.";}

 case DI_FROSTBITE_TORSO: return "\
 Your torso is frostbitten from prolonged exposure to the cold. It is extremely painful.";
 case DI_FROSTBITE_ARMS: return "\
 Your arms are frostbitten from prolonged exposure to the cold. It is extremely painful.";
 case DI_FROSTBITE_LEGS: return "\
 Your legs are frostbitten from prolonged exposure to the cold. It is extremely painful.";

 case DI_HOT_HEAD:		
  switch (dis.intensity) {
   case 1: return "Your head feels warm.";
   case 2: return "Your head is sweating from the heat. You feel nauseated. You have a headache.";
   case 3: return "Your head is sweating profusely. You feel very nauseated. You have a headache.";}	

 case DI_HOT_TORSO:		
  switch (dis.intensity) {
   case 1: return "Your torso feels warm.";
   case 2: return "Your torso is sweating from the heat. You feel weak.";
   case 3: return "Your torso is sweating profusely. You feel very weak.";}	

 case DI_HOT_ARMS:		
  switch (dis.intensity) {
   case 1: return "Your arms feel warm.";
   case 2: return "Your arms are sweating from the heat.";
   case 3: return "Your arms are sweating profusely. Your muscles are in pain due to cramps.";}	

 case DI_HOT_HANDS:		
  switch (dis.intensity) {
   case 1: return "Your hands feel warm.";
   case 2: return "Your hands feel hot and incoordinated.";
   case 3: return "Your hands feel disgustinly hot and are very incoordinated.";}	

 case DI_HOT_LEGS:		
  switch (dis.intensity) {
   case 1: return "Your legs feel warm.";
   case 2: return "Your legs are sweating from the heat.";
   case 3: return "Your legs are sweating profusely. Your muscles are in pain due to cramps.";}	

 case DI_HOT_FEET:		
  switch (dis.intensity) {
   case 1: return "Your feet feel hot.";
   case 2: return "Your feet are painfully swollen due to the heat.";
   case 3: return "Your feet are painfully swollen due to the heat.";}	

 case DI_BLISTERS_MOUTH: return "\
 Your face is blistering from the intense heat. It is extremely painful.";
 case DI_BLISTERS_TORSO: return "\
 Your torso is blistering from the intense heat. It is extremely painful.";
 case DI_BLISTERS_ARMS: return "\
 Your arms are blistering from the intense heat. It is extremely painful.";
 case DI_BLISTERS_HANDS: return "\
 Your hands are blistering from the intense heat. It is extremely painful.";
 case DI_BLISTERS_LEGS: return "\
 Your legs are blistering from the intense heat. It is extremely painful.";
 case DI_BLISTERS_FEET: return "\
 Your feet are blistering from the intense heat. It is extremely painful.";

 case DI_COMMON_COLD:	return "\
Increased thirst;  Frequent coughing\n\
Strength - 3;  Dexterity - 1;  Intelligence - 2;  Perception - 1\n\
Symptoms alleviated by medication (Dayquil or Nyquil).";

 case DI_FLU:		return "\
Increased thirst;  Frequent coughing;  Occasional vomiting\n\
Strength - 4;  Dexterity - 2;  Intelligence - 2;  Perception - 1\n\
Symptoms alleviated by medication (Dayquil or Nyquil).";

 case DI_SMOKE:		return "\
Strength - 1;     Dexterity - 1;\n\
Occasionally you will cough, costing movement and creating noise.\n\
Loss of health - Torso";

 case DI_TEARGAS:	return "\
Strength - 2;     Dexterity - 2;    Intelligence - 1;    Perception - 4\n\
Occasionally you will cough, costing movement and creating noise.\n\
Loss of health - Torso";

 case DI_ONFIRE:	return "\
Loss of health - Entire Body\n\
Your clothing and other equipment may be consumed by the flames.";

 case DI_BOOMERED:	return "\
Perception - 5\n\
Range of Sight: 1;     All sight is tinted magenta";

 case DI_SAP:		return "\
Dexterity - 3;   Speed - 25";

 case DI_SPORES:	return "\
Speed -40%\
You can feel the tiny spores sinking directly into your flesh.";

 case DI_SLIMED:	return "\
Speed -40%;     Dexterity - 2";

 case DI_DEAF:		return "\
Sounds will not be reported.  You cannot talk with NPCs.";

 case DI_BLIND:		return "\
Range of Sight: 0";

 case DI_STUNNED:	return "\
Your movement is randomized.";

 case DI_DOWNED:	return "\
You're knocked to the ground.  You have to get up before you can move.";

 case DI_POISON:	return "\
Perception - 1;    Dexterity - 1;   Strength - 2 IF not resistant\n\
Occasional pain and/or damage.";

 case DI_BLEED:	return "\
 You are slowly losing blood.";

 case DI_BADPOISON:	return "\
Perception - 2;    Dexterity - 2;\n\
Strength - 3 IF not resistant, -1 otherwise\n\
Frequent pain and/or damage.";

 case DI_FOODPOISON:	return "\
Speed - 35%;     Strength - 3;     Dexterity - 1;     Perception - 1\n\
Your stomach is extremely upset, and you keep having pangs of pain and nausea.";

 case DI_SHAKES:	return "\
Strength - 1;     Dexterity - 4;";

 case DI_FORMICATION:	return "\
Strength - 1;     Intelligence - 2;\n\
You stop to scratch yourself frequently; high intelligence helps you resist\n\
this urge.";

 case DI_WEBBED:	return "\
Strength - 1;     Dexterity - 4;    Speed - 25";

 case DI_RAT:
  intpen = int(dis.duration / 20);
  perpen = int(dis.duration / 25);
  strpen = int(dis.duration / 50);
  stream << "You feal nauseated and rat-like.\n";
  if (intpen > 0)
   stream << "Intelligence - " << intpen << ";   ";
  if (perpen > 0)
   stream << "Perception - " << perpen << ";   ";
  if (strpen > 0)
   stream << "Strength - " << strpen << ";   ";
  return stream.str();

 case DI_DRUNK:
  perpen = int(dis.duration / 1000);
  dexpen = int(dis.duration / 1000);
  intpen = int(dis.duration /  700);
  strpen = int(dis.duration / 1500);
  if (strpen > 0)
   stream << "Strength - " << strpen << ";    ";
  else if (dis.duration <= 600)
   stream << "Strength + 1;    ";
  if (dexpen > 0)
   stream << "Dexterity - " << dexpen << ";    ";
  if (intpen > 0)
   stream << "Intelligence - " << intpen << ";    ";
  if (perpen > 0)
   stream << "Perception - " << perpen;

  return stream.str();

 case DI_CIG:
  if (dis.duration >= 600)
   return "\
Strength - 1;     Dexterity - 1\n\
You smoked too much.";
  return "\
Dexterity + 1;     Intelligence + 1;     Perception + 1";

 case DI_HIGH:
  return "\
Intelligence - 1;     Perception - 1";

 case DI_VISUALS:
  return "\
You can't trust everything that you see.";

 case DI_ADRENALINE:
  if (dis.duration > 150)
   return "\
Speed +80;   Strength + 5;   Dexterity + 3;   Intelligence - 8;   Perception + 1";
  return "\
Strength - 2;     Dexterity - 1;     Intelligence - 1;     Perception - 1";

 case DI_ASTHMA:
  stream<< "Speed - " << int(dis.duration / 5) << "%;     Strength - 2;     " <<
           "Dexterity - 3";
  return stream.str();

 case DI_GRACK:
  return "\
Unleashed the Gracken";

 case DI_METH:
  if (dis.duration > 600)
   return "\
Speed +50;  Strength + 2;  Dexterity + 2;  Intelligence + 3;  Perception + 3";
   return "\
Speed -40;   Strength - 3;   Dexterity - 2;   Intelligence - 2";

 case DI_IN_PIT:
  return "\
You're stuck in a pit.  Sight distance is limited and you have to climb out.";

 case DI_ATTACK_BOOST:
  stream << "To-hit bonus + " << dis.intensity;
  return stream.str();

 case DI_DAMAGE_BOOST:
  stream << "Damage bonus + " << dis.intensity;
  return stream.str();

 case DI_DODGE_BOOST:
  stream << "Dodge bonus + " << dis.intensity;
  return stream.str();

 case DI_ARMOR_BOOST:
  stream << "Armor bonus + " << dis.intensity;
  return stream.str();

 case DI_SPEED_BOOST:
  stream << "Attack speed + " << dis.intensity;
  return stream.str();

 case DI_VIPER_COMBO:
  switch (dis.intensity) {
   case 1: return "\
Your next strike will be a Snakebite, using your hand in a cone shape.  This\n\
will deal piercing damage.";
   case 2: return "\
Your next strike will be a Viper Strike.  It requires both arms to be in good\n\
condition, and deals massive damage.";
  }

 default:
  return "Who knows?  This is probably a bug.";
 }
}

