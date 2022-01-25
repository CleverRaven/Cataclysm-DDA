#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <set>
#include <string>
#include <utility>

#include "action.h"
#include "addiction.h"
#include "avatar_action.h"
#include "character.h"
#include "display.h"
#include "effect.h"
#include "flag.h"
#include "game.h"
#include "output.h"
#include "ui_manager.h"
#include "vitamin.h"
#include "weather.h"

static const efftype_id effect_bandaged("bandaged");
static const efftype_id effect_bite("bite");
static const efftype_id effect_bleed("bleed");
static const efftype_id effect_infected("infected");
static const efftype_id effect_disinfected("disinfected");
static const efftype_id effect_mending("mending");
static const efftype_id effect_hypovolemia("hypovolemia");

static const trait_id trait_NOPAIN("NOPAIN");
static const trait_id trait_TROGLO("TROGLO");
static const trait_id trait_TROGLO2("TROGLO2");
static const trait_id trait_TROGLO3("TROGLO3");

static const vitamin_id vitamin_blood("blood");

enum class medical_tab_mode {
    TAB_SUMMARY
};

static std::string coloured_stat_display(int statCur, int statMax) {
    nc_color cstatus;
    if (statCur <= 0) {
        cstatus = c_dark_gray;
    }
    else if (statCur < statMax / 2) {
        cstatus = c_red;
    }
    else if (statCur < statMax) {
        cstatus = c_light_red;
    }
    else if (statCur == statMax) {
        cstatus = c_white;
    }
    else if (statCur < statMax * 1.5) {
        cstatus = c_light_green;
    }
    else {
        cstatus = c_green;
    }
    std::string cur = colorize(string_format(_("%2d"), statCur), cstatus);
    return string_format(_("%s (%s)"), cur, statMax);
}

