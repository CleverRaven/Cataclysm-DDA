#include "npc.h"
#include "output.h"
#include "game.h"
#include "dialogue.h"
#include "rng.h"
#include "line.h"
#include "debug.h"
#include "catacharset.h"
#include "messages.h"
#include "mission.h"
#include "ammo.h"
#include "overmapbuffer.h"
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

#define TRIAL(tr, diff) ret.back().trial.type = tr;\
                        ret.back().trial.difficulty = diff

#define SUCCESS(topic_)  ret.back().success.topic = topic_
#define FAILURE(topic_)  ret.back().failure.topic = topic_

#define SUCCESS_OPINION(T, F, V, A, O)   ret.back().success.opinion =\
                                         npc_opinion(T, F, V, A, O)
#define FAILURE_OPINION(T, F, V, A, O)   ret.back().failure.opinion =\
                                         npc_opinion(T, F, V, A, O)

#define SUCCESS_ACTION(func)  ret.back().success.effect = func
#define FAILURE_ACTION(func)  ret.back().failure.effect = func

#define dbg(x) DebugLog((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "

int topic_category(talk_topic topic);

talk_topic special_talk(char ch);

bool trade(npc *p, int cost, std::string deal);

const std::string &talk_trial::name() const
{
    static std::array<std::string, NUM_TALK_TRIALS> const texts = { {
        "", _("LIE"), _("PERSUADE"), _("INTIMIDATE")
    } };
    if( static_cast<size_t>( type ) >= texts.size() ) {
        debugmsg( "invalid trial type %d", static_cast<int>( type ) );
        return texts[0];
    }
    return texts[type];
}

/** Time (in turns) and cost (in cent) for training: */
// TODO: maybe move this function into the skill class? Or into the NPC class?
static int calc_skill_training_time( const Skill *skill )
{
    return MINUTES( 10 ) + MINUTES( 5 ) * g->u.skillLevel( skill );
}

static int calc_skill_training_cost( const Skill *skill )
{
    return 200 * ( 1 + g->u.skillLevel( skill ) );
}

// TODO: all styles cost the same and take the same time to train,
// maybe add values to the ma_style class to makes this variable
// TODO: maybe move this function into the ma_style class? Or into the NPC class?
static int calc_ma_style_training_time( const std::string & /* id */ )
{
    return MINUTES( 30 );
}

static int calc_ma_style_training_cost( const std::string & /* id */ )
{
    return 800;
}

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
    _("sad"), _("bummed"), _("depressed"), _("pissed"), _("unhappy"), _("<very> <sad>"),
    _("dejected"), _("down"),
    //~ The word blue here means "depressed", not the color blue.
    pgettext("npc_depressed", "blue"),
    _("glum")};
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

void npc_chatbin::check_missions()
{
    // TODO: or simply fail them? Some missions might only need to be reported.
    auto &ma = missions_assigned;
    auto const last = std::remove_if( ma.begin(), ma.end(), []( class mission const *m ) {
        return !m->is_assigned();
    } );
    std::copy( last, ma.end(), std::back_inserter( missions ) );
    ma.erase( last, ma.end() );
}

void npc::talk_to_u()
{
    // This is necessary so that we don't bug the player over and over
    if (attitude == NPCATT_TALK) {
        attitude = NPCATT_NULL;
    } else if (attitude == NPCATT_FLEE) {
        add_msg(_("%s is fleeing from you!"), name.c_str());
        return;
    } else if (attitude == NPCATT_KILL) {
        add_msg(_("%s is hostile!"), name.c_str());
        return;
    }
    dialogue d;
    d.alpha = &g->u;
    d.beta = this;

    chatbin.check_missions();

    for( auto &mission : chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() == g->u.getID() ) {
            d.missions_assigned.push_back( mission );
        }
    }

    d.topic_stack.push_back(chatbin.first_topic);

    if (is_leader()) {
        d.topic_stack.push_back(TALK_LEADER);
    } else if (is_friend()) {
        d.topic_stack.push_back(TALK_FRIEND);
    }

    int most_difficult_mission = 0;
    for( auto &mission : chatbin.missions ) {
        const auto &type = mission->get_type();
        if (type.urgent && type.difficulty > most_difficult_mission) {
            d.topic_stack.push_back(TALK_MISSION_DESCRIBE);
            chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }
    most_difficult_mission = 0;
    bool chosen_urgent = false;
    for( auto &mission : chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() != g->u.getID() ) {
            // Not assigned to the player that is currently talking to the npc
            continue;
        }
        const auto &type = mission->get_type();
        if ((type.urgent && !chosen_urgent) || (type.difficulty > most_difficult_mission &&
              (type.urgent || !chosen_urgent))) {
            chosen_urgent = type.urgent;
            d.topic_stack.push_back(TALK_MISSION_INQUIRE);
            chatbin.mission_selected = mission;
            most_difficult_mission = type.difficulty;
        }
    }
    if( chatbin.mission_selected != nullptr ) {
        if( chatbin.mission_selected->get_assigned_player_id() != g->u.getID() ) {
            // Don't talk about a mission that is assigned to someone else.
            chatbin.mission_selected = nullptr;
        }
    }
    if( chatbin.mission_selected == nullptr ) {
        // if possible, select a mission to talk about
        if( !chatbin.missions.empty() ) {
            chatbin.mission_selected = chatbin.missions.front();
        } else if( !d.missions_assigned.empty() ) {
            chatbin.mission_selected = d.missions_assigned.front();
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
            do {
                d.topic_stack.pop_back();
            } while (cat != -1 && topic_category(d.topic_stack.back()) == cat);
        }
        if (next == TALK_DONE || d.topic_stack.empty()) {
            d.done = true;
        } else if (next != TALK_NONE) {
            d.topic_stack.push_back(next);
        }
    } while (!d.done);
    delwin(d.win);
    g->refresh_all();
}

