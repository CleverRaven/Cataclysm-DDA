#include "faction.h"
#include "facdata.h"
#include "rng.h"
#include "math.h"
#include "output.h"
#include "omdata.h"
#include "game.h"

#include <sstream>

std::string invent_name();
std::string invent_adj();

faction::faction()
{
// debugmsg("Warning: Faction created without UID!");
 name = "";
 values = 0;
 likes_u = 0;
 respects_u = 0;
 known_by_u = false;
 goal = FACGOAL_NULL;
 job1 = FACJOB_NULL;
 job2 = FACJOB_NULL;
 strength = 0;
 sneak = 0;
 crime = 0;
 cult = 0;
 good = 0;
 omx = 0;
 omy = 0;
 mapx = 0;
 mapy = 0;
 size = 0;
 power = 0;
 id = -1;
}

faction::faction(int uid)
{
 name = "";
 values = 0;
 likes_u = 0;
 respects_u = 0;
 known_by_u = false;
 goal = FACGOAL_NULL;
 job1 = FACJOB_NULL;
 job2 = FACJOB_NULL;
 strength = 0;
 sneak = 0;
 crime = 0;
 cult = 0;
 good = 0;
 omx = 0;
 omy = 0;
 mapx = 0;
 mapy = 0;
 size = 0;
 power = 0;
 uid = id;
}


faction::~faction()
{
}

std::string faction::save_info()
{
 std::stringstream dump;
 dump << id << " " << values << " " << goal << " " << job1 << " " << job2 <<
         " " << likes_u << " " << respects_u << " " << known_by_u << " " <<
         strength << " " << sneak << " " << crime << " " << cult << " " <<
         good << " " << omx << " " << omy << " " << mapx << " " << mapy <<
         " " << size << " " << power << " ";
 dump << opinion_of.size() << " ";
 for (int i = 0; i < opinion_of.size(); i++)
  dump << opinion_of[i] << " ";
 dump << name;
 return dump.str();
}

void faction::load_info(std::string data)
{
 std::stringstream dump;
 int valuetmp, goaltmp, jobtmp1, jobtmp2;
 dump << data;
 dump >> id >> valuetmp >> goaltmp >> jobtmp1 >> jobtmp2 >> likes_u >>
         respects_u >> known_by_u >> strength >> sneak >> crime >> cult >>
         good >> omx >> omy >> mapx >> mapy >> size >> power;
 values = valuetmp;
 goal = faction_goal(goaltmp);
 job1 = faction_job(jobtmp1);
 job2 = faction_job(jobtmp2);
 int size, tmpop;
 dump >> size;
 for (int i = 0; i < size; i++) {
  dump >> tmpop;
  opinion_of.push_back(tmpop);
 }
 std::string subname;
 while (dump >> subname)
  name += " " + subname;
}

