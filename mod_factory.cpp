#include "mod_factory.h"

#include "output.h"
#include "keypress.h"
// these aren't used for anything outside of this file specifically, so no need for them to be inside the header
#include <sstream>

#include <stdlib.h>
#include <fstream>
#include <algorithm>

// FILE I/O
#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif


mod_factory::mod_factory()
{
    //ctor
}

mod_factory::~mod_factory()
{
    //dtor
}

void mod_factory::refresh_mod_list()
{
    DebugLog() << "Attempting to refresh mod list\n";
    // starts in ./data, then works through /cores and /mods directories to collect modpack information
    const std::string data_folder = "data", core_folder = "/cores", mods_folder = "/mods";

    mods.clear(); // may need to delete manually?

    // try to open data folder
    DIR *data = opendir(data_folder.c_str());

    // if data doesn't exist, we can't continue as there is nothing to load
    if (!data)
    {
        return;
    }
    // close data
    closedir(data);

    // try to load cores
    if (!load_mods_from(data_folder + core_folder))
    {
        // there are no core mods for some reason?
    }
    // try to load supplementary mods
    if (!load_mods_from(data_folder + mods_folder))
    {
        // there are no supplemental mods?
    }
    // for debuggery purposes, print to debug log all of the mods found!
    print_debug_mods();
}

bool mod_factory::load_mods_from(std::string path)
{
    // return false on non-existance of folder path
    DIR *folder = opendir(path.c_str());

    if (!folder)
    {
        return false;
    }
    // look through folder and read in all of the subdirectories as mods
    DIR *subfolder;
    struct dirent *dp, *sdp;
    struct stat _buff;

    while ((dp = readdir(folder)))
    {
        if (stat(dp->d_name, &_buff) != 0x4)
        {
            // ignore "." and ".."
            if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
            {
                std::string subpath = path + "/" + dp->d_name;

                subfolder = opendir(subpath.c_str());
                MOD_INFORMATION *modinfo;

                // if we can't open it for some reason, abort this operation and continue
                if (!subfolder)
                {
                    continue;
                }
                // now we need to look through the folder to get the information file!
                else
                {
                    modinfo = new MOD_INFORMATION;
                    modinfo->_path = subpath;
                }
                if (!load_mod_info(modinfo))
                {
                    // malformed modinfo, continue
                    continue;
                }
                // save modinfo into storage vector
                mods.push_back(modinfo);
            }
        }
    }
    return true;
}

bool mod_factory::load_mod_info(MOD_INFORMATION *mod)
{
    // treat it as a catajson object
    const std::string modfile_name = "/modinfo.json";

    std::stringstream modfile;

    modfile << mod->path() << modfile_name;

    catajson moddata(modfile.str());

    if (!json_good())
    {
        return false;
    }
    // required, all others have defaults
    if (!moddata.has("ident") || !moddata.get("ident").is_string())
    {
        return false;
    }
    else
    {
        mod->_ident = moddata.get("ident").as_string();
    }

    // load each of the pieces of information
    if (!moddata.has("type") || !moddata.get("type").is_string())
    {
        mod->_type = MOD_UNKNOWN;
    }
    else
    {
        std::string type = moddata.get("type").as_string();
        if (type == "CORE")
        {
            mod->_type = MOD_CORE;
        }
        else if (type == "SUPPLEMENTAL")
        {
            mod->_type = MOD_SUPPLEMENTAL;
        }
        else
        {
            mod->_type = MOD_UNKNOWN;
        }
    }
    if (!moddata.has("author") || !moddata.get("author").is_string())
    {
        mod->_author = "Unknown Author";
    }
    else
    {
        mod->_author = moddata.get("author").as_string();
    }
    if (!moddata.has("name") || !moddata.get("name").is_string())
    {
        mod->_name = "No Name";
    }
    else
    {
        mod->_name = moddata.get("name").as_string();
    }

    if (!moddata.has("description") || !moddata.get("description").is_string())
    {
        mod->_desc = "No Description Available";
    }
    else
    {
        mod->_desc = moddata.get("description").as_string();
    }

    if (!moddata.has("dependencies"))
    {
        mod->_dependencies = std::vector<std::string>();
    }
    else
    {
        catajson mod_dependencies = moddata.get("dependencies");

        for (mod_dependencies.set_begin(); mod_dependencies.has_curr(); mod_dependencies.next())
        {
            if (mod_dependencies.curr().is_string())
            {
                mod->_dependencies.push_back(mod_dependencies.curr().as_string());
            }
        }
    }
}