std::string dialogue::dynamic_line( const talk_topic topic ) const
{
    const auto p = beta; // for compatibility, later replace it in the code below
    talk_function effect;
    // First, a sanity test for mission stuff
    if (topic >= TALK_MISSION_START && topic <= TALK_MISSION_END) {
        if (topic == TALK_MISSION_START) {
            return "Used TALK_MISSION_START - not meant to be used!";
        }
        if (topic == TALK_MISSION_END) {
            return "Used TALK_MISSION_END - not meant to be used!";
        }

        if( p->chatbin.mission_selected == nullptr ) {
            return "mission_selected == nullptr; BUG! (npctalk.cpp:dynamic_line)";
        }

        // Mission stuff is a special case, so we'll handle it up here
        mission *miss = p->chatbin.mission_selected;
        const auto &type = miss->get_type();
        std::string ret = mission_dialogue( type.id, topic);
        if (ret.empty()) {
            debugmsg("Bug in npctalk.cpp:dynamic_line. Wrong mission_id(%d) or topic(%d)",
                     type.id, topic);
            return "";
        }

        if (topic == TALK_MISSION_SUCCESS && miss->has_follow_up() ) {
            switch (rng(1,3)){
                case 1:
                    return ret + _("  And I have more I'd like you to do.");
                case 2:
                    return ret + _("  I could use a hand with something else if you are interested.");
                case 3:
                    return ret + _("  If you are interested, I have another job for you.");
            }
        }

    return ret;
    }

    switch (topic) {
        case TALK_NONE:
        case TALK_DONE:
            return "";

        case TALK_GUARD:
            switch (rng(1,5)){
            case 1:
                return _("I'm not in charge here, you're looking for someone else...");
            case 2:
                return _("Keep civil or I'll bring the pain.");
            case 3:
                return _("Just on watch, move along.");
            case 4:
                if (g->u.male)
                    return _("Sir.");
                else
                    return _("Ma'am");
            case 5:
                if (g->u.male)
                    return _("Rough out there, isn't it?");
                else
                    return _("Ma'am, you really shouldn't be traveling out there.");
            //}
            //else {
            //    return _("Keep civil or I'll bring the pain.");
            }

        case TALK_MISSION_LIST:
            if (p->chatbin.missions.empty()) {
                if( missions_assigned.empty() ) {
                    return _("I don't have any jobs for you.");
                } else {
                    return _("I don't have any more jobs for you.");
                }
            } else if (p->chatbin.missions.size() == 1) {
                if( missions_assigned.empty() ) {
                    return _("I just have one job for you.  Want to hear about it?");
                } else {
                    return _("I have another job for you.  Want to hear about it?");
                }
            } else if( missions_assigned.empty() ) {
                return _("I have several jobs for you.  Which should I describe?");
            } else {
                return _("I have several more jobs for you.  Which should I describe?");
            }

        case TALK_MISSION_LIST_ASSIGNED:
            if( missions_assigned.empty() ) {
                return _("You're not working on anything for me right now.");
            } else if( missions_assigned.size() == 1 ) {
                return _("What about it?");
            } else {
                return _("Which job?");
            }

        case TALK_MISSION_REWARD:
            return _("Sure, here you go!");

        case TALK_EVAC_MERCHANT:
            if (g->u.is_wearing("badge_marshal"))
                return _("Welcome marshal...");
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
                    "of it ever being used.  From the passing scavengers I've heard nothing but prime loot'n spots and rumors of hordes.");

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
            if (g->u.is_wearing("badge_marshal"))
                return _("Hello marshal.");
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
            return _("That's just our nickname for them. They're what's left of the federal government.  "
                     "Don't know how legitimate they are but they are named after some military unit "
                     "that once protected the president.  Their liaison is usually hanging around "
                     "here somewhere.");

        case TALK_EVAC_GUARD1_BYE:
            return _("Stay safe out there. Hate to have to kill you after you've already died.");

        case TALK_EVAC_GUARD2:
            if (g->u.is_wearing("badge_marshal"))
                return _("Hello marshal.");
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

        case TALK_EVAC_GUARD3:
            return _("Keep to yourself and you won't find any problems.");

        case TALK_EVAC_GUARD3_NEW:
            return _("I haven't been here for long but I do my best to watch who "
                     "comes and goes.  You can't always predict who will bring trouble.");

        case TALK_EVAC_GUARD3_RULES:
            return _("Keep your head down and stay out of my way.");

        case TALK_EVAC_GUARD3_HIDE1:
            return _("Like what?");

        case TALK_EVAC_GUARD3_HIDE2:
            return _("You're new here, who the hell put you up to this crap?");

        case TALK_EVAC_GUARD3_WASTE:
            return _("If you don't get on with your business I'm going to have to "
                     "ask you to leave and not come back.");

        case TALK_EVAC_GUARD3_DEAD:
            return _("That's it, you're dead!");

        case TALK_EVAC_GUARD3_HOSTILE:
            return _("You must really have a death wish!");

        case TALK_EVAC_GUARD3_INSULT:
            return _("We don't put-up with garbage like you, finish your business and "
                     "get the hell out.");

        case TALK_EVAC_HUNTER:
            if (g->u.is_wearing("badge_marshal"))
                return _("I thought I smelled a pig.  I jest... please don't arrest me.");
            return _("Huh, thought I smelled someone new. Can I help you?");

        case TALK_EVAC_HUNTER_SMELL:
            return _("Oh, I didn't mean that in a bad way. Been out in the wilderness "
                     "so long, I find myself noticing things by scent before sight.");

        case TALK_EVAC_HUNTER_DO:
            return _("I trade food here in exchange for a place to crash and general "
                     "supplies. Well, more specifically I trade food that isn't stale "
                     "chips and flat cola.");

        case TALK_EVAC_HUNTER_LIFE:
            return _("Not really, just trying to lead my life.");

        case TALK_EVAC_HUNTER_HUNT:
            return _("Yep. Whatever game I spot, I bag and sell the meat and other "
                     "parts here. Got the occasional fish and basket full of wild "
                     "fruit, but nothing comes close to a freshly-cooked moose steak "
                     "for supper!");

        case TALK_EVAC_HUNTER_SALE:
            return _("Sure, just bagged a fresh batch of meat. You may want to grill "
                     "it up before it gets too, uh... 'tender'. ");

        case TALK_EVAC_HUNTER_ADVICE:
            switch (rng(1,7)){
            case 1:
                return _("Feed a man a fish, he's full for a day. Feed a man a bullet, "
                         "he's full for the rest of his life.");
            case 2:
                return _("Spot your prey before something nastier spots you.");
            case 3:
                return _("I've heard that cougars sometimes leap. Maybe it's just a myth.");
            case 4:
                return _("The Jabberwock is real, don't listen to what anybody else says. "
                         "If you see it, RUN.");
            case 5:
                return _("Zombie animal meat isn't good for eating, but sometimes you "
                         "might find usable fur on 'em.");
            case 6:
                return _("A steady diet of cooked meat and clean water will keep you "
                         "alive forever, but your taste buds and your colon may start "
                         "to get angry at you. Eat a piece of fruit every once in a while.");
            case 7:
                return _("Smoke crack to get more shit done.");
            }

        case TALK_EVAC_HUNTER_BYE:
            return _("Watch your back out there.");

        case TALK_OLD_GUARD_REP:
            // The rep should know whether you're a sworn officer.
            // TODO: wearing the badge w/o the trait => Bad Idea
            if (g->u.has_trait("PROF_FED")) {
                return _("Marshal...");
            }
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

         case TALK_OLD_GUARD_SOLDIER:
	        if (g->u.is_wearing("badge_marshal"))
	            switch (rng(1,4)){
	                case 1:
                        return _("Hello marshal.");
                    case 2:
                        return _("Marshal, I'm afraid I can't talk now.");
                    case 3:
                        return _("I'm not in charge here marshal.");
                    case 4:
                        return _("I'm supposed to direct all questions to my leadership, marshal.");
	            }
            switch (rng(1,5)){
            case 1:
                return _("Hey, citizen... I'm not sure you belong here.");
            case 2:
                return _("You should mind your own business, nothing to see here.");
            case 3:
                return _("If you need something you'll need to talk to someone else.");
            case 4:
                if (g->u.male)
                    return _("Sir.");
                else
                    return _("Ma'am");
            case 5:
                if (g->u.male)
                    return _("Dude, if you can hold your own you should look into enlisting.");
                else
                    return _("Hey miss, don't you think it would be safer if you stuck with me?");
            }

        case TALK_OLD_GUARD_NEC_CPT:
            if (g->u.has_trait("PROF_FED")) {
                return _("Marshal, I hope you're here to assist us.");
            }
            if (g->u.male)
                    return _("Sir, I don't know how the hell you got down here but if you have any sense you'll get out while you can.");
            return _("Ma'am, I don't know how the hell you got down here but if you have any sense you'll get out while you can.");

        case TALK_OLD_GUARD_NEC_CPT_GOAL:
            return _("I'm leading what remains of my company on a mission to re-secure this facility.  We entered the complex with two "
                     "dozen men and immediately went about securing this control room.  From here I dispatched my men to secure vital "
                     "systems located on this floor and the floors below this one.  If  we are successful, this facility can be cleared "
                     "and used as a permanent base of operations in the region.  Most importantly it will allow us to redirect refugee "
                     "traffic away from overcrowded outposts and free up more of our forces to conduct recovery operations.");

        case TALK_OLD_GUARD_NEC_CPT_VAULT:
            return _("This facility was constructed to provide a safe haven in the event of a global conflict.  The vault can support "
                     "several thousand people for a few years if all systems are operational and sufficient notification is given.  "
                     "Unfortunately, the power system was damaged or sabotaged at some point and released a single extremely lethal "
                     "burst of radiation.  The catastrophic event lasted for several minutes and resulted in the deaths of most people "
                     "located on the 2nd and lower floors.  Those working on this floor were able to seal the access ways to the "
                     "lower floors before succumbing to radiation sickness.  The only other thing the logs tell us is that all water "
                     "pressure was diverted to the lower levels.");

        case TALK_OLD_GUARD_NEC_COMMO:
            if (g->u.has_trait("PROF_FED")) {
                return _("Marshal, I'm rather surprised to see you here.");
            }
            if (g->u.male)
                    return _("Sir you are not authorized to be here... you should leave.");
            return _("Ma'am you are not authorized to be here... you should leave.");

        case TALK_OLD_GUARD_NEC_COMMO_GOAL:
            return _("We are securing the external communications array for this facility.  I'm rather restricted in what I can release"
                     "... go find my commander if you have any questions.");

        case TALK_OLD_GUARD_NEC_COMMO_FREQ:
            return _("I was expecting the captain to send a runner.  Here is the list you are looking for.  What we can identify from here are simply the "
                     "frequencies that have traffic on them.  Many of the transmissions are indecipherable without repairing or "
                     "replacing the equipment here.  When the facility was being overrun, standard procedure was to destroy encryption "
                     "hardware to protect federal secrets and maintain the integrity of the comms network.  We are hoping a few plain "
                     "text messages can get picked up though.");

        case TALK_ARSONIST:
            if (g->u.is_wearing("badge_marshal"))
                return _("That sure is a shiny badge you got there!");
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
            if (g->u.is_wearing("badge_marshal"))
                return _("I haven't done anything wrong...");
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

        case TALK_FREE_MERCHANT_STOCKS:
             return _("Hope you're here to trade.");

        case TALK_FREE_MERCHANT_STOCKS_NEW:
             return _("I oversee the food stocks for the center.  There was significant looting during "
                      "the panic when we first arrived so most of our food was carried away.  I manage "
                      "what we have left and do everything I can to increase our supplies.  Rot and mold "
                      "are more significant in the damp basement so I prioritize non-perishable food, "
                      "such as cornmeal, jerky, and fruit wine.");

        case TALK_FREE_MERCHANT_STOCKS_WHY:
             return _("All three are easy to locally produce in significant quantities and are "
                      "non-perishable.  We have a local farmer or two and a few hunter types that have "
                      "been making attempts to provide us with the nutritious supplies.  We do always "
                      "need more suppliers though.  Because this stuff is rather cheap in bulk I can "
                      "pay a premium for any you have on you.  Canned food and other edibles are "
                      "handled by the merchant in the front.");

        case TALK_FREE_MERCHANT_STOCKS_ALL:
             return _("I'm actually accepting a number of different foodstuffs: homebrew beer, sugar, flour, "
                      "smoked meat, smoked fish, cooking oil; and as mentioned before, jerky, cornmeal, "
                      "and fruit wine.");

        case TALK_FREE_MERCHANT_STOCKS_JERKY:
            return effect.bulk_trade_inquire(p, "jerky");

        case TALK_FREE_MERCHANT_STOCKS_CORNMEAL:
            return effect.bulk_trade_inquire(p, "cornmeal");

        case TALK_FREE_MERCHANT_STOCKS_WINE:
            return effect.bulk_trade_inquire(p, "fruit_wine");

        case TALK_FREE_MERCHANT_STOCKS_FLOUR:
            return effect.bulk_trade_inquire(p, "flour");

        case TALK_FREE_MERCHANT_STOCKS_SUGAR:
            return effect.bulk_trade_inquire(p, "sugar");

        case TALK_FREE_MERCHANT_STOCKS_BEER:
            return effect.bulk_trade_inquire(p, "hb_beer");

        case TALK_FREE_MERCHANT_STOCKS_SMMEAT:
            return effect.bulk_trade_inquire(p, "meat_smoked");

        case TALK_FREE_MERCHANT_STOCKS_SMFISH:
            return effect.bulk_trade_inquire(p, "fish_smoked");

        case TALK_FREE_MERCHANT_STOCKS_OIL:
            return effect.bulk_trade_inquire(p, "cooking_oil");

        case TALK_FREE_MERCHANT_STOCKS_DELIVERED:
             return _("Thank you for your business!");

        case TALK_SHELTER:
            switch (rng(1, 2)) {
                case 1: return _("Well, I guess it's just us.");
                case 2: return _("At least we've got shelter.");
            }

        case TALK_SHELTER_PLANS:
            switch (rng(1, 5)) {
                case 1: return _("I don't know, look for supplies and other survivors I guess.");
                case 2: return _("Maybe we should start boarding up this place.");
                case 3: return _("I suppose getting a car up and running should really be useful if we have to disappear quickly from here.");
                case 4: return _("We could look for one of those farms out here. They can provide plenty of food and aren't close to the cities.");
                case 5: return _("We should probably stay away from those cities, even if there's plenty of useful stuff there.");
            }
            
        case TALK_SHARE_EQUIPMENT:
            if (p->has_effect(_("asked_for_item"))) {
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
                if( !g->u.backlog.empty() && g->u.backlog.front().type == ACT_TRAIN ) {
                return _("Shall we resume?");
            }
            std::vector<const Skill*> trainable = p->skills_offered_to(g->u);
            std::vector<matype_id> styles = p->styles_offered_to(g->u);
            if (trainable.empty() && styles.empty()) {
                return _("Sorry, but it doesn't seem I have anything to teach you.");
            } else {
                return _("Here's what I can teach you...");
            }
            }
            break;

        case TALK_TRAIN_START:
            // Technically the player could be on another (unsafe) overmap terrain, but the
            // NPC is only concerned about themselves.
            if( overmap_buffer.is_safe( p->global_omt_location() ) ) {
                return _("Alright, let's begin.");
            } else {
                return _("It's not safe here.  Let's get to safety first.");
            }
            break;

        case TALK_TRAIN_FORCE:
            return _("Alright, let's begin.");

        case TALK_SUGGEST_FOLLOW:
            if (p->has_effect(_("infection"))) {
                return _("Not until I get some antibiotics...");
            }
            if (p->has_effect(_("asked_to_follow"))) {
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
            const tripoint player_pos = p->global_omt_location();
            int dist = rl_dist(player_pos, p->goal);
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

        case TALK_DENY_TRAIN:
            return _("Give it some time, I'll show you something new later...");

        case TALK_DENY_PERSONAL:
            return _("I'd prefer to keep that to myself.");

        case TALK_FRIEND_UNCOMFORTABLE:
            return _("I really don't feel comfortable doing so...");

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
                case NPC_MISSION_GUARD:
                    return _("I'm guarding this location.");
                case NPC_MISSION_NULL:
                    switch (p->myclass) {
                        case NC_SHOPKEEP:
                            return _("I'm a local shopkeeper.");
                        case NC_EVAC_SHOPKEEP:
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
                        case NC_THUG:
                            return _("I'm just here for the paycheck.");
                        case NC_SCAVENGER:
                            return _("I'm just trying to survive.");
                        case NC_ARSONIST:
                            return _("I'm just watching the world burn.");
                        case NC_HUNTER:
                            return _("I'm tracking game.");
                        case NC_MAX:
                            return _("I should not be able to exist!");
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
            info << string_format(_("Str %d - %d"), str_min, str_min + str_range);

            if (ability >= 40) {
                int dex_range = int(160 / ability);
                int dex_min = int(p->dex_max / dex_range) * dex_range;
                info << "  " << string_format(_("Dex %d - %d"), dex_min, dex_min + dex_range);
            }

            if (ability >= 50) {
                int int_range = int(200 / ability);
                int int_min = int(p->int_max / int_range) * int_range;
                info << "  " << string_format(_("Int %d - %d"), int_min, int_min + int_range);
            }

            if (ability >= 60) {
                int per_range = int(240 / ability);
                int per_min = int(p->per_max / per_range) * per_range;
                info << "  " << string_format(_("Per %d - %d"), per_min, per_min + per_range);
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

        default:
            //Suppress warnings
            break;
    }

    return "I don't know what to say. (BUG (npctalk.cpp:dynamic_line))";
}

talk_response &dialogue::add_response( const std::string &text, talk_topic const r ) const
{
    responses.push_back( talk_response() );
    talk_response &result = responses.back();
    result.text = text;
    result.success.topic = r;
    return result;
}

talk_response &dialogue::add_response_done( const std::string &text ) const
{
    return add_response( text, TALK_DONE );
}

talk_response &dialogue::add_response_none( const std::string &text ) const
{
    return add_response( text, TALK_NONE );
}

talk_response &dialogue::add_response( const std::string &text, talk_topic const r,
                                       void (talk_function::*effect_success)(npc *) ) const
{
    talk_response &result = add_response( text, r );
    result.success.effect = effect_success;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, talk_topic const r, mission *miss ) const
{
    if( miss == nullptr ) {
        debugmsg( "tried to select null mission" );
    }
    talk_response &result = add_response( text, r );
    result.mission_selected = miss;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, talk_topic const r, const Skill *skill ) const
{
    if( skill == nullptr ) {
        debugmsg( "tried to select null skill" );
    }
    talk_response &result = add_response( text, r );
    result.skill = skill;
    return result;
}

talk_response &dialogue::add_response( const std::string &text, talk_topic const r, const martialart &style ) const
{
    talk_response &result = add_response( text, r );
    result.style = style.id;
    return result;
}

void dialogue::gen_responses( const talk_topic topic ) const
{
    const auto p = beta; // for compatibility, later replace it in the code below
    auto &ret = responses; // for compatibility, later replace it in the code below
    ret.clear();
    mission *miss = p->chatbin.mission_selected;
    talk_function effect;

    switch (topic) {
        case TALK_GUARD:
            add_response_done( _("Don't mind me...") );
            break;

        case TALK_MISSION_LIST:
            if (p->chatbin.missions.empty()) {
                add_response_none( _("Oh, okay.") );
            } else if (p->chatbin.missions.size() == 1) {
                add_response( _("Tell me about it."),TALK_MISSION_OFFER,  p->chatbin.missions.front() );
                add_response_none( _("Never mind, I'm not interested.") );
            } else {
                for( auto &mission : p->chatbin.missions ) {
                    add_response( mission->get_type().name, TALK_MISSION_OFFER, mission );
                }
                add_response_none( _("Never mind, I'm not interested.") );
            }
            break;

        case TALK_MISSION_LIST_ASSIGNED:
            if( missions_assigned.empty() ) {
                add_response_none( _("Never mind then.") );
            } else if( missions_assigned.size() == 1 ) {
                add_response( _("I have news."), TALK_MISSION_INQUIRE, missions_assigned.front() );
                add_response_none( _("Never mind.") );
            } else {
                for( auto &miss : missions_assigned ) {
                    add_response( miss->get_type().name, TALK_MISSION_INQUIRE, miss );
                }
                add_response_none( _("Never mind.") );
            }
            break;

        case TALK_MISSION_DESCRIBE:
            add_response( _("What's the matter?"), TALK_MISSION_OFFER );
            add_response( _("I don't care."), TALK_MISSION_REJECTED );
            break;

        case TALK_MISSION_OFFER:
            add_response( _("I'll do it!"), TALK_MISSION_ACCEPTED, &talk_function::assign_mission );
            add_response( _("Not interested."), TALK_MISSION_REJECTED );
            break;

        case TALK_MISSION_ACCEPTED:
            add_response_none( _("Not a problem.") );
            add_response( _("Got any advice?"), TALK_MISSION_ADVICE );
            add_response( _("Can you share some equipment?"), TALK_SHARE_EQUIPMENT );
            add_response_done( _("I'll be back soon!") );
            break;

        case TALK_MISSION_ADVICE:
            add_response_none( _("Sounds good, thanks.") );
            add_response_done( _("Sounds good.  Bye!") );
            break;

        case TALK_MISSION_REJECTED:
            add_response_none( _("I'm sorry.") );
            add_response_done( _("Whatever.  Bye.") );
            break;

        case TALK_MISSION_INQUIRE: {
            const auto mission = p->chatbin.mission_selected;
            if( mission->has_failed() ) {
                RESPONSE(_("I'm sorry... I failed."));
                    SUCCESS(TALK_MISSION_FAILURE);
                        SUCCESS_OPINION(-1, 0, -1, 1, 0);
                RESPONSE(_("Not yet."));
                    TRIAL(TALK_TRIAL_LIE, 10 + p->op_of_u.trust * 3);
                    SUCCESS(TALK_NONE);
                    FAILURE(TALK_MISSION_FAILURE);
                        FAILURE_OPINION(-3, 0, -1, 2, 0);
            } else if( !mission->is_complete( p->getID() ) ) {
                add_response_none( _("Not yet.") );
                if( mission->get_type().goal == MGOAL_KILL_MONSTER ) {
                    RESPONSE(_("Yup, I killed it."));
                    TRIAL(TALK_TRIAL_LIE, 10 + p->op_of_u.trust * 5);
                        SUCCESS(TALK_MISSION_SUCCESS);
                            SUCCESS_ACTION(&talk_function::mission_success);
                        FAILURE(TALK_MISSION_SUCCESS_LIE);
                            FAILURE_OPINION(-5, 0, -1, 5, 0);
                            FAILURE_ACTION(&talk_function::mission_failure);
                }
                add_response_done( _("No.  I'll get back to it, bye!") );
            } else {
                // TODO: Lie about mission
                switch( mission->get_type().goal ) {
                    case MGOAL_FIND_ITEM:
                    case MGOAL_FIND_ANY_ITEM:
                        add_response( _("Yup!  Here it is!"), TALK_MISSION_SUCCESS,
                                      &talk_function::mission_success );
                        break;
                    case MGOAL_GO_TO_TYPE:
                        add_response( _("We're here!"), TALK_MISSION_SUCCESS, &talk_function::mission_success );
                        break;
                    case MGOAL_GO_TO:
                    case MGOAL_FIND_NPC:
                        add_response( _("Here I am."), TALK_MISSION_SUCCESS, &talk_function::mission_success );
                        break;
                    case MGOAL_FIND_MONSTER:
                        add_response( _("Here it is!"), TALK_MISSION_SUCCESS, &talk_function::mission_success );
                        break;
                    case MGOAL_ASSASSINATE:
                        add_response( _("Justice has been served."), TALK_MISSION_SUCCESS,
                                      &talk_function::mission_success );
                        break;
                    case MGOAL_KILL_MONSTER:
                        add_response( _("I killed it."), TALK_MISSION_SUCCESS, &talk_function::mission_success );
                        break;
                    case MGOAL_RECRUIT_NPC:
                    case MGOAL_RECRUIT_NPC_CLASS:
                        add_response( _("I brought'em."), TALK_MISSION_SUCCESS, &talk_function::mission_success );
                        break;
                    case MGOAL_COMPUTER_TOGGLE:
                        add_response( _("I've taken care of it..."), TALK_MISSION_SUCCESS,
                                      &talk_function::mission_success );
                        break;
                    default:
                        add_response( _("Mission success!  I don't know what else to say."),
                                      TALK_MISSION_SUCCESS, &talk_function::mission_success );
                        break;
                }
            }
        }
            break;

        case TALK_MISSION_SUCCESS:
            RESPONSE(_("Glad to help.  I need no payment."));
                SUCCESS(TALK_NONE);
                    SUCCESS_OPINION(miss->get_value() / (OWED_VAL * 4), -1,
                                    miss->get_value() / (OWED_VAL * 2), -1, 0 - miss->get_value());
                    SUCCESS_ACTION(&talk_function::clear_mission);
            add_response( _("How about some items as payment?"), TALK_MISSION_REWARD,
                          &talk_function::mission_reward );
            if((!p->skills_offered_to(g->u).empty() || !p->styles_offered_to(g->u).empty())
                  && p->myclass != NC_EVAC_SHOPKEEP) {
                RESPONSE(_("Maybe you can teach me something as payment."));
                    SUCCESS(TALK_TRAIN);
                        SUCCESS_ACTION(&talk_function::clear_mission);
            }
            add_response( _("I'll take cash if you got it!"), TALK_MISSION_REWARD,
                          &talk_function::mission_reward_cash );
            RESPONSE(_("Glad to help.  I need no payment.  Bye!"));
                SUCCESS(TALK_DONE);
                    SUCCESS_ACTION(&talk_function::clear_mission);
                    SUCCESS_OPINION(p->op_of_u.owed / (OWED_VAL * 4), -1,
                                    p->op_of_u.owed / (OWED_VAL * 2), -1, 0 - miss->get_value());
            break;

        case TALK_MISSION_SUCCESS_LIE:
            add_response( _("Well, um, sorry."), TALK_NONE, &talk_function::clear_mission );

        case TALK_MISSION_FAILURE:
            add_response_none( _("I'm sorry.  I did what I could.") );
            break;

        case TALK_MISSION_REWARD:
            add_response( _("Thank you."), TALK_NONE, &talk_function::clear_mission );
            add_response( _("Thanks, bye."), TALK_DONE, &talk_function::clear_mission );
            break;

        case TALK_EVAC_MERCHANT:
            add_response( _("I'm actually new..."), TALK_EVAC_MERCHANT_NEW );
            add_response( _("What are you doing here?"), TALK_EVAC_MERCHANT_PLANS );
            add_response( _("Heard anything about the outside world?"), TALK_EVAC_MERCHANT_WORLD );
            add_response( _("Is there any way I can join your group?"), TALK_EVAC_MERCHANT_ASK_JOIN );
            add_response( _("Can I do anything for the center?"), TALK_MISSION_LIST );
            if( missions_assigned.size() == 1 ) {
                add_response( _("About that job..."), TALK_MISSION_INQUIRE );
            } else if( missions_assigned.size() >= 2 ) {
                add_response( _("About one of those jobs..."), TALK_MISSION_LIST_ASSIGNED );
            }
            add_response( _("Let's trade then."), TALK_EVAC_MERCHANT, &talk_function::start_trade );
            add_response_done( _("Well, bye.") );
            break;

        case TALK_EVAC_MERCHANT_NEW:
            add_response( _("No rest for the weary..."), TALK_EVAC_MERCHANT );
            break;
        case TALK_EVAC_MERCHANT_PLANS:
            add_response( _("It's just as bad out here, if not worse."), TALK_EVAC_MERCHANT_PLANS2 );
            break;
        case TALK_EVAC_MERCHANT_PLANS2:
            if (g->u.int_cur >= 12){
                add_response( _("[INT 12] Wait, six buses and refugees... how many people do you still have crammed in here?"),
                                  TALK_EVAC_MERCHANT_PLANS3 );
            }
            add_response( _("Guess shit's a mess everywhere..."), TALK_EVAC_MERCHANT );
            break;
        case TALK_EVAC_MERCHANT_PLANS3:
            add_response( _("Guess it works for you..."), TALK_EVAC_MERCHANT );
            break;
        case TALK_EVAC_MERCHANT_HORDES:
            add_response( _("Thanks for the tip."), TALK_EVAC_MERCHANT );
            break;
        case TALK_EVAC_MERCHANT_PRIME_LOOT:
            add_response( _("Thanks, I'll keep an eye out."), TALK_EVAC_MERCHANT );
            break;
        case TALK_EVAC_MERCHANT_NO:
            add_response( _("Fine..."), TALK_EVAC_MERCHANT );
            break;
        case TALK_EVAC_MERCHANT_HELL_NO:
            add_response( _("Fine... *coughupyourscough*"), TALK_EVAC_MERCHANT );
            break;

        case TALK_EVAC_MERCHANT_ASK_JOIN:
            if (g->u.int_cur > 10){
                add_response( _("[INT 11] I'm sure I can organize salvage operations to increase the bounty scavengers bring in!"),
                                  TALK_EVAC_MERCHANT_NO );
            }
            if (g->u.int_cur <= 6 && g->u.str_cur > 10){
                add_response( _("[STR 11] I punch things in face real good!"), TALK_EVAC_MERCHANT_NO );
            }
            add_response( _("I'm sure I can do something to change your mind *wink*"), TALK_EVAC_MERCHANT_HELL_NO );
            add_response( _("I can pull my own weight!"), TALK_EVAC_MERCHANT_NO );
            add_response( _("I guess I'll look somewhere else..."), TALK_EVAC_MERCHANT );
            break;

        case TALK_EVAC_MERCHANT_WORLD:
            add_response( _("Hordes?"), TALK_EVAC_MERCHANT_HORDES );
            add_response( _("Heard of anything better than the odd gun cache?"), TALK_EVAC_MERCHANT_PRIME_LOOT );
            add_response( _("Was hoping for something more..."), TALK_EVAC_MERCHANT );
            break;

        case TALK_EVAC_GUARD1:
            add_response( _("What is this place?"), TALK_EVAC_GUARD1_PLACE );
            add_response( _("Can I join you guys?"), TALK_EVAC_GUARD1_JOIN );
            add_response( _("Anything I can do for you?"), TALK_EVAC_GUARD1_JOB );
            add_response( _("See you later."), TALK_EVAC_GUARD1_BYE );
            break;

        case TALK_EVAC_GUARD1_PLACE:
            add_response( _("So are you with the government or something?"), TALK_EVAC_GUARD1_GOVERNMENT );
            add_response( _("What do you trade?"), TALK_EVAC_GUARD1_TRADE );
            break;

        case TALK_EVAC_GUARD1_GOVERNMENT:
            add_response( _("Oh, okay."), TALK_EVAC_GUARD1 );
            add_response_done( _("Oh, okay. I'll go look for him") );
            break;

        case TALK_EVAC_GUARD1_TRADE:
            add_response( _("I'll go talk to them later."), TALK_EVAC_GUARD1 );
            add_response_done( _("Will do, thanks!") );
            break;

        case TALK_EVAC_GUARD1_JOIN:
            add_response( _("That's pretty blunt!"), TALK_EVAC_GUARD1_JOIN2 );
            break;

        case TALK_EVAC_GUARD1_JOIN2:
            add_response( _("So no negotiating? No, 'If you do this quest then we'll let you in?'"),
                              TALK_EVAC_GUARD1_JOIN3 );
            break;

        case TALK_EVAC_GUARD1_JOIN3:
            add_response( _("I don't like your attitude."), TALK_EVAC_GUARD1_ATTITUDE );
            add_response( _("Well alright then."), TALK_EVAC_GUARD1 );
            break;

        case TALK_EVAC_GUARD1_ATTITUDE:
            add_response( _("I think I'd rather rearrange your face instead!"), TALK_DONE,
                          &talk_function::insult_combat );
            add_response_done( _("I will.") );
            break;

        case TALK_EVAC_GUARD1_JOB:
            add_response( _("Alright then."), TALK_EVAC_GUARD1 );
            add_response_done( _("Old Guard huh, I'll go talk to him!") );
            add_response( _("Who are the Old Guard?"), TALK_EVAC_GUARD1_OLDGUARD );
            break;

        case TALK_EVAC_GUARD1_OLDGUARD:
            add_response( _("Whatever, I had another question."), TALK_EVAC_GUARD1 );
            add_response_done( _("Okay, I'll go look for him then.") );
            break;

        case TALK_EVAC_GUARD1_BYE:
            add_response_done( _("...") );
            break;

        case TALK_EVAC_GUARD2:
            add_response( _("I am actually new."), TALK_EVAC_GUARD2_NEW );
            add_response( _("Are there any rules I should follow while inside?"), TALK_EVAC_GUARD2_RULES );
            add_response( _("So who is everyone around here?"), TALK_EVAC_GUARD2_WHO );
            add_response( _("Lets trade!"), TALK_EVAC_GUARD2_TRADE );
            add_response( _("Is there anything I can do to help?"), TALK_MISSION_LIST );
            if( missions_assigned.size() == 1) {
                add_response( _("About that job..."), TALK_MISSION_INQUIRE );
            } else if( missions_assigned.size() >= 2) {
                add_response( _("About one of those jobs..."), TALK_MISSION_LIST_ASSIGNED );
            }
            add_response_done( _("Thanks! I will be on my way.") );
            break;

        case  TALK_EVAC_GUARD2_NEW:
            add_response( _("..."), TALK_EVAC_GUARD2 );
            break;

        case TALK_EVAC_GUARD2_RULES:
            add_response( _("Ok, thanks."), TALK_EVAC_GUARD2 );
            add_response( _("So uhhh, why not?"), TALK_EVAC_GUARD2_RULES_BASEMENT );
            break;

        case  TALK_EVAC_GUARD2_RULES_BASEMENT:
            add_response( _("..."), TALK_EVAC_GUARD2 );
            break;

        case TALK_EVAC_GUARD2_WHO:
            add_response( _("Thanks for the heads-up."), TALK_EVAC_GUARD2 );
            break;

        case TALK_EVAC_GUARD2_TRADE:
            add_response( _("..."), TALK_EVAC_GUARD2 );
            break;

        case TALK_EVAC_GUARD3:
            add_response( _("What do you do around here?"), TALK_EVAC_GUARD3_NEW );
            add_response( _("Got tips for avoiding trouble?"), TALK_EVAC_GUARD3_RULES );
            add_response( _("Have you seen anyone who might be hiding something?"), TALK_EVAC_GUARD3_HIDE1 );
            add_response_done( _("Bye...") );
            break;

        case TALK_EVAC_GUARD3_NEW:
            add_response( _("..."), TALK_EVAC_GUARD3 );
            break;

        case TALK_EVAC_GUARD3_RULES:
            add_response( _("OK..."), TALK_EVAC_GUARD3 );
            break;

        case TALK_EVAC_GUARD3_WASTE:
            add_response( _("Sorry..."), TALK_EVAC_GUARD3 );
            break;

        case TALK_EVAC_GUARD3_HIDE1:
            add_response( _("I'm not sure..."), TALK_EVAC_GUARD3_WASTE );
            add_response( _("Like they could be working for someone else?"), TALK_EVAC_GUARD3_HIDE2 );
            break;

        case TALK_EVAC_GUARD3_HIDE2:
            add_response( _("Sorry, I didn't mean to offend you..."), TALK_EVAC_GUARD3_WASTE );
            RESPONSE(_("Get bent, traitor!"));
                TRIAL(TALK_TRIAL_INTIMIDATE, 20 + p->op_of_u.fear * 3);
                    SUCCESS(TALK_EVAC_GUARD3_HOSTILE);
                    FAILURE(TALK_EVAC_GUARD3_INSULT);
            RESPONSE(_("Got something to hide?"));
                TRIAL(TALK_TRIAL_PERSUADE, 10 + p->op_of_u.trust * 3);
                    SUCCESS(TALK_EVAC_GUARD3_DEAD);
                    FAILURE(TALK_EVAC_GUARD3_INSULT);
            break;

        case TALK_EVAC_GUARD3_HOSTILE:
            p->my_fac->likes_u -= 15;//The Free Merchants are insulted by your actions!
            p->my_fac->respects_u -= 15;
            p->my_fac = g->faction_by_ident("hells_raiders");
            add_response_done( _("I didn't mean it!") );
            add_response_done( _("...") );
            break;

        case TALK_EVAC_GUARD3_INSULT:
            p->my_fac->likes_u -= 5;//The Free Merchants are insulted by your actions!
            p->my_fac->respects_u -= 5;
            add_response_done( _("...") );
            break;

        case TALK_EVAC_GUARD3_DEAD:
            p->my_fac = g->faction_by_ident("hells_raiders");
            add_response_done( _("I didn't mean it!") );
            add_response_done( _("...") );
            break;

        case TALK_EVAC_HUNTER:
            add_response( _("You... smelled me?"), TALK_EVAC_HUNTER_SMELL );
            add_response( _("What do you do around here?"), TALK_EVAC_HUNTER_DO );
            add_response( _("Got anything for sale?"), TALK_EVAC_HUNTER_SALE );
            add_response( _("Got any survival advice?"), TALK_EVAC_HUNTER_ADVICE );
            add_response( _("Goodbye."), TALK_EVAC_HUNTER_BYE );
            break;

        case TALK_EVAC_HUNTER_SMELL:
            add_response( _("O..kay..? "), TALK_EVAC_HUNTER );
            break;

        case TALK_EVAC_HUNTER_DO:
            add_response( _("Interesting."), TALK_EVAC_HUNTER_LIFE );
            add_response( _("Oh, so you hunt?"), TALK_EVAC_HUNTER_HUNT );
            break;

        case TALK_EVAC_HUNTER_LIFE:
            add_response( _("..."), TALK_EVAC_HUNTER );
            break;

        case TALK_EVAC_HUNTER_HUNT:
            add_response( _("Great, now my mouth is watering..."), TALK_EVAC_HUNTER );
            break;

        case TALK_EVAC_HUNTER_SALE:
            add_response( _("..."), TALK_EVAC_HUNTER, &talk_function::start_trade );
            break;

        case TALK_EVAC_HUNTER_ADVICE:
            add_response( _("..."), TALK_EVAC_HUNTER );
            break;

        case TALK_EVAC_HUNTER_BYE:
            add_response_done( _("...") );
            break;

        case TALK_OLD_GUARD_REP:
            add_response( _("Who are you?"), TALK_OLD_GUARD_REP_NEW );
            add_response( _("Heard anything about the outside world?"), TALK_OLD_GUARD_REP_WORLD );
            add_response( _("Is there any way I can join the 'Old Guard'?"), TALK_OLD_GUARD_REP_ASK_JOIN );
            add_response( _("Does the Old Guard need anything?"), TALK_MISSION_LIST );
            if( missions_assigned.size() == 1 ) {
                add_response( _("About that job..."), TALK_MISSION_INQUIRE );
            } else if( missions_assigned.size() >= 2 ) {
                add_response( _("About one of those jobs..."), TALK_MISSION_LIST_ASSIGNED );
            }
            add_response_done( _("Well, bye.") );
            break;

        case TALK_OLD_GUARD_REP_NEW:
            add_response( _("So what are you actually doing here?"), TALK_OLD_GUARD_REP_NEW_DOING );
            add_response( _("Nevermind..."), TALK_OLD_GUARD_REP );
            break;

        case TALK_OLD_GUARD_REP_NEW_DOING:
            add_response( _("Is there a catch?"), TALK_OLD_GUARD_REP_NEW_DOWNSIDE );
            add_response( _("Anything more to it?"), TALK_OLD_GUARD_REP_NEW_DOWNSIDE );
            add_response( _("Nevermind..."), TALK_OLD_GUARD_REP );
            break;
        case TALK_OLD_GUARD_REP_NEW_DOWNSIDE:
            add_response( _("Hmmm..."), TALK_OLD_GUARD_REP );
            break;

        case TALK_OLD_GUARD_REP_WORLD:
            add_response( _("The 2nd Fleet?"), TALK_OLD_GUARD_REP_WORLD_2NDFLEET );
            add_response( _("Tell me about the footholds."), TALK_OLD_GUARD_REP_WORLD_FOOTHOLDS );
            add_response( _("Nevermind..."), TALK_OLD_GUARD_REP );
            break;

        case TALK_OLD_GUARD_REP_WORLD_2NDFLEET:
            add_response( _("Hmmm..."), TALK_OLD_GUARD_REP );
            break;
        case TALK_OLD_GUARD_REP_WORLD_FOOTHOLDS:
            add_response( _("Hmmm..."), TALK_OLD_GUARD_REP );
            break;
        case TALK_OLD_GUARD_REP_ASK_JOIN:
            add_response( _("Hmmm..."), TALK_OLD_GUARD_REP );
            break;

        case TALK_OLD_GUARD_SOLDIER:
            add_response_done( _("Don't mind me...") );
            break;

        case TALK_OLD_GUARD_NEC_CPT:
            if (g->u.has_trait("PROF_FED")){
                add_response( _("What are you doing down here?"), TALK_OLD_GUARD_NEC_CPT_GOAL );
                add_response( _("Can you tell me about this facility?"), TALK_OLD_GUARD_NEC_CPT_VAULT );
                add_response( _("What do you need done?"), TALK_MISSION_LIST );
                if (p->chatbin.missions_assigned.size() == 1) {
                    add_response( _("About the mission..."), TALK_MISSION_INQUIRE );
                } else if (p->chatbin.missions_assigned.size() >= 2) {
                    add_response( _("About one of those missions..."), TALK_MISSION_LIST_ASSIGNED );
                }
            }
            add_response_done( _("I've got to go...") );
            break;

        case TALK_OLD_GUARD_NEC_CPT_GOAL:
            add_response( _("Seems like a decent plan..."), TALK_OLD_GUARD_NEC_CPT );
            break;

        case TALK_OLD_GUARD_NEC_CPT_VAULT:
            add_response( _("Whatever they did it must have worked since we are still alive..."),
                              TALK_OLD_GUARD_NEC_CPT );
            break;

        case TALK_OLD_GUARD_NEC_COMMO:
            if (g->u.has_trait("PROF_FED")){
                for( auto miss : g->u.get_active_missions() ) {
                    if( miss->name() == "Locate Commo Team"){
                        add_response( _("[MISSION] The captain sent me to get a frequency list from you."),
                                          TALK_OLD_GUARD_NEC_COMMO_FREQ );
                    }
                }
                add_response( _("What are you doing here?"), TALK_OLD_GUARD_NEC_COMMO_GOAL );
                add_response( _("Do you need any help?"), TALK_MISSION_LIST );
                if (p->chatbin.missions_assigned.size() == 1) {
                    add_response( _("About the mission..."), TALK_MISSION_INQUIRE );
                } else if (p->chatbin.missions_assigned.size() >= 2) {
                    add_response( _("About one of those missions..."), TALK_MISSION_LIST_ASSIGNED );
                }
            }
            add_response_done( _("I should be going...") );
            break;

        case TALK_OLD_GUARD_NEC_COMMO_GOAL:
            add_response( _("I'll try and find your commander then..."), TALK_OLD_GUARD_NEC_COMMO );
            break;

        case TALK_OLD_GUARD_NEC_COMMO_FREQ:
            popup(_("%s gives you a %s"), p->name.c_str(), item("necropolis_freq", 0).tname().c_str());
            g->u.i_add( item("necropolis_freq", 0) );
            add_response( _("Thanks."), TALK_OLD_GUARD_NEC_COMMO );
            break;

        case TALK_ARSONIST:
            add_response( _("I'm actually new."), TALK_ARSONIST_NEW );
            add_response( _("What are you doing here?"), TALK_ARSONIST_DOING );
            add_response( _("Heard anything about the outside world?"), TALK_ARSONIST_WORLD );
            add_response( _("Is there any way I can join your group?"), TALK_ARSONIST_JOIN );
            add_response( _("What's with your ears?"), TALK_ARSONIST_MUTATION );
            add_response( _("Anything I can help with?"), TALK_MISSION_LIST );
            if( missions_assigned.size() == 1 ) {
                add_response( _("About that job..."), TALK_MISSION_INQUIRE );
            } else if( missions_assigned.size() >= 2 ) {
                add_response( _("About one of those jobs..."), TALK_MISSION_LIST_ASSIGNED );
            }
            add_response_done( _("Well, bye.") );
            break;

        case TALK_ARSONIST_NEW:
            add_response( _("Sucks..."), TALK_ARSONIST );
            break;
        case TALK_ARSONIST_DOING:
            add_response( _("I'll buy."), TALK_ARSONIST, &talk_function::start_trade );
            add_response( _("Who needs rebar?"), TALK_ARSONIST_DOING_REBAR );
            break;
        case TALK_ARSONIST_DOING_REBAR:
            add_response( _("Well, then..."), TALK_ARSONIST );
            break;
        case TALK_ARSONIST_WORLD:
            add_response( _("Nothing optimistic?"), TALK_ARSONIST_WORLD_OPTIMISTIC );
            add_response( _("..."), TALK_ARSONIST );
            break;
        case TALK_ARSONIST_WORLD_OPTIMISTIC:
            add_response( _("I feel bad for asking."), TALK_ARSONIST );
            break;
        case TALK_ARSONIST_JOIN:
            add_response( _("..."), TALK_ARSONIST );
            break;
        case TALK_ARSONIST_MUTATION:
            add_response( _("Sorry to ask"), TALK_ARSONIST );
            add_response( _("You're disgusting."), TALK_ARSONIST_MUTATION_INSULT );
            break;
        case TALK_ARSONIST_MUTATION_INSULT:
            RESPONSE(_("..."));
                SUCCESS(TALK_DONE);
                    SUCCESS_OPINION(-1, -2, -1, 1, 0);
                    SUCCESS_ACTION(&talk_function::end_conversation);
            break;

        case TALK_SCAVENGER_MERC:
            add_response( _("Who are you?"), TALK_SCAVENGER_MERC_NEW );
            add_response( _("Any tips for surviving?"), TALK_SCAVENGER_MERC_TIPS );
            add_response( _("What would it cost to hire you?"), TALK_SCAVENGER_MERC_HIRE );
            add_response_done( _("Well, bye.") );
            break;

        case TALK_SCAVENGER_MERC_NEW:
            add_response( _("..."), TALK_SCAVENGER_MERC );
            break;
        case TALK_SCAVENGER_MERC_TIPS:
            add_response( _("I suppose I should hire a party then?"), TALK_SCAVENGER_MERC );
            break;
        case TALK_SCAVENGER_MERC_HIRE:
            if (g->u.cash >= 800000){
                add_response( _("[$8000] You have a deal."), TALK_SCAVENGER_MERC_HIRE_SUCCESS );
                g->u.cash -= 800000;
            }
            add_response( _("I might be back."), TALK_SCAVENGER_MERC );
            break;

        case TALK_SCAVENGER_MERC_HIRE_SUCCESS:
            RESPONSE(_("Glad to have you aboard."));
                SUCCESS_ACTION(&talk_function::follow);
                SUCCESS_OPINION(1, 0, 1, 0, 0);
                SUCCESS(TALK_DONE);
            break;

        case TALK_FREE_MERCHANT_STOCKS:
            add_response( _("Who are you?"), TALK_FREE_MERCHANT_STOCKS_NEW );
            if (g->u.charges_of("jerky") > 0){
                add_response( _("Delivering jerky."), TALK_FREE_MERCHANT_STOCKS_JERKY );
            }
            if (g->u.charges_of("meat_smoked") > 0){
                add_response( _("Delivering smoked meat."), TALK_FREE_MERCHANT_STOCKS_SMMEAT );
            }
            if (g->u.charges_of("fish_smoked") > 0){
                add_response( _("Delivering smoked fish."), TALK_FREE_MERCHANT_STOCKS_SMFISH );
            }
            if (g->u.charges_of("cooking_oil") > 0){
                add_response( _("Delivering cooking oil."), TALK_FREE_MERCHANT_STOCKS_OIL );
            }
            if (g->u.charges_of("cornmeal") > 0){
                add_response( _("Delivering cornmeal."), TALK_FREE_MERCHANT_STOCKS_CORNMEAL );
            }
            if (g->u.charges_of("flour") > 0){
                add_response( _("Delivering flour."), TALK_FREE_MERCHANT_STOCKS_FLOUR );
            }
            if (g->u.charges_of("fruit_wine") > 0){
                add_response( _("Delivering fruit wine."), TALK_FREE_MERCHANT_STOCKS_WINE );
            }
            if (g->u.charges_of("hb_beer") > 0){
                add_response( _("Delivering homebrew beer."), TALK_FREE_MERCHANT_STOCKS_BEER );
            }
            if (g->u.charges_of("sugar") > 0){
                add_response( _("Delivering sugar."), TALK_FREE_MERCHANT_STOCKS_SUGAR );
            }
            add_response_done( _("Well, bye.") );
            break;

        case TALK_FREE_MERCHANT_STOCKS_JERKY:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "jerky");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_SMMEAT:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "meat_smoked");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_SMFISH:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "fish_smoked");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_OIL:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "cooking_oil");
            SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_CORNMEAL:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "cornmeal");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_FLOUR:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "flour");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_SUGAR:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "sugar");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_WINE:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "fruit_wine");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;
        case TALK_FREE_MERCHANT_STOCKS_BEER:
            RESPONSE(_("Works for me."));
                effect.bulk_trade_accept(p, "hb_beer");
                SUCCESS(TALK_FREE_MERCHANT_STOCKS_DELIVERED);
            break;

        case TALK_FREE_MERCHANT_STOCKS_NEW:
            add_response( _("Why cornmeal, jerky, and fruit wine?"), TALK_FREE_MERCHANT_STOCKS_WHY );
            break;
        case TALK_FREE_MERCHANT_STOCKS_WHY:
            add_response( _("Are you looking to buy anything else?"), TALK_FREE_MERCHANT_STOCKS_ALL );
            add_response( _("Very well..."), TALK_FREE_MERCHANT_STOCKS );
            break;
        case TALK_FREE_MERCHANT_STOCKS_ALL:
            add_response( _("Interesting..."), TALK_FREE_MERCHANT_STOCKS );
            break;
        case TALK_FREE_MERCHANT_STOCKS_DELIVERED:
            add_response( _("You might be seeing more of me..."), TALK_FREE_MERCHANT_STOCKS );
            break;

        case TALK_SHELTER:
            add_response( _("What should we do now?"), TALK_SHELTER_PLANS );
            add_response( _("Can I do anything for you?"), TALK_MISSION_LIST );
            if (!p->is_following()) {
                add_response( _("Want to travel with me?"), TALK_SUGGEST_FOLLOW );
            }
            add_response( _("Let's trade items."), TALK_NONE, &talk_function::start_trade );
            if( missions_assigned.size() == 1 ) {
                add_response( _("About that job..."), TALK_MISSION_INQUIRE );
            } else if( missions_assigned.size() >= 2 ) {
                add_response( _("About one of those jobs..."), TALK_MISSION_LIST_ASSIGNED );
            }
            add_response( _("I can't leave the shelter without equipment..."), TALK_SHARE_EQUIPMENT );
            add_response_done( _("Well, bye.") );
            break;

        case TALK_SHELTER_PLANS:
            // TODO: Add _("follow me")
            add_response_none( _("Hmm, okay.") );
            break;

        case TALK_SHARE_EQUIPMENT:
            if (p->has_effect(_("asked_for_item"))) {
                add_response_none( _("Okay, fine.") );
            } else {
                int score = p->op_of_u.trust + p->op_of_u.value * 3 +
                              p->personality.altruism * 2;
                int missions_value = p->assigned_missions_value();
                if (g->u.has_amount("mininuke", 1)) {
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
                add_response_none( _("Eh, never mind.") );
                add_response_done( _("Never mind, I'll do without.  Bye.") );
            }
            break;

        case TALK_GIVE_EQUIPMENT:
            add_response_none( _("Thank you!") );
            add_response( _("Thanks!  But can I have some more?"), TALK_SHARE_EQUIPMENT );
            add_response_done( _("Thanks, see you later!") );
            break;

        case TALK_DENY_EQUIPMENT:
            add_response_none( _("Okay, okay, sorry.") );
            add_response( _("Seriously, give me more stuff!"), TALK_SHARE_EQUIPMENT );
            add_response_done( _("Okay, fine, bye.") );
            break;

        case TALK_TRAIN: {
            if( !g->u.backlog.empty() && g->u.backlog.front().type == ACT_TRAIN ) {
                player_activity &backlog = g->u.backlog.front();
                std::stringstream resume;
                resume << _("Yes, let's resume training ");
                // TODO: add a Skill::exists or is_defined or similar function
                const Skill* skillt = Skill::skill(backlog.name);
                if(skillt == NULL) {
                    auto &style = martialarts[backlog.name];
                    resume << style.name;
                    add_response( resume.str(), TALK_TRAIN_START, style );
                } else {
                    resume << skillt->name();
                    add_response( resume.str(), TALK_TRAIN_START, skillt );
                }
            }
            std::vector<matype_id> styles = p->styles_offered_to(g->u);
            std::vector<const Skill*> trainable = p->skills_offered_to(g->u);
            if (trainable.empty() && styles.empty()) {
                add_response_none( _("Oh, okay.") );
                break;
            }
            for( auto & trained : trainable ) {
                const int cost = calc_skill_training_cost( trained );
                const int cur_level = g->u.skillLevel( trained );
                //~Skill name: current level -> next level (cost in cent)
                std::string text = string_format(_("%s: %d -> %d (cost %d)"), trained->name().c_str(),
                      cur_level, cur_level + 1, cost );
                add_response( text, TALK_TRAIN_START, trained );
            }
            for( auto & style_id : styles ) {
                auto &style = martialarts[style_id];
                const int cost = calc_ma_style_training_cost( style.id );
                //~Martial art style (cost in cent)
                const std::string text = string_format( _("%s (cost %d)"), style.name.c_str(), cost );
                add_response( text, TALK_TRAIN_START, style );
            }
            add_response_none( _("Eh, never mind.") );
        }
            break;

        case TALK_TRAIN_START:
            if( overmap_buffer.is_safe( p->global_omt_location() ) ) {
                add_response( _("Sounds good."), TALK_DONE, &talk_function::start_training );
                add_response_none( _("On second thought, never mind.") );
            } else {
                add_response( _("Okay.  Lead the way."), TALK_DONE, &talk_function::lead_to_safety );
                add_response( _("No, we'll be okay here."), TALK_TRAIN_FORCE );
                add_response_none( _("On second thought, never mind.") );
            }
            break;

        case TALK_TRAIN_FORCE:
            add_response( _("Sounds good."), TALK_DONE, &talk_function::start_training );
            add_response_none( _("On second thought, never mind.") );
            break;

        case TALK_SUGGEST_FOLLOW:
            if (p->has_effect(_("infection"))) {
                add_response_none( _("Understood.  I'll get those antibiotics.") );
            } else if (p->has_effect(_("asked_to_follow"))) {
                add_response_none( _("Right, right, I'll ask later.") );
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
            add_response( _("Awesome!"), TALK_FRIEND );
            add_response_done( _("Okay, let's go!") );
            break;

        case TALK_DENY_FOLLOW:
            add_response_none( _("Oh, okay.") );
            break;

        case TALK_LEADER: {
            int persuade = p->op_of_u.fear + p->op_of_u.value + p->op_of_u.trust -
                            p->personality.bravery - p->personality.aggression;
            if (p->has_destination()) {
                add_response( _("How much further?"), TALK_HOW_MUCH_FURTHER );
            }
            add_response( _("I'm going to go my own way for a while."), TALK_LEAVE );
            if (!p->has_effect(_("asked_to_lead"))) {
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
            add_response( _("Can I do anything for you?"), TALK_MISSION_LIST );
            add_response( _("Let's trade items."), TALK_NONE, &talk_function::start_trade );
            add_response_done( _("Let's go.") );
        }
            break;

        case TALK_LEAVE:
            add_response_none( _("Nah, I'm just kidding.") );
            add_response( _("Yeah, I'm sure.  Bye."), TALK_DONE, &talk_function::leave );
            break;

        case TALK_PLAYER_LEADS:
            add_response( _("Good.  Something else..."), TALK_FRIEND );
            add_response_done( _("Alright, let's go.") );
            break;

        case TALK_LEADER_STAYS:
            add_response_none( _("Okay, okay.") );
            break;

        case TALK_HOW_MUCH_FURTHER:
            add_response_none( _("Okay, thanks.") );
            add_response_done( _("Let's keep moving.") );
            break;

        case TALK_FRIEND_GUARD:
            add_response( _("I need you to come with me."), TALK_FRIEND, &talk_function::stop_guard );
            add_response_done( _("See you around.") );
            break;

        case TALK_FRIEND:
            add_response( _("Combat commands..."), TALK_COMBAT_COMMANDS );
            add_response( _("Can I do anything for you?"), TALK_MISSION_LIST );
            RESPONSE(_("Can you teach me anything?"));
            if (!p->has_effect("asked_to_train")) {
                int commitment = 2 * p->op_of_u.trust + 1 * p->op_of_u.value -
                                  3 * p->op_of_u.anger + p->op_of_u.owed / 50;
                TRIAL(TALK_TRIAL_PERSUADE, commitment * 2);
                    SUCCESS(TALK_TRAIN);
                    FAILURE(TALK_DENY_PERSONAL);
                        FAILURE_ACTION(&talk_function::deny_train);
            } else {
                SUCCESS(TALK_DENY_TRAIN);
            }
            add_response( _("Let's trade items."), TALK_NONE, &talk_function::start_trade );
            if (p->is_following() && g->m.camp_at( g->u.pos3() )) {
                add_response( _("Wait at this base."), TALK_DONE, &talk_function::assign_base );
            }
            if (p->is_following()) {
                RESPONSE(_("Guard this position."));
                int loyalty = 3 * p->op_of_u.trust + 1 * p->op_of_u.value -
                              1 * p->op_of_u.anger + p->op_of_u.owed / 50;
                    TRIAL(TALK_TRIAL_PERSUADE, loyalty * 2);
                        SUCCESS(TALK_FRIEND_GUARD);
                            SUCCESS_ACTION(&talk_function::assign_guard);
                        FAILURE(TALK_DENY_GUARD);
                            FAILURE_OPINION(-1, -2, -1, 1, 0);
            }
            if (p->is_following()) {
                RESPONSE(_("I'd like to know a bit more about you..."));
                if (!p->has_effect("asked_personal_info")) {
                    int loyalty = 3 * p->op_of_u.trust + 1 * p->op_of_u.value -
                                    3 * p->op_of_u.anger + p->op_of_u.owed / 25;
                    TRIAL(TALK_TRIAL_PERSUADE, loyalty * 2);
                        SUCCESS(TALK_FRIEND);
                            SUCCESS_ACTION(&talk_function::reveal_stats);
                        FAILURE(TALK_DENY_PERSONAL);
                            FAILURE_ACTION(&talk_function::deny_personal_info);
                } else {
                    SUCCESS (TALK_FRIEND_UNCOMFORTABLE);
                }
            }
            add_response( _("I'm going to go my own way for a while."), TALK_LEAVE );
            add_response_done( _("Let's go.") );
            break;

        case TALK_FRIEND_UNCOMFORTABLE:
            add_response( _("I'll give you some space."), TALK_FRIEND );
            break;

        case TALK_DENY_TRAIN:
            add_response( _("Very well..."), TALK_FRIEND );
            break;

        case TALK_DENY_PERSONAL:
            add_response( _("I understand..."), TALK_FRIEND );
            break;

        case TALK_COMBAT_COMMANDS: {
            add_response( _("Change your engagement rules..."), TALK_COMBAT_ENGAGEMENT );
            if (p->combat_rules.use_guns) {
                add_response( _("Don't use guns anymore."), TALK_COMBAT_COMMANDS,
                              &talk_function::toggle_use_guns );
            } else {
                add_response( _("You can use guns."), TALK_COMBAT_COMMANDS,
                              &talk_function::toggle_use_guns );
            }
            if (p->combat_rules.use_silent) {
                add_response( _("Don't worry about noise."), TALK_COMBAT_COMMANDS,
                              &talk_function::toggle_use_silent );
            } else {
                add_response( _("Use only silent weapons."), TALK_COMBAT_COMMANDS,
                              &talk_function::toggle_use_silent );
            }
            if (p->combat_rules.use_grenades) {
                add_response( _("Don't use grenades anymore."), TALK_COMBAT_COMMANDS,
                              &talk_function::toggle_use_grenades );
            } else {
                add_response( _("You can use grenades."), TALK_COMBAT_COMMANDS,
                              &talk_function::toggle_use_grenades );
            }
            add_response_none( _("Never mind.") );
        }
            break;

        case TALK_COMBAT_ENGAGEMENT: {
            if (p->combat_rules.engagement != ENGAGE_NONE) {
                add_response( _("Don't fight unless your life depends on it."), TALK_NONE,
                              &talk_function::set_engagement_none );
            }
            if (p->combat_rules.engagement != ENGAGE_CLOSE) {
                add_response( _("Attack enemies that get too close."), TALK_NONE,
                              &talk_function::set_engagement_close);
            }
            if (p->combat_rules.engagement != ENGAGE_WEAK) {
                add_response( _("Attack enemies that you can kill easily."), TALK_NONE,
                              &talk_function::set_engagement_weak );
            }
            if (p->combat_rules.engagement != ENGAGE_HIT) {
                add_response( _("Attack only enemies that I attack first."), TALK_NONE,
                              &talk_function::set_engagement_hit );
            }
            if (p->combat_rules.engagement != ENGAGE_ALL) {
                add_response( _("Attack anything you want."), TALK_NONE,
                              &talk_function::set_engagement_all );
            }
            add_response_none( _("Never mind.") );
        }
            break;

        case TALK_STRANGER_NEUTRAL:
        case TALK_STRANGER_WARY:
        case TALK_STRANGER_SCARED:
        case TALK_STRANGER_FRIENDLY:
            if (topic == TALK_STRANGER_NEUTRAL || topic == TALK_STRANGER_FRIENDLY) {
                add_response( _("Another survivor!  We should travel together."), TALK_SUGGEST_FOLLOW );
                add_response( _("What are you doing?"), TALK_DESCRIBE_MISSION );
                add_response( _("Care to trade?"), TALK_DONE, &talk_function::start_trade );
                add_response_done( _("Bye.") );
            } else {
                if (!g->u.unarmed_attack()) {
                    if (g->u.volume_carried() + g->u.weapon.volume() <= g->u.volume_capacity()){
                        RESPONSE(_("&Put away weapon."));
                            SUCCESS(TALK_STRANGER_NEUTRAL);
                                SUCCESS_ACTION(&talk_function::player_weapon_away);
                                SUCCESS_OPINION(2, -2, 0, 0, 0);
                                SUCCESS_ACTION(&talk_function::stranger_neutral);
                    }
                    RESPONSE(_("&Drop weapon."));
                        SUCCESS(TALK_STRANGER_NEUTRAL);
                            SUCCESS_ACTION(&talk_function::player_weapon_drop);
                            SUCCESS_OPINION(4, -3, 0, 0, 0);
                }
                int diff = 50 + p->personality.bravery - 2 * p->op_of_u.fear + 2 * p->op_of_u.trust;
                RESPONSE(_("Don't worry, I'm not going to hurt you."));
                    TRIAL(TALK_TRIAL_PERSUADE, diff);
                        SUCCESS(TALK_STRANGER_NEUTRAL);
                            SUCCESS_OPINION(1, -1, 0, 0, 0);
                            SUCCESS_ACTION(&talk_function::stranger_neutral);
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
                            SUCCESS_ACTION(&talk_function::stranger_neutral);
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
                            SUCCESS_ACTION(&talk_function::stranger_neutral);
                        FAILURE(TALK_DONE);
                            FAILURE_ACTION(&talk_function::hostile);
                RESPONSE(_("!Screw you, no."));
                    TRIAL(TALK_TRIAL_INTIMIDATE, chance - 5);
                        SUCCESS(TALK_STRANGER_SCARED);
                            SUCCESS_OPINION(-2, 1, 0, 1, 0);
                        FAILURE(TALK_DONE);
                            FAILURE_ACTION(&talk_function::hostile);
                add_response( _("&Put hands up."), TALK_DONE, &talk_function::start_mugging );
            }
            break;

        case TALK_DESCRIBE_MISSION:
            add_response_none( _("I see.") );
            add_response_done( _("Bye.") );
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
            add_response_none( _("Okay.") );
            break;

        default:
            //Suppress warnings
            break;
    }

    if (ret.empty()) {
        add_response_done( _("Bye.") );
    }
}

int talk_trial::calc_chance( const dialogue &d ) const
{
    player &u = *d.alpha;
    npc &p = *d.beta;
 int chance = difficulty;
 switch (type) {
  case TALK_TRIAL_NONE:
  case NUM_TALK_TRIALS:
   dbg( D_ERROR ) << "called calc_chance with invalid talk_trial value: " << type;
   break;
  case TALK_TRIAL_LIE:
   chance += u.talk_skill() - p.talk_skill() + p.op_of_u.trust * 3;
   if (u.has_trait("TRUTHTELLER")) {
      chance -= 40;
   }
   if (u.has_trait("TAIL_FLUFFY")) {
      chance -= 20;
   }
   else if (u.has_trait("LIAR")) {
      chance += 40;
   }
   if (u.has_trait("ELFAEYES")) {
      chance += 10;
   }
   if ((u.has_trait("WINGS_BUTTERFLY")) || (u.has_trait("FLOWERS"))) {
      chance += 10;
   }
   if (u.has_bionic("bio_voice")) { //come on, who would suspect a robot of lying?
      chance += 10;
   }
   if (u.has_bionic("bio_face_mask")) {
   chance += 20;
   }
   break;

  case TALK_TRIAL_PERSUADE:
   chance += u.talk_skill() - int(p.talk_skill() / 2) +
           p.op_of_u.trust * 2 + p.op_of_u.value;
   if (u.has_trait("ELFAEYES")) {
      chance += 20;
   }
   if (u.has_trait("TAIL_FLUFFY")) {
      chance += 10;
   }
   if (u.has_trait("WINGS_BUTTERFLY")) {
      chance += 15; // Flutter your wings at 'em
   }
   if (u.has_bionic("bio_face_mask")) {
      chance += 10;
   }
   if (u.has_trait("GROWL")) {
      chance -= 25;
   }
   if (u.has_trait("HISS")) {
      chance -= 25;
   }
   if (u.has_trait("SNARL")) {
      chance -= 60;
   }
   if (u.has_bionic("bio_deformity")) {
      chance -= 50;
   }
   if (u.has_bionic("bio_voice")) {
      chance -= 20;
   }
   break;

  case TALK_TRIAL_INTIMIDATE:
   chance += u.intimidation() - p.intimidation() + p.op_of_u.fear * 2 -
           p.personality.bravery * 2;
   if (u.has_trait("MINOTAUR")) {
      chance += 15;
   }
   if (u.has_trait("MUZZLE")) {
      chance += 6;
   }
   if (u.has_trait("MUZZLE_LONG")) {
      chance += 20;
   }
   if (u.has_trait("SABER_TEETH")) {
      chance += 15;
   }
   if (u.has_trait("TERRIFYING")) {
      chance += 15;
   }
   if (u.has_trait("ELFAEYES")) {
      chance += 10;
   }
 //if (p.has_trait("TERRIFYING")) // This appears to do nothing, since NPCs don't seem to actually check for it.
 // chance -= 15;
   if (u.has_trait("GROWL")) {
      chance += 15;
   }
   if (u.has_trait("HISS")) {
      chance += 15;
   }
   if (u.has_trait("SNARL")) {
      chance += 30;
   }
   if (u.has_trait("WINGS_BUTTERFLY")) {
      chance -= 20; // Butterflies are not terribly threatening.  :-(
   }
   if (u.has_bionic("bio_face_mask")) {
      chance += 10;
   }
   if (u.has_bionic("bio_armor_eyes")) {
      chance += 10;
   }
   if (u.has_bionic("bio_deformity")) {
      chance += 20;
   }
   if (u.has_bionic("bio_voice")) {
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

bool talk_trial::roll( dialogue &d ) const
{
    if( type == TALK_TRIAL_NONE ) {
        return true;
    }
    int const chance = calc_chance( d );
    bool const success = rng( 0, 99 ) < chance;
    if( success ) {
        d.alpha->practice( "speech", ( 100 - chance ) / 10 );
    } else {
        d.alpha->practice( "speech", ( 100 - chance ) / 7 );
    }
    return success;
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
    mission *miss = p->chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "assign_mission: mission_selected == nullptr" );
        return;
    }
    miss->assign( g->u );
    p->chatbin.missions_assigned.push_back( miss );
    const auto it = std::find( p->chatbin.missions.begin(), p->chatbin.missions.end(), miss );
    p->chatbin.missions.erase( it );
}

void talk_function::mission_success(npc *p)
{
    mission *miss = p->chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "mission_success: mission_selected == nullptr" );
        return;
    }
    npc_opinion tmp( 0, 0, 1 + (miss->get_value() / 1000), -1, miss->get_value());
    p->op_of_u += tmp;
    if (p->my_fac != NULL){
        p->my_fac->likes_u += 10;
        p->my_fac->respects_u += 10;
    }
    miss->wrap_up();
}

