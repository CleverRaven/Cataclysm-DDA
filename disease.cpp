#include "rng.h"
#include "game.h"
#include "bodypart.h"
#include "disease.h"
#include "weather.h"
#include "translations.h"
#include <stdlib.h>
#include <sstream>

// Used only internally for fast lookups.
enum dis_type_enum {
 DI_NULL,
// Weather
 DI_GLARE, DI_WET,
// Temperature, the order is important (dependant on bodypart.h)
 DI_COLD,
 DI_COLD_TORSO, DI_COLD_HEAD, DI_COLD_EYES, DI_COLD_MOUTH,
 DI_COLD_ARMS, DI_COLD_HANDS, DI_COLD_LEGS, DI_COLD_FEET,
 DI_FROSTBITE,
 DI_FROSTBITE_TORSO, DI_FROSTBITE_HEAD, DI_FROSTBITE_EYES, DI_FROSTBITE_MOUTH,
 DI_FROSTBITE_ARMS, DI_FROSTBITE_HANDS, DI_FROSTBITE_LEGS, DI_FROSTBITE_FEET,
 DI_HOT,
 DI_HOT_TORSO, DI_HOT_HEAD, DI_HOT_EYES, DI_HOT_MOUTH,
 DI_HOT_ARMS, DI_HOT_HANDS, DI_HOT_LEGS, DI_HOT_FEET,
 DI_BLISTERS,
 DI_BLISTERS_TORSO, DI_BLISTERS_HEAD, DI_BLISTERS_EYES, DI_BLISTERS_MOUTH,
 DI_BLISTERS_ARMS, DI_BLISTERS_HANDS, DI_BLISTERS_LEGS, DI_BLISTERS_FEET,
// Diseases
 DI_INFECTION,
 DI_COMMON_COLD, DI_FLU, DI_RECOVER,
// Fields
 DI_SMOKE, DI_ONFIRE, DI_TEARGAS, DI_CRUSHED, DI_BOULDERING,
// Monsters
 DI_BOOMERED, DI_SAP, DI_SPORES, DI_FUNGUS, DI_SLIMED,
 DI_DEAF, DI_BLIND,
 DI_LYING_DOWN, DI_SLEEP, DI_ALARM_CLOCK,
 DI_POISON, DI_BLEED, DI_BADPOISON, DI_FOODPOISON, DI_SHAKES,
 DI_DERMATIK, DI_FORMICATION,
 DI_WEBBED,
 DI_RAT, DI_BITE,
// Food & Drugs
 DI_PKILL1, DI_PKILL2, DI_PKILL3, DI_PKILL_L, DI_DRUNK, DI_CIG, DI_HIGH,
  DI_HALLU, DI_VISUALS, DI_IODINE, DI_TOOK_XANAX, DI_TOOK_PROZAC,
  DI_TOOK_FLUMED, DI_ADRENALINE, DI_ASTHMA, DI_GRACK, DI_METH,
// Traps
 DI_BEARTRAP, DI_LIGHTSNARE, DI_HEAVYSNARE, DI_IN_PIT, DI_STUNNED, DI_DOWNED,
// Martial Arts
 DI_ATTACK_BOOST, DI_DAMAGE_BOOST, DI_DODGE_BOOST, DI_ARMOR_BOOST,
  DI_SPEED_BOOST, DI_VIPER_COMBO,
// Other
 DI_AMIGARA, DI_STEMCELL_TREATMENT, DI_TELEGLOW, DI_ATTENTION, DI_EVIL, DI_INFECTED,
// Inflicted by an NPC
 DI_ASKED_TO_FOLLOW, DI_ASKED_TO_LEAD, DI_ASKED_FOR_ITEM,
// NPC-only
 DI_CATCH_UP
};

std::map<std::string, dis_type_enum> disease_type_lookup;

void game::init_diseases() {
    // Initialize the disease lookup table.

    disease_type_lookup["null"] = DI_NULL;
    disease_type_lookup["glare"] = DI_GLARE;
    disease_type_lookup["wet"] = DI_WET;
    disease_type_lookup["cold"] = DI_COLD;
    disease_type_lookup["cold_torso"] = DI_COLD_TORSO;
    disease_type_lookup["cold_head"] = DI_COLD_HEAD;
    disease_type_lookup["cold_eyes"] = DI_COLD_EYES;
    disease_type_lookup["cold_mouth"] = DI_COLD_MOUTH;
    disease_type_lookup["cold_arms"] = DI_COLD_ARMS;
    disease_type_lookup["cold_hands"] = DI_COLD_HANDS;
    disease_type_lookup["cold_legs"] = DI_COLD_LEGS;
    disease_type_lookup["cold_feet"] = DI_COLD_FEET;
    disease_type_lookup["frostbite"] = DI_FROSTBITE;
    disease_type_lookup["frostbite_torso"] = DI_FROSTBITE_TORSO;
    disease_type_lookup["frostbite_head"] = DI_FROSTBITE_HEAD;
    disease_type_lookup["frostbite_eyes"] = DI_FROSTBITE_EYES;
    disease_type_lookup["frostbite_mouth"] = DI_FROSTBITE_MOUTH;
    disease_type_lookup["frostbite_arms"] = DI_FROSTBITE_ARMS;
    disease_type_lookup["frostbite_hands"] = DI_FROSTBITE_HANDS;
    disease_type_lookup["frostbite_legs"] = DI_FROSTBITE_LEGS;
    disease_type_lookup["frostbite_feet"] = DI_FROSTBITE_FEET;
    disease_type_lookup["hot"] = DI_HOT;
    disease_type_lookup["hot_torso"] = DI_HOT_TORSO;
    disease_type_lookup["hot_head"] = DI_HOT_HEAD;
    disease_type_lookup["hot_eyes"] = DI_HOT_EYES;
    disease_type_lookup["hot_mouth"] = DI_HOT_MOUTH;
    disease_type_lookup["hot_arms"] = DI_HOT_ARMS;
    disease_type_lookup["hot_hands"] = DI_HOT_HANDS;
    disease_type_lookup["hot_legs"] = DI_HOT_LEGS;
    disease_type_lookup["hot_feet"] = DI_HOT_FEET;
    disease_type_lookup["blisters"] = DI_BLISTERS;
    disease_type_lookup["blisters_torso"] = DI_BLISTERS_TORSO;
    disease_type_lookup["blisters_head"] = DI_BLISTERS_HEAD;
    disease_type_lookup["blisters_eyes"] = DI_BLISTERS_EYES;
    disease_type_lookup["blisters_mouth"] = DI_BLISTERS_MOUTH;
    disease_type_lookup["blisters_arms"] = DI_BLISTERS_ARMS;
    disease_type_lookup["blisters_hands"] = DI_BLISTERS_HANDS;
    disease_type_lookup["blisters_legs"] = DI_BLISTERS_LEGS;
    disease_type_lookup["blisters_feet"] = DI_BLISTERS_FEET;
    disease_type_lookup["infection"] = DI_INFECTION;
    disease_type_lookup["common_cold"] = DI_COMMON_COLD;
    disease_type_lookup["flu"] = DI_FLU;
    disease_type_lookup["recover"] = DI_RECOVER;
    disease_type_lookup["smoke"] = DI_SMOKE;
    disease_type_lookup["onfire"] = DI_ONFIRE;
    disease_type_lookup["teargas"] = DI_TEARGAS;
    disease_type_lookup["crushed"] = DI_CRUSHED;
    disease_type_lookup["bouldering"] = DI_BOULDERING;
    disease_type_lookup["boomered"] = DI_BOOMERED;
    disease_type_lookup["sap"] = DI_SAP;
    disease_type_lookup["spores"] = DI_SPORES;
    disease_type_lookup["fungus"] = DI_FUNGUS;
    disease_type_lookup["slimed"] = DI_SLIMED;
    disease_type_lookup["deaf"] = DI_DEAF;
    disease_type_lookup["blind"] = DI_BLIND;
    disease_type_lookup["lying_down"] = DI_LYING_DOWN;
    disease_type_lookup["sleep"] = DI_SLEEP;
    disease_type_lookup["alarm_clock"] = DI_ALARM_CLOCK;
    disease_type_lookup["poison"] = DI_POISON;
    disease_type_lookup["bleed"] = DI_BLEED;
    disease_type_lookup["badpoison"] = DI_BADPOISON;
    disease_type_lookup["foodpoison"] = DI_FOODPOISON;
    disease_type_lookup["shakes"] = DI_SHAKES;
    disease_type_lookup["dermatik"] = DI_DERMATIK;
    disease_type_lookup["formication"] = DI_FORMICATION;
    disease_type_lookup["webbed"] = DI_WEBBED;
    disease_type_lookup["rat"] = DI_RAT;
    disease_type_lookup["bite"] = DI_BITE;
    disease_type_lookup["pkill1"] = DI_PKILL1;
    disease_type_lookup["pkill2"] = DI_PKILL2;
    disease_type_lookup["pkill3"] = DI_PKILL3;
    disease_type_lookup["pkill_l"] = DI_PKILL_L;
    disease_type_lookup["drunk"] = DI_DRUNK;
    disease_type_lookup["cig"] = DI_CIG;
    disease_type_lookup["high"] = DI_HIGH;
    disease_type_lookup["hallu"] = DI_HALLU;
    disease_type_lookup["visuals"] = DI_VISUALS;
    disease_type_lookup["iodine"] = DI_IODINE;
    disease_type_lookup["took_xanax"] = DI_TOOK_XANAX;
    disease_type_lookup["took_prozac"] = DI_TOOK_PROZAC;
    disease_type_lookup["took_flumed"] = DI_TOOK_FLUMED;
    disease_type_lookup["adrenaline"] = DI_ADRENALINE;
    disease_type_lookup["asthma"] = DI_ASTHMA;
    disease_type_lookup["grack"] = DI_GRACK;
    disease_type_lookup["meth"] = DI_METH;
    disease_type_lookup["beartrap"] = DI_BEARTRAP;
    disease_type_lookup["lightsnare"] = DI_LIGHTSNARE;
    disease_type_lookup["heavysnare"] = DI_HEAVYSNARE;
    disease_type_lookup["in_pit"] = DI_IN_PIT;
    disease_type_lookup["stunned"] = DI_STUNNED;
    disease_type_lookup["downed"] = DI_DOWNED;
    disease_type_lookup["attack_boost"] = DI_ATTACK_BOOST;
    disease_type_lookup["damage_boost"] = DI_DAMAGE_BOOST;
    disease_type_lookup["dodge_boost"] = DI_DODGE_BOOST;
    disease_type_lookup["armor_boost"] = DI_ARMOR_BOOST;
    disease_type_lookup["speed_boost"] = DI_SPEED_BOOST;
    disease_type_lookup["viper_combo"] = DI_VIPER_COMBO;
    disease_type_lookup["amigara"] = DI_AMIGARA;
    disease_type_lookup["stemcell_treatment"] = DI_STEMCELL_TREATMENT;
    disease_type_lookup["teleglow"] = DI_TELEGLOW;
    disease_type_lookup["attention"] = DI_ATTENTION;
    disease_type_lookup["evil"] = DI_EVIL;
    disease_type_lookup["infected"] = DI_INFECTED;
    disease_type_lookup["asked_to_follow"] = DI_ASKED_TO_FOLLOW;
    disease_type_lookup["asked_to_lead"] = DI_ASKED_TO_LEAD;
    disease_type_lookup["asked_for_item"] = DI_ASKED_FOR_ITEM;
    disease_type_lookup["catch_up"] = DI_CATCH_UP;
}