void mod_factory::print_debug_mods()
{
    if (mods.size() == 0)
    {
        DebugLog() << "[NO MODS FOUND]\n";
    }
    for (int i = 0; i < mods.size(); ++i)
    {
        std::stringstream debugstring;

        debugstring << "***********************\n";
        debugstring << "\tName: " << mods[i]->name() << "\n";
        debugstring << "\tAuthor: " << mods[i]->author() << "\n";
        debugstring << "\tType: " << mods[i]->type() << "\n";
        debugstring << "\tIdent: " << mods[i]->ident() << "\n";
        debugstring << "\tDescription: \"" << mods[i]->desc() << "\"\n";
        debugstring << "\tMod Path: " << mods[i]->path() << "\n";
        if (mods[i]->dependencies().size() > 0)
        {
            debugstring << "\tDependencies:\n";
            std::vector<std::string> mod_dependencies = mods[i]->dependencies();
            for (int j = 0; j < mod_dependencies.size(); ++j)
            {
                debugstring << "\t\t" << mod_dependencies[j] << "\n";
            }
        }
        debugstring << "\n";

        DebugLog() << debugstring.str();
    }
}

// needs to be drawn out for 80x25 screen including border
void mod_factory::show_mod_layering_ui()
{
    // get list of mods, don't assume it has been refreshed lately though.
    refresh_mod_list();
    // build map of mods using their ident strings
    std::map<std::string, MOD_INFORMATION*> modmap;
    // build core/supplementary mod ident vectors
    std::vector<std::string> mod_cores, mod_supplements;
    std::vector<std::string> mod_full, active_mods;

    for (int i = 0; i < mods.size(); ++i)
    {
        modmap[mods[i]->ident()] = mods[i];
        switch(mods[i]->raw_type())
        {
            case MOD_CORE: mod_cores.push_back(mods[i]->ident()); break;
            case MOD_SUPPLEMENTAL: mod_supplements.push_back(mods[i]->ident()); break;
        }
    }
    // add them all to a single vector, in core then supplemental order
    std::vector<std::string>::iterator it = mod_full.begin();
    mod_full.insert(it, mod_supplements.begin(), mod_supplements.end());
    it = mod_full.begin();
    mod_full.insert(it, mod_cores.begin(), mod_cores.end());

    // map of supplemental mod ids and notes
    std::map<std::string, std::string> mod_notes;
    // go through mod_supplements and look for dependencies! If no dependency then ""
    for (int i = 0; i < mod_supplements.size(); ++i)
    {
        if (modmap[mod_supplements[i]]->dependencies().size() > 0)
        {
            std::vector<std::string> mod_dependencies = modmap[mod_supplements[i]]->dependencies();
            std::string note = "";
            for (int j = 0; j < mod_dependencies.size(); ++j)
            {
                if (modmap.find(mod_dependencies[j]) == modmap.end())
                {
                    if (note.size() == 0)
                    {
                        note += "Missing Dependency(ies): [";
                    }
                    else
                    {
                        note += ", ";
                    }
                    note += "\"" + mod_dependencies[j] + "\"";
                }
            }
            if (note.size() != 0)
            {
                note += "]\n";
                mod_notes[mod_supplements[i]] = note;
            }
        }
    }

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    // set up windows for display
    WINDOW *mod_screen;//, *mod_infopanel, *mod_list, *mod_order;
    mod_screen = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    std::vector<std::string> headers;
    headers.push_back(" Mod  List ");
    headers.push_back(" Mod  Load  Order ");

    char ch;
    int selection_group = 0;
    int sel = 0;
    int sel2 = 0;

    do
    {
        werase(mod_screen);

        // draw lines
        draw_layering_ui_lines(mod_screen);
        // draw information!

        for (int i = 0; i < headers.size(); ++i)
        {
            int header_x = (FULL_SCREEN_WIDTH/headers.size() + 3) * i + ((FULL_SCREEN_WIDTH/headers.size() - 4) - headers[i].size())/2;
            mvwprintz(mod_screen, 4, header_x , c_cyan, headers[i].c_str());
            if (selection_group == i)
            {
                mvwputch(mod_screen, 4, header_x - 2, c_red, '<');
                mvwputch(mod_screen, 4, header_x + headers[i].size() + 2, c_red, '>');
            }
        }

        int y = 6;
        const int infopanel_start_y = FULL_SCREEN_HEIGHT - 6;
        const int infopanel_start_x = 1;

        // draw the mod list!
        for (int i = 0; i < mod_full.size(); ++i)
        {
            bool has_note = mod_notes.find(mod_full[i]) != mod_notes.end();
            std::string note = has_note? mod_notes[mod_full[i]] : "";

            std::string selmarker = (sel == i)?">> " : "   ";
            nc_color selcol, namecol;
            if (selection_group == 0)
            {
                selcol = (sel == i)?c_yellow : c_white;
            }
            else
            {
                selcol = (sel == i)?c_cyan : c_white;
            }

            if (has_note)
            {
                namecol = c_red;
            }
            else
            {
                namecol = c_white;
            }

            mvwprintz(mod_screen, y + i, 1, selcol, selmarker.c_str());
            mvwprintz(mod_screen, y + i, 4, namecol, modmap[mod_full[i]]->name().c_str());

            if (sel == i && selection_group == 0)
            {
                show_mod_information(mod_screen, infopanel_start_x, infopanel_start_y, FULL_SCREEN_WIDTH-1, modmap[mod_full[i]], note);
            }
        }
        // draw the active mods
        for (int i = 0; i < active_mods.size(); ++i)
        {
            bool has_note = mod_notes.find(active_mods[i]) != mod_notes.end();
            std::string note = has_note? mod_notes[active_mods[i]] : "";

            std::string selmarker = (sel2 == i)?">> " : "   ";
            nc_color selcol, namecol;
            if (selection_group == 1)
            {
                selcol = (sel2 == i)?c_yellow : c_white;
            }
            else
            {
                selcol = (sel2 == i)?c_cyan : c_white;
            }

            if (has_note)
            {
                namecol = c_red;
            }
            else
            {
                namecol = c_white;
            }

            mvwprintz(mod_screen, y + i, (FULL_SCREEN_WIDTH/2)+2+1, selcol, selmarker.c_str());
            mvwprintz(mod_screen, y + i, (FULL_SCREEN_WIDTH/2)+2+4, namecol, modmap[active_mods[i]]->name().c_str());

            if (sel2 == i && selection_group == 1)
            {
                show_mod_information(mod_screen, infopanel_start_x, infopanel_start_y, FULL_SCREEN_WIDTH-1, modmap[active_mods[i]], note);
            }
        }

        if (selection_group == 1)
        {
            nc_color shift_color = c_cyan;
            std::string shiftstring = "";
            if (can_shift_up(sel2, active_mods, modmap))
            {
                // '+'
                shiftstring += "+";
            }
            else
            {
                shiftstring += " ";
            }
            shiftstring += " ";
            if (can_shift_down(sel2, active_mods, modmap))
            {
                // '-'
                shiftstring += "-";
            }
            else
            {
                shiftstring += " ";
            }
            mvwprintz(mod_screen, y + 1, (FULL_SCREEN_WIDTH/2)-2, shift_color, shiftstring.c_str());

        }

        wrefresh(mod_screen);
        refresh();

        ch = input();

        switch(ch)
        {
        case 'j': // move down
            if (selection_group == 0)
            {
                ++sel;
                if (sel >= mod_cores.size() + mod_supplements.size())
                {
                    sel = 0;
                }
            }
            else if (selection_group == 1)
            {
                ++sel2;
                if (sel2 >= active_mods.size())
                {
                    sel2 = 0;
                }
            }

            break;
        case 'k': // move up
            if (selection_group == 0)
            {
                --sel;
                if (sel < 0)
                {
                    sel = (mod_cores.size() + mod_supplements.size()) - 1;
                }
            }
            else if (selection_group == 1)
            {
                --sel2;
                if (sel2 < 0)
                {
                    sel2 = active_mods.size()-1;
                }
            }
            break;
        case 'l': // switch active section
            --selection_group;
            if (selection_group < 0)
            {
                selection_group = headers.size() - 1;
            }
            break;
        case 'h': // ^
            ++selection_group;
            if (selection_group >= headers.size())
            {
                selection_group = 0;
            }
            break;

        case '\n': // select
            if (selection_group == 0)
            {
                // try to add from mod_full -> active_mods
                try_add(sel, mod_full, active_mods, modmap);
            }
            else if (selection_group == 1)
            {
                // try to remove from active_mods
                try_rem(sel2, active_mods, modmap);
            }
            break;

        case '+': // move active mod up
        case '-': // move active mod down
            try_shift(ch, sel2, active_mods, modmap);
            break;
        case KEY_ESCAPE: // exit!
            return;
        }

        if (selection_group == 1)
        {
            if (sel2 < 0)
            {
                sel2 = 0;
            }
            else if (sel2 >= active_mods.size())
            {
                sel2 = active_mods.size() - 1;
            }
        }

    }while(true);
}