void talk_function::mission_failure(npc *p)
{
    mission *miss = p->chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "mission_failure: mission_selected == nullptr" );
        return;
    }
    npc_opinion tmp( -1, 0, -1, 1, 0);
    p->op_of_u += tmp;
    miss->fail();
}

void talk_function::clear_mission(npc *p)
{
    mission *miss = p->chatbin.mission_selected;
    if( miss == nullptr ) {
        debugmsg( "clear_mission: mission_selected == nullptr" );
        return;
    }
    const auto it = std::find( p->chatbin.missions_assigned.begin(), p->chatbin.missions_assigned.end(), miss );
    // This function might get called twice or more if the player chooses the talk responses
    // "train skill" -> "Never mind" -> "train skill", each "train skill" response calls this function,
    // it also called when the dialogue is left through the other reward options.
    if( it == p->chatbin.missions_assigned.end() ) {
        return;
    }
    p->chatbin.missions_assigned.erase( it );
    if( p->chatbin.missions_assigned.empty() ) {
        p->chatbin.mission_selected = nullptr;
    } else {
        p->chatbin.mission_selected = p->chatbin.missions_assigned.front();
    }
    if( miss->has_follow_up() ) {
        p->add_new_mission( mission::reserve_new( miss->get_follow_up(), p->getID() ) );
    }
}

