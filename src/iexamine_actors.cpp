#include "iexamine_actors.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <cmath>
#include <set>

#include "ammo_effect.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "condition.h"
#include "creature.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "effect_on_condition.h"
#include "explosion.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "game_constants.h"
#include "game_inventory.h"
#include "generic_factory.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapgen_functions.h"
#include "mapgendata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "point.h"
#include "ret_val.h"
#include "rng.h"
#include "talker.h"
#include "timed_event.h"
#include "translations.h"
#include "uilist.h"
#include "value_ptr.h"
#include "veh_appliance.h"

static const activity_id ACT_MORTAR_AIMING( "ACT_MORTAR_AIMING" );

static const ter_str_id ter_t_door_metal_c( "t_door_metal_c" );
static const ter_str_id ter_t_door_metal_locked( "t_door_metal_locked" );

void appliance_convert_examine_actor::load( const JsonObject &jo, const std::string & )
{
    optional( jo, false, "furn_set", furn_set );
    optional( jo, false, "ter_set", ter_set );
    mandatory( jo, false, "item", appliance_item );
}

void appliance_convert_examine_actor::call( Character &you, const tripoint_bub_ms &examp ) const
{
    if( !query_yn( _( "Connect %s to grid?" ), item::nname( appliance_item ) ) ) {
        return;
    }
    map &here = get_map();
    if( furn_set ) {
        here.furn_set( examp, *furn_set );
    }
    if( ter_set ) {
        here.ter_set( examp, *ter_set );
    }

    place_appliance( here, examp, vpart_appliance_from_item( appliance_item ), you );
}

void appliance_convert_examine_actor::finalize() const
{
    if( furn_set && !furn_set->is_valid() ) {
        debugmsg( "Invalid furniture id %s in appliance_convert action", furn_set->str() );
    }
    if( ter_set && !ter_set->is_valid() ) {
        debugmsg( "Invalid terrain id %s in appliance_convert action", ter_set->str() );
    }

    if( !appliance_item.is_valid() ) {
        debugmsg( "Invalid appliance item %s in appliance_convert action", appliance_item.str() );
    } else if( !vpart_appliance_from_item( appliance_item ).is_valid() ) {
        // This will never actually trigger now, but is here if the semantics of vpart_appliance_from_item change
        debugmsg( "In appliance_convert action, %s does not correspond to an appliance",
                  appliance_item.str() );
    }
}

std::unique_ptr<iexamine_actor> appliance_convert_examine_actor::clone() const
{
    return std::make_unique<appliance_convert_examine_actor>( *this );
}

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
        const tripoint_bub_ms &examp )const
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
            tripoint_abs_omt cardloc = coords::project_to<coords::omt>(
                                           it->get_var( "spawn_location", tripoint_abs_ms::min ) );
            // Cards without a location are treated as valid
            if( cardloc == tripoint_abs_omt::min ) {
                ret.push_back( it );
                continue;
            }
            int dist = rl_dist( cardloc.xy(),
                                coords::project_to<coords::omt>( get_map().get_abs( examp ) ).xy() );
            if( dist > *omt_allowed_radius ) {
                continue;
            }
        }

        ret.push_back( it );
    }

    return ret;
}

bool cardreader_examine_actor::apply( const tripoint_bub_ms &examp ) const
{
    bool open = true;

    map &here = get_map();
    if( map_regen ) {
        tripoint_abs_omt omt_pos( coords::project_to<coords::omt>( here.get_abs( examp ) ) );
        const ret_val<void> has_colliding_vehicle = run_mapgen_update_func( mapgen_id, omt_pos, {}, nullptr,
                false );
        if( !has_colliding_vehicle.success() ) {
            debugmsg( "Failed to apply magen function %s, collision with %s", mapgen_id.str(),
                      has_colliding_vehicle.str() );
        }
        set_queued_points();
        here.set_seen_cache_dirty( examp );
        here.set_transparency_cache_dirty( examp.z() );
    } else {
        open = false;
        const tripoint_range<tripoint_bub_ms> points = here.points_in_radius( examp, radius );
        for( const tripoint_bub_ms &tmp : points ) {
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
void cardreader_examine_actor::call( Character &you, const tripoint_bub_ms &examp ) const
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
            if( coords::project_to<coords::omt>( here.get_abs( critter.pos_bub() ) ) ==
                coords::project_to<coords::omt>( here.get_abs( examp ) ) &&
                critter.has_flag( mon_flag_ID_CARD_DESPAWN ) &&
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
    } else if( allow_hacking && iexamine::can_hack( you ) &&
               query_yn( _( "Attempt to hack this card-reader?" ) ) ) {
        iexamine::try_start_hacking( you, examp );
    } else if( !allow_hacking && iexamine::can_hack( you ) ) {
        add_msg( _( "This card-reader cannot be hacked." ) );
    }
}

void cardreader_examine_actor::load( const JsonObject &jo, const std::string & )
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
                           terrain_changes.count( ter_t_door_metal_locked ) != 1 ||
                           terrain_changes.at( ter_t_door_metal_locked ) != ter_t_door_metal_c ) ) {
        debugmsg( "Cardreader allows hacking, but activities different that if hacked." );
    }
}

std::unique_ptr<iexamine_actor> cardreader_examine_actor::clone() const
{
    return std::make_unique<cardreader_examine_actor>( *this );
}

void eoc_examine_actor::call( Character &you, const tripoint_bub_ms &examp ) const
{
    map &here = get_map();

    dialogue d( get_talker_for( you ), nullptr );
    d.set_value( "this", here.furn( examp ).id().str() );
    d.set_value( "pos", here.get_abs( examp ) );
    for( const effect_on_condition_id &eoc : eocs ) {
        eoc->activate( d );
    }
}