dis_type disease_for_body_part(dis_type base, int body_part) {
    if(base == "hot") {
        switch(body_part) {
        case 0:
            return "hot_torso";
        case 1:
            return "hot_head";
        case 2:
            return "hot_eyes";
        case 3:
            return "hot_mouth";
        case 4:
            return "hot_arms";
        case 5:
            return "hot_hands";
        case 6:
            return "hot_legs";
        case 7:
            return "hot_feet";
        default:
            return "none";
        }
    }
    else if(base == "cold") {
        switch(body_part) {
        case 0:
            return "cold_torso";
        case 1:
            return "cold_head";
        case 2:
            return "cold_eyes";
        case 3:
            return "cold_mouth";
        case 4:
            return "cold_arms";
        case 5:
            return "cold_hands";
        case 6:
            return "cold_legs";
        case 7:
            return "cold_feet";
        default:
            return "none";
        }
    }
    else if(base == "blisters") {
        switch(body_part) {
        case 0:
            return "blisters_torso";
        case 1:
            return "blisters_head";
        case 2:
            return "blisters_eyes";
        case 3:
            return "blisters_mouth";
        case 4:
            return "blisters_arms";
        case 5:
            return "blisters_hands";
        case 6:
            return "blisters_legs";
        case 7:
            return "blisters_feet";
        default:
            return "none";
        }
    }
    else if(base == "frostbite") {
        switch(body_part) {
        case 0:
            return "frostbite_torso";
        case 1:
            return "frostbite_head";
        case 2:
            return "frostbite_eyes";
        case 3:
            return "frostbite_mouth";
        case 4:
            return "frostbite_arms";
        case 5:
            return "frostbite_hands";
        case 6:
            return "frostbite_legs";
        case 7:
            return "frostbite_feet";
        default:
            return "none";
        }
    }

    return "none";
}

void dis_msg(game *g, dis_type type_string)
{
    dis_type_enum type = disease_type_lookup[type_string];
    switch (type) {
    case DI_GLARE:
        g->add_msg(_("The sunlight's glare makes it hard to see."));
        break;
    case DI_WET:
        g->add_msg(_("You're getting soaked!"));
        break;
    case DI_COMMON_COLD:
        g->add_msg(_("You feel a cold coming on..."));
        g->u.add_memorial_log(_("Caught a cold."));
        break;
    case DI_FLU:
        g->add_msg(_("You feel a flu coming on..."));
        g->u.add_memorial_log(_("Caught the flu."));
        break;
    case DI_ONFIRE:
        g->add_msg(_("You're on fire!"));
        g->u.add_memorial_log(_("Caught on fire."));
        break;
    case DI_SMOKE:
        g->add_msg(_("You inhale a lungful of thick smoke."));
        break;
    case DI_TEARGAS:
        g->add_msg(_("You inhale a lungful of tear gas."));
        break;
    case DI_CRUSHED:
        g->add_msg(_("The ceiling collapses on you!"));
        break;
    case DI_BOULDERING:
        g->add_msg(_("You are slowed by the rubble."));
        break;
    case DI_BOOMERED:
        g->add_msg(_("You're covered in bile!"));
        break;
    case DI_SAP:
        g->add_msg(_("You're coated in sap!"));
        break;
    case DI_SPORES:
        g->add_msg(_("You're covered in tiny spores!"));
        g->u.add_memorial_log(_("Contracted a fungal infection."));
        break;
    case DI_SLIMED:
        g->add_msg(_("You're covered in thick goo!"));
        break;
    case DI_LYING_DOWN:
        g->add_msg(_("You lie down to go to sleep..."));
        break;
    case DI_FORMICATION:
        g->add_msg(_("There's bugs crawling under your skin!"));
        g->u.add_memorial_log(_("Injected with dermatik eggs."));
        break;
    case DI_WEBBED:
        g->add_msg(_("You're covered in webs!"));
        break;
    case DI_DRUNK:
    case DI_HIGH:
        g->add_msg(_("You feel lightheaded."));
        break;
    case DI_ADRENALINE:
        g->add_msg(_("You feel a surge of adrenaline!"));
        g->u.moves += 800; // FIXME this should not be here
        break;
    case DI_ASTHMA:
        g->add_msg(_("You can't breathe... asthma attack!"));
        break;
    case DI_DEAF:
        g->add_msg(_("You're deafened!"));
        break;
    case DI_BLIND:
        g->add_msg(_("You're blinded!"));
        break;
    case DI_STUNNED:
        g->add_msg(_("You're stunned!"));
        break;
    case DI_DOWNED:
        g->add_msg(_("You're knocked to the floor!"));
        break;
    case DI_AMIGARA:
        g->add_msg(_("You can't look away from the faultline..."));
        break;
    case DI_STEMCELL_TREATMENT:
        g->add_msg(_("You receive a pureed bone & enamel injection into your eyeball."));
        g->add_msg(_("It is excruciating."));
        break;
    case DI_BITE:
        g->add_msg(_("The bite wound feels really deep..."));
        g->u.add_memorial_log(_("Received a deep bite wound."));
        break;
    case DI_INFECTED:
        g->add_msg(_("Your bite wound feels infected"));
        g->u.add_memorial_log(_("Contracted the infection."));
        break;
    case DI_LIGHTSNARE:
        g->add_msg(_("You are snared."));
        break;
    case DI_HEAVYSNARE:
        g->add_msg(_("You are snared."));
        break;
    default:
        break;
    }
}

