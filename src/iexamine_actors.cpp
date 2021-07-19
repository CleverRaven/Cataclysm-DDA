#include "iexamine_actors.h"

#include "game.h"
#include "generic_factory.h"
#include "itype.h"
#include "map.h"
#include "mapgen_functions.h"
#include "map_iterator.h"
#include "messages.h"
#include "output.h"
#include "player.h"

void cardreader_examine_actor::consume_card( player &guy ) const
{
    if( !consume ) {
        return;
    }
    std::vector<itype_id> cards;
    for( const flag_id &flag : allowed_flags ) {
        for( const item *it : guy.all_items_with_flag( flag ) ) {
            itype_id card_type = it->typeId();
            if( std::find( cards.begin(), cards.end(), card_type ) == cards.end() ) {
                cards.push_back( card_type );
            }
        }
    }
    if( cards.size() > 1 ) {
        uilist query;
        query.text = _( "Use which item?" );
        for( size_t i = 0; i < cards.size(); ++i ) {
            query.entries.emplace_back( static_cast<int>( i ), true, -1, cards[i]->nname( 1 ) );
        }
        query.query();
        while( query.ret == UILIST_CANCEL ) {
            query.query();
        }
        guy.use_amount( cards[query.ret], 1 );
        return;
    }
    guy.use_amount( cards[0], 1 );
}

bool cardreader_examine_actor::apply( const tripoint &examp ) const
{
    bool open = true;

    map &here = get_map();
    if( map_regen ) {
        tripoint_abs_omt omt_pos( ms_to_omt_copy( here.getabs( examp ) ) );
        if( !run_mapgen_update_func( mapgen_id, omt_pos, nullptr, false ) ) {
            debugmsg( "Failed to apply magen function %s", mapgen_id );
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
void cardreader_examine_actor::call( player &guy, const tripoint &examp ) const
{
    bool open = false;
    map &here = get_map();

    bool has_item = false;
    for( const flag_id &flag : allowed_flags ) {
        if( guy.has_item_with_flag( flag ) ) {
            has_item = true;
            break;
        }
    }


    if( has_item && query_yn( _( query_msg ) ) ) {
        guy.mod_moves( -to_moves<int>( 1_seconds ) );
        open = apply( examp );
        for( monster &critter : g->all_monsters() ) {
            if( !despawn_monsters ) {
                break;
            }
            // Check 1) same overmap coords, 2) turret, 3) hostile
            if( ms_to_omt_copy( here.getabs( critter.pos() ) ) == ms_to_omt_copy( here.getabs( examp ) ) &&
                critter.has_flag( MF_ID_CARD_DESPAWN ) &&
                critter.attitude_to( guy ) == Creature::Attitude::HOSTILE ) {
                g->remove_zombie( critter );
            }
        }
        if( open ) {
            add_msg( _( success_msg ) );
            consume_card( guy );
        } else {
            add_msg( _( redundant_msg ) );
        }
    } else if( allow_hacking && query_yn( _( "Attempt to hack this card-reader?" ) ) ) {
        iexamine::try_start_hacking( guy, examp );
    }
}

void cardreader_examine_actor::load( const JsonObject &jo )
{
    mandatory( jo, false, "flags", allowed_flags );
    optional( jo, false, "consume_card", consume, true );
    optional( jo, false, "allow_hacking", allow_hacking, true );
    optional( jo, false, "despawn_monsters", despawn_monsters, true );
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

    if( terrain_changes.empty() && furn_changes.empty() && mapgen_id.empty() ) {
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
