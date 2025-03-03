#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>

#include "avatar.h"
#include "character.h"
#include "creature.h"
#include "debug.h"
#include "enums.h"
#include "gun_mode.h"
#include "item.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "itype.h"
#include "messages.h"
#include "npc.h"
#include "pocket_type.h"
#include "projectile.h"
#include "ranged.h"
#include "string_formatter.h"
#include "translations.h"
#include "uilist.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const efftype_id effect_on_roof( "on_roof" );

static const itype_id fuel_type_battery( "battery" );

static const skill_id skill_gun( "gun" );

static const trait_id trait_BRAWLER( "BRAWLER" );

static const trait_id trait_GUNSHY( "GUNSHY" );

std::vector<vehicle_part *> vehicle::turrets()
{
    std::vector<vehicle_part *> res;

    for( int index : turret_locations ) {
        vehicle_part &e = parts[index];
        if( !e.is_broken() && e.is_turret() ) {
            res.push_back( &e );
        }
    }
    return res;
}

turret_data vehicle::turret_query( vehicle_part &pt )
{
    if( !pt.is_turret() || pt.removed || pt.is_broken() ) {
        return turret_data();
    }
    return turret_data( this, &pt );
}

turret_data vehicle::turret_query( map *here, const tripoint_bub_ms &pos )
{
    return vehicle::turret_query( here->get_abs( pos ) );
}

