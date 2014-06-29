#include "character.h"
#include "item.h"
#include "output.h"
#include "game.h"
#include "messages.h"
#include <algorithm>
#include <numeric>
#include <cmath>

Character::Character() {
    Creature::set_speed_base(100);
};

Character &Character::operator= (const Character &rhs)
{
    Creature::operator=(rhs);
    return (*this);
}

field_id Character::bloodType() {
    return fd_blood;
}
field_id Character::gibType() {
    return fd_gibs_flesh;
}
void Character::cough(bool harmful)
{
    if (is_player()) {
        add_msg(_("You cough heavily."));
        g->sound(xpos(), ypos(), 4, "");
    } else {
        g->sound(xpos(), ypos(), 4, _("a hacking cough."));
    }
    mod_moves(-80);
    if (harmful && !one_in(4)) {
        hurt(bp_torso, -1, 1);
    }
    if (has_effect("sleep") && ((harmful && one_in(3)) || one_in(10)) ) {
        if(is_player()) {
            //wake_up(_("You wake up coughing."));
        }
    }
}
bool Character::will_vomit(int chance)
{
    bool drunk = g->u.has_disease("drunk");
    bool antiEmetics = g->u.has_disease("weed_high");
    bool hasNausea = g->u.has_trait("NAUSEA") && one_in(chance*2);
    bool stomachUpset = g->u.has_trait("WEAKSTOMACH") && one_in(chance*3);
    bool suppressed = (g->u.has_trait("STRONGSTOMACH") && one_in(2)) ||
        (antiEmetics && !drunk && !one_in(chance));
    return ((stomachUpset || hasNausea) && !suppressed);
}

