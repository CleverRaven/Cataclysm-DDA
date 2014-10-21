#include "rng.h"
#include "game.h"
#include "monster.h"
#include "bodypart.h"
#include "disease.h"
#include "weather.h"
#include "translations.h"
#include "martialarts.h"
#include "monstergenerator.h"
#include "messages.h"
#include <stdlib.h>
#include <sstream>
#include <algorithm>

// Used only internally for fast lookups.
enum dis_type_enum {
 DI_NULL,
// Diseases
 DI_RECOVER,
// Monsters
 DI_LYING_DOWN, DI_SLEEP, DI_ALARM_CLOCK,
 DI_BITE,
// Food & Drugs
  DI_DATURA,
// Bite wound infected (dependent on bodypart.h)
 DI_INFECTED,
// Martial arts-related buffs
 DI_MA_BUFF,
 // Lack/sleep
 DI_LACKSLEEP,
 // Grabbed (from MA or monster)
 DI_GRABBED
};

std::map<std::string, dis_type_enum> disease_type_lookup;

// Todo: Move helper functions into a DiseaseHandler Class.
// Should standardize parameters so we can make function pointers.
static void manage_sleep(player& p, disease& dis);

static void handle_bite_wound(player& p, disease& dis);
static void handle_infected_wound(player& p, disease& dis);
static void handle_recovery(player& p, disease& dis);

static bool will_vomit(player& p, int chance = 1000);

void game::init_diseases() {
    // Initialize the disease lookup table.

    disease_type_lookup["null"] = DI_NULL;
    disease_type_lookup["recover"] = DI_RECOVER;
    disease_type_lookup["lying_down"] = DI_LYING_DOWN;
    disease_type_lookup["sleep"] = DI_SLEEP;
    disease_type_lookup["alarm_clock"] = DI_ALARM_CLOCK;
    disease_type_lookup["bite"] = DI_BITE;
    disease_type_lookup["datura"] = DI_DATURA;
    disease_type_lookup["infected"] = DI_INFECTED;
    disease_type_lookup["ma_buff"] = DI_MA_BUFF;
    disease_type_lookup["lack_sleep"] = DI_LACKSLEEP;
    disease_type_lookup["grabbed"] = DI_GRABBED;
}

bool dis_msg(dis_type type_string) {
    dis_type_enum type = disease_type_lookup[type_string];
    switch (type) {
    case DI_LYING_DOWN:
        add_msg(_("You lie down to go to sleep..."));
        break;
    case DI_BITE:
        add_msg(m_bad, _("The bite wound feels really deep..."));
        g->u.add_memorial_log(pgettext("memorial_male", "Received a deep bite wound."),
                              pgettext("memorial_female", "Received a deep bite wound."));
        break;
    case DI_INFECTED:
        add_msg(m_bad, _("Your bite wound feels infected."));
        g->u.add_memorial_log(pgettext("memorial_male", "Contracted an infection."),
                              pgettext("memorial_female", "Contracted an infection."));
        break;
    case DI_LACKSLEEP:
        add_msg(m_warning, _("You are too tired to function well."));
        break;
    case DI_GRABBED:
        add_msg(m_bad, _("You have been grabbed."));
        break;
    default:
        return false;
        break;
    }

    return true;
}

