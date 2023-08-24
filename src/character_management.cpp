#include "character_management.h"

#include "character.h"
#include "item.h"
#include "output.h"
#include "try_parse_integer.h"

static bool has_time_passed( const item &it, const std::string &var, const time_duration &interval )
{
    return calendar::turn - it.get_var( var, calendar::turn - 2 * interval ) > ( interval - 1_seconds );
}

static std::optional<int> reload_options_menu( const std::vector<item_location> &mags,
        const std::string &it_name )
{
    int num_mags = mags.size();

    uilist rmenu;
    rmenu.text = string_format( _( "Reload your %s with what?" ), it_name );
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

static void add_use_log_entry( item &it )
{
    if( !it.has_var( "autoreload_use_log" ) ) {
        return;
    }

    std::string log = it.get_var( "autoreload_use_log" );
    log = string_format( "%d:%s,%s",
                         it.ammo_remaining(),
                         calendar::turn.to_string(),
                         log );
    it.set_var( "autoreload_use_log", log );
}

static void clear_use_log( item &it )
{
    it.erase_var( "autoreload_use_log" );
}

static void init_use_log( item &it )
{
    it.set_var( "autoreload_use_log",
                string_format( "%d:%s", it.ammo_remaining(), calendar::turn.to_string() ) );
}

static std::vector<std::pair<int, time_point>> get_use_log( item &it )
{
    // The log is encoded as a string list of entries is the form amount:time,amount:time,...
    // Split those up and place them in a vector so we can examine them
    std::string log_str = it.get_var( "autoreload_use_log" );
    std::vector<std::pair<int, time_point>> log;

    // Use std::getline to split into entries at the commas
    std::istringstream to_split;
    to_split.str( log_str );
    for( std::string entry; std::getline( to_split, entry, ',' ); ) {
        // And use some dirty string manipulation to split each entry
        char *amt_str = &entry[0];
        char *time_str = &entry[1];
        while( *( time_str - 1 ) != ':' && static_cast<size_t>( time_str - amt_str ) < entry.size() ) {
            ++time_str;
        }
        // The parse will fail if the two strings aren't separated
        *( time_str - 1 ) = '\0';
        ret_val<int> amount = try_parse_integer<int>( amt_str, false );
        // Any failure to parse data will create an invalid log
        if( !amount.success() ) {
            debugmsg( "Failed to parse usage data for autoreload of %s", it.display_name() );
            return std::vector<std::pair<int, time_point>>();
        }
        log.emplace_back( amount.value(), time_point::from_string( time_str ) );
    }

    return log;
}

static ret_val<time_duration> use_log_charge_interval_estimate( item &it )
{
    if( !it.has_var( "autoreload_use_log" ) ) {
        return ret_val<time_duration>::make_failure( 0_seconds );
    }

    std::vector<std::pair<int, time_point>> log = get_use_log( it );

    // If that variable is set, we should have a log
    if( log.size() == 0 ) {
        debugmsg( "No usage data for autoreload of %s", it.display_name() );
        init_use_log( it );
    }
    // There's simply no data to make conclusion on
    if( log.size() < 2 ) {
        return ret_val<time_duration>::make_failure( 0_seconds );
    }

    // If the last N had the same or similar separation, we can assume that's
    // the rate consumption will occur at. Otherwise, we'll take the average, and
    // stop if we find a big outlier
    // N = 5
    time_duration expected = log[0].second - log[1].second;
    for( size_t i = 2; i < 5 && i < log.size(); ) {
        if( log[i - 1].second - log[i].second != expected ) {
            break;
        }
        // Doing the increment in the loop here slightly simplifies the next check
        ++i;
        // Which is checking if the loop would repeat after this. If it would not, we've gone
        // through all N (or the entire list) and had the same difference for each
        if( i == 5 || i == log.size() ) {
            return ret_val<time_duration>::make_success( expected );
        }
    }

    // Now, we get onto trickier estimation. We're going to look at the difference between the
    // first and second most recent entries then build an average time between uses going through
    // the list using that as a basis, stopping once we reach an outlier
    // Also, we know there are more than 2 entries due to the above check
    float avg_time = to_seconds<float>( expected );
    for( size_t i = 2; i < log.size(); ++i ) {
        float difference = to_seconds<float>( log[i - 1].second - log[i].second );
        if( avg_time * 0.1 > difference || avg_time < difference * 0.1 ) {
            break;
        }
        avg_time *= static_cast<float>( i ) / ( i + 1.f );
        avg_time += difference * ( 1.f / ( i + 1.f ) );
    }

    return ret_val<time_duration>::make_success( time_duration::from_seconds( avg_time ) );
}

static bool item_is_low( item &it )
{
    if( it.has_var( "autoreload_use_log" ) ) {
        ret_val<time_duration> time_left = use_log_charge_interval_estimate( it );
        if( time_left.success() ) {
            return ( it.ammo_remaining() - 1 ) * time_left.value() < 5_minutes;
        }
    }

    return it.ammo_remaining() < 9;
}

static bool item_almost_out( item &it )
{
    if( it.has_var( "autoreload_use_log" ) ) {
        ret_val<time_duration> time_left = use_log_charge_interval_estimate( it );
        if( time_left.success() ) {
            return ( it.ammo_remaining() - 1 ) * time_left.value() < 1_minutes;
        }
    }

    return it.ammo_remaining() < 1;
}

bool Character::try_autoreload( item &it, const char_autoreload::params &info )
{
    const std::string fail_var_name = "autoreload_failed_time";
    const std::string warn_var_name = "autoreload_warn_time";
    const bool is_npc = !is_avatar();

    // Just get the name once
    std::string it_name = it.tname();

    // If it's not too low, we'll just let it be
    if( !item_is_low( it ) ) {
        return false;
    }

    // And we'll only warn unless we're just about to run out
    if( !item_almost_out( it ) ) {
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
            popup( _( "You don't have anything to reload your %s with!" ), it_name );
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
    int moves_avoided = mags[picked].obtain_cost( *this );
    item_location loc = mags[picked].obtain( *this );
    moves_avoided += this->item_reload_cost( it, *loc, 1 );
    add_msg( m_good, "Avoided %d moves", moves_avoided );
    std::string mag_name = mags[picked]->display_name();
    bool status = it.reload( *this, mags[picked], 1 );
    if( !status ) {
        add_msg_if_player( m_bad, _( "Unable to reload %s!\n" ), it_name );
        return false;
    }

    add_msg_if_player( m_good, _( "You notice your %s is running low and reload it with %s." ), it_name,
                       mag_name );

    // TODO: NPCs complain about running out of reloads
    if( mags.size() == 1 && info.warn_on_last && !is_npc ) {
        popup( _( "Checking your pockets, you see this will be the last reload for your %s." ),
               it_name );
    } else {
        add_msg_if_player( m_info, _( "You have %d remaining reloads for the %s" ),
                           num_mags - 1, it_name );
    }

    if( it.has_var( "autoreload_use_log" ) ) {
        init_use_log( it );
    }

    return true;
}

void Character::autoreload_log_use( item &it )
{
    add_use_log_entry( it );
}

void Character::autoreload_activate( item &it )
{
    init_use_log( it );
}

void Character::autoreload_deactivate( item &it )
{
    clear_use_log( it );
}
