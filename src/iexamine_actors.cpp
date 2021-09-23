#include "iexamine_actors.h"

#include "game.h"
#include "generic_factory.h"
#include "itype.h"
#include "map.h"
#include "mapgen_functions.h"
#include "map_iterator.h"
#include "messages.h"
#include "mtype.h"
#include "output.h"

void cardreader_examine_actor::consume_card( const std::vector<item_location> &cards ) const
{
    if( !consume ) {
        return;
    }
    std::vector<item_location> opts;
    for( const item_location &it : cards ) {
        const auto stacks = [&it]( const item_location & compare ) {
            return it->stacks_with( *compare );
        };
        if( std::any_of( opts.begin(), opts.end(), stacks ) ) {
            continue;
        }
        opts.push_back( it );
    }

    if( opts.size() == 1 ) {
        opts[0].remove_item();
        return;
    }

    uilist query;
    query.text = _( "Use which item?" );
    query.allow_cancel = false;
    for( size_t i = 0; i < opts.size(); ++i ) {
        query.entries.emplace_back( static_cast<int>( i ), true, -1, opts[i]->tname() );
    }
    query.query();
    opts[query.ret].remove_item();
}

std::vector<item_location> cardreader_examine_actor::get_cards( Character &you,
        const tripoint &examp )const
{
    std::vector<item_location> ret;

    for( const item_location &it : you.all_items_loc() ) {
        const auto has_card_flag = [&it]( const flag_id & flag ) {
            return it->has_flag( flag );
        };
        if( std::none_of( allowed_flags.begin(), allowed_flags.end(), has_card_flag ) ) {
            continue;
        }
        if( omt_allowed_radius ) {
            tripoint cardloc = it->get_var( "spawn_location_omt", tripoint_min );
            // Cards without a location are treated as valid
            if( cardloc == tripoint_min ) {
                ret.push_back( it );
                continue;
            }
            int dist = rl_dist( cardloc.xy(), ms_to_omt_copy( get_map().getabs( examp ) ).xy() );
            if( dist > *omt_allowed_radius ) {
                continue;
            }
        }

        ret.push_back( it );
    }

    return ret;
}

bool cardreader_examine_actor::apply( const tripoint &examp ) const
{
    bool open = true;

    map &here = get_map();
    if( map_regen ) {
        tripoint_abs_omt omt_pos( ms_to_omt_copy( here.getabs( examp ) ) );
        if( !run_mapgen_update_func( mapgen_id, omt_pos, nullptr, false ) ) {
            debugmsg( "Failed to apply magen function %s", mapgen_id.str() );
        }
        here.set_seen_cache_dirty( examp );
        here.set_transparency_cache_dirty( examp.z );
    } else {
        open = false;
        const tripoint_range<tripoint> points = here.points_in_radius( examp, radius );
        for( const tripoint &tmp : points ) {
            const auto ter_iter = terrain_changes.find( here.ter( tmp ).id() );
            const auto furn_iter = furn_changes.find( here.furn( tmp ).id() );
            if( ter_iter != terrain_changes.end() ) {
                here.ter_set( tmp, ter_iter->second );
                open = true;
            }
            if( furn_iter != furn_changes.end() ) {
                here.furn_set( tmp, furn_iter->second );
                open = true;
            }
        }
    }

    return open;
}

/**
 * Use id/hack reader. Using an id despawns turrets.
 */
void cardreader_examine_actor::call( Character &you, const tripoint &examp ) const
{
    bool open = false;
    map &here = get_map();

    std::vector<item_location> cards = get_cards( you, examp );

    if( !cards.empty() && query_yn( _( query_msg ) ) ) {
        you.mod_moves( -to_moves<int>( 1_seconds ) );
        open = apply( examp );
        for( monster &critter : g->all_monsters() ) {
            if( !despawn_monsters ) {
                break;
            }
            // Check 1) same overmap coords, 2) turret, 3) hostile
            if( ms_to_omt_copy( here.getabs( critter.pos() ) ) == ms_to_omt_copy( here.getabs( examp ) ) &&
                critter.has_flag( MF_ID_CARD_DESPAWN ) &&
                critter.attitude_to( you ) == Creature::Attitude::HOSTILE ) {
                g->remove_zombie( critter );
            }
        }
        if( open ) {
            add_msg( _( success_msg ) );
            consume_card( cards );
        } else {
            add_msg( _( redundant_msg ) );
        }
    } else if( allow_hacking && query_yn( _( "Attempt to hack this card-reader?" ) ) ) {
        iexamine::try_start_hacking( you, examp );
    }
}

void cardreader_examine_actor::load( const JsonObject &jo )
{
    mandatory( jo, false, "flags", allowed_flags );
    optional( jo, false, "consume_card", consume, true );
    optional( jo, false, "allow_hacking", allow_hacking, true );
    optional( jo, false, "despawn_monsters", despawn_monsters, true );
    optional( jo, false, "omt_allowed_radius", omt_allowed_radius );
    if( jo.has_string( "mapgen_id" ) ) {
        optional( jo, false, "mapgen_id", mapgen_id );
        map_regen = true;
    } else {
        optional( jo, false, "radius", radius, 3 );
        optional( jo, false, "terrain_changes", terrain_changes );
        optional( jo, false, "furn_changes", furn_changes );
    }
    optional( jo, false, "query", query, true );
    optional( jo, false, "query_msg", query_msg );
    mandatory( jo, false, "success_msg", success_msg );
    mandatory( jo, false, "redundant_msg", redundant_msg );

}

void cardreader_examine_actor::finalize() const
{
    if( allowed_flags.empty() ) {
        debugmsg( "Cardreader examine actor has no allowed card flags." );
    }

    for( const flag_id &flag : allowed_flags ) {
        if( !flag.is_valid() ) {
            debugmsg( "Cardreader uses flag %s that does not exist!", flag.str() );
        }
    }

    if( terrain_changes.empty() && furn_changes.empty() && mapgen_id.is_empty() ) {
        debugmsg( "Cardreader examine actor does not change either terrain or furniture" );
    }

    if( query && query_msg.empty() ) {
        debugmsg( "Cardreader is told to query, yet does not have a query message defined." );
    }

    if( allow_hacking && ( !furn_changes.empty() || terrain_changes.size() != 1 ||
                           terrain_changes.count( ter_str_id( "t_door_metal_locked" ) ) != 1 ||
                           terrain_changes.at( ter_str_id( "t_door_metal_locked" ) ) != ter_str_id( "t_door_metal_c" ) ) ) {
        debugmsg( "Cardreader allows hacking, but activites different that if hacked." );
    }
}

std::unique_ptr<iexamine_actor> cardreader_examine_actor::clone() const
{
    return std::make_unique<cardreader_examine_actor>( *this );
}