void weed_msg(player *p) {
    int howhigh = p->get_effect_dur("weed_high");
    int smarts = p->get_int();
    if(howhigh > 125 && one_in(7)) {
        int msg = rng(0,5);
        switch(msg) {
        case 0: // Freakazoid
            p->add_msg_if_player(_("The scariest thing in the world would be... if all the air in the world turned to WOOD!"));
            return;
        case 1: // Simpsons
            p->add_msg_if_player(_("Could Jesus microwave a burrito so hot, that he himself couldn't eat it?"));
            p->hunger += 2;
            return;
        case 2:
            if(smarts > 8) { // Timothy Leary
                p->add_msg_if_player(_("Science is all metaphor."));
            } else if(smarts < 3){ // It's Always Sunny in Phildelphia
                p->add_msg_if_player(_("Science is a liar sometimes."));
            } else { // Durr
                p->add_msg_if_player(_("Science is... wait, what was I talking about again?"));
            }
            return;
        case 3: // Dazed and Confused
            p->add_msg_if_player(_("Behind every good man there is a woman, and that woman was Martha Washington, man."));
            if(one_in(2)) {
                p->add_msg_if_player(_("Every day, George would come home, and she would have a big fat bowl waiting for him when he came in the door, man."));
                if(one_in(2)) {
                    p->add_msg_if_player(_("She was a hip, hip, hip lady, man."));
                }
            }
            return;
        case 4:
            if(p->has_amount("money_bundle", 1)) { // Half Baked
                p->add_msg_if_player(_("You ever see the back of a twenty dollar bill... on weed?"));
                if(one_in(2)) {
                    p->add_msg_if_player(_("Oh, there's some crazy shit, man. There's a dude in the bushes. Has he got a gun? I dunno!"));
                    if(one_in(3)) {
                        p->add_msg_if_player(_("RED TEAM GO, RED TEAM GO!"));
                    }
                }
            } else if(p->has_amount("holybook_bible", 1)) {
                p->add_msg_if_player(_("You have a sudden urge to flip your bible open to Genesis 1:29..."));
            } else { // Big Lebowski
                p->add_msg_if_player(_("That rug really tied the room together..."));
            }
            return;
        case 5:
            p->add_msg_if_player(_("I used to do drugs...  I still do, but I used to, too."));
        default:
            return;
        }
    } else if(howhigh > 100 && one_in(5)) {
        int msg = rng(0, 5);
        switch(msg) {
        case 0: // Bob Marley
            p->add_msg_if_player(_("The herb reveals you to yourself."));
            return;
        case 1: // Freakazoid
            p->add_msg_if_player(_("Okay, like, the scariest thing in the world would be... if like you went to grab something and it wasn't there!"));
            return;
        case 2: // Simpsons
            p->add_msg_if_player(_("They call them fingers, but I never see them fing."));
            if(smarts > 2 && one_in(2)) {
                p->add_msg_if_player(_("... oh, there they go."));
            }
            return;
        case 3: // Bill Hicks
            p->add_msg_if_player(_("You suddenly realize that all matter is merely energy condensed to a slow vibration, and we are all one consciousness experiencing itself subjectively."));
            return;
        case 4: // Steve Martin
            p->add_msg_if_player(_("I usually only smoke in the late evening."));
            if(one_in(4)) {
                p->add_msg_if_player(_("Oh, occasionally the early evening, but usually the late evening, or the mid-evening."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Just the early evening, mid-evening and late evening."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Occasionally, early afternoon, early mid-afternoon, or perhaps the late mid-afternoon."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Oh, sometimes the early-mid-late-early-morning."));
            }
            if(smarts > 2) {
                p->add_msg_if_player(_("...But never at dusk."));
            }
            return;
        case 5:
        default:
            return;
        }
    } else if(howhigh > 50 && one_in(3)) {
        int msg = rng(0, 5);
        switch(msg) {
        case 0: // Cheech and Chong
            p->add_msg_if_player(_("Dave's not here, man."));
            return;
        case 1: // Real Life
            p->add_msg_if_player(_("Man, a cheeseburger sounds SO awesome right now."));
            p->hunger += 4;
            if(p->has_trait("VEGETARIAN")) {
               p->add_msg_if_player(_("Eh... maybe not."));
            } else if(p->has_trait("LACTOSE")) {
                p->add_msg_if_player(_("I guess, maybe, without the cheese... yeah."));
            }
            return;
        case 2: // Dazed and Confused
            p->add_msg_if_player( _("Walkin' down the hall, by myself, smokin' a j with fifty elves."));
            return;
        case 3: // Half Baked
            p->add_msg_if_player(_("That weed was the shiz-nittlebam snip-snap-sack."));
            return;
        case 4:
            weed_msg(p); // re-roll
        case 5:
        default:
            return;
        }
    }
}

void dis_end_msg(player &p, disease &dis)
{
    switch (disease_type_lookup[dis.type]) {
    case DI_SLEEP:
        p.add_msg_if_player(_("You wake up."));
        break;
    default:
        break;
    }
}

void dis_remove_memorial(dis_type type_string) {

  dis_type_enum type = disease_type_lookup[type_string];

  switch(type) {
    case DI_BITE:
      g->u.add_memorial_log(pgettext("memorial_male", "Recovered from a bite wound."),
                            pgettext("memorial_female", "Recovered from a bite wound."));
      break;
    case DI_INFECTED:
      g->u.add_memorial_log(pgettext("memorial_male", "Recovered from an infection... this time."),
                            pgettext("memorial_female", "Recovered from an infection... this time."));
      break;

    default:
        break;
  }

}

void dis_effect(player &p, disease &dis)
{
    dis_type_enum disType = disease_type_lookup[dis.type];
    int grackPower = 500;

    switch(disType) {
        case DI_LYING_DOWN:
            p.moves = 0;
            if (p.can_sleep()) {
                dis.duration = 1;
                p.add_msg_if_player(_("You fall asleep."));
                // Communicate to the player that he is using items on the floor
                std::string item_name = p.is_snuggling();
                if (item_name == "many") {
                    if (one_in(15) ) {
                        add_msg(_("You nestle your pile of clothes for warmth."));
                    } else {
                        add_msg(_("You use your pile of clothes for warmth."));
                    }
                } else if (item_name != "nothing") {
                    if (one_in(15)) {
                        add_msg(_("You snuggle your %s to keep warm."), item_name.c_str());
                    } else {
                        add_msg(_("You use your %s to keep warm."), item_name.c_str());
                    }
                }
                if (p.has_trait("HIBERNATE") && (p.hunger < -60)) {
                    p.add_memorial_log(pgettext("memorial_male", "Entered hibernation."),
                                       pgettext("memorial_female", "Entered hibernation."));
                    // 10 days' worth of round-the-clock Snooze.  Cata seasons default to 14 days.
                    p.fall_asleep(144000);
                }
                // If you're not fatigued enough for 10 days, you won't sleep the whole thing.
                // In practice, the fatigue from filling the tank from (no msg) to Time For Bed
                // will last about 8 days.
                if (p.hunger >= -60) {
                p.fall_asleep(6000); //10 hours, default max sleep time.
                }
            }
            if (dis.duration == 1 && !p.has_disease("sleep")) {
                p.add_msg_if_player(_("You try to sleep, but can't..."));
            }
            break;

        case DI_ALARM_CLOCK:
            {
                if (p.has_disease("sleep")) {
                    if (dis.duration == 1) {
                        if(p.has_bionic("bio_watch")) {
                            // Normal alarm is volume 12, tested against (2/3/6)d15 for
                            // normal/HEAVYSLEEPER/HEAVYSLEEPER2.
                            //
                            // It's much harder to ignore an alarm inside your own skull,
                            // so this uses an effective volume of 20.
                            const int volume = 20;
                            if ((!(p.has_trait("HEAVYSLEEPER") ||
                                   p.has_trait("HEAVYSLEEPER2")) && dice(2, 15) < volume) ||
                                (p.has_trait("HEAVYSLEEPER") && dice(3, 15) < volume) ||
                                (p.has_trait("HEAVYSLEEPER2") && dice(6, 15) < volume)) {
                                p.rem_disease("sleep");
                                add_msg(_("Your internal chronometer wakes you up."));
                            } else {
                                // 10 minute cyber-snooze
                                dis.duration += 100;
                            }
                        } else {
                            if(!g->sound(p.posx, p.posy, 12, _("beep-beep-beep!"))) {
                                // 10 minute automatic snooze
                                dis.duration += 100;
                            } else {
                                add_msg(_("You turn off your alarm-clock."));
                            }
                        }
                    }
                } else if (!p.has_disease("lying_down")) {
                    // Turn the alarm-clock off if you woke up before the alarm
                    dis.duration = 1;
                }
            }

        case DI_SLEEP:
            manage_sleep(p, dis);
            break;

        case DI_DATURA:
        {
                p.mod_per_bonus(-6);
                p.mod_dex_bonus(-3);
                if (p.has_effect("asthma")) {
                    add_msg(m_good, _("You can breathe again!"));
                    p.remove_effect("asthma");
              } if (p.thirst < 20 && one_in(8)) {
                  p.thirst++;
              } if (dis.duration > 1000 && p.focus_pool >= 1 && one_in(4)) {
                  p.focus_pool--;
            } if (dis.duration > 2000 && one_in(8) && p.stim < 20) {
                  p.stim++;
            } if (dis.duration > 3000 && p.focus_pool >= 1 && one_in(2)) {
                  p.focus_pool--;
            } if (dis.duration > 4000 && one_in(64)) {
                  p.mod_pain(rng(-1, -8));
            } if ((!p.has_effect("hallu")) && (dis.duration > 5000 && one_in(4))) {
                  p.add_effect("hallu", 3600);
            } if (dis.duration > 6000 && one_in(128)) {
                  p.mod_pain(rng(-3, -24));
                  if (dis.duration > 8000 && one_in(16)) {
                      add_msg(m_bad, _("You're experiencing loss of basic motor skills and blurred vision.  Your mind recoils in horror, unable to communicate with your spinal column."));
                      add_msg(m_bad, _("You stagger and fall!"));
                      p.add_effect("downed",rng(1,4));
                      if (one_in(8) || will_vomit(p, 10)) {
                            p.vomit();
                       }
                  }
            } if (dis.duration > 7000 && p.focus_pool >= 1) {
                  p.focus_pool--;
            } if (dis.duration > 8000 && one_in(256)) {
                  p.add_effect("visuals", rng(40, 200));
                  p.mod_pain(rng(-8, -40));
            } if (dis.duration > 12000 && one_in(256)) {
                  add_msg(m_bad, _("There's some kind of big machine in the sky."));
                  p.add_effect("visuals", rng(80, 400));
                  if (one_in(32)) {
                        add_msg(m_bad, _("It's some kind of electric snake, coming right at you!"));
                        p.mod_pain(rng(4, 40));
                        p.vomit();
                  }
            } if (dis.duration > 14000 && one_in(128)) {
                  add_msg(m_bad, _("Order us some golf shoes, otherwise we'll never get out of this place alive."));
                  p.add_effect("visuals", rng(400, 2000));
                  if (one_in(8)) {
                  add_msg(m_bad, _("The possibility of physical and mental collapse is now very real."));
                    if (one_in(2) || will_vomit(p, 10)) {
                        add_msg(m_bad, _("No one should be asked to handle this trip."));
                        p.vomit();
                        p.mod_pain(rng(8, 40));
                    }
                  }
            }
        }
            break;

        case DI_BITE:
            handle_bite_wound(p, dis);
            break;

        case DI_INFECTED:
            handle_infected_wound(p, dis);
            break;

        case DI_RECOVER:
            handle_recovery(p, dis);
            break;

        case DI_MA_BUFF:
            if (ma_buffs.find(dis.buff_id) != ma_buffs.end()) {
              ma_buff b = ma_buffs[dis.buff_id];
              if (b.is_valid_player(p)) {
                b.apply_player(p);
              }
              else {
                p.rem_disease(dis.type);
              }
            }
            break;

        case DI_LACKSLEEP:
            p.mod_str_bonus(-1);
            p.mod_dex_bonus(-1);
            p.add_miss_reason(_("You don't have energy to fight."), 1);
            p.mod_int_bonus(-2);
            p.mod_per_bonus(-2);
            break;

        case DI_GRABBED:
            p.blocks_left -= dis.intensity;
            p.dodges_left = 0;
            p.rem_disease(dis.type);
            break;
        default: // Other diseases don't have any effects. Suppress warning.
            break;
    }
}

int disease_speed_boost(disease dis)
{
    dis_type_enum type = disease_type_lookup[dis.type];
    switch (type) {
        case DI_LACKSLEEP:  return -5;
        case DI_GRABBED:    return -25;
        default:            break;
    }
    return 0;
}

std::string dis_name(disease& dis)
{
    // Maximum length of returned string is 26 characters
    dis_type_enum type = disease_type_lookup[dis.type];
    switch (type) {
    case DI_NULL: return "";

    case DI_DATURA: return _("Experiencing Datura");

    case DI_BITE:
    {
        std::string status = "";
        if ((dis.duration > 2401) || (g->u.has_trait("INFIMMUNE"))) {status = _("Bite - ");
        } else { status = _("Painful Bite - ");
        }
        switch (dis.bp) {
            case bp_head:
                status += _("Head");
                break;
            case bp_torso:
                status += _("Torso");
                break;
            case bp_arm_l:
                status += _("Left Arm");
                break;
            case bp_arm_r:
                status += _("Right Arm");
                break;
            case bp_leg_l:
                status += _("Left Leg");
                break;
            case bp_leg_r:
                status += _("Right Leg");
                break;
            default: // Suppress compiler warning [-Wswitch]
                break;
        }
        return status;
    }
    case DI_INFECTED:
    {
        std::string status = "";
        if (dis.duration > 8401) {
            status = _("Infected - ");
        } else if (dis.duration > 3601) {
            status = _("Badly Infected - ");
        } else {
            status = _("Pus Filled - ");
        }
        switch (dis.bp) {
            case bp_head:
                status += _("Head");
                break;
            case bp_torso:
                status += _("Torso");
                break;
            case bp_arm_l:
                status += _("Left Arm");
                break;
            case bp_arm_r:
                status += _("Right Arm");
                break;
            case bp_leg_l:
                status += _("Left Leg");
                break;
            case bp_leg_r:
                status += _("Right Leg");
                break;
            default: // Suppress compiler warning [-Wswitch]
                break;
        }
        return status;
    }
    case DI_RECOVER: return _("Recovering From Infection");

    case DI_MA_BUFF:
        if (ma_buffs.find(dis.buff_id) != ma_buffs.end()) {
            std::stringstream buf;
            if (ma_buffs[dis.buff_id].max_stacks > 1) {
                buf << ma_buffs[dis.buff_id].name << " (" << dis.intensity << ")";
                return buf.str();
            } else {
                buf << ma_buffs[dis.buff_id].name.c_str();
                return buf.str();
            }
        } else
            return "Invalid martial arts buff";

    case DI_LACKSLEEP: return _("Lacking Sleep");
    case DI_GRABBED: return _("Grabbed");
    default: break;
    }
    return "";
}

std::string dis_description(disease& dis)
{
    int strpen, dexpen, intpen, perpen;
    std::stringstream stream;
    dis_type_enum type = disease_type_lookup[dis.type];
    switch (type) {

    case DI_NULL:
        return _("None");
    case DI_DATURA: return _("Buy the ticket, take the ride.  The datura has you now.");

    case DI_BITE: return _("You have a nasty bite wound.");
    case DI_INFECTED: return _("You have an infected wound.");
    case DI_RECOVER: return _("You are recovering from an infection.");

    case DI_MA_BUFF:
        if (ma_buffs.find(dis.buff_id) != ma_buffs.end())
          return ma_buffs[dis.buff_id].description.c_str();
        else
          return "This is probably a bug.";

    case DI_LACKSLEEP: return _("You haven't slept in a while, and it shows. \n\
    You can't move as quickly and your stats just aren't where they should be.");
    case DI_GRABBED: return _("You have been grabbed by an attacker. \n\
    You cannot dodge and blocking is very difficult.");
    default: break;
    }
    return "Who knows?  This is probably a bug. (disease.cpp:dis_description)";
}

void manage_sleep(player& p, disease& dis)
{
    p.moves = 0;
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add fatigue and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    if((int(calendar::turn) % 350 == 0) && p.has_trait("HIBERNATE") && (p.hunger < -60) && !(p.thirst >= 80)) {
        int recovery_chance;
        // Hibernators' metabolism slows down: you heal and recover Fatigue much more slowly.
        // Accelerated recovery capped to 2x over 2 hours...well, it was ;-P
        // After 16 hours of activity, equal to 7.25 hours of rest
        if (dis.intensity < 24) {
            dis.intensity++;
        } else if (dis.intensity < 1) {
            dis.intensity = 1;
        }
        recovery_chance = 24 - dis.intensity + 1;
        if (p.fatigue > 0) {
            p.fatigue -= 1 + one_in(recovery_chance);
        }
        int heal_chance = p.get_healthy() / 4;
        if ((p.has_trait("FLIMSY") && x_in_y(3, 4)) || (p.has_trait("FLIMSY2") && one_in(2)) ||
              (p.has_trait("FLIMSY3") && one_in(4)) ||
              (!(p.has_trait("FLIMSY")) && (!(p.has_trait("FLIMSY2"))) &&
               (!(p.has_trait("FLIMSY3"))))) {
            if (p.has_trait("FASTHEALER")) {
                heal_chance += 100;
            } else if (p.has_trait("FASTHEALER2")) {
                heal_chance += 150;
            } else if (p.has_trait("REGEN")) {
                heal_chance += 200;
            } else if (p.has_trait("SLOWHEALER")) {
                heal_chance += 13;
            } else {
                heal_chance += 25;
            }
            p.healall(heal_chance / 100);
            heal_chance %= 100;
            if (x_in_y(heal_chance, 100)) {
                p.healall(1);
            }
        }

        if (p.fatigue <= 0 && p.fatigue > -20) {
            p.fatigue = -25;
            add_msg(m_good, _("You feel well rested."));
            dis.duration = dice(3, 100);
            p.add_memorial_log(pgettext("memorial_male", "Awoke from hibernation."),
                               pgettext("memorial_female", "Awoke from hibernation."));
        }

    // If you hit Very Thirsty, you kick up into regular Sleep as a safety precaution.
    // See above.  No log note for you. :-/
    } else if(int(calendar::turn) % 50 == 0) {
        int recovery_chance;
        // Accelerated recovery capped to 2x over 2 hours
        // After 16 hours of activity, equal to 7.25 hours of rest
        if (dis.intensity < 24) {
            dis.intensity++;
        } else if (dis.intensity < 1) {
            dis.intensity = 1;
        }
        recovery_chance = 24 - dis.intensity + 1;
        if (p.fatigue > 0) {
            p.fatigue -= 1 + one_in(recovery_chance);
            // You fatigue & recover faster with Sleepy
            // Very Sleepy, you just fatigue faster
            if (p.has_trait("SLEEPY")) {
                p.fatigue -=(1 + one_in(recovery_chance) / 2);
            }
            // Tireless folks recover fatigue really fast
            // as well as gaining it really slowly
            // (Doesn't speed healing any, though...)
            if (p.has_trait("WAKEFUL3")) {
                p.fatigue -=(2 + one_in(recovery_chance) / 2);
            }
        }
        int heal_chance = p.get_healthy() / 4;
        if ((p.has_trait("FLIMSY") && x_in_y(3, 4)) || (p.has_trait("FLIMSY2") && one_in(2)) ||
              (p.has_trait("FLIMSY3") && one_in(4)) ||
              (!(p.has_trait("FLIMSY")) && (!(p.has_trait("FLIMSY2"))) &&
               (!(p.has_trait("FLIMSY3"))))) {
            if (p.has_trait("FASTHEALER")) {
                heal_chance += 100;
            } else if (p.has_trait("FASTHEALER2")) {
                heal_chance += 150;
            } else if (p.has_trait("REGEN")) {
                heal_chance += 200;
            } else if (p.has_trait("SLOWHEALER")) {
                heal_chance += 13;
            } else {
                heal_chance += 25;
            }
            p.healall(heal_chance / 100);
            heal_chance %= 100;
            if (x_in_y(heal_chance, 100)) {
                p.healall(1);
            }
        }

        if (p.fatigue <= 0 && p.fatigue > -20) {
            p.fatigue = -25;
            add_msg(m_good, _("You feel well rested."));
            dis.duration = dice(3, 100);
        }
    }

    if (int(calendar::turn) % 100 == 0 && !p.has_bionic("bio_recycler") && !(p.hunger < -60)) {
        // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
        p.hunger--;
        p.thirst--;
    }

    // Hunger and thirst advance *much* more slowly whilst we hibernate.
    // (int (calendar::turn) % 50 would be zero burn.)
    // Very Thirsty catch deliberately NOT applied here, to fend off Dehydration debuffs
    // until the char wakes.  This was time-trial'd quite thoroughly,so kindly don't "rebalance"
    // without a good explanation and taking a night to make sure it works
    // with the extended sleep duration, OK?
    if (int(calendar::turn) % 70 == 0 && !p.has_bionic("bio_recycler") && (p.hunger < -60)) {
        p.hunger--;
        p.thirst--;
    }

    if (int(calendar::turn) % 100 == 0 && p.has_trait("CHLOROMORPH") &&
    g->is_in_sunlight(g->u.posx, g->u.posy) ) {
        // Hunger and thirst fall before your Chloromorphic physiology!
        if (p.hunger >= -30) {
            p.hunger -= 5;
        }
        if (p.thirst >= -30) {
            p.thirst -= 5;
        }
    }

    // Check mutation category strengths to see if we're mutated enough to get a dream
    std::string highcat = p.get_highest_category();
    int highest = p.mutation_category_level[highcat];

    // Determine the strength of effects or dreams based upon category strength
    int strength = 0; // Category too weak for any effect or dream
    if (g->u.crossed_threshold()) {
        strength = 4; // Post-human.
    } else if (highest >= 20 && highest < 35) {
        strength = 1; // Low strength
    } else if (highest >= 35 && highest < 50) {
        strength = 2; // Medium strength
    } else if (highest >= 50) {
        strength = 3; // High strength
    }

    // Get a dream if category strength is high enough.
    if (strength != 0) {
        //Once every 6 / 3 / 2 hours, with a bit of randomness
        if ((int(calendar::turn) % (3600 / strength) == 0) && one_in(3)) {
            // Select a dream
            std::string dream = p.get_category_dream(highcat, strength);
            if( !dream.empty() ) {
                add_msg( "%s", dream.c_str() );
            }
            // Mycus folks upgrade in their sleep.
            if (p.has_trait("THRESH_MYCUS")) {
                if (one_in(8)) {
                    p.mutate_category("MUTCAT_MYCUS");
                    p.hunger += 10;
                    p.fatigue += 5;
                    p.thirst += 10;
                }
            }
        }
    }

    int tirednessVal = rng(5, 200) + rng(0,abs(p.fatigue * 2 * 5));
    if (p.has_trait("HEAVYSLEEPER2") && !p.has_trait("HIBERNATE")) {
        // So you can too sleep through noon
        if ((tirednessVal * 1.25) < g->light_level() && (p.fatigue < 10 || one_in(p.fatigue / 2))) {
        add_msg(_("The light wakes you up."));
        dis.duration = 1;
        }
        return;}
     // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
    if (p.has_trait("HIBERNATE")) {
        if ((tirednessVal * 5) < g->light_level() && (p.fatigue < 10 || one_in(p.fatigue / 2))) {
        add_msg(_("The light wakes you up."));
        dis.duration = 1;
        }
        return;}
    if (tirednessVal < g->light_level() && (p.fatigue < 10 || one_in(p.fatigue / 2))) {
        add_msg(_("The light wakes you up."));
        dis.duration = 1;
        return;
    }

    // Cold or heat may wake you up.
    // Player will sleep through cold or heat if fatigued enough
    for (int i = 0 ; i < num_bp ; i++) {
        if (p.temp_cur[i] < BODYTEMP_VERY_COLD - p.fatigue/2) {
            if (one_in(5000)) {
                add_msg(_("You toss and turn trying to keep warm."));
            }
            if (p.temp_cur[i] < BODYTEMP_FREEZING - p.fatigue/2 ||
                                (one_in(p.temp_cur[i] + 5000))) {
                add_msg(m_bad, _("The cold wakes you up."));
                dis.duration = 1;
                return;
            }
        } else if (p.temp_cur[i] > BODYTEMP_VERY_HOT + p.fatigue/2) {
            if (one_in(5000)) {
                add_msg(_("You toss and turn in the heat."));
            }
            if (p.temp_cur[i] > BODYTEMP_SCORCHING + p.fatigue/2 ||
                                (one_in(15000 - p.temp_cur[i]))) {
                add_msg(m_bad, _("The heat wakes you up."));
                dis.duration = 1;
                return;
            }
        }
    }
}

static void handle_bite_wound(player& p, disease& dis)
{
    // Recovery chance
    if(int(calendar::turn) % 10 == 1) {
        int recover_factor = 100;
        if (p.has_disease("recover")) {
            recover_factor -= std::min(p.disease_duration("recover") / 720, 100);
        }
        // Infection Resist is exactly that: doesn't make the Deep Bites go away
        // but it does make it much more likely they won't progress
        if (p.has_trait("INFRESIST")) { recover_factor += 1000; }
        recover_factor += p.get_healthy() / 10; // Health still helps if factor is zero
        recover_factor = std::max(recover_factor, 0); // but can't hurt

        if ((x_in_y(recover_factor, 108000)) || (p.has_trait("INFIMMUNE"))) {
            //~ %s is bodypart name.
            p.add_msg_if_player(m_good, _("Your %s wound begins to feel better."),
                                 body_part_name(dis.bp).c_str());
             //No recovery time threshold
            if (((3601 - dis.duration) > 2400) && (!(p.has_trait("INFIMMUNE")))) {
                p.add_disease("recover", 2 * (3601 - dis.duration) - 4800);
            }
            p.rem_disease("bite", dis.bp);
            return;
        }
    }

    // 3600 (6-hour) lifespan + 1 "tick" for conversion
    if (dis.duration > 2401) {
        // No real symptoms for 2 hours
        if ((one_in(300)) && (!(p.has_trait("NOPAIN")))) {
            //~ %s is bodypart name.
            p.add_msg_if_player(m_bad, _("Your %s wound really hurts."),
                                 body_part_name(dis.bp).c_str());
        }
    } else if (dis.duration > 1) {
        // Then some pain for 4 hours
        if ((one_in(100)) && (!(p.has_trait("NOPAIN")))) {
            if (p.has_disease("sleep")) {
                p.wake_up();
            }
            //~ %s is bodypart name.
            p.add_msg_if_player(m_bad, _("Your %s wound feels swollen and painful."),
                                 body_part_name(dis.bp).c_str());
            if (p.pain < 10) {
                p.mod_pain(1);
            }
        }
        p.mod_dex_bonus(-1);
        p.add_miss_reason(_("Your wound distracts you."), 1);
    } else {
        // Infection starts
         // 1 day of timer + 1 tick
        p.add_disease("infected", 14401, false, 1, 1, 0, 0, dis.bp, true);
        p.rem_disease("bite", dis.bp);
    }
}

static void handle_infected_wound(player& p, disease& dis)
{
    // Recovery chance
    if(int(calendar::turn) % 10 == 1) {
        if(x_in_y(100 + p.get_healthy() / 10, 864000)) {
            //~ %s is bodypart name.
            p.add_msg_if_player(m_good, _("Your %s wound begins to feel better."),
                                 body_part_name(dis.bp).c_str());
            if (dis.duration > 8401) {
                p.add_disease("recover", 3 * (14401 - dis.duration + 3600) - 4800);
            } else {
                p.add_disease("recover", 4 * (14401 - dis.duration + 3600) - 4800);
            }
            p.rem_disease("infected", dis.bp);
            return;
        }
    }

    if (dis.duration > 8401) {
        // 10 hours bad pain
        if ((one_in(100)) && (!(p.has_trait("NOPAIN")))) {
            if (p.has_disease("sleep")) {
                p.wake_up();
            }
            //~ %s is bodypart name.
            p.add_msg_if_player(m_bad, _("Your %s wound is incredibly painful."),
                                 body_part_name(dis.bp).c_str());
            if(p.pain < 30) {
                p.mod_pain(1);
            }
        }
        p.mod_str_bonus(-1);
        p.mod_dex_bonus(-1);
        p.add_miss_reason(_("Your wound distracts you."), 1);
    } else if (dis.duration > 3601) {
        // 8 hours of vomiting + pain
        if (one_in(100)) {
            if (p.has_disease("sleep")) {
                p.wake_up();
            }
            //~ %s is bodypart name.
            p.add_msg_if_player(m_bad, _("You feel feverish and nauseous, your %s wound has begun to turn green."),
                  body_part_name(dis.bp).c_str());
            p.vomit();
            if(p.pain < 50) {
                p.mod_pain(1);
            }
        }
        p.mod_str_bonus(-2);
        p.mod_dex_bonus(-2);
        p.add_miss_reason(_("Your wound distracts you."), 2);
    } else if (dis.duration > 1) {
        // 6 hours extreme symptoms
        if (one_in(100)) {
            if (p.has_disease("sleep")) {
                p.wake_up();
                p.add_msg_if_player(m_warning, _("You feel terribly weak, standing up is nearly impossible."));
            } else {
                p.add_msg_if_player(m_warning, _("You can barely remain standing."));
            }
            p.vomit();
            if(p.pain < 100) {
                p.mod_pain(1);
            }
        }
        p.mod_str_bonus(-3);
        p.mod_dex_bonus(-3);
        p.add_miss_reason(_("You can barely keep fighting."), 3);
        if (!p.has_disease("sleep") && one_in(100)) {
            add_msg(m_bad, _("You pass out."));
            p.fall_asleep(60);
        }
    } else {
        // Death. 24 hours after infection. Total time, 30 hours including bite.
        if (p.has_disease("sleep")) {
            p.rem_disease("sleep");
        }
        add_msg(m_bad, _("You succumb to the infection."));
        g->u.add_memorial_log(pgettext("memorial_male", "Succumbed to the infection."),
                              pgettext("memorial_female", "Succumbed to the infection."));
        p.hurtall(500);
    }
}

static void handle_recovery(player& p, disease& dis)
{
    if (dis.duration > 52800) {
        if (one_in(100)) {
            if (p.has_disease("sleep")) {
                p.wake_up();
                p.add_msg_if_player(m_warning, _("You feel terribly weak, standing up is nearly impossible."));
            } else {
                p.add_msg_if_player(m_warning, _("You can barely remain standing."));
            }
            p.vomit();
            if(p.pain < 80) {
                p.mod_pain(1);
            }
        }
        p.mod_str_bonus(-3);
        p.mod_dex_bonus(-3);
        p.add_miss_reason(_("You can barely keep fighting."), 3);
        if (!p.has_disease("sleep") && one_in(100)) {
            add_msg(m_bad, _("You pass out."));
            p.fall_asleep(60);
        }
    } else if (dis.duration > 33600) {
        if (one_in(100)) {
            if (p.has_disease("sleep")) {
                p.wake_up();
            }
            p.add_msg_if_player(m_bad, _("You feel feverish and nauseous."));
            p.vomit();
            if(p.pain < 40) {
                p.mod_pain(1);
            }
        }
        p.mod_str_bonus(-2);
        p.mod_dex_bonus(-2);
        p.add_miss_reason(_("Your wound distracts you."), 2);
    } else if (dis.duration > 9600) {
        if ((one_in(100)) && (!(p.has_trait("NOPAIN")))) {
            if (p.has_disease("sleep")) {
                p.wake_up();
            }
            p.add_msg_if_player(m_bad, _("Your healing wound is incredibly painful."));
            if(p.pain < 24) {
                p.mod_pain(1);
            }
        }
        p.mod_str_bonus(-1);
        p.mod_dex_bonus(-1);
        p.add_miss_reason(_("Your wound distracts you."), 1);
    } else {
        if ((one_in(100)) && (!(p.has_trait("NOPAIN")))) {
            if (p.has_disease("sleep")) {
                p.wake_up();
            }
            p.add_msg_if_player(m_bad, _("Your healing wound feels swollen and painful."));
            if(p.pain < 8) {
                p.mod_pain(1);
            }
        }
        p.mod_dex_bonus(-1);
        p.add_miss_reason(_("Your wound distracts you."), 1);
    }
}

bool will_vomit(player& p, int chance)
{
    bool drunk = p.has_effect("drunk");
    bool antiEmetics = p.has_effect("weed_high");
    bool hasNausea = p.has_trait("NAUSEA") && one_in(chance*2);
    bool stomachUpset = p.has_trait("WEAKSTOMACH") && one_in(chance*3);
    bool suppressed = (p.has_trait("STRONGSTOMACH") && one_in(2)) ||
        (antiEmetics && !drunk && !one_in(chance));
    return ((stomachUpset || hasNausea) && !suppressed);
}
