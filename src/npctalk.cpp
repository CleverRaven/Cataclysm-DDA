#include "npc.h"
#include "output.h"
#include "game.h"
#include "dialogue.h"
#include "rng.h"
#include "line.h"
#include "debug.h"
#include "catacharset.h"
#include "messages.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

std::string talk_needs[num_needs][5];
std::string talk_okay[10];
std::string talk_no[10];
std::string talk_bad_names[10];
std::string talk_good_names[10];
std::string talk_swear[10];
std::string talk_swear_interjection[10];
std::string talk_fuck_you[10];
std::string talk_very[10];
std::string talk_really[10];
std::string talk_happy[10];
std::string talk_sad[10];
std::string talk_greeting_gen[10];
std::string talk_ill_die[10];
std::string talk_ill_kill_you[10];
std::string talk_drop_weapon[10];
std::string talk_hands_up[10];
std::string talk_no_faction[10];
std::string talk_come_here[10];
std::string talk_keep_up[10] ;
std::string talk_wait[10];
std::string talk_let_me_pass[10];
// Used to tell player to move to avoid friendly fire
std::string talk_move[10];
std::string talk_done_mugging[10];
std::string talk_leaving[10];
std::string talk_catch_up[10];

#define NUM_STATIC_TAGS 26

tag_data talk_tags[NUM_STATIC_TAGS] = {
{"<okay>",          &talk_okay},
{"<no>",            &talk_no},
{"<name_b>",        &talk_bad_names},
{"<name_g>",        &talk_good_names},
{"<swear>",         &talk_swear},
{"<swear!>",        &talk_swear_interjection},
{"<fuck_you>",      &talk_fuck_you},
{"<very>",          &talk_very},
{"<really>",        &talk_really},
{"<happy>",         &talk_happy},
{"<sad>",           &talk_sad},
{"<greet>",         &talk_greeting_gen},
{"<ill_die>",       &talk_ill_die},
{"<ill_kill_you>",  &talk_ill_kill_you},
{"<drop_it>",       &talk_drop_weapon},
{"<hands_up>",      &talk_hands_up},
{"<no_faction>",    &talk_no_faction},
{"<come_here>",     &talk_come_here},
{"<keep_up>",       &talk_keep_up},
{"<lets_talk>",     &talk_come_here},
{"<wait>",          &talk_wait},
{"<let_me_pass>",   &talk_let_me_pass},
{"<move>",          &talk_move},
{"<done_mugging>",  &talk_done_mugging},
{"<catch_up>",      &talk_catch_up},
{"<im_leaving_you>",&talk_leaving}
};

// Every OWED_VAL that the NPC owes you counts as +1 towards convincing
#define OWED_VAL 250

// Some aliases to help with gen_responses
#define RESPONSE(txt)      ret.push_back(talk_response());\
                           ret.back().text = txt

#define SELECT_MISS(txt, index)  ret.push_back(talk_response());\
                                 ret.back().text = txt;\
                                 ret.back().mission_index = index

#define SELECT_TEMP(txt, index)  ret.push_back(talk_response());\
                                 ret.back().text = txt;\
                                 ret.back().tempvalue = index

#define SELECT_SKIL(txt, skillIn)  ret.push_back(talk_response());\
                                 ret.back().text = txt;\
                                 ret.back().skill = skillIn;

#define SELECT_STYLE(txt, styleIn)  ret.push_back(talk_response());\
                                 ret.back().text = txt;\
                                 ret.back().style = styleIn;

#define TRIAL(tr, diff) ret.back().trial = tr;\
                        ret.back().difficulty = diff

#define SUCCESS(topic)  ret.back().success = topic
#define FAILURE(topic)  ret.back().failure = topic

#define SUCCESS_OPINION(T, F, V, A, O)   ret.back().opinion_success =\
                                         npc_opinion(T, F, V, A, O)
#define FAILURE_OPINION(T, F, V, A, O)   ret.back().opinion_failure =\
                                         npc_opinion(T, F, V, A, O)

#define SUCCESS_ACTION(func)  ret.back().effect_success = func
#define FAILURE_ACTION(func)  ret.back().effect_failure = func

#define SUCCESS_MISSION(type) ret.back().miss = type

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

std::string dynamic_line(talk_topic topic, npc *p);
std::vector<talk_response> gen_responses(talk_topic topic, npc *p);
int topic_category(talk_topic topic);

talk_topic special_talk(char ch);

int trial_chance(talk_response response, player *u, npc *p);

bool trade(npc *p, int cost, std::string deal);

void game::init_npctalk()
{
    std::string tmp_talk_needs[num_needs][5] = {
    {"", "", "", "", ""},
    {_("Hey<punc> You got any <ammo>?"), _("I'll need some <ammo> soon, got any?"),
     _("I really need some <ammo><punc>"), _("I need <ammo> for my <mywp>, got any?"),
     _("I need some <ammo> <very> bad<punc>")},
    {_("Got anything I can use as a weapon?"),
     _("<ill_die> without a good weapon<punc>"),
     _("I'm sick of fighting with my <swear> <mywp>, got something better?"),
     _("Hey <name_g>, care to sell me a weapon?"),
     _("My <mywp> just won't cut it, I need a real weapon...")},
    {_("Hey <name_g>, I could really use a gun."),
     _("Hey, you got a spare gun?  It'd be better than my <swear> <mywp><punc>"),
     _("<ill_die> if I don't find a gun soon!"),
     _("<name_g><punc> Feel like selling me a gun?"),
     _("I need a gun, any kind will do!")},
    {_("I could use some food, here."), _("I need some food, <very> bad!"),
     _("Man, am I <happy> to see you!  Got any food to trade?"),
     _("<ill_die> unless I get some food in me<punc> <okay>?"),
     _("Please tell me you have some food to trade!")},
    {_("Got anything to drink?"), _("I need some water or something."),
     _("<name_g>, I need some water... got any?"),
     _("<ill_die> without something to drink."), _("You got anything to drink?")}
    };

    for(int i = 0; i < num_needs; i++) {
        for(int j = 0; j < 5; j++) {
            talk_needs[i][j] = tmp_talk_needs[i][j];
        }
    }

    std::string tmp_talk_okay[10] = {
    _("okay"), _("get it"), _("you dig"), _("dig"), _("got it"), _("you see"), _("see, <name_g>"),
    _("alright"), _("that clear")};
    for(int j=0; j<10; j++) {talk_okay[j] = tmp_talk_okay[j];}

    std::string tmp_talk_no[10] = {
    _("no"), _("fuck no"), _("hell no"), _("no way"), _("not a chance"),
    _("I don't think so"), _("no way in hell"), _("nuh uh"), _("nope"), _("fat chance")};
    for(int j=0; j<10; j++) {talk_no[j] = tmp_talk_no[j];}

    std::string tmp_talk_bad_names[10] = {
    _("punk"),  _("loser"), _("dickhead"), _("asshole"), _("fucker"),
    _("sucker"), _("fuckwad"), _("jerk"), _("motherfucker"), _("shithead")};
    for(int j=0; j<10; j++) {talk_bad_names[j] = tmp_talk_bad_names[j];}

    std::string tmp_talk_good_names[10] = {
    _("stranger"), _("friend"), _("pilgrim"), _("traveler"), _("pal"),
    _("fella"), _("you"),  _("dude"),  _("buddy"), _("man")};
    for(int j=0; j<10; j++) {talk_good_names[j] = tmp_talk_good_names[j];}

    std::string tmp_talk_swear[10] = { // e.g. _("drop the <swear> weapon")
    _("fucking"), _("goddamn"), _("motherfucking"), _("freaking"), _("damn"), _("<swear> <swear>"),
    _("fucking"), _("fuckin'"), _("god damn"), _("mafuckin'")};
    for(int j=0; j<10; j++) {talk_swear[j] = tmp_talk_swear[j];}

    std::string tmp_talk_swear_interjection[10] = {
    _("fuck"), _("damn"), _("damnit"), _("shit"), _("fuckit"), _("crap"),
    _("motherfucker"), _("<swear><punc> <swear!>"), _("<very> <swear!>"), _("son of an ass")};
    for(int j=0; j<10; j++) {talk_swear_interjection[j] = tmp_talk_swear_interjection[j];}

    std::string tmp_talk_fuck_you[10] = {
    _("fuck you"), _("fuck off"), _("go fuck yourself"), _("<fuck_you>, <name_b>"),
    _("<fuck_you>, <swear> <name_b>"), _("<name_b>"), _("<swear> <name_b>"),
    _("fuck you"), _("fuck off"), _("go fuck yourself")};
    for(int j=0; j<10; j++) {talk_fuck_you[j] = tmp_talk_fuck_you[j];}

    std::string tmp_talk_very[10] = { // Synonyms for _("very") -- applied to adjectives
    _("really"), _("fucking"), _("super"), _("wicked"), _("very"), _("mega"), _("uber"), _("ultra"),
    _("so <very>"), _("<very> <very>")};
    for(int j=0; j<10; j++) {talk_very[j] = tmp_talk_very[j];}

    std::string tmp_talk_really[10] = { // Synonyms for _("really") -- applied to verbs
    _("really"), _("fucking"), _("absolutely"), _("definitely"), _("for real"), _("honestly"),
    _("<really> <really>"), _("most <really>"), _("urgently"), _("REALLY")};
    for(int j=0; j<10; j++) {talk_really[j] = tmp_talk_really[j];}

    std::string tmp_talk_happy[10] = {
    _("glad"), _("happy"), _("overjoyed"), _("ecstatic"), _("thrilled"), _("stoked"),
    _("<very> <happy>"), _("tickled pink"), _("delighted"), _("pumped")};
    for(int j=0; j<10; j++) {talk_happy[j] = tmp_talk_happy[j];}

    std::string tmp_talk_sad[10] = {
    _("sad"), _("bummed"), _("depressed"), _("pissed"), _("unhappy"), _("<very> <sad>"), _("dejected"),
    _("down"), _("blue"), _("glum")};
    for(int j=0; j<10; j++) {talk_sad[j] = tmp_talk_sad[j];}

    std::string tmp_talk_greeting_gen[10] = {
    _("Hey <name_g>."), _("Greetings <name_g>."), _("Hi <name_g><punc> You okay?"),
    _("<name_g><punc>  Let's talk."), _("Well hey there."),
    _("<name_g><punc>  Hello."), _("What's up, <name_g>?"), _("You okay, <name_g>?"),
    _("Hello, <name_g>."), _("Hi <name_g>")};
    for(int j=0; j<10; j++) {talk_greeting_gen[j] = tmp_talk_greeting_gen[j];}

    std::string tmp_talk_ill_die[10] = {
    _("I'm not gonna last much longer"), _("I'll be dead soon"), _("I'll be a goner"),
    _("I'm dead, <name_g>,"), _("I'm dead meat"), _("I'm in <very> serious trouble"),
    _("I'm <very> doomed"), _("I'm done for"), _("I won't last much longer"),
    _("my days are <really> numbered")};
    for(int j=0; j<10; j++) {talk_ill_die[j] = tmp_talk_ill_die[j];}

    std::string tmp_talk_ill_kill_you[10] = {
    _("I'll kill you"), _("you're dead"), _("I'll <swear> kill you"), _("you're dead meat"),
    _("<ill_kill_you>, <name_b>"), _("you're a dead <man>"), _("you'll taste my <mywp>"),
    _("you're <swear> dead"), _("<name_b>, <ill_kill_you>")};
    for(int j=0; j<10; j++) {talk_ill_kill_you[j] = tmp_talk_ill_kill_you[j];}

    std::string tmp_talk_drop_weapon[10] = {
    _("Drop your <swear> weapon!"),
    _("Okay <name_b>, drop your weapon!"),
    _("Put your <swear> weapon down!"),
    _("Drop the <yrwp>, <name_b>!"),
    _("Drop the <swear> <yrwp>!"),
    _("Drop your <yrwp>!"),
    _("Put down the <yrwp>!"),
    _("Drop your <swear> weapon, <name_b>!"),
    _("Put down your <yrwp>!"),
    _("Alright, drop the <yrwp>!")
    };
    for(int j=0; j<10; j++) {talk_drop_weapon[j] = tmp_talk_drop_weapon[j];}

    std::string tmp_talk_hands_up[10] = {
    _("Put your <swear> hands up!"),
    _("Put your hands up, <name_b>!"),
    _("Reach for the sky!"),
    _("Hands up!"),
    _("Hands in the air!"),
    _("Hands up, <name_b>!"),
    _("Hands where I can see them!"),
    _("Okay <name_b>, hands up!"),
    _("Okay <name_b><punc> hands up!"),
    _("Hands in the air, <name_b>!")
    };
    for(int j=0; j<10; j++) {talk_hands_up[j] = tmp_talk_hands_up[j];}

    std::string tmp_talk_no_faction[10] = {
    _("I'm unaffiliated."),
    _("I don't run with a crew."),
    _("I'm a solo artist, <okay>?"),
    _("I don't kowtow to any group, <okay>?"),
    _("I'm a freelancer."),
    _("I work alone, <name_g>."),
    _("I'm a free agent, more money that way."),
    _("I prefer to work uninhibited by that kind of connection."),
    _("I haven't found one that's good enough for me."),
    _("I don't belong to a faction, <name_g>.")
    };
    for(int j=0; j<10; j++) {talk_no_faction[j] = tmp_talk_no_faction[j];}

    std::string tmp_talk_come_here[10] = {
    _("Wait up, let's talk!"),
    _("Hey, I <really> want to talk to you!"),
    _("Come on, talk to me!"),
    _("Hey <name_g>, let's talk!"),
    _("<name_g>, we <really> need to talk!"),
    _("Hey, we should talk, <okay>?"),
    _("<name_g>!  Wait up!"),
    _("Wait up, <okay>?"),
    _("Let's talk, <name_g>!"),
    _("Look, <name_g><punc> let's talk!")
    };
    for(int j=0; j<10; j++) {talk_come_here[j] = tmp_talk_come_here[j];}

    std::string tmp_talk_keep_up[10] = {
    _("Catch up!"),
    _("Get over here!"),
    _("Catch up, <name_g>!"),
    _("Keep up!"),
    _("Come on, <catch_up>!"),

    _("Keep it moving!"),
    _("Stay with me!"),
    _("Keep close!"),
    _("Stay close!"),
    _("Let's keep going!")
    };
    for(int j=0; j<10; j++) {talk_keep_up[j] = tmp_talk_keep_up[j];}

    std::string tmp_talk_wait[10] = {
    _("Hey, where are you?"),
    _("Wait up, <name_g>!"),
    _("<name_g>, wait for me!"),
    _("Hey, wait up, <okay>?"),
    _("You <really> need to wait for me!"),
    _("You <swear> need to wait!"),
    _("<name_g>, where are you?"),
    _("Hey <name_g><punc> Wait for me!"),
    _("Where are you?!"),
    _("Hey, I'm over here!")
    };
    for(int j=0; j<10; j++) {talk_wait[j] = tmp_talk_wait[j];}

    std::string tmp_talk_let_me_pass[10] = {
    _("Excuse me, let me pass."),
    _("Hey <name_g>, can I get through?"),
    _("Let me get past you, <name_g>."),
    _("Let me through, <okay>?"),
    _("Can I get past you, <name_g>?"),
    _("I need to get past you, <name_g>."),
    _("Move your <swear> ass, <name_b>!"),
    _("Out of my way, <name_b>!"),
    _("Move it, <name_g>!"),
    _("You need to move, <name_g>, <okay>?")
    };
    for(int j=0; j<10; j++) {talk_let_me_pass[j] = tmp_talk_let_me_pass[j];}

    // Used to tell player to move to avoid friendly fire
    std::string tmp_talk_move[10] = {
    _("Move"),
    _("Move your ass"),
    _("Get out of the way"),
    _("You need to move"),
    _("Hey <name_g>, move"),
    _("<swear> move it"),
    _("Move your <swear> ass"),
    _("Get out of my way, <name_b>,"),
    _("Move to the side"),
    _("Get out of my line of fire")
    };
    for(int j=0; j<10; j++) {talk_move[j] = tmp_talk_move[j];}

    std::string tmp_talk_done_mugging[10] = {
    _("Thanks for the cash, <name_b>!"),
    _("So long, <name_b>!"),
    _("Thanks a lot, <name_g>!"),
    _("Catch you later, <name_g>!"),
    _("See you later, <name_b>!"),
    _("See you in hell, <name_b>!"),
    _("Hasta luego, <name_g>!"),
    _("I'm outta here! <done_mugging>"),
    _("Bye bye, <name_b>!"),
    _("Thanks, <name_g>!")
    };
    for(int j=0; j<10; j++) {talk_done_mugging[j] = tmp_talk_done_mugging[j];}


    std::string tmp_talk_leaving[10] = {
    _("So long, <name_b>!"),
    _("Hasta luego, <name_g>!"),
    _("I'm outta here!"),
    _("Bye bye, <name_b>!"),
    _("So long, <name_b>!"),
    _("Hasta luego, <name_g>!"),
    _("I'm outta here!"),
    _("Bye bye, <name_b>!"),
    _("I'm outta here!"),
    _("Bye bye, <name_b>!")
    };
    for(int j=0; j<10; j++) {talk_leaving[j] = tmp_talk_leaving[j];}

    std::string tmp_talk_catch_up[10] = {
    _("You're too far away, <name_b>!"),
    _("Hurry up, <name_g>!"),
    _("I'm outta here soon!"),
    _("Come on molasses!"),
    _("What's taking so long?"),
    _("Get with the program laggard!"),
    _("Did the zombies get you?"),
    _("Wait up <name_b>!"),
    _("Did you evolve from a snail?"),
    _("How 'bout picking up the pace!")
    };
    for(int j=0; j<10; j++) {talk_catch_up[j] = tmp_talk_catch_up[j];}
}