void faction::randomize()
{
// Set up values
// TODO: Not always in overmap 0,0
 omx = 0;
 omy = 0;
 mapx = rng(OMAPX / 10, OMAPX - OMAPX / 10);
 mapy = rng(OMAPY / 10, OMAPY - OMAPY / 10);
// Pick an overall goal.
 goal = faction_goal(rng(1, NUM_FACGOALS - 1));
 if (one_in(4))
  goal = FACGOAL_NONE;	// Slightly more likely to not have a real goal
 good     = facgoal_data[goal].good;
 strength = facgoal_data[goal].strength;
 sneak    = facgoal_data[goal].sneak;
 crime    = facgoal_data[goal].crime;
 cult     = facgoal_data[goal].cult;
 job1 = faction_job(rng(1, NUM_FACJOBS - 1));
 do
  job2 = faction_job(rng(0, NUM_FACJOBS - 1));
 while (job2 == job1);
 good     += facjob_data[job1].good     + facjob_data[job2].good;
 strength += facjob_data[job1].strength + facjob_data[job2].strength;
 sneak    += facjob_data[job1].sneak    + facjob_data[job2].sneak;
 crime    += facjob_data[job1].crime    + facjob_data[job2].crime;
 cult     += facjob_data[job1].cult     + facjob_data[job2].cult;

 int num_values = 0;
 int tries = 0;
 values = 0;
 do {
  int v = rng(1, NUM_FACVALS - 1);
  if (!has_value(faction_value(v)) && matches_us(faction_value(v))) {
   values |= mfb(v);
   tries = 0;
   num_values++;
   good     += facval_data[v].good;
   strength += facval_data[v].strength;
   sneak    += facval_data[v].sneak;
   crime    += facval_data[v].crime;
   cult     += facval_data[v].cult;
  } else
   tries++;
 } while((one_in(num_values) || one_in(num_values)) && tries < 15);

 std::string noun;
 int sel = 1, best = strength;
 if (sneak > best) {
  sel = 2;
  best = sneak;
 }
 if (crime > best) {
  sel = 3;
  best = crime;
 }
 if (cult > best)
  sel = 4;
 if (strength <= 0 && sneak <= 0 && crime <= 0 && cult <= 0)
  sel = 0;

 switch (sel) {
 case 1:
  noun  = faction_noun_strong[rng(0, 14)];
  power = dice(5, 20);
  size  = dice(5, 6);
  break;
 case 2:
  noun  = faction_noun_sneak [rng(0, 14)];
  power = dice(5, 8);
  size  = dice(5, 8);
  break;
 case 3:
  noun  = faction_noun_crime [rng(0, 14)];
  power = dice(5, 16);
  size  = dice(5, 8);
  break;
 case 4:
  noun  = faction_noun_cult  [rng(0, 14)];
  power = dice(8, 8);
  size  = dice(4, 6);
  break;
 default:
  noun  = faction_noun_none  [rng(0, 14)];
  power = dice(6, 8);
  size  = dice(6, 6);
 }

 if (one_in(4)) {
  do
   name = "The " + noun + " of " + invent_name();
  while (name.length() > MAX_FAC_NAME_SIZE);
 }
 else if (one_in(2)) {
  do
   name = "The " + invent_adj() + " " + noun;
  while (name.length() > MAX_FAC_NAME_SIZE);
 }
 else {
  do {
   std::string adj;
   if (good >= 3)
    adj = faction_adj_pos[rng(0, 14)];
   else if  (good <= -3)
    adj = faction_adj_bad[rng(0, 14)];
   else
    adj = faction_adj_neu[rng(0, 14)];
   name = "The " + adj + " " + noun;
   if (one_in(4))
    name += " of " + invent_name();
  } while (name.length() > MAX_FAC_NAME_SIZE);
 }
}

void faction::make_army()
{
 name = "The army";
 omx = 0;
 omy = 0;
 mapx = OMAPX / 2;
 mapy = OMAPY / 2;
 size = OMAPX * 2;
 power = OMAPX;
 goal = FACGOAL_DOMINANCE;
 job1 = FACJOB_MERCENARIES;
 job2 = FACJOB_NULL;
 if (one_in(4))
  values |= mfb(FACVAL_CHARITABLE);
 if (!one_in(4))
  values |= mfb(FACVAL_EXPLORATION);
 if (one_in(3))
  values |= mfb(FACVAL_BIONICS);
 if (one_in(3))
  values |= mfb(FACVAL_ROBOTS);
 if (one_in(4))
  values |= mfb(FACVAL_TREACHERY);
 if (one_in(4))
  values |= mfb(FACVAL_STRAIGHTEDGE);
 if (!one_in(3))
  values |= mfb(FACVAL_LAWFUL);
 if (one_in(8))
  values |= mfb(FACVAL_CRUELTY);
 id = 0;
}

bool faction::has_job(faction_job j)
{
 return (job1 == j || job2 == j);
}

bool faction::has_value(faction_value v)
{
 return values & mfb(v);
}

bool faction::matches_us(faction_value v)
{
 int numvals = 2;
 if (job2 != FACJOB_NULL)
  numvals++;
 for (int i = 0; i < NUM_FACVALS; i++) {
  if (has_value(faction_value(i)))
   numvals++;
 }
 if (has_job(FACJOB_DRUGS) && v == FACVAL_STRAIGHTEDGE)	// Mutually exclusive
  return false;
 int avggood = (good / numvals + good) / 2;
 int avgstrength = (strength / numvals + strength) / 2;
 int avgsneak = (sneak / numvals + sneak / 2);
 int avgcrime = (crime / numvals + crime / 2);
 int avgcult = (cult / numvals + cult / 2);
/*
 debugmsg("AVG: GOO %d STR %d SNK %d CRM %d CLT %d\n\
       VAL: GOO %d STR %d SNK %d CRM %d CLT %d (%s)", avggood, avgstrength,
avgsneak, avgcrime, avgcult, facval_data[v].good, facval_data[v].strength,
facval_data[v].sneak, facval_data[v].crime, facval_data[v].cult,
facval_data[v].name.c_str());
*/
 if ((abs(facval_data[v].good     - avggood)  <= 3 ||
      (avggood >=  5 && facval_data[v].good >=  1) ||
      (avggood <= -5 && facval_data[v].good <=  0))  &&
     (abs(facval_data[v].strength - avgstrength)   <= 5 ||
      (avgstrength >=  5 && facval_data[v].strength >=  3) ||
      (avgstrength <= -5 && facval_data[v].strength <= -1))  &&
     (abs(facval_data[v].sneak    - avgsneak) <= 4 ||
      (avgsneak >=  5 && facval_data[v].sneak >=  1) ||
      (avgsneak <= -5 && facval_data[v].sneak <= -1))  &&
     (abs(facval_data[v].crime    - avgcrime) <= 4 ||
      (avgcrime >=  5 && facval_data[v].crime >=  0) ||
      (avgcrime <= -5 && facval_data[v].crime <= -1))  &&
     (abs(facval_data[v].cult     - avgcult)  <= 3 ||
      (avgcult >=  5 && facval_data[v].cult >=  1) ||
      (avgcult <= -5 && facval_data[v].cult <= -1)))
  return true;
 return false;
}