static void draw_medical_titlebar(const catacurses::window& window, avatar* player,
    const int WIDTH)
{
    input_context ctxt("MEDICAL", keyboard_mode::keychar);
    const Character& you = *player->as_character();

    werase(window);
    draw_border(window, BORDER_COLOR, _(" MEDICAL "));

    /* Window Title */
    center_print(window, 0, c_blue, _(" MEDICAL "));

    /* Tabs */
    const std::vector<std::pair<medical_tab_mode, std::string>> tabs = {
        { medical_tab_mode::TAB_SUMMARY, string_format(_("SUMMARY")) }
    };
    draw_tabs(window, tabs, medical_tab_mode::TAB_SUMMARY);

    const int TAB_WIDTH = 12;

    // Draw symbols to connect additional lines to border

    int width = getmaxx(window);
    int height = getmaxy(window);
    for (int i = 1; i < height - 1; ++i) {
        // |
        mvwputch(window, point(0, i), BORDER_COLOR, LINE_XOXO);
        // |
        mvwputch(window, point(width - 1, i), BORDER_COLOR, LINE_XOXO);
    }
    // |-
    mvwputch(window, point(0, height - 1), BORDER_COLOR, LINE_XXXO);
    // -|
    mvwputch(window, point(width - 1, height - 1), BORDER_COLOR, LINE_XOXX);

    int right_indent = 2;

    /* Pain Indicator */
    auto pain_descriptor = display::pain_text_color(*player);
    if (pain_descriptor.first.size() > 0) {
        const std::string pain_str = "In " + pain_descriptor.first;

        const int pain_str_pos = right_print(window, 1, right_indent, pain_descriptor.second, pain_str);

        /* ~~Borders */
        for (int i = 1; i < getmaxy(window) - 1; i++) {
            mvwputch(window, point(pain_str_pos - 2, i), BORDER_COLOR, LINE_XOXO); // |
        }
        mvwputch(window, point(pain_str_pos - 2, 0), BORDER_COLOR, LINE_OXXX); // ^|^   
        mvwputch(window, point(pain_str_pos - 2, 2), BORDER_COLOR, LINE_XXOX); // _|_
        right_indent += utf8_width(remove_color_tags(pain_str)) + 3;
    }

    /* Hunger, Thirst & Fatigue Indicator */

    const std::pair<std::string, nc_color> hunger_pair = display::hunger_text_color(you);
    const std::pair<std::string, nc_color> thirst_pair = display::thirst_text_color(you);
    const std::pair<std::string, nc_color> fatigue_pair = display::fatigue_text_color(you);
    int cur_str_pos = 0;

    // Hunger
    if (hunger_pair.first.size() > 0) {
        cur_str_pos = right_print(window, 1, right_indent, hunger_pair.second, hunger_pair.first);

        /* ~~Borders */
        for (int i = 1; i < getmaxy(window) - 1; i++) {
            mvwputch(window, point(cur_str_pos - 2, i), BORDER_COLOR, LINE_XOXO); // |
        }
        mvwputch(window, point(cur_str_pos - 2, 0), BORDER_COLOR, LINE_OXXX); // ^|^   
        mvwputch(window, point(cur_str_pos - 2, 2), BORDER_COLOR, LINE_XXOX); // _|_

        right_indent += utf8_width(hunger_pair.first) + 3;
    }

    // Thirst
    if (thirst_pair.first.size() > 0) {
        cur_str_pos = right_print(window, 1, right_indent, thirst_pair.second, thirst_pair.first);

        /* ~~Borders */
        for (int i = 1; i < getmaxy(window) - 1; i++) {
            mvwputch(window, point(cur_str_pos - 2, i), BORDER_COLOR, LINE_XOXO); // |
        }
        mvwputch(window, point(cur_str_pos - 2, 0), BORDER_COLOR, LINE_OXXX); // ^|^   
        mvwputch(window, point(cur_str_pos - 2, 2), BORDER_COLOR, LINE_XXOX); // _|_

        right_indent += utf8_width(thirst_pair.first) + 3;
    }

    //Fatigue
    if (fatigue_pair.first.size() > 0) {
        cur_str_pos = right_print(window, 1, right_indent, fatigue_pair.second, fatigue_pair.first);

        /* ~~Borders */
        for (int i = 1; i < getmaxy(window) - 1; i++) {
            mvwputch(window, point(cur_str_pos - 2, i), BORDER_COLOR, LINE_XOXO); // |
        }
        mvwputch(window, point(cur_str_pos - 2, 0), BORDER_COLOR, LINE_OXXX); // ^|^   
        mvwputch(window, point(cur_str_pos - 2, 2), BORDER_COLOR, LINE_XXOX); // _|_

        right_indent += utf8_width(fatigue_pair.first) + 3;
    }

    /* Hotkey Helper */
    std::string desc;
    desc = string_format(_(
        "[<color_yellow>%s</color>]Apply Item [<color_yellow>%s</color>]Keybindings"),
        ctxt.get_desc("APPLY"), ctxt.get_desc("HELP_KEYBINDINGS"));

    const int max_width = right_indent + TAB_WIDTH;
    if (WIDTH - max_width > 3) // If the window runs out of room, we won't print keybindings.
    {
        right_print(window, 1, right_indent, c_white, desc);
    }
}

