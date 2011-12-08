#include "npc.h"
#include "output.h"
#include "game.h"
#include "dialogue.h"
#include "rng.h"
#include "line.h"
#include "keypress.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

// Some aliases to help with gen_responses
#define RESPONSE(txt)	   ret.push_back(talk_response());\
                           ret.back().text = txt

#define SELECT_MISS(txt, index)  ret.push_back(talk_response());\
                                 ret.back().text = txt;\
                                 ret.back().mission_index = index

#define TRIAL(tr)       ret.back().trial = tr
#define DIFF(diff)      ret.back().difficulty = diff
#define SUCCESS(topic)  ret.back().success = topic
#define FAILURE(topic)  ret.back().failure = topic
#define SUCCESS_ACTION(func)  ret.back().effect_success = func
#define FAILURE_ACTION(func)  ret.back().effect_failure = func

std::string dynamic_line(talk_topic topic, game *g, npc *p);
std::vector<talk_response> gen_responses(talk_topic topic, game *g, npc *p);
int trial_chance(talk_response response, player *u, npc *p);
bool trade(game *g, dialogue &d, int cost, std::string deal);

void npc::talk_to_u(game *g)
{
 if (attitude == NPCATT_TALK)
  attitude = NPCATT_NULL;
 dialogue d;
 d.alpha = &g->u;
 d.beta = this;
 d.topic_stack.push_back(chatbin.first_topic);

 int most_difficult_mission = 0;
 for (int i = 0; i < chatbin.missions.size(); i++) {
  mission_type *type = g->find_mission_type(chatbin.missions[i]);
  if (type->urgent && type->difficulty > most_difficult_mission) {
   d.topic_stack.push_back(TALK_DESCRIBE_MISSION);
   chatbin.mission_selected = i;
   most_difficult_mission = type->difficulty;
  }
 }
 most_difficult_mission = 0;
 for (int i = 0; i < chatbin.missions_assigned.size(); i++) {
  mission_type *type = g->find_mission_type(chatbin.missions_assigned[i]);
  if (type->urgent && type->difficulty > most_difficult_mission) {
   d.topic_stack.push_back(TALK_INQUIRE_MISSION);
   chatbin.mission_selected = i;
   most_difficult_mission = type->difficulty;
  }
 }

 if (d.topic_stack.back() == TALK_NONE) {
  g->add_msg("%s says, \"Leave me alone.\"", name.c_str());
  return;
 }

 moves -= 100;
 decide_needs();
  

 d.win = newwin(25, 80, 0, 0);
 wborder(d.win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 for (int i = 1; i < 24; i++)
  mvwputch(d.win, i, 41, c_ltgray, LINE_XOXO);
 mvwputch(d.win,  0, 41, c_ltgray, LINE_OXXX);
 mvwputch(d.win, 24, 41, c_ltgray, LINE_XXOX);
 mvwprintz(d.win, 1,  1, c_white, "Dialogue with %s", name.c_str());
 mvwprintz(d.win, 1, 43, c_white, "Your response:");

// Main dialogue loop
 do { 
  talk_topic next = d.opt(d.topic_stack.back(), g);
  if (next == TALK_NONE) {
   d.topic_stack.pop_back();
   while (d.topic_stack.back() >= TALK_MISSION_START &&
          d.topic_stack.back() <= TALK_MISSION_END     )
    d.topic_stack.pop_back();
  }
  if (next == TALK_DONE || d.topic_stack.empty())
   d.done = true;
  else if (next != TALK_NONE)
   d.topic_stack.push_back(next);
 } while (!d.done);
 delwin(d.win);
 g->refresh_all();
}

std::string dynamic_line(talk_topic topic, game *g, npc *p)
{
// First, a sanity test for mission stuff
 if (topic >= TALK_MISSION_START && topic <= TALK_MISSION_END) {

  if (topic == TALK_MISSION_START)
   return "Used TALK_MISSION_START - not meant to be used!";
  if (topic == TALK_MISSION_END)
   return "Used TALK_MISSION_END - not meant to be used!";

  if (p->chatbin.mission_selected == -1)
   return "mission_selected = -1; BUG!";
  int id = -1;
  if (topic == TALK_INQUIRE_MISSION || topic == TALK_MISSION_ACCEPTED ||
      topic == TALK_MISSION_SUCCESS) {
   if (p->chatbin.mission_selected >= p->chatbin.missions_assigned.size())
    return "mission_selected is too high; BUG!";
   id = p->chatbin.missions_assigned[ p->chatbin.mission_selected ];
  } else {
   if (p->chatbin.mission_selected >= p->chatbin.missions.size())
    return "mission_selected is too high; BUG!";
   id = p->chatbin.missions[ p->chatbin.mission_selected ];
  }

// Mission stuff is a special case, so we'll handle it up here
  mission_dialogue_state dia_state;
  switch (topic) {
   case TALK_DESCRIBE_MISSION: dia_state = MISSION_DIA_MENTION; break;
   case TALK_OFFER_MISSION:    dia_state = MISSION_DIA_OFFER;   break;
   case TALK_MISSION_ACCEPTED: dia_state = MISSION_DIA_ACCEPT;  break;
   case TALK_MISSION_REJECTED: dia_state = MISSION_DIA_REFUSE;  break;
   case TALK_INQUIRE_MISSION:  dia_state = MISSION_DIA_INQUIRE; break;
   case TALK_MISSION_SUCCESS:  dia_state = MISSION_DIA_SUCCESS; break;
  }

  mission_type *type = g->find_mission_type(id);

  return mission_dialogue(mission_id(type->id), dia_state);
 }
  
 switch (topic) {
 case TALK_NONE:
 case TALK_DONE:
  return "";

 case TALK_MISSION_LIST:
  if (p->chatbin.missions.empty()) {
   if (p->chatbin.missions_assigned.empty())
    return "I don't have any jobs for you.";
   else
    return "I don't have any more jobs for you.";
  } else if (p->chatbin.missions.size() == 1) {
    if (p->chatbin.missions_assigned.empty())
     return "I just have one job for you.  Want to hear about it?";
    else
     return "I have other one job for you.  Want to hear about it?";
  } else if (p->chatbin.missions_assigned.empty())
    return "I have several jobs for you.  Which should I describe?";
  else
   return "I have several more jobs for you.  Which should I describe?";

 case TALK_SHELTER:
  switch (rng(1, 2)) {
   case 1: return "Well, I guess it's just us.";
   case 2: return "At least we've got shelter.";
  }

 case TALK_SHELTER_PLANS:
  return "I don't know, look for supplies and other survivors I guess.";

 }

 return "I don't know what to say. (BUG)";
}

std::vector<talk_response> gen_responses(talk_topic topic, game *g, npc *p)
{
 std::vector<talk_response> ret;

 switch (topic) {
 case TALK_MISSION_LIST:
  if (p->chatbin.missions.empty()) {
   RESPONSE("Oh, okay");
    SUCCESS(TALK_NONE);
  } else if (p->chatbin.missions.size() == 1) {
   SELECT_MISS("Tell me about it", 0);
    SUCCESS(TALK_OFFER_MISSION);
   RESPONSE("Never mind, I'm not interested.");
    SUCCESS(TALK_NONE);
  } else {
   for (int i = 0; i < p->chatbin.missions.size(); i++) {
    SELECT_MISS(g->find_mission_type( p->chatbin.missions[i] )->name, i);
     SUCCESS(TALK_OFFER_MISSION);
   }
   RESPONSE("Never mind, I'm not interested.");
    SUCCESS(TALK_NONE);
  }
  break;

 case TALK_DESCRIBE_MISSION:
  RESPONSE("What's the matter?");
   SUCCESS(TALK_OFFER_MISSION);
  RESPONSE("I don't care.");
   SUCCESS(TALK_MISSION_REJECTED);
  break;

 case TALK_OFFER_MISSION:
  RESPONSE("I'll do it!");
   SUCCESS(TALK_MISSION_ACCEPTED);
   SUCCESS_ACTION(&talk_function::assign_mission);
  RESPONSE("Not interested.");
   SUCCESS(TALK_MISSION_REJECTED);
  break;

 case TALK_MISSION_ACCEPTED:
  RESPONSE("Not a problem.");
   SUCCESS(TALK_NONE);
  RESPONSE("I'll be back soon!");
   SUCCESS(TALK_DONE);
  break;

 case TALK_MISSION_REJECTED:
  RESPONSE("I'm sorry.");
   SUCCESS(TALK_NONE);
  RESPONSE("Whatever.  Bye.");
   SUCCESS(TALK_DONE);
  break;

 case TALK_INQUIRE_MISSION: {
  int id = p->chatbin.missions_assigned[ p->chatbin.mission_selected ];
  if (!g->mission_complete(id, p->id)) {
   RESPONSE("Not yet.");
    SUCCESS(TALK_NONE);
// TODO: Lie about mission
   RESPONSE("No.  I'll get back to it, bye!");
    SUCCESS(TALK_DONE);
  } else {
// TODO: Lie about mission
   mission_type *type = g->find_mission_type(id);
   switch (type->goal) {
   case MGOAL_FIND_ITEM:
    RESPONSE("Yup!  Here it is!");
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   case MGOAL_GO_TO:
   case MGOAL_FIND_NPC:
    RESPONSE("Here I am.");
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   default:
    RESPONSE("Mission success!  I don't know what else to say.");
     SUCCESS(TALK_MISSION_SUCCESS);
     SUCCESS_ACTION(&talk_function::mission_success);
    break;
   }
  }
 } break;

 case TALK_MISSION_SUCCESS:
// TODO: Demand reward.
  RESPONSE("Glad to help.");
   SUCCESS(TALK_NONE);
   SUCCESS_ACTION(&talk_function::clear_mission);
  RESPONSE("Glad to help.  Bye!");
   SUCCESS(TALK_DONE);
   SUCCESS_ACTION(&talk_function::clear_mission);
  break;

 case TALK_SHELTER:
  RESPONSE("What should we do now?");
   SUCCESS(TALK_SHELTER_PLANS);
  RESPONSE("Well, bye.");
   SUCCESS(TALK_DONE);
  break;

 case TALK_SHELTER_PLANS:
// TODO: Add "follow me"
  RESPONSE("Hmm, okay.  Bye.");
   SUCCESS(TALK_DONE);
  break;

 }

 if (ret.empty()) {
  RESPONSE("Bye.");
   SUCCESS(TALK_DONE);
 }

 return ret;
}

int trial_chance(talk_response response, player *u, npc *p)
{
 talk_trial trial = response.trial;
 int diff = response.difficulty;
 switch (trial) {
  case TALK_TRIAL_LIE:
   return diff - u->talk_skill() + p->talk_skill();

  case TALK_TRIAL_PERSUADE:
   return diff - u->talk_skill() + int(p->talk_skill() / 2);

  case TALK_TRIAL_INTIMIDATE:
   return diff - u->intimidation() + p->intimidation();

 }

 return 0;
}

void talk_function::assign_mission(game *g, npc *p)
{
 int selected = p->chatbin.mission_selected;
 if (selected == -1 || selected >= p->chatbin.missions.size()) {
  debugmsg("mission_selected = %d; missions.size() = %d!",
           selected, p->chatbin.missions.size());
  return;
 }
 g->assign_mission(p->chatbin.missions[selected]);
 g->u.active_missions.back().npc_id = p->id;
 p->chatbin.missions_assigned.push_back( p->chatbin.missions[selected] );
 p->chatbin.missions.erase(p->chatbin.missions.begin() + selected);
}

void talk_function::mission_success(game *g, npc *p)
{
 int selected = p->chatbin.mission_selected;
 if (selected == -1 || selected >= p->chatbin.missions_assigned.size()) {
  debugmsg("mission_selected = %d; missions_assigned.size() = %d!",
           selected, p->chatbin.missions_assigned.size());
  return;
 }
 g->wrap_up_mission(p->chatbin.missions_assigned[selected]);
}

void talk_function::clear_mission(game *g, npc *p)
{
 int selected = p->chatbin.mission_selected;
 p->chatbin.mission_selected = -1;
 if (selected == -1 || selected >= p->chatbin.missions_assigned.size()) {
  debugmsg("mission_selected = %d; missions_assigned.size() = %d!",
           selected, p->chatbin.missions_assigned.size());
  return;
 }
 p->chatbin.missions_assigned.erase( p->chatbin.missions_assigned.begin() +
                                     selected);
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
   if (tag == "<mywp>") {
    if (me->weapon.type->id == 0)
     phrase.replace(fa, l, "fists");
    else
     phrase.replace(fa, l, me->weapon.tname());
   } else if (tag == "<ammo>") {
    if (!me->weapon.is_gun())
     phrase.replace(fa, l, "BADAMMO");
    else {
     it_gun* gun = dynamic_cast<it_gun*>(me->weapon.type);
     phrase.replace(fa, l, ammo_name(gun->ammo));
    }
   } else if (tag == "<punc>") {
    switch (rng(0, 2)) {
     case 0: phrase.replace(fa, l, ".");   break;
     case 1: phrase.replace(fa, l, "..."); break;
     case 2: phrase.replace(fa, l, "!");   break;
    }
   } else if (tag != "") {
    debugmsg("Bad tag. '%s' (%d - %d)", tag.c_str(), fa, fb);
    phrase.replace(fa, fb - fa + 1, "????");
   }
  }
 } while (fa != std::string::npos && fb != std::string::npos);
}

 
talk_topic dialogue::opt(talk_topic topic, game *g)
{
 std::string challenge = dynamic_line(topic, g, beta);
 std::vector<talk_response> responses = gen_responses(topic, g, beta);
// Put quotes around challenge (unless it's an action)
 if (challenge.find_first_of("*") != 0) {
  std::stringstream tmp;
  tmp << "\"" << challenge << "\"";
 }
// Parse any tags in challenge
 parse_tags(challenge, alpha, beta);
 if (challenge[0] >= 'a' && challenge[0] <= 'z')
  challenge[0] += 'A' - 'a';
// Prepend "My Name: "
 challenge = beta->name + ": " + challenge;
 history.push_back(""); // Empty line between lines of dialogue
 
// Number of lines to highlight
 int hilight_lines = 1;
 size_t split;
 while (challenge.length() > 40) {
  hilight_lines++;
  split = challenge.find_last_of(' ', 40);
  history.push_back(challenge.substr(0, split));
  challenge = challenge.substr(split);
 }
 history.push_back(challenge);

 std::vector<std::string> options;
 std::vector<nc_color>    colors;
 for (int i = 0; i < responses.size(); i++) {
  std::stringstream text;
  text << i + 1 << ": ";
  if (responses[i].trial != TALK_TRIAL_NONE)
   text << "[" << talk_trial_text[responses[i].trial] << " " <<
           trial_chance(responses[i], alpha, beta) << "%] ";
  text << responses[i].text;
  options.push_back(text.str());
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
  
 for (int i = 2; i < 24; i++) {
  for (int j = 1; j < 79; j++) {
   if (j != 41)
    mvwputch(win, i, j, c_black, ' ');
  }
 }

 int curline = 23, curhist = 1;
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
  while (options[i].size() > 36) {
   split = options[i].find_last_of(' ', 36);
   mvwprintz(win, curline, 42, colors[i], options[i].substr(0, split).c_str());
   options[i] = "  " + options[i].substr(split);
   curline++;
  }
  mvwprintz(win, curline, 42, colors[i], options[i].c_str());
  curline++;
 }

 wrefresh(win);

 int ch;
 bool okay;
 do {
  do {
   ch = getch() - '1';
  } while (ch < 0 || ch >= options.size());
  okay = false;
  if (colors[ch] == c_white || colors[ch] == c_green)
   okay = true;
  if (colors[ch] == c_red && query_yn("You may be attacked! Proceed?"))
   okay = true;
  if (colors[ch] == c_ltred && query_yn("You'll be helpless! Proceed?"))
   okay = true;
 } while (!okay);
 history.push_back("");
 history.push_back("You: " + responses[ch].text);

 talk_response chosen = responses[ch];
 if (chosen.mission_index != -1)
  beta->chatbin.mission_selected = chosen.mission_index;
 talk_function effect;
 if (chosen.trial == TALK_TRIAL_NONE ||
     rng(0, 99) < trial_chance(chosen, alpha, beta)) {
  (effect.*chosen.effect_success)(g, beta);
  return chosen.success;
 } else {
  (effect.*chosen.effect_failure)(g, beta);
  return chosen.failure;
 }
 return TALK_NONE; // Shouldn't ever happen
}