void npc::talk_to_u()
{
// This is necessary so that we don't bug the player over and over
 if (attitude == NPCATT_TALK)
  attitude = NPCATT_NULL;
 else if (attitude == NPCATT_FLEE) {
  add_msg(_("%s is fleeing from you!"), name.c_str());
  return;
 } else if (attitude == NPCATT_KILL) {
  add_msg(_("%s is hostile!"), name.c_str());
  return;
 }
 dialogue d;
 d.alpha = &g->u;
 d.beta = this;

 d.topic_stack.push_back(chatbin.first_topic);

 if (is_leader())
  d.topic_stack.push_back(TALK_LEADER);
 else if (is_friend())
  d.topic_stack.push_back(TALK_FRIEND);

 int most_difficult_mission = 0;
 for (int i = 0; i < chatbin.missions.size(); i++) {
  mission_type *type = g->find_mission_type(chatbin.missions[i]);
  if (type->urgent && type->difficulty > most_difficult_mission) {
   d.topic_stack.push_back(TALK_MISSION_DESCRIBE);
   chatbin.mission_selected = i;
   most_difficult_mission = type->difficulty;
  }
 }
 most_difficult_mission = 0;
 bool chosen_urgent = false;
 for (int i = 0; i < chatbin.missions_assigned.size(); i++) {
  mission_type *type = g->find_mission_type(chatbin.missions_assigned[i]);
  if ((type->urgent && !chosen_urgent) ||
      (type->difficulty > most_difficult_mission &&
       (type->urgent || !chosen_urgent)            )) {
   chosen_urgent = type->urgent;
   d.topic_stack.push_back(TALK_MISSION_INQUIRE);
   chatbin.mission_selected = i;
   most_difficult_mission = type->difficulty;
  }
 }

 if (d.topic_stack.back() == TALK_NONE) {
  d.topic_stack.back() = pick_talk_topic(&(g->u));
 }

 moves -= 100;

 if(g->u.is_deaf()) {
  add_msg(_("%s tries to talk to you, but you're deaf!"), name.c_str());
  if(d.topic_stack.back() == TALK_MUG) {
   add_msg(_("When you don't respond, %s becomes angry!"), name.c_str());
   make_angry();
  }
  return;
 }

 decide_needs();

 d.win = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT) / 2 : 0,
                (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH) / 2 : 0);
 draw_border(d.win);
 mvwvline(d.win, 1, (FULL_SCREEN_WIDTH / 2) + 1, LINE_XOXO, FULL_SCREEN_HEIGHT - 1);
 mvwputch(d.win, 0, (FULL_SCREEN_WIDTH / 2) + 1, BORDER_COLOR, LINE_OXXX);
 mvwputch(d.win, FULL_SCREEN_HEIGHT - 1, (FULL_SCREEN_WIDTH / 2) + 1, BORDER_COLOR, LINE_XXOX);
 mvwprintz(d.win, 1,  1, c_white, _("Dialogue with %s"), name.c_str());
 mvwprintz(d.win, 1, (FULL_SCREEN_WIDTH / 2) + 3, c_white, _("Your response:"));

// Main dialogue loop
 do {
  talk_topic next = d.opt(d.topic_stack.back());
  if (next == TALK_NONE) {
   int cat = topic_category(d.topic_stack.back());
   do
    d.topic_stack.pop_back();
   while (cat != -1 && topic_category(d.topic_stack.back()) == cat);
  }
  if (next == TALK_DONE || d.topic_stack.empty())
   d.done = true;
  else if (next != TALK_NONE)
   d.topic_stack.push_back(next);
 } while (!d.done);
 delwin(d.win);
 g->refresh_all();
}