void talk_function::mission_reward(npc *p)
{
 int trade_amount = p->op_of_u.owed;
 p->op_of_u.owed = 0;
 trade(p, trade_amount, _("Reward"));
}

void talk_function::mission_reward_cash(npc *p)
{
 int trade_amount = p->op_of_u.owed * .6;
 p->op_of_u.owed = 0;
 g->u.cash += trade_amount;
}

void talk_function::start_trade(npc *p)
{
 int trade_amount = p->op_of_u.owed;
 p->op_of_u.owed = 0;
 trade(p, trade_amount, _("Trade"));
}

std::string talk_function::bulk_trade_inquire(npc *p, itype_id it)
{
 int you_have = g->u.charges_of(it);
 item tmp(it, 0);
 int item_cost = tmp.price();
 tmp.charges = you_have;
 int total_cost = tmp.price();
 p->add_msg_if_player(m_good, _("Let's see what you've got..."));
 std::stringstream response;
 response << string_format(ngettext("I'm willing to pay $%.2f per serving. You have "
                                    "%d serving for a total of $%.2f. No questions asked, here is your cash.",
                                    "I'm willing to pay $%.2f per serving. You have "
                                    "%d servings for a total of $%.2f. No questions asked, here is your cash.",
                            you_have)
                            ,(double)item_cost/100 ,you_have, (double)total_cost/100);
 return response.str();
}

