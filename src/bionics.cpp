#include "player.h"
#include "action.h"
#include "game.h"
#include "map.h"
#include "debug.h"
#include "rng.h"
#include "input.h"
#include "item.h"
#include "bionics.h"
#include "line.h"
#include "json.h"
#include "messages.h"
#include "overmapbuffer.h"
#include "sounds.h"
#include "translations.h"
#include "catacharset.h"
#include "input.h"
#include "monster.h"
#include "overmap.h"
#include "itype.h"
#include "vehicle.h"
#include "field.h"
#include "weather_gen.h"
#include "weather.h"
#include "cata_utility.h"

#include <math.h>    //sqrt
#include <algorithm> //std::min
#include <sstream>

// '!', '-' and '=' are uses as default bindings in the menu
const invlet_wrapper bionic_chars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"#&()*+./:;@[\\]^_{|}");

const skill_id skilll_electronics( "electronics" );
const skill_id skilll_firstaid( "firstaid" );
const skill_id skilll_mechanics( "mechanics" );

namespace {
std::map<std::string, bionic_data> bionics;
std::vector<std::string> faulty_bionics;
} //namespace

bool is_valid_bionic(std::string const& id)
{
    return !!bionics.count(id);
}

bionic_data const& bionic_info(std::string const &id)
{
    auto const it = bionics.find(id);
    if (it != bionics.end()) {
        return it->second;
    }

    debugmsg("bad bionic id");

    static bionic_data const null_value {"bad bionic", false, false, 0, 0, 0, 0, 0, "bad_bionic", false};
    return null_value;
}

void bionics_install_failure(player *u, int difficulty, int success);

bionic_data::bionic_data(std::string nname, bool ps, bool tog, int pac, int pad, int pot,
                         int ct, int cap, std::string desc, bool fault
) : name(std::move(nname)), description(std::move(desc)), power_activate(pac),
    power_deactivate(pad), power_over_time(pot), charge_time(ct), capacity(cap), faulty(fault),
    power_source(ps), activated(tog || pac || ct), toggled(tog)
{
}

void show_bionics_titlebar(WINDOW *window, player *p, std::string menu_mode)
{
    werase(window);

    std::string caption = _("BIONICS -");
    int cap_offset = utf8_width(caption) + 1;
    mvwprintz(window, 0,  0, c_blue, "%s", caption.c_str());

    std::stringstream pwr;
    pwr << string_format(_("Power: %i/%i"), int(p->power_level), int(p->max_power_level));
    int pwr_length = utf8_width(pwr.str()) + 1;
    mvwprintz(window, 0, getmaxx(window) - pwr_length, c_white, "%s", pwr.str().c_str());

    std::string desc;
    int desc_length = getmaxx(window) - cap_offset - pwr_length;

    if(menu_mode == "reassigning") {
        desc = _("Reassigning.\nSelect a bionic to reassign or press SPACE to cancel.");
    } else if(menu_mode == "activating") {
        desc = _("<color_green>Activating</color>  <color_yellow>!</color> to examine, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs.");
    } else if(menu_mode == "removing") {
        desc = _("<color_red>Removing</color>  <color_yellow>!</color> to activate, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs.");
    } else if(menu_mode == "examining") {
        desc = _("<color_ltblue>Examining</color>  <color_yellow>!</color> to activate, <color_yellow>-</color> to remove, <color_yellow>=</color> to reassign, <color_yellow>TAB</color> to switch tabs.");
    }
    fold_and_print(window, 0, cap_offset, desc_length, c_white, desc);

    wrefresh(window);
}

//builds the power usage string of a given bionic
std::string build_bionic_poweronly_string(bionic const &bio)
{
    std::ostringstream power_desc;
    bool hasPreviousText = false;
    if (bionics[bio.id].power_over_time > 0 && bionics[bio.id].charge_time > 0) {
        power_desc << (
            bionics[bio.id].charge_time == 1
          ? string_format(_("%d PU / turn"),
                bionics[bio.id].power_over_time)
          : string_format(_("%d PU / %d turns"),
                bionics[bio.id].power_over_time,
                bionics[bio.id].charge_time));
        hasPreviousText = true;
    }
    if (bionics[bio.id].power_activate > 0 && !bionics[bio.id].charge_time) {
        if(hasPreviousText){
            power_desc << ", ";
        }
        power_desc << string_format(_("%d PU act"),
                        bionics[bio.id].power_activate);
        hasPreviousText = true;
    }
    if (bionics[bio.id].power_deactivate > 0 && !bionics[bio.id].charge_time) {
        if(hasPreviousText){
            power_desc << ", ";
        }
        power_desc << string_format(_("%d PU deact"),
                        bionics[bio.id].power_deactivate);
        hasPreviousText = true;
    }
    if (bionics[bio.id].toggled) {
        if(hasPreviousText){
            power_desc << ", ";
        }
        power_desc << (bio.powered ? _("ON") : _("OFF"));
    }

    return power_desc.str();
}

//generates the string that show how much power a bionic uses
std::string build_bionic_powerdesc_string(bionic const &bio)
{
    std::ostringstream power_desc;
    std::string power_string = build_bionic_poweronly_string(bio);
    power_desc << bionics[bio.id].name;
    if(power_string.length()>0){
        power_desc << ", " << power_string;
    }
    return power_desc.str();
}

//get a text color depending on the power/powering state of the bionic
nc_color get_bionic_text_color(bionic const &bio, bool const isHighlightedBionic)
{
    nc_color type = c_white;
    if(bionics[bio.id].activated){
        if(isHighlightedBionic){
            if (bio.powered && !bionics[bio.id].power_source) {
                type = h_red;
            } else if (bionics[bio.id].power_source && !bio.powered) {
                type = h_ltcyan;
            } else if (bionics[bio.id].power_source && bio.powered) {
                type = h_ltgreen;
            } else {
                type = h_ltred;
            }
        }else{
            if (bio.powered && !bionics[bio.id].power_source) {
                type = c_red;
            } else if (bionics[bio.id].power_source && !bio.powered) {
                type = c_ltcyan;
            } else if (bionics[bio.id].power_source && bio.powered) {
                type = c_ltgreen;
            } else {
                type = c_ltred;
            }
        }
    }else{
        if(isHighlightedBionic){
            if (bionics[bio.id].power_source) {
                type = h_ltcyan;
            } else {
                type = h_cyan;
            }
        }else{
            if (bionics[bio.id].power_source) {
                type = c_ltcyan;
            } else {
                type = c_cyan;
            }
        }
    }
    return type;
}

