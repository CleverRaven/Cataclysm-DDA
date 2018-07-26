#include "mod_manager.h"
#include "input.h"
#include "output.h"
#include "debug.h"
#include "translations.h"
#include "string_formatter.h"
#include "dependency_tree.h"

#include <algorithm>

mod_ui::mod_ui( mod_manager &mman )
    : active_manager( mman )
    , mm_tree( active_manager.get_tree() )
{
}

std::string mod_ui::get_information( const MOD_INFORMATION *mod )
{
    if( mod == NULL ) {
        return "";
    }

    std::ostringstream info;

    if( !mod->authors.empty() ) {
        info << "<color_light_blue>" << ngettext( "Author", "Authors", mod->authors.size() )
             << "</color>: " << enumerate_as_string( mod->authors ) << "\n";
    }

    if( !mod->maintainers.empty() ) {
        info << "<color_light_blue>" << ngettext( "Maintainer", "Maintainers", mod->maintainers.size() )
             << "</color>: " << enumerate_as_string( mod->maintainers ) << "\n";
    }

    if( !mod->dependencies.empty() ) {
        const auto &deps = mod->dependencies;
        auto str = enumerate_as_string( deps.begin(), deps.end(), [&]( const mod_id & e ) {
            if( e.is_valid() ) {
                return string_format( "[%s]", e->name() );
            } else {
                return string_format( "[<color_red>%s</color>]", e.c_str() );
            }
        } );
        info << "<color_light_blue>" << ngettext( "Dependency", "Dependencies", deps.size() )
             << "</color>: " << str << "\n";
    }

    if( !mod->version.empty() ) {
        info << "<color_light_blue>" << _( "Mod version" ) << "</color>: "
             << mod->version << std::endl;
    }

    if( !mod->description.empty() ) {
        info << _( mod->description.c_str() ) << "\n";
    }

    if( mod->need_lua() ) {
#ifdef LUA
        info << _( "This mod requires <color_green>Lua support</color>" ) << "\n";
#else
        info << _( "This mod requires <color_red>Lua support</color>" ) << "\n";
#endif
    }

    std::string note = !mm_tree.is_available( mod->ident ) ? mm_tree.get_node(
                           mod->ident )->s_errors() : "";
    if( !note.empty() ) {
        info << "<color_red>" << note << "</color>";
    }

    return info.str();
}

void mod_ui::try_add( const mod_id &mod_to_add,
                      std::vector<mod_id> &active_list )
{
    if( std::find( active_list.begin(), active_list.end(), mod_to_add ) != active_list.end() ) {
        // The same mod can not be added twice. That makes no sense.
        return;
    }
    if( !mod_to_add.is_valid() ) {
        debugmsg( "Unable to load mod \"%s\".", mod_to_add.c_str() );
        return;
    }
    const MOD_INFORMATION &mod = *mod_to_add;
    bool errs;
    try {
        dependency_node *checknode = mm_tree.get_node( mod.ident );
        if( !checknode ) {
            return;
        }
        errs = checknode->has_errors();
    } catch( std::exception &e ) {
        errs = true;
    }

    if( errs ) {
        // cannot add, something wrong!
        return;
    }
    // get dependencies of selection in the order that they would appear from the top of the active list
    std::vector<mod_id> dependencies = mm_tree.get_dependencies_of_X_as_strings( mod.ident );

    // check to see if mod is a core, and if so check to see if there is already a core in the mod list
    if( mod.core ) {
        //  (more than 0 active elements) && (active[0] is a CORE)                            &&    active[0] is not the add candidate
        if( !active_list.empty() && active_list[0]->core &&
            ( active_list[0] != mod_to_add ) ) {
            // remove existing core
            try_rem( 0, active_list );
        }

        // add to start of active_list if it doesn't already exist in it
        active_list.insert( active_list.begin(), mod_to_add );
    } else {
        // now check dependencies and add them as necessary
        std::vector<mod_id> mods_to_add;
        bool new_core = false;
        for( auto &i : dependencies ) {
            if( std::find( active_list.begin(), active_list.end(), i ) == active_list.end() ) {
                if( i->core ) {
                    mods_to_add.insert( mods_to_add.begin(), i );
                    new_core = true;
                } else {
                    mods_to_add.push_back( i );
                }
            }
        }

        if( new_core && !active_list.empty() ) {
            try_rem( 0, active_list );
            active_list.insert( active_list.begin(), mods_to_add[0] );
            mods_to_add.erase( mods_to_add.begin() );
        }
        // now add the rest of the dependencies serially to the end
        for( auto &i : mods_to_add ) {
            active_list.push_back( i );
        }
        // and finally add the one we are trying to add!
        active_list.push_back( mod.ident );
    }
}