void talk_function::bulk_trade_accept(npc *p, itype_id it)
{
 int you_have = g->u.charges_of(it);
 item tmp(it, 0);
 tmp.charges = you_have;
 int total = tmp.price();
 g->u.use_charges(it, you_have);
 g->u.cash += total;
 p->add_msg_if_player(m_good, _("Pleasure doing business!"));
}

void talk_function::assign_base(npc *p)
{
    // TODO: decide what to do upon assign? maybe pathing required
    basecamp* camp = g->m.camp_at(g->u.posx(), g->u.posy());
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

void talk_function::reveal_stats (npc *p)
{
    p->disp_info();
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
        for (auto &i : slice) {
            giving.push_back(&i->front());
            prices.push_back(i->front().price());
        }
    }
    while (chosen == -1 && giving.size() > 1) {
        int index = rng(0, giving.size() - 1);
        if (prices[index] < p->op_of_u.owed) {
            chosen = index;
        }
        giving.erase(giving.begin() + index);
        prices.erase(prices.begin() + index);
    }
    if (giving.empty()) {
        popup(_("%s has nothing to give!"), p->name.c_str());
        return;
    }
    if (chosen == -1) {
        chosen = 0;
    }
    item it = p->i_rem(giving[chosen]);
    popup(_("%s gives you a %s"), p->name.c_str(), it.tname().c_str());

    g->u.i_add( it );
    p->op_of_u.owed -= prices[chosen];
    p->add_effect("asked_for_item", 1800);
}