void dis_effect(game *g, player &p, disease &dis)
{
 std::stringstream sTemp;

 int bonus;
 dis_type_enum type = disease_type_lookup[dis.type];
 switch (type) {
 case DI_GLARE:
  p.per_cur -= 1;
  break;

 case DI_WET:
  p.add_morale(MORALE_WET, -1, -50, 60, 10);
  break;

 case DI_COLD_HEAD:
  switch (dis.intensity) {
   case 3 : p.int_cur -= 3; if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your thoughts are unclear."));
   case 2 : p.int_cur--;
  }
  break;

  case DI_COLD_MOUTH:
  switch (dis.intensity) {
   case 3 : p.per_cur -= 3; if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your face is stiff from the cold."));
   case 2 : p.per_cur--;
  }
  break;

 case DI_COLD_TORSO:
  switch (dis.intensity) {
   case 3 : p.dex_cur -= 2; if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your torso is burning up. You should remove some layers."));
   // Speed -20
   case 2 :	p.dex_cur -= 1;
   // Speed -5
   // case1 : Speed -2
  }
  break;

 case DI_COLD_ARMS:
  switch (dis.intensity) {
   case 3 : p.dex_cur -= 2;
   case 2 :	p.dex_cur--; if (!p.has_disease("sleep") && one_in(800)) g->add_msg(_("Your arms are shivering."));
  }
  break;

 case DI_COLD_HANDS:
  switch (dis.intensity) {
   case 3 :	p.dex_cur -= 2;
   case 2 :	p.dex_cur -= 2; if (!p.has_disease("sleep") && one_in(800)) g->add_msg(_("Your hands are shivering."));
  }
  break;

 case DI_COLD_LEGS:
  switch (dis.intensity) {
   case 3 :	p.str_cur--; if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your legs are seizing from the incredible cold."));
   // Speed -20
   case 2 :	p.str_cur--;
   // Speed -5
   // case 1 : Speed -2
  }
  break;

 case DI_COLD_FEET:
  switch (dis.intensity) {
   case 3 : p.str_cur--;
   case 2 : p.str_cur--;
  }
  break;

 case DI_FROSTBITE_HANDS:
  switch (dis.intensity) {
   case 2 : p.dex_cur -= 3;
   case 1 :
    if (p.temp_cur[bp_hands] > BODYTEMP_COLD && p.pain < 40) p.pain++;
    if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your hands feel numb."));
  }
 break;

 case DI_FROSTBITE_FEET:
  switch (dis.intensity) {
   case 2 : // -4 speed
   case 1 :
    if (p.temp_cur[bp_feet] > BODYTEMP_COLD && p.pain < 40) p.pain++;
	if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your feet feel numb."));
  }
 break;

 case DI_FROSTBITE_MOUTH:
  switch (dis.intensity) {
   case 2 : p.per_cur -= 3;
   case 1 :
    if (p.temp_cur[bp_mouth] > BODYTEMP_COLD && p.pain < 40) p.pain++;
	if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your face feels numb."));
  }
 break;

 case DI_BLISTERS_HANDS:
  p.dex_cur--;
  if (p.pain < 35) p.pain++;
  if (one_in(2)) p.hp_cur[hp_arm_r]--;
  else p.hp_cur[hp_arm_l]--;
  break;

 case DI_BLISTERS_FEET:
  p.str_cur--;
  if (p.pain < 35) p.pain++;
  if (one_in(2)) p.hp_cur[hp_leg_r]--;
  else p.hp_cur[hp_leg_l]--;
  break;

 case DI_BLISTERS_MOUTH:
  p.per_cur--;
  p.hp_cur[hp_head]--;
  if (p.pain < 35) p.pain++;
  break;

 case DI_HOT_HEAD:
  switch (dis.intensity) {
   case 3 :
    if (int(g->turn) % 150 == 0)
     p.thirst++;
	if (p.pain < 40) p.pain++;
    if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your head is pounding from the heat."));
    // Speed -20
   case 2 :
    if (int(g->turn) % 300 == 0)
     p.thirst++;
    // Hallucinations handled in game.cpp
    if (one_in(std::min(14500,15000-p.temp_cur[bp_head]))) p.vomit(g);
	if (p.pain < 20) p.pain++;
    if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("The heat is making you see things."));
    // Speed -5
    // case 1 : Speed -2
  }
  break;

 case DI_HOT_MOUTH:
  switch (dis.intensity) {
   case 3 : if (int(g->turn) % 150 == 0) p.thirst++;
    if (p.pain < 30) p.pain++;
   case 2 : if (int(g->turn) % 300 == 0) p.thirst++;
  }
  break;

 case DI_HOT_TORSO:
  switch (dis.intensity) {
   case 3 :
    if (int(g->turn) % 150 == 0) p.thirst++;
    p.str_cur--;
    if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("You are sweating profusely."));
    // Speed -20
   case 2 :
    if (int(g->turn) % 300 == 0) p.thirst++;
	p.str_cur--;
    // Speed -5
    // case 1 : Speed -2
  }
  break;

 case DI_HOT_ARMS:
  switch (dis.intensity) {
   case 3 :
    if (int(g->turn) % 150 == 0) p.thirst++;
    if (p.pain < 30) p.pain++;
   case 2 : if (int(g->turn) % 300 == 0) p.thirst++;
  }
  break;

 case DI_HOT_HANDS:
  switch (dis.intensity) {
   case 3 : p.dex_cur--;
   case 2 : p.dex_cur--;
  }
  break;

 case DI_HOT_LEGS:
  switch (dis.intensity) {
   case 3 :
    if (int(g->turn) % 150 == 0) p.thirst++;
    if (p.pain < 30) p.pain++;
    if (one_in(400)) g->add_msg(_("Your legs are cramping up."));
   case 2 : if (int(g->turn) % 300 == 0) p.thirst++;
  }
  break;

 case DI_HOT_FEET:
  switch (dis.intensity) {
   case 3 :
    if (p.pain < 30) p.pain++;
    if (!p.has_disease("sleep") && one_in(400)) g->add_msg(_("Your feet are swelling in the heat."));
  }
  break;

 case DI_COMMON_COLD:
  if (int(g->turn) % 300 == 0)
   p.thirst++;
  if (int(g->turn) % 50 == 0)
   p.fatigue++;
  if (p.has_disease("took_flumed")) {
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
    g->add_msg(_("You cough noisily."));
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, _("loud coughing."));
  }
  break;

 case DI_FLU:
  if (int(g->turn) % 300 == 0)
   p.thirst++;
  if (p.has_disease("took_flumed")) {
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
    g->add_msg(_("You cough noisily."));
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, _("loud coughing."));
  }
  if (one_in(3600) || (p.has_trait("WEAKSTOMACH") && one_in(3000)) ||
      (p.has_trait("NAUSEA") && one_in(2400))) {
   if (!p.has_disease("took_flumed") || one_in(2))
    p.vomit(g);
  }
  break;

 case DI_SMOKE:
  // A hard limit on the duration of the smoke disease.
  if(dis.duration >= 600) {
    dis.duration = 600;
  }
  p.str_cur--;
  p.dex_cur--;
  if (one_in(5)) {
   if (!p.is_npc()) {
    g->add_msg(_("You cough heavily."));
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, _("a hacking cough."));
   p.moves -= 80;
   p.hurt(g, bp_torso, 0, 1 - (rng(0, 1) * rng(0, 1)));
   if (p.has_disease("sleep")) {
       p.rem_disease("sleep");
       g->add_msg_if_player(&p,_("Your coughing wakes you up."));
   }
  }
  break;

 case DI_TEARGAS:
  p.str_cur -= 2;
  p.dex_cur -= 2;
  p.int_cur -= 1;
  p.per_cur -= 4;
  if (one_in(3)) {
   if (!p.is_npc()) {
    g->add_msg(_("You cough heavily."));
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, _("a hacking cough."));
   p.moves -= 100;
   p.hurt(g, bp_torso, 0, rng(0, 3) * rng(0, 1));
  }
  break;