void mod_factory::show_mod_information(WINDOW *win, int sx, int sy, int width, MOD_INFORMATION *mod, std::string note)
{
    int yy = sy;
    int xx = sx;
    int ww = width;
    std::string dependency_string = "";
    std::vector<std::string> dependency_vector = mod->dependencies();
    if (dependency_vector.size() == 0)
    {
        dependency_string = "NONE";
    }
    else
    {
        for (int v = 0; v < dependency_vector.size(); ++v)
        {
            if (v == 0)
            {
                dependency_string += dependency_vector[v];
            }
            else
            {
                dependency_string += ", " + dependency_vector[v];
            }
        }
    }
    yy += fold_and_print(win, yy, xx, ww, c_white, "Mod Name: \"%s\"   Author(s): %s", mod->name().c_str(), mod->author().c_str());
    yy += fold_and_print(win, yy, xx, ww, c_white, "Description: \"%s\"", mod->desc().c_str());
    yy += fold_and_print(win, yy, xx, ww, c_white, "Dependencies: %s", dependency_string.c_str());
    if (mod->raw_type() == MOD_SUPPLEMENTAL && note.size() > 0)
    {
        fold_and_print(win, yy, xx, ww, c_red, "%s", note.c_str());
    }
}

void mod_factory::draw_layering_ui_lines(WINDOW *win)
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

