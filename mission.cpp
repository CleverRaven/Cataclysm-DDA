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
            case 1: return _("Hey <name_g>... I really need your help...");
            case 2: return _("<swear!><punc> I'm hurting...");
            case 3: return _("This infection is bad, <very> bad...");
            case 4: return _("Oh god, it <swear> hurts...");
            }
            break;
        case TALK_MISSION_OFFER:
            return _("\
I'm infected.  Badly.  I need you to get some antibiotics for me...");
        case TALK_MISSION_ACCEPTED:
            return _("\
Oh, thank god, thank you so much!  I won't last more than a couple of days, \
so hurry...");
        case TALK_MISSION_REJECTED:
            return _("\
What?!  Please, <ill_die> without your help!");
        case TALK_MISSION_ADVICE:
            return _("\
There's a town nearby.  Check pharmacies; it'll be behind the counter.");
        case TALK_MISSION_INQUIRE:
            return _("Find any antibiotics yet?");
        case TALK_MISSION_SUCCESS:
            return _("Oh thank god!  I'll be right as rain in no time.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  You're lying, I can tell!  Ugh, forget it!");
        case TALK_MISSION_FAILURE:
            return _("How am I not dead already?!");
        }
        break;

    case MISSION_GET_SOFTWARE:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Oh man, I can't believe I forgot to download it...");
        case TALK_MISSION_OFFER:
            return _("There's some important software on my computer that I need on USB.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Thanks!  Just pull the data onto this USB drive and bring it to me.");
        case TALK_MISSION_REJECTED:
            return _("Seriously?  It's an easy job...");
        case TALK_MISSION_ADVICE:
            return _("Take this USB drive.  Use the console, and download the software.");
        case TALK_MISSION_INQUIRE:
            return _("So, do you have my software yet?");
        case TALK_MISSION_SUCCESS:
            return _("Excellent, thank you!");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  You liar!");
        case TALK_MISSION_FAILURE:
            return _("Wow, you failed?  All that work, down the drain...");
        }
        break;

    case MISSION_GET_ZOMBIE_BLOOD_ANAL:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("\
It could be very informative to perform an analysis of zombie blood...");
        case TALK_MISSION_OFFER:
            return _("\
I need someone to get a sample of zombie blood, take it to a hospital, and \
perform a centrifuge analysis of it.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Excellent.  Take this vacutainer; once you've produced a zombie corpse, use it \
to extrace blood from the body, then take it to a hospital for analysis.");
        case TALK_MISSION_REJECTED:
            return _("\
Are you sure?  The scientific value of that blood data could be priceless...");
        case TALK_MISSION_ADVICE:
            return _("\
The centrifuge is a bit technical; you might want to study up on the usage of \
computers before completing that part.");
        case TALK_MISSION_INQUIRE:
            return _("Well, do you have the data yet?");
        case TALK_MISSION_SUCCESS:
            return _("Excellent!  This may be the key to removing the infection.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Wait, you couldn't possibly have the data!  Liar!");
        case TALK_MISSION_FAILURE:
            return _("What a shame, that data could have proved invaluable...");
        }
        break;

    case MISSION_RESCUE_DOG:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Oh, my poor puppy...");

        case TALK_MISSION_OFFER:
            return _("\
I left my poor dog in a house, not far from here.  Can you retrieve it?");

        case TALK_MISSION_ACCEPTED:
            return _("\
Thank you!  Please hurry back!");

        case TALK_MISSION_REJECTED:
            return _("\
Please, think of my poor little puppy!");

        case TALK_MISSION_ADVICE:
            return _("\
Take my dog whistle; if the dog starts running off, blow it and he'll return \
to your side.");

        case TALK_MISSION_INQUIRE:
            return _("\
Have you found my dog yet?");

        case TALK_MISSION_SUCCESS:
            return _("\
Thank you so much for finding him!");

        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  You're lying, I can tell!  Ugh, forget it!");

        case TALK_MISSION_FAILURE:
            return _("Oh no!  My poor puppy...");

        }
        break;

    case MISSION_KILL_ZOMBIE_MOM:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Oh god, I can't believe it happened...");
        case TALK_MISSION_OFFER:
            return _("\
My mom... she's... she was killed, but then she just got back up... she's one \
of those things now.  Can you put her out of her misery for me?");
        case TALK_MISSION_ACCEPTED:
            return _("Thank you... she would've wanted it this way.");
        case TALK_MISSION_REJECTED:
            return _("Please reconsider, I know she's suffering...");
        case TALK_MISSION_ADVICE:
            return _("Find a gun if you can, make it quick...");
        case TALK_MISSION_INQUIRE:
            return _("Well...?  Did you... finish things for my mom?");
        case TALK_MISSION_SUCCESS:
            return _("Thank you.  I couldn't rest until I knew that was finished.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  You're lying, I can tell!  Ugh, forget it!");
        case TALK_MISSION_FAILURE:
            return _("Really... that's too bad.");
        }
        break;