void avatar::disp_medical()
{
    medical_tab_mode tab_mode = medical_tab_mode::TAB_SUMMARY;
    const Character& you = *this->as_character();

    /* Title Bar - Tabs, Pain Indicator & Blood Indicator */
    catacurses::window w_title;
    /* Primary Window */
    catacurses::window wMedical;
    /* Bottom detail bar, for printing selection_line.description() */
    catacurses::window w_description;

    const int TITLE_W_HEIGHT = 3;
    const int DESC_W_HEIGHT = 6; // Consistent with Player Info (@) Menu
    const int TEXT_START_Y = TITLE_W_HEIGHT;
    int DESC_W_BEGIN = 0;

    int HEIGHT = 0;
    int WIDTH = 0;
    int second_column_x = 0;
    int third_column_x = 0;

    point cursor(0, 0);
    int selected_column_rows = 0;

    class selection_line
    {
    public:
        selection_line() = default;
        selection_line(const std::string& header_str, const std::string& desc_str)
            : header_str(header_str), desc_str(desc_str) {}

        int print(const catacurses::window& win, const point begin, const int& width, const nc_color& base_color, bool selected)
        {
            if (selected) {
                std::string highlight = colorize(">", h_white);
                header_str = highlight + header_str;
            }
            return linecount = fold_and_print(win, begin, width, base_color, header_str);
        }

        std::string description() {
            return desc_str;
        }

        int get_line_count() {
            return linecount;
        }

    private:
        std::string header_str;
        std::string desc_str;
        int linecount;
    };

    ui_adaptor ui;
    ui.on_screen_resize([&](ui_adaptor& ui) {
        HEIGHT = std::min(TERMY,
            std::max(FULL_SCREEN_HEIGHT,
                TITLE_W_HEIGHT + DESC_W_HEIGHT + 5));
        WIDTH = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 5;

        const point win((TERMX - WIDTH) / 2, (TERMY - HEIGHT) / 2);

        wMedical = catacurses::newwin(HEIGHT, WIDTH, win);

        w_title = catacurses::newwin(TITLE_W_HEIGHT, WIDTH, win);

        DESC_W_BEGIN = HEIGHT - DESC_W_HEIGHT - 1;
        w_description = catacurses::newwin(DESC_W_HEIGHT, WIDTH - 2,
            point(win.x + 1, DESC_W_BEGIN + win.y));

        second_column_x = 20 + (TERMX - FULL_SCREEN_WIDTH) / 6;
        third_column_x = second_column_x + 8 + (TERMX - FULL_SCREEN_WIDTH) / 6;

        ui.position_from_window(wMedical);
        });

    ui.mark_resize();

    std::vector<selection_line> SummaryLines;
    std::vector<selection_line> EffectLines;
    std::vector<selection_line> StatLines;
    selection_line highlightline;

    ui.on_redraw([&](const ui_adaptor&) {
        werase(wMedical);

        draw_border(wMedical, BORDER_COLOR, _(" MEDICAL "));
        mvwputch(wMedical, point(getmaxx(wMedical) - 1, 2), BORDER_COLOR, LINE_XOXX); // -|

        wnoutrefresh(wMedical);

        draw_medical_titlebar(w_title, this, WIDTH);
        mvwputch(w_title, point(second_column_x - 2, TEXT_START_Y - 1), BORDER_COLOR, LINE_OXXX); // ^|^ 
        mvwputch(w_title, point(third_column_x - 2, TEXT_START_Y - 1), BORDER_COLOR, LINE_OXXX); // ^|^ 
        wnoutrefresh(w_title);


        /* FIRST COLUUMN - HEALTH */

        SummaryLines.clear();

        // Header
        fold_and_print(wMedical, point(2, TEXT_START_Y), WIDTH - 2, c_light_blue, "HEALTH");

        const int left_padding = 1; // including border
        const int right_padding = 1; // including border
        const int SUMMARY_WIDTH = second_column_x - left_padding; // For nicely folding multiple tags.
        int linerow = 0; // Current row of TEXT to print
        int selection_row = 0; // Current index of selection vector




        for (const bodypart_id& part : this->get_all_body_parts(get_body_part_flags::sorted)) {

            std::string header; // Bodypart Title
            std::string hp_str; // Bodypart HP
            std::string detail;
            std::string description;


            // Colorized strings for each status
            std::vector<std::string> color_strings;
            widget_id wid("bodypart_status_indicator_template");
            for (const auto& sc : display::bodypart_status_colors(you, part, "bodypart_status_indicator_template")) {
                std::string txt = io::enum_to_string(sc.first);
                if (wid.is_valid()) {
                    translation t = widget_phrase::get_text_for_id(txt, wid);
                    txt = to_upper_case(t.empty() ? txt : t.translated());
                }
                color_strings.emplace_back(string_format("[ %s ]", colorize(txt, sc.second)));
            }
            detail += join(color_strings, " ");

            bool no_feeling = this->is_avatar() && this->has_trait(trait_NOPAIN);
            const int maximal_hp = this->get_part_hp_max(part);
            const int current_hp = this->get_part_hp_cur(part);
            const bool limb_is_broken = this->is_limb_broken(part);
            const bool limb_is_mending = this->worn_with_flag(flag_SPLINT, part);

            if (limb_is_mending) {
                detail += colorize(_("Splinted"),
                    c_blue);
                if (no_feeling) {
                    hp_str = colorize("==%==", c_blue);
                }
                else {
                    const auto& eff = this->get_effect(effect_mending, part);
                    const int mend_perc = eff.is_null() ? 0.0 : 100 * eff.get_duration() / eff.get_max_duration();

                    const int num = mend_perc / 20;
                    hp_str = colorize(std::string(num, '#') + std::string(5 - num, '='), c_blue);
                }
            }
            else if (limb_is_broken) {
                detail += colorize(_("Broken"), c_red);
                hp_str = "==%==";
            }
            else if (no_feeling) {
                if (current_hp < maximal_hp * 0.25) {
                    hp_str = colorize(_("Very Bad"), c_red);
                }
                else if (current_hp < maximal_hp * 0.5) {
                    hp_str = colorize(_("Bad"), c_light_red);
                }
                else if (current_hp < maximal_hp * 0.75) {
                    hp_str = colorize(_("Okay"), c_light_green);
                }
                else {
                    hp_str = colorize(_("Good"), c_green);
                }
            }
            else {
                std::pair<std::string, nc_color> h_bar = get_hp_bar(current_hp, maximal_hp, false);
                hp_str = colorize(h_bar.first, h_bar.second) +
                    colorize(std::string(5 - utf8_width(h_bar.first), '.'), c_white);
            }
            header += colorize(uppercase_first_letter(body_part_name(part, 1)), display::limb_color(*this, part, true, true, true)) + " " + hp_str;

            /* Descriptions */

            bool bleeding = this->has_effect(effect_bleed, part.id());
            bool bitten = this->has_effect(effect_bite, part.id());
            bool infected = this->has_effect(effect_infected, part.id());
            bool bandaged = this->has_effect(effect_bandaged, part.id());

            // BLEEDING block
            if (bleeding) {
                const int bleed_intensity = this->get_effect_int(effect_bleed, part);
                const effect bleed_effect = this->get_effect(effect_bleed, part);
                description += string_format("[ %s ] - %s\n",
                    colorize(bleed_effect.get_speed_name(), colorize_bleeding_intensity(bleed_intensity)),
                    bleed_effect.disp_short_desc());
            }

            // BITTEN block
            if (bitten) {
                const effect bite_effect = this->get_effect(effect_bite, part);
                description += string_format("[ %s ] - %s\n",
                    colorize(bite_effect.get_speed_name(), c_red),
                    bite_effect.disp_short_desc());
            }

            // INFECTED block
            if (infected) {
                const effect infected_effect = this->get_effect(effect_infected, part);
                description += string_format("[ %s ] - %s\n",
                    colorize(infected_effect.get_speed_name(), c_light_green),
                    infected_effect.disp_short_desc());
            }

            selection_line line;

            if (detail.length() > 0) {
                const std::string header_str = string_format("[%s] - %s", header, detail);
                std::vector<std::string> textformatted = foldstring(header_str, SUMMARY_WIDTH - 2, (char)93);
                const int lineCount = textformatted.size();
                if (lineCount <= 1) {
                    line = selection_line(header_str, description);
                }
                else {
                    //If there are too many tags, display them neatly on a new line.
                    std::string print_line = string_format("%s\n", textformatted[0]);
                    for (int i = 1; i < lineCount; i++) {
                        if (i != lineCount) {
                            print_line += string_format("->%s\n", textformatted[i]);
                        }
                        else {
                            print_line += string_format("->%s", textformatted[i]);
                        }
                    }
                    line = selection_line(print_line, description);
                }
            }
            else {
                line = selection_line(string_format("[%s]", header), description);
            }

            SummaryLines.emplace_back(line);
        }



        /* Print Lines */


        for (selection_line& line : SummaryLines) {
            const bool is_highlighted = cursor == point(0, selection_row);
            if (is_highlighted) { highlightline = line; }
            linerow += line.print(wMedical, point(left_padding, TEXT_START_Y + 2 + linerow), SUMMARY_WIDTH, c_light_gray, is_highlighted);
            ++selection_row;
        }

        /* SECOND COLUMN - EFFECTS */
        mvwprintz(wMedical, point(second_column_x, TEXT_START_Y), c_light_blue, _("EFFECTS"));

        EffectLines.clear();

        for (auto& elem : *effects) {
            for (auto& _effect_it : elem.second) {
                const std::string name = _effect_it.second.disp_name();
                if (name.empty()) {
                    continue;
                }
                EffectLines.emplace_back(selection_line(name, _effect_it.second.disp_desc()));
            }
        }

        const float bmi = get_bmi();

        if (bmi < character_weight_category::underweight) {
            std::string starvation_name;
            std::string starvation_text;

            if (bmi < character_weight_category::emaciated) {
                starvation_name = _("Severely Malnourished");
                starvation_text =
                    _("Your body is severely weakened by starvation.  You might die if you don't start eating regular meals!\n\n");
            }
            else {
                starvation_name = _("Malnourished");
                starvation_text =
                    _("Your body is weakened by starvation.  Only time and regular meals will help you recover.\n\n");
            }

            if (bmi < character_weight_category::underweight) {
                const float str_penalty = 1.0f - ((bmi - 13.0f) / 3.0f);
                starvation_text += std::string(_("Strength")) + " -" + string_format("%2.0f%%\n",
                    str_penalty * 100.0f);
                starvation_text += std::string(_("Dexterity")) + " -" + string_format("%2.0f%%\n",
                    str_penalty * 50.0f);
                starvation_text += std::string(_("Intelligence")) + " -" + string_format("%2.0f%%",
                    str_penalty * 50.0f);
            }

            EffectLines.emplace_back(selection_line(starvation_name, starvation_text));
        }

        if (has_trait(trait_TROGLO) && g->is_in_sunlight(pos()) &&
            get_weather().weather_id->sun_intensity >= sun_intensity_type::high) {
            EffectLines.emplace_back(selection_line("In Sunlight", "The sunlight irritates you.\n"));
        }
        else if (has_trait(trait_TROGLO2) && g->is_in_sunlight(pos())) {
            EffectLines.emplace_back(selection_line("In Sunlight", "The sunlight irritates you badly.\n"));
        }
        else if (has_trait(trait_TROGLO3) && g->is_in_sunlight(pos())) {
            EffectLines.emplace_back(selection_line("In Sunlight", "The sunlight irritates you terribly.\n"));
        }

        for (auto& elem : addictions) {
            if (elem.sated < 0_turns && elem.intensity >= MIN_ADDICTION_LEVEL) {
                EffectLines.emplace_back(selection_line(addiction_name(elem), addiction_text(elem)));
            }
        }

        /* Print Effect Lines */

        linerow = 0;
        selection_row = 0;
        if (EffectLines.size() == 0) { EffectLines.emplace_back(selection_line(colorize("None", c_dark_gray), "")); }
        for (selection_line& line : EffectLines) {
            const bool is_highlighted = cursor == point(1, selection_row);
            if (is_highlighted) { highlightline = line; }
            linerow += line.print(wMedical, point(second_column_x, TEXT_START_Y + 2 + linerow), third_column_x - second_column_x, c_light_gray, is_highlighted);
            ++selection_row;
        }

        /* THIRD COLUMN - Stats */
        StatLines.clear();

        mvwprintz(wMedical, point(third_column_x, TEXT_START_Y), c_light_blue, _("STATS"));

        std::string strength_str = coloured_stat_display(this->get_str(), this->get_str_base());
        StatLines.emplace_back(
            selection_line(string_format("Strength: %s", strength_str),
                "Strength affects your melee damage, the amount of weight you can carry, your total HP, "
                "your resistance to many diseases, and the effectiveness of actions which require brute force."));

        std::string dexterity_str = coloured_stat_display(this->get_dex(), this->get_dex_base());
        StatLines.emplace_back(
            selection_line(string_format("Dexterity: %s", dexterity_str),
                "Dexterity affects your chance to hit in melee combat, helps you steady your "
                "gun for ranged combat, and enhances many actions that require finesse."));

        std::string intelligence_str = coloured_stat_display(this->get_int(), this->get_int_base());
        StatLines.emplace_back(
            selection_line(string_format("Intelligence: %s", intelligence_str),
                "Intelligence is less important in most situations, but it is vital for more complex tasks like "
                "electronics crafting.  It also affects how much skill you can pick up from reading a book."));

        std::string perception_str = coloured_stat_display(this->get_per(), this->get_per_base());
        StatLines.emplace_back(
            selection_line(string_format("Perception: %s", perception_str),
                "Perception is the most important stat for ranged combat.  It's also used for "
                "detecting traps and other things of interest."));

        /* Print Stat Lines */

        linerow = 0;
        selection_row = 0;
        for (selection_line& line : StatLines) {
            const bool is_highlighted = cursor == point(2, selection_row);
            if (is_highlighted) { highlightline = line; }
            linerow += line.print(wMedical, point(third_column_x, TEXT_START_Y + 2 + linerow), WIDTH - third_column_x - right_padding, c_light_gray, is_highlighted);
            ++selection_row;
        }

        /* Update Cursor Boundary */

        switch (cursor.x) {
        case 0:
            selected_column_rows = SummaryLines.size();
            break;
        case 1:
            selected_column_rows = EffectLines.size();
            break;
        case 2:
            selected_column_rows = StatLines.size();
            break;
        }

        /* DESCRIPTION BAR */

        werase(w_description);
        // Number of display rows required by highlightline.description()
        const std::string desc_str = highlightline.description();
        std::vector<std::string> textformatted = foldstring(desc_str, WIDTH - 2, (char)32);
        // Beginning row of description text [1-3] (w_description)
        const int DESCRIPTION_TEXT_Y = DESC_W_HEIGHT - std::min(DESC_W_HEIGHT, static_cast<int>(textformatted.size()));
        // Actual position of the description bar (wMedical)
        int DESCRIPTION_WIN_OFFSET = DESC_W_BEGIN + DESCRIPTION_TEXT_Y;
        if (desc_str.size() > 0)
        {
            mvwputch(wMedical, point(0, DESCRIPTION_WIN_OFFSET - 1), BORDER_COLOR, LINE_XXXO);
            mvwhline(wMedical, point(1, DESCRIPTION_WIN_OFFSET - 1), LINE_OXOX, getmaxx(wMedical) - 2);
            mvwputch(wMedical, point(getmaxx(wMedical) - 1, DESCRIPTION_WIN_OFFSET - 1), BORDER_COLOR, LINE_XOXX);
            fold_and_print(w_description, point(1, DESCRIPTION_TEXT_Y), WIDTH - 2, c_light_gray, highlightline.description());
        }
        else {
            DESCRIPTION_WIN_OFFSET = getmaxy(wMedical);
        }

        wnoutrefresh(w_description);

        /* COLUMN BORDERS - Dependent on DESCRIPTION_WIN_OFFSET */
        //Second Column
        mvwvline(wMedical, point(second_column_x - 2, TEXT_START_Y), LINE_XOXO, DESCRIPTION_WIN_OFFSET - 4); // |
        mvwputch(wMedical, point(second_column_x - 2, DESCRIPTION_WIN_OFFSET - 1), BORDER_COLOR, LINE_XXOX); // _|_
        //Third Column
        mvwvline(wMedical, point(third_column_x - 2, TEXT_START_Y), LINE_XOXO, DESCRIPTION_WIN_OFFSET - 4); // |
        mvwputch(wMedical, point(third_column_x - 2, DESCRIPTION_WIN_OFFSET - 1), BORDER_COLOR, LINE_XXOX); // _|_

        wnoutrefresh(wMedical);
        });

    input_context ctxt("MEDICAL");
    ctxt.register_action("UP");
    ctxt.register_action("DOWN");
    ctxt.register_action("LEFT");
    ctxt.register_action("RIGHT");
    ctxt.register_action("APPLY");
    ctxt.register_action("HELP_KEYBINDINGS");
    ctxt.register_action("QUIT");

    std::string action;
    for (;; ) {
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int ch = ctxt.get_raw_input().get_first_input();

        if (action == "DOWN" || action == "UP") {
            const int step = cursor.y + (action == "DOWN" ? 1 : -1);
            if (step == -1) cursor.y = selected_column_rows - 1;
            else if (step > selected_column_rows - 1) cursor.y = 0;
            else { cursor.y = step; }
        }
        else if (action == "RIGHT" || action == "LEFT") {
            const int step = cursor.x + (action == "RIGHT" ? 1 : -1);
            if (step == -1) cursor.x = 2;
            else if (step == 3) cursor.x = 0;
            else { cursor.x = step; }

            switch (cursor.x) {
            case 0:
                selected_column_rows = SummaryLines.size();
                break;
            case 1:
                selected_column_rows = EffectLines.size();
                break;
            case 2:
                selected_column_rows = StatLines.size();
                break;
            }

            if (cursor.y > selected_column_rows - 1) {
                cursor.y = selected_column_rows - 1;
            }
        }
        else if (action == "APPLY") {
            avatar_action::use_item(*this);
        }
        else if (action == "QUIT") {
            break;
        }
    }
}
