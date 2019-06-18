#include "avatar_action.h"

#include "action.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "creature.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "itype.h"
#include "iuse_actor.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "projectile.h"
#include "ranged.h"
#include "recipe_dictionary.h"
#include "translations.h"
#include "type_id.h"
#include "ui.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_reference.h"

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

static const trait_id trait_BURROW( "BURROW" );
static const trait_id trait_SHELL2( "SHELL2" );

static const efftype_id effect_amigara( "amigara" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_relax_gas( "relax_gas" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_riding( "riding" );
static const efftype_id effect_harnessed( "harnessed" );

static const fault_id fault_gun_clogged( "fault_gun_clogged" );

bool avatar_action::move( avatar &you, map &m, int dx, int dy, int dz )
{
    if( ( !g->check_safe_mode_allowed() ) || you.has_active_mutation( trait_SHELL2 ) ) {
        if( you.has_active_mutation( trait_SHELL2 ) ) {
            add_msg( m_warning, _( "You can't move while in your shell.  Deactivate it to go mobile." ) );
        }
        return false;
    }

    tripoint dest_loc;
    if( dz == 0 && you.has_effect( effect_stunned ) ) {
        dest_loc.x = rng( you.posx() - 1, you.posx() + 1 );
        dest_loc.y = rng( you.posy() - 1, you.posy() + 1 );
        dest_loc.z = you.posz();
    } else {
        if( tile_iso && use_tiles && !you.has_destination() ) {
            rotate_direction_cw( dx, dy );
        }
        dest_loc.x = you.posx() + dx;
        dest_loc.y = you.posy() + dy;
        dest_loc.z = you.posz() + dz;
    }

    if( dest_loc == you.pos() ) {
        // Well that sure was easy
        return true;
    }

    if( m.has_flag( TFLAG_MINEABLE, dest_loc ) && g->mostseen == 0 &&
        get_option<bool>( "AUTO_FEATURES" ) && get_option<bool>( "AUTO_MINING" ) &&
        !m.veh_at( dest_loc ) && !you.is_underwater() && !you.has_effect( effect_stunned ) &&
        !you.has_effect( effect_riding ) ) {
        if( you.weapon.has_flag( "DIG_TOOL" ) ) {
            if( you.weapon.type->can_use( "JACKHAMMER" ) && you.weapon.ammo_sufficient() ) {
                you.invoke_item( &you.weapon, "JACKHAMMER", dest_loc );
                you.defer_move( dest_loc ); // don't move into the tile until done mining
                return true;
            } else if( you.weapon.type->can_use( "PICKAXE" ) ) {
                you.invoke_item( &you.weapon, "PICKAXE", dest_loc );
                you.defer_move( dest_loc ); // don't move into the tile until done mining
                return true;
            }
        }
        if( you.has_trait( trait_BURROW ) ) {
            item burrowing_item( itype_id( "fake_burrowing" ) );
            you.invoke_item( &burrowing_item, "BURROW", dest_loc );
            you.defer_move( dest_loc ); // don't move into the tile until done mining
            return true;
        }
    }

    // by this point we're either walking, running, crouching, or attacking, so update the activity level to match
    if( you.get_movement_mode() == "walk" ) {
        you.increase_activity_level( LIGHT_EXERCISE );
    } else if( you.get_movement_mode() == "crouch" ) {
        you.increase_activity_level( MODERATE_EXERCISE );
    } else {
        you.increase_activity_level( ACTIVE_EXERCISE );
    }

    // If the player is *attempting to* move on the X axis, update facing direction of their sprite to match.
    int new_dx = dest_loc.x - you.posx();
    int new_dy = dest_loc.y - you.posy();

    if( ! tile_iso ) {
        if( new_dx > 0 ) {
            you.facing = FD_RIGHT;
            if( you.has_effect( effect_riding ) && you.mounted_creature ) {
                auto mons = you.mounted_creature.get();
                mons->facing = FD_RIGHT;
            }
        } else if( new_dx < 0 ) {
            you.facing = FD_LEFT;
            if( you.has_effect( effect_riding ) && you.mounted_creature ) {
                auto mons = you.mounted_creature.get();
                mons->facing = FD_LEFT;
            }
        }
    } else {
        //
        // iso:
        //
        // right key            =>  +x -y       FD_RIGHT
        // left key             =>  -x +y       FD_LEFT
        // up key               =>  +x +y       ______
        // down key             =>  -x -y       ______
        // y: left-up key       =>  __ +y       FD_LEFT
        // u: right-up key      =>  +x __       FD_RIGHT
        // b: left-down key     =>  -x __       FD_LEFT
        // n: right-down key    =>  __ -y       FD_RIGHT
        //
        // right key            =>  +x -y       FD_RIGHT
        // u: right-up key      =>  +x __       FD_RIGHT
        // n: right-down key    =>  __ -y       FD_RIGHT
        // up key               =>  +x +y       ______
        // down key             =>  -x -y       ______
        // left key             =>  -x +y       FD_LEFT
        // y: left-up key       =>  __ +y       FD_LEFT
        // b: left-down key     =>  -x __       FD_LEFT
        //
        // right key            =>  +x +y       FD_RIGHT
        // u: right-up key      =>  +x __       FD_RIGHT
        // n: right-down key    =>  __ +y       FD_RIGHT
        // up key               =>  +x -y       ______
        // left key             =>  -x -y       FD_LEFT
        // b: left-down key     =>  -x __       FD_LEFT
        // y: left-up key       =>  __ -y       FD_LEFT
        // down key             =>  -x +y       ______
        //
        if( new_dx >= 0 && new_dy >= 0 ) {
            you.facing = FD_RIGHT;
            if( you.has_effect( effect_riding ) && you.mounted_creature ) {
                auto mons = you.mounted_creature.get();
                mons->facing = FD_RIGHT;
            }
        }
        if( new_dy <= 0 && new_dx <= 0 ) {
            you.facing = FD_LEFT;
            if( you.has_effect( effect_riding ) && you.mounted_creature ) {
                auto mons = you.mounted_creature.get();
                mons->facing = FD_LEFT;
            }
        }
    }

    if( dz == 0 && ramp_move( you, m, dest_loc ) ) {
        // TODO: Make it work nice with automove (if it doesn't do so already?)
        return false;
    }

    if( you.has_effect( effect_amigara ) ) {
        int curdist = INT_MAX;
        int newdist = INT_MAX;
        const tripoint minp = tripoint( 0, 0, you.posz() );
        const tripoint maxp = tripoint( MAPSIZE_X, MAPSIZE_Y, you.posz() );
        for( const tripoint &pt : m.points_in_rectangle( minp, maxp ) ) {
            if( m.ter( pt ) == t_fault ) {
                int dist = rl_dist( pt, you.pos() );
                if( dist < curdist ) {
                    curdist = dist;
                }
                dist = rl_dist( pt, dest_loc );
                if( dist < newdist ) {
                    newdist = dist;
                }
            }
        }
        if( newdist > curdist ) {
            add_msg( m_info, _( "You cannot pull yourself away from the faultline..." ) );
            return false;
        }
    }

    dbg( D_PEDANTIC_INFO ) << "game:plmove: From (" <<
                           you.posx() << "," << you.posy() << "," << you.posz() << ") to (" <<
                           dest_loc.x << "," << dest_loc.y << "," << dest_loc.z << ")";

    if( g->disable_robot( dest_loc ) ) {
        return false;
    }

    // Check if our movement is actually an attack on a monster or npc
    // Are we displacing a monster?

    bool attacking = false;
    if( g->critter_at( dest_loc ) ) {
        attacking = true;
    }

    if( !you.move_effects( attacking ) ) {
        you.moves -= 100;
        return false;
    }

    if( monster *const mon_ptr = g->critter_at<monster>( dest_loc, true ) ) {
        monster &critter = *mon_ptr;
        if( critter.friendly == 0 &&
            !critter.has_effect( effect_pet ) ) {
            if( you.has_destination() ) {
                add_msg( m_warning, _( "Monster in the way. Auto-move canceled." ) );
                add_msg( m_info, _( "Click directly on monster to attack." ) );
                you.clear_destination();
                return false;
            } else {
                // fighting is hard work!
                you.increase_activity_level( EXTRA_EXERCISE );
            }
            if( you.has_effect( effect_relax_gas ) ) {
                if( one_in( 8 ) ) {
                    add_msg( m_good, _( "Your willpower asserts itself, and so do you!" ) );
                } else {
                    you.moves -= rng( 2, 8 ) * 10;
                    add_msg( m_bad, _( "You're too pacified to strike anything..." ) );
                    return false;
                }
            }
            you.melee_attack( critter, true );
            if( critter.is_hallucination() ) {
                critter.die( &you );
            }
            g->draw_hit_mon( dest_loc, critter, critter.is_dead() );
            return false;
        } else if( critter.has_flag( MF_IMMOBILE ) || critter.has_effect( effect_harnessed ) ) {
            add_msg( m_info, _( "You can't displace your %s." ), critter.name() );
            return false;
        }
        // Successful displacing is handled (much) later
    }
    // If not a monster, maybe there's an NPC there
    if( npc *const np_ = g->critter_at<npc>( dest_loc ) ) {
        npc &np = *np_;
        if( you.has_destination() ) {
            add_msg( _( "NPC in the way, Auto-move canceled." ) );
            add_msg( m_info, _( "Click directly on NPC to attack." ) );
            you.clear_destination();
            return false;
        }

        if( !np.is_enemy() ) {
            g->npc_menu( np );
            return false;
        }

        you.melee_attack( np, true );
        // fighting is hard work!
        you.increase_activity_level( EXTRA_EXERCISE );
        np.make_angry();
        return false;
    }

    // GRAB: pre-action checking.
    int dpart = -1;
    const optional_vpart_position vp0 = m.veh_at( you.pos() );
    vehicle *const veh0 = veh_pointer_or_null( vp0 );
    const optional_vpart_position vp1 = m.veh_at( dest_loc );
    vehicle *const veh1 = veh_pointer_or_null( vp1 );

    bool veh_closed_door = false;
    bool outside_vehicle = ( veh0 == nullptr || veh0 != veh1 );
    if( veh1 != nullptr ) {
        dpart = veh1->next_part_to_open( vp1->part_index(), outside_vehicle );
        veh_closed_door = dpart >= 0 && !veh1->parts[dpart].open;
    }

    if( veh0 != nullptr && abs( veh0->velocity ) > 100 ) {
        if( veh1 == nullptr ) {
            if( query_yn( _( "Dive from moving vehicle?" ) ) ) {
                g->moving_vehicle_dismount( dest_loc );
            }
            return false;
        } else if( veh1 != veh0 ) {
            add_msg( m_info, _( "There is another vehicle in the way." ) );
            return false;
        } else if( !vp1.part_with_feature( "BOARDABLE", true ) ) {
            add_msg( m_info, _( "That part of the vehicle is currently unsafe." ) );
            return false;
        }
    }

    bool toSwimmable = m.has_flag( "SWIMMABLE", dest_loc );
    bool toDeepWater = m.has_flag( TFLAG_DEEP_WATER, dest_loc );
    bool fromSwimmable = m.has_flag( "SWIMMABLE", you.pos() );
    bool fromDeepWater = m.has_flag( TFLAG_DEEP_WATER, you.pos() );
    bool fromBoat = veh0 != nullptr && veh0->is_in_water();
    bool toBoat = veh1 != nullptr && veh1->is_in_water();

    if( toSwimmable && toDeepWater && !toBoat ) {  // Dive into water!
        // Requires confirmation if we were on dry land previously
        if( you.has_effect( effect_riding ) && you.mounted_creature != nullptr ) {
            auto mon = you.mounted_creature.get();
            if( !mon->has_flag( MF_SWIMS ) || mon->get_size() < you.get_size() + 2 ) {
                add_msg( m_warning, _( "Your mount shies away from the water!" ) );
                return false;
            }
        }
        if( ( fromSwimmable && fromDeepWater && !fromBoat ) || query_yn( _( "Dive into the water?" ) ) ) {
            if( ( !fromDeepWater || fromBoat ) && you.swim_speed() < 500 ) {
                add_msg( _( "You start swimming." ) );
                add_msg( m_info, _( "%s to dive underwater." ),
                         press_x( ACTION_MOVE_DOWN ) );
            }
            avatar_action::swim( g->m, g->u, dest_loc );
        }

        g->on_move_effects();
        return true;
    }

    //Wooden Fence Gate (or equivalently walkable doors):
    // open it if we are walking
    // vault over it if we are running
    if( m.passable_ter_furn( dest_loc )
        && you.get_movement_mode() == "walk"
        && m.open_door( dest_loc, !m.is_outside( you.pos() ) ) ) {
        you.moves -= 100;
        // if auto-move is on, continue moving next turn
        if( you.has_destination() ) {
            you.defer_move( dest_loc );
        }
        return true;
    }

    if( g->walk_move( dest_loc ) ) {
        return true;
    }

    if( g->phasing_move( dest_loc ) ) {
        return true;
    }

    if( veh_closed_door ) {
        if( outside_vehicle ) {
            veh1->open_all_at( dpart );
        } else {
            veh1->open( dpart );
            add_msg( _( "You open the %1$s's %2$s." ), veh1->name,
                     veh1->part_info( dpart ).name() );
        }
        you.moves -= 100;
        // if auto-move is on, continue moving next turn
        if( you.has_destination() ) {
            you.defer_move( dest_loc );
        }
        return true;
    }

    if( m.furn( dest_loc ) != f_safe_c && m.open_door( dest_loc, !m.is_outside( you.pos() ) ) ) {
        you.moves -= 100;
        // if auto-move is on, continue moving next turn
        if( you.has_destination() ) {
            you.defer_move( dest_loc );
        }
        return true;
    }

    // Invalid move
    const bool waste_moves = you.is_blind() || you.has_effect( effect_stunned );
    if( waste_moves || dest_loc.z != you.posz() ) {
        add_msg( _( "You bump into the %s!" ), m.obstacle_name( dest_loc ) );
        // Only lose movement if we're blind
        if( waste_moves ) {
            you.moves -= 100;
        }
    } else if( m.ter( dest_loc ) == t_door_locked || m.ter( dest_loc ) == t_door_locked_peep ||
               m.ter( dest_loc ) == t_door_locked_alarm || m.ter( dest_loc ) == t_door_locked_interior ) {
        // Don't drain move points for learning something you could learn just by looking
        add_msg( _( "That door is locked!" ) );
    } else if( m.ter( dest_loc ) == t_door_bar_locked ) {
        add_msg( _( "You rattle the bars but the door is locked!" ) );
    }
    return false;
}

bool avatar_action::ramp_move( avatar &you, map &m, const tripoint &dest_loc )
{
    if( dest_loc.z != you.posz() ) {
        // No recursive ramp_moves
        return false;
    }

    // We're moving onto a tile with no support, check if it has a ramp below
    if( !m.has_floor_or_support( dest_loc ) ) {
        tripoint below( dest_loc.x, dest_loc.y, dest_loc.z - 1 );
        if( m.has_flag( TFLAG_RAMP, below ) ) {
            // But we're moving onto one from above
            const tripoint dp = dest_loc - you.pos();
            move( you, m, dp.x, dp.y, -1 );
            // No penalty for misaligned stairs here
            // Also cheaper than climbing up
            return true;
        }

        return false;
    }

    if( !m.has_flag( TFLAG_RAMP, you.pos() ) ||
        m.passable( dest_loc ) ) {
        return false;
    }

    // Try to find an aligned end of the ramp that will make our climb faster
    // Basically, finish walking on the stairs instead of pulling self up by hand
    bool aligned_ramps = false;
    for( const tripoint &pt : m.points_in_radius( you.pos(), 1 ) ) {
        if( rl_dist( pt, dest_loc ) < 2 && m.has_flag( "RAMP_END", pt ) ) {
            aligned_ramps = true;
            break;
        }
    }

    const tripoint above_u( you.posx(), you.posy(), you.posz() + 1 );
    if( m.has_floor_or_support( above_u ) ) {
        add_msg( m_warning, _( "You can't climb here - there's a ceiling above." ) );
        return false;
    }

    const tripoint dp = dest_loc - you.pos();
    const tripoint old_pos = you.pos();
    move( you, m, dp.x, dp.y, 1 );
    // We can't just take the result of the above function here
    if( you.pos() != old_pos ) {
        you.moves -= 50 + ( aligned_ramps ? 0 : 50 );
    }

    return true;
}

void avatar_action::swim( map &m, avatar &you, const tripoint &p )
{
    if( !m.has_flag( "SWIMMABLE", p ) ) {
        dbg( D_ERROR ) << "game:plswim: Tried to swim in "
                       << m.tername( p ) << "!";
        debugmsg( "Tried to swim in %s!", m.tername( p ) );
        return;
    }
    if( you.has_effect( effect_onfire ) ) {
        add_msg( _( "The water puts out the flames!" ) );
        you.remove_effect( effect_onfire );
        if( you.has_effect( effect_riding ) && you.mounted_creature != nullptr ) {
            monster *mon = you.mounted_creature.get();
            if( mon->has_effect( effect_onfire ) ) {
                mon->remove_effect( effect_onfire );
            }
        }
    }
    if( you.has_effect( effect_glowing ) ) {
        add_msg( _( "The water washes off the glowing goo!" ) );
        you.remove_effect( effect_glowing );
    }
    int movecost = you.swim_speed();
    you.practice( skill_id( "swimming" ), you.is_underwater() ? 2 : 1 );
    if( movecost >= 500 ) {
        if( !you.is_underwater() && !( you.shoe_type_count( "swim_fins" ) == 2 ||
                                       ( you.shoe_type_count( "swim_fins" ) == 1 && one_in( 2 ) ) ) ) {
            add_msg( m_bad, _( "You sink like a rock!" ) );
            you.set_underwater( true );
            ///\EFFECT_STR increases breath-holding capacity while sinking
            you.oxygen = 30 + 2 * you.str_cur;
        }
    }
    if( you.oxygen <= 5 && you.is_underwater() ) {
        if( movecost < 500 ) {
            popup( _( "You need to breathe! (%s to surface.)" ), press_x( ACTION_MOVE_UP ) );
        } else {
            popup( _( "You need to breathe but you can't swim!  Get to dry land, quick!" ) );
        }
    }
    bool diagonal = ( p.x != you.posx() && p.y != you.posy() );
    if( you.in_vehicle ) {
        m.unboard_vehicle( you.pos() );
    }
    if( you.has_effect( effect_riding ) &&
        m.veh_at( you.pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        add_msg( m_warning, _( "You cannot board a vehicle while mounted." ) );
        return;
    }
    you.setpos( p );
    g->update_map( you );
    if( m.veh_at( you.pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        m.board_vehicle( you.pos(), &you );
    }
    you.moves -= ( movecost > 200 ? 200 : movecost ) * ( trigdist && diagonal ? 1.41 : 1 );
    you.inv.rust_iron_items();

    if( !you.has_effect( effect_riding ) ) {
        you.burn_move_stamina( movecost );
    }

    body_part_set drenchFlags{ {
            bp_leg_l, bp_leg_r, bp_torso, bp_arm_l,
            bp_arm_r, bp_foot_l, bp_foot_r, bp_hand_l, bp_hand_r
        }
    };

    if( you.is_underwater() ) {
        drenchFlags |= { { bp_head, bp_eyes, bp_mouth, bp_hand_l, bp_hand_r } };
    }
    you.drench( 100, drenchFlags, true );
}

static float rate_critter( const Creature &c )
{
    const npc *np = dynamic_cast<const npc *>( &c );
    if( np != nullptr ) {
        return np->weapon_value( np->weapon );
    }

    const monster *m = dynamic_cast<const monster *>( &c );
    return m->type->difficulty;
}

void avatar_action::autoattack( avatar &you, map &m )
{
    int reach = you.weapon.reach_range( you );
    auto critters = you.get_hostile_creatures( reach );
    if( critters.empty() ) {
        add_msg( m_info, _( "No hostile creature in reach. Waiting a turn." ) );
        if( g->check_safe_mode_allowed() ) {
            you.pause();
        }
        return;
    }

    Creature &best = **std::max_element( critters.begin(), critters.end(),
    []( const Creature * l, const Creature * r ) {
        return rate_critter( *l ) > rate_critter( *r );
    } );

    const tripoint diff = best.pos() - you.pos();
    if( abs( diff.x ) <= 1 && abs( diff.y ) <= 1 && diff.z == 0 ) {
        move( you, m, diff.x, diff.y );
        return;
    }

    you.reach_attack( best.pos() );
}

// TODO: Move data/functions related to targeting out of game class
bool avatar_action::fire_check( avatar &you, const map &m, const targeting_data &args )
{
    // TODO: Make this check not needed
    if( args.relevant == nullptr ) {
        debugmsg( "Can't plfire_check a null" );
        return false;
    }

    if( you.has_effect( effect_relax_gas ) ) {
        if( one_in( 5 ) ) {
            add_msg( m_good, _( "Your eyes steel, and you raise your weapon!" ) );
        } else {
            you.moves -= rng( 2, 5 ) * 10;
            add_msg( m_bad, _( "You can't fire your weapon, it's too heavy..." ) );
            // break a possible loop when aiming
            if( you.activity ) {
                you.cancel_activity();
            }

            return false;
        }
    }

    item &weapon = *args.relevant;
    if( weapon.is_gunmod() ) {
        add_msg( m_info,
                 _( "The %s must be attached to a gun, it can not be fired separately." ),
                 weapon.tname() );
        return false;
    }

    auto gun = weapon.gun_current_mode();
    // check that a valid mode was returned and we are able to use it
    if( !( gun && you.can_use( *gun ) ) ) {
        add_msg( m_info, _( "You can no longer fire." ) );
        return false;
    }

    const optional_vpart_position vp = m.veh_at( you.pos() );
    if( vp && vp->vehicle().player_in_control( you ) && gun->is_two_handed( you ) ) {
        add_msg( m_info, _( "You need a free arm to drive!" ) );
        return false;
    }

    if( !weapon.is_gun() ) {
        // The weapon itself isn't a gun, this weapon is not fireable.
        return false;
    }

    if( weapon.faults.count( fault_gun_clogged ) ) {
        add_msg( m_info, _( "Your %s is too clogged with blackpowder fouling to fire." ), gun->tname() );
        return false;
    }

    if( gun->has_flag( "FIRE_TWOHAND" ) && ( !you.has_two_arms() ||
            you.worn_with_flag( "RESTRICT_HANDS" ) ) ) {
        add_msg( m_info, _( "You need two free hands to fire your %s." ), gun->tname() );
        return false;
    }

    // Skip certain checks if we are directly firing a vehicle turret
    if( args.mode != TARGET_MODE_TURRET_MANUAL ) {
        if( !gun->ammo_sufficient() && !gun->has_flag( "RELOAD_AND_SHOOT" ) ) {
            if( !gun->ammo_remaining() ) {
                add_msg( m_info, _( "You need to reload!" ) );
            } else {
                add_msg( m_info, _( "Your %s needs %i charges to fire!" ),
                         gun->tname(), gun->ammo_required() );
            }
            return false;
        }

        if( gun->get_gun_ups_drain() > 0 ) {
            const int ups_drain = gun->get_gun_ups_drain();
            const int adv_ups_drain = std::max( 1, ups_drain * 3 / 5 );

            if( !( you.has_charges( "UPS_off", ups_drain ) ||
                   you.has_charges( "adv_UPS_off", adv_ups_drain ) ||
                   ( you.has_active_bionic( bionic_id( "bio_ups" ) ) && you.power_level >= ups_drain ) ) ) {
                add_msg( m_info,
                         _( "You need a UPS with at least %d charges or an advanced UPS with at least %d charges to fire that!" ),
                         ups_drain, adv_ups_drain );
                return false;
            }
        }

        if( gun->has_flag( "MOUNTED_GUN" ) ) {
            const bool v_mountable = static_cast<bool>( m.veh_at( you.pos() ).part_with_feature( "MOUNTABLE",
                                     true ) );
            bool t_mountable = m.has_flag_ter_or_furn( "MOUNTABLE", you.pos() );
            if( !t_mountable && !v_mountable ) {
                add_msg( m_info,
                         _( "You must stand near acceptable terrain or furniture to use this weapon. A table, a mound of dirt, a broken window, etc." ) );
                return false;
            }
        }
    }

    return true;
}

bool avatar_action::fire( avatar &you, map &m )
{
    targeting_data args = you.get_targeting_data();
    if( !args.relevant ) {
        // args missing a valid weapon, this shouldn't happen.
        debugmsg( "Player tried to fire a null weapon." );
        return false;
    }
    // If we were wielding this weapon when we started aiming, make sure we still are.
    bool lost_weapon = ( args.held && &you.weapon != args.relevant );
    bool failed_check = !avatar_action::fire_check( you, m, args );
    if( lost_weapon || failed_check ) {
        you.cancel_activity();
        return false;
    }

    int reload_time = 0;
    gun_mode gun = args.relevant->gun_current_mode();

    // bows take more energy to fire than guns.
    you.weapon.is_gun() ? you.increase_activity_level( LIGHT_EXERCISE ) : you.increase_activity_level(
        MODERATE_EXERCISE );

    // TODO: move handling "RELOAD_AND_SHOOT" flagged guns to a separate function.
    if( gun->has_flag( "RELOAD_AND_SHOOT" ) ) {
        if( !gun->ammo_remaining() ) {
            item::reload_option opt = you.ammo_location &&
                                      gun->can_reload_with( you.ammo_location->typeId() ) ? item::reload_option( &you, args.relevant,
                                              args.relevant, you.ammo_location.clone() ) : you.select_ammo( *gun );
            if( !opt ) {
                // Menu canceled
                return false;
            }
            reload_time += opt.moves();
            if( !gun->reload( you, std::move( opt.ammo ), 1 ) ) {
                // Reload not allowed
                return false;
            }

            // Burn 2x the strength required to fire in stamina.
            you.mod_stat( "stamina", gun->get_min_str() * -2 );
            // At low stamina levels, firing starts getting slow.
            int sta_percent = ( 100 * you.stamina ) / you.get_stamina_max();
            reload_time += ( sta_percent < 25 ) ? ( ( 25 - sta_percent ) * 2 ) : 0;

            // Update targeting data to include ammo's range bonus
            args.range = gun.target->gun_range( &you );
            args.ammo = gun->ammo_data();
            you.set_targeting_data( args );

            g->refresh_all();
        }
    }

    g->temp_exit_fullscreen();
    m.draw( g->w_terrain, you.pos() );
    std::vector<tripoint> trajectory = target_handler().target_ui( you, args );

    if( trajectory.empty() ) {
        bool not_aiming = you.activity.id() != activity_id( "ACT_AIM" );
        if( not_aiming && gun->has_flag( "RELOAD_AND_SHOOT" ) ) {
            const auto previous_moves = you.moves;
            g->unload( *gun );
            // Give back time for unloading as essentially nothing has been done.
            // Note that reload_time has not been applied either.
            you.moves = previous_moves;
        }
        g->reenter_fullscreen();
        return false;
    }
    g->draw_ter(); // Recenter our view
    wrefresh( g->w_terrain );
    g->draw_panels();

    int shots = 0;

    you.moves -= reload_time;
    // TODO: add check for TRIGGERHAPPY
    if( args.pre_fire ) {
        args.pre_fire( shots );
    }
    shots = you.fire_gun( trajectory.back(), gun.qty, *gun );
    if( args.post_fire ) {
        args.post_fire( shots );
    }

    if( shots && args.power_cost ) {
        you.charge_power( -args.power_cost * shots );
    }
    g->reenter_fullscreen();
    return shots != 0;
}

bool avatar_action::fire( avatar &you, map &m, item &weapon, int bp_cost )
{
    // TODO: bionic power cost of firing should be derived from a value of the relevant weapon.
    gun_mode gun = weapon.gun_current_mode();
    // gun can be null if the item is an unattached gunmod
    if( !gun ) {
        add_msg( m_info, _( "The %s can't be fired in its current state." ), weapon.tname() );
        return false;
    } else if( weapon.ammo_data() && !weapon.ammo_types().count( weapon.ammo_data()->ammo->type ) ) {
        add_msg( m_info, _( "The %s can't be fired while loaded with incompatible ammunition %s" ),
                 weapon.tname(), weapon.ammo_current() );
        return false;
    }

    targeting_data args = {
        TARGET_MODE_FIRE, &weapon, gun.target->gun_range( &you ),
        bp_cost, &you.weapon == &weapon, gun->ammo_data(),
        target_callback(), target_callback(),
        firing_callback(), firing_callback()
    };
    you.set_targeting_data( args );
    return avatar_action::fire( you, m );
}

void avatar_action::plthrow( avatar &you, int pos,
                             const cata::optional<tripoint> &blind_throw_from_pos )
{
    if( you.has_active_mutation( trait_SHELL2 ) ) {
        add_msg( m_info, _( "You can't effectively throw while you're in your shell." ) );
        return;
    }

    if( pos == INT_MIN ) {
        pos = g->inv_for_all( _( "Throw item" ), _( "You don't have any items to throw." ) );
        g->refresh_all();
    }

    if( pos == INT_MIN ) {
        add_msg( _( "Never mind." ) );
        return;
    }

    item thrown = you.i_at( pos );
    int range = you.throw_range( thrown );
    if( range < 0 ) {
        add_msg( m_info, _( "You don't have that item." ) );
        return;
    } else if( range == 0 ) {
        add_msg( m_info, _( "That is too heavy to throw." ) );
        return;
    }

    if( pos == -1 && thrown.has_flag( "NO_UNWIELD" ) ) {
        // pos == -1 is the weapon, NO_UNWIELD is used for bio_claws_weapon
        add_msg( m_info, _( "That's part of your body, you can't throw that!" ) );
        return;
    }

    if( you.has_effect( effect_relax_gas ) ) {
        if( one_in( 5 ) ) {
            add_msg( m_good, _( "You concentrate mightily, and your body obeys!" ) );
        } else {
            you.moves -= rng( 2, 5 ) * 10;
            add_msg( m_bad, _( "You can't muster up the effort to throw anything..." ) );
            return;
        }
    }
    // if you're wearing the item you need to be able to take it off
    if( pos < -1 ) {
        auto ret = you.can_takeoff( you.i_at( pos ) );
        if( !ret.success() ) {
            add_msg( m_info, "%s", ret.c_str() );
            return;
        }
    }
    // you must wield the item to throw it
    if( pos != -1 ) {
        you.i_rem( pos );
        if( !you.wield( thrown ) ) {
            // We have to remove the item before checking for wield because it
            // can invalidate our pos index.  Which means we have to add it
            // back if the player changed their mind about unwielding their
            // current item
            you.i_add( thrown );
            return;
        }
    }

    // Shift our position to our "peeking" position, so that the UI
    // for picking a throw point lets us target the location we couldn't
    // otherwise see.
    const tripoint original_player_position = you.pos();
    if( blind_throw_from_pos ) {
        you.setpos( *blind_throw_from_pos );
        g->draw_ter();
    }

    g->temp_exit_fullscreen();
    g->m.draw( g->w_terrain, you.pos() );

    const target_mode throwing_target_mode = blind_throw_from_pos ? TARGET_MODE_THROW_BLIND :
            TARGET_MODE_THROW;
    // target_ui() sets x and y, or returns empty vector if we canceled (by pressing Esc)
    std::vector<tripoint> trajectory = target_handler().target_ui( you, throwing_target_mode, &thrown,
                                       range );

    // If we previously shifted our position, put ourselves back now that we've picked our target.
    if( blind_throw_from_pos ) {
        you.setpos( original_player_position );
    }

    if( trajectory.empty() ) {
        return;
    }

    if( thrown.count_by_charges() && thrown.charges > 1 ) {
        you.i_at( -1 ).charges--;
        thrown.charges = 1;
    } else {
        you.i_rem( -1 );
    }
    you.throw_item( trajectory.back(), thrown, blind_throw_from_pos );
    g->reenter_fullscreen();
}

// Used to set up the first Hotkey in the display set
static int get_initial_hotkey( const size_t menu_index )
{
    int hotkey = -1;
    if( menu_index == 0 ) {
        const int butcher_key = inp_mngr.get_previously_pressed_key();
        if( butcher_key != 0 ) {
            hotkey = butcher_key;
        }
    }
    return hotkey;
}

// Corpses are always individual items
// Just add them individually to the menu
static void add_corpses( uilist &menu, map_stack &items,
                         const std::vector<int> &indices, size_t &menu_index )
{
    int hotkey = get_initial_hotkey( menu_index );

    for( const auto index : indices ) {
        const item &it = items[index];
        menu.addentry( menu_index++, true, hotkey, it.get_mtype()->nname() );
        hotkey = -1;
    }
}

// Returns a vector of pairs.
//    Pair.first is the first items index with a unique tname.
//    Pair.second is the number of equivalent items per unique tname
// There are options for optimization here, but the function is hit infrequently
// enough that optimizing now is not a useful time expenditure.
static const std::vector<std::pair<int, int>> generate_butcher_stack_display(
            map_stack &items, const std::vector<int> &indices )
{
    std::vector<std::pair<int, int>> result;
    std::vector<std::string> result_strings;
    result.reserve( indices.size() );
    result_strings.reserve( indices.size() );

    for( const size_t ndx : indices ) {
        const item &it = items[ndx];

        const std::string tname = it.tname();
        size_t s = 0;
        // Search for the index with a string equivalent to tname
        for( ; s < result_strings.size(); ++s ) {
            if( result_strings[s] == tname ) {
                break;
            }
        }
        // If none is found, this is a unique tname so we need to add
        // the tname to string vector, and make an empty result pair.
        // Has the side effect of making 's' a valid index
        if( s == result_strings.size() ) {
            // make a new entry
            result.push_back( std::make_pair<int, int>( static_cast<int>( ndx ), 0 ) );
            // Also push new entry string
            result_strings.push_back( tname );
        }
        // Increase count result pair at index s
        ++result[s].second;
    }

    return result;
}

// Disassemblables stack so we need to pass in a stack vector rather than an item index vector
static void add_disassemblables( uilist &menu, map_stack &items,
                                 const std::vector<std::pair<int, int>> &stacks, size_t &menu_index )
{
    if( !stacks.empty() ) {
        int hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = items[stack.first];

            //~ Name, number of items and time to complete disassembling
            const auto &msg = string_format( pgettext( "butchery menu", "%s (%d)" ),
                                             it.tname(), stack.second );
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( time_duration::from_turns( recipe_dictionary::get_uncraft(
                                       it.typeId() ).time / 100 ) ) );
            hotkey = -1;
        }
    }
}

// Salvagables stack so we need to pass in a stack vector rather than an item index vector
static void add_salvagables( uilist &menu, map_stack &items,
                             const std::vector<std::pair<int, int>> &stacks, size_t &menu_index,
                             const salvage_actor &salvage_iuse )
{
    if( !stacks.empty() ) {
        int hotkey = get_initial_hotkey( menu_index );

        for( const auto &stack : stacks ) {
            const item &it = items[stack.first];

            //~ Name and number of items listed for cutting up
            const auto &msg = string_format( pgettext( "butchery menu", "Cut up %s (%d)" ),
                                             it.tname(), stack.second );
            menu.addentry_col( menu_index++, true, hotkey, msg,
                               to_string_clipped( time_duration::from_turns( salvage_iuse.time_to_cut_up( it ) / 100 ) ) );
            hotkey = -1;
        }
    }
}

// Butchery sub-menu and time calculation
static void butcher_submenu( map_stack &items, const std::vector<int> &corpses, int corpse = -1 )
{
    auto cut_time = [&]( enum butcher_type bt ) {
        int time_to_cut = 0;
        if( corpse != -1 ) {
            time_to_cut = butcher_time_to_cut( g->u, items[corpses[corpse]], bt );
        } else {
            for( int i : corpses ) {
                time_to_cut += butcher_time_to_cut( g->u, items[i], bt );
            }
        }
        return to_string_clipped( time_duration::from_turns( time_to_cut / 100 ) );
    };
    const bool enough_light = g->u.fine_detail_vision_mod() <= 4;
    if( !enough_light ) {
        popup( _( "Some types of butchery are not possible when it is dark." ) );
    }
    uilist smenu;
    smenu.desc_enabled = true;
    smenu.text = _( "Choose type of butchery:" );

    smenu.addentry_col( BUTCHER, enough_light, 'B', _( "Quick butchery" ), cut_time( BUTCHER ),
                        _( "This technique is used when you are in a hurry, but still want to harvest something from the corpse.  Yields are lower as you don't try to be precise, but it's useful if you don't want to set up a workshop.  Prevents zombies from raising." ) );
    smenu.addentry_col( BUTCHER_FULL, enough_light, 'b', _( "Full butchery" ),
                        cut_time( BUTCHER_FULL ),
                        _( "This technique is used to properly butcher a corpse, and requires a rope & a tree or a butchering rack, a flat surface (for ex. a table, a leather tarp, etc.) and good tools.  Yields are plentiful and varied, but it is time consuming." ) );
    smenu.addentry_col( F_DRESS, enough_light, 'f', _( "Field dress corpse" ),
                        cut_time( F_DRESS ),
                        _( "Technique that involves removing internal organs and viscera to protect the corpse from rotting from inside. Yields internal organs. Carcass will be lighter and will stay fresh longer.  Can be combined with other methods for better effects." ) );
    smenu.addentry_col( SKIN, enough_light, 's', ( "Skin corpse" ), cut_time( SKIN ),
                        _( "Skinning a corpse is an involved and careful process that usually takes some time.  You need skill and an appropriately sharp and precise knife to do a good job.  Some corpses are too small to yield a full-sized hide and will instead produce scraps that can be used in other ways." ) );
    smenu.addentry_col( QUARTER, enough_light, 'k', _( "Quarter corpse" ), cut_time( QUARTER ),
                        _( "By quartering a previously field dressed corpse you will acquire four parts with reduced weight and volume.  It may help in transporting large game.  This action destroys skin, hide, pelt, etc., so don't use it if you want to harvest them later." ) );
    smenu.addentry_col( DISMEMBER, true, 'm', _( "Dismember corpse" ), cut_time( DISMEMBER ),
                        _( "If you're aiming to just destroy a body outright, and don't care about harvesting it, dismembering it will hack it apart in a very short amount of time, but yield little to no usable flesh." ) );
    smenu.addentry_col( DISSECT, enough_light, 'd', _( "Dissect corpse" ),
                        cut_time( DISSECT ),
                        _( "By careful dissection of the corpse, you will examine it for possible bionic implants, or discrete organs and harvest them if possible.  Requires scalpel-grade cutting tools, ruins corpse, and consumes a lot of time.  Your medical knowledge is most useful here." ) );
    smenu.query();
    switch( smenu.ret ) {
        case BUTCHER:
            g->u.assign_activity( activity_id( "ACT_BUTCHER" ), 0, -1 );
            break;
        case BUTCHER_FULL:
            g->u.assign_activity( activity_id( "ACT_BUTCHER_FULL" ), 0, -1 );
            break;
        case F_DRESS:
            g->u.assign_activity( activity_id( "ACT_FIELD_DRESS" ), 0, -1 );
            break;
        case SKIN:
            g->u.assign_activity( activity_id( "ACT_SKIN" ), 0, -1 );
            break;
        case QUARTER:
            g->u.assign_activity( activity_id( "ACT_QUARTER" ), 0, -1 );
            break;
        case DISMEMBER:
            g->u.assign_activity( activity_id( "ACT_DISMEMBER" ), 0, -1 );
            break;
        case DISSECT:
            g->u.assign_activity( activity_id( "ACT_DISSECT" ), 0, -1 );
            break;
        default:
            return;
    }
}

void game::butcher()
{
    static const std::string salvage_string = "salvage";
    if( u.controlling_vehicle ) {
        add_msg( m_info, _( "You can't butcher while driving!" ) );
        return;
    }

    const int factor = u.max_quality( quality_id( "BUTCHER" ) );
    const int factorD = u.max_quality( quality_id( "CUT_FINE" ) );
    static const char *no_knife_msg = _( "You don't have a butchering tool." );
    static const char *no_corpse_msg = _( "There are no corpses here to butcher." );

    //You can't butcher on sealed terrain- you have to smash/shovel/etc it open first
    if( m.has_flag( "SEALED", u.pos() ) ) {
        if( m.sees_some_items( u.pos(), u ) ) {
            add_msg( m_info, _( "You can't access the items here." ) );
        } else if( factor > INT_MIN || factorD > INT_MIN ) {
            add_msg( m_info, no_corpse_msg );
        } else {
            add_msg( m_info, no_knife_msg );
        }
        return;
    }

    const item *first_item_without_tools = nullptr;
    // Indices of relevant items
    std::vector<int> corpses;
    std::vector<int> disassembles;
    std::vector<int> salvageables;
    auto items = m.i_at( u.pos() );
    const inventory &crafting_inv = u.crafting_inventory();

    // TODO: Properly handle different material whitelists
    // TODO: Improve quality of this section
    auto salvage_filter = []( item it ) {
        const auto usable = it.get_usable_item( salvage_string );
        return usable != nullptr;
    };

    std::vector< item * > salvage_tools = u.items_with( salvage_filter );
    int salvage_tool_index = INT_MIN;
    item *salvage_tool = nullptr;
    const salvage_actor *salvage_iuse = nullptr;
    if( !salvage_tools.empty() ) {
        salvage_tool = salvage_tools.front();
        salvage_tool_index = u.get_item_position( salvage_tool );
        item *usable = salvage_tool->get_usable_item( salvage_string );
        salvage_iuse = dynamic_cast<const salvage_actor *>(
                           usable->get_use( salvage_string )->get_actor_ptr() );
    }

    // Reserve capacity for each to hold entire item set if necessary to prevent
    // reallocations later on
    corpses.reserve( items.size() );
    salvageables.reserve( items.size() );
    disassembles.reserve( items.size() );

    // Split into corpses, disassemble-able, and salvageable items
    // It's not much additional work to just generate a corpse list and
    // clear it later, but does make the splitting process nicer.
    for( size_t i = 0; i < items.size(); ++i ) {
        if( items[i].is_corpse() ) {
            corpses.push_back( i );
        } else {
            if( ( salvage_tool_index != INT_MIN ) && salvage_iuse->valid_to_cut_up( items[i] ) ) {
                salvageables.push_back( i );
            }
            if( u.can_disassemble( items[i], crafting_inv ).success() ) {
                disassembles.push_back( i );
            } else if( first_item_without_tools == nullptr ) {
                first_item_without_tools = &items[i];
            }
        }
    }

    // Clear corpses if butcher and dissect factors are INT_MIN
    if( factor == INT_MIN && factorD == INT_MIN ) {
        corpses.clear();
    }

    if( corpses.empty() && disassembles.empty() && salvageables.empty() ) {
        if( factor > INT_MIN || factorD > INT_MIN ) {
            add_msg( m_info, no_corpse_msg );
        } else {
            add_msg( m_info, no_knife_msg );
        }

        if( first_item_without_tools != nullptr ) {
            add_msg( m_info, _( "You don't have the necessary tools to disassemble any items here." ) );
            // Just for the "You need x to disassemble y" messages
            const auto ret = u.can_disassemble( *first_item_without_tools, crafting_inv );
            if( !ret.success() ) {
                add_msg( m_info, "%s", ret.c_str() );
            }
        }
        return;
    }

    Creature *hostile_critter = is_hostile_very_close();
    if( hostile_critter != nullptr ) {
        if( !query_yn( _( "You see %s nearby! Start butchering anyway?" ),
                       hostile_critter->disp_name() ) ) {
            return;
        }
    }

    // Magic indices for special butcher options
    enum : int {
        MULTISALVAGE = MAX_ITEM_IN_SQUARE + 1,
        MULTIBUTCHER,
        MULTIDISASSEMBLE_ONE,
        MULTIDISASSEMBLE_ALL,
        NUM_BUTCHER_ACTIONS
    };
    // What are we butchering (ie. which vector to pick indices from)
    enum {
        BUTCHER_CORPSE,
        BUTCHER_DISASSEMBLE,
        BUTCHER_SALVAGE,
        BUTCHER_OTHER // For multisalvage etc.
    } butcher_select = BUTCHER_CORPSE;
    // Index to std::vector of indices...
    int indexer_index = 0;

    // Generate the indexed stacks so we can display them nicely
    const auto disassembly_stacks = generate_butcher_stack_display( items, disassembles );
    const auto salvage_stacks = generate_butcher_stack_display( items, salvageables );
    // Always ask before cutting up/disassembly, but not before butchery
    size_t ret = 0;
    if( corpses.size() > 1 || !disassembles.empty() || !salvageables.empty() ) {
        uilist kmenu;
        kmenu.text = _( "Choose corpse to butcher / item to disassemble" );

        size_t i = 0;
        // Add corpses, disassembleables, and salvagables to the UI
        add_corpses( kmenu, items, corpses, i );
        add_disassemblables( kmenu, items, disassembly_stacks, i );
        if( !salvageables.empty() ) {
            assert( salvage_iuse ); // To appease static analysis
            add_salvagables( kmenu, items, salvage_stacks, i, *salvage_iuse );
        }

        if( corpses.size() > 1 ) {
            kmenu.addentry( MULTIBUTCHER, true, 'b', _( "Butcher everything" ) );
        }
        if( disassembles.size() > 1 ) {
            int time_to_disassemble = 0;
            int time_to_disassemble_all = 0;
            for( const auto &stack : disassembly_stacks ) {
                const item &it = items[stack.first];
                const int time = recipe_dictionary::get_uncraft( it.typeId() ).time;
                time_to_disassemble += time;
                time_to_disassemble_all += time * stack.second;
            }

            kmenu.addentry_col( MULTIDISASSEMBLE_ONE, true, 'D', _( "Disassemble everything once" ),
                                to_string_clipped( time_duration::from_turns( time_to_disassemble / 100 ) ) );
            kmenu.addentry_col( MULTIDISASSEMBLE_ALL, true, 'd', _( "Disassemble everything" ),
                                to_string_clipped( time_duration::from_turns( time_to_disassemble_all / 100 ) ) );
        }
        if( salvageables.size() > 1 ) {
            assert( salvage_iuse ); // To appease static analysis
            int time_to_salvage = 0;
            for( const auto &stack : salvage_stacks ) {
                const item &it = items[stack.first];
                time_to_salvage += salvage_iuse->time_to_cut_up( it ) * stack.second;
            }

            kmenu.addentry_col( MULTISALVAGE, true, 'z', _( "Cut up everything" ),
                                to_string_clipped( time_duration::from_turns( time_to_salvage / 100 ) ) );
        }

        kmenu.query();

        if( kmenu.ret < 0 || kmenu.ret >= NUM_BUTCHER_ACTIONS ) {
            return;
        }

        ret = static_cast<size_t>( kmenu.ret );
        if( ret >= MULTISALVAGE && ret < NUM_BUTCHER_ACTIONS ) {
            butcher_select = BUTCHER_OTHER;
            indexer_index = ret;
        } else if( ret < corpses.size() ) {
            butcher_select = BUTCHER_CORPSE;
            indexer_index = ret;
        } else if( ret < corpses.size() + disassembly_stacks.size() ) {
            butcher_select = BUTCHER_DISASSEMBLE;
            indexer_index = ret - corpses.size();
        } else if( ret < corpses.size() + disassembly_stacks.size() + salvage_stacks.size() ) {
            butcher_select = BUTCHER_SALVAGE;
            indexer_index = ret - corpses.size() - disassembly_stacks.size();
        } else {
            debugmsg( "Invalid butchery index: %d", ret );
            return;
        }
    }

    if( !u.has_morale_to_craft() ) {
        if( butcher_select == BUTCHER_CORPSE || indexer_index == MULTIBUTCHER ) {
            add_msg( m_info,
                     _( "You are not in the mood and the prospect of guts and blood on your hands convinces you to turn away." ) );
        } else {
            add_msg( m_info,
                     _( "You are not in the mood and the prospect of work stops you before you begin." ) );
        }
        return;
    }
    const auto helpers = u.get_crafting_helpers();
    for( const npc *np : helpers ) {
        add_msg( m_info, _( "%s helps with this task..." ), np->name );
        break;
    }
    switch( butcher_select ) {
        case BUTCHER_OTHER:
            switch( indexer_index ) {
                case MULTISALVAGE:
                    u.assign_activity( activity_id( "ACT_LONGSALVAGE" ), 0, salvage_tool_index );
                    break;
                case MULTIBUTCHER:
                    butcher_submenu( items, corpses );
                    for( int i : corpses ) {
                        u.activity.values.push_back( i );
                    }
                    break;
                case MULTIDISASSEMBLE_ONE:
                    u.disassemble_all( true );
                    break;
                case MULTIDISASSEMBLE_ALL:
                    u.disassemble_all( false );
                    break;
                default:
                    debugmsg( "Invalid butchery type: %d", indexer_index );
                    return;
            }
            break;
        case BUTCHER_CORPSE: {
            butcher_submenu( items, corpses, indexer_index );
            draw_ter();
            wrefresh( w_terrain );
            draw_panels( true );
            u.activity.values.push_back( corpses[indexer_index] );
        }
        break;
        case BUTCHER_DISASSEMBLE: {
            // Pick index of first item in the disassembly stack
            size_t index = disassembly_stacks[indexer_index].first;
            u.disassemble( items[index], index, true );
        }
        break;
        case BUTCHER_SALVAGE: {
            // Pick index of first item in the salvage stack
            size_t index = salvage_stacks[indexer_index].first;
            item_location item_loc( map_cursor( u.pos() ), &items[index] );
            salvage_iuse->cut_up( u, *salvage_tool, item_loc );
        }
        break;
    }
}
