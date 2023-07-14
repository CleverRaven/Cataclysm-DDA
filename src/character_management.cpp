#include "character_management.h"

#include "character.h"
#include "item.h"
#include "output.h"

static bool has_time_passed( const item &it, const std::string &var, const time_duration &interval )
{
    return calendar::turn - it.get_var( var, calendar::turn - 2 * interval ) > ( interval - 1_seconds );
}

static std::optional<int> reload_options_menu( const std::vector<item_location> &mags,
        const std::string it_name )
{
    int num_mags = mags.size();

    uilist rmenu;
    rmenu.text = _( string_format( "Reload your %s with what?", it_name ) );
    // To avoid having duplicate entries in the menu, only add the first entry with a unique name
    std::set<std::string> added_names;
    for( int i = 0; i < num_mags; ++i ) {
        std::string name = mags[i]->display_name();
        // Duplicate entry
        if( added_names.count( name ) != 0 ) {
            continue;
        }
        added_names.emplace( name );
        rmenu.addentry( i, true, MENU_AUTOASSIGN, name );
    }
    // Give them an option to chose nothing, too
    rmenu.addentry( num_mags, true, MENU_AUTOASSIGN, _( "Don't reload" ) );
    // Now, see what they want
    rmenu.query();
    // If they picked one of the valid mags
    if( rmenu.ret >= 0 && rmenu.ret < num_mags ) {
        return std::optional<int>( rmenu.ret );
    }
    return std::nullopt;
}

bool Character::try_autoreload( item &it, const char_autoreload::params &info )
{
    const std::string fail_var_name = "autoreload_failed_time";
    const std::string warn_var_name = "autoreload_warn_time";
    const bool is_npc = !is_avatar();

    // Just get the name once
    std::string it_name = it.tname();

    // If it's not too low, we'll just let it be
    int uses_left = it.ammo_remaining();
    if( uses_left > 9 ) {
        return false;
    }

    // And we'll only warn unless we're just about to run out
    if( uses_left > 1 ) {
        if( !is_npc && has_time_passed( it, warn_var_name, 8_seconds ) ) {
            add_msg_if_player( m_info, _( "Your %s is running low" ), it_name );
            it.set_var( warn_var_name, calendar::turn );
        }
        return false;
    }

    // If we tried to reload it recently but couldn't, avoid spamming the player
    if( it.has_var( fail_var_name ) && !has_time_passed( it, fail_var_name, 1_minutes ) ) {
        return false;
    }

    // Now that we're here, we've decided to reload
    // So, find all acceptable reloadables in the character inventory
    std::vector<item_location> mags;
    for( item_location &loc : all_items_loc() ) {
        if( !it.can_reload_with( *loc, false ) ) {
            continue;
        }
        // Only mags with sufficient ammo to avoid immediate reload, too!
        if( loc->ammo_remaining() < 2 ) {
            continue;
        }
        mags.push_back( loc );
    }

    // TODO: Have NPCs complain about this
    // No valid targets? Tell the player, and don't try to autoreload for a bit
    if( mags.empty() ) {
        if( !is_npc && info.warn_no_ammo ) {
            popup( "You don't have anything to reload your %s with!", it_name );
        }
        it.set_var( fail_var_name, calendar::turn );
        return false;
    }

    int num_mags = mags.size();
    // Index in mags of the magazine chosen to reload with
    int picked = -1;

    // Sort the mags for nice display/easy choice
    std::sort( mags.begin(), mags.end(), []( const item_location & l, const item_location & r ) {
        return l->ammo_remaining() > r->ammo_remaining();
    } );

    switch( info.reload_style ) {
        // The player will choose what to reload (and if they want to reload)
        case char_autoreload::RELOAD_CHOICE: {
            if( is_npc ) {
                debugmsg( "Called autoreload with player choice for an NPC" );
                return false;
            }
            std::optional<int> ret = reload_options_menu( mags, it_name );
            if( ret ) {
                picked = *ret;
            } else {
                // They chose not to reload, just set the timer to not bother them for a bit and exit
                it.set_var( fail_var_name, calendar::turn );
                return false;
            }
            break;
        }
        case char_autoreload::RELOAD_LEAST: {
            picked = mags.size() - 1;
            break;
        }
        default:
        case char_autoreload::RELOAD_MOST: {
            picked = 0;
            break;
        }
    }

    if( picked < 0 || picked >= num_mags ) {
        debugmsg( "Picked invalid mag for character autoreload" );
        return false;
    }

    // Try to reload and see how it goes
    bool status = it.reload( *this, mags[picked], 1 );
    if( !status ) {
        add_msg_if_player( m_bad, _( "Unable to reload %s!\n" ), it_name );
        return false;
    }

    add_msg_if_player( m_good, _( "You notice your %s is running low and reload it." ), it_name );

    // TODO: NPCs complain about running out of reloads
    if( mags.size() == 1 && info.warn_on_last && !is_npc ) {
        popup( _( "Checking your pockets, you see this will be the last reload for your %s." ),
               it_name );
    } else {
        add_msg_if_player( m_info, _( "You have %d remaining reloads for the %s" ),
                           num_mags - 1, it_name );
    }

    return true;
}
