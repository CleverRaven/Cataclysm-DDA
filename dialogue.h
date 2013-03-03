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
 void nothing			(game *g, npc *p) {};
 void assign_mission		(game *g, npc *p);
 void mission_success		(game *g, npc *p);
 void mission_failure		(game *g, npc *p);
 void clear_mission		(game *g, npc *p);
 void mission_reward		(game *g, npc *p);
 void mission_favor		(game *g, npc *p);
 void give_equipment		(game *g, npc *p);
 void start_trade		(game *g, npc *p);
 void follow			(game *g, npc *p); // p follows u
 void deny_follow		(game *g, npc *p); // p gets DI_ASKED_TO_FOLLOW
 void deny_lead			(game *g, npc *p); // p gets DI_ASKED_TO_LEAD
 void deny_equipment		(game *g, npc *p); // p gets DI_ASKED_FOR_ITEM
 void enslave			(game *g, npc *p) {}; // p becomes slave of u
 void hostile			(game *g, npc *p); // p turns hostile to u
 void flee			(game *g, npc *p);
 void leave			(game *g, npc *p); // p becomes indifferant

 void start_mugging		(game *g, npc *p);
 void player_leaving		(game *g, npc *p);

 void drop_weapon		(game *g, npc *p);
 void player_weapon_away	(game *g, npc *p);
 void player_weapon_drop	(game *g, npc *p);

 void lead_to_safety		(game *g, npc *p);
 void start_training		(game *g, npc *p);

 void toggle_use_guns		(game *g, npc *p);
 void toggle_use_grenades	(game *g, npc *p);
 void set_engagement_none	(game *g, npc *p);
 void set_engagement_close	(game *g, npc *p);
 void set_engagement_weak	(game *g, npc *p);
 void set_engagement_hit	(game *g, npc *p);
 void set_engagement_all	(game *g, npc *p);
};

enum talk_trial
{
 TALK_TRIAL_NONE, // No challenge here!
 TALK_TRIAL_LIE, // Straight up lying
 TALK_TRIAL_PERSUADE, // Convince them
 TALK_TRIAL_INTIMIDATE, // Physical intimidation
 NUM_TALK_TRIALS
};

std::string talk_trial_text[NUM_TALK_TRIALS] = {
"", "LIE", "PERSUADE", "INTIMIDATE"
};

