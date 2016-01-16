#include "addiction.h"
#include "debug.h"
#include "pldata.h"
#include "player.h"
#include "morale.h"
#include "rng.h"
#include "translations.h"

void addict_effect(player &u, addiction &add,
                   std::function<void (char const*)> const &cancel_activity)
{
    int const in = add.intensity;

    switch (add.type) {
    case ADD_CIG:
        if (in > 20 || one_in((500 - 20 * in))) {
            u.add_msg_if_player(rng(0, 6) < in ? _("You need some nicotine.") :
                    _("You could use some nicotine."));
            u.add_morale(MORALE_CRAVING_NICOTINE, -15, -50);
            if (one_in(800 - 50 * in)) {
                u.fatigue++;
            }
            if (u.stim > -50 && one_in(400 - 20 * in)) {
                u.stim--;
            }
        }
        break;

    case ADD_CAFFEINE:
        u.moves -= 2;
        if (in > 20 || one_in((500 - 20 * in))) {
            u.add_msg_if_player(m_warning, _("You want some caffeine."));
            u.add_morale(MORALE_CRAVING_CAFFEINE, -5, -30);
            if (u.stim > -150 && rng(0, 10) < in) {
                u.stim--;
            }
            if (rng(8, 400) < in) {
                u.add_msg_if_player(m_bad, _("Your hands start shaking... you need it bad!"));
                u.add_effect("shakes", 20);
            }
        }
        break;

    case ADD_ALCOHOL:
        u.mod_per_bonus(-1);
        u.mod_int_bonus(-1);
        if (rng(40, 1200) <= in * 10) {
            u.mod_healthy_mod(-1, -in * 10);
        }
        if (one_in(20) && rng(0, 20) < in) {
            u.add_msg_if_player(m_warning, _("You could use a drink."));
            u.add_morale(MORALE_CRAVING_ALCOHOL, -35, -120);
        } else if (rng(8, 300) < in) {
            u.add_msg_if_player(m_bad, _("Your hands start shaking... you need a drink bad!"));
            u.add_morale(MORALE_CRAVING_ALCOHOL, -35, -120);
            u.add_effect("shakes", 50);
        } else if (!u.has_effect("hallu") && rng(10, 1600) < in) {
            u.add_effect("hallu", 3600);
        }
        break;

    case ADD_SLEEP:
        // No effects here--just in player::can_sleep()
        // EXCEPT!  Prolong this addiction longer than usual.
        if (one_in(2) && add.sated < 0) {
            add.sated++;
        }
        break;

    case ADD_PKILLER:
        if ((in >= 25 || int(calendar::turn) % (100 - in * 4) == 0) && u.pkill > 0) {
            u.pkill--;    // Tolerance increases!
        }
        if (u.pkill >= 35) { // No further effects if we're doped up.
            add.sated = 0;
        } else {
            u.mod_str_bonus(-(1 + int(in / 7)));
            u.mod_per_bonus(-1);
            u.mod_dex_bonus(-1);
            if (u.pain < in * 3) {
                u.mod_pain(1);
            }
            if (in >= 40 || one_in(1200 - 30 * in)) {
                u.mod_healthy_mod(-1, -in * 30);
            }
            if (one_in(20) && dice(2, 20) < in) {
                u.add_msg_if_player(m_bad, _("Your hands start shaking... you need some painkillers."));
                u.add_morale(MORALE_CRAVING_OPIATE, -40, -200);
                u.add_effect("shakes", 20 + in * 5);
            } else if (one_in(20) && dice(2, 30) < in) {
                u.add_msg_if_player(m_bad, _("You feel anxious.  You need your painkillers!"));
                u.add_morale(MORALE_CRAVING_OPIATE, -30, -200);
            } else if (one_in(50) && dice(3, 50) < in) {
                u.add_msg_if_player(m_bad, _("You throw up heavily!"));
                cancel_activity(_("Throwing up."));
                u.vomit();
            }
        }
        break;

    case ADD_SPEED: {
        // Minimal speed of PC is 0.25 * base speed, that is
        // usually 25 moves, this ensures that even at minimal speed
        // the PC gets 5 moves per turn.
        int move_pen = std::min(in * 5, 20);
        u.moves -= move_pen;
        u.mod_int_bonus(-1);
        u.mod_str_bonus(-1);
        if (u.stim > -100 && (in >= 20 || int(calendar::turn) % (100 - in * 5) == 0)) {
            u.stim--;
        }
        if (rng(0, 150) <= in) {
            u.mod_healthy_mod(-1, -in);
        }
        if (dice(2, 100) < in) {
            u.add_msg_if_player(m_warning, _("You feel depressed.  Speed would help."));
            u.add_morale(MORALE_CRAVING_SPEED, -25, -200);
        } else if (one_in(10) && dice(2, 80) < in) {
            u.add_msg_if_player(m_bad, _("Your hands start shaking... you need a pick-me-up."));
            u.add_morale(MORALE_CRAVING_SPEED, -25, -200);
            u.add_effect("shakes", in * 20);
        } else if (one_in(50) && dice(2, 100) < in) {
            u.add_msg_if_player(m_bad, _("You stop suddenly, feeling bewildered."));
            cancel_activity(nullptr);
            u.moves -= 300;
        } else if (!u.has_effect("hallu") && one_in(20) && 8 + dice(2, 80) < in) {
            u.add_effect("hallu", 3600);
        }
    }
    break;

    case ADD_COKE:
        u.mod_int_bonus(-1);
        u.mod_per_bonus(-1);
        if (in >= 30 || one_in((900 - 30 * in))) {
            u.add_msg_if_player(m_warning, _("You feel like you need a bump."));
            u.add_morale(MORALE_CRAVING_COCAINE, -20, -250);
        }
        if (dice(2, 80) <= in) {
            u.add_msg_if_player(m_warning, _("You feel like you need a bump."));
            u.add_morale(MORALE_CRAVING_COCAINE, -20, -250);
            if (u.stim > -150) {
                u.stim -= 3;
            }
        }
        break;

    case ADD_CRACK:
        u.mod_int_bonus(-1);
        u.mod_per_bonus(-1);
        if (in >= 30 || one_in((900 - 30 * in))) {
            u.add_msg_if_player(m_bad, _("You're shivering, you need some crack."));
            u.add_morale(MORALE_CRAVING_CRACK, -80, -250);
        }
        if (dice(2, 80) <= in) {
            u.add_msg_if_player(m_bad, _("You're shivering, you need some crack."));
            u.add_morale(MORALE_CRAVING_CRACK, -80, -250);
            if (u.stim > -150) {
                u.stim -= 3;
            }
        }
        break;

    case ADD_MUTAGEN:
        if (u.has_trait("MUT_JUNKIE")) {
            if (one_in(600 - 50 * in)) {
                u.add_msg_if_player(m_warning, rng(0, 6) < in ? _("You so miss the exquisite rainbow of post-humanity.") :
                        _("Your body is SOO booorrrring.  Just a little sip to liven things up?"));
                u.add_morale(MORALE_CRAVING_MUTAGEN, -20, -200);
            }
            if (u.focus_pool > 40 && one_in(800 - 20 * in)) {
                u.focus_pool -= (in);
                u.add_msg_if_player(m_warning,
                        _("You daydream what it'd be like if you were *different*.  Different is good."));
            }
        } else if (in > 5 || one_in((500 - 15 * in))) {
            u.add_msg_if_player(m_warning, rng(0, 6) < in ? _("You haven't had any mutagen lately.") :
                    _("You could use some new parts..."));
            u.add_morale(MORALE_CRAVING_MUTAGEN, -5, -50);
        }
        break;

    case ADD_DIAZEPAM:
        u.mod_per_bonus(-1);
        u.mod_int_bonus(-1);
        if (rng(40, 1200) <= in * 10) {
            u.mod_healthy_mod(-1, -in * 10);
        }
        if (one_in(20) && rng(0, 20) < in) {
            u.add_msg_if_player(m_warning, _("You could use some diazepam."));
            u.add_morale(MORALE_CRAVING_DIAZEPAM, -35, -120);
        } else if (rng(8, 200) < in) {
            u.add_msg_if_player(m_bad, _("You're shaking... you need some diazepam!"));
            u.add_morale(MORALE_CRAVING_DIAZEPAM, -35, -120);
            u.add_effect("shakes", 50);
        } else if (!u.has_effect("hallu") && rng(10, 3200) < in) {
            u.add_effect("hallu", 3600);
        } else if (one_in(50) && dice(3, 50) < in) {
            u.add_msg_if_player(m_bad, _("You throw up heavily!"));
            cancel_activity(_("Throwing up."));
            u.vomit();
        }
        break;
    case ADD_MARLOSS_R:
        if (one_in(800 - 20 * in)) {
            u.add_morale(MORALE_CRAVING_MARLOSS, -5, -25);
            u.add_msg_if_player(m_info, _("You daydream about luscious pink berries as big as your fist."));
            if (u.focus_pool > 40) {
                u.focus_pool -= (in);
            }
        }
        break;
    case ADD_MARLOSS_B:
        if (one_in(800 - 20 * in)) {
            u.add_morale(MORALE_CRAVING_MARLOSS, -5, -25);
            u.add_msg_if_player(m_info, _("You daydream about nutty cyan seeds as big as your hand."));
            if (u.focus_pool > 40) {
                u.focus_pool -= (in);
            }
        }
        break;
    case ADD_MARLOSS_Y:
        if (one_in(800 - 20 * in)) {
            u.add_morale(MORALE_CRAVING_MARLOSS, -5, -25);
            u.add_msg_if_player(m_info, _("You daydream about succulent, pale golden gel, sweet but light."));
            if (u.focus_pool > 40) {
                u.focus_pool -= (in);
            }
        }
        break;

    //for any other unhandled cases
    default:
        break;

    }
}