//patriot mission 1
    case MISSION_GET_FLAG:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Does our flag still yet wave?");
        case TALK_MISSION_OFFER:
            return _("Does our flag still yet wave? We're battered but not yet out of the\
 fight, we need the old colors!");
        case TALK_MISSION_ACCEPTED:
            return _("\
Hell ya!  Find me one of those big ol' American flags.");
        case TALK_MISSION_REJECTED:
            return _("Seriously?  God damned commie...");
        case TALK_MISSION_ADVICE:
            return _("Find a large federal building or school, they must have one.");
        case TALK_MISSION_INQUIRE:
            return _("Rescued the standard yet?");
        case TALK_MISSION_SUCCESS:
            return _("America, fuck ya!");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  You liar!");
        case TALK_MISSION_FAILURE:
            return _("You give up?  This country fell apart because no one could find a\
good man to rely on... might as well give up, I guess.");
        }
        break;

//patriot mission 2
    case MISSION_GET_BLACK_BOX:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("We've got the flag, now we need to locate US forces.");
        case TALK_MISSION_OFFER:
            return _("We have the flag but now we need to locate US troops to see\
 what we can do to help.  I haven't seen any but I'm figure'n one of\
 those choppers that were fly'n round during th outbreak would have a good\
 idea.  If you can get me a black box from one of the wrecks I'll look\
 into where we might open'er at.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Fuck ya, America!");
        case TALK_MISSION_REJECTED:
            return _("Do you have any better ideas?");
        case TALK_MISSION_ADVICE:
            return _("Survivors were talking about them crashing but I don't know\
 where.  If I were a pilot I'd avoid crash landing in a city or forest\
 though.");
        case TALK_MISSION_INQUIRE:
            return _("How 'bout that black box?");
        case TALK_MISSION_SUCCESS:
            return _("America, fuck ya!");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  I out'ta whip you're ass.");
        case TALK_MISSION_FAILURE:
            return _("Damn, I'll have to find'er myself.");
        }
        break;

//patriot mission 3
    case MISSION_GET_BLACK_BOX_TRANSCRIPT:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("With the black box in hand, we need to find a lab.");
        case TALK_MISSION_OFFER:
            return _("Thanks to your searching we've got the black box but now we\
 need to have a look'n-side her.  Now, most buildings don't have power\
 anymore but there are a few that might be of use.  Have you ever seen\
 one of those science labs that have popped up in the middle of nowhere?\
 Them suckers have a glowing terminal out front so I know they have power\
 somewhere inside'em.  If you can get inside and find a computer lab that\
 still works you ought to be able to find out what's in the black box.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Fuck ya, America!");
        case TALK_MISSION_REJECTED:
            return _("Do you have any better ideas?");
        case TALK_MISSION_ADVICE:
            return _("When I was play'n with the terminal for the one I ran into it\
 kept asking for an ID card.  Finding one would be the first order of business.");
        case TALK_MISSION_INQUIRE:
            return _("How 'bout that black box?");
        case TALK_MISSION_SUCCESS:
            return _("America, fuck ya!  I was in the guard a few years back so I'm\
 confident I can make heads-or-tails of these transmissions.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  I out'ta whip you're ass.");
        case TALK_MISSION_FAILURE:
            return _("Damn, I maybe we can find an egg-head to crack the terminal.");
        }
        break;

