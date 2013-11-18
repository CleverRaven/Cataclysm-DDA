#include "mod_manager.h"
#include "output.h"
#include "keypress.h"
#include "debug.h"

mod_ui::mod_ui(mod_manager *mman)
{
    if (mman) {
        active_manager = mman;
        mm_tree = active_manager->get_tree();
        set_usable_mods();
        DebugLog() << "mod_ui initialized\n";
    } else {
        DebugLog() << "mod_ui initialized with NULL mod_manager pointer\n";
    }
}

mod_ui::~mod_ui()
{
    active_manager = NULL;
    mm_tree = NULL;
}

void mod_ui::set_usable_mods()
{
    std::vector<std::string> available_cores, available_supplementals;
    std::vector<std::string> ordered_mods;

    for (int i = 0; i < active_manager->mods.size(); ++i) {
        MOD_INFORMATION *mod = active_manager->mods[i];

        switch(mod->_type) {
            case MT_CORE:
                available_cores.push_back(mod->ident);
                break;
            case MT_SUPPLEMENTAL:
                available_supplementals.push_back(mod->ident);
                break;
        }
    }
    std::vector<std::string>::iterator it = ordered_mods.begin();
    ordered_mods.insert(it, available_supplementals.begin(), available_supplementals.end());
    it = ordered_mods.begin();
    ordered_mods.insert(it, available_cores.begin(), available_cores.end());

    usable_mods = ordered_mods;
}
int mod_ui::show_layering_ui()
{
    DebugLog() << "mod_ui:: now showing layering ui\n";

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    const int header_y = 4;
    const int list_y = 6;
    const int shift_y = list_y + 1;

    DebugLog() << "iOffsetX <" << iOffsetX << ">\tiOffsetY <" << iOffsetY << ">\n";
    DebugLog() << "TERMX, TERMY  <" << TERMX << ", " << TERMY << ">\nFULL_SCREEN_[WIDTH|HEIGHT] <" << FULL_SCREEN_WIDTH << ", " << FULL_SCREEN_HEIGHT << ">\n";

    std::vector<std::string> available_cores, available_supplementals;
    std::vector<std::string> ordered_mods, active_mods;

    for (int i = 0; i < active_manager->mods.size(); ++i) {
        MOD_INFORMATION *mod = active_manager->mods[i];

        switch(mod->_type) {
            case MT_CORE:
                available_cores.push_back(mod->ident);
                break;
            case MT_SUPPLEMENTAL:
                available_supplementals.push_back(mod->ident);
                break;
        }
    }
    std::vector<std::string>::iterator it = ordered_mods.begin();
    ordered_mods.insert(it, available_supplementals.begin(), available_supplementals.end());
    it = ordered_mods.begin();
    ordered_mods.insert(it, available_cores.begin(), available_cores.end());

    // set up windows for display
    WINDOW *mod_screen;//, *mod_infopanel, *mod_list, *mod_order;
    mod_screen = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    werase(mod_screen);

    std::vector<std::string> headers;
    headers.push_back(" Mod List ");
    headers.push_back(" Mod Load Order ");

    int selection[2] = {0, 0};
    int active_header = 0;
    do {
        werase(mod_screen);
        draw_layering_ui_lines(mod_screen);
        draw_headers(mod_screen, header_y, headers, active_header);
        draw_modlist(mod_screen, list_y, 0, ordered_mods, active_header == 0, selection[0]);
        draw_modlist(mod_screen, list_y, FULL_SCREEN_WIDTH / 2 + 2, active_mods, active_header == 1, selection[1]);

        if (active_header == 1) {
            draw_shift_information(mod_screen, shift_y, FULL_SCREEN_WIDTH / 2 - 2, active_mods, selection[1]);
        }
        refresh();
        wrefresh(mod_screen);
        refresh();

        if (gather_input(active_header, selection[active_header], ordered_mods, active_mods) != 0) {
            return -1;
        }

    } while(true);
}

