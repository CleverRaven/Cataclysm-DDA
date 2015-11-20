#include "player.h"
#include "profession.h"
#include "scenario.h"
#include "start_location.h"
#include "input.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "name.h"
#include "options.h"
#include "catacharset.h"
#include "debug.h"
#include "char_validity_check.h"
#include "path_info.h"
#include "mapsharing.h"
#include "translations.h"
#include "martialarts.h"
#include "addiction.h"
#include "ui.h"
#include "mutation.h"
#include "recipe_dictionary.h"
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <cstring>
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

#define NEWCHAR_TAB_MAX 5 // The ID of the rightmost tab

void draw_tabs(WINDOW *w, std::string sTab);

int set_stats(WINDOW *w, player *u, int &points);
int set_traits(WINDOW *w, player *u, int &points, int max_trait_points);
int set_scenario(WINDOW *w, player *u, int &points);
int set_profession(WINDOW *w, player *u, int &points);
int set_skills(WINDOW *w, player *u, int &points);

int set_description(WINDOW *w, player *u, character_type type, int &points);

void save_template(player *u);

bool lcmatch(const std::string &str, const std::string &findstr); // ui.cpp

void Character::pick_name(bool bUseDefault)
{
    if (bUseDefault && OPTIONS["DEF_CHAR_NAME"]) {
        name = OPTIONS["DEF_CHAR_NAME"].getValue();
    } else {
        name = Name::generate(male);
    }
}

matype_id choose_ma_style( const character_type type, const std::vector<matype_id> &styles )
{
    if( type == PLTYPE_NOW ) {
        return random_entry( styles );
    }
    if( styles.size() == 1 ) {
        return styles.front();
    }
    uimenu menu;
    menu.text = _( "Pick your style:" );
    menu.desc_enabled = true;
    for( auto & s : styles ) {
        auto &style = s.obj();
        menu.addentry_desc( style.name, style.description );
    }
    menu.selected = 0;
    while( true ) {
        menu.query(true);
        auto &selected = styles[menu.ret];
        if( query_yn( _( "Use this style?" ) ) ) {
            return selected;
        }
    }
}

