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
// Monsters
 DI_LYING_DOWN, DI_SLEEP, DI_ALARM_CLOCK,
// Martial arts-related buffs
 DI_MA_BUFF
};

std::map<std::string, dis_type_enum> disease_type_lookup;

// Todo: Move helper functions into a DiseaseHandler Class.
// Should standardize parameters so we can make function pointers.
static void manage_sleep(player& p, disease& dis);

void game::init_diseases() {
    // Initialize the disease lookup table.

    disease_type_lookup["null"] = DI_NULL;
    disease_type_lookup["lying_down"] = DI_LYING_DOWN;
    disease_type_lookup["sleep"] = DI_SLEEP;
    disease_type_lookup["alarm_clock"] = DI_ALARM_CLOCK;
    disease_type_lookup["ma_buff"] = DI_MA_BUFF;
}

bool dis_msg(dis_type type_string) {
    dis_type_enum type = disease_type_lookup[type_string];
    switch (type) {
    case DI_LYING_DOWN:
        add_msg(_("You lie down to go to sleep..."));
        break;
    default:
        return false;
        break;
    }

    return true;
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

    default:
        break;
  }

}

void dis_effect(player &p, disease &dis)
{
    dis_type_enum disType = disease_type_lookup[dis.type];

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
        default: // Other diseases don't have any effects. Suppress warning.
            break;
    }
}

int disease_speed_boost(disease dis)
{
    dis_type_enum type = disease_type_lookup[dis.type];
    switch (type) {
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

    default: break;
    }
    return "";
}

std::string dis_description(disease& dis)
{
    std::stringstream stream;
    dis_type_enum type = disease_type_lookup[dis.type];
    switch (type) {

    case DI_NULL:
        return _("None");

    case DI_MA_BUFF:
        if (ma_buffs.find(dis.buff_id) != ma_buffs.end())
          return ma_buffs[dis.buff_id].description.c_str();
        else
          return "This is probably a bug.";
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
