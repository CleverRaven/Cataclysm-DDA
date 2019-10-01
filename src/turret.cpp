#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <numeric>
#include <memory>

#include "avatar.h"
#include "game.h"
#include "gun_mode.h"
#include "item.h"
#include "itype.h"
#include "messages.h"
#include "npc.h"
#include "player.h"
#include "projectile.h"
#include "ranged.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "veh_type.h"
#include "vehicle_selector.h"
#include "creature.h"
#include "debug.h"
#include "optional.h"
#include "enums.h"

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

turret_data vehicle::turret_query( const vehicle_part &pt ) const
{
    return const_cast<vehicle *>( this )->turret_query( const_cast<vehicle_part &>( pt ) );
}

turret_data vehicle::turret_query( const tripoint &pos )
{
    auto res = get_parts_at( pos, "TURRET", part_status_flag::any );
    return !res.empty() ? turret_query( *res.front() ) : turret_data();
}

turret_data vehicle::turret_query( const tripoint &pos ) const
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

item_location turret_data::base() const
{
    return item_location( vehicle_cursor( *veh, veh->index_of_part( part ) ), &part->base );
}

int turret_data::ammo_remaining() const
{
    if( !veh || !part ) {
        return 0;
    }
    if( part->info().has_flag( "USE_TANKS" ) ) {
        return veh->fuel_left( ammo_current() );
    }
    return part->base.ammo_remaining();
}

int turret_data::ammo_capacity() const
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
            if( fuel->ammo && part->base.ammo_types().count( fuel->ammo->type ) &&
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

void turret_data::prepare_fire( player &p )
{
    // prevent turrets from shooting their own vehicles
    p.add_effect( effect_on_roof, 1_turns );

    // turrets are subject only to recoil_vehicle()
    cached_recoil = p.recoil;
    p.recoil = 0;

    // set fuel tank fluid as ammo, if appropriate
    if( part->info().has_flag( "USE_TANKS" ) ) {
        auto mode = base()->gun_current_mode();
        int qty  = mode->ammo_required();
        int fuel_left = veh->fuel_left( ammo_current() );
        mode->ammo_set( ammo_current(), std::min( qty * mode.qty, fuel_left ) );
    }
}

void turret_data::post_fire( player &p, int shots )
{
    // remove any temporary recoil adjustments
    p.remove_effect( effect_on_roof );
    p.recoil = cached_recoil;

    auto mode = base()->gun_current_mode();

    // handle draining of vehicle tanks and UPS charges, if applicable
    if( part->info().has_flag( "USE_TANKS" ) ) {
        veh->drain( ammo_current(), mode->ammo_required() * shots );
        mode->ammo_unset();
    }

    veh->drain( fuel_type_battery, mode->get_gun_ups_drain() * shots );
}

int turret_data::fire( player &p, const tripoint &target )
{
    if( !veh || !part ) {
        return 0;
    }
    int shots = 0;
    auto mode = base()->gun_current_mode();

    prepare_fire( p );
    shots = p.fire_gun( target, mode.qty, *mode );
    post_fire( p, shots );
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
        uilist menu;
        menu.text = _( "Set turret targeting" );
        menu.callback = &callback;
        menu.selected = sel;
        menu.fselected = sel;
        menu.w_y = 2;

        for( auto &p : turrets ) {
            menu.addentry( -1, true, MENU_AUTOASSIGN, "%s [%s]", p->name(),
                           p->enabled ? _( "auto" ) : _( "manual" ) );
        }

        menu.query();
        if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= turrets.size() ) {
            break;
        }

        sel = menu.ret;
        turrets[ sel ]->enabled = !turrets[ sel ]->enabled;

        // clear the turret's current targets to prevent unwanted auto-firing
        tripoint pos = locations[ sel ];
        turrets[ sel ]->reset_target( pos );
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
        uilist menu;
        menu.text = _( "Set turret firing modes" );
        menu.callback = &callback;
        menu.selected = sel;
        menu.fselected = sel;
        menu.w_y = 2;

        for( auto &p : turrets ) {
            menu.addentry( -1, true, MENU_AUTOASSIGN, "%s [%s]",
                           p->name(), p->base.gun_current_mode().tname() );
        }

        menu.query();
        if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= turrets.size() ) {
            break;
        }

        sel = menu.ret;
        turrets[ sel ]->base.gun_cycle_mode();
    }
}