bool trade(game *g, dialogue &d, int cost, std::string deal)
{
 WINDOW* w_head = newwin( 4, 80,  0,  0);
 WINDOW* w_them = newwin(21, 40,  4,  0);
 WINDOW* w_you  = newwin(21, 40,  4, 40);
 WINDOW* w_tmp;
 mvwprintz(w_head, 0, 0, c_white, "\
Trading with %s\n\
Tab key to switch lists, letters to pick items, Enter to finalize, Esc to quit\n\
? to get information on an item", d.beta->name.c_str());

// Set up line drawings
 for (int i = 0; i < 80; i++)
  mvwputch(w_head,  3, i, c_white, LINE_OXOX);
 wrefresh(w_head);
 
  
// End of line drawings

// Populate the list of what the NPC is willing to buy, and the prices they pay
// Note that the NPC's barter skill is factored into these prices.
 std::vector<int> theirs, their_price, yours, your_price;
 d.beta->init_selling(theirs, their_price);
 d.beta->init_buying(d.alpha->inv, yours, your_price);
 bool getting_theirs[theirs.size()], getting_yours[yours.size()];

// Adjust the prices based on your barter skill.
 for (int i = 0; i < their_price.size(); i++) {
  their_price[i] *= (price_adjustment(d.alpha->sklevel[sk_barter]) +
                     (d.beta->int_cur - d.alpha->int_cur) / 15);
  getting_theirs[i] = false;
 }
 for (int i = 0; i < your_price.size(); i++) {
  your_price[i] /= (price_adjustment(d.alpha->sklevel[sk_barter]) +
                    (d.beta->int_cur - d.alpha->int_cur) / 15);
  getting_yours[i] = false;
 }

 int cash = cost;// How much cash you get in the deal (negative = losing money)
 bool focus_them = true;	// Is the focus on them?
 bool update = true;		// Re-draw the screen?
 int them_off = 0, you_off = 0;	// Offset from the start of the list
 char ch, help;
 
 do {
  if (update) {	// Time to re-draw
   update = false;
// Draw borders, one of which is highlighted
   werase(w_them);
   werase(w_you);
   for (int i = 1; i < 80; i++)
    mvwputch(w_head, 3, i, c_white, LINE_OXOX);
   mvwprintz(w_head, 3, 30, ((cash <  0 && d.alpha->cash >= cash * -1) ||
                             (cash >= 0 && d.beta->cash  >= cash) ?
                             c_green : c_red),
             "%s $%d", (cash >= 0 ? "Profit" : "Cost"), abs(cash));
   if (deal != "")
    mvwprintz(w_head, 3, 45, (cost < 0 ? c_ltred : c_ltgreen), deal.c_str());
   if (focus_them)
    wattron(w_them, c_yellow);
   else
    wattron(w_you,  c_yellow);
   wborder(w_them, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                   LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
   wborder(w_you,  LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                   LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
   wattroff(w_them, c_yellow);
   wattroff(w_you,  c_yellow);
   mvwprintz(w_them, 0, 1, (cash < 0 || d.beta->cash >= cash ? c_green : c_red),
             "%s: $%d", d.beta->name.c_str(), d.beta->cash);
   mvwprintz(w_you,  0, 2, (cash > 0 || d.alpha->cash>=cash*-1 ? c_green:c_red),
             "You: $%d", d.alpha->cash);
// Draw their list of items, starting from them_off
   for (int i = them_off; i < theirs.size() && i < 17; i++)
    mvwprintz(w_them, i - them_off + 1, 1,
              (getting_theirs[i] ? c_white : c_ltgray), "%c %c %s - $%d",
              char(i + 'a'), (getting_theirs[i] ? '+' : '-'),
              d.beta->inv[theirs[i + them_off]].tname().substr( 0,25).c_str(),
              their_price[i + them_off]);
   if (them_off > 0)
    mvwprintw(w_them, 19, 1, "< Back");
   if (them_off + 17 < theirs.size())
    mvwprintw(w_them, 19, 9, "More >");
// Draw your list of items, starting from you_off
   for (int i = you_off; i < yours.size() && i < 17; i++)
    mvwprintz(w_you, i - you_off + 1, 1,
              (getting_yours[i] ? c_white : c_ltgray), "%c %c %s - $%d",
              char(i + 'a'), (getting_yours[i] ? '+' : '-'),
              d.alpha->inv[yours[i + you_off]].tname().substr( 0,25).c_str(),
              your_price[i + you_off]);
   if (you_off > 0)
    mvwprintw(w_you, 19, 1, "< Back");
   if (you_off + 17 < yours.size())
    mvwprintw(w_you, 19, 9, "More >");
   wrefresh(w_head);
   wrefresh(w_them);
   wrefresh(w_you);
  }	// Done updating the screen
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
   w_tmp = newwin(3, 21, 1, 30);
   mvwprintz(w_tmp, 1, 1, c_red, "Examine which item?");
   wborder(w_tmp, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                  LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
   wrefresh(w_tmp);
   help = getch();
   help -= 'a';
   werase(w_tmp);
   delwin(w_tmp);
   wrefresh(w_head);
   if (focus_them) {
    if (help >= 0 && help < theirs.size())
     popup(d.beta->inv[theirs[help]].info().c_str());
   } else {
    if (help >= 0 && help < yours.size())
     popup(d.alpha->inv[theirs[help]].info().c_str());
   }
   break;
  case '\n':	// Check if we have enough cash...
   if (cash < 0 && d.alpha->cash < cash * -1) {
    popup("Not enough cash!  You have $%d, price is $%d.", d.alpha->cash, cash);
    update = true;
    ch = ' ';
   } else if (cash > 0 && d.beta->cash < cash) {
    popup("They don't have enough cash!");
    update = true;
    ch = ' ';
   }
   break;
  default:	// Letters & such
   if (ch >= 'a' && ch <= 'z') {
    ch -= 'a';
    if (focus_them) {
     if (ch < theirs.size()) {
      getting_theirs[ch] = !getting_theirs[ch];
      if (getting_theirs[ch])
       cash -= their_price[ch];
      else
       cash += their_price[ch];
      update = true;
     }
    } else {	// Focus is on the player's inventory
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
 } while (ch != KEY_ESCAPE && ch != '\n');

 if (ch == '\n') {
  int practice = 0;
  std::vector<char> removing;
  debugmsg("Old size: %d", d.beta->inv.size());
  for (int i = 0; i < yours.size(); i++) {
   if (getting_yours[i]) {
    d.beta->inv.push_back(d.alpha->inv[yours[i]]);
    practice++;
    debugmsg("Gave a %s", d.alpha->inv[yours[i]].tname().c_str());
    removing.push_back(d.alpha->inv[yours[i]].invlet);
   }
  }
  debugmsg("New size: %d", d.beta->inv.size());
// Do it in two passes, so removing items doesn't corrupt yours[]
  for (int i = 0; i < removing.size(); i++)
    d.alpha->i_rem(removing[i]);

  inventory newinv;
  for (int i = 0; i < theirs.size(); i++) {
   item tmp = d.beta->inv[theirs[i]];
   if (getting_theirs[i]) {
    practice += 2;
    tmp.invlet = 'a';
    while (d.alpha->has_item(tmp.invlet)) {
     if (tmp.invlet == 'z')
      tmp.invlet = 'A';
     else if (tmp.invlet == 'Z')
      return false;	// TODO: Do something else with these.
     else
      tmp.invlet++;
    }
    d.alpha->inv.push_back(tmp);
   } else
    newinv.push_back(tmp);
  }
  d.alpha->practice(sk_barter, practice / 2);
  d.beta->inv = newinv;
  d.alpha->cash += cash;
  d.beta->cash  -= cash;
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
