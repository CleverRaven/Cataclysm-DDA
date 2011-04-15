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

void say_hello		(game *g, dialogue &d);
void say_ask_for	(game *g, dialogue &d, npc_need need);
void say_put_em_up	(game *g, dialogue &d);
void say_listen		(game *g, dialogue &d);
void say_listen_need	(game *g, dialogue &d);
void say_give_advice	(game *g, dialogue &d);
void say_listen_about	(game *g, dialogue &d);
void say_why_join	(game *g, dialogue &d);

bool trade(game *g, dialogue &d, int cost, std::string deal);

void npc::talk_to_u(game *g)
{
 moves -= 100;
 if (attitude == NPCATT_TALK)
  attitude = NPCATT_NULL;
 dialogue d;
 d.alpha = &g->u;
 d.beta = this;
 d.done = false;
 d.win = newwin(25, 80, 0, 0);
 wborder(d.win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 for (int i = 1; i < 24; i++)
  mvwputch(d.win, i, 41, c_ltgray, LINE_XOXO);
 mvwputch(d.win,  0, 41, c_ltgray, LINE_OXXX);
 mvwputch(d.win, 24, 41, c_ltgray, LINE_XXOX);
 mvwprintz(d.win, 1,  1, c_white, "Dialogue with %s", name.c_str());
 mvwprintz(d.win, 1, 43, c_white, "Your response:");
 decide_needs();

// Decide on opening line
 do { 
  if (op_of_u.fear  < personality.bravery &&
      op_of_u.value < personality.aggression * 2)
   say_put_em_up(g, d);
  else if (attitude == NPCATT_TALK && needs.size() > 0 && needs[0] != need_none)
   say_ask_for(g, d, needs[0]);
  else if (attitude == NPCATT_TALK)
   say_hello(g, d);
  else
   say_listen(g, d);
 } while (!d.done);
 delwin(d.win);
 g->refresh_all();
}

void say_hello(game *g, dialogue &d)
{
 talk_topic topic = TALK_NONE;
 int opt = d.opt("Hello.", "\"Let's travel together.\"", "\"Leave me alone.\"",
                 "Ignore them", NULL);
 switch (opt) {
  case 1:
   say_why_join(g, d);
   break;
  case 2:
   d.done = true;
   d.beta->attitude = NPCATT_NULL;
   break;
  case 0:
   d.done = true;
   if (d.beta->op_of_u.fear <= 0 && d.beta->op_of_u.value > 5)
    d.beta->attitude = NPCATT_FOLLOW;
   else
    d.beta->attitude = NPCATT_NULL;
   break;
 }
}

// need is what we have determined to be our dearest need at the moment.
// This is determined in npc::decide_needs(), found in npc.cpp
// See dialogue.h for the strings talk_needs
void say_ask_for(game *g, dialogue &d, npc_need need)
{
 int opt = d.opt(talk_needs[need][rng(0, 4)],
                 "\"Okay, let's trade.\"",
                 "&\"Follow me, we'll search together.\"",
                 "\"I don't have any, but...\"", "Ignore them", NULL);
 switch (opt) {
  case 1:
   trade(g, d, 0, "");
   wrefresh(d.win);
   break;
  case 2:
   d.done = true;
// TODO: Maybe not follow
   d.beta->attitude = NPCATT_FOLLOW;
   break;
  case 3:
   say_listen(g, d);
   break;
  case 0:
   d.done = true;
   if (d.beta->op_of_u.fear <= 0 && d.beta->op_of_u.value > 5)
    d.beta->attitude = NPCATT_FOLLOW;
   else
    d.beta->attitude = NPCATT_NULL;
   break;
 }
}

void say_put_em_up(game *g, dialogue &d)
{
 int opt;
 if (d.alpha->weapon.type->id == 0) {
  opt = d.opt(talk_hands_up[rng(0, 9)],
              "*Put hands up", "!Exit dialogue", NULL);
  switch (opt) {
   case 1:
    d.beta->attitude = NPCATT_MUG;
    d.done = true;
    break;
   case 0:
    d.beta->attitude = NPCATT_KILL;
    d.beta->op_of_u.trust -= 8;
    d.done = true;
  }
 } else {
  opt = d.opt("<drop_it>",
              "Drop weapon", "!Exit dialogue", NULL);
  switch (opt) {
  case 1:
   g->m.add_item(d.alpha->posx, d.alpha->posy, d.alpha->remove_weapon());
   say_put_em_up(g, d);
   break;
  case 0:
   d.beta->attitude = NPCATT_KILL;
   d.beta->op_of_u.trust -= 10;
   d.done = true;
   break;
  }
 }
}

void say_listen(game *g, dialogue &d)
{
 if (d.beta->is_following()) {
  int opt = d.opt("Yes?", "\"I need...",
                          "\"Tell me about...",
                          "\"Let's do some trading.\"",
                          "\"We need to get out of here!\"",
                          (d.beta->attitude == NPCATT_DEFEND ?
                           "\"I'm okay, just watch your own ass.\"" :
                           "\"Help me!\""),
                          (d.beta->attitude == NPCATT_WAIT ?
                           "\"Okay, let's go.\"" :
                           "\"Wait here.\""),
                          "\"Stop following me.\"",
                          "\"Nothing, let's go.\"", NULL);
  switch (opt) {
  case 1:
   say_listen_need(g, d);
   break;
  case 2:
   say_listen_about(g, d);
   break;
  case 3:
   if (trade(g, d, 0, ""))
    d.done = true;
   wrefresh(d.win);
   break;
  case 4:
   d.beta->attitude = NPCATT_FOLLOW_RUN;
   d.done = true;
   break;
  case 5:
   if (d.beta->attitude == NPCATT_DEFEND)
    d.beta->attitude = NPCATT_FOLLOW;
   else
    d.beta->told_to_help(g);
   d.done = true;
   break;
  case 6:
   if (d.beta->attitude == NPCATT_WAIT)
    d.beta->attitude = NPCATT_FOLLOW;
   else
    d.beta->told_to_wait(g);
   d.done = true;
   break;
  case 7:
   d.beta->told_to_leave(g);
   d.done = true;
   break;
  case 0:
   d.done = true;
   break;
  }
 } else {
  int opt = d.opt("Yes?", "\"I need...\"",
                          "\"Want to travel together?\"", 
                          "\"Want to trade?\"",
                          "\"Nothing, bye.\"", NULL);
  switch (opt) {
  case 1:
   say_listen_need(g, d);
   break;
  case 2:
   say_why_join(g, d);
   break;
  case 3:
   if (trade(g, d, 0, ""))
    d.done = true;
   wrefresh(d.win);
   break;
  case 0:
   d.done = true;
   break;
  }
 }
}

void say_listen_need(game *g, dialogue &d)
{
 std::string response;
 point lookp;
 npc_opinion opinion = d.beta->op_of_u;
 int opt2;
 int opt = d.opt("What do you need, <name_g>?",
                 "\"Healing.\"",
                 "\"Directions to...\"",
                 "\"Advice!\"",
                 "\"Never mind.\"",
                 "\"Bye.\"", NULL);
 switch (opt) {
 case 1:
  opinion.value--;
  if (opinion.fear > 0)
   opinion.fear--;
  if (opinion.value * 4 + opinion.trust + d.beta->personality.altruism * 3 +
      (d.beta->fac_has_value(FACVAL_CHARITABLE)    ?  5 : 0) +
      (d.beta->fac_has_job(FACJOB_DOCTORS) ? 15 : 0) -
      opinion.fear * 3 >= 25) {
   if (d.beta->has_amount(itm_bandages,1) || d.beta->has_amount(itm_1st_aid,1)){
    d.beta->attitude = NPCATT_HEAL;
    d.done = true;
    d.beta->say(g, "Okay, hold still and I'll heal you.");
   } else
    d.beta->say(g, "I would if I had some medical supplies...");
  } else {
   d.done = true;
   d.beta->say(g, "Sorry... I can't help you.");
  }
  break;
 case 2:
  do {
   opt2 = d.opt("Directions to what?",
                "\"A gun store.\"",
                "\"A pharmacy.\"",
                "\"A grocery store.\"",
                "\"A liquor store.\"",
                "\"A hardware store.\"",
                "\"Bye.\"", NULL);
   oter_id look;
   switch (opt2) {
    case 1: look = ot_s_gun_north;      break;
    case 2: look = ot_s_pharm_north;    break;
    case 3: look = ot_s_grocery_north;  break;
    case 4: look = ot_s_liquor_north;   break;
    case 5: look = ot_s_hardware_north; break;
    case 0: d.done = true;              break;
   }
   if (opt2 != 0) {
    int dist = 50;
    lookp = g->cur_om.find_closest(point(g->levx / 2, g->levy / 2), look, 4,
                                   dist, false);
    if (lookp.x != -1) {
     lookp.x = lookp.x * 2 - 1;
     lookp.y = lookp.y * 2 - 1;
     if (lookp.x == g->levx && lookp.y == g->levy)
      response = "We're standing in one!";
     else {
      std::stringstream msg;
      std::string unit = "feet";
      dist *= SEEX * 3;
      msg << "There's one to the " <<
             direction_from(g->levx, g->levy, lookp.x, lookp.y);
      if (dist >= 50000)
       msg << ", many miles away.";
      else if (dist >= 15000)
       msg << ", a few miles away.";
      else if (dist >= 9000)
       msg << ", a couple miles away.";
      else if (dist >= 5000)
       msg << ", about a mile away.";
      else if (dist >= 2500)
       msg << ", about half a mile away.";
      else {
       dist = int(dist / 50) * 50;
       if (dist == 0)
        msg << ", like... right there.";
       else
        msg << ", about " << dist << " feet away.";
      }
      response = msg.str();
     }
    } else
     response = "I honestly have no idea.";
    opt2 = d.opt(response,
                 "\"Thanks.  I need more directions.\"",
                 "\"Thanks.  Something else...\"",
                 "\"Thanks, that's all.\"", NULL);
   }
  } while (opt2 == 1);
  if (opt2 == 2)
   say_listen(g, d);
  if (opt2 == 0)
   d.done = true;
  break;
 case 3:
  say_give_advice(g, d);
  break;
 case 4:
  say_listen(g, d);
  break;
 case 0:
  d.done = true;
  break;
 }
}

void say_give_advice(game *g, dialogue &d)
{
 std::ifstream fin;
 fin.open("data/NPC_HINTS");
 if (!fin.is_open()) {
  debugmsg("Couldn't open NPC_HINTS.");
  d.done = true;
  return;
 }
// Count the number of lines in NPC_HINTS
 int num_hints = 0;
 std::string hint;
 while (getline(fin, hint)) {
  if (hint.length() > 2)	// Ignore empty or near-empty lines
   num_hints++;
 }
 int nhint = rng(1, num_hints);
 int cur = 0;
 fin.clear();
 fin.seekg(0, std::ios::beg);
 fin.clear();
 while (cur < nhint && getline(fin, hint)) {
  if (hint.length() > 2)
   cur++;
 }

 int opt = d.opt(hint,
                 "\"Thanks.  Anything else?\"",
                 "\"Thanks.  That's all I need.\"",
                 "\"Thanks.  Bye.\"", NULL);
 switch (opt) {
  case 1: say_give_advice(g, d);break;
  case 2: say_listen(g, d);	break;
  case 0: d.done = true;	break;
 }
}

void say_listen_about(game *g, dialogue &d)
{
 int opt = d.opt("About what?",
                 "\"About you.\"",
                 "\"About your faction.\"",
                 "\"About other factions.\"",
                 "\"Never mind.\"",
                 "\"Never mind, bye.\"", NULL);
 std::string describe;
 switch (opt) {
 case 1:
  //describe = d.beta->describe_self();
  describe = "Not coded yet.";
  break;
 case 2:
  if (d.beta->my_fac == NULL)
   describe = talk_no_faction[rng(0, 9)];
  else
   describe = d.beta->my_fac->describe();
  break;
 case 3:
  debugmsg("Not coded yet.");
  d.done = true;
 case 4:
  say_listen(g, d);
  break;
 case 0:
  d.done = true;
  break;
 }
 if (opt == 1 || opt == 2) {
  switch(d.opt(describe,
               "\"Okay, another question...\"",
               "\"Thanks, that's it.\"", NULL)) {
   case 1: say_listen_about(g, d); break;
   case 0: d.done = true; break;
  }
 }
}
 
void say_why_join(game *g, dialogue &d)
{
 int opt = d.opt("Why should I travel with you?",
                 "\"We'll be safer together.\"",
                 "\"I'll pay you.\"",
                 "!\"I'll kill you if you don't.\"",
                 "\"Never mind.  Bye.\"", NULL);
 bool will_travel = false;
 switch (opt) {
 case 1:
  d.alpha->practice(sk_speech, 4);
  if (d.beta->wants_to_travel_with(d.alpha)) {
   d.beta->op_of_u.value++;
   d.beta->op_of_u.trust++;
   d.beta->say(g, "Alright, let's do it!");
   will_travel = true;
  }
  break;
 case 2:
 // if (trade(g, d, d.beta->price_to_follow())) {
  if (trade(g, d, -1000, "Hire: Travel together")) {
   d.beta->op_of_u.value--;
   d.beta->op_of_u.trust += 2;
   d.beta->op_of_u.fear--;
   will_travel = true;
   d.beta->say(g, "You've got yourself a deal!");
  }
  wrefresh(d.win);
  break;
 case 3:
  d.beta->op_of_u.trust -= 8;
  d.beta->op_of_u.value -= 3;
  d.beta->op_of_u.fear++;
  if (d.beta->op_of_u.fear - d.beta->personality.bravery -
      d.beta->personality.aggression < 0) {
   d.beta->op_of_u.value++;
   d.beta->say(g, "Fine, you monster!");
   will_travel = true;
  } else if (d.beta->op_of_u.fear - 2 * d.beta->personality.aggression -
             d.beta->personality.bravery < -4) {
   d.beta->say(g, "Never!");
   d.beta->attitude = NPCATT_KILL;
   d.done = true;
  } else {
   d.beta->attitude = NPCATT_FLEE;
   d.done = true;
  }
  break;
 case 0:
  d.done = true;
  break;
 }
 if (will_travel) {
  if (opt == 3)
   d.beta->attitude = NPCATT_SLAVE;
  else
   d.beta->attitude = NPCATT_FOLLOW;
  d.done = true;
 } else if (d.done == false) {
  opt = d.opt("Eh.  No thanks.", "\"Oh.  But what if...\"",
                                 "\"That's fine.  How about...\"",
                                 "\"Alright.  Bye.\"", NULL);
  switch (opt) {
   case 1: say_why_join(g, d); break;
   case 2: say_listen(g, d);   break;
   case 0: d.done = true;      break;
  }
 }
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
    debugmsg("Bad tag. %s (%d - %d)", tag.c_str(), fa, fb);
    phrase.replace(fa, fb - fa + 1, "????");
   }
  }
 } while (fa != std::string::npos && fb != std::string::npos);
}
 
