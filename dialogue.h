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
 talk_topic opt(talk_topic topic, game *g);

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
    void nothing              (game *g, npc *p) {};
    void assign_mission       (game *g, npc *p);
    void mission_success      (game *g, npc *p);
    void mission_failure      (game *g, npc *p);
    void clear_mission        (game *g, npc *p);
    void mission_reward       (game *g, npc *p);
    void mission_favor        (game *g, npc *p);
    void give_equipment       (game *g, npc *p);
    void start_trade          (game *g, npc *p);
    void assign_base          (game *g, npc *p);
    void follow               (game *g, npc *p); // p follows u
    void deny_follow          (game *g, npc *p); // p gets DI_ASKED_TO_FOLLOW
    void deny_lead            (game *g, npc *p); // p gets DI_ASKED_TO_LEAD
    void deny_equipment       (game *g, npc *p); // p gets DI_ASKED_FOR_ITEM
    void enslave              (game *g, npc *p) {}; // p becomes slave of u
    void hostile              (game *g, npc *p); // p turns hostile to u
    void flee                 (game *g, npc *p);
    void leave                (game *g, npc *p); // p becomes indifferant

    void start_mugging        (game *g, npc *p);
    void player_leaving       (game *g, npc *p);

    void drop_weapon          (game *g, npc *p);
    void player_weapon_away   (game *g, npc *p);
    void player_weapon_drop   (game *g, npc *p);

    void lead_to_safety       (game *g, npc *p);
    void start_training       (game *g, npc *p);

    void toggle_use_guns      (game *g, npc *p);
    void toggle_use_silent    (game *g, npc *p);
    void toggle_use_grenades  (game *g, npc *p);
    void set_engagement_none  (game *g, npc *p);
    void set_engagement_close (game *g, npc *p);
    void set_engagement_weak  (game *g, npc *p);
    void set_engagement_hit   (game *g, npc *p);
    void set_engagement_all   (game *g, npc *p);
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
 npc_opinion opinion_success;
 npc_opinion opinion_failure;
 void (talk_function::*effect_success)(game *, npc *);
 void (talk_function::*effect_failure)(game *, npc *);
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
 std::vector<talk_response> none(game *g, npc *p);
 std::vector<talk_response> shelter(game *g, npc *p);
 std::vector<talk_response> shopkeep(game *g, npc *p);
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
