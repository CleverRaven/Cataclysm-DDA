#include "player.h"
#include "profession.h"
#include "start_location.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "options.h"
#include "catacharset.h"
#include "debug.h"
#include "char_validity_check.h"
#include "path_info.h"
#include "mapsharing.h"
#include "item_factory.h"
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>

// Colors used in this file: (Most else defaults to c_ltgray)
#define COL_STAT_ACT        c_white   // Selected stat
#define COL_STAT_BONUS      c_ltgreen // Bonus
#define COL_STAT_NEUTRAL    c_white   // Neutral Property
#define COL_STAT_PENALTY    c_ltred   // Penalty
#define COL_TR_GOOD         c_green   // Good trait descriptive text
#define COL_TR_GOOD_OFF_ACT c_ltgray  // A toggled-off good trait
#define COL_TR_GOOD_ON_ACT  c_ltgreen // A toggled-on good trait
#define COL_TR_GOOD_OFF_PAS c_dkgray  // A toggled-off good trait
#define COL_TR_GOOD_ON_PAS  c_green   // A toggled-on good trait
#define COL_TR_BAD          c_red     // Bad trait descriptive text
#define COL_TR_BAD_OFF_ACT  c_ltgray  // A toggled-off bad trait
#define COL_TR_BAD_ON_ACT   c_ltred   // A toggled-on bad trait
#define COL_TR_BAD_OFF_PAS  c_dkgray  // A toggled-off bad trait
#define COL_TR_BAD_ON_PAS   c_red     // A toggled-on bad trait
#define COL_SKILL_USED      c_green   // A skill with at least one point
#define COL_HEADER          c_white   // Captions, like "Profession items"
#define COL_NOTE_MAJOR      c_green   // Important note
#define COL_NOTE_MINOR      c_ltgray  // Just regular note

#define HIGH_STAT 14 // The point after which stats cost double
#define MAX_STAT 20 // The point after which stats can not be increased further
#define MAX_SKILL 20 // The maximum selectable skill level

#define NEWCHAR_TAB_MAX 4 // The ID of the rightmost tab

void draw_tabs(WINDOW *w, std::string sTab);

int set_stats(WINDOW *w, player *u, int &points);
int set_traits(WINDOW *w, player *u, int &points, int max_trait_points);
int set_profession(WINDOW *w, player *u, int &points);
int set_skills(WINDOW *w, player *u, int &points);
int set_description(WINDOW *w, player *u, character_type type, int &points);

Skill *random_skill();

void save_template(player *u);