struct talk_response
{
 std::string text;
 talk_trial trial;
 int difficulty;
 int mission_index;
 mission_id miss;	// If it generates a new mission
 int tempvalue;		// Used for various stuff
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

std::string talk_needs[num_needs][5] = {
{"", "", "", "", ""},
{"Hey<punc> You got any <ammo>?", "I'll need some <ammo> soon, got any?",
 "I really need some <ammo><punc>", "I need <ammo> for my <mywp>, got any?",
 "I need some <ammo> <very> bad<punc>"},
{"Got anything I can use as a weapon?",
 "<ill_die> without a good weapon<punc>",
 "I'm sick of fighting with my <swear> <mywp>, got something better?",
 "Hey <name_g>, care to sell me a weapon?",
 "My <mywp> just won't cut it, I need a real weapon..."},
{"Hey <name_g>, I could really use a gun.",
 "Hey, you got a spare gun?  It'd be better than my <swear> <mywp><punc>",
 "<ill_die> if I don't find a gun soon!",
 "<name_g><punc> Feel like selling me a gun?",
 "I need a gun, any kind will do!"},
{"I could use some food, here.", "I need some food, <very> bad!",
 "Man, am I <happy> to see you!  Got any food to trade?",
 "<ill_die> unless I get some food in me<punc> <okay>?",
 "Please tell me you have some food to trade!"},
{"Got anything to drink?", "I need some water or something.",
 "<name_g>, I need some water... got any?",
 "<ill_die> without something to drink.", "You got anything to drink?"}
/*
{"<ill_die> unless I get healed<punc>", "You gotta heal me up, <name_g><punc>",
 "Help me<punc> <ill_die> if you don't heal me<punc>",
 "Please... I need medical help<punc>", "
*/
};

std::string talk_okay[10] = {
"okay", "get it", "you dig", "dig", "got it", "you see", "see, <name_g>",
"alright", "that clear"};

std::string talk_no[10] = {
"no", "fuck no", "hell no", "no way", "not a chance",
"I don't think so", "no way in hell", "nuh uh", "nope", "fat chance"};

std::string talk_bad_names[10] = {
"punk",		"bitch",	"dickhead",	"asshole",	"fucker",
"sucker",	"fuckwad",	"cocksucker",	"motherfucker",	"shithead"};

std::string talk_good_names[10] = {
"stranger",	"friend",	"pilgrim",	"traveler",	"pal",
"fella",	"you",		"dude",		"buddy",	"man"};

std::string talk_swear[10] = { // e.g. "drop the <swear> weapon"
"fucking", "goddamn", "motherfucking", "freaking", "damn", "<swear> <swear>",
"fucking", "fuckin'", "god damn", "mafuckin'"};

std::string talk_swear_interjection[10] = {
"fuck", "damn", "damnit", "shit", "cocksucker", "crap",
"motherfucker", "<swear><punc> <swear!>", "<very> <swear!>", "son of a bitch"};

std::string talk_fuck_you[10] = {
"fuck you", "fuck off", "go fuck yourself", "<fuck_you>, <name_b>",
"<fuck_you>, <swear> <name_b>", "<name_b>", "<swear> <name_b>",
"fuck you", "fuck off", "go fuck yourself"};

std::string talk_very[10] = { // Synonyms for "very" -- applied to adjectives
"really", "fucking", "super", "wicked", "very", "mega", "uber", "ultra",
"so <very>", "<very> <very>"};

std::string talk_really[10] = { // Synonyms for "really" -- applied to verbs
"really", "fucking", "absolutely", "definitely", "for real", "honestly",
"<really> <really>", "most <really>", "urgently", "REALLY"};

std::string talk_happy[10] = {
"glad", "happy", "overjoyed", "ecstatic", "thrilled", "stoked",
"<very> <happy>", "tickled pink", "delighted", "pumped"};

std::string talk_sad[10] = {
"sad", "bummed", "depressed", "pissed", "unhappy", "<very> <sad>", "dejected",
"down", "blue", "glum"};

std::string talk_greeting_gen[10] = {
"Hey <name_g>.", "Greetings <name_g>.", "Hi <name_g><punc> You okay?",
"<name_g><punc>  Let's talk.", "Well hey there.",
"<name_g><punc>  Hello.", "What's up, <name_g>?", "You okay, <name_g>?",
"Hello, <name_g>.", "Hi <name_g>"};

std::string talk_ill_die[10] = {
"I'm not gonna last much longer", "I'll be dead soon", "I'll be a goner",
"I'm dead, <name_g>,", "I'm dead meat", "I'm in <very> serious trouble",
"I'm <very> doomed", "I'm done for", "I won't last much longer",
"my days are <really> numbered"};

std::string talk_ill_kill_you[10] = {
"I'll kill you", "you're dead", "I'll <swear> kill you", "you're dead meat",
"<ill_kill_you>, <name_b>", "you're a dead <man>", "you'll taste my <mywp>",
"you're <swear> dead", "<name_b>, <ill_kill_you>"};

std::string talk_drop_weapon[10] = {
"Drop your <swear> weapon!",
"Okay <name_b>, drop your weapon!",
"Put your <swear> weapon down!",
"Drop the <yrwp>, <name_b>!",
"Drop the <swear> <yrwp>!",
"Drop your <yrwp>!",
"Put down the <yrwp>!",
"Drop your <swear> weapon, <name_b>!",
"Put down your <yrwp>!",
"Alright, drop the <yrwp>!"
};

std::string talk_hands_up[10] = {
"Put your <swear> hands up!",
"Put your hands up, <name_b>!",
"Reach for the sky!",
"Hands up!",
"Hands in the air!",
"Hands up, <name_b>!",
"Hands where I can see them!",
"Okay <name_b>, hands up!",
"Okay <name_b><punc> hands up!",
"Hands in the air, <name_b>!"
};

std::string talk_no_faction[10] = {
"I'm unaffiliated.",
"I don't run with a crew.",
"I'm a solo artist, <okay>?",
"I don't kowtow to any group, <okay>?",
"I'm a freelancer.",
"I work alone, <name_g>.",
"I'm a free agent, more money that way.",
"I prefer to work uninhibited by that kind of connection.",
"I haven't found one that's good enough for me.",
"I don't belong to a faction, <name_g>."
};

std::string talk_come_here[10] = {
"Wait up, let's talk!",
"Hey, I <really> want to talk to you!",
"Come on, talk to me!",
"Hey <name_g>, let's talk!",
"<name_g>, we <really> need to talk!",
"Hey, we should talk, <okay>?",
"<name_g>!  Wait up!",
"Wait up, <okay>?",
"Let's talk, <name_g>!",
"Look, <name_g><punc> let's talk!"
};

std::string talk_keep_up[10] = {
"Catch up!",
"Get over here!",
"Catch up, <name_g>!",
"Keep up!",
"Come on, <catch_up>!",

"Keep it moving!",
"Stay with me!",
"Keep close!",
"Stay close!",
"Let's keep going!"
};

std::string talk_wait[10] = {
"Hey, where are you?",
"Wait up, <name_g>!",
"<name_g>, wait for me!",
"Hey, wait up, <okay>?",
"You <really> need to wait for me!",
"You <swear> need to wait!",
"<name_g>, where are you?",
"Hey <name_g><punc> Wait for me!",
"Where are you?!",
"Hey, I'm over here!"
};

std::string talk_let_me_pass[10] = {
"Excuse me, let me pass.",
"Hey <name_g>, can I get through?",
"Let me get past you, <name_g>.",
"Let me through, <okay>?",
"Can I get past you, <name_g>?",
"I need to get past you, <name_g>.",
"Move your <swear> ass, <name_b>!",
"Out of my way, <name_b>!",
"Move it, <name_g>!",
"You need to move, <name_g>, <okay>?"
};

// Used to tell player to move to avoid friendly fire
std::string talk_move[10] = {
"Move",
"Move your ass",
"Get out of the way",
"You need to move"
"Hey <name_g>, move",
"<swear> move it",
"Move your <swear> ass",
"Get out of my way, <name_b>,",
"Move to the side",
"Get out of my line of fire"
};

std::string talk_done_mugging[10] = {
"Thanks for the cash, <name_b>!",
"So long, <name_b>!",
"Thanks a lot, <name_g>!",
"Catch you later, <name_g>!",
"See you later, <name_b>!",
"See you in hell, <name_b>!",
"Hasta luego, <name_g>!",
"I'm outta here! <done_mugging>",
"Bye bye, <name_b>!",
"Thanks, <name_g>!"
};

std::string talk_leaving[10] = {
"So long, <name_b>!",
"Hasta luego, <name_g>!",
"I'm outta here!",
"Bye bye, <name_b>!",
"So long, <name_b>!",
"Hasta luego, <name_g>!",
"I'm outta here!",
"Bye bye, <name_b>!",
"I'm outta here!",
"Bye bye, <name_b>!"
};

std::string talk_catch_up[10] = {
"You're too far away, <name_b>!",
"Hurry up, <name_g>!",
"I'm outta here soon!",
"Come on molasses!",
"What's taking so long?",
"Get with the program laggard!",
"Did the zombies get you?",
"Wait up <name_b>!",
"Did you evolve from a snail?",
"How 'bout picking up the pace!"
};

#define NUM_STATIC_TAGS 26

tag_data talk_tags[NUM_STATIC_TAGS] = {
{"<okay>",		&talk_okay},
{"<no>",		&talk_no},
{"<name_b>",		&talk_bad_names},
{"<name_g>",		&talk_good_names},
{"<swear>",		&talk_swear},
{"<swear!>",		&talk_swear_interjection},
{"<fuck_you>",		&talk_fuck_you},
{"<very>",		&talk_very},
{"<really>",		&talk_really},
{"<happy>",		&talk_happy},
{"<sad>",		&talk_sad},
{"<greet>",		&talk_greeting_gen},
{"<ill_die>",		&talk_ill_die},
{"<ill_kill_you>",	&talk_ill_kill_you},
{"<drop_it>",		&talk_drop_weapon},
{"<hands_up>",		&talk_hands_up},
{"<no_faction>",	&talk_no_faction},
{"<come_here>",		&talk_come_here},
{"<keep_up>",		&talk_keep_up},
{"<lets_talk>",		&talk_come_here},
{"<wait>",		&talk_wait},
{"<let_me_pass>",	&talk_let_me_pass},
{"<move>",		&talk_move},
{"<done_mugging>",	&talk_done_mugging},
{"<catch_up>",		&talk_catch_up},
{"<im_leaving_you>",	&talk_leaving}
};

#endif
