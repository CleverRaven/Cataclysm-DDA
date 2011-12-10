#include "mission.h"
#include "game.h"

mission mission_type::create(game *g)
{
 mission ret;
 ret.type = this;
 if (deadline_low != 0 || deadline_high != 0)
  ret.deadline = int(g->turn) + rng(deadline_low, deadline_high);
 else
  ret.deadline = 0;

 ret.item_id = item_id;
 ret.value = value;

 mission_start m_s;
 (m_s.*start)(g, &ret);

 return ret;
}

std::string mission::name()
{
 if (type == NULL)
  return "NULL";
 return type->name;
}

std::string mission_dialogue (mission_id id, talk_topic state)
{
 switch (id) {

 case MISSION_GET_ANTIBIOTICS:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    switch (rng(1, 4)) {
     case 1: return "Hey <name_g>... I really need your help...";
     case 2: return "<swear!><punc> I'm hurting...";
     case 3: return "This infection is bad, <very> bad...";
     case 4: return "Oh god, it <swear> hurts...";
    }
    break;
   case TALK_MISSION_OFFER:
    return "\
I'm infected.  Badly.  I need you to get some antibiotics for me...";
   case TALK_MISSION_ACCEPTED:
    return "\
Oh, thank god, thank you so much!  I won't last more than a couple of days, \
so hurry...";
   case TALK_MISSION_REJECTED:
    return "\
What?!  Please, <ill_die> without your help!";
   case TALK_MISSION_ADVICE:
    return "\
There's a town nearby.  Check pharmacies; it'll be behind the counter.";
   case TALK_MISSION_INQUIRE:
    return "Find any antibiotics yet?";
   case TALK_MISSION_SUCCESS:
    return "Oh thank god!  I'll be right as rain in no time.";
  }
  break;

  default:
   return "Someone forgot to code this message!";
 }
 return "Someone forgot to code this message!";
}