bool player::create(character_type type, std::string tempname)
{
    weapon = item("null", 0);

    g->u.prof = profession::generic();

    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    int tab = 0;
    int points = OPTIONS["INITIAL_POINTS"];
    int max_trait_points = OPTIONS["MAX_TRAIT_POINTS"];
    if (type != PLTYPE_CUSTOM) {
        points = points + 32;
        switch (type) {
            case PLTYPE_CUSTOM:
                break;
            case PLTYPE_MAX:
                break;
            case PLTYPE_NOW:
                g->u.male = (rng(1, 100) > 50);

                if(!MAP_SHARING::isSharing()) {
                    g->u.pick_name();
                } else {
                    g->u.name = MAP_SHARING::getUsername();
                }
            case PLTYPE_RANDOM: {
                g->u.male = (rng(1, 100) > 50);
                if(!MAP_SHARING::isSharing()) {
                    g->u.pick_name();
                } else {
                    g->u.name = MAP_SHARING::getUsername();
                }
                g->u.prof = profession::weighted_random();
                str_max = rng(6, 12);
                dex_max = rng(6, 12);
                int_max = rng(6, 12);
                per_max = rng(6, 12);
                points = points - str_max - dex_max - int_max - per_max - g->u.prof->point_cost();
                if (str_max > HIGH_STAT) {
                    points -= (str_max - HIGH_STAT);
                }
                if (dex_max > HIGH_STAT) {
                    points -= (dex_max - HIGH_STAT);
                }
                if (int_max > HIGH_STAT) {
                    points -= (int_max - HIGH_STAT);
                }
                if (per_max > HIGH_STAT) {
                    points -= (per_max - HIGH_STAT);
                }

                int num_gtraits = 0, num_btraits = 0, tries = 0;
                std::string rn = "";

                while (points < 0 || rng(-3, 20) > points) {
                    if (num_btraits < max_trait_points && one_in(3)) {
                        tries = 0;
                        do {
                            rn = random_bad_trait();
                            tries++;
                        } while ((has_trait(rn) || num_btraits - traits[rn].points > max_trait_points) &&
                                 tries < 5);

                        if (tries < 5 && !has_conflicting_trait(rn)) {
                            toggle_trait(rn);
                            points -= traits[rn].points;
                            num_btraits -= traits[rn].points;
                        }
                    } else {
                        switch (rng(1, 4)) {
                            case 1:
                                if (str_max > 5) {
                                    str_max--;
                                    points++;
                                }
                                break;
                            case 2:
                                if (dex_max > 5) {
                                    dex_max--;
                                    points++;
                                }
                                break;
                            case 3:
                                if (int_max > 5) {
                                    int_max--;
                                    points++;
                                }
                                break;
                            case 4:
                                if (per_max > 5) {
                                    per_max--;
                                    points++;
                                }
                                break;
                        }
                    }
                }

                /* The loops variable is used to prevent the algorithm running in an infinte loop */
                unsigned int loops = 0;
                while (points > 0 && loops <= 30000) {
                    switch (rng((num_gtraits < max_trait_points ? 1 : 5), 9)) {
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                            rn = random_good_trait();
                            if (!has_trait(rn) && points >= traits[rn].points &&
                                num_gtraits + traits[rn].points <= max_trait_points &&
                                !has_conflicting_trait(rn)) {
                                toggle_trait(rn);
                                points -= traits[rn].points;
                                num_gtraits += traits[rn].points;
                            }
                            break;
                        case 5:
                            switch (rng(1, 4)) {
                                case 1:
                                    if (str_max < HIGH_STAT) {
                                        str_max++;
                                        points--;
                                    } else if (points >= 2 && str_max < MAX_STAT) {
                                        str_max++;
                                        points = points - 2;
                                    }
                                    break;
                                case 2:
                                    if (dex_max < HIGH_STAT) {
                                        dex_max++;
                                        points--;
                                    } else if (points >= 2 && dex_max < MAX_STAT) {
                                        dex_max++;
                                        points = points - 2;
                                    }
                                    break;
                                case 3:
                                    if (int_max < HIGH_STAT) {
                                        int_max++;
                                        points--;
                                    } else if (points >= 2 && int_max < MAX_STAT) {
                                        int_max++;
                                        points = points - 2;
                                    }
                                    break;
                                case 4:
                                    if (per_max < HIGH_STAT) {
                                        per_max++;
                                        points--;
                                    } else if (points >= 2 && per_max < MAX_STAT) {
                                        per_max++;
                                        points = points - 2;
                                    }
                                    break;
                            }
                            break;
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                            Skill *aSkill = random_skill();
                            int level = skillLevel(aSkill);

                            if (level < points && level < MAX_SKILL && (level <= 10 || loops > 10000)) {
                                points -= level + 1;
                                skillLevel(aSkill).level(level + 2);
                            }
                            break;
                    }
                    loops++;
                }
            }
            break;
            case PLTYPE_TEMPLATE: {
                std::ifstream fin;
                std::stringstream filename;
                filename << FILENAMES["templatedir"] << tempname << ".template";
                fin.open(filename.str().c_str());
                if (!fin.is_open()) {
                    debugmsg("Couldn't open %s!", filename.str().c_str());
                    return false;
                }
                std::string(data);
                getline(fin, data);
                load_info(data);
                points = 0;

                if(MAP_SHARING::isSharing()) {
                    name = MAP_SHARING::getUsername(); //just to make sure we have the right name
                }
            }
            break;
        }
        tab = NEWCHAR_TAB_MAX;
    }

    do {
        werase(w);
        wrefresh(w);
        switch (tab) {
            case 0:
                tab += set_stats      (w, this, points);
                break;
            case 1:
                tab += set_traits     (w, this, points, max_trait_points);
                break;
            case 2:
                tab += set_profession (w, this, points);
                break;
            case 3:
                tab += set_skills     (w, this, points);
                break;
            case 4:
                tab += set_description(w, this, type, points);
                break;
        }
    } while (tab >= 0 && tab <= NEWCHAR_TAB_MAX);
    delwin(w);

    if (tab < 0) {
        return false;
    }

    recalc_hp();
    for (int i = 0; i < num_hp_parts; i++) {
        hp_cur[i] = hp_max[i];
    }

    if (has_trait("SMELLY")) {
        scent = 800;
    }
    if (has_trait("WEAKSCENT")) {
        scent = 300;
    }

    if (has_trait("MARTIAL_ARTS")) {
        matype_id ma_type;
        do {
            int choice = (PLTYPE_NOW == type) ? rng(1, 5) :
                         menu(false, _("Pick your style:"), _("Karate"), _("Judo"), _("Aikido"),
                              _("Tai Chi"), _("Taekwondo"), NULL);
            if (choice == 1) {
                ma_type = "style_karate";
            } else if (choice == 2) {
                ma_type = "style_judo";
            } else if (choice == 3) {
                ma_type = "style_aikido";
            } else if (choice == 4) {
                ma_type = "style_tai_chi";
            } else { // choice == 5
                ma_type = "style_taekwondo";
            }
            if (PLTYPE_NOW != type) {
                popup(martialarts[ma_type].description, PF_NONE);
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }
    if (has_trait("MARTIAL_ARTS2")) {
        matype_id ma_type;
        do {
            int choice = (PLTYPE_NOW == type) ? rng(1, 5) :
                         menu(false, _("Pick your style:"), _("Krav Maga"), _("Muay Thai"),
                              _("Ninjutsu"), _("Capoeira"), _("Zui Quan"), NULL);
            if (choice == 1) {
                ma_type = "style_krav_maga";
            } else if (choice == 2) {
                ma_type = "style_muay_thai";
            } else if (choice == 3) {
                ma_type = "style_ninjutsu";
            } else if (choice == 4) {
                ma_type = "style_capoeira";
            } else { // choice == 5
                ma_type = "style_zui_quan";
            }
            if (PLTYPE_NOW != type) {
                popup(martialarts[ma_type].description, PF_NONE);
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }
    if (has_trait("MARTIAL_ARTS3")) {
        matype_id ma_type;
        do {
            int choice = (PLTYPE_NOW == type) ? rng(1, 5) :
                         menu(false, _("Pick your style:"), _("Tiger"), _("Crane"), _("Leopard"),
                              _("Snake"), _("Dragon"), NULL);
            if (choice == 1) {
                ma_type = "style_tiger";
            } else if (choice == 2) {
                ma_type = "style_crane";
            } else if (choice == 3) {
                ma_type = "style_leopard";
            } else if (choice == 4) {
                ma_type = "style_snake";
            } else { // choice == 5
                ma_type = "style_dragon";
            }
            if (PLTYPE_NOW != type) {
                popup(martialarts[ma_type].description, PF_NONE);
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }
    if (has_trait("MARTIAL_ARTS4")) {
        matype_id ma_type;
        do {
            int choice = (PLTYPE_NOW == type) ? rng(1, 5) :
                         menu(false, _("Pick your style:"), _("Centipede"), _("Viper"),
                              _("Scorpion"), _("Lizard"), _("Toad"), NULL);
            if (choice == 1) {
                ma_type = "style_centipede";
            } else if (choice == 2) {
                ma_type = "style_venom_snake";
            } else if (choice == 3) {
                ma_type = "style_scorpion";
            } else if (choice == 4) {
                ma_type = "style_lizard";
            } else { // choice == 5
                ma_type = "style_toad";
            }
            if (PLTYPE_NOW != type) {
                popup(martialarts[ma_type].description, PF_NONE);
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }
    if (has_trait("MARTIAL_ARTS5")) {
        matype_id ma_type;
        do {
            int choice = (PLTYPE_NOW == type) ? rng(1, 2) :
                         menu(false, _("Pick your style:"), _("Eskrima"), _("Fencing"), _("Pentjak Silat"),
                              NULL);
            if (choice == 1) {
                ma_type = "style_eskrima";
            } else if (choice == 2) {
                ma_type = "style_fencing";
            } else if (choice == 3) {
                ma_type = "style_silat";
            }
            if (PLTYPE_NOW != type) {
                popup(martialarts[ma_type].description, PF_NONE);
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }


    ret_null = item("null", 0);
    weapon = ret_null;


    item tmp; //gets used several times

    std::vector<std::string> prof_items = g->u.prof->items();
    std::vector<std::string> gender_items;
    if(g->u.male) {
        gender_items = g->u.prof->items_male();
    } else {
        gender_items = g->u.prof->items_female();
    }
    prof_items.insert(prof_items.begin(), gender_items.begin(), gender_items.end());

    // Those who are both near-sighted and far-sighted start with bifocal glasses.
    if (has_trait("HYPEROPIC") && has_trait("MYOPIC")) {
        prof_items.push_back("glasses_bifocal");
    }
    // The near-sighted start with eyeglasses.
    else if (has_trait("MYOPIC")) {
        prof_items.push_back("glasses_eye");
    }
    // The far-sighted start with reading glasses.
    else if (has_trait("HYPEROPIC")) {
        prof_items.push_back("glasses_reading");
    }

    for (std::vector<std::string>::const_iterator iter = prof_items.begin();
         iter != prof_items.end(); ++iter) {
        tmp = item(*iter, 0, false);
        tmp = tmp.in_its_container();
        if(tmp.is_armor()) {
            if(tmp.has_flag("VARSIZE")) {
                tmp.item_tags.insert("FIT");
            }
            // If wearing an item fails we fail silently.
            wear_item(&tmp, false);
        // if something is wet, start it as active with some time to dry off
        } else if(tmp.has_flag("WET")) {
            tmp.active = true;
            tmp.item_counter = 450;
            inv.push_back(tmp);
        } else {
            inv.push_back(tmp);
        }
    }

    std::vector<addiction> prof_addictions = g->u.prof->addictions();
    for (std::vector<addiction>::const_iterator iter = prof_addictions.begin();
         iter != prof_addictions.end(); ++iter) {
        g->u.addictions.push_back(*iter);
    }

    // Grab the skills from the profession, if there are any
    profession::StartingSkillList prof_skills = g->u.prof->skills();
    for (profession::StartingSkillList::const_iterator iter = prof_skills.begin();
         iter != prof_skills.end(); ++iter) {
        assert(Skill::skill(iter->first));
        if (Skill::skill(iter->first)) {
            g->u.boost_skill_level(iter->first, iter->second);
        }
    }

    // Get CBMs
    std::vector<std::string> prof_CBMs = g->u.prof->CBMs();
    for (std::vector<std::string>::const_iterator iter = prof_CBMs.begin();
         iter != prof_CBMs.end(); ++iter) {
        if (*iter == "bio_power_storage") {
            max_power_level += 4;
            power_level += 4;
        } else if (*iter == "bio_power_storage_mkII") {
            max_power_level += 10;
            power_level += 10;
        } else {
            add_bionic(*iter);
        }
    }

    // Likewise, the asthmatic start with their medication.
    if (has_trait("ASTHMA")) {
        tmp = item("inhaler", 0, false);
        inv.push_back(tmp);
    }

    // And cannibals start with a special cookbook.
    if (has_trait("CANNIBAL")) {
        tmp = item("cookbook_human", 0);
        inv.push_back(tmp);
    }

    // Albinoes have their umbrella handy.
    // Since they have to wield it, I don't think it breaks things
    // too badly to issue one.
    if (has_trait("ALBINO")) {
        tmp = item("teleumbrella", 0);
        inv.push_back(tmp);
    }

    // make sure we have no mutations
    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter)
        if (!has_base_trait(iter->first)) {
            my_mutations.erase(iter->first);
        }

    // Ensure that persistent morale effects (e.g. Optimist) are present at the start.
    apply_persistent_morale();
    return true;
}

void draw_tabs(WINDOW *w, std::string sTab)
{
    for (int i = 1; i < FULL_SCREEN_WIDTH - 1; i++) {
        mvwputch(w, 2, i, BORDER_COLOR, LINE_OXOX);
        mvwputch(w, 4, i, BORDER_COLOR, LINE_OXOX);
        mvwputch(w, FULL_SCREEN_HEIGHT - 1, i, BORDER_COLOR, LINE_OXOX);

        if (i > 2 && i < FULL_SCREEN_HEIGHT - 1) {
            mvwputch(w, i, 0, BORDER_COLOR, LINE_XOXO);
            mvwputch(w, i, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOXO);
        }
    }
    std::vector<std::string> tab_captions;
    tab_captions.push_back(_("STATS"));
    tab_captions.push_back(_("TRAITS"));
    tab_captions.push_back(_("PROFESSION"));
    tab_captions.push_back(_("SKILLS"));
    tab_captions.push_back(_("DESCRIPTION"));
    std::vector<int> tab_pos(tab_captions.size() + 1, 0);
    tab_pos[0] = 2;
    for (size_t pos = 0; pos < tab_captions.size(); pos++) {
        tab_pos[pos + 1] = tab_pos[pos] + utf8_width(tab_captions[pos].c_str());
    }
    int space = (FULL_SCREEN_WIDTH - tab_pos[tab_captions.size()]) / (tab_captions.size() - 1) - 3;
    for (size_t i = 0; i < tab_captions.size(); i++) {
        draw_tab(w, tab_pos[i] + space * i, tab_captions[i].c_str(), (sTab == tab_captions[i]));
    }

    mvwputch(w, 2,  0, BORDER_COLOR, LINE_OXXO); // |^
    mvwputch(w, 2, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_OOXX); // ^|

    mvwputch(w, 4, 0, BORDER_COLOR, LINE_XXXO); // |-
    mvwputch(w, 4, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOXX); // -|

    mvwputch(w, FULL_SCREEN_HEIGHT - 1, 0, BORDER_COLOR, LINE_XXOO); // |_
    mvwputch(w, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOOX); // _|
}

int set_stats(WINDOW *w, player *u, int &points)
{
    unsigned char sel = 1;
    const int iSecondColumn = 27;
    input_context ctxt("NEW_CHAR_STATS");
    ctxt.register_cardinal();
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("NEXT_TAB");
    int read_spd;
    WINDOW *w_description = newwin(8, FULL_SCREEN_WIDTH - iSecondColumn - 1, 6 + getbegy(w),
                                   iSecondColumn + getbegx(w));
    // There is no map loaded currently, so any access to the map will
    // fail (player::suffer, called from player::reset_stats), might access
    // the map:
    // There are traits that check/change the radioactivity on the map,
    // that check if in sunlight...
    // Setting the position to -1 ensures that the INBOUNDS check in
    // map.cpp is triggered. This check prevents access to invalid position
    // on the map (like -1,0) and instead returns a dummy default value.
    u->posx = -1;
    u->reset();

    const char clear[] = "                                                ";

    do {
        werase(w);
        draw_tabs(w, _("STATS"));
        fold_and_print(w, 16, 2, getmaxx(w) - 4, COL_NOTE_MINOR, _("\
    <color_light_green>%s</color> / <color_light_green>%s</color> to select a statistic.\n\
    <color_light_green>%s</color> to increase the statistic.\n\
    <color_light_green>%s</color> to decrease the statistic."),
    ctxt.get_desc("UP").c_str(), ctxt.get_desc("DOWN").c_str(),
    ctxt.get_desc("RIGHT").c_str(), ctxt.get_desc("LEFT").c_str()
        );
        mvwprintz(w, FULL_SCREEN_HEIGHT - 3, 2, COL_NOTE_MAJOR, _("%s Takes you to the next tab."), ctxt.get_desc("NEXT_TAB").c_str());
        mvwprintz(w, FULL_SCREEN_HEIGHT - 2, 2, COL_NOTE_MAJOR, _("%s Returns you to the main menu."), ctxt.get_desc("PREV_TAB").c_str());

        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%4d "), points);
        mvwprintz(w, 3, iSecondColumn, c_black, clear);
        for (int i = 6; i < 13; i++) {
            mvwprintz(w, i, iSecondColumn, c_black, clear);
        }
        mvwprintz(w, 6,  2, c_ltgray, _("Strength:"));
        mvwprintz(w, 6, 16, c_ltgray, "%2d", u->str_max);
        mvwprintz(w, 7,  2, c_ltgray, _("Dexterity:"));
        mvwprintz(w, 7, 16, c_ltgray, "%2d", u->dex_max);
        mvwprintz(w, 8,  2, c_ltgray, _("Intelligence:"));
        mvwprintz(w, 8, 16, c_ltgray, "%2d", u->int_max);
        mvwprintz(w, 9,  2, c_ltgray, _("Perception:"));
        mvwprintz(w, 9, 16, c_ltgray, "%2d", u->per_max);

        werase(w_description);
        switch (sel) {
            case 1:
                mvwprintz(w, 6, 2, COL_STAT_ACT, _("Strength:"));
                mvwprintz(w, 6, 16, COL_STAT_ACT, "%2d", u->str_max);
                if (u->str_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Str further costs 2 points."));
                }
                u->recalc_hp();
                mvwprintz(w_description, 0, 0, COL_STAT_NEUTRAL, _("Base HP: %d"), u->hp_max[0]);
                mvwprintz(w_description, 1, 0, COL_STAT_NEUTRAL, _("Carry weight: %.1f %s"),
                          u->convert_weight(u->weight_capacity(false)),
                          OPTIONS["USE_METRIC_WEIGHTS"] == "kg" ? _("kg") : _("lbs"));
                mvwprintz(w_description, 2, 0, COL_STAT_NEUTRAL, _("Melee damage: %d"),
                          u->base_damage(false));
                fold_and_print(w_description, 4, 0, getmaxx(w_description) - 1, COL_STAT_NEUTRAL,
                               _("Strength also makes you more resistant to many diseases and poisons, and makes actions which require brute force more effective."));
                break;

            case 2:
                mvwprintz(w, 7,  2, COL_STAT_ACT, _("Dexterity:"));
                mvwprintz(w, 7,  16, COL_STAT_ACT, "%2d", u->dex_max);
                if (u->dex_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Dex further costs 2 points."));
                }
                mvwprintz(w_description, 0, 0, COL_STAT_BONUS, _("Melee to-hit bonus: +%d"),
                          u->base_to_hit(false));
                if (u->throw_dex_mod(false) <= 0) {
                    mvwprintz(w_description, 1, 0, COL_STAT_BONUS, _("Throwing bonus: +%d"),
                              abs(u->throw_dex_mod(false)));
                } else {
                    mvwprintz(w_description, 1, 0, COL_STAT_PENALTY, _("Throwing penalty: -%d"),
                              abs(u->throw_dex_mod(false)));
                }
                if (u->ranged_dex_mod(false) != 0) {
                    mvwprintz(w_description, 2, 0, COL_STAT_PENALTY, _("Ranged penalty: -%d"),
                              abs(u->ranged_dex_mod(false)));
                }
                fold_and_print(w_description, 4, 0, getmaxx(w_description) - 1, COL_STAT_NEUTRAL,
                               _("Dexterity also enhances many actions which require finesse."));
                break;

            case 3:
                mvwprintz(w, 8,  2, COL_STAT_ACT, _("Intelligence:"));
                mvwprintz(w, 8,  16, COL_STAT_ACT, "%2d", u->int_max);
                if (u->int_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Int further costs 2 points."));
                }
                read_spd = u->read_speed(false);
                mvwprintz(w_description, 0, 0, (read_spd == 100 ? COL_STAT_NEUTRAL :
                                                (read_spd < 100 ? COL_STAT_BONUS : COL_STAT_PENALTY)),
                          _("Read times: %d%%"), read_spd);
                mvwprintz(w_description, 1, 0, COL_STAT_PENALTY, _("Skill rust: %d%%"),
                          u->rust_rate(false));
                fold_and_print(w_description, 3, 0, getmaxx(w_description) - 1, COL_STAT_NEUTRAL,
                               _("Intelligence is also used when crafting, installing bionics, and interacting with NPCs."));
                break;

            case 4:
                mvwprintz(w, 9,  2, COL_STAT_ACT, _("Perception:"));
                mvwprintz(w, 9,  16, COL_STAT_ACT, "%2d", u->per_max);
                if (u->per_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Per further costs 2 points."));
                }
                if (u->ranged_per_mod(false) != 0) {
                    mvwprintz(w_description, 0, 0, COL_STAT_PENALTY, _("Ranged penalty: -%d"),
                              abs(u->ranged_per_mod(false)));
                }
                fold_and_print(w_description, 2, 0, getmaxx(w_description) - 1, COL_STAT_NEUTRAL,
                               _("Perception is also used for detecting traps and other things of interest."));
                break;
        }

        wrefresh(w);
        wrefresh(w_description);
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            if (sel < 4) {
                sel++;
            } else {
                sel = 1;
            }
        } else if (action == "UP") {
            if (sel > 1) {
                sel--;
            } else {
                sel = 4;
            }
        } else if (action == "LEFT") {
            if (sel == 1 && u->str_max > 4) {
                if (u->str_max > HIGH_STAT) {
                    points++;
                }
                u->str_max--;
                points++;
            } else if (sel == 2 && u->dex_max > 4) {
                if (u->dex_max > HIGH_STAT) {
                    points++;
                }
                u->dex_max--;
                points++;
            } else if (sel == 3 && u->int_max > 4) {
                if (u->int_max > HIGH_STAT) {
                    points++;
                }
                u->int_max--;
                points++;
            } else if (sel == 4 && u->per_max > 4) {
                if (u->per_max > HIGH_STAT) {
                    points++;
                }
                u->per_max--;
                points++;
            }
        } else if (action == "RIGHT") {
            if (sel == 1 && u->str_max < MAX_STAT) {
                points--;
                if (u->str_max >= HIGH_STAT) {
                    points--;
                }
                u->str_max++;
            } else if (sel == 2 && u->dex_max < MAX_STAT) {
                points--;
                if (u->dex_max >= HIGH_STAT) {
                    points--;
                }
                u->dex_max++;
            } else if (sel == 3 && u->int_max < MAX_STAT) {
                points--;
                if (u->int_max >= HIGH_STAT) {
                    points--;
                }
                u->int_max++;
            } else if (sel == 4 && u->per_max < MAX_STAT) {
                points--;
                if (u->per_max >= HIGH_STAT) {
                    points--;
                }
                u->per_max++;
            }
        } else if (action == "PREV_TAB" && query_yn(_("Return to main menu?"))) {
            delwin(w_description);
            return -1;
        } else if (action == "NEXT_TAB") {
            delwin(w_description);
            return 1;
        }
    } while (true);
}

int set_traits(WINDOW *w, player *u, int &points, int max_trait_points)
{
    draw_tabs(w, _("TRAITS"));

    WINDOW *w_description = newwin(3, FULL_SCREEN_WIDTH - 2, FULL_SCREEN_HEIGHT - 4 + getbegy(w),
                                   1 + getbegx(w));
    // Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
    int num_good = 0, num_bad = 0;

    std::vector<std::string> vStartingTraits[2];

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        if (iter->second.startingtrait) {
            if (iter->second.points >= 0) {
                vStartingTraits[0].push_back(iter->first);

                if (u->has_trait(iter->first)) {
                    num_good += iter->second.points;
                }
            } else {
                vStartingTraits[1].push_back(iter->first);

                if (u->has_trait(iter->first)) {
                    num_bad += iter->second.points;
                }
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        std::sort(vStartingTraits[i].begin(), vStartingTraits[i].end(), trait_display_sort);
    }

    nc_color col_on_act, col_off_act, col_on_pas, col_off_pas, hi_on, hi_off, col_tr;

    const size_t iContentHeight = FULL_SCREEN_HEIGHT - 9;
    int iCurWorkingPage = 0;

    int iStartPos[2];
    iStartPos[0] = 0;
    iStartPos[1] = 0;

    int iCurrentLine[2];
    iCurrentLine[0] = 0;
    iCurrentLine[1] = 0;

    input_context ctxt("NEW_CHAR_TRAITS");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("HELP_KEYBINDINGS");

    do {
        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%4d "), points);
        mvwprintz(w, 3, 19, c_ltgreen, "%4d/%-4d", num_good, max_trait_points);
        mvwprintz(w, 3, 29, c_ltred, "%5d/-%-4d ", num_bad, max_trait_points);

        // Clear the bottom of the screen.
        werase(w_description);

        for (int iCurrentPage = 0; iCurrentPage < 2; iCurrentPage++) { //Good/Bad
            if (iCurrentPage == 0) {
                col_on_act  = COL_TR_GOOD_ON_ACT;
                col_off_act = COL_TR_GOOD_OFF_ACT;
                col_on_pas  = COL_TR_GOOD_ON_PAS;
                col_off_pas = COL_TR_GOOD_OFF_PAS;
                col_tr = COL_TR_GOOD;
                hi_on  = hilite(col_on_act);
                hi_off = hilite(col_off_act);
            } else {
                col_on_act  = COL_TR_BAD_ON_ACT;
                col_off_act = COL_TR_BAD_OFF_ACT;
                col_on_pas  = COL_TR_BAD_ON_PAS;
                col_off_pas = COL_TR_BAD_OFF_PAS;
                col_tr = COL_TR_BAD;
                hi_on  = hilite(col_on_act);
                hi_off = hilite(col_off_act);
            }

            calcStartPos(iStartPos[iCurrentPage], iCurrentLine[iCurrentPage], iContentHeight,
                         vStartingTraits[iCurrentPage].size());

            //Draw Traits
            for (int i = iStartPos[iCurrentPage]; (size_t) i < vStartingTraits[iCurrentPage].size(); i++) {
                if (i >= iStartPos[iCurrentPage] && (size_t) i < iStartPos[iCurrentPage] +
                    ((iContentHeight > vStartingTraits[iCurrentPage].size()) ?
                     vStartingTraits[iCurrentPage].size() : iContentHeight)) {
                    if (iCurrentLine[iCurrentPage] == i && iCurrentPage == iCurWorkingPage) {
                        mvwprintz(w,  3, 41, c_ltgray,
                                  "                                      ");
                        int points = traits[vStartingTraits[iCurrentPage][i]].points;
                        bool negativeTrait = points < 0;
                        if (negativeTrait) {
                                  points *=-1;
                        }
                        mvwprintz(w,  3, 41, col_tr, ngettext("%s %s %d point", "%s %s %d points", points),
                                  traits[vStartingTraits[iCurrentPage][i]].name.c_str(),
                                  negativeTrait ? _("earns"):_("costs"),
                                  points);
                        fold_and_print(w_description, 0, 0,
                                       FULL_SCREEN_WIDTH - 2, col_tr,
                                       traits[vStartingTraits[iCurrentPage][i]].description);
                    }

                    nc_color cLine = col_off_pas;
                    if (iCurWorkingPage == iCurrentPage) {
                        cLine = col_off_act;
                        if (iCurrentLine[iCurrentPage] == i) {
                            cLine = hi_off;

                            if (u->has_conflicting_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = hilite(c_dkgray);
                            } else if (u->has_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = hi_on;
                            }
                        } else {
                            if (u->has_conflicting_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = c_dkgray;

                            } else if (u->has_trait(vStartingTraits[iCurrentPage][i])) {
                                cLine = col_on_act;
                            }
                        }
                    } else if (u->has_trait(vStartingTraits[iCurrentPage][i])) {
                        cLine = col_on_pas;

                    } else if (u->has_conflicting_trait(vStartingTraits[iCurrentPage][i])) {
                        cLine = c_ltgray;
                    }

                    mvwprintz(w, 5 + i - iStartPos[iCurrentPage],
                              (iCurrentPage == 0) ? 2 : 40, c_ltgray, "\
                                  "); // Clear the line
                    mvwprintz(w, 5 + i - iStartPos[iCurrentPage], (iCurrentPage == 0) ? 2 : 40, cLine,
                              traits[vStartingTraits[iCurrentPage][i]].name.c_str());
                }
            }

            //Draw Scrollbar Good Traits
            draw_scrollbar(w, iCurrentLine[0], iContentHeight, vStartingTraits[0].size(), 5);

            //Draw Scrollbar Bad Traits
            draw_scrollbar(w, iCurrentLine[1], iContentHeight,
                           vStartingTraits[1].size(), 5, getmaxx(w) - 1);
        }

        wrefresh(w);
        wrefresh(w_description);
        const std::string action = ctxt.handle_input();
        if (action == "LEFT") {
            iCurWorkingPage--;
            if (iCurWorkingPage < 0) {
                iCurWorkingPage = 1;
            }
        } else if (action == "RIGHT") {
            iCurWorkingPage++;
            if (iCurWorkingPage > 1) {
                iCurWorkingPage = 0;
            }
        } else if (action == "UP") {
                iCurrentLine[iCurWorkingPage]--;
                if (iCurrentLine[iCurWorkingPage] < 0) {
                    iCurrentLine[iCurWorkingPage] = vStartingTraits[iCurWorkingPage].size() - 1;
                }
        } else if (action == "DOWN") {
                iCurrentLine[iCurWorkingPage]++;
                if ((size_t) iCurrentLine[iCurWorkingPage] >= vStartingTraits[iCurWorkingPage].size()) {
                    iCurrentLine[iCurWorkingPage] = 0;
                }
        } else if (action == "CONFIRM") {
                int inc_type = 0;
                std::string cur_trait = vStartingTraits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
                if (u->has_trait(cur_trait)) {

                    inc_type = -1;


                    // If turning off the trait violates a profession condition,
                    // turn it back on.
                    if(u->prof->can_pick(u, 0) != "YES") {
                        inc_type = 0;
                        popup(_("Your profession of %s prevents you from removing this trait."),
                              u->prof->gender_appropriate_name(u->male).c_str());

                    }

                } else if(u->has_conflicting_trait(cur_trait)) {
                    popup(_("You already picked a conflicting trait!"));

                } else if (iCurWorkingPage == 0 && num_good + traits[cur_trait].points >
                           max_trait_points) {
                    popup(ngettext("Sorry, but you can only take %d point of advantages.", "Sorry, but you can only take %d points of advantages.", max_trait_points),
                          max_trait_points);

                } else if (iCurWorkingPage != 0 && num_bad + traits[cur_trait].points <
                           -max_trait_points) {
                    popup(ngettext("Sorry, but you can only take %d point of disadvantages.", "Sorry, but you can only take %d points of disadvantages.", max_trait_points),
                          max_trait_points);

                } else {
                    inc_type = 1;

                    // If turning on the trait violates a profession condition,
                    // turn it back off.
                    if(u->prof->can_pick(u, 0) != "YES") {
                        inc_type = 0;
                        popup(_("Your profession of %s prevents you from taking this trait."),
                              u->prof->gender_appropriate_name(u->male).c_str());

                    }
                }

                //inc_type is either -1 or 1, so we can just multiply by it to invert
                if(inc_type != 0) {
                    u->toggle_trait(cur_trait);
                    points -= traits[cur_trait].points * inc_type;
                    if (iCurWorkingPage == 0) {
                        num_good += traits[cur_trait].points * inc_type;
                    } else {
                        num_bad += traits[cur_trait].points * inc_type;
                    }
                }
        } else if (action == "PREV_TAB") {
                delwin(w_description);
                return -1;
        } else if (action == "NEXT_TAB") {
                delwin(w_description);
                return 1;
        }
    } while (true);
}

inline bool profession_display_sort(const profession *a, const profession *b)
{
    // The generic ("Unemployed") profession should be listed first.
    const profession *gen = profession::generic();
    if (b == gen) {
        return false;
    } else if (a == gen) {
        return true;
    }

    return a->point_cost() < b->point_cost();
}

int set_profession(WINDOW *w, player *u, int &points)
{
    draw_tabs(w, _("PROFESSION"));

    int cur_id = 0;
    int retval = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 10;
    int iStartPos = 0;

    WINDOW *w_description = newwin(4, FULL_SCREEN_WIDTH - 2,
                                   FULL_SCREEN_HEIGHT - 5 + getbegy(w), 1 + getbegx(w));

    WINDOW *w_items =       newwin(iContentHeight - 1, 25,  6 + getbegy(w), 24 + getbegx(w));
    WINDOW *w_skills =      newwin(iContentHeight - 6, 30,  6 + getbegy(w), 49 + getbegx(w));
    WINDOW *w_addictions =  newwin(5,                  30, 14 + getbegy(w), 49 + getbegx(w));
    WINDOW *w_genderswap =  newwin(1,                  55,  5 + getbegy(w), 24 + getbegx(w));

    std::vector<const profession *> sorted_profs;
    for (profmap::const_iterator iter = profession::begin(); iter != profession::end(); ++iter) {
        sorted_profs.push_back(&(iter->second));
    }

    // Sort professions by name.
    // profession_display_sort() keeps "unemployed" at the top.
    std::sort(sorted_profs.begin(), sorted_profs.end(), profession_display_sort);

    // Select the current profession, if possible.
    for (size_t i = 0; i < sorted_profs.size(); ++i) {
        if (sorted_profs[i]->ident() == u->prof->ident()) {
            cur_id = i;
            break;
        }
    }

    input_context ctxt("NEW_CHAR_PROFESSIONS");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("CHANGE_GENDER");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("HELP_KEYBINDINGS");

    do {
        int netPointCost = sorted_profs[cur_id]->point_cost() - u->prof->point_cost();
        std::string can_pick = sorted_profs[cur_id]->can_pick(u, points);

        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%4d "), points);
        // Clear the bottom of the screen.
        werase(w_description);
        mvwprintz(w, 3, 40, c_ltgray, "                                       ");

        int pointsForProf = sorted_profs[cur_id]->point_cost();
        bool negativeProf = pointsForProf < 0;
        if (negativeProf) {
                  pointsForProf *=-1;
        }
        mvwprintz(w, 3, 21, can_pick == "YES" ? c_green:c_ltred, ngettext("Profession %1$s %2$s %3$d point (net: %4$d)",
                                                                          "Profession %1$s %2$s %3$d points (net: %4$d)",
                                                                          pointsForProf),
                      sorted_profs[cur_id]->gender_appropriate_name(u->male).c_str(),
                      negativeProf ? _("earns"):_("costs"),
                      pointsForProf, netPointCost);

        fold_and_print(w_description, 0, 0, FULL_SCREEN_WIDTH - 2, c_green,
                       sorted_profs[cur_id]->description());

        calcStartPos(iStartPos, cur_id, iContentHeight, profession::count());

        //Draw options
        for (int i = iStartPos; i < iStartPos + ((iContentHeight > profession::count()) ?
             profession::count() : iContentHeight); i++) {
            mvwprintz(w, 5 + i - iStartPos, 2, c_ltgray, "\
                                             "); // Clear the line
            nc_color col;
            if (u->prof != sorted_profs[i]) {
                col = (sorted_profs[i] == sorted_profs[cur_id] ? h_ltgray : c_ltgray);
            } else {
                col = (sorted_profs[i] == sorted_profs[cur_id] ? hilite(COL_SKILL_USED) : COL_SKILL_USED);
            }
            // Use gender neutral name if it has one, prevents cluttering
            // the list with "female X", "female Y", "female Z", ...
            std::string name;
            if (!sorted_profs[i]->name().empty()) {
                name = sorted_profs[i]->name();
            } else {
                name = sorted_profs[i]->gender_appropriate_name(u->male);
            }
            mvwprintz(w, 5 + i - iStartPos, 2, col, "%s", name.c_str());
        }

        std::vector<std::string> prof_items = sorted_profs[cur_id]->items();
        std::vector<std::string> prof_gender_items;
        if (u->male) {
            prof_gender_items = sorted_profs[cur_id]->items_male();
        } else {
            prof_gender_items = sorted_profs[cur_id]->items_female();
        }
        prof_items.insert( prof_items.end(), prof_gender_items.begin(), prof_gender_items.end() );
        int line_offset = 1;
        werase(w_items);
        mvwprintz(w_items, 0, 0, COL_HEADER, _("Profession items:"));
        for (size_t i = 0; i < prof_items.size(); i++) {
            itype *it = item_controller->find_template(prof_items[i]);
            wprintz(w_items, c_ltgray, _("\n"));
            line_offset += fold_and_print(w_items, i + line_offset, 0, getmaxx(w_items), c_ltgray,
                             it->nname(1)) - 1;
        }

        werase(w_skills);
        profession::StartingSkillList prof_skills = sorted_profs[cur_id]->skills();
        mvwprintz(w_skills, 0, 0, COL_HEADER, _("Profession skills:\n"));
        if (!prof_skills.empty()) {
            for (size_t i = 0; i < prof_skills.size(); i++) {
                Skill *skill = Skill::skill(prof_skills[i].first);
                if (skill == NULL) {
                    continue;  // skip unrecognized skills.
                }
                wprintz(w_skills, c_ltgray, _("%1$s (%2$d)\n"),
                        skill->name().c_str(), prof_skills[i].second);
            }
        } else {
            wprintz(w_skills, c_ltgray, _("None"));
        }

        werase(w_addictions);
        std::vector<addiction> prof_addictions = sorted_profs[cur_id]->addictions();
        if (!prof_addictions.empty()) {
            mvwprintz(w_addictions, 0, 0, COL_HEADER, _("Addictions:"));
            int add_y = 1;
            for (size_t i = 0; i < prof_addictions.size(); i++) {
                add_y += fold_and_print(w_addictions, i + add_y, 0, getmaxx(w_addictions), c_ltgray,
                                        _("%1$s (%2$d)"), addiction_name(prof_addictions[i]).c_str(),
                                        prof_addictions[i].intensity) - 1;
            }
        }

        werase(w_genderswap);
        mvwprintz(w_genderswap, 0, 0, c_magenta, _("Press %1$s to switch to %2$s."),
                    ctxt.get_desc("CHANGE_GENDER").c_str(),
                    sorted_profs[cur_id]->gender_appropriate_name(!u->male).c_str());

        //Draw Scrollbar
        draw_scrollbar(w, cur_id, iContentHeight, profession::count(), 5);

        wrefresh(w);
        wrefresh(w_description);
        wrefresh(w_items);
        wrefresh(w_skills);
        wrefresh(w_addictions);
        wrefresh(w_genderswap);

        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
                cur_id++;
                if (cur_id > profession::count() - 1) {
                    cur_id = 0;
                }
        } else if (action == "UP") {
                cur_id--;
                if (cur_id < 0) {
                    cur_id = profession::count() - 1;
                }
        } else if (action == "CONFIRM") {
                u->prof = profession::prof(sorted_profs[cur_id]->ident()); // we've got a const*
                points -= netPointCost;
        } else if (action == "CHANGE_GENDER") {
                u->male = !u->male;
        } else if (action == "PREV_TAB") {
                retval = -1;
        } else if (action == "NEXT_TAB") {
                retval = 1;
        }
    } while (retval == 0);

    delwin(w_description);
    delwin(w_items);
    delwin(w_skills);
    delwin(w_addictions);
    delwin(w_genderswap);
    return retval;
}

inline bool skill_display_sort(const Skill *a, const Skill *b)
{
    return a->name() < b->name();
}

int set_skills(WINDOW *w, player *u, int &points)
{
    draw_tabs(w, _("SKILLS"));
    const int iContentHeight = FULL_SCREEN_HEIGHT - 6;
    const int iHalf = iContentHeight / 2;
    WINDOW *w_description = newwin(iContentHeight, FULL_SCREEN_WIDTH - 35,
                                   5 + getbegy(w), 31 + getbegx(w));

    std::vector<Skill *> sorted_skills = Skill::skills;
    std::sort(sorted_skills.begin(), sorted_skills.end(), skill_display_sort);
    const int num_skills = Skill::skills.size();
    int cur_pos = 0;
    Skill *currentSkill = sorted_skills[cur_pos];

    input_context ctxt("NEW_CHAR_SKILLS");
    ctxt.register_cardinal();
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("HELP_KEYBINDINGS");

    do {
        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%4d "), points);
        // Clear the bottom of the screen.
        werase(w_description);
        mvwprintz(w, 3, 31, c_ltgray, "                                              ");
        int cost = std::max(1, (u->skillLevel(currentSkill) + 1) / 2);
        mvwprintz(w, 3, 31, points >= cost ? COL_SKILL_USED : c_ltred,
                  ngettext("Upgrading %s costs %d point", "Upgrading %s costs %d points", cost),
                  currentSkill->name().c_str(), cost);
        fold_and_print(w_description, 0, 0, getmaxx(w_description), COL_SKILL_USED,
                       currentSkill->description());

        int first_i, end_i, base_y;
        if (cur_pos < iHalf) {
            first_i = 0;
            end_i = iContentHeight;
            base_y = 5;
        } else if (cur_pos > num_skills - iContentHeight + iHalf) {
            first_i = num_skills - iContentHeight;
            end_i = num_skills;
            base_y = FULL_SCREEN_HEIGHT - 1 - num_skills;
        } else {
            first_i = cur_pos - iHalf;
            end_i = cur_pos + iContentHeight - iHalf;
            base_y = 5 + iHalf - cur_pos;
        }
        for (int i = first_i; i < end_i; ++i) {
            Skill *thisSkill = sorted_skills[i];
            // Clear the line
            mvwprintz(w, base_y + i, 2, c_ltgray, "                            ");
            if (u->skillLevel(thisSkill) == 0) {
                mvwprintz(w, base_y + i, 2,
                          (i == cur_pos ? h_ltgray : c_ltgray), thisSkill->name().c_str());
            } else {
                mvwprintz(w, base_y + i, 2,
                          (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED), _("%s"),
                          thisSkill->name().c_str());
                wprintz(w, (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
                        " (%d)", int(u->skillLevel(thisSkill)));
            }
            profession::StartingSkillList prof_skills = u->prof->skills();//profession skills
            for (size_t k = 0; k < prof_skills.size(); k++) {
                Skill *skill = Skill::skill(prof_skills[k].first);
                if (skill == NULL) {
                    continue;  // skip unrecognized skills.
                }
                if (skill->ident() == thisSkill->ident()){
                    wprintz(w, (i == cur_pos ? h_white : c_white),
                            " (+%d)", int(prof_skills[k].second));
                    break;
                }
            }
        }

        //Draw Scrollbar
        draw_scrollbar(w, cur_pos, iContentHeight, num_skills, 5);

        wrefresh(w);
        wrefresh(w_description);
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            cur_pos++;
            if (cur_pos >= num_skills) {
                cur_pos = 0;
            }
            currentSkill = sorted_skills[cur_pos];
        } else if (action == "UP") {
            cur_pos--;
            if (cur_pos < 0) {
                cur_pos = num_skills - 1;
            }
            currentSkill = sorted_skills[cur_pos];
        } else if (action == "LEFT") {
            SkillLevel& level = u->skillLevel(currentSkill);
            if (level) {
                if (level == 2) {  // lower 2->0 for 1 point
                    level.level(0);
                    points += 1;
                } else {
                    level.level(level - 1);
                    points += (level + 1) / 2;
                }
            }
        } else if (action == "RIGHT") {
            SkillLevel& level = u->skillLevel(currentSkill);
            if (level <= 19) {
                if (level == 0) {  // raise 0->2 for 1 point
                    level.level(2);
                    points -= 1;
                } else {
                    points -= (level + 1) / 2;
                    level.level(level + 1);
                }
            }
        } else if (action == "PREV_TAB") {
            delwin(w_description);
            return -1;
        } else if (action == "NEXT_TAB") {
            delwin(w_description);
            return 1;
        }
    } while (true);
}

inline bool skill_description_sort(const std::pair<Skill *, int> &a, const std::pair<Skill *, int> &b)
{
    int levelA = a.second;
    int levelB = b.second;
    return levelA > levelB || (levelA == levelB && a.first->name() < b.first->name());
}

int set_description(WINDOW *w, player *u, character_type type, int &points)
{
    if (PLTYPE_NOW == type) {
        return 1;
    }

    draw_tabs(w, _("DESCRIPTION"));

    WINDOW *w_name = newwin(2, 42, getbegy(w) + 6, getbegx(w) + 2);
    WINDOW *w_gender = newwin(2, 32, getbegy(w) + 6, getbegx(w) + 47);
    WINDOW *w_location = newwin(1, 76, getbegy(w) + 8, getbegx(w) + 2);
    WINDOW *w_stats = newwin(6, 16, getbegy(w) + 10, getbegx(w) + 2);
    WINDOW *w_traits = newwin(13, 24, getbegy(w) + 10, getbegx(w) + 24);
    WINDOW *w_profession = newwin(1, 32, getbegy(w) + 10, getbegx(w) + 47);
    WINDOW *w_skills = newwin(9, 24, getbegy(w) + 12, getbegx(w) + 47);
    WINDOW *w_guide = newwin(2, FULL_SCREEN_WIDTH - 4, getbegy(w) + 21, getbegx(w) + 2);

    mvwprintz(w, 3, 2, c_ltgray, _("Points left:%4d "), points);

    const unsigned namebar_pos = 1 + utf8_width(_("Name:"));
    unsigned male_pos = 1 + utf8_width(_("Gender:"));
    unsigned female_pos = 2 + male_pos + utf8_width(_("Male"));
    bool redraw = true;

    input_context ctxt("NEW_CHAR_DESCRIPTION");
    ctxt.register_action("SAVE_TEMPLATE");
    ctxt.register_action("PICK_RANDOM_NAME");
    ctxt.register_action("CHANGE_GENDER");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("CHOOSE_LOCATION");
    ctxt.register_action("ANY_INPUT");

    uimenu select_location;
    select_location.text = _("Select a starting location.");
    int offset = 0;
    for( location_map::iterator loc = start_location::begin();
         loc != start_location::end(); ++loc, ++offset ) {
        select_location.entries.push_back( uimenu_entry( _( loc->second.name().c_str() ) ) );
        if( loc->second.ident() == u->start_location ) {
            select_location.selected = offset;
        }
    }
    select_location.setup();
    if(MAP_SHARING::isSharing()) {
        u->name = MAP_SHARING::getUsername();  // set the current username as default character name
    }
    do {
        if (redraw) {
            //Draw the line between editable and non-editable stuff.
            for (int i = 0; i < getmaxx(w); ++i) {
                if (i == 0) {
                    mvwputch(w, 9, i, BORDER_COLOR, LINE_XXXO);
                } else if (i == getmaxx(w) - 1) {
                    wputch(w, BORDER_COLOR, LINE_XOXX);
                } else {
                    wputch(w, BORDER_COLOR, LINE_OXOX);
                }
            }
            wrefresh(w);

            std::vector<std::string> vStatNames;
            mvwprintz(w_stats, 0, 0, COL_HEADER, _("Stats:"));
            vStatNames.push_back(_("Strength:"));
            vStatNames.push_back(_("Dexterity:"));
            vStatNames.push_back(_("Intelligence:"));
            vStatNames.push_back(_("Perception:"));
            int pos = 0;
            for (size_t i = 0; i < vStatNames.size(); i++) {
                pos = (utf8_width(vStatNames[i].c_str()) > pos ?
                       utf8_width(vStatNames[i].c_str()) : pos);
                mvwprintz(w_stats, i + 1, 0, c_ltgray, vStatNames[i].c_str());
            }
            mvwprintz(w_stats, 1, pos + 1, c_ltgray, "%2d", u->str_max);
            mvwprintz(w_stats, 2, pos + 1, c_ltgray, "%2d", u->dex_max);
            mvwprintz(w_stats, 3, pos + 1, c_ltgray, "%2d", u->int_max);
            mvwprintz(w_stats, 4, pos + 1, c_ltgray, "%2d", u->per_max);
            wrefresh(w_stats);

            mvwprintz(w_traits, 0, 0, COL_HEADER, _("Traits: "));
            std::unordered_set<std::string> current_traits = u->get_traits();
            if (current_traits.empty()) {
                wprintz(w_traits, c_ltred, _("None!"));
            } else {
                for (std::unordered_set<std::string>::iterator i = current_traits.begin();
                     i != current_traits.end(); ++i) {
                    wprintz(w_traits, c_ltgray, "\n");
                    wprintz(w_traits, (traits[*i].points > 0) ? c_ltgreen : c_ltred,
                            traits[*i].name.c_str());
                }
            }
            wrefresh(w_traits);

            mvwprintz(w_skills, 0, 0, COL_HEADER, _("Skills:"));
            std::vector<Skill*> skillslist;

            std::vector<std::pair<Skill *, int> > sorted;
            int num_skills = Skill::skills.size();
            for (int i = 0; i < num_skills; i++) {
                Skill *s = Skill::skills[i];
                SkillLevel &sl = u->skillLevel(s);
                sorted.push_back(std::pair<Skill *, int>(s, sl.level() * 100 + sl.exercise()));
            }
            std::sort(sorted.begin(), sorted.end(), skill_description_sort);
            for (std::vector<std::pair<Skill *, int> >::iterator i = sorted.begin();
                 i != sorted.end(); ++i) {
                skillslist.push_back((*i).first);
            }

            int line = 1;
            bool has_skills = false;
            profession::StartingSkillList list_skills=u->prof->skills();
            for (std::vector<Skill*>::iterator aSkill = skillslist.begin();
                 aSkill != skillslist.end(); ++aSkill) {
                int level = int(u->skillLevel(*aSkill));
                profession::StartingSkillList::iterator i=list_skills.begin();
                while (i != list_skills.end()) {
                    if (i->first == (*aSkill)->ident()) {
                        level += i->second;
                        break;
                    }
                    ++i;
                }

                if (level > 0) {
                    mvwprintz(w_skills, line, 0, c_ltgray, "%s", ((*aSkill)->name() + ":").c_str());
                    mvwprintz(w_skills, line, 17, c_ltgray, "%-2d", (int)level);
                    line++;
                    has_skills = true;
                }
            }
            if (!has_skills) {
                mvwprintz(w_skills, 0, utf8_width(_("Skills:")) + 1, c_ltred, _("None!"));
            } else if (line > 10) {
                mvwprintz(w_skills, 0, utf8_width(_("Skills:")) + 1, c_ltgray, _("(Top 8)"));
            }
            wrefresh(w_skills);

            mvwprintz(w_guide, 0, 0, c_green, _("Press %s to finish character creation or %s to go back and make revisions."),
                      ctxt.get_desc("NEXT_TAB").c_str(),
                      ctxt.get_desc("PREV_TAB").c_str());
            mvwprintz(w_guide, 1, 0, c_green, _("Press %s to save a template of this character."),
                      ctxt.get_desc("SAVE_TEMPLATE").c_str());
            wrefresh(w_guide);

            redraw = false;
        }

        //We draw this stuff every loop because this is user-editable
        mvwprintz(w_name, 0, 0, c_ltgray, _("Name:"));
        mvwprintz(w_name, 0, namebar_pos, c_ltgray, "_______________________________");
        mvwprintz(w_name, 0, namebar_pos, c_ltgray, "%s", u->name.c_str());
        wprintz(w_name, h_ltgray, "_");

        if(!MAP_SHARING::isSharing()) { // no random names when sharing maps
            mvwprintz(w_name, 1, 0, c_ltgray, _("Press %s to pick a random name."),
                      ctxt.get_desc("PICK_RANDOM_NAME").c_str());
        }
        wrefresh(w_name);

        mvwprintz(w_gender, 0, 0, c_ltgray, _("Gender:"));
        mvwprintz(w_gender, 0, male_pos, (u->male ? c_ltred : c_ltgray), _("Male"));
        mvwprintz(w_gender, 0, female_pos, (u->male ? c_ltgray : c_ltred), _("Female"));
        mvwprintz(w_gender, 1, 0, c_ltgray, _("Press %s to switch gender"),
                      ctxt.get_desc("CHANGE_GENDER").c_str());
        wrefresh(w_gender);

        const std::string location_prompt = string_format(_("Press %s to select location."),
                                                          ctxt.get_desc("CHOOSE_LOCATION").c_str() );
        const int prompt_offset = utf8_width( location_prompt.c_str() );
        werase(w_location);
        mvwprintz( w_location, 0, 0, c_ltgray, location_prompt.c_str() );
        mvwprintz( w_location, 0, prompt_offset + 1, c_ltgray, _("Starting location:") );
        mvwprintz( w_location, 0, prompt_offset + utf8_width(_("Starting location:")) + 2,
                   c_ltgray, _(select_location.entries[select_location.selected].txt.c_str()) );
        wrefresh(w_location);

        werase(w_profession);
        mvwprintz(w_profession, 0, 0, COL_HEADER, _("Profession: "));
        wprintz (w_profession, c_ltgray, u->prof->gender_appropriate_name(u->male).c_str());
        wrefresh(w_profession);

        const std::string action = ctxt.handle_input();

        if (action == "NEXT_TAB") {
            if (points < 0) {
                popup(_("Too many points allocated, change some features and try again."));
                redraw = true;
                continue;
            } else if (points > 0 &&
                       !query_yn(_("Remaining points will be discarded, are you sure you want to proceed?"))) {
                redraw = true;
                continue;
            } else if (u->name.empty()) {
                mvwprintz(w_name, 0, namebar_pos, h_ltgray, _("______NO NAME ENTERED!!!______"));
                wrefresh(w_name);
                if (!query_yn(_("Are you SURE you're finished? Your name will be randomly generated."))) {
                    redraw = true;
                    continue;
                } else {
                    u->pick_name();
                    delwin(w_name);
                    delwin(w_gender);
                    delwin(w_stats);
                    delwin(w_traits);
                    delwin(w_profession);
                    delwin(w_skills);
                    delwin(w_guide);
                    return 1;
                }
            } else if (query_yn(_("Are you SURE you're finished?"))) {
                delwin(w_name);
                delwin(w_gender);
                delwin(w_stats);
                delwin(w_traits);
                delwin(w_profession);
                delwin(w_skills);
                delwin(w_guide);
                return 1;
            } else {
                redraw = true;
                continue;
            }
        } else if (action == "PREV_TAB") {
            delwin(w_name);
            delwin(w_gender);
            delwin(w_stats);
            delwin(w_traits);
            delwin(w_profession);
            delwin(w_skills);
            delwin(w_guide);
            return -1;
        } else if (action == "SAVE_TEMPLATE") {
            if (points > 0) {
                if(query_yn(_("You are attempting to save a template with unused points. "
                              "Any unspent points will be lost, are you sure you want to proceed?"))) {
                    save_template(u);
                }
            } else if (points < 0) {
                popup(_("You cannot save a template with negative unused points"));
            } else {
                save_template(u);
            }
            redraw = true;
        } else if (action == "PICK_RANDOM_NAME") {
            if(!MAP_SHARING::isSharing()){ // Don't allow random names when sharing maps. We don't need to check at the top as you won't be able to edit the name
                u->pick_name();
            }
        } else if (action == "CHANGE_GENDER") {
            u->male = !u->male;
        } else if ( action == "CHOOSE_LOCATION" ){
            select_location.redraw();
            select_location.query();
            for( location_map::iterator loc = start_location::begin();
                 loc != start_location::end(); ++loc ) {
                if( 0 == strcmp( _( loc->second.name().c_str() ),
                                 select_location.entries[ select_location.selected ].txt.c_str() ) ) {
                    u->start_location = loc->second.ident();
                }
            }
            werase(select_location.window);
            select_location.refresh();
            redraw = true;
        } else if (action == "ANY_INPUT" && !MAP_SHARING::isSharing()) {  // Don't edit names when sharing maps
            const long ch = ctxt.get_raw_input().get_first_input();
            if ((ch == KEY_BACKSPACE || ch == 127) && !u->name.empty()) {
                //erase utf8 character TODO: make a function
                while(!u->name.empty() && ((unsigned char)u->name[u->name.size() - 1]) >= 128 &&
                        ((unsigned char)u->name[(int)u->name.size() - 1]) <= 191) {
                    u->name.erase(u->name.size() - 1);
                }
                u->name.erase(u->name.size() - 1);
            } else if (is_char_allowed(ch) && utf8_width(u->name.c_str()) < 30) {
                u->name.push_back(ch);
            } else if(ch == KEY_F(2)) {
                std::string tmp = get_input_string_from_file();
                int tmplen = utf8_width(tmp.c_str());
                if(tmplen > 0 && tmplen + utf8_width(u->name.c_str()) < 30) {
                    u->name.append(tmp);
                }
            }
            //experimental unicode input
            else if(ch > 127 && !MAP_SHARING::isSharing()) { //Don't edit name when sharing
                std::string tmp = utf32_to_utf8(ch);
                int tmplen = utf8_width(tmp.c_str());
                if(tmplen > 0 && tmplen + utf8_width(u->name.c_str()) < 30) {
                    u->name.append(tmp);
                }
            }
        }
    } while (true);
}

std::unordered_set<std::string> player::get_traits() const
{
    return my_traits;
}

std::string player::random_good_trait()
{
    std::vector<std::string> vTraitsGood;

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        if (iter->second.startingtrait && iter->second.points >= 0) {
            vTraitsGood.push_back(iter->first);
        }
    }

    return vTraitsGood[rng(0, vTraitsGood.size() - 1)];
}

std::string player::random_bad_trait()
{
    std::vector<std::string> vTraitsBad;

    for (std::map<std::string, trait>::iterator iter = traits.begin(); iter != traits.end(); ++iter) {
        if (iter->second.startingtrait && iter->second.points < 0) {
            vTraitsBad.push_back(iter->first);
        }
    }

    return vTraitsBad[rng(0, vTraitsBad.size() - 1)];
}

Skill *random_skill()
{
    return Skill::skill(rng(0, Skill::skill_count() - 1));
}

void save_template(player *u)
{
    std::string title = _("Name of template:");
    std::string name = string_input_popup(title, FULL_SCREEN_WIDTH - utf8_width(title.c_str()) - 8);
    if (name.length() == 0) {
        return;
    }
    std::string playerfile = FILENAMES["templatedir"] + name + ".template";

    std::ofstream fout;
    fout.open(playerfile.c_str());
    fout << u->save_info();
    fout.close();
    if (fout.fail()) {
        popup(_("Failed to write template to %s"), playerfile.c_str());
    }
}
