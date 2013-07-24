#include "addiction.h"

void addict_effect(game *g, addiction &add)
{
 int in = add.intensity;

 switch (add.type) {
 case ADD_CIG:
  if (in > 20 || one_in((500 - 20 * in))) {
   g->add_msg(rng(0, 6) < in ? _("You need a cigarette.") : _("You could use a cigarette."));
   g->cancel_activity_query(_("You have a nicotine craving."));
   g->u.add_morale(MORALE_CRAVING_NICOTINE, -15, -50);
   if (one_in(800 - 50 * in))
    g->u.fatigue++;
   if (one_in(400 - 20 * in))
    g->u.stim--;
  }
  break;

 case ADD_CAFFEINE:
  g->u.moves -= 2;
  if (in > 20 || one_in((500 - 20 * in))) {
   g->add_msg(_("You want some caffeine."));
   g->cancel_activity_query(_("You have a caffeine craving."));
   g->u.add_morale(MORALE_CRAVING_CAFFEINE, -5, -30);
   if (rng(0, 10) < in)
    g->u.stim--;
   if (rng(8, 400) < in) {
    g->add_msg(_("Your hands start shaking... you need it bad!"));
    g->u.add_disease("shakes", 20);
   }
  }
  break;

 case ADD_ALCOHOL:
  g->u.per_cur--;
  g->u.int_cur--;
  if (rng(40, 1200) <= in * 10)
   g->u.health--;
  if (one_in(20) && rng(0, 20) < in) {
   g->add_msg(_("You could use a drink."));
   g->cancel_activity_query(_("You have an alcohol craving."));
   g->u.add_morale(MORALE_CRAVING_ALCOHOL, -35, -120);
  } else if (rng(8, 300) < in) {
   g->add_msg(_("Your hands start shaking... you need a drink bad!"));
   g->cancel_activity_query(_("You have an alcohol craving."));
   g->u.add_morale(MORALE_CRAVING_ALCOHOL, -35, -120);
   g->u.add_disease("shakes", 50);
  } else if (!g->u.has_disease("hallu") && rng(10, 1600) < in)
   g->u.add_disease("hallu", 3600);
  break;

 case ADD_SLEEP:
// No effects here--just in player::can_sleep()
// EXCEPT!  Prolong this addiction longer than usual.
  if (one_in(2) && add.sated < 0)
   add.sated++;
  break;

 case ADD_PKILLER:
  if ((in >= 25 || int(g->turn) % (100 - in * 4) == 0) && g->u.pkill > 0)
   g->u.pkill--;	// Tolerance increases!
  if (g->u.pkill >= 35) // No further effects if we're doped up.
   add.sated = 0;
  else {
   g->u.str_cur -= 1 + int(in / 7);
   g->u.per_cur--;
   g->u.dex_cur--;
   if (g->u.pain < in * 3)
    g->u.pain++;
   if (in >= 40 || one_in((1200 - 30 * in)))
    g->u.health--;
   if (one_in(20) && dice(2, 20) < in) {
    g->add_msg(_("Your hands start shaking... you need some painkillers."));
    g->cancel_activity_query(_("You have an opiate craving."));
    g->u.add_morale(MORALE_CRAVING_OPIATE, -40, -200);
    g->u.add_disease("shakes", 20 + in * 5);
   } else if (one_in(20) && dice(2, 30) < in) {
    g->add_msg(_("You feel anxious.  You need your painkillers!"));
    g->u.add_morale(MORALE_CRAVING_OPIATE, -30, -200);
    g->cancel_activity_query(_("You have a craving."));
   } else if (one_in(50) && dice(3, 50) < in) {
    g->add_msg(_("You throw up heavily!"));
    g->cancel_activity_query(_("Throwing up."));
    g->u.vomit(g);
   }
  }
  break;

 case ADD_SPEED: {
  int move_pen = in * 5;
  if (move_pen > 30)
   move_pen = 30;
  g->u.moves -= move_pen;
  g->u.int_cur--;
  g->u.str_cur--;
  if (g->u.stim > -100 && (in >= 20 || int(g->turn) % (100 - in * 5) == 0))
   g->u.stim--;
  if (rng(0, 150) <= in)
   g->u.health--;
  if (dice(2, 100) < in) {
   g->add_msg(_("You feel depressed.  Speed would help."));
   g->cancel_activity_query(_("You have a speed craving."));
   g->u.add_morale(MORALE_CRAVING_SPEED, -25, -200);
  } else if (one_in(10) && dice(2, 80) < in) {
   g->add_msg(_("Your hands start shaking... you need a pick-me-up."));
   g->cancel_activity_query(_("You have a speed craving."));
   g->u.add_morale(MORALE_CRAVING_SPEED, -25, -200);
   g->u.add_disease("shakes", in * 20);
  } else if (one_in(50) && dice(2, 100) < in) {
   g->add_msg(_("You stop suddenly, feeling bewildered."));
   g->cancel_activity();
   g->u.moves -= 300;
  } else if (!g->u.has_disease("hallu") && one_in(20) &&
             8 + dice(2, 80) < in)
   g->u.add_disease("hallu", 3600);
 } break;

 case ADD_COKE:
  g->u.int_cur--;
  g->u.per_cur--;
  if (in >= 30 || one_in((900 - 30 * in))) {
   g->add_msg(_("You feel like you need a bump."));
   g->cancel_activity_query(_("You have a craving for cocaine."));
   g->u.add_morale(MORALE_CRAVING_COCAINE, -20, -250);
  }
  if (dice(2, 80) <= in) {
   g->add_msg(_("You feel like you need a bump."));
   g->cancel_activity_query(_("You have a craving for cocaine."));
   g->u.add_morale(MORALE_CRAVING_COCAINE, -20, -250);
   g->u.stim -= 3;
  }
  break;
 case ADD_CRACK:
  g->u.int_cur--;
  g->u.per_cur--;
  if (in >= 30 || one_in((900 - 30 * in))) {
   g->add_msg(_("You're shivering, you need some crack."));
   g->cancel_activity_query(_("You have a craving for crack."));
   g->u.add_morale(MORALE_CRAVING_CRACK, -80, -250);
  }
  if (dice(2, 80) <= in) {
   g->add_msg(_("You're shivering, you need some crack."));
   g->cancel_activity_query(_("You have a craving for crack."));
   g->u.add_morale(MORALE_CRAVING_CRACK, -80, -250);
   g->u.stim -= 3;
  }
  break;
 }
}