int dialogue::opt(std::string challenge, ...)
{
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

 va_list ap;
 va_start(ap, challenge);
 std::vector<std::string> options;
 char* tmp;
 while (tmp = va_arg(ap, char*))
  options.push_back(tmp);
 va_end(ap);
 nc_color cols[options.size()];

 for (int i = 0; i < options.size(); i++) {
  if (options[i][0] == '!') {
   cols[i] = c_red;
   options[i] = options[i].substr(1);
  } else if (options[i][0] == '*') {
   cols[i] = c_ltred;
   options[i] = options[i].substr(1);
  } else if (options[i][0] == '&') {
   cols[i] = c_green;
   options[i] = options[i].substr(1);
  } else {
   cols[i] = c_white;
  }
  std::stringstream msg;
  if (i < options.size() - 1)
   msg << i + 1 << ": " << options[i];
  else
   msg << "0: " << options[i];
  options[i] = msg.str();
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
   mvwprintz(win, curline, 42, cols[i], options[i].substr(0, split).c_str());
   options[i] = "  " + options[i].substr(split);
   curline++;
  }
  mvwprintz(win, curline, 42, cols[i], options[i].c_str());
  curline++;
 }

 wrefresh(win);

 int ch, r;
 bool okay;
 do {
  do {
   ch = getch() - '0';
   r = ch - 1;
   if (r < 0)
    r += options.size();
  } while (ch < 0 || ch >= options.size());
  okay = false;
  if (cols[r] == c_white || cols[r] == c_green)
   okay = true;
  if (cols[r] == c_red && query_yn("You may be attacked! Proceed?"))
   okay = true;
  if (cols[r] == c_ltred && query_yn("You'll be helpless! Proceed?"))
   okay = true;
 } while (!okay);
 history.push_back("");
 if (ch == 0)
  history.push_back("You: " + options[options.size() - 1].substr(3));
 else
  history.push_back("You: " + options[ch - 1].substr(3));
 return ch;
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

  std::vector<item> newinv;
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
