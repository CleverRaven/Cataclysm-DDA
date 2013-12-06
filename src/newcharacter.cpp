#include "player.h"
#include "profession.h"
#include "item_factory.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "keypress.h"
#include "game.h"
#include "options.h"
#include "catacharset.h"
#include "debug.h"
#include "char_validity_check.h"
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
#define COL_NOTE_MAJOR      c_green   // Important note
#define COL_NOTE_MINOR      c_ltgray  // Just regular note

#define HIGH_STAT 14 // The point after which stats cost double

#define NEWCHAR_TAB_MAX 4 // The ID of the rightmost tab

void draw_tabs(WINDOW *w, std::string sTab);

int set_stats(WINDOW *w, game *g, player *u, character_type type, int &points);
int set_traits(WINDOW *w, game *g, player *u, character_type type, int &points,
               int max_trait_points);
int set_profession(WINDOW *w, game *g, player *u, character_type type, int &points);
int set_skills(WINDOW *w, game *g, player *u, character_type type, int &points);
int set_description(WINDOW *w, game *g, player *u, character_type type, int &points);

int random_skill();

int calc_HP(int strength, bool tough);

void save_template(player *u);

bool player::create(game *g, character_type type, std::string tempname)
{
    weapon = item(itypes["null"], 0);

    g->u.prof = profession::generic();

    WINDOW *w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    int tab = 0, points = 38, max_trait_points = 12;
    if (type != PLTYPE_CUSTOM) {
        switch (type) {
            case PLTYPE_NOW:
                g->u.male = (rng(1, 100) > 50);
                g->u.pick_name();
            case PLTYPE_RANDOM: {
                str_max = rng(6, 12);
                dex_max = rng(6, 12);
                int_max = rng(6, 12);
                per_max = rng(6, 12);
                points = points - str_max - dex_max - int_max - per_max;
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
                while (points > 0) {
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
                                    }
                                    break;
                                case 2:
                                    if (dex_max < HIGH_STAT) {
                                        dex_max++;
                                        points--;
                                    }
                                    break;
                                case 3:
                                    if (int_max < HIGH_STAT) {
                                        int_max++;
                                        points--;
                                    }
                                    break;
                                case 4:
                                    if (per_max < HIGH_STAT) {
                                        per_max++;
                                        points--;
                                    }
                                    break;
                            }
                            break;
                        case 6:
                        case 7:
                        case 8:
                        case 9:
                            rn = random_skill();

                            Skill *aSkill = Skill::skill(rn);
                            int level = skillLevel(aSkill);

                            if (level < points) {
                                points -= level + 1;
                                skillLevel(aSkill).level(level + 2);
                            }
                            break;
                    }
                }
            }
            break;
            case PLTYPE_TEMPLATE: {
                std::ifstream fin;
                std::stringstream filename;
                filename << "data/" << tempname << ".template";
                fin.open(filename.str().c_str());
                if (!fin.is_open()) {
                    debugmsg("Couldn't open %s!", filename.str().c_str());
                    return false;
                }
                std::string(data);
                getline(fin, data);
                load_info(g, data);
                points = 0;
            }
            break;
        }
        tab = NEWCHAR_TAB_MAX;
    } else {
        points = OPTIONS["INITIAL_POINTS"];
    }
    max_trait_points = OPTIONS["MAX_TRAIT_POINTS"];

    do {
        werase(w);
        wrefresh(w);
        switch (tab) {
            case 0:
                tab += set_stats      (w, g, this, type, points);
                break;
            case 1:
                tab += set_traits     (w, g, this, type, points, max_trait_points);
                break;
            case 2:
                tab += set_profession (w, g, this, type, points);
                break;
            case 3:
                tab += set_skills     (w, g, this, type, points);
                break;
            case 4:
                tab += set_description(w, g, this, type, points);
                break;
        }
    } while (tab >= 0 && tab <= NEWCHAR_TAB_MAX);
    delwin(w);

    if (tab < 0) {
        return false;
    }

    // Character is finalized.  Now just set up HP, &c
    for (int i = 0; i < num_hp_parts; i++) {
        hp_max[i] = calc_HP(str_max, has_trait("TOUGH"));
        hp_cur[i] = hp_max[i];
    }
    if (has_trait("FRAIL")) {
        for (int i = 0; i < num_hp_parts; i++) {
            hp_max[i] = int(hp_max[i] * .25);
            hp_cur[i] = hp_max[i];
        }
    }
    if (has_trait("GLASSJAW")) {
        hp_max[hp_head] = int(hp_max[hp_head] * .80);
        hp_cur[hp_head] = hp_max[hp_head];
    }
    if (has_trait("SMELLY")) {
        scent = 800;
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
                popup(martialarts[ma_type].description.c_str());
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }
    if (has_trait("MARTIAL_ARTS2")) {
        matype_id ma_type;
        do {
            int choice = (PLTYPE_NOW == type) ? rng(1, 5) :
                         menu(false, _("Pick your style:"), _("Krav Maga"), _("Muay Thai"), _("Ninjutsu"),
                              _("Capoeira"), _("Zui Quan"), NULL);
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
                popup(martialarts[ma_type].description.c_str());
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
                popup(martialarts[ma_type].description.c_str());
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }
    if (has_trait("MARTIAL_ARTS4")) {
        matype_id ma_type;
        do {
            int choice = (PLTYPE_NOW == type) ? rng(1, 5) :
                         menu(false, _("Pick your style:"), _("Centipede"), _("Viper"), _("Scorpion"),
                              _("Lizard"), _("Toad"), NULL);
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
                popup(martialarts[ma_type].description.c_str());
            }
        } while (PLTYPE_NOW != type && !query_yn(_("Use this style?")));
        ma_styles.push_back(ma_type);
        style_selected = ma_type;
    }


    ret_null = item(itypes["null"], 0);
    weapon = ret_null;


    item tmp; //gets used several times

    std::vector<std::string> prof_items = g->u.prof->items();
    for (std::vector<std::string>::const_iterator iter = prof_items.begin();
         iter != prof_items.end(); ++iter) {
        tmp = item(item_controller->find_template(*iter), 0);
        tmp = tmp.in_its_container(&(itypes));
        if(tmp.is_armor()) {
            if(tmp.has_flag("VARSIZE")) {
                tmp.item_tags.insert("FIT");
            }
            // If wearing an item fails we fail silently.
            wear_item(g, &tmp, false);
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

    // Those who are both near-sighted and far-sighted start with bifocal glasses.
    if (has_trait("HYPEROPIC") && has_trait("MYOPIC")) {
        tmp = item(itypes["glasses_bifocal"], 0);
        inv.push_back(tmp);
    }
    // The near-sighted start with eyeglasses.
    else if (has_trait("MYOPIC")) {
        tmp = item(itypes["glasses_eye"], 0);
        inv.push_back(tmp);
    }
    // The far-sighted start with reading glasses.
    else if (has_trait("HYPEROPIC")) {
        tmp = item(itypes["glasses_reading"], 0);
        inv.push_back(tmp);
    }

    // Likewise, the asthmatic start with their medication.
    if (has_trait("ASTHMA")) {
        tmp = item(itypes["inhaler"], 0);
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
        mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 4, i, c_ltgray, LINE_OXOX);
        mvwputch(w, FULL_SCREEN_HEIGHT - 1, i, c_ltgray, LINE_OXOX);

        if (i > 2 && i < FULL_SCREEN_HEIGHT - 1) {
            mvwputch(w, i, 0, c_ltgray, LINE_XOXO);
            mvwputch(w, i, FULL_SCREEN_WIDTH - 1, c_ltgray, LINE_XOXO);
        }
    }
    std::vector<std::string> tab_captions;
    tab_captions.push_back(_("STATS"));
    tab_captions.push_back(_("TRAITS"));
    tab_captions.push_back(_("PROFESSION"));
    tab_captions.push_back(_("SKILLS"));
    tab_captions.push_back(_("DESCRIPTION"));
    int tab_pos[6];   //this is actually tab_captions.size() + 1
    tab_pos[0] = 2;
    for (int pos = 1; pos < 6; pos++) {
        tab_pos[pos] = tab_pos[pos-1] + utf8_width(tab_captions[pos-1].c_str());
    }
    int space = (FULL_SCREEN_WIDTH - tab_pos[5]) / 4 - 3;
    for (size_t i = 0; i < tab_captions.size(); i++) {
        draw_tab(w, tab_pos[i] + space * i, tab_captions[i].c_str(), (sTab == tab_captions[i]));
    }

    mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 2, FULL_SCREEN_WIDTH - 1, c_ltgray, LINE_OOXX); // ^|

    mvwputch(w, 4, 0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w, 4, FULL_SCREEN_WIDTH - 1, c_ltgray, LINE_XOXX); // -|

    mvwputch(w, FULL_SCREEN_HEIGHT - 1, 0, c_ltgray, LINE_XXOO); // |_
    mvwputch(w, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, c_ltgray, LINE_XOOX); // _|
}