bool vehicle::turrets_aim( bool manual, bool automatic, vehicle_part *tur_part )
{
    // Only work with turrets we want and are able to fire
    auto opts = turrets();
    auto last = opts.end();
    opts.erase( std::remove_if( opts.begin(), last, [&]( vehicle_part * vp ) {
        bool available = turret_query( *vp ).query() == turret_data::status::ready;
        bool mode_ok = ( ( !vp->enabled && manual ) || ( vp->enabled && automatic ) );
        bool part_wanted = mode_ok || ( tur_part && vp == tur_part );
        return !available || !part_wanted;
    } ), last );

    if( opts.empty() ) {
        add_msg( m_warning, _( "Can't aim turrets: all turrets are offline" ) );
        return false;
    }

    // Clear any existing targets for turrets we want to aim
    std::for_each( opts.begin(), opts.end(), [&]( vehicle_part * vp ) {
        vp->reset_target( global_part_pos3( *vp ) );
    } );

    // find radius of a circle centered at u encompassing all points turrets can aim at
    int range = std::accumulate( opts.begin(), opts.end(), 0, [&]( const int lhs, vehicle_part * e ) {

        const auto gun = turret_query( *e );
        if( gun.query() != turret_data::status::ready ) {
            return lhs;
        }

        tripoint pos = global_part_pos3( *e );
        const int rng = gun.range();

        int res = 0;
        res = std::max( res, rl_dist( g->u.pos(), pos + point( rng, 0 ) ) );
        res = std::max( res, rl_dist( g->u.pos(), pos + point( -rng, 0 ) ) );
        res = std::max( res, rl_dist( g->u.pos(), pos + point( 0, rng ) ) );
        res = std::max( res, rl_dist( g->u.pos(), pos + point( 0, -rng ) ) );
        return std::max( lhs, res );
    } );

    std::vector<tripoint> trajectory = target_handler().target_ui( g->u, TARGET_MODE_TURRET, nullptr,
                                       range );

    bool got_target = !trajectory.empty();
    if( got_target ) {
        // set target for any turrets in range
        for( vehicle_part *vp : opts ) {
            tripoint targ = trajectory.back();
            if( rl_dist( global_part_pos3( *vp ), targ ) <= vp->base.gun_range() ) {
                vp->target.second = targ;
            }
        }
        ///\EFFECT_INT speeds up aiming of vehicle turrets
        g->u.moves = std::min( 0, g->u.moves - 100 + ( 5 * g->u.int_cur ) );
    }

    return got_target;
}

int vehicle::turrets_aim_and_fire( bool manual, bool automatic, vehicle_part *tur_part )
{
    int shots = 0;
    auto fire_if_able = [&]( vehicle_part * vp ) {
        if( vp->target.first != vp->target.second && !vp->enabled ) {
            turret_data turret = this->turret_query( *vp );
            npc cpu = get_targeting_npc( *vp );
            shots += turret.fire( cpu, vp->target.second );
            vp->reset_target( global_part_pos3( *vp ) );
        }
    };

    if( turrets_aim( manual, automatic, tur_part ) ) {
        // turrets_aim already set the targets for any available turrets that can reach the target.
        const auto &turs = turrets();
        std::for_each( turs.begin(), turs.end(), fire_if_able );
    }

    return shots;
}

