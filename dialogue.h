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

 int opt(std::string challenge, ...);
};

std::string talk_okay[10] = {
"okay", "get it", "you dig", "dig", "got it", "you see", "see, <name_g>",
"savvy", "that clear"};

std::string talk_bad_names[10] = {
"punk",		"bitch",	"mark",		"asshole",	"fucker",
"sucker",	"fuckwad",	"fuckface",	"motherfucker",	"shithead"};

std::string talk_good_names[10] = {
"stranger",	"friend",	"pilgrim",	"traveler",	"pal",
"fella",	"you",		"dude",		"buddy",	"man"};

std::string talk_greeting_gen[10] = {
"Hey <name_g>.", "Greetings <name_g>.", "Hi <name_g><punc> You okay?",
"<name_g><punc>  Let's talk.", "Well hey there.",
"<name_g><punc>  Hello.", "What's up, <name_g>?", "You okay, <name_g>?",
"Hello, <name_g>.", "Hi <name_g>"};

std::string ill_die[10] = {
"I'm not gonna last much longer", "I'll be dead soon", "I'll be a goner",
"I'm dead, <name_g>,", "I'm dead meat", "I'm in serious trouble", "I'm doomed",
"I'm done for", "I won't last much longer", "my days are numbered"};

std::string talk_needs[num_needs][5] = {
{"", "", "", "", ""},
{"Hey<punc> You got any <ammo>?", "I'll need some <ammo> soon, got any?",
 "I really need some <ammo><punc>", "I need <ammo> for my <mywp>, got any?",
 "I need some <ammo> bad<punc>"},
{"Got anything I can use as a weapon?",
 "<ill_die> without a good weapon<punc>",
 "I'm sick of fighting with my <mywp>, got something better?",
 "Hey <name_g>, care to sell me a weapon?",
 "My <mywp> just won't cut it, I need a real weapon..."},
{"Hey <name_g>, I could really use a gun.",
 "Hey, you got a spare gun?  It'd be better than my <mywp><punc>",
 "<ill_die> if I don't find a gun soon!",
 "<name_g><punc> Feel like selling me a gun?",
 "I need a gun, any kind will do!"},
{"I could use some food, here.", "I need some food, real bad!",
 "Man, am I glad to see you!  Got any food to trade?",
 "<ill_die> unless I get some food in me... got any?",
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

std::string talk_drop_weap[10] = {
"Drop your weapon!",
"Okay <name_b>, drop your weapon!",
"Put your weapon down!",
"Drop the <yrwp>, <name_b>!",
"Drop the <yrwp>!",
"Drop your <yrwp>!",
"Put down the <yrwp>!",
"Drop your weapon, <name_b>!",
"Put down your <yrwp>!",
"Alright, drop the <yrwp>!"
};

std::string talk_hands_up[10] = {
"Put your hands up!",
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
"I don't belong to a faction, <name_g>"
};
