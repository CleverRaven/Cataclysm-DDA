#include "mod_manager.h" // IWYU pragma: associated

#include <algorithm>
#include <exception>

#include "color.h"
#include "debug.h"
#include "dependency_tree.h"
#include "output.h"
#include "string_formatter.h"

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
        info += colorize( n_gettext( "Author", "Authors", mod->authors.size() ),
                          c_light_blue ) + ": " + enumerate_as_string( mod->authors );
        if( mod->maintainers.empty() ) {
            info += "\n";
        } else {
            info += "  ";
        }
    }

    if( !mod->maintainers.empty() ) {
        info += colorize( n_gettext( "Maintainer", "Maintainers", mod->maintainers.size() ),
                          c_light_blue ) + u8":\u00a0"/*non-breaking space*/ + enumerate_as_string( mod->maintainers ) + "\n";
    }

    if( !mod->dependencies.empty() ) {
        const auto &deps = mod->dependencies;
        std::string str = enumerate_as_string( deps.begin(), deps.end(), [&]( const mod_id & e ) {
            if( e.is_valid() ) {
                return string_format( "[%s]", e->name() );
            } else {
                return string_format( "[<color_red>%s</color>]", e.c_str() );
            }
        } );
        info += colorize( n_gettext( "Dependency", "Dependencies", deps.size() ),
                          c_light_blue ) + ": " + str + "\n";
    }

    if( !mod->version.empty() ) {
        info += colorize( _( "Mod version" ), c_light_blue ) + ": " + mod->version + "\n";
    }

    if( !mod->description.empty() ) {
        info += mod->description + "\n";
    }

    std::string note = !mm_tree.is_available( mod->ident ) ? mm_tree.get_node(
                           mod->ident )->s_errors() : "";
    if( !note.empty() ) {
        info += colorize( note, c_red );
    }

    return info;
}

bool mod_ui::try_add( const mod_id &mod_to_add, std::vector<mod_id> &active_list,
                      bool debugmsg_on_error )
{
    if( std::find( active_list.begin(), active_list.end(), mod_to_add ) != active_list.end() ) {
        // The same mod can not be added twice. That makes no sense.
        // But since it's already loaded, it's been added
        return true;
    }
    if( !mod_to_add.is_valid() ) {
        if( debugmsg_on_error ) {
            debugmsg( "Unable to load mod \"%s\".", mod_to_add.c_str() );
        }
        return false;
    }
    const MOD_INFORMATION &mod = *mod_to_add;
    bool errs;
    try {
        dependency_node *checknode = mm_tree.get_node( mod.ident );
        if( !checknode ) {
            return false;
        }
        errs = checknode->has_errors();
    } catch( std::exception & ) {
        errs = true;
    }

    if( errs ) {
        // cannot add, something wrong!
        return false;
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

        // Keep track of the mods we add, so we can roll it back
        std::vector<mod_id> added_so_far;
        added_so_far.reserve( mods_to_add.size() );
        // Try to add the mods we depend on one at a time, then ourself
        for( auto &i : mods_to_add ) {
            bool added = try_add( i, active_list, false );
            if( !added ) {
                for( const mod_id &to_remove : added_so_far )  {
                    auto spot = std::find( active_list.begin(), active_list.end(), to_remove );
                    active_list.erase( spot );
                }
                return false;
            } else {
                added_so_far.push_back( i );
            }
        }

        active_list.push_back( mod.ident );
        // Adjust for load_after
        for( auto it = active_list.begin(); it != active_list.end(); ) {
            const mod_id checking = *it;
            if( checking == mod.ident || checking->load_after.count( mod.ident ) == 0 ) {
                ++it;
                continue;
            }
            assert( it != active_list.end() );
            if( std::find( dependencies.begin(), dependencies.end(), checking ) != dependencies.end() ) {
                if( debugmsg_on_error ) {
                    debugmsg( "%s is a dependency of %s but wants to be loaded after it!", checking.c_str(),
                              mod.ident.c_str() );
                }
                return false;
            }
            assert( it != active_list.end() );
            std::vector<mod_id> mods_to_move;
            for( auto lower = it; lower != active_list.end(); ++lower ) {
                if( lower == it || *lower == mod.ident ) {
                    continue;
                }
                const mod_id &after = *lower;

                if( after->load_after.count( checking ) ||
                    std::find( after->dependencies.begin(), after->dependencies.end(),
                               checking ) != after->dependencies.end() ) {
                    mods_to_move.push_back( after );
                }
            }
            assert( it != active_list.end() );
            if( mods_to_move.empty() ) {
                it = active_list.erase( it );
                bool at_end = it == active_list.end();
                int index = std::distance( active_list.begin(), it );
                active_list.push_back( checking );
                it = active_list.begin();
                std::advance( it, at_end ? index + 1 : index );
            } else {
                ++it;
                debugmsg( "FIXME!" );
            }
        }

    }
    return true;
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