bool Character::hibernating()
{
    if (g->u.has_trait("HIBERNATE") && (g->u.hunger < -60) && !(g->u.thirst >= 80)) {
        return true;
    }
    return false;
}
void Character::manage_sleep()
{
    Creature::manage_sleep();
    // TODO: Still a lot to generalize away from the player here.
    // Hibernating only kicks in whilst Engorged; separate tracking for hunger/thirst here
    // as a safety catch.  One test subject managed to get two Colds during hibernation;
    // since those add fatigue and dry out the character, the subject went for the full 10 days plus
    // a little, and came out of it well into Parched.  Hibernating shouldn't endanger your
    // life like that--but since there's much less fluid reserve than food reserve,
    // simply using the same numbers won't work.
    effect sleep = get_effect("sleep");
    if (sleep.get_id() == "") {
        debugmsg("Tried to manage sleep for non-sleeping Character.");
        return;
    }
    bool hibernate = hibernating();
    if( ((int(calendar::turn) % 350 == 0) && hibernate) || ((int(calendar::turn) % 50 == 0) && !hibernate) ){
        int recovery_chance;
        // Accelerated recovery capped to 2x over 2 hours...well, it was ;-P
        // After 16 hours of activity, equal to 7.25 hours of rest
        if (sleep.get_intensity() < 24) {
            sleep.mod_intensity(1);
        } else if (sleep.get_intensity() < 1) {
            sleep.set_intensity(1);
        }
        recovery_chance = 24 - sleep.get_intensity() + 1;
        if (g->u.fatigue > 0) {
            g->u.fatigue -= 1 + one_in(recovery_chance);
            // You fatigue & recover faster with Sleepy
            // Very Sleepy, you just fatigue faster
            if (g->u.has_trait("SLEEPY")) {
                g->u.fatigue -=(1 + one_in(recovery_chance) / 2);
            }
            // Tireless folks recover fatigue really fast
            // as well as gaining it really slowly
            // (Doesn't speed healing any, though...)
            if (g->u.has_trait("WAKEFUL3")) {
                g->u.fatigue -=(2 + one_in(recovery_chance) / 2);
            }
        }
        if ((!(g->u.has_trait("FLIMSY")) && (!(g->u.has_trait("FLIMSY2"))) &&
               (!(g->u.has_trait("FLIMSY3")))) || (g->u.has_trait("FLIMSY") && x_in_y(3 , 4)) ||
               (g->u.has_trait("FLIMSY2") && one_in(2)) ||
               (g->u.has_trait("FLIMSY3") && one_in(4))) {
            int heal_chance = get_healthy() / 25;
            if (g->u.has_trait("FASTHEALER")) {
                heal_chance += 100;
            } else if (g->u.has_trait("FASTHEALER2")) {
                heal_chance += 150;
            } else if (g->u.has_trait("REGEN")) {
                heal_chance += 200;
            } else if (g->u.has_trait("SLOWHEALER")) {
                heal_chance += 13;
            } else {
                heal_chance += 25;
            }
            int tmp_heal = 0;
            while (heal_chance >= 100) {
                tmp_heal++;
                heal_chance -= 100;
            }
            g->u.healall(tmp_heal + x_in_y(heal_chance, 100));
        }

        if (g->u.fatigue <= 0 && g->u.fatigue > -20) {
            g->u.fatigue = -25;
            add_msg_if_player(_("You feel well rested."));
            sleep.set_duration(dice(3, 100));
            if (hibernate && is_player()) {
                g->u.add_memorial_log(pgettext("memorial_male", "Awoke from hibernation."),
                                    pgettext("memorial_female", "Awoke from hibernation."));
            }
        }
    }

    if (int(calendar::turn) % 100 == 0 && !g->u.has_bionic("bio_recycler") && !(g->u.hunger < -60)) {
        // Hunger and thirst advance more slowly while we sleep. This is the standard rate.
        g->u.hunger--;
        g->u.thirst--;
    }

    // Hunger and thirst advance *much* more slowly whilst we hibernate.
    // (int (calendar::turn) % 50 would be zero burn.)
    // Very Thirsty catch deliberately NOT applied here, to fend off Dehydration debuffs
    // until the char wakes.  This was time-trial'd quite thoroughly,so kindly don't "rebalance"
    // without a good explanation and taking a night to make sure it works
    // with the extended sleep duration, OK?
    if (int(calendar::turn) % 70 == 0 && g->u.has_trait("HIBERNATE") && !g->u.has_bionic("bio_recycler") &&
        (g->u.hunger < -60)) {
        g->u.hunger--;
        g->u.thirst--;
    }

    // Check mutation category strengths to see if we're mutated enough to get a dream
    std::string highcat = g->u.get_highest_category();
    int highest = g->u.mutation_category_level[highcat];

    // Determine the strength of effects or dreams based upon category strength
    int strength = 0; // Category too weak for any effect or dream
    if (highest >= 20 && highest < 35) {
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
            std::string dream = g->u.get_category_dream(highcat, strength);
            add_msg_if_player("%s",dream.c_str());
        }
    }

    int tirednessVal = rng(5, 200) + rng(0,abs(g->u.fatigue * 2 * 5));
    if (g->u.has_trait("HEAVYSLEEPER2") && !g->u.has_trait("HIBERNATE")) {
        // So you can too sleep through noon
        if ((tirednessVal * 1.25) < g->light_level() && (g->u.fatigue < 10 || one_in(g->u.fatigue / 2))) {
            add_msg_if_player(_("The light wakes you up."));
            sleep.set_duration(1);
        }
        return;}
     // Ursine hibernators would likely do so indoors.  Plants, though, might be in the sun.
    if (g->u.has_trait("HIBERNATE")) {
        if ((tirednessVal * 5) < g->light_level() && (g->u.fatigue < 10 || one_in(g->u.fatigue / 2))) {
            add_msg_if_player(_("The light wakes you up."));
            sleep.set_duration(1);
        }
        return;}
    if (tirednessVal < g->light_level() && (g->u.fatigue < 10 || one_in(g->u.fatigue / 2))) {
        add_msg_if_player(_("The light wakes you up."));
        sleep.set_duration(1);
        return;
    }

    // Cold or heat may wake you up.
    // Player will sleep through cold or heat if fatigued enough
    for (int i = 0 ; i < num_bp ; i++) {
        if (g->u.temp_cur[i] < BODYTEMP_VERY_COLD - g->u.fatigue/2) {
            if (one_in(5000)) {
                add_msg_if_player(_("You toss and turn trying to keep warm."));
            }
            if (g->u.temp_cur[i] < BODYTEMP_FREEZING - g->u.fatigue/2 ||
                                (one_in(g->u.temp_cur[i] + 5000))) {
                add_msg_if_player(_("The cold wakes you up."));
                sleep.set_duration(1);
                return;
            }
        } else if (g->u.temp_cur[i] > BODYTEMP_VERY_HOT + g->u.fatigue/2) {
            if (one_in(5000)) {
                add_msg_if_player(_("You toss and turn in the heat."));
            }
            if (g->u.temp_cur[i] > BODYTEMP_SCORCHING + g->u.fatigue/2 ||
                                (one_in(15000 - g->u.temp_cur[i]))) {
                add_msg_if_player(_("The heat wakes you up."));
                sleep.set_duration(1);
                return;
            }
        }
    }
}

void Character::add_wound(body_part target, int nside, int sev,
                            std::vector<wefftype_id> wound_effs)
{
    wounds.push_back(wound(target, nside, sev, wound_effs));
}

void Character::remove_wounds(body_part target, int nside, int sev,
                            std::vector<wefftype_id> wound_effs)
{
    for (std::vector<wound>::iterator it = wounds.begin(); it != wounds.end(); ++it) {
        if (target == it->get_bp() && nside == it->get_side()) {
            if (sev == it->get_severity() || sev == 0) {
                bool eff_match = true;
                for (std::vector<wefftype_id>::iterator it2 = wound_effs.begin();
                      it2 != wound_effs.end(); ++it2) {
                    if (!it->has_wound_effect(*it2)) {
                        eff_match = false;
                        break;
                    }
                }
                if (eff_match == true) {
                    it = wounds.erase(it);
                }
            }
        }
    }
}