void talk_function::follow(npc *p)
{
    p->attitude = NPCATT_FOLLOW;
}

void talk_function::deny_follow(npc *p)
{
    p->add_effect("asked_to_follow", 3600);
}

void talk_function::deny_lead(npc *p)
{
 p->add_effect("asked_to_lead", 3600);
}

void talk_function::deny_equipment(npc *p)
{
 p->add_effect("asked_for_item", 600);
}

void talk_function::deny_train(npc *p)
{
 p->add_effect("asked_to_train", 3600);
}

void talk_function::deny_personal_info(npc *p)
{
 p->add_effect("asked_personal_info", 1800);
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

void talk_function::stranger_neutral(npc *p)
{
 add_msg(_("%s feels less threatened by you."), p->name.c_str());
 p->attitude = NPCATT_NULL;
 p->chatbin.first_topic = TALK_STRANGER_NEUTRAL;
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
 g->m.add_item_or_charges(p->posx(), p->posy(), p->remove_weapon());
}

void talk_function::player_weapon_away(npc *p)
{
    (void)p; //unused
    g->u.i_add(g->u.remove_weapon());
}

void talk_function::player_weapon_drop(npc *p)
{
    (void)p; // unused
    g->m.add_item_or_charges(g->u.posx(), g->u.posy(), g->u.remove_weapon());
}

void talk_function::lead_to_safety(npc *p)
{
    const auto mission = mission::reserve_new( MISSION_REACH_SAFETY, -1 );
    mission->assign( g->u );
    const point target = mission->get_target();
 // TODO: the target has no z-component
 p->goal.x = target.x;
 p->goal.y = target.y;
 p->goal.z = g->get_levz();
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

void talk_function::start_training( npc *p )
{
    int cost;
    int time;
    std::string name;
    if( p->chatbin.skill == NULL ) {
        auto &ma_style_id = p->chatbin.style;
        cost = calc_ma_style_training_cost( ma_style_id );
        time = calc_ma_style_training_time( ma_style_id );
        name = p->chatbin.style;
    } else {
        const Skill *skill = p->chatbin.skill;
        cost = calc_skill_training_cost( skill );
        time = calc_skill_training_time( skill );
        name = skill->ident();
    }

    if( p->op_of_u.owed >= cost ) {
        p->op_of_u.owed -= cost;
    } else if( !trade( p, -cost, _( "Pay for training:" ) ) ) {
        return;
    }
    g->u.assign_activity( ACT_TRAIN, time * 100, 0, 0, name );
    p->add_effect( "asked_to_train", 3600 );
}

void parse_tags(std::string &phrase, const player *u, const npc *me)
{
 if (u == NULL || me == NULL) {
  debugmsg("Called parse_tags() with NULL pointers!");
  return;
 }

 phrase = remove_color_tags( phrase );

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
    phrase.replace( fa, l, remove_color_tags( u->weapon.tname() ) );
   else if (tag == "<mywp>") {
    if (me->weapon.type->id == "null")
     phrase.replace(fa, l, _("fists"));
    else
     phrase.replace( fa, l, remove_color_tags( me->weapon.tname() ) );
   } else if (tag == "<ammo>") {
    if (!me->weapon.is_gun())
     phrase.replace(fa, l, _("BADAMMO"));
    else {
     phrase.replace(fa, l, ammo_name(me->weapon.ammo_type()) );
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

void dialogue::clear_window_texts()
{
    // Note: don't erase the borders, therefor start and end one unit inwards.
    // Note: start at second line because the first line contains the headers which are not
    // reprinted.
    // TODO: make this call werase and reprint the border & the header
    for( int i = 2; i < FULL_SCREEN_HEIGHT - 1; i++ ) {
        for( int j = 1; j < FULL_SCREEN_WIDTH - 1; j++ ) {
            if( j != ( FULL_SCREEN_WIDTH / 2 ) + 1 ) {
                mvwputch( win, i, j, c_black, ' ' );
            }
        }
    }
}

size_t dialogue::add_to_history( const std::string &text )
{
    auto const folded = foldstring( text, FULL_SCREEN_WIDTH / 2 );
    history.insert( history.end(), folded.begin(), folded.end() );
    return folded.size();
}

void dialogue::print_history( size_t const hilight_lines )
{
    int curline = FULL_SCREEN_HEIGHT - 2;
    int curindex = history.size() - 1;
    // index of the first line that is highlighted
    int newindex = history.size() - hilight_lines;
    // Print at line 2 and below, line 1 contains the header, line 0 the border
    while( curindex >= 0 && curline >= 2 ) {
        // red for new text, gray for old, similar to coloring of messages
        nc_color const col = ( curindex >= newindex ) ? c_red : c_dkgray;
        mvwprintz( win, curline, 1, col, "%s", history[curindex].c_str() );
        curline--;
        curindex--;
    }
}

// Number of lines that can be used for the list of responses:
// -2 for border, -2 for options that are always there, -1 for header
static int RESPONSE_AREA_HEIGHT() {
    return FULL_SCREEN_HEIGHT - 2 - 2 - 1;
}

bool dialogue::print_responses( int const yoffset )
{
    // Responses go on the right side of the window, add 2 for spacing
    size_t const xoffset = FULL_SCREEN_WIDTH / 2 + 2;
    // First line we can print to, +2 for borders, +1 for the header.
    int const min_line = 2 + 1;
    // Bottom most line we can print to
    int const max_line = min_line + RESPONSE_AREA_HEIGHT() - 1;

    int curline = min_line - (int) yoffset;
    size_t i;
    for( i = 0; i < responses.size() && curline <= max_line; i++ ) {
        auto const &folded = responses[i].formated_text;
        auto const &color = responses[i].color;
        for( size_t j = 0; j < folded.size(); j++, curline++ ) {
            if( curline < min_line ) {
                continue;
            } else if( curline > max_line ) {
                break;
            }
            int const off = ( j != 0 ) ? +3 : 0;
            mvwprintz( win, curline, xoffset + off, color, "%s", folded[j].c_str() );
        }
    }
    // Those are always available, their key bindings are fixed as well.
    mvwprintz( win, curline + 2, xoffset, c_magenta, _( "L: Look at" ) );
    mvwprintz( win, curline + 3, xoffset, c_magenta, _( "S: Size up stats" ) );
    return curline > max_line; // whether there is more to print.
}

int dialogue::choose_response( int const hilight_lines )
{
    int yoffset = 0;
    while( true ) {
        clear_window_texts();
        print_history( hilight_lines );
        bool const can_sroll_down = print_responses( yoffset );
        bool const can_sroll_up = yoffset > 0;
        if( can_sroll_up ) {
            mvwprintz( win, 2, FULL_SCREEN_WIDTH - 2 - 2, c_green, "^^" );
        }
        if( can_sroll_down ) {
            mvwprintz( win, FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2 - 2, c_green, "vv" );
        }
        wrefresh( win );
        // TODO: input_context?
        const long ch = getch();
        switch( ch ) {
            case KEY_DOWN:
            case KEY_NPAGE:
                if( can_sroll_down ) {
                    yoffset += RESPONSE_AREA_HEIGHT();
                }
                break;
            case KEY_UP:
            case KEY_PPAGE:
                if( can_sroll_up ) {
                    yoffset = std::max( 0, yoffset - RESPONSE_AREA_HEIGHT() );
                }
                break;
            default:
                return ch;
        }
    }
}

void talk_response::do_formatting( const dialogue &d, char const letter )
{
    std::string ftext;
    if( trial != TALK_TRIAL_NONE ) { // dialogue w/ a % chance to work
        ftext = rmp_format(
            _( "<talk option>%1$c: [%2$s %3$d%%] %4$s" ),
            letter,                         // option letter
            trial.name().c_str(),     // trial type
            trial.calc_chance( d ), // trial % chance
            text.c_str()                // response
        );
    } else { // regular dialogue
        ftext = rmp_format(
            _( "<talk option>%1$c: %2$s" ),
            letter,          // option letter
            text.c_str() // response
        );
    }
    parse_tags( ftext, d.alpha, d.beta );
    // Remaining width of the responses area, -2 for the border, -2 for indentation
    int const fold_width = FULL_SCREEN_WIDTH / 2 - 2 - 2;
    formated_text = foldstring( ftext, fold_width );

    if( text[0] == '!' ) {
        color = c_red;
    } else if( text[0] == '*' ) {
        color = c_ltred;
    } else if( text[0] == '&' ) {
        color = c_green;
    } else {
        color = c_white;
    }
}

talk_topic talk_response::effect_t::apply( dialogue &d ) const
{
    talk_function tmp;
    (tmp.*effect)( d.beta );
    d.beta->op_of_u += opinion;
    if( d.beta->turned_hostile() ) {
        d.beta->make_angry();
        return TALK_DONE;
    }

    // TODO: this is a hack, it should be in clear_mission or so, but those functions have
    // no access to the dialogue object.
    auto &ma = d.missions_assigned;
    ma.clear();
    // Update the missions we can talk about (must only be current, non-complete ones)
    for( auto &mission : d.beta->chatbin.missions_assigned ) {
        if( mission->get_assigned_player_id() == d.alpha->getID() ) {
            ma.push_back( mission );
        }
    }

    return topic;
}

talk_topic dialogue::opt(talk_topic topic)
{
 std::string challenge = dynamic_line( topic );
 gen_responses( topic );
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
    size_t const hilight_lines = add_to_history( challenge );
    for( size_t i = 0; i < responses.size(); i++ ) {
        responses[i].do_formatting( *this, 'a' + i );
    }

 int ch;
 bool okay;
 do {
  do {
   ch = choose_response( hilight_lines );
        auto st = special_talk(ch);
        if( st != TALK_NONE) {
            return st;
        }
    ch -= 'a';
  } while ((ch < 0 || ch >= (int)responses.size()));
  okay = false;
  if (responses[ch].color == c_white || responses[ch].color == c_green)
   okay = true;
  else if (responses[ch].color == c_red && query_yn(_("You may be attacked! Proceed?")))
   okay = true;
  else if (responses[ch].color == c_ltred && query_yn(_("You'll be helpless! Proceed?")))
   okay = true;
 } while (!okay);
 history.push_back("");

 std::string response_printed = rmp_format(_("<you say something>You: %s"), responses[ch].text.c_str());
    add_to_history( response_printed );

 talk_response chosen = responses[ch];
 if (chosen.mission_selected != nullptr)
  beta->chatbin.mission_selected = chosen.mission_selected;
 if (chosen.skill != NULL)
  beta->chatbin.skill = chosen.skill;
 if (!chosen.style.empty())
  beta->chatbin.style = chosen.style;

    const bool success = chosen.trial.roll( *this );
    const auto &effects = success ? chosen.success : chosen.failure;
    return effects.apply( *this );
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
        their_price[i] *= (price_adjustment(p->skillLevel("barter") - g->u.skillLevel("barter")) +
                     (p->int_cur - g->u.int_cur) / 20.0);
        getting_theirs[i] = false;
    }
    for (size_t i = 0; i < your_price.size(); i++) {
        your_price[i] *= (price_adjustment(g->u.skillLevel("barter") - p->skillLevel("barter")) +
                    (g->u.int_cur - p->int_cur) / 20.0);
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
            for (int i = 1; i < FULL_SCREEN_WIDTH; i++) {
                mvwputch(w_head, 3, i, c_white, LINE_OXOX);
            }
            mvwprintz(w_head, 3, 30,
                    (cash < 0 && (int)g->u.cash >= cash * -1) || (cash >= 0 && (int)p->cash  >= cash) ?
                    c_green : c_red, (cash >= 0 ? _("Profit $%.2f") : _("Cost $%.2f")),
                    (double)std::abs(cash)/100);

            if (deal != "") {
                mvwprintz(w_head, 3, 45, (cost < 0 ? c_ltred : c_ltgreen), deal.c_str());
            }
            draw_border(w_them, (focus_them ? c_yellow : BORDER_COLOR));
            draw_border(w_you, (!focus_them ? c_yellow : BORDER_COLOR));

            mvwprintz(w_them, 0, 2, (cash < 0 || (int)p->cash >= cash ? c_green : c_red),
                        _("%s: $%.2f"), p->name.c_str(), (double)p->cash/100);
            mvwprintz(w_you,  0, 2, (cash > 0 || (int)g->u.cash >= cash*-1 ? c_green:c_red),
                        _("You: $%.2f"), (double)g->u.cash/100);
            // Draw their list of items, starting from them_off
            for (int i = them_off; i < (int)theirs.size() && i < (17 + them_off); i++) {
                trim_and_print(w_them, i - them_off + 1, 1, 30,
                        (getting_theirs[i] ? c_white : c_ltgray), "%c %c %s",
                        char((i -them_off) + 'a'), (getting_theirs[i] ? '+' : '-'),
                        theirs[i]->tname().c_str());

                mvwprintz(w_them, i - them_off + 1, 32,
                        (getting_theirs[i] ? c_white : c_ltgray), "$%.2f",
                        (double)their_price[i]/100);
            }
            if (them_off > 0) {
                mvwprintw(w_them, 19, 1, "< Back");
            }
            if (them_off + 17 < (int)theirs.size()) {
                mvwprintw(w_them, 19, 9, "More >");
            }
            // Draw your list of items, starting from you_off
            for (int i = you_off; i < (int)yours.size() && (i < (17 + you_off)) ; i++) {
                trim_and_print(w_you, i - you_off + 1, 1, 30,
                        (getting_yours[i] ? c_white : c_ltgray), "%c %c %s",
                        char((i -you_off) + 'a'), (getting_yours[i] ? '+' : '-'),
                        yours[i]->tname().c_str());

                mvwprintz(w_you, i - you_off + 1, 32,
                        (getting_yours[i] ? c_white : c_ltgray), "$%.2f",
                        (double)your_price[i]/100);
            }
            if (you_off > 0) {
                mvwprintw(w_you, 19, 1, _("< Back"));
            }
            if (you_off + 17 < (int)yours.size()) {
                mvwprintw(w_you, 19, 9, _("More >"));
            }
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
                    if (them_off + 17 < (int)theirs.size()) {
                        them_off += 17;
                        update = true;
                    }
                } else {
                    if (you_off + 17 < (int)yours.size()) {
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
                    if (help >= 0 && help < (int)theirs.size()) {
                        popup(theirs[help]->info(), PF_NONE);
                    }
                } else {
                    help += you_off;
                    if (help >= 0 && help < (int)yours.size()) {
                        popup(yours[help]->info(), PF_NONE);
                    }
                }
                break;
            case '\n': // Check if we have enough cash...
            case 'T'://T means the trade was forced.
                // The player must pay cash, and it should not put the player negative.
                if(cash < 0 && (int)g->u.cash < cash * -1) {
                    popup(_("Not enough cash!  You have $%.2f, price is $%.2f."), (double)g->u.cash/100, -(double)cash/100);
                    update = true;
                    ch = ' ';
                //Else the player gets cash, and it should not make the NPC negative.
                } else if (cash > 0 && (int)p->cash < cash  && ch != 'T') {
                    popup(_("Not enough cash! %s has $%.2f, but the price is $%.2f. Use (T) to force the trade."),
                              p->name.c_str(), (double)p->cash/100, (double)cash/100);
                    update = true;
                    ch = ' ';
                }
                break;
            default: // Letters & such
                if (ch >= 'a' && ch <= 'z') {
                    ch -= 'a';
                    if (focus_them) {
                        ch += them_off;
                        if (ch < (int)theirs.size()) {
                            getting_theirs[ch] = !getting_theirs[ch];
                            if (getting_theirs[ch]) {
                                cash -= their_price[ch];
                            } else {
                                cash += their_price[ch];
                            }
                            update = true;
                        }
                    } else { // Focus is on the player's inventory
                        ch += you_off;
                        if (ch < (int)yours.size()) {
                            getting_yours[ch] = !getting_yours[ch];
                            if (getting_yours[ch]) {
                                cash += your_price[ch];
                            } else {
                                cash -= your_price[ch];
                            }
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
        std::vector<item*> removing;
        for (size_t i = 0; i < yours.size(); i++) {
            if (getting_yours[i]) {
                newinv.push_back(*yours[i]);
                practice++;
                removing.push_back(yours[i]);
            }
        }
        // Do it in two passes, so removing items doesn't corrupt yours[]
        for( auto &elem : removing ) {
            g->u.i_rem( elem );
        }

        for (size_t i = 0; i < theirs.size(); i++) {
            item tmp = *theirs[i];
            if (getting_theirs[i]) {
                practice += 2;
                g->u.inv.push_back(tmp);
            } else {
                newinv.push_back(tmp);
            }
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
    if (ch == '\n') {
        return true;
    }
    return false;
}