void mod_factory::try_add(int selection, std::vector<std::string> modlist, std::vector<std::string>& activelist, std::map<std::string, MOD_INFORMATION* > modmap)
{
    MOD_INFORMATION &mod = *modmap[modlist[selection]];
    // check for dependencies!
    if (mod.dependencies().size() == 0)
    {
        // no dependencies, can add! figure out where to add it now!
        if (mod.raw_type() == MOD_CORE)
        {
            // check to see if the mod at 0 is a core! if it is then proceed
            if (activelist.size() > 0 && modmap[activelist[0]]->raw_type() == MOD_CORE)
            {
                // if there is a core already, we need to remove that first!
                try_rem(0, activelist, modmap);
            }
            // otherwise just insert at the front :D
            // now add the new core
            activelist.insert(activelist.begin(), modlist[selection]);
        }
        else if (mod.raw_type() == MOD_SUPPLEMENTAL)
        {
            // see if the mod already exists in the active list first
            std::vector<std::string>::iterator it = std::find(activelist.begin(), activelist.end(), mod.ident());
            // since there are no dependencies, just add it in :D
            if (it == activelist.end())
            {
                activelist.push_back(mod.ident());
            }
        }
    }
    else
    {
        // core mods CANNOT have dependencies, or else they will not get loaded!
        // make sure that the mod has not been added already
        std::vector<std::string>::iterator it = std::find(activelist.begin(), activelist.end(), mod.ident());
        if (it == activelist.end())
        {
            std::vector<std::string> mod_dependencies = mod.dependencies();

            for (int i = 0; i < mod_dependencies.size(); ++i)
            {
                // check for dependencies not existing in the list
                if (std::find(activelist.begin(), activelist.end(), mod_dependencies[i]) == activelist.end())
                {
                    // doesn't exist in list, return early
                    return;
                }
            }
            // assumes that all dependencies exist
            activelist.push_back(mod.ident());
        }
    }
}