int set_stats(WINDOW *w, game *g, player *u, character_type type, int &points)
{
    unsigned char sel = 1;
    const int iSecondColumn = 27;
    char ch;
    int read_spd;

    draw_tabs(w, _("STATS"));

    mvwprintz(w, 16, 2, COL_NOTE_MINOR, _("j/k, 8/2, or up/down arrows to select a statistic."));
    mvwprintz(w, 17, 2, COL_NOTE_MINOR, _("l, 6, or right arrow to increase the statistic."));
    mvwprintz(w, 18, 2, COL_NOTE_MINOR, _("h, 4, or left arrow to decrease the statistic."));
    mvwprintz(w, FULL_SCREEN_HEIGHT - 3, 2, COL_NOTE_MAJOR, _("> Takes you to the next tab."));
    mvwprintz(w, FULL_SCREEN_HEIGHT - 2, 2, COL_NOTE_MAJOR, _("< Returns you to the main menu."));

    const char clear[] = "                                                ";

    do {
        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%3d"), points);
        mvwprintz(w, 3, iSecondColumn, c_black, clear);
        for (int i = 6; i < 13; i++) {
            mvwprintz(w, i, iSecondColumn, c_black, clear);
        }
        mvwprintz(w, 6,  2, c_ltgray, _("Strength:"));
        mvwprintz(w, 6,  16, c_ltgray, "%2d", u->str_max);
        mvwprintz(w, 7,  2, c_ltgray, _("Dexterity:"));
        mvwprintz(w, 7,  16, c_ltgray, "%2d", u->dex_max);
        mvwprintz(w, 8,  2, c_ltgray, _("Intelligence:"));
        mvwprintz(w, 8,  16, c_ltgray, "%2d", u->int_max);
        mvwprintz(w, 9,  2, c_ltgray, _("Perception:"));
        mvwprintz(w, 9,  16, c_ltgray, "%2d", u->per_max);

        switch (sel) {
            case 1:
                mvwprintz(w, 6, 2, COL_STAT_ACT, _("Strength:"));
                mvwprintz(w, 6, 16, COL_STAT_ACT, "%2d", u->str_max);
                if (u->str_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Str further costs 2 points."));
                }
                mvwprintz(w, 6, iSecondColumn, COL_STAT_NEUTRAL, _("Base HP: %d"),
                          calc_HP(u->str_max, u->has_trait("TOUGH")));
                mvwprintz(w, 7, iSecondColumn, COL_STAT_NEUTRAL, _("Carry weight: %.1f %s"),
                          u->convert_weight(u->weight_capacity(false)),
                          OPTIONS["USE_METRIC_WEIGHTS"] == "kg" ? _("kg") : _("lbs"));
                mvwprintz(w, 8, iSecondColumn, COL_STAT_NEUTRAL, _("Melee damage: %d"),
                          u->base_damage(false));
                fold_and_print(w, 10, iSecondColumn, FULL_SCREEN_WIDTH - iSecondColumn - 2, COL_STAT_NEUTRAL,
                               _("Strength also makes you more resistant to many diseases and poisons, and makes actions which require brute force more effective."));
                break;

            case 2:
                mvwprintz(w, 7,  2, COL_STAT_ACT, _("Dexterity:"));
                mvwprintz(w, 7,  16, COL_STAT_ACT, "%2d", u->dex_max);
                if (u->dex_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Dex further costs 2 points."));
                }
                mvwprintz(w, 6, iSecondColumn, COL_STAT_BONUS, _("Melee to-hit bonus: +%d"),
                          u->base_to_hit(false));
                if (u->throw_dex_mod(false) <= 0) {
                    mvwprintz(w, 7, iSecondColumn, COL_STAT_BONUS, _("Throwing bonus: +%d"),
                              abs(u->throw_dex_mod(false)));
                } else {
                    mvwprintz(w, 7, iSecondColumn, COL_STAT_PENALTY, _("Throwing penalty: -%d"),
                              abs(u->throw_dex_mod(false)));
                }
                if (u->ranged_dex_mod(false) != 0) {
                    mvwprintz(w, 8, iSecondColumn, COL_STAT_PENALTY, _("Ranged penalty: -%d"),
                              abs(u->ranged_dex_mod(false)));
                }
                fold_and_print(w, 10, iSecondColumn, FULL_SCREEN_WIDTH - iSecondColumn - 2, COL_STAT_NEUTRAL,
                               _("Dexterity also enhances many actions which require finesse."));
                break;

            case 3:
                mvwprintz(w, 8,  2, COL_STAT_ACT, _("Intelligence:"));
                mvwprintz(w, 8,  16, COL_STAT_ACT, "%2d", u->int_max);
                if (u->int_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Int further costs 2 points."));
                }
                read_spd = u->read_speed(false);
                mvwprintz(w, 6, iSecondColumn, (read_spd == 100 ? COL_STAT_NEUTRAL :
                                                (read_spd < 100 ? COL_STAT_BONUS : COL_STAT_PENALTY)),
                          _("Read times: %d%%"), read_spd);
                mvwprintz(w, 7, iSecondColumn, COL_STAT_PENALTY, _("Skill rust: %d%%"),
                          u->rust_rate(false));
                fold_and_print(w, 9, iSecondColumn, FULL_SCREEN_WIDTH - iSecondColumn - 2, COL_STAT_NEUTRAL,
                               _("Intelligence is also used when crafting, installing bionics, and interacting with NPCs."));
                break;

            case 4:
                mvwprintz(w, 9,  2, COL_STAT_ACT, _("Perception:"));
                mvwprintz(w, 9,  16, COL_STAT_ACT, "%2d", u->per_max);
                if (u->per_max >= HIGH_STAT) {
                    mvwprintz(w, 3, iSecondColumn, c_ltred, _("Increasing Per further costs 2 points."));
                }
                if (u->ranged_per_mod(false) != 0) {
                    mvwprintz(w, 6, iSecondColumn, COL_STAT_PENALTY, _("Ranged penalty: -%d"),
                              abs(u->ranged_per_mod(false)));
                }
                fold_and_print(w, 8, iSecondColumn, FULL_SCREEN_WIDTH - iSecondColumn - 2, COL_STAT_NEUTRAL,
                               _("Perception is also used for detecting traps and other things of interest."));
                break;
        }

        wrefresh(w);
        ch = input();
        if ((ch == 'j' || ch == '2') && sel < 4) {
            sel++;
        }
        if ((ch == 'k' || ch == '8') && sel > 1) {
            sel--;
        }
        if (ch == 'h' || ch == '4') {
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
        }
        if ((ch == 'l' || ch == '6')) {
            if (sel == 1 && u->str_max < 20) {
                points--;
                if (u->str_max >= HIGH_STAT) {
                    points--;
                }
                u->str_max++;
            } else if (sel == 2 && u->dex_max < 20) {
                points--;
                if (u->dex_max >= HIGH_STAT) {
                    points--;
                }
                u->dex_max++;
            } else if (sel == 3 && u->int_max < 20) {
                points--;
                if (u->int_max >= HIGH_STAT) {
                    points--;
                }
                u->int_max++;
            } else if (sel == 4 && u->per_max < 20) {
                points--;
                if (u->per_max >= HIGH_STAT) {
                    points--;
                }
                u->per_max++;
            }
        }
        if (ch == '<' && query_yn(_("Return to main menu?"))) {
            return -1;
        }
        if (ch == '>') {
            return 1;
        }
    } while (true);
}

