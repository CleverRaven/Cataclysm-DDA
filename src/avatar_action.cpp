#include "avatar_action.h"

#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "action.h"
#include "avatar.h"
#include "creature.h"
#include "game.h"
#include "input.h"
#include "item.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "map_iterator.h"
#include "messages.h"
#include "monster.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "projectile.h"
#include "ranged.h"
#include "translations.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "bodypart.h"
#include "cursesdef.h"
#include "debug.h"
#include "enums.h"
#include "game_constants.h"
#include "gun_mode.h"
#include "int_id.h"
#include "inventory.h"
#include "item_location.h"
#include "mtype.h"
#include "player_activity.h"
#include "ret_val.h"
#include "rng.h"

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

static const trait_id trait_BURROW( "BURROW" );
static const trait_id trait_SHELL2( "SHELL2" );

static const efftype_id effect_amigara( "amigara" );
static const efftype_id effect_glowing( "glowing" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_relax_gas( "relax_gas" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_harnessed( "harnessed" );

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
        !you.is_mounted() ) {
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
    if( you.movement_mode_is( PMM_WALK ) ) {
        you.increase_activity_level( LIGHT_EXERCISE );
    } else if( you.movement_mode_is( PMM_CROUCH ) ) {
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
            if( you.is_mounted() ) {
                you.mounted_creature->facing = FD_RIGHT;
            }
        } else if( new_dx < 0 ) {
            you.facing = FD_LEFT;
            if( you.is_mounted() ) {
                you.mounted_creature->facing = FD_LEFT;
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
            if( you.is_mounted() ) {
                auto mons = you.mounted_creature.get();
                mons->facing = FD_RIGHT;
            }
        }
        if( new_dy <= 0 && new_dx <= 0 ) {
            you.facing = FD_LEFT;
            if( you.is_mounted() ) {
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
        } else if( critter.has_flag( MF_IMMOBILE ) || critter.has_effect( effect_harnessed ) ||
                   critter.has_effect( effect_ridden ) ) {
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
        if( you.is_mounted() ) {
            auto mon = you.mounted_creature.get();
            if( !mon->has_flag( MF_SWIMS ) || mon->get_size() < you.get_size() + 2 ) {
                add_msg( m_warning, _( "The %s cannot swim while it is carrying you!" ), mon->get_name() );
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
        && you.movement_mode_is( PMM_WALK )
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
        if( !veh1->handle_potential_theft( dynamic_cast<player &>( you ) ) ) {
            return true;
        } else {
            if( outside_vehicle ) {
                veh1->open_all_at( dpart );
            } else {
                veh1->open( dpart );
                add_msg( _( "You open the %1$s's %2$s." ), veh1->name,
                         veh1->part_info( dpart ).name() );
            }
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
        tripoint below( dest_loc.xy(), dest_loc.z - 1 );
        if( m.has_flag( TFLAG_RAMP, below ) ) {
            // But we're moving onto one from above
            const tripoint dp = dest_loc - you.pos();
            move( you, m, tripoint( dp.xy(), -1 ) );
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
    move( you, m, tripoint( dp.xy(), 1 ) );
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
        if( you.is_mounted() ) {
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
    if( you.is_mounted() && m.veh_at( you.pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        add_msg( m_warning, _( "You cannot board a vehicle while mounted." ) );
        return;
    }
    if( const auto vp = m.veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        if( !vp->vehicle().handle_potential_theft( dynamic_cast<player &>( you ) ) ) {
            return;
        }
    }
    you.setpos( p );
    g->update_map( you );
    if( m.veh_at( you.pos() ).part_with_feature( VPFLAG_BOARDABLE, true ) ) {
        m.board_vehicle( you.pos(), &you );
    }
    you.moves -= ( movecost > 200 ? 200 : movecost ) * ( trigdist && diagonal ? 1.41 : 1 );
    you.inv.rust_iron_items();

    if( !you.is_mounted() ) {
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
        move( you, m, tripoint( diff.xy(), 0 ) );
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
            bool is_mech_weapon = false;
            if( you.is_mounted() ) {
                auto mons = g->u.mounted_creature.get();
                if( !mons->type->mech_weapon.empty() ) {
                    is_mech_weapon = true;
                }
            }
            if( !is_mech_weapon ) {
                if( !( you.has_charges( "UPS_off", ups_drain ) ||
                       you.has_charges( "adv_UPS_off", adv_ups_drain ) ||
                       ( you.has_active_bionic( bionic_id( "bio_ups" ) ) &&
                         you.power_level >= units::from_kilojoule( ups_drain ) ) ) ) {
                    add_msg( m_info,
                             _( "You need a UPS with at least %d charges or an advanced UPS with at least %d charges to fire that!" ),
                             ups_drain, adv_ups_drain );
                    return false;
                }
            } else {
                if( !you.has_charges( "UPS", ups_drain ) ) {
                    add_msg( m_info, _( "Your mech has an empty battery, its weapon will not fire." ) );
                    return false;
                }
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
            const auto ammo_location_is_valid = [&]() -> bool {
                if( !you.ammo_location )
                {
                    return false;
                }
                if( !gun->can_reload_with( you.ammo_location->typeId() ) )
                {
                    return false;
                }
                if( square_dist( you.pos(), you.ammo_location.position() ) > 1 )
                {
                    return false;
                }
                return true;
            };
            item::reload_option opt = ammo_location_is_valid() ? item::reload_option( &you, args.relevant,
                                      args.relevant, you.ammo_location ) : you.select_ammo( *gun );
            if( !opt ) {
                // Menu canceled
                return false;
            }
            reload_time += opt.moves();
            if( !gun->reload( you, std::move( opt.ammo ), 1 ) ) {
                // Reload not allowed
                return false;
            }

            // Burn 0.2% max base stamina x the strength required to fire.
            you.mod_stat( "stamina", gun->get_min_str() * static_cast<int>( 0.002f *
                          get_option<int>( "PLAYER_MAX_STAMINA" ) ) );
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

    //may be changed in target_ui
    gun = args.relevant->gun_current_mode();

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
        you.charge_power( units::from_kilojoule( -args.power_cost ) * shots );
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
    if( you.is_mounted() ) {
        auto mons = g->u.mounted_creature.get();
        if( mons->has_flag( MF_RIDEABLE_MECH ) ) {
            if( !mons->check_mech_powered() ) {
                add_msg( m_bad, _( "Your %s refuses to move as its batteries have been drained." ),
                         mons->get_name() );
                return;
            }
        }
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
