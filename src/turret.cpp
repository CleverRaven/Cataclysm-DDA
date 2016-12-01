#include "vehicle.h"

#include "game.h"
#include "player.h"
#include "item.h"
#include "itype.h"
#include "veh_type.h"
#include "vehicle_selector.h"
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
        if( !e.is_broken() && e.base.is_gun() ) {
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
        return turret_query( *e ).query() != turret_data::status::ready ||
               rl_dist( global_part_pos3( *e ), target ) > e->base.gun_range();
    } ), res.end() );
    return res;
}

turret_data vehicle::turret_query( vehicle_part &pt )
{
    if( !pt.is_turret() || pt.removed || pt.is_broken() ) {
        return turret_data();
    }
    return turret_data( this, &pt );
}

const turret_data vehicle::turret_query( const vehicle_part &pt ) const
{
    return const_cast<vehicle *>( this )->turret_query( const_cast<vehicle_part &>( pt ) );
}

turret_data vehicle::turret_query( const tripoint &pos )
{
    auto res = get_parts( pos, "TURRET" );
    return !res.empty() ? turret_query( *res.front() ) : turret_data();
}

const turret_data vehicle::turret_query( const tripoint &pos ) const
{
    return const_cast<vehicle *>( this )->turret_query( pos );
}

std::string turret_data::name() const
{
    return part->name();
}

item_location turret_data::base()
{
    return item_location( vehicle_cursor( *veh, veh->index_of_part( part ) ), &part->base );
}

const item_location turret_data::base() const
{
    return item_location( vehicle_cursor( *veh, veh->index_of_part( part ) ), &part->base );
}

long turret_data::ammo_remaining() const
{
    if( !veh || !part ) {
        return 0;
    }
    if( part->info().has_flag( "USE_TANKS" ) ) {
        return veh->fuel_left( ammo_current() );
    }
    return part->base.ammo_remaining();
}

long turret_data::ammo_capacity() const
{
    if( !veh || !part || part->info().has_flag( "USE_TANKS" ) ) {
        return 0;
    }
    return part->base.ammo_capacity();
}

const itype *turret_data::ammo_data() const
{
    if( !veh || !part ) {
        return nullptr;
    }
    if( part->info().has_flag( "USE_TANKS" ) ) {
        return ammo_current() != "null" ? item::find_type( ammo_current() ) : nullptr;
    }
    return part->base.ammo_data();
}


itype_id turret_data::ammo_current() const
{
    auto opts = ammo_options();
    if( opts.count( part->ammo_pref ) ) {
        return part->ammo_pref;
    }
    if( opts.count( part->info().default_ammo ) ) {
        return part->info().default_ammo;
    }
    if( opts.count( part->base.ammo_default() ) ) {
        return part->base.ammo_default();
    }
    return opts.empty() ? "null" : *opts.begin();
}

std::set<itype_id> turret_data::ammo_options() const
{
    std::set<itype_id> opts;

    if( !veh || !part ) {
        return opts;
    }

    if( !part->info().has_flag( "USE_TANKS" ) ) {
        if( part->base.ammo_current() != "null" ) {
            opts.insert( part->base.ammo_current() );
        }

    } else {
        for( const auto &e : veh->fuels_left() ) {
            const itype *fuel = item::find_type( e.first );
            if( fuel->ammo && fuel->ammo->type.count( part->base.ammo_type() ) &&
                e.second >= part->base.ammo_required() ) {

                opts.insert( fuel->get_id() );
            }
        }
    }

    return opts;
}

bool turret_data::ammo_select( const itype_id &ammo )
{
    if( !ammo_options().count( ammo ) ) {
        return false;
    }

    part->ammo_pref = ammo;
    return true;
}

std::set<std::string> turret_data::ammo_effects() const
{
    if( !veh || !part ) {
        return std::set<std::string>();
    }
    auto res = part->base.ammo_effects();
    if( part->info().has_flag( "USE_TANKS" ) && ammo_data() ) {
        res.insert( ammo_data()->ammo->ammo_effects.begin(), ammo_data()->ammo->ammo_effects.end() );
    }
    return res;
}

int turret_data::range() const
{
    if( !veh || !part ) {
        return 0;
    }
    int res = part->base.gun_range();
    if( part->info().has_flag( "USE_TANKS" ) && ammo_data() ) {
        res += ammo_data()->ammo->range;
    }
    return res;
}

bool turret_data::can_reload() const
{
    if( !veh || !part || part->info().has_flag( "USE_TANKS" ) ) {
        return false;
    }
    if( !part->base.magazine_integral() ) {
        return true; // always allow changing of magazines
    }
    return part->base.ammo_remaining() < part->base.ammo_capacity();
}