void eoc_examine_actor::load( const JsonObject &jo, const std::string &src )
{
    for( JsonValue jv : jo.get_array( "effect_on_conditions" ) ) {
        eocs.emplace_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
}

void eoc_examine_actor::finalize() const
{
    for( const effect_on_condition_id &eoc : eocs ) {
        if( !eoc.is_valid() ) {
            debugmsg( "Invalid effect_on_condition_id: %s", eoc.str() );
        }
    }
}

std::unique_ptr<iexamine_actor> eoc_examine_actor::clone() const
{
    return std::make_unique<eoc_examine_actor>( *this );
}

void mortar_examine_actor::call( Character &you, const tripoint_bub_ms &examp ) const
{
    map &here = get_map();

    dialogue d( get_talker_for( you ), nullptr );

    if( has_condition && !condition( d ) ) {
        add_msg( condition_fail_msg.translated() );
        return;
    }

    std::vector<ammotype> expected_ammo_types = ammo_type;
    inventory_filter_preset preset( [expected_ammo_types]( const item_location & loc ) {
        for( ammotype desired_ammo : expected_ammo_types ) {
            if( desired_ammo == loc->ammo_type() ) {
                return true;
            };
        }
        return false;
    } );
    inventory_pick_selector inv_s( you, preset );
    inv_s.add_nearby_items( PICKUP_RANGE );
    inv_s.add_character_items( you );
    inv_s.set_title( _( "Pick a projectile to be used." ) );

    if( inv_s.empty() ) {
        add_msg( _( "You have no rounds to use the mortar." ) );
        return;
    }

    item_location loc;
    if( inv_s.item_entry_count() == 0 ) {
        add_msg( _( "You have no rounds to use the mortar." ) );
        return;
    } else if( inv_s.item_entry_count() == 1 ) {
        loc = inv_s.get_only_choice().first;
    } else {
        loc = inv_s.execute();
    }

    if( loc == item_location::nowhere ) {
        return;
    }

    const int aim_range = range / 24;
    const tripoint_abs_omt pos_omt = project_to<coords::omt>( here.get_abs( examp ) );
    tripoint_abs_omt target = ui::omap::choose_point( "Pick a target.", pos_omt, false, aim_range );

    if( target == tripoint_abs_omt::invalid ) {
        return;
    }

    if( rl_dist( you.pos_abs_omt(), target ) <=
        std::ceil( static_cast<float>( MAX_VIEW_DISTANCE ) / ( 2 * SEEX ) ) ) {
        add_msg( _( "Target is too close." ) );
        return;
    }
    time_duration aim_dur = aim_duration.evaluate( d );
    you.assign_activity( ACT_MORTAR_AIMING, to_moves<int>( aim_dur ) );

    tripoint_abs_ms target_abs_ms = project_to<coords::ms>( target );

    const int deviation = ( aim_deviation.evaluate( d ) * rl_dist( you.pos_abs(), target_abs_ms ) / 2 );
    // aim at the center of OMT, but with some deviation
    // we just assume mortar projectiles fall at 90 degrees, duh
    target_abs_ms.x() += rng_float( 12 + deviation, 12 - deviation );
    target_abs_ms.y() += rng_float( 12 + deviation, 12 - deviation );
    // we can have edge cases with it if, for example, we target radio tower (high building, but with small profile)
    target_abs_ms.z() = overmap_buffer.highest_omt_point( project_to<coords::omt>( target_abs_ms ) );

    for( ammo_effect_str_id ammo_eff : loc.get_item()->ammo_data()->ammo->ammo_effects ) {
        if( ammo_eff.obj().aoe_explosion_data.power > 0 ) {
            get_timed_events().add( timed_event_type::EXPLOSION,
                                    calendar::turn + flight_time.evaluate( d ) + aim_dur,
                                    target_abs_ms, ammo_eff.obj().aoe_explosion_data );
        }

    }

    loc->charges--;
    if( loc->charges <= 0 ) {
        loc.remove_item();
    }

    d.set_value( "this", here.furn( examp ).id().str() );
    d.set_value( "pos", here.get_abs( examp ) );
    d.set_value( "target", target_abs_ms );
    for( const effect_on_condition_id &eoc : eocs ) {
        eoc->activate( d );
    }
}

void mortar_examine_actor::load( const JsonObject &jo, const std::string &src )
{
    mandatory( jo, false, "ammo", ammo_type );
    mandatory( jo, false, "range", range );
    if( jo.has_member( "condition" ) ) {
        read_condition( jo, "condition", condition, false );
        has_condition = true;
    }
    optional( jo, false, "condition_fail_msg", condition_fail_msg,
              to_translation( "You can't use this mortar." ) );

    aim_deviation = get_dbl_or_var( jo, "aim_deviation", false, 0.0f );
    aim_duration = get_duration_or_var( jo, "aim_duration", false, 0_seconds );
    flight_time = get_duration_or_var( jo, "flight_time", false, 0_seconds );

    for( JsonValue jv : jo.get_array( "effect_on_conditions" ) ) {
        eocs.emplace_back( effect_on_conditions::load_inline_eoc( jv, src ) );
    }
}

void mortar_examine_actor::finalize() const
{
    for( ammotype ammo : ammo_type ) {
        if( !ammo.is_valid() ) {
            debugmsg( "Invalid ammo type: %s", ammo.str() );
        }
    }
}

std::unique_ptr<iexamine_actor> mortar_examine_actor::clone() const
{
    return std::make_unique<mortar_examine_actor>( *this );
}