void mod_ui::draw_layering_ui_lines(WINDOW *win)
{
    // make window border
    wborder(win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    // make appropriate lines
    mvwhline(win, 3, 1, 0, FULL_SCREEN_WIDTH - 2);
    mvwhline(win, FULL_SCREEN_HEIGHT - 7, 1, 0, FULL_SCREEN_WIDTH - 2);
    mvwhline(win, 5, 1, 0, (FULL_SCREEN_WIDTH / 2) - 4);
    mvwhline(win, 5, (FULL_SCREEN_WIDTH / 2) + 2, 0, (FULL_SCREEN_WIDTH / 2) - 3);

    mvwvline(win, 3, (FULL_SCREEN_WIDTH / 2) - 4, 0, FULL_SCREEN_HEIGHT - 10);
    mvwvline(win, 3, (FULL_SCREEN_WIDTH / 2) + 2, 0, FULL_SCREEN_HEIGHT - 10);
    // Add in connective characters
    mvwaddch(win, 3, 0, LINE_XXXO);
    mvwaddch(win, 5, 0, LINE_XXXO);
    mvwaddch(win, FULL_SCREEN_HEIGHT - 7, 0, LINE_XXXO);
    mvwaddch(win, 5, (FULL_SCREEN_WIDTH / 2) + 2, LINE_XXXO);

    mvwaddch(win, 3, FULL_SCREEN_WIDTH - 1, LINE_XOXX);
    mvwaddch(win, 5, FULL_SCREEN_WIDTH - 1, LINE_XOXX);
    mvwaddch(win, FULL_SCREEN_HEIGHT - 7, FULL_SCREEN_WIDTH - 1, LINE_XOXX);
    mvwaddch(win, 5, (FULL_SCREEN_WIDTH / 2) - 4, LINE_XOXX);

    mvwaddch(win, 3, FULL_SCREEN_WIDTH / 2 - 4, LINE_OXXX);
    mvwaddch(win, 3, FULL_SCREEN_WIDTH / 2 + 2, LINE_OXXX);

    mvwaddch(win, FULL_SCREEN_HEIGHT - 7, FULL_SCREEN_WIDTH / 2 - 4, LINE_XXOX);
    mvwaddch(win, FULL_SCREEN_HEIGHT - 7, FULL_SCREEN_WIDTH / 2 + 2, LINE_XXOX);
}

void mod_ui::draw_headers(WINDOW *win, int sy, const std::vector<std::string> headers, unsigned char selected_header)
{
    const int header_space = FULL_SCREEN_WIDTH / headers.size();

    for (int i = 0; i < headers.size(); ++i) {
        const int header_x = (header_space + 3) * i + ((header_space - 4) - headers[i].size()) / 2;
        mvwprintz(win, sy, header_x , c_cyan, headers[i].c_str());
        if (selected_header == i) {
            mvwputch(win, sy, header_x - 2, c_red, '<');
            mvwputch(win, sy, header_x + headers[i].size() + 2, c_red, '>');
        }
    }
}

void mod_ui::draw_modlist(WINDOW *win, int sy, int sx, const std::vector<std::string> modlist, bool header_active, int last_selection)
{
    // draw the mod list!
    for (int i = 0; i < modlist.size(); ++i) {
        bool has_note = !mm_tree->is_available(modlist[i]);
        std::string note = has_note ? mm_tree->get_node(modlist[i])->s_errors() : "";

        std::string selmarker = (last_selection == i) ? ">> " : "   ";
        nc_color selcol, namecol;
        if (header_active) {
            selcol = (last_selection == i) ? c_yellow : c_white;
        } else {
            selcol = (last_selection == i) ? c_cyan : c_white;
        }

        if (has_note) {
            namecol = c_red;
        } else {
            namecol = c_white;
        }

        mvwprintz(win, sy + i, sx + 1, selcol, selmarker.c_str());
        mvwprintz(win, sy + i, sx + 4, namecol, active_manager->mods[active_manager->mod_map[modlist[i]]]->name.c_str());
    }
    if (header_active && last_selection < modlist.size()) {
        MOD_INFORMATION *selmod = active_manager->mods[active_manager->mod_map[modlist[last_selection]]];
        std::string note = (!mm_tree->is_available(modlist[last_selection])) ? mm_tree->get_node(modlist[last_selection])->s_errors() : "";

        show_mod_information(win, FULL_SCREEN_WIDTH - 1, selmod, note);
    }
}

std::string mod_ui::get_information(MOD_INFORMATION *mod)
{
    if (mod == NULL){
        return "";
    }
    std::string modident = mod->ident;
    std::string note = (!mm_tree->is_available(modident)) ? mm_tree->get_node(modident)->s_errors() : "";

    std::stringstream info;

    // color the note red!
    if (note.size() > 0) {
        std::stringstream newnote;
        newnote << "<color_red>" << note << "</color>";
        note = newnote.str();
    }
    std::vector<std::string> dependencies = mod->dependencies;
    std::string dependency_string = "";
    if (dependencies.size() == 0) {
        dependency_string = "[NONE]";
    } else {
        DebugLog() << mod->name << " Dependencies --";
        for (int i = 0; i < dependencies.size(); ++i) {
            if (i > 0) {
                dependency_string += ", ";
            }
            DebugLog() << "\t"<<dependencies[i];
            if (active_manager->mod_map.find(dependencies[i]) != active_manager->mod_map.end()){
                dependency_string += "[" + active_manager->mods[active_manager->mod_map[dependencies[i]]]->name + "]";
            }else{
                dependency_string += "[<color_red>" + dependencies[i] + "</color>]";
            }
        }
        DebugLog() << "\n";
    }
    info << "Name: \"" << mod->name << "\"  Author(s): " << mod->author << "\n";
    info << "Description: \"" << mod->description << "\"\n";
    info << "Dependencies: " << dependency_string << "\n";
    if (mod->_type == MT_SUPPLEMENTAL && note.size() > 0) {
        info << note;
    }

    return info.str();
}

void mod_ui::show_mod_information(WINDOW *win, int width, MOD_INFORMATION *mod, std::string note)
{
    const int infopanel_start_y = FULL_SCREEN_HEIGHT - 6;
    const int infopanel_start_x = 1;

    std::stringstream info;
    info.str("");
    // color the note red!
    if (note.size() > 0) {
        std::stringstream newnote;
        newnote << "<color_red>" << note << "</color>";
        note = newnote.str();
    }
    std::vector<std::string> dependencies = mod->dependencies;
    std::string dependency_string = "";
    if (dependencies.size() == 0) {
        dependency_string = "[NONE]";
    } else {
        for (int i = 0; i < dependencies.size(); ++i) {
            if (i > 0) {
                dependency_string += ", ";
            }
            dependency_string += "[" + active_manager->mods[active_manager->mod_map[dependencies[i]]]->name + "]";
        }
    }
    info << "Name: \"" << mod->name << "\"  Author(s): " << mod->author << "\n";
    info << "Description: \"" << mod->description << "\"\n";
    info << "Dependencies: " << dependency_string << "\n";
    if (mod->_type == MT_SUPPLEMENTAL && note.size() > 0) {
        info << note;
    }
    fold_and_print(win, infopanel_start_y, infopanel_start_x, width, c_white, info.str().c_str());
}

void mod_ui::draw_shift_information(WINDOW *win, int sy, int sx, const std::vector<std::string> mod_list, const int selection)
{
    const std::string can_shift_true = "<color_cyan>";
    const std::string can_shift_false = "<color_magenta>";
    const std::string can_shift_close = "</color>";

    std::string shift_string = "";
    if (can_shift_up(selection, mod_list)) {
        shift_string += can_shift_true;
    } else {
        shift_string += can_shift_false;
    }
    shift_string += "+" + can_shift_close;

    shift_string += " ";

    if (can_shift_down(selection, mod_list)) {
        shift_string += can_shift_true;
    } else {
        shift_string += can_shift_false;
    }
    shift_string += "-" + can_shift_close;
    fold_and_print(win, sy, sx, 4, c_black, shift_string.c_str());
}

int mod_ui::gather_input(int &active_header, int &selection, std::vector<std::string> mod_list, std::vector<std::string> &active_mods_list)
{
    char ch = input();
    const int next_header = (active_header == 1) ? 0 : 1;
    const int prev_header = (active_header == 0) ? 1 : 0;

    const int input_header = active_header;

    int next_selection = selection + 1;
    int prev_selection = selection - 1;

    if (active_header == 0) {
        if (prev_selection < 0) {
            prev_selection = mod_list.size() - 1;
        }
        if (next_selection >= mod_list.size()) {
            next_selection = 0;
        }
    } else if (active_header == 1) {
        if (prev_selection < 0) {
            prev_selection = active_mods_list.size() - 1;
        }
        if (next_selection >= active_mods_list.size()) {
            next_selection = 0;
        }
    }


    switch (ch) {
        case 'j': // move down
            selection = next_selection;
            break;
        case 'k': // move up
            selection = prev_selection;
            break;
        case 'l': // switch active section
            active_header = next_header;
            break;
        case 'h': // switch active section
            active_header = prev_header;
            break;
        case '\n': // select
            if (active_header == 0) {
                try_add(selection, mod_list, active_mods_list);
            } else if (active_header == 1) {
                try_rem(selection, active_mods_list);
            }
            break;
        case '+': // move active mod up
        case '-': // move active mod down
            if (active_header == 1) {
                try_shift(ch, selection, active_mods_list);
            }
            //try_shift(ch, sel2, active_mods);
            break;
        case KEY_ESCAPE: // exit!
            return -1;
    }

    if (input_header == 1 && active_header == 1) {
        if (active_mods_list.size() == 0) {
            selection = -1;
        } else {
            if (selection < 0) {
                selection = 0;
            } else if (selection >= active_mods_list.size()) {
                selection = active_mods_list.size() - 1;
            }
        }
    }

    return 0;
}

void mod_ui::try_add(int selection, std::vector<std::string> modlist, std::vector<std::string> &active_list)
{
    MOD_INFORMATION &mod = *active_manager->mods[active_manager->mod_map[modlist[selection]]];
    bool errs;
    try {
        dependency_node *checknode = mm_tree->get_node(mod.ident);
        if (!checknode) {
            return;
        }
        errs = checknode->has_errors();
    } catch (std::exception &e) {
        errs = true;
    }

    if (errs) {
        // cannot add, something wrong!
        return;
    }
    // get dependencies of selection in the order that they would appear from the top of the active list
    std::vector<std::string> dependencies = mm_tree->get_dependencies_of_X_as_strings(mod.ident);

    // check to see if mod is a core, and if so check to see if there is already a core in the mod list
    if (mod._type == MT_CORE) {
        //  (more than 0 active elements) && (active[0] is a CORE)                            &&    active[0] is not the add candidate
        if ((active_list.size() > 0) && (active_manager->mods[active_manager->mod_map[active_list[0]]]->_type == MT_CORE) && (active_list[0] != modlist[selection])) {
            // remove existing core
            try_rem(0, active_list);
        }

        if (std::find(active_list.begin(), active_list.end(), modlist[selection]) == active_list.end()) {
            // add to start of active_list if it doesn't already exist in it
            active_list.insert(active_list.begin(), modlist[selection]);
        }
    } else { // _type == MT_SUPPLEMENTAL
        // see if this mod has already been added to the list
        std::vector<std::string>::iterator it = std::find(active_list.begin(), active_list.end(), mod.ident);

        // now check dependencies and add them as necessary
        std::vector<std::string> mods_to_add;
        bool new_core = false;
        for (int i = 0; i < dependencies.size(); ++i) {
            if(std::find(active_list.begin(), active_list.end(), dependencies[i]) == active_list.end()) {
                if (active_manager->mods[active_manager->mod_map[dependencies[i]]]->_type == MT_CORE) {
                    mods_to_add.insert(mods_to_add.begin(), dependencies[i]);
                    new_core = true;
                } else {
                    mods_to_add.push_back(dependencies[i]);
                }
            }
        }

        if (new_core && active_list.size() > 0) {
            try_rem(0, active_list);
            active_list.insert(active_list.begin(), mods_to_add[0]);
            mods_to_add.erase(mods_to_add.begin());
        }
        // now add the rest of the dependencies serially to the end
        for (int i = 0; i < mods_to_add.size(); ++i) {
            active_list.push_back(mods_to_add[i]);
        }
        // and finally add the one we are trying to add!
        active_list.push_back(mod.ident);
    }
}
void mod_ui::try_rem(int selection, std::vector<std::string> &active_list)
{
    // first make sure that what we are looking for exists in the list
    if (selection >= active_list.size()) {
        // trying to access an out of bounds value! quit early
        return;
    }
    std::string sel_string = active_list[selection];

    MOD_INFORMATION &mod = *active_manager->mods[active_manager->mod_map[active_list[selection]]];

    std::vector<std::string> dependents = mm_tree->get_dependents_of_X_as_strings(mod.ident);

    // search through the rest of the active list for mods that depend on this one
    if (dependents.size() > 0) {
        for (int i = 0; i < dependents.size(); ++i) {
            std::vector<std::string>::iterator rem = std::find(active_list.begin(), active_list.end(), dependents[i]);
            if (rem != active_list.end()) {
                active_list.erase(rem);
            }
        }
    }
    std::vector<std::string>::iterator rem = std::find(active_list.begin(), active_list.end(), sel_string);
    if (rem != active_list.end()) {
        active_list.erase(rem);
    }
}
void mod_ui::try_shift(char direction, int &selection, std::vector<std::string> &active_list)
{
    // error catch for out of bounds
    if (selection < 0 || selection >= active_list.size()) {
        return;
    }

    int newsel;
    int oldsel;
    std::string selstring;
    std::string modstring;
    int selshift = 0;

    // shift up (towards 0)
    if (direction == '+' && can_shift_up(selection, active_list)) {
        // see if the mod at selection-1 is a) a core, or b) is depended on by this mod
        newsel = selection - 1;
        oldsel = selection;

        selshift = -1;
    }
    // shift down (towards active_list.size()-1)
    else if (direction == '-' && can_shift_down(selection, active_list)) {
        newsel = selection;
        oldsel = selection + 1;

        selshift = +1;
    }

    if (!selshift) { // false if selshift is 0, true if selshift is +/- 1: bool(int(0)) evaluates to false and bool(int(!0)) evaluates to true
        return;
    }

    modstring = active_list[newsel];
    selstring = active_list[oldsel];

    // we can shift!
    // switch values!
    active_list[newsel] = selstring;
    active_list[oldsel] = modstring;

    selection += selshift;
}

bool mod_ui::can_shift_up(int selection, std::vector<std::string> active_list)
{
    // error catch for out of bounds
    if (selection < 0 || selection >= active_list.size()) {
        return false;
    }
    // dependencies of this active element
    std::vector<std::string> dependencies = mm_tree->get_dependencies_of_X_as_strings(active_list[selection]);

    int newsel;
    int oldsel;
    std::string selstring;
    std::string modstring;

    // figure out if we can move up!
    if (selection == 0) {
        // can't move up, don't bother trying
        return false;
    }
    // see if the mod at selection-1 is a) a core, or b) is depended on by this mod
    newsel = selection - 1;
    oldsel = selection;

    modstring = active_list[newsel];
    selstring = active_list[oldsel];

    if (active_manager->mods[active_manager->mod_map[modstring]]->_type == MT_CORE ||
            std::find(dependencies.begin(), dependencies.end(), modstring) != dependencies.end()) {
        // can't move up due to a blocker
        return false;
    } else {
        // we can shift!
        return true;
    }
}
bool mod_ui::can_shift_down(int selection, std::vector<std::string> active_list)
{
    // error catch for out of bounds
    if (selection < 0 || selection >= active_list.size()) {
        return false;
    }
    std::vector<std::string> dependents = mm_tree->get_dependents_of_X_as_strings(active_list[selection]);

    int newsel;
    int oldsel;
    std::string selstring;
    std::string modstring;

    // figure out if we can move down!
    if (selection == active_list.size() - 1) {
        // can't move down, don't bother trying
        return false;
    }
    newsel = selection;
    oldsel = selection + 1;

    modstring = active_list[newsel];
    selstring = active_list[oldsel];

    if (active_manager->mods[active_manager->mod_map[modstring]]->_type == MT_CORE ||
            std::find(dependents.begin(), dependents.end(), selstring) != dependents.end()) {
        // can't move down due to a blocker
        return false;
    } else {
        // we can shift!
        return true;
    }
}