void player::power_bionics()
{
    std::vector <bionic *> passive;
    std::vector <bionic *> active;
    bionic *bio_last = NULL;
    std::string tab_mode = "TAB_ACTIVE";

    for( auto &elem : my_bionics ) {
        if( !bionics[elem.id].activated ) {
            passive.push_back( &elem );
        } else {
            active.push_back( &elem );
        }
    }

    // maximal number of rows in both columns
    int active_bionic_count = active.size();
    int passive_bionic_count = passive.size();
    int bionic_count = std::max(passive_bionic_count, active_bionic_count);

    //added title_tab_height for the tabbed bionic display
    int TITLE_HEIGHT = 2;
    int TITLE_TAB_HEIGHT = 3;

    // Main window
    /** Total required height is:
     * top frame line:                                         + 1
     * height of title window:                                 + TITLE_HEIGHT
     * height of tabs:                                         + TITLE_TAB_HEIGHT
     * height of the biggest list of active/passive bionics:   + bionic_count
     * bottom frame line:                                      + 1
     * TOTAL: TITLE_HEIGHT + TITLE_TAB_HEIGHT + bionic_count + 2
     */
    int HEIGHT = std::min(TERMY, std::max(FULL_SCREEN_HEIGHT,
                TITLE_HEIGHT + TITLE_TAB_HEIGHT + bionic_count + 2));
    int WIDTH = FULL_SCREEN_WIDTH + (TERMX - FULL_SCREEN_WIDTH) / 2;
    int START_X = (TERMX - WIDTH) / 2;
    int START_Y = (TERMY - HEIGHT) / 2;
    //wBio is the entire bionic window
    WINDOW *wBio = newwin(HEIGHT, WIDTH, START_Y, START_X);
    WINDOW_PTR wBioptr( wBio );

    int LIST_HEIGHT = HEIGHT - TITLE_HEIGHT - TITLE_TAB_HEIGHT - 2;

    int DESCRIPTION_WIDTH = WIDTH - 2 - 40;
    int DESCRIPTION_START_Y = START_Y + TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1;
    int DESCRIPTION_START_X = START_X + 1 + 40;
    //w_description is the description panel that is controlled with ! key
    WINDOW *w_description = newwin(LIST_HEIGHT, DESCRIPTION_WIDTH,
            DESCRIPTION_START_Y, DESCRIPTION_START_X);
    WINDOW_PTR w_descriptionptr( w_description );

    // Title window
    int TITLE_START_Y = START_Y + 1;
    int HEADER_LINE_Y = TITLE_HEIGHT + TITLE_TAB_HEIGHT + 1; // + lines with text in titlebar, local
    WINDOW *w_title = newwin(TITLE_HEIGHT, WIDTH - 2, TITLE_START_Y, START_X + 1);
    WINDOW_PTR w_titleptr( w_title );

    int TAB_START_Y = TITLE_START_Y + 2;
    //w_tabs is the tab bar for passive and active bionic groups
    WINDOW *w_tabs = newwin(TITLE_TAB_HEIGHT, WIDTH - 2, TAB_START_Y, START_X + 1);
    WINDOW_PTR w_tabsptr( w_tabs );

    int scroll_position = 0;
    int cursor = 0;

    //generate the tab title string and a count of the bionics owned
    std::string menu_mode = "activating";
    std::ostringstream tabname;
    tabname << _("ACTIVE");
    if(active_bionic_count>0){
        tabname << "(" << active_bionic_count << ")";
    }
    std::string active_tab_name = tabname.str();
    tabname.str("");
    tabname << _("PASSIVE");
    if(passive_bionic_count > 0){
        tabname << "(" << passive_bionic_count << ")";
    }
    std::string passive_tab_name = tabname.str();
    const int tabs_start = 1;
    const int tab_step = 3;

    // offset for display: bionic with index i is drawn at y=list_start_y+i
    // drawing the bionics starts with bionic[scroll_position]
    const int list_start_y = HEADER_LINE_Y;// - scroll_position;
    int half_list_view_location = LIST_HEIGHT / 2;
    int max_scroll_position = std::max(0, (tab_mode == "TAB_ACTIVE" ? active_bionic_count : passive_bionic_count) - LIST_HEIGHT);

    input_context ctxt("BIONICS");
    ctxt.register_updown();
    ctxt.register_action("ANY_INPUT");
    ctxt.register_action("TOGGLE_EXAMINE");
    ctxt.register_action("REASSIGN");
    ctxt.register_action("REMOVE");
    ctxt.register_action("NEXT_TAB");
    ctxt.register_action("PREV_TAB");
    ctxt.register_action("CONFIRM");
    ctxt.register_action("HELP_KEYBINDINGS");

    bool recalc = false;
    bool redraw = true;

    for (;;) {
        if(recalc) {
            active.clear();
            passive.clear();

            for( auto &elem : my_bionics ) {
                if( !bionics[elem.id].activated ) {
                    passive.push_back( &elem );
                } else {
                    active.push_back( &elem );
                }
            }

            active_bionic_count = active.size();
            passive_bionic_count = passive.size();
            bionic_count = std::max(passive_bionic_count, active_bionic_count);

            if(active_bionic_count == 0 && passive_bionic_count > 0){
                tab_mode = "TAB_PASSIVE";
            }

            max_scroll_position = std::max(0, (tab_mode == "TAB_ACTIVE" ? active_bionic_count : passive_bionic_count) - LIST_HEIGHT);
            if(--cursor < 0) {
                cursor = 0;
            }
            if(scroll_position > max_scroll_position && cursor - scroll_position < LIST_HEIGHT - half_list_view_location) {
                scroll_position--;
            }

            recalc = false;
        }

        //track which list we are looking at
        std::vector<bionic*> *current_bionic_list = (tab_mode == "TAB_ACTIVE" ? &active : &passive);

        if(redraw) {
            redraw = false;

            werase(wBio);
            draw_border(wBio);
            // Draw symbols to connect additional lines to border
            mvwputch(wBio, HEADER_LINE_Y - 1, 0, BORDER_COLOR, LINE_XXXO); // |-
            mvwputch(wBio, HEADER_LINE_Y - 1, WIDTH - 1, BORDER_COLOR, LINE_XOXX); // -|

            nc_color type;
            if(tab_mode == "TAB_PASSIVE"){
                if (passive.empty()) {
                    mvwprintz(wBio, list_start_y + 1, 2, c_ltgray, _("No passive bionics installed."));
                } else {
                    for (size_t i = scroll_position; i < passive.size(); i++) {
                        if (list_start_y + static_cast<int>(i) - scroll_position == HEIGHT - 1) {
                            break;
                        }

                        bool isHighlighted = false;
                        if(cursor == static_cast<int>(i)){
                            isHighlighted = true;
                        }
                        type = get_bionic_text_color(*passive[i], isHighlighted);

                        mvwprintz(wBio, list_start_y + i - scroll_position, 2, type, "%c %s", passive[i]->invlet,
                                bionics[passive[i]->id].name.c_str());
                    }
                }
            }

            if(tab_mode == "TAB_ACTIVE"){
                if (active.empty()) {
                    mvwprintz(wBio, list_start_y + 1, 2, c_ltgray, _("No activatable bionics installed."));
                } else {
                    for (size_t i = scroll_position; i < active.size(); i++) {
                        if (list_start_y + static_cast<int>(i) - scroll_position == HEIGHT - 1) {
                            break;
                        }
                        bool isHighlighted = false;
                        if(cursor == static_cast<int>(i)){
                            isHighlighted = true;
                        }
                        type = get_bionic_text_color(*active[i], isHighlighted);
                        mvwputch(wBio, list_start_y + i - scroll_position, 2, type, active[i]->invlet);
                        mvwputch(wBio, list_start_y + i - scroll_position, 3, type, ' ');

                        std::string power_desc = build_bionic_powerdesc_string(*active[i]);
                        std::string tmp = utf8_truncate(power_desc, WIDTH - 3);
                        mvwprintz(wBio, list_start_y + i - scroll_position, 2 + 2, type, tmp.c_str());
                    }
                }
            }

            // Scrollbar
            if(scroll_position > 0) {
                mvwputch(wBio, HEADER_LINE_Y, 0, c_ltgreen, '^');
            }
            if(scroll_position < max_scroll_position && max_scroll_position > 0) {
                mvwputch(wBio, HEIGHT - 1 - 1,
                        0, c_ltgreen, 'v');
            }
        }
        wrefresh(wBio);

        //handle tab drawing after main window is refreshed
        werase(w_tabs);
        int width = getmaxx(w_tabs);
        for (int i = 0; i < width; i++) {
            mvwputch(w_tabs, 2, i, BORDER_COLOR, LINE_OXOX);
        }
        int tab_x = tabs_start;
        draw_tab(w_tabs, tab_x, active_tab_name, tab_mode == "TAB_ACTIVE");
        tab_x += tab_step + utf8_width(active_tab_name);
        draw_tab(w_tabs, tab_x, passive_tab_name, tab_mode != "TAB_ACTIVE");
        wrefresh(w_tabs);

        show_bionics_titlebar(w_title, this, menu_mode);

        // Description
        if(menu_mode == "examining" && current_bionic_list->size() > 0){
            werase(w_description);
            std::ostringstream power_only_desc;
            std::string poweronly_string;
            std::string bionic_name;
            if(tab_mode == "TAB_ACTIVE"){
                bionic_name = bionics[active[cursor]->id].name;
                poweronly_string = build_bionic_poweronly_string(*active[cursor]);
            }else{
                bionic_name = bionics[passive[cursor]->id].name;
                poweronly_string = build_bionic_poweronly_string(*passive[cursor]);
            }
            int ypos = 0;
            ypos += fold_and_print(w_description, ypos, 0, DESCRIPTION_WIDTH, c_white, bionic_name);
            if(poweronly_string.length() > 0){
                power_only_desc << _("Power usage: ") << poweronly_string;
                ypos += fold_and_print(w_description, ypos, 0, DESCRIPTION_WIDTH, c_ltgray, power_only_desc.str());
            }
            ypos += fold_and_print(w_description, ypos, 0, DESCRIPTION_WIDTH, c_ltblue, bionics[(*current_bionic_list)[cursor]->id].description);
            wrefresh(w_description);
        }

        const std::string action = ctxt.handle_input();
        const long ch = ctxt.get_raw_input().get_first_input();
        bionic *tmp = NULL;
        bool confirmCheck = false;
        if (menu_mode == "reassigning") {
            menu_mode = "activating";
            tmp = bionic_by_invlet(ch);
            if(tmp == nullptr) {
                // Selected an non-existing bionic (or escape, or ...)
                continue;
            }
            redraw = true;
            const long newch = popup_getkey(_("%s; enter new letter."),
                    bionics[tmp->id].name.c_str());
            wrefresh(wBio);
            if(newch == ch || newch == ' ' || newch == KEY_ESCAPE) {
                continue;
            }
            if( !bionic_chars.valid( newch ) ) {
                popup( _("Invlid bionic letter. Only those characters are valid:\n\n%s"),
                       bionic_chars.get_allowed_chars().c_str() );
                continue;
            }
            bionic *otmp = bionic_by_invlet(newch);
            if(otmp != nullptr) {
                std::swap(tmp->invlet, otmp->invlet);
            } else {
                tmp->invlet = newch;
            }
            // TODO: show a message like when reassigning a key to an item?
        } else if (action == "NEXT_TAB") {
            redraw = true;
            scroll_position = 0;
            cursor = 0;
            if(tab_mode == "TAB_ACTIVE"){
                tab_mode = "TAB_PASSIVE";
            }else{
                tab_mode = "TAB_ACTIVE";
            }
        } else if (action == "PREV_TAB") {
            redraw = true;
            scroll_position = 0;
            cursor = 0;
            if(tab_mode == "TAB_PASSIVE"){
                tab_mode = "TAB_ACTIVE";
            }else{
                tab_mode = "TAB_PASSIVE";
            }
        } else if (action == "DOWN") {
            redraw = true;
            if(static_cast<size_t>(cursor)<current_bionic_list->size()-1){
                cursor++;
            }
            if(scroll_position < max_scroll_position && cursor - scroll_position > LIST_HEIGHT - half_list_view_location) {
                scroll_position++;
            }
        } else if (action == "UP") {
            redraw = true;
            if(cursor>0){
                cursor--;
            }
            if(scroll_position > 0 && cursor - scroll_position < half_list_view_location) {
                scroll_position--;
            }
        } else if (action == "REASSIGN") {
            menu_mode = "reassigning";
        } else if (action == "TOGGLE_EXAMINE") { // switches between activation and examination
            menu_mode = menu_mode == "activating" ? "examining" : "activating";
            redraw = true;
        } else if (action == "REMOVE") {
            menu_mode = "removing";
            redraw = true;
        } else if (action == "HELP_KEYBINDINGS") {
            redraw = true;
        } else if (action == "CONFIRM"){
            confirmCheck = true;
        } else {
            confirmCheck = true;
        }
        //confirmation either occurred by pressing enter where the bionic cursor is, or the hotkey was selected
        if(confirmCheck){
            auto& bio_list = tab_mode == "TAB_ACTIVE" ? active : passive;
            if(action == "CONFIRM" && current_bionic_list->size() > 0){
                tmp = bio_list[cursor];
            }else{
                tmp = bionic_by_invlet(ch);
                if(tmp && tmp != bio_last) {
                    // new bionic selected, update cursor and scroll position
                    int temp_cursor = 0;
                    for(temp_cursor = 0; temp_cursor < (int)bio_list.size(); temp_cursor++) {
                        if(bio_list[temp_cursor] == tmp) {
                            break;
                        }
                    }
                    // if bionic is not found in current list, ignore the attempt to view/activate
                    if(temp_cursor >= (int)bio_list.size()) {
                        continue;
                    }
                    //relocate cursor to the bionic that was found
                    cursor = temp_cursor;
                    scroll_position = 0;
                    while(scroll_position < max_scroll_position && cursor - scroll_position > LIST_HEIGHT - half_list_view_location) {
                        scroll_position++;
                    }
                }
            }
            if(!tmp) {
                // entered a key that is not mapped to any bionic,
                // -> leave screen
                break;
            }
            bio_last = tmp;
            const std::string &bio_id = tmp->id;
            const bionic_data &bio_data = bionics[bio_id];
            if (menu_mode == "removing") {
                if (uninstall_bionic(bio_id)) {
                    recalc = true;
                    redraw = true;
                    continue;
                }
            }
            if (menu_mode == "activating") {
                if (bio_data.activated) {
                    int b = tmp - &my_bionics[0];
                    if (tmp->powered) {
                        deactivate_bionic(b);
                    } else {
                        activate_bionic(b);
                    }
                    // update message log and the menu
                    g->refresh_all();
                    redraw = true;
                    continue;
                } else {
                    popup(_("You can not activate %s!\n"
                            "To read a description of %s, press '!', then '%c'."), bio_data.name.c_str(), bio_data.name.c_str(), tmp->invlet);
                    redraw = true;
                }
            } else if (menu_mode == "examining") { // Describing bionics, allow user to jump to description key
                redraw = true;
                if(action != "CONFIRM"){
                    for(size_t i = 0; i < active.size(); i++){
                        if(active[i] == tmp){
                            tab_mode = "TAB_ACTIVE";
                            cursor = static_cast<int>(i);
                            int max_scroll_check = std::max(0, active_bionic_count - LIST_HEIGHT);
                            if(static_cast<int>(i) > max_scroll_check){
                                scroll_position = max_scroll_check;
                            }else{
                                scroll_position = i;
                            }
                            break;
                        }
                    }
                    for(size_t i = 0; i < passive.size(); i++){
                        if(passive[i] == tmp){
                            tab_mode = "TAB_PASSIVE";
                            cursor = static_cast<int>(i);
                            int max_scroll_check = std::max(0, passive_bionic_count - LIST_HEIGHT);
                            if(static_cast<int>(i) > max_scroll_check){
                                scroll_position = max_scroll_check;
                            }else{
                                scroll_position = i;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

void draw_exam_window(WINDOW *win, int border_line, bool examination)
{
    int width = getmaxx(win);
    if (examination) {
        for (int i = 1; i < width - 1; i++) {
            mvwputch(win, border_line, i, BORDER_COLOR, LINE_OXOX); // Draw line above description
        }
        mvwputch(win, border_line, 0, BORDER_COLOR, LINE_XXXO); // |-
        mvwputch(win, border_line, width - 1, BORDER_COLOR, LINE_XOXX); // -|
    } else {
        for (int i = 1; i < width - 1; i++) {
            mvwprintz(win, border_line, i, c_black, " "); // Erase line
        }
        mvwputch(win, border_line, 0, BORDER_COLOR, LINE_XOXO); // |
        mvwputch(win, border_line, width, BORDER_COLOR, LINE_XOXO); // |
    }
}

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
bool player::activate_bionic(int b, bool eff_only)
{
    bionic &bio = my_bionics[b];

    // Special compatibility code for people who updated saves with their claws out
    if ((weapon.type->id == "bio_claws_weapon" && bio.id == "bio_claws_weapon") ||
            (weapon.type->id == "bio_blade_weapon" && bio.id == "bio_blade_weapon")) {
        return deactivate_bionic(b);
    }

    // eff_only means only do the effect without messing with stats or displaying messages
    if (!eff_only) {
        if (bio.powered) {
            // It's already on!
            return false;
        }
        if (power_level < bionics[bio.id].power_activate) {
            add_msg(m_info, _("You don't have the power to activate your %s."), bionics[bio.id].name.c_str());
            return false;
        }

        //We can actually activate now, do activation-y things
        charge_power(-bionics[bio.id].power_activate);
        if (bionics[bio.id].toggled || bionics[bio.id].charge_time > 0) {
            bio.powered = true;
        }
        if (bionics[bio.id].charge_time > 0) {
            bio.charge = bionics[bio.id].charge_time;
        }
        add_msg(m_info, _("You activate your %s."), bionics[bio.id].name.c_str());
    }

    std::vector<std::string> good;
    std::vector<std::string> bad;
    tripoint dirp = pos();
    int &dirx = dirp.x;
    int &diry = dirp.y;
    item tmp_item;
    w_point const weatherPoint = g->weather_gen->get_weather( global_square_location(), calendar::turn );

    // On activation effects go here
    if(bio.id == "bio_painkiller") {
        pkill += 6;
        pain -= 2;
        if (pkill > pain) {
            pkill = pain;
        }
    } else if (bio.id == "bio_ears" && has_active_bionic("bio_earplugs")) {
        for (auto &i : my_bionics) {
            if (i.id == "bio_earplugs") {
                i.powered = false;
                add_msg(m_info, _("Your %s automatically turn off."), bionics[i.id].name.c_str());
            }
        }
    } else if (bio.id == "bio_earplugs" && has_active_bionic("bio_ears")) {
        for (auto &i : my_bionics) {
            if (i.id == "bio_ears") {
                i.powered = false;
                add_msg(m_info, _("Your %s automatically turns off."), bionics[i.id].name.c_str());
            }
        }
    } else if (bio.id == "bio_tools") {
        invalidate_crafting_inventory();
    } else if (bio.id == "bio_cqb") {
        if (!pick_style()) {
            bio.powered = false;
            add_msg(m_info, _("You change your mind and turn it off."));
            return false;
        }
    } else if (bio.id == "bio_nanobots") {
        remove_effect("bleed");
        healall(4);
    } else if (bio.id == "bio_resonator") {
        //~Sound of a bionic sonic-resonator shaking the area
        sounds::sound( pos(), 30, _("VRRRRMP!"));
        for (int i = posx() - 1; i <= posx() + 1; i++) {
            for (int j = posy() - 1; j <= posy() + 1; j++) {
                tripoint bashpoint( i, j, posz() );
                g->m.bash( bashpoint, 110 );
                g->m.bash( bashpoint, 110 ); // Multibash effect, so that doors &c will fall
                g->m.bash( bashpoint, 110 );
            }
        }
    } else if (bio.id == "bio_time_freeze") {
        moves += power_level;
        power_level = 0;
        add_msg(m_good, _("Your speed suddenly increases!"));
        if (one_in(3)) {
            add_msg(m_bad, _("Your muscles tear with the strain."));
            apply_damage( nullptr, bp_arm_l, rng( 5, 10 ) );
            apply_damage( nullptr, bp_arm_r, rng( 5, 10 ) );
            apply_damage( nullptr, bp_leg_l, rng( 7, 12 ) );
            apply_damage( nullptr, bp_leg_r, rng( 7, 12 ) );
            apply_damage( nullptr, bp_torso, rng( 5, 15 ) );
        }
        if (one_in(5)) {
            add_effect("teleglow", rng(50, 400));
        }
    } else if (bio.id == "bio_teleport") {
        g->teleport();
        add_effect("teleglow", 300);
        // TODO: More stuff here (and bio_blood_filter)
    } else if(bio.id == "bio_blood_anal") {
        WINDOW *w = newwin(20, 40, 3 + ((TERMY > 25) ? (TERMY - 25) / 2 : 0),
                10 + ((TERMX > 80) ? (TERMX - 80) / 2 : 0));
        draw_border(w);
        if (has_effect("fungus")) {
            bad.push_back(_("Fungal Parasite"));
        }
        if (has_effect("dermatik")) {
            bad.push_back(_("Insect Parasite"));
        }
        if (has_effect("stung")) {
            bad.push_back(_("Stung"));
        }
        if (has_effect("poison")) {
            bad.push_back(_("Poison"));
        }
        if (radiation > 0) {
            bad.push_back(_("Irradiated"));
        }
        if (has_effect("pkill1")) {
            good.push_back(_("Minor Painkiller"));
        }
        if (has_effect("pkill2")) {
            good.push_back(_("Moderate Painkiller"));
        }
        if (has_effect("pkill3")) {
            good.push_back(_("Heavy Painkiller"));
        }
        if (has_effect("pkill_l")) {
            good.push_back(_("Slow-Release Painkiller"));
        }
        if (has_effect("drunk")) {
            good.push_back(_("Alcohol"));
        }
        if (has_effect("cig")) {
            good.push_back(_("Nicotine"));
        }
        if (has_effect("meth")) {
            good.push_back(_("Methamphetamines"));
        }
        if (has_effect("high")) {
            good.push_back(_("Intoxicant: Other"));
        }
        if (has_effect("weed_high")) {
            good.push_back(_("THC Intoxication"));
        }
        if (has_effect("hallu") || has_effect("visuals")) {
            bad.push_back(_("Hallucinations"));
        }
        if (has_effect("iodine")) {
            good.push_back(_("Iodine"));
        }
        if (has_effect("datura")) {
            good.push_back(_("Anticholinergic Tropane Alkaloids"));
        }
        if (has_effect("took_xanax")) {
            good.push_back(_("Xanax"));
        }
        if (has_effect("took_prozac")) {
            good.push_back(_("Prozac"));
        }
        if (has_effect("took_flumed")) {
            good.push_back(_("Antihistamines"));
        }
        if (has_effect("adrenaline")) {
            good.push_back(_("Adrenaline Spike"));
        }
        if (has_effect("adrenaline_mycus")) {
            good.push_back(_("Mycal Spike"));
        }
        if (has_effect("tapeworm")) {  // This little guy is immune to the blood filter though, as he lives in your bowels.
            good.push_back(_("Intestinal Parasite"));
        }
        if (has_effect("bloodworms")) {
            good.push_back(_("Hemolytic Parasites"));
        }
        if (has_effect("brainworms")) {  // These little guys are immune to the blood filter too, as they live in your brain.
            good.push_back(_("Intracranial Parasite"));
        }
        if (has_effect("paincysts")) {  // These little guys are immune to the blood filter too, as they live in your muscles.
            good.push_back(_("Intramuscular Parasites"));
        }
        if (has_effect("tetanus")) {  // Tetanus infection.
            good.push_back(_("Clostridium Tetani Infection"));
        }
        if (good.empty() && bad.empty()) {
            mvwprintz(w, 1, 1, c_white, _("No effects."));
        } else {
            for (unsigned line = 1; line < 39 && line <= good.size() + bad.size(); line++) {
                if (line <= bad.size()) {
                    mvwprintz(w, line, 1, c_red, "%s", bad[line - 1].c_str());
                } else {
                    mvwprintz(w, line, 1, c_green, "%s", good[line - 1 - bad.size()].c_str());
                }
            }
        }
        wrefresh(w);
        refresh();
        getch();
        delwin(w);
    } else if(bio.id == "bio_blood_filter") {
        remove_effect("fungus");
        remove_effect("dermatik");
        remove_effect("bloodworms");
        remove_effect("tetanus");
        remove_effect("poison");
        remove_effect("stung");
        remove_effect("pkill1");
        remove_effect("pkill2");
        remove_effect("pkill3");
        remove_effect("pkill_l");
        remove_effect("drunk");
        remove_effect("cig");
        remove_effect("high");
        remove_effect("hallu");
        remove_effect("visuals");
        remove_effect("iodine");
        remove_effect("datura");
        remove_effect("took_xanax");
        remove_effect("took_prozac");
        remove_effect("took_flumed");
        remove_effect("adrenaline");
        remove_effect("meth");
        pkill = 0;
        stim = 0;
    } else if(bio.id == "bio_evap") {
        item water = item("water_clean", 0);
        int humidity = weatherPoint.humidity;
        int water_charges = (humidity * 3.0) / 100.0 + 0.5;
        // At 50% relative humidity or more, the player will draw 2 units of water
        // At 16% relative humidity or less, the player will draw 0 units of water
        water.charges = water_charges;
        if (water_charges == 0) {
            add_msg_if_player(m_bad, _("There was not enough moisture in the air from which to draw water!"));
        } else if (g->handle_liquid(water, true, false)) {
            moves -= 100;
        } else {
            water.charges -= drink_from_hands( water );
            if( water.charges == water_charges ) {
                charge_power(bionics["bio_evap"].power_activate);
            }
        }
    } else if(bio.id == "bio_lighter") {
        if(!choose_adjacent(_("Start a fire where?"), dirp) ||
           (!g->m.add_field(dirp, fd_fire, 1, 0))) {
            add_msg_if_player(m_info, _("You can't light a fire there."));
            charge_power(bionics["bio_lighter"].power_activate);
        }
    } else if(bio.id == "bio_leukocyte") {
        set_healthy(std::min(100, get_healthy() + 2));
        mod_healthy_mod(20, 100);
    } else if(bio.id == "bio_geiger") {
        add_msg(m_info, _("Your radiation level: %d"), radiation);
    } else if(bio.id == "bio_radscrubber") {
        if (radiation > 4) {
            radiation -= 5;
        } else {
            radiation = 0;
        }
    } else if(bio.id == "bio_adrenaline") {
        if (has_effect("adrenaline")) {
            add_effect("adrenaline", 50);
        } else {
            add_effect("adrenaline", 200);
        }
    } else if(bio.id == "bio_blaster") {
        tmp_item = weapon;
        weapon = item("bio_blaster_gun", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            charge_power(bionics[bio.id].power_activate);
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_laser") {
        tmp_item = weapon;
        weapon = item("bio_laser_gun", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            charge_power(bionics[bio.id].power_activate);
        }
        weapon = tmp_item;
    } else if(bio.id == "bio_chain_lightning") {
        tmp_item = weapon;
        weapon = item("bio_lightning", 0);
        g->refresh_all();
        g->plfire(false);
        if(weapon.charges == 1) { // not fired
            charge_power(bionics[bio.id].power_activate);
        }
        weapon = tmp_item;
    } else if (bio.id == "bio_emp") {
        if(choose_adjacent(_("Create an EMP where?"), dirx, diry)) {
            g->emp_blast( tripoint( dirx, diry, posz() ) );
        } else {
            charge_power(bionics["bio_emp"].power_activate);
        }
    } else if (bio.id == "bio_hydraulics") {
        add_msg(m_good, _("Your muscles hiss as hydraulic strength fills them!"));
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, _("HISISSS!"));
    } else if (bio.id == "bio_water_extractor") {
        bool extracted = false;
        for( auto it = g->m.i_at(posx(), posy()).begin();
             it != g->m.i_at(posx(), posy()).end(); ++it) {
            if( it->is_corpse() ) {
                const int avail = it->get_var( "remaining_water", it->volume() / 2 );
                if(avail > 0 && query_yn(_("Extract water from the %s"), it->tname().c_str())) {
                    item water = item("water_clean", 0);
                    water.charges = avail;
                    if (g->handle_liquid(water, true, false)) {
                        moves -= 100;
                    } else {
                        water.charges -= drink_from_hands( water );
                    }
                    if( water.charges != avail ) {
                        extracted = true;
                        it->set_var( "remaining_water", static_cast<int>( water.charges ) );
                    }
                    break;
                }
            }
        }
        if (!extracted) {
            charge_power(bionics["bio_water_extractor"].power_activate);
        }
    } else if(bio.id == "bio_magnet") {
        std::vector<tripoint> traj;
        for (int i = posx() - 10; i <= posx() + 10; i++) {
            for (int j = posy() - 10; j <= posy() + 10; j++) {
                if (g->m.i_at(i, j).size() > 0) {
                    traj = g->m.find_clear_path( {i, j, posz()}, pos3() );
                }
                traj.insert(traj.begin(), {i, j, posz()});
                if( g->m.has_flag( "SEALED", i, j ) ) {
                    continue;
                }
                for (unsigned k = 0; k < g->m.i_at(i, j).size(); k++) {
                    tmp_item = g->m.i_at(i, j)[k];
                    if( (tmp_item.made_of("iron") || tmp_item.made_of("steel")) &&
                        tmp_item.weight() < weight_capacity() ) {
                        g->m.i_rem(i, j, k);
                        std::vector<tripoint>::iterator it;
                        for (it = traj.begin(); it != traj.end(); ++it) {
                            int index = g->mon_at(*it);
                            if (index != -1) {
                                g->zombie(index).apply_damage( this, bp_torso, tmp_item.weight() / 225 );
                                g->zombie(index).check_dead_state();
                                g->m.add_item_or_charges(it->x, it->y, tmp_item);
                                break;
                            } else if (g->m.move_cost(it->x, it->y) == 0) {
                                if (it != traj.begin()) {
                                    g->m.bash( tripoint( it->x, it->y, posz() ), tmp_item.weight() / 225 );
                                    if (g->m.move_cost(it->x, it->y) == 0) {
                                        g->m.add_item_or_charges((it - 1)->x, (it - 1)->y, tmp_item);
                                        break;
                                    }
                                } else {
                                    g->m.bash( *it, tmp_item.weight() / 225 );
                                    if (g->m.move_cost(it->x, it->y) == 0) {
                                        break;
                                    }
                                }
                            }
                        }
                        if (it == traj.end()) {
                            g->m.add_item_or_charges(posx(), posy(), tmp_item);
                        }
                    }
                }
            }
        }
        moves -= 100;
    } else if(bio.id == "bio_lockpick") {
        tmp_item = item( "pseuso_bio_picklock", 0 );
        if( invoke_item( &tmp_item ) == 0 ) {
            if (tmp_item.charges > 0) {
                // restore the energy since CBM wasn't used
                charge_power(bionics[bio.id].power_activate);
            }
            return true;
        }
        if( tmp_item.damage > 0 ) {
            // TODO: damage the player / their bionics
        }
    } else if(bio.id == "bio_flashbang") {
        g->flashbang( pos3(), true);
    } else if(bio.id == "bio_shockwave") {
        g->shockwave( pos3(), 3, 4, 2, 8, true );
        add_msg_if_player(m_neutral, _("You unleash a powerful shockwave!"));
    } else if(bio.id == "bio_meteorologist") {
        // Calculate local wind power
        int vpart = -1;
        vehicle *veh = g->m.veh_at( pos(), vpart );
        int vehwindspeed = 0;
        if( veh != nullptr ) {
            vehwindspeed = abs(veh->velocity / 100); // vehicle velocity in mph
        }
        const oter_id &cur_om_ter = overmap_buffer.ter( global_omt_location() );
        std::string omtername = otermap[cur_om_ter].name;
        /* windpower defined in internal velocity units (=.01 mph) */
        double windpower = 100.0f * get_local_windpower(weatherPoint.windpower + vehwindspeed,
                                                        omtername, g->is_sheltered(g->u.pos()));
        add_msg_if_player(m_info, _("Temperature: %s."), print_temperature(g->get_temperature()).c_str());
        add_msg_if_player(m_info, _("Relative Humidity: %s."), print_humidity(get_local_humidity(weatherPoint.humidity, g->weather, g->is_sheltered(g->u.pos()))).c_str());
        add_msg_if_player(m_info, _("Pressure: %s."), print_pressure((int)weatherPoint.pressure).c_str());
        add_msg_if_player(m_info, _("Wind Speed: %.1f %s."),
                                                 convert_velocity(int(windpower), true),
                                                 velocity_units(true));
        add_msg_if_player(m_info, _("Feels Like: %s."), print_temperature(get_local_windchill(weatherPoint.temperature, weatherPoint.humidity, windpower) + g->get_temperature()).c_str());
    } else if(bio.id == "bio_claws") {
        if (weapon.has_flag ("NO_UNWIELD")) {
            add_msg(m_info, _("Deactivate your %s first!"),
                    weapon.tname().c_str());
            charge_power(bionics[bio.id].power_activate);
            bio.powered = false;
            return false;
        } else if(weapon.type->id != "null") {
            add_msg(m_warning, _("Your claws extend, forcing you to drop your %s."),
                    weapon.tname().c_str());
            g->m.add_item_or_charges(posx(), posy(), weapon);
            weapon = item("bio_claws_weapon", 0);
            weapon.invlet = '#';
        } else {
            add_msg(m_neutral, _("Your claws extend!"));
            weapon = item("bio_claws_weapon", 0);
            weapon.invlet = '#';
        }
    } else if(bio.id == "bio_blade") {
        if (weapon.has_flag ("NO_UNWIELD")) {
            add_msg(m_info, _("Deactivate your %s first!"),
                    weapon.tname().c_str());
            charge_power(bionics[bio.id].power_activate);
            bio.powered = false;
            return false;
        } else if(weapon.type->id != "null") {
            add_msg(m_warning, _("Your blade extends, forcing you to drop your %s."),
                    weapon.tname().c_str());
            g->m.add_item_or_charges(posx(), posy(), weapon);
            weapon = item("bio_blade_weapon", 0);
            weapon.invlet = '#';
        } else {
            add_msg(m_neutral, _("You extend your blade!"));
            weapon = item("bio_blade_weapon", 0);
            weapon.invlet = '#';
        }
    } else if( bio.id == "bio_remote" ) {
        int choice = menu( true, _("Perform which function:"), _("Nothing"),
                           _("Control vehicle"), _("RC radio"), NULL );
        if( choice >= 2 && choice <= 3 ) {
            item ctr;
            if( choice == 2 ) {
                ctr = item( "remotevehcontrol", 0 );
            } else {
                ctr = item( "radiocontrol", 0 );
            }
            ctr.charges = power_level;
            int power_use = invoke_item( &ctr );
            charge_power(-power_use);
            bio.powered = ctr.active;
        } else {
            bio.powered = g->remoteveh() != nullptr || get_value( "remote_controlling" ) != "";
        }
    } else if (bio.id == "bio_plutdump") {
        if (query_yn(_("WARNING: Purging all fuel is likely to result in radiation!  Purge anyway?"))) {
            slow_rad += (tank_plut + reactor_plut);
            tank_plut = 0;
            reactor_plut = 0;
            }
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    reset();

    return true;
}

bool player::deactivate_bionic(int b, bool eff_only)
{
    bionic &bio = my_bionics[b];

    // Just do the effect, no stat changing or messages
    if (!eff_only) {
        if (!bio.powered) {
            // It's already off!
            return false;
        }
        if (!bionics[bio.id].toggled) {
            // It's a fire-and-forget bionic, we can't turn it off but have to wait for it to run out of charge
            add_msg(m_info, _("You can't deactivate your %s manually!"), bionics[bio.id].name.c_str());
            return false;
        }
        if (power_level < bionics[bio.id].power_deactivate) {
            add_msg(m_info, _("You don't have the power to deactivate your %s."), bionics[bio.id].name.c_str());
            return false;
        }

        //We can actually deactivate now, do deactivation-y things
        charge_power(-bionics[bio.id].power_deactivate);
        bio.powered = false;
        add_msg(m_neutral, _("You deactivate your %s."), bionics[bio.id].name.c_str());
    }

    // Deactivation effects go here
    if (bio.id == "bio_cqb") {
        // check if player knows current style naturally, otherwise drop them back to style_none
        if( style_selected != matype_id( "style_none" ) ) {
            bool has_style = false;
            for( auto &elem : ma_styles ) {
                if( elem == style_selected ) {
                    has_style = true;
                }
            }
            if (!has_style) {
                style_selected = matype_id( "style_none" );
            }
        }
    } else if(bio.id == "bio_claws") {
        if (weapon.type->id == "bio_claws_weapon") {
            add_msg(m_neutral, _("You withdraw your claws."));
            weapon = ret_null;
        }
    } else if(bio.id == "bio_blade") {
        if (weapon.type->id == "bio_blade_weapon") {
            add_msg(m_neutral, _("You retract your blade."));
            weapon = ret_null;
        }
    } else if( bio.id == "bio_remote" ) {
        if( g->remoteveh() != nullptr && !has_active_item( "remotevehcontrol" ) ) {
            g->setremoteveh( nullptr );
        } else if( get_value( "remote_controlling" ) != "" && !has_active_item( "radiocontrol" ) ) {
            set_value( "remote_controlling", "" );
        }
    } else if( bio.id == "bio_tools" ) {
        invalidate_crafting_inventory();
    }

    // Recalculate stats (strength, mods from pain etc.) that could have been affected
    reset();

    return true;
}

void player::process_bionic(int b)
{
    bionic &bio = my_bionics[b];
    if (!bio.powered) {
        // Only powered bionics should be processed
        return;
    }

    if (bio.charge > 0) {
        // Units already with charge just lose charge
        bio.charge--;
    } else {
        if (bionics[bio.id].charge_time > 0) {
            // Try to recharge our bionic if it is made for it
            if (bionics[bio.id].power_over_time > 0) {
                if (power_level < bionics[bio.id].power_over_time) {
                    // No power to recharge, so deactivate
                    bio.powered = false;
                    add_msg(m_neutral, _("Your %s powers down."), bionics[bio.id].name.c_str());
                    // This purposely bypasses the deactivation cost
                    deactivate_bionic(b, true);
                    return;
                } else {
                    // Pay the recharging cost
                    charge_power(-bionics[bio.id].power_over_time);
                    // We just spent our first turn of charge, so -1 here
                    bio.charge = bionics[bio.id].charge_time - 1;
                }
            // Some bionics are a 1-shot activation so they just deactivate at 0 charge.
            } else {
                bio.powered = false;
                add_msg(m_neutral, _("Your %s powers down."), bionics[bio.id].name.c_str());
                // This purposely bypasses the deactivation cost
                deactivate_bionic(b, true);
                return;
            }
        }
    }

    // Bionic effects on every turn they are active go here.
    if( bio.id == "bio_night" ) {
        if( calendar::once_every(5) ) {
            add_msg(m_neutral, _("Artificial night generator active!"));
        }
    } else if( bio.id == "bio_remote" ) {
        if( g->remoteveh() == nullptr && get_value( "remote_controlling" ) == "" ) {
            bio.powered = false;
            add_msg( m_warning, _("Your %s has lost connection and is turning off."),
                     bionics[bio.id].name.c_str() );
        }
    } else if (bio.id == "bio_hydraulics") {
        // Sound of hissing hydraulic muscle! (not quite as loud as a car horn)
        sounds::sound( pos(), 19, _("HISISSS!"));
    }
}

void bionics_uninstall_failure(player *u)
{
    switch (rng(1, 5)) {
    case 1:
        add_msg(m_neutral, _("You flub the removal."));
        break;
    case 2:
        add_msg(m_neutral, _("You mess up the removal."));
        break;
    case 3:
        add_msg(m_neutral, _("The removal fails."));
        break;
    case 4:
        add_msg(m_neutral, _("The removal is a failure."));
        break;
    case 5:
        add_msg(m_neutral, _("You screw up the removal."));
        break;
    }
    add_msg(m_bad, _("Your body is severely damaged!"));
    u->hurtall(rng(30, 80), u); // stop hurting yourself!
}

// bionic manipulation chance of success
int bionic_manip_cos(int p_int, int s_electronics, int s_firstaid, int s_mechanics,
                     int bionic_difficulty)
{
    int pl_skill = p_int         * 4 +
                   s_electronics * 4 +
                   s_firstaid    * 3 +
                   s_mechanics   * 1;

    // Medical residents have some idea what they're doing
    if (g->u.has_trait("PROF_MED")) {
        pl_skill += 3;
        add_msg(m_neutral, _("You prep yourself to begin surgery."));
    }

    // for chance_of_success calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // we will base chance_of_success on a ratio of skill and difficulty
    // when skill=difficulty, this gives us 1.  skill < difficulty gives a fraction.
    float skill_difficulty_parameter = float(adjusted_skill / (4.0 * bionic_difficulty));

    // when skill == difficulty, chance_of_success is 50%. Chance of success drops quickly below that
    // to reserve bionics for characters with the appropriate skill.  For more difficult bionics, the
    // curve flattens out just above 80%
    int chance_of_success = int((100 * skill_difficulty_parameter) /
                                (skill_difficulty_parameter + sqrt( 1 / skill_difficulty_parameter)));

    return chance_of_success;
}

bool player::uninstall_bionic(std::string const &b_id, int skill_level)
{
    // malfunctioning bionics don't have associated items and get a difficulty of 12
    int difficulty = 12;
    if( item::type_is_defined( b_id ) ) {
        auto type = item::find_type( b_id );
        if( type->bionic ) {
            difficulty = type->bionic->difficulty;
        }
    }

    if (!has_bionic(b_id)) {
        popup(_("You don't have this bionic installed."));
        return false;
    }
    //If you are paying the doctor to do it, shouldn't use your supplies
    if (!(has_items_with_quality("CUT", 1, 1) && has_amount("1st_aid", 1)) && skill_level == -1) {
        popup(_("Removing bionics requires a cutting tool and a first aid kit."));
        return false;
    }

    if ( b_id == "bio_blaster" ) {
        popup(_("Removing your Fusion Blaster Arm would leave you with a useless stump."));
        return false;
    }

    if (( b_id == "bio_reactor" ) || ( b_id == "bio_advreactor" )) {
        if (!query_yn(_("WARNING: Removing a reactor may leave radioactive material! Remove anyway?"))) {
            return false;
        }
    } else if (b_id == "bio_plutdump") {
        popup(_("You must remove your reactor to remove the Plutonium Purger."));
    }

    if ( b_id == "bio_earplugs") {
        popup(_("You must remove the Enhanced Hearing bionic to remove the Sound Dampeners."));
        return false;
    }

	if ( b_id == "bio_blindfold") {
        popup(_("You must remove the Anti-glare Compensators bionic to remove the Optical Dampers."));
        return false;
    }

    // removal of bionics adds +2 difficulty over installation
    int chance_of_success;
    if (skill_level != -1){
        chance_of_success = bionic_manip_cos(skill_level,
                                skill_level,
                                skill_level,
                                skill_level,
                                difficulty + 2);
    } else {
        ///\EFFECT_INT increases chance of success removing bionics with unspecified skil level
        chance_of_success = bionic_manip_cos(int_cur,
                                skillLevel( skilll_electronics ),
                                skillLevel( skilll_firstaid ),
                                skillLevel( skilll_mechanics ),
                                difficulty + 2);
    }

    if (!query_yn(_("WARNING: %i percent chance of failure and SEVERE bodily damage! Remove anyway?"),
                  100 - chance_of_success)) {
        return false;
    }

    // surgery is imminent, retract claws or blade if active
    if (has_bionic("bio_claws") && skill_level == -1 ) {
        if (weapon.type->id == "bio_claws_weapon") {
            add_msg(m_neutral, _("You withdraw your claws."));
            weapon = ret_null;
          }
    }

    if (has_bionic("bio_blade") && skill_level == -1 ) {
        if (weapon.type->id == "bio_blade_weapon") {
            add_msg(m_neutral, _("You retract your blade."));
            weapon = ret_null;
        }
    }

    //If you are paying the doctor to do it, shouldn't use your supplies
    if (skill_level == -1)
        use_charges("1st_aid", 1);

    practice( skilll_electronics, int((100 - chance_of_success) * 1.5) );
    practice( skilll_firstaid, int((100 - chance_of_success) * 1.0) );
    practice( skilll_mechanics, int((100 - chance_of_success) * 0.5) );

    int success = chance_of_success - rng(1, 100);

    if (success > 0) {
        add_memorial_log(pgettext("memorial_male", "Removed bionic: %s."),
                         pgettext("memorial_female", "Removed bionic: %s."),
                         bionics[b_id].name.c_str());
        // until bionics can be flagged as non-removable
        add_msg(m_neutral, _("You jiggle your parts back into their familiar places."));
        add_msg(m_good, _("Successfully removed %s."), bionics[b_id].name.c_str());
        // remove power bank provided by bionic
        max_power_level -= bionics[b_id].capacity;
        remove_bionic(b_id);
        if (b_id == "bio_reactor" || b_id == "bio_advreactor") {
            remove_bionic("bio_plutdump");
        }
        g->m.spawn_item(posx(), posy(), "burnt_out_bionic", 1);
    } else {
        add_memorial_log(pgettext("memorial_male", "Removed bionic: %s."),
                         pgettext("memorial_female", "Removed bionic: %s."),
                         bionics[b_id].name.c_str());
        bionics_uninstall_failure(this);
    }
    g->refresh_all();
    return true;
}

bool player::install_bionics(const itype &type, int skill_level)
{
    if( type.bionic.get() == nullptr ) {
        debugmsg("Tried to install NULL bionic");
        return false;
    }
    const std::string &bioid = type.bionic->bionic_id;
    if( bionics.count( bioid ) == 0 ) {
        popup("invalid / unknown bionic id %s", bioid.c_str());
        return false;
    }
    if (bioid == "bio_reactor" || bioid == "bio_advreactor") {
        if (has_bionic("bio_furnace") && has_bionic("bio_storage")) {
            popup(_("Your internal storage and furnace take up too much room!"));
            return false;
        } else if (has_bionic("bio_furnace")) {
            popup(_("Your internal furnace takes up too much room!"));
            return false;
        } else if (has_bionic("bio_storage")) {
            popup(_("Your internal storage takes up too much room!"));
            return false;
        }
    }
    if ((bioid == "bio_furnace" ) || (bioid == "bio_storage") || (bioid == "bio_reactor") || (bioid == "bio_advreactor")) {
        if (has_bionic("bio_reactor") || has_bionic("bio_advreactor")) {
            popup(_("Your installed reactor leaves no room!"));
            return false;
        }
    }
    if (bioid == "bio_reactor_upgrade" ){
        if (!has_bionic("bio_reactor")) {
            popup(_("There is nothing to upgrade!"));
            return false;
        }
    }
    if( has_bionic( bioid ) ) {
        if( !( bioid == "bio_power_storage" || bioid == "bio_power_storage_mkII" ) ) {
            popup(_("You have already installed this bionic."));
            return false;
        }
    }
    const int difficult = type.bionic->difficulty;
    int chance_of_success;
    if (skill_level != -1){
        chance_of_success = bionic_manip_cos(skill_level,
                                skill_level,
                                skill_level,
                                skill_level,
                                difficult);
    } else {
        ///\EFFECT_INT increases chance of success installing bionics with unspecified skill level
        chance_of_success = bionic_manip_cos(int_cur,
                                skillLevel( skilll_electronics ),
                                skillLevel( skilll_firstaid ),
                                skillLevel( skilll_mechanics ),
                                difficult);
    }

    if (!query_yn(
            _("WARNING: %i percent chance of genetic damage, blood loss, or damage to existing bionics! Install anyway?"),
            100 - chance_of_success)) {
        return false;
    }

    practice( skilll_electronics, int((100 - chance_of_success) * 1.5) );
    practice( skilll_firstaid, int((100 - chance_of_success) * 1.0) );
    practice( skilll_mechanics, int((100 - chance_of_success) * 0.5) );
    int success = chance_of_success - rng(0, 99);
    if (success > 0) {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
                         pgettext("memorial_female", "Installed bionic: %s."),
                         bionics[bioid].name.c_str());

        add_msg(m_good, _("Successfully installed %s."), bionics[bioid].name.c_str());
        add_bionic(bioid);

        if (bioid == "bio_ears") {
            add_bionic("bio_earplugs"); // automatically add the earplugs, they're part of the same bionic
        } else if (bioid == "bio_sunglasses") {
			add_bionic("bio_blindfold"); // automatically add the Optical Dampers, they're part of the same bionic
        } else if (bioid == "bio_reactor_upgrade") {
            remove_bionic("bio_reactor");
            remove_bionic("bio_reactor_upgrade");
            add_bionic("bio_advreactor");
        } else if (bioid == "bio_reactor" || bioid == "bio_advreactor") {
            add_bionic("bio_plutdump");
        }
    } else {
        add_memorial_log(pgettext("memorial_male", "Installed bionic: %s."),
                         pgettext("memorial_female", "Installed bionic: %s."),
                         bionics[bioid].name.c_str());
        bionics_install_failure(this, difficult, success);
    }
    g->refresh_all();
    return true;
}

void bionics_install_failure(player *u, int difficulty, int success)
{
    // "success" should be passed in as a negative integer representing how far off we
    // were for a successful install.  We use this to determine consequences for failing.
    success = abs(success);

    // it would be better for code reuse just to pass in skill as an argument from install_bionic
    // pl_skill should be calculated the same as in install_bionics
    ///\EFFECT_INT randomly decreases severity of bionics installation failure
    int pl_skill = u->int_cur * 4 +
                   u->skillLevel( skilll_electronics ) * 4 +
                   u->skillLevel( skilll_firstaid )    * 3 +
                   u->skillLevel( skilll_mechanics )   * 1;
    // Medical residents get a substantial assist here
    if (u->has_trait("PROF_MED")) {
        pl_skill += 6;
    }

    // for failure_level calculation, shift skill down to a float between ~0.4 - 30
    float adjusted_skill = float (pl_skill) - std::min( float (40),
                           float (pl_skill) - float (pl_skill) / float (10.0));

    // failure level is decided by how far off the character was from a successful install, and
    // this is scaled up or down by the ratio of difficulty/skill.  At high skill levels (or low
    // difficulties), only minor consequences occur.  At low skill levels, severe consequences
    // are more likely.
    int failure_level = int(sqrt(success * 4.0 * difficulty / float (adjusted_skill)));
    int fail_type = (failure_level > 5 ? 5 : failure_level);

    if (fail_type <= 0) {
        add_msg(m_neutral, _("The installation fails without incident."));
        return;
    }

    switch (rng(1, 5)) {
    case 1:
        add_msg(m_neutral, _("You flub the installation."));
        break;
    case 2:
        add_msg(m_neutral, _("You mess up the installation."));
        break;
    case 3:
        add_msg(m_neutral, _("The installation fails."));
        break;
    case 4:
        add_msg(m_neutral, _("The installation is a failure."));
        break;
    case 5:
        add_msg(m_neutral, _("You screw up the installation."));
        break;
    }

    if (u->has_trait("PROF_MED")) {
    //~"Complications" is USian medical-speak for "unintended damage from a medical procedure".
        add_msg(m_neutral, _("Your training helps you minimize the complications."));
    // In addition to the bonus, medical residents know enough OR protocol to avoid botching.
    // Take MD and be immune to faulty bionics.
        if (fail_type == 5) {
            fail_type = rng(1,3);
        }
    }

    if (fail_type == 3 && u->num_bionics() == 0) {
        fail_type = 2;    // If we have no bionics, take damage instead of losing some
    }

    switch (fail_type) {

    case 1:
        if (!(u->has_trait("NOPAIN"))) {
            add_msg(m_bad, _("It really hurts!"));
            u->mod_pain( rng(failure_level * 3, failure_level * 6) );
        }
        break;

    case 2:
        add_msg(m_bad, _("Your body is damaged!"));
        u->hurtall(rng(failure_level, failure_level * 2), u); // you hurt yourself
        break;

    case 3:
        if (u->num_bionics() <= failure_level && u->max_power_level == 0) {
            add_msg(m_bad, _("All of your existing bionics are lost!"));
        } else {
            add_msg(m_bad, _("Some of your existing bionics are lost!"));
        }
        for (int i = 0; i < failure_level && u->remove_random_bionic(); i++) {
            ;
        }
        break;

    case 4:
        add_msg(m_mixed, _("You do damage to your genetics, causing mutation!"));
        while (failure_level > 0) {
            u->mutate();
            failure_level -= rng(1, failure_level + 2);
        }
        break;

    case 5: {
        add_msg(m_bad, _("The installation is faulty!"));
        std::vector<std::string> valid;
        std::copy_if(begin(faulty_bionics), end(faulty_bionics), std::back_inserter(valid),
            [&](std::string const &id) { return !u->has_bionic(id); });

        if (valid.empty()) { // We've got all the bad bionics!
            if (u->max_power_level > 0) {
                int old_power = u->max_power_level;
                add_msg(m_bad, _("You lose power capacity!"));
                u->max_power_level = rng(0, u->max_power_level - 25);
                u->add_memorial_log(pgettext("memorial_male", "Lost %d units of power capacity."),
                                      pgettext("memorial_female", "Lost %d units of power capacity."),
                                      old_power - u->max_power_level);
            }
            // TODO: What if we can't lose power capacity?  No penalty?
        } else {
            const std::string& id = random_entry( valid );
            u->add_bionic( id );
            u->add_memorial_log(pgettext("memorial_male", "Installed bad bionic: %s."),
                                pgettext("memorial_female", "Installed bad bionic: %s."),
                                bionics[ id ].name.c_str());
        }
    }
    break;
    }
}

void player::add_bionic( std::string const &b )
{
    if( has_bionic( b ) ) {
        debugmsg( "Tried to install bionic %s that is already installed!", b.c_str() );
        return;
    }
    char newinv = ' ';
    for( auto &inv_char : bionic_chars ) {
        if( bionic_by_invlet( inv_char ) == nullptr ) {
            newinv = inv_char;
            break;
        }
    }

    int pow_up = bionics[b].capacity;
    max_power_level += pow_up;
    if ( b == "bio_power_storage" || b == "bio_power_storage_mkII" ) {
        add_msg_if_player(m_good, _("Increased storage capacity by %i."), pow_up);
        // Power Storage CBMs are not real bionic units, so return without adding it to my_bionics
        return;
    }

    my_bionics.push_back( bionic( b, newinv ) );
    if ( b == "bio_tools" || b == "bio_ears" ) {
        activate_bionic(my_bionics.size() -1);
    }
    recalc_sight_limits();
}

void player::remove_bionic(std::string const &b) {
    std::vector<bionic> new_my_bionics;
    for(auto &i : my_bionics) {
        if (b == i.id) {
            continue;
        }

        // Ears and earplugs and sunglasses and blindfold go together like peanut butter and jelly.
        // Therefore, removing one, should remove the other.
        if ((b == "bio_ears" && i.id == "bio_earplugs") ||
            (b == "bio_earplugs" && i.id == "bio_ears")) {
            continue;
        } else if ((b == "bio_sunglasses" && i.id == "bio_blindfold") ||
		           (b == "bio_blindfold" && i.id == "bio_sunglasses")) {
				   continue;
        }

        new_my_bionics.push_back(bionic(i.id, i.invlet));
    }
    my_bionics = new_my_bionics;
    recalc_sight_limits();
}

int player::num_bionics() const
{
    return my_bionics.size();
}

std::pair<int, int> player::amount_of_storage_bionics() const
{
    int lvl = max_power_level;

    // exclude amount of power capacity obtained via non-power-storage CBMs
    for( auto it : my_bionics ) {
        lvl -= bionics[it.id].capacity;
    }

    std::pair<int, int> results (0, 0);
    if (lvl <= 0) {
        return results;
    }

    int pow_mkI = bionics["bio_power_storage"].capacity;
    int pow_mkII = bionics["bio_power_storage_mkII"].capacity;

    while (lvl >= std::min(pow_mkI, pow_mkII)) {
        if ( one_in(2) ) {
            if (lvl >= pow_mkI) {
                results.first++;
                lvl -= pow_mkI;
            }
        } else {
            if (lvl >= pow_mkII) {
                results.second++;
                lvl -= pow_mkII;
            }
        }
    }
    return results;
}

bionic& player::bionic_at_index(int i)
{
    return my_bionics[i];
}

bionic* player::bionic_by_invlet( const long ch ) {
    for( auto &elem : my_bionics ) {
        if( elem.invlet == ch ) {
            return &elem;
        }
    }
    return nullptr;
}

// Returns true if a bionic was removed.
bool player::remove_random_bionic() {
    const int numb = num_bionics();
    if (numb) {
        int rem = rng(0, num_bionics() - 1);
        const auto bionic = my_bionics[rem];
        remove_bionic(bionic.id);
        recalc_sight_limits();
    }
    return numb;
}

void reset_bionics()
{
    bionics.clear();
    faulty_bionics.clear();
}

void load_bionic(JsonObject &jsobj)
{
    std::string id = jsobj.get_string("id");
    std::string name = _(jsobj.get_string("name").c_str());
    std::string description = _(jsobj.get_string("description").c_str());
    int on_cost = jsobj.get_int("act_cost", 0);

    bool toggled = jsobj.get_bool("toggled", false);
    // Requires ability to toggle
    int off_cost = jsobj.get_int("deact_cost", 0);

    int time = jsobj.get_int("time", 0);
    // Requires a non-zero time
    int react_cost = jsobj.get_int("react_cost", 0);

    int capacity = jsobj.get_int("capacity", 0);

    bool faulty = jsobj.get_bool("faulty", false);
    bool power_source = jsobj.get_bool("power_source", false);

    if (faulty) {
        faulty_bionics.push_back(id);
    }

    auto const result = bionics.insert(std::make_pair(std::move(id),
        bionic_data(std::move(name), power_source, toggled, on_cost, off_cost, react_cost, time,
                    capacity, std::move(description), faulty)));

    if (!result.second) {
        debugmsg("duplicate bionic id");
    }
}

void bionic::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("id", id);
    json.member("invlet", (int)invlet);
    json.member("powered", powered);
    json.member("charge", charge);
    json.end_object();
}

void bionic::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();
    id = jo.get_string("id");
    invlet = jo.get_int("invlet");
    powered = jo.get_bool("powered");
    charge = jo.get_int("charge");
}
