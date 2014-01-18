#ifndef _DIALOGUE_H_
#define _DIALOGUE_H_

#include "player.h"
#include "output.h"
#include "npc.h"
#include <vector>
#include <string>

struct dialogue {
 player *alpha;
 npc *beta;
 WINDOW *win;
 bool done;
 std::vector<std::string> history;
 std::vector<talk_topic> topic_stack;

 int opt(std::string challenge, ...);
 talk_topic opt(talk_topic topic);

 dialogue()
 {
  alpha = NULL;
  beta = NULL;
  win = NULL;
  done = false;
 }
};

struct talk_function
{
    void nothing              (npc*) {};
    void assign_mission       (npc*);
    void mission_success      (npc*);
    void mission_failure      (npc*);
    void clear_mission        (npc*);
    void mission_reward       (npc*);
    void mission_favor        (npc*);
    void give_equipment       (npc*);
    void start_trade          (npc*);
    void assign_base          (npc*);
    void follow               (npc*); // p follows u
    void deny_follow          (npc*); // p gets DI_ASKED_TO_FOLLOW
    void deny_lead            (npc*); // p gets DI_ASKED_TO_LEAD
    void deny_equipment       (npc*); // p gets DI_ASKED_FOR_ITEM
    void enslave              (npc*) {}; // p becomes slave of u
    void hostile              (npc*); // p turns hostile to u
    void flee                 (npc*);
    void leave                (npc*); // p becomes indifferant

    void start_mugging        (npc*);
    void player_leaving       (npc*);

    void drop_weapon          (npc*);
    void player_weapon_away   (npc*);
    void player_weapon_drop   (npc*);

    void lead_to_safety       (npc*);
    void start_training       (npc*);

    void toggle_use_guns      (npc*);
    void toggle_use_silent    (npc*);
    void toggle_use_grenades  (npc*);
    void set_engagement_none  (npc*);
    void set_engagement_close (npc*);
    void set_engagement_weak  (npc*);
    void set_engagement_hit   (npc*);
    void set_engagement_all   (npc*);
};

enum talk_trial
{
 TALK_TRIAL_NONE, // No challenge here!
 TALK_TRIAL_LIE, // Straight up lying
 TALK_TRIAL_PERSUADE, // Convince them
 TALK_TRIAL_INTIMIDATE, // Physical intimidation
 NUM_TALK_TRIALS
};

struct talk_response
{
 std::string text;
 talk_trial trial;
 int difficulty;
 int mission_index;
 mission_id miss; // If it generates a new mission
 int tempvalue; // Used for various stuff
 Skill* skill;
 matype_id style;
 npc_opinion opinion_success;
 npc_opinion opinion_failure;
 void (talk_function::*effect_success)(npc*);
 void (talk_function::*effect_failure)(npc*);
 talk_topic success;
 talk_topic failure;

 talk_response()
 {
  text = "";
  trial = TALK_TRIAL_NONE;
  difficulty = 0;
  mission_index = -1;
  miss = MISSION_NULL;
  tempvalue = -1;
  skill = NULL;
  style = "";
  effect_success = &talk_function::nothing;
  effect_failure = &talk_function::nothing;
  opinion_success = npc_opinion();
  opinion_failure = npc_opinion();
  success = TALK_NONE;
  failure = TALK_NONE;
 }

 talk_response(const talk_response &rhs)
 {
  text = rhs.text;
  trial = rhs.trial;
  difficulty = rhs.difficulty;
  mission_index = rhs.mission_index;
  miss = rhs.miss;
  tempvalue = rhs.tempvalue;
  skill = rhs.skill;
  style = rhs.style;
  effect_success = rhs.effect_success;
  effect_failure = rhs.effect_failure;
  opinion_success = rhs.opinion_success;
  opinion_failure = rhs.opinion_failure;
  success = rhs.success;
  failure = rhs.failure;
 }
};

struct talk_response_list
{
 std::vector<talk_response> none(npc*);
 std::vector<talk_response> shelter(npc*);
 std::vector<talk_response> shopkeep(npc*);
};

/* There is a array of tag_data, "tags", at the bottom of this file.
 * It maps tags to the array of string replacements;
 * e.g. "<name_g>" => talk_good_names
 * Other tags, like "<yrwp>", are mapped to dynamic things
 *  (like the player's weapon), and are defined in parse_tags() (npctalk.cpp)
 */
struct tag_data
{
 std::string tag;
 std::string (*replacement)[10];
};

#endif  // _DIALOGUE_H_