std::string dynamic_line(talk_topic topic, npc *p)
{
    // First, a sanity test for mission stuff
    if (topic >= TALK_MISSION_START && topic <= TALK_MISSION_END) {
        if (topic == TALK_MISSION_START) {
            return "Used TALK_MISSION_START - not meant to be used!";
        }
        if (topic == TALK_MISSION_END) {
            return "Used TALK_MISSION_END - not meant to be used!";
        }

        if (p->chatbin.mission_selected == -1) {
            return "mission_selected = -1; BUG! (npctalk.cpp:dynamic_line)";
        }

        int id = -1;
        if (topic == TALK_MISSION_INQUIRE || topic == TALK_MISSION_ACCEPTED ||
            topic == TALK_MISSION_SUCCESS || topic == TALK_MISSION_ADVICE ||
            topic == TALK_MISSION_FAILURE || topic == TALK_MISSION_SUCCESS_LIE) {
            if (p->chatbin.mission_selected >= p->chatbin.missions_assigned.size()) {
                return "mission_selected is too high; BUG! (npctalk.cpp:dynamic_line)";
            }
            id = p->chatbin.missions_assigned[p->chatbin.mission_selected];
        } else {
            if (p->chatbin.mission_selected >= p->chatbin.missions.size()) {
                return "mission_selected is too high; BUG! (npctalk.cpp:dynamic_line (2))";
            }
            id = p->chatbin.missions[p->chatbin.mission_selected];
        }

        // Mission stuff is a special case, so we'll handle it up here
        mission *miss = g->find_mission(id);
        mission_type *type = miss->type;
        std::string ret = mission_dialogue(mission_id(type->id), topic);
        if (ret.empty()) {
            debugmsg("Bug in npctalk.cpp:dynamic_line. Wrong mission_id(%d) or topic(%d)",
                     mission_id(type->id), topic);
            return "";
        }

        if (topic == TALK_MISSION_SUCCESS && miss->follow_up != MISSION_NULL) {
            return ret + _("  And I have more I'd like you to do.");
        }

    return ret;
    }

    switch (topic) {
        case TALK_NONE:
        case TALK_DONE:
            return "";

        case TALK_GUARD:
            switch (rng(1,5)){
            //if(one_in(2)){
            case 1:
                return _("I'm not in charge here, you're looking for someone else...");
            case 2:
                return _("Keep civil or I'll bring the pain.");
            case 3://acidia
                return _("Just on watch, move along.");
            case 4:
                if (g->u.male)
                    return _("Sir.");
                else
                    return _("Ma'am");
            case 5:
                if (g->u.male)
                    return _("Rough out there isn't it.");
                else
                    return _("Ma'am, you really shouldn't be traveling out there.");
            //}
            //else {
            //    return _("Keep civil or I'll bring the pain.");
            }

        case TALK_MISSION_LIST:
            if (p->chatbin.missions.empty()) {
                if (p->chatbin.missions_assigned.empty()) {
                    return _("I don't have any jobs for you.");
                } else {
                    return _("I don't have any more jobs for you.");
                }
            } else if (p->chatbin.missions.size() == 1) {
                if (p->chatbin.missions_assigned.empty()) {
                    return _("I just have one job for you.  Want to hear about it?");
                } else {
                    return _("I have another job for you.  Want to hear about it?");
                }
            } else if (p->chatbin.missions_assigned.empty()) {
                return _("I have several jobs for you.  Which should I describe?");
            } else {
                return _("I have several more jobs for you.  Which should I describe?");
            }

        case TALK_MISSION_LIST_ASSIGNED:
            if (p->chatbin.missions_assigned.empty()) {
                return _("You're not working on anything for me right now.");
            } else if (p->chatbin.missions_assigned.size() == 1) {
                return _("What about it?");
            } else {
                return _("Which job?");
            }

        case TALK_MISSION_REWARD:
            return _("Sure, here you go!");

        case TALK_EVAC_MERCHANT:
             return _("Welcome...");

        case TALK_EVAC_MERCHANT_NEW:
            return _("Before you say anything else, we're full.  Few days ago we had an outbreak due to lett'n in too many new refugees."
                     "  We do desperately need supplies and are willing to trade what we can for it.  Pay top dollar for jerky if you have any.");

        case TALK_EVAC_MERCHANT_PLANS:
            return _("To be honest, we started out with six buses full of office workers and soccer moms... after the refugee outbreak a day or two"
                    " ago the more courageous ones in our party ended up dead.  The only thing we want now is to run enough trade through"
                    " here to keep us alive.  Don't care who your goods come from or how you got them, just don't bring trouble.");

        case TALK_EVAC_MERCHANT_PLANS2:
            return _("I'm sorry, but the only way we're going to make it is if we keep our gates buttoned fast.  The guards in the basement "
                    "have orders to shoot on sight, if you so much as peep your head in the lower levels.  I don't know what made the scavengers "
                    "out there so ruthless but some of us have had to kill our own bloody kids... don't even think about strong arming us.");

        case TALK_EVAC_MERCHANT_PLANS3:
            return _("Well the refugees were staying here on the first floor when one their parties tried to sneak a dying guy in through the loading bay, "
                     "we ended up being awoken to shrieks and screams. Maybe two dozen people died that night.  The remaining refugees were banished the next "
                     "day and went on to form a couple of scavenging bands.  I'd say we got twenty decent men or women still here but our real strength comes "
                     "from all of our business partners that are accustomed to doing whatever is needed to survive.");

        case TALK_EVAC_MERCHANT_WORLD:
            return _("Can't say we've heard much.  Most these shelters seemed to have been designed to make people feel safer... not actually "
                    "aid in their survival.  Our radio equipment is utter garbage that someone convinced the government to buy, with no intention "
                    "of it ever being used.  From the passing scavangers I've heard nothing but prime loot'n spots and rumors of hordes.");

        case TALK_EVAC_MERCHANT_HORDES:
            return _("Had one guy pop in here a while back saying he had tried to drive into Syracuse after the outbreak.  Didn't even make it "
                     "downtown before he ran into a wall of the living dead that could stop a tank.  He hightailed it out but claims there were "
                     "several thousand at least.  Guess when you get a bunch of them together they end up making enough noise to attract everyone "
                     "in the neighborhood.  Luckily we haven't had a mob like that pass by here.");

        case TALK_EVAC_MERCHANT_PRIME_LOOT:
            return _("Well, there is a party of about a dozen 'scavengers' that found some sort of government facility.  They bring us a literal "
                     "truck load of jumpsuits, m4's, and canned food every week or so.  Since some of those guys got family here, we've been "
                     "doing alright.  As to where it is, I don't have the foggiest of ideas.");

        case TALK_EVAC_MERCHANT_ASK_JOIN:
            return _("Sorry, last thing we need is another mouth to feed.  Most of us lack any real survival skills so keeping our group "
                     "small enough to survive on the food random scavengers bring to trade with us is important.");

        case TALK_EVAC_MERCHANT_NO:
            return _("I'm sorry, not a risk we are willing to take right now.");

        case TALK_EVAC_MERCHANT_HELL_NO:
            return _("There isn't a chance in hell!  We had one guy come in here with bloody fur all over his body... well I guess that isn't all that "
                     "strange but I'm pretty sure whatever toxic waste is still out there is bound to mutate more than just his hair.");

        case TALK_EVAC_GUARD1:
            return _("Hello there.");

        case TALK_EVAC_GUARD1_PLACE:
            return _("This is a refugee center that we've made into a sort of trading hub.");

        case TALK_EVAC_GUARD1_GOVERNMENT:
            return _("Ha ha ha, no. Though there is Old Guard somewhere around here if you have any questions "
                     "relating to what the government is up to.");

        case TALK_EVAC_GUARD1_TRADE:
            return _("Anything valuable really. If you really want to know, go ask one of the actual traders. I'm just protection.");

        case TALK_EVAC_GUARD1_JOIN:
            return _("Nope.");

        case TALK_EVAC_GUARD1_JOIN2:
            return _("Death is pretty blunt.");

        case TALK_EVAC_GUARD1_JOIN3:
            return _("Nope.");

        case TALK_EVAC_GUARD1_ATTITUDE:
            return _("Then leave, you have two feet.");

        case TALK_EVAC_GUARD1_JOB:
            return _("Uh, not really. Go talk to a merchant if you have anything to sell. Otherwise the Old Guard liaison "
                     "might have something, if you can find him.");

        case TALK_EVAC_GUARD1_OLDGUARD:
            return _("That's just our nickname for them. They're whats left of the federal government.  "
                     "Don't know how legitimate they are but they are named after some military unit "
                     "that once protected the president.  Their liaison is usually hanging around "
                     "here somewhere.");

        case TALK_EVAC_GUARD1_BYE:
            return _("Stay safe out there. Hate to have to kill you after you've already died.");

        case TALK_EVAC_GUARD2:
            return _("Hello.");

        case TALK_EVAC_GUARD2_NEW:
            return _("Yes of course. Just don't bring any trouble and it's all fine by me.");

        case TALK_EVAC_GUARD2_RULES:
            return _("Well mostly no. Just don't go around robbing others and starting fights "
                     "and you will be all set. Also, don't go into the basement. Outsiders "
                     "are not allowed in there.");

        case TALK_EVAC_GUARD2_RULES_BASEMENT:
            return _("In short, we had a problem when a sick refugee died and turned "
                     "into a zombie.  We had to expel the refugees and most of our "
                     "surviving group now stays to the basement to prevent it from "
                     "happening again. Unless you really prove your worth I don't "
                     "foresee any exceptions to that rule.");

         case TALK_EVAC_GUARD2_WHO:
            return _("Most are scavengers like you.  They now make a living by "
                     "looting the cities in search for anything useful: food, "
                     "weapons, tools, gasoline. In exchange for their findings "
                     "we offer them a temporary place to rest and the services "
                     "of our shop. I bet some of them would be willing to organize "
                     "resource runs with you if you ask.");

        case  TALK_EVAC_GUARD2_TRADE:
            return _("You are asking the wrong person, should look for our "
                     "merchant by the main entrance. Perhaps one of the scavengers "
                     "is also interested.");

        case TALK_OLD_GUARD_REP:
            return _("Citizen...");

        case TALK_OLD_GUARD_REP_NEW:
             return _("I'm the region's federal liaison.  Most people here call us the 'Old Guard' and I rather like the sound of it.  "
                      "Despite how things currently appear, the federal government was not entirely destroyed.  After the outbreak I was "
                      "chosen to coordinate civilian and militia efforts in support of military operations.");

        case TALK_OLD_GUARD_REP_NEW_DOING:
             return _("I ensure that the citizens here have what they need to survive and protect themselves from raiders.  Keeping "
                      "some form law is going to be the most important  element in rebuilding the world.  We do what we can to keep the "
                      "'Free Merchants' here prospering and in return they have provided us with spare men and supplies when they can.");

        case TALK_OLD_GUARD_REP_NEW_DOWNSIDE:
             return _("Well... I was like any other civilian till they conscripted me so I'll tell it to you straight.  They're the "
                      " best hope we got right now.  They are stretched impossibly thin but are willing to do what is needed to maintain "
                      "order.  They don't care much about looters since they understand most everyone is dead, but if you have something "
                      "they need... you WILL give it to them.  Since most survivors have have nothing they want, they are welcomed as champions.");

        case TALK_OLD_GUARD_REP_WORLD:
             return _("There isn't much pushed out by public relations that I'd actually believe.  From what I gather, communication "
                      "between the regional force commands is almost non-existent.  What I do know is that the 'Old Guard' is currently "
                      "based out of the 2nd Fleet and patrols the Atlantic coast trying to provide support to the remaining footholds.");

        case TALK_OLD_GUARD_REP_WORLD_2NDFLEET:
             return _("I don't know much about how it formed but it is the armada of military and commercial ships that's floating off the "
                      "coast.  They have everything from supertankers and carriers to fishing trawlers... even a few NATO ships.  Most civilians "
                      "are offered a cabin on one of the liners to retire to if they serve as a federal employee for a few years.");

        case TALK_OLD_GUARD_REP_WORLD_FOOTHOLDS:
             return _("They may just be propaganda but apparently one or two cities were successful in 'walling themselves off.' Around "
                      "here I was told that there were a few places like this one but I couldn't tell you where.");

        case TALK_OLD_GUARD_REP_ASK_JOIN:
             return _("You can't actually join unless you go through a recruiter.  We can usually use help though, ask me from time to time "
                      "if there is any work available.  Completing missions as a contractor is a great way to make a name for yourself among "
                      "the most powerful men left in the world.");

        case TALK_ARSONIST:
            return _("Heh, you look important.");

        case TALK_ARSONIST_NEW:
            return _("Guess that makes two of us. Well, kind of. I don't think we're open, though. Full up as hell; it's almost a crowd "
                     "downstairs. See the trader over there? There's the one to ask.");

        case TALK_ARSONIST_DOING:
            return _("I burn down buildings and sell the Free Merchants the materials. No, seriously. If you've seen burned "
                     "wreckage in place of suburbs or even see the pile of rebar for sale, that's probably me. They've kept "
                     "me well off in exchange, I guess. I'll sell you a Molotov Cocktail or two, if you want.");

        case TALK_ARSONIST_DOING_REBAR:
            return _("Well, there's a guy downstairs who got a working pneumatic cannon. It shoots metal like... like a "
                     "cannon without the bang. Cost-efficient as hell. And there's no shortage of improvised weapons you "
                     "can make.  The big thing though, seems to be continuing construction of fortifications.  Very few of "
                     "those monsters seem to be able to break through a fence or wall constructed with the stuff.");

        case TALK_ARSONIST_WORLD:
            return _("Nothing optimistic, at least. Had a pal on the road with a ham radio, but she's gone and so is that "
                     "thing. Kaput.");

        case TALK_ARSONIST_WORLD_OPTIMISTIC:
            return _("Most of the emergency camps have dissolved by now. The cities are mobbed, the forests crawling with "
                     "glowing eyes and zombies. Some insane shit out there, and everyone with a radio seems to feel like "
                     "documenting their last awful moments.");

        case TALK_ARSONIST_JOIN:
            return _("I don't know. I mean, if you can make yourself useful. But that's become a real hazy thing nowadays."
                     " It depends who you ask. The merchant definitely doesn't want me here when I'm not selling, but... "
                     "some people get away with it.");

        case TALK_ARSONIST_MUTATION:
            return _("Ssh. Some people in here hate... mutations. This was an accident.");

        case TALK_ARSONIST_MUTATION_INSULT:
            return _("Screw You!");

        case TALK_SCAVENGER_MERC:
             return _("...");

        case TALK_SCAVENGER_MERC_NEW:
             return _("I'm just a hired hand.  Someone pays me and I do what needs to be done.");

        case TALK_SCAVENGER_MERC_TIPS:
             return _("If you have to fight your way out of an ambush, the only thing that is going to save you is having a party that can "
                      "return fire.  People who work alone are easy pickings for monsters and bandits.");

        case TALK_SCAVENGER_MERC_HIRE:
             return _("I'm currently waiting for a customer to return... I'll make you a deal though, "
                      " $8,000 will cover my expenses if I get a small cut of the loot.");

        case TALK_SCAVENGER_MERC_HIRE_SUCCESS:
             return _("I guess you're the boss.");

        case TALK_SHELTER:
            switch (rng(1, 2)) {
                case 1: return _("Well, I guess it's just us.");
                case 2: return _("At least we've got shelter.");
            }

        case TALK_SHELTER_PLANS:
            return _("I don't know, look for supplies and other survivors I guess.");

        case TALK_SHARE_EQUIPMENT:
            if (p->has_disease(_("asked_for_item"))) {
                return _("You just asked me for stuff; ask later.");
            }
            return _("Why should I share my equipment with you?");

        case TALK_GIVE_EQUIPMENT:
            return _("Okay, here you go.");

        case TALK_DENY_EQUIPMENT:
            if (p->op_of_u.anger >= p->hostile_anger_level() - 4) {
                return _("<no>, and if you ask again, <ill_kill_you>!");
            } else {
                return _("<no><punc> <fuck_you>!");
            }

        case TALK_TRAIN:
            {
            if (g->u.backlog.type == ACT_TRAIN) {
                return _("Shall we resume?");
            }
            std::vector<Skill*> trainable = p->skills_offered_to(&(g->u));
            std::vector<matype_id> styles = p->styles_offered_to(&(g->u));
            if (trainable.empty() && styles.empty()) {
                return _("Sorry, but it doesn't seem I have anything to teach you.");
            } else {
                return _("Here's what I can teach you...");
            }
            }
            break;

        case TALK_TRAIN_START:
            if (g->cur_om->is_safe(g->om_location().x, g->om_location().y, g->levz)) {
                return _("Alright, let's begin.");
            } else {
                return _("It's not safe here.  Let's get to safety first.");
            }
            break;

        case TALK_TRAIN_FORCE:
            return _("Alright, let's begin.");

        case TALK_SUGGEST_FOLLOW:
            if (p->has_disease(_("infection"))) {
                return _("Not until I get some antibiotics...");
            }
            if (p->has_disease(_("asked_to_follow"))) {
                return _("You asked me recently; ask again later.");
            }
            return _("Why should I travel with you?");

        case TALK_AGREE_FOLLOW:
            return _("You got it, I'm with you!");

        case TALK_DENY_FOLLOW:
            return _("Yeah... I don't think so.");

        case TALK_LEADER:
            return _("What is it?");

        case TALK_LEAVE:
            return _("You're really leaving?");

        case TALK_PLAYER_LEADS:
            return _("Alright.  You can lead now.");

        case TALK_LEADER_STAYS:
            return _("No.  I'm the leader here.");

        case TALK_HOW_MUCH_FURTHER:
            {
            // TODO: this ignores the z-component
            const tripoint player_pos = g->om_global_location();
            int dist = rl_dist(player_pos.x, player_pos.y, p->goal.x, p->goal.y);
            std::stringstream response;
            dist *= 100;
            if (dist >= 1300) {
            int miles = dist / 25; // *100, e.g. quarter mile is "25"
            miles -= miles % 25; // Round to nearest quarter-mile
            int fullmiles = (miles - miles % 100) / 100; // Left of the decimal point
            if (fullmiles <= 0) {
                fullmiles = 0;
            }
            response << string_format(_("%d.%d miles."), fullmiles, miles);
            } else {
                response << string_format(ngettext("%d foot.","%d feet.",dist), dist);
            }
            return response.str();
            }

        case TALK_FRIEND:
            return _("What is it?");

        case TALK_FRIEND_GUARD:
            return _("I'm on watch.");

        case TALK_DENY_GUARD:
            return _("Not a bloody chance, I'm going to get left behind!");

        case TALK_COMBAT_COMMANDS:
            {
            std::stringstream status;
            // Prepending * makes this an action, not a phrase
            switch (p->combat_rules.engagement) {
                case ENGAGE_NONE:  status << _("*is not engaging enemies.");         break;
                case ENGAGE_CLOSE: status << _("*is engaging nearby enemies.");      break;
                case ENGAGE_WEAK:  status << _("*is engaging weak enemies.");        break;
                case ENGAGE_HIT:   status << _("*is engaging enemies you attack.");  break;
                case ENGAGE_ALL:   status << _("*is engaging all enemies.");         break;
            }
            std::string npcstr = rm_prefix(p->male ? _("<npc>He") : _("<npc>She"));
            if (p->combat_rules.use_guns) {
                if (p->combat_rules.use_silent) {
                    status << string_format(_(" %s will use silenced firearms."), npcstr.c_str());
                } else {
                    status << string_format(_(" %s will use firearms."), npcstr.c_str());
                }
            } else {
                status << string_format(_(" %s will not use firearms."), npcstr.c_str());
            }
            if (p->combat_rules.use_grenades) {
                status << string_format(_(" %s will use grenades."), npcstr.c_str());
            } else {
                status << string_format(_(" %s will not use grenades."), npcstr.c_str());
            }

            return status.str();
            }

        case TALK_COMBAT_ENGAGEMENT:
            return _("What should I do?");

        case TALK_STRANGER_NEUTRAL:
            if (p->myclass == NC_TRADER) {
                return _("Hello!  Would you care to see my goods?");
            }
            return _("Hello there.");

        case TALK_STRANGER_WARY:
            return _("Okay, no sudden movements...");

        case TALK_STRANGER_SCARED:
            return _("Keep your distance!");

        case TALK_STRANGER_FRIENDLY:
            if (p->myclass == NC_TRADER) {
                return _("Hello!  Would you care to see my goods?");
            }
            return _("Hey there, <name_g>.");

        case TALK_STRANGER_AGGRESSIVE:
            if (!g->u.unarmed_attack()) {
                return "<drop_it>";
            } else {
                return _("This is my territory, <name_b>.");
            }

        case TALK_MUG:
        if (!g->u.unarmed_attack()) {
            return "<drop_it>";
        } else {
            return "<hands_up>";
        }

        case TALK_DESCRIBE_MISSION:
            switch (p->mission) {
                case NPC_MISSION_RESCUE_U:
                    return _("I'm here to save you!");
                case NPC_MISSION_SHELTER:
                    return _("I'm holing up here for safety.");
                case NPC_MISSION_SHOPKEEP:
                    return _("I run the shop here.");
                case NPC_MISSION_MISSING:
                    return _("Well, I was lost, but you found me...");
                case NPC_MISSION_KIDNAPPED:
                    return _("Well, I was kidnapped, but you saved me...");
                case NPC_MISSION_BASE:
                    return _("I'm guarding this location.");
                case NPC_MISSION_NULL:
                    switch (p->myclass) {
                        case NC_SHOPKEEP:
                            return _("I'm a local shopkeeper.");
                        case NC_HACKER:
                            return _("I'm looking for some choice systems to hack.");
                        case NC_DOCTOR:
                            return _("I'm looking for wounded to help.");
                        case NC_TRADER:
                            return _("I'm collecting gear and selling it.");
                        case NC_NINJA: // TODO: implement this
                            return _("I'm a wandering master of martial arts but I'm currently not implemented in the code.");
                        case NC_COWBOY:
                            return _("Just looking for some wrongs to right.");
                        case NC_SCIENTIST:
                            return _("I'm looking for clues concerning these monsters' origins...");
                        case NC_BOUNTY_HUNTER:
                            return _("I'm a killer for hire.");
                        case NC_NONE:
                            return _("I'm just wandering.");
                        default:
                            return "ERROR: Someone forgot to code an npc_class text.";
                    } // switch (p->myclass)
                default:
                    return "ERROR: Someone forgot to code an npc_mission text.";
            } // switch (p->mission)
            break;

        case TALK_WEAPON_DROPPED:
            {
            std::string npcstr = rm_prefix(p->male ? _("<npc>his") : _("<npc>her"));
            return string_format(_("*drops %s weapon."), npcstr.c_str());
            }

        case TALK_DEMAND_LEAVE:
            return _("Now get out of here, before I kill you.");

        case TALK_SIZE_UP:
            {
            int ability = g->u.per_cur * 3 + g->u.int_cur;
            if (ability <= 10) {
                return "&You can't make anything out.";
            }

            if (ability > 100) {
                ability = 100;
            }

            std::stringstream info;
            info << "&";
            int str_range = int(100 / ability);
            int str_min = int(p->str_max / str_range) * str_range;
            info << _("Str ") << str_min << " - " << str_min + str_range;

            if (ability >= 40) {
                int dex_range = int(160 / ability);
                int dex_min = int(p->dex_max / dex_range) * dex_range;
                info << _("  Dex ") << dex_min << " - " << dex_min + dex_range;
            }

            if (ability >= 50) {
                int int_range = int(200 / ability);
                int int_min = int(p->int_max / int_range) * int_range;
                info << _("  Int ") << int_min << " - " << int_min + int_range;
            }

            if (ability >= 60) {
                int per_range = int(240 / ability);
                int per_min = int(p->per_max / per_range) * per_range;
                info << _("  Per ") << per_min << " - " << per_min + per_range;
            }

            return info.str();
            }
            break;

        case TALK_LOOK_AT:
            {
            std::stringstream look;
            look << "&" << p->short_description();
            return look.str();
            }
            break;

        case TALK_OPINION:
            {
            std::stringstream opinion;
            opinion << "&" << p->opinion_text();
            return opinion.str();
            }
            break;

    }

    return "I don't know what to say. (BUG (npctalk.cpp:dynamic_line))";
}