int player::create(character_type type, std::string tempname)
{
    weapon = item("null", 0);

    g->u.prof = profession::generic();
    g->scen = scenario::generic();


    WINDOW *w = nullptr;
    if( type != PLTYPE_NOW ) {
        w = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                       (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                       (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    }

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
                g->u.pick_name(true);
            } else {
                g->u.name = MAP_SHARING::getUsername();
            }
        case PLTYPE_RANDOM_WITH_SCENARIO:
        case PLTYPE_RANDOM: {
            g->u.male = (rng(1, 100) > 50);
            if(!MAP_SHARING::isSharing()) {
                g->u.pick_name(true);
            } else {
                g->u.name = MAP_SHARING::getUsername();
            }
            if (type == PLTYPE_RANDOM_WITH_SCENARIO) {
                std::vector<scenario *> scenarios;
                for (scenmap::const_iterator iter = scenario::begin(); iter != scenario::end(); iter++) {
                    if (!(iter->second).has_flag("CHALLENGE")) {
                        scenarios.emplace_back(scenario::scen((iter->second).ident()));
                    }
                }
                g->scen = random_entry( scenarios );
                if (g->scen->profsize() > 0) {
                    g->u.prof = g->scen->random_profession();
                } else {
                    g->u.prof = profession::weighted_random();
                }
                g->u.start_location = g->scen->random_start_location();
            } else {
                g->u.prof = profession::weighted_random();
            }
            str_max = rng(6, 12);
            dex_max = rng(6, 12);
            int_max = rng(6, 12);
            per_max = rng(6, 12);
            points = points - str_max - dex_max - int_max - per_max - g->u.prof->point_cost() - g->scen->point_cost();
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
                    } while ((has_trait(rn) || num_btraits - mutation_branch::get( rn ).points > max_trait_points) &&
                             tries < 5);

                    if (tries < 5 && !has_conflicting_trait(rn)) {
                        toggle_trait(rn);
                        points -= mutation_branch::get( rn ).points;
                        num_btraits -= mutation_branch::get( rn ).points;
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

            /* The loops variable is used to prevent the algorithm running in an infinite loop */
            unsigned int loops = 0;
            while (points > 0 && loops <= 30000) {
                switch (rng((num_gtraits < max_trait_points ? 1 : 5), 9)) {
                case 1:
                case 2:
                case 3:
                case 4:
                    rn = random_good_trait();
                    {
                        auto &mdata = mutation_branch::get( rn );
                    if (!has_trait(rn) && points >= mdata.points &&
                        num_gtraits + mdata.points <= max_trait_points &&
                        !has_conflicting_trait(rn)) {
                        toggle_trait(rn);
                        points -= mdata.points;
                        num_gtraits += mdata.points;
                    }
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
                    const Skill* aSkill = Skill::random_skill();
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
                return 0;
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
        if( w == nullptr ) {
            // assert( type == PLTYPE_NOW );
            // no window is created because "Play now"  does not require any configuration
            break;
        }
        werase(w);
        wrefresh(w);
        switch (tab) {
        case 0:
            tab += set_scenario   (w, this, points);
            break;
        case 1:
            tab += set_profession (w, this, points);
            break;
        case 2:
            tab += set_traits     (w, this, points, max_trait_points);
            break;
        case 3:
            tab += set_stats      (w, this, points);
            break;
        case 4:
            tab += set_skills     (w, this, points);
            break;
        case 5:
            tab += set_description(w, this, type, points);
            break;
        }
    } while (tab >= 0 && tab <= NEWCHAR_TAB_MAX);
    delwin(w);

    if (tab < -1) {
        // Returned from set_description for reroll
        if (tab == -3) {
            return -2;
        }
        return -1;
    } else if (tab < 0) {
        return 0;
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

    ret_null = item("null", 0);
    weapon = ret_null;


    item tmp; //gets used several times
    item tmp2;

    auto prof_items = g->u.prof->items( g->u.male );

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
    for( auto &itd : prof_items ) {
        tmp = item(itd.type_id, 0, false);
        if( !itd.snippet_id.empty() ) {
            tmp.set_snippet( itd.snippet_id );
        }
        tmp = tmp.in_its_container();
        if(tmp.is_armor()) {
            if(tmp.has_flag("VARSIZE")) {
                tmp.item_tags.insert("FIT");
            }
            // If wearing an item fails we fail silently.
            wear_item(tmp, false);

            // if something is wet, start it as active with some time to dry off
        } else if(tmp.has_flag("WET")) {
            tmp.active = true;
            tmp.item_counter = 450;
            inv.push_back(tmp);
        } else {
            inv.push_back(tmp);
        }
        if( tmp.is_book() ) {
            items_identified.insert( tmp.typeId() );
        }
    }

    std::vector<addiction> prof_addictions = g->u.prof->addictions();
    for (std::vector<addiction>::const_iterator iter = prof_addictions.begin();
         iter != prof_addictions.end(); ++iter) {
        g->u.addictions.push_back(*iter);
    }

    // Grab the skills from the profession, if there are any
    for( auto &e : g->u.prof->skills() ) {
        g->u.boost_skill_level( e.first, e.second );
    }

    // Get CBMs
    std::vector<std::string> prof_CBMs = g->u.prof->CBMs();
    for (std::vector<std::string>::const_iterator iter = prof_CBMs.begin();
         iter != prof_CBMs.end(); ++iter) {
        add_bionic(*iter);
    }
    // Adjust current energy level to maximum
    power_level = max_power_level;

    // Get traits
    std::vector<std::string> prof_traits = g->u.prof->traits();
    for (std::vector<std::string>::const_iterator iter = prof_traits.begin();
         iter != prof_traits.end(); ++iter) {
         g->u.toggle_trait(*iter);
    }
    for( auto &t : get_base_traits() ) {
        std::vector<matype_id> styles;
        for( auto &s : mutation_branch::get( t ).initial_ma_styles ) {
            if( !has_martialart( s ) ) {
                styles.push_back( s );
            }
        }
        if( !styles.empty() ) {
            const auto ma_type = choose_ma_style( type, styles );
            ma_styles.push_back( ma_type );
            style_selected = ma_type;
        }
    }
    // For compatibility with old versions and for better user experience:
    // activate some mutations right from the start.
    // TODO: (maybe) move this to json?
    if( has_trait( "NIGHTVISION" ) ) {
        my_mutations["NIGHTVISION"].powered = true;
    } else if( has_trait( "URSINE_EYE" ) ) {
        my_mutations["URSINE_EYE"].powered = true;
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

    // Ensure that persistent morale effects (e.g. Optimist) are present at the start.
    apply_persistent_morale();
    return 1;
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
    tab_captions.push_back(_("SCENARIO"));
    tab_captions.push_back(_("PROFESSION"));
    tab_captions.push_back(_("TRAITS"));
    tab_captions.push_back(_("STATS"));
    tab_captions.push_back(_("SKILLS"));
    tab_captions.push_back(_("DESCRIPTION"));

    size_t temp_len = 0;
    size_t tabs_length = 0;
    std::vector<int> tab_len;
    for (auto tab_name : tab_captions) {
        // String length + borders
        temp_len = utf8_width(tab_name) + 2;
        tabs_length += temp_len;
        tab_len.push_back(temp_len);
    }

    int next_pos = 2;
    // Free space on tabs window. '<', '>' symbols is drawning on free space.
    // Initial value of next_pos is free space too.
    // '1' is used for SDL/curses screen column reference.
    int free_space = (FULL_SCREEN_WIDTH - tabs_length - 1 - next_pos);
    int spaces = free_space / ((int)tab_captions.size() - 1);
    if (spaces < 0) {
        spaces = 0;
    }
    for (size_t i = 0; i < tab_captions.size(); ++i) {
        draw_tab(w, next_pos, tab_captions[i].c_str(), (sTab == tab_captions[i]));
        next_pos += tab_len[i] + spaces;
    }

    mvwputch(w, 2,  0, BORDER_COLOR, LINE_OXXO); // |^
    mvwputch(w, 2, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_OOXX); // ^|

    mvwputch(w, 4, 0, BORDER_COLOR, LINE_XXXO); // |-
    mvwputch(w, 4, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOXX); // -|

    mvwputch(w, FULL_SCREEN_HEIGHT - 1, 0, BORDER_COLOR, LINE_XXOO); // |_
    mvwputch(w, FULL_SCREEN_HEIGHT - 1, FULL_SCREEN_WIDTH - 1, BORDER_COLOR, LINE_XOOX); // _|
}

template <class Compare>
void draw_sorting_indicator(WINDOW *w_sorting, input_context ctxt, Compare sorter)
{
    auto const sort_order = sorter.sort_by_points ? _("points") : _("name");
    auto const sort_help = string_format( _("(Press <color_light_green>%s</color> to change)"),
                                           ctxt.get_desc("SORT").c_str() );
    wprintz(w_sorting, COL_HEADER, _("Sort by:"));
    wprintz(w_sorting, c_ltgray, " %s", sort_order);
    fold_and_print(w_sorting, 0, 16, (FULL_SCREEN_WIDTH / 2), c_ltgray, sort_help);
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
    u->setx( -1 );
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

        mvwprintz(w, FULL_SCREEN_HEIGHT - 4, 2, COL_NOTE_MAJOR,
                  _("%s lets you view and alter keybindings."), ctxt.get_desc("HELP_KEYBINDINGS").c_str());
        mvwprintz(w, FULL_SCREEN_HEIGHT - 3, 2, COL_NOTE_MAJOR, _("%s takes you to the next tab."),
                  ctxt.get_desc("NEXT_TAB").c_str());
        mvwprintz(w, FULL_SCREEN_HEIGHT - 2, 2, COL_NOTE_MAJOR, _("%s returns you to the main menu."),
                  ctxt.get_desc("PREV_TAB").c_str());

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
                      u->convert_weight(u->weight_capacity()),
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
                      u->get_hit_base());
            if (u->throw_dex_mod(false) <= 0) {
                mvwprintz(w_description, 1, 0, COL_STAT_BONUS, _("Throwing bonus: +%d"),
                          abs(u->throw_dex_mod(false)));
            } else {
                mvwprintz(w_description, 1, 0, COL_STAT_PENALTY, _("Throwing penalty: -%d"),
                          abs(u->throw_dex_mod(false)));
            }
            if (u->ranged_dex_mod() != 0) {
                mvwprintz(w_description, 2, 0, COL_STAT_PENALTY, _("Ranged penalty: -%d"),
                          abs(u->ranged_dex_mod()));
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
            if (u->ranged_per_mod() != 0) {
                mvwprintz(w_description, 0, 0, COL_STAT_PENALTY, _("Ranged penalty: -%d"),
                          abs(u->ranged_per_mod()));
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
        } else if (action == "PREV_TAB") {
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

    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( traits_iter.second.startingtrait || g->scen->traitquery( traits_iter.first ) == true ) {
            if( traits_iter.second.points >= 0 ) {
                vStartingTraits[0].push_back( traits_iter.first );

                if( u->has_trait( traits_iter.first ) ) {
                    num_good += traits_iter.second.points;
                }
            } else {
                vStartingTraits[1].push_back( traits_iter.first );

                if( u->has_trait( traits_iter.first ) ) {
                    num_bad += traits_iter.second.points;
                }
            }
        }
    }

    for( auto &vStartingTrait : vStartingTraits ) {
        std::sort( vStartingTrait.begin(), vStartingTrait.end(), trait_display_sort );
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

    size_t traits_size[2];
    traits_size[0] = vStartingTraits[0].size();
    traits_size[1] = vStartingTraits[1].size();

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
                         traits_size[iCurrentPage]);

            //Draw Traits
            for (int i = iStartPos[iCurrentPage]; i < (int)traits_size[iCurrentPage]; i++) {
                if (i >= iStartPos[iCurrentPage] && i < iStartPos[iCurrentPage] +
                    (int)((iContentHeight > traits_size[iCurrentPage]) ?
                          traits_size[iCurrentPage] : iContentHeight)) {
                    auto &mdata = mutation_branch::get( vStartingTraits[iCurrentPage][i] );
                    if (iCurrentLine[iCurrentPage] == i && iCurrentPage == iCurWorkingPage) {
                        mvwprintz(w,  3, 41, c_ltgray,
                                  "                                      ");
                        int points = mdata.points;
                        bool negativeTrait = points < 0;
                        if (negativeTrait) {
                            points *= -1;
                        }
                        mvwprintz(w,  3, 41, col_tr, ngettext("%s %s %d point", "%s %s %d points", points),
                                  mdata.name.c_str(),
                                  negativeTrait ? _("earns") : _("costs"),
                                  points);
                        fold_and_print(w_description, 0, 0,
                                       FULL_SCREEN_WIDTH - 2, col_tr,
                                       mdata.description);
                    }

                    nc_color cLine = col_off_pas;
                    if (iCurWorkingPage == iCurrentPage) {
                        cLine = col_off_act;
                        if (iCurrentLine[iCurrentPage] == (int)i) {
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
                              mdata.name.c_str());
                }
            }

            //Draw Scrollbar, one for good and one for bad traits
            draw_scrollbar(w, iCurrentLine[0], iContentHeight, traits_size[0], 5);
            draw_scrollbar(w, iCurrentLine[1], iContentHeight, traits_size[1], 5, getmaxx(w) - 1);
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
            if (iCurrentLine[iCurWorkingPage] == 0) {
                iCurrentLine[iCurWorkingPage] = traits_size[iCurWorkingPage] - 1;
            } else {
                iCurrentLine[iCurWorkingPage]--;
            }
        } else if (action == "DOWN") {
            iCurrentLine[iCurWorkingPage]++;
            if ((size_t) iCurrentLine[iCurWorkingPage] >= traits_size[iCurWorkingPage]) {
                iCurrentLine[iCurWorkingPage] = 0;
            }
        } else if (action == "CONFIRM") {
            int inc_type = 0;
            std::string cur_trait = vStartingTraits[iCurWorkingPage][iCurrentLine[iCurWorkingPage]];
            const auto &mdata = mutation_branch::get( cur_trait );
            if (u->has_trait(cur_trait)) {

                inc_type = -1;
                // If turning off the trait violates a profession condition,
                // turn it back on.
                if(!(u->prof->can_pick(u, 0))) {
                    inc_type = 0;
                    popup(_("Your profession of %s prevents you from removing this trait."),
                          u->prof->gender_appropriate_name(u->male).c_str());
                }
                if(g->scen->locked_traits(cur_trait)) {
                    inc_type = 0;
                    popup(_("The scenario you picked prevents you from removing this trait!"));
                }
            } else if(u->has_conflicting_trait(cur_trait)) {
                popup(_("You already picked a conflicting trait!"));
            } else if(g->scen->forbidden_traits(cur_trait)) {
                popup(_("The scenario you picked prevents you from taking this trait!"));
            } else if (iCurWorkingPage == 0 && num_good + mdata.points >
                       max_trait_points) {
                popup(ngettext("Sorry, but you can only take %d point of advantages.",
                               "Sorry, but you can only take %d points of advantages.", max_trait_points),
                      max_trait_points);

            } else if (iCurWorkingPage != 0 && num_bad + mdata.points <
                       -max_trait_points) {
                popup(ngettext("Sorry, but you can only take %d point of disadvantages.",
                               "Sorry, but you can only take %d points of disadvantages.", max_trait_points),
                      max_trait_points);

            } else {
                inc_type = 1;

                // If turning on the trait violates a profession condition,
                // turn it back off.
                if(!(u->prof->can_pick(u, 0))) {
                    inc_type = 0;
                    popup(_("Your profession of %s prevents you from taking this trait."),
                          u->prof->gender_appropriate_name(u->male).c_str());

                }
            }

            //inc_type is either -1 or 1, so we can just multiply by it to invert
            if(inc_type != 0) {
                u->toggle_trait(cur_trait);
                points -= mdata.points * inc_type;
                if (iCurWorkingPage == 0) {
                    num_good += mdata.points * inc_type;
                } else {
                    num_bad += mdata.points * inc_type;
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

struct {
    bool sort_by_points = true;
    bool male;
    bool operator() (const profession *a, const profession *b)
    {
        // The generic ("Unemployed") profession should be listed first.
        const profession *gen = profession::generic();
        if (b == gen) {
            return false;
        } else if (a == gen) {
            return true;
        }

        if (sort_by_points) {
            return a->point_cost() < b->point_cost();
        } else {
            return a->gender_appropriate_name(male) <
                   b->gender_appropriate_name(male);
        }
    }
} profession_sorter;

int set_profession(WINDOW *w, player *u, int &points)
{
    draw_tabs(w, _("PROFESSION"));
    int cur_id = 0;
    int retval = 0;
    int desc_offset = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 10;
    int iStartPos = 0;

    WINDOW *w_description = newwin(4, FULL_SCREEN_WIDTH - 2,
                                   FULL_SCREEN_HEIGHT - 5 + getbegy(w), 1 + getbegx(w));

    WINDOW *w_sorting =     newwin(1,                  55,  5 + getbegy(w), 24 + getbegx(w));
    WINDOW *w_genderswap =  newwin(1,                  55,  6 + getbegy(w), 24 + getbegx(w));
    WINDOW *w_items =       newwin(iContentHeight - 2, 55,  7 + getbegy(w), 24 + getbegx(w));

    input_context ctxt("NEW_CHAR_PROFESSIONS");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("CHANGE_GENDER");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("SORT");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("FILTER");

    bool recalc_profs = true;
    int profs_length = 0;
    std::string filterstring;
    std::vector<const profession *> sorted_profs;

    do {
        if (recalc_profs) {
            sorted_profs.clear();
            for (profmap::const_iterator iter = profession::begin(); iter != profession::end(); ++iter) {
                if ((g->scen->profsize() == 0 && (iter->second).has_flag("SCEN_ONLY") == false) ||
                    g->scen->profquery(&(iter->second)) == true) {
                    if (!lcmatch(iter->second.gender_appropriate_name(u->male), filterstring)) {
                        continue;
                    }
                    sorted_profs.push_back(&(iter->second));
                }
            }
            profs_length = sorted_profs.size();
            if (profs_length == 0) {
                popup(_("Nothing found.")); // another case of black box in tiles
                filterstring.clear();
                continue;
            }

            // Sort professions by points.
            // profession_display_sort() keeps "unemployed" at the top.
            profession_sorter.male = u->male;
            std::stable_sort(sorted_profs.begin(), sorted_profs.end(), profession_sorter);

            // Select the current profession, if possible.
            for (int i = 0; i < profs_length; ++i) {
                if (sorted_profs[i]->ident() == u->prof->ident()) {
                    cur_id = i;
                    break;
                }
            }
            if (cur_id > profs_length - 1) {
                cur_id = 0;
            }

            // Draw filter indicator
            for (int i = 1; i < FULL_SCREEN_WIDTH - 1; i++) {
                mvwputch(w, FULL_SCREEN_HEIGHT - 1, i, BORDER_COLOR, LINE_OXOX);
            }
            const auto filter_indicator = filterstring.empty() ? _("no filter")
                                          : filterstring;
            mvwprintz(w, getmaxy(w) - 1, 2, c_ltgray, "<%s>", filter_indicator.c_str());

            recalc_profs = false;
        }

        int netPointCost = sorted_profs[cur_id]->point_cost() - u->prof->point_cost();
        bool can_pick = sorted_profs[cur_id]->can_pick(u, points);
        // Magic number. Strongly related to window width (w_width - borders).
        const std::string empty_line(78, ' ');

        // Clear the bottom of the screen and header.
        werase(w_description);
        mvwprintz(w, 3, 1, c_ltgray, empty_line.c_str());

        int pointsForProf = sorted_profs[cur_id]->point_cost();
        bool negativeProf = pointsForProf < 0;
        if (negativeProf) {
            pointsForProf *= -1;
        }
        // Draw header.
        std::string points_msg = string_format(_("Points left: %2d"), points);
        int pMsg_length = utf8_width(_(points_msg.c_str()));
        if (netPointCost > 0) {
            mvwprintz(w, 3, 2, c_ltgray, _(points_msg.c_str()));
            mvwprintz(w, 3, pMsg_length + 2, c_red, "(-%d)", abs(netPointCost));
        } else if (netPointCost == 0) {
            mvwprintz(w, 3, 2, c_ltgray, _(points_msg.c_str()));
        } else {
            mvwprintz(w, 3, 2, c_ltgray, _(points_msg.c_str()));
            mvwprintz(w, 3, pMsg_length + 2, c_green, "(+%d)", abs(netPointCost));
        }

        std::string prof_msg_temp;
        if (negativeProf) {
            //~ 1s - profession name, 2d - current character points.
            prof_msg_temp = ngettext("Profession %1$s earns %2$d point",
                                     "Profession %1$s earns %2$d points",
                                     pointsForProf);
        } else {
            //~ 1s - profession name, 2d - current character points.
            prof_msg_temp = ngettext("Profession %1$s costs %2$d point",
                                     "Profession %1$s costs %2$d points",
                                     pointsForProf);
        }
        // This string has fixed start pos(7 = 2(start) + 5(length of "(+%d)" and space))
        mvwprintz(w, 3, pMsg_length + 7, can_pick ? c_green : c_ltred, prof_msg_temp.c_str(),
                  sorted_profs[cur_id]->gender_appropriate_name(u->male).c_str(),
                  pointsForProf);

        fold_and_print(w_description, 0, 0, FULL_SCREEN_WIDTH - 2, c_green,
                       sorted_profs[cur_id]->description(u->male));

        //Draw options
        calcStartPos(iStartPos, cur_id, iContentHeight, profs_length);
        const int end_pos = iStartPos + ((iContentHeight > profs_length) ?
                            profs_length : iContentHeight);
        int i;
        for (i = iStartPos; i < end_pos; i++) {
            mvwprintz(w, 5 + i - iStartPos, 2, c_ltgray, "\
                                             "); // Clear the line
            nc_color col;
            if (u->prof != sorted_profs[i]) {
                col = (sorted_profs[i] == sorted_profs[cur_id] ? h_ltgray : c_ltgray);
            } else {
                col = (sorted_profs[i] == sorted_profs[cur_id] ? hilite(COL_SKILL_USED) : COL_SKILL_USED);
            }
            mvwprintz(w, 5 + i - iStartPos, 2, col,
                      sorted_profs[i]->gender_appropriate_name(u->male).c_str());
        }
        //Clear rest of space in case stuff got filtered out
        for (; i < iStartPos + iContentHeight; ++i) {
            mvwprintz(w, 5 + i - iStartPos, 2, c_ltgray, "\
                                             "); // Clear the line
        }

        std::ostringstream buffer;

        // Profession addictions
        const auto prof_addictions = sorted_profs[cur_id]->addictions();
        if( !prof_addictions.empty() ) {
            buffer << "<color_ltblue>" << _( "Addictions:" ) << "</color>\n";
            for( const auto &a : prof_addictions ) {
                const auto format = pgettext( "set_profession_addictions", "%1$s (%2$d)" );
                buffer << string_format( format, addiction_name( a ).c_str(), a.intensity ) << "\n";
            }
        }

        // Profession traits
        const auto prof_traits = sorted_profs[cur_id]->traits();
        buffer << "<color_ltblue>" << _( "Profession traits:" ) << "</color>\n";
        if( prof_traits.empty() ) {
            buffer << pgettext( "set_profession_trait", "None" ) << "\n";
        } else {
            for( const auto &t : sorted_profs[cur_id]->traits() ) {
                buffer << mutation_branch::get_name( t ) << "\n";
            }
        }

        // Profession skills
        const auto prof_skills = sorted_profs[cur_id]->skills();
        buffer << "<color_ltblue>" << _( "Profession skills:" ) << "</color>\n";
        if( prof_skills.empty() ) {
            buffer << pgettext( "set_profession_skill", "None" ) << "\n";
        } else {
            for( const auto &sl : prof_skills ) {
                const auto format = pgettext( "set_profession_skill", "%1$s (%2$d)" );
                buffer << string_format( format, sl.first.obj().name().c_str(), sl.second ) << "\n";
            }
        }

        // Profession items
        const auto prof_items = sorted_profs[cur_id]->items( u->male );
        if( prof_items.empty() ) {
            buffer << pgettext( "set_profession_item", "None" ) << "\n";
        } else {
            buffer << "<color_ltblue>" << _( "Profession items:" ) << "</color>\n";
            for( const auto &i : prof_items ) {
                buffer << item::nname( i.type_id ) << "\n";
            }
        }

        // Profession bionics, active bionics shown first
        auto prof_CBMs = sorted_profs[cur_id]->CBMs();
        std::sort(begin(prof_CBMs), end(prof_CBMs), [](std::string const &a, std::string const &b) {
            return bionic_info(a).activated && !bionic_info(b).activated;
        });
        buffer << "<color_ltblue>" << _( "Profession bionics:" ) << "</color>\n";
        if( prof_CBMs.empty() ) {
            buffer << pgettext( "set_profession_bionic", "None" ) << "\n";
        } else {
            for( const auto &b : prof_CBMs ) {
                auto const &cbm = bionic_info(b);

                if (cbm.activated && cbm.toggled) {
                    buffer << cbm.name << " (" << _("toggled") << ")\n";
                } else if (cbm.activated) {
                    buffer << cbm.name << " (" << _("activated") << ")\n";
                } else {
                    buffer << cbm.name << "\n";
                }
            }
        }

        werase( w_items );
        const auto scroll_msg = string_format( _( "Press <color_light_green>%1$s</color> or <color_light_green>%2$s</color> to scroll." ),
                                               ctxt.get_desc("LEFT").c_str(),
                                               ctxt.get_desc("RIGHT").c_str() );
        const int iheight = print_scrollable( w_items, desc_offset, buffer.str(), c_ltgray, scroll_msg );

        werase(w_sorting);
        draw_sorting_indicator(w_sorting, ctxt, profession_sorter);

        werase(w_genderswap);
        //~ Gender switch message. 1s - change key name, 2s - profession name.
        std::string g_switch_msg = u->male ? _("Press %1$s to switch to %2$s(female).") :
                                   _("Press %1$s to switch to %2$s(male).");
        mvwprintz(w_genderswap, 0, 0, c_magenta, g_switch_msg.c_str(),
                  ctxt.get_desc("CHANGE_GENDER").c_str(),
                  sorted_profs[cur_id]->gender_appropriate_name(!u->male).c_str());

        draw_scrollbar(w, cur_id, iContentHeight, profs_length, 5);

        wrefresh(w);
        wrefresh(w_description);
        wrefresh(w_items);
        wrefresh(w_genderswap);
        wrefresh(w_sorting);

        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            cur_id++;
            if (cur_id > (int)profs_length - 1) {
                cur_id = 0;
            }
            desc_offset = 0;
        } else if (action == "UP") {
            cur_id--;
            if (cur_id < 0) {
                cur_id = profs_length - 1;
            }
            desc_offset = 0;
        } else if( action == "LEFT" ) {
            if( desc_offset > 0 ) {
                desc_offset--;
            }
        } else if( action == "RIGHT" ) {
            if( desc_offset < iheight ) {
                desc_offset++;
            }
        } else if (action == "CONFIRM") {
            u->prof = profession::prof(sorted_profs[cur_id]->ident()); // we've got a const*
            points -= netPointCost;
        } else if (action == "CHANGE_GENDER") {
            u->male = !u->male;
            profession_sorter.male = u->male;
            if (!profession_sorter.sort_by_points) {
                std::sort(sorted_profs.begin(), sorted_profs.end(), profession_sorter);
            }
        } else if (action == "PREV_TAB") {
            retval = -1;
        } else if (action == "NEXT_TAB") {
            retval = 1;
        } else if (action == "SORT") {
            profession_sorter.sort_by_points = !profession_sorter.sort_by_points;
            recalc_profs = true;
        } else if (action == "FILTER") {
            filterstring = string_input_popup(_("Search:"), 60, filterstring,
                _("Search by profession name."));
            recalc_profs = true;
        }

    } while (retval == 0);

    delwin(w_description);
    delwin(w_sorting);
    delwin(w_items);
    delwin(w_genderswap);
    return retval;
}

int set_skills(WINDOW *w, player *u, int &points)
{
    draw_tabs(w, _("SKILLS"));
    const int iContentHeight = FULL_SCREEN_HEIGHT - 6;
    const int iHalf = iContentHeight / 2;
    WINDOW *w_description = newwin(iContentHeight, FULL_SCREEN_WIDTH - 35,
                                   5 + getbegy(w), 31 + getbegx(w));

    auto sorted_skills = Skill::get_skills_sorted_by([](Skill const &a, Skill const &b) {
        return a.name() < b.name();
    });

    const int num_skills = Skill::skills.size();
    int cur_pos = 0;
    const Skill* currentSkill = sorted_skills[cur_pos];
    int selected = 0;
    int line_num = 1;

    input_context ctxt("NEW_CHAR_SKILLS");
    ctxt.register_cardinal();
    ctxt.register_action("PAGE_DOWN");
    ctxt.register_action("PAGE_UP");
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

        std::map<std::string, std::stringstream > recipes;
        for( auto &cur_recipe : recipe_dict ) {
            if ( !cur_recipe->autolearn &&
                 ( cur_recipe->skill_used == currentSkill->ident() ||
                   cur_recipe->required_skills[currentSkill->ident()] > 0 ) &&
                 u->has_recipe_requirements(cur_recipe) &&
                 cur_recipe->ident.find("uncraft") == std::string::npos
               )  {

                if ( recipes[cur_recipe->skill_used.obj().name()].str() != "" ) {
                    recipes[cur_recipe->skill_used.obj().name()] << ", ";
                }

                /*
                int skill = ((cur_recipe->required_skills[currentSkill->ident()] > 0) ?
                             cur_recipe->required_skills[currentSkill->ident()] :
                             cur_recipe->difficulty);

                if ( cur_recipe->skill_used != currentSkill->ident() ) {
                    skill = ((cur_recipe->required_skills[currentSkill->ident()] > 0) ?
                             cur_recipe->difficulty :
                             cur_recipe->required_skills[currentSkill->ident()]);
                }

                recipes[cur_recipe->skill_used.obj().name()] << item::nname(cur_recipe->result) << " (" << skill << ")";
                */
                
                recipes[cur_recipe->skill_used.obj().name()] << item::nname(cur_recipe->result) << " ("
                                                             << ((cur_recipe->required_skills[currentSkill->ident()] > 0) ?
                                                                 cur_recipe->required_skills[currentSkill->ident()] :
                                                                 cur_recipe->difficulty)
                                                             << ")";
            }
        }

        std::stringstream rec_disp;
        rec_disp << currentSkill->description() << "<color_c_ltgray>";

        if (recipes.find(currentSkill->name()) != recipes.end()) {
            rec_disp << "\n \n<color_c_brown>" << recipes[currentSkill->name()].str() << "</color>";
        }

        for ( auto iter = recipes.begin(); iter != recipes.end(); ++iter ) {
            if ( iter->first != currentSkill->name() ) {
                if ( rec_disp.str() != "" ) {
                    rec_disp << "\n \n";
                }
                rec_disp << "<color_c_white>[" << iter->first << "]</color>\n<color_c_ltgray>" << (iter->second).str() << "</color>";
            }
        }

        rec_disp << "</color>";

        const auto vFolded = foldstring(rec_disp.str(), getmaxx(w_description));
        int iLines = vFolded.size();

        if( selected < 0 ) {
            selected = 0;
        } else if( iLines < iContentHeight ) {
            selected = 0;
        } else if( selected >= iLines - iContentHeight ) {
            selected = iLines - iContentHeight;
        }

        fold_and_print_from( w_description, line_num, 0, getmaxx(w_description), selected, COL_SKILL_USED, rec_disp.str() );

        draw_scrollbar( w, selected, iContentHeight, iLines-iContentHeight, 5, getmaxx(w) - 1, BORDER_COLOR, true );

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
            const Skill* thisSkill = sorted_skills[i];
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
            for( auto &prof_skill : u->prof->skills() ) {
                if( prof_skill.first == thisSkill->ident() ) {
                    wprintz( w, ( i == cur_pos ? h_white : c_white ), " (+%d)",
                             int( prof_skill.second ) );
                    break;
                }
            }
        }

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
            SkillLevel &level = u->skillLevel(currentSkill);
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
            SkillLevel &level = u->skillLevel(currentSkill);
            if (level <= 19) {
                if (level == 0) {  // raise 0->2 for 1 point
                    level.level(2);
                    points -= 1;
                } else {
                    points -= (level + 1) / 2;
                    level.level(level + 1);
                }
            }
        } else if (action == "PAGE_DOWN") {
            selected++;
        } else if (action == "PAGE_UP") {
            selected--;
        } else if (action == "PREV_TAB") {
            delwin(w_description);
            return -1;
        } else if (action == "NEXT_TAB") {
            delwin(w_description);
            return 1;
        }
    } while (true);
}

struct {
    bool sort_by_points = true;
    bool male;
    bool operator() (const scenario *a, const scenario *b)
    {
        // The generic ("Unemployed") profession should be listed first.
        const scenario *gen = scenario::generic();
        if (b == gen) {
            return false;
        } else if (a == gen) {
            return true;
        }

        if (sort_by_points) {
            return a->point_cost() < b->point_cost();
        } else {
            return a->gender_appropriate_name(male) <
                   b->gender_appropriate_name(male);
        }
    }
} scenario_sorter;

int set_scenario(WINDOW *w, player *u, int &points)
{
    draw_tabs(w, _("SCENARIO"));

    int cur_id = 0;
    int retval = 0;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 10;
    int iStartPos = 0;

    WINDOW *w_description = newwin(4, FULL_SCREEN_WIDTH - 2,
                                   FULL_SCREEN_HEIGHT - 5 + getbegy(w), 1 + getbegx(w));
    WINDOW_PTR w_descriptionptr( w_description );

    WINDOW *w_sorting = newwin(2, (FULL_SCREEN_WIDTH / 2) - 1,
                               5 + getbegy(w),  (FULL_SCREEN_WIDTH / 2) + getbegx(w));
    WINDOW_PTR w_sortingptr( w_sorting );

    WINDOW *w_profession = newwin(4, (FULL_SCREEN_WIDTH / 2) - 1,
                                  7 + getbegy(w),  (FULL_SCREEN_WIDTH / 2) + getbegx(w));
    WINDOW_PTR w_professionptr( w_profession );

    WINDOW *w_location =   newwin(3, (FULL_SCREEN_WIDTH / 2) - 1,
                                  11 + getbegy(w), (FULL_SCREEN_WIDTH / 2) + getbegx(w));

    WINDOW_PTR w_locationptr( w_location );

    // 9 = 2 + 4 + 3, so we use rest of space for flags
    WINDOW *w_flags = newwin(iContentHeight - 9, (FULL_SCREEN_WIDTH / 2) - 1,
                             14 + getbegy(w), (FULL_SCREEN_WIDTH / 2) + getbegx(w));

    WINDOW_PTR w_flagsptr( w_flags );

    input_context ctxt("NEW_CHAR_SCENARIOS");
    ctxt.register_cardinal();
    ctxt.register_action("CONFIRM");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("SORT");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("FILTER");

    bool recalc_scens = true;
    int scens_length = 0;
    std::string filterstring;
    std::vector<const scenario *> sorted_scens;

    do {
        if (recalc_scens) {
            sorted_scens.clear();
            for (scenmap::const_iterator iter = scenario::begin(); iter != scenario::end(); ++iter) {
                if (!lcmatch(iter->second.gender_appropriate_name(u->male), filterstring)) {
                    continue;
                }
                sorted_scens.push_back(&(iter->second));
            }
            scens_length = sorted_scens.size();
            if (scens_length == 0) {
                popup(_("Nothing found.")); // another case of black box in tiles
                filterstring.clear();
                continue;
            }

            // Sort scenarios by points.
            // scenario_display_sort() keeps "Evacuee" at the top.
            scenario_sorter.male = u->male;
            std::stable_sort(sorted_scens.begin(), sorted_scens.end(), scenario_sorter);

            // Select the current scenario, if possible.
            for (int i = 0; i < scens_length; ++i) {
                if (sorted_scens[i]->ident() == g->scen->ident()) {
                    cur_id = i;
                    break;
                }
            }
            if (cur_id > scens_length - 1) {
                cur_id = 0;
            }

            // Draw filter indicator
            for (int i = 1; i < FULL_SCREEN_WIDTH - 1; i++) {
                mvwputch(w, FULL_SCREEN_HEIGHT - 1, i, BORDER_COLOR, LINE_OXOX);
            }
            const auto filter_indicator = filterstring.empty() ? _("no filter")
                                          : filterstring;
            mvwprintz(w, getmaxy(w) - 1, 2, c_ltgray, "<%s>", filter_indicator.c_str());

            recalc_scens = false;
        }

        int netPointCost = sorted_scens[cur_id]->point_cost() - g->scen->point_cost();
        bool can_pick = sorted_scens[cur_id]->can_pick(points);
        const std::string empty_line(getmaxx(w_description), ' ');

        // Clear the bottom of the screen and header.
        werase(w_description);
        mvwprintz(w, 3, 1, c_ltgray, empty_line.c_str());

        int pointsForScen = sorted_scens[cur_id]->point_cost();
        bool negativeScen = pointsForScen < 0;
        if (negativeScen) {
            pointsForScen *= -1;
        }

        // Draw header.
        std::string points_msg = string_format(_("Points left: %2d"), points);
        int pMsg_length = utf8_width(_(points_msg.c_str()));
        if (netPointCost > 0) {
            mvwprintz(w, 3, 2, c_ltgray, _(points_msg.c_str()));
            mvwprintz(w, 3, pMsg_length + 2, c_red, "(-%d)", abs(netPointCost));
        } else if (netPointCost == 0) {
            mvwprintz(w, 3, 2, c_ltgray, _(points_msg.c_str()));
        } else {
            mvwprintz(w, 3, 2, c_ltgray, _(points_msg.c_str()));
            mvwprintz(w, 3, pMsg_length + 2, c_green, "(+%d)", abs(netPointCost));
        }

        std::string scen_msg_temp;
        if (negativeScen) {
            //~ 1s - scenario name, 2d - current character points.
            scen_msg_temp = ngettext("Scenario %1$s earns %2$d point",
                                     "Scenario %1$s earns %2$d points",
                                     pointsForScen);
        } else {
            //~ 1s - scenario name, 2d - current character points.
            scen_msg_temp = ngettext("Scenario %1$s costs %2$d point",
                                     "Scenario %1$s cost %2$d points",
                                     pointsForScen);
        }
        ///* This string has fixed start pos(7 = 2(start) + 5(length of "(+%d)" and space))
        mvwprintz(w, 3, pMsg_length + 7, can_pick ? c_green : c_ltred, scen_msg_temp.c_str(),
                  _(sorted_scens[cur_id]->gender_appropriate_name(u->male).c_str()),
                  pointsForScen);

        fold_and_print(w_description, 0, 0, FULL_SCREEN_WIDTH - 2, c_green,
                       _(sorted_scens[cur_id]->description(u->male).c_str()));

        //Draw options
        calcStartPos(iStartPos, cur_id, iContentHeight, scens_length);
        const int end_pos = iStartPos + ((iContentHeight > scens_length) ?
                            scens_length : iContentHeight);
        int i;
        for (i = iStartPos; i < end_pos; i++) {
            mvwprintz(w, 5 + i - iStartPos, 2, c_ltgray, "\
                                             ");
            nc_color col;
            if (g->scen != sorted_scens[i]) {
                col = (sorted_scens[i] == sorted_scens[cur_id] ? h_ltgray : c_ltgray);
            } else {
                col = (sorted_scens[i] == sorted_scens[cur_id] ? hilite(COL_SKILL_USED) : COL_SKILL_USED);
            }
            mvwprintz(w, 5 + i - iStartPos, 2, col,
                      _(sorted_scens[i]->gender_appropriate_name(u->male).c_str()));

        }
        //Clear rest of space in case stuff got filtered out
        for (; i < iStartPos + iContentHeight; ++i) {
            mvwprintz(w, 5 + i - iStartPos, 2, c_ltgray, "\
                                             "); // Clear the line
        }

        std::vector<std::string> scen_items = sorted_scens[cur_id]->items();
        std::vector<std::string> scen_gender_items;

        if (u->male) {
            scen_gender_items = sorted_scens[cur_id]->items_male();
        } else {
            scen_gender_items = sorted_scens[cur_id]->items_female();
        }
        scen_items.insert( scen_items.end(), scen_gender_items.begin(), scen_gender_items.end() );
        werase(w_sorting);
        werase(w_profession);
        werase(w_location);
        werase(w_flags);

        draw_sorting_indicator(w_sorting, ctxt, scenario_sorter);

        mvwprintz(w_profession, 0, 0, COL_HEADER, _("Professions:"));
        wprintz(w_profession, c_ltgray, _("\n"));
        if (sorted_scens[cur_id]->profsize() > 0) {
            wprintz(w_profession, c_ltgray, _("Limited"));
        } else {
            wprintz(w_profession, c_ltgray, _("All"));
        }
        wprintz(w_profession, c_ltgray, _(", default:\n"));
        auto const scenario_prof = sorted_scens[cur_id]->get_profession();
        auto const prof_points = scenario_prof->point_cost();
        auto const prof_color = prof_points > 0 ? c_green : c_ltgray;
        wprintz(w_profession, prof_color, scenario_prof->gender_appropriate_name(u->male).c_str());
        if ( prof_points > 0 ) {
            wprintz(w_profession, c_green, " (+%d)", prof_points);
        }

        mvwprintz(w_location, 0, 0, COL_HEADER, _("Scenario Location:"));
        wprintz(w_location, c_ltgray, ("\n"));
        wprintz(w_location, c_ltgray, _(sorted_scens[cur_id]->start_name().c_str()));

        mvwprintz(w_flags, 0, 0, COL_HEADER, _("Scenario Flags:"));
        wprintz(w_flags, c_ltgray, ("\n"));

        if ( sorted_scens[cur_id]->has_flag("SPR_START")) {
            wprintz(w_flags, c_ltgray, _("Spring start"));
            wprintz(w_flags, c_ltgray, ("\n"));
        } else if ( sorted_scens[cur_id]->has_flag("SUM_START")) {
            wprintz(w_flags, c_ltgray, _("Summer start"));
            wprintz(w_flags, c_ltgray, ("\n"));
        } else if ( sorted_scens[cur_id]->has_flag("AUT_START")) {
            wprintz(w_flags, c_ltgray, _("Autumn start"));
            wprintz(w_flags, c_ltgray, ("\n"));
        } else if ( sorted_scens[cur_id]->has_flag("WIN_START")) {
            wprintz(w_flags, c_ltgray, _("Winter start"));
            wprintz(w_flags, c_ltgray, ("\n"));
        }

        if ( sorted_scens[cur_id]->has_flag("INFECTED") ) {
            wprintz(w_flags, c_ltgray, _("Infected player"));
            wprintz(w_flags, c_ltgray, ("\n"));
        }
        if ( sorted_scens[cur_id]->has_flag("BAD_DAY") ) {
            wprintz(w_flags, c_ltgray, _("Drunk and sick player"));
            wprintz(w_flags, c_ltgray, ("\n"));
        }
        if ( sorted_scens[cur_id]->has_flag("FIRE_START") ) {
            wprintz(w_flags, c_ltgray, _("Fire nearby"));
            wprintz(w_flags, c_ltgray, ("\n"));
        }
        if ( sorted_scens[cur_id]->has_flag("SUR_START") ) {
            wprintz(w_flags, c_ltgray, _("Zombies nearby"));
            wprintz(w_flags, c_ltgray, ("\n"));
        }
        if ( sorted_scens[cur_id]->has_flag("HELI_CRASH") ) {
            wprintz(w_flags, c_ltgray, _("Various limb wounds"));
            wprintz(w_flags, c_ltgray, ("\n"));
        }

        draw_scrollbar(w, cur_id, iContentHeight, scens_length, 5);
        wrefresh(w);
        wrefresh(w_description);
        wrefresh(w_sorting);
        wrefresh(w_profession);
        wrefresh(w_location);
        wrefresh(w_flags);

        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            cur_id++;
            if (cur_id > scens_length - 1) {
                cur_id = 0;
            }
        } else if (action == "UP") {
            cur_id--;
            if (cur_id < 0) {
                cur_id = scens_length - 1;
            }
        } else if (action == "CONFIRM") {
            u->start_location = sorted_scens[cur_id]->start_location();
            u->str_max = 8;
            u->dex_max = 8;
            u->int_max = 8;
            u->per_max = 8;
            g->scen = scenario::scen(sorted_scens[cur_id]->ident());
            u->prof = g->scen->get_profession();
            u->empty_traits();
            u->empty_skills();
            u->add_traits();
            points = OPTIONS["INITIAL_POINTS"] - sorted_scens[cur_id]->point_cost();


        } else if (action == "PREV_TAB" && query_yn(_("Return to main menu?"))) {
            return -1;
        } else if (action == "NEXT_TAB") {
            retval = 1;
        } else if (action == "SORT") {
            scenario_sorter.sort_by_points = !scenario_sorter.sort_by_points;
            recalc_scens = true;
        } else if (action == "FILTER") {
            filterstring = string_input_popup(_("Search:"), 60, filterstring,
                _("Search by scenario name."));
            recalc_scens = true;
        }
    } while (retval == 0);

    return retval;
}

int set_description(WINDOW *w, player *u, character_type type, int &points)
{
    if (PLTYPE_NOW == type) {
        return 1;
    }

    draw_tabs(w, _("DESCRIPTION"));

    WINDOW *w_name = newwin(2, 42, getbegy(w) + 5, getbegx(w) + 2);
    WINDOW_PTR w_nameptr( w_name );
    WINDOW *w_gender = newwin(2, 33, getbegy(w) + 5, getbegx(w) + 46);
    WINDOW_PTR w_genderptr( w_gender );
    WINDOW *w_location = newwin(1, 76, getbegy(w) + 7, getbegx(w) + 2);
    WINDOW_PTR w_locationptr( w_location );
    WINDOW *w_stats = newwin(6, 16, getbegy(w) + 9, getbegx(w) + 2);
    WINDOW_PTR w_statstptr( w_stats );
    WINDOW *w_traits = newwin(13, 24, getbegy(w) + 9, getbegx(w) + 22);
    WINDOW_PTR w_traitsptr( w_traits );
    WINDOW *w_scenario = newwin(1, 33, getbegy(w) + 9, getbegx(w) + 46);
    WINDOW_PTR w_scenarioptr( w_scenario );
    WINDOW *w_profession = newwin(1, 33, getbegy(w) + 10, getbegx(w) + 46);
    WINDOW_PTR w_professionptr( w_profession );
    WINDOW *w_skills = newwin(9, 33, getbegy(w) + 11, getbegx(w) + 46);
    WINDOW_PTR w_skillsptr( w_skills );
    WINDOW *w_guide = newwin(4, FULL_SCREEN_WIDTH - 3, getbegy(w) + 19, getbegx(w) + 2);
    WINDOW_PTR w_guideptr( w_guide );

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
    ctxt.register_action("REROLL_CHARACTER");
    ctxt.register_action("REROLL_CHARACTER_WITH_SCENARIO");
    ctxt.register_action("ANY_INPUT");

    uimenu select_location;
    select_location.text = _("Select a starting location.");
    int offset = 0;
    for( location_map::iterator loc = start_location::begin();
         loc != start_location::end(); ++loc) {
        if (g->scen->allowed_start(loc->second.ident()) || g->scen->has_flag("ALL_STARTS")) {
            select_location.entries.push_back( uimenu_entry( _( loc->second.name().c_str() ) ) );
            if( loc->second.ident() == u->start_location ) {
                select_location.selected = offset;
            }
            offset++;
        }
    }
    select_location.setup();
    if(MAP_SHARING::isSharing()) {
        u->name = MAP_SHARING::getUsername();  // set the current username as default character name
    } else if (OPTIONS["DEF_CHAR_NAME"]) {
        u->name = OPTIONS["DEF_CHAR_NAME"].getValue();
    }
    do {
        if (redraw) {
            //Draw the line between editable and non-editable stuff.
            for (int i = 0; i < getmaxx(w); ++i) {
                if (i == 0) {
                    mvwputch(w, 8, i, BORDER_COLOR, LINE_XXXO);
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
                pos = (utf8_width(vStatNames[i]) > pos ?
                       utf8_width(vStatNames[i]) : pos);
                mvwprintz(w_stats, i + 1, 0, c_ltgray, vStatNames[i].c_str());
            }
            mvwprintz(w_stats, 1, pos + 1, c_ltgray, "%2d", u->str_max);
            mvwprintz(w_stats, 2, pos + 1, c_ltgray, "%2d", u->dex_max);
            mvwprintz(w_stats, 3, pos + 1, c_ltgray, "%2d", u->int_max);
            mvwprintz(w_stats, 4, pos + 1, c_ltgray, "%2d", u->per_max);
            wrefresh(w_stats);

            mvwprintz(w_traits, 0, 0, COL_HEADER, _("Traits: "));
            std::vector<std::string> current_traits = u->get_base_traits();
            if (current_traits.empty()) {
                wprintz(w_traits, c_ltred, _("None!"));
            } else {
                for( auto &current_trait : current_traits ) {
                    wprintz(w_traits, c_ltgray, "\n");
                    wprintz( w_traits, ( mutation_branch::get( current_trait ).points > 0 ) ? c_ltgreen : c_ltred,
                             mutation_branch::get_name( current_trait ).c_str() );
                }
            }
            wrefresh(w_traits);

            mvwprintz(w_skills, 0, 0, COL_HEADER, _("Skills:"));

            auto skillslist = Skill::get_skills_sorted_by([&](Skill const& a, Skill const& b) {
                int const level_a = u->skillLevel(a).exercised_level();
                int const level_b = u->skillLevel(b).exercised_level();
                return level_a > level_b || (level_a == level_b && a.name() < b.name());
            });

            int line = 1;
            bool has_skills = false;
            profession::StartingSkillList list_skills = u->prof->skills();
            for( auto &elem : skillslist ) {
                int level = int( u->skillLevel( elem ) );
                profession::StartingSkillList::iterator i = list_skills.begin();
                while (i != list_skills.end()) {
                    if( i->first == ( elem )->ident() ) {
                        level += i->second;
                        break;
                    }
                    ++i;
                }

                if (level > 0) {
                    mvwprintz( w_skills, line, 0, c_ltgray, "%s",
                               ( ( elem )->name() + ":" ).c_str() );
                    mvwprintz(w_skills, line, 23, c_ltgray, "%-2d", (int)level);
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

            mvwprintz(w_guide, 2, 0, c_green,
                      _("Press %s to finish character creation or %s to go back."),
                      ctxt.get_desc("NEXT_TAB").c_str(),
                      ctxt.get_desc("PREV_TAB").c_str());
            if( type == PLTYPE_RANDOM || type == PLTYPE_RANDOM_WITH_SCENARIO ) {
                    mvwprintz( w_guide, 0, 0, c_green,
                               _("Press %s to save character template, %s to re-roll or %s for random scenario."),
                               ctxt.get_desc("SAVE_TEMPLATE").c_str(),
                               ctxt.get_desc("REROLL_CHARACTER").c_str(),
                               ctxt.get_desc("REROLL_CHARACTER_WITH_SCENARIO").c_str());
            } else {
                    mvwprintz(w_guide, 0, 0, c_green, _("Press %s to save a template of this character."),
                    ctxt.get_desc("SAVE_TEMPLATE").c_str());
            }
            wrefresh(w_guide);

            redraw = false;
        }

        //We draw this stuff every loop because this is user-editable
        mvwprintz(w_name, 0, 0, c_ltgray, _("Name:"));
        mvwprintz(w_name, 0, namebar_pos, c_ltgray, "_______________________________");
        mvwprintz(w_name, 0, namebar_pos, c_white, "%s", u->name.c_str());
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
        const int prompt_offset = utf8_width( location_prompt );
        werase(w_location);
        mvwprintz( w_location, 0, 0, c_ltgray, location_prompt.c_str() );
        mvwprintz( w_location, 0, prompt_offset + 1, c_ltgray, _("Starting location:") );
        // ::find will return empty location if id was not found. Debug msg will be printed too.
        mvwprintz( w_location, 0, prompt_offset + utf8_width(_("Starting location:")) + 2,
                   c_ltgray, _(start_location::find(u->start_location)->name().c_str()));
        wrefresh(w_location);

        werase(w_scenario);
        mvwprintz(w_scenario, 0, 0, COL_HEADER, _("Scenario: "));
        wprintz(w_scenario, c_ltgray, _(g->scen->gender_appropriate_name(u->male).c_str()));
        wrefresh(w_scenario);

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
                    return 1;
                }
            } else if (query_yn(_("Are you SURE you're finished?"))) {
                return 1;
            } else {
                redraw = true;
                continue;
            }
        } else if (action == "PREV_TAB") {
            return -1;
        } else if (action == "REROLL_CHARACTER" && (type == PLTYPE_RANDOM || type == PLTYPE_RANDOM_WITH_SCENARIO)) {
            return -7;
        } else if (action == "REROLL_CHARACTER_WITH_SCENARIO" && (type == PLTYPE_RANDOM || type == PLTYPE_RANDOM_WITH_SCENARIO)) {
            return -8;
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
            if(!MAP_SHARING::isSharing()) { // Don't allow random names when sharing maps. We don't need to check at the top as you won't be able to edit the name
                u->pick_name();
            }
        } else if (action == "CHANGE_GENDER") {
            u->male = !u->male;
        } else if ( action == "CHOOSE_LOCATION" ) {
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
        } else if (action == "ANY_INPUT" &&
                   !MAP_SHARING::isSharing()) {  // Don't edit names when sharing maps
            const long ch = ctxt.get_raw_input().get_first_input();
            utf8_wrapper wrap(u->name);
            if( ch == KEY_BACKSPACE ) {
                if( !wrap.empty() ) {
                    wrap.erase( wrap.length() - 1, 1 );
                    u->name = wrap.str();
                }
            } else if(ch == KEY_F(2)) {
                utf8_wrapper tmp(get_input_string_from_file());
                if(!tmp.empty() && tmp.length() + wrap.length() < 30) {
                    u->name.append(tmp.str());
                }
            } else if( ch == '\n' ) {
                // nope, we ignore this newline, don't want it in char names
            } else {
                wrap.append( ctxt.get_raw_input().text );
                u->name = wrap.str();
            }
        }
    } while (true);
}

std::vector<std::string> Character::get_base_traits() const
{
    return std::vector<std::string>( my_traits.begin(), my_traits.end() );
}

std::vector<std::string> Character::get_mutations() const
{
    std::vector<std::string> result;
    for( auto &t : my_mutations ) {
        result.push_back( t.first );
    }
    return result;
}

void Character::empty_traits()
{
    my_traits.clear();
    my_mutations.clear();
}
void Character::empty_skills()
{
    for( auto &skill : Skill::skills ) {
        SkillLevel &level = skillLevel( skill );
        level.level(0);
    }
}
void Character::add_traits()
{
    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( g->scen->locked_traits( traits_iter.first ) ) {
            toggle_trait( traits_iter.first );
        }
    }
}
std::string Character::random_good_trait()
{
    std::vector<std::string> vTraitsGood;

    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( traits_iter.second.startingtrait && traits_iter.second.points >= 0 ) {
            vTraitsGood.push_back( traits_iter.first );
        }
    }

    return random_entry( vTraitsGood );
}

std::string Character::random_bad_trait()
{
    std::vector<std::string> vTraitsBad;

    for( auto &traits_iter : mutation_branch::get_all() ) {
        if( traits_iter.second.startingtrait && traits_iter.second.points < 0 ) {
            vTraitsBad.push_back( traits_iter.first );
        }
    }

    return random_entry( vTraitsBad );
}

void save_template(player *u)
{
    std::string title = _("Name of template:");
    std::string name = string_input_popup( title, FULL_SCREEN_WIDTH - utf8_width(title) - 8 );
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
