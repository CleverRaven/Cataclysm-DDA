#include "mod_manager.h"
#include "input.h"
#include "output.h"
#include "debug.h"
#include <algorithm>

// Note: Functions for drawing of UI are moved to the file worldfactory.cpp.

mod_ui::mod_ui(mod_manager *mman)
{
    if (mman) {
        active_manager = mman;
        mm_tree = &active_manager->get_tree();
        set_usable_mods();
        DebugLog( D_INFO, DC_ALL ) << "mod_ui initialized";
    } else {
        DebugLog( D_ERROR, DC_ALL ) << "mod_ui initialized with NULL mod_manager pointer";
    }
}

mod_ui::~mod_ui()
{
    active_manager = NULL;
    mm_tree = NULL;
}

bool compare_mod_by_name(const MOD_INFORMATION *a, const MOD_INFORMATION *b)
{
    return a->name < b->name;
}

void mod_ui::set_usable_mods()
{
    std::vector<std::string> available_cores, available_supplementals;
    std::vector<std::string> ordered_mods;

    std::vector<MOD_INFORMATION *> mods;
    for( auto &modinfo_pair : active_manager->mod_map ) {
        if( !modinfo_pair.second->obsolete ) {
            mods.push_back(modinfo_pair.second);
        }
    }
    std::sort(mods.begin(), mods.end(), &compare_mod_by_name);

    for( auto modinfo : mods ) {
        switch(modinfo->_type) {
        case MT_CORE:
            available_cores.push_back(modinfo->ident);
            break;
        case MT_SUPPLEMENTAL:
            available_supplementals.push_back(modinfo->ident);
            break;
        }
    }
    std::vector<std::string>::iterator it = ordered_mods.begin();
    ordered_mods.insert(it, available_supplementals.begin(), available_supplementals.end());
    it = ordered_mods.begin();
    ordered_mods.insert(it, available_cores.begin(), available_cores.end());

    usable_mods = ordered_mods;
}

std::string mod_ui::get_information(MOD_INFORMATION *mod)
{
    if (mod == NULL) {
        return "";
    }
    std::string modident = mod->ident;
    std::string note = (!mm_tree->is_available(modident)) ? mm_tree->get_node(
                           modident)->s_errors() : "";

    std::stringstream info;

    // color the note red!
    if (!note.empty()) {
        std::stringstream newnote;
        newnote << "<color_red>" << note << "</color>";
        note = newnote.str();
    }
    std::vector<std::string> dependencies = mod->dependencies;
    std::vector<std::string> authors = mod->authors;
    std::string description = mod->description;
    std::string dependency_string = "";
    if (!dependencies.empty()) {
        DebugLog( D_PEDANTIC_INFO, DC_ALL ) << mod->name << " Dependencies --";
        for (size_t i = 0; i < dependencies.size(); ++i) {
            if (i > 0) {
                //~ delimiter for mod dependency enumeration
                dependency_string += pgettext("mod manager", ", ");
            }
            DebugLog( D_PEDANTIC_INFO, DC_ALL ) << "\t" << dependencies[i];
            if (active_manager->mod_map.find(dependencies[i]) != active_manager->mod_map.end()) {
                dependency_string += "[" + active_manager->mod_map[dependencies[i]]->name + "]";
            } else {
                dependency_string += "[<color_red>" + dependencies[i] + "</color>]";
            }
        }
        DebugLog( D_PEDANTIC_INFO, DC_ALL ) << "\n";
    }
    std::string author_string = "";
    if (!authors.empty()) {
        for (size_t i = 0; i < authors.size(); ++i) {
            if (i > 0) {
                //~ delimiter for mod author enumeration
                author_string += pgettext("mod manager", ", ");
            }
            author_string += authors[i];
        }
        info << string_format(ngettext("Author: %s\n", "Authors: %s\n", authors.size()),
                              author_string.c_str());
    } else {
        info << _("Authors: [UNKNOWN]\n");
    }

    if(!dependencies.empty()) {
        info << string_format(ngettext("Dependency: %s\n", "Dependencies: %s\n", dependencies.size()),
                              dependency_string.c_str());
    } else {
        info << _("Dependencies: [NONE]\n");
    }

    if(!description.empty()) {
        info << string_format(_("Description: %s\n"), description.c_str());
    } else {
        info << _("Description: [NONE]\n");
    }

    if (mod->_type == MT_SUPPLEMENTAL && !note.empty()) {
        info << note;
    }

    return info.str();
}

