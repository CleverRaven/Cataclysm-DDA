#include "mod_manager.h" // IWYU pragma: associated

#include <algorithm>
#include <exception>

#include "color.h"
#include "debug.h"
#include "dependency_tree.h"
#include "output.h"
#include "string_formatter.h"
#include "string_id.h"
#include "translations.h"

mod_ui::mod_ui( mod_manager &mman )
    : active_manager( mman )
    , mm_tree( active_manager.get_tree() )
{
}

std::string mod_ui::get_information( const MOD_INFORMATION *mod )
{
    if( mod == nullptr ) {
        return "";
    }

    std::string info;

    if( !mod->authors.empty() ) {
        info += colorize( ngettext( "Author", "Authors", mod->authors.size() ),
                          c_light_blue ) + ": " + enumerate_as_string( mod->authors );
        if( mod->maintainers.empty() ) {
            info += "\n";
        } else {
            info += "  ";
        }
    }

    if( !mod->maintainers.empty() ) {
        info += colorize( ngettext( "Maintainer", "Maintainers", mod->maintainers.size() ),
                          c_light_blue ) + u8":\u00a0"/*non-breaking space*/ + enumerate_as_string( mod->maintainers ) + "\n";
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
        info += colorize( ngettext( "Dependency", "Dependencies", deps.size() ),
                          c_light_blue ) + ": " + str + "\n";
    }

    if( !mod->version.empty() ) {
        info += colorize( _( "Mod version" ), c_light_blue ) + ": " + mod->version + "\n";
    }

    if( !mod->description.empty() ) {
        info += _( mod->description ) + "\n";
    }

    std::string note = !mm_tree.is_available( mod->ident ) ? mm_tree.get_node(
                           mod->ident )->s_errors() : "";
    if( !note.empty() ) {
        info += colorize( note, c_red );
    }

    return info;
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
    } catch( std::exception & ) {
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
        //  (more than 0 active elements) && (active[0] is a CORE) && active[0] is not the add candidate
        if( !active_list.empty() && active_list[0]->core && active_list[0] != mod_to_add ) {
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
    } else {
        return;
    }

    mod_id modstring = active_list[newsel];
    mod_id selstring = active_list[oldsel];

    // we can shift!
    // switch values!
    active_list[newsel] = selstring;
    active_list[oldsel] = modstring;

    selection += selshift;
}

bool mod_ui::can_shift_up( size_t selection, const std::vector<mod_id> &active_list )
{
    // error catch for out of bounds
    if( selection >= active_list.size() ) {
        return false;
    }
    // dependencies of this active element
    std::vector<mod_id> dependencies = mm_tree.get_dependencies_of_X_as_strings(
                                           active_list[selection] );

    // figure out if we can move up!
    if( selection == 0 ) {
        // can't move up, don't bother trying
        return false;
    }
    // see if the mod at selection-1 is a) a core, or b) is depended on by this mod
    int newsel = selection - 1;

    mod_id newsel_id = active_list[newsel];
    bool newsel_is_dependency =
        std::find( dependencies.begin(), dependencies.end(), newsel_id ) != dependencies.end();

    return !newsel_id->core && !newsel_is_dependency;
}

bool mod_ui::can_shift_down( size_t selection, const std::vector<mod_id> &active_list )
{
    // error catch for out of bounds
    if( selection >= active_list.size() ) {
        return false;
    }
    std::vector<mod_id> dependents = mm_tree.get_dependents_of_X_as_strings(
                                         active_list[selection] );

    // figure out if we can move down!
    if( selection == active_list.size() - 1 ) {
        // can't move down, don't bother trying
        return false;
    }
    int newsel = selection;
    int oldsel = selection + 1;

    mod_id modstring = active_list[newsel];
    mod_id selstring = active_list[oldsel];
    bool sel_is_dependency =
        std::find( dependents.begin(), dependents.end(), selstring ) != dependents.end();

    return !modstring->core && !sel_is_dependency;
}