// tries to remove the selected item, also will remove any items added that depend on this item
void mod_factory::try_rem(int selection, std::vector<std::string>& activelist, std::map<std::string, MOD_INFORMATION* > modmap)
{
    // first make sure that what we are looking for exists in the list
    if (selection >= activelist.size())
    {
        // trying to access an out of bounds value! quit early
        return;
    }
    std::string sel_string = activelist[selection];

    MOD_INFORMATION &mod = *modmap[activelist[selection]];
    // search through the rest of the active list for mods that depend on this one
    std::vector<std::string> all_dependants = get_mods_dependent_on(activelist[selection], activelist, modmap);
    if (all_dependants.size() > 0)
    {
        for (std::vector<std::string>::iterator it = all_dependants.begin(); it != all_dependants.end(); ++it)
        {
            // make sure that activelist still contains *it
            std::vector<std::string>::iterator rem = std::find(activelist.begin(), activelist.end(), *it);
            if (rem != activelist.end())
            {
                activelist.erase(rem);
            }
        }
    }
    std::vector<std::string>::iterator rem = std::find(activelist.begin(), activelist.end(), sel_string);
    if (rem != activelist.end())
    {
        activelist.erase(rem);
    }
}

std::vector<std::string> mod_factory::get_mods_dependent_on(std::string dependency, std::vector<std::string> activelist, std::map<std::string, MOD_INFORMATION*> modmap)
{
    std::vector<std::string> ret_ids;
    for (std::vector<std::string>::iterator it = activelist.begin(); it != activelist.end(); ++it)
    {
        std::vector<std::string> depends = modmap[*it]->dependencies();

        if (std::find(depends.begin(), depends.end(), dependency) != depends.end())
        {
            ret_ids.push_back(*it);

            std::vector<std::string> recurse = get_mods_dependent_on(*it, activelist, modmap);
            for (int i = 0; i < recurse.size(); ++i)
            {
                ret_ids.push_back(recurse[i]);
            }
        }
    }
    return ret_ids;
}