/**
 * Returns the name of an addiction. It should be able to finish the sentence
 * "Became addicted to ______".
 */
std::string addiction_type_name(add_type const cur)
{
    switch(cur) {
    case ADD_CIG:
        return _("nicotine");
    case ADD_CAFFEINE:
        return _("caffeine");
    case ADD_ALCOHOL:
        return _("alcohol");
    case ADD_SLEEP:
        return _("sleeping pills");
    case ADD_PKILLER:
        return _("opiates");
    case ADD_SPEED:
        return _("amphetamine");
    case ADD_COKE:
        return _("cocaine");
    case ADD_CRACK:
        return _("crack cocaine");
    case ADD_MUTAGEN:
        return _("mutation");
    case ADD_DIAZEPAM:
        return _("diazepam");
    case ADD_MARLOSS_R:
        return _("Marloss berries");
    case ADD_MARLOSS_B:
        return _("Marloss seeds");
    case ADD_MARLOSS_Y:
        return _("Marloss gel");
    default:
        return "bugs in addiction.cpp";
    }
}

std::string addiction_name(addiction const &cur)
{
    switch (cur.type) {
    case ADD_CIG:
        return _("Nicotine Withdrawal");
    case ADD_CAFFEINE:
        return _("Caffeine Withdrawal");
    case ADD_ALCOHOL:
        return _("Alcohol Withdrawal");
    case ADD_SLEEP:
        return _("Sleeping Pill Dependence");
    case ADD_PKILLER:
        return _("Opiate Withdrawal");
    case ADD_SPEED:
        return _("Amphetamine Withdrawal");
    case ADD_COKE:
        return _("Cocaine Withdrawal");
    case ADD_CRACK:
        return _("Crack Cocaine Withdrawal");
    case ADD_MUTAGEN:
        return _("Mutation Withdrawal");
    case ADD_DIAZEPAM:
        return _("Diazepam Withdrawal");
    case ADD_MARLOSS_R:
        return _("Marloss Longing");
    case ADD_MARLOSS_B:
        return _("Marloss Desire");
    case ADD_MARLOSS_Y:
        return _("Marloss Cravings");
    default:
        return "Erroneous addiction";
    }
}

