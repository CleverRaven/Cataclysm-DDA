#include "vehicle.h"

#include "game.h"
#include "player.h"
#include "item.h"
#include "itype.h"
#include "veh_type.h"
#include "npc.h"
#include "projectile.h"
#include "messages.h"
#include "translations.h"
#include "ui.h"

#include <algorithm>
#include <numeric>

static const itype_id fuel_type_battery( "battery" );
const efftype_id effect_on_roof( "on_roof" );

std::vector<vehicle_part *> vehicle::turrets()
{
    std::vector<vehicle_part *> res;

    for( auto &e : parts ) {
        if( e.hp > 0 && e.base.is_gun() ) {
            res.push_back( &e );
        }
    }
    return res;
}

std::vector<vehicle_part *> vehicle::turrets( const tripoint &target )
{
    std::vector<vehicle_part *> res = turrets();

    // exclude turrets not ready to fire or where target is out of range
    res.erase( std::remove_if( res.begin(), res.end(), [&]( const vehicle_part * e ) {
        return turret_query( *e ) != turret_status::ready ||
               rl_dist( global_part_pos3( *e ), target ) > e->base.gun_range();
    } ), res.end() );

    return res;
}

vehicle::turret_status vehicle::turret_query( const vehicle_part &pt ) const
{
    if( !pt.base.ammo_sufficient() ) {
        return turret_status::no_ammo;
    }

    auto ups = pt.base.get_gun_ups_drain() * pt.base.gun_current_mode().qty;
    if( ups > fuel_left( fuel_type_battery ) ) {
        return turret_status::no_power;
    }

    return turret_status::ready;
}

int vehicle::turret_fire( vehicle_part &pt )
{
    int shots = 0;

    item &gun = pt.base;
    if( !gun.is_gun() ) {
        return false;
    }

    turret_reload( pt );

    switch( turret_query( pt ) ) {
        case turret_status::no_ammo:
            add_msg( m_bad, string_format( _( "The %s is out of ammo." ), pt.name().c_str() ).c_str() );
            break;

        case turret_status::no_power:
            add_msg( m_bad, string_format( _( "The %s is not powered." ), pt.name().c_str() ).c_str() );
            break;

        case turret_status::ready: {
            // Clone the shooter and place them at turret on roof
            auto shooter = g->u;
            shooter.setpos( global_part_pos3( pt ) );
            shooter.add_effect( effect_on_roof, 1 );
            shooter.recoil = abs( velocity ) / 100 / 4;

            tripoint pos = shooter.pos();
            auto trajectory = g->pl_target_ui( pos, gun.gun_range(), &gun, TARGET_MODE_TURRET_MANUAL );
            g->draw_ter();

            if( !trajectory.empty() ) {
                auto mode = gun.gun_current_mode();
                shots = shooter.fire_gun( trajectory.back(), mode.qty, *mode );
            }
            break;
        }

        default:
            debugmsg( "unknown turret status" );
            break;
    }

    turret_unload( pt );
    drain( fuel_type_battery, gun.get_gun_ups_drain() * shots );

    return shots;
}

void vehicle::turrets_set_targeting()
{
    std::vector<vehicle_part *> turrets;
    std::vector<tripoint> locations;

    for( auto &p : parts ) {
        if( p.base.is_gun() && !p.info().has_flag( "MANUAL" ) ) {
            turrets.push_back( &p );
            locations.push_back( global_part_pos3( p ) );
        }
    }

    pointmenu_cb callback( locations );

    int sel = 0;
    while( true ) {
        uimenu menu;
        menu.text = _( "Set turret targeting" );
        menu.return_invalid = true;
        menu.callback = &callback;
        menu.selected = sel;
        menu.fselected = sel;
        menu.w_y = 2;

        for( auto &p : turrets ) {
            menu.addentry( -1, true, MENU_AUTOASSIGN, "%s [%s]", p->name().c_str(),
                           p->enabled ? _( "auto" ) : _( "manual" ) );
        }

        menu.query();
        if( menu.ret < 0 || menu.ret >= static_cast<int>( turrets.size() ) ) {
            break;
        }

        sel = menu.ret;
        turrets[ sel ]->enabled = !turrets[ sel ]->enabled;
    }
}

