#include "mission.h"
#include "game.h"

mission mission_type::create(game *g, int npc_id)
{
 mission ret;
 ret.uid = g->assign_mission_id();
 ret.type = this;
 ret.npc_id = npc_id;
 ret.item_id = item_id;
 ret.value = value;
 ret.follow_up = follow_up;

 if (deadline_low != 0 || deadline_high != 0)
  ret.deadline = int(g->turn) + rng(deadline_low, deadline_high);
 else
  ret.deadline = 0;

 return ret;
}

std::string mission::name()
{
 if (type == NULL)
  return "NULL";
 return type->name;
}

std::string mission::save_info()
{
 std::stringstream ret;
 if (type == NULL)
  ret << -1;
 else
  ret << type->id;
 ret << description << " <> " << (failed ? 1 : 0) << " " << value <<
        " " << reward.type << " " << reward.value << " " << reward.item_id <<
        " " << (reward.skill?reward.skill->id():0) << " " << uid << " " << target.x << " " <<
        target.y << " " << item_id << " " << count << " " << deadline << " " <<
        npc_id << " " << good_fac_id << " " << bad_fac_id << " " << step <<
        " " << follow_up;

 return ret.str();
}

void mission::load_info(game *g, std::ifstream &data)
{
 int type_id, rewtype, reward_id, rew_skill, tmpfollow;
 std::string rew_item, itemid;
 data >> type_id;
 type = &(g->mission_types[type_id]);
 std::string tmpdesc;
 do {
  data >> tmpdesc;
  if (tmpdesc != "<>")
   description += tmpdesc + " ";
 } while (tmpdesc != "<>");
 description = description.substr( 0, description.size() - 1 ); // Ending ' '
 data >> failed >> value >> rewtype >> reward_id >> rew_item >> rew_skill >>
         uid >> target.x >> target.y >> itemid >> count >> deadline >> npc_id >>
         good_fac_id >> bad_fac_id >> step >> tmpfollow;
 follow_up = mission_id(tmpfollow);
 reward.type = npc_favor_type(reward_id);
 reward.item_id = itype_id( rew_item );
 reward.skill = Skill::skill( rew_skill );
 item_id = itype_id(itemid);
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
   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You're lying, I can tell!  Ugh, forget it!";
   case TALK_MISSION_FAILURE:
    return "How am I not dead already?!";
  }
  break;

 case MISSION_GET_SOFTWARE:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Oh man, I can't believe I forgot to download it...";
   case TALK_MISSION_OFFER:
    return "There's some important software on my computer that I need on USB.";
   case TALK_MISSION_ACCEPTED:
    return "\
Thanks!  Just pull the data onto this USB drive and bring it to me.";
   case TALK_MISSION_REJECTED:
    return "Seriously?  It's an easy job...";
   case TALK_MISSION_ADVICE:
    return "Take this USB drive.  Use the console, and download the software.";
   case TALK_MISSION_INQUIRE:
    return "So, do you have my software yet?";
   case TALK_MISSION_SUCCESS:
    return "Excellent, thank you!";
   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You liar!";
   case TALK_MISSION_FAILURE:
    return "Wow, you failed?  All that work, down the drain...";
  }
  break;

 case MISSION_GET_ZOMBIE_BLOOD_ANAL:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "\
It could be very informative to perform an analysis of zombie blood...";
   case TALK_MISSION_OFFER:
    return "\
I need someone to get a sample of zombie blood, take it to a hospital, and \
perform a centrifuge analysis of it.";
   case TALK_MISSION_ACCEPTED:
    return "\
Excellent.  Take this vacutainer; once you've produced a zombie corpse, use it \
to extrace blood from the body, then take it to a hospital for analysis.";
   case TALK_MISSION_REJECTED:
    return "\
Are you sure?  The scientific value of that blood data could be priceless...";
   case TALK_MISSION_ADVICE:
    return "\
The centrifuge is a bit technical; you might want to study up on the usage of \
computers before completing that part.";
   case TALK_MISSION_INQUIRE:
    return "Well, do you have the data yet?";
   case TALK_MISSION_SUCCESS:
    return "Excellent!  This may be the key to removing the infection.";
   case TALK_MISSION_SUCCESS_LIE:
    return "Wait, you couldn't possibly have the data!  Liar!";
   case TALK_MISSION_FAILURE:
    return "What a shame, that data could have proved invaluable...";
  }
  break;

 case MISSION_RESCUE_DOG:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Oh, my poor puppy...";

   case TALK_MISSION_OFFER:
    return "\
I left my poor dog in a house, not far from here.  Can you retrieve it?";

   case TALK_MISSION_ACCEPTED:
    return "\
Thank you!  Please hurry back!";

   case TALK_MISSION_REJECTED:
    return "\
Please, think of my poor little puppy!";

   case TALK_MISSION_ADVICE:
    return "\
Take my dog whistle; if the dog starts running off, blow it and he'll return \
to your side.";

   case TALK_MISSION_INQUIRE:
    return "\
Have you found my dog yet?";

   case TALK_MISSION_SUCCESS:
    return "\
Thank you so much for finding him!";

   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You're lying, I can tell!  Ugh, forget it!";

   case TALK_MISSION_FAILURE:
    return "Oh no!  My poor puppy...";

  }
  break;

 case MISSION_KILL_ZOMBIE_MOM:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Oh god, I can't believe it happened...";
   case TALK_MISSION_OFFER:
    return "\
My mom... she's... she was killed, but then she just got back up... she's one \
of those things now.  Can you put her out of her misery for me?";
   case TALK_MISSION_ACCEPTED:
    return "Thank you... she would've wanted it this way.";
   case TALK_MISSION_REJECTED:
    return "Please reconsider, I know she's suffering...";
   case TALK_MISSION_ADVICE:
    return "Find a gun if you can, make it quick...";
   case TALK_MISSION_INQUIRE:
    return "Well...?  Did you... finish things for my mom?";
   case TALK_MISSION_SUCCESS:
    return "Thank you.  I couldn't rest until I knew that was finished.";
   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You're lying, I can tell!  Ugh, forget it!";
   case TALK_MISSION_FAILURE:
    return "Really... that's too bad.";
  }
  break;

 case MISSION_GET_FLAG:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Does she still yet wave?";
   case TALK_MISSION_OFFER:
    return "Does she still yet wave? We're battered but not yet out of the\
 fight, we need the colors!";
   case TALK_MISSION_ACCEPTED:
    return "\
Hell ya!  Find me one of those big ol' American flags.";
   case TALK_MISSION_REJECTED:
    return "Seriously?  God damned commie...";
   case TALK_MISSION_ADVICE:
    return "Find a large federal building or school, they must have one.";
   case TALK_MISSION_INQUIRE:
    return "Rescued the standard yet?";
   case TALK_MISSION_SUCCESS:
    return "America, hell ya!";
   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You liar!";
   case TALK_MISSION_FAILURE:
    return "You give up?  This country fell apart becuase no one could find a\
good man to rely on... might as well give up, I guess.";
  }
  break;

   case MISSION_GET_RELIC:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "St. Michael the archangel defend me in battle...";
   case TALK_MISSION_OFFER:
    return "As the world seems to abandon the reality that we once knew, it \
becomes plausable that the old superstitions that were cast aside may \
have had some truth to them.  Please go and find me a religious relic...\
I doubt it will be of much use but I've got to hope in something.";
   case TALK_MISSION_ACCEPTED:
    return "\
I wish you the best of luck, may whatever god you please guide your path.";
   case TALK_MISSION_REJECTED:
    return "Ya, I guess the stress may just be getting to me...";
   case TALK_MISSION_ADVICE:
    return "I suppose a large church or cathedral may have something.";
   case TALK_MISSION_INQUIRE:
    return "Any luck?";
   case TALK_MISSION_SUCCESS:
    return "Thankyou, I need some time alone now...";
   case TALK_MISSION_SUCCESS_LIE:
    return "What good does this do us?";
   case TALK_MISSION_FAILURE:
    return "It was a lost cause anyways...";
  }
  break;

   case MISSION_GET_RECORD_WEATHER:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "I wonder if a retreat might exist...";
   case TALK_MISSION_OFFER:
    return "Everyone who dies gets back up, right?  Which means that whatever \
is causing this it must be airborne to have infected everyone.  I believe that \
if that is the case then there should be regions that were not downwind from \
where-ever the disease was released.  We need to find a record of all the \
weather paterns leading up to the outbreak.";
   case TALK_MISSION_ACCEPTED:
    return "\
Thanks so much, you may save both of us yet.";
   case TALK_MISSION_REJECTED:
    return "Ya, it was a long shot I admit.";
   case TALK_MISSION_ADVICE:
    return "I'm not sure, maybe a news station would have what we are looking?";
   case TALK_MISSION_INQUIRE:
    return "Any luck?";
   case TALK_MISSION_SUCCESS:
    return "These look more complicated than I thought, just give me some time.";
   case TALK_MISSION_SUCCESS_LIE:
    return "This isn't what we need.";
   case TALK_MISSION_FAILURE:
    return "If only we could find a great valley or something.";
  }
  break;

   case MISSION_GET_RECORD_PATIENT:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "I hope I don't see many names I know...";
   case TALK_MISSION_OFFER:
    return "I've lost so many friends... please find me a patient list from the \
regional hospital or doctor's office.  I just want to know who might still be out there.";
   case TALK_MISSION_ACCEPTED:
    return "\
Thankyou, I suppose it wont change what has already happened but it will bring me closure.";
   case TALK_MISSION_REJECTED:
    return "Please, I just want to know what happened to everyone.";
   case TALK_MISSION_ADVICE:
    return "I bet you'll run into a lot of those things in the hospital, please be careful.";
   case TALK_MISSION_INQUIRE:
    return "Any luck?";
   case TALK_MISSION_SUCCESS:
    return "Oh dear, I thought Timmy would have made it...";
   case TALK_MISSION_SUCCESS_LIE:
    return "Thanks for trying... I guess.";
   case TALK_MISSION_FAILURE:
    return "I bet some of them are still out there...";
  }
  break;

   case MISSION_GET_RECORD_ACCOUNTING:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Those twisted snakes...";
   case TALK_MISSION_OFFER:
    return "Our world fell apart because our leaders were as crooked as the conmen that \
paid for their elections.  Just find me one of those corporate accounting books and I'll \
show you and the rest of the world just who is at fault.";
   case TALK_MISSION_ACCEPTED:
    return "\
You'll see, I know I'm right.";
   case TALK_MISSION_REJECTED:
    return "I know it isn't pressing but the big corporations didn't get a chance to destroy \
the evidence yet.";
   case TALK_MISSION_ADVICE:
    return "Try a big corporate building of some sort, they're bound to have an accounting department.";
   case TALK_MISSION_INQUIRE:
    return "Any luck?";
   case TALK_MISSION_SUCCESS:
    return "Great, let's see... uh... hmmm...  Fine, I didn't even do my own taxes but I'm sure this \
will prove their guilt if we get an expert to examine it.";
   case TALK_MISSION_SUCCESS_LIE:
    return "Thanks for trying... I guess.";
   case TALK_MISSION_FAILURE:
    return "The day of reconing will come for the corporations if it hasn't already.";
  }
  break;

 default:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
   case TALK_MISSION_OFFER:
   case TALK_MISSION_ACCEPTED:
   case TALK_MISSION_REJECTED:
   case TALK_MISSION_ADVICE:
   case TALK_MISSION_INQUIRE:
   case TALK_MISSION_SUCCESS:
   case TALK_MISSION_SUCCESS_LIE:
   case TALK_MISSION_FAILURE:
    return "Someone forgot to code this message!";
  }
  break;
 }
 return "Someone forgot to code this message!";
}