morale_type addiction_craving(add_type const cur)
{
    switch (cur) {
    case ADD_CIG:
        return MORALE_CRAVING_NICOTINE;
    case ADD_CAFFEINE:
        return MORALE_CRAVING_CAFFEINE;
    case ADD_ALCOHOL:
        return MORALE_CRAVING_ALCOHOL;
    case ADD_PKILLER:
        return MORALE_CRAVING_OPIATE;
    case ADD_SPEED:
        return MORALE_CRAVING_SPEED;
    case ADD_COKE:
        return MORALE_CRAVING_COCAINE;
    case ADD_CRACK:
        return MORALE_CRAVING_CRACK;
    case ADD_MUTAGEN:
        return MORALE_CRAVING_MUTAGEN;
    case ADD_DIAZEPAM:
        return MORALE_CRAVING_DIAZEPAM;
    case ADD_MARLOSS_R:
        return MORALE_CRAVING_MARLOSS;
    case ADD_MARLOSS_B:
        return MORALE_CRAVING_MARLOSS;
    case ADD_MARLOSS_Y:
        return MORALE_CRAVING_MARLOSS;
    default:
        return MORALE_NULL;
    }
}

add_type addiction_type(std::string const &name)
{
    if (name == "nicotine") {
        return ADD_CIG;
    } else if (name == "caffeine") {
        return ADD_CAFFEINE;
    } else if (name == "alcohol") {
        return ADD_ALCOHOL;
    } else if (name == "sleeping pill") {
        return ADD_SLEEP;
    } else if (name == "opiate") {
        return ADD_PKILLER;
    } else if (name == "amphetamine") {
        return ADD_SPEED;
    } else if (name == "cocaine") {
        return ADD_COKE;
    } else if (name == "crack") {
        return ADD_CRACK;
    } else if (name == "mutagen") {
        return ADD_MUTAGEN;
    } else if (name == "diazepam") {
        return ADD_DIAZEPAM;
    } else if (name == "marloss_r") {
        return ADD_MARLOSS_R;
    } else if (name == "marloss_b") {
        return ADD_MARLOSS_B;
    } else if (name == "marloss_y") {
        return ADD_MARLOSS_Y;
    } else {
        if (name != "none") {
            debugmsg("unknown addiction type: %s.  For no addictive potential, use \"none\"", name.c_str());
        }
        return ADD_NULL;
    }
}

