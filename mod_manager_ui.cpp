#include "mod_manager.h"
#include "output.h"
#include "keypress.h"
#include "debug.h"

mod_ui::mod_ui(mod_manager *mman)
{
    active_manager = mman;
    DebugLog() << "mod_ui initialized\n";
}

mod_ui::~mod_ui()
{
    DebugLog() << "mod_ui destroyed\n";
}

int mod_ui::show_layering_ui()
{
    //active_manager->refresh_mod_list();
    DebugLog() << "mod_ui:: now showing layering ui\n";

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    const int header_y = 4;
    const int list_y = 6;

    std::vector<std::string> available_cores, available_supplementals;
    std::vector<std::string> ordered_mods, active_mods;

    std::vector<std::string> modlist[2];

    for (int i = 0; i < active_manager->mods.size(); ++i){
        MOD_INFORMATION *mod = active_manager->mods[i];
        //DebugLog() << "mod_ui:: mod<"<<i<<"> type = <"<<mod->type() << ">\n";
        //DebugLog() << "\tIDENT: "<<mod->ident<<"\n\tTYPE: "<<mod->_type<<"\n\tAUTHOR: "<<mod->author<<"\n\tNAME: "<<mod->name<<"\n";

        switch(mod->_type){
            case MT_CORE: available_cores.push_back(mod->ident); break;
            case MT_SUPPLEMENTAL: available_supplementals.push_back(mod->ident); break;
        }
    }
    std::vector<std::string>::iterator it = ordered_mods.begin();
    ordered_mods.insert(it, available_supplementals.begin(), available_supplementals.end());
    it = ordered_mods.begin();
    ordered_mods.insert(it, available_cores.begin(), available_cores.end());

    // set up windows for display
    WINDOW *mod_screen;//, *mod_infopanel, *mod_list, *mod_order;
    mod_screen = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    std::vector<std::string> headers;
    headers.push_back(" Mod List ");
    headers.push_back(" Mod Load Order ");

    int selection[2] = {0, 0};
    int active_header = 0;
    char ch;
//DebugLog() << "mod_ui:: Entering display loop\n";
    do{
//DebugLog() << "mod_ui:: Erasing mod_screen\n";
        werase(mod_screen);
        wrefresh(mod_screen);
//DebugLog() << "mod_ui:: drawing layering ui lines\n";
        draw_layering_ui_lines(mod_screen);
//DebugLog() << "mod_ui:: drawing headers\n";
        draw_headers(mod_screen, header_y, headers, active_header);
//DebugLog() << "mod_ui:: drawing ordered_mod list\n";
        draw_modlist(mod_screen, list_y, 0, ordered_mods, active_header == 0, selection[0]);
//DebugLog() << "mod_ui:: drawing active mod list\n";
        draw_modlist(mod_screen, list_y, FULL_SCREEN_WIDTH/2 + 2, active_mods, active_header == 1, selection[1]);

        wrefresh(mod_screen);
        refresh();
        if (gather_input() != 0){
            return -1;
        }

    }while(true);
}