std::string faction::describe()
{
 std::string ret = name + " have the ultimate goal of " +
                   facgoal_data[goal].name + ". Their primary concern is " +
                   facjob_data[job1].name;
 if (job2 == FACJOB_NULL)
  ret += ".";
 else
  ret += ", but they are also involved in " + facjob_data[job2].name + ".";
 if (values != 0) {
  ret += " They are known for ";
  for (int i = 0; i < NUM_FACVALS; i++) {
   if (has_value(faction_value(i)))
    ret += facval_data[i].name + ", ";
  }
 }
 size_t pos = ret.find_last_of(",");
 if (pos != std::string::npos) {
  ret.replace(pos, 2, ".");
  pos = ret.find_last_of(",");
  if (pos != std::string::npos)
   ret.replace(pos, 2, ", and ");
 }
 return ret;
}

int faction::response_time(game *g)
{
 int base = abs(mapx - g->levx);
 if (abs(mapy - g->levy) > base)
  base = abs(mapy - g->levy);
 if (base > size)	// Out of our sphere of influence
  base *= 2.5;
 base *= 24;	// 24 turns to move one overmap square
 int maxdiv = 10;
 if (goal == FACGOAL_DOMINANCE)
  maxdiv += 2;
 if (has_job(FACJOB_CARAVANS))
  maxdiv += 2;
 if (has_job(FACJOB_SCAVENGE))
  maxdiv++;
 if (has_job(FACJOB_MERCENARIES))
  maxdiv += 2;
 if (has_job(FACJOB_FARMERS))
  maxdiv -= 2;
 if (has_value(FACVAL_EXPLORATION))
  maxdiv += 2;
 if (has_value(FACVAL_LONERS))
  maxdiv -= 3;
 if (has_value(FACVAL_TREACHERY))
  maxdiv -= rng(0, 3);
 int mindiv = (maxdiv > 9 ? maxdiv - 9 : 1);
 base /= rng(mindiv, maxdiv);// We might be in the field
 base -= likes_u;	// We'll hurry, if we like you
 if (base < 100)
  base = 100;
 return base;
}


// END faction:: MEMBER FUNCTIONS


std::string invent_name()
{
 std::string ret = "";
 std::string tmp;
 int syllables = rng(2, 3);
 for (int i = 0; i < syllables; i++) {
  switch (rng(0, 25)) {
   case  0: tmp = "ab";  break;
   case  1: tmp = "bon"; break;
   case  2: tmp = "cor"; break;
   case  3: tmp = "den"; break;
   case  4: tmp = "el";  break;
   case  5: tmp = "fes"; break;
   case  6: tmp = "gun"; break;
   case  7: tmp = "hit"; break;
   case  8: tmp = "id";  break;
   case  9: tmp = "jan"; break;
   case 10: tmp = "kal"; break;
   case 11: tmp = "ler"; break;
   case 12: tmp = "mal"; break;
   case 13: tmp = "nor"; break;
   case 14: tmp = "or";  break;
   case 15: tmp = "pan"; break;
   case 16: tmp = "qua"; break;
   case 17: tmp = "ros"; break;
   case 18: tmp = "sin"; break;
   case 19: tmp = "tor"; break;
   case 20: tmp = "urr"; break;
   case 21: tmp = "ven"; break;
   case 22: tmp = "wel"; break;
   case 23: tmp = "oxo";  break;
   case 24: tmp = "yen"; break;
   case 25: tmp = "zu";  break;
  }
  ret += tmp;
 }
 ret[0] += 'A' - 'a';
 return ret;
}