void mod_ui::try_rem( size_t selection, std::vector<mod_id> &active_list )
{
    // first make sure that what we are looking for exists in the list
    if( selection >= active_list.size() ) {
        // trying to access an out of bounds value! quit early
        return;
    }
    const mod_id sel_string = active_list[selection];

    const MOD_INFORMATION &mod = *active_list[selection];

    std::vector<mod_id> dependents = mm_tree.get_dependents_of_X_as_strings( mod.ident );

    // search through the rest of the active list for mods that depend on this one
    for( auto &i : dependents ) {
        auto rem = std::find( active_list.begin(), active_list.end(), i );
        if( rem != active_list.end() ) {
            active_list.erase( rem );
        }
    }
    std::vector<mod_id>::iterator rem = std::find( active_list.begin(), active_list.end(),
                                        sel_string );
    if( rem != active_list.end() ) {
        active_list.erase( rem );
    }
}

void mod_ui::try_shift( char direction, size_t &selection, std::vector<mod_id> &active_list )
{
    // error catch for out of bounds
    if( selection >= active_list.size() ) {
        return;
    }

    // eliminates 'uninitialized variable' warning
    size_t newsel = 0;
    size_t oldsel = 0;
    mod_id selstring;
    mod_id modstring;
    int selshift = 0;

    // shift up (towards 0)
    if( direction == '+' && can_shift_up( selection, active_list ) ) {
        // see if the mod at selection-1 is a) a core, or b) is depended on by this mod
        newsel = selection - 1;
        oldsel = selection;

        selshift = -1;
    }
    // shift down (towards active_list.size()-1)
    else if( direction == '-' && can_shift_down( selection, active_list ) ) {
        newsel = selection;
        oldsel = selection + 1;

        selshift = +1;
    }

    if( !selshift ) { // false if selshift is 0, true if selshift is +/- 1: bool(int(0)) evaluates to false and bool(int(!0)) evaluates to true
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

bool mod_ui::can_shift_up( long selection, const std::vector<mod_id> &active_list )
{
    // error catch for out of bounds
    if( selection < 0 || selection >= ( int )active_list.size() ) {
        return false;
    }
    // dependencies of this active element
    std::vector<mod_id> dependencies = mm_tree.get_dependencies_of_X_as_strings(
                                           active_list[selection] );

    int newsel;
    mod_id modstring;

    // figure out if we can move up!
    if( selection == 0 ) {
        // can't move up, don't bother trying
        return false;
    }
    // see if the mod at selection-1 is a) a core, or b) is depended on by this mod
    newsel = selection - 1;

    modstring = active_list[newsel];

    if( modstring->core ||
        std::find( dependencies.begin(), dependencies.end(), modstring ) != dependencies.end() ) {
        // can't move up due to a blocker
        return false;
    } else {
        // we can shift!
        return true;
    }
}

bool mod_ui::can_shift_down( long selection, const std::vector<mod_id> &active_list )
{
    // error catch for out of bounds
    if( selection < 0 || selection >= ( int )active_list.size() ) {
        return false;
    }
    std::vector<mod_id> dependents = mm_tree.get_dependents_of_X_as_strings(
                                         active_list[selection] );

    int newsel;
    int oldsel;
    mod_id selstring;
    mod_id modstring;

    // figure out if we can move down!
    if( selection == ( int )active_list.size() - 1 ) {
        // can't move down, don't bother trying
        return false;
    }
    newsel = selection;
    oldsel = selection + 1;

    modstring = active_list[newsel];
    selstring = active_list[oldsel];

    if( modstring->core ||
        std::find( dependents.begin(), dependents.end(), selstring ) != dependents.end() ) {
        // can't move down due to a blocker
        return false;
    } else {
        // we can shift!
        return true;
    }
}