//patriot mission 4
    case MISSION_EXPLORE_SARCOPHAGUS:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("You wouldn't believe what I found...");
        case TALK_MISSION_OFFER:
            return _("Holy hell, the crash you recovered the black box from wasn't as old as I thought.  \
Check this out, it was on its approach to pick up a team sent to secure and destroy something called a \
'Hazardous Waste Sarcophagus' in the middle of nowhere.  If the bird never picked up the team then we \
may still have a chance to meet up with them.  It includes an access code for the elevator and an encoded \
message for the team leader, I guess.  If we want to join up with what remains of the government then now \
may be our only chance.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Fuck ya, America!");
        case TALK_MISSION_REJECTED:
            return _("Are you going to forfeit your duty when the country needs you the most?");
        case TALK_MISSION_ADVICE:
            return _("If there is a military team down there then we better go in prepared if we want to \
impress them.  Carry as much ammo as you can and prepare to ditch this place if they have a second \
bird coming to pick them up.");
        case TALK_MISSION_INQUIRE:
            return _("Having any trouble following the map?");
        case TALK_MISSION_SUCCESS:
            return _("We got this shit!");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What?!  I out'ta whip your ass.");
        case TALK_MISSION_FAILURE:
            return _("Damn, we were so close.");
        }
        break;

//martyr mission 1
    case MISSION_GET_RELIC:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("St. Michael the archangel defend me in battle...");
        case TALK_MISSION_OFFER:
            return _("As the world seems to abandon the reality that we once knew, it \
becomes plausible that the old superstitions that were cast aside may \
have had some truth to them.  Please go and find me a religious relic...\
I doubt it will be of much use but I've got to hope in something.");
        case TALK_MISSION_ACCEPTED:
            return _("\
I wish you the best of luck, may whatever god you please guide your path.");
        case TALK_MISSION_REJECTED:
            return _("Ya, I guess the stress may just be getting to me...");
        case TALK_MISSION_ADVICE:
            return _("I suppose a large church or cathedral may have something.");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Thank you, I need some time alone now...");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What good does this do us?");
        case TALK_MISSION_FAILURE:
            return _("It was a lost cause anyways...");
        }
        break;

//martyr mission 2
    case MISSION_RECOVER_PRIEST_DIARY:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("St. Michael the archangel defend me in battle...");
        case TALK_MISSION_OFFER:
            return _("From what I understand, the creatures you encountered \
surrounding this relic were unlike anything I've heard of.  It is \
laughable that I now consider the living dead to be part of our \
ordinary reality.  Never-the-less, the church must have some explanation \
for these events.  I have located the residence of a local clergy man, \
could you go to this address and recover any items that may reveal what \
the church's stance is on these events?");
        case TALK_MISSION_ACCEPTED:
            return _("\
I wish you the best of luck, may whatever god you please guide your path.");
        case TALK_MISSION_REJECTED:
            return _("Ya, I guess the stress may just be getting to me...");
        case TALK_MISSION_ADVICE:
            return _("If the information is confidential the priest must have it hidden \
within his own home.");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Thank you, a diary is exactly what I was looking for.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What good does this do us?");
        case TALK_MISSION_FAILURE:
            return _("It was a lost cause anyways...");
        }
        break;