std::string invent_adj()
{
 int syllables = dice(2, 2) - 1;
 std::string ret,  tmp;
 switch (rng(0, 25)) {
  case  0: ret = "Ald";   break;
  case  1: ret = "Brogg"; break;
  case  2: ret = "Cald";  break;
  case  3: ret = "Dredd"; break;
  case  4: ret = "Eld";   break;
  case  5: ret = "Forr";  break;
  case  6: ret = "Gugg";  break;
  case  7: ret = "Horr";  break;
  case  8: ret = "Ill";   break;
  case  9: ret = "Jov";   break;
  case 10: ret = "Kok";   break;
  case 11: ret = "Lill";  break;
  case 12: ret = "Moom";  break;
  case 13: ret = "Nov";   break;
  case 14: ret = "Orb";   break;
  case 15: ret = "Perv";  break;
  case 16: ret = "Quot";  break;
  case 17: ret = "Rar";   break;
  case 18: ret = "Suss";  break;
  case 19: ret = "Torr";  break;
  case 20: ret = "Umbr";  break;
  case 21: ret = "Viv";   break;
  case 22: ret = "Warr";  break;
  case 23: ret = "Xen";   break;
  case 24: ret = "Yend";  break;
  case 25: ret = "Zor";   break;
 }
 for (int i = 0; i < syllables - 2; i++) {
  switch (rng(0, 17)) {
   case  0: tmp = "al";   break;
   case  1: tmp = "arn";  break;
   case  2: tmp = "astr"; break;
   case  3: tmp = "antr"; break;
   case  4: tmp = "ent";  break;
   case  5: tmp = "ell";  break;
   case  6: tmp = "ev";   break;
   case  7: tmp = "emm";  break;
   case  8: tmp = "empr"; break;
   case  9: tmp = "ill";  break;
   case 10: tmp = "ial";  break;
   case 11: tmp = "ior";  break;
   case 12: tmp = "ordr"; break;
   case 13: tmp = "oth";  break;
   case 14: tmp = "omn";  break;
   case 15: tmp = "uv";   break;
   case 16: tmp = "ulv";  break;
   case 17: tmp = "urn";  break;
  }
  ret += tmp;
 }
 switch (rng(0, 24)) {
  case  0: tmp = "";      break;
  case  1: tmp = "al";    break;
  case  2: tmp = "an";    break;
  case  3: tmp = "ard";   break;
  case  4: tmp = "ate";   break;
  case  5: tmp = "e";     break;
  case  6: tmp = "ed";    break;
  case  7: tmp = "en";    break;
  case  8: tmp = "er";    break;
  case  9: tmp = "ial";   break;
  case 10: tmp = "ian";   break;
  case 11: tmp = "iated"; break;
  case 12: tmp = "ier";   break;
  case 13: tmp = "ious";  break;
  case 14: tmp = "ish";   break;
  case 15: tmp = "ive";   break;
  case 16: tmp = "oo";    break;
  case 17: tmp = "or";    break;
  case 18: tmp = "oth";   break;
  case 19: tmp = "old";   break;
  case 20: tmp = "ous";   break;
  case 21: tmp = "ul";    break;
  case 22: tmp = "un";    break;
  case 23: tmp = "ule";   break;
  case 24: tmp = "y";     break;
 }
 ret += tmp;
 return ret;
}

// Used in game.cpp
std::string fac_ranking_text(int val)
{
 if (val <= -100)
  return "Archenemy";
 if (val <= -80)
  return "Wanted Dead";
 if (val <= -60)
  return "Enemy of the People";
 if (val <= -40)
  return "Wanted Criminal";
 if (val <= -20)
  return "Not Welcome";
 if (val <= -10)
  return "Pariah";
 if (val <=  -5)
  return "Disliked";
 if (val >= 100)
  return "Hero";
 if (val >= 80)
  return "Idol";
 if (val >= 60)
  return "Beloved";
 if (val >= 40)
  return "Highly Valued";
 if (val >= 20)
  return "Valued";
 if (val >= 10)
  return "Well-Liked";
 if (val >= 5)
  return "Liked";

 return "Neutral";
}

// Used in game.cpp
std::string fac_respect_text(int val)
{
// Respected, feared, etc.
 if (val >= 100)
  return "Legendary";
 if (val >= 80)
  return "Unchallenged";
 if (val >= 60)
  return "Mighty";
 if (val >= 40)
  return "Famous";
 if (val >= 20)
  return "Well-Known";
 if (val >= 10)
  return "Spoken Of";

// Disrepected, laughed at, etc.
 if (val <= -100)
  return "Worthless Scum";
 if (val <= -80)
  return "Vermin";
 if (val <= -60)
  return "Despicable";
 if (val <= -40)
  return "Parasite";
 if (val <= -20)
  return "Leech";
 if (val <= -10)
  return "Laughingstock";

 return "Neutral";
}