bool turret_data::can_unload() const
{
    if( !veh || !part || part->info().has_flag( "USE_TANKS" ) ) {
        return false;
    }
    return part->base.ammo_remaining() || part->base.magazine_current();
}

turret_data::status turret_data::query() const
{
    if( !veh || !part ) {
        return status::invalid;
    }

    if( part->info().has_flag( "USE_TANKS" ) ) {
        if( veh->fuel_left( ammo_current() ) < part->base.ammo_required() ) {
            return status::no_ammo;
        }

    } else {
        if( !part->base.ammo_sufficient() ) {
            return status::no_ammo;
        }
    }

    auto ups = part->base.get_gun_ups_drain() * part->base.gun_current_mode().qty;
    if( ups > veh->fuel_left( fuel_type_battery ) ) {
        return status::no_power;
    }

    return status::ready;
}

int turret_data::fire( player &p, const tripoint &target )
{
    if( !veh || !part ) {
        return 0;
    }

    int shots = 0;

    p.add_effect( effect_on_roof, 1 );

    // turrets are subject only to recoil_vehicle()
    int old = p.recoil;
    p.recoil = 0;

    auto mode = base()->gun_current_mode();

    auto ammo = ammo_current();
    long qty  = mode->ammo_required();

    if( part->info().has_flag( "USE_TANKS" ) ) {
        mode->ammo_set( ammo, std::min( qty * mode.qty, long( veh->fuel_left( ammo ) ) ) );
    }

    shots = p.fire_gun( target, mode.qty, *mode );

    if( part->info().has_flag( "USE_TANKS" ) ) {
        veh->drain( ammo, qty * shots );
        mode->ammo_unset();
    }

    veh->drain( fuel_type_battery, mode->get_gun_ups_drain() * shots );

    p.remove_effect( effect_on_roof );
    p.recoil = old;

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
        e->target.first  = tripoint_min;
        e->target.second = tripoint_min;
    }

    // find radius of a circle centered at u encompassing all points turrets can aim at
    int range = std::accumulate( opts.begin(), opts.end(), 0, [&]( const int lhs, vehicle_part * e ) {

        const auto gun = turret_query( *e );
        if( gun.query() != turret_data::status::ready ) {
            return lhs;
        }

        tripoint pos = global_part_pos3( *e );
        const int rng = gun.range();

        int res = 0;
        res = std::max( res, rl_dist( g->u.pos(), { pos.x + rng, pos.y, pos.z } ) );
        res = std::max( res, rl_dist( g->u.pos(), { pos.x - rng, pos.y, pos.z } ) );
        res = std::max( res, rl_dist( g->u.pos(), { pos.x, pos.y + rng, pos.z } ) );
        res = std::max( res, rl_dist( g->u.pos(), { pos.x, pos.y - rng, pos.z } ) );
        return std::max( lhs, res );
    } );

    if( opts.empty() ) {
        add_msg( m_warning, _( "Can't aim turrets: all turrets are offline" ) );
        return false;
    }

    tripoint pos = g->u.pos();
    std::vector<tripoint> trajectory = g->pl_target_ui( TARGET_MODE_TURRET, nullptr, range );

    if( !trajectory.empty() ) {
        // set target for any turrets in range
        for( auto e : turrets( trajectory.back() ) ) {
            e->target.second = trajectory.back();
        }
        ///\EFFECT_INT speeds up aiming of vehicle turrets
        g->u.moves = std::min( 0, g->u.moves - 100 + ( 5 * g->u.int_cur ) );
    }

    return !trajectory.empty();
}

int vehicle::automatic_fire_turret( vehicle_part &pt )
{
    auto gun = turret_query( pt );
    if( gun.query() != turret_data::status::ready ) {
        return 0;
    }

    tripoint pos = global_part_pos3( pt );

    npc tmp;
    tmp.set_fake( true );
    tmp.name = string_format( pgettext( "vehicle turret", "The %s" ), pt.name().c_str() );
    tmp.set_skill_level( gun.base()->gun_skill(), 8 );
    tmp.set_skill_level( skill_id( "gun" ), 4 );
    tmp.recoil = 0; // turrets are subject only to recoil_vehicle()
    tmp.setpos( pos );
    tmp.str_cur = 16;
    tmp.dex_cur = 8;
    tmp.per_cur = 12;
    // Assume vehicle turrets are friendly to the player.
    tmp.attitude = NPCATT_FOLLOW;

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
        int range = std::min( gun.range(), 12 );
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

    auto shots = gun.fire( tmp, targ );

    if( g->u.sees( pos ) && shots ) {
        add_msg( _( "The %1$s fires its %2$s!" ), name.c_str(), pt.name().c_str() );
    }

    return shots;
}