std::string addiction_text(player const& u, addiction const &cur)
{
    int const strpen = 1 + cur.intensity / 7;
    switch (cur.type) {
    case ADD_CIG:
        return _("Intelligence - 1;   Occasional cravings");

    case ADD_CAFFEINE:
        return _("Strength - 1;   Slight sluggishness;   Occasional cravings");

    case ADD_ALCOHOL:
        return _("Perception - 1;   Intelligence - 1;   Occasional Cravings;\n"
                 "Risk of delirium tremens");

    case ADD_SLEEP:
        return _("You may find it difficult to sleep without medication.");

    case ADD_PKILLER: {
        if (u.has_trait("NOPAIN")) {
            return string_format(_( "Strength - %d;   Perception - 1;   Dexterity - 1;\n"
                                    "Depression.  Frequent cravings.  Vomiting."), strpen);
        } else {
            return string_format(_( "Strength - %d;   Perception - 1;   Dexterity - 1;\n"
                                    "Depression and physical pain to some degree.  Frequent cravings.  Vomiting."), strpen);
        }
    }

    case ADD_SPEED:
        return _("Strength - 1;   Intelligence - 1;\n"
                 "Movement rate reduction.  Depression.  Weak immune system.  Frequent cravings.");

    case ADD_COKE:
        return _("Perception - 1;   Intelligence - 1;  Frequent cravings.");

    case ADD_CRACK:
        return _("Perception - 2;   Intelligence - 2;  Frequent cravings.");

    case ADD_MUTAGEN:
        return _("You've gotten a taste for mutating and the chemicals that cause it.  But you can stop, yeah, any time you want.");

    case ADD_DIAZEPAM:
        return _("Perception - 1;   Intelligence - 1;\n"
                 "Anxiety, nausea, hallucinations, and general malaise.");

    case ADD_MARLOSS_R:
        return _("You should try some of those pink berries.");

    case ADD_MARLOSS_B:
        return _("You should try some of those cyan seeds.");

    case ADD_MARLOSS_Y:
        return _("You should try some of that golden gel.");

    default:
        return "";
    }
}