std::vector<talk_response> gen_responses(talk_topic topic, npc *p)
{
 std::vector<talk_response> ret;
 int selected = p->chatbin.mission_selected;
 mission *miss = NULL;
 if (selected != -1 && selected < p->chatbin.missions_assigned.size())
  miss = g->find_mission( p->chatbin.missions_assigned[selected] );

 switch (topic) {
 case TALK_GUARD:
  RESPONSE(_("Don't mind me..."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_MISSION_LIST:
  if (p->chatbin.missions.empty()) {
   RESPONSE(_("Oh, okay."));
    SUCCESS(TALK_NONE);
  } else if (p->chatbin.missions.size() == 1) {
   SELECT_MISS(_("Tell me about it."), 0);
    SUCCESS(TALK_MISSION_OFFER);
   RESPONSE(_("Never mind, I'm not interested."));
    SUCCESS(TALK_NONE);
  } else {
   for (int i = 0; i < p->chatbin.missions.size(); i++) {
    SELECT_MISS(g->find_mission_type( p->chatbin.missions[i] )->name, i);
     SUCCESS(TALK_MISSION_OFFER);
   }
   RESPONSE(_("Never mind, I'm not interested."));
    SUCCESS(TALK_NONE);
  }
  break;

 case TALK_MISSION_LIST_ASSIGNED:
  if (p->chatbin.missions_assigned.empty()) {
   RESPONSE(_("Never mind then."));
    SUCCESS(TALK_NONE);
  } else if (p->chatbin.missions_assigned.size() == 1) {
   SELECT_MISS(_("I have news."), 0);
    SUCCESS(TALK_MISSION_INQUIRE);
   RESPONSE(_("Never mind."));
    SUCCESS(TALK_NONE);
  } else {
   for (int i = 0; i < p->chatbin.missions_assigned.size(); i++) {
    SELECT_MISS(g->find_mission_type( p->chatbin.missions_assigned[i] )->name,
                i);
     SUCCESS(TALK_MISSION_INQUIRE);
   }
   RESPONSE(_("Never mind."));
    SUCCESS(TALK_NONE);
  }
  break;

 case TALK_MISSION_DESCRIBE:
  RESPONSE(_("What's the matter?"));
   SUCCESS(TALK_MISSION_OFFER);
  RESPONSE(_("I don't care."));
   SUCCESS(TALK_MISSION_REJECTED);
  break;

 case TALK_MISSION_OFFER:
  RESPONSE(_("I'll do it!"));
   SUCCESS(TALK_MISSION_ACCEPTED);
   SUCCESS_ACTION(&talk_function::assign_mission);
  RESPONSE(_("Not interested."));
   SUCCESS(TALK_MISSION_REJECTED);
  break;

 case TALK_MISSION_ACCEPTED:
  RESPONSE(_("Not a problem."));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Got any advice?"));
   SUCCESS(TALK_MISSION_ADVICE);
  RESPONSE(_("Can you share some equipment?"));
   SUCCESS(TALK_SHARE_EQUIPMENT);
  RESPONSE(_("I'll be back soon!"));
   SUCCESS(TALK_DONE);
  break;

 case TALK_MISSION_ADVICE:
  RESPONSE(_("Sounds good, thanks."));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Sounds good.  Bye!"));
   SUCCESS(TALK_DONE);
  break;

 case TALK_MISSION_REJECTED:
  RESPONSE(_("I'm sorry."));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Whatever.  Bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_MISSION_INQUIRE: {
  int id = p->chatbin.missions_assigned[ p->chatbin.mission_selected ];
  if (g->mission_failed(id)) {
   RESPONSE(_("I'm sorry... I failed."));
    SUCCESS(TALK_MISSION_FAILURE);
     SUCCESS_OPINION(-1, 0, -1, 1, 0);
   RESPONSE(_("Not yet."));
    TRIAL(TALK_TRIAL_LIE, 10 + p->op_of_u.trust * 3);
    SUCCESS(TALK_NONE);
    FAILURE(TALK_MISSION_FAILURE);
     FAILURE_OPINION(-3, 0, -1, 2, 0);
  } else if (!g->mission_complete(id, p->getID())) {
   mission_type *type = g->find_mission_type(id);
   RESPONSE(_("Not yet."));
    SUCCESS(TALK_NONE);
   if (type->goal == MGOAL_KILL_MONSTER) {
    RESPONSE(_("Yup, I killed it."));
    TRIAL(TALK_TRIAL_LIE, 10 + p->op_of_u.trust * 5);
    SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    FAILURE(TALK_MISSION_SUCCESS_LIE);
     FAILURE_OPINION(-5, 0, -1, 5, 0);
     FAILURE_ACTION(&talk_function::mission_failure);
   }
   RESPONSE(_("No.  I'll get back to it, bye!"));
    SUCCESS(TALK_DONE);
  } else {
// TODO: Lie about mission
   mission_type *type = g->find_mission_type(id);
   switch (type->goal) {
   case MGOAL_FIND_ITEM:
   case MGOAL_FIND_ANY_ITEM:
    RESPONSE(_("Yup!  Here it is!"));
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   case MGOAL_GO_TO_TYPE:
    RESPONSE(_("We're here!"));
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   case MGOAL_GO_TO:
   case MGOAL_FIND_NPC:
    RESPONSE(_("Here I am."));
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   case MGOAL_FIND_MONSTER:
    RESPONSE(_("Here it is!"));
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   case MGOAL_KILL_MONSTER:
    RESPONSE(_("I killed it."));
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   case MGOAL_RECRUIT_NPC:
   case MGOAL_RECRUIT_NPC_CLASS:
    RESPONSE(_("I brought'em."));
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   default:
    RESPONSE(_("Mission success!  I don't know what else to say."));
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   }
  }
 } break;

 case TALK_MISSION_SUCCESS:
  RESPONSE(_("Glad to help.  I need no payment."));
   SUCCESS(TALK_NONE);
   SUCCESS_OPINION(miss->value / (OWED_VAL * 4), -1,
                   miss->value / (OWED_VAL * 2), -1, 0 - miss->value);
   SUCCESS_ACTION(&talk_function::clear_mission);
  RESPONSE(_("How about some items as payment?"));
   SUCCESS(TALK_MISSION_REWARD);
   SUCCESS_ACTION(&talk_function::mission_reward);
  if(!p->skills_offered_to(&(g->u)).empty() || !p->styles_offered_to(&(g->u)).empty()) {
  SELECT_TEMP(_("Maybe you can teach me something as payment."), 0);
   SUCCESS(TALK_TRAIN);
   SUCCESS_ACTION(&talk_function::clear_mission);
  }
  RESPONSE(_("Alright, well, you owe me one."));
   SUCCESS(TALK_NONE);
   SUCCESS_ACTION(&talk_function::clear_mission);
  RESPONSE(_("Glad to help.  I need no payment.  Bye!"));
   SUCCESS(TALK_DONE);
   SUCCESS_ACTION(&talk_function::clear_mission);
   SUCCESS_OPINION(p->op_of_u.owed / (OWED_VAL * 4), -1,
                   p->op_of_u.owed / (OWED_VAL * 2), -1, 0 - miss->value);
  break;

 case TALK_MISSION_SUCCESS_LIE:
  RESPONSE(_("Well, um, sorry."));
   SUCCESS(TALK_NONE);
   SUCCESS_ACTION(&talk_function::clear_mission);

 case TALK_MISSION_FAILURE:
  RESPONSE(_("I'm sorry.  I did what I could."));
   SUCCESS(TALK_NONE);
  break;

 case TALK_MISSION_REWARD:
  RESPONSE(_("Thank you."));
   SUCCESS(TALK_NONE);
   SUCCESS_ACTION(&talk_function::clear_mission);
  RESPONSE(_("Thanks, bye."));
   SUCCESS(TALK_DONE);
   SUCCESS_ACTION(&talk_function::clear_mission);
  break;

 case TALK_EVAC_MERCHANT:
  RESPONSE(_("I'm actually new..."));
   SUCCESS(TALK_EVAC_MERCHANT_NEW);
  RESPONSE(_("What are you doing here?"));
   SUCCESS(TALK_EVAC_MERCHANT_PLANS);
  RESPONSE(_("Heard anything about the outside world?"));
   SUCCESS(TALK_EVAC_MERCHANT_WORLD);
  RESPONSE(_("Is there any way I can join your group?"));
   SUCCESS(TALK_EVAC_MERCHANT_ASK_JOIN);
  RESPONSE(_("Can I do anything for the center?"));
   SUCCESS(TALK_MISSION_LIST);
  if (p->chatbin.missions_assigned.size() == 1) {
   RESPONSE(_("About that job..."));
    SUCCESS(TALK_MISSION_INQUIRE);
  } else if (p->chatbin.missions_assigned.size() >= 2) {
   RESPONSE(_("About one of those jobs..."));
    SUCCESS(TALK_MISSION_LIST_ASSIGNED);
  }
  RESPONSE(_("Let's trade then."));
   SUCCESS_ACTION(&talk_function::start_trade);
   SUCCESS(TALK_EVAC_MERCHANT);
  RESPONSE(_("Well, bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_EVAC_MERCHANT_NEW:
  RESPONSE(_("No rest for the weary..."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;
 case TALK_EVAC_MERCHANT_PLANS:
  RESPONSE(_("It's just as bad out here, if not worse."));
   SUCCESS(TALK_EVAC_MERCHANT_PLANS2);
  break;
 case TALK_EVAC_MERCHANT_PLANS2:
  if (g->u.int_cur >= 12){
    RESPONSE(_("[INT 12] Wait, six buses and refugees... how many people do you still have crammed in here?"));
        SUCCESS(TALK_EVAC_MERCHANT_PLANS3);
  }
  RESPONSE(_("Guess shit's a mess everywhere..."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;
 case TALK_EVAC_MERCHANT_PLANS3:
  RESPONSE(_("Guess it works for you..."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;
 case TALK_EVAC_MERCHANT_HORDES:
  RESPONSE(_("Thanks for the tip."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;
 case TALK_EVAC_MERCHANT_PRIME_LOOT:
  RESPONSE(_("Thanks, I'll keep an eye out."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;
 case TALK_EVAC_MERCHANT_NO:
  RESPONSE(_("Fine..."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;
 case TALK_EVAC_MERCHANT_HELL_NO:
  RESPONSE(_("Fine... *coughupyourscough*"));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;

 case TALK_EVAC_MERCHANT_ASK_JOIN:
  if (g->u.int_cur > 10){
    RESPONSE(_("[INT 11] I'm sure I can organize salvage operations to increase the bounty scavengers bring in!"));
        SUCCESS(TALK_EVAC_MERCHANT_NO);
  }
  if (g->u.int_cur <= 6 && g->u.str_cur > 10){
    RESPONSE(_("[STR 11] I punch things in face real good!"));
        SUCCESS(TALK_EVAC_MERCHANT_NO);
  }
  RESPONSE(_("I'm sure I can do something to change your mind *wink*"));
   SUCCESS(TALK_EVAC_MERCHANT_HELL_NO);
  RESPONSE(_("I can pull my own weight!"));
   SUCCESS(TALK_EVAC_MERCHANT_NO);
  RESPONSE(_("I guess I'll look somewhere else..."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;

 case TALK_EVAC_MERCHANT_WORLD:
  RESPONSE(_("Hordes?"));
   SUCCESS(TALK_EVAC_MERCHANT_HORDES);
  RESPONSE(_("Heard of anything better than the odd gun cache?"));
   SUCCESS(TALK_EVAC_MERCHANT_PRIME_LOOT);
  RESPONSE(_("Was hoping for something more..."));
   SUCCESS(TALK_EVAC_MERCHANT);
  break;

 case TALK_EVAC_GUARD1:
  RESPONSE(_("What is this place?"));
   SUCCESS(TALK_EVAC_GUARD1_PLACE);
  RESPONSE(_("Can I join you guys?"));
   SUCCESS(TALK_EVAC_GUARD1_JOIN);
  RESPONSE(_("Anything I can do for you?"));
   SUCCESS(TALK_EVAC_GUARD1_JOB);
  RESPONSE(_("See you later."));
   SUCCESS(TALK_EVAC_GUARD1_BYE);
  break;

 case TALK_EVAC_GUARD1_PLACE:
  RESPONSE(_("So are you with the government or something?"));
   SUCCESS(TALK_EVAC_GUARD1_GOVERNMENT);
  RESPONSE(_("What do you trade?"));
   SUCCESS(TALK_EVAC_GUARD1_TRADE);
  break;

 case TALK_EVAC_GUARD1_GOVERNMENT:
  RESPONSE(_("Oh, okay."));
   SUCCESS(TALK_EVAC_GUARD1);
  RESPONSE(_("Oh, okay. I'll go look for him"));
   SUCCESS(TALK_DONE);
  break;

 case TALK_EVAC_GUARD1_TRADE:
  RESPONSE(_("I'll go talk to them later."));
   SUCCESS(TALK_EVAC_GUARD1);
  RESPONSE(_("Will do, thanks!"));
   SUCCESS(TALK_DONE);
  break;

 case TALK_EVAC_GUARD1_JOIN:
  RESPONSE(_("That's pretty blunt!"));
   SUCCESS(TALK_EVAC_GUARD1_JOIN2);
  break;

 case TALK_EVAC_GUARD1_JOIN2:
  RESPONSE(_("So no negotiating? No, 'If you do this quest then we'll let you in?'"));
   SUCCESS(TALK_EVAC_GUARD1_JOIN3);
  break;

 case TALK_EVAC_GUARD1_JOIN3:
  RESPONSE(_("I don't like you're attitude."));
   SUCCESS(TALK_EVAC_GUARD1_ATTITUDE);
  RESPONSE(_("Well alright then."));
   SUCCESS(TALK_EVAC_GUARD1);
  break;

 case TALK_EVAC_GUARD1_ATTITUDE:
  RESPONSE(_("I think I'd rather rearrange your face instead!"));
   SUCCESS_ACTION(&talk_function::insult_combat);
   SUCCESS(TALK_DONE);
  RESPONSE(_("I will."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_EVAC_GUARD1_JOB:
  RESPONSE(_("Alright then."));
   SUCCESS(TALK_EVAC_GUARD1);
  RESPONSE(_("Old Guard huh, I'll go talk to him!"));
   SUCCESS(TALK_DONE);
  RESPONSE(_("Who are the Old Guard?"));
   SUCCESS(TALK_EVAC_GUARD1_OLDGUARD);
  break;

 case TALK_EVAC_GUARD2:
  RESPONSE(_("I am actually new."));
   SUCCESS(TALK_EVAC_GUARD2_NEW);
  RESPONSE(_("Are there any rules I should follow while inside?"));
   SUCCESS(TALK_EVAC_GUARD2_RULES);
  RESPONSE(_("So who is everyone around here?"));
   SUCCESS(TALK_EVAC_GUARD2_WHO);
  RESPONSE(_("Lets trade!"));
   SUCCESS(TALK_EVAC_GUARD2_TRADE);
  RESPONSE(_("Is there anything I can do to help?"));
   SUCCESS(TALK_MISSION_LIST);
  if (p->chatbin.missions_assigned.size() == 1) {
   RESPONSE(_("About that job..."));
    SUCCESS(TALK_MISSION_INQUIRE);
  } else if (p->chatbin.missions_assigned.size() >= 2) {
   RESPONSE(_("About one of those jobs..."));
    SUCCESS(TALK_MISSION_LIST_ASSIGNED);
  }
  RESPONSE(_("Thanks! I will be on my way."));
   SUCCESS(TALK_DONE);
  break;

 case  TALK_EVAC_GUARD2_NEW:
  RESPONSE(_("..."));
   SUCCESS(TALK_EVAC_GUARD2);
  break;

 case TALK_EVAC_GUARD2_RULES:
  RESPONSE(_("Ok, thanks."));
   SUCCESS(TALK_EVAC_GUARD2);
  RESPONSE(_("So uhhh, why not?"));
   SUCCESS(TALK_EVAC_GUARD2_RULES_BASEMENT);
  break;

 case  TALK_EVAC_GUARD2_RULES_BASEMENT:
  RESPONSE(_("..."));
   SUCCESS(TALK_EVAC_GUARD2);
  break;

 case TALK_EVAC_GUARD2_WHO:
  RESPONSE(_("Thanks for the heads-up."));
   SUCCESS(TALK_EVAC_GUARD2);
  break;

 case TALK_EVAC_GUARD2_TRADE:
  RESPONSE(_("..."));
   SUCCESS(TALK_EVAC_GUARD2);
  break;

 case TALK_EVAC_GUARD1_OLDGUARD:
  RESPONSE(_("Whatever, I had another question."));
   SUCCESS(TALK_EVAC_GUARD1);
  RESPONSE(_("Okay, I'll go look for him then."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_EVAC_GUARD1_BYE:
  RESPONSE(_("..."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_OLD_GUARD_REP:
  RESPONSE(_("Who are you?"));
   SUCCESS(TALK_OLD_GUARD_REP_NEW);
  RESPONSE(_("Heard anything about the outside world?"));
   SUCCESS(TALK_OLD_GUARD_REP_WORLD);
  RESPONSE(_("Is there any way I can join the 'Old Guard'?"));
   SUCCESS( TALK_OLD_GUARD_REP_ASK_JOIN);
  RESPONSE(_("Does the Old Guard need anything?"));
   SUCCESS(TALK_MISSION_LIST);
  if (p->chatbin.missions_assigned.size() == 1) {
   RESPONSE(_("About that job..."));
    SUCCESS(TALK_MISSION_INQUIRE);
  } else if (p->chatbin.missions_assigned.size() >= 2) {
   RESPONSE(_("About one of those jobs..."));
    SUCCESS(TALK_MISSION_LIST_ASSIGNED);
  }
  RESPONSE(_("Well, bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_OLD_GUARD_REP_NEW:
  RESPONSE(_("So what are you actually doing here?"));
   SUCCESS(TALK_OLD_GUARD_REP_NEW_DOING);
  RESPONSE(_("Nevermind..."));
   SUCCESS(TALK_OLD_GUARD_REP);
  break;

 case TALK_OLD_GUARD_REP_NEW_DOING:
  RESPONSE(_("Is there a catch?"));
   SUCCESS(TALK_OLD_GUARD_REP_NEW_DOWNSIDE);
  RESPONSE(_("Anything more to it?"));
   SUCCESS(TALK_OLD_GUARD_REP_NEW_DOWNSIDE);
  RESPONSE(_("Nevermind..."));
   SUCCESS(TALK_OLD_GUARD_REP);
  break;
 case TALK_OLD_GUARD_REP_NEW_DOWNSIDE:
  RESPONSE(_("Hmmm..."));
   SUCCESS(TALK_OLD_GUARD_REP);
  break;

 case TALK_OLD_GUARD_REP_WORLD:
  RESPONSE(_("The 2nd Fleet?"));
   SUCCESS(TALK_OLD_GUARD_REP_WORLD_2NDFLEET);
  RESPONSE(_("Tell me about the footholds."));
   SUCCESS(TALK_OLD_GUARD_REP_WORLD_FOOTHOLDS);
  RESPONSE(_("Nevermind..."));
   SUCCESS(TALK_OLD_GUARD_REP);
  break;

 case TALK_OLD_GUARD_REP_WORLD_2NDFLEET:
  RESPONSE(_("Hmmm..."));
   SUCCESS(TALK_OLD_GUARD_REP);
  break;
 case TALK_OLD_GUARD_REP_WORLD_FOOTHOLDS:
  RESPONSE(_("Hmmm..."));
   SUCCESS(TALK_OLD_GUARD_REP);
  break;
 case TALK_OLD_GUARD_REP_ASK_JOIN:
  RESPONSE(_("Hmmm..."));
   SUCCESS(TALK_OLD_GUARD_REP);
  break;

 case TALK_ARSONIST:
  RESPONSE(_("I'm actually new."));
   SUCCESS(TALK_ARSONIST_NEW);
  RESPONSE(_("What are you doing here?"));
   SUCCESS(TALK_ARSONIST_DOING);
  RESPONSE(_("Heard anything about the outside world?"));
   SUCCESS(TALK_ARSONIST_WORLD);
  RESPONSE(_("Is there any way I can join your group?"));
   SUCCESS(TALK_ARSONIST_JOIN);
  RESPONSE(_("What's with your ears?"));
   SUCCESS(TALK_ARSONIST_MUTATION);
  RESPONSE(_("Anything I can help with?"));
   SUCCESS(TALK_MISSION_LIST);
  if (p->chatbin.missions_assigned.size() == 1) {
   RESPONSE(_("About that job..."));
    SUCCESS(TALK_MISSION_INQUIRE);
  } else if (p->chatbin.missions_assigned.size() >= 2) {
   RESPONSE(_("About one of those jobs..."));
    SUCCESS(TALK_MISSION_LIST_ASSIGNED);
  }
  RESPONSE(_("Well, bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_ARSONIST_NEW:
  RESPONSE(_("Sucks..."));
   SUCCESS(TALK_ARSONIST);
  break;
 case TALK_ARSONIST_DOING:
  RESPONSE(_("I'll buy."));
   SUCCESS_ACTION(&talk_function::start_trade);
   SUCCESS(TALK_ARSONIST);
  RESPONSE(_("Who needs rebar?"));
   SUCCESS(TALK_ARSONIST_DOING_REBAR);
  break;
 case TALK_ARSONIST_DOING_REBAR:
  RESPONSE(_("Well, then..."));
   SUCCESS(TALK_ARSONIST);
  break;
 case TALK_ARSONIST_WORLD:
  RESPONSE(_("Nothing optimistic?"));
   SUCCESS(TALK_ARSONIST_WORLD_OPTIMISTIC);
  RESPONSE(_("..."));
   SUCCESS(TALK_ARSONIST);
  break;
 case TALK_ARSONIST_WORLD_OPTIMISTIC:
  RESPONSE(_("I feel bad for asking."));
   SUCCESS(TALK_ARSONIST);
  break;
 case TALK_ARSONIST_JOIN:
  RESPONSE(_("..."));
   SUCCESS(TALK_ARSONIST);
  break;
 case TALK_ARSONIST_MUTATION:
  RESPONSE(_("Sorry to ask"));
   SUCCESS(TALK_ARSONIST);
  RESPONSE(_("You're disgusting."));
   SUCCESS(TALK_ARSONIST_MUTATION_INSULT);
  break;
 case TALK_ARSONIST_MUTATION_INSULT:
  RESPONSE(_("..."));
   SUCCESS(TALK_DONE);
    SUCCESS_OPINION(-1, -2, -1, 1, 0);
    SUCCESS_ACTION(&talk_function::end_conversation);
  break;

 case TALK_SCAVENGER_MERC:
  RESPONSE(_("Who are you?"));
   SUCCESS(TALK_SCAVENGER_MERC_NEW);
  RESPONSE(_("Any tips for surviving?"));
   SUCCESS(TALK_SCAVENGER_MERC_TIPS);
  RESPONSE(_("What would it cost to hire you?"));
   SUCCESS(TALK_SCAVENGER_MERC_HIRE);
  RESPONSE(_("Well, bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_SCAVENGER_MERC_NEW:
  RESPONSE(_("..."));
   SUCCESS(TALK_SCAVENGER_MERC);
  break;
 case TALK_SCAVENGER_MERC_TIPS:
  RESPONSE(_("I suppose I should hire a party then?"));
   SUCCESS(TALK_SCAVENGER_MERC);
  break;
 case TALK_SCAVENGER_MERC_HIRE:
  if (g->u.cash >= 800000){
  RESPONSE(_("[$8000] You have a deal."));
   g->u.cash -= 800000;
   SUCCESS(TALK_SCAVENGER_MERC_HIRE_SUCCESS);
  }
  RESPONSE(_("I might be back."));
   SUCCESS(TALK_SCAVENGER_MERC);
  break;

 case TALK_SCAVENGER_MERC_HIRE_SUCCESS:
  RESPONSE(_("Glad to have you aboard."));
   SUCCESS_ACTION(&talk_function::follow);
   SUCCESS_OPINION(1, 0, 1, 0, 0);
   SUCCESS(TALK_DONE);
  break;

 case TALK_SHELTER:
  RESPONSE(_("What should we do now?"));
   SUCCESS(TALK_SHELTER_PLANS);
  RESPONSE(_("Can I do anything for you?"));
   SUCCESS(TALK_MISSION_LIST);
  if (!p->is_following()) {
   RESPONSE(_("Want to travel with me?"));
    SUCCESS(TALK_SUGGEST_FOLLOW);
  }
  if (p->chatbin.missions_assigned.size() == 1) {
   RESPONSE(_("About that job..."));
    SUCCESS(TALK_MISSION_INQUIRE);
  } else if (p->chatbin.missions_assigned.size() >= 2) {
   RESPONSE(_("About one of those jobs..."));
    SUCCESS(TALK_MISSION_LIST_ASSIGNED);
  }
  RESPONSE(_("I can't leave the shelter without equipment..."));
   SUCCESS(TALK_SHARE_EQUIPMENT);
  RESPONSE(_("Well, bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_SHELTER_PLANS:
// TODO: Add _("follow me")
  RESPONSE(_("Hmm, okay.  Bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_SHARE_EQUIPMENT:
  if (p->has_disease(_("asked_for_item"))) {
   RESPONSE(_("Okay, fine."));
    SUCCESS(TALK_NONE);
  } else {
   int score = p->op_of_u.trust + p->op_of_u.value * 3 +
               p->personality.altruism * 2;
   int missions_value = p->assigned_missions_value();
   if (g->u.has_amount(_("mininuke"), 1)) {
   RESPONSE(_("Because I'm holding a thermal detonator!"));
    SUCCESS(TALK_GIVE_EQUIPMENT);
     SUCCESS_ACTION(&talk_function::give_equipment);
     SUCCESS_OPINION(0, 0, -1, 0, score * 300);
    FAILURE(TALK_DENY_EQUIPMENT);
     FAILURE_OPINION(0, 0, -1, 0, 0);
  }
   RESPONSE(_("Because I'm your friend!"));
    TRIAL(TALK_TRIAL_PERSUADE, 10 + score);
    SUCCESS(TALK_GIVE_EQUIPMENT);
     SUCCESS_ACTION(&talk_function::give_equipment);
     SUCCESS_OPINION(0, 0, -1, 0, score * 300);
    FAILURE(TALK_DENY_EQUIPMENT);
     FAILURE_OPINION(0, 0, -1, 0, 0);
   if (missions_value >= 1) {
    RESPONSE(_("Well, I am helping you out..."));
     TRIAL(TALK_TRIAL_PERSUADE, 12 + (.8 * score) + missions_value / OWED_VAL);
     SUCCESS(TALK_GIVE_EQUIPMENT);
      SUCCESS_ACTION(&talk_function::give_equipment);
     FAILURE(TALK_DENY_EQUIPMENT);
      FAILURE_OPINION(0, 0, -1, 0, 0);
   }
   RESPONSE(_("I'll give it back!"));
    TRIAL(TALK_TRIAL_LIE, score * 1.5);
    SUCCESS(TALK_GIVE_EQUIPMENT);
     SUCCESS_ACTION(&talk_function::give_equipment);
     SUCCESS_OPINION(0, 0, -1, 0, score * 300);
    FAILURE(TALK_DENY_EQUIPMENT);
     FAILURE_OPINION(0, -1, -1, 1, 0);
   RESPONSE(_("Give it to me, or else!"));
    TRIAL(TALK_TRIAL_INTIMIDATE, 40);
    SUCCESS(TALK_GIVE_EQUIPMENT);
     SUCCESS_ACTION(&talk_function::give_equipment);
     SUCCESS_OPINION(-3, 2, -2, 2,
                     (g->u.intimidation() + p->op_of_u.fear -
                      p->personality.bravery - p->intimidation()) * 500);
    FAILURE(TALK_DENY_EQUIPMENT);
     FAILURE_OPINION(-3, 1, -3, 5, 0);
   RESPONSE(_("Eh, never mind."));
    SUCCESS(TALK_NONE);
   RESPONSE(_("Never mind, I'll do without.  Bye."));
    SUCCESS(TALK_DONE);
  }
  break;

 case TALK_GIVE_EQUIPMENT:
  RESPONSE(_("Thank you!"));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Thanks!  But can I have some more?"));
   SUCCESS(TALK_SHARE_EQUIPMENT);
  RESPONSE(_("Thanks, see you later!"));
   SUCCESS(TALK_DONE);
  break;

 case TALK_DENY_EQUIPMENT:
  RESPONSE(_("Okay, okay, sorry."));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Seriously, give me more stuff!"));
   SUCCESS(TALK_SHARE_EQUIPMENT);
  RESPONSE(_("Okay, fine, bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_TRAIN: {
  if (g->u.backlog.type == ACT_TRAIN) {
    std::stringstream resume;
    resume << _("Yes, let's resume training ");
    Skill *skillt = Skill::skill(g->u.backlog.name);
    if(skillt == NULL) {
        resume << martialarts[g->u.backlog.name].name;
        SELECT_STYLE(resume.str(), g->u.backlog.name);
    } else {
        resume << skillt->name();
        SELECT_SKIL(resume.str(), skillt);
    }
    SUCCESS(TALK_TRAIN_START);
  }
  std::vector<matype_id> styles = p->styles_offered_to( &(g->u) );
  std::vector<Skill*> trainable = p->skills_offered_to( &(g->u) );
  if (trainable.empty() && styles.empty()) {
   RESPONSE(_("Oh, okay.")); // Nothing to learn here
    SUCCESS(TALK_NONE);
   break;
  }
  int printed = 0;
  int shift = p->chatbin.tempvalue;
  bool more = trainable.size() + styles.size() - shift > 9;
  for (int i = shift; i < trainable.size() && printed < 9; i++) {
   //shift--;
   printed++;
   Skill* trained = trainable[i];
   SELECT_SKIL(
    string_format(_("%s: %d -> %d (cost %d)"), trained->name().c_str(), static_cast<int>(g->u.skillLevel(trained)), g->u.skillLevel(trained) + 1, 200 * (g->u.skillLevel(trained) + 1)),
    trainable[i] );
    SUCCESS(TALK_TRAIN_START);
  }
  if (shift < 0)
   shift = 0;
  for (int i = 0; i < styles.size() && printed < 9; i++) {
   printed++;
   SELECT_STYLE(
    string_format(_("%s (cost 800)"), martialarts[styles[i]].name.c_str()),
    styles[i] );
    SUCCESS(TALK_TRAIN_START);
  }
  if (more) {
   SELECT_TEMP(_("More..."), shift + 9);
    SUCCESS(TALK_TRAIN);
  }
  if (shift > 0) {
   int newshift = shift - 9;
   if (newshift < 0)
    newshift = 0;
   SELECT_TEMP(_("Back..."), newshift);
    SUCCESS(TALK_TRAIN);
  }
  RESPONSE(_("Eh, never mind."));
   SUCCESS(TALK_NONE);
  } break;

 case TALK_TRAIN_START:
  if (g->cur_om->is_safe(g->om_location().x, g->om_location().y, g->levz)) {
   RESPONSE(_("Sounds good."));
    SUCCESS(TALK_DONE);
    SUCCESS_ACTION(&talk_function::start_training);
   RESPONSE(_("On second thought, never mind."));
    SUCCESS(TALK_NONE);
  } else {
   RESPONSE(_("Okay.  Lead the way."));
    SUCCESS(TALK_DONE);
    SUCCESS_ACTION(&talk_function::lead_to_safety);
   RESPONSE(_("No, we'll be okay here."));
    SUCCESS(TALK_TRAIN_FORCE);
   RESPONSE(_("On second thought, never mind."));
    SUCCESS(TALK_NONE);
  }
  break;

 case TALK_TRAIN_FORCE:
  RESPONSE(_("Sounds good."));
   SUCCESS(TALK_DONE);
   SUCCESS_ACTION(&talk_function::start_training);
  RESPONSE(_("On second thought, never mind."));
   SUCCESS(TALK_NONE);
  break;

 case TALK_SUGGEST_FOLLOW:
  if (p->has_disease(_("infection"))) {
   RESPONSE(_("Understood.  I'll get those antibiotics."));
    SUCCESS(TALK_NONE);
  } else if (p->has_disease(_("asked_to_follow"))) {
   RESPONSE(_("Right, right, I'll ask later."));
    SUCCESS(TALK_NONE);
  } else {
   int strength = 3 * p->op_of_u.fear + p->op_of_u.value + p->op_of_u.trust +
                  (10 - p->personality.bravery);
   int weakness = 3 * p->personality.altruism + p->personality.bravery -
                  p->op_of_u.fear + p->op_of_u.value;
   int friends = 2 * p->op_of_u.trust + 2 * p->op_of_u.value -
                 2 * p->op_of_u.anger + p->op_of_u.owed / 50;
   RESPONSE(_("I can keep you safe."));
    TRIAL(TALK_TRIAL_PERSUADE, strength * 2);
    SUCCESS(TALK_AGREE_FOLLOW);
     SUCCESS_ACTION(&talk_function::follow);
     SUCCESS_OPINION(1, 0, 1, 0, 0);
    FAILURE(TALK_DENY_FOLLOW);
     FAILURE_ACTION(&talk_function::deny_follow);
     FAILURE_OPINION(0, 0, -1, 1, 0);
   RESPONSE(_("You can keep me safe."));
    TRIAL(TALK_TRIAL_PERSUADE, weakness * 2);
    SUCCESS(TALK_AGREE_FOLLOW);
     SUCCESS_ACTION(&talk_function::follow);
     SUCCESS_OPINION(0, 0, -1, 0, 0);
    FAILURE(TALK_DENY_FOLLOW);
     FAILURE_ACTION(&talk_function::deny_follow);
     FAILURE_OPINION(0, -1, -1, 1, 0);
   RESPONSE(_("We're friends, aren't we?"));
    TRIAL(TALK_TRIAL_PERSUADE, friends * 1.5);
    SUCCESS(TALK_AGREE_FOLLOW);
     SUCCESS_ACTION(&talk_function::follow);
     SUCCESS_OPINION(2, 0, 0, -1, 0);
    FAILURE(TALK_DENY_FOLLOW);
     FAILURE_ACTION(&talk_function::deny_follow);
     FAILURE_OPINION(-1, -2, -1, 1, 0);
   RESPONSE(_("!I'll kill you if you don't."));
    TRIAL(TALK_TRIAL_INTIMIDATE, strength * 2);
    SUCCESS(TALK_AGREE_FOLLOW);
     SUCCESS_ACTION(&talk_function::follow);
     SUCCESS_OPINION(-4, 3, -1, 4, 0);
    FAILURE(TALK_DENY_FOLLOW);
     FAILURE_OPINION(-4, 0, -5, 10, 0);
  }
  break;

 case TALK_AGREE_FOLLOW:
  RESPONSE(_("Awesome!"));
   SUCCESS(TALK_FRIEND);
  RESPONSE(_("Okay, let's go!"));
   SUCCESS(TALK_DONE);
  break;

 case TALK_DENY_FOLLOW:
  RESPONSE(_("Oh, okay."));
   SUCCESS(TALK_NONE);
  break;

 case TALK_LEADER: {
  int persuade = p->op_of_u.fear + p->op_of_u.value + p->op_of_u.trust -
                 p->personality.bravery - p->personality.aggression;
  if (p->has_destination()) {
   RESPONSE(_("How much further?"));
    SUCCESS(TALK_HOW_MUCH_FURTHER);
  }
  RESPONSE(_("I'm going to go my own way for a while."));
   SUCCESS(TALK_LEAVE);
  if (!p->has_disease(_("asked_to_lead"))) {
   RESPONSE(_("I'd like to lead for a while."));
    TRIAL(TALK_TRIAL_PERSUADE, persuade);
    SUCCESS(TALK_PLAYER_LEADS);
     SUCCESS_ACTION(&talk_function::follow);
    FAILURE(TALK_LEADER_STAYS);
     FAILURE_OPINION(0, 0, -1, -1, 0);
   RESPONSE(_("Step aside.  I'm leader now."));
    TRIAL(TALK_TRIAL_INTIMIDATE, 40);
    SUCCESS(TALK_PLAYER_LEADS);
     SUCCESS_ACTION(&talk_function::follow);
     SUCCESS_OPINION(-1, 1, -1, 1, 0);
    FAILURE(TALK_LEADER_STAYS);
     FAILURE_OPINION(-1, 0, -1, 1, 0);
  }
  RESPONSE(_("Can I do anything for you?"));
   SUCCESS(TALK_MISSION_LIST);
  RESPONSE(_("Let's trade items."));
   SUCCESS(TALK_NONE);
   SUCCESS_ACTION(&talk_function::start_trade);
  RESPONSE(_("Let's go."));
   SUCCESS(TALK_DONE);
 } break;

 case TALK_LEAVE:
  RESPONSE(_("Nah, I'm just kidding."));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Yeah, I'm sure.  Bye."));
   SUCCESS_ACTION(&talk_function::leave);
   SUCCESS(TALK_DONE);
  break;

 case TALK_PLAYER_LEADS:
  RESPONSE(_("Good.  Something else..."));
   SUCCESS(TALK_FRIEND);
  RESPONSE(_("Alright, let's go."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_LEADER_STAYS:
  RESPONSE(_("Okay, okay."));
   SUCCESS(TALK_NONE);
  break;

 case TALK_HOW_MUCH_FURTHER:
  RESPONSE(_("Okay, thanks."));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Let's keep moving."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_FRIEND_GUARD:
  RESPONSE(_("I need you to come with me."));
    SUCCESS(TALK_FRIEND);
    SUCCESS_ACTION(&talk_function::stop_guard);
  RESPONSE(_("See you around."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_FRIEND:
  RESPONSE(_("Combat commands..."));
   SUCCESS(TALK_COMBAT_COMMANDS);
  RESPONSE(_("Can I do anything for you?"));
   SUCCESS(TALK_MISSION_LIST);
  SELECT_TEMP(_("Can you teach me anything?"), 0);
   SUCCESS(TALK_TRAIN);
  RESPONSE(_("Let's trade items."));
   SUCCESS(TALK_NONE);
   SUCCESS_ACTION(&talk_function::start_trade);
  if (p->is_following() && g->m.camp_at(g->u.posx, g->u.posy)) {
   RESPONSE(_("Wait at this base."));
    SUCCESS(TALK_DONE);
    SUCCESS_ACTION(&talk_function::assign_base);
  }
  if (p->is_following()) {//acidia
   RESPONSE(_("Guard this position."));
    int loyalty = 3 * p->op_of_u.trust + 1 * p->op_of_u.value -
                 1 * p->op_of_u.anger + p->op_of_u.owed / 50;
    TRIAL(TALK_TRIAL_PERSUADE, loyalty * 2);
    SUCCESS(TALK_FRIEND_GUARD);
    SUCCESS_ACTION(&talk_function::assign_guard);
    FAILURE(TALK_DENY_GUARD);
     FAILURE_OPINION(-1, -2, -1, 1, 0);
  }
  RESPONSE(_("I'm going to go my own way for a while."));
   SUCCESS(TALK_LEAVE);
  RESPONSE(_("Let's go."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_COMBAT_COMMANDS: {
  RESPONSE(_("Change your engagement rules..."));
   SUCCESS(TALK_COMBAT_ENGAGEMENT);
  if (p->combat_rules.use_guns) {
   RESPONSE(_("Don't use guns anymore."));
    SUCCESS(TALK_COMBAT_COMMANDS);
    SUCCESS_ACTION(&talk_function::toggle_use_guns);
  } else {
   RESPONSE(_("You can use guns."));
    SUCCESS(TALK_COMBAT_COMMANDS);
    SUCCESS_ACTION(&talk_function::toggle_use_guns);
  }
  if (p->combat_rules.use_silent) {
   RESPONSE(_("Don't worry about noise."));
    SUCCESS(TALK_COMBAT_COMMANDS);
    SUCCESS_ACTION(&talk_function::toggle_use_silent);
  } else {
   RESPONSE(_("Use only silent weapons."));
    SUCCESS(TALK_COMBAT_COMMANDS);
    SUCCESS_ACTION(&talk_function::toggle_use_silent);
  }
  if (p->combat_rules.use_grenades) {
   RESPONSE(_("Don't use grenades anymore."));
    SUCCESS(TALK_COMBAT_COMMANDS);
    SUCCESS_ACTION(&talk_function::toggle_use_grenades);
  } else {
   RESPONSE(_("You can use grenades."));
    SUCCESS(TALK_COMBAT_COMMANDS);
    SUCCESS_ACTION(&talk_function::toggle_use_grenades);
  }
  RESPONSE(_("Never mind."));
   SUCCESS(TALK_NONE);
 } break;

 case TALK_COMBAT_ENGAGEMENT: {
  if (p->combat_rules.engagement != ENGAGE_NONE) {
   RESPONSE(_("Don't fight unless your life depends on it."));
    SUCCESS(TALK_NONE);
    SUCCESS_ACTION(&talk_function::set_engagement_none);
  }
  if (p->combat_rules.engagement != ENGAGE_CLOSE) {
   RESPONSE(_("Attack enemies that get too close."));
    SUCCESS(TALK_NONE);
    SUCCESS_ACTION(&talk_function::set_engagement_close);
  }
  if (p->combat_rules.engagement != ENGAGE_WEAK) {
   RESPONSE(_("Attack enemies that you can kill easily."));
    SUCCESS(TALK_NONE);
    SUCCESS_ACTION(&talk_function::set_engagement_weak);
  }
  if (p->combat_rules.engagement != ENGAGE_HIT) {
   RESPONSE(_("Attack only enemies that I attack first."));
    SUCCESS(TALK_NONE);
    SUCCESS_ACTION(&talk_function::set_engagement_hit);
  }
  if (p->combat_rules.engagement != ENGAGE_ALL) {
   RESPONSE(_("Attack anything you want."));
    SUCCESS(TALK_NONE);
    SUCCESS_ACTION(&talk_function::set_engagement_all);
  }
  RESPONSE(_("Never mind."));
   SUCCESS(TALK_NONE);
 } break;

 case TALK_STRANGER_NEUTRAL:
 case TALK_STRANGER_WARY:
 case TALK_STRANGER_SCARED:
 case TALK_STRANGER_FRIENDLY:
  if (topic == TALK_STRANGER_NEUTRAL || topic == TALK_STRANGER_FRIENDLY) {
   RESPONSE(_("Another survivor!  We should travel together."));
    SUCCESS(TALK_SUGGEST_FOLLOW);
   RESPONSE(_("What are you doing?"));
    SUCCESS(TALK_DESCRIBE_MISSION);
   RESPONSE(_("Care to trade?"));
    SUCCESS(TALK_NONE);
    SUCCESS_ACTION(&talk_function::start_trade);
   RESPONSE(_("Bye."));
    SUCCESS(TALK_DONE);
  } else {
   if (!g->u.unarmed_attack()) {
    if (g->u.volume_carried() + g->u.weapon.volume() <= g->u.volume_capacity()){
     RESPONSE(_("&Put away weapon."));
      SUCCESS(TALK_STRANGER_NEUTRAL);
      SUCCESS_ACTION(&talk_function::player_weapon_away);
      SUCCESS_OPINION(2, -2, 0, 0, 0);
    }
    RESPONSE(_("&Drop weapon."));
     SUCCESS(TALK_STRANGER_NEUTRAL);
     SUCCESS_ACTION(&talk_function::player_weapon_drop);
     SUCCESS_OPINION(4, -3, 0, 0, 0);
   }
   int diff = 50 + p->personality.bravery - 2 * p->op_of_u.fear +
                                            2 * p->op_of_u.trust;
   RESPONSE(_("Don't worry, I'm not going to hurt you."));
    TRIAL(TALK_TRIAL_PERSUADE, diff);
    SUCCESS(TALK_STRANGER_NEUTRAL);
     SUCCESS_OPINION(1, -1, 0, 0, 0);
    FAILURE(TALK_DONE);
     FAILURE_ACTION(&talk_function::flee);
  }
  if (!p->unarmed_attack()) {
   RESPONSE(_("!Drop your weapon!"));
    TRIAL(TALK_TRIAL_INTIMIDATE, 30);
    SUCCESS(TALK_WEAPON_DROPPED);
     SUCCESS_ACTION(&talk_function::drop_weapon);
    FAILURE(TALK_DONE);
     FAILURE_ACTION(&talk_function::hostile);
  }
  RESPONSE(_("!Get out of here or I'll kill you."));
   TRIAL(TALK_TRIAL_INTIMIDATE, 20);
   SUCCESS(TALK_DONE);
    SUCCESS_ACTION(&talk_function::flee);
   FAILURE(TALK_DONE);
    FAILURE_ACTION(&talk_function::hostile);
  break;

 case TALK_STRANGER_AGGRESSIVE:
 case TALK_MUG:
  if (!g->u.unarmed_attack()) {
   int chance = 30 + p->personality.bravery - 3 * p->personality.aggression +
                 2 * p->personality.altruism - 2 * p->op_of_u.fear +
                 3 * p->op_of_u.trust;
   RESPONSE(_("!Calm down.  I'm not going to hurt you."));
    TRIAL(TALK_TRIAL_PERSUADE, chance);
    SUCCESS(TALK_STRANGER_WARY);
     SUCCESS_OPINION(1, -1, 0, 0, 0);
    FAILURE(TALK_DONE);
     FAILURE_ACTION(&talk_function::hostile);
   RESPONSE(_("!Screw you, no."));
    TRIAL(TALK_TRIAL_INTIMIDATE, chance - 5);
    SUCCESS(TALK_STRANGER_SCARED);
     SUCCESS_OPINION(-2, 1, 0, 1, 0);
    FAILURE(TALK_DONE);
     FAILURE_ACTION(&talk_function::hostile);
   RESPONSE(_("&Drop weapon."));
    if (topic == TALK_MUG) {
     SUCCESS(TALK_MUG);
    } else {
     SUCCESS(TALK_DEMAND_LEAVE);
    }
    SUCCESS_ACTION(&talk_function::player_weapon_drop);
  } else if (topic == TALK_MUG) {
   int chance = 35 + p->personality.bravery - 3 * p->personality.aggression +
                 2 * p->personality.altruism - 2 * p->op_of_u.fear +
                 3 * p->op_of_u.trust;
   RESPONSE(_("!Calm down.  I'm not going to hurt you."));
    TRIAL(TALK_TRIAL_PERSUADE, chance);
    SUCCESS(TALK_STRANGER_WARY);
     SUCCESS_OPINION(1, -1, 0, 0, 0);
    FAILURE(TALK_DONE);
     FAILURE_ACTION(&talk_function::hostile);
   RESPONSE(_("!Screw you, no."));
    TRIAL(TALK_TRIAL_INTIMIDATE, chance - 5);
    SUCCESS(TALK_STRANGER_SCARED);
     SUCCESS_OPINION(-2, 1, 0, 1, 0);
    FAILURE(TALK_DONE);
     FAILURE_ACTION(&talk_function::hostile);
   RESPONSE(_("&Put hands up."));
    SUCCESS(TALK_DONE);
    SUCCESS_ACTION(&talk_function::start_mugging);
  }
  break;

 case TALK_DESCRIBE_MISSION:
  RESPONSE(_("I see."));
   SUCCESS(TALK_NONE);
  RESPONSE(_("Bye."));
   SUCCESS(TALK_DONE);
  break;

 case TALK_WEAPON_DROPPED:
  RESPONSE(_("Now get out of here."));
   SUCCESS(TALK_DONE);
   SUCCESS_OPINION(-1, -2, 0, -2, 0);
   SUCCESS_ACTION(&talk_function::flee);
  break;

 case TALK_DEMAND_LEAVE:
  RESPONSE(_("Okay, I'm going."));
   SUCCESS(TALK_DONE);
   SUCCESS_OPINION(0, -1, 0, 0, 0);
   SUCCESS_ACTION(&talk_function::player_leaving);
  break;

 case TALK_SIZE_UP:
 case TALK_LOOK_AT:
 case TALK_OPINION:
  RESPONSE(_("Okay."));
   SUCCESS(TALK_NONE);
  break;
 }

 if (ret.empty()) {
  RESPONSE(_("Bye."));
   SUCCESS(TALK_DONE);
 }

 return ret;
}

int trial_chance(talk_response response, player *u, npc *p)
{
 talk_trial trial = response.trial;
 int chance = response.difficulty;
 switch (trial) {
  case TALK_TRIAL_LIE:
   chance += u->talk_skill() - p->talk_skill() + p->op_of_u.trust * 3;
   if (u->has_trait("TRUTHTELLER")) {
      chance -= 40;
   }
   if (u->has_trait("TAIL_FLUFFY")) {
      chance -= 20;
   }
   else if (u->has_trait("LIAR")) {
      chance += 40;
   }
   if (u->has_trait("ELFAEYES")) {
      chance += 10;
   }
   if ((u->has_trait("WINGS_BUTTERFLY")) || (u->has_trait("FLOWERS"))) {
      chance += 10;
   }
   if (u->has_bionic("bio_voice")) { //come on, who would suspect a robot of lying?
      chance += 10;
   }
   if (u->has_bionic("bio_face_mask")) {
   chance += 20;
   }
   break;

  case TALK_TRIAL_PERSUADE:
   chance += u->talk_skill() - int(p->talk_skill() / 2) +
           p->op_of_u.trust * 2 + p->op_of_u.value;
   if (u->has_trait("ELFAEYES")) {
      chance += 20;
   }
   if (u->has_trait("TAIL_FLUFFY")) {
      chance += 10;
   }
   if (u->has_trait("WINGS_BUTTERFLY")) {
      chance += 15; // Flutter your wings at 'em
   }
   if (u->has_bionic("bio_face_mask")) {
      chance += 10;
   }
   if (u->has_trait("GROWL")) {
      chance -= 25;
   }
   if (u->has_trait("HISS")) {
      chance -= 25;
   }
   if (u->has_trait("SNARL")) {
      chance -= 60;
   }
   if (u->has_bionic("bio_deformity")) {
      chance -= 50;
   }
   if (u->has_bionic("bio_voice")) {
      chance -= 20;
   }
   break;

  case TALK_TRIAL_INTIMIDATE:
   chance += u->intimidation() - p->intimidation() + p->op_of_u.fear * 2 -
           p->personality.bravery * 2;
   if (u->has_trait("MINOTAUR")) {
      chance += 15;
   }
   if (u->has_trait("MUZZLE")) {
      chance += 6;
   }
   if (u->has_trait("MUZZLE_LONG")) {
      chance += 20;
   }
   if (u->has_trait("SABER_TEETH")) {
      chance += 15;
   }
   if (u->has_trait("TERRIFYING")) {
      chance += 15;
   }
   if (u->has_trait("ELFAEYES")) {
      chance += 10;
   }
 //if (p->has_trait("TERRIFYING")) // This appears to do nothing, since NPCs don't seem to actually check for it.
 // chance -= 15;
   if (u->has_trait("GROWL")) {
      chance += 15;
   }
   if (u->has_trait("HISS")) {
      chance += 15;
   }
   if (u->has_trait("SNARL")) {
      chance += 30;
   }
   if (u->has_trait("WINGS_BUTTERFLY")) {
      chance -= 20; // Butterflies are not terribly threatening.  :-(
   }
   if (u->has_bionic("bio_face_mask")) {
      chance += 10;
   }
   if (u->has_bionic("bio_armor_eyes")) {
      chance += 10;
   }
   if (u->has_bionic("bio_deformity")) {
      chance += 20;
   }
   if (u->has_bionic("bio_voice")) {
      chance += 20;
   }
   break;

 }

 if (chance < 0)
  return 0;
 if (chance > 100)
  return 100;

 return chance;
}

int topic_category(talk_topic topic)
{
 switch (topic) {
  case TALK_MISSION_START:
  case TALK_MISSION_DESCRIBE:
  case TALK_MISSION_OFFER:
  case TALK_MISSION_ACCEPTED:
  case TALK_MISSION_REJECTED:
  case TALK_MISSION_ADVICE:
  case TALK_MISSION_INQUIRE:
  case TALK_MISSION_SUCCESS:
  case TALK_MISSION_SUCCESS_LIE:
  case TALK_MISSION_FAILURE:
  case TALK_MISSION_REWARD:
  case TALK_MISSION_END:
   return 1;

  case TALK_SHARE_EQUIPMENT:
  case TALK_GIVE_EQUIPMENT:
  case TALK_DENY_EQUIPMENT:
   return 2;

  case TALK_SUGGEST_FOLLOW:
  case TALK_AGREE_FOLLOW:
  case TALK_DENY_FOLLOW:
   return 3;

  case TALK_COMBAT_ENGAGEMENT:
   return 4;

  case TALK_COMBAT_COMMANDS:
   return 5;

  case TALK_TRAIN:
  case TALK_TRAIN_START:
  case TALK_TRAIN_FORCE:
   return 6;

  case TALK_SIZE_UP:
  case TALK_LOOK_AT:
  case TALK_OPINION:
   return 99;

  default:
   return -1; // Not grouped with other topics
 }
 return -1;
}

void talk_function::assign_mission(npc *p)
{
 int selected = p->chatbin.mission_selected;
 if (selected == -1 || selected >= p->chatbin.missions.size()) {
  debugmsg("mission_selected = %d; missions.size() = %d!",
           selected, p->chatbin.missions.size());
  return;
 }
 mission *miss = g->find_mission( p->chatbin.missions[selected] );
 g->assign_mission(p->chatbin.missions[selected]);
 miss->npc_id = p->getID();
 g->u.active_mission = g->u.active_missions.size() - 1;
 p->chatbin.missions_assigned.push_back( p->chatbin.missions[selected] );
 p->chatbin.missions.erase(p->chatbin.missions.begin() + selected);
}

void talk_function::mission_success(npc *p)
{
 int selected = p->chatbin.mission_selected;
 if (selected == -1 || selected >= p->chatbin.missions_assigned.size()) {
  debugmsg("mission_selected = %d; missions_assigned.size() = %d!",
           selected, p->chatbin.missions_assigned.size());
  return;
 }
 int index = p->chatbin.missions_assigned[selected];
 mission *miss = g->find_mission(index);
 npc_opinion tmp( 0, 0, 1 + (miss->value / 1000), -1, miss->value);
 p->op_of_u += tmp;
 g->wrap_up_mission(index);
}

void talk_function::mission_failure(npc *p)
{
 int selected = p->chatbin.mission_selected;
 if (selected == -1 || selected >= p->chatbin.missions_assigned.size()) {
  debugmsg("mission_selected = %d; missions_assigned.size() = %d!",
           selected, p->chatbin.missions_assigned.size());
  return;
 }
 npc_opinion tmp( -1, 0, -1, 1, 0);
 p->op_of_u += tmp;
 g->mission_failed(p->chatbin.missions_assigned[selected]);
}

void talk_function::clear_mission(npc *p)
{
 int selected = p->chatbin.mission_selected;
 p->chatbin.mission_selected = -1;
 if (selected == -1 || selected >= p->chatbin.missions_assigned.size()) {
  debugmsg("mission_selected = %d; missions_assigned.size() = %d!",
           selected, p->chatbin.missions_assigned.size());
  return;
 }
 mission *miss = g->find_mission( p->chatbin.missions_assigned[selected] );
 p->chatbin.missions_assigned.erase( p->chatbin.missions_assigned.begin() +
                                     selected);
 if (miss->follow_up != MISSION_NULL)
  p->chatbin.missions.push_back( g->reserve_mission(miss->follow_up, p->getID()) );
}

void talk_function::mission_reward(npc *p)
{
 int trade_amount = p->op_of_u.owed;
 p->op_of_u.owed = 0;
 trade(p, trade_amount, _("Reward"));
}

void talk_function::start_trade(npc *p)
{
 int trade_amount = p->op_of_u.owed;
 p->op_of_u.owed = 0;
 trade(p, trade_amount, _("Trade"));
}

void talk_function::assign_base(npc *p)
{
    // TODO: decide what to do upon assign? maybe pathing required
    basecamp* camp = g->m.camp_at(g->u.posx, g->u.posy);
    if(!camp) {
        dbg(D_ERROR) << "talk_function::assign_base: Assigned to base but no base here.";
        return;
    }

    add_msg(_("%s waits at %s"), p->name.c_str(), camp->camp_name().c_str());
    p->mission = NPC_MISSION_BASE;
    p->attitude = NPCATT_NULL;
}

void talk_function::assign_guard(npc *p)
{
    add_msg(_("%s is posted as a guard."), p->name.c_str());
    p->attitude = NPCATT_NULL;
    p->mission = NPC_MISSION_GUARD;
    p->chatbin.first_topic = TALK_FRIEND_GUARD;
    p->set_destination();
}

void talk_function::stop_guard(npc *p)
{
    p->attitude = NPCATT_FOLLOW;
    add_msg(_("%s begins to follow you."), p->name.c_str());
    p->mission = NPC_MISSION_NULL;
    p->chatbin.first_topic = TALK_FRIEND;
    p->goal = p->no_goal_point;
    p->guardx = -1;
    p->guardy = -1;
}

void talk_function::end_conversation(npc *p)
{
    add_msg(_("%s starts ignoring you."), p->name.c_str());
    p->chatbin.first_topic = TALK_DONE;
}

void talk_function::insult_combat(npc *p)
{
    add_msg(_("You start a fight with %s!"), p->name.c_str());
    p->chatbin.first_topic = TALK_DONE;
    p->attitude =  NPCATT_KILL;
}

void talk_function::give_equipment(npc *p)
{
 std::vector<item*> giving;
 std::vector<int> prices;
 p->init_selling(giving, prices);
 int chosen = -1;
 if (giving.empty()) {
  invslice slice = p->inv.slice();
  for (int i = 0; i < slice.size(); i++) {
   giving.push_back(&slice[i]->front());
   prices.push_back(slice[i]->front().price());
  }
 }
 while (chosen == -1 && giving.size() > 1) {
  int index = rng(0, giving.size() - 1);
  if (prices[index] < p->op_of_u.owed)
   chosen = index;
  giving.erase(giving.begin() + index);
  prices.erase(prices.begin() + index);
 }
 if (giving.empty()) {
  popup(_("%s has nothing to give!"), p->name.c_str());
  return;
 }
 if (chosen == -1)
  chosen = 0;
 item it = p->i_remn(giving[chosen]->invlet);
 popup(_("%s gives you a %s"), p->name.c_str(), it.tname().c_str());

 g->u.i_add( it );
 p->op_of_u.owed -= prices[chosen];
 p->add_disease("asked_for_item", 1800);
}

void talk_function::follow(npc *p)
{
    p->attitude = NPCATT_FOLLOW;
}

void talk_function::deny_follow(npc *p)
{
    p->add_disease("asked_to_follow", 3600);
}

void talk_function::deny_lead(npc *p)
{
 p->add_disease("asked_to_lead", 3600);
}

void talk_function::deny_equipment(npc *p)
{
 p->add_disease("asked_for_item", 600);
}

void talk_function::hostile(npc *p)
{
 add_msg(_("%s turns hostile!"), p->name.c_str());
    g->u.add_memorial_log(pgettext("memorial_male","%s became hostile."),
        pgettext("memorial_female", "%s became hostile."),
        p->name.c_str());
 p->attitude = NPCATT_KILL;
}

void talk_function::flee(npc *p)
{
 add_msg(_("%s turns to flee!"), p->name.c_str());
 p->attitude = NPCATT_FLEE;
}

void talk_function::leave(npc *p)
{
 add_msg(_("%s leaves."), p->name.c_str());
 p->attitude = NPCATT_NULL;
}

void talk_function::start_mugging(npc *p)
{
 p->attitude = NPCATT_MUG;
 add_msg(_("Pause to stay still.  Any movement may cause %s to attack."),
            p->name.c_str());
}

void talk_function::player_leaving(npc *p)
{
 p->attitude = NPCATT_WAIT_FOR_LEAVE;
 p->patience = 15 - p->personality.aggression;
}

void talk_function::drop_weapon(npc *p)
{
 g->m.add_item_or_charges(p->posx, p->posy, p->remove_weapon());
}

void talk_function::player_weapon_away(npc *p)
{
    (void)p; //unused
    g->u.i_add(g->u.remove_weapon());
}

void talk_function::player_weapon_drop(npc *p)
{
    (void)p; // unused
    g->m.add_item_or_charges(g->u.posx, g->u.posy, g->u.remove_weapon());
}

void talk_function::lead_to_safety(npc *p)
{
 g->give_mission(MISSION_REACH_SAFETY);
 int missid = g->u.active_missions[g->u.active_mission];
 point target = g->find_mission( missid )->target;
 // TODO: the target has no z-component
 p->goal.x = target.x;
 p->goal.y = target.y;
 p->goal.z = g->levz;
 p->attitude = NPCATT_LEAD;
}

void talk_function::toggle_use_guns(npc *p)
{
 p->combat_rules.use_guns = !p->combat_rules.use_guns;
}

void talk_function::toggle_use_silent(npc *p)
{
 p->combat_rules.use_silent = !p->combat_rules.use_silent;
}

void talk_function::toggle_use_grenades(npc *p)
{
 p->combat_rules.use_grenades = !p->combat_rules.use_grenades;
}

void talk_function::set_engagement_none(npc *p)
{
 p->combat_rules.engagement = ENGAGE_NONE;
}

void talk_function::set_engagement_close(npc *p)
{
 p->combat_rules.engagement = ENGAGE_CLOSE;
}

void talk_function::set_engagement_weak(npc *p)
{
 p->combat_rules.engagement = ENGAGE_WEAK;
}

void talk_function::set_engagement_hit(npc *p)
{
 p->combat_rules.engagement = ENGAGE_HIT;
}

void talk_function::set_engagement_all(npc *p)
{
 p->combat_rules.engagement = ENGAGE_ALL;
}

//TODO currently this does not handle martial art styles correctly
void talk_function::start_training(npc *p)
{
 int cost = 0, time = 0;
 Skill* sk_used = NULL;
 std::string name;
 if (p->chatbin.skill == NULL) {
  // we're training a martial art style
  cost = -800;
  time = 30000;
  name = p->chatbin.style;
 } else {
   sk_used = p->chatbin.skill;
   cost = -200 * (1 + g->u.skillLevel(sk_used));
   time = 10000 + 5000 * g->u.skillLevel(sk_used);
   name = p->chatbin.skill->ident();
 }

// Pay for it
 if (p->op_of_u.owed >= 0 - cost)
  p->op_of_u.owed += cost;
 else if (!trade(p, cost, _("Pay for training:")))
  return;
// Then receive it
 g->u.assign_activity(ACT_TRAIN, time, p->chatbin.tempvalue, 0, name);
}

void parse_tags(std::string &phrase, player *u, npc *me)
{
 if (u == NULL || me == NULL) {
  debugmsg("Called parse_tags() with NULL pointers!");
  return;
 }
 size_t fa, fb;
 std::string tag;
 do {
  fa = phrase.find("<");
  fb = phrase.find(">");
  int l = fb - fa + 1;
  if (fa != std::string::npos && fb != std::string::npos)
   tag = phrase.substr(fa, fb - fa + 1);
  else
   tag = "";
  bool replaced = false;
  for (int i = 0; i < NUM_STATIC_TAGS && !replaced; i++) {
   if (tag == talk_tags[i].tag) {
    phrase.replace(fa, l, (*talk_tags[i].replacement)[rng(0, 9)]);
    replaced = true;
   }
  }
  if (!replaced) { // Special, dynamic tags go here
   if (tag == "<yrwp>")
    phrase.replace(fa, l, u->weapon.tname());
   else if (tag == "<mywp>") {
    if (me->weapon.type->id == "null")
     phrase.replace(fa, l, _("fists"));
    else
     phrase.replace(fa, l, me->weapon.tname());
   } else if (tag == "<ammo>") {
    if (!me->weapon.is_gun())
     phrase.replace(fa, l, _("BADAMMO"));
    else {
     it_gun* gun = dynamic_cast<it_gun*>(me->weapon.type);
     phrase.replace(fa, l, ammo_name(gun->ammo));
    }
   } else if (tag == "<punc>") {
    switch (rng(0, 2)) {
     case 0: phrase.replace(fa, l, rm_prefix(_("<punc>.")));   break;
     case 1: phrase.replace(fa, l, rm_prefix(_("<punc>..."))); break;
     case 2: phrase.replace(fa, l, rm_prefix(_("<punc>!")));   break;
    }
   } else if (tag != "") {
    debugmsg("Bad tag. '%s' (%d - %d)", tag.c_str(), fa, fb);
    phrase.replace(fa, fb - fa + 1, "????");
   }
  }
 } while (fa != std::string::npos && fb != std::string::npos);
}

talk_topic dialogue::opt(talk_topic topic)
{
 const char* talk_trial_text[NUM_TALK_TRIALS] = {
  "", _("LIE"), _("PERSUADE"), _("INTIMIDATE")
 };
 std::string challenge = dynamic_line(topic, beta);
 std::vector<talk_response> responses = gen_responses(topic, beta);
// Put quotes around challenge (unless it's an action)
 if (challenge[0] != '*' && challenge[0] != '&') {
  std::stringstream tmp;
  tmp << "\"" << challenge << "\"";
 }
// Parse any tags in challenge
 parse_tags(challenge, alpha, beta);
 capitalize_letter(challenge);
// Prepend "My Name: "
 if (challenge[0] == '&') // No name prepended!
  challenge = challenge.substr(1);
 else if (challenge[0] == '*')
  challenge = rmp_format(_("<npc does something>%s %s"), beta->name.c_str(),
     challenge.substr(1).c_str());
 else
  challenge = rmp_format(_("<npc says something>%s: %s"), beta->name.c_str(),
     challenge.c_str());
 history.push_back(""); // Empty line between lines of dialogue

// Number of lines to highlight
 int hilight_lines = 1;
 std::vector<std::string> folded = foldstring(challenge, FULL_SCREEN_WIDTH / 2);
 for (size_t i = 0; i < folded.size(); i++) {
  history.push_back(folded[i]);
  hilight_lines++;
 }

 std::vector<std::string> options;
 std::vector<nc_color>    colors;
 for (size_t i = 0; i < responses.size(); i++) {
     if (responses[i].trial > 0) {  // dialogue w/ a % chance to work
         options.push_back(
             rmp_format(
                 _("<talk option>%1$c: [%2$s %3$d%%] %4$s"),
                 char('a' + i),                           // option letter
                 talk_trial_text[responses[i].trial],     // trial type
                 trial_chance(responses[i], alpha, beta), // trial % chance
                 responses[i].text.c_str()                // response
             )
         );
     }
     else { // regular dialogue
         options.push_back(
             rmp_format(
                 _("<talk option>%1$c: %2$s"),
                 char('a' + i),            // option letter
                 responses[i].text.c_str() // response
             )
         );
     }

     parse_tags(options.back(), alpha, beta);
     if (responses[i].text[0] == '!')
         colors.push_back(c_red);
     else if (responses[i].text[0] == '*')
         colors.push_back(c_ltred);
     else if (responses[i].text[0] == '&')
         colors.push_back(c_green);
     else
         colors.push_back(c_white);
 }

 for (int i = 2; i < (FULL_SCREEN_HEIGHT - 1); i++) {
  for (int j = 1; j < (FULL_SCREEN_WIDTH - 1); j++) {
   if (j != (FULL_SCREEN_WIDTH / 2) + 1)
    mvwputch(win, i, j, c_black, ' ');
  }
 }

 int curline = FULL_SCREEN_HEIGHT - 2, curhist = 1;
 nc_color col;
 while (curhist <= history.size() && curline > 0) {
  if (curhist <= hilight_lines)
   col = c_red;
  else
   col = c_dkgray;
  mvwprintz(win, curline, 1, col, history[history.size() - curhist].c_str());
  curline--;
  curhist++;
 }

 curline = 3;
 for (int i = 0; i < options.size(); i++) {
  folded = foldstring(options[i], (FULL_SCREEN_WIDTH / 2) - 4);
  for( size_t j = 0; j < folded.size(); ++j ) {
   mvwprintz(win, curline, (FULL_SCREEN_WIDTH / 2) + 2, colors[i],
             ((j == 0 ? "" : "   ") + folded[j]).c_str());
   curline++;
  }
 }
 mvwprintz(win, curline + 2, (FULL_SCREEN_WIDTH / 2) + 2, c_magenta, _("L: Look at"));
 mvwprintz(win, curline + 3, (FULL_SCREEN_WIDTH / 2) + 2, c_magenta, _("S: Size up stats"));

 wrefresh(win);

 int ch;
 bool okay;
 do {
  do {
   ch = getch();
   if (special_talk(ch) == TALK_NONE)
    ch -= 'a';
  } while (special_talk(ch) == TALK_NONE && (ch < 0 || ch >= options.size()));
  okay = false;
  if (special_talk(ch) != TALK_NONE)
   okay = true;
  else if (colors[ch] == c_white || colors[ch] == c_green)
   okay = true;
  else if (colors[ch] == c_red && query_yn(_("You may be attacked! Proceed?")))
   okay = true;
  else if (colors[ch] == c_ltred && query_yn(_("You'll be helpless! Proceed?")))
   okay = true;
 } while (!okay);
 history.push_back("");

 if (special_talk(ch) != TALK_NONE)
  return special_talk(ch);

 std::string response_printed = rmp_format("<you say something>You: %s", responses[ch].text.c_str());
 folded = foldstring(response_printed, FULL_SCREEN_WIDTH / 2);
 for( size_t i = 0; i < folded.size(); ++i ){
   history.push_back(folded[i]);
   hilight_lines++;
 }

 talk_response chosen = responses[ch];
 if (chosen.mission_index != -1)
  beta->chatbin.mission_selected = chosen.mission_index;
 if (chosen.tempvalue != -1)
  beta->chatbin.tempvalue = chosen.tempvalue;
 if (chosen.skill != NULL)
  beta->chatbin.skill = chosen.skill;
 if (!chosen.style.empty())
  beta->chatbin.style = chosen.style;

 talk_function effect;
 if (chosen.trial == TALK_TRIAL_NONE ||
     rng(0, 99) < trial_chance(chosen, alpha, beta)) {
  if (chosen.trial != TALK_TRIAL_NONE)
    alpha->practice( "speech", (100 - trial_chance(chosen, alpha, beta)) / 10 );
  (effect.*chosen.effect_success)(beta);
  beta->op_of_u += chosen.opinion_success;
  if (beta->turned_hostile()) {
   beta->make_angry();
   done = true;
  }
  return chosen.success;
 } else {
   alpha->practice( "speech", (100 - trial_chance(chosen, alpha, beta)) / 7 );
  (effect.*chosen.effect_failure)(beta);
  beta->op_of_u += chosen.opinion_failure;
  if (beta->turned_hostile()) {
   beta->make_angry();
   done = true;
  }
  return chosen.failure;
 }
 return TALK_NONE; // Shouldn't ever happen
}

talk_topic special_talk(char ch)
{
 switch (ch) {
  case 'L':
  case 'l':
   return TALK_LOOK_AT;
  case 'S':
  case 's':
   return TALK_SIZE_UP;
  case 'O':
  case 'o':
   return TALK_OPINION;
  default:
   return TALK_NONE;
 }
 return TALK_NONE;
}

bool trade(npc *p, int cost, std::string deal) {
 WINDOW* w_head = newwin(4, FULL_SCREEN_WIDTH,
                         (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                         (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
 WINDOW* w_them = newwin(FULL_SCREEN_HEIGHT - 4, FULL_SCREEN_WIDTH / 2,
                         4 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                         (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
 WINDOW* w_you = newwin(FULL_SCREEN_HEIGHT - 4, FULL_SCREEN_WIDTH - (FULL_SCREEN_WIDTH / 2),
                        4 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                        (FULL_SCREEN_WIDTH / 2) + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0));
 WINDOW* w_tmp;
 std::string header_message = _("\
Trading with %s\n\
TAB key to switch lists, letters to pick items, Enter to finalize, Esc to quit,\n\
? to get information on an item.");
 mvwprintz(w_head, 0, 0, c_white, header_message.c_str(), p->name.c_str());

// Set up line drawings
 for (int i = 0; i < FULL_SCREEN_WIDTH; i++) {
  mvwputch(w_head, 3, i, c_white, LINE_OXOX);
 }
 wrefresh(w_head);
// End of line drawings

// Populate the list of what the NPC is willing to buy, and the prices they pay
// Note that the NPC's barter skill is factored into these prices.
     std::vector<item*> theirs, yours;
     std::vector<int> their_price, your_price;
     p->init_selling(theirs, their_price);
     p->init_buying(g->u.inv, yours, your_price);
     std::vector<bool> getting_theirs, getting_yours;
     getting_theirs.resize(theirs.size());
     getting_yours.resize(yours.size());

// Adjust the prices based on your barter skill.
    for (size_t i = 0; i < their_price.size(); i++) {
        their_price[i] *= (price_adjustment(g->u.skillLevel("barter")) +
                     (p->int_cur - g->u.int_cur) / 15);
        getting_theirs[i] = false;
    }
    for (size_t i = 0; i < your_price.size(); i++) {
        your_price[i] /= (price_adjustment(g->u.skillLevel("barter")) +
                    (p->int_cur - g->u.int_cur) / 15);
        getting_yours[i] = false;
    }

    long cash = cost;       // How much cash you get in the deal (negative = losing money)
    bool focus_them = true; // Is the focus on them?
    bool update = true;     // Re-draw the screen?
    int  them_off = 0, you_off = 0; // Offset from the start of the list
    signed char ch, help;

 do {
  if (update) { // Time to re-draw
   update = false;
// Draw borders, one of which is highlighted
   werase(w_them);
   werase(w_you);
   for (int i = 1; i < FULL_SCREEN_WIDTH; i++)
    mvwputch(w_head, 3, i, c_white, LINE_OXOX);
    mvwprintz(w_head, 3, 30,
             (cash < 0 && g->u.cash >= cash * -1) || (cash >= 0 && p->cash  >= cash) ?
             c_green : c_red, (cash >= 0 ? _("Profit $%.2f") : _("Cost $%.2f")), (double)abs(cash)/100);

    if (deal != "") {
        mvwprintz(w_head, 3, 45, (cost < 0 ? c_ltred : c_ltgreen), deal.c_str());
    }
   draw_border(w_them, (focus_them ? c_yellow : BORDER_COLOR));
   draw_border(w_you, (!focus_them ? c_yellow : BORDER_COLOR));

   mvwprintz(w_them, 0, 2, (cash < 0 || p->cash >= cash ? c_green : c_red),
             _("%s: $%.2f"), p->name.c_str(), (double)p->cash/100);
   mvwprintz(w_you,  0, 2, (cash > 0 || g->u.cash>=cash*-1 ? c_green:c_red),
             _("You: $%.2f"), (double)g->u.cash/100);
// Draw their list of items, starting from them_off
   for (int i = them_off; i < theirs.size() && i < (17 + them_off); i++)
    mvwprintz(w_them, i - them_off + 1, 1,
              (getting_theirs[i] ? c_white : c_ltgray), "%c %c %s - $%.2f",
              char((i -them_off) + 'a'), (getting_theirs[i] ? '+' : '-'),
              utf8_substr(theirs[i]->tname(), 0, 25).c_str(),
              (double)their_price[i]/100);
   if (them_off > 0)
    mvwprintw(w_them, 19, 1, "< Back");
   if (them_off + 17 < theirs.size())
    mvwprintw(w_them, 19, 9, "More >");
// Draw your list of items, starting from you_off
   for (int i = you_off; i < yours.size() && (i < (17 + you_off)) ; i++)
    mvwprintz(w_you, i - you_off + 1, 1,
              (getting_yours[i] ? c_white : c_ltgray), "%c %c %s - $%.2f",
              char((i -you_off) + 'a'), (getting_yours[i] ? '+' : '-'),
              utf8_substr(yours[i]->tname(), 0,25).c_str(),
              (double)your_price[i]/100);
   if (you_off > 0)
    mvwprintw(w_you, 19, 1, _("< Back"));
   if (you_off + 17 < yours.size())
    mvwprintw(w_you, 19, 9, _("More >"));
   wrefresh(w_head);
   wrefresh(w_them);
   wrefresh(w_you);
  } // Done updating the screen
  ch = getch();
  switch (ch) {
  case '\t':
   focus_them = !focus_them;
   update = true;
   break;
  case '<':
   if (focus_them) {
    if (them_off > 0) {
     them_off -= 17;
     update = true;
    }
   } else {
    if (you_off > 0) {
     you_off -= 17;
     update = true;
    }
   }
   break;
  case '>':
   if (focus_them) {
    if (them_off + 17 < theirs.size()) {
     them_off += 17;
     update = true;
    }
   } else {
    if (you_off + 17 < yours.size()) {
     you_off += 17;
     update = true;
    }
   }
   break;
  case '?':
   update = true;
   w_tmp = newwin(3, 21, 1+(TERMY-FULL_SCREEN_HEIGHT)/2, 30+(TERMX-FULL_SCREEN_WIDTH)/2);
   mvwprintz(w_tmp, 1, 1, c_red, _("Examine which item?"));
   draw_border(w_tmp);
   wrefresh(w_tmp);
   help = getch();
   help -= 'a';
   werase(w_tmp);
   delwin(w_tmp);
   mvwprintz(w_head, 0, 0, c_white, header_message.c_str(), p->name.c_str());
   wrefresh(w_head);
   update = true;
   if (focus_them) {
    help += them_off;
    if (help >= 0 && help < theirs.size())
     popup(theirs[help]->info(), PF_NONE);
   } else {
    help += you_off;
    if (help >= 0 && help < yours.size())
     popup(yours[help]->info(), PF_NONE);
   }
   break;
  case '\n': // Check if we have enough cash...
  case 'T'://T means the trade was forced.
    // The player must pay cash, and it should not put the player negative.
    if(cash < 0 && g->u.cash < cash * -1) {
        popup(_("Not enough cash!  You have $%.2f, price is $%.2f."), (double)g->u.cash/100, -(double)cash/100);
        update = true;
        ch = ' ';
        //Else the player gets cash, and it should not make the NPC negative.
    } else if (cash > 0 && p->cash < cash  && ch != 'T') {
        popup(_("Not enough cash! %s has $%.2f, but the price is $%.2f. Use (T) to force the trade."), p->name.c_str(), (double)p->cash/100, (double)cash/100);
        update = true;
        ch = ' ';
    }
  break;
  default: // Letters & such
   if (ch >= 'a' && ch <= 'z') {
    ch -= 'a';
    if (focus_them) {
     ch += them_off;
     if (ch < theirs.size()) {
      getting_theirs[ch] = !getting_theirs[ch];
      if (getting_theirs[ch])
       cash -= their_price[ch];
      else
       cash += their_price[ch];
      update = true;
     }
    } else { // Focus is on the player's inventory
     ch += you_off;
     if (ch < yours.size()) {
      getting_yours[ch] = !getting_yours[ch];
      if (getting_yours[ch])
       cash += your_price[ch];
      else
       cash -= your_price[ch];
      update = true;
     }
    }
    ch = 0;
   }
  }
 } while (ch != KEY_ESCAPE && ch != '\n' && ch != 'T');

 if (ch == '\n' || ch == 'T') {
  inventory newinv;
  int practice = 0;
  std::vector<char> removing;
  for (size_t i = 0; i < yours.size(); i++) {
   if (getting_yours[i]) {
    newinv.push_back(*yours[i]);
    practice++;
    removing.push_back(yours[i]->invlet);
   }
  }
// Do it in two passes, so removing items doesn't corrupt yours[]
  for (size_t i = 0; i < removing.size(); i++)
   g->u.i_rem(removing[i]);

  for (size_t i = 0; i < theirs.size(); i++) {
   item tmp = *theirs[i];
   if (getting_theirs[i]) {
    practice += 2;
    tmp.invlet = 'a';
    while (g->u.has_item(tmp.invlet)) {
     if (tmp.invlet == 'z')
      tmp.invlet = 'A';
     else if (tmp.invlet == 'Z')
      return false; // TODO: Do something else with these.
     else
      tmp.invlet++;
    }
    g->u.inv.push_back(tmp);
   } else
    newinv.push_back(tmp);
  }
  g->u.practice( "barter", practice / 2 );
  p->inv = newinv;
  if(ch == 'T' && cash > 0) { //Trade was forced, give the NPC's cash to the player.
    p->op_of_u.owed += (cash - p->cash);
    g->u.cash += p->cash;
    p->cash = 0;
  } else {
      g->u.cash += cash;
      p->cash   -= cash;
  }
 }
 werase(w_head);
 werase(w_you);
 werase(w_them);
 wrefresh(w_head);
 wrefresh(w_you);
 wrefresh(w_them);
 delwin(w_head);
 delwin(w_you);
 delwin(w_them);
 if (ch == '\n')
  return true;
 return false;
}