// MATERIALS-TODO: this should be determined by materials properties
 case DI_ONFIRE:
  p.hurtall(3);
  for (int i = 0; i < p.worn.size(); i++) {
   if (p.worn[i].made_of("veggy") || p.worn[i].made_of("paper")) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   } else if ((p.worn[i].made_of("cotton") || p.worn[i].made_of("wool")) &&
              one_in(10)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   } else if (p.worn[i].made_of("plastic") && one_in(50)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   }
  }
  break;

 case DI_CRUSHED:
  p.hurtall(10);
  //This could be developed on later, for instance
  //to deal different damage amounts to different body parts and
  //to account for helmets and other armor
  break;

 case DI_BOULDERING:
  switch(g->u.disease_intensity("bouldering")){
   case 1:
    p.dex_cur -= 1;
    break;
   case 2:
    p.dex_cur -= 3;
    break;
   case 3:
    p.dex_cur -= 5;
    break;
   default:
    debugmsg("Something went wrong with DI_BOULDERING.");
    debugmsg("Check disease.cpp");
  }
   if (p.dex_cur < 1) {
    p.dex_cur = 1;
   }

 case DI_BOOMERED:
  p.per_cur -= 5;
  break;

 case DI_SAP:
  p.dex_cur -= 3;
  break;

 case DI_SPORES:
  if (one_in(30))
   p.add_disease("fungus", -1);
  break;

 case DI_FUNGUS:
  bonus = 0;
  if (p.has_trait("POISRESIST"))
   bonus = 100;
  p.moves -= 10;
  p.str_cur -= 1;
  p.dex_cur -= 1;
  if (dis.duration > -600) {	// First hour symptoms
   if (one_in(160 + bonus)) {
    if (!p.is_npc()) {
     g->add_msg(_("You cough heavily."));
     g->sound(p.posx, p.posy, 12, "");
    } else
     g->sound(p.posx, p.posy, 12, _("a hacking cough."));
    p.pain++;
   }
   if (one_in(100 + bonus)) {
    g->add_msg_if_player(&p,_("You feel nauseous."));
   }
   if (one_in(100 + bonus)) {
     g->add_msg_if_player(&p,_("You smell and taste mushrooms."));
   }
  } else if (dis.duration > -3600) {	// One to six hours
   if (one_in(600 + bonus * 3)) {
    g->add_msg_if_player(&p,_("You spasm suddenly!"));
    p.moves -= 100;
    p.hurt(g, bp_torso, 0, 5);
   }
   if ((p.has_trait("WEAKSTOMACH") && one_in(1600 + bonus *  8)) ||
       (p.has_trait("NAUSEA") && one_in(800 + bonus * 6)) ||
       one_in(2000 + bonus * 10)) {
       g->add_msg_player_or_npc( &p, _("You vomit a thick, gray goop."),
                                 _("<npcname> vomits a thick, grey goop.") );

       p.moves = -200;
       p.hunger += 50;
       p.thirst += 68;
   }
  } else {	// Full symptoms
   if (one_in(1000 + bonus * 8)) {
    g->add_msg_player_or_npc( &p, _("You vomit thousands of live spores!"),
                              _("<npcname> vomits thousands of live spores!") );

    p.moves = -500;
    int sporex, sporey;
    monster spore(g->mtypes[mon_spore]);
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++) {
      sporex = p.posx + i;
      sporey = p.posy + j;
      if (g->m.move_cost(sporex, sporey) > 0 && one_in(5)) {
       if (g->mon_at(sporex, sporey) >= 0) {	// Spores hit a monster
        if (g->u_see(sporex, sporey))
         g->add_msg(_("The %s is covered in tiny spores!"),
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
       g->add_msg_player_or_npc( &p, _("Your hands bulge. Fungus stalks burst through the bulge!"),
                                 _("<npcname>'s hands bulge. Fungus stalks burst through the bulge!") );
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
   g->add_msg_if_player(&p,_("You fall asleep."));
   p.add_disease("sleep", 6000);
  }
  if (dis.duration == 1 && !p.has_disease("sleep"))
   g->add_msg_if_player(&p,_("You try to sleep, but can't..."));
  break;

  case DI_ALARM_CLOCK:
    {
        if (p.has_disease("sleep"))
        {
            if (dis.duration == 1)
            {
                if (!g->sound(p.posx, p.posy, 12, _("beep-beep-beep!"))) {
                    //You didn't hear the alarm
                    dis.duration += 100; //10 minute alarm interval
                    g->add_msg(_("An alarm rings but you don't hear it."));
                }
                else
                {
                    g->add_msg(_("You wake up to the ringing of your alarm-clock."));
                }
            }
        }
        else if (!p.has_disease("lying_down"))
        {
            //Turn the alarm-clock off if you woke up before the alarm
            dis.duration = 1;
        }
    }

  case DI_SLEEP:
  {
    p.moves = 0;

    if (int(g->turn) % 25 == 0) {
      if (p.fatigue > 0)
        p.fatigue -= 1 + rng(0, 1) * rng(0, 1);
      if (p.has_trait("FASTHEALER")) {
        p.healall(rng(0, 1));
      } else if (p.has_trait("FASTHEALER2")) {
        p.healall(rng(0, 2));
      } else if (p.has_trait("REGEN")) {
        p.healall(rng(1, 2));
      } else {
        p.healall(rng(0, 1) * rng(0, 1) * rng(0, 1));
      }

      if (p.fatigue <= 0 && p.fatigue > -20) {
        p.fatigue = -25;
        g->add_msg(_("Fully rested."));
        dis.duration = dice(3, 100);
      }
    }

    if (int(g->turn) % 100 == 0 && !p.has_bionic("bio_recycler")) {
      // Hunger and thirst advance more slowly while we sleep.
      p.hunger--;
      p.thirst--;
    }

	// Check mutation category strengths to see if we're mutated enough to get a dream
	std::string highcat = p.get_highest_category();
	int highest = p.mutation_category_level[highcat];

	// Determine the strength of effects or dreams based upon category strength
	int strength = 0;	// Category too weak for any effect or dream
	if (highest >= 20 && highest < 35)
	{
		strength = 1;	// Low strength
	}
	else if (highest >= 35 && highest < 50)
	{
		strength = 2;	// Medium strength
	}
	else if (highest >= 50)
	{
		strength = 3;	// High strength
	}

	// See if we'll get a dream
	if ((!strength == 0)) //Only if category strength is high enough to get a dream.
	{
		if ((int(g->turn) % (3600 / strength) == 0) && one_in(3)) //Once every 6 / 3 / 2 hours, with a bit of randomness
		{
			// Select a dream
			std::string dream = p.get_category_dream(highcat, strength);
			g->add_msg(_("%s"),dream.c_str());
		}
	}

    if (rng(5, 80) + rng(0, 120) + rng(0, abs(p.fatigue)) +
        rng(0, abs(p.fatigue * 5)) < g->light_level() &&
        (p.fatigue < 10 || one_in(p.fatigue / 2))) {
      g->add_msg(_("The light wakes you up."));
      dis.duration = 1;
    }

    // Cold or heat may wake you up.
    // Player will sleep through cold or heat if fatigued enough
    for (int i = 0 ; i < num_bp ; i++)
    {
        if (p.temp_cur[i] < BODYTEMP_VERY_COLD - p.fatigue/2)
        {
            if (one_in(5000)) { g->add_msg(_("You toss and turn trying to keep warm.")); }
            if (p.temp_cur[i] < BODYTEMP_FREEZING - p.fatigue/2 || (one_in(p.temp_cur[i] + 5000)))
            {
                g->add_msg(_("The cold wakes you up."));
                dis.duration = 1;
            }
        }
        else if (p.temp_cur[i] > BODYTEMP_VERY_HOT + p.fatigue/2)
        {
            if (one_in(5000)) { g->add_msg(_("You toss and turn in the heat.")); }
            if (p.temp_cur[i] > BODYTEMP_SCORCHING + p.fatigue/2 || (one_in(15000 - p.temp_cur[i])))
            {
                g->add_msg(_("The heat wakes you up."));
                dis.duration = 1;
            }
        }
    }
  }

    break;

 case DI_STEMCELL_TREATMENT:
  //slightly repair broken limbs. (also nonbroken limbs (unless they're too healthy))
  for (int i = 0; i < num_hp_parts; i++) {
   if(one_in(6)){
    if (p.hp_cur[i] < rng(0,40)){
     g->add_msg(_("You feel your skeleton melt and mend."));
     p.hp_cur[i]+= rng(1,8);
    }
    else if (p.hp_cur[i] > rng(10,2000)){
     g->add_msg(_("You feel your skeleton melt"));
     p.hp_cur[i] -= rng(0,8);
    }
   }
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
      (p.has_trait("WEAKSTOMACH") || p.has_trait("NAUSEA") || one_in(20)))
   p.vomit(g);
  if (!p.has_disease("sleep") && dis.duration >= 4500 &&
      one_in(500 - int(dis.duration / 80))) {
   g->add_msg_if_player(&p,_("You pass out."));
   p.add_disease("sleep", dis.duration / 2);
  }
  break;

 case DI_CIG:
  if (dis.duration >= 600) {	// Smoked too much
   p.str_cur--;
   p.dex_cur--;
   if (dis.duration >= 1200 && (one_in(50) ||
                                (p.has_trait("WEAKSTOMACH") && one_in(30)) ||
                                (p.has_trait("NAUSEA") && one_in(20))))
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
  if ((!p.has_trait("POISRESIST") && one_in(150)) ||
      ( p.has_trait("POISRESIST") && one_in(900))   ) {
   g->add_msg_if_player(&p,_("You're suddenly wracked with pain!"));
   p.pain++;
   p.hurt(g, bp_torso, 0, rng(0, 2) * rng(0, 1));
  }
  p.per_cur--;
  p.dex_cur--;
  if (!p.has_trait("POISRESIST"))
   p.str_cur -= 2;
  break;

 case DI_BLEED:
  if (one_in(6)) {
      g->add_msg_player_or_npc( &p, _("You lose some blood."), _("<npcname> loses some blood.") );

      p.pain++;
      p.hurt(g, bp_torso, 0, 1);
      p.per_cur--;
      p.str_cur --;
      g->m.add_field(g, p.posx, p.posy, fd_blood, 1);
  }
  break;

 case DI_BADPOISON:
  if ((!p.has_trait("POISRESIST") && one_in(100)) ||
      ( p.has_trait("POISRESIST") && one_in(500))   ) {
   g->add_msg_if_player(&p,_("You're suddenly wracked with pain!"));
   p.pain += 2;
   p.hurt(g, bp_torso, 0, rng(0, 2));
  }
  p.per_cur -= 2;
  p.dex_cur -= 2;
  if (!p.has_trait("POISRESIST"))
   p.str_cur -= 3;
  else
   p.str_cur--;
  break;

 case DI_FOODPOISON:
  bonus = 0;
  if (p.has_trait("POISRESIST"))
   bonus = 600;
  if (one_in(300 + bonus)) {
   g->add_msg_if_player(&p,_("You're suddenly wracked with pain and nausea!"));
   p.hurt(g, bp_torso, 0, 1);
  }
  if ((p.has_trait("WEAKSTOMACH") && one_in(300 + bonus)) ||
      (p.has_trait("NAUSEA") && one_in(50 + bonus)) ||
      one_in(600 + bonus))
   p.vomit(g);
  p.str_cur -= 3;
  p.dex_cur--;
  p.per_cur--;
  if (p.has_trait("POISRESIST"))
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
   p.add_disease("formication", 1200);

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
    p.rem_disease("dermatik"); // No more infection!  yay.
    g->add_msg_player_or_npc( &p,
        _("Your flesh crawls; insects tear through the flesh and begin to emerge!"),
        _("Insects begin to emerge from <npcname>'s skin!") );
    if(!p.is_npc()) {
        p.add_memorial_log(_("Dermatik eggs hatched."));
    }

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
  if (rng(50, 500) < rng(0, dis.duration))
   p.mutate(g);
  break;

 case DI_FORMICATION:
  p.int_cur -= 2;
  p.str_cur -= 1;
  if (one_in(10 + 40 * p.int_cur)) {
   if (!p.is_npc()) {
    g->add_msg(_("You start scratching yourself all over!"));
    g->cancel_activity();
   } else if (g->u_see(p.posx, p.posy)) {
    if (p.male)
     g->add_msg(_("%s starts scratching himself all over!"), p.name.c_str());
    else
     g->add_msg(_("%s starts scratching herself all over!"), p.name.c_str());
   }
   p.moves -= 150;
   p.hurt(g, bp_torso, 0, 1);
  }
  break;

 case DI_HALLU:
// This assumes that we were given DI_HALLU with a 3600 (6-hour) lifespan
  if (dis.duration > 3000) {	// First hour symptoms
   if (one_in(300)) {
     g->add_msg_if_player(&p,_("You feel a little strange."));
   }
  } else if (dis.duration > 2400) {	// Coming up
   if (one_in(100) || (p.has_trait("WEAKSTOMACH") && one_in(100))) {
    g->add_msg_if_player(&p,_("You feel nauseous."));
    p.hunger -= 5;
   }
   if (!p.is_npc()) {
    if (one_in(200))
     g->add_msg(_("Huh?  What was that?"));
    else if (one_in(200))
     g->add_msg(_("Oh god, what's happening?"));
    else if (one_in(200))
     g->add_msg(_("Of course... it's all fractals!"));
   }
  } else if (dis.duration == 2400)	// Visuals start
   p.add_disease("visuals", 2400);
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
   g->add_msg_if_player(&p,_("Your adrenaline rush wears off.  You feel AWFUL!"));
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
   g->add_msg_if_player(&p,_("Your asthma overcomes you.  You stop breathing and die..."));
   g->u.add_memorial_log(_("Succumbed to an asthma attack."));
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
// TODO: This this with regards to NPCS
  if(&p != &(g->u)) return; // NO, no teleporting around the player because an NPC has teleglow!
  if (dis.duration > 6000) {	// 20 teles (no decay; in practice at least 21)
   if (one_in(1000 - ((dis.duration - 6000) / 10))) {
    if (!p.is_npc()) {
     g->add_msg(_("Glowing lights surround you, and you teleport."));
     g->u.add_memorial_log(_("Spontaneous teleport."));
    }
    g->teleport();
    if (one_in(10))
     p.rem_disease("teleglow");
   }
   if (one_in(1200 - ((dis.duration - 6000) / 5)) && one_in(20)) {
    if (!p.is_npc())
     g->add_msg(_("You pass out."));
    p.add_disease("sleep", 1200);
    if (one_in(6))
     p.rem_disease("teleglow");
   }
  }
  if (dis.duration > 3600) { // 12 teles
   if (one_in(4000 - int(.25 * (dis.duration - 3600)))) {
    mon_id type = MonsterGroupManager::GetMonsterFromGroup("GROUP_NETHER", &g->mtypes);
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
      g->m.ter_set(x, y, t_rubble);
     beast.spawn(x, y);
     g->z.push_back(beast);
     if (g->u_see(x, y)) {
      g->cancel_activity_query(_("A monster appears nearby!"));
      g->add_msg(_("A portal opens nearby, and a monster crawls through!"));
     }
     if (one_in(2))
      p.rem_disease("teleglow");
    }
   }
   if (one_in(3500 - int(.25 * (dis.duration - 3600)))) {
    g->add_msg_if_player(&p,_("You shudder suddenly."));
    p.mutate(g);
    if (one_in(4))
     p.rem_disease("teleglow");
   }
  }
  if (dis.duration > 2400) {	// 8 teleports
   if (one_in(10000 - dis.duration))
    p.add_disease("shakes", rng(40, 80));
   if (one_in(12000 - dis.duration)) {
    g->add_msg_if_player(&p,_("Your vision is filled with bright lights..."));
    p.add_disease("blind", rng(10, 20));
    if (one_in(8))
     p.rem_disease("teleglow");
   }
   if (one_in(5000) && !p.has_disease("hallu")) {
    p.add_disease("hallu", 3600);
    if (one_in(5))
     p.rem_disease("teleglow");
   }
  }
  if (one_in(4000)) {
   g->add_msg_if_player(&p,_("You're suddenly covered in ectoplasm."));
   p.add_disease("boomered", 100);
   if (one_in(4))
    p.rem_disease("teleglow");
  }
  if (one_in(10000)) {
   p.add_disease("fungus", -1);
   p.rem_disease("teleglow");
  }
  break;

 case DI_ATTENTION:
  if (one_in( 100000 / dis.duration ) && one_in( 100000 / dis.duration ) &&
      one_in(250)) {
   mon_id type = MonsterGroupManager::GetMonsterFromGroup("GROUP_NETHER", &g->mtypes);
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
     g->m.ter_set(x, y, t_rubble);
    beast.spawn(x, y);
    g->z.push_back(beast);
    if (g->u_see(x, y)) {
     g->cancel_activity_query(_("A monster appears nearby!"));
     g->add_msg(_("A portal opens nearby, and a monster crawls through!"));
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
    for (int j = 0; j < armor->effects_worn.size(); j++) {
     if (armor->effects_worn[j] == AEP_EVIL)
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

 case DI_BITE:
//3600 (6-hour) lifespan
  if (dis.duration > 2400) {	// First symptoms for 2 hours
   if (one_in(300)) {
     g->add_msg_if_player(&p,_("Your bite wound really hurts."));
   }
  } else if (dis.duration > 1200) {	//Pain at 4 hours in
   if (one_in(100)) {
     if (p.has_disease("sleep")) {
      p.rem_disease("sleep");
      g->add_msg_if_player(&p,_("You wake up."));
     }
     g->add_msg_if_player(&p,_("Your bite wound feels swollen and painful."));
    if(p.pain < 20)
	    p.pain++;
   }

   p.dex_cur-= 1;
  } else {	// Infection starts
   p.rem_disease("bite");
   p.add_disease("infected", 14400); // 1 day of timer
  }
  break;

 case DI_LIGHTSNARE:
  p.moves = -500;
  if(one_in(10)) {
    g->add_msg(_("You attempt to free yourself from the snare."));
  }
  break;

 case DI_HEAVYSNARE:
  p.moves = -500;
  if(one_in(20)) {
    g->add_msg(_("You attempt to free yourself from the snare."));
  }
  break;

 case DI_INFECTED:
	 p.dex_cur-= 1;
  if (dis.duration > 10800) {	// Infection Symptoms 6 hours into infection
   if (one_in(300)) {
     if (p.has_disease("sleep")) {
      p.rem_disease("sleep");
      g->add_msg_if_player(&p,_("You wake up."));
     }
     g->add_msg_if_player(&p,"Your infected wound is incredibly painful.");
	   if(p.pain < 40)
	    p.pain++;
   }
   p.str_cur-= 1;
   p.dex_cur-= 1;

  } else if (dis.duration > 7200) {	//Infection Symptoms 12 hours into infection
   if (one_in(100)) {
     if (p.has_disease("sleep")) {
      p.rem_disease("sleep");
      g->add_msg_if_player(&p,_("You wake up."));
     }
     g->add_msg_if_player(&p,_("You feel feverish and nauseous, your wound has begun to turn green."));
    p.vomit(g);
    if(p.pain < 60)
	    p.pain++;
   }
   p.str_cur-= 2;
   p.dex_cur-= 2;

  } else if (dis.duration > 3600) {	//Infection Symptoms 18 hours into infection
   if (one_in(100)) {
       if (p.has_disease("sleep")) {
           p.rem_disease("sleep");
           g->add_msg_if_player(&p,_("You wake up."));
           g->add_msg_if_player(&p,_("You feel terribly weak, standing up is nearly impossible."));
       } else {
           g->add_msg_if_player(&p,_("You can barely remain standing."));
       }
       p.vomit(g);
       if(p.pain < 100)
       {
           p.pain++;
       }
   }
   p.str_cur-= 2;
   p.dex_cur-= 2;

   if(one_in(10)) {
    if (p.has_disease("sleep"))
     p.rem_disease("sleep");
    g->add_msg(_("You pass out."));
    p.add_disease("sleep", 60);
   }
  } else {	// You die. 24 hours after infection Total time, 30 hours including bite.
   if (p.has_disease("sleep"))
    p.rem_disease("sleep");
   g->add_msg(_("You succumb to the infection."));
   g->u.add_memorial_log(_("Succumbed to the infection."));
   p.hurtall(500);
  }
  break;

  case DI_RECOVER:
   p.str_cur-= 1;
   p.dex_cur-= 1;
  break;
 }
}

int disease_speed_boost(disease dis)
{
 dis_type_enum type = disease_type_lookup[dis.type];
 switch (type) {
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
 case DI_BOULDERING: return ( 0 - (dis.intensity * 10));
 default:		return 0;
 }
}

std::string dis_name(disease dis)
{
    dis_type_enum type = disease_type_lookup[dis.type];
    switch (type) {
    case DI_NULL: return "";
    case DI_GLARE: return _("Glare");
    case DI_COLD_HEAD:
        switch (dis.intensity) {
        case 1: return _("Chilly head");
        case 2: return _("Cold head!");
        case 3: return _("Freezing head!!");}
    case DI_COLD_MOUTH:
        switch (dis.intensity) {
        case 1: return _("Chilly face");
        case 2: return _("Cold face!");
        case 3: return _("Freezing face!!");}
    case DI_COLD_TORSO:
        switch (dis.intensity) {
        case 1: return _("Chilly torso");
        case 2: return _("Cold torso!");
        case 3: return _("Freezing torso!!");}
    case DI_COLD_ARMS:
        switch (dis.intensity) {
        case 1: return _("Chilly arms");
        case 2: return _("Cold arms!");
        case 3: return _("Freezing arms!!");}
    case DI_COLD_HANDS:
        switch (dis.intensity) {
        case 1: return _("Chilly hands");
        case 2: return _("Cold hands!");
        case 3: return _("Freezing hands!!");}
    case DI_COLD_LEGS:
        switch (dis.intensity) {
        case 1: return _("Chilly legs");
        case 2: return _("Cold legs!");
        case 3: return _("Freezing legs!!");}
    case DI_COLD_FEET:
        switch (dis.intensity) {
        case 1: return _("Chilly feet");
        case 2: return _("Cold feet!");
        case 3: return _("Freezing feet!!");}
    case DI_FROSTBITE_HANDS:
        switch (dis.intensity) {
        case 1: return _("Frostnip - hands");
        case 2: return _("Frostbite - hands");}
    case DI_FROSTBITE_FEET:
        switch (dis.intensity) {
        case 1: return _("Frostnip - feet");
        case 2: return _("Frostbite - feet");}
    case DI_FROSTBITE_MOUTH:
        switch (dis.intensity) {
        case 1: return _("Frostnip - face");
        case 2: return _("Frostbite - face");}
    case DI_HOT_HEAD:
        switch (dis.intensity) {
        case 1: return _("Warm head");
        case 2: return _("Hot head!");
        case 3: return _("Scorching head!!");}
    case DI_HOT_MOUTH:
        switch (dis.intensity) {
        case 1: return _("Warm face");
        case 2: return _("Hot face!");
        case 3: return _("Scorching face!!");}
    case DI_HOT_TORSO:
        switch (dis.intensity) {
        case 1: return _("Warm torso");
        case 2: return _("Hot torso!");
        case 3: return _("Scorching torso!!");}
    case DI_HOT_ARMS:
        switch (dis.intensity) {
        case 1: return _("Warm arms");
        case 2: return _("Hot arms!");
        case 3: return _("Scorching arms!!");}
    case DI_HOT_HANDS:
        switch (dis.intensity) {
        case 1: return _("Warm hands");
        case 2: return _("Hot hands!");
        case 3: return _("Scorching hands!!");}
    case DI_HOT_LEGS:
        switch (dis.intensity) {
        case 1: return _("Warm legs");
        case 2: return _("Hot legs!");
        case 3: return _("Scorching legs!!");}
    case DI_HOT_FEET:
        switch (dis.intensity) {
        case 1: return _("Warm feet");
        case 2: return _("Hot feet!");
        case 3: return _("Scorching feet!!");}
    case DI_BLISTERS_MOUTH: return _("Blisters - face");
    case DI_BLISTERS_TORSO: return _("Blisters - torso");
    case DI_BLISTERS_ARMS: return _("Blisters - arms");
    case DI_BLISTERS_HANDS: return _("Blisters - hands");
    case DI_BLISTERS_LEGS: return _("Blisters - legs");
    case DI_BLISTERS_FEET: return _("Blisters - feet");
    case DI_COMMON_COLD: return _("Common Cold");
    case DI_FLU: return _("Influenza");
    case DI_SMOKE: return _("Smoke");
    case DI_TEARGAS: return _("Tear gas");
    case DI_ONFIRE: return _("On Fire");
    case DI_BOOMERED: return _("Boomered");
    case DI_SAP: return _("Sap-coated");
    case DI_SPORES: return _("Spores");
    case DI_SLIMED: return _("Slimed");
    case DI_DEAF: return _("Deaf");
    case DI_BLIND: return _("Blind");
    case DI_STUNNED: return _("Stunned");
    case DI_DOWNED: return _("Downed");
    case DI_POISON: return _("Poisoned");
    case DI_BLEED: return _("Bleeding");
    case DI_BADPOISON: return _("Badly Poisoned");
    case DI_FOODPOISON: return _("Food Poisoning");
    case DI_SHAKES: return _("Shakes");
    case DI_FORMICATION: return _("Bugs Under Skin");
    case DI_WEBBED: return _("Webbed");
    case DI_RAT: return _("Ratting");
    case DI_DRUNK:
        if (dis.duration > 2200) return _("Wasted");
        if (dis.duration > 1400) return _("Trashed");
        if (dis.duration > 800)  return _("Drunk");
        else return _("Tipsy");

    case DI_CIG: return _("Cigarette");
    case DI_HIGH: return _("High");
    case DI_VISUALS: return _("Hallucinating");

    case DI_ADRENALINE:
        if (dis.duration > 150) return _("Adrenaline Rush");
        else return _("Adrenaline Comedown");

    case DI_ASTHMA:
        if (dis.duration > 800) return _("Heavy Asthma");
        else return _("Asthma");

    case DI_GRACK: return _("RELEASE THE GRACKEN!!!!");

    case DI_METH:
        if (dis.duration > 600) return _("High on Meth");
        else return _("Meth Comedown");

    case DI_IN_PIT: return _("Stuck in Pit");
    case DI_BOULDERING: return _("Clambering Over Rubble");

    case DI_STEMCELL_TREATMENT: return _("Stem cell treatment");
    case DI_ATTACK_BOOST: return _("Hit Bonus");
    case DI_DAMAGE_BOOST: return _("Damage Bonus");
    case DI_DODGE_BOOST: return _("Dodge Bonus");
    case DI_ARMOR_BOOST: return _("Armor Bonus");
    case DI_SPEED_BOOST: return _("Attack Speed Bonus");
    case DI_VIPER_COMBO:
        switch (dis.intensity) {
        case 1: return _("Snakebite Unlocked!");
        case 2: return _("Viper Strike Unlocked!");
        default: return "Viper combo bug. (in disease.cpp:dis_name)";}
    case DI_BITE:
        if (dis.duration > 3000) return _("Bite Wound");
        else return _("Painful Bite Wound");
    case DI_INFECTED:
        if (dis.duration > 10800) return _("Infected Wound");
        if (dis.duration > 7200) return _("Painful Infected Wound");
        else return _("Pus Filled Wound");
    case DI_RECOVER: return _("Recovering From Infection");

    default: return "";
    }
}

std::string dis_description(disease dis)
{
    int strpen, dexpen, intpen, perpen;
    std::stringstream stream;
    dis_type_enum type = disease_type_lookup[dis.type];
    switch (type) {

    case DI_NULL:
        return _("None");

    case DI_GLARE:
        stream << _("Perception - 1");
        return stream.str();

    case DI_COLD_HEAD:
        switch (dis.intensity) {
        case 1: return _("Your head is exposed to the cold.");
        case 2: return _("Your head is very exposed to the cold. It is hard to concentrate.");
        case 3: return _("Your head is dangerously cold. Getting undressed sounds like a good idea.");
        }

    case DI_COLD_MOUTH:
        switch (dis.intensity) {
        case 1: return _("Your face is exposed to the cold.");
        case 2: return _("Your face is very exposed to the cold.");
        case 3: return _("Your face is dangerously cold.");
        }

    case DI_COLD_TORSO:
        switch (dis.intensity) {
        case 1: return _("Your torso is exposed to the cold.");
        case 2: return _("Your torso is very exposed to the cold. Your actions are incoordinated.");
        case 3: return _("Your torso is dangerously cold. Your actions are incredibly incoordinated.");
        }

    case DI_COLD_ARMS:
        switch (dis.intensity) {
        case 1: return _("Your arms are exposed to the cold.");
        case 2: return _("Your arms are very exposed to the cold. Your arms are shivering.");
        case 3: return _("Your arms are dangerously cold. Your arms are shivering uncontrollably");
        }

    case DI_COLD_HANDS:
        switch (dis.intensity) {
        case 1: return _("Your hands are exposed to the cold.");
        case 2: return _("Your hands are very exposed to the cold. Your hands are shivering.");
        case 3: return _("Your hands are dangerously cold. Your hands are shivering uncontrollably");
        }

    case DI_COLD_LEGS:
        switch (dis.intensity) {
        case 1: return _("Your legs are exposed to the cold.");
        case 2: return _("Your legs are very exposed to the cold. Your strength is sapped.");
        case 3: return _("Your legs are dangerously cold. Your strength is sapped.");
        }

    case DI_COLD_FEET:
        switch (dis.intensity) {
        case 1: return _("Your feet are exposed to the cold.");
        case 2: return _("Your feet are very exposed to the cold. Your strength is sapped.");
        case 3: return _("Your feet are dangerously cold. Your strength is sapped.");
        }

    case DI_FROSTBITE_HANDS:
        switch (dis.intensity) {
        case 1: return _("\
Your hands are frostnipped from the prolonged exposure to the cold and have gone numb. When the blood begins to flow, it will be painful.");
        case 2: return _("\
Your hands are frostbitten from the prolonged exposure to the cold. The tissues in your hands are frozen.");
        }

    case DI_FROSTBITE_FEET:
        switch (dis.intensity) {
        case 1: return _("\
Your feet are frostnipped from the prolonged exposure to the cold and have gone numb. When the blood begins to flow, it will be painful.");
        case 2: return _("\
Your feet are frostbitten from the prolonged exposure to the cold. The tissues in your feet are frozen.");
        }

    case DI_FROSTBITE_MOUTH:
        switch (dis.intensity) {
        case 1: return _("\
Your face is frostnipped from the prolonged exposure to the cold and has gone numb. When the blood begins to flow, it will be painful.");
        case 2: return _("\
Your face is frostbitten from the prolonged exposure to the cold. The tissues in your face are frozen.");
        }

    case DI_FROSTBITE_TORSO: return _("\
Your torso is frostbitten from prolonged exposure to the cold. It is extremely painful.");
    case DI_FROSTBITE_ARMS: return _("\
Your arms are frostbitten from prolonged exposure to the cold. It is extremely painful.");
    case DI_FROSTBITE_LEGS: return _("\
Your legs are frostbitten from prolonged exposure to the cold. It is extremely painful.");

    case DI_HOT_HEAD:
        switch (dis.intensity) {
        case 1: return _("Your head feels warm.");
        case 2: return _("Your head is sweating from the heat. You feel nauseated. You have a headache.");
        case 3: return _("Your head is sweating profusely. You feel very nauseated. You have a headache.");
        }

    case DI_HOT_MOUTH:
        switch (dis.intensity) {
        case 1: return _("Your face feels warm.");
        case 2: return _("Your face is sweating from the heat, making it hard to see.");
        case 3: return _("Your face is sweating profusely, making it hard to see.");
        }

    case DI_HOT_TORSO:
        switch (dis.intensity) {
        case 1: return _("Your torso feels warm.");
        case 2: return _("Your torso is sweating from the heat. You feel weak.");
        case 3: return _("Your torso is sweating profusely. You feel very weak.");
        }

    case DI_HOT_ARMS:
        switch (dis.intensity) {
        case 1: return _("Your arms feel warm.");
        case 2: return _("Your arms are sweating from the heat.");
        case 3: return _("Your arms are sweating profusely. Your muscles are in pain due to cramps.");
        }

    case DI_HOT_HANDS:
        switch (dis.intensity) {
        case 1: return _("Your hands feel warm.");
        case 2: return _("Your hands feel hot and incoordinated.");
        case 3: return _("Your hands feel disgustinly hot and are very incoordinated.");
        }

    case DI_HOT_LEGS:
        switch (dis.intensity) {
        case 1: return _("Your legs feel warm.");
        case 2: return _("Your legs are sweating from the heat.");
        case 3: return _("Your legs are sweating profusely. Your muscles are in pain due to cramps.");
        }

    case DI_HOT_FEET:
        switch (dis.intensity) {
        case 1: return _("Your feet feel warm.");
        case 2: return _("Your feet are painfully swollen due to the heat.");
        case 3: return _("Your feet are painfully swollen due to the heat.");
        }

    case DI_BLISTERS_MOUTH: return _("\
Your face is blistering from the intense heat. It is extremely painful.");
    case DI_BLISTERS_TORSO: return _("\
Your torso is blistering from the intense heat. It is extremely painful.");
    case DI_BLISTERS_ARMS: return _("\
Your arms are blistering from the intense heat. It is extremely painful.");
    case DI_BLISTERS_HANDS: return _("\
Your hands are blistering from the intense heat. It is extremely painful.");
    case DI_BLISTERS_LEGS: return _("\
Your legs are blistering from the intense heat. It is extremely painful.");
    case DI_BLISTERS_FEET: return _("\
Your feet are blistering from the intense heat. It is extremely painful.");

    case DI_COMMON_COLD:
        return _(
        "Increased thirst;   Frequent coughing\n"
        "Strength - 3;   Dexterity - 1;   Intelligence - 2;   Perception - 1\n"
        "Symptoms alleviated by medication (Dayquil or Nyquil).");

    case DI_FLU:
        return _(
        "Increased thirst;   Frequent coughing;   Occasional vomiting\n"
        "Strength - 4;   Dexterity - 2;   Intelligence - 2;   Perception - 1\n"
        "Symptoms alleviated by medication (Dayquil or Nyquil).");

    case DI_SMOKE:
        return _(
        "Strength - 1;   Dexterity - 1;\n"
        "Occasionally you will cough, costing movement and creating noise.\n"
        "Loss of health - Torso");

    case DI_TEARGAS:
        return _(
        "Strength - 2;   Dexterity - 2;   Intelligence - 1;   Perception - 4\n"
        "Occasionally you will cough, costing movement and creating noise.\n"
        "Loss of health - Torso");

    case DI_ONFIRE:
        return _(
        "Loss of health - Entire Body\n"
        "Your clothing and other equipment may be consumed by the flames.");

    case DI_CRUSHED: return "If you're seeing this, there is a bug in disease.cpp!";

    case DI_BOULDERING:
        switch (dis.intensity){
        case 1:
            stream << _(
            "Dexterity - 1;   Speed -10%\n"
            "You are being slowed by climbing over a pile of rubble.");
        case 2:
            stream << _(
            "Dexterity - 3;   Speed -20%\n"
            "You are being slowed by climbing over a heap of rubble.");
        case 3:
            stream << _(
            "Dexterity - 5;   Speed -30%\n"
            "You are being slowed by climbing over a mountain of rubble.");
        }
        return stream.str();

    case DI_STEMCELL_TREATMENT: return _("Your insides are shifting in strange ways as the treatment takes effect.");

    case DI_BOOMERED:
        return _(
        "Perception - 5\n"
        "Range of Sight: 1;   All sight is tinted magenta.");

    case DI_SAP:
        return _("Dexterity - 3;   Speed - 25");

    case DI_SPORES:
        return _(
        "Speed -40%\n"
        "You can feel the tiny spores sinking directly into your flesh.");

    case DI_SLIMED:
        return _("Speed -40%;   Dexterity - 2");

    case DI_DEAF: return _("Sounds will not be reported.  You cannot talk with NPCs.");

    case DI_BLIND:
        return _("Range of Sight: 0");

    case DI_STUNNED: return _("Your movement is randomized.");

    case DI_DOWNED: return _("You're knocked to the ground.  You have to get up before you can move.");

    case DI_POISON:
        return _(
        "Perception - 1;   Dexterity - 1;   Strength - 2 IF not resistant\n"
        "Occasional pain and/or damage.");

    case DI_BLEED: return _("You are slowly losing blood.");

    case DI_BADPOISON:
        return _(
        "Perception - 2;   Dexterity - 2;\n"
        "Strength - 3 IF not resistant, -1 otherwise\n"
        "Frequent pain and/or damage.");

    case DI_FOODPOISON:
        return _(
        "Speed - 35%;   Strength - 3;   Dexterity - 1;   Perception - 1\n"
        "Your stomach is extremely upset, and you keep having pangs of pain and nausea.");

    case DI_SHAKES:
        return _("Strength - 1;   Dexterity - 4");

    case DI_FORMICATION:
        return _(
        "Strength - 1;   Intelligence - 2\n"
        "You stop to scratch yourself frequently; high intelligence helps you resist\n"
        "this urge.");

    case DI_WEBBED:
        return _(
        "Strength - 1;   Dexterity - 4;   Speed - 25");

    case DI_RAT:
        {
        intpen = int(dis.duration / 20);
        perpen = int(dis.duration / 25);
        strpen = int(dis.duration / 50);
        stream << _("You feal nauseated and rat-like.\n");
        if (intpen > 0) {
            stream << string_format(_("Intelligence - %d;   "), intpen);
        }
        if (perpen > 0) {
            stream << string_format(_("Perception - %d;   "), perpen);
        }
        if (strpen > 0) {
            stream << string_format(_("Strength - %d;   "), strpen);
        }
        return stream.str();
        }

    case DI_DRUNK:
        {
        perpen = int(dis.duration / 1000);
        dexpen = int(dis.duration / 1000);
        intpen = int(dis.duration /  700);
        strpen = int(dis.duration / 1500);
        if (strpen > 0) {
            stream << string_format(_("Strength - %d;   "), strpen);
        }
        else if (dis.duration <= 600)
            stream << _("Strength + 1;    ");
        if (dexpen > 0) {
            stream << string_format(_("Dexterity - %d;   "), dexpen);
        }
        if (intpen > 0) {
            stream << string_format(_("Intelligence - %d;   "), intpen);
        }
        if (perpen > 0) {
            stream << string_format(_("Perception - %d;   "), perpen);
        }
        return stream.str();
        }

    case DI_CIG:
        if (dis.duration >= 600)
            return _(
            "Strength - 1;   Dexterity - 1\n"
            "You smoked too much.");
        else
            return _(
            "Dexterity + 1;   Intelligence + 1;   Perception + 1");

    case DI_HIGH:
        return _("Intelligence - 1;   Perception - 1");

    case DI_VISUALS: return _("You can't trust everything that you see.");

    case DI_ADRENALINE:
        if (dis.duration > 150)
            return _(
            "Speed +80;   Strength + 5;   Dexterity + 3;\n"
            "Intelligence - 8;   Perception + 1");
        else
            return _(
            "Strength - 2;   Dexterity - 1;   Intelligence - 1;   Perception - 1");

    case DI_ASTHMA:
        return string_format(_("Speed - %d%%;   Strength - 2;   Dexterity - 3"), int(dis.duration / 5));

    case DI_GRACK: return _("Unleashed the Gracken.");

    case DI_METH:
        if (dis.duration > 600)
            return _(
            "Speed +50;   Strength + 2;   Dexterity + 2;\n"
            "Intelligence + 3;   Perception + 3");
        else
            return _(
            "Speed -40;   Strength - 3;   Dexterity - 2;   Intelligence - 2");

    case DI_IN_PIT: return _("You're stuck in a pit.  Sight distance is limited and you have to climb out.");

    case DI_ATTACK_BOOST:
        return string_format(_("To-hit bonus + %d"), dis.intensity);

    case DI_DAMAGE_BOOST:
        return string_format(_("Damage bonus + %d"), dis.intensity);

    case DI_DODGE_BOOST:
        return string_format(_("Dodge bonus + %d"), dis.intensity);

    case DI_ARMOR_BOOST:
        return string_format(_("Armor bonus + %d"), dis.intensity);

    case DI_SPEED_BOOST:
        return string_format(_("Attack speed + %d"), dis.intensity);

    case DI_VIPER_COMBO:
        switch (dis.intensity) {
        case 1: return _("\
Your next strike will be a Snakebite, using your hand in a cone shape.  This\n\
will deal piercing damage.");
        case 2: return _("\
Your next strike will be a Viper Strike.  It requires both arms to be in good\n\
condition, and deals massive damage.");
        }

    case DI_BITE: return _("You have a nasty bite wound.");
    case DI_INFECTED: return _("You have an infected wound.");
    case DI_RECOVER: return _("You are recovering from an infection.");

    default: return "Who knows?  This is probably a bug. (disease.cpp:dis_description)";
    }
}