turret_data vehicle::turret_query( const tripoint_abs_ms &pos )
{
    const std::vector<vehicle_part *> res = get_parts_at( pos, "TURRET", part_status_flag::any );
    return !res.empty() ? turret_query( *res.front() ) : turret_data();
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

bool turret_data::uses_vehicle_tanks_or_batteries() const
{
    const vpart_info &vpi = part->info();
    return vpi.has_flag( "USE_TANKS" ) ||
           vpi.has_flag( "USE_BATTERIES" );
}

int turret_data::ammo_remaining() const
{
    map &here = get_map();

    if( !veh || !part ) {
        return 0;
    }
    if( uses_vehicle_tanks_or_batteries() ) {
        return veh->fuel_left( here, ammo_current() );
    }
    return part->base.ammo_remaining( );
}

int turret_data::ammo_capacity( const ammotype &ammo ) const
{
    if( !veh || !part || uses_vehicle_tanks_or_batteries() ) {
        return 0;
    }
    return part->base.ammo_capacity( ammo );
}

const itype *turret_data::ammo_data() const
{
    if( !veh || !part ) {
        return nullptr;
    }
    if( uses_vehicle_tanks_or_batteries() ) {
        return !ammo_current().is_null() ? item::find_type( ammo_current() ) : nullptr;
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
    return opts.empty() ? itype_id::NULL_ID() : *opts.begin();
}

std::set<itype_id> turret_data::ammo_options() const
{
    std::set<itype_id> opts;

    if( !veh || !part ) {
        return opts;
    }

    if( !uses_vehicle_tanks_or_batteries() ) {
        if( !part->base.ammo_current().is_null() ) {
            opts.insert( part->base.ammo_current() );
        }

    } else {
        for( const auto &e : veh->fuels_left( ) ) {
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

std::set<ammo_effect_str_id> turret_data::ammo_effects() const
{
    if( !veh || !part ) {
        return std::set<ammo_effect_str_id>();
    }
    auto res = part->base.ammo_effects();
    if( uses_vehicle_tanks_or_batteries() && ammo_data() ) {
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
    if( uses_vehicle_tanks_or_batteries() && ammo_data() ) {
        res += ammo_data()->ammo->range;
    }
    return res;
}

bool turret_data::in_range( const tripoint_abs_ms &target ) const
{
    if( !veh || !part ) {
        return false;
    }
    int range = veh->turret_query( *part ).range();
    int dist = rl_dist( veh->abs_part_pos( *part ), target );
    return range >= dist;
}

bool turret_data::can_reload() const
{
    if( !veh || !part || uses_vehicle_tanks_or_batteries() ) {
        return false;
    }
    if( !part->base.magazine_integral() ) {
        // always allow changing of magazines
        return true;
    }
    if( part->base.ammo_remaining( ) == 0 ) {
        return true;
    }
    return part->base.ammo_remaining( ) <
           part->base.ammo_capacity( part->base.ammo_data()->ammo->type );
}

bool turret_data::can_unload() const
{
    if( !veh || !part || uses_vehicle_tanks_or_batteries() ) {
        return false;
    }
    return part->base.ammo_remaining( ) || part->base.magazine_current();
}

turret_data::status turret_data::query() const
{
    map &here = get_map();

    if( !veh || !part ) {
        return status::invalid;
    }

    const item &gun = part->base;

    if( uses_vehicle_tanks_or_batteries() ) {
        if( veh->fuel_left( here, ammo_current() ) < gun.ammo_required() ) {
            return status::no_ammo;
        }
    } else {
        if( gun.ammo_required() && gun.ammo_remaining( ) < gun.ammo_required() ) {
            return status::no_ammo;
        }
    }

    const units::energy energy_drain = gun.get_gun_energy_drain() * gun.gun_current_mode().qty;
    if( energy_drain > units::from_kilojoule( veh->fuel_left( here, fuel_type_battery ) ) ) {
        return status::no_power;
    }

    return status::ready;
}

void turret_data::prepare_fire( Character &you )
{
    map &here = get_map();

    // prevent turrets from shooting their own vehicles
    you.add_effect( effect_on_roof, 1_turns );

    // turrets are subject only to recoil_vehicle()
    cached_recoil = you.recoil;
    you.recoil = 0;

    // set fuel tank fluid as ammo, if appropriate
    if( uses_vehicle_tanks_or_batteries() ) {
        gun_mode mode = base()->gun_current_mode();
        int qty  = mode->ammo_required();
        int fuel_left = veh->fuel_left( here, ammo_current() );
        mode->ammo_set( ammo_current(), std::min( qty * mode.qty, fuel_left ) );
    }
}

void turret_data::post_fire( map *here, Character &you, int shots )
{
    // remove any temporary recoil adjustments
    you.remove_effect( effect_on_roof );
    you.recoil = cached_recoil;

    gun_mode mode = base()->gun_current_mode();

    // Clears item's MAGAZINE_WELL pockets, this gets rid of e.g. flamethrower's
    // "pressurized tanks", or chemoelectric cartridges etc that are left over after firing.
    static const auto clear_mag_wells = []( item & it ) {
        std::vector<item_pocket *> magazine_wells = it.get_contents().get_pockets(
        []( const item_pocket & pocket ) {
            return pocket.is_type( pocket_type::MAGAZINE_WELL );
        } );
        for( item_pocket *pocket : magazine_wells ) {
            pocket->clear_items();
        }
    };

    // handle draining of vehicle tanks or batteries, if applicable
    if( uses_vehicle_tanks_or_batteries() ) {
        veh->drain( here, ammo_current(), mode->ammo_required() * shots );
        mode->ammo_unset();

        clear_mag_wells( *base() );
    }

    veh->drain( here, fuel_type_battery, units::to_kilojoule( mode->get_gun_energy_drain() * shots ) );
}

int turret_data::fire( Character &c, map *here, const tripoint_bub_ms &target )
{
    if( !veh || !part ) {
        return 0;
    }
    int shots = 0;
    gun_mode mode = base()->gun_current_mode();

    prepare_fire( c );
    shots = c.fire_gun( *here, target, mode.qty, *mode );
    post_fire( here, c, shots );
    return shots;
}

void vehicle::turrets_aim_and_fire_single()
{
    std::vector<std::string> option_names;
    std::vector<vehicle_part *> options;

    // Find all turrets that are ready to fire
    for( vehicle_part *&t : turrets() ) {
        turret_data data = turret_query( *t );
        if( data.query() == turret_data::status::ready ) {
            option_names.push_back( t->name() );
            options.push_back( t );
        }
    }

    // Select one
    if( options.empty() ) {
        add_msg( m_warning, _( "None of the turrets are ready to fire." ) );
        return;
    }
    const int idx = uilist( _( "Aim which turret?" ), option_names );
    if( idx < 0 ) {
        return;
    }
    vehicle_part *turret = options[idx];

    std::vector<vehicle_part *> turrets;
    turrets.push_back( turret );
    turrets_aim_and_fire( turrets );
}

bool vehicle::turrets_aim_and_fire_all_manual( bool show_msg )
{
    std::vector<vehicle_part *> turrets = find_all_ready_turrets( true, false );

    if( turrets.empty() ) {
        if( show_msg ) {
            add_msg( m_warning,
                     _( "Can't aim turrets: all turrets are offline or set to automatic targeting mode." ) );
        }
        return false;
    }

    turrets_aim_and_fire( turrets );
    return true;
}

void vehicle::turrets_override_automatic_aim()
{
    std::vector<vehicle_part *> turrets = find_all_ready_turrets( false, true );

    if( turrets.empty() ) {
        add_msg( m_warning,
                 _( "Can't aim turrets: all turrets are offline or set to manual targeting mode." ) );
        return;
    }

    turrets_aim( turrets );
}

int vehicle::turrets_aim_and_fire( std::vector<vehicle_part *> &turrets )
{
    map &here = get_map();

    int shots = 0;
    if( turrets_aim( turrets ) ) {
        for( vehicle_part *t : turrets ) {
            bool has_target = t->target.first != t->target.second;
            if( has_target ) {
                turret_data turret = turret_query( *t );
                npc &cpu = t->get_targeting_npc( *this );
                shots += turret.fire( cpu, &here, here.get_bub( t->target.second ) );
                t->reset_target( abs_part_pos( *t ) );
            }
        }
    }
    return shots;
}

bool vehicle::turrets_aim( std::vector<vehicle_part *> &turrets )
{
    map &here = get_map();

    // Clear existing targets
    for( vehicle_part *t : turrets ) {
        if( !turret_query( *t ) ) {
            debugmsg( "Expected a valid vehicle turret" );
            return false;
        }
        t->reset_target( abs_part_pos( *t ) );
    }

    avatar &player_character = get_avatar();
    if( player_character.has_trait( trait_BRAWLER ) ) {
        player_character.add_msg_if_player(
            _( "Pfft.  You are a brawler; using turrets is beneath you." ) );
        return false;
    }
    if( player_character.has_trait( trait_GUNSHY ) ) {
        player_character.add_msg_if_player(
            _( "You refuse to use a firearm even when it's mounted." ) );
        return false;
    }

    // Get target
    target_handler::trajectory trajectory = target_handler::mode_turrets( player_character, *this,
                                            turrets );

    bool got_target = !trajectory.empty();
    if( got_target ) {
        tripoint_bub_ms target = trajectory.back();
        // Set target for any turret in range
        const tripoint_abs_ms abs_target = here.get_abs( target );

        for( vehicle_part *t : turrets ) {
            if( turret_query( *t ).in_range( abs_target ) ) {
                t->target.second = abs_target;
            }
        }

        ///\EFFECT_INT speeds up aiming of vehicle turrets
        player_character.set_moves( std::min( 0,
                                              player_character.get_moves() - 100 + ( 5 * player_character.int_cur ) ) );
    }
    return got_target;
}

std::vector<vehicle_part *> vehicle::find_all_ready_turrets( bool manual, bool automatic )
{
    std::vector<vehicle_part *> res;
    for( vehicle_part *t : turrets() ) {
        if( ( t->enabled && automatic ) || ( !t->enabled && manual ) ) {
            if( turret_query( *t ).query() == turret_data::status::ready ) {
                res.push_back( t );
            }
        }
    }
    return res;
}

void vehicle::turrets_set_targeting()
{
    map &here = get_map();

    std::vector<vehicle_part *> turrets;
    std::vector<tripoint_bub_ms> locations;

    for( vehicle_part &p : parts ) {
        if( p.is_turret() ) {
            turrets.push_back( &p );
            locations.push_back( bub_part_pos( here, p ) );
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

        for( vehicle_part *&p : turrets ) {
            menu.addentry( -1, has_part( abs_part_pos( *p ), "TURRET_CONTROLS" ), MENU_AUTOASSIGN,
                           "%s [%s]", p->name(), p->enabled ?
                           _( "auto -> manual" ) : has_part( abs_part_pos( *p ), "TURRET_CONTROLS" ) ?
                           _( "manual -> auto" ) :
                           _( "manual (turret control unit required for auto mode)" ) );
        }

        menu.query();
        if( menu.ret < 0 || static_cast<size_t>( menu.ret ) >= turrets.size() ) {
            break;
        }

        sel = menu.ret;
        if( has_part( &here, locations[ sel ], "TURRET_CONTROLS" ) ) {
            turrets[sel]->enabled = !turrets[sel]->enabled;
        } else {
            turrets[sel]->enabled = false;
        }

        for( const vpart_reference &vp : get_avail_parts( "TURRET_CONTROLS" ) ) {
            vehicle_part &e = vp.part();
            if( e.mount == turrets[sel]->mount ) {
                e.enabled = turrets[sel]->enabled;
            }
        }

        // clear the turret's current targets to prevent unwanted auto-firing
        tripoint_bub_ms pos = locations[ sel ];
        turrets[ sel ]->reset_target( here.get_abs( pos ) );
    }
}

void vehicle::turrets_set_mode()
{
    map &here = get_map();
    std::vector<vehicle_part *> turrets;
    std::vector<tripoint_bub_ms> locations;

    for( vehicle_part &p : parts ) {
        if( p.base.is_gun() ) {
            turrets.push_back( &p );
            locations.push_back( bub_part_pos( here, p ) );
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

        for( vehicle_part *&p : turrets ) {
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

npc &vehicle_part::get_targeting_npc( vehicle &veh )
{
    map &here = get_map();

    // Make a fake NPC to represent the targeting system
    if( !cpu.brain ) {
        cpu.brain = std::make_unique<npc>();
        npc &brain = *cpu.brain;
        brain.set_body();
        brain.set_fake( true );
        // turrets are subject only to recoil_vehicle()
        brain.recoil = 0;
        // These might all be affected by vehicle part damage, weather effects, etc.
        brain.str_cur = 16;
        brain.dex_cur = 8;
        brain.per_cur = 12;
        // Assume vehicle turrets are friendly to the player.
        brain.set_attitude( NPCATT_FOLLOW );
        brain.set_fac( veh.get_owner() );
        brain.set_skill_level( skill_gun, 4 );
        brain.name = string_format( _( "The %s turret" ), get_base().tname( 1 ) );
        brain.set_skill_level( get_base().gun_skill(), 8 );
    }
    cpu.brain->setpos( here, veh.bub_part_pos( here, *this ) );
    cpu.brain->recalc_sight_limits();
    return *cpu.brain;
}

int vehicle::automatic_fire_turret( vehicle_part &pt )
{
    map &here = get_map();

    turret_data gun = turret_query( pt );

    int shots = 0;

    if( gun.query() != turret_data::status::ready ) {
        return shots;
    }

    // An automatic turret will not fire if the gun is too hot.
    double overheat_modifier = 0;
    float overheat_multiplier = 1.0f;
    for( const item *mod : gun.base()->gunmods() ) {
        overheat_modifier += mod->type->gunmod->overheat_threshold_modifier;
        overheat_multiplier *= mod->type->gunmod->overheat_threshold_multiplier;
    }
    double heat = gun.base()->get_var( "gun_heat", 0.0 );
    double threshold = std::max( ( gun.base()->type->gun->overheat_threshold * overheat_multiplier ) +
                                 overheat_modifier, 5.0 );

    // Heat is too hot, do not fire. The warning chime will have just started sounding.
    if( heat > threshold * 0.6 ) {
        return shots;
    }

    // The position of the vehicle part.
    tripoint_bub_ms pos = bub_part_pos( here, pt );

    // Create the targeting computer's npc
    npc &cpu = pt.get_targeting_npc( *this );

    int area = max_aoe_size( gun.ammo_effects() );
    if( area > 0 ) {
        // Pad a bit for less friendly fire
        area += area == 1 ? 1 : 2;
    }

    Character &player_character = get_player_character();
    const bool u_see = player_character.sees( here, pos );
    const bool u_hear = !player_character.is_deaf();
    // The current target of the turret.
    auto &target = pt.target;
    if( target.first == target.second ) {
        // Manual target not set, find one automatically.
        // BEWARE: Calling turret_data.fire on tripoint min coordinates starts a crash
        //      triggered at `trajectory.insert( trajectory.begin(), source )` at ranged.cpp:236
        pt.reset_target( here.get_abs( pos ) );
        int boo_hoo;

        // TODO: calculate chance to hit and cap range based upon this
        int max_range = 20;
        int range = std::min( gun.range(), max_range );
        Creature *auto_target = cpu.auto_find_hostile_target( range, boo_hoo, area );
        if( auto_target == nullptr ) {
            if( boo_hoo ) {
                cpu.get_name() = string_format( pgettext( "vehicle turret", "The %s" ), pt.name() );
                // check if the player can see or hear then print chooses a message accordingly
                if( u_see && u_hear ) {
                    add_msg( m_warning, n_gettext( "%s points in your direction and emits an IFF warning beep.",
                                                   "%s points in your direction and emits %d annoyed sounding beeps.",
                                                   boo_hoo ),
                             cpu.get_name(), boo_hoo );
                } else if( !u_see && u_hear ) {
                    add_msg( m_warning, n_gettext( "You hear a warning beep.",
                                                   "You hear %d annoyed sounding beeps.",
                                                   boo_hoo ), boo_hoo );
                } else if( u_see && !u_hear ) {
                    add_msg( m_warning, _( "%s points in your direction." ), cpu.get_name() );
                }
            }
            return shots;
        }

        target.second = auto_target->pos_abs();

    } else {
        // Target is already set, make sure we didn't move after aiming (it's a bug if we did).
        if( pos != here.get_bub( target.first ) ) {
            target.second = target.first;
            debugmsg( "%s moved after aiming but before it could fire.", cpu.get_name() );
            return shots;
        }
    }

    // Get the turret's target and reset it
    tripoint_bub_ms targ( here.get_bub( target.second ) );
    pt.reset_target( here.get_abs( pos ) );

    shots = gun.fire( cpu, &here, targ );

    if( shots && u_see ) {
        add_msg_if_player_sees( targ, _( "The %1$s fires its %2$s!" ), name, pt.name() );
    }

    return shots;
}
