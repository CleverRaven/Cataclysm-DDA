#include "mondefense.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ballistics.h"
#include "bodypart.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "dispersion.h"
#include "enums.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "mattack_actors.h"
#include "mattack_common.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "projectile.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "type_id.h"
#include "viewer.h"

static const ammo_effect_str_id ammo_effect_DRAW_AS_LINE( "DRAW_AS_LINE" );
static const ammo_effect_str_id ammo_effect_NO_DAMAGE_SCALING( "NO_DAMAGE_SCALING" );

static const damage_type_id damage_acid( "acid" );
static const damage_type_id damage_electric( "electric" );

static const gun_mode_id gun_mode_DEFAULT( "DEFAULT" );

void mdefense::none( monster &, Creature *, const dealt_projectile_attack * )
{
}

void mdefense::zapback( monster &m, Creature *const source,
                        dealt_projectile_attack const *proj )
{
    map &here = get_map();

    if( source == nullptr ) {
        return;
    }
    // If we have a projectile, we're a ranged attack, no zapback.
    if( proj != nullptr ) {
        return;
    }

    if( const Character *const foe = dynamic_cast<Character *>( source ) ) {
        // Players/NPCs can avoid the shock if they wear non-conductive gear on their hands
        if( !foe->worn.hands_conductive() ) {
            return;
        }
        // Players/NPCs can avoid the shock by using non-conductive weapons
        if( foe->get_wielded_item() && !foe->get_wielded_item()->conductive() ) {
            if( foe->reach_attacking ) {
                return;
            }
            if( foe->used_weapon() ) {
                return;
            }
        }
    }

    if( source->is_elec_immune() ) {
        return;
    }

    if( get_player_view().sees( here, source->pos_bub( here ) ) ) {
        const game_message_type msg_type = source->is_avatar() ? m_bad : m_info;
        add_msg( msg_type, _( "Striking the %1$s shocks %2$s!" ),
                 m.name(), source->disp_name() );
    }

    const damage_instance shock {
        damage_electric, static_cast<float>( rng( 1, 5 ) )
    };
    source->deal_damage( &m, bodypart_id( "arm_l" ), shock );
    source->deal_damage( &m, bodypart_id( "arm_r" ), shock );

    source->check_dead_state( &here );
}

void mdefense::acidsplash( monster &m, Creature *const source,
                           dealt_projectile_attack const *const proj )
{
    const map &here = get_map();

    if( source == nullptr ) {
        return;
    }
    size_t num_drops = rng( 4, 6 );
    // Would be useful to have the attack data here, for cutting vs. bashing etc.
    if( proj ) {
        // Projectile didn't penetrate the target, no acid will splash out of it.
        if( proj->dealt_dam.total_damage() <= 0 ) {
            return;
        }
        // Less likely for a projectile to deliver enough force
        if( !one_in( 3 ) ) {
            return;
        }
    } else {
        if( const Character *const foe = dynamic_cast<Character *>( source ) ) {
            const item_location weapon = foe->get_wielded_item();
            if( weapon && weapon->has_edged_damage() ) {
                num_drops += rng( 3, 4 );
            }
            if( foe->unarmed_attack() ) {
                const damage_instance acid_burn{
                    damage_acid, static_cast<float>( rng( 1, 5 ) )
                };
                source->deal_damage( &m, one_in( 2 ) ? bodypart_id( "hand_l" ) : bodypart_id( "hand_r" ),
                                     acid_burn );
                source->add_msg_if_player( m_bad, _( "Acid covering %s burns your hand!" ), m.disp_name() );
            }
        }
    }

    // Don't splatter directly on the `m`, that doesn't work well
    std::vector<tripoint_bub_ms> pts = closest_points_first( source->pos_bub( here ), 1 );
    pts.erase( std::remove( pts.begin(), pts.end(), m.pos_bub() ), pts.end() );

    projectile prj;
    prj.speed = 10;
    prj.range = 4;
    prj.proj_effects.insert( ammo_effect_DRAW_AS_LINE );
    prj.proj_effects.insert( ammo_effect_NO_DAMAGE_SCALING );
    prj.impact.add_damage( damage_acid, rng( 1, 3 ) );
    dealt_projectile_attack atk;
    for( size_t i = 0; i < num_drops; i++ ) {
        const tripoint_bub_ms &target = random_entry( pts );
        projectile_attack( atk, prj, m.pos_bub( here ), target, dispersion_sources{ 1200 }, &m );
    }

    if( get_player_view().sees( here, m.pos_bub( here ) ) ) {
        add_msg( m_warning, _( "Acid sprays out of %s as it is hit!" ), m.disp_name() );
    }
}

void mdefense::return_fire( monster &m, Creature *source, const dealt_projectile_attack *proj )
{
    const map &here = get_map();

    // No return fire for untargeted projectiles, i.e. from explosions.
    if( source == nullptr ) {
        return;
    }
    // No return fire for melee attacks.
    if( proj == nullptr ) {
        return;
    }
    // No return fire from dead monsters.
    if( m.is_dead_state() ) {
        return;
    }

    const Character *const foe = dynamic_cast<Character *>( source );
    // No return fire for quiet or completely silent projectiles (bows, throwing etc).
    if( foe == nullptr || !foe->get_wielded_item() ||
        foe->get_wielded_item()->gun_noise().volume < rl_dist( m.pos_bub(), source->pos_bub() ) ) {
        return;
    }

    const tripoint_bub_ms fire_point = source->pos_bub();
    // If target actually was not damaged by projectile - then do not bother
    // Also it covers potential exploit - peek throwing potentially can be used to exhaust turret ammo
    if( proj != nullptr && proj->dealt_dam.total_damage() == 0 ) {
        return;
    }

    // No return fire if attacker is seen
    if( m.sees( here, *source ) ) {
        return;
    }

    const int distance_to_source = rl_dist( m.pos_bub(), source->pos_bub() );

    // TODO: implement different rule, dependent on sound and probably some other things
    // Add some inaccuracy since it is blind fire (at a tile, not the player directly)
    const int dispersion = 150;

    for( const std::pair<const std::string, mtype_special_attack> &attack : m.type->special_attacks ) {
        if( attack.second->id == "gun" ) {
            sounds::sound( m.pos_bub(), 50, sounds::sound_t::alert,
                           _( "Detected shots from unseen attacker, return fire mode engaged." ) );
            const gun_actor *gunactor = dynamic_cast<const gun_actor *>( attack.second.get() );
            if( gunactor->get_max_range() < distance_to_source ) {
                continue;
            }

            if( gunactor->shoot( m, fire_point, gun_mode_DEFAULT, dispersion ) ) {
                // We only return fire once with one gun.
                return;
            }
        }
    }
}