//martyr mission 3
    case MISSION_INVESTIGATE_CULT:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("St. Michael the archangel defend me in battle...");
        case TALK_MISSION_OFFER:
            return _("You have no idea how interesting this diary is.  I have two \
very promising leads...  First things first, the Catholic Church has been \
performing its own investigations into global cult phenomenon and it appears \
to have become very interested in a local cult as of recently.  Could you investigate \
a location for me?  I'm not sure what was going on here but the priest seemed \
fairly worried about it.");
        case TALK_MISSION_ACCEPTED:
            return _("\
I wish you the best of luck, may whatever god you please guide your path...  You may need it \
this time more than the past excursions you have gone on.  There is a note about potential human \
sacrifice in the days immediately before and after the outbreak.  The name of the cult is believed \
to be the Church of Starry Wisdom but it is noted that accounts differ.");
        case TALK_MISSION_REJECTED:
            return _("Ya, I guess the stress may just be getting to me...");
        case TALK_MISSION_ADVICE:
            return _("I doubt the site is still occupied but I'd carry a firearm at least... I'm not sure what \
you might be looking for but I'm positive you'll find something out of the ordinary if you look long \
enough.");
        case TALK_MISSION_INQUIRE:
            return _("I'm positive there is something there... there has to be, any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Thank you, your account of these... demonic creations proves the fears the churches \
had were well founded.  Our priority should be routing out any survivors of this cult... I don't \
known if they are responsible for the outbreak but they certainly know more about it than I do.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What good does this do us?");
        case TALK_MISSION_FAILURE:
            return _("It was a lost cause anyways...");
        }
        break;

//martyr mission 4
    case MISSION_INVESTIGATE_PRISON_VISIONARY:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("St. Michael the archangel defend me in battle...");
        case TALK_MISSION_OFFER:
            return _("I have another task if you are feeling up to it.  There is a prisoner that \
the priest made special mention of.  I was wondering if you could see \
what may have happened to him or if he left anything in his cell.  The priest admits the individual is rather \
unstable, to put it lightly, but the priest personally believed the man was some kind of repentant visionary.  I'm \
not in a position to cast out the account just yet... it seems the man has prophesied  events accurately before \
concerning the Church of Starry Wisdom.");
        case TALK_MISSION_ACCEPTED:
            return _("\
I wish you the best of luck, may whatever god you please guide your path...  I can only imagine that the prison \
will be a little slice of hell.  I'm not sure what they would have decided to do with the inmates when they knew \
death was almost certain.  ");
        case TALK_MISSION_REJECTED:
            return _("Ya, I guess the stress may just be getting to me...");
        case TALK_MISSION_ADVICE:
            return _("The worst case scenario will probably be that the prisoners have escaped their cells and turned the \
building into their own little fortress.  Best case, the building went into lock-down and secured the prisoners in \
their cells.  Either way, navigating the building will pose its own difficulties.");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Thank you, I'm not sure what to make of this but I'll ponder your account.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("What good does this do us?");
        case TALK_MISSION_FAILURE:
            return _("It was a lost cause anyways...");
        }
        break;

    case MISSION_GET_RECORD_WEATHER:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("I wonder if a retreat might exist...");
        case TALK_MISSION_OFFER:
            return _("Everyone who dies gets back up, right?  Which means that whatever \
is causing this it must be airborne to have infected everyone.  I believe that \
if that is the case then there should be regions that were not downwind from \
where-ever the disease was released.  We need to find a record of all the \
weather patterns leading up to the outbreak.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Thanks so much, you may save both of us yet.");
        case TALK_MISSION_REJECTED:
            return _("Ya, it was a long shot I admit.");
        case TALK_MISSION_ADVICE:
            return _("I'm not sure, maybe a news station would have what we are looking?");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("These look more complicated than I thought, just give me some time.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("This isn't what we need.");
        case TALK_MISSION_FAILURE:
            return _("If only we could find a great valley or something.");
        }
        break;

//humanitarian mission 1
    case MISSION_GET_RECORD_PATIENT:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("I hope I don't see many names I know...");
        case TALK_MISSION_OFFER:
            return _("I've lost so many friends... please find me a patient list from the \
regional hospital or doctor's office.  I just want to know who might still be out there.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Thank you, I suppose it wont change what has already happened but it will bring me closure.");
        case TALK_MISSION_REJECTED:
            return _("Please, I just want to know what happened to everyone.");
        case TALK_MISSION_ADVICE:
            return _("I bet you'll run into a lot of those things in the hospital, please be careful.");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Oh dear, I thought Timmy would have made it...");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Thanks for trying... I guess.");
        case TALK_MISSION_FAILURE:
            return _("I bet some of them are still out there...");
        }
        break;