int vehicle::turrets_aim_single( vehicle_part *tur_part )
{
    int shots = 0;
    if( tur_part ) {
        return turrets_aim_and_fire( false, false, tur_part );
    }

    std::vector<std::string> options;
    std::vector<vehicle_part *> guns;

    // Get a group of turrets that are ready to fire
    for( auto &t : turrets() ) {
        if( t != nullptr ) {
            turret_data data = turret_query( *t );
            if( data.query() == turret_data::status::ready ) {
                options.push_back( t->name() );
                guns.push_back( t );
            }
        }
    }

    vehicle_part *chosen;

    if( !options.empty() ) {
        const int ret = uilist( _( "Aim which turret?" ), options );
        if( ret >= 0 ) {
            chosen = guns[ret];
        } else {
            chosen = nullptr;
        }
    } else {
        add_msg( m_warning, _( "None of the turrets are available to fire." ) );
        return shots;
    }

    if( chosen != nullptr ) {
        shots = turrets_aim_and_fire( false, false, chosen );
    }

    return shots;

}

npc vehicle::get_targeting_npc( const vehicle_part &pt )
{
    // Make a fake NPC to represent the targeting system
    npc cpu;
    cpu.set_fake( true );
    cpu.name = string_format( _( "The %s turret" ), pt.get_base().tname( 1 ) );
    // turrets are subject only to recoil_vehicle()
    cpu.recoil = 0;

    // These might all be affected by vehicle part damage, weather effects, etc.
    cpu.set_skill_level( pt.get_base().gun_skill(), 8 );
    cpu.set_skill_level( skill_id( "gun" ), 4 );

    cpu.str_cur = 16;
    cpu.dex_cur = 8;
    cpu.per_cur = 12;
    cpu.setpos( global_part_pos3( pt ) );
    // Assume vehicle turrets are friendly to the player.
    cpu.set_attitude( NPCATT_FOLLOW );
    cpu.set_fac( get_owner() );
    return cpu;
}

int vehicle::automatic_fire_turret( vehicle_part &pt )
{
    turret_data gun = turret_query( pt );

    int shots = 0;

    if( gun.query() != turret_data::status::ready ) {
        return shots;
    }

    // The position of the vehicle part.
    tripoint pos = global_part_pos3( pt );

    // Create the targeting computer's npc
    npc cpu = get_targeting_npc( pt );

    int area = aoe_size( gun.ammo_effects() );
    if( area > 0 ) {
        // Pad a bit for less friendly fire
        area += area == 1 ? 1 : 2;
    }

    const bool u_see = g->u.sees( pos );
    // The current target of the turret.
    auto &target = pt.target;
    if( target.first == target.second ) {
        // Manual target not set, find one automatically.
        // BEWARE: Calling turret_data.fire on tripoint min coordinates starts a crash
        //      triggered at `trajectory.insert( trajectory.begin(), source )` at ranged.cpp:236
        pt.reset_target( pos );
        int boo_hoo;

        // TODO: calculate chance to hit and cap range based upon this
        int max_range = 20;
        int range = std::min( gun.range(), max_range );
        Creature *auto_target = cpu.auto_find_hostile_target( range, boo_hoo, area );
        if( auto_target == nullptr ) {
            if( boo_hoo ) {
                cpu.name = string_format( pgettext( "vehicle turret", "The %s" ), pt.name() );
                if( u_see ) {
                    add_msg( m_warning, ngettext( "%s points in your direction and emits an IFF warning beep.",
                                                  "%s points in your direction and emits %d annoyed sounding beeps.",
                                                  boo_hoo ),
                             cpu.name, boo_hoo );
                } else {
                    add_msg( m_warning, ngettext( "%s emits an IFF warning beep.",
                                                  "%s emits %d annoyed sounding beeps.",
                                                  boo_hoo ),
                             cpu.name, boo_hoo );
                }
            }
            return shots;
        }

        target.second = auto_target->pos();

    } else {
        // Target is already set, make sure we didn't move after aiming (it's a bug if we did).
        if( pos != target.first ) {
            target.second = target.first;
            debugmsg( "%s moved after aiming but before it could fire.", cpu.name );
            return shots;
        }
    }

    // Get the turret's target and reset it
    tripoint targ = target.second;
    pt.reset_target( pos );

    shots = gun.fire( cpu, targ );

    if( shots && u_see && !g->u.sees( targ ) ) {
        add_msg( _( "The %1$s fires its %2$s!" ), name, pt.name() );
    }

    return shots;
}