void vehicle::turrets_set_mode()
{
    std::vector<vehicle_part *> turrets;
    std::vector<tripoint> locations;

    for( auto &p : parts ) {
        if( p.base.is_gun() ) {
            turrets.push_back( &p );
            locations.push_back( global_part_pos3( p ) );
        }
    }

    pointmenu_cb callback( locations );

    int sel = 0;
    while( true ) {
        uimenu menu;
        menu.text = _( "Set turret firing modes" );
        menu.return_invalid = true;
        menu.callback = &callback;
        menu.selected = sel;
        menu.fselected = sel;
        menu.w_y = 2;

        for( auto &p : turrets ) {
            menu.addentry( -1, true, MENU_AUTOASSIGN, "%s [%s]",
                           p->name().c_str(), p->base.gun_current_mode().mode.c_str() );
        }

        menu.query();
        if( menu.ret < 0 || menu.ret >= static_cast<int>( turrets.size() ) ) {
            break;
        }

        sel = menu.ret;
        turrets[ sel ]->base.gun_cycle_mode();
    }
}

bool vehicle::turrets_aim()
{
    // reload all turrets and clear any existing targets
    auto opts = turrets();
    for( auto e : opts ) {
        turret_reload( *e );
        e->target.first  = tripoint_min;
        e->target.second = tripoint_min;
    }

    // find radius of a circle centered at u encompassing all points turrets can aim at
    int range = std::accumulate( opts.begin(), opts.end(), 0, [&]( const int lhs,
    const vehicle_part * e ) {
        if( turret_query( *e ) != turret_status::ready ) {
            return lhs;
        }

        tripoint pos = global_part_pos3( *e );
        const int rng = e->base.gun_range();

        int res = 0;
        res = std::max( res, rl_dist( g->u.pos(), { pos.x + rng, pos.y, pos.z } ) );
        res = std::max( res, rl_dist( g->u.pos(), { pos.x - rng, pos.y, pos.z } ) );
        res = std::max( res, rl_dist( g->u.pos(), { pos.x, pos.y + rng, pos.z } ) );
        res = std::max( res, rl_dist( g->u.pos(), { pos.x, pos.y - rng, pos.z } ) );
        return std::max( lhs, res );
    } );

    // fake gun item to aim
    item pointer( "vehicle_pointer" );

    tripoint pos = g->u.pos();
    std::vector<tripoint> trajectory;

    if( opts.empty() ) {
        add_msg( m_warning, _( "Can't aim turrets: all turrets are offline" ) );
    } else {
        trajectory = g->pl_target_ui( pos, range, &pointer, TARGET_MODE_TURRET );
        g->draw_ter();
    }

    if( !trajectory.empty() ) {
        // set target for any turrets in range
        for( auto e : turrets( trajectory.back() ) ) {
            e->target.second = trajectory.back();
        }
        ///\EFFECT_INT speeds up aiming of vehicle turrets
        g->u.moves = std::min( 0, g->u.moves - 100 + ( 5 * g->u.int_cur ) );
    }

    for( auto e : opts ) {
        turret_unload( *e );
    }

    return !trajectory.empty();
}

void vehicle::turret_reload( vehicle_part &pt )
{
    item &gun = pt.base;
    if( !gun.is_gun() || !gun.ammo_type() ) {
        return;
    }

    // try to reload using stored fuel from onboard vehicle tanks
    for( const auto &e : fuels_left() ) {
        const itype *fuel = item::find_type( e.first );
        if( !fuel->ammo || e.second < gun.ammo_required() ) {
            continue;
        }

        if( ( gun.ammo_current() == "null" && fuel->ammo->type == gun.ammo_type() ) ||
            ( gun.ammo_current() == e.first ) ) {

            int qty = gun.ammo_remaining();
            gun.ammo_set( e.first, e.second );

            drain( e.first, gun.ammo_remaining() - qty );

            return;
        }
    }

    // otherwise try to reload from turret cargo space
    for( auto it = pt.items.begin(); it != pt.items.end(); ++it ) {
        if( it->is_ammo() &&
            ( ( gun.ammo_current() == "null" && it->ammo_type() == gun.ammo_type() ) ||
              ( gun.ammo_current() == it->typeId() ) ) ) {

            int qty = gun.ammo_remaining();
            gun.ammo_set( it->typeId(), it->charges );

            it->charges -= gun.ammo_remaining() - qty;

            if( it->charges <= 0 ) {
                pt.items.erase( it );
            }

            return;
        }
    }
}