std::string addiction_name(addiction cur)
{
 switch (cur.type) {
  case ADD_CIG:		return _("Nicotine Withdrawal");
  case ADD_CAFFEINE:	return _("Caffeine Withdrawal");
  case ADD_ALCOHOL:	return _("Alcohol Withdrawal");
  case ADD_SLEEP:	return _("Sleeping Pill Dependance");
  case ADD_PKILLER:	return _("Opiate Withdrawal");
  case ADD_SPEED:	return _("Amphetamine Withdrawal");
  case ADD_COKE:	return _("Cocaine Withdrawal");
  case ADD_CRACK:       return _("Crack Cocaine Withdrawal");
  default:		return "Erroneous addiction";
 }
}

morale_type addiction_craving(add_type cur)
{
 switch (cur) {
  case ADD_CIG: return MORALE_CRAVING_NICOTINE;
  case ADD_CAFFEINE:    return MORALE_CRAVING_CAFFEINE;
  case ADD_ALCOHOL: return MORALE_CRAVING_ALCOHOL;
  case ADD_PKILLER: return MORALE_CRAVING_OPIATE;
  case ADD_SPEED:   return MORALE_CRAVING_SPEED;
  case ADD_COKE:    return MORALE_CRAVING_COCAINE;
  case ADD_CRACK:   return MORALE_CRAVING_CRACK;
  default:  return MORALE_NULL;
 }
}

add_type addiction_type(std::string name)
{
    if (name == "nicotine")
	return ADD_CIG;
    else if (name == "caffeine")
	return ADD_CAFFEINE;
    else if (name == "alcohol")
	return ADD_ALCOHOL;
    else if (name == "sleeping pill")
	return ADD_SLEEP;
    else if (name == "opiate")
	return ADD_PKILLER;
    else if (name == "amphetamine")
	return ADD_SPEED;
    else if (name == "cocaine")
	return ADD_COKE;
    else if (name == "crack")
	return ADD_CRACK;
    else
	return ADD_NULL;
}

std::string addiction_text(addiction cur)
{
 int strpen = 1 + int(cur.intensity / 7);
 switch (cur.type) {
 case ADD_CIG:
  return _("Intelligence - 1;   Occasional cravings");

 case ADD_CAFFEINE:
  return _("Strength - 1;   Slight sluggishness;   Occasional cravings");

 case ADD_ALCOHOL:
  return _("\
Perception - 1;   Intelligence - 1;   Occasional Cravings;\n\
Risk of delirium tremens");

 case ADD_SLEEP:
  return _("You may find it difficult to sleep without medication.");

 case ADD_PKILLER:
  {
  std::string dump;
  char* buf = new char[1000];
  sprintf(buf, _(
"Strength - %d;   Perception - 1;   Dexterity - 1;\n"
"Depression and physical pain to some degree.  Frequent cravings.  Vomiting."), strpen);
  dump = buf;
  delete buf; buf=NULL;
  return dump;
  }

 case ADD_SPEED:
  return _("Strength - 1;   Intelligence - 1;\n\
Movement rate reduction.  Depression.  Weak immune system.  Frequent cravings.");

 case ADD_COKE:
  return _("Perception - 1;   Intelligence - 1;  Frequent cravings.");

 case ADD_CRACK:
  return _("Perception - 2;   Intelligence - 2;  Frequent cravings.");
 default:
  return "";
 }
}