//humanitarian mission 2
    case MISSION_REACH_FEMA_CAMP:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Maybe they escaped to one of the camps...");
        case TALK_MISSION_OFFER:
            return _("I can't thank you enough for bringing me the patient records but I do have \
another request.  You seem to know your way around... could you take me to one of the \
FEMA camps?  I know some were overrun but I don't want to believe all of them could have \
fallen.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Thank you, just bring me to the camp... I just want to see.");
        case TALK_MISSION_REJECTED:
            return _("Please, I don't know what else to do.");
        case TALK_MISSION_ADVICE:
            return _("We should go at night, if it is overrun then we can quickly make our escape.");
        case TALK_MISSION_INQUIRE:
            return _("Any leads on where a camp might be?");
        case TALK_MISSION_SUCCESS:
            return _("I guess this wasn't as bright an idea as I thought.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Thanks for trying... I guess.");
        case TALK_MISSION_FAILURE:
            return _("I bet some of them are still out there...");
        }
        break;

//humanitarian mission 3
    case MISSION_REACH_FARM_HOUSE:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("I just need a place to start over...");
        case TALK_MISSION_OFFER:
            return _("I've accepted that everyone I used to know is dead... one way or another.  I \
really wish I could have done something to save my brother but he was one of the first to go. \
I'd like to start over, just rebuild at one of the farms in the countryside.  Can you help me \
secure one?");
        case TALK_MISSION_ACCEPTED:
            return _("\
Thank you, let's find a remote one so we don't have to worry about many zombies.");
        case TALK_MISSION_REJECTED:
            return _("Please, I just don't know what to do otherwise.");
        case TALK_MISSION_ADVICE:
            return _("Traveling the backroads would be a good way to search for one.");
        case TALK_MISSION_INQUIRE:
            return _("Shall we keep looking for a farm house?");
        case TALK_MISSION_SUCCESS:
            return _("Well, my adventuring days are over.  I can't thank you enough.  Trying to make \
this place self sustaining will take some work but the future is looking brighter.  At least it \
ought to be safe for now.  You'll always be welcome here.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Thanks for trying... I guess.");
        case TALK_MISSION_FAILURE:
            return _("I guess it was just a pipe dream.");
        }
        break;

//vigilante mission 1
    case MISSION_GET_RECORD_ACCOUNTING:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Those twisted snakes...");
        case TALK_MISSION_OFFER:
            return _("Our world fell apart because our leaders were as crooked as the con-men that \
paid for their elections.  Just find me one of those corporate accounting books and I'll \
show you and the rest of the world just who is at fault.");
        case TALK_MISSION_ACCEPTED:
            return _("\
You'll see, I know I'm right.");
        case TALK_MISSION_REJECTED:
            return _("I know it isn't pressing but the big corporations didn't get a chance to destroy \
the evidence yet.");
        case TALK_MISSION_ADVICE:
            return _("Try a big corporate building of some sort, they're bound to have an accounting department.");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Great, let's see... uh... hmmm...  Fine, I didn't even do my own taxes but I'm sure this \
will prove their guilt if we get an expert to examine it.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Thanks for trying... I guess.");
        case TALK_MISSION_FAILURE:
            return _("The day of reckoning will come for the corporations if it hasn't already.");
        }
        break;

//vigilante mission 2
    case MISSION_GET_SAFE_BOX:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Those twisted snakes...");
        case TALK_MISSION_OFFER:
            return _("Now I don't mean to upset you but I'm not sure what I can do with the accounting ledger at the \
moment.  We do have a new lead though, the ledger has a safe deposit box under the regional manager's name.  \
Guess what, dumb sucker wrote down his combination.  Come with me to retrieve the box and you can keep any \
of the goodies in it that I can't use to press charges against these bastards.");
        case TALK_MISSION_ACCEPTED:
            return _("\
You may make a tidy profit from this.");
        case TALK_MISSION_REJECTED:
            return _("I know it isn't pressing but the world is going to be just as corrupt when we start rebuilding unless \
we take measure to stop those who seek to rule over us.");
        case TALK_MISSION_ADVICE:
            return _("This shouldn't be hard unless we run into a horde.");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Great, anything I can't use to prosecute the bastards is yours, as promised.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Thanks for trying... I guess.");
        case TALK_MISSION_FAILURE:
            return _("The day of reckoning will come for the corporations if it hasn't already.");
        }
        break;