void mod_ui::draw_layering_ui_lines(WINDOW *win)
{
    // make window border
    wborder(win, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                        LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
    // make appropriate lines
    mvwhline(win, 3, 1, 0, FULL_SCREEN_WIDTH - 2);
    mvwhline(win,FULL_SCREEN_HEIGHT - 7, 1, 0, FULL_SCREEN_WIDTH - 2);
    mvwhline(win, 5, 1, 0, (FULL_SCREEN_WIDTH/2)-4);
    mvwhline(win, 5, (FULL_SCREEN_WIDTH/2)+2, 0, (FULL_SCREEN_WIDTH/2)-3);

    mvwvline(win, 3, (FULL_SCREEN_WIDTH/2)-4, 0, FULL_SCREEN_HEIGHT - 10);
    mvwvline(win, 3, (FULL_SCREEN_WIDTH/2)+2, 0, FULL_SCREEN_HEIGHT - 10);
    // Add in connective characters
    mvwaddch(win, 3, 0, LINE_XXXO);
    mvwaddch(win, 5, 0, LINE_XXXO);
    mvwaddch(win, FULL_SCREEN_HEIGHT-7, 0, LINE_XXXO);
    mvwaddch(win, 5, (FULL_SCREEN_WIDTH/2)+2, LINE_XXXO);

    mvwaddch(win, 3, FULL_SCREEN_WIDTH-1, LINE_XOXX);
    mvwaddch(win, 5, FULL_SCREEN_WIDTH-1, LINE_XOXX);
    mvwaddch(win, FULL_SCREEN_HEIGHT-7, FULL_SCREEN_WIDTH-1, LINE_XOXX);
    mvwaddch(win, 5, (FULL_SCREEN_WIDTH/2)-4, LINE_XOXX);

    mvwaddch(win, 3, FULL_SCREEN_WIDTH/2 - 4, LINE_OXXX);
    mvwaddch(win, 3, FULL_SCREEN_WIDTH/2 + 2, LINE_OXXX);

    mvwaddch(win, FULL_SCREEN_HEIGHT-7, FULL_SCREEN_WIDTH/2 - 4, LINE_XXOX);
    mvwaddch(win, FULL_SCREEN_HEIGHT-7, FULL_SCREEN_WIDTH/2 + 2, LINE_XXOX);
}

void mod_ui::draw_headers(WINDOW *win, int sy, const std::vector<std::string> headers, unsigned char selected_header)
{
    const int header_space = FULL_SCREEN_WIDTH / headers.size();

    for (int i = 0; i < headers.size(); ++i){
        const int header_x = (header_space +3) * i + ((header_space - 4) - headers[i].size())/2;
        mvwprintz(win, sy, header_x , c_cyan, headers[i].c_str());
        if (selected_header == i)
        {
            mvwputch(win, sy, header_x - 2, c_red, '<');
            mvwputch(win, sy, header_x + headers[i].size() + 2, c_red, '>');
        }
    }
}

void mod_ui::draw_modlist(WINDOW *win, int sy, int sx, const std::vector<std::string> modlist, bool header_active, int last_selection)
{
//DebugLog() << "mod_ui:: Trying to draw a modlist starting at <"<<sx<<", "<<sy<<"> with list size of <"<<modlist.size()<<">\n";
    // draw the mod list!
    dependency_tree tree = active_manager->get_tree();

    for (int i = 0; i < modlist.size(); ++i){
        bool has_note = !tree.is_available(modlist[i]);
        std::string note = has_note? tree.get_node(modlist[i])->s_errors() : "";

        std::string selmarker = (last_selection == i)?">> " : "   ";
        nc_color selcol, namecol;
        if (header_active){
            selcol = (last_selection == i)?c_yellow : c_white;
        }else{
            selcol = (last_selection == i)?c_cyan : c_white;
        }

        if (has_note){
            namecol = c_red;
        }else{
            namecol = c_white;
        }

        mvwprintz(win, sy + i, sx + 1, selcol, selmarker.c_str());
        mvwprintz(win, sy + i, sx + 4, namecol, active_manager->mods[active_manager->mod_map[modlist[i]]]->name.c_str());
    }
    if (header_active && last_selection < modlist.size()){
        MOD_INFORMATION *selmod = active_manager->mods[active_manager->mod_map[modlist[last_selection]]];
        std::string note = (!tree.is_available(modlist[last_selection]))? tree.get_node(modlist[last_selection])->s_errors() : "";

        show_mod_information(win, FULL_SCREEN_WIDTH - 1, selmod, note);
    }
}

void mod_ui::show_mod_information(WINDOW *win, int width, MOD_INFORMATION *mod, std::string note)
{
    const int infopanel_start_y = FULL_SCREEN_HEIGHT - 6;
    const int infopanel_start_x = 1;
    const dependency_tree tree = active_manager->get_tree();

    std::stringstream info;
    info.str("");
    // color the note red!
    if (note.size() > 0){
        std::stringstream newnote;
        newnote << "<color_red>" << note << "</color>";
        note = newnote.str();
    }
    std::vector<std::string> dependencies = mod->dependencies;
    std::string dependency_string = "";
    if (dependencies.size() == 0){
        dependency_string = "[NONE]";
    }else{
        for (int i = 0; i < dependencies.size(); ++i){
            if (i > 0){
                dependency_string += ", ";
            }
            dependency_string += "[" + active_manager->mods[active_manager->mod_map[dependencies[i]]]->name + "]";
        }
    }
    info << "Name: \"" << mod->name << "\"  Author(s): " << mod->author << "\n";
    info << "Description: \""<<mod->description << "\"\n";
    info << "Dependencies: " << dependency_string << "\n";
    if (mod->_type == MT_SUPPLEMENTAL && note.size() > 0){
        info << note;
    }
    fold_and_print(win, infopanel_start_y, infopanel_start_x, width, c_white, info.str().c_str());
}

int mod_ui::gather_input()
{
    char ch = input();

    return -1;
}

void mod_ui::try_add(int selection, std::vector<std::string> modlist, std::vector<std::string> &active_list)
{

}
void mod_ui::try_rem(int selection, std::vector<std::string> &active_list)
{

}
void mod_ui::try_shift(char direction, int &selection, std::vector<std::string> &active_list)
{

}

bool mod_ui::can_shift_up(int selection, std::vector<std::string> active_list)
{

}
bool mod_ui::can_shift_down(int selection, std::vector<std::string> active_list)
{

}