int set_traits(WINDOW *w, game *g, player *u, character_type type, int &points,
               int max_trait_points)
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

    const int iContentHeight = FULL_SCREEN_HEIGHT - 9;
    int iCurWorkingPage = 0;

    int iStartPos[2];
    iStartPos[0] = 0;
    iStartPos[1] = 0;

    int iCurrentLine[2];
    iCurrentLine[0] = 0;
    iCurrentLine[1] = 0;

    do {
        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%3d"), points);
        mvwprintz(w, 3, 18, c_ltgreen, "%2d/%d", num_good, max_trait_points);
        mvwprintz(w, 3, 25, c_ltred, "%2d/%d", num_bad, max_trait_points);

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
            for (int i = iStartPos[iCurrentPage]; i < vStartingTraits[iCurrentPage].size(); i++) {
                if (i >= iStartPos[iCurrentPage] && i < iStartPos[iCurrentPage] +
                    ((iContentHeight > vStartingTraits[iCurrentPage].size()) ?
                     vStartingTraits[iCurrentPage].size() : iContentHeight)) {
                    if (iCurrentLine[iCurrentPage] == i && iCurrentPage == iCurWorkingPage) {
                        mvwprintz(w,  3, 33, c_ltgray,
                                  "                                              ");
                        mvwprintz(w,  3, 33, col_tr, _("%s earns %d points"),
                                  traits[vStartingTraits[iCurrentPage][i]].name.c_str(),
                                  traits[vStartingTraits[iCurrentPage][i]].points * -1);
                        fold_and_print(w_description, 0, 0,
                                       FULL_SCREEN_WIDTH - 2, col_tr, "%s",
                                       traits[vStartingTraits[iCurrentPage][i]].description.c_str());
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
        switch (input()) {
            case 'h':
            case '4':
            case 'l':
            case '6':
            case '\t':
                if (iCurWorkingPage == 0) {
                    iCurWorkingPage = 1;
                } else {
                    iCurWorkingPage = 0;
                }
                wrefresh(w);
                break;
            case 'k':
            case '8':
                iCurrentLine[iCurWorkingPage]--;
                if (iCurrentLine[iCurWorkingPage] < 0) {
                    iCurrentLine[iCurWorkingPage] = vStartingTraits[iCurWorkingPage].size() - 1;
                }
                break;
            case 'j':
            case '2':
                iCurrentLine[iCurWorkingPage]++;
                if (iCurrentLine[iCurWorkingPage] >= vStartingTraits[iCurWorkingPage].size()) {
                    iCurrentLine[iCurWorkingPage] = 0;
                }
                break;
            case ' ':
            case '\n':
            case '5': {
                std::string cur_trait = vStartingTraits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
                if (u->has_trait(cur_trait)) {

                    u->toggle_trait(cur_trait);

                    // If turning off the trait violates a profession condition,
                    // turn it back on.
                    if(u->prof->can_pick(u, 0) != "YES") {
                        u->toggle_trait(cur_trait);
                        popup(_("Your profession of %s prevents you from removing this trait."),
                              u->prof->name().c_str());

                    } else {
                        points += traits[cur_trait].points;
                        if (iCurWorkingPage == 0) {
                            num_good -= traits[cur_trait].points;
                        } else {
                            num_bad -= traits[cur_trait].points;
                        }
                    }

                } else if(u->has_conflicting_trait(cur_trait)) {
                    popup(_("You already picked a conflicting trait!"));

                } else if (iCurWorkingPage == 0 && num_good + traits[cur_trait].points >
                           max_trait_points) {
                    popup(_("Sorry, but you can only take %d points of advantages."),
                          max_trait_points);

                } else if (iCurWorkingPage != 0 && num_bad + traits[cur_trait].points <
                           -max_trait_points) {
                    popup(_("Sorry, but you can only take %d points of disadvantages."),
                          max_trait_points);

                } else {
                    u->toggle_trait(cur_trait);

                    // If turning on the trait violates a profession condition,
                    // turn it back off.
                    if(u->prof->can_pick(u, 0) != "YES") {
                        u->toggle_trait(cur_trait);
                        popup(_("Your profession of %s prevents you from taking this trait."),
                              u->prof->name().c_str());

                    } else {
                        points -= traits[cur_trait].points;
                        if (iCurWorkingPage == 0) {
                            num_good += traits[cur_trait].points;
                        } else {
                            num_bad += traits[cur_trait].points;
                        }
                    }
                }
                break;
            }
            case '<':
                return -1;
            case '>':
                return 1;
        }
    } while (true);

    return 1;
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

    return a->name() < b->name();
}

int set_profession(WINDOW *w, game *g, player *u, character_type type, int &points)
{
    draw_tabs(w, _("PROFESSION"));

    WINDOW *w_description = newwin(3, FULL_SCREEN_WIDTH - 2, FULL_SCREEN_HEIGHT - 4 + getbegy(w),
                                   1 + getbegx(w));

    int cur_id = 0;
    int retval = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 9;
    int iStartPos = 0;

    WINDOW *w_items = newwin(iContentHeight, 22, 5 + getbegy(w), 21 + getbegx(w));
    WINDOW *w_skills = newwin(iContentHeight, 32, 5 + getbegy(w), 43+ getbegx(w));
    WINDOW *w_addictions = newwin(iContentHeight - 10, 32, 15 + getbegy(w), 43+ getbegx(w));

    std::vector<const profession *> sorted_profs;
    for (profmap::const_iterator iter = profession::begin(); iter != profession::end(); ++iter) {
        sorted_profs.push_back(&(iter->second));
    }

    // Sort professions by name.
    // profession_display_sort() keeps "unemployed" at the top.
    std::sort(sorted_profs.begin(), sorted_profs.end(), profession_display_sort);

    // Select the current profession, if possible.
    for (int i = 0; i < sorted_profs.size(); ++i) {
        if (sorted_profs[i]->ident() == u->prof->ident()) {
            cur_id = i;
            break;
        }
    }

    do {
        int netPointCost = sorted_profs[cur_id]->point_cost() - u->prof->point_cost();
        std::string can_pick = sorted_profs[cur_id]->can_pick(u, points);

        mvwprintz(w,  3, 2, c_ltgray, _("Points left:%3d"), points);
        // Clear the bottom of the screen.
        werase(w_description);
        mvwprintz(w,  3, 40, c_ltgray, "                                      ");
        if (can_pick == "YES") {
            mvwprintz(w,  3, 20, c_green, _("Profession %1$s costs %2$d points (net: %3$d)"),
                      sorted_profs[cur_id]->name().c_str(), sorted_profs[cur_id]->point_cost(),
                      netPointCost);
        } else if(can_pick == "INSUFFICIENT_POINTS") {
            mvwprintz(w,  3, 20, c_ltred, _("Profession %1$s costs %2$d points (net: %3$d)"),
                      sorted_profs[cur_id]->name().c_str(), sorted_profs[cur_id]->point_cost(),
                      netPointCost);
        }
        fold_and_print(w_description, 0, 0, FULL_SCREEN_WIDTH - 2, c_green,
                       sorted_profs[cur_id]->description().c_str());

        calcStartPos(iStartPos, cur_id, iContentHeight, profession::count());

        //Draw options
        for (int i = iStartPos;
             i < iStartPos + ((iContentHeight > profession::count()) ? profession::count() : iContentHeight);
             i++) {
            mvwprintz(w, 5 + i - iStartPos, 2, c_ltgray, "\
                                             "); // Clear the line

            if (u->prof != sorted_profs[i]) {
                mvwprintz(w, 5 + i - iStartPos, 2, (sorted_profs[i] == sorted_profs[cur_id] ? h_ltgray : c_ltgray),
                          sorted_profs[i]->name().c_str());
            } else {
                mvwprintz(w, 5 + i - iStartPos, 2,
                          (sorted_profs[i] == sorted_profs[cur_id] ?
                           hilite(COL_SKILL_USED) : COL_SKILL_USED),
                          sorted_profs[i]->name().c_str());
            }
        }

        std::vector<std::string>  pipo = sorted_profs[cur_id]->items();
        mvwprintz(w_items, 0, 0, c_ltgray, _("Profession items:"));
        for (int i = 0; i < iContentHeight; i++) {
            // clean
            mvwprintz(w_items, 1 + i, 0, c_ltgray, "                                        ");
            if (i < pipo.size()) {
                // dirty
                mvwprintz(w_items, 1 + i , 0, c_ltgray, itypes[pipo[i]]->name.c_str());
            }
        }
        profession::StartingSkillList prof_skills = sorted_profs[cur_id]->skills();
        mvwprintz(w_skills, 0, 0, c_ltgray, _("Profession skills:"));
        for (int i = 0; i < iContentHeight; i++) {
            // clean
            mvwprintz(w_skills, 1 + i, 0, c_ltgray, "                                                  ");
            if (i < prof_skills.size()) {
                Skill *skill = Skill::skill(prof_skills[i].first);
                if (skill == NULL) {
                    continue;  // skip unrecognized skills.
            	}
            	// dirty
                std::stringstream skill_listing;
                skill_listing << skill->name() << " (" << prof_skills[i].second << ")";
                mvwprintz(w_skills, 1 + i , 0, c_ltgray, skill_listing.str().c_str());
            }
        }
        std::vector<addiction> prof_addictions = sorted_profs[cur_id]->addictions();
        if(prof_addictions.size() > 0){
            mvwprintz(w_addictions, 0, 0, c_ltgray, _("Addictions:"));
            for (int i = 0; i < iContentHeight; i++) {
                // clean
                mvwprintz(w_addictions, 1 + i, 0, c_ltgray, "                                                  ");
                if (i < prof_addictions.size()) {
                    // dirty
                    std::stringstream addiction_listing;
                    addiction_listing << addiction_name(prof_addictions[i]) << " (" << prof_addictions[i].intensity << ")";
                    mvwprintz(w_addictions, 1 + i , 0, c_ltgray, addiction_listing.str().c_str());
                }
            }
        }

        //Draw Scrollbar
        draw_scrollbar(w, cur_id, iContentHeight, profession::count(), 5);

        wrefresh(w);
        wrefresh(w_description);
        wrefresh(w_items);
        wrefresh(w_skills);
        wrefresh(w_addictions);
        switch (input()) {
            case 'j':
            case '2':
                cur_id++;
                if (cur_id > profession::count() - 1) {
                    cur_id = 0;
                }
                break;

            case 'k':
            case '8':
                cur_id--;
                if (cur_id < 0) {
                    cur_id = profession::count() - 1;
                }
                break;

            case '\n':
            case '5':
                u->prof = profession::prof(sorted_profs[cur_id]->ident()); // we've got a const*
                points -= netPointCost;
                break;

            case '<':
                retval = -1;
                break;

            case '>':
                retval = 1;
                break;
        }
    } while (retval == 0);

    return retval;
}

inline bool skill_display_sort(const Skill *a, const Skill *b)
{
    return a->name() < b->name();
}

int set_skills(WINDOW *w, game *g, player *u, character_type type, int &points)
{
    draw_tabs(w, _("SKILLS"));

    WINDOW *w_description = newwin(3, FULL_SCREEN_WIDTH - 2, FULL_SCREEN_HEIGHT - 4 + getbegy(w),
                                   1 + getbegx(w));

    std::vector<Skill *> sorted_skills = Skill::skills;
    std::sort(sorted_skills.begin(), sorted_skills.end(), skill_display_sort);
    const int num_skills = Skill::skills.size();
    int cur_pos = 0;
    Skill *currentSkill = sorted_skills[cur_pos];

    const int iContentHeight = FULL_SCREEN_HEIGHT - 9;
    const int iHalf = iContentHeight / 2;

    do {
        mvwprintz(w, 3, 2, c_ltgray, _("Points left:%3d "), points);
        // Clear the bottom of the screen.
        werase(w_description);
        mvwprintz(w, 3, 40, c_ltgray, "                                    ");
        int cost = std::max(1, (u->skillLevel(currentSkill) + 1) / 2);
        mvwprintz(w, 3, 30, points >= cost ? COL_SKILL_USED : c_ltred,
                  _("Upgrading %s costs %d points"),
                  currentSkill->name().c_str(), cost);
        fold_and_print(w_description, 0, 0, FULL_SCREEN_WIDTH - 2, COL_SKILL_USED,
                       currentSkill->description().c_str());

        if (cur_pos < iHalf) {
            for (int i = 0; i < iContentHeight; ++i) {
                Skill *thisSkill = sorted_skills[i];

                mvwprintz(w, 5 + i, 2, c_ltgray, "\
                                             "); // Clear the line
                if (u->skillLevel(thisSkill) == 0) {
                    mvwprintz(w, 5 + i, 2, (i == cur_pos ? h_ltgray : c_ltgray),
                              thisSkill->name().c_str());
                } else {
                    mvwprintz(w, 5 + i, 2,
                              (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
                              "%s ", thisSkill->name().c_str());
                    for (int j = 0; j < u->skillLevel(thisSkill); j++) {
                        wprintz(w, (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
                    }
                }
            }
        } else if (cur_pos > num_skills - iContentHeight + iHalf) {
            for (int i = num_skills - iContentHeight; i < num_skills; ++i) {
                Skill *thisSkill = sorted_skills[i];
                mvwprintz(w, FULL_SCREEN_HEIGHT - 4 + i - num_skills, 2, c_ltgray, "\
                                             "); // Clear the line
                if (u->skillLevel(thisSkill) == 0) {
                    mvwprintz(w, FULL_SCREEN_HEIGHT - 4 + i - num_skills, 2,
                              (i == cur_pos ? h_ltgray : c_ltgray), thisSkill->name().c_str());
                } else {
                    mvwprintz(w, FULL_SCREEN_HEIGHT - 4 + i - num_skills, 2,
                              (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "%s ",
                              thisSkill->name().c_str());
                    for (int j = 0; j < u->skillLevel(thisSkill); j++) {
                        wprintz(w, (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
                    }
                }
            }
        } else {
            for (int i = cur_pos - iHalf; i < cur_pos + iContentHeight - iHalf; ++i) {
                Skill *thisSkill = sorted_skills[i];
                mvwprintz(w, 5 + iHalf + i - cur_pos, 2, c_ltgray, "\
                                             "); // Clear the line
                if (u->skillLevel(thisSkill) == 0) {
                    mvwprintz(w, 5 + iHalf + i - cur_pos, 2, (i == cur_pos ? h_ltgray : c_ltgray),
                              thisSkill->name().c_str());
                } else {
                    mvwprintz(w, 5 + iHalf + i - cur_pos, 2,
                              (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED),
                              "%s ", thisSkill->name().c_str());
                    for (int j = 0; j < u->skillLevel(thisSkill); j++) {
                        wprintz(w, (i == cur_pos ? hilite(COL_SKILL_USED) : COL_SKILL_USED), "*");
                    }
                }
            }
        }

        //Draw Scrollbar
        draw_scrollbar(w, cur_pos, iContentHeight, num_skills, 5);

        wrefresh(w);
        wrefresh(w_description);
        switch (input()) {
            case 'j':
            case '2':
                cur_pos++;
                if (cur_pos >= num_skills) {
                    cur_pos = 0;
                }
                currentSkill = sorted_skills[cur_pos];
                break;
            case 'k':
            case '8':
                cur_pos--;
                if (cur_pos < 0) {
                    cur_pos = num_skills - 1;
                }
                currentSkill = sorted_skills[cur_pos];
                break;
            case 'h':
            case '4': {
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
            	break;
            }
            case 'l':
            case '6': {
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
            	break;
            }
            case '<':
                return -1;
            case '>':
                return 1;
        }
    } while (true);
}

int set_description(WINDOW *w, game *g, player *u, character_type type, int &points)
{
    if(PLTYPE_NOW == type) {
        return 1;
    }

    draw_tabs(w, _("DESCRIPTION"));
    mvwprintz(w, 3, 2, c_ltgray, _("Points left:%3d"), points);

    const unsigned name_line = 6;
    const unsigned gender_line = 9;
    const unsigned namebar_pos = 3 + utf8_width(_("Name:"));
    mvwprintz(w, name_line, 2, c_ltgray, _("Name:"));
    mvwprintz(w, name_line, namebar_pos, c_ltgray, "_______________________________");
    mvwprintz(w, name_line, namebar_pos + 32, COL_NOTE_MINOR, _("(Press TAB to move off this line)"));
    fold_and_print(w, name_line + 1, 2, FULL_SCREEN_WIDTH - 4, COL_NOTE_MINOR,
                   _("To pick a random name for your character, press ?"));
    fold_and_print(w, FULL_SCREEN_HEIGHT / 2 + 3 , 2, FULL_SCREEN_WIDTH - 4, COL_NOTE_MAJOR,
                   _("When your character is finished and you're ready to start playing, press >"));
    fold_and_print(w, FULL_SCREEN_HEIGHT / 2 + 4, 2, FULL_SCREEN_WIDTH - 4, COL_NOTE_MAJOR,
                   _("To go back and review your character, press <"));
    fold_and_print(w, FULL_SCREEN_HEIGHT - 2, 2, FULL_SCREEN_WIDTH - 4, COL_NOTE_MINOR,
                   _("To save this character as a template, press !"));

    int line = 1;
    bool noname = false;
    long ch;
    unsigned male_pos = 3 + utf8_width(_("Gender:"));
    unsigned female_pos = 1 + male_pos + utf8_width(_("Male"));

    do {
        if (!noname) {
            mvwprintz(w, name_line, namebar_pos, c_ltgray, "%s", u->name.c_str());
            if (line == 1) {
                wprintz(w, h_ltgray, "_");
            }
        }

        mvwprintz(w, gender_line, 2, (line == 2 ? h_ltgray : c_ltgray), _("Gender:"));
        mvwprintz(w, gender_line, male_pos, (u->male ? c_ltred : c_ltgray), _("Male"));
        mvwprintz(w, gender_line, female_pos, (u->male ? c_ltgray : c_ltred), _("Female"));

        wrefresh(w);
        ch = input();
        if (noname) {
            mvwprintz(w, name_line, namebar_pos, c_ltgray, "______________________________");
            noname = false;
        }

        if (ch == '>') {
            if (points < 0) {
                popup(_("Too many points allocated, change some features and try again."));
                continue;
            } else if (points > 0 &&
                       !query_yn(_("Remaining points will be discarded, are you sure you want to proceed?"))) {
                continue;
            } else if (u->name.size() == 0) {
                mvwprintz(w, name_line, namebar_pos, h_ltgray, _("______NO NAME ENTERED!!!______"));
                noname = true;
                wrefresh(w);
                if (!query_yn(_("Are you SURE you're finished? Your name will be randomly generated."))) {
                    continue;
                } else {
                    u->pick_name();
                    return 1;
                }
            } else if (query_yn(_("Are you SURE you're finished?"))) {
                return 1;
            } else {
                continue;
            }
        } else if (ch == '<') {
            return -1;
        } else if (ch == '!') {
            if (points != 0) {
                popup(_("You cannot save a template with nonzero unused points!"));
            } else {
                save_template(u);
            }

            wrefresh(w);
        } else if (ch == '?') {
            mvwprintz(w, name_line, namebar_pos, c_ltgray, "______________________________");
            u->pick_name();
        } else {
            switch (line) {
                case 1: // name line
                    if (ch == KEY_BACKSPACE || ch == 127) {
                        if (u->name.size() > 0) {
                            //erase utf8 character TODO: make a function
                            while(u->name.size() > 0 && ((unsigned char)u->name[u->name.size() - 1]) >= 128 &&
                                  ((unsigned char)u->name[(int)u->name.size() - 1]) <= 191) {
                                u->name.erase(u->name.size() - 1);
                            }
                            u->name.erase(u->name.size() - 1);
                            mvwprintz(w, name_line, namebar_pos, c_ltgray, "_______________________________");
                            mvwprintz(w, name_line, namebar_pos, c_ltgray, "%s", u->name.c_str());
                            wprintz(w, h_ltgray, "_");
                        }
                    } else if (ch == '\t') {
                        line = 2;
                        mvwprintz(w, name_line, namebar_pos + utf8_width(u->name.c_str()), c_ltgray, "_");
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
                    else if(ch > 127) {
                        std::string tmp = utf32_to_utf8(ch);
                        int tmplen = utf8_width(tmp.c_str());
                        if(tmplen > 0 && tmplen + utf8_width(u->name.c_str()) < 30) {
                            u->name.append(tmp);
                        }
                    }
                    break;
                case 2: // gender line
                    if (ch == ' ' || ch == 'l' || ch == 'h') {
                        u->male = !u->male;
                    } else if (ch == 'j' || ch == 'k' || ch == '\t') {
                        line = 1;
                    }
                    break;
            }
        }
    } while (true);
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

int random_skill()
{
    return rng(1, Skill::skills.size() - 1);
}

int calc_HP(int strength, bool tough)
{
    return int((60 + 3 * strength) * (tough ? 1.2 : 1));
}

void save_template(player *u)
{
    std::string title = _("Name of template:");
    std::string name = string_input_popup(title, FULL_SCREEN_WIDTH - utf8_width(title.c_str()) - 8);
    if (name.length() == 0) {
        return;
    }
    std::stringstream playerfile;
    playerfile << "data/" << name << ".template";
    std::ofstream fout;
    fout.open(playerfile.str().c_str());
    fout << u->save_info();
}