//vigilante mission 3
    case MISSION_GET_DEPUTY_BADGE:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Those twisted snakes...");
        case TALK_MISSION_OFFER:
            return _("I hope you will find use from what you got out of the deposit box but I also have another job for you \
that might lead you to an opportunity to deal out justice for those who cannot.  First things first, we can't just \
look like ruffians.  Find us a deputy badge, easy enough?");
        case TALK_MISSION_ACCEPTED:
            return _("\
I'd check the police station.");
        case TALK_MISSION_REJECTED:
            return _("We're also official... just hang in there and I'll show you what we can really do.");
        case TALK_MISSION_ADVICE:
            return _("They shouldn't be that hard to find... should they?");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("Great work, Deputy.  We're in business.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Thanks for trying... I guess.");
        case TALK_MISSION_FAILURE:
            return _("The day of reckoning will come for the criminals if it hasn't already.");
        }
        break;

//demon slayer mission 1
    case MISSION_KILL_JABBERWOCK:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("The eater of the dead... something was ripping zombies to shreds and only leaving a few scattered limbs...");
        case TALK_MISSION_OFFER:
            return _("A few days ago another survivor and I were trying to avoid the cities by staying in the woods \
during the day and foraging for gear at night. It worked well against the normal zed's but one night something \
caught onto our trail and chased us for ten minutes or so until we decided to split up and meet-up back here.  My \
buddy never showed up and I don't have the means to kill whatever it was.  Can you lend a hand?");
        case TALK_MISSION_ACCEPTED:
            return _("\
Thanks, make sure you're ready for whatever the beast is.");
        case TALK_MISSION_REJECTED:
            return _("Hey, I know I wouldn't volunteer for it either.");
        case TALK_MISSION_ADVICE:
            return _("I'd carry a shotgun at least, it sounded pretty big.");
        case TALK_MISSION_INQUIRE:
            return _("Any luck?");
        case TALK_MISSION_SUCCESS:
            return _("You look a little shaken up, I can't tell you how glad I am that you killed it though.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("Something in the shadows still seems to stare at me when I look at the woods.");
        case TALK_MISSION_FAILURE:
            return _("I'm glad you came back alive... I wasn't sure if I had sent you to your death.");
        }
        break;

//demon slayer mission 2
    case MISSION_KILL_100_Z:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("You seem to know this new world better than most...");
        case TALK_MISSION_OFFER:
            return _("You're kitted out better than most... would you be interested in making this world a \
little better for the rest of us?  The towns have enough supplies for us survivors to \
start securing a foothold but we don't have anyone with the skills and equipment to thin the masses \
of undead.  I'll lend you a hand to the best of my ability but you really \
showed promise taking out that other beast.  You, I, and a 100 regular zombies laid to rest, what do you say?");
        case TALK_MISSION_ACCEPTED:
            return _("\
Hell ya, we may get ourselves killed but we'll be among the first legends of the apocalypse.");
        case TALK_MISSION_REJECTED:
            return _("Hey, I know I wouldn't volunteer for it either... but then I remember that most of us survivors \
won't make it unless someone decides to take the initiative.");
        case TALK_MISSION_ADVICE:
            return _("I'd secure an ammo cache and try to sweep a town in multiple passes.");
        case TALK_MISSION_INQUIRE:
            return _("Got this knocked out?");
        case TALK_MISSION_SUCCESS:
            return _("Man... you're a goddamn machine.  It was a pleasure working with you.  You know, you may just change \
our little neck of the world if you keep this up.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("I don't think that was quite a hundred dead zeds.");
        case TALK_MISSION_FAILURE:
            return _("Quitting already?");
        }
        break;