void mod_ui::try_add(const std::string &mod_to_add,
                     std::vector<std::string> &active_list)
{
    if (std::find(active_list.begin(), active_list.end(), mod_to_add) != active_list.end()) {
        // The same mod can not be added twice. That makes no sense.
        return;
    }
    MOD_INFORMATION &mod = *active_manager->mod_map[mod_to_add];
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
        if ((!active_list.empty()) && (active_manager->mod_map[active_list[0]]->_type == MT_CORE) &&
            (active_list[0] != mod_to_add)) {
            // remove existing core
            try_rem(0, active_list);
        }

        // add to start of active_list if it doesn't already exist in it
        active_list.insert(active_list.begin(), mod_to_add);
    } else { // _type == MT_SUPPLEMENTAL
        // now check dependencies and add them as necessary
        std::vector<std::string> mods_to_add;
        bool new_core = false;
        for (auto &i : dependencies) {
            if(std::find(active_list.begin(), active_list.end(), i) == active_list.end()) {
                if (active_manager->mod_map[i]->_type == MT_CORE) {
                    mods_to_add.insert(mods_to_add.begin(), i);
                    new_core = true;
                } else {
                    mods_to_add.push_back(i);
                }
            }
        }

        if (new_core && !active_list.empty()) {
            try_rem(0, active_list);
            active_list.insert(active_list.begin(), mods_to_add[0]);
            mods_to_add.erase(mods_to_add.begin());
        }
        // now add the rest of the dependencies serially to the end
        for (auto &i : mods_to_add) {
            active_list.push_back(i);
        }
        // and finally add the one we are trying to add!
        active_list.push_back(mod.ident);
    }
}

void mod_ui::try_rem(int selection, std::vector<std::string> &active_list)
{
    // first make sure that what we are looking for exists in the list
    if (selection >= (int)active_list.size()) {
        // trying to access an out of bounds value! quit early
        return;
    }
    std::string sel_string = active_list[selection];

    MOD_INFORMATION &mod = *active_manager->mod_map[active_list[selection]];

    std::vector<std::string> dependents = mm_tree->get_dependents_of_X_as_strings(mod.ident);

    // search through the rest of the active list for mods that depend on this one
    if (!dependents.empty()) {
        for (auto &i : dependents) {
            auto rem = std::find(active_list.begin(), active_list.end(), i);
            if (rem != active_list.end()) {
                active_list.erase(rem);
            }
        }
    }
    std::vector<std::string>::iterator rem = std::find(active_list.begin(), active_list.end(),
            sel_string);
    if (rem != active_list.end()) {
        active_list.erase(rem);
    }
}

void mod_ui::try_shift(char direction, int &selection, std::vector<std::string> &active_list)
{
    // error catch for out of bounds
    if (selection < 0 || selection >= (int)active_list.size()) {
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
    if (selection < 0 || selection >= (int)active_list.size()) {
        return false;
    }
    // dependencies of this active element
    std::vector<std::string> dependencies = mm_tree->get_dependencies_of_X_as_strings(
            active_list[selection]);

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

    if (active_manager->mod_map[modstring]->_type == MT_CORE ||
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
    if (selection < 0 || selection >= (int)active_list.size()) {
        return false;
    }
    std::vector<std::string> dependents = mm_tree->get_dependents_of_X_as_strings(
            active_list[selection]);

    int newsel;
    int oldsel;
    std::string selstring;
    std::string modstring;

    // figure out if we can move down!
    if (selection == (int)active_list.size() - 1) {
        // can't move down, don't bother trying
        return false;
    }
    newsel = selection;
    oldsel = selection + 1;

    modstring = active_list[newsel];
    selstring = active_list[oldsel];

    if (active_manager->mod_map[modstring]->_type == MT_CORE ||
        std::find(dependents.begin(), dependents.end(), selstring) != dependents.end()) {
        // can't move down due to a blocker
        return false;
    } else {
        // we can shift!
        return true;
    }
}