void mod_factory::try_shift(char direction, int &selection, std::vector<std::string >& activelist, std::map<std::string, MOD_INFORMATION* > modmap)
{
    // error catch for out of bounds
    if (selection < 0 || selection >= activelist.size())
    {
        return;
    }
    std::map<std::string, std::vector<std::string> > dependencymap; // used to check for movement allowance

    for (int i = 0; i < activelist.size(); ++i)
    {
        dependencymap[activelist[i]] = get_mods_dependent_on(activelist[i], activelist, modmap);
    }

    int newsel;
    int oldsel;
    std::string selstring;
    std::string modstring;
    int selshift = 0;

    // shift up (towards 0)
    if (direction == '+')
    {
        // figure out if we can move up!
        if (selection == 0)
        {
            // can't move up, don't bother trying
            return;
        }
        // see if the mod at selection-1 is a) a core, or b) is depended on by this mod
        newsel = selection - 1;
        oldsel = selection;

        selshift = -1;
    }
    // shift down (towards activelist.size()-1)
    else if (direction == '-')
    {
        // figure out if we can move down!
        if (selection == activelist.size() - 1)
        {
            // can't move down, don't bother trying
            return;
        }
        newsel = selection;
        oldsel = selection + 1;

        selshift = +1;
    }

    modstring = activelist[newsel];
    selstring = activelist[oldsel];

    if (modmap[modstring]->raw_type() == MOD_CORE ||
        std::find(dependencymap[modstring].begin(), dependencymap[modstring].end(), selstring) != dependencymap[modstring].end())
    {
        // can't move up due to a blocker
        // return
        return;
    }
    else
    {
        // we can shift!
        // switch values!
        activelist[newsel] = selstring;
        activelist[oldsel] = modstring;

        selection += selshift;
    }
}
bool mod_factory::can_shift_up(int selection, std::vector<std::string > activelist, std::map<std::string, MOD_INFORMATION* > modmap)
{
    char direction = '+';

    // error catch for out of bounds
    if (selection < 0 || selection >= activelist.size())
    {
        return false;
    }
    std::map<std::string, std::vector<std::string> > dependencymap; // used to check for movement allowance

    for (int i = 0; i < activelist.size(); ++i)
    {
        dependencymap[activelist[i]] = get_mods_dependent_on(activelist[i], activelist, modmap);
    }

    int newsel;
    int oldsel;
    std::string selstring;
    std::string modstring;
    int selshift = 0;

    // figure out if we can move up!
    if (selection == 0)
    {
        // can't move up, don't bother trying
        return false;
    }
    // see if the mod at selection-1 is a) a core, or b) is depended on by this mod
    newsel = selection - 1;
    oldsel = selection;

    selshift = -1;

    modstring = activelist[newsel];
    selstring = activelist[oldsel];

    if (modmap[modstring]->raw_type() == MOD_CORE ||
        std::find(dependencymap[modstring].begin(), dependencymap[modstring].end(), selstring) != dependencymap[modstring].end())
    {
        // can't move up due to a blocker
        // return
        return false;
    }
    else
    {
        // we can shift!
        // switch values!

        return true;
    }
}

bool mod_factory::can_shift_down(int selection, std::vector<std::string > activelist, std::map<std::string, MOD_INFORMATION* > modmap)
{
    char direction = '-';

    // error catch for out of bounds
    if (selection < 0 || selection >= activelist.size())
    {
        return false;
    }
    std::map<std::string, std::vector<std::string> > dependencymap; // used to check for movement allowance

    for (int i = 0; i < activelist.size(); ++i)
    {
        dependencymap[activelist[i]] = get_mods_dependent_on(activelist[i], activelist, modmap);
    }

    int newsel;
    int oldsel;
    std::string selstring;
    std::string modstring;
    int selshift = 0;


    // figure out if we can move down!
    if (selection == activelist.size() - 1)
    {
        // can't move down, don't bother trying
        return false;
    }
    newsel = selection;
    oldsel = selection + 1;

    selshift = +1;


    modstring = activelist[newsel];
    selstring = activelist[oldsel];

    if (modmap[modstring]->raw_type() == MOD_CORE ||
        std::find(dependencymap[modstring].begin(), dependencymap[modstring].end(), selstring) != dependencymap[modstring].end())
    {
        // can't move up due to a blocker
        // return
        return false;
    }
    else
    {
        // we can shift!
        // switch values!
        return true;
    }
}

bool mod_factory::consolidate_mod_layers(std::vector<std::string > layers)
{
    if (layers.empty())
    {
        // no layers to consolidate, something horrible happened!
        return false;
    }

    // go through layers from core to the last layered supplemental (if exists) and consolildate all of the information together
    for (std::vector<std::string>::iterator mod_it = layers.begin(); mod_it != layers.end(); ++mod_it)
    {
        MOD_INFORMATION *mod = mods[*mod_it];

        // pull all of the files from the raws folder and json folder
        std::vector<std::string> mod_jsons, mod_raws;
        const std::string json_folderpath = "/json", raw_folderpath = "/raw";
        // recursive search to pull files from each folder. Assume that they are already ordered in load-priority order
        // ** Use file_finder since it's really easy and reduces redundancy of code **

    }
}