//demon slayer mission 3
    case MISSION_KILL_HORDE_MASTER:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("I've heard some bad rumors so I hope you are up for another challenge...");
        case TALK_MISSION_OFFER:
            return _("Apparently one of the other survivors picked up on an unusually dense horde of undead \
moving into the area.  At the center of this throng there was a 'leader' of some sort.  The short of it is, \
kill the son of a bitch.  We don't know what it is capable of or why it is surrounded by other zombies but \
this thing reeks of trouble.  Do whatever it takes but we can't risk it getting away.");
        case TALK_MISSION_ACCEPTED:
            return _("\
I'll lend you a hand but I'd try and recruit another gunslinger if you can.");
        case TALK_MISSION_REJECTED:
            return _("What's the use of walking away, they'll track you down eventually.");
        case TALK_MISSION_ADVICE:
            return _("Don't risk torching the building it may be hiding in if it has a basement.  The sucker may still be \
alive under the rubble and ash.");
        case TALK_MISSION_INQUIRE:
            return _("Got this knocked out?");
        case TALK_MISSION_SUCCESS:
            return _("May that bastard never get up again.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("I don't think we got it yet.");
        case TALK_MISSION_FAILURE:
            return _("Quitting already?");
        }
        break;



//demon slayer mission 4
    case MISSION_RECRUIT_TRACKER:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("You seem to know this new world better than most...");
        case TALK_MISSION_OFFER:
            return _("We've got another problem to deal with but I don't think we can handle \
it on our own.  So, I sent word out and found us a volunteer... of sorts.  He's vain \
as hell but has a little skill with firearms.  He was supposed to collect whatever he \
had of value and is going to meet us at a cabin in the woods.  Wasn't sure how long \
we were going to be so I told him to just camp there until we picked him up.");
        case TALK_MISSION_ACCEPTED:
            return _("\
Rodger, if he's a no-show then any other gunslinger will do... but I doubt he'll quit \
before we even begin.");
        case TALK_MISSION_REJECTED:
            return _("Hey, I know I wouldn't volunteer for it either... but then I remember that most of us survivors \
won't make it unless someone decides to take the initiative.");
        case TALK_MISSION_ADVICE:
            return _("I hope the bastard is packing heat... else we'll need to grab him a gun before we hit our next \
target.");
        case TALK_MISSION_INQUIRE:
            return _("Found a gunslinger?");
        case TALK_MISSION_SUCCESS:
            return _("Great, just let me know when you are ready to wade knee-deep in an ocean of blood.");
        case TALK_MISSION_SUCCESS_LIE:
            return _("I don't think so...");
        case TALK_MISSION_FAILURE:
            return _("Quitting already?");
        }
        break;

//demon slayer mission 4b
    case MISSION_JOIN_TRACKER:
        switch (state) {
        case TALK_MISSION_DESCRIBE:
            return _("Well damn, you must be the guys here to pick me up...");
        case TALK_MISSION_OFFER:
            return _("I got the brief so I know what I'm getting into.  Let me be upfront, \
treat me like shit and I'm going to cover my own hide.  Without a strong band a man doesn't \
stand a chance in this world.  You ready to take charge boss?");
        case TALK_MISSION_ACCEPTED:
            return _("\
Before we get into a major fight just make sure we have the gear we need, boss.");
        case TALK_MISSION_REJECTED:
            return _("I don't think you're going to find many other survivors who haven't taken up a faction yet.");
        case TALK_MISSION_ADVICE:
            return _("I'm a pretty good shot with a rifle or pistol.");
        case TALK_MISSION_INQUIRE:
            return _("Any problems boss?");
        case TALK_MISSION_SUCCESS:
            return _("Wait... are you really making me a deputy?");
        case TALK_MISSION_SUCCESS_LIE:
            return _("I don't think so...");
        case TALK_MISSION_FAILURE:
            return _("Quitting already?");
        }
        break;

    default:
        return "Someone forgot to code this message!";
        break;
    }
    return "Someone forgot to code this message!";
}