void vehicle::turret_unload( vehicle_part &pt )
{
    item &gun = pt.base;
    if( !gun.is_gun() || !gun.ammo_type() ||
        gun.ammo_current() == "null" || gun.ammo_remaining() == 0 ) {
        return;
    }

    // return fuels to onboard vehicle tanks
    if( fuel_capacity( gun.ammo_current() ) > 0 ) {
        refill( gun.ammo_current(), gun.ammo_remaining() );
        gun.ammo_unset();
        return;
    }

    // otherwise unload to cargo space
    item ammo( gun.ammo_current(), calendar::turn, gun.ammo_remaining() );
    gun.ammo_unset();

    // @todo check ammo is not liquid or otherwise irrecoverable

    for( item &e : pt.items ) {
        if( e.merge_charges( ammo ) ) {
            return;
        }
    }
    pt.items.push_back( std::move( ammo ) );
}

int vehicle::automatic_fire_turret( vehicle_part &pt )
{
    item &gun = pt.base;
    tripoint pos = global_part_pos3( pt );

    npc tmp;
    tmp.set_fake( true );
    tmp.add_effect( effect_on_roof, 1 );
    tmp.name = rmp_format( _( "<veh_player>The %s" ), pt.name().c_str() );
    tmp.set_skill_level( gun.gun_skill(), 8 );
    tmp.set_skill_level( skill_id( "gun" ), 4 );
    tmp.recoil = abs( velocity ) / 100 / 4;
    tmp.setpos( pos );
    tmp.str_cur = 16;
    tmp.dex_cur = 8;
    tmp.per_cur = 12;
    // Assume vehicle turrets are defending the player.
    tmp.attitude = NPCATT_DEFEND;

    int area = aoe_size( gun.ammo_effects() );
    if( area > 0 ) {
        area += area == 1 ? 1 : 2; // Pad a bit for less friendly fire
    }

    tripoint targ = pos;
    auto &target = pt.target;
    if( target.first == target.second ) {
        // Manual target not set, find one automatically
        const bool u_see = g->u.sees( pos );
        int boo_hoo;

        // @todo calculate chance to hit and cap range based upon this
        int range = std::min( gun.gun_range(), 12 );
        Creature *auto_target = tmp.auto_find_hostile_target( range, boo_hoo, area );
        if( auto_target == nullptr ) {
            if( u_see && boo_hoo ) {
                add_msg( m_warning, ngettext( "%s points in your direction and emits an IFF warning beep.",
                                              "%s points in your direction and emits %d annoyed sounding beeps.",
                                              boo_hoo ),
                         tmp.name.c_str(), boo_hoo );
            }
            return 0;
        }

        targ = auto_target->pos();
    } else if( target.first != target.second ) {
        // Target set manually
        // Make sure we didn't move between aiming and firing (it's a bug if we did)
        if( targ != target.first ) {
            target.second = target.first;
            return 0;
        }

        targ = target.second;
        // Remove the target
        target.second = target.first;
    } else {
        // Shouldn't happen
        target.first = target.second;
        return 0;
    }

    // notify player if player can see the shot
    if( g->u.sees( pos ) ) {
        add_msg( _( "The %1$s fires its %2$s!" ), name.c_str(), pt.name().c_str() );
    }

    auto mode = gun.gun_current_mode();
    return tmp.fire_gun( targ, mode.qty, *mode );
}
